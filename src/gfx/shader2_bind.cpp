// The larger shader Bind slots (Shader vtable slot 8):
// void <Shader>_Bind(Shader* s, Material* mat)
#include "shader2.h"

#include "../view/view2.h"

using namespace sly4;

namespace {

namespace d3 = sly4::render::draw3;
namespace s2 = sly4::render::shader2;

f64 as_f64(u32 word) { return static_cast<f64>(std::bit_cast<f32>(word)); }

// MaterialAnim_UploadUV(anim, first) with the two scales given as raw f32 words.
void upload_uv(PPUContext& ctx, u32 anim, u32 first, u32 su, u32 sv) {
    callf(ctx, d3::fn::MaterialAnim_UploadUV, {anim, first}, {as_f64(su), as_f64(sv)});
}

// FProgram_SetConstant(index, v) on four words in a stack vector.
void set_constant(PPUContext& ctx, u32 index, u32 x, u32 y, u32 z, u32 w) {
    Stack frame(ctx, 16);
    const u32 v = frame.addr();
    store<u32>(v, x);
    store<u32>(v + 4, y);
    store<u32>(v + 8, z);
    store<u32>(v + 12, w);
    call(ctx, d3::fn::FProgram_SetConstant, {index, v});
}

}  // namespace

// SlyCaustics: the UV transform when animated (scales: parameters +0 /
// +4, k for missing parameters), then fragment constant 0 = {1 / p+8,
// 1 / p+12, blend mode 2 ? 1.0 : 0.0, ?} (k when missing). As in the
// original the fourth word is left unset (whatever the stack frame holds).
DECOMP(005BA7D0, SlyCaustics_Bind) {
    constexpr u32 k = 0x005BA7C8;  // f32 pair in .text (0.0, 1.0)
    const u32 mat = arg<u32>(ctx, 1);
    const u32 k0 = load<u32>(k), k1 = load<u32>(k + 4);
    u32 params = load<u32>(mat + d3::kMatParams);
    const u32 anim = load<u32>(mat + d3::kMatAnim);
    if (anim != 0) {
        upload_uv(ctx, anim, 4, s2::param_or(params, 0, k0), s2::param_or(params, 4, k0));
        params = load<u32>(mat + d3::kMatParams);  // reloaded after the call
    }
    Stack frame(ctx, 16);
    const u32 v = frame.addr();
    store<u32>(v, s2::fdivs_bits(k1, s2::param_or(params, 8, k0)));
    store<u32>(v + 4, s2::fdivs_bits(k1, s2::param_or(params, 12, k0)));
    store<u32>(v + 8, s2::blend_mode(mat) == 2 ? k1 : k0);
    call(ctx, d3::fn::FProgram_SetConstant, {0, v});
}

// SlyDecal: the UV transform when animated (scales: parameters +0 / +4),
// fragment constant 3.x = blend mode 2 ? 1.0 : 0.0 (y, z, w unset, as in
// the original), then fragment constant 0 = {p+8, p+12, p+16, 0.0}
// (0.0 for missing parameters).
DECOMP(005C38C0, SlyDecal_Bind) {
    constexpr u32 k = 0x005C38B4;  // f32 pair in .text (0.0, 1.0)
    const u32 mat = arg<u32>(ctx, 1);
    const u32 k0 = load<u32>(k);
    const u32 params = load<u32>(mat + d3::kMatParams);
    const u32 anim = load<u32>(mat + d3::kMatAnim);
    if (anim != 0) upload_uv(ctx, anim, 4, s2::param_or(params, 0, k0), s2::param_or(params, 4, k0));
    {
        Stack frame(ctx, 16);
        store<u32>(frame.addr(), s2::blend_mode(mat) == 2 ? load<u32>(k + 4) : k0);
        call(ctx, d3::fn::FProgram_SetConstant, {3, frame.addr()});
    }
    const u32 p = load<u32>(mat + d3::kMatParams);
    set_constant(ctx, 0, s2::param_or(p, 8, k0), s2::param_or(p, 12, k0), s2::param_or(p, 16, k0), k0);
}

// Diffuse: the UV transform (scales k, also without an animation block),
// fragment constant 0 = {p+0, p+4, p+8, 0.0} (0.0 for missing
// parameters); then, when the parameters' +12 alpha reference is
// positive (signed), alpha test GREATER than it, on.
DECOMP(005B1E88, Diffuse_Bind) {
    constexpr u32 k = 0x005B1E7C;  // f32 pair in .text (1.0, 0.0)
    const u32 mat = arg<u32>(ctx, 1);
    const u32 k0 = load<u32>(k);
    upload_uv(ctx, load<u32>(mat + d3::kMatAnim), 4, k0, k0);
    const u32 k1 = load<u32>(k + 4);
    const u32 params = load<u32>(mat + d3::kMatParams);
    set_constant(ctx, 0, s2::param_or(params, 0, k1), s2::param_or(params, 4, k1), s2::param_or(params, 8, k1), k1);
    const u32 p = load<u32>(mat + d3::kMatParams);
    if (p == 0) return;
    const u32 ref = load<u32>(p + 12);
    if (static_cast<s32>(ref) <= 0) return;  // cmpwi / bgt
    fx::emit({s2::kAlphaFunc2, s2::kGreater, ref});
    fx::emit({render::layers4::kAlphaTest, 1});
    ctx.gpr[3] = load<u32>(fx::g::state);  // left in r3
}

// SlyCharacter2: the UV transform (scales k, also without an animation
// block), fragment constant 0 = {p+0, p+4, p+8, 0.0}, then {p+40, p+48,
// 0.0, 0.0} (0.0 for missing parameters) as vertex constant 33 and
// fragment constant 4.
DECOMP(005BD298, SlyCharacter2_Bind) {
    constexpr u32 k = 0x005BD28C;  // f32 pair in .text (1.0, 0.0)
    const u32 mat = arg<u32>(ctx, 1);
    const u32 k0 = load<u32>(k);
    upload_uv(ctx, load<u32>(mat + d3::kMatAnim), 4, k0, k0);
    const u32 k1 = load<u32>(k + 4);
    const u32 params = load<u32>(mat + d3::kMatParams);
    set_constant(ctx, 0, s2::param_or(params, 0, k1), s2::param_or(params, 4, k1), s2::param_or(params, 8, k1), k1);
    const u32 p = load<u32>(mat + d3::kMatParams);
    Stack frame(ctx, 16);
    const u32 v = frame.addr();
    store<u32>(v, s2::param_or(p, 40, k1));
    store<u32>(v + 4, s2::param_or(p, 48, k1));
    store<u32>(v + 8, k1);
    store<u32>(v + 12, k1);
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 33, 4, v});
    call(ctx, d3::fn::FProgram_SetConstant, {4, v});
}

// TextureWarp: TextureWarp_BuildMatrices fills two 4 x 4 matrices and a
// vec4; the vec4 goes to fragment constant 1, the first three rows of
// each matrix (x, y, z; w left as the frame holds it, as in the
// original) to fragment constants 2..4 and 5..7.
DECOMP(005AE9A0, TextureWarp_Bind) {
    const u32 mat = arg<u32>(ctx, 1);
    // The original's sp+112..+303: vec4 (+0), three row vectors (+16..),
    // matrix 1 (+64), matrix 2 (+128).
    Stack frame(ctx, 192);
    const u32 b = frame.addr(), v = b, rows = b + 16, m1 = b + 64, m2 = b + 128;
    call(ctx, s2::fn::TextureWarp_BuildMatrices, {mat, m1, m2, v});
    call(ctx, d3::fn::FProgram_SetConstant, {1, v});
    for (u32 m : {m1, m2}) {
        for (u32 r = 0; r < 3; ++r) render::pool::copy3(rows + 16 * r, m + 16 * r);
        const u32 first = m == m1 ? 2 : 5;
        for (u32 r = 0; r < 3; ++r) call(ctx, d3::fn::FProgram_SetConstant, {first + r, rows + 16 * r});
    }
}

namespace {

// The two packed colours at params + off / + off + 4 (0 for missing
// parameters) as vertex constants 6 and 7 = {r, g, b, 0} * the f32 at
// `scale` (bits 16..23, 8..15, 0..7; the top byte unused).
void upload_colours(PPUContext& ctx, u32 params, u32 off, u32 scale_at) {
    const u32 colour[2] = {s2::param_or(params, off, 0), s2::param_or(params, off + 4, 0)};
    const f64 scale = static_cast<f64>(load<f32>(scale_at));
    Stack frame(ctx, 32);
    const u32 v = frame.addr();
    for (u32 c = 0; c < 2; ++c) {
        const u32 w = colour[c];
        const u32 bytes[3] = {(w >> 16) & 0xFF, (w >> 8) & 0xFF, w & 0xFF};
        for (u32 i = 0; i < 3; ++i) store<f32>(v + 16 * c + 4 * i, static_cast<f32>(static_cast<f64>(bytes[i]) * scale));  // fcfid, frsp, fmuls
        store<u32>(v + 16 * c + 12, 0);
    }
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 6, 4, v});
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 7, 4, v + 16});
}

}  // namespace

// SlyTerrainSnow: the UV transform when animated (scales k), then the two
// packed colours at parameters +12 / +16 as vertex constants 6 and 7
// (/ 255).
DECOMP(005CE508, SlyTerrainSnow_Bind) {
    constexpr u32 k = 0x005CE500;  // f32 in .text (1.0), then f32 1/255
    const u32 mat = arg<u32>(ctx, 1);
    if (const u32 anim = load<u32>(mat + d3::kMatAnim); anim != 0) upload_uv(ctx, anim, 4, load<u32>(k), load<u32>(k));
    upload_colours(ctx, load<u32>(mat + d3::kMatParams), 12, k + 4);
}

namespace {

// The depth-texture and depth-range set-up SprParticle_Bind and
// SlyHeightFog_Bind share: the scene depth texture on `unit` (bound when it
// changed; filter from the render state tables, clamp to edge), then
// fragment constant 0 = {near, far, k / near, (near - far) / (far *
// near)} from the current view.
void bind_depth(PPUContext& ctx, u32 unit, u32 address_method, u32 k) {
    const u32 st = fx::g::state;
    const u32 tex = static_cast<u32>(call(ctx, s2::fn::Display_GetDepthTexture, {sly4::g::mem_stats}));
    if (load<u32>(st + 52 + 4 * unit) != tex) {
        store<u32>(st + 52 + 4 * unit, tex);
        call(ctx, draw::fn::cellGcmSetTexture, {load<u32>(st), unit, tex + 16});
        const u32 levels = ref<u8>(tex + 17);
        call(ctx, draw::fn::cellGcmSetTextureControl, {load<u32>(st), unit, 1, 0, ((levels << 8) - 256) & 0xFFFF, 0});
    }
    call(ctx, draw::fn::cellGcmSetTextureFilter, {load<u32>(st), unit, 0, ref<u8>(st + 225), ref<u8>(st + 222), 1});
    fx::emit({address_method, s2::kClampToEdge3});
    const u32 view = static_cast<u32>(call(ctx, view::v2fn::View_GetCurrent));
    const u32 near_w = load<u32>(view + 60), far_w = load<u32>(view + 64);
    const f64 n = as_f64(near_w), f = as_f64(far_w);
    const f64 diff = render::pool::sgl(n - f), prod = render::pool::sgl(f * n);  // fsubs, fmuls
    set_constant(ctx, 0, near_w, far_w, s2::fdivs_bits(load<u32>(k), near_w),
                 std::bit_cast<u32>(static_cast<f32>(diff / prod)));  // fdivs
}

}  // namespace

// SlyHeightFog: the UV transform when animated (vertex constants 8..9,
// scales k), the scene depth texture on unit 4 (bound when it changed;
// filter from the render state tables; clamp to edge), fragment constant
// 0 = {near, far, 1 / near, (near - far) / (far * near)} from the current
// view, then the fog vertex constants (SlyHeightFog_UploadFog).
DECOMP(005C6D38, SlyHeightFog_Bind) {
    constexpr u32 k = 0x005C6D34;  // f32 in .text (1.0)
    const u32 mat = arg<u32>(ctx, 1);
    if (const u32 anim = load<u32>(mat + d3::kMatAnim); anim != 0) upload_uv(ctx, anim, 8, load<u32>(k), load<u32>(k));
    bind_depth(ctx, 4, s2::kTexAddress4, k);
    call(ctx, s2::fn::SlyHeightFog_UploadFog, {mat});
}

// SlyTextureWarp: TextureWarp's matrices (its own builder,
// SlyTextureWarp_BuildMatrices), the UV transform (scales k, also without
// an animation block), the vec4 as fragment constant 1 and the first
// three rows of each matrix (w as the frame holds it) as vertex constants
// 6..8 and 9..11.
DECOMP(005D1628, SlyTextureWarp_Bind) {
    constexpr u32 k = 0x005D1620;  // f32 in .text (1.0)
    const u32 mat = arg<u32>(ctx, 1);
    // The original's sp+112..+303, laid out as in TextureWarp_Bind.
    Stack frame(ctx, 192);
    const u32 b = frame.addr(), v = b, rows = b + 16, m1 = b + 64, m2 = b + 128;
    call(ctx, s2::fn::SlyTextureWarp_BuildMatrices, {mat, m1, m2, v});
    upload_uv(ctx, load<u32>(mat + d3::kMatAnim), 4, load<u32>(k), load<u32>(k));
    call(ctx, d3::fn::FProgram_SetConstant, {1, v});
    for (u32 m : {m1, m2}) {
        for (u32 r = 0; r < 3; ++r) render::pool::copy3(rows + 16 * r, m + 16 * r);
        const u32 first = m == m1 ? 6 : 9;
        for (u32 r = 0; r < 3; ++r) {
            call(ctx, s2::fn::cellGcmSetVertexProgramParameterBlock, {load<u32>(fx::g::state), first + r, 1, rows + 16 * r});
        }
    }
}

namespace {

// The two warp vectors ScreenWarp and SlyScreenWarp send as fragment
// constants 1 and 2: {t * p+0, t * p+4, p+8 * (k1 / w), 1.0}, {t * p+12,
// t * p+16, p+20 * (k1 / w), 1.0}, with t the f32 at
// layers3::g::block_dd5a8 +8, w g::config's u16 +4 and k0 for missing
// parameters.
void warp_vectors(u32 mat, u32 k, u32 v) {
    namespace sp = render::pool;
    const u32 params = load<u32>(mat + d3::kMatParams);
    const u32 k0 = load<u32>(k);
    const f64 t = sp::lfs(render::layers3::g::block_dd5a8 + 8);
    const auto scaled = [&](u32 off) { return static_cast<f32>(sp::sgl(t * as_f64(s2::param_or(params, off, k0)))); };  // fmuls
    store<f32>(v, scaled(0));
    store<f32>(v + 4, scaled(4));
    store<u32>(v + 8, s2::param_or(params, 8, k0));
    store<u32>(v + 12, s2::kOne);
    store<f32>(v + 16, scaled(12));
    store<f32>(v + 20, scaled(16));
    store<u32>(v + 24, s2::param_or(params, 20, k0));
    store<u32>(v + 28, s2::kOne);
    const f64 k1 = sp::lfs(k + 4);
    for (u32 off : {8u, 24u}) {
        const f64 w = static_cast<f64>(load<u16>(sly4::g::config + 4));  // fcfid, frsp (exact)
        sp::stfs(v + off, sp::sgl(sp::lfs(v + off) * sp::sgl(k1 / w)));  // fdivs, fmuls
    }
}

// Binds `tex` to unit 0 when it isn't already (render state +52).
void bind_unit0(PPUContext& ctx, u32 tex) {
    const u32 st = fx::g::state;
    store<u32>(st + 52, tex);
    call(ctx, draw::fn::cellGcmSetTexture, {load<u32>(st), 0, tex + 16});
    const u32 levels = ref<u8>(tex + 17);
    call(ctx, draw::fn::cellGcmSetTextureControl, {load<u32>(st), 0, 1, 0, ((levels << 8) - 256) & 0xFFFF, 0});
}

}  // namespace

// ScreenWarp: the warp vectors; unless g::config +152 bit 2 is set (then
// nothing more), fragment constants 1 and 2 and the screen copy
// (Draw_GetScreenCopy) on texture unit 0.
DECOMP(005AF518, ScreenWarp_Bind) {
    constexpr u32 k = 0x005AF50C;  // f32 pair in .text (0.0, 640.0)
    const u32 mat = arg<u32>(ctx, 1);
    Stack frame(ctx, 32);
    const u32 v = frame.addr();
    warp_vectors(mat, k, v);
    if (((ref<u8>(sly4::g::config + 152) >> 2) & 1) != 0) {  // rlwinm 30,31,31
        ctx.gpr[3] = 1;  // left in r3
        return;
    }
    call(ctx, d3::fn::FProgram_SetConstant, {1, v});
    call(ctx, d3::fn::FProgram_SetConstant, {2, v + 16});
    const u32 tex = static_cast<u32>(call(ctx, s2::fn::Draw_GetScreenCopy));
    if (load<u32>(fx::g::state + 52) == tex) {
        ctx.gpr[3] = fx::g::state;  // left in r3
        return;
    }
    bind_unit0(ctx, tex);
}

// SlyScreenWarp: the warp vectors as fragment constants 1 and 2, then the
// resolved scene texture (Stats_ResolveTexture) on unit 0 (bound when it
// changed; filter min from table entry +229, mag from +223; clamp to
// edge).
DECOMP(005CD480, SlyScreenWarp_Bind) {
    constexpr u32 k = 0x005CD474;  // f32 pair in .text (0.0, 640.0)
    const u32 mat = arg<u32>(ctx, 1);
    const u32 st = fx::g::state;
    {
        Stack frame(ctx, 32);
        const u32 v = frame.addr();
        warp_vectors(mat, k, v);
        call(ctx, d3::fn::FProgram_SetConstant, {1, v});
        call(ctx, d3::fn::FProgram_SetConstant, {2, v + 16});
    }
    const u32 tex = static_cast<u32>(call(ctx, render::layers4::fn::Stats_ResolveTexture, {sly4::g::mem_stats}));
    if (load<u32>(st + 52) != tex) bind_unit0(ctx, tex);
    call(ctx, draw::fn::cellGcmSetTextureFilter, {load<u32>(st), 0, 0, ref<u8>(st + 229), ref<u8>(st + 223), 1});
    fx::emit({render::layers4::kTexAddress, s2::kClampToEdge3});
    ctx.gpr[3] = load<u32>(st);  // left in r3
}

namespace {

// The parabolic sine approximation SlyDecal4 inlines (k: {0, 4/pi,
// -4/pi^2, 0.225, pi/2, pi, 2 pi}): y = x * k1 + x * k2 * |x|, refined
// to (y * |y| - y) * k3 + y; every step single precision.
f64 sine(u32 k, f64 x) {
    namespace sp = render::pool;
    const f64 k1 = sp::lfs(k + 4), k2 = sp::lfs(k + 8), k3 = sp::lfs(k + 12);
    const f64 y = sp::fmadds(x, k1, sp::sgl(sp::sgl(x * k2) * std::fabs(x)));
    return sp::fmadds(sp::fmsubs(y, std::fabs(y), y), k3, y);
}
// cos(x) = sine(x + pi/2), wrapped by `two_pi` (by default the f32 at k +
// 24) above pi.
f64 cosine(u32 k, f64 x, u32 two_pi = 0) {
    namespace sp = render::pool;
    f64 c = sp::sgl(x + sp::lfs(k + 16));  // fadds
    if (c > sp::lfs(k + 20)) c = sp::sgl(c - sp::lfs(two_pi != 0 ? two_pi : k + 24));  // fcmpu / bgt, fsubs
    return sine(k, c);
}

// `n` 2 x 2 rotation-scale rows (SlyDecal4, SlyTerrainSnowDecal) as vertex
// constants first..: row i (angle params + angle_off + 4i, scales
// params + scale_off + 8i / + 4; def_word for missing parameters) = {cos
// * sx, -(sin * sy), sin * sx, cos * sy}, sine constants at k.
void upload_rotations(PPUContext& ctx, u32 params, u32 def_word, u32 k, u32 angle_off, u32 scale_off, u32 n, u32 first) {
    Stack frame(ctx, 48);
    const u32 v = frame.addr();
    for (u32 i = 0; i < n; ++i) {
        const f64 a = as_f64(s2::param_or(params, angle_off + 4 * i, def_word));
        const f64 sx = as_f64(s2::param_or(params, scale_off + 8 * i, def_word));
        const f64 sy = as_f64(s2::param_or(params, scale_off + 8 * i + 4, def_word));
        const f64 s = sine(k, a), c = cosine(k, a);
        const u32 row = v + 16 * i;
        store<f32>(row, static_cast<f32>(c * sx));  // fmuls
        store<f32>(row + 4, -static_cast<f32>(s * sy));  // fmuls, fneg
        store<f32>(row + 8, static_cast<f32>(s * sx));
        store<f32>(row + 12, static_cast<f32>(c * sy));
    }
    for (u32 i = 0; i < n; ++i) {
        call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), first + i, 4, v + 16 * i});
    }
}

}  // namespace

// SlyDecal4: three rotation-scale rows for the decal layers as vertex
// constants 4..6 (angles p+24.., scales p+0..; k0 for missing
// parameters).
DECOMP(005C49C0, SlyDecal4_Bind) {
    constexpr u32 k = 0x005C49A4;  // f32[7] in .text: 0, then the sine constants
    const u32 mat = arg<u32>(ctx, 1);
    upload_rotations(ctx, load<u32>(mat + d3::kMatParams), load<u32>(k), k, 24, 0, 3, 4);
}

// SlyTerrainSnowDecal: the UV transform when animated (scales k), the two
// packed colours at parameters +0 / +4 as vertex constants 6 and 7 (/
// 255), then two rotation-scale rows (angles p+48.., scales p+32..; 0.0
// for missing parameters) as vertex constants 8 and 9.
DECOMP(005CFA68, SlyTerrainSnowDecal_Bind) {
    constexpr u32 k = 0x005CFA40;  // f32 in .text: 1.0, 1/255, 0.0, then the sine constants
    const u32 mat = arg<u32>(ctx, 1);
    if (const u32 anim = load<u32>(mat + d3::kMatAnim); anim != 0) upload_uv(ctx, anim, 4, load<u32>(k), load<u32>(k));
    upload_colours(ctx, load<u32>(mat + d3::kMatParams), 0, k + 4);
    upload_rotations(ctx, load<u32>(mat + d3::kMatParams), load<u32>(k + 8), k + 8, 48, 32, 2, 8);
}

namespace {

// The first three rows of 4 x 4 matrix m (x, y, z; w left as the frame
// holds it, as in the original) copied to `rows` and sent as fragment
// constants (or, with `vertex`, vertex constants) first..first + 2.
void send_rows(PPUContext& ctx, u32 rows, u32 m, u32 first, bool vertex) {
    for (u32 r = 0; r < 3; ++r) render::pool::copy3(rows + 16 * r, m + 16 * r);
    for (u32 r = 0; r < 3; ++r) {
        if (vertex) {
            call(ctx, s2::fn::cellGcmSetVertexProgramParameterBlock, {load<u32>(fx::g::state), first + r, 1, rows + 16 * r});
        } else {
            call(ctx, d3::fn::FProgram_SetConstant, {first + r, rows + 16 * r});
        }
    }
}

// Render state +88 bit `bit` = on, then sent with method 0x1FF8.
void set_1ff8_bit(PPUContext& ctx, u32 bit, bool on) {
    const u32 st = fx::g::state;
    const u32 ctxp = load<u32>(st);
    const u32 w = (load<u32>(st + 88) & ~(1u << bit)) | (static_cast<u32>(on) << bit);
    store<u32>(st + 88, w);
    call(ctx, s2::fn::cellGcmSetMethod1FF8, {ctxp, w});
}

// Parameters +off set (nonzero) on a material with parameters.
bool param_set(u32 params, u32 off) { return params != 0 && load<u32>(params + off) != 0; }

}  // namespace

// SlyWater: render state +88 bit 0 = parameters +60 set; the UV transform
// when animated (vertex constants 9..10, scales k0); material +64 as
// vertex constant 12; {p+48, p+52, 0, 0} as vertex constant 13;
// TextureWarp's matrices (fragment constants 1, 2..4, 5..7); then {0,
// p+40, p+44, p+56} as fragment constant 9 (0.0 for missing parameters).
DECOMP(005D2360, SlyWater_Bind) {
    constexpr u32 k = 0x005D2354;  // f32 pair in .text (1.0, 0.0)
    const u32 mat = arg<u32>(ctx, 1);
    set_1ff8_bit(ctx, 0, param_set(load<u32>(mat + d3::kMatParams), 60));
    if (const u32 anim = load<u32>(mat + d3::kMatAnim); anim != 0) upload_uv(ctx, anim, 9, load<u32>(k), load<u32>(k));
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 12, 4, mat + 64});
    const u32 k1 = load<u32>(k + 4);
    // The original's sp+112..+335: vertex constant 13 (+0), the warp vec4
    // (+16), fragment constant 9 (+32), rows (+48..), matrices (+96, +160).
    Stack frame(ctx, 224);
    const u32 b = frame.addr(), v13 = b, vec = b + 16, c9 = b + 32, rows = b + 48, m1 = b + 96, m2 = b + 160;
    u32 p = load<u32>(mat + d3::kMatParams);
    store<u32>(v13, s2::param_or(p, 48, k1));
    store<u32>(v13 + 4, s2::param_or(p, 52, k1));
    store<u32>(v13 + 8, k1);
    store<u32>(v13 + 12, k1);
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 13, 4, v13});
    call(ctx, s2::fn::TextureWarp_BuildMatrices, {mat, m1, m2, vec});
    call(ctx, d3::fn::FProgram_SetConstant, {1, vec});
    send_rows(ctx, rows, m1, 2, false);
    send_rows(ctx, rows, m2, 5, false);
    p = load<u32>(mat + d3::kMatParams);
    store<u32>(c9, k1);
    store<u32>(c9 + 4, s2::param_or(p, 40, k1));
    store<u32>(c9 + 8, s2::param_or(p, 44, k1));
    store<u32>(c9 + 12, s2::param_or(p, 56, k1));
    call(ctx, d3::fn::FProgram_SetConstant, {9, c9});
}

namespace {

// SlyWater2 / SlyWater3 (they differ in the constant indices and when
// their wave upload runs). Render state +88 bit 0 = parameters +60 set;
// splat(p+64) as vertex constant `splat`; the UV transform (vertex
// constants 13..14, scales k1, also without an animation block); material
// +64 as vertex constant 33; {p+48, p+52, 0, 0} as vertex constant 34;
// the wave upload; TextureWarp's matrices: {p+68, p+80, 1, 1} as fragment
// constant `scroll`, the vec4 as fragment constant 0, the rows as vertex
// constants rows_first..+5; then {0, p+40, p+44, p+56} as fragment
// constant 1 (0.0 for missing parameters).
void sly_water23_bind(PPUContext& ctx, u32 k, u32 splat, u32 waves, bool waves_first, u32 scroll, u32 rows_first) {
    const u32 mat = arg<u32>(ctx, 1);
    const u32 k0 = load<u32>(k);
    // The original's sp+112..+367: splat (+0), vertex constant 34 (+16),
    // the warp vec4 (+32), scroll (+48), fragment constant 1 (+64), rows
    // (+80..), matrices (+128, +192).
    Stack frame(ctx, 256);
    const u32 b = frame.addr(), vs = b, v34 = b + 16, vec = b + 32, vsc = b + 48, c1 = b + 64, rows = b + 80, m1 = b + 128,
              m2 = b + 192;
    u32 p = load<u32>(mat + d3::kMatParams);
    const bool on = param_set(p, 60);
    const u32 s = s2::param_or(p, 64, k0);
    for (u32 i = 0; i < 16; i += 4) store<u32>(vs + i, s);
    set_1ff8_bit(ctx, 0, on);
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), splat, 4, vs});
    upload_uv(ctx, load<u32>(mat + d3::kMatAnim), 13, load<u32>(k + 4), load<u32>(k + 4));
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 33, 4, mat + 64});
    p = load<u32>(mat + d3::kMatParams);
    store<u32>(v34, s2::param_or(p, 48, k0));
    store<u32>(v34 + 4, s2::param_or(p, 52, k0));
    store<u32>(v34 + 8, k0);
    store<u32>(v34 + 12, k0);
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 34, 4, v34});
    if (waves_first) call(ctx, waves, {mat});
    call(ctx, s2::fn::TextureWarp_BuildMatrices, {mat, m1, m2, vec});
    if (!waves_first) call(ctx, waves, {mat});
    p = load<u32>(mat + d3::kMatParams);
    store<u32>(vsc, s2::param_or(p, 68, k0));
    store<u32>(vsc + 4, s2::param_or(p, 80, k0));
    store<u32>(vsc + 8, s2::kOne);
    store<u32>(vsc + 12, s2::kOne);
    call(ctx, d3::fn::FProgram_SetConstant, {scroll, vsc});
    call(ctx, d3::fn::FProgram_SetConstant, {0, vec});
    send_rows(ctx, rows, m1, rows_first, true);
    send_rows(ctx, rows, m2, rows_first + 3, true);
    p = load<u32>(mat + d3::kMatParams);
    store<u32>(c1, k0);
    store<u32>(c1 + 4, s2::param_or(p, 40, k0));
    store<u32>(c1 + 8, s2::param_or(p, 44, k0));
    store<u32>(c1 + 12, s2::param_or(p, 56, k0));
    call(ctx, d3::fn::FProgram_SetConstant, {1, c1});
}

}  // namespace

DECOMP(005D33C8, SlyWater2_Bind) {
    sly_water23_bind(ctx, 0x005D33BC, 40, s2::fn::SlyWater2_UploadWaves, true, 4, 41);  // k: f32 pair in .text (0.0, 1.0)
}
DECOMP(005D4C80, SlyWater3_Bind) {
    sly_water23_bind(ctx, 0x005D4C74, 39, s2::fn::SlyWater3_UploadWaves, false, 5, 40);  // k: f32 pair in .text (0.0, 1.0)
}

// Water: the UV transform (MaterialAnim_UploadUVAlt, vertex constants
// 9..10, scales k0, also without an animation block); render state +88
// bit 0 = parameters +40 set; TextureWarp's matrices (fragment constants
// 1, 2..4, 5..7); bit 1 = p+44 > 0 or p+64 > 0; then two waves (i = 0, 1;
// parameters +44 + 20i: amplitude, +48 speed, +52 length, +56, +60 phase
// in degrees; 0.0 when missing) as vertex constants 11..12 = {2 pi /
// length, amplitude, speed * 2 pi / length * t, p+56} and 13..14 =
// {sin(phase), 0, cos(phase), 0}, with t the f32 at
// layers3::g::block_dd5a8 +8 and the phase wrapped to [-pi, pi].
DECOMP(005B5990, Water_Bind) {
    namespace sp = render::pool;
    constexpr u32 k = 0x005B5958;  // .text: f32 1.0, 0.0, 2 pi, pi / 180; f64 pi, -pi; f32 sine constants (from +28)
    constexpr u32 kSine = k + 28;
    const u32 mat = arg<u32>(ctx, 1);
    const u32 k0 = load<u32>(k), zero = load<u32>(k + 4);
    callf(ctx, s2::fn::MaterialAnim_UploadUVAlt, {load<u32>(mat + d3::kMatAnim), 9}, {as_f64(k0), as_f64(k0)});
    set_1ff8_bit(ctx, 0, param_set(load<u32>(mat + d3::kMatParams), 40));
    // The original's sp+112..+367: the warp vec4 (+0), wave vectors (+16,
    // +48), rows (+80..), matrices (+128, +192).
    Stack frame(ctx, 256);
    const u32 b = frame.addr(), vec = b, wa = b + 16, wb = b + 48, rows = b + 80, m1 = b + 128, m2 = b + 192;
    call(ctx, s2::fn::TextureWarp_BuildMatrices, {mat, m1, m2, vec});
    call(ctx, d3::fn::FProgram_SetConstant, {1, vec});
    send_rows(ctx, rows, m1, 2, false);
    send_rows(ctx, rows, m2, 5, false);

    u32 p = load<u32>(mat + d3::kMatParams);
    const f64 fzero = as_f64(zero);
    const bool moving = as_f64(s2::param_or(p, 44, zero)) > fzero || as_f64(s2::param_or(p, 64, zero)) > fzero;  // fcmpu / bgt
    set_1ff8_bit(ctx, 1, moving);

    const f64 t = sp::lfs(render::layers3::g::block_dd5a8 + 8);
    const f64 two_pi = sp::lfs(k + 8), deg = sp::lfs(k + 12), pi = load<f64>(k + 16), neg_pi = load<f64>(k + 24);
    p = load<u32>(mat + d3::kMatParams);
    for (u32 i = 0; i < 2; ++i) {
        const u32 o = 20 * i, a = wa + 16 * i, w = wb + 16 * i;
        const auto param = [&](u32 off) { return s2::param_or(p, off + o, zero); };
        sp::stfs(a, sp::sgl(two_pi / as_f64(param(52))));  // fdivs
        store<u32>(a + 4, param(44));
        sp::stfs(a + 8, sp::sgl(sp::sgl(as_f64(param(48)) * sp::lfs(a)) * t));  // fmuls, fmuls
        store<u32>(a + 12, param(56));
        const f64 ang = sp::sgl(as_f64(param(60)) * deg);  // fmuls
        const f64 half = sp::sgl(sp::sel_ge(ang, pi, neg_pi));  // fsel, frsp
        f64 x = sp::sgl(half + ang);  // fadds
        const f64 turns = sp::sgl(rt::fcfid(rt::fctid(sp::sgl(x / two_pi), true)));  // fdivs, fctidz, fcfid, frsp
        x = sp::fnmsubs(turns, two_pi, x);
        const f64 phase = sp::sgl(x - half);  // fsubs
        sp::stfs(w, sine(kSine, phase));
        store<u32>(w + 4, zero);
        sp::stfs(w + 8, cosine(kSine, phase, k + 8));
        store<u32>(w + 12, zero);
    }
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 11, 8, wa});
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 13, 8, wb});
}


// SprParticle: the scene depth texture on unit 1 and the depth range
// constant; then the depth bias (parameters +16, k1 = 0.0 when missing):
// none when it is 0.0; polygon offset off without depth test (render
// state +93) or depth target (+84), or for a depth format (target +24)
// other than 4 / 6 (bias * 65536) and 5 / 7 (bias * 2^24), which also
// sets g::depth_format_warned; else polygon offset on with {0.0, the
// scaled bias}.
DECOMP(005B06B0, SprParticle_Bind) {
    constexpr u32 k = 0x005B069C;  // f32[4] in .text (1.0, 0.0, 65536.0, 16777216.0)
    const u32 mat = arg<u32>(ctx, 1);
    const u32 st = fx::g::state;
    bind_depth(ctx, 1, s2::kTexAddress1, k);
    const u32 k1 = load<u32>(k + 4);
    const u32 target = load<u32>(st + 84);
    const f64 bias = as_f64(s2::param_or(load<u32>(mat + d3::kMatParams), 16, k1));
    if (bias == as_f64(k1)) {  // fcmpu / beq
        ctx.gpr[3] = target;  // left in r3
        return;
    }
    bool off = ref<u8>(st + 93) == 0 || target == 0;
    f64 scaled = bias;
    if (!off) {
        const s32 format = load<s32>(target + 24);
        if (format == 4 || format == 6) {
            scaled = render::pool::sgl(bias * render::pool::lfs(k + 8));  // fmuls
        } else if (format == 5 || format == 7) {
            scaled = render::pool::sgl(bias * render::pool::lfs(k + 12));
        } else {
            if (ref<u8>(s2::g::depth_format_warned) == 0) store<u8>(s2::g::depth_format_warned, 1);
            off = true;
        }
    }
    if (off) {
        fx::emit({s2::kPolyOffsetFill, 0});
        ctx.gpr[3] = s2::kPolyOffsetFill;  // left in r3
        return;
    }
    fx::emit({s2::kPolyOffsetFill, 1});
    fx::emit({s2::kPolyOffset2, k1, std::bit_cast<u32>(static_cast<f32>(scaled))});
    ctx.gpr[3] = load<u32>(st);  // left in r3
}
