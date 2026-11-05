// The renderer range's next recompiled functions: the callees gfx14*.cpp
// left (the effect slot texture pick, the emitter check, ...) and the
// next smallest ones. Names carry the address where the role is unclear.
#include <bit>
#include <cmath>

#include "fx.h"
#include "gfx15.h"

using namespace sly4;

namespace {

namespace g11 = sly4::render::gfx11;
namespace g14 = sly4::render::gfx14;
namespace g15 = sly4::render::gfx15;
using g11::lfs;
using g11::sgl;

}  // namespace

// Texture* EffectSlot_Texture_005AB5F0(Texture** list, u32 count, Owner* owner, View* view)
// (EffectSlot_Submit_005AB908; name?)
// Picks the slot's texture by its size on screen: the four corners
// {(a or b).x, (a or b).y, 0} of the owner's rectangle (a = owner->+208
// +16, b = +32) go through owner+80 x view+880 (rows read with lvx, 3x3
// times the rows plus row 3, fused per lane in vmaddfp order) and the
// divide by w; n = (u32) fctidz((max x - min x) * view->+512->+56) (at
// least 1). The list is sorted by u16 +52, largest first: the result is
// the entry before the first one smaller than n (the first entry if it
// is the first, the last if none is), clamped to count - 1. A count of 0
// returns list[0] (unchecked). f1 is left with fctidz's bits.
DECOMP(005AB5F0, EffectSlot_Texture_005AB5F0) {
    constexpr u32 K = g15::kSlotTextureConsts;
    const u32 list = arg<u32>(ctx, 0), count = arg<u32>(ctx, 1), owner = arg<u32>(ctx, 2), view = arg<u32>(ctx, 3);
    Stack frame(ctx, 128);
    const u32 in = frame.addr(), out = in + 16, a = in + 32, b = in + 48, mtx = in + 64;
    const u32 rect = load<u32>(owner + 208) + 16;
    for (u32 k = 0; k < 3; ++k) store<u32>(a + 4 * k, load<u32>(rect + 4 * k));
    for (u32 k = 0; k < 3; ++k) store<u32>(b + 4 * k, load<u32>(rect + 16 + 4 * k));
    const u32 m = (owner + 80) & ~15u, v = (view + 880) & ~15u;  // lvx
    for (u32 i = 0; i < 4; ++i) {
        const f32 x = load<f32>(m + 16 * i), y = load<f32>(m + 16 * i + 4), z = load<f32>(m + 16 * i + 8);
        for (u32 k = 0; k < 4; ++k) {
            f32 r = std::fmaf(load<f32>(v + 4 * k), x, i == 3 ? load<f32>(v + 48 + 4 * k) : 0.0f);
            r = std::fmaf(load<f32>(v + 16 + 4 * k), y, r);
            store<f32>(mtx + 16 * i + 4 * k, std::fmaf(load<f32>(v + 32 + 4 * k), z, r));
        }
    }
    f64 lo = lfs(K), hi = lfs(K + 4);
    const f64 one = lfs(K + 8);
    for (u32 i = 0; i < 4; ++i) {
        store<u32>(in, load<u32>(((i & 2) != 0 ? b : a)));        // rlwinm 3,27,27
        store<u32>(in + 8, 0);
        store<u32>(in + 4, load<u32>(((i & 1) != 0 ? b : a) + 4));  // rlwinm 4,27,27
        call(ctx, g15::fn::Vec_TransformByMatrix_553A2C, {out, in, mtx});
        const f64 w = lfs(out + 12);
        const f64 s = sgl(one / w);  // fdivs
        const f64 x = sgl(lfs(out) * s), y = sgl(lfs(out + 4) * s), z = sgl(lfs(out + 8) * s), ww = sgl(w * s);
        store<u32>(out, g11::stfs_bits(x));
        store<u32>(out + 4, g11::stfs_bits(y));
        store<u32>(out + 8, g11::stfs_bits(z));
        store<u32>(out + 12, g11::stfs_bits(ww));
        const f64 dlo = sgl(lo - x), dhi = sgl(hi - x);
        lo = dlo >= 0.0 ? x : lo;  // fsel
        hi = dhi >= 0.0 ? hi : x;  // fsel
    }
    const s32 size = static_cast<s32>(load<u32>(load<u32>(view + 512) + 56));
    const f64 span = sgl(hi - lo);
    const u64 bits = rt::fctid(sgl(span * sgl(static_cast<f64>(size))), true);  // fcfid / frsp, fmuls, fctidz
    ctx.fpr[1] = std::bit_cast<f64>(bits);
    const u32 n = static_cast<u32>(bits);
    const u32 want = n > 1 ? n : 1u;  // cmplwi
    u32 idx = 0;
    if (count != 0) {
        for (u32 off = 0; idx < count; ++idx, off += 4) {  // cmplw
            if (load<u16>(load<u32>(list + off) + g15::kTexLodSize) < want) break;  // cmplw
        }
        if (idx != 0) --idx;
    }
    if (count - 1 < idx) idx = count - 1;  // cmplw
    ret(ctx, load<u32>(list + 4 * idx));
}

// bool Emitter_Check_00506050(Emitter* e, DrawRecord* out, const f32* a, b, u32 c)
// (Emitter_PrepareDraw_0050809C; name?)
// 0 (nothing written) without e->+72 +20 / +12. The colour starts as
// white; without e->+24 its alpha becomes the depth fade: z = (view +752
// x a+48 (lvx, vmaddfp order)).z, clamp01((z - props+56) / props+60) *
// 255 (fctiwz). A zero alpha returns 0. Otherwise the props (e->+4) +132
// planes get Particle_CollidePlanes_00505E90, the light colour is
// applied (Particle_LightColour_00500D40, b set and no e->+24), and out
// gets a's 3x4 rows, the props' fields, the colour and c. Returns 1.
DECOMP(00506050, Emitter_Check_00506050) {
    constexpr u32 K = g15::kEmitterCheckConsts;
    const u32 e = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3), c = arg<u32>(ctx, 4);
    const u32 x72 = load<u32>(e + 72);
    const u32 e24 = load<u32>(e + 24);
    u32 props = load<u32>(e + 4);
    if (load<u32>(x72 + 20) == 0 || load<u32>(x72 + 12) == 0) {
        ret(ctx, 0);
        return;
    }
    Stack frame(ctx, 16);
    const u32 col = frame.addr();
    for (u32 k = 0; k < 4; ++k) store<u8>(col + k, ref<u8>(g15::g::white + k));
    if (e24 == 0) {
        const u32 m = (load<u32>(load<u32>(g15::g::current_surface) + 672) + g15::kViewMatrix) & ~15u;  // lvx
        const u32 p = (a + 48) & ~15u;
        f32 z = std::fmaf(load<f32>(m + 8), load<f32>(p), 0.0f);
        z = std::fmaf(load<f32>(m + 24), load<f32>(p + 4), z);
        z = std::fmaf(load<f32>(m + 40), load<f32>(p + 8), z);
        z = load<f32>(m + 56) + z;  // vaddfp
        f64 t = sgl(static_cast<f64>(z) - lfs(props + 56));
        t = sgl(t / lfs(props + 60));
        t = sgl(sgl(lfs(K) - t) >= 0.0 ? t : g11::lfd(K + 8));  // fsel: at most 1
        t = sgl(t >= 0.0 ? t : g11::lfd(K + 16));               // fsel: at least 0
        const u64 bits = rt::fctiw(sgl(t * lfs(K + 24)), true);  // fctiwz
        ctx.fpr[1] = std::bit_cast<f64>(bits);  // f1 as the original leaves it on a zero alpha
        store<u8>(col + 3, static_cast<u8>(bits));
    }
    if (ref<u8>(col + 3) == 0) {
        ret(ctx, 0);
        return;
    }
    if (load<u32>(props + 132) != 0) call(ctx, g15::fn::Particle_CollidePlanes_00505E90, {e});
    if (b != 0 && load<u32>(e + 24) == 0) call(ctx, g15::fn::Particle_LightColour_00500D40, {e, col, a, b});  // crorc
    for (u32 row = 0; row < 64; row += 16)
        for (u32 k = 0; k < 12; k += 4) store<u32>(out + row + k, load<u32>(a + row + k));
    props = load<u32>(e + 4);
    for (u32 k = 0; k < 16; k += 4) store<u32>(out + 64 + k, load<u32>(props + 92 + k));
    store<u32>(out + 80, load<u32>(props + 8));
    store<u32>(out + 84, load<u32>(props + 48));
    store<u32>(out + 88, load<u32>(props + 52));
    store<u32>(out + 92, load<u32>(props + 112));
    store<u32>(out + 96, load<u32>(props + 120));
    store<u32>(out + 100, load<u32>(props + 116));
    const u32 w128 = load<u32>(props + 128);
    store<u32>(out + 104, w128);
    const u32 w124 = load<u32>(props + 124);
    store<u32>(out + 112, 0);
    store<u32>(out + 108, w124);
    for (u32 k = 0; k < 4; ++k) store<u8>(out + 116 + k, ref<u8>(col + k));
    store<u32>(out + 120, c);
    store<u32>(out + 124, 0);
    store<u8>(out + 128, ref<u8>(props + 44));
    store<u8>(out + 129, ref<u8>(props + 68));
    ctx.fpr[1] = lfs(props + 52);  // the last lfs / stfs pair's
    ret(ctx, 1);
}

namespace {

// The AglMaterial free list's allocator, made on first use (as
// Material_Alloc inlines it). A failed Mem_Alloc stores and returns 0.
u32 material_allocator(PPUContext& ctx) {
    const u32 list = load<u32>(g15::g::material_list);
    u32 alloc = load<u32>(list);
    if (alloc != 0) return alloc;
    const u32 obj = static_cast<u32>(call(ctx, g15::fn::Mem_Alloc, {60, 0, 16}));
    if (obj != 0) {
        const u32 capacity = load<u32>(list + 4);
        for (u32 off : {8u, 4u, 12u, 20u, 16u}) store<u32>(obj + off, 0);
        const u16 h54 = load<u16>(obj + 54);
        for (u32 off : {24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(obj + off, 0);
        store<u16>(obj + 52, 0);
        store<u16>(obj + 54, static_cast<u16>(h54 & 0x7FFF));
        store<u32>(obj, g15::g::vt_material_list);
        store<u32>(obj + 56, g15::g::str_material_list);
        call(ctx, g15::fn::FreeList_Init, {obj, g15::kMaterialSize, capacity, 0, 0});
    }
    alloc = obj;
    store<u32>(list, alloc);
    return alloc;
}

// The live material (s16 refs +4 > 0) with name hash +40 == hash, walking
// the allocator's used list (Obj_GetField4: the first node; materials sit
// at node +16, the next node at node +0); 0 if none.
u32 material_find(PPUContext& ctx, u32 hash) {
    const u32 first = static_cast<u32>(call(ctx, g15::fn::Obj_GetField4, {material_allocator(ctx)}));
    u32 m = first != 0 ? first + 16 : 0;
    while (m != 0 && load<u32>(m + 40) != hash) {  // cmpw
        const u32 next = load<u32>(m - 16);
        m = next != 0 ? next + 16 : 0;
    }
    if (m != 0 && static_cast<s16>(load<u16>(m + 4)) <= 0) m = 0;  // extsh, cmpwi
    return m;
}

// *MeshMap_Find(mesh map, &key) ->+48 ->+0 to EmitProps_AddMesh_004FEBF0.
u64 add_mesh(PPUContext& ctx, u32 props, u32 key, u32 key_slot) {
    store<u32>(key_slot, key);
    const u32 slot = static_cast<u32>(call(ctx, g15::fn::MeshMap_Find, {load<u32>(g15::g::mesh_map), key_slot}));
    return call(ctx, g15::fn::EmitProps_AddMesh_004FEBF0, {props, load<u32>(load<u32>(load<u32>(slot) + 48))});
}

}  // namespace

// void EmitProps_SetDesc_004FF4C0(AglEmitProperties* p, const EmitDesc* d)
// (Iff_HandleEmitDesc_005286BC; name?)
// The emitter properties from the 68-byte chunk: +4 rate d+0, +8 1 /
// rate, +16 / +24 / +32 raw, +12 / +20 / +28 / +36 d+4 / +36 / +44 / +52
// clamped to (0, 0.75] (0 when not positive), u16 +40 = max(1, fctiwz(d+8
// * d+0)). Kind d+16: 0 binds a material (the live one named d+12, else a
// new one with shader 0x90873B1B, blending SRC_ALPHA / FUNC_ADD and
// shgroup's group texture in its next slot), 1 adds the mesh d+12 and
// up to 3 more from d+56 (to the first 0). Then +64 = d+17 bit 3 ? 1 : 0,
// +42 bits 7 / 6 / 5 = d+17 bits 2 / 5 / 4, +48..+56 = -d+20..+28, +60 =
// max(1, d+32), +43 / +44 = d+18 / +19. r3 is left as the original
// leaves it (d+18, or the last mesh call's / 0 for kind 1).
DECOMP(004FF4C0, EmitProps_SetDesc_004FF4C0) {
    constexpr u32 K = g15::kEmitDescConsts;
    const u32 p = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    const f64 rate = lfs(d);
    store<u32>(p + 4, g11::stfs_bits(rate));
    const f64 one = lfs(K);
    store<u32>(p + 8, g11::stfs_bits(sgl(one / rate)));  // fdivs
    const u32 hash = load<u32>(d + 12);
    const f64 cap = lfs(K + 4);
    const f64 d4 = lfs(d + 4), d8 = lfs(d + 8), d36 = lfs(d + 36), d40 = lfs(d + 40), d44 = lfs(d + 44),
              d48 = lfs(d + 48), d52 = lfs(d + 52);
    const u32 kind = ref<u8>(d + 16);
    const f64 zero = lfs(K + 8);
    auto clamp = [&](f64 x) { return (x < cap ? x : cap) > zero ? (x < cap ? x : cap) : zero; };  // fcmpu
    store<u32>(p + 12, g11::stfs_bits(clamp(d4)));
    store<u32>(p + 16, g11::stfs_bits(d8));
    store<u32>(p + 20, g11::stfs_bits(clamp(d36)));
    const s32 n = static_cast<s32>(rt::fctiw(sgl(d8 * rate), true));  // fmuls, fctiwz
    store<u32>(p + 24, g11::stfs_bits(d40));
    store<u16>(p + 40, static_cast<u16>(n < 1 ? 1 : n));  // cmpwi
    store<u32>(p + 28, g11::stfs_bits(clamp(d44)));
    store<u32>(p + 32, g11::stfs_bits(d48));
    store<u32>(p + 36, g11::stfs_bits(clamp(d52)));
    Stack frame(ctx, 16);
    const u32 key = frame.addr();
    if (kind == 1) {
        add_mesh(ctx, p, hash, key + 4);
    } else if (kind == 0) {
        u32 m = hash != 0 ? material_find(ctx, hash) : 0;
        if (m == 0) {
            const u32 shader = static_cast<u32>(call(ctx, g15::fn::Shader_FindByHash, {g15::kEmitShaderHash}));
            if (shader != 0) {
                m = static_cast<u32>(call(ctx, g15::fn::Material_Alloc, {}));
                store<u32>(m + 16, shader);  // unchecked: a failed Material_Alloc writes guest address 16
            }
            // A missing shader or material leaves m 0: the writes below then
            // go to guest address 0 (unchecked, as the original).
            const u32 b = m + 104;
            const u32 w0 = (load<u32>(b) | 0x80000000u) & ~0x7FFF8000u;
            const u32 w8 = load<u32>(b + 8) & 0x7FFFFFFFu;  // rlwinm 0,1,31
            store<u16>(b + 4, static_cast<u16>(g15::kEmitBlendEq));
            store<u16>(b + 6, 1);
            store<u32>(b + 8, w8);
            store<u32>(b, w0 | (g15::kEmitBlendSrc << 15));  // rlwimi 15,1,16
            // Material_AddTexture_004FF008 inlined: the next slot +20.. gets
            // the group texture (refs u16 +4 bumped, the old one released by
            // vtable slot 2); slot 0 also sets u16 +48 = the texture's +112.
            const u32 tex = load<u32>(g15::g::group_texture);
            const u8 count = static_cast<u8>(ref<u8>(m + 44) + 1);
            const u32 index = static_cast<u32>(count) - 1;
            store<u8>(m + 44, count);
            const u32 slot = m + 4 * index + 20;
            const u32 old = load<u32>(slot);
            if (old != tex) {  // cmpw
                store<u32>(slot, tex);
                if (tex != 0) store<u16>(tex + 4, static_cast<u16>(load<u16>(tex + 4) + 1));
                if (old != 0) fx::vcall(ctx, old, 8, {old});
            }
            if (index == 0) store<u16>(m + 48, static_cast<u16>(tex != 0 ? load<u32>(tex + 112) : 0u));
        }
        call(ctx, g15::fn::EmitProps_SetMaterial_004FEDD0, {p, m});
    }
    const u32 f17 = ref<u8>(d + 17);
    u32 b42 = ref<u8>(p + 42);
    store<u32>(p + 64, g11::stfs_bits((f17 & 8) != 0 ? one : zero));
    b42 = (b42 & ~0x80u) | ((f17 & 4) != 0 ? 0x80u : 0u);  // rlwimi 7,24,24
    store<u8>(p + 42, static_cast<u8>(b42));
    b42 = (b42 & ~0x40u) | ((ref<u8>(d + 17) & 0x20) != 0 ? 0x40u : 0u);  // rlwimi 6,25,25
    store<u8>(p + 42, static_cast<u8>(b42));
    b42 = (b42 & ~0x20u) | ((ref<u8>(d + 17) & 0x10) != 0 ? 0x20u : 0u);  // rlwimi 5,26,26
    store<u8>(p + 42, static_cast<u8>(b42));
    for (u32 k = 0; k < 12; k += 4) store<u32>(p + 48 + k, load<u32>(d + 20 + k) ^ 0x80000000u);  // fneg
    const f64 d32 = lfs(d + 32);
    const f64 far = d32 > one ? d32 : one;  // fcmpu
    const u32 kind2 = ref<u8>(d + 16), b18 = ref<u8>(d + 18), b19 = ref<u8>(d + 19);
    store<u32>(p + 60, g11::stfs_bits(far));
    store<u8>(p + 43, static_cast<u8>(b18));
    store<u8>(p + 44, static_cast<u8>(b19));
    ctx.fpr[1] = far;
    ctx.gpr[3] = b18;
    if (kind2 != 1) return;
    for (u32 k = 0; k < 3; ++k) {
        const u32 h = load<u32>(d + 56 + 4 * k);
        if (h == 0) {
            ctx.gpr[3] = 0;
            return;
        }
        ctx.gpr[3] = add_mesh(ctx, p, h, key);
    }
}

// void Texture_InitDefaults_0051DA1C()
// (after the texture system init; name?)
// Makes the built-in textures from the BMPs in .data: each is allocated,
// named "MEM:<name>.Bmp" (its hash +48) and loaded with Texture_Init;
// one that fails is released (vtable slot 2), one that loads is renamed
// "APE_<NAME>" and stored in default_textures (White in slots 0 and 1).
// The original also tests each table address for 0 (never true).
DECOMP(0051DA1C, Texture_InitDefaults_0051DA1C) {
    namespace g10 = sly4::render::gfx10;
    for (const g15::BuiltinTexture& b : g15::kBuiltinTextures) {
        const u32 size = load<u32>(b.data);
        const u32 t = static_cast<u32>(call(ctx, g10::fn::Texture_Alloc, {}));
        const u32 name = t + 16;
        call(ctx, g10::fn::String_CopyN, {name, b.mem_name, g15::kTexNameSize});
        store<u8>(t + 47, 0);
        store<u32>(t + 48, static_cast<u32>(call(ctx, sly4::fn::String_HashNoCase, {name})));
        if (static_cast<u32>(call(ctx, g10::fn::Texture_Init, {t, b.data + 4, size, g15::str_bmp, b.mem_name})) == 0) {  // cmpwi
            fx::vcall(ctx, t, 8, {t});
            continue;
        }
        if (t == 0) continue;
        call(ctx, g10::fn::String_CopyN, {name, b.ape_name, g15::kTexNameSize});
        store<u8>(t + 47, 0);
        store<u32>(t + 48, static_cast<u32>(call(ctx, sly4::fn::String_HashNoCase, {name})));
        store<u32>(g15::g::default_textures + b.slot, t);
        if (b.slot == 0) store<u32>(g15::g::default_textures + 4, t);
        ctx.gpr[3] = g15::g::default_textures;  // r3 as the original leaves it after Black
    }
}

// void Stencil_Pass_0054C268(StencilDraw* o, u32 pass; f32 scale)
// (Stencil_Draw_0054CB58; name?)
// One pass over the o->+20 (u8) lights at o+4..: vertex program
// 0x010F0ADC with vertex constant 1 = zeros, blending SRC_ALPHA /
// ONE_MINUS_SRC_ALPHA, the depth texture (by display mode) on unit 0,
// unit 1 clamped with the state's filter, depth bounds on. Per light l:
// the depth bounds from {0, 0, -l+12} / {0, 0, -l+16} through view +1008
// (z / w), the fragment program fp_stencil[Stencil_Variant_0052DAEC(o,
// pass, i)], vertex constants 2.. = l+352 (16 floats), fragment
// parameters 0 = {o+48, +52, +56, +60 * scale}, 2 = {1 / near, (near -
// far) / (near far), 0, 0}, 1 = {l+1536 / width, / height, o+84 - o+88,
// o+88}, 3 = {l+360, +376, +392, +408}, texture unit 1 = l->+644; with
// pass bits 0..1 also parameter 7 = o+72..+80 / l+336 (with a 0) and
// l->+648 on unit 2; then the quad. Depth bounds off and the depth
// restored at the end.
DECOMP(0054C268, Stencil_Pass_0054C268) {
    namespace dfn = sly4::draw::fn;
    const u32 o = arg<u32>(ctx, 0), pass = arg<u32>(ctx, 1);
    const f64 scale = ctx.fpr[1];
    const u32 st = fx::g::state;
    const u32 view = static_cast<u32>(call(ctx, g15::fn::View_GetCurrent, {}));
    if (ref<u8>(g15::g::stencil_vconst_ready) == 0) {
        for (u32 k = 0; k < 16; k += 4) store<u32>(g15::g::stencil_vconst + k, 0);
        store<u8>(g15::g::stencil_vconst_ready, 1);
    }
    const f64 nearz = lfs(view + 60), farz = lfs(view + 64);
    const f64 diff = sgl(nearz - farz);
    const f64 prod = sgl(farz * nearz);
    f64 f1 = sgl(lfs(g15::kStencilPassOne) / nearz);
    const f64 depth_k = sgl(diff / prod);
    const u32 surface = load<u32>(load<u32>(load<u32>(o + 28) + 672) + 512);
    const u32 width = load<u32>(surface + 56), height = load<u32>(surface + 60);
    Stack frame(ctx, 64);
    const u32 dk = frame.addr(), in = dk + 16, out0 = dk + 32, out1 = dk + 48;
    store<u32>(dk + 8, 0);
    store<u32>(dk + 12, 0);
    store<u32>(dk, g11::stfs_bits(f1));
    store<u32>(dk + 4, g11::stfs_bits(depth_k));
    u32 depth;
    const u32 kStats = sly4::g::mem_stats;
    if (((ref<u8>(load<u32>(g15::g::display) + 148) >> 7) & 1) != 0) {  // rlwinm 25,31,31
        depth = load<u32>(kStats + g15::kStatsDepthAlt);
    } else if (field<u8>(sly4::g::config, 34) == 0) {
        depth = load<u32>(kStats + g15::kStatsDepthMono);
    } else {
        depth = load<u32>(kStats + (load<u32>(kStats + 184) == 1 ? g15::kStatsDepthEye0 : g15::kStatsDepthEye1));
    }
    effect::bind_vertex(ctx, g15::g::vp_stencil);
    call(ctx, dfn::cellGcmSetVertexProgramConstants, {load<u32>(st), 1, 4, g15::g::stencil_vconst});
    call(ctx, g15::fn::Draw_ResetState, {0, 0});
    fx::emit({fx::kBlendEnable, 1});
    fx::emit({fx::kBlendEnable, 1});
    fx::emit({fx::kBlendFunc, fx::kSrcAlpha2, fx::kOneMinusSrcAlpha2});
    fx::emit({fx::kBlendEquation, fx::kFuncAdd2});
    fx::invalidate_textures(ctx);
    call(ctx, g15::fn::Draw_SetTexture0, {depth, 0, 1});
    call(ctx, g15::fn::Gcm_SetDepthBoundsTest_005E2BD4, {load<u32>(st), 1});
    fx::emit({g15::kTexAddressUnit1, g15::kTexAddressClampBorder});
    call(ctx, dfn::cellGcmSetTextureFilter, {load<u32>(st), 1, 0, ref<u8>(st + 226), ref<u8>(st + 223), 1});
    const u32 m = view + g15::kViewInvProj;
    if (static_cast<s32>(ref<u8>(o + 20)) > 0) {
        const f64 w = sgl(static_cast<f64>(width)), h = sgl(static_cast<f64>(height));  // fcfid, frsp
        for (u32 i = 0; i < ref<u8>(o + 20); i = (i + 1) & 0xFF) {  // cmpw
            const u32 l = load<u32>(o + 4 + 4 * i);
            store<u32>(in, 0);
            store<u32>(in + 4, 0);
            store<u32>(in + 8, load<u32>(l + 12) ^ 0x80000000u);  // fneg
            call(ctx, g15::fn::Vec_TransformByMatrix_553A2C, {out0, in, m});
            store<u32>(in, 0);
            store<u32>(in + 4, 0);
            store<u32>(in + 8, load<u32>(l + 16) ^ 0x80000000u);
            call(ctx, g15::fn::Vec_TransformByMatrix_553A2C, {out1, in, m});
            const f64 zmin = sgl(lfs(out0 + 8) / lfs(out0 + 12)), zmax = sgl(lfs(out1 + 8) / lfs(out1 + 12));
            callf(ctx, g15::fn::Gcm_SetDepthBounds_005E341C, {load<u32>(st)}, {zmin, zmax});
            const u32 k = static_cast<u32>(call(ctx, g15::fn::Stencil_Variant_0052DAEC, {o, pass, i}));
            const u32 gcm = load<u32>(st);
            effect::bind_fragment(g15::g::fp_stencil + 4 * k);
            call(ctx, dfn::cellGcmSetVertexProgramConstants, {gcm, 2, 16, l + 352});
            const u32 tex = load<u32>(l + 644);
            fx::set_fconst(0, load<u32>(o + 48), load<u32>(o + 52), load<u32>(o + 56), g11::stfs_bits(sgl(lfs(o + 60) * scale)));
            fx::set_fconst_from(2, dk);
            const f64 size = lfs(l + 1536), o84 = lfs(o + 84), o88 = lfs(o + 88);
            fx::set_fconst(1, g11::stfs_bits(sgl(size / w)), g11::stfs_bits(sgl(size / h)), g11::stfs_bits(sgl(o84 - o88)),
                           g11::stfs_bits(o88));
            f1 = lfs(l + 360);
            fx::set_fconst(3, load<u32>(l + 360), load<u32>(l + 376), load<u32>(l + 392), load<u32>(l + 408));
            if (load<u32>(st + 56) != tex) {  // cmpw
                store<u32>(st + 56, tex);
                call(ctx, dfn::cellGcmSetTexture, {load<u32>(st), 1, tex + 16});
                const u32 levels = ref<u8>(tex + 17);
                call(ctx, dfn::cellGcmSetTextureControl, {load<u32>(st), 1, 1, 0, ((levels << 8) - 256) & 0xFFFF, 0});
            }
            if ((pass & 3) != 0) {
                const f64 r = lfs(l + 336);
                f1 = sgl(lfs(o + 72) / r);
                const f64 y = sgl(lfs(o + 76) / r), z = sgl(lfs(o + 80) / r);
                fx::set_fconst(7, g11::stfs_bits(f1), 0, g11::stfs_bits(y), g11::stfs_bits(z));
                call(ctx, g15::fn::Draw_SetTexture0, {load<u32>(l + 648), 2, 0});
            }
            call(ctx, g15::fn::Draw_PrepareQuad, {});
        }
    }
    call(ctx, g15::fn::Gcm_SetDepthBoundsTest_005E2BD4, {load<u32>(st), 0});
    call(ctx, g15::fn::Draw_RestoreDepth, {});
    ctx.fpr[1] = f1;  // f1 as the original leaves it
}

namespace {

// The pass stream's cursor rules (Stream_ReadTwo_00514CC8 and the other
// readers share them): the 12-byte header sits at the cursor +12, or at
// the start +0 when the cursor + 15 passes the end +4 (unsigned), aligned
// up to 4; a record of n bytes follows at header +12, or at the start when
// header + n + 11 passes the end. `end` is the one read at the top.
u32 pass_header(u32 s, u32 p, u32 end) { return ((p + 15 > end ? load<u32>(s) : p) + 3) & ~3u; }  // cmplw
u32 pass_record(u32 s, u32 h, u32 n, u32 end) { return h + n + 11 > end ? load<u32>(s) : h + 12; }   // cmplw
// A 64-byte matrix record (16-byte aligned); the cursor moves past it.
u32 pass_matrix(u32 s, u32 p, u32 end) {
    const u32 m = (pass_record(s, pass_header(s, p, end), 80, end) + 15) & ~15u;
    store<u32>(s + 12, m + 64);
    return m;
}
// A 4- or 8-byte record (4-byte aligned); the cursor moves past it.
u32 pass_words(u32 s, u32 p, u32 end, u32 bytes) {
    const u32 a = (pass_record(s, pass_header(s, p, end), bytes + 4, end) + 3) & ~3u;
    store<u32>(s + 12, a + bytes);
    return a;
}

void pass_unbind_arrays(PPUContext& ctx) {
    const u32 st = fx::g::state;
    for (u32 k = 0; k < 16; ++k)
        call(ctx, g15::fn::cellGcmSetVertexDataArrayUnsafe, {load<u32>(st), k, 0, 0, 0, 2, 0, 0});
    store<s32>(st + 8, -1);
    store<u16>(st + 32, 0);
}

void pass_bind_arrays(PPUContext& ctx, u32 s) {
    call(ctx, g15::fn::Draw_BindVertexArrays_0079E0C0, {s + 20 + 16 * ref<u8>(s + 58), g15::g::vertex_format_1, 0, 0});
}

u32 dup16(u32 v) { return (v & 0xFFFF) | (v << 16); }  // rlwimi 16,0,15

}  // namespace

// void Draw_Pass_005159E8(Stream* s, Pass* pass)
// (Render_DrawFrame_0051A918, Layer9_RunStream_005165D0 and others; name?)
// Runs a pass's command stream (s: +0 start, +4 end, +12 cursor; each
// command a 12-byte header whose first byte is the command, and its
// record): matrices into pass +0 / +4 (+16 = +4 x +0), the model-view
// constants, materials and shaders (the ShaderType at material +16 picks
// the shader; the previous one's slot 4 ends it; a new material rebinds
// s's vertex arrays), textures, scissor, depth / blend / cull state
// (mirrored into the state caches 0x010DD598 / 0x010DD5A0), stencil,
// alpha test, line width, a sub-stream (run by a recursive call with a
// copy of the pass) and draws, until command 22 (23 returns at once,
// without the clean-up). The clean-up ends the last shader, resets the
// depth bounds once (s +200) and unbinds the 16 vertex arrays. Unknown
// commands are skipped without moving the cursor (the original then
// loops on them forever).
DECOMP(005159E8, Draw_Pass_005159E8) {
    namespace pc = g15::pass_cmd;
    const u32 s = arg<u32>(ctx, 0), pass = arg<u32>(ctx, 1);
    const u32 st = fx::g::state;
    const u32 sel_arg = load<u32>(g15::g::draw_pass_arg);
    u32 mat = 0, shader = 0;
    bool done = false;
    f64 f1 = ctx.fpr[1];
    Stack frame(ctx, 128);
    const u32 out = frame.addr();  // +0 texture pair, +4..+12 / +16 the 4x16 read, +20 / +24 the sub-stream pair
    const u32 sub_pass = out + 32;  // Pass {+0, +4, -, -, +16 f32[16]}
    while (!done) {
        const u32 p = load<u32>(s + 12), end = load<u32>(s + 4);
        const u32 cmd = ref<u8>(pass_header(s, p, end));
        switch (cmd) {
        case pc::kSetMatrix:
            store<u32>(pass, pass_matrix(s, p, end));
            break;
        case pc::kMulMatrix: {
            const u32 m = pass_matrix(s, p, end);
            store<u32>(pass + 4, m);
            call(ctx, g15::fn::Matrix_MulAffine_0079DE98, {pass + 16, m, load<u32>(pass)});
            break;
        }
        case pc::kMvp2: {
            const u32 m = pass_matrix(s, p, end);
            call(ctx, g15::fn::VpConst_SetMvp_005158B4, {s, m, pass + 16});
            break;
        }
        case pc::kMvp: {
            const u32 m = pass_matrix(s, p, end);
            call(ctx, g15::fn::VpConst_SetMvp_00515784, {s, m, load<u32>(pass)});
            break;
        }
        case pc::kBindMaterial:
        case pc::kBindShader: {
            const u32 m = load<u32>(pass_words(s, p, end, 4));
            const u32 type = load<u32>(m + 16);
            const u32 sh = static_cast<u32>(fx::vcall(ctx, type, g15::kShaderTypeSelect, {type, m, sel_arg}));
            if (mat != 0) fx::vcall(ctx, shader, g15::kShaderEnd, {shader, mat});
            if (cmd == pc::kBindShader) {
                effect::bind_vertex(ctx, sh + 4);
                effect::bind_fragment(sh + 8);
                fx::vcall(ctx, sh, g15::kShaderBegin, {sh, m});
            } else {
                call(ctx, g15::fn::Material_Bind, {m, sh});
            }
            if (mat != m) pass_bind_arrays(ctx, s);  // cmpw
            mat = m;
            shader = sh;
            break;
        }
        case pc::kVpConstant:
            call(ctx, g15::fn::Stream_SetVpConstant_00515688, {s});
            break;
        case pc::kFpConstant:
            call(ctx, g15::fn::Stream_SetFpConstant_005155C4, {s});
            break;
        case pc::kTexture: {
            store<u32>(out, 0);
            const u32 tex = static_cast<u32>(call(ctx, g15::fn::Stream_ReadPair_00515544, {s, out}));
            call(ctx, g15::fn::Texture_Bind_0079DF78, {tex, static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(out))))});  // extsw
            break;
        }
        case pc::kScissor: {
            const u32 a = pass_words(s, p, end, 8);
            fx::emit({g15::kScissorMethod, (u32{load<u16>(a + 4)} << 16) | load<u16>(a), (u32{load<u16>(a + 6)} << 16) | load<u16>(a + 2)});
            break;
        }
        case pc::kDepth: {
            const u32 a = pass_words(s, p, end, 8);
            const u32 func = load<u32>(a), write = load<u32>(a + 4) == 1 ? 1u : 0u;  // addic -1, cntlzw
            fx::emit({fx::kDepthTestEnable, ref<u8>(st + 93)});
            fx::emit({g15::kDepthFuncMethod, func});
            fx::emit({fx::kDepthMask, (write & ref<u8>(st + 93)) != 0 ? 1u : 0u});
            const u32 c = load<u32>(g15::g::depth_cache);
            store<u32>(g15::g::depth_cache, (c & 0x00007FFFu) | ((func & 0xFFFF) << 15) | (write << 31));  // rlwimi 15,1,16; 26,0,0
            break;
        }
        case pc::kCull: {
            const u32 v = load<u32>(pass_words(s, p, end, 4));
            if (v == 0) {
                fx::emit({fx::kCullFaceEnable, 0});
            } else {
                fx::emit({fx::kCullFaceEnable, 1});
                fx::emit({g15::kFrontFaceMethod, v == g15::kCw ? g15::kCcw : g15::kCw});
            }
            store<u16>(g15::g::depth_cache + 4, static_cast<u16>(v));
            break;
        }
        case pc::kBlend: {
            call(ctx, g15::fn::Stream_Read4x16_00515168, {s, out + 4, out + 8, out + 12, out + 16});
            const u32 on = ref<u8>(out + 16), sf = load<u32>(out + 4), df = load<u32>(out + 8), eq = load<u32>(out + 12);
            if (on != 0) {
                fx::emit({fx::kBlendEnable, 1});
                fx::emit({fx::kBlendFunc, dup16(sf), dup16(df)});
                fx::emit({fx::kBlendEquation, dup16(eq)});
            } else {
                fx::emit({fx::kBlendEnable, 0});
            }
            const u32 c = load<u32>(g15::g::blend_cache);
            store<u32>(g15::g::blend_cache, (c & 0x00007FFFu) | ((on & 1u) << 31) | ((load<u32>(out + 4) & 0xFFFF) << 15));  // rlwimi 31,0,0; 15,1,16
            store<u16>(g15::g::blend_cache + 4, static_cast<u16>(load<u32>(out + 12)));
            store<u16>(g15::g::blend_cache + 6, static_cast<u16>(load<u32>(out + 8)));
            break;
        }
        case pc::kStencil:
            call(ctx, g15::fn::Cmd_SetStencil_00515350, {s});
            break;
        case pc::kStencilRecord:
            call(ctx, g15::fn::Record_Stencil_005151E8, {s});
            break;
        case pc::kAlphaTest:
            call(ctx, g15::fn::Stream_SetAlphaTest_005150A8, {s});
            break;
        case pc::kLineWidth:
            call(ctx, g15::fn::Stream_SetLineWidth_00515018, {s});
            break;
        case pc::kDepthBias:
            call(ctx, g15::fn::Stream_SetDepthBias_00514D68, {s});
            break;
        case pc::kSubStream: {
            store<u32>(out + 20, 0);
            store<u32>(out + 24, 0);
            call(ctx, g15::fn::Stream_ReadTwo_00514CC8, {s, out + 20, out + 24});
            pass_unbind_arrays(ctx);
            const u32 sub = load<u32>(out + 20);
            store<u32>(sub + 16, load<u32>(sub + 4));
            store<u32>(load<u32>(out + 20) + 12, load<u32>(out + 24));
            store<u32>(sub_pass, load<u32>(pass));
            store<u32>(sub_pass + 4, load<u32>(pass + 4));
            for (u32 k = 16; k < 80; k += 4) store<u32>(sub_pass + k, load<u32>(pass + k));  // lfs / stfs
            f1 = lfs(pass + 76);
            call(ctx, g15::fn::Draw_Pass_005159E8, {load<u32>(out + 20), sub_pass});
            mat = 0;
            shader = 0;
            break;
        }
        case pc::kDrawArrays: {
            const u32 h = pass_header(s, p, end);
            store<u32>(s + 12, h + 12);
            const u32 count = load<u16>(h + 4), mode = load<u16>(h + 2), first = load<u32>(h + 8);
            sly4::render::pool::upload_fragment(ctx);
            call(ctx, g15::fn::cellGcmSetDrawArrays, {load<u32>(st), mode & 0xFF, first, count});
            break;
        }
        case pc::kEnd:
            store<u32>(s + 12, pass_header(s, p, end) + 12);
            done = true;
            break;
        case pc::kReturn:
            ctx.fpr[1] = f1;
            ctx.gpr[3] = p + 15;  // r3 as the original leaves it
            return;
        default:
            break;  // the original does not move the cursor either
        }
    }
    if (mat != 0) fx::vcall(ctx, shader, g15::kShaderEnd, {shader, mat});
    if (ref<u8>(s + 200) != 0) {
        store<u8>(s + 200, 0);
        f1 = lfs(g15::kDrawPassZero);
        callf(ctx, g15::fn::Draw_SetDepthBounds_0079E2B8, {}, {f1, f1});
    }
    pass_unbind_arrays(ctx);
    ctx.fpr[1] = f1;
    ctx.gpr[3] = st;  // r3 as the original leaves it
}

// void Stream_SetDepthBias_00514D68(Stream* s)
// (Draw_Pass_005159E8 command 17; name?)
// Reads a u8 record: set, with depth writes on (render state u8 +93) and
// a depth target (+84) of format 4 / 6 (x 65536) or 5 / 7 (x 2^24),
// polygon offset fill goes on with factor 0 and bias -1e-4 x the scale;
// any other case turns it off (an unknown format sets the warned flag
// once). f1 / r3 are left as the original leaves them.
DECOMP(00514D68, Stream_SetDepthBias_00514D68) {
    namespace s2 = sly4::render::shader2;
    constexpr u32 K = g15::kDepthBiasConsts, st = fx::g::state;
    const u32 s = arg<u32>(ctx, 0);
    const u32 p = load<u32>(s + 12), end = load<u32>(s + 4), start = load<u32>(s);
    const u32 h = ((p + 15 > end ? start : p) + 3) & ~3u;          // cmplw
    const u32 a = ((h + 16 > end ? start : h + 12) + 3) & ~3u;     // cmplw
    store<u32>(s + 12, a + 1);
    if (ref<u8>(a) == 0) {
        ctx.fpr[1] = lfs(K + 4);
        fx::emit({s2::kPolyOffsetFill, 0});
        ctx.gpr[3] = 0;
        return;
    }
    f64 bias = lfs(K);
    ctx.fpr[1] = bias;
    const u32 target = load<u32>(st + 84);
    bool on = ref<u8>(st + 93) != 0 && target != 0;
    if (on) {
        const s32 format = static_cast<s32>(load<u32>(target + 24));
        if (format == 4 || format == 6) {
            bias = sgl(bias * lfs(K + 8));
        } else if (format == 5 || format == 7) {
            bias = sgl(bias * lfs(K + 12));
        } else {
            if (ref<u8>(s2::g::depth_format_warned) == 0) store<u8>(s2::g::depth_format_warned, 1);
            on = false;
        }
    }
    if (!on) {
        fx::emit({s2::kPolyOffsetFill, 0});
    } else {
        ctx.fpr[1] = bias;
        fx::emit({s2::kPolyOffsetFill, 1});
        fx::emit({s2::kPolyOffset2, 0, g11::stfs_bits(bias)});
    }
    ctx.gpr[3] = load<u32>(st);
}

namespace {

// Writes a command header {w0, w1, 0} at the frame buffer's cursor (+16;
// at the start +0 when cursor + 15 passes the end +4), then a 64-byte
// matrix record copied from m (read with lvx) 16-byte aligned after it
// (at the start when header + 91 passes the end). The cursor (+16) is
// left at the header for the first, at the record for the second
// store; returns the record + 64.
u32 fb_write_matrix(u32 fb, u32 cursor, u32 w0, u32 w1, u32 m, bool w8_first) {
    const u32 h = ((cursor + 15 > load<u32>(fb + 4) ? load<u32>(fb) : cursor) + 3) & ~3u;  // cmplw
    store<u32>(fb + 16, h);
    if (w8_first) store<u32>(h + 8, 0);
    store<u32>(h, w0);
    if (!w8_first) store<u32>(h + 8, 0);
    store<u32>(h + 4, w1);
    const u32 end = load<u32>(fb + 4);
    const u32 at = load<u32>(fb + 16);
    const u32 r = ((at + 91 > end ? load<u32>(fb) : at + 12) + 15) & ~15u;  // cmplw
    store<u32>(fb + 16, r);
    u32 v[16];
    m &= ~15u;  // lvx
    for (u32 k = 0; k < 16; ++k) v[k] = load<u32>(m + 4 * k);
    for (u32 k = 0; k < 16; ++k) store<u32>(r + 4 * k, v[k]);
    return load<u32>(fb + 16) + 64;
}

}  // namespace

// u32 FrameBuffer_BeginLayer(FrameBuffer* fb, View* view, u32 flags)
// (Job_Begin, View_BeginFrame)
// Resets the frame buffer's state cache (+80.. to -1, +96 = 100000, +196
// = -1.0, +200 = 0). With flags' low byte set: View_Update(view, 1) (view
// 0: the current surface's view), then into the command stream: with
// flags bit 0 command 0 (set matrix) = view +1008, then command 1
// (multiply) = view +752, the identity state (Draw_SetState) and the
// scissor {0, 0, surface width, height} (also cached at +202..+208).
// Returns the cursor before the commands when fb u8 +76 is set, else 0.
DECOMP(00521A5C, FrameBuffer_BeginLayer) {
    const u32 fb = arg<u32>(ctx, 0), flags = arg<u32>(ctx, 2);
    u32 view = arg<u32>(ctx, 1);
    if (view == 0) view = load<u32>(load<u32>(g15::g::current_surface) + 672);
    store<u32>(fb + 84, 0xFFFFFFFFu);
    store<u32>(fb + 80, 0xFFFFFFFFu);
    store<u32>(fb + 88, 0xFFFFFFFFu);
    store<u32>(fb + 96, g15::kFarDefault);
    store<u32>(fb + 160, 0xFFFFFFFFu);
    store<u64>(fb + 176, ~0ull);
    store<u16>(fb + 164, 0xFFFF);
    store<u64>(fb + 184, ~0ull);
    store<u32>(fb + 192, 0xFFFFFFFFu);
    store<u32>(fb + 196, g15::kMinusOne);
    store<u8>(fb + 200, 0);
    if ((flags & 0xFF) == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 mark = ref<u8>(fb + 76) != 0 ? load<u32>(fb + 16) : 0u;
    call(ctx, g15::fn::View_Update, {view, 1});
    u32 cursor = load<u32>(fb + 16);
    if ((flags & 1) != 0) cursor = fb_write_matrix(fb, cursor, 0x00000000u, 1, view + 1008, false);  // command 0
    store<u32>(fb + 160, 0xFFFFFFFFu);
    cursor = fb_write_matrix(fb, cursor, 0x01FF0000u, 0x0000FFFFu, view + 752, true);  // command 1
    store<u32>(fb + 16, cursor);
    call(ctx, g15::fn::Draw_SetState, {fb, g15::g::identity});
    const u32 surface = load<u32>(view + 512);
    const u32 w = load<u32>(surface + 56), h = load<u32>(surface + 60);
    Stack frame(ctx, 16);
    const u32 rect = frame.addr();
    store<u16>(rect, 0);
    store<u16>(rect + 2, 0);
    store<u16>(rect + 4, static_cast<u16>(w));
    store<u16>(rect + 6, static_cast<u16>(h));
    store<u16>(fb + 202, 0);
    store<u16>(fb + 204, 0);
    store<u16>(fb + 206, static_cast<u16>(w));
    store<u16>(fb + 208, static_cast<u16>(h));
    call(ctx, g15::fn::FrameBuffer_SetScissor_0052173C, {fb, rect});
    ret(ctx, mark);
}

// bool Iff_HandleTextureData_005399CC(IffReader* r, Texture** slot)
// [IFF chunk handler, Texture_SystemInit_0051DF3C's data chunk (name?)]
// For a shared reader (u8 +12 bit 2) the texture's old data is freed
// first (unless +64 bit 0 marks it borrowed; pool 4 or 5 by +60 bit 0);
// no texture returns 1. With config +160 bit 7 the default texture's
// format may be taken instead (Texture_CopyDefaultFormat_005398CC, then
// done). Otherwise the chunk is read into a new 128-aligned pool block
// (+56; its RSX offset << 1 into +60), and if it holds a valid header
// (at least 12 bytes, its +4 size and the image +12 {+4 offset, +8 size}
// within the chunk, location 0 or 1) the 24 bytes after the image
// record (+24) are copied to +72 with the location (+86) and the image's
// RSX offset (+92). Then +100 =
// the format table's value for format +72 (0 if unknown) and +52 / +54
// = +80 / +82. Returns 1.
DECOMP(005399CC, Iff_HandleTextureData_005399CC) {
    const u32 r = arg<u32>(ctx, 0), slot = arg<u32>(ctx, 1);
    if ((ref<u8>(r + 12) & g14::kIffShared) != 0) {
        const u32 t = load<u32>(slot);
        if (t == 0) {
            ret(ctx, 1);
            return;
        }
        const u32 d = t + 56;
        if (load<u32>(d) != 0 && (load<u32>(d + 8) & 1) == 0) {  // rlwinm 0,31,31
            const u32 pool = (load<u32>(d + 4) & 1) == 0 ? g15::kPoolTexture : g15::kPoolTextureAlt;
            call(ctx, g15::fn::Mem_FreePool, {load<u32>(d), pool, load<u32>(d + 12) >> 28});  // rlwinm 4,28,31
            store<u32>(d, 0);
        }
    }
    const u32 t = load<u32>(slot);
    if (((field<u8>(sly4::g::config, 160) >> 7) & 1) != 0 &&  // rlwinm 25,31,31
        static_cast<u32>(call(ctx, g15::fn::Texture_CopyDefaultFormat_005398CC, {t})) != 0) {  // cmpwi
        ret(ctx, 1);
        return;
    }
    const u32 size = static_cast<u32>(call(ctx, g15::fn::Iff_ChunkSize, {r}));
    const u32 w64 = load<u32>(t + 64), w68 = load<u32>(t + 68);
    store<u32>(t + 64, (w64 & 0xFu) | (size << 4));                 // rlwimi 4,0,27
    store<u32>(t + 68, (w68 & 0x0FFFFFFFu) | (1u << 28));           // rlwimi 28,0,3
    store<u32>(t + 56, static_cast<u32>(call(ctx, g15::fn::Mem_AllocPool, {g15::kTexDataAlign, size & 0x0FFFFFFFu, 4, 1})));
    Stack frame(ctx, 16);
    const u32 off = frame.addr();
    store<u32>(off, 0);
    call(ctx, g15::fn::cellGcmAddressToOffset, {load<u32>(t + 56), off});
    store<u32>(t + 60, load<u32>(off) << 1);
    call(ctx, g15::fn::FileReader_Read, {r, load<u32>(t + 56), size});
    const u32 data = load<u32>(t + 56);
    const u32 loc = load<u32>(t + 60) & 1;
    if (data != 0 && size >= g15::kHeaderMin && size >= load<u32>(data + 4)) {  // cmplwi, cmplw
        const u32 img = data + 12;
        // (the original also tests img and t + 72 for 0)
        if (img != 0 && (loc == 0 || loc == 1) && size >= load<u32>(img + 4) + load<u32>(img + 8)) {  // cmplw
            for (u32 k = 0; k < 24; k += 4) store<u32>(t + 72 + k, load<u32>(img + 12 + k));
            store<u8>(t + 86, static_cast<u8>(loc));
            call(ctx, g15::fn::cellGcmAddressToOffset, {data + load<u32>(img + 4), t + 92});
        }
    }
    u32 value = 0;
    for (u32 k = 0; k < g15::kFormatCount; ++k) {
        const u32 e = g15::format_table + 24 * k;
        if (static_cast<s32>(ref<u8>(t + 72)) == static_cast<s32>(load<u32>(e + 4))) {  // cmpw
            value = load<u32>(e);
            break;
        }
    }
    const u16 w = load<u16>(t + 80);
    store<u32>(t + 100, value);
    const u16 h = load<u16>(t + 82);
    store<u16>(t + 52, w);
    store<u16>(t + 54, h);
    ret(ctx, 1);
}

// void LightGroup_Draw_0052CD5C(LightGroup* o, View* view, x)
// [virtual; name?]
// Each of the u8 +20 lights at +4.. gets its slot 3 (prepare); then a
// LightView block on the stack: +0 inverse(view +1008), +64 uv_to_clip x
// inverse(view +880) (rows 3x3 plus row 3, vmaddfp order), +128 view
// +816's 4 rows of 3 floats, +192 view +864 (3 floats), +208 near (view
// +60), +212 o+84 - near; each light then gets slot 6 (draw) with (light,
// o, block, x). The block's unused words are left unwritten, as in the
// original.
DECOMP(0052CD5C, LightGroup_Draw_0052CD5C) {
    const u32 o = arg<u32>(ctx, 0), view = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    for (u32 i = 0; i < ref<u8>(o + 20); i = (i + 1) & 0xFF) {  // cmpw
        const u32 l = load<u32>(o + 4 + 4 * i);
        fx::vcall(ctx, l, g15::kLightPrepare, {l});
    }
    Stack frame(ctx, 352);
    const u32 tmp = frame.addr(), inv = tmp + 64, blk = tmp + 128;
    call(ctx, g15::fn::Mat4_Invert, {tmp, inv, view + 880});
    call(ctx, g15::fn::Mat4_Invert, {tmp, blk, view + 1008});
    const u32 a = g15::g::uv_to_clip;  // lvx (aligned)
    for (u32 i = 0; i < 4; ++i) {
        const f32 a0 = load<f32>(a + 16 * i), a1 = load<f32>(a + 16 * i + 4), a2 = load<f32>(a + 16 * i + 8);
        for (u32 k = 0; k < 4; ++k) {
            f32 r = std::fmaf(load<f32>(inv + 4 * k), a0, i == 3 ? load<f32>(inv + 48 + 4 * k) : 0.0f);
            r = std::fmaf(load<f32>(inv + 16 + 4 * k), a1, r);
            store<f32>(blk + 64 + 16 * i + 4 * k, std::fmaf(load<f32>(inv + 32 + 4 * k), a2, r));
        }
    }
    const u32 rows = view + 816;
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 12; k += 4) store<u32>(blk + 128 + 16 * i + k, load<u32>(rows + 16 * i + k));  // lfs / stfs
    for (u32 k = 0; k < 12; k += 4) store<u32>(blk + 192 + k, load<u32>(view + 864 + k));
    const f64 nearz = lfs(view + 60);
    store<u32>(blk + 208, load<u32>(view + 60));
    const f64 d = sgl(lfs(o + 84) - nearz);
    store<u32>(blk + 212, g11::stfs_bits(d));
    ctx.fpr[1] = d;
    ctx.fpr[2] = nearz;
    for (u32 i = 0; i < ref<u8>(o + 20); i = (i + 1) & 0xFF) {  // cmpw
        const u32 l = load<u32>(o + 4 + 4 * i);
        fx::vcall(ctx, l, g15::kLightDraw, {l, o, blk, x});
    }
    ret(ctx, ref<u8>(o + 20));  // r3 as the original leaves it (the count's last lbz)
}

// void RenderState_Reset_0053170C()
// (f_005325D8; name?)
// Puts the RSX in the renderer's default state (front face CW, cull
// back, cull on, depth LEQUAL, depth test on, primitive restart 0xFFFF on,
// all colour channels) and then turns cull, depth test, alpha test and
// blending off; then resets the render state's cache (fx::g::state):
// programs / textures / vertex arrays forgotten, the stencil, blend and
// surface fields to their defaults. r3 is left as the original leaves it
// (2).
DECOMP(0053170C, RenderState_Reset_0053170C) {
    const u32 st = fx::g::state;
    auto gcm1 = [&](u32 fn, u32 v) { call(ctx, fn, {load<u32>(st), v}); };
    gcm1(g15::fn::cellGcmSetFrontFace_5E3D68, g15::kCw);
    gcm1(g15::fn::cellGcmSetCullFace_5E3D20, 0x0405);   // BACK
    gcm1(g15::fn::cellGcmSetCullFaceEnable_5E3DF8, 1);
    gcm1(g15::fn::cellGcmSetDepthFunc_5E3D44, 0x0203);  // LEQUAL
    gcm1(g15::fn::cellGcmSetDepthTestEnable_5E3E1C, 1);
    gcm1(g15::fn::cellGcmSetRestartIndex_5E3F5C, 0xFFFF);
    gcm1(g15::fn::cellGcmSetRestartIndexEnable_5E3E40, 1);
    gcm1(g15::fn::cellGcmSetColorMask_5E3CFC, 0x01010101);
    const u32 gcm = load<u32>(st);
    if (ref<u8>(g15::g::state_reset_once) == 0) store<u8>(g15::g::state_reset_once, 1);
    call(ctx, g15::fn::cellGcmSetCullFaceEnable_5E3DF8, {gcm, 0});
    gcm1(g15::fn::cellGcmSetDepthTestEnable_5E3E1C, 0);
    gcm1(g15::fn::cellGcmSetAlphaTestEnable_5E3DB0, 0);
    gcm1(g15::fn::cellGcmSetBlendEnable_5E3DD4, 0);
    call(ctx, g15::fn::Gcm_5E3FA4, {load<u32>(st), 1, 0});
    store<u32>(st + 4, 0);
    store<u32>(st + 16, 0);
    store<s32>(st + 8, -1);
    store<u32>(st + 20, 0);
    store<u32>(st + 24, 0);
    store<u32>(st + 28, 0);
    store<u16>(st + 32, 0xFFFF);
    store<u32>(st + 88, 0);
    for (u32 k = 0; k < 32; k += 4) store<u32>(st + 52 + k, 0);  // the texture units
    store<s32>(st + 216, -1);
    store<u8>(st + 220, 0);
    store<u8>(st + 221, 0);
    const u32 dirty = load<u32>(st + 92) & 0x7FFFFFFFu;  // rlwinm 0,1,31
    constexpr u32 kAlways = 519, kKeep = 7680;  // GL_ALWAYS, GL_KEEP
    store<u32>(st + 160, kAlways);
    store<u32>(st + 164, 0);
    store<s32>(st + 168, -1);
    store<u32>(st + 92, dirty);
    store<s32>(st + 172, -1);
    store<u32>(st + 176, kKeep);
    store<u32>(st + 180, kKeep);
    store<u32>(st + 184, kKeep);
    store<u32>(st + 188, kAlways);
    store<u32>(st + 192, 0);
    store<s32>(st + 196, -1);
    store<s32>(st + 200, -1);
    store<u32>(st + 204, kKeep);
    store<u32>(st + 208, kKeep);
    store<u32>(st + 212, kKeep);
    for (u32 off : {36u, 40u, 44u, 48u, 84u}) store<u32>(st + off, 0);
    store<u8>(st + 93, 0);
    store<u8>(st + 96, 1);
    store<u8>(st + 97, 0);
    store<u8>(st + 98, 8);
    store<u8>(st + 99, 1);
    for (u32 k = 100; k < 104; ++k) store<u8>(st + k, 0);
    for (u32 k = 104; k < 120; k += 4) store<u32>(st + k, 0);
    for (u32 k = 120; k < 136; k += 4) store<u32>(st + k, 64);
    store<u8>(st + 136, 2);
    store<u8>(st + 137, 0);
    store<u32>(st + 140, 0);
    store<u32>(st + 144, 64);
    store<u16>(st + 148, 64);
    store<u16>(st + 150, 64);
    store<u16>(st + 152, 0);
    store<u16>(st + 154, 0);
    ctx.gpr[3] = 2;
    ctx.gpr[4] = 8;
}

// void GodRays_Update_005437F0(GodRays* o)
// [vtable 0x00B23FF0 slot 4; name?]
// Nothing unless slot 3 says the effect is active. Then slot 6 and
// Trail_UpdateFade_0051ED00. Disabled (u8 +20 bit 7 clear): flags +120
// bits 6 / 7 cleared and, while the fade +84 is above 0, slot 15 with f1
// = +84 * +116. Enabled: the first time (bit 6 clear) slots 8, 9(2) and
// bit 6 set, the next time (bit 6 set) slots 11, 12 (2, 0), (0, 2), (2,
// 1), 13 and bit 7 instead; then slot 15 with f1 = +116 once primed
// (bit 7), else as when disabled.
DECOMP(005437F0, GodRays_Update_005437F0) {
    namespace pv = g15::post_vt;
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<u32>(fx::vcall(ctx, o, pv::kIsActive, {o})) == 0) return;  // cmpwi
    fx::vcall(ctx, o, pv::kPrepare, {o});
    call(ctx, g15::fn::Trail_UpdateFade_0051ED00, {o});
    const f64 zero = lfs(g15::kGodRaysZero);
    if ((ref<u8>(o + 20) & g15::kPostOn) == 0) {  // rlwinm 25,31,31
        store<u8>(o + 120, static_cast<u8>(ref<u8>(o + 120) & ~(g15::kGodRaysStarted | g15::kGodRaysPrimed)));
        const f64 fade = lfs(o + 84);
        ctx.fpr[1] = fade;
        if (fade > zero) {  // fcmpu
            ctx.fpr[1] = sgl(fade * lfs(o + 116));
            fx::vcall(ctx, o, pv::kSetIntensity, {o});
        } else {
            ctx.gpr[3] = 0;  // r3 as the original leaves it
        }
        return;
    }
    u32 flags;
    if ((ref<u8>(o + 120) & g15::kGodRaysStarted) == 0) {  // rlwinm 26,31,31
        fx::vcall(ctx, o, pv::kBegin, {o});
        fx::vcall(ctx, o, pv::kBeginPass, {o, 2});
        flags = ref<u8>(o + 120) | g15::kGodRaysStarted;
    } else {
        fx::vcall(ctx, o, pv::kFirstFrame, {o});
        fx::vcall(ctx, o, pv::kPass, {o, 2, 0});
        fx::vcall(ctx, o, pv::kPass, {o, 0, 2});
        fx::vcall(ctx, o, pv::kPass, {o, 2, 1});
        fx::vcall(ctx, o, pv::kFirstFrameEnd, {o});
        flags = (ref<u8>(o + 120) & ~g15::kGodRaysStarted) | g15::kGodRaysPrimed;
    }
    store<u8>(o + 120, static_cast<u8>(flags));
    if ((flags & g15::kGodRaysPrimed) != 0) {  // rlwinm 25,31,31
        ctx.fpr[1] = lfs(o + 116);
        fx::vcall(ctx, o, pv::kSetIntensity, {o});
        return;
    }
    const f64 fade = lfs(o + 84);
    ctx.fpr[1] = fade;
    if (fade > zero) {
        ctx.fpr[1] = sgl(fade * lfs(o + 116));
        fx::vcall(ctx, o, pv::kSetIntensity, {o});
    } else {
        ctx.gpr[3] = g15::kGodRaysZero;  // r3 as the original leaves it (the constant's address)
    }
}

// void Skin_TransformPositions_00518748(SkinMesh* o, const SkinPose* pose)
// (Skin_Update_00518B64; name?)
// For the m = o->+0 mesh's +40 (s32) vertices: position i (o->+0x800C,
// 16-byte stride) times pose +80, through the 3x4 bone matrix (pose +88,
// 48 bytes each) of the vertex's bone (m +72, 8-byte entries, byte 0),
// remapped through m +84 [1 + bone] when m has that table, into o->+0x8010.
// Each row is y * m1, + x * m0, + z * m2 (fused), + m3. r3 / f1 are left
// as the original leaves them.
DECOMP(00518748, Skin_TransformPositions_00518748) {
    const u32 o = arg<u32>(ctx, 0), pose = arg<u32>(ctx, 1);
    const u32 m = load<u32>(o);
    const f64 scale = lfs(pose + 80);
    ctx.fpr[1] = scale;
    const s32 n = static_cast<s32>(load<u32>(m + 40));
    const u32 bones = load<u32>(m + 72), remap = load<u32>(m + 84);
    if (n <= 0) return;  // r3 stays o
    const u32 mats = load<u32>(pose + 88);
    const u32 src = load<u32>(o + 0x800C), dst = load<u32>(o + 0x8010);
    for (s32 i = 0; i < n; ++i) {
        const u32 b = ref<u8>(bones + 8 * static_cast<u32>(i));
        const u32 idx = remap != 0 ? ref<u8>(remap + 1 + b) : b;
        const u32 mx = mats + 48 * idx;
        const u32 p = src + 16 * static_cast<u32>(i);
        const f64 y = sgl(lfs(p + 4) * scale), x = sgl(lfs(p) * scale), z = sgl(lfs(p + 8) * scale);
        auto row = [&](u32 r) {
            f64 v = sgl(y * lfs(r + 4));
            v = g11::madds(x, lfs(r), v);
            v = g11::madds(z, lfs(r + 8), v);
            return sgl(v + lfs(r + 12));
        };
        const f64 ox = row(mx), oy = row(mx + 16), oz = row(mx + 32);
        const u32 q = dst + 16 * static_cast<u32>(i);
        store<u32>(q, g11::stfs_bits(ox));
        store<u32>(q + 4, g11::stfs_bits(oy));
        store<u32>(q + 8, g11::stfs_bits(oz));
    }
    ctx.gpr[3] = (remap != 0 ? dst : src) + 16 * static_cast<u32>(n);  // r3 as the original leaves it
}

// void Trail_Update_00544870(Trail* o)
// [vtable 0x00B24038 slot 4; name?]
// Nothing unless slot 3 says the effect is active. Ticks the countdown
// by the frame time (SystemState +4), slots 6, View_SaveRestore and slot
// 8; the time +32 += dt. Past the period +28 the position +48 moves
// toward +64 by (+32 - +28) / +32. Then while +32 is past the step +20:
// the point lerped from +48 toward +64 by +20 / +32 and +48 itself are
// projected (View_ProjectPoint) and slot 10 gets their screen delta {dx,
// dy, 0, 0} (and 1 the first time, else 0); +48 = that point, +32 -=
// +20. Draw_BindScreenTargets at the end.
DECOMP(00544870, Trail_Update_00544870) {
    namespace pv = g15::post_vt;
    const u32 o = arg<u32>(ctx, 0);
    if (static_cast<u32>(fx::vcall(ctx, o, pv::kIsActive, {o})) == 0) return;  // cmpwi
    const u32 sys = system_state::object;
    if (ref<u8>(system_state::guard) == 0) {  // system_state::get, with logged stores
        const u8 b32 = ref<u8>(sys + 32);
        store<u32>(sys + 20, 0);
        store<u32>(sys + 24, 0);
        store<u8>(sys + 32, static_cast<u8>(b32 & ~0xC0u));
        call(ctx, sly4::fn::memset, {sys + 92, 0, 400});
        store<u32>(system_state::registry_slot, sys);
        store<u8>(system_state::guard, 1);
    }
    const f64 dt = lfs(sys + 4);
    callf(ctx, g15::fn::Countdown_Tick, {o}, {dt});
    fx::vcall(ctx, o, pv::kPrepare, {o});
    call(ctx, g15::fn::View_SaveRestore_00543AAC, {o});
    fx::vcall(ctx, o, pv::kBegin, {o});
    const f64 one = lfs(g15::kTrailOne);
    f64 t = sgl(lfs(o + 32) + dt);
    const f64 period = lfs(o + 28);
    f64 step = lfs(o + 20);
    store<u32>(o + 32, g11::stfs_bits(t));
    auto lerp = [&](u32 k, f64 a) { return g11::madds(lfs(o + 48 + k), sgl(one - a), sgl(lfs(o + 64 + k) * a)); };
    if (t > period) {  // fcmpu
        const f64 a = sgl(sgl(t - period) / t);
        const f64 x = lerp(0, a), y = lerp(4, a), z = lerp(8, a);
        store<u32>(o + 48, g11::stfs_bits(x));
        store<u32>(o + 52, g11::stfs_bits(y));
        store<u32>(o + 56, g11::stfs_bits(z));
    }
    ctx.fpr[1] = step;
    if (t > step) {
        Stack frame(ctx, 64);
        const u32 p = frame.addr(), sa = p + 16, sb = p + 32, d = p + 48;
        u32 first = 1;
        do {
            const f64 a = sgl(step / t);
            const f64 x = lerp(0, a), y = lerp(4, a), z = lerp(8, a);
            store<u32>(p, g11::stfs_bits(x));
            store<u32>(p + 4, g11::stfs_bits(y));
            store<u32>(p + 8, g11::stfs_bits(z));
            call(ctx, g15::fn::View_ProjectPoint, {call(ctx, g15::fn::View_GetCurrent, {}), sa, p});
            call(ctx, g15::fn::View_ProjectPoint, {call(ctx, g15::fn::View_GetCurrent, {}), sb, o + 48});
            store<u32>(d + 8, 0);
            store<u32>(d + 12, 0);
            store<u32>(d, g11::stfs_bits(sgl(lfs(sa) - lfs(sb))));
            store<u32>(d + 4, g11::stfs_bits(sgl(lfs(sa + 4) - lfs(sb + 4))));
            fx::vcall(ctx, o, pv::kTrailStep, {o, d, first});
            first = 0;
            t = lfs(o + 32);
            step = lfs(o + 20);
            for (u32 k = 0; k < 12; k += 4) store<u32>(o + 48 + k, load<u32>(p + k));  // lfs / stfs
            t = sgl(t - step);
            store<u32>(o + 32, g11::stfs_bits(t));
            ctx.fpr[1] = step;
        } while (t > step);  // fcmpu
    }
    call(ctx, g15::fn::Draw_BindScreenTargets, {});
}

namespace {
// {+0 arg, +4 holder {+4 arg, +12 Obj*}}: the object's slot 2 with (obj,
// +0, holder +4).
void free_callback(PPUContext& ctx, u32 cb) {
    const u32 arg0 = load<u32>(cb);
    if (arg0 == 0) return;
    const u32 h = load<u32>(cb + 4);
    const u32 obj = load<u32>(h + 12);
    fx::vcall(ctx, obj, 8, {obj, arg0, load<u32>(h + 4)});
}
}  // namespace

// void Render_FreeLists_005AD15C()
// [shutdown table 0x00F10864; name?]
// Frees the renderer's lists: the transform list (when built), the bank
// globals' kind list and blocks (+64, +40, +48; counts +68, +5200 and
// 0x01193000 cleared), the 32 pools from the last one down (their
// objects released from the top, slot 2; s32 +208 ends at -1, as in the
// original; then the +72 block), callback_a, the 3 parts from the last
// (Part_Release_0052F06C and the block unless u8 +48; then
// Pool_DestructPart), callback_b and block_16bd08.
DECOMP(005AD15C, Render_FreeLists_005AD15C) {
    if (load<u64>(g15::g::tr_list_live) != 0) {
        call(ctx, g15::fn::PtrVector_Clear, {g15::g::tr_list});
        call(ctx, g15::fn::Nop, {g15::g::tr_list});
        store<u64>(g15::g::tr_list_live, 0);
    }
    const u32 bank = g15::g::bank_globals;
    call(ctx, g15::fn::PtrVector_Clear, {bank + 5220});
    call(ctx, g15::fn::Nop, {bank + 5220});
    if (load<u32>(bank + 64) != 0) {
        call(ctx, g15::fn::Mem_FreeFrom, {load<u32>(bank + 64), 0});
        store<u32>(bank + 64, 0);
    }
    const u32 b40 = load<u32>(bank + 40);
    store<u16>(bank + 68, 0);
    store<u16>(bank + 5200, 0);
    store<u16>(g15::g::bank_count_193000, 0);
    if (b40 != 0) {
        call(ctx, g15::fn::Mem_FreeFrom, {b40, 0});
        store<u32>(bank + 40, 0);
    }
    if (load<u32>(bank + 48) != 0) {
        call(ctx, g15::fn::Mem_FreeFrom, {load<u32>(bank + 48), 0});
        store<u32>(bank + 48, 0);
    }
    for (u32 j = 0; j < 32; ++j) {
        const u32 e = g15::g::pools_last - 272 * j;
        for (;;) {
            const s32 n = static_cast<s32>(load<u32>(e + 208)) - 1;
            store<u32>(e + 208, static_cast<u32>(n));
            if (n < 0) break;  // cmpwi
            const u32 obj = load<u32>(e + 216 + 4 * static_cast<u32>(n));
            fx::vcall(ctx, obj, 8, {obj});
        }
        if (load<u32>(e + 72) != 0) {
            call(ctx, g15::fn::Mem_FreeFrom, {load<u32>(e + 72), 0});
            store<u32>(e + 72, 0);
        }
    }
    free_callback(ctx, g15::g::callback_a);
    for (u32 j = 0; j < 3; ++j) {
        const u32 part = g15::g::parts_last - 52 * j + 44;
        if (ref<u8>(part + 48) == 0) {
            const u32 p = load<u32>(part + 40);
            if (p != 0) {
                call(ctx, g15::fn::Part_Release_0052F06C, {p});
                call(ctx, g15::fn::Mem_FreeFrom, {p, 0});
                store<u32>(part + 40, 0);
            }
        }
        call(ctx, g15::fn::Pool_DestructPart, {part});
    }
    free_callback(ctx, g15::g::callback_b);
    const u32 last = load<u32>(g15::g::block_16bd08);
    if (last != 0) {
        call(ctx, g15::fn::Mem_FreeFrom, {last, 0});
        store<u32>(g15::g::block_16bd08, 0);
        ctx.gpr[3] = 0x01170000;  // r3 as the original leaves it (lis)
    } else {
        ctx.gpr[3] = 0;
    }
}

namespace {
// Material_ParamSlot(m, k) = v.
void set_param15(PPUContext& ctx, u32 m, u32 k, u32 v) {
    store<u32>(static_cast<u32>(call(ctx, g15::fn::Material_ParamSlot, {m, k})), v);
}
// Material_AddTexture_004FF008(m, t), then t's slot 4 (+16) with (t, m->+20
// u16 +122 / +124, 1); `sizes_after`: the sequence leaves r7 = the OPD,
// r8 / r9 = the sizes (else r7 = the OPD, r8 = its entry).
void add_sized_texture15(PPUContext& ctx, u32 m, u32 t, bool sizes_after) {
    call(ctx, g15::fn::Material_AddTexture_004FF008, {m, t});
    const u32 sz = load<u32>(m + 20);
    const u32 w = load<u16>(sz + 122), h = load<u16>(sz + 124);
    const u32 opd = load<u32>(load<u32>(t) + 16);
    if (sizes_after) call_ptr(ctx, opd, {t, w, h, 1, opd, w, h});
    else call_ptr(ctx, opd, {t, w, h, 1, opd, load<u32>(opd)});
}
}  // namespace

// bool LoadParams_005C3528(ShaderType*, Material* m, const Desc* d)
// [vtable 0x00B25AF0 slot 3, a shader type's LoadParams; name?]
// Nothing for desc version (u8 d+84) 0. The texture named d+4 (slot 4
// with (t, 1, 3, 1)) is added, m+120's top nibble = 2 when u8 d+76 == 2,
// else 3; without it the default texture (kind 5) is added with 3.
// Unless config +152 bit 7: the texture named d+8 is added and sized,
// params 0..2 = d+88 / +92 / +96 and the texture named d+12 added and
// sized; without d+8's texture, the one named d+16 (if loaded) gets its
// slot 3 instead. Param 4 = (version > 4 ? d+104 : 15), 0 when not
// positive. Returns 1.
DECOMP(005C3528, LoadParams_005C3528) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    if (static_cast<s32>(ref<u8>(d + 84)) <= 0) return ret(ctx, 1u);  // cmpwi
    const u32 t = static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {load<u32>(d + 4)}));
    u32 kind;
    if (t != 0) {
        fx::vcall(ctx, t, 16, {t, 1, 3, 1});
        call(ctx, g15::fn::Material_AddTexture_004FF008, {m, t});
        kind = ref<u8>(d + 76) == 2 ? 2u : 3u;
    } else {
        call(ctx, g15::fn::Material_AddTexture_004FF008, {m, static_cast<u32>(call(ctx, g15::fn::Tex_GetDefault, {g14::kDefaultTexKind}))});
        kind = 3;
    }
    store<u32>(m + 120, (load<u32>(m + 120) & 0x0FFFFFFFu) | (kind << 28));  // rlwinm 0,4,31; rlwimi 28
    if ((field<u8>(sly4::g::config, 152) & 0x80) == 0) {  // rlwinm 25,31,31
        const u32 t2 = static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {load<u32>(d + 8)}));
        if (t2 != 0) {
            add_sized_texture15(ctx, m, t2, true);
            for (u32 k = 0; k < 3; ++k) set_param15(ctx, m, k, load<u32>(d + 88 + 4 * k));  // lfs / stfs
            const u32 t3 = static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {load<u32>(d + 12)}));
            if (t3 != 0) add_sized_texture15(ctx, m, t3, false);
        } else if (load<u32>(d + 16) != 0 && static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {load<u32>(d + 16)})) != 0) {
            const u32 t4 = static_cast<u32>(call(ctx, g15::fn::Tex_FindLoaded, {load<u32>(d + 16)}));
            fx::vcall(ctx, t4, 12, {t4});
        }
    }
    constexpr u32 K = g15::kLoadParams5C3518;
    const f64 x = static_cast<s32>(ref<u8>(d + 84)) > 4 ? lfs(d + 104) : lfs(K);  // cmpwi
    const f64 v = -x >= 0.0 ? g11::lfd(K + 8) : x;  // fneg, fsel
    ctx.fpr[1] = v;
    set_param15(ctx, m, 4, g11::stfs_bits(sgl(v)));  // frsp
    ret(ctx, 1u);
}
