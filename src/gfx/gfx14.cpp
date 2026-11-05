// The renderer range's next recompiled functions: the callees gfx13*.cpp
// left (the post-effect chain's constructor, the effect slot submit, ...)
// and the next smallest ones. Names carry the address where the role is
// unclear.
#include <atomic>
#include <bit>
#include <cmath>

#include "fx.h"
#include "gfx14.h"

using namespace sly4;

namespace {

namespace g11 = sly4::render::gfx11;
namespace g12 = sly4::render::gfx12;
namespace g13 = sly4::render::gfx13;
namespace g14 = sly4::render::gfx14;
namespace g10 = sly4::render::gfx10;
using g11::lfs;
using g11::sgl;

// The post-effect base's header: u8 +4 / +5 / +12 = 0 and the base
// vtable (the stores come in this order everywhere it is inlined).
void post_effect_base(u32 e, u32 vt) {
    store<u8>(e + 4, 0);
    store<u8>(e + 5, 0);
    store<u8>(e + 12, 0);
    store<u32>(e, vt);
}

// The name +8 and its hash +16.
void post_effect_name(PPUContext& ctx, u32 e, u32 name) {
    store<u32>(e + 8, name);
    store<u32>(e + 16, static_cast<u32>(call(ctx, sly4::fn::String_HashNoCase, {name})));
}

// Copies three f32 words (lfs / stfs: raw).
void copy_vec3(u32 dst, u32 src) {
    for (u32 k = 0; k < 3; ++k) store<u32>(dst + 4 * k, load<u32>(src + 4 * k));
}

// The 16-byte slot the chain keeps at +6112 / +6128 / +6144: +0 = 0, +4 =
// 1, +8 = 2 (the original masks and
// inserts bits that always give these), +12 keeps its low 28 bits. `order_8_first`: the loop stores +8 before +4.
void chain_slot(u32 s, bool order_8_first) {
    const u32 a = (load<u32>(s + 4) & 1u) | 1u;     // rlwinm 0,31,31; ori 1: always 1
    const u32 b = (load<u32>(s + 8) & 0xEu & ~0xEu) | 2u;  // rlwinm 0,28,30; rlwimi (1 << 1),28,30: always 2
    const u32 c = load<u32>(s + 12) & 0x0FFFFFFFu;       // rlwinm 0,4,31
    store<u32>(s, 0);
    if (order_8_first) {
        store<u32>(s + 8, b);
        store<u32>(s + 4, a);
    } else {
        store<u32>(s + 4, a);
        store<u32>(s + 8, b);
    }
    store<u32>(s + 12, c);
}

}  // namespace

// void PostChain_Construct_0051F850(PostChain* c)
// (RenderTarget_Setup_005203A8; name?)
// Builds the 6848-byte post-effect chain in place: the chain's base
// vtable, then each effect (the post-effect base header, its fields, the
// name and its hash, then its own vtable): Bloom +48, DoF +112,
// Desaturation +192, Blur +256, RadialBlur +284, ScreenWarp +316
// (ScreenWarp_Construct_0051F1C0), three 16-byte slots +6112..+6144 and
// u32 +6160 = 1, GodRays +6176, Trail +6336 (Trail_Construct_0051EF28),
// BloomNoise +6560 (its colour bytes +92..+95 are fctiwz of 255 x {0.5,
// 0.4, 0.2, 0.9}) and the last effect +6736 (Construct_0051F708). The
// 10 effects go into +4..+40 in update order, and each gets its vtable
// slot 2 called with (effect, 0).
DECOMP(0051F850, PostChain_Construct_0051F850) {
    namespace pc = g14::post_chain;
    const u32 c = arg<u32>(ctx, 0);
    constexpr u32 kOne = 0x3F800000u, k64 = 0x42800000u, k3 = 0x40400000u, k255 = 0x437F0000u;
    store<u32>(c, g14::vt_post_chain_base);

    // Bloom.
    const u32 bloom = c + pc::kBloom;
    post_effect_base(bloom, g14::vt_post_effect);
    store<u32>(bloom + 32, k64);
    store<u32>(bloom + 36, k64);
    store<u32>(bloom + 40, k64);
    store<u32>(bloom + 48, k3);
    store<u8>(bloom + 52, 4);
    post_effect_name(ctx, bloom, g14::str_bloom);

    // DoF.
    const u32 dof = c + pc::kDoF;
    store<u32>(bloom, g14::vt_bloom);
    store<u8>(dof + 4, 0);
    store<u8>(dof + 5, 0);
    store<u32>(dof + 8, 0);
    store<u8>(dof + 12, 0);
    store<u32>(dof, g14::vt_dof_base);
    store<u8>(dof + 20, 0);
    store<u32>(dof + 24, 0x41200000u);  // 10
    store<u32>(dof + 28, 0x40000000u);  // 2
    store<u32>(dof + 32, kOne);
    store<u32>(dof + 48, load<u32>(g14::kVec3_164B40));
    store<u32>(dof + 52, load<u32>(g14::kVec3_164B40 + 4));
    store<u8>(dof + 64, 1);
    store<u32>(dof + 56, load<u32>(g14::kVec3_164B40 + 8));
    post_effect_name(ctx, dof, g14::str_dof);

    // Desaturation.
    const u32 de = c + pc::kDesaturation;
    store<u32>(dof, g14::vt_dof);
    post_effect_base(de, g14::vt_desaturation_base);
    store<u32>(de + 20, kOne);
    const f64 f255 = lfs(g14::kPostChainConsts);       // f30
    store<u32>(de + 32, k255);
    store<u32>(de + 36, k255);
    store<u32>(de + 40, k255);
    post_effect_name(ctx, de, g14::str_desaturation);

    // Blur.
    store<u8>(c + 260, 0);
    store<u8>(c + 261, 0);
    store<u8>(c + 268, 0);
    store<u32>(de, g14::vt_desaturation);
    store<u32>(c + pc::kBlur, g14::vt_blur_base);
    store<u8>(c + 276, 4);
    store<u32>(c + 280, 0);
    post_effect_name(ctx, c + pc::kBlur, g14::str_blur);

    // RadialBlur.
    const u32 rb = c + pc::kRadialBlur;
    store<u8>(rb + 4, 0);
    store<u8>(rb + 5, 0);
    store<u8>(rb + 12, 0);
    store<u8>(rb + 20, 1);
    store<u32>(c + pc::kBlur, g14::vt_blur);
    store<u32>(rb, g14::vt_radial_blur_base);
    store<u32>(rb + 24, 0x3D4CCCCDu);  // 0.05
    const f64 f09 = lfs(g14::kPostChainConsts + 4);    // f31
    store<u32>(rb + 28, 0x3F666666u);  // 0.9
    post_effect_name(ctx, rb, g14::str_radial_blur);
    store<u32>(rb, g14::vt_radial_blur);

    // ScreenWarp and the three slots.
    const u32 sw = c + pc::kScreenWarp;
    call(ctx, g14::fn::ScreenWarp_Construct_0051F1C0, {sw});
    store<u8>(c + 6092, 0);
    store<u32>(c + 6096, 0);
    store<u8>(c + 6100, 8);
    store<u32>(sw, g14::vt_screen_warp2);
    store<u8>(c + 6101, 8);
    store<u32>(c + 6104, 0);
    store<u32>(c + 6108, 0);
    chain_slot(c + 6112, true);
    chain_slot(c + 6128, true);
    chain_slot(c + 6144, false);
    store<u32>(c + 6160, 1);

    // GodRays.
    const u32 gr = c + pc::kGodRays;
    const u8 b20 = static_cast<u8>((load<u8>(gr + 20) & 0x7F) | 0x40);  // rlwimi 0,24,24; ori 0x40
    store<u8>(gr + 4, 0);
    store<u8>(gr + 5, 0);
    store<u32>(gr + 8, 0);
    store<u8>(gr + 12, 0);
    store<u32>(gr, g14::vt_god_rays_base);
    store<u8>(gr + 20, b20);
    store<u32>(gr + 24, 0);
    store<u32>(gr + 28, 0);
    store<u32>(gr + 32, kOne);
    store<u32>(gr + 36, kOne);
    store<u32>(gr + 48, load<u32>(g14::kVec3_164B40));
    store<u32>(gr + 52, load<u32>(g14::kVec3_164B40 + 4));
    store<u32>(gr + 60, 0x43480000u);  // 200
    store<u32>(gr + 56, load<u32>(g14::kVec3_164B40 + 8));
    for (u32 k = 0; k < 3; ++k) store<u32>(gr + 64 + 4 * k, load<u32>(g14::kVec4_164BC0 + 4 * k));
    store<u32>(gr + 80, 0);
    store<u32>(gr + 76, load<u32>(g14::kVec4_164BC0 + 12));
    store<u32>(gr + 96, 0x3E3020C5u);
    store<u32>(gr + 100, 0x3E22D0E5u);
    store<u32>(gr + 104, 0x3F70A3D7u);
    store<u32>(gr + 108, 0x3ED70A3Du);
    for (u32 k = 0; k < 4; ++k) store<u8>(gr + 112 + k, load<u8>(g14::g::white + k));
    store<u32>(gr + 116, kOne);
    store<u32>(gr + 84, 0);
    post_effect_name(ctx, gr, g14::str_god_rays);
    const u8 b120 = static_cast<u8>(load<u8>(gr + 120) & 0x3F);  // rlwimi 0,24,24 / 0,25,25
    store<u32>(gr, g14::vt_god_rays);
    store<u8>(gr + 120, b120);

    // Trail.
    const u32 tr = c + pc::kTrail;
    call(ctx, g14::fn::Trail_Construct_0051EF28, {tr});
    store<u32>(c + 6496, 0);
    store<u32>(tr, g14::vt_trail);

    // BloomNoise.
    const u32 bn = c + pc::kBloomNoise;
    store<u8>(bn + 4, 0);
    store<u8>(bn + 5, 0);
    store<u32>(bn + 8, 0);
    store<u8>(bn + 12, 0);
    store<u32>(bn, g14::vt_bloom_noise_base);
    store<u32>(bn + 20, 0x3F000000u);  // 0.5
    store<u32>(bn + 24, 0x3F000000u);
    store<u32>(bn + 28, 0x3BE56042u);  // 0.007
    store<u32>(bn + 32, 0xBC1374BCu);  // -0.009
    store<u32>(bn + 36, 0);
    store<u32>(bn + 40, 0x3F000000u);
    store<u32>(bn + 44, 0x3F400000u);  // 0.75
    store<u32>(bn + 48, 0x3F400000u);
    store<u32>(bn + 52, 0xBD0F5C29u);  // -0.035
    store<u32>(bn + 56, 0x3D19999Au);  // 0.0375
    store<u32>(bn + 60, 0xBE4CCCCDu);  // -0.2
    store<u32>(bn + 64, 0x3E800000u);  // 0.25
    store<u32>(bn + 68, kOne);
    store<u32>(bn + 72, kOne);
    store<u32>(bn + 76, 0xBD4CCCCDu);  // -0.05
    const f64 r = sgl(lfs(g14::kPostChainConsts + 8) * f255);
    store<u32>(bn + 80, 0xBD75C28Fu);  // -0.06
    const f64 a = sgl(f09 * f255);
    store<u32>(bn + 84, 0x3E99999Au);  // 0.3
    store<u32>(bn + 88, 0x3E000000u);  // 0.125
    const f64 gg = sgl(lfs(g14::kPostChainConsts + 12) * f255);
    const f64 b = sgl(lfs(g14::kPostChainConsts + 16) * f255);
    store<u8>(bn + 92, static_cast<u8>(rt::fctiw(r, true)));
    store<u8>(bn + 93, static_cast<u8>(rt::fctiw(gg, true)));
    store<u8>(bn + 94, static_cast<u8>(rt::fctiw(b, true)));
    store<u8>(bn + 95, static_cast<u8>(rt::fctiw(a, true)));
    store<u32>(bn + 96, 0x40800000u);   // 4
    store<u32>(bn + 100, kOne);
    store<u32>(bn + 108, 0x43160000u);  // 150
    store<u32>(bn + 116, 0x40800000u);
    store<u32>(bn + 112, 0x43610000u);  // 225
    store<u32>(bn + 120, 0x40800000u);
    store<u32>(bn + 124, 0x3B03126Fu);  // 0.002
    store<u32>(bn + 128, 0x3B03126Fu);
    store<u32>(bn + 144, k64);
    store<u32>(bn + 148, k64);
    store<u32>(bn + 152, k64);
    store<u32>(bn + 160, k3);
    store<u32>(bn + 164, kOne);
    ctx.fpr[1] = std::bit_cast<f64>(rt::fctiw(r, true));  // f1 keeps the fctiwz word (seen by later callees)
    post_effect_name(ctx, bn, g14::str_bloom_noise);

    // The last effect, then the update list.
    const u32 last = c + pc::kLast;
    store<u32>(bn, g14::vt_bloom_noise);
    call(ctx, g14::fn::Construct_0051F708, {last});
    store<u32>(c + 8, bloom);
    store<u32>(c + 12, dof);
    store<u32>(c + 24, de);
    store<u32>(c + 16, c + pc::kBlur);
    store<u32>(last, g14::vt_last);
    store<u32>(c + 20, rb);
    store<u32>(c + 28, sw);
    store<u32>(c + 32, gr);
    store<u32>(c + 36, tr);
    store<u32>(c + 40, bn);
    store<u32>(c + 4, last);
    for (u32 k = 0; k < pc::kNumEffects; ++k) {
        const u32 e = load<u32>(c + pc::kEffects + 4 * k);
        fx::vcall(ctx, e, 8, {e, 0});
    }
}

// u32 EffectSlot_Submit_005AB908(View* view, EffectSlot* s)
// (EffectSlots_Cull_005AC280: the visible slots without a +0 owner; name?)
// d = view+864 - owner+240 (the owner s->+8, its position), n = owner+112
// (its axis): the sign k = 1 when dot(n, d) > 0, else -1; the plane
// {k n, -(k n . pos)} goes on the stack. When the material s->+12 has
// u16 +116 set and k < 0, or |k| < 0.01, the material just gets the
// default texture (kind 5) in slot 20. Otherwise the slot's texture
// (EffectSlot_Texture_005AB5F0) is bound to the render target s->+24,
// and EffectSlot_Draw_005AA998 draws with the fade (1 - clamp01((|d| -
// 0.4 r) / (r - 0.4 r)) * 0.8) * s->+16 (r = s->+20), then the
// material gets the target's +516 texture and +880 matrix. Returns the
// last call's r3 (r3 untouched for a null view).
DECOMP(005AB908, EffectSlot_Submit_005AB908) {
    constexpr u32 K = g14::kSlotSubmitConsts;
    const u32 view = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    if (view == 0) return;
    const u32 v = view + 864, owner = load<u32>(s + 8), p = owner + 240, n = owner + 112;
    const f64 d1 = sgl(lfs(v + 4) - lfs(p + 4));
    const f64 d0 = sgl(lfs(v) - lfs(p));
    const f64 d2 = sgl(lfs(v + 8) - lfs(p + 8));
    const f64 n1 = lfs(n + 4), n0 = lfs(n), n2 = lfs(n + 8);
    const f64 zero = lfs(K);
    f64 dot = sgl(n1 * d1);
    dot = g11::madds(n0, d0, dot);
    dot = g11::madds(n2, d2, dot);
    const f64 k = dot > zero ? lfs(K + 8) : lfs(K + 4);
    const u32 mat = load<u32>(s + 12);
    const f64 a0 = sgl(n0 * k), a1 = sgl(n1 * k), a2 = sgl(n2 * k);
    Stack frame(ctx, 16);
    const u32 plane = frame.addr();
    store<u32>(plane, g11::stfs_bits(a0));
    store<u32>(plane + 4, g11::stfs_bits(a1));
    store<u32>(plane + 8, g11::stfs_bits(a2));
    const f64 one = lfs(K + 8);
    store<u32>(plane + 12, 0x3F800000u);
    f64 w = sgl(a1 * lfs(p + 4));
    w = g11::madds(a0, lfs(p), w);
    w = sgl(-std::fma(a2, lfs(p + 8), w));  // fnmadds
    store<u32>(plane + 12, g11::stfs_bits(w));
    // f1 as the original leaves it on this path: 0 (the constant) when the
    // u16 test decides, else |k|.
    const bool facing_away = load<u16>(mat + 116) != 0 && k < zero;
    ctx.fpr[1] = facing_away ? zero : std::fabs(k);
    if (facing_away || std::fabs(k) < lfs(K + 12)) {
        const u32 tex = static_cast<u32>(call(ctx, g11::fn::Tex_GetDefault, {g14::kDefaultTexKind}));
        call(ctx, g11::fn::Obj_SetSlot20, {mat, tex, 1});
        return;
    }
    const u32 tex = static_cast<u32>(call(ctx, g14::fn::EffectSlot_Texture_005AB5F0, {s + 28, load<u32>(s + 40), owner, view}));
    const u32 rt = load<u32>(s + 24);
    call(ctx, g14::fn::RenderTarget_BindTexture_005229B0, {rt, tex});
    const f64 inner = lfs(K + 16);  // 0.4
    const f64 r = lfs(s + 20);
    const f64 span = g11::nmsubs(r, inner, r);  // r - 0.4 r
    f64 len = sgl(d1 * d1);
    len = g11::madds(d0, d0, len);
    len = g11::madds(d2, d2, len);
    len = sgl(std::sqrt(len));  // fsqrts
    f64 t = sgl(g11::nmsubs(r, inner, len) / span);
    t = sgl(sgl(one - t) >= 0.0 ? t : g11::lfd(K + 24));  // fsel: at most 1
    t = sgl(t >= 0.0 ? t : g11::lfd(K + 32));             // fsel: at least 0
    t = g11::nmsubs(t, lfs(K + 40), one);
    t = sgl(t * lfs(s + 16));
    callf(ctx, g14::fn::EffectSlot_Draw_005AA998, {view, rt, plane, load<u32>(s + 8), load<u32>(s + 4)}, {t});
    call(ctx, g14::fn::Material_SetMatrixParams_005AA718, {mat, load<u32>(rt + 516), rt + 880});
}

namespace {

// A LoadParams texture: added to m (Material_AddTexture_004FF008), then
// sized to m+20's u16 +122 / +124 through its virtual slot 4 (+16): (t,
// w, h, 1). `sizes_after`: the sequence leaves r7 = the OPD, r8 / r9 =
// the sizes (else r7 = the OPD, r8 = its entry).
void add_sized_texture14(PPUContext& ctx, u32 m, u32 t, bool sizes_after) {
    call(ctx, g11::fn::Material_AddTexture_004FF008, {m, t});
    const u32 sz = load<u32>(m + 20);
    const u32 w = load<u16>(sz + 122), h = load<u16>(sz + 124);
    const u32 opd = load<u32>(load<u32>(t) + 16);
    if (sizes_after) call_ptr(ctx, opd, {t, w, h, 1, opd, w, h});
    else call_ptr(ctx, opd, {t, w, h, 1, opd, load<u32>(opd)});
}

// Material_ParamSlot(m, k) = v (lfs / stfs: raw).
void set_param14(PPUContext& ctx, u32 m, u32 k, u32 v) {
    store<u32>(static_cast<u32>(call(ctx, g12::fn::Material_ParamSlot, {m, k})), v);
}

}  // namespace

namespace {

// The body LoadParams_005B9A1C and LoadParams_005C67E0 share: the
// texture named d+4 (Tex_FindLoaded; none: returns false) gets its
// virtual slot 4 called with (t, 1, 3, 1) and is added to m; m+120's top
// nibble = 2 when u8 d+76 == 2, else 3. Unless config +152 bit 7, with a
// desc version (u8 d+84) the texture named d+8 is added and sized,
// params 0..2 = d+88 / +92 / +96, and unless config +152 bit 6 the
// texture named d+12 is added and sized too.
bool load_params_tex14(PPUContext& ctx, u32 m, u32 d) {
    constexpr u32 kCfg = sly4::g::config;
    const u32 t = static_cast<u32>(call(ctx, g11::fn::Tex_FindLoaded, {load<u32>(d + 4)}));
    if (t == 0) return false;
    fx::vcall(ctx, t, 16, {t, 1, 3, 1});
    call(ctx, g11::fn::Material_AddTexture_004FF008, {m, t});
    const u32 kind = load<u8>(d + 76) == 2 ? 2u : 3u;
    store<u32>(m + 120, (load<u32>(m + 120) & 0x0FFFFFFFu) | (kind << 28));  // rlwinm 0,4,31; rlwimi 28,2,3
    if ((load<u8>(kCfg + 152) & 0x80) != 0) return true;
    if (static_cast<s32>(load<u8>(d + 84)) <= 0) return true;
    const u32 t2 = static_cast<u32>(call(ctx, g11::fn::Tex_FindLoaded, {load<u32>(d + 8)}));
    if (t2 == 0) return true;
    add_sized_texture14(ctx, m, t2, true);
    for (u32 k = 0; k < 3; ++k) set_param14(ctx, m, k, load<u32>(d + 88 + 4 * k));
    if ((load<u8>(kCfg + 152) & 0x40) != 0) return true;
    const u32 t3 = static_cast<u32>(call(ctx, g11::fn::Tex_FindLoaded, {load<u32>(d + 12)}));
    if (t3 != 0) add_sized_texture14(ctx, m, t3, false);
    return true;
}

}  // namespace

// bool LoadParams_005B9A1C(ShaderType*, Material* m, const Desc* d)
// [a shader type's LoadParams] (name?)
// load_params_tex14. Returns 1 once the first texture is found, else 0.
DECOMP(005B9A1C, LoadParams_005B9A1C) {
    ret(ctx, load_params_tex14(ctx, arg<u32>(ctx, 1), arg<u32>(ctx, 2)) ? 1u : 0u);
}

// bool LoadParams_005C67E0(ShaderType*, Material* m, const Desc* d)
// [a shader type's LoadParams] (name?)
// load_params_tex14, then param 3 = 1.0 when desc version (u8 d+84) > 3
// and d+100 is set, else 0.0 (kParamFlag_5C67D8 {1, 0}). Returns 1
// once the first texture is found, else 0.
DECOMP(005C67E0, LoadParams_005C67E0) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    if (!load_params_tex14(ctx, m, d)) return ret(ctx, 0u);
    const bool on = static_cast<s32>(load<u8>(d + 84)) > 3 && load<u32>(d + 100) != 0;
    const u32 k = g14::kParamFlag_5C67D8 + (on ? 0u : 4u);
    set_param14(ctx, m, 3, load<u32>(k));  // lfs / stfs: raw
    ret(ctx, 1u);
}

namespace {

// A new 52-byte surface (Mem_Alloc(52, 0, 16)) with its header set as
// inlined: +0 = 0, +40 = 0, +4 = 1, +8 = 2, +12 keeps its low 28 bits
// (the original masks and inserts bits that always give 1 and 2).
// `order` picks the inlined store sequence: 0 = +4, +8, +12; 1 = +12,
// +4, +8. Returns 0 when the allocation fails.
u32 new_surface14(PPUContext& ctx, int order) {
    const u32 s = static_cast<u32>(call(ctx, fn::Mem_Alloc, {52, 0, 16}));
    if (s == 0) return 0;
    const u32 a = (load<u32>(s + 4) & 1u) | 1u;
    const u32 b = (load<u32>(s + 8) & 0xEu & ~0xEu) | 2u;
    const u32 c = load<u32>(s + 12) & 0x0FFFFFFFu;
    store<u32>(s, 0);
    store<u32>(s + 40, 0);
    if (order == 0) {
        store<u32>(s + 4, a);
        store<u32>(s + 8, b);
        store<u32>(s + 12, c);
    } else {
        store<u32>(s + 12, c);
        store<u32>(s + 4, a);
        store<u32>(s + 8, b);
    }
    return s;
}

}  // namespace

// void Surface_AttachDepth_00549160(Effect* o, Pass* p)   [virtual] (name?)
// The display d = (p->+28)->+512: its depth target slot is picked as
// Draw_BindScreenTargets does (d+24 unless d+52 bit 25; then d+72 / +88
// by d+148 bit 7 unless config +34 and +35 are both set; then d+120 when
// the screen's +184 == 1, else d+124). A new surface goes to o+644 and
// the slot is attached to o+640 (Target_Attach with o's u16 +32 / +34 /
// +28 / +30), then bound (Surface_SetBuffer). When the target's +24 kind
// is 4 or 5 it is switched to 6 / 7 for a second new surface (o+648,
// Surface_SetBuffer), then restored, and o's virtual slot 3 (+12) runs.
// Quirk: a failed allocation stores 0 and is still passed on.
DECOMP(00549160, Surface_AttachDepth_00549160) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    constexpr u32 kCfg = sly4::g::config;
    const u32 d = load<u32>(load<u32>(p + 28) + 512);
    u32 slot;
    if (((load<u32>(d + 52) >> 25) & 1) == 0) slot = d + 24;  // rlwinm 7,31,31
    else if (load<u8>(kCfg + 35) == 0 || load<u8>(kCfg + 34) == 0)
        slot = ((load<u8>(d + 148) >> 7) & 1) != 0 ? d + 88 : d + 72;  // rlwinm 25,31,31
    else
        slot = load<u32>(g12::g::screen + 184) == 1 ? d + 120 : d + 124;
    store<u32>(o + 644, new_surface14(ctx, 0));
    call(ctx, tgt::fn::Target_Attach, {o + 640, slot, load<u16>(o + 32), load<u16>(o + 34), load<u16>(o + 28), load<u16>(o + 30)});
    call(ctx, tgt::fn::Surface_SetBuffer, {load<u32>(o + 644), load<u32>(o + 640)});
    const u32 t = load<u32>(o + 640);
    const s32 kind = static_cast<s32>(load<u32>(t + 24));
    if (kind != 4 && kind != 5) return;
    store<u32>(t + 24, kind == 4 ? 6u : 7u);
    const u32 s2 = new_surface14(ctx, 1);
    const u32 t2 = load<u32>(o + 640);
    store<u32>(o + 648, s2);
    call(ctx, tgt::fn::Surface_SetBuffer, {s2, t2});
    store<u32>(load<u32>(o + 640) + 24, static_cast<u32>(kind));
    fx::vcall(ctx, o, 12, {o});
}

namespace {

// The body LoadParams_005D20E0 and LoadParams_005D7DE8 share: the
// texture named d+4 (Tex_FindLoaded; none: returns 0) gets its virtual
// slot 4 called with (t, 1, 1, 1) and is added to m; params 0..7 =
// d+88..+116. From desc version (u8 d+84) 9 on, d+120 and d+124 are
// turned into radians in place (x the f32 at `deg_to_rad`) and become
// params 8 / 9; before that params 8 / 9 = 0. Returns 1.
void load_params_rad14(PPUContext& ctx, u32 deg_to_rad) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    const u32 t = static_cast<u32>(call(ctx, g11::fn::Tex_FindLoaded, {load<u32>(d + 4)}));
    if (t == 0) return ret(ctx, 0u);
    fx::vcall(ctx, t, 16, {t, 1, 1, 1});
    call(ctx, g11::fn::Material_AddTexture_004FF008, {m, t});
    for (u32 k = 0; k < 8; ++k) set_param14(ctx, m, k, load<u32>(d + 88 + 4 * k));
    if (static_cast<s32>(load<u8>(d + 84)) > 8) {
        const f64 rad = lfs(deg_to_rad);
        for (u32 k = 0; k < 2; ++k) {
            const u32 a = d + 120 + 4 * k;
            const f64 x = sgl(lfs(a) * rad);
            store<u32>(a, g11::stfs_bits(x));
            ctx.fpr[1] = x;  // f1 keeps the last product
            set_param14(ctx, m, 8 + k, load<u32>(a));
        }
    } else {
        set_param14(ctx, m, 8, 0);
        set_param14(ctx, m, 9, 0);
    }
    ret(ctx, 1u);
}

}  // namespace

// bool LoadParams_005D20E0(ShaderType*, Material* m, Desc* d)
// [a shader type's LoadParams] (name?)
// load_params_rad14 with its own pi / 180 constant.
DECOMP(005D20E0, LoadParams_005D20E0) { load_params_rad14(ctx, g14::kDegToRad_5D20D8); }

// bool LoadParams_005D7DE8(ShaderType*, Material* m, Desc* d)
// [a shader type's LoadParams] (name?)
// The same as LoadParams_005D20E0 (load_params_rad14), with its own
// pi / 180 constant.
DECOMP(005D7DE8, LoadParams_005D7DE8) { load_params_rad14(ctx, g14::kDegToRad_5D7DE4); }

// bool Emitter_PrepareDraw_0050809C(Emitter* e, Item* item, a, b, c, u32 out[4])
// [virtual] (name?)
// Null `out`: returns 0. Clears out[0..3], then unless
// Emitter_Check_00506050(e, item, a, b, c) says no (returns 0), builds
// the draw record from the frame stack (g12::g::frame_stack) into
// item+112 by the properties' u8 +68 kind: kind 0, 32 bytes: {e, e+72,
// out, the e+68 record's +0 (or 0), u16 its +4 (or 0)}; kind 1, 128
// bytes: the same head, +120 = u8 props+88 (the layer count), u16 +116
// = props+90, and per layer i (props+72[i]): +32 / +48 / +64 [i] = its
// +76 / +40 / +64, and when +64 is set, +80 [i] = its +60, u8 +112 [i]
// = its +48 bit 0, +96 [i] = its +48 >> 1. Returns 1 (any other kind:
// 0, nothing built).
DECOMP(0050809C, Emitter_PrepareDraw_0050809C) {
    const u32 e = arg<u32>(ctx, 0), item = arg<u32>(ctx, 1);
    const u32 a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3), c = arg<u32>(ctx, 4), out = arg<u32>(ctx, 5);
    if (out == 0) return ret(ctx, 0u);
    store<u32>(out, 0);
    store<u32>(out + 8, 0);
    store<u32>(out + 12, 0);
    store<u32>(out + 4, 0);
    if (static_cast<u32>(call(ctx, g14::fn::Emitter_Check_00506050, {e, item, a, b, c})) == 0) return ret(ctx, 0u);
    const u32 kind = load<u8>(load<u32>(e + 4) + 68);
    if (kind > 1) return ret(ctx, 0u);
    const u32 p = static_cast<u32>(call(ctx, g12::fn::MemStack_Alloc, {load<u32>(g12::g::frame_stack), kind == 0 ? 32u : 128u}));
    store<u32>(p, e);
    store<u32>(p + 8, out);
    store<u32>(p + 4, load<u32>(e + 72));
    const u32 r = load<u32>(e + 68);
    store<u32>(p + 12, r != 0 ? load<u32>(r) : 0u);
    const u32 r2 = load<u32>(e + 68);
    store<u16>(p + 16, r2 != 0 ? load<u16>(r2 + 4) : u16{0});
    store<u32>(item + 112, p);
    if (kind == 0) return ret(ctx, 1u);
    const u32 n = load<u8>(load<u32>(e + 4) + 88);
    store<u32>(p + 120, n);
    store<u16>(p + 116, load<u16>(load<u32>(e + 4) + 90));
    if (n == 0) return ret(ctx, 1u);
    for (u32 i = 0; i < load<u32>(p + 120); ++i) {
        const u32 l = load<u32>(load<u32>(e + 4) + 72 + 4 * i);
        store<u32>(p + 32 + 4 * i, load<u32>(l + 76));
        store<u32>(p + 48 + 4 * i, load<u32>(l + 40));
        store<u32>(p + 64 + 4 * i, load<u32>(l + 64));
        if (load<u32>(l + 64) == 0) continue;
        store<u32>(p + 80 + 4 * i, load<u32>(l + 60));
        store<u8>(p + 112 + i, static_cast<u8>(load<u32>(l + 48) & 1));
        store<u32>(p + 96 + 4 * i, load<u32>(l + 48) >> 1);  // rlwinm 31,1,31
    }
    ret(ctx, 1u);
}

namespace {

// The draw list run DrawList_Run_00517754 does, inlined: when the list's
// count (+5120) is set, the {key, Item*} entries are sorted
// (SortEntries), a 300 us sys_timer_usleep wait runs while g::draw_busy,
// then each item's +64 renderer gets its virtual slot 3 (+12) called
// with (renderer, item, +68, +72, the 32-byte slot +76 of g::draw_slots;
// r8 / r9 the entry and OPD, and with `r10` r10 = +72 too), the count
// re-read after each; the count is then cleared. Returns whether it ran.
bool run_draw_list14(PPUContext& ctx, u32 list, bool r10) {
    const u32 count = list + 5120;
    if (load<u32>(count) == 0) return false;
    call(ctx, sly4::render::layer::SortEntries, {list, load<u32>(count), 8});
    while (load<u32>(g14::g::draw_busy) != 0) syscall(ctx, g14::kSysTimerUsleep, {g14::kDrawWaitUsec});
    for (u32 i = 0; i < load<u32>(count); ++i) {  // cmplw, the count re-read each time
        const u32 item = load<u32>(list + 8 * i + 4);
        const u32 table = load<u32>(g14::g::draw_slots);
        const u32 rend = load<u32>(item + 64);
        const u32 slot = table + (load<u32>(item + 76) << 5);
        const u32 opd = load<u32>(load<u32>(rend) + 12);
        const u32 a = load<u32>(item + 68), b = load<u32>(item + 72);
        if (r10) call_ptr(ctx, opd, {rend, item, a, b, slot, load<u32>(opd), opd, b});
        else call_ptr(ctx, opd, {rend, item, a, b, slot, load<u32>(opd), opd});
    }
    store<u32>(count, 0);
    return true;
}

}  // namespace

// u32 Render_DrawFrame_0051A918()   [called by f_0048C91C] (name?)
// Clears colour, depth (1.0) and stencil (Draw_Clear(1, 0xFFFFFFFF, 0,
// 255)), draws layer 12 (its virtual Draw), runs the draw list as
// DrawList_Run_00517754 does (sorted, a 300 us wait while
// g::draw_busy, each item's +64 virtual slot 3, the count cleared; all
// skipped while empty), Draw_Layer_00517488(3), Draw_Pass_005159E8 on
// the state-cache object with a pass {0, 0, -, -, identity matrix}, then
// draws layer 13. Returns layer 13's Draw result.
DECOMP(0051A918, Render_DrawFrame_0051A918) {
    namespace rg = sly4::render::g;
    // r5 is left holding the constant's address (Draw_Clear doesn't read it).
    callf(ctx, g10::fn::Draw_Clear, {1, 0xFFFFFFFFu, g14::kClearDepth_51A914, 0, 255}, {lfs(g14::kClearDepth_51A914)});
    const u32 l12 = load<u32>(rg::layers + 52);
    fx::vcall(ctx, l12, 16, {l12});
    run_draw_list14(ctx, g14::g::draw_list, false);
    call(ctx, g14::fn::Draw_Layer_00517488, {3});
    Stack frame(ctx, 80);
    const u32 pass = frame.addr();
    const u32 sc = load<u32>(rg::state_cache);
    store<u32>(pass, 0);
    store<u32>(pass + 4, 0);
    for (u32 k = 0; k < 16; ++k) store<u32>(pass + 16 + 4 * k, load<u32>(g14::g::identity_164C10 + 4 * k));  // lfs / stfs: raw
    ctx.fpr[1] = lfs(g14::g::identity_164C10 + 56);  // f1 / f2 keep the copy's last pair
    ctx.fpr[2] = lfs(g14::g::identity_164C10 + 60);
    call(ctx, g14::fn::Draw_Pass_005159E8, {sc, pass});
    const u32 l13 = load<u32>(rg::layers + 56);
    fx::vcall(ctx, l13, 16, {l13});
}

namespace {

// The front stencil state Stencil_Draw_0054CB58 sets for each pass: ops
// all KEEP, func EQUAL against `ref` / `mask` (render state +160..+184,
// +168 = 0), then sent (op, mask 0, func).
void stencil_equal14(u32 ref, u32 mask) {
    namespace pl = sly4::render::pool;
    pl::st_set(176, pl::kKeep);
    pl::st_set(180, pl::kKeep);
    pl::st_set(184, pl::kKeep);
    pl::st_set(160, g14::kStencilEqual);
    pl::st_set(164, ref);
    pl::st_set(172, mask);
    pl::st_set(168, 0);
    fx::emit({pl::kStencilOp, pl::kKeep, pl::kKeep, pl::kKeep});
    fx::emit({pl::kStencilMask, 0});
    pl::front_func();
}

}  // namespace

// RsxContext* Stencil_Draw_0054CB58(o)   [virtual] (name?)
// With the stencil test on: the first pass (Stencil_Pass_0054C268(o,
// moving, 1.0); `moving` = 0 only when |o+80| and |o+76| are both below
// 1e-5) where the stencil equals 0 (mask 80), and when u8 o+96 bit 5 a
// second (o, 2, 0.5) where it equals 64 (mask 255); then the test off.
// Returns the render context (r3 of the last inline command).
DECOMP(0054CB58, Stencil_Draw_0054CB58) {
    constexpr u32 K = g14::kStencilPassConsts;
    const u32 o = arg<u32>(ctx, 0);
    fx::emit({fx::kStencilTestEnable, 1});
    const bool still = std::fabs(lfs(o + 80)) < lfs(K) && std::fabs(lfs(o + 76)) < lfs(K);
    stencil_equal14(0, 80);
    callf(ctx, g14::fn::Stencil_Pass_0054C268, {o, still ? 0u : 1u}, {lfs(K + 4)});
    if (((load<u8>(o + 96) >> 5) & 1) != 0) {  // rlwinm 27,31,31
        stencil_equal14(64, 255);
        callf(ctx, g14::fn::Stencil_Pass_0054C268, {o, 2}, {lfs(K + 8)});
    }
    fx::emit({fx::kStencilTestEnable, 0});
    ret(ctx, load<u32>(fx::g::state));
}

// bool Iff_HandleTopology_00525E40(IffReader* r)   [an IFF chunk handler]
// (name?)
// A new AglTopology from its ApeTListAllocator (g12::g::topology_list;
// the free list built on first use as get_list13 does: 60 bytes,
// FreeList_Init(list, 112, heap, 0, 0)) through virtual slot 3 (+12),
// Topology_Construct; its chunks (Iff_ReadChunks). Then each of its
// u16 +52 entries {Mesh* +0, Material* +4} at +48 without a material
// gets the mesh's +16 one, its u16 +4 refcount bumped. Last the topology
// is appended to the topology map (g12::g::topology_map: Array {u16 +0
// capacity, +2 count, +4 high-water, +6 flags (bit 0: unsorted), +8
// {key, value}[]}; doubled by Array_Reserve when full) under its +80
// key. Returns 1. Quirks: without the allocator holder the chunks are
// read into 0; a failed list allocation is still called through.
DECOMP(00525E40, Iff_HandleTopology_00525E40) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 h = load<u32>(g12::g::topology_list);
    u32 o = 0;
    if (h != 0) {
        u32 fl = load<u32>(h);
        if (fl == 0) {
            const u32 p = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {60, 0, 16}));
            if (p != 0) {
                const u32 heap = load<u32>(h + 4);
                for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(p + off, 0);
                const u32 bits = load<u16>(p + 54) & ~0x8000u;  // rlwimi 0,16,16
                store<u16>(p + 52, 0);
                store<u16>(p + 54, static_cast<u16>(bits));
                store<u32>(p, g14::g::vt_topology_list);
                store<u32>(p + 56, g14::g::str_topology_list);
                call(ctx, g12::fn::FreeList_Init, {p, g14::kTopologySize, heap, 0, 0});
            }
            fl = p;
            store<u32>(h, fl);
        }
        if (const u32 e = static_cast<u32>(fx::vcall(ctx, fl, 12, {fl})); e != 0) {
            call(ctx, g14::fn::Topology_Construct, {e});
            o = e;
        }
    }
    call(ctx, g12::fn::Iff_ReadChunks, {r, o});
    for (s32 i = 0; i < static_cast<s32>(load<u16>(o + 52)); ++i) {
        const u32 ent = load<u32>(o + 48) + 8 * static_cast<u32>(i);
        if (load<u32>(ent + 4) != 0) continue;
        const u32 mat = load<u32>(load<u32>(ent) + 16);
        if (mat == 0) continue;
        store<u32>(ent + 4, mat);
        store<u16>(mat + 4, static_cast<u16>(load<u16>(mat + 4) + 1));
    }
    const u32 a = load<u32>(g12::g::topology_map);
    const u32 key = load<u32>(o + 80);
    u32 n = load<u16>(a + 2);
    if (n == load<u16>(a)) {
        call(ctx, g14::fn::Array_Reserve, {a, (static_cast<u32>(load<u16>(a)) << 1) & 0xFFFEu});
        n = load<u16>(a + 2);
    }
    const u32 data = load<u32>(a + 8);
    store<u32>(data + (n << 3), key);
    const u32 n2 = load<u16>(a + 2);
    store<u32>(data + (n2 << 3) + 4, o);
    const u32 hw = load<u16>(a + 4), flags = load<u16>(a + 6);
    const u32 cnt = n2 + 1;
    store<u16>(a + 2, static_cast<u16>(cnt));
    store<u16>(a + 4, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(cnt & 0xFFFF) ? hw : cnt));
    store<u16>(a + 6, static_cast<u16>(flags | 1));
    ret(ctx, 1u);
}

// bool Iff_HandleTexture_0051D4B4(IffReader* r, Texture** out)   [an IFF
// chunk handler] (name?)
// Reads the 52-byte texture header on the stack {u32 nameHash, u16
// width, height, u8 depth, bpp, type, char name[32] +11, u32
// packBufOfst +44, packBufSize +48 (from ver 1 on)}, ver (u8, default 1)
// just after it. When the reader's u8 +12 bit 2 is set, *out = the
// texture list's entry for the hash (Map_Find on g11::g::texture_list).
// Otherwise a texture already listed under the hash gets its virtual
// slot 3 (+12) called and 0 is returned; a new one (Texture_Alloc) gets
// the allocator's load callback (g11::g::texture_alloc +8 / +12, phase
// 0), goes to *out and is loaded from the header
// (Texture_LoadFromHeader_0053A94C). Returns 1 otherwise.
DECOMP(0051D4B4, Iff_HandleTexture_0051D4B4) {
    const u32 r = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    Stack frame(ctx, 64);  // the original's sp+112..+175
    const u32 key0 = frame.addr(), key1 = key0 + 4, hdr = key0 + 8, ver = hdr + 52;
    for (u32 k = 0; k < 52; k += 4) store<u32>(hdr + k, 0);
    store<u8>(ver, 1);
    call(ctx, g10::fn::Iff_ReadU8, {r, ver, g14::str_ver});
    call(ctx, g10::fn::Iff_ReadU32, {r, hdr, g14::str_name_hash});
    call(ctx, g11::fn::Iff_ReadU16, {r, hdr + 4, g14::str_width});
    call(ctx, g11::fn::Iff_ReadU16, {r, hdr + 6, g14::str_height});
    call(ctx, g10::fn::Iff_ReadU8, {r, hdr + 8, g14::str_depth});
    call(ctx, g10::fn::Iff_ReadU8, {r, hdr + 9, g14::str_bpp});
    call(ctx, g10::fn::Iff_ReadU8, {r, hdr + 10, g14::str_type});
    call(ctx, g14::fn::FileReader_Read32, {r, hdr + 11, g14::str_name});
    if (static_cast<s32>(load<u8>(ver)) >= 1) {
        call(ctx, g10::fn::Iff_ReadU32, {r, hdr + 44, g14::str_pack_buf_ofst});
        call(ctx, g10::fn::Iff_ReadU32, {r, hdr + 48, g14::str_pack_buf_size});
    }
    if ((load<u8>(r + 12) & g14::kIffShared) != 0) {  // rlwinm 0,29,29
        store<u32>(key1, load<u32>(hdr));
        const u32 v = static_cast<u32>(call(ctx, g14::fn::Map_Find, {load<u32>(g11::g::texture_list), key1}));
        store<u32>(out, load<u32>(v));
        return ret(ctx, 1u);
    }
    store<u32>(key0, load<u32>(hdr));
    const u32 v = static_cast<u32>(call(ctx, g14::fn::Map_Find, {load<u32>(g11::g::texture_list), key0}));
    if (const u32 t = load<u32>(v); t != 0) {
        fx::vcall(ctx, t, 12, {t});
        return ret(ctx, 0u);
    }
    const u32 t = static_cast<u32>(call(ctx, g10::fn::Texture_Alloc, {0}));  // r3 = the 0 just tested
    const u32 h = load<u32>(g11::g::texture_alloc);
    if (const u32 cb = load<u32>(h + 8); cb != 0) call_ptr(ctx, cb, {t, load<u32>(h + 12), 0, cb, load<u32>(cb)});
    store<u32>(out, t);
    call(ctx, g14::fn::Texture_LoadFromHeader_0053A94C, {t, hdr});
    ret(ctx, 1u);
}

// u32 Particle_LightColour_00500D40(-, u8 colour[4], Particle* p, const Lights* l)
// (Emitter_Check_00506050; name?)
// Lights a particle colour: the ambient l+0..+8 plus, for each of the u8
// l+214 lights i, its colour l+16+16i (xyz) - for a point light (u8
// l+208+i == 2) scaled by the attenuation a3 / (a0 + a1 * |pos - p+48|)
// (a = l+144+16i, pos = l+80+16i; at least 0). Each sum is clamped to
// [0, 1] (NaN: 1), scaled by 255 and truncated; colour[0..2] = colour *
// that >> 8, colour[3] likewise with 255 (stored 0, 1, 3, 2). Returns
// colour[0]'s product >> 8 (f1: the last fctiwz word).
DECOMP(00500D40, Particle_LightColour_00500D40) {
    constexpr u32 K = g14::kLightConsts;
    const u32 col = arg<u32>(ctx, 1), p = arg<u32>(ctx, 2), l = arg<u32>(ctx, 3);
    f64 r = lfs(l), g = lfs(l + 4), b = lfs(l + 8);
    const u32 n = load<u8>(l + 214);
    const f64 zero = lfs(K);
    const u32 q = p + 48;
    for (u32 i = 0; i < n; ++i) {
        const u32 li = l + 16 * i;
        f64 c0, c1, c2;
        if (load<u8>(l + 208 + i) == 2) {
            const u32 pos = li + 80, at = li + 144;
            const f64 d1 = sgl(lfs(pos + 4) - lfs(q + 4));
            const f64 d0 = sgl(lfs(pos) - lfs(q));
            const f64 d2 = sgl(lfs(pos + 8) - lfs(q + 8));
            f64 len = sgl(d1 * d1);
            len = g11::madds(d0, d0, len);
            len = g11::madds(d2, d2, len);
            len = sgl(std::sqrt(len));
            f64 att = g11::madds(lfs(at + 4), len, lfs(at));
            att = sgl(lfs(at + 12) / att);
            if (att < zero) att = zero;
            c0 = sgl(lfs(li + 16) * att);
            c1 = sgl(lfs(li + 20) * att);
            c2 = sgl(lfs(li + 24) * att);
        } else {
            c0 = lfs(li + 16);
            c1 = lfs(li + 20);
            c2 = lfs(li + 24);
        }
        b = sgl(b + c2);
        g = sgl(g + c1);
        r = sgl(r + c0);
    }
    const f64 one = lfs(K + 4);
    auto clamp01 = [&](f64 x) {
        const f64 m = x < one ? x : one;
        return m > zero ? m : zero;
    };
    const f64 s = lfs(K + 8);
    const u32 ir = static_cast<u32>(rt::fctiw(sgl(clamp01(r) * s), true)) & 0xFF;
    const u32 ig = static_cast<u32>(rt::fctiw(sgl(clamp01(g) * s), true)) & 0xFF;
    const u32 ib = static_cast<u32>(rt::fctiw(sgl(clamp01(b) * s), true)) & 0xFF;
    const u64 wa = rt::fctiw(sgl(one * s), true);
    const u32 ia = static_cast<u32>(wa) & 0xFF;
    const u32 v0 = (load<u8>(col) * ir) >> 8, v1 = (load<u8>(col + 1) * ig) >> 8;
    const u32 v2 = (load<u8>(col + 2) * ib) >> 8, v3 = (load<u8>(col + 3) * ia) >> 8;
    store<u8>(col, static_cast<u8>(v0));
    store<u8>(col + 1, static_cast<u8>(v1));
    store<u8>(col + 3, static_cast<u8>(v3));
    store<u8>(col + 2, static_cast<u8>(v2));
    ctx.fpr[1] = std::bit_cast<f64>(wa);
    ret(ctx, v0);
}

// bool Draw_Lists_00517488(u32 mask)   [Render_DrawFrame_0051A918 (3), ...]
// (name?)
// Runs the draw list g::draw_list_a when mask bit 0, then
// g::draw_list_b when bit 1 (run_draw_list14). Returns 1 when either
// had entries.
DECOMP(00517488, Draw_Lists_00517488) {
    const u32 mask = arg<u32>(ctx, 0);
    bool ran = false;
    if ((mask & 1) != 0) ran = run_draw_list14(ctx, g14::g::draw_list_a, false);
    if ((mask & 2) != 0) ran = run_draw_list14(ctx, g14::g::draw_list_b, true) || ran;
    ret(ctx, ran ? 1u : 0u);
}

// bool Iff_HandleEmitDesc_005286BC(IffReader* r, AglEmitProperties* o)
// [an IFF chunk handler] (name?)
// Reads the 68-byte emitter description on the stack (cleared first;
// "version" just after it, default 3): f32 ttl, ttlVar, emitRate, u32
// emitObjHash, u8 emitObjType, renderFlags, processClass, alignment, f32
// ptlNearZCull, ptlFarZCull, emitFarZCull, emitFarZFadeDist; from
// version 2 f32 emitRateVar, burstEmitDelay(Var), burstEmitDuration(Var);
// from version 3 u32 emitMeshHash[3]. Hands it to
// EmitProps_SetDesc_004FF4C0(o, desc). Returns 1.
DECOMP(005286BC, Iff_HandleEmitDesc_005286BC) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Stack frame(ctx, 80);
    const u32 d = frame.addr(), ver = d + 68;
    call(ctx, g12::fn::memset, {d, 0, 68});
    store<u8>(ver, 3);
    call(ctx, g10::fn::Iff_ReadU8, {r, ver, 0x00B23A30u /* "version" */});
    enum Kind { F32, U32, U8 };
    struct Field { u32 off, name; Kind kind; };
    auto read = [&](const Field& f) {
        const u32 fn = f.kind == F32 ? g10::fn::FileReader_ReadF32Swapped : f.kind == U32 ? g10::fn::Iff_ReadU32 : g10::fn::Iff_ReadU8;
        call(ctx, fn, {r, d + f.off, f.name});
    };
    static constexpr Field kBase[] = {
        {0, 0x00B23A38, F32},  {4, 0x00B23A3C, F32},  {8, 0x00B23A43, F32},  {12, 0x00B23A4C, U32},
        {16, 0x00B23A58, U8},  {17, 0x00B23A64, U8},  {18, 0x00B23A70, U8},  {19, 0x00B23A7D, U8},
        {20, 0x00B23A87, F32}, {24, 0x00B23A94, F32}, {28, 0x00B23AA0, F32}, {32, 0x00B23AAD, F32},
    };
    static constexpr Field kV2[] = {
        {36, 0x00B23ABE, F32}, {40, 0x00B23ACA, F32}, {44, 0x00B23AD9, F32}, {48, 0x00B23AEB, F32}, {52, 0x00B23AFD, F32},
    };
    static constexpr Field kV3[] = {{56, 0x00B23B12, U32}, {60, 0x00B23B22, U32}, {64, 0x00B23B32, U32}};
    for (const Field& f : kBase) read(f);
    if (static_cast<s32>(load<u8>(ver)) >= 2)
        for (const Field& f : kV2) read(f);
    if (static_cast<s32>(load<u8>(ver)) >= 3)
        for (const Field& f : kV3) read(f);
    call(ctx, g14::fn::EmitProps_SetDesc_004FF4C0, {o, d});
    ret(ctx, 1u);
}

namespace {

// A one-word inline command whose cursor is written back before the
// value (as some sequences store it).
void emit_cursor_first14(u32 cmd, u32 v) {
    const u32 c = load<u32>(fx::g::state);
    const u32 cur = load<u32>(c + 8);
    store<u32>(cur, cmd);
    store<u32>(c + 8, cur + 8);
    store<u32>(cur + 4, v);
}

}  // namespace

// RsxContext* DepthState_Apply_005495D8(const DepthState* o)   [virtual]
// (name?)
// The depth bias {f32 o+64 factor, o+68 units}: polygon offset off when
// both are 0, without depth test (render state u8 +93) or depth target
// (+84), or for a depth format (target +24) other than 4 / 6 (units x
// 65536) and 5 / 7 (x 2^24; any other also sets
// shader2::g::depth_format_warned); else on with {factor (raw), the
// scaled units}. Then depth mask = (+93 & 1) != 0, depth test = +93,
// depth func LEQUAL, blend off, cull on, front face CCW, colour mask 0.
// Returns the render context.
DECOMP(005495D8, DepthState_Apply_005495D8) {
    namespace s2 = sly4::render::shader2;
    namespace l4 = sly4::render::layers4;
    constexpr u32 K = g14::kDepthStateConsts, st = fx::g::state;
    const u32 o = arg<u32>(ctx, 0);
    const f64 zero = lfs(K);
    const u32 factor = load<u32>(o + 64);
    f64 units = lfs(o + 68);
    bool off = lfs(o + 64) == zero && units == zero;
    const u32 target = load<u32>(st + 84);
    off = off || load<u8>(st + 93) == 0 || target == 0;
    ctx.fpr[1] = zero;
    if (!off) {
        const s32 format = static_cast<s32>(load<u32>(target + 24));
        if (format == 4 || format == 6) {
            ctx.fpr[1] = lfs(K + 4);
            units = sgl(units * ctx.fpr[1]);
        } else if (format == 5 || format == 7) {
            ctx.fpr[1] = lfs(K + 8);
            units = sgl(units * ctx.fpr[1]);
        } else {
            if (load<u8>(s2::g::depth_format_warned) == 0) store<u8>(s2::g::depth_format_warned, 1);
            off = true;
        }
    }
    if (off) {
        fx::emit({s2::kPolyOffsetFill, 0});
    } else {
        fx::emit({s2::kPolyOffsetFill, 1});
        fx::emit({s2::kPolyOffset2, factor, g11::stfs_bits(units)});
    }
    const u32 b = load<u8>(st + 93);
    emit_cursor_first14(fx::kDepthMask, (b & 1) != 0 ? 1u : 0u);
    fx::emit({fx::kDepthTestEnable, b});
    fx::emit({fx::kDepthFunc, g14::kLequal});
    fx::emit({fx::kBlendEnable, 0});
    fx::emit({fx::kCullFaceEnable, 1});
    emit_cursor_first14(g14::kFrontFace, g14::kCcw);
    fx::emit({l4::kColorMask, 0});
    ctx.fpr[2] = units;
    ret(ctx, load<u32>(st));
}

// u32 Surface_Copy_00537118(const Surface* a, const Surface* b)
// (f_005105E0; name?)
// Copies surface a into b on the RSX ({+4 bit 0 location, u16 +24
// width, +26 height, +32 pitch, +36 offset, +44 format index into
// tgt::g::format_table}): the transfer mode from the two locations (0
// both local, 1 a main, 2 b main, 3 both main). Same sizes:
// cellGcmSetTransferImage(b's offset / pitch, a's, a's size, b's bits
// per pixel / 8). Otherwise a scaled blit: cellGcmSetTransferScaleMode
// (mode, 0) and cellGcmSetTransferScaleSurface with the scale {1, 3, 3,
// clip / out 0, 0, b's size, ratios a / b x 2^20 (fctiwz), in a's size,
// pitch, origin 2, interp 1, offset, in 8, 8} and the surface {10, b's
// pitch, b's offset}. Returns the last call's r3.
DECOMP(00537118, Surface_Copy_00537118) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 aw = load<u16>(a + 24), ah = load<u16>(a + 26), bw = load<u16>(b + 24), bh = load<u16>(b + 26);
    const u32 apitch = load<u32>(a + 32), aoff = load<u32>(a + 36), bpitch = load<u32>(b + 32), boff = load<u32>(b + 36);
    const s32 bits = static_cast<s32>(load<u32>(tgt::g::format_table + 24 * load<u32>(b + 44) + 8));
    const u32 bytes = static_cast<u32>(bits / 8);  // srawi / addze
    const u32 la = load<u32>(a + 4) & 1, lb = load<u32>(b + 4) & 1;
    const u32 mode = (la | lb) == 0 ? 0u : (la == 0 && lb == 1) ? 2u : (la == 1 && lb == 0) ? 1u : 3u;
    const u32 context = load<u32>(fx::g::state);
    if (aw == bw && ah == bh) {
        call(ctx, sly4::render::layers4::fn::cellGcmSetTransferImage,
             {context, mode, boff, bpitch, 0, 0, aoff, apitch, 0, 0, aw, ah, bytes & 0xFF});
        return;
    }
    Stack frame(ctx, 64);  // the original's sp+160..+223
    const u32 surf = frame.addr(), sc = surf + 12;
    store<u16>(sc + 12, 0);
    store<u32>(sc, 1);
    store<u32>(sc + 4, 3);
    store<u32>(sc + 8, 3);
    store<u16>(sc + 14, 0);
    store<u16>(sc + 16, static_cast<u16>(bw));
    store<u16>(sc + 18, static_cast<u16>(bh));
    store<u16>(sc + 20, 0);
    store<u16>(sc + 22, 0);
    store<u16>(sc + 24, static_cast<u16>(bw));
    store<u16>(sc + 26, static_cast<u16>(bh));
    store<u16>(sc + 48, 8);
    store<u16>(sc + 50, 8);
    store<u16>(sc + 36, static_cast<u16>(aw));
    store<u16>(sc + 38, static_cast<u16>(ah));
    store<u16>(sc + 40, static_cast<u16>(apitch));
    store<u8>(sc + 43, 1);
    store<u32>(sc + 44, aoff);
    store<u16>(surf + 4, static_cast<u16>(bpitch));
    store<u8>(sc + 42, 2);
    store<u32>(surf, g14::kTransferFormatA8R8G8B8);
    store<u32>(surf + 8, boff);
    const f64 one = lfs(g14::kRatioOne);
    const u64 rx = rt::fctiw(sgl(sgl(sgl(static_cast<f64>(aw)) / sgl(static_cast<f64>(bw))) * one), true);
    store<u32>(sc + 28, static_cast<u32>(rx));  // stfiwx
    const u64 ry = rt::fctiw(sgl(sgl(sgl(static_cast<f64>(ah)) / sgl(static_cast<f64>(bh))) * one), true);
    store<u32>(sc + 32, static_cast<u32>(ry));
    ctx.fpr[1] = std::bit_cast<f64>(ry);  // f1 keeps the last fctiwz word
    call(ctx, g12::fn::cellGcmSetTransferScaleMode, {context, mode, 0});
    call(ctx, g12::fn::cellGcmSetTransferScaleSurface, {load<u32>(fx::g::state), sc, surf});
}

// u32 Display_Readback_0053BC14()   [Gcm_FlipContext_0053BE94, f_00457BFC]
// (name?)
// Once per frame (g::readback_busy) while g::readback_on: the display's
// current colour target T (the eye d+52 >> 30: d+16.. without per-eye
// buffers (+52 bit 25), else d+84 / d+64.. by d+148 bit 7 unless config
// +34 and +35 are both set, then d+100.. when the screen's +184 == 1,
// else d+108..) is copied (Gcm_Transfer_005E104C mode 2) into the
// buffer that is not being read (g::readback_offsets[index ^ 1], pitch
// g::readback_pitch; T's offset +4 >> 1, pitch +32, size u16 +36 /
// +38, u8 +44 / 8 bytes per pixel), the buffer is marked pending and the
// label 251 written (g::readback_label + 1). When the buffer being read
// is pending, it is copied back to T (mode 1, g::readback_out_w / _h /
// _bpp). Returns that index (else the last byte tested or call r3).
DECOMP(0053BC14, Display_Readback_0053BC14) {
    namespace g = g14::g;
    constexpr u32 kCfg = sly4::g::config;
    if (load<u8>(g::readback_on) == 0) return ret(ctx, 0u);
    if (const u8 b = load<u8>(g::readback_busy); b != 0) return ret(ctx, b);
    store<u8>(g::readback_busy, 1);
    const u32 d = load<u32>(sly4::draw::g::display);
    const u32 flags = load<u32>(d + 52);
    const u32 eye = (flags >> 30) << 2;  // rlwinm 4,28,29
    u32 slot;
    if (((flags >> 25) & 1) == 0) slot = d + eye + 16;
    else if (load<u8>(kCfg + 35) == 0 || load<u8>(kCfg + 34) == 0)
        slot = ((load<u8>(d + 148) >> 7) & 1) != 0 ? d + 84 : d + eye + 64;  // rlwinm 25,31,31
    else
        slot = load<u32>(g12::g::screen + 184) == 1 ? d + eye + 100 : d + eye + 108;
    const u32 t = load<u32>(slot);
    const u32 w = load<u32>(g::readback_index) ^ 1;
    const u32 bytes = static_cast<u32>(static_cast<s32>(load<u8>(t + 44)) / 8);  // srawi / addze
    call(ctx, g14::fn::Gcm_Transfer_005E104C,
         {load<u32>(fx::g::state), g14::kTransferLocalToMain, load<u32>(g::readback_offsets + 4 * w), load<u32>(g::readback_pitch), 0, 0,
          load<u32>(t + 4) >> 1, load<u32>(t + 32), 0, 0, load<u16>(t + 36), load<u16>(t + 38), bytes});
    const u32 w2 = load<u32>(g::readback_index) ^ 1;
    const u32 context = load<u32>(fx::g::state);
    const u32 label = load<u32>(g::readback_label) + 1;
    store<u8>(g::readback_pending + w2, 1);
    call(ctx, g14::fn::Gcm_WriteLabel_005E0688, {context, g14::kReadbackLabel, label});
    const u32 r = load<u32>(g::readback_index);
    if (load<u8>(g::readback_pending + r) == 0) return ret(ctx, r);
    call(ctx, g14::fn::Gcm_Transfer_005E104C,
         {load<u32>(fx::g::state), g14::kTransferMainToLocal, load<u32>(t + 4) >> 1, load<u32>(t + 32), 0, 0,
          load<u32>(g::readback_offsets + 4 * r), load<u32>(g::readback_pitch), 0, 0, load<u32>(g::readback_out_w),
          load<u32>(g::readback_out_h), load<u32>(g::readback_out_bpp)});
}

namespace {

// transpose(item rows * view_proj) as vertex constants 0..3 (one 17-word
// SET_TRANSFORM_CONSTANT_LOAD), as draw3.cpp's upload_item_matrix: rows
// read with lvx (address rounded down to 16), fused per lane in vmaddfp
// order, row 3 adding view_proj's row 3; the cursor is advanced first.
void upload_item_matrix14(u32 item) {
    namespace l4 = sly4::render::layers4;
    const u32 m = l4::g::view_proj;
    f32 r[4][4];
    for (u32 i = 0; i < 4; ++i) {
        const u32 row = (item & ~15u) + 16 * i;
        const f32 x = load<f32>(row), y = load<f32>(row + 4), z = load<f32>(row + 8);
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fmaf(load<f32>(m + 4 * k), x, i == 3 ? load<f32>(m + 48 + 4 * k) : 0.0f);
            v = std::fmaf(load<f32>(m + 16 + 4 * k), y, v);
            r[i][k] = std::fmaf(load<f32>(m + 32 + 4 * k), z, v);
        }
    }
    const u32 c = load<u32>(fx::g::state);
    const u32 cur = load<u32>(c + 8);
    store<u32>(c + 8, cur + 72);
    store<u32>(cur, l4::kVertexConst17);
    store<u32>(cur + 4, 0);  // first constant
    for (u32 k = 0; k < 4; ++k)
        for (u32 i = 0; i < 4; ++i) store<f32>(cur + 8 + 16 * k + 4 * i, r[i][k]);  // stvlx / stvrx pairs
}

}  // namespace

// u32 DrawItem_DrawLit_0054D23C(-, DrawItem* item)   [virtual] (name?)
// Uploads the item matrix (upload_item_matrix14), puts the camera
// position in the item's frame (Listener_LocalDirection(out,
// render::g::camera_pos, item)) into fragment parameter 0 as {x, y, z,
// 1} (fx::set_fconst), then draws the item's mesh (+12) through its
// virtual slot 4 (+16): (mesh, item). Returns its r3.
DECOMP(0054D23C, DrawItem_DrawLit_0054D23C) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_matrix14(item);
    Stack frame(ctx, 16);
    const u32 v = frame.addr();
    call(ctx, sly4::render::shader4::fn::Listener_LocalDirection, {v, sly4::render::g::camera_pos, item});
    const u32 mesh = load<u32>(item + 12);
    ctx.fpr[3] = lfs(v);  // the lfs f3 / f2 / f1 the original leaves
    ctx.fpr[2] = lfs(v + 4);
    ctx.fpr[1] = lfs(v + 8);
    fx::set_fconst(0, load<u32>(v), load<u32>(v + 4), load<u32>(v + 8), 0x3F800000u);
    fx::vcall(ctx, mesh, 16, {mesh, item});
}

// bool Iff_HandleDeflector_0052A7C0(IffReader* r, AglEmitProperties* o)
// [an IFF chunk handler] (name?)
// Warns (String_Format) unless the properties emit in world space (u8
// o+42 bit 7). A new 112-byte deflector (SmallAlloc; vtable
// vt_deflector, refs 1, no planes, bounce 1.0) is referenced from o+132
// (the new one's refs up, the old one's down: at zero its virtual slot 1
// (+4)), its creation reference dropped. Then the chunk: a version byte,
// "bounce" and six "planes[i]" vec4s; each plane whose w isn't FLT_MAX
// is appended, and the bounce stored. Returns 1.
DECOMP(0052A7C0, Iff_HandleDeflector_0052A7C0) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    if (((load<u8>(o + 42) >> 7) & 1) == 0)  // rlwinm 25,31,31
        call(ctx, g14::fn::String_Format, {g14::str_deflector_ignored, g14::str_empty_b232c8});
    u32 p = static_cast<u32>(call(ctx, g12::fn::SmallAlloc, {112}));
    if (p != 0) {
        store<u16>(p + 4, 1);
        store<u32>(p + 8, 0);
        store<u32>(p, g14::vt_deflector);
        store<u32>(p + 12, 0x3F800000u);
    }
    if (const u32 old = load<u32>(o + 132); old != p) {
        store<u32>(o + 132, p);
        if (p != 0) store<u16>(p + 4, static_cast<u16>(load<u16>(p + 4) + 1));
        if (old != 0) {
            const u16 n = static_cast<u16>(load<u16>(old + 4) - 1);
            store<u16>(old + 4, n);
            if (static_cast<s16>(n) <= 0) fx::vcall(ctx, old, 4, {old});
        }
    }
    {
        const u32 q = load<u32>(o + 132);
        store<u16>(q + 4, static_cast<u16>(load<u16>(q + 4) - 1));
    }
    Stack frame(ctx, 112);  // the original's sp+112..+212
    const u32 sp = frame.addr();
    call(ctx, g12::fn::memset, {sp, 0, 100});
    call(ctx, g10::fn::Iff_ReadU8, {r, sp + 100, 0});
    call(ctx, g10::fn::FileReader_ReadF32Swapped, {r, sp, g14::str_bounce});
    for (u32 i = 0; i < 6; ++i) call(ctx, g14::fn::FileReader_ReadVec4Swapped, {r, sp + 4 + 16 * i, g14::str_planes0 + 10 * i});
    const f64 none = lfs(g14::kNoPlane);
    for (u32 i = 0; i < 6; ++i) {
        const u32 v = sp + 4 + 16 * i;
        if (lfs(v + 12) == none) continue;
        const u32 d = load<u32>(o + 132);
        const u32 dst = d + 16 + (load<u32>(d + 8) << 4);
        for (u32 k = 0; k < 16; k += 4) store<u32>(dst + k, load<u32>(v + k));  // lfs / stfs: raw
        store<u32>(d + 8, load<u32>(d + 8) + 1);
    }
    store<u32>(load<u32>(o + 132) + 12, load<u32>(sp));
    ctx.fpr[1] = lfs(sp);  // f1 / f2 as the original leaves them
    ctx.fpr[2] = none;
    ret(ctx, 1u);
}

namespace {

// The body SelectShader_005D47A0 and SelectShader_005D62E8 share; each
// has its own default texture static (ready flag, texture), constants
// {0, 1e-5} and params offsets (the two radius pads, then a and b).
struct SelectVariant14 {
    u32 ready, tex, consts, pad_a, pad_b, a, b;
};
void select_shader14(PPUContext& ctx, const SelectVariant14& v) {
    namespace g = g14::g;
    const u32 K = v.consts;
    const u32 m = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    const u32 kind = load<u8>(m + 44);
    u32 base = 0;
    if (((load<u8>(load<u32>(item + 28) + 100) >> 7) & 1) != 0)  // rlwinm 25,31,31
        if (const u32 e = load<u32>(item + 44); e != 0) base = load<u32>(e) + 32;
    if (load<u8>(v.ready) == 0) {
        const u32 t = static_cast<u32>(call(ctx, g11::fn::Tex_GetDefault, {5}));
        store<u32>(v.tex, t);
        store<u8>(v.ready, 1);
    }
    const u32 textured = load<u32>(m + 28) != load<u32>(v.tex) ? 4u : 0u;
    const u32 p = load<u32>(m + 124);
    const f64 zero = lfs(K);
    const f64 pad_a = p != 0 ? lfs(p + v.pad_a) : zero, pad_b = p != 0 ? lfs(p + v.pad_b) : zero;
    const u32 sph = load<u32>(item + 12) + 92;
    const f64 cx = lfs(sph), cy = lfs(sph + 4), cz = lfs(sph + 8), rad = lfs(sph + 12);
    Stack frame(ctx, 32);  // the original's sp+112 (eye), +128 (direction)
    const u32 eye = frame.addr(), dir = eye + 16;
    call(ctx, sly4::render::shader4::fn::Listener_LocalDirection, {eye, sly4::render::g::camera_pos, item});
    call(ctx, sly4::render::shader4::fn::Vec3_RotateTo, {dir, g::view_dir, item});
    const f64 dy = sgl(cy - lfs(eye + 4)), dx = sgl(cx - lfs(eye)), dz = sgl(cz - lfs(eye + 8));
    const f64 reach = sgl(sgl(rad + pad_a) + pad_b);
    const u32 table = load<u32>(load<u32>(m + 16) + 12);
    f64 d = sgl(dy * lfs(dir + 4));
    d = g11::madds(dx, lfs(dir), d);
    d = sgl(-std::fma(dz, lfs(dir + 8), d));  // fnmadds
    const bool beyond = d > reach;
    const f64 a = p != 0 ? lfs(p + v.a) : zero;
    const f64 b = p != 0 ? lfs(p + v.b) : zero;
    bool lit = false;
    ctx.fpr[1] = a;
    if (!beyond) {
        const f64 eps = lfs(K + 4);
        ctx.fpr[1] = eps;
        lit = !(std::fabs(a) < eps && std::fabs(b) < eps);
    }
    u32 index = base | textured | (lit ? 16u : 0u);
    if (kind >= 5 && !beyond) index |= 8;
    store<u8>(item + 69, static_cast<u8>(index));
    ret(ctx, load<u32>(table + ((index << 2) & 0x3FCu)));
}

}  // namespace

// Shader* SelectShader_005D47A0(ShaderType*, Material* m, DrawItem* item)
// [a shader type's variant select] (name?)
// The variant index: [[item+44]] + 32 when the item's +28 material has
// u8 +100 bit 7 and item+44 is set; | 4 when m's texture +28 isn't the
// default one (Tex_GetDefault(5), fetched once); with `beyond` = the
// mesh's bounding sphere (mesh +92: centre, radius; params +124 / +128
// added to the radius) lies entirely on the far side of the camera
// plane (render::g::camera_pos / g::view_dir in the item's frame:
// -(dot(centre - eye, dir)) > radius): | 8 for u8 m+44 >= 5 unless
// beyond; | 16 unless beyond or both params +100 and +112 are below
// 1e-5 in magnitude (params m+124; none: all 0). Stored in u8 item+69; returns the shader table's entry
// ((m+16)->+12)[index & 0xFF].
DECOMP(005D47A0, SelectShader_005D47A0) {
    select_shader14(ctx, {g14::g::default_tex_ready, g14::g::default_tex, g14::kSelectConsts_5D4798, 124, 128, 100, 112});
}

// Shader* SelectShader_005D62E8(ShaderType*, Material* m, DrawItem* item)
// [a shader type's variant select] (name?)
// As SelectShader_005D47A0 (select_shader14) with its own default
// texture static and constants, the radius pads at params +132 / +136
// and the two tested params +108 / +120.
DECOMP(005D62E8, SelectShader_005D62E8) {
    select_shader14(ctx, {g14::g::default_tex_ready2, g14::g::default_tex2, g14::kSelectConsts_5D62DC, 132, 136, 108, 120});
}

namespace {

// A FogRange slot's state field (+32): bits 20..27 the state (3: unset),
// bits 28..31 a flag.
u32 with_state14(u32 w, u32 st) { return (w & ~0x0FF00000u) | ((st << 20) & 0x0FF00000u); }  // rlwimi 20,4,11

}  // namespace

// u32 FogRanges_Set_0050B6D8(FogRanges* o, const FogRange* r)
// (f_0051CD48; name?)
// Copies the range r (f32 +0.., u8 +32 state, u8 +33 slot) into slot
// u8 r+33 of o (0: +8, 6 floats; 1: +56, 6 floats; 2: +104, 8 floats;
// other: none): when r+0 is 0 the slot is just marked unset (state 3);
// otherwise the floats are copied (the 6-float slots also get +24 / +28
// = 0) with the state r+32, and for state 1 with a zero r+4 the flag
// nibble = 1. Then when neither slot 0 nor 1 is unset, both have +4 > 0
// and slot 0's +0 > slot 1's +0, u8 o+4 = 1 and slot 0 is marked unset.
// Returns o (r3), or the new slot 0 word on that last path.
DECOMP(0050B6D8, FogRanges_Set_0050B6D8) {
    const u32 o = arg<u32>(ctx, 0), r = arg<u32>(ctx, 1);
    const u32 slot = load<u8>(r + 33);
    const f64 zero = lfs(g14::kZero_50B6D0);
    if (slot <= 2) {
        const u32 s = o + (slot == 0 ? 8u : slot == 1 ? 56u : 104u);
        const bool eight = slot == 2;
        ctx.fpr[1] = zero;
        const u32 st = load<u8>(r + 32);
        if (lfs(r) == zero) {
            store<u32>(s + 32, with_state14(load<u32>(s + 32), 3));
        } else {
            u32 v[8];
            for (u32 k = 0; k < (eight ? 8u : 6u); ++k) v[k] = load<u32>(r + 4 * k);  // lfs / stfs: raw
            const u32 w = with_state14(load<u32>(s + 32), st);
            store<u32>(s, v[0]);
            store<u32>(s + 4, v[1]);
            store<u32>(s + 8, v[2]);
            store<u32>(s + 24, eight ? v[6] : 0u);
            store<u32>(s + 28, eight ? v[7] : 0u);
            store<u32>(s + 12, v[3]);
            store<u32>(s + 16, v[4]);
            store<u32>(s + 20, v[5]);
            store<u32>(s + 32, w);
            if (st == 1 && lfs(r + 4) == zero) store<u32>(s + 32, (w & 0x0FFFFFFFu) | 0x10000000u);  // rlwimi 28,0,3
        }
    }
    const u32 a = o + 8, b = o + 56;
    if (((load<u32>(a + 32) >> 20) & 0xFF) == 3) return;  // rlwinm 12,24,31
    if (((load<u32>(b + 32) >> 20) & 0xFF) == 3) return;
    ctx.fpr[1] = zero;
    if (!(lfs(a + 4) > zero) || !(lfs(b + 4) > zero)) return;
    ctx.fpr[1] = lfs(a);
    if (!(lfs(a) > lfs(b))) return;
    store<u8>(o + 4, 1);
    const u32 w = with_state14(load<u32>(a + 32), 3);
    store<u32>(a + 32, w);
    ret(ctx, w);
}

// u32 SpursQueue_Push_007A61F4(JobQueue* q, const Job* job)   [virtual]
// (name?)
// While q accepts jobs (u8 +11992), takes a free slot in the 16-bit mask
// q+11264 (searching from slot 0, wrapping; the bit set with an atomic
// compare-and-swap, the search position kept across retries; quirk: a
// full mask (0xFFFF) skips the search and swaps in the previous attempt's
// value, 0 at first), copies the 40-byte job into the slot (q+11268 + 44
// slot), marks it busy (+40 = 1), clears the slot's bit in the SPURS
// event flag q+10624, then calls q's virtual slot 10 (+40) with (q, job
// +12, +16, ...): when job+28 is set and job+20 is 0, (+28, +32, +36,
// +24 - (+32 + +36)); else (+20, 0, +24, 0); then &busy, the event flag
// and the slot's bit on the stack. Returns the slot (0 when not
// accepting).
DECOMP(007A61F4, SpursQueue_Push_007A61F4) {
    const u32 q = arg<u32>(ctx, 0), job = arg<u32>(ctx, 1);
    if (load<u8>(q + g14::kJobQueueOn) == 0) return ret(ctx, 0u);
    const u32 mask = q + g14::kJobQueueMask;
    u32 slot = 0, want = 0;
    for (;;) {
        std::atomic_thread_fence(std::memory_order_seq_cst);  // lwsync
        const u32 old = mem::r32(mask);
        if (old != 0xFFFFu) {
            while ((old & (1u << slot)) != 0) slot = (slot + 1) & 15;
            want = old | (1u << slot);
        }
        if (mem::cas32(mask, old, want)) break;  // lwarx / stwcx.
    }
    const u32 e = q + g14::kJobQueueSlots + g14::kJobQueueSlotSize * slot, busy = e + 40;
    for (u32 k = 0; k < 40; k += 4) store<u32>(e + k, load<u32>(job + k));
    store<u32>(busy, 1);
    const u32 bit = (1u << slot) & 0xFFFFu, flag = q + g14::kJobQueueFlag;
    call(ctx, g14::fn::cellSpursEventFlagClear, {flag, bit});
    const u32 opd = load<u32>(load<u32>(q) + 40);
    const u32 a = load<u32>(job + 28), c = load<u32>(job + 20);
    if (a != 0 && c == 0) {
        const u32 x = load<u32>(job + 32), y = load<u32>(job + 36);
        call_ptr(ctx, opd, {q, load<u32>(job + 12), load<u32>(job + 16), a, x, y, load<u32>(job + 24) - (x + y), busy, flag, bit});
    } else {
        call_ptr(ctx, opd, {q, load<u32>(job + 12), load<u32>(job + 16), c, 0, load<u32>(job + 24), 0, busy, flag, bit});
    }
    ret(ctx, e);
}

// bool Iff_HandleTexAnim_0052A528(IffReader* r, Material* o)   [an IFF
// chunk handler] (name?)
// Reads a version byte, u8 "mode", f32 "numFrameX", "speed" and
// "variance" (unused). Mode 0: nothing. Otherwise a new 48-byte texture
// animation (Mem_Alloc; vtable vt_tex_anim, refs 1, +8 = 1.0) is
// referenced from o+128 (the new one's refs up, the old one's down: at
// zero its virtual slot 1 (+4)), its creation reference dropped, and
// filled: mode, speed, frames, inv = 1 / frames; per mode +24 = inv (1,
// 2, 3, 4), +28 = speed / frames (2) or speed / ((frames - 1) * 2) (3,
// with +32 that divisor). Then o's texture scale (f32 +92 / +96) has +92
// multiplied by inv and both divided by the larger of the two. Returns 1.
DECOMP(0052A528, Iff_HandleTexAnim_0052A528) {
    constexpr u32 K = g14::kTexAnimConsts;
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Stack frame(ctx, 32);  // the original's sp+112..+127 (cleared) and +128 (version)
    const u32 sp = frame.addr();
    for (u32 k = 0; k < 16; k += 4) store<u32>(sp + k, 0);  // stvx of zero
    call(ctx, g10::fn::Iff_ReadU8, {r, sp + 16, 0});
    call(ctx, g10::fn::Iff_ReadU8, {r, sp, g14::str_mode});
    call(ctx, g10::fn::FileReader_ReadF32Swapped, {r, sp + 4, g14::str_num_frame_x});
    call(ctx, g10::fn::FileReader_ReadF32Swapped, {r, sp + 8, g14::str_speed});
    call(ctx, g10::fn::FileReader_ReadF32Swapped, {r, sp + 12, g14::str_variance2});
    if (load<u8>(sp) == 0) return ret(ctx, 1u);
    const u32 p = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {48, 0, 16}));
    if (p != 0) {
        store<u32>(p + 16, 0);
        store<u16>(p + 4, 1);
        store<u32>(p, g14::vt_tex_anim);
        store<u32>(p + 8, 0x3F800000u);
        store<u32>(p + 12, 0);
        store<u8>(p + 20, 0);
        store<u32>(p + 24, 0);
        store<u32>(p + 28, 0);
        store<u32>(p + 32, 0);
    }
    if (const u32 old = load<u32>(o + 128); old != p) {
        store<u32>(o + 128, p);
        if (p != 0) store<u16>(p + 4, static_cast<u16>(load<u16>(p + 4) + 1));
        if (old != 0) {
            const u16 n = static_cast<u16>(load<u16>(old + 4) - 1);
            store<u16>(old + 4, n);
            if (static_cast<s16>(n) <= 0) fx::vcall(ctx, old, 4, {old});
        }
    }
    {
        const u32 q = load<u32>(o + 128);
        store<u16>(q + 4, static_cast<u16>(load<u16>(q + 4) - 1));
    }
    const u32 a = load<u32>(o + 128);
    const u32 mode = load<u8>(sp);
    const f64 speed = lfs(sp + 8), frames = lfs(sp + 4), one = lfs(K);
    store<u8>(a + 20, static_cast<u8>(mode));
    store<u32>(a + 8, load<u32>(sp + 8));  // lfs / stfs: raw
    store<u32>(a + 16, load<u32>(sp + 4));
    const f64 inv = sgl(one / frames);
    store<u32>(a + 12, g11::stfs_bits(inv));
    if (mode == 1 || mode == 4) {
        store<u32>(a + 24, g11::stfs_bits(inv));
    } else if (mode == 2) {
        const f64 rate = sgl(speed / frames);
        store<u32>(a + 24, g11::stfs_bits(inv));
        store<u32>(a + 28, g11::stfs_bits(rate));
    } else if (mode == 3) {
        const f64 span = sgl(sgl(frames - one) * lfs(K + 4));
        const f64 rate = sgl(speed / span);
        store<u32>(a + 24, g11::stfs_bits(inv));
        store<u32>(a + 32, g11::stfs_bits(span));
        store<u32>(a + 28, g11::stfs_bits(rate));
    }
    const f64 sx = sgl(lfs(o + 92) * inv);
    store<u32>(o + 92, g11::stfs_bits(sx));
    const f64 sy = lfs(o + 96), x = lfs(o + 92);
    const f64 big = sx > sy ? sx : sy;
    const f64 s = sgl(one / big);
    const f64 nx = sgl(x * s), ny = sgl(sy * s);
    store<u32>(o + 92, g11::stfs_bits(nx));
    store<u32>(o + 96, g11::stfs_bits(ny));
    ctx.fpr[1] = ny;
    ret(ctx, 1u);
}

// u32 Texture_SystemInit_0051DF3C()   (name?)
// The texture capacity n = config u16 +350 (signed; 0: 1024, negative:
// 0). The texture allocator state (16 bytes {0, n, 0}; g11::g::
// texture_alloc) and the texture list (an Array {u16 n, 0, 0, 16, the
// default allocator's block of n x 8 bytes when n, +16 0, +20 the u32
// compare}; g11::g::texture_list) are made (each 0 when its Mem_Alloc
// fails); the texture chunks are registered (FRXT; RXET, HRXT, CNIT with
// 16; NTXD, SDDT, AGTT, PMBT to the shared data handler), then
// Texture_InitDefaults_0051DA1C and EnsureTextures_00542804. Returns the
// latter's r3.
DECOMP(0051DF3C, Texture_SystemInit_0051DF3C) {
    const u32 c = load<u16>(sly4::g::config + 350);
    u32 n = g14::kTexturesDefault;
    if (c != 0) n = static_cast<s16>(c) > 0 ? static_cast<u32>(static_cast<s16>(c)) : 0u;
    u32 st = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {16, 0, 16}));
    if (st != 0) {
        store<u32>(st, 0);
        store<u32>(st + 8, 0);
        store<u32>(st + 4, n);
    }
    store<u32>(g11::g::texture_alloc, st);
    u32 list = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {24, 0, 16}));
    if (list != 0) {
        store<u16>(list, static_cast<u16>(n));
        store<u16>(list + 2, 0);
        store<u16>(list + 4, 0);
        store<u16>(list + 6, 16);
        store<u32>(list + 8, 0);
        const u32 alloc = static_cast<u32>(call(ctx, g12::fn::Mem_DefaultAllocator, {}));
        store<u32>(list + 12, alloc);
        if (const u32 cap = load<u16>(list); cap != 0) {
            const u32 opd = load<u32>(load<u32>(alloc));
            const u32 size = cap * ((static_cast<u32>(load<u16>(list + 6)) >> 1) & 0x7FFF);  // rlwinm 31,17,31
            store<u32>(list + 8, static_cast<u32>(call_ptr(ctx, opd, {alloc, size, 0, 16, load<u32>(opd), opd})));  // r7 entry, r8 OPD
        }
        call(ctx, g12::fn::Array_Init, {list});
        store<u32>(list + 16, 0);
        store<u32>(list + 20, g14::opd_map_compare_u32);
    }
    store<u32>(g11::g::texture_list, list);
    struct Chunk { u32 tag, opd, align; };
    static constexpr Chunk kChunks[] = {
        {0x46525854, g14::opd_iff_texture_frxt, 0},  {0x52584554, g14::opd_iff_texture_rxet, 16},
        {0x48525854, g14::opd_iff_texture_hrxt, 16}, {0x434E4954, g14::opd_iff_texture_cnit, 16},
        {0x4E545844, g14::opd_iff_texture_data, 0},  {0x53444454, g14::opd_iff_texture_data, 0},
        {0x41475454, g14::opd_iff_texture_data, 0},  {0x504D4254, g14::opd_iff_texture_data, 0},
    };
    for (const Chunk& k : kChunks) call(ctx, g12::fn::Iff_RegisterChunk, {k.tag, k.opd, 0, 0, k.align});
    call(ctx, g14::fn::Texture_InitDefaults_0051DA1C, {});
    call(ctx, g14::fn::EnsureTextures_00542804, {});
}
