// Render layer draw and sort internals: DrawItem_Draw, RenderLayer_End,
// RenderLayer_RadixSort and SortEntries; the render module's static
// destructor.
#include "layer2.h"
#include "rsx.h"

#include <utility>

namespace sly4::gfx {

namespace {

namespace l2 = sly4::render::layer2;
constexpr u32 kBinding = 0x010DC840;
constexpr u32 kCurrentSurface = 0x010DDA74;  // object whose +672 is the current view (+664 a forced shader)
constexpr u32 kStencilBytes = 0x010DDA60;    // u8[2]
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 kGlAlways = 0x0207;

DrawBinding& binding() { return global<DrawBinding>(kBinding); }

// Ends a shader (slot 16 with 0).
void end_shader(ShaderPrograms* shader) { vcall(shader, l2::kShaderEnd, 0u); }

}  // namespace

// void DrawItem_Draw(DrawItem* item)   (name?)
// Picks the shader for the item's material (+28; the material's +16
// object, slot 16; the current view's +664 overrides it when set). A new
// shader ends the old one and clears the binding; a new material ends the
// shader and binds the material to it (Material_Bind). Then shader slot 12
// draws the item.
void DrawItem_Draw(DrawItem* item) {
    Material* mat = item->mat;
    u8* picker = host<u8>(or_zero_page(mat)->w16);
    ShaderPrograms* shader = vcall<ShaderPrograms*>(picker, l2::kPickShader, mat, item);
    u8* view = *at<gp<u8>>(global<gp<u8>>(kCurrentSurface).get(), 672);
    if (ShaderPrograms* forced = *at<gp<ShaderPrograms>>(view, 664)) shader = forced;

    DrawBinding& b = binding();
    if (ShaderPrograms* cur = b.shader; cur != shader) {
        if (cur) end_shader(cur);
        b.shader = shader;
        b.material = nullptr;
        b.w8 = 0u;
    }
    if (Material* bound = b.material; bound != mat) {
        // The original ends the *new* shader here (r3 still holds it).
        if (bound) end_shader(shader);
        b.material = mat;
        Material_Bind(mat, shader);
    }
    vcall(shader, l2::kShaderDraw, item);
}

// void RenderLayer_End()   (name?)
// Ends the bound shader, clears the binding and turns the stencil test
// off (ops KEEP / KEEP / KEEP, mask 0, func ALWAYS 0 0, cached at render
// state +160..+184), clearing render::g::stencil_bytes.
void RenderLayer_End() {
    DrawBinding& b = binding();
    if (ShaderPrograms* shader = b.shader) end_shader(shader);
    b.material = nullptr;
    b.shader = nullptr;
    b.w8 = 0u;
    RenderState& st = rs();
    emit({kStencilTestEnable, 0});
    st.stencil_func = kGlAlways;
    st.stencil_ref = 0u;
    st.stencil_func_mask = 0u;
    st.stencil_op[0] = kGlKeep;
    st.stencil_op[1] = kGlKeep;
    st.stencil_op[2] = kGlKeep;
    st.stencil_mask = 0u;
    emit({kStencilOp, kGlKeep, kGlKeep, kGlKeep});
    emit({kStencilMask, 0});
    front_func();
    global<u8>(kStencilBytes) = 0;
    global<u8>(kStencilBytes + 1) = 0;
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
void RenderLayer_RadixSort(RenderLayer* l) {
    GuestFrame f(0x8A10 - 112);  // the original's 35344-byte frame, from sp+112
    be_u16* hist = f.as<be_u16>(0);           // u16[256]
    u8* tmp = f.as<u8>(512);                  // Entry[4200]
    be_u32* offs = f.as<be_u32>(0x85B0 - 112);  // u32[256]
    u8* const base = reinterpret_cast<u8*>(l->base.get());
    const u32 count = static_cast<u32>(static_cast<s32>(l->cursor.ea - l->base.ea) / 8);  // srawi + addze
    for (u32 i = 0; i < 4200; ++i) *reinterpret_cast<be_u32*>(tmp + 8 * i) = 0u;

    u8* src = base;
    u8* dst = tmp;
    for (s32 pass = 0; pass < 4; ++pass) {
        call(sly4::fn::memset, hist, 0u, 512u);
        const u32 byte = static_cast<u32>(3 - pass);
        const auto bucket = [&](u32 k) -> u32 { return *at<u8>(src, 8 * k + byte); };
        const auto count_one = [&](u32 k) {
            be_u16& h = hist[bucket(k)];
            h = static_cast<u16>(h + 1);
        };
        const auto put = [&](u32 k) {
            be_u32& o = offs[bucket(k)];
            const u32 slot = o;
            *at<be_u64>(dst, slot << 3) = at<be_u64>(src, 8 * k)->get();
            o = slot + 1;
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

        offs[0] = 0u;
        for (u32 i = 1; i < 256; ++i) offs[i] = offs[i - 1] + hist[i - 1];

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
    if (src != base) call(memmove, src, dst, count << 3);
}

// void SortEntries(LayerEntry* e, s32 count, u32 size)   (name?)
// In-place quicksort of 8-byte {u32 key, u32 value} entries by unsigned
// key (size is ignored), with an explicit stack of {lo, hi} ranges at
// sp+56 in its 608-byte frame. Partition: the pivot starts at hi; i and
// j close in, and each swap switches which side moves. The right part is
// pushed when it has two or more entries, the left one taken next.
// Quirks kept: the partition loop runs once even for an empty range (so
// count 0 compares and may swap e[0] with e[-1]); the range stack isn't
// bounds checked (it lives in guest stack memory, as the original's).
void SortEntries(LayerEntry* e, s32 count, u32) {
    GuestFrame f(608);
    const auto entry = [&](s32 k) { return at<be_u64>(e, static_cast<u32>(k) * 8); };
    const auto key = [&](s32 k) -> u32 { return *at<be_u32>(e, static_cast<u32>(k) * 8); };
    const auto range = [&](s32 d) { return host<be_s32>(f.addr() + 48 + 8 * static_cast<u32>(d)); };

    s32 depth = 0, lo = 0, hi = count - 1;
    for (;;) {
        s32 i = lo, j = hi;
        bool move_j = true;
        do {
            if (key(i) > key(j)) {
                const u64 a = entry(j)->get(), b = entry(i)->get();
                *entry(i) = a;
                *entry(j) = b;
                move_j = !move_j;
            }
            if (move_j) --j;
            else ++i;
        } while (i < j);
        if (i + 1 < hi) {
            ++depth;
            be_s32* p = range(depth);
            p[0] = i + 1;
            p[1] = hi;
        }
        hi = i - 1;
        if (lo < hi) continue;
        if (depth == 0) break;
        const be_s32* p = range(depth);
        --depth;
        lo = p[0];
        hi = p[1];
    }
}

namespace {

// The render module's statics torn down by RenderLayers_Destroy.
constexpr u32 kHandle0B58 = 0x010F0B58;    // u64, zeroed if set (name?)
constexpr u32 kVecLive = 0x011DE480;       // u64: nonzero while kVecA is built (name?)
constexpr u32 kVecA = 0x011DE304;          // PtrVector (name?)
constexpr u32 kVecB = 0x010DFA24;          // PtrVector (name?)
constexpr u32 kPool = 0x010DF8B0;          // StencilShadow: +4 and +304 buffers, +12 sub-object
constexpr u32 kPoolBaseVtable = 0x00B236E8;
constexpr u32 kSlots = 0x010DFAD0;         // 16 QuerySlots of 176 bytes
constexpr u32 kSlotBaseVtable = 0x00B23720;
constexpr u32 kSlotCount = 16, kSlotSize = 176;
constexpr u32 kLayerBaseVtable = 0x00B233E8;
// The 17 RenderLayer statics, Opaque (0x010DC84C) first; most are 172
// bytes, the two at 0x010DCD00 / 0x010DCDC0 192.
constexpr u32 kLayers[17] = {0x010DC84C, 0x010DC8F8, 0x010DC9A4, 0x010DCA50, 0x010DCAFC, 0x010DCBA8,
                             0x010DCC54, 0x010DCD00, 0x010DCDC0, 0x010DCE80, 0x010DCF2C, 0x010DCFD8,
                             0x010DD084, 0x010DD130, 0x010DD1DC, 0x010DD288, 0x010DD334};

constexpr u32 PtrVector_Clear = 0x0054EC1C, Nop = 0x0054EC18, Mem_FreeFrom = 0x0054EE6C;

}  // namespace

// void RenderLayers_Destroy()   (static destructor; name?)
// Clears 0x010F0B58, empties the two pointer vectors (the first only if
// it was built), frees the pool object's two buffers (no null check) and
// destroys its part at +12, drops the 16 slot objects to their base
// vtable, then destroys the 17 render layers from the last to the first
// (base vtable, entry buffer freed and cleared), as RenderLayer's
// destructor does.
void RenderLayers_Destroy() {
    be_u64& handle = global<be_u64>(kHandle0B58);
    if (handle != 0) handle = 0;
    be_u64& live = global<be_u64>(kVecLive);
    if (live != 0) {
        call(PtrVector_Clear, kVecA);
        call(Nop, kVecA);
        live = 0;
    }
    call(PtrVector_Clear, kVecB);
    call(Nop, kVecB);
    StencilShadow& pool = global<StencilShadow>(kPool);
    pool.vtable = kPoolBaseVtable;
    call(Mem_FreeFrom, pool.cursor.ea.get(), 0u);
    call(Mem_FreeFrom, pool.face_sign.ea.get(), 0u);
    Pool_DestructPart(&pool.quad);
    for (u32 i = 0; i < kSlotCount; ++i) global<be_u32>(kSlots + kSlotSize * i) = kSlotBaseVtable;
    for (u32 i = 17; i-- > 0;) {
        RenderLayer& l = global<RenderLayer>(kLayers[i]);
        l.vtable = kLayerBaseVtable;
        if (l.base) {
            call(Mem_FreeFrom, l.base.ea.get(), 0u);
            l.base = nullptr;
        }
    }
}

}  // namespace sly4::gfx

NATIVE(00512C30, sly4::gfx::DrawItem_Draw)
NATIVE(00512DA0, sly4::gfx::RenderLayer_End)
NATIVE(00513394, sly4::gfx::RenderLayer_RadixSort)
NATIVE(0056A81C, sly4::gfx::SortEntries)
NATIVE(0052DD00, sly4::gfx::RenderLayers_Destroy)
