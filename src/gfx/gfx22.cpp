// The renderer range's next recompiled functions (gfx22): the larger
// remaining ones. Names carry the address where the role is unclear.
#include <atomic>
#include <bit>
#include <cmath>

#include "fx.h"
#include "gfx22.h"
#include "post.h"

using namespace sly4;

namespace {

namespace g22 = sly4::render::gfx22;
namespace mat = sly4::render::gfx22::mat;
using sly4::fx::vcall;

inline f32 f32_at(u32 a) { return std::bit_cast<f32>(load<u32>(a)); }
inline void put_f32(u32 a, f32 v) { store<u32>(a, std::bit_cast<u32>(v)); }
inline u64 sx(u32 v) { return static_cast<u64>(static_cast<s64>(static_cast<s32>(v))); }

// A u8 table lookup the original guards with an unconditional trap (tw
// 31) when the index is out of range, then reads entry 0 anyway.
u32 table_at(PPUContext& ctx, u32 table, u32 i, u32 n, u32 trap_pc) {
    if (i < n) return load<u32>(table + 4 * i);
    trap(ctx, trap_pc);
    return load<u32>(table);
}

}  // namespace

// u32 Material_Apply_0050C3D8(AglMaterial* m, const MaterialRecord* rec)
// [called by Iff_HandleMaterial_0051C704] (name?)
// The material chunk's record into the material: its name hash; the
// diffuse texture (rec +0 looked up in the texture map, the default one if
// absent) added as the next texture (the slot's old texture released, the
// new one's references bumped; the first also sets +48 from its +112); the
// colours (u8 / 255); the blend mode, cull face and wrap modes from their
// tables; the flag bits; the depth function (LEQUAL with record flag 8,
// else ALWAYS); the sort class from the sort layer; then the shader of
// rec +80 (the default hash's after calling the material's vfunc +12 when
// none is found) bound until its Bind (vfunc +4) succeeds, a failing hash
// being replaced by the default in the record. Returns Bind's result.
// Quirks: the texture count wraps at 256 (slot -1 is +16, the shader
// pointer); the wrap call does not check for a null texture.
DECOMP(0050C3D8, Material_Apply_0050C3D8) {
    const u32 m = arg<u32>(ctx, 0), rec = arg<u32>(ctx, 1);
    Stack frame(ctx, 176);
    const u32 key = frame.addr() + 112;
    store<u32>(m + mat::kNameHash, load<u32>(rec + mat::kRecNameHash));
    store<u32>(key, load<u32>(rec + mat::kRecDiffuseHash));
    const u32 found = static_cast<u32>(call(ctx, g22::fn::Map_Find, {load<u32>(mat::kTexMapHolder + 24), key}));
    u32 tex = load<u32>(found);
    if (tex == 0) tex = load<u32>(mat::kTexMapHolder);
    const u32 count = static_cast<u8>(load<u8>(m + mat::kTexCount) + 1);
    const s32 idx = static_cast<s32>(count) - 1;
    store<u8>(m + mat::kTexCount, static_cast<u8>(count));
    const u32 slot = m + mat::kTextures + 4 * static_cast<u32>(idx);
    const u32 old = load<u32>(slot);
    if (old != tex) {
        store<u32>(slot, tex);
        if (tex != 0) store<u16>(tex + mat::kTexRefs, static_cast<u16>(load<u16>(tex + mat::kTexRefs) + 1));
        if (old != 0) vcall(ctx, old, mat::kTexVRelease, {old});
    }
    if (idx == 0) store<u16>(m + mat::kTexWord, static_cast<u16>(tex == 0 ? 0 : load<u32>(tex + 112)));

    const f32 scale = f32_at(mat::kColourScale);
    auto colour = [&](u32 dst, u32 src) {
        f32 c[4];
        for (u32 k = 0; k < 4; ++k)
            c[k] = static_cast<f32>(static_cast<f64>(static_cast<f32>(load<u8>(src + k))) / static_cast<f64>(scale));
        for (u32 k = 0; k < 4; ++k) put_f32(dst + 4 * k, c[k]);
    };
    colour(m + mat::kDiffuse, rec + mat::kRecDiffuse);
    colour(m + mat::kSpecular, rec + mat::kRecSpecular);
    const u32 power = load<u32>(rec + mat::kRecSpecPower);
    store<u32>(m + mat::kSpecPower, power);
    ctx.fpr[1] = render::gfx10::lfs_value(power);  // the lfs / stfs copy's f1

    const u32 st = m + mat::kState;
    const u32 mode = table_at(ctx, mat::kBlendModes, load<u8>(rec + mat::kRecBlend), 5, 0x0050C610);
    auto blend = [&](u32 eq, u32 src, u32 dst, bool enable) {
        u32 w = (load<u32>(st + mat::kStBlend) | 0x80000000u) & ~0x7FFF8000u;
        w |= (src << 15) & 0x7FFF8000u;
        if (!enable) w &= 0x7FFFFFFFu;
        store<u16>(st + mat::kStEquation, static_cast<u16>(eq));
        store<u16>(st + mat::kStDest, static_cast<u16>(dst));
        store<u32>(st + mat::kStBlend, w);
    };
    switch (mode) {
    case 4: blend(mat::kFuncReverseSubtract, mat::kBlendSrcAlpha, mat::kBlendOne, true); break;
    case 3: blend(mat::kFuncAdd, mat::kBlendSrcAlpha, mat::kBlendOne, true); break;
    case 2: blend(mat::kFuncAdd, mat::kBlendSrcAlpha, mat::kBlendOneMinusSrcAlpha, true); break;
    case 1: blend(mat::kFuncAdd, mat::kBlendOne, mat::kBlendZero, true); break;
    case 0: blend(mat::kFuncAdd, mat::kBlendOne, mat::kBlendZero, false); break;
    default: break;
    }
    store<u16>(st + mat::kStCull,
               static_cast<u16>(table_at(ctx, mat::kCullFaces, load<u8>(rec + mat::kRecCull), 3, 0x0050C670)));
    const u32 wrap_u = table_at(ctx, mat::kWrapModes, load<u8>(rec + mat::kRecClampU), 3, 0x0050C6A0);
    const u32 wrap_v = table_at(ctx, mat::kWrapModes, load<u8>(rec + mat::kRecClampV), 3, 0x0050C6B8);
    vcall(ctx, tex, mat::kTexVSetWrap, {tex, sx(wrap_u), sx(wrap_v), 1});

    // The flag bits, each stored as it is merged.
    auto bit = [&](u32 mask) { return (load<u8>(rec + mat::kRecFlags) & mask) != 0 ? 1u : 0u; };
    u8 f = load<u8>(m + mat::kFlags);
    f = static_cast<u8>((f & ~0x20) | (bit(0x02) << 5));
    store<u8>(m + mat::kFlags, f);
    f = static_cast<u8>((f & ~0x80) | (bit(0x01) << 7));
    store<u8>(m + mat::kFlags, f);
    f = static_cast<u8>((f & ~0x40) | (bit(0x04) << 6));
    store<u8>(m + mat::kFlags, f);
    f = static_cast<u8>((f & ~0x10) | (bit(0x20) << 4));
    store<u8>(m + mat::kFlags, f);
    const u32 depth_write = bit(0x10);
    store<u32>(st + mat::kStDepth, (load<u32>(st + mat::kStDepth) & 0x7FFFFFFFu) | (depth_write << 31));
    store<u8>(m + mat::kFlags2, (load<u8>(m + mat::kFlags) & 0x10) != 0 ? 8 : 0);
    const u32 func = bit(0x08) ? mat::kDepthLequal : mat::kDepthAlways;
    store<u32>(st + mat::kStDepth, (load<u32>(st + mat::kStDepth) & ~0x7FFF8000u) | ((func << 15) & 0x7FFF8000u));

    u8 sort = 0;
    switch (load<u8>(rec + mat::kRecSortLayer)) {
    case 44: sort = 2; break;
    case 40: sort = 1; break;
    case 46: sort = 10; break;
    case 47: sort = 3; break;
    case 48: sort = 4; break;
    case 56: sort = 5; break;
    case 60: sort = 6; break;
    case 64: case 80: case 96: sort = 9; break;
    case 102: sort = 12; break;
    default: break;  // 32 and the rest
    }
    store<u8>(m + mat::kSortClass, sort);

    for (;;) {
        const u32 hash = load<u32>(rec + mat::kRecShaderHash);
        u32 sh = static_cast<u32>(call(ctx, g22::fn::Shader_FindByHash, {hash}));
        if (sh == 0) {
            if (load<u32>(rec + mat::kRecShaderHash) == hash) sh = static_cast<u32>(call(ctx, g22::fn::Shader_FindByHash, {hash}));
            if (sh == 0) {
                vcall(ctx, m, mat::kMatVNoShader, {m});
                sh = static_cast<u32>(call(ctx, g22::fn::Shader_FindByHash, {load<u32>(mat::kDefaultShaderHash)}));
            }
        }
        const u32 hash2 = load<u32>(rec + mat::kRecShaderHash);
        const u64 ok = vcall(ctx, sh, mat::kShaderVBind, {sh, m, rec});
        if (static_cast<u32>(ok) == 0 && load<u32>(rec + mat::kRecShaderHash) == hash2)
            store<u32>(rec + mat::kRecShaderHash, load<u32>(mat::kDefaultShaderHash));
        if (static_cast<u32>(ok) != 0) {
            store<u32>(m + mat::kShader, sh);
            ctx.gpr[3] = ok;
            return;
        }
    }
}

// void SlotsWork_Build_00599154()  [called through a pointer] (name?)
// SlotsWork_Prepare, then (when the view work's +96 flag 0x80 is clear and
// the current view's +256 bit 0 is set) the view block (swb::) built on
// the stack and passed to the work object's vfunc +24 (obj, work, block,
// kVfArg) with its +4 / +8 set to 0 / 1.0 for the call and restored
// after; a frame block of kBlockCount * 128 bytes (kBlockPtr, kSlotsAux),
// and the object's +112 / +208 results copied into the slots work. With
// the view off: zeros copied, the old kBlockPtr in kSlotsAux and +3456,
// and r3 left as that pointer (kWork otherwise), as the original does.
DECOMP(00599154, SlotsWork_Build_00599154) {
    namespace swb = g22::swb;
    call(ctx, g22::fn::SlotsWork_Prepare, {});
    const u32 work = load<u32>(swb::kViewWork);
    const u32 view = static_cast<u32>(call(ctx, g22::fn::View_GetCurrent, {}));
    const u32 old_block = load<u32>(swb::kBlockPtr);
    Stack frame(ctx, 752);
    const u32 sp = frame.addr();
    auto copy = [](u32 dst, u32 src, u32 n) {  // lfs / stfs pairs: raw words
        for (u32 k = 0; k < n; ++k) store<u32>(dst + 4 * k, load<u32>(src + 4 * k));
    };
    if ((load<u8>(work + 96) & 0x80) != 0 || (load<u32>(view + swb::kViewOn) & 1) == 0) {
        for (u32 o = 240; o < 464; o += 4) store<u32>(sp + o, 0);
        store<u32>(swb::kSlotsAux, old_block);
        for (u32 k = 0; k < 6; ++k) copy(swb::kWork + swb::kWorkRows + 16 * k, sp + 240 + 16 * k, 4);
        for (u32 k = 0; k < 2; ++k) copy(swb::kWork + swb::kWorkMats + 64 * k, sp + 336 + 64 * k, 16);
        store<u32>(swb::kWork + swb::kWorkEnd, old_block);
        store<u32>(swb::kWork + swb::kWorkAux, 0);
        ctx.fpr[1] = 0.0;  // the last lfs of the zeros
        ctx.gpr[3] = old_block;
        return;
    }
    const u32 obj = load<u32>(work + 4);
    const u32 tmp = sp + 112, inv_a = sp + 176, blk = sp + 464;
    call(ctx, g22::fn::Mat4_Invert, {tmp, inv_a, view + swb::kViewInvA});
    call(ctx, g22::fn::Mat4_Invert, {tmp, blk, view + swb::kViewInvB});
    // blk +64: row i = inv_a rows 0-2 times kUvToClip row i's x, y, z
    // (vmaddfp chains from 0, row 3 from inv_a's row 3).
    for (u32 i = 0; i < 4; ++i) {
        const u32 r = swb::kUvToClip + 16 * i;
        for (u32 l = 0; l < 4; ++l) {
            f32 acc = i == 3 ? f32_at(inv_a + 48 + 4 * l) : 0.0f;
            for (u32 j = 0; j < 3; ++j) acc = std::fmaf(f32_at(inv_a + 16 * j + 4 * l), f32_at(r + 4 * j), acc);
            put_f32(blk + 64 + 16 * i + 4 * l, acc);
        }
    }
    for (u32 k = 0; k < 4; ++k) copy(blk + 128 + 16 * k, view + swb::kViewRows + 16 * k, 3);
    const f32 f60 = f32_at(view + swb::kViewF60);
    store<u32>(blk + 208, load<u32>(view + swb::kViewF60));
    put_f32(blk + 212, static_cast<f32>(static_cast<f64>(f32_at(work + 84)) - static_cast<f64>(f60)));
    copy(blk + 192, view + swb::kViewPos, 3);
    const u32 save4 = load<u32>(obj + 4), save8 = load<u32>(obj + 8);
    store<u32>(obj + 4, 0);
    store<u32>(obj + 8, 0x3F800000);  // 1.0f
    ctx.fpr[1] = render::gfx10::lfs_value(load<u32>(view + swb::kViewPos + 8));
    vcall(ctx, obj, swb::kObjVf, {obj, work, blk, swb::kVfArg});
    store<u32>(obj + 4, save4);
    store<u32>(obj + 8, save8);
    const u32 block = static_cast<u32>(call(ctx, g22::fn::Render_FrameAlloc, {load<u32>(swb::kBlockCount) << 7}));
    const u32 count = load<u32>(swb::kBlockCount);
    store<u32>(swb::kBlockPtr, block);
    store<u32>(swb::kSlotsAux, block);
    for (u32 k = 0; k < 6; ++k) copy(swb::kWork + swb::kWorkRows + 16 * k, obj + swb::kObjRows + 16 * k, 4);
    for (u32 k = 0; k < 2; ++k) copy(swb::kWork + swb::kWorkMats + 64 * k, obj + swb::kObjMats + 64 * k, 16);
    store<u32>(swb::kWork + swb::kWorkEnd, block + (count << 7));
    store<u32>(swb::kWork + swb::kWorkAux, swb::kSlotsAux);
    ctx.fpr[1] = render::gfx10::lfs_value(load<u32>(obj + swb::kObjMats + 64 + 60));
    ctx.gpr[3] = swb::kWork;
}

namespace {

namespace th = sly4::render::gfx22::thermal;

// Appends s to the type's shader list (PtrVector at +12), doubling the
// capacity when full. Returns what r3 holds after the inlined copy: the
// high-water mark before the update (`old_high`) or after it.
u32 shader_push(PPUContext& ctx, u32 type, u32 s, bool old_high) {
    if (load<u16>(type + th::kCount) == load<u16>(type + th::kCapacity))
        call(ctx, g22::fn::PtrVector_Reserve, {type + th::kShaders, (static_cast<u32>(load<u16>(type + th::kCapacity)) << 1) & 0x1FFFEu});
    const u32 count = load<u16>(type + th::kCount);
    store<u32>(load<u32>(type + th::kShaders) + 4 * count, s);
    const u32 next = load<u16>(type + th::kCount) + 1u;  // up to 0x10000 in the register
    const u32 high = load<u16>(type + th::kHighWater);
    store<u16>(type + th::kCount, static_cast<u16>(next));
    const u32 top = high > (next & 0xFFFF) ? high : next;
    store<u16>(type + th::kHighWater, static_cast<u16>(top));
    return old_high ? high : top;
}

// The stack slots one inlined Shader_Load uses (the two copies differ).
struct LoadSlots {
    u32 vpath, vkey, vsize, fpath, fkey, fsize;
};

// Shader_Load inlined: the name into +12, then the vertex program `name`
// (VMap, else System\Shaders\<name>_V.vpo) and the fragment program
// `fname` (or `name`; FMap, else <fname>.fpo, bound with kBindings).
void thermal_load(PPUContext& ctx, u32 s, u32 name, u32 fname, u32 sp, const LoadSlots& o) {
    namespace pfn = sly4::post::fn;
    namespace pg = sly4::post::g;
    call(ctx, g22::fn::Shader_Construct, {s});
    store<u8>(s + th::kName, 0);
    store<u32>(s, th::kVtbl);
    call(ctx, g22::fn::String_Copy, {s + th::kName, name, th::kNameSize});
    store<u32>(sp + o.vkey, static_cast<u32>(call(ctx, pfn::String_HashNoCase, {name})));
    const u32 vp = load<u32>(static_cast<u32>(call(ctx, pfn::VMap_Find, {pg::vmap, sp + o.vkey})));
    store<u32>(s + th::kVProgram, vp);
    if (vp == 0) {
        call(ctx, pfn::String_Printf, {sp + o.vpath, pg::fmt_vpo, name});
        store<u32>(sp + o.vsize, 0);
        const u32 data = static_cast<u32>(call(ctx, pfn::File_LoadWhole, {sp + o.vpath, sp + o.vsize}));
        call(ctx, pfn::VProgram_Create, {s + th::kVProgram, data, 0});
        call(ctx, pfn::VMap_Insert, {pg::vmap, sp + o.vkey, load<u32>(s + th::kVProgram)});
        const u64 mask = call(ctx, pfn::Cg_GetAttributeMask, {load<u32>(load<u32>(s + th::kVProgram) + 4)});
        store<u16>(load<u32>(s + th::kVProgram) + th::kVpAttribMask, static_cast<u16>(mask));
    }
    const u32 fn_name = fname != 0 ? fname : name;
    store<u32>(sp + o.fkey, static_cast<u32>(call(ctx, pfn::String_HashNoCase, {fn_name})));
    const u32 fp = load<u32>(static_cast<u32>(call(ctx, pfn::FMap_Find, {pg::fmap, sp + o.fkey})));
    store<u32>(s + th::kFProgram, fp);
    if (fp == 0) {
        call(ctx, pfn::String_Printf, {sp + o.fpath, pg::fmt_fpo, fn_name});
        store<u32>(sp + o.fsize, 0);
        const u32 data = static_cast<u32>(call(ctx, pfn::File_LoadWhole, {sp + o.fpath, sp + o.fsize}));
        call(ctx, pfn::FProgram_Create, {s + th::kFProgram, data, load<u32>(sp + o.fsize)});
        call(ctx, pfn::FMap_Insert, {pg::fmap, sp + o.fkey, load<u32>(s + th::kFProgram)});
        call(ctx, pfn::FProgram_Setup, {s + th::kFProgram, th::kBindings});
    }
}

}  // namespace

// void ThermalObjectType_Create_0054D4C0(ShaderType* t)  [ShaderType slot 0, PostProc_ThermalObject] (name?)
// shader3_types.cpp's add_table(t, kVtbl, kTable, 0, 2, skinned, null
// check) with Shader_Load inlined (no vertex parameter table): per entry
// a shader for +0 and one for +4 (both with the fragment program +8),
// failed allocations and null entries as null list entries. The names are
// read after the allocation. r3 is left as the last inlined list append
// leaves it.
DECOMP(0054D4C0, ThermalObjectType_Create_0054D4C0) {
    const u32 t = arg<u32>(ctx, 0);
    Stack frame(ctx, 1344);
    const u32 sp = frame.addr();
    constexpr LoadSlots kFirst = {880, 1160, 1164, 624, 1152, 1156};
    constexpr LoadSlots kSecond = {368, 1144, 1148, 112, 1136, 1140};
    u32 r3 = 0;
    for (u32 i = 0; i < th::kEntries; ++i) {  // cmplwi
        const u32 e = th::kTable + 12 * i;
        if (load<u32>(e) == 0) {
            shader_push(ctx, t, 0, false);
            r3 = shader_push(ctx, t, 0, true);
            continue;
        }
        u32 s = static_cast<u32>(call(ctx, g22::fn::Mem_Alloc, {th::kShaderSize, 0, 16}));
        if (s != 0) thermal_load(ctx, s, load<u32>(e), load<u32>(e + 8), sp, kFirst);
        shader_push(ctx, t, s, true);
        s = static_cast<u32>(call(ctx, g22::fn::Mem_Alloc, {th::kShaderSize, 0, 16}));
        if (s != 0) thermal_load(ctx, s, load<u32>(e + 4), load<u32>(e + 8), sp, kSecond);
        r3 = shader_push(ctx, t, s, false);
    }
    ctx.gpr[3] = r3;
}

namespace {

namespace rd = sly4::render::gfx22::reduced;

// The 52-byte Surface's inlined default constructor: +0 and +40 cleared,
// +12's top nibble cleared, +4 = 1 and +8 = 2 (the bitfield merges keep
// nothing of the old words). `alt_order`: the store order of the copies
// that store the guard between +40 and +12.
void surface_construct(u32 p, u32 guard, bool alt_order, bool set_guard = true) {
    const u32 w12 = load<u32>(p + 12) & 0x0FFFFFFFu;
    store<u32>(p, 0);
    if (alt_order) {
        store<u32>(p + 40, 0);
        if (set_guard) store<u8>(guard, 1);
        store<u32>(p + 12, w12);
    } else {
        store<u32>(p + 12, w12);
        store<u32>(p + 40, 0);
    }
    store<u32>(p + 4, 1);
    store<u32>(p + 8, 2);
    if (!alt_order && set_guard) store<u8>(guard, 1);
}

// A static surface made on first use, then pair = {surface, Target_Attach
// of `target` at y = slice * h}, the surface's buffer set to it.
void attach(PPUContext& ctx, const rd::Attach& a, u32 w, u32 h) {
    if (load<u8>(a.surface - 8) == 0) surface_construct(a.surface, a.surface - 8, a.alt_order);
    store<u32>(a.pair, a.surface);
    call(ctx, g22::fn::Target_Attach, {a.pair + 4, a.target, w, h, 0, a.slice * h});
    call(ctx, g22::fn::Surface_SetBuffer, {load<u32>(a.pair), load<u32>(a.pair + 4)});
}

u32 round8(u32 v) { return v == 0 ? 0 : ((v - 1) & ~7u) + 8; }

}  // namespace

// void Targets_InitReduced_005467DC()  [called by Render_Init546EF4] (name?)
// The static target array (5 surfaces) made on first use, then from the
// config size (w, h): a quarter-size target (w / 4 by 3 * round8(h / 4)
// + h / 4) with four surfaces at rows 0, h/4, 2h/4, 3h/4 and a depth
// target; an eighth-size one (w / 8 by round8(h / 8) + h / 8) with two
// surfaces at rows 0 and h/8, a surface on the first quarter slice and
// an attach of the quarter depth target; then the half-size texture
// surface (Texture_InitLayout) and a half-size depth target. r3 is
// Target_InitDepth's result.
DECOMP(005467DC, Targets_InitReduced_005467DC) {
    if (load<u8>(rd::kStaticArrayGuard) == 0) {
        for (u32 k = 0; k < rd::kStaticArrayCount; ++k) surface_construct(rd::kStaticArray + rd::kSurfaceSize * k, 0, false, false);
        store<u8>(rd::kStaticArrayGuard, 1);
    }
    const u32 h4 = load<u16>(rd::kConfig + 6) >> 2, w4 = load<u16>(rd::kConfig + 4) >> 2;  // srawi / addze of a u16
    call(ctx, g22::fn::Target_Init, {rd::kQuarter, w4, 3 * round8(h4) + h4, rd::kTargetFormat, 0, 0});
    constexpr rd::Attach kQuarterSlices[4] = {
        {0x01164590, 0x01164040, rd::kQuarter, 0, false},
        {0x01164550, 0x01164048, rd::kQuarter, 1, false},
        {0x01164510, 0x01164050, rd::kQuarter, 2, true},
        {0x011644D0, 0x01164058, rd::kQuarter, 3, false},
    };
    for (const auto& a : kQuarterSlices) attach(ctx, a, w4, h4);
    call(ctx, g22::fn::Target_InitDepth, {rd::kQuarterDepth, w4, h4, rd::kDepthFormat, 0});
    const u32 h8 = h4 >> 1, w8 = w4 >> 1;
    call(ctx, g22::fn::Target_Init, {rd::kEighth, w8, round8(h8) + h8, rd::kTargetFormat, 0, 0});
    constexpr rd::Attach kEighthSlices[3] = {
        {0x01164490, 0x01164060, rd::kEighth, 0, false},
        {0x01164450, 0x01164068, rd::kEighth, 1, true},
        {0x01164410, 0x01164070, 0x01164044, 0, false},  // on the first quarter slice
    };
    for (const auto& a : kEighthSlices) attach(ctx, a, w8, h8);
    call(ctx, g22::fn::Target_Attach, {0x0116407C, rd::kQuarterDepth, w8, h8, 0, 0});
    const u32 w = load<u16>(rd::kConfig + 4), h = load<u16>(rd::kConfig + 6);
    if (load<u8>(rd::kHalfTextureGuard) == 0) surface_construct(rd::kHalfTexture, rd::kHalfTextureGuard, false);
    store<u32>(rd::kHalfTextureRef, rd::kHalfTexture);
    call(ctx, g22::fn::Texture_InitLayout, {rd::kHalfTexture, w >> 1, h >> 1, 1, 3, 1, 0, 0});
    store<u32>(rd::kHalfTextureTarget, load<u32>(rd::kHalfTexture + 40));
    const u32 w2 = load<u16>(rd::kConfig + 4) >> 1, h2 = load<u16>(rd::kConfig + 6) >> 1;
    ctx.gpr[3] = call(ctx, g22::fn::Target_InitDepth, {rd::kHalfDepth, w2, h2, rd::kDepthFormat, 0});
}

namespace {

namespace in = sly4::render::gfx22::inst;
namespace g21 = sly4::render::gfx21;

// ApeTListAllocator<T>::GetAllocator() inlined: the class's free list,
// made on first use (60 bytes zeroed, its vtable and name,
// FreeList_Init(l, size, heap, 0, 0) with r8 / r9 = the vtable / name;
// FreeList_Init reads r3..r7). 0 is stored when Mem_Alloc fails.
u32 class_list(PPUContext& ctx, const in::InstClass& c) {
    const u32 header = load<u32>(c.holder);
    u32 list = load<u32>(header);
    if (list != 0) return list;
    list = static_cast<u32>(call(ctx, g22::fn::Mem_Alloc, {60, 0, 16}));
    if (list != 0) {
        const u32 heap = load<u32>(header + 4);
        for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(list + off, 0);
        const u16 h54 = load<u16>(list + 54);
        store<u16>(list + 52, 0);
        store<u16>(list + 54, static_cast<u16>(h54 & 0x7FFF));
        store<u32>(list, c.vtbl);
        store<u32>(list + 56, c.name);
        call(ctx, g22::fn::FreeList_Init, {list, c.size, heap, 0, 0});
    }
    store<u32>(header, list);
    return list;
}

// The instance after item link `p` (0 at the end).
u32 next_inst(u32 inst) {
    const u32 item = load<u32>(inst - in::kLink);
    return item != 0 ? item + in::kLink : 0;
}
u32 first_inst(PPUContext& ctx, const in::InstClass& c = in::kGeoms) {
    const u32 item = static_cast<u32>(call(ctx, g22::fn::Obj_GetField4, {class_list(ctx, c)}));
    return item != 0 ? item + in::kLink : 0;
}

}  // namespace

// void GeomInstances_Instantiate_0058EA70(Scene* scene)  [called by Bank_Start] (name?)
// Builds a local map of the instances with a def (key +204 -> instance;
// capacity the free list's +12, from Mem_Allocator552588 or the default
// allocator), then for each instance not yet done whose def names another
// instance (def +96, found in the map, not itself, not its own key, and
// that instance is its own def's): a named instance (+8 0x400) whose
// source's +320 kind is not 3 leaves the class's named map (when it is
// the one there) and loses the flag; copies the source (flags 57, 61 with
// +12, | 0x40 with +168 0x20000), clones the source's children onto it
// (Clone(21), AddChild(-1), release), collects its tree, vfunc +40 and
// WorldObj_SetFlag4000 with its +164 bits, ForEach (with +168 0x20000),
// Node_SyncTree, and the source's vfunc +40(0, 1). Every visited
// instance is marked done (+322 0x20). The map's data is returned to its
// allocator (vfunc +8) and the Array destructed; r3 is Array_Destruct's.
// Quirk: the free list's +12 is read before the list is made.
DECOMP(0058EA70, GeomInstances_Instantiate_0058EA70) {
    const u32 scene = arg<u32>(ctx, 0);
    Stack frame(ctx, 240);
    const u32 sp = frame.addr(), map = sp + 124;
    const u32 cap0 = load<u32>(load<u32>(load<u32>(g21::kGeomInstClass)) + 12);
    u32 alloc = static_cast<u32>(call(ctx, g22::fn::Mem_Allocator552588, {}));
    store<u16>(map + 0, static_cast<u16>(cap0));
    store<u16>(map + 2, 0);
    store<u16>(map + 4, 0);
    store<u16>(map + 6, 16);  // 8-byte elements << 1
    store<u32>(map + 8, 0);
    if (alloc == 0) alloc = static_cast<u32>(call(ctx, g22::fn::Mem_DefaultAllocator, {}));
    store<u32>(map + 12, alloc);
    const u32 cap = load<u16>(map);
    if (cap != 0) {
        const u32 size = cap * ((load<u16>(map + 6) >> 1) & 0x7FFF);
        store<u32>(map + 8, static_cast<u32>(vcall(ctx, alloc, 0, {alloc, size, 0, 16})));
    }
    call(ctx, g22::fn::Array_Init, {map});
    store<u32>(map + 20, in::kMapCompare);
    store<u32>(map + 16, 0);

    for (u32 p = first_inst(ctx); p != 0; p = next_inst(p)) {
        if (load<u32>(p + in::kDef) == 0) continue;
        const u32 key = load<u32>(p + in::kKey);
        if (load<u16>(map + 2) == load<u16>(map))
            call(ctx, g22::fn::Array_Reserve, {map, (static_cast<u32>(load<u16>(map)) << 1) & 0xFFFE});
        const u32 n = load<u16>(map + 2), data = load<u32>(map + 8);
        store<u32>(data + 8 * n, key);
        store<u32>(data + 8 * n + 4, p);
        const u32 next = n + 1, high = load<u16>(map + 4), flags = load<u16>(map + 6);
        store<u16>(map + 2, static_cast<u16>(next));
        store<u16>(map + 4, static_cast<u16>(high > (next & 0xFFFF) ? high : next));
        store<u16>(map + 6, static_cast<u16>(flags | 1));
    }

    auto mark_done = [](u32 p) { store<u8>(p + in::kDone322, load<u8>(p + in::kDone322) | in::kDoneBit); };
    for (u32 p = first_inst(ctx); p != 0; p = next_inst(p)) {
        if ((load<u8>(p + in::kDone322) & in::kDoneBit) != 0) continue;
        store<u32>(sp + 112, load<u32>(load<u32>(p + in::kDef) + in::kDefKey));
        const u32 src = load<u32>(static_cast<u32>(call(ctx, g22::fn::MapB_Find, {map, sp + 112})));
        const u32 own = load<u32>(p + in::kKey);
        if (src == 0 || src == p || own == load<u32>(load<u32>(p + in::kDef) + in::kDefKey) ||
            load<u32>(src + in::kKey) != load<u32>(load<u32>(src + in::kDef) + in::kDefKey)) {
            mark_done(p);
            continue;
        }
        const u32 w164 = load<u32>(p + in::kFlags164);
        const u32 b8000 = (w164 & 0x8000) != 0 ? 1 : 0, b4000 = (w164 & 0x4000) != 0 ? 1 : 0;
        if ((load<u32>(p + in::kFlags8) & in::kNamed) != 0 && ((load<u32>(src + in::kWord320) >> 9) & 0xF) != 3) {
            store<u32>(sp + 116, own);
            const u32 slot = static_cast<u32>(
                call(ctx, g22::fn::MapB_Find, {load<u32>(load<u32>(g21::kGeomInstClass) + in::kClassMap), sp + 116}));
            if (load<u32>(slot) == p) {
                store<u32>(sp + 120, load<u32>(p + in::kKey));
                call(ctx, g22::fn::MapB_Remove, {load<u32>(load<u32>(g21::kGeomInstClass) + in::kClassMap), sp + 120});
            }
            store<u32>(p + in::kFlags8, load<u32>(p + in::kFlags8) & ~in::kNamed);
        }
        u32 copy = load<u32>(p + in::kWord12) != 0 ? 61 : 57;
        if ((load<u32>(p + in::kFlags168) & 0x20000) != 0) copy |= 0x40;
        call(ctx, g22::fn::WorldObj_CopyFrom, {p, src, copy});
        for (u32 c = load<u32>(src + in::kChild); c != 0; c = load<u32>(c + in::kSibling)) {
            const u32 clone = static_cast<u32>(vcall(ctx, c, in::kVClone, {c, 21}));
            vcall(ctx, p, in::kVAddChild, {p, clone, ~0ull});
            vcall(ctx, clone, in::kVRelease, {clone});
        }
        call(ctx, g22::fn::Tree_Collect, {scene, load<u32>(p + in::kChild)});
        vcall(ctx, p, in::kVSet40, {p, b8000, 1});
        call(ctx, g22::fn::WorldObj_SetFlag4000, {p, b4000, 1});
        if ((load<u32>(p + in::kFlags168) & 0x20000) != 0)
            call(ctx, g22::fn::WorldObj_ForEach, {p, in::kForEachFn, load<u32>(p + in::kArg292), 0});
        call(ctx, g22::fn::Node_SyncTree, {scene, p});
        vcall(ctx, src, in::kVSet40, {src, 0, 1});
        mark_done(p);
    }

    const u32 size = static_cast<u32>(load<u16>(map)) * ((load<u16>(map + 6) >> 1) & 0x7FFF);
    const u32 data = load<u32>(map + 8);
    if (data != 0) {
        const u32 a = load<u32>(map + 12);
        vcall(ctx, a, 8, {a, data, size});
    }
    ctx.gpr[3] = call(ctx, g22::fn::Array_Destruct, {map});
}

namespace {

namespace df = sly4::render::gfx22::defer;

inline void atomic_put(u32 ea, u32 v) {  // lwarx / stwcx. loop in the original
    std::atomic_ref<u32>(ref<u32>(ea)).exchange(__builtin_bswap32(v));
}

void sleep_while(PPUContext& ctx, u32 flag) {
    while (load<u32>(flag) != 0) syscall(ctx, df::kSysTimerUsleep, {df::kSleepUs});
}

void bump_state(PPUContext& ctx) {
    call(ctx, g22::fn::Draw_InvalidateState, {});
    store<u32>(df::kStateStamp, load<u32>(df::kStateStamp) + 1);
}

void segment_body(PPUContext& ctx, df::Body body) {
    switch (body) {
    case df::Body::Stream0: {
        const u32 s = load<u32>(df::kStreams);
        vcall(ctx, s, 16, {s});
        break;
    }
    case df::Body::Records:
        if (load<u8>(df::kRecListGuard) == 0) {
            store<u8>(df::kRecList, 1);
            call(ctx, g22::fn::PtrVector_Construct, {df::kRecList + 4});
            store<u32>(df::kRecList + 32, 0);
            store<u16>(df::kRecList + 10, 160);
            atomic_put(df::kRecList + 128, 0);
            atomic_put(df::kRecList + 256, 0);
            store<u8>(df::kRecListGuard, 1);
        }
        sleep_while(ctx, df::kRecList + 128);
        call(ctx, g22::fn::DrawQueue_Run, {df::kRecList});
        break;
    case df::Body::Sorted: {
        const u32 n = load<u32>(df::kSortedCount);
        if (n == 0) break;
        call(ctx, g22::fn::SortEntries, {df::kSorted, n, 8});
        sleep_while(ctx, df::kSortedWait);
        for (u32 i = 0; i < load<u32>(df::kSortedCount); ++i) {  // cmplw, the count reloaded
            const u32 obj = load<u32>(df::kSorted + 8 * i + 4);
            const u32 d = load<u32>(obj + 64);
            const u32 rec = load<u32>(df::kSortedBase) + (load<u32>(obj + 76) << 5);
            const u32 opd = load<u32>(load<u32>(d) + 12);  // r8 / r9 = the entry and the OPD
            call_ptr(ctx, opd, {d, obj, load<u32>(obj + 68), load<u32>(obj + 72), rec, load<u32>(opd), opd});
        }
        store<u32>(df::kSortedCount, 0);
        break;
    }
    case df::Body::Overlay:
        if (load<u8>(df::kOverlayOn) != 0) {
            const u32 o = load<u32>(df::kOverlayObj);
            if (o != 0) vcall(ctx, o, 16, {o});
        }
        break;
    case df::Body::Lists1: call(ctx, g22::fn::Draw_Lists, {1}); break;
    case df::Body::Lists2: call(ctx, g22::fn::Draw_Lists, {2}); break;
    case df::Body::Lists3: call(ctx, g22::fn::Draw_Lists, {3}); break;
    case df::Body::Pass: call(ctx, g22::fn::Render_Pass575C34, {}); break;
    }
}

}  // namespace

// void DeferredLists_Flush_00517880()  [called by f_00518E78] (name?)
// With kPending set: waits for kBusy, clears stream 0's +12 bit 0x40,
// pads one NOP, then for each pending segment (defer::kSegments) records
// its commands at the current command pointer (Draw_InvalidateState and
// the state stamp, the segment's body, a CALL back to its +8 offset),
// patches its jump word to JUMP there and clears it. When any segment
// was recorded, the word at the start (before the NOP) becomes a JUMP to
// the last segment's end, and the state is invalidated again. kPending is
// cleared. r3 is left 0 (nothing pending) or 0x00F60000 (the last lis),
// as the original leaves it.
DECOMP(00517880, DeferredLists_Flush_00517880) {
    if (load<u8>(df::kPending) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    Stack frame(ctx, 224);
    const u32 sp = frame.addr();
    sleep_while(ctx, df::kBusy);
    const u32 s0 = load<u32>(df::kStreams);
    store<u8>(s0 + 12, load<u8>(s0 + 12) & ~0x40);
    const u32 start = load<u32>(load<u32>(df::kGcmContext) + 8);
    call(ctx, g22::fn::Gcm_PadNops, {load<u32>(df::kGcmContext), 1});
    u32 end = 0;
    for (const auto& seg : df::kSegments) {
        if (load<u32>(seg.addr) == 0) continue;
        call(ctx, g22::fn::cellGcmAddressToOffset, {load<u32>(load<u32>(df::kGcmContext) + 8), sp + seg.slot_start});
        bump_state(ctx);
        segment_body(ctx, seg.body);
        call(ctx, g22::fn::Gcm_SetCall, {load<u32>(df::kGcmContext), load<u32>(seg.addr + 8)});
        call(ctx, g22::fn::cellGcmAddressToOffset, {load<u32>(load<u32>(df::kGcmContext) + 8), sp + seg.slot_end});
        store<u32>(load<u32>(seg.addr), load<u32>(sp + seg.slot_start) | df::kJump);
        end = load<u32>(sp + seg.slot_end);
        store<u32>(seg.addr, 0);
    }
    if (end != 0) {
        store<u32>(start, end | df::kJump);
        bump_state(ctx);
    }
    store<u8>(df::kPending, 0);
    ctx.gpr[3] = 0x00F60000;
}

// void Instances_UpdateAll_0058F118()  [called by Bank_Start] (name?)
// Every instance of the six classes (lights, geometry, emitters, points,
// cameras, splines, in that order; each free list made on first use):
// WorldObj_UpdateLod(inst, 0, 0) unless its LOD target (+260) is set;
// lights, geometry and emitters also WorldObj_UpdateZoneMask(inst, 1),
// and geometry whose +320 kind is not 3 or 4 has its key (+204) cleared.
// r3 is left 0.
DECOMP(0058F118, Instances_UpdateAll_0058F118) {
    auto each = [&](const in::InstClass& c, bool zones, bool geom) {
        for (u32 p = first_inst(ctx, c); p != 0; p = next_inst(p)) {
            if (load<u32>(p + in::kLodSet) == 0) call(ctx, g22::fn::WorldObj_UpdateLod, {p, 0, 0});
            if (zones) call(ctx, g22::fn::WorldObj_UpdateZoneMask, {p, 1});
            if (geom) {
                const u32 kind = (load<u32>(p + in::kWord320) >> 9) & 0xF;
                if (kind != 3 && kind != 4 && load<u32>(p + in::kKey) != 0) store<u32>(p + in::kKey, 0);
            }
        }
    };
    each(in::kLights, true, false);
    each(in::kGeoms, true, true);
    each(in::kEmitters, true, false);
    each(in::kPoints, false, false);
    each(in::kCameras, false, false);
    each(in::kSplines, false, false);
    ctx.gpr[3] = 0;
}

namespace {

namespace pd = sly4::render::gfx22::pad;

f32 pad_const(u32 i) { return f32_at(pd::kConsts + 4 * i); }

// A stick axis: the dead zone of +-10 around 128, then (v - 10) / 117,
// or ((v + 10) + 1) / 117 below it (the original's asymmetry).
f32 stick(u32 raw) {
    const s32 v = static_cast<s16>(static_cast<u16>(raw - pd::kStickCentre));
    if (v > pd::kDeadZone) return static_cast<f32>(static_cast<f64>(static_cast<f32>(v - pd::kDeadZone)) / pad_const(0));
    if (v < -pd::kDeadZone) {
        const f32 t = static_cast<f32>(static_cast<f64>(static_cast<f32>(v + pd::kDeadZone)) + pad_const(5));
        return static_cast<f32>(static_cast<f64>(t) / pad_const(0));
    }
    return pad_const(1);
}

// A pressure / sensor value: v / scale, 0 when |that| <= 0.01.
f32 scaled(s32 v, f32 scale) {
    const f32 t = static_cast<f32>(static_cast<f64>(static_cast<f32>(v)) / scale);
    return std::fabs(t) <= pad_const(3) ? pad_const(1) : t;
}

}  // namespace

// u32 Pad_ReadState_0056FC28(PadDevice* d)  [called through a pointer] (name?)
// Once per vblank the pad info (cellPadGetInfo2; on failure nothing more).
// A connected port's cellPadGetData into +152 sets +4 bit 15 when it
// succeeds; on new data (len != 0) the buttons are mapped (Pad_MapButtons
// of digital 1 | digital 2 << 8, InputDevice_SetButtonMask), the sticks
// get a dead zone, and with the port's press / sensor settings the
// pressures (/ 255) and sensors ((v - 512) / 511) are read, +128 marking
// which values are valid. +8 = 4 when the port has no capability bits,
// then InputDevice_UpdateButtons, whose r3 is returned.
DECOMP(0056FC28, Pad_ReadState_0056FC28) {
    const u32 d = arg<u32>(ctx, 0);
    const u32 info = pd::kInfoBlock;
    const u32 vb = static_cast<u32>(call(ctx, g22::fn::cellGcmGetVBlankCount, {}));
    if (load<u32>(info + pd::kLastVblank) != vb) {
        store<u32>(info + pd::kLastVblank, vb);
        const u64 r = call(ctx, g22::fn::cellPadGetInfo2, {info + pd::kInfo});
        if (static_cast<u32>(r) != 0) return;
    }
    const u32 data = d + pd::kDevData;
    u32 ok = 0;
    {
        const u32 port = load<u8>(d + pd::kDevPort);
        if ((load<u32>(info + pd::kPortStatus + 4 * port) & pd::kStatusConnected) != 0)
            ok = static_cast<u32>(call(ctx, g22::fn::cellPadGetData, {port, data})) == 0 ? 1 : 0;
    }
    store<u16>(d + pd::kDevConnected, static_cast<u16>((load<u16>(d + pd::kDevConnected) & 0x7FFF) | (ok << 15)));
    if (ok != 0 && load<u32>(data) != 0) {
        store<u32>(d + pd::kDevButtons, 0);
        store<u32>(d + 20, 0);
        store<u32>(d + 24, 0);
        call(ctx, g22::fn::memset, {d + pd::kDevSticks, 0, 96});
        const u32 raw = (load<u16>(data + 8) & 0xFF) | ((load<u16>(data + 10) & 0xFF) << 8);
        store<u32>(d + pd::kDevValid, 0);
        const u32 mask = static_cast<u32>(call(ctx, g22::fn::Pad_MapButtons, {d, raw}));
        store<u32>(d + pd::kDevButtons, mask);
        call(ctx, g22::fn::InputDevice_SetButtonMask, {d, mask});
        u32 valid = load<u32>(d + pd::kDevValid) | 0xF;
        store<u32>(d + pd::kDevValid, valid);
        const u32 port = load<u8>(d + pd::kDevPort);
        constexpr u32 kSticks[4] = {16, 18, 12, 14};  // LX, LY, RX, RY
        for (u32 k = 0; k < 4; ++k) put_f32(d + pd::kDevSticks + 4 * k, stick(load<u16>(data + kSticks[k])));
        ctx.fpr[1] = pad_const(0);
        const u32 setting = load<u32>(info + pd::kPortSetting + ((4 * port) & 0x3FC));
        if ((setting & pd::kSettingSensor) != 0) {
            valid |= 0x003C0000;
            store<u32>(d + pd::kDevValid, valid);
            for (u32 k = 0; k < 4; ++k) {
                const s32 v = static_cast<s16>(static_cast<u16>(load<u16>(data + 44 + 2 * k) - pd::kSensorCentre));
                put_f32(d + pd::kDevSensors + 4 * k, scaled(v, pad_const(4)));
            }
            ctx.fpr[1] = pad_const(3);
        }
        if ((setting & pd::kSettingPress) != 0) {
            store<u32>(d + pd::kDevValid, valid | 0xF0);
            for (u32 k = 0; k < 12; ++k) {
                if (k == 4) store<u32>(d + pd::kDevValid, valid | 0xFF0);
                if (k == 8) store<u32>(d + pd::kDevValid, valid | 0xFF0 | 0xF000);
                const s32 v = static_cast<s16>(load<u16>(data + pd::kPressRead[k]));
                put_f32(d + pd::kDevPress + 4 * k, scaled(v, pad_const(2)));
            }
            ctx.fpr[1] = pad_const(3);
        }
    }
    if (ok != 0) {  // with or without new data
        const u32 port = load<u8>(d + pd::kDevPort);
        store<u32>(d + pd::kDevWord8, load<u32>(info + pd::kCapability + 4 * port) != 0 ? 0 : 4);
    }
    ctx.gpr[3] = call(ctx, g22::fn::InputDevice_UpdateButtons, {d});
}

namespace {

namespace tr = sly4::render::gfx22::trophy;

u32 trophy_mgr() { return load<u32>(tr::kManager); }

// Sets bit (s16)id of the unlocked-trophy bitmap at t +144.
void trophy_set_bit(u32 t, u32 id) {
    const u32 i = static_cast<u32>(static_cast<s32>(static_cast<s16>(id)));
    const u32 w = t + tr::kBits + ((i >> 3) & 0x1FFFFFFCu);
    store<u32>(w, load<u32>(w) | (1u << (i & 31)));
}

// The manager's unlock state copied under its lock (when the state reads).
void trophy_copy_state(PPUContext& ctx, u32 sp, u32 flags, u32 count) {
    const u32 m = trophy_mgr();
    if (load<u8>(m + tr::kReady) == 0) return;
    for (u32 k = 0; k < 16; k += 4) store<u32>(sp + flags + k, 0);
    const u64 r = call(ctx, g22::fn::sceNpTrophyGetTrophyUnlockState,
                       {load<u32>(m + tr::kMgrContext), load<u32>(m + tr::kMgrHandle), sp + flags, sp + count});
    if (static_cast<s32>(r) < 0) return;
    call(ctx, g22::fn::Lock_Enter, {m + tr::kLockState});
    for (u32 k = 0; k < 16; k += 4) store<u32>(m + tr::kStateCopy + k, load<u32>(sp + flags + k));
    call(ctx, g22::fn::Lock_Leave, {m + tr::kLockState});
}

}  // namespace

// u32 Trophy_ThreadMain_004FB704(TrophyThreadArgs* t)  [the TrophyThread entry; t = manager + 24] (name?)
// Runs the trophy state machine (t +84), sleeping 1 ms between steps,
// until t +68 is set; returns 0.
//  0: after 5 s registers the context (stop if the manager's +94 abort
//     flag is set or it fails, else state 1 and t +69).
//  1 (once registered): the game info; with manager +11 the trophy list
//     (each trophy's name hash and id appended to the manager's Array,
//     +10 after the last; too many for its capacity, or a failed info,
//     stops), then state 4, ready, and the unlock state copied.
//  2: the required disk space (stop on failure) into manager +16, then
//     state 3 with a status callback, else state 0 and +13.
//  3: the callback asked for the free space until it answers; not enough
//     shows the no-space error dialog (+12 meanwhile) and stops, else
//     state 0 and +13.
//  4: the unlocked bits read from the unlock state, then state 5.
//  5: when an id is pending (manager +164) and ready, the next pooled id
//     is taken under the lock and unlocked (its bit and the unlock
//     callback on success; known errors logged), a new platinum's bit and
//     callback, then the unlock state copied. Nothing pending sleeps twice.
//  6: manager +13 and t +68 set (the thread ends).
DECOMP(004FB704, Trophy_ThreadMain_004FB704) {
    const u32 t = arg<u32>(ctx, 0);
    Stack frame(ctx, 1472);
    const u32 sp = frame.addr();
    auto usleep = [&] { syscall(ctx, tr::kSysTimerUsleep, {tr::kSleepUs}); };
    auto set_state = [&](u32 s) { store<u32>(t + tr::kState, s); };
    while (load<u8>(t + tr::kQuit) == 0) {
        const s32 state = static_cast<s32>(load<u32>(t + tr::kState));
        if (state == tr::kStop) {
            store<u8>(trophy_mgr() + tr::kIdle, 1);
            store<u8>(t + tr::kQuit, 1);
        } else if (state == tr::kRun) {
            const u32 m = trophy_mgr();
            if (load<u32>(m + tr::kPending) == 0) {
                usleep();
            } else if (load<u8>(m + tr::kReady) != 0) {
                call(ctx, g22::fn::Lock_Enter, {m + tr::kLockPool});
                s32 id = -1;
                u32 pending = load<u32>(m + tr::kPoolCount);
                if (pending != 0) {
                    const u32 e = load<u32>(m + tr::kPoolEntries) + (load<u32>(m + tr::kPoolIndex) << 4);
                    id = static_cast<s32>(load<u32>(e + 8));
                    call(ctx, g22::fn::Pool_AllocEntry, {m + tr::kPool});
                    pending = load<u32>(m + tr::kPoolCount);
                }
                store<u32>(m + tr::kPending, pending);
                call(ctx, g22::fn::Lock_Leave, {m + tr::kLockPool});
                if (id >= 0) {
                    store<u32>(sp + 156, 0xFFFFFFFFu);
                    const u32 r = static_cast<u32>(call(ctx, g22::fn::sceNpTrophyUnlockTrophy,
                                                         {load<u32>(t + tr::kContext), load<u32>(t + tr::kHandle), sx(id), sp + 156}));
                    if (static_cast<s32>(r) >= 0) {
                        trophy_set_bit(t, static_cast<u32>(id));
                        const u32 cb = load<u32>(t + tr::kOnUnlock);
                        if (cb != 0) call_ptr(ctx, cb, {sx(id), cb, load<u32>(cb)});
                    } else if (r == tr::kErrAlreadyUnlocked || r == tr::kErrPlatinumUnlocked) {
                    } else if (r == tr::kErrCannotUnlockPlatinum) {
                        call(ctx, g22::fn::String_Format, {tr::kStrPlatinum, sx(id)});
                    } else if (r == tr::kErrInvalidTrophyId) {
                        call(ctx, g22::fn::String_Format, {tr::kStrNotExist, sx(id)});
                    } else {
                        call(ctx, g22::fn::String_Format, {tr::kStrUnlockFail, sx(id)});
                    }
                    const u32 plat = load<u32>(sp + 156);
                    if (static_cast<s32>(plat) != -1) {
                        trophy_set_bit(t, plat);
                        const u32 cb = load<u32>(t + tr::kOnPlatinum);
                        if (cb != 0) call_ptr(ctx, cb, {sx(plat), cb, load<u32>(cb)});
                    }
                    trophy_copy_state(ctx, sp, 184, 160);
                }
            }
        } else if (state == tr::kReadState) {
            store<u32>(sp + 152, 0);
            call(ctx, g22::fn::sceNpTrophyGetTrophyUnlockState, {load<u32>(t + tr::kContext), load<u32>(t + tr::kHandle), sp + 168, sp + 152});
            const s32 n = static_cast<s32>(load<u32>(sp + 152));
            for (s32 i = 0; i < n; ++i)
                if ((load<u32>(sp + 168 + 4 * static_cast<u32>(i >> 5)) & (1u << (i & 31))) != 0) trophy_set_bit(t, static_cast<u32>(i));
            set_state(tr::kRun);
        } else if (state == tr::kCheckSpace) {
            store<u64>(sp + 120, 0);
            store<u64>(sp + 128, 0);
            for (;;) {
                const u32 cb = load<u32>(t + tr::kOnStatus);
                if (static_cast<u32>(call_ptr(ctx, cb, {sp + 120, sp + 128, 0, cb, load<u32>(cb)})) != 0) break;
                usleep();
                if (load<u8>(t + tr::kQuit) != 0) break;
            }
            const u32 m = trophy_mgr();
            const u64 need = static_cast<u32>((load<u64>(m + tr::kRequired) >> 10) + load<u64>(sp + 120));
            if (need > load<u64>(sp + 128)) {
                store<u8>(m + tr::kDialog, 1);
                call(ctx, g22::fn::cellGameContentErrorDialog,
                     {tr::kErrorDialogNoSpaceExit, sx(static_cast<u32>(need - load<u64>(sp + 128))), 0});
                set_state(tr::kStop);
                const u32 cb = load<u32>(t + tr::kOnStatus);
                if (cb != 0) call_ptr(ctx, cb, {sp + 120, sp + 128, 1, cb, load<u32>(cb)});
                store<u8>(trophy_mgr() + tr::kDialog, 0);
            } else {
                set_state(tr::kRegister);
                store<u8>(m + tr::kIdle, 1);
            }
        } else if (state == tr::kSpace) {
            const u32 r = static_cast<u32>(call(ctx, g22::fn::sceNpTrophyGetRequiredDiskSpace,
                                                 {load<u32>(t + tr::kContext), load<u32>(t + tr::kHandle), sp + 112, 0}));
            if (r != 0) {
                set_state(tr::kStop);
            } else {
                store<u64>(trophy_mgr() + tr::kRequired, load<u64>(sp + 112));
                store<u8>(trophy_mgr() + tr::kSpaceKnown, 1);
                if (load<u32>(t + tr::kOnStatus) != 0) {
                    set_state(tr::kCheckSpace);
                } else {
                    set_state(tr::kRegister);
                    store<u8>(trophy_mgr() + tr::kIdle, 1);
                }
            }
        } else if (state == tr::kListTrophies) {
            if (load<u8>(t + tr::kRegistered) != 0) {
                const u32 r = static_cast<u32>(call(ctx, g22::fn::sceNpTrophyGetGameInfo,
                                                     {load<u32>(t + tr::kContext), load<u32>(t + tr::kHandle), trophy_mgr() + tr::kDetails, sp + 216}));
                bool stop = static_cast<s32>(r) < 0;
                if (!stop && load<u8>(trophy_mgr() + tr::kWantList) != 0) {
                    const u32 m = trophy_mgr();
                    const u32 n = load<u32>(m + tr::kDetails) & 0xFFFF, cap = load<u16>(m + tr::kList);
                    if (n > cap) {
                        call(ctx, g22::fn::String_Format, {tr::kStrTooMany, rt::divwu(n, cap)});
                        stop = true;
                    } else {
                        for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(n); i = (i + 1) & 0xFFFF) {
                            if (load<u8>(t + tr::kQuit) != 0) break;
                            const u32 ri = static_cast<u32>(call(ctx, g22::fn::sceNpTrophyGetTrophyInfo,
                                                                  {load<u32>(t + tr::kContext), load<u32>(t + tr::kHandle), i, sp + 236, sp + 136}));
                            if (ri == tr::kErrHidden) continue;
                            if (static_cast<s32>(ri) < 0) {
                                call(ctx, g22::fn::String_Format, {tr::kStrInfoFail, i});
                                stop = true;
                                break;
                            }
                            const u32 h = static_cast<u32>(call(ctx, g22::fn::String_HashNoCase, {sp + 244}));
                            const u32 a = trophy_mgr() + tr::kList;  // Array {+0 capacity, +2 count, +4 high, +6 flags, +8 data}
                            if (load<u16>(a + 2) == load<u16>(a))
                                call(ctx, g22::fn::Array_Reserve, {a, (static_cast<u32>(load<u16>(a)) << 1) & 0xFFFE});
                            const u32 c = load<u16>(a + 2), data = load<u32>(a + 8);
                            store<u32>(data + 8 * c, h);
                            store<u32>(data + 8 * c + 4, i);
                            const u32 next = c + 1, high = load<u16>(a + 4), fl = load<u16>(a + 6);
                            store<u16>(a + 2, static_cast<u16>(next));
                            store<u16>(a + 4, static_cast<u16>(high > (next & 0xFFFF) ? high : next));
                            store<u16>(a + 6, static_cast<u16>(fl | 1));
                            if (i == n - 1) store<u8>(trophy_mgr() + tr::kListDone, 1);
                        }
                    }
                }
                if (stop) {
                    set_state(tr::kStop);
                } else {
                    set_state(tr::kReadState);
                    store<u8>(trophy_mgr() + tr::kReady, 1);
                    trophy_copy_state(ctx, sp, 200, 164);
                }
            }
        } else if (state == tr::kRegister) {
            syscall(ctx, tr::kSysTimerSleep, {tr::kRegisterDelay});
            const u32 r = static_cast<u32>(call(ctx, g22::fn::sceNpTrophyRegisterContext,
                                                 {load<u32>(t + tr::kContext), load<u32>(t + tr::kHandle), tr::kStatusCallback, 0, 1}));
            const u32 m = trophy_mgr();
            if (load<u8>(m + tr::kAbort) != 0 || static_cast<s32>(r) < 0) {
                store<u8>(m + tr::kReady, 0);
                set_state(tr::kStop);
            } else {
                set_state(tr::kListTrophies);
                store<u8>(t + tr::kRegistered, 1);
            }
        }
        usleep();
    }
    ctx.gpr[3] = 0;
}

namespace {

namespace bt = sly4::render::gfx22::batch;

u32 batch_at(u32 k) { return bt::kBatches + bt::kBatchSize * k; }

// A sub-mesh appended to the batch (index n = its +208); +264 / +268
// take the last one's +68 / +76.
void batch_add_submesh(u32 b, u32 w) {
    const u32 nb = load<u8>(w + 109);
    store<u8>(b + 212 + load<u32>(b + 208), static_cast<u8>(nb));
    for (u32 j = 0; j < nb; ++j) store<u32>(b + 232 + 4 * load<u32>(b + 208) + 4 * j, w + 24 + 16 * j);
    store<u32>(b + 240 + 4 * load<u32>(b + 208), load<u32>(w + 40));
    store<u32>(b + 248 + 4 * load<u32>(b + 208), w + 44);
    store<u32>(b + 256 + 4 * load<u32>(b + 208), load<u32>(w + 64));
    store<u32>(b + 216 + 4 * load<u32>(b + 208), w);
    store<u16>(w + 4, static_cast<u16>(load<u16>(w + 4) + 1));
    store<u32>(b + 208, load<u32>(b + 208) + 1);
    store<u32>(b + 264, load<u32>(w + 68));
    store<u32>(b + 268, load<u32>(w + 76));
}

// The first sub-mesh of a mesh list (+48 -> {SubMesh*, ...}).
u32 first_submesh(u32 list) { return load<u32>(load<u32>(list + 48)); }

}  // namespace

// void GeomBatches_Build_00595968()  [called by Bank_Start] (name?)
// With kBusy set for the duration: every batchable geometry instance
// joins the batch of its def (the first instance of a def starts one:
// render-state defaults, the def's sphere and name, the material's
// state, textures and colours, its sub-meshes (one, or two for a def
// whose +112 is not 1)); the instance's +276 / +280 name its batch. Each
// batch then gets a zeroed array of its instance count of 80-byte
// records, filled in reverse instance order with the instance's matrix
// rows (scaled by +144) and the def's sphere transformed by them, the
// instance's vfunc +8 called after. r3 is left 0x01170000.
// Quirks: no limit on the 32 batches or the 32 stack slots for their
// defs (the 33rd overwrites the first collected instance).
DECOMP(00595968, GeomBatches_Build_00595968) {
    Stack frame(ctx, bt::kFrame);
    const u32 sp = frame.addr();
    const u8 was_busy = load<u8>(bt::kBusy);
    store<u8>(bt::kBusy, 1);
    store<u32>(bt::kBatchCount, 0);
    u32 n = 0;  // instances collected
    for (u32 p = first_inst(ctx); p != 0; p = next_inst(p)) {
        if (static_cast<u32>(call(ctx, g22::fn::WorldObj_CanBatch, {p})) == 0) continue;
        const u32 count = load<u32>(bt::kBatchCount), def = load<u32>(p + in::kDef);
        s32 found = -1;
        for (s32 k = static_cast<s32>(count) - 1; count != 0 && k >= 0; --k)
            if (load<u32>(sp + bt::kDefs + 4 * static_cast<u32>(k)) == def) { found = k; break; }
        if (found >= 0) {
            const u32 b = batch_at(static_cast<u32>(found));
            store<u32>(b + 64, load<u32>(b + 64) + 1);
            store<u32>(p + bt::kInstBatchSet, 0);
            store<u32>(p + bt::kInstBatch, static_cast<u32>(found));
        } else {
            store<u32>(sp + bt::kDefs + 4 * count, def);
            const u32 b = batch_at(count);
            store<u32>(b + 76, 515);
            store<u8>(b + 80, 0);
            store<u32>(b + 84, 2304);
            store<u8>(b + 88, 0);
            store<u32>(b + 92, 1);
            store<u32>(b + 96, 0);
            store<u32>(b + 100, 0x8006);
            store<u32>(b + 104, 0);
            for (u32 o : {180u, 184u, 188u, 192u, 196u}) store<u32>(b + o, 1);
            store<u32>(b + 208, 0);
            put_f32(b + 4, static_cast<f32>(static_cast<f64>(f32_at(bt::kOne)) / static_cast<f64>(f32_at(def + 120))));
            store<u32>(b + 12, load<u32>(def + 124));
            for (u32 k = 0; k < 16; k += 4) store<u32>(b + 16 + k, load<u32>(def + 48 + k));
            const u32 name = static_cast<u32>(vcall(ctx, def, 12, {def}));
            call(ctx, g22::fn::String_Assign, {b + 32, name});
            store<u32>(b + 72, 0);
            store<u32>(b + 64, 1);
            store<u32>(b + 68, 0);
            const u32 mesh = load<u32>(def + 132);
            const u32 mat = load<u32>(load<u32>(mesh + 48) + 4), st = mat + 104;
            store<u32>(b + 76, (load<u32>(st + 8) >> 15) & 0xFFFF);
            store<u8>(b + 80, static_cast<u8>(load<u32>(st + 8) >> 31));
            store<u32>(b + 84, load<u16>(st + 12));
            store<u8>(b + 88, static_cast<u8>(load<u32>(st) >> 31));
            store<u32>(b + 92, (load<u32>(st) >> 15) & 0xFFFF);
            store<u32>(b + 100, load<u16>(st + 4));
            store<u32>(b + 96, load<u16>(st + 6));
            const u32 tex = load<u32>(mat + 20);
            store<u32>(b + 176, tex + 56);
            store<u32>(b + 192, load<u16>(tex + 122));
            store<u32>(b + 196, load<u16>(tex + 124));
            store<u32>(b + 180, load<u16>(tex + 116));
            store<u32>(b + 184, load<u16>(tex + 118));
            store<u32>(b + 188, load<u16>(tex + 120));
            const u32 prog = load<u32>(load<u32>(load<u32>(mat + 16) + 12));
            store<u32>(b + 200, prog + 4);
            store<u32>(b + 204, prog + 8);
            const u32 col = load<u32>(mat + 124);
            store<u32>(b + 104, col != 0 ? load<u32>(col) : 0);
            store<u32>(b + 108, 0);
            store<u32>(b + 112, (load<u8>(mat + 100) & 0x40) != 0 ? 1 : 0);
            const u32 zero = load<u32>(bt::kZero);  // lfs / stfs pairs: raw words
            auto colour = [&](u32 off) { return load<u32>(mat + 124) != 0 ? load<u32>(load<u32>(mat + 124) + off) : zero; };
            store<u32>(b + 128, colour(4));
            store<u32>(b + 132, 0x3F800000);
            store<u32>(b + 136, colour(8));
            store<u32>(b + 144, colour(12));
            store<u32>(b + 148, 0x3F800000);
            store<u32>(b + 152, colour(16));
            store<u32>(b + 160, colour(20));
            store<u32>(b + 164, 0x3F800000);
            store<u32>(b + 168, colour(24));
            store<u32>(b + 108, load<u8>(p + bt::kInstFlags321) & 1);
            auto sq = [](u32 w) { const f32 v = std::bit_cast<f32>(w); return static_cast<f32>(static_cast<f64>(v) * v); };
            // def +132 / +140: {Mesh*, f32 radius} for the two LODs (+112 == 1: one).
            if (load<u8>(def + 112) == 1) {
                const u32 r = load<u32>(def + 136);
                store<u32>(b + 0, r);
                put_f32(b + 8, sq(r));
                put_f32(b + 224, sq(load<u32>(def + 136)));
                put_f32(b + 228, sq(load<u32>(def + 136)));
                batch_add_submesh(b, first_submesh(load<u32>(def + 132)));
            } else {
                const u32 r = load<u32>(def + 144);
                store<u32>(b + 0, r);
                put_f32(b + 8, sq(r));
                put_f32(b + 224, sq(load<u32>(def + 136)));
                put_f32(b + 228, sq(load<u32>(def + 144)));
                batch_add_submesh(b, first_submesh(load<u32>(def + 132)));
                batch_add_submesh(b, first_submesh(load<u32>(def + 140)));
            }
            store<u32>(p + bt::kInstBatchSet, 0);
            store<u32>(p + bt::kInstBatch, load<u32>(bt::kBatchCount));
            store<u32>(bt::kBatchCount, load<u32>(bt::kBatchCount) + 1);
        }
        store<u32>(sp + bt::kInsts + 4 * n, p);
        ++n;
    }
    for (s32 k = 0; static_cast<s32>(load<u32>(bt::kBatchCount)) > k; ++k) {
        const u32 b = batch_at(static_cast<u32>(k));
        const u32 a = static_cast<u32>(call(ctx, g22::fn::Mem_Alloc, {load<u32>(b + 64) * bt::kRecordSize, 0, 16}));
        store<u32>(b + 72, static_cast<u32>(call(ctx, g22::fn::memset, {a, 0, load<u32>(b + 64) * bt::kRecordSize})));
    }
    for (u32 i = n; i-- != 0;) {
        const u32 p = load<u32>(sp + bt::kInsts + 4 * i);
        const u32 b = batch_at(load<u32>(p + bt::kInstBatchSet) == 0 ? load<u32>(p + bt::kInstBatch) : 0);
        const u32 m = p + bt::kInstMatrix, tmp = sp + 112;
        const u32 used = load<u32>(b + 68);
        store<u32>(b + 68, used + 1);
        const u32 rec = load<u32>(b + 72) + bt::kRecordSize * used;
        constexpr u32 kRows[12] = {0, 4, 8, 16, 20, 24, 32, 36, 40, 48, 52, 56};
        for (u32 o : kRows) store<u32>(tmp + o, load<u32>(m + o));
        call(ctx, g22::fn::Matrix_ScaleRows, {tmp, p + bt::kInstScale});
        for (u32 o : kRows) store<u32>(rec + o, load<u32>(tmp + o));
        for (u32 k = 0; k < 16; k += 4) store<u32>(rec + 64 + k, load<u32>(b + 16 + k));
        ctx.fpr[1] = render::gfx10::lfs_value(load<u32>(b + 28));
        call(ctx, g22::fn::Sphere_Transform, {rec + 64, rec, 1});
        vcall(ctx, p, 8, {p});
    }
    store<u8>(bt::kBusy, was_busy);
    ctx.gpr[3] = 0x01170000;  // the lis of kBusy's store, as the original leaves it
}

namespace {

namespace rf = sly4::render::gfx22::refl;

u32 trunc_u32(f32 v) { return static_cast<u32>(rt::fctid(static_cast<f64>(v), true)); }  // fctidz, low word
f32 mul_s(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) * static_cast<f64>(b)); }

// The parameter's data offset in the sorted hash list (the scan stops at
// a larger hash), or -1.
s32 param_find(u32 params, u32 hash) {
    const s32 n = static_cast<s32>(load<u16>(params + 12));
    const u32 list = load<u32>(params + 16);
    for (s32 i = 0; i < n; ++i) {
        const u32 v = load<u32>(list + 4 * static_cast<u32>(i));
        if (v == hash) return static_cast<s32>(load<u16>(load<u32>(params + 20) + 2 * static_cast<u32>(i)));
        if (v > hash) break;
    }
    return -1;
}

}  // namespace

// u32 Reflection_Create_005A9DB0(Obj* scene, Obj* o, u32 arg, const ParamBlock* params)  [called by Call5A9DB0_005AC7AC] (name?)
// The next reflection record (refl::): its size fits the config size to
// the aspect of o's def box (+208 -> +16 min / +32 max), for each of the
// three levels (halved, at least 1) a target attached over the depth
// holder's target and one over the memory stats' texture, the level's
// surface given the second and a colour texture made over the first.
// Then a ShadowSource on level 0's texture, named "Reflection", with
// +256 = 0, +660 = kSourceCallback, SetByte49(2), and two settings
// blocks (+128 / +192: u8, f32 +8 / +12, colour bytes +2 and as f32 / 255
// at +16) from the parameters (defaults 0, 10 / 100 (also the second's),
// white); record {+0 0, +4 scene, +8 o, +12 arg, +16 f32 (30), +20 f32
// (50), +24 the source, +40 3}. Returns o's vfunc +40(0, 1) (r8 = 3 left).
DECOMP(005A9DB0, Reflection_Create_005A9DB0) {
    const u32 scene = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), a2 = arg<u32>(ctx, 2), params = arg<u32>(ctx, 3);
    Stack frame(ctx, 320);
    const u32 sp = frame.addr();
    const u32 idx = load<u32>(rf::kCount);
    store<u32>(rf::kCount, idx + 1);
    const u32 box = load<u32>(o + 208) + 16;
    const f32 width = static_cast<f32>(static_cast<f64>(f32_at(box + 16)) - f32_at(box + 0));
    const f32 height = static_cast<f32>(static_cast<f64>(f32_at(box + 20)) - f32_at(box + 4));
    const f32 aspect = static_cast<f32>(static_cast<f64>(height) / width);
    const u32 rec = rf::kRecords + rf::kRecordSize * idx;
    const u32 cw = load<u16>(rf::kConfig + 4), chh = load<u16>(rf::kConfig + 6);
    const f32 inv = static_cast<f32>(static_cast<f64>(f32_at(rf::kConsts)) / aspect);
    u32 w = cw, h = chh;
    if (width < height) {
        w = trunc_u32(mul_s(static_cast<f32>(h), inv));
        if (w > cw) {
            w = cw;
            h = trunc_u32(mul_s(static_cast<f32>(w), aspect));
        }
    } else {
        h = trunc_u32(mul_s(static_cast<f32>(w), aspect));
        if (h > chh) {
            h = chh;
            w = trunc_u32(mul_s(static_cast<f32>(h), inv));
        }
    }
    for (u32 k = 0; k < rf::kLevels; ++k) {
        store<u32>(sp + 116, load<u32>(load<u32>(rf::kDepthHolder) + 72));
        call(ctx, g22::fn::Target_Attach, {sp + 112, sp + 116, w, h, 0, 0});
        const u32 tex = static_cast<u32>(call(ctx, g22::fn::Stats_ResolveTexture, {rf::kMemStats}));
        store<u32>(sp + 120, 0);
        call(ctx, g22::fn::Target_Attach, {sp + 120, tex + 40, w, h, 0, 0});
        const u32 surf = rec + rf::kLevelSurface + rf::kLevelSurfaceSize * k;
        call(ctx, g22::fn::Surface_SetBuffer, {surf, load<u32>(sp + 120)});
        const u32 ct = static_cast<u32>(call(ctx, g22::fn::RenderTarget_ColourTexture, {surf, load<u32>(sp + 112)}));
        store<u32>(rec + rf::kLevelTexture + 4 * k, ct);
        w = (w >> 1) > 1 ? (w >> 1) : 1;
        h = (h >> 1) > 1 ? (h >> 1) : 1;
    }
    // The settings, defaults then the parameters found.
    u32 f16 = load<u32>(rf::kConsts + 32), f0a = load<u32>(rf::kConsts + 36), f1a = load<u32>(rf::kConsts + 40);
    u32 f20 = load<u32>(rf::kConsts + 44), f0b = f0a, f1b = f1a;
    u8 ca[4], cb[4];
    for (u32 k = 0; k < 4; ++k) ca[k] = cb[k] = load<u8>(rf::kWhite + k);
    u8 u8a = 0, u8b = 0;
    const u32 data = load<u32>(params + 24);
    auto get = [&](u32 hash) { return param_find(params, hash); };
    s32 off;
    if ((off = get(rf::kHashF16)) >= 0) f16 = load<u32>(data + off);
    if ((off = get(rf::kHashU8A)) >= 0) u8a = load<u8>(data + off);
    if ((off = get(rf::kHashF0A)) >= 0) f0a = load<u32>(data + off);
    if ((off = get(rf::kHashF1A)) >= 0) f1a = load<u32>(data + off);
    if ((off = get(rf::kHashColA)) >= 0) for (u32 k = 0; k < 4; ++k) ca[k] = load<u8>(data + off + k);
    if ((off = get(rf::kHashU8B)) >= 0) u8b = load<u8>(data + off);
    if ((off = get(rf::kHashF0B)) >= 0) f0b = load<u32>(data + off);
    if ((off = get(rf::kHashF1B)) >= 0) f1b = load<u32>(data + off);
    if ((off = get(rf::kHashColB)) >= 0) for (u32 k = 0; k < 4; ++k) cb[k] = load<u8>(data + off + k);
    if ((off = get(rf::kHashF20)) >= 0) f20 = load<u32>(data + off);

    const u32 src = static_cast<u32>(call(ctx, g22::fn::ShadowSource_Create, {load<u32>(rec + rf::kLevelTexture)}));
    call(ctx, g22::fn::String_Copy, {src + 16, rf::kName, 32});
    store<u32>(src + 256, 0);
    store<u32>(src + 660, rf::kSourceCallback);
    call(ctx, g22::fn::RenderTarget_SetByte49, {src, 2});
    const f32 scale = f32_at(rf::kConsts + 48);
    f32 last = 0.0f;
    auto settings = [&](u32 base, u8 b, u32 f0, u32 f1, const u8* c) {
        store<u8>(base, b);
        store<u32>(base + 8, f0);
        store<u32>(base + 12, f1);
        for (u32 k = 0; k < 4; ++k) store<u8>(base + 2 + k, c[k]);
        for (u32 k = 0; k < 4; ++k) put_f32(base + 16 + 4 * k, last = mul_s(static_cast<f32>(c[k]), scale));
    };
    settings(src + 128, u8a, f0a, f1a, ca);
    settings(src + 192, u8b, f0b, f1b, cb);
    store<u32>(rec + 24, src);
    store<u32>(rec + 4, scene);
    store<u32>(rec + 8, o);
    store<u32>(rec + 12, a2);
    store<u32>(rec + 0, 0);
    store<u32>(rec + 16, f16);
    store<u32>(rec + 20, f20);
    store<u32>(rec + 40, 3);
    ctx.fpr[1] = last;
    const u32 opd = load<u32>(load<u32>(o) + 40);
    ctx.gpr[3] = call_ptr(ctx, opd, {o, 0, 1, opd, load<u32>(opd), 3});
}
