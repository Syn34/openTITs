// Render layer draw and sort internals: DrawItem_Draw, RenderLayer_End,
// RenderLayer_RadixSort and SortEntries.
#include "layer2.h"

using namespace sly4;

namespace {

namespace rl = sly4::render::layer;
namespace l2 = sly4::render::layer2;
namespace rg = sly4::render::g;

constexpr u32 kStencilOp = 0x000C033C, kStencilMask = 0x0004032C, kStencilFunc = 0x000C0330;
constexpr u32 kKeep = 0x1E00, kAlways = 0x0207;

// Ends the bound shader (slot 16 with 0), OPD / entry left in r5 / r6.
void end_shader(PPUContext& ctx, u32 shader) { fx::vcall(ctx, shader, l2::kShaderEnd, {shader, 0}); }

}  // namespace

// void DrawItem_Draw(DrawItem* item)   (name?)
// Picks the shader for the item's material (the material's +16 object,
// slot 16; the current view's +664 overrides it when set). A new shader
// ends the old one and clears the binding; a new material ends the shader
// and binds the material to it (Material_Bind). Then shader slot 12 draws
// the item. Returns what the draw returns.
DECOMP(00512C30, DrawItem_Draw) {
    const u32 item = arg<u32>(ctx, 0);
    const u32 mat = load<u32>(item + 28);
    const u32 picker = load<u32>(mat + 16);
    u32 shader = static_cast<u32>(fx::vcall(ctx, picker, l2::kPickShader, {picker, mat, item}));
    const u32 view = load<u32>(load<u32>(draw::g::current_surface) + 672);
    if (const u32 forced = load<u32>(view + 664); forced != 0) shader = forced;

    const u32 b = l2::g::binding;
    if (const u32 cur = load<u32>(b + 4); cur != shader) {
        if (cur != 0) end_shader(ctx, cur);
        store<u32>(b + 4, shader);
        store<u32>(b, 0);
        store<u32>(b + 8, 0);
    }
    if (const u32 bound = load<u32>(b); bound != mat) {
        // The original ends the *new* shader here (r3 still holds it).
        if (bound != 0) end_shader(ctx, shader);
        store<u32>(b, mat);
        call(ctx, l2::fn::Material_Bind, {mat, shader});
    }
    ctx.gpr[3] = fx::vcall(ctx, shader, l2::kShaderDraw, {shader, item});
}

// void RenderLayer_End()   (name?)
// Ends the bound shader, clears the binding and turns the stencil test
// off (ops KEEP / KEEP / KEEP, mask 0, func ALWAYS 0 0, cached at render
// state +160..+184), clearing render::g::stencil_bytes.
DECOMP(00512DA0, RenderLayer_End) {
    const u32 b = l2::g::binding;
    if (const u32 shader = load<u32>(b + 4); shader != 0) end_shader(ctx, shader);
    store<u32>(b, 0);
    store<u32>(b + 4, 0);
    store<u32>(b + 8, 0);
    const u32 st = fx::g::state;
    fx::emit({fx::kStencilTestEnable, 0});
    store<u32>(st + 160, kAlways);
    store<u32>(st + 164, 0);
    store<u32>(st + 172, 0);
    store<u32>(st + 176, kKeep);
    store<u32>(st + 180, kKeep);
    store<u32>(st + 184, kKeep);
    store<u32>(st + 168, 0);
    fx::emit({kStencilOp, kKeep, kKeep, kKeep});
    fx::emit({kStencilMask, 0});
    fx::emit({kStencilFunc, load<u32>(st + 160), load<u32>(st + 164), load<u32>(st + 172)});
    ref<u8>(rg::stencil_bytes) = 0;
    ref<u8>(rg::stencil_bytes + 1) = 0;
    ctx.gpr[3] = rg::stencil_bytes - 736;  // 0x010DD780, left in r3
    ctx.gpr[4] = 0;
}

// void RenderLayer_RadixSort(RenderLayer* l)   (name?)
// Sorts the layer's 8-byte entries by their u32 key, one byte per pass
// from the lowest, through a 4200-entry buffer on the stack (whose keys
// are zeroed first). Quirks kept:
//  - each pass scatters the entries from the top down (the last count & 3
//    one by one, then groups of four, each group in ascending order), so
//    the passes aren't stable and only the last (top) byte is sure to
//    end up ordered;
//  - the count loops are do-whiles: fewer than 4 entries would run far
//    past the list (RenderLayer_Sort only calls this above 512);
//  - nothing checks the 4200-entry limit;
//  - after the four passes the result is back in the layer, so the final
//    memmove never runs.
DECOMP(00513394, RenderLayer_RadixSort) {
    const u32 l = arg<u32>(ctx, 0);
    Stack frame(ctx, 0x8A10 - 112);  // the original's 35344-byte frame
    const u32 sp = frame.addr() - 112;
    const u32 hist = sp + 112, tmp = sp + 624, offs = sp + 0x85B0;  // u16[256], Entry[4200], u32[256]
    const u32 base = load<u32>(l + rl::kBase);
    const u32 count = static_cast<u32>(static_cast<s32>(load<u32>(l + rl::kCursor) - base) / 8);  // srawi + addze
    for (u32 i = 0; i < 4200; ++i) store<u32>(tmp + 8 * i, 0);

    u32 src = base, dst = tmp;
    for (s32 pass = 0; pass < 4; ++pass) {
        call(ctx, sly4::fn::memset, {hist, 0, 512});
        const u32 byte = static_cast<u32>(3 - pass);
        const auto bucket = [&](u32 k) -> u32 { return ref<u8>(src + 8 * k + byte); };
        const auto count_one = [&](u32 k) {
            const u32 h = hist + 2 * bucket(k);
            store<u16>(h, static_cast<u16>(load<u16>(h) + 1));
        };
        const auto put = [&](u32 k) {
            const u32 o = offs + 4 * bucket(k);
            const u32 slot = load<u32>(o);
            store<u64>(dst + (slot << 3), load<u64>(src + 8 * k));
            store<u32>(o, slot + 1);
        };
        const u32 rem = count & 3;

        u32 n = count;
        if (rem != 0) {
            do count_one(--n);
            while ((n & 3) != 0);
        }
        do {
            n -= 4;
            count_one(n);
            count_one(n + 1);
            count_one(n + 2);
            count_one(n + 3);
        } while (n != 0);

        store<u32>(offs, 0);
        for (u32 i = 1; i < 256; ++i) store<u32>(offs + 4 * i, load<u32>(offs + 4 * (i - 1)) + load<u16>(hist + 2 * (i - 1)));

        n = count;
        if (rem != 0) {
            do put(--n);
            while ((n & 3) != 0);
        }
        do {
            n -= 4;
            put(n);
            put(n + 1);
            put(n + 2);
            put(n + 3);
        } while (n != 0);

        std::swap(src, dst);
    }
    if (src != base) {
        ctx.gpr[3] = call(ctx, l2::fn::memmove, {src, dst, count << 3});
        return;
    }
    ctx.gpr[3] = src;  // the last pass's output (the layer's entries), left in r3
}

// u32 SortEntries(Entry* e, u32 count, u32 size)   (name?)
// In-place quicksort of 8-byte {u32 key, u32 value} entries by unsigned
// key (size is ignored), with an explicit stack of {lo, hi} ranges at
// sp+56 in its 608-byte frame. Partition: the pivot starts at hi; i and
// j close in, and each swap switches which side moves. The right part is
// pushed when it has two or more entries, the left one taken next.
// Quirks kept: the partition loop runs once even for an empty range (so
// count 0 compares and may swap e[0] with e[-1]); the range stack isn't
// bounds checked. Returns 1.
DECOMP(0056A81C, SortEntries) {
    const u32 e = arg<u32>(ctx, 0);
    const s32 count = arg<s32>(ctx, 1);
    Stack frame(ctx, 608 - 112);
    const u32 sp = frame.addr() - 112;
    const auto at = [&](s32 k) { return e + static_cast<u32>(k) * 8; };

    s32 depth = 0, lo = 0, hi = count - 1;
    for (;;) {
        s32 i = lo, j = hi;
        bool move_j = true;
        do {
            if (load<u32>(at(i)) > load<u32>(at(j))) {
                const u64 a = load<u64>(at(j)), b = load<u64>(at(i));
                store<u64>(at(i), a);
                store<u64>(at(j), b);
                move_j = !move_j;
            }
            if (move_j) --j;
            else ++i;
        } while (i < j);
        if (i + 1 < hi) {
            ++depth;
            const u32 p = sp + 48 + 8 * static_cast<u32>(depth);
            store<s32>(p, i + 1);
            store<s32>(p + 4, hi);
        }
        hi = i - 1;
        if (lo < hi) continue;
        if (depth == 0) break;
        const u32 p = sp + 48 + 8 * static_cast<u32>(depth);
        --depth;
        lo = load<s32>(p);
        hi = load<s32>(p + 4);
    }
    ctx.gpr[3] = 1;
}

namespace {

// The render module's statics torn down by RenderLayers_Destroy.
constexpr u32 kHandle0B58 = 0x010F0B58;    // u64, zeroed if set (name?)
constexpr u32 kVecLive = 0x011DE480;       // u64: nonzero while kVecA is built (name?)
constexpr u32 kVecA = 0x011DE304;          // PtrVector (name?)
constexpr u32 kVecB = 0x010DFA24;          // PtrVector (name?)
constexpr u32 kPool = 0x010DF8B0;          // object: +4 and +304 buffers, +12 sub-object (name?)
constexpr u32 kPoolBaseVtable = 0x00B236E8;
constexpr u32 kSlots = 0x010DFAD0;         // 16 objects of 176 bytes (name?)
constexpr u32 kSlotBaseVtable = 0x00B23720;
constexpr u32 kSlotCount = 16, kSlotSize = 176;
// The 17 RenderLayer statics, Opaque (0x010DC84C) first; most are 172
// bytes, the two at 0x010DCD00 / 0x010DCDC0 192.
constexpr u32 kLayers[17] = {0x010DC84C, 0x010DC8F8, 0x010DC9A4, 0x010DCA50, 0x010DCAFC, 0x010DCBA8,
                             0x010DCC54, 0x010DCD00, 0x010DCDC0, 0x010DCE80, 0x010DCF2C, 0x010DCFD8,
                             0x010DD084, 0x010DD130, 0x010DD1DC, 0x010DD288, 0x010DD334};

namespace dfn {
constexpr u32 PtrVector_Clear = 0x0054EC1C, Nop = 0x0054EC18, Mem_FreeFrom = 0x0054EE6C;
constexpr u32 Pool_DestructPart = 0x0052EFE0;  // (object +12) (name?)
}  // namespace dfn

}  // namespace

// void RenderLayers_Destroy()   (static destructor; name?)
// Clears 0x010F0B58, empties the two pointer vectors (the first only if
// it was built), frees the pool object's two buffers (no null check) and
// destroys its part at +12, drops the 16 slot objects to their base
// vtable, then destroys the 17 render layers from the last to the first
// (base vtable, entry buffer freed and cleared), as RenderLayer's
// destructor does.
DECOMP(0052DD00, RenderLayers_Destroy) {
    if (load<u64>(kHandle0B58) != 0) store<u64>(kHandle0B58, 0);
    if (load<u64>(kVecLive) != 0) {
        call(ctx, dfn::PtrVector_Clear, {kVecA});
        call(ctx, dfn::Nop, {kVecA});
        store<u64>(kVecLive, 0);
    }
    call(ctx, dfn::PtrVector_Clear, {kVecB});
    call(ctx, dfn::Nop, {kVecB});
    store<u32>(kPool, kPoolBaseVtable);
    call(ctx, dfn::Mem_FreeFrom, {load<u32>(kPool + 4), 0});
    call(ctx, dfn::Mem_FreeFrom, {load<u32>(kPool + 304), 0});
    call(ctx, dfn::Pool_DestructPart, {kPool + 12});
    for (u32 i = 0; i < kSlotCount; ++i) store<u32>(kSlots + kSlotSize * i, kSlotBaseVtable);
    for (u32 i = 17; i-- > 0;) {
        const u32 l = kLayers[i];
        store<u32>(l, rl::kBaseVtable);
        if (load<u32>(l + rl::kBase) != 0) {
            call(ctx, dfn::Mem_FreeFrom, {load<u32>(l + rl::kBase), 0});
            store<u32>(l + rl::kBase, 0);
        }
    }
    ctx.gpr[3] = 0;
}
