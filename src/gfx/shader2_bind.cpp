// The larger shader Bind slots (Shader vtable slot 8):
// void <Shader>_Bind(ShaderPrograms* s, Material* mat)
#include "shader2.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

namespace d3 = sly4::render::draw3;
namespace s2 = sly4::render::shader2;
constexpr u32 kAnimTime = 0x010DD5A8 + 8;  // f32 (layers3::g::block_dd5a8 +8)

const be_u32* params_of(const Material* mat) { return host<be_u32>(or_zero_page(mat)->w124); }
u32 anim_of(const Material* mat) { return or_zero_page(mat)->w128; }
// params + off when there are parameters, else the default word.
u32 param_or(const be_u32* params, u32 off, u32 def_word) { return params ? at<be_u32>(params, off)->get() : def_word; }
// Parameters +off set (nonzero) on a material with parameters.
bool param_set(const be_u32* params, u32 off) { return params && *at<be_u32>(params, off) != 0; }
u32 word(u32 ea) { return global<be_u32>(ea); }
u32 blend_mode(const Material* mat) { return or_zero_page(mat)->w120 >> 28; }
u32 fdivs_bits(u32 a, u32 b) { return bits(as_f32(a) / as_f32(b)); }

// MaterialAnim_UploadUV(anim, first) with the two scales given as raw f32 words.
void upload_uv(u32 anim, u32 first, u32 su, u32 sv) {
    MaterialAnim_UploadUV(anim, first, as_f32(su), as_f32(sv));
}

void vconst(u32 first, const void* v) { call(fn::cellGcmSetVertexProgramConstants, gcm(), first, 4u, v); }
void fconst_set(u32 index, const void* v) { FProgram_SetConstant(index, static_cast<const be_u32*>(v)); }

// FProgram_SetConstant(index, v) on four words in a stack vector.
void set_constant(u32 index, u32 x, u32 y, u32 z, u32 w) {
    GuestFrame f(16);
    be_u32* v = f.as<be_u32>();
    v[0] = x;
    v[1] = y;
    v[2] = z;
    v[3] = w;
    fconst_set(index, v);
}

// The first three rows of 4 x 4 matrix m (x, y, z; w left as the frame
// holds it, as in the original) copied to `rows` and sent as fragment
// constants (or, with `vertex`, vertex constants) first..first + 2.
void send_rows(be_u32* rows, const be_u32* m, u32 first, bool vertex) {
    for (u32 r = 0; r < 3; ++r) copy3(rows + 4 * r, m + 4 * r);
    for (u32 r = 0; r < 3; ++r) {
        if (vertex) call(s2::fn::cellGcmSetVertexProgramParameterBlock, gcm(), first + r, 1u, rows + 4 * r);
        else fconst_set(first + r, rows + 4 * r);
    }
}

}  // namespace

// SlyCaustics: the UV transform when animated (scales: parameters +0 /
// +4, k for missing parameters), then fragment constant 0 = {1 / p+8,
// 1 / p+12, blend mode 2 ? 1.0 : 0.0, ?} (k when missing). As in the
// original the fourth word is left unset (whatever the stack frame holds).
void SlyCaustics_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005BA7C8;  // f32 pair in .text (0.0, 1.0)
    const u32 k0 = word(k), k1 = word(k + 4);
    const be_u32* params = params_of(mat);
    const u32 anim = anim_of(mat);
    if (anim != 0) {
        upload_uv(anim, 4, param_or(params, 0, k0), param_or(params, 4, k0));
        params = params_of(mat);  // reloaded after the call
    }
    GuestFrame f(16);
    be_u32* v = f.as<be_u32>();
    v[0] = fdivs_bits(k1, param_or(params, 8, k0));
    v[1] = fdivs_bits(k1, param_or(params, 12, k0));
    v[2] = blend_mode(mat) == 2 ? k1 : k0;
    fconst_set(0, v);
}

// SlyDecal: the UV transform when animated (scales: parameters +0 / +4),
// fragment constant 3.x = blend mode 2 ? 1.0 : 0.0 (y, z, w unset, as in
// the original), then fragment constant 0 = {p+8, p+12, p+16, 0.0}
// (0.0 for missing parameters).
void SlyDecal_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005C38B4;  // f32 pair in .text (0.0, 1.0)
    const u32 k0 = word(k);
    const be_u32* params = params_of(mat);
    const u32 anim = anim_of(mat);
    if (anim != 0) upload_uv(anim, 4, param_or(params, 0, k0), param_or(params, 4, k0));
    {
        GuestFrame f(16);
        be_u32* v = f.as<be_u32>();
        v[0] = blend_mode(mat) == 2 ? word(k + 4) : k0;
        fconst_set(3, v);
    }
    const be_u32* p = params_of(mat);
    set_constant(0, param_or(p, 8, k0), param_or(p, 12, k0), param_or(p, 16, k0), k0);
}

// Diffuse: the UV transform (scales k, also without an animation block),
// fragment constant 0 = {p+0, p+4, p+8, 0.0} (0.0 for missing
// parameters); then, when the parameters' +12 alpha reference is
// positive (signed), alpha test GREATER than it, on.
void Diffuse_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005B1E7C;  // f32 pair in .text (1.0, 0.0)
    const u32 k0 = word(k);
    upload_uv(anim_of(mat), 4, k0, k0);
    const u32 k1 = word(k + 4);
    const be_u32* params = params_of(mat);
    set_constant(0, param_or(params, 0, k1), param_or(params, 4, k1), param_or(params, 8, k1), k1);
    const be_u32* p = params_of(mat);
    if (!p) return;
    const u32 ref = p[3];
    if (static_cast<s32>(ref) <= 0) return;  // cmpwi / bgt
    emit({s2::kAlphaFunc2, s2::kGreater, ref});
    emit({kAlphaTestEnable, 1});
}

// SlyCharacter2: the UV transform (scales k, also without an animation
// block), fragment constant 0 = {p+0, p+4, p+8, 0.0}, then {p+40, p+48,
// 0.0, 0.0} (0.0 for missing parameters) as vertex constant 33 and
// fragment constant 4.
void SlyCharacter2_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005BD28C;  // f32 pair in .text (1.0, 0.0)
    const u32 k0 = word(k);
    upload_uv(anim_of(mat), 4, k0, k0);
    const u32 k1 = word(k + 4);
    const be_u32* params = params_of(mat);
    set_constant(0, param_or(params, 0, k1), param_or(params, 4, k1), param_or(params, 8, k1), k1);
    const be_u32* p = params_of(mat);
    GuestFrame f(16);
    be_u32* v = f.as<be_u32>();
    v[0] = param_or(p, 40, k1);
    v[1] = param_or(p, 48, k1);
    v[2] = k1;
    v[3] = k1;
    vconst(33, v);
    fconst_set(4, v);
}

// TextureWarp: TextureWarp_BuildMatrices fills two 4 x 4 matrices and a
// vec4; the vec4 goes to fragment constant 1, the first three rows of
// each matrix (x, y, z; w left as the frame holds it, as in the
// original) to fragment constants 2..4 and 5..7.
void TextureWarp_Bind(ShaderPrograms*, Material* mat) {
    // The original's sp+112..+303: vec4 (+0), three row vectors (+16..),
    // matrix 1 (+64), matrix 2 (+128).
    GuestFrame f(192);
    be_u32* v = f.as<be_u32>(0);
    be_u32* rows = f.as<be_u32>(16);
    be_u32* m1 = f.as<be_u32>(64);
    be_u32* m2 = f.as<be_u32>(128);
    TextureWarp_BuildMatrices(mat, reinterpret_cast<be_f32*>(m1), reinterpret_cast<be_f32*>(m2), v);
    fconst_set(1, v);
    send_rows(rows, m1, 2, false);
    send_rows(rows, m2, 5, false);
}

namespace {

// The two packed colours at params + off / + off + 4 (0 for missing
// parameters) as vertex constants 6 and 7 = {r, g, b, 0} * the f32 at
// `scale` (bits 16..23, 8..15, 0..7; the top byte unused).
void upload_colours(const be_u32* params, u32 off, u32 scale_at) {
    const u32 colour[2] = {param_or(params, off, 0), param_or(params, off + 4, 0)};
    const f32 scale = global<be_f32>(scale_at);
    GuestFrame f(32);
    be_f32* v = f.as<be_f32>();
    for (u32 c = 0; c < 2; ++c) {
        const u32 w = colour[c];
        const u32 bytes[3] = {(w >> 16) & 0xFF, (w >> 8) & 0xFF, w & 0xFF};
        for (u32 i = 0; i < 3; ++i) v[4 * c + i] = static_cast<f32>(bytes[i]) * scale;  // fcfid, frsp, fmuls
        *reinterpret_cast<be_u32*>(v + 4 * c + 3) = 0u;
    }
    vconst(6, v);
    vconst(7, v + 4);
}

}  // namespace

// SlyTerrainSnow: the UV transform when animated (scales k), then the two
// packed colours at parameters +12 / +16 as vertex constants 6 and 7
// (/ 255).
void SlyTerrainSnow_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005CE500;  // f32 in .text (1.0), then f32 1/255
    if (const u32 anim = anim_of(mat); anim != 0) upload_uv(anim, 4, word(k), word(k));
    upload_colours(params_of(mat), 12, k + 4);
}

namespace {

// The depth-texture and depth-range set-up SprParticle_Bind and
// SlyHeightFog_Bind share: the scene depth texture on `unit` (bound when it
// changed; filter from the render state tables, clamp to edge), then
// fragment constant 0 = {near, far, k / near, (near - far) / (far *
// near)} from the current view.
void bind_depth(u32 unit, u32 address_method, u32 k) {
    RenderState& st = rs();
    Texture* tex = Display_GetDepthTexture(&mem_stats());
    gp<Texture>* slot = at<gp<Texture>>(&st, 52 + 4 * unit);
    if (slot->get() != tex) {
        *slot = tex;
        call(fn::cellGcmSetTexture, gcm(), unit, at<GcmTexture>(tex, 16));
        const u32 levels = *at<u8>(tex, 17);
        call(fn::cellGcmSetTextureControl, gcm(), unit, 1u, 0u, ((levels << 8) - 256) & 0xFFFF, 0u);
    }
    call(fn::cellGcmSetTextureFilter, gcm(), unit, 0u, static_cast<u32>(st.min_filter[0]), static_cast<u32>(st.mag_filter[0]), 1u);
    emit({address_method, s2::kClampToEdge3});
    const View* view = or_zero_page(current_view());
    const u32 near_w = *reinterpret_cast<const be_u32*>(&view->near_z);
    const u32 far_w = *reinterpret_cast<const be_u32*>(&view->far_z);
    const f32 n = as_f32(near_w), f = as_f32(far_w);
    const f32 diff = n - f, prod = f * n;  // fsubs, fmuls
    set_constant(0, near_w, far_w, fdivs_bits(word(k), near_w), bits(diff / prod));  // fdivs
}

}  // namespace

// SlyHeightFog: the UV transform when animated (vertex constants 8..9,
// scales k), the scene depth texture on unit 4 (bound when it changed;
// filter from the render state tables; clamp to edge), fragment constant
// 0 = {near, far, 1 / near, (near - far) / (far * near)} from the current
// view, then the fog vertex constants (SlyHeightFog_UploadFog).
void SlyHeightFog_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005C6D34;  // f32 in .text (1.0)
    if (const u32 anim = anim_of(mat); anim != 0) upload_uv(anim, 8, word(k), word(k));
    bind_depth(4, s2::kTexAddress4, k);
    SlyHeightFog_UploadFog(mat);
}

// SlyTextureWarp: TextureWarp's matrices (its own builder,
// SlyTextureWarp_BuildMatrices), the UV transform (scales k, also without
// an animation block), the vec4 as fragment constant 1 and the first
// three rows of each matrix (w as the frame holds it) as vertex constants
// 6..8 and 9..11.
void SlyTextureWarp_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005D1620;  // f32 in .text (1.0)
    // The original's sp+112..+303, laid out as in TextureWarp_Bind.
    GuestFrame f(192);
    be_u32* v = f.as<be_u32>(0);
    be_u32* rows = f.as<be_u32>(16);
    be_u32* m1 = f.as<be_u32>(64);
    be_u32* m2 = f.as<be_u32>(128);
    SlyTextureWarp_BuildMatrices(mat, reinterpret_cast<be_f32*>(m1), reinterpret_cast<be_f32*>(m2), v);
    upload_uv(anim_of(mat), 4, word(k), word(k));
    fconst_set(1, v);
    send_rows(rows, m1, 6, true);
    send_rows(rows, m2, 9, true);
}

namespace {

// The two warp vectors ScreenWarp and SlyScreenWarp send as fragment
// constants 1 and 2: {t * p+0, t * p+4, p+8 * (k1 / w), 1.0}, {t * p+12,
// t * p+16, p+20 * (k1 / w), 1.0}, with t the f32 at
// layers3::g::block_dd5a8 +8, w g::config's u16 +4 and k0 for missing
// parameters.
void warp_vectors(Material* mat, u32 k, be_f32* v) {
    const be_u32* params = params_of(mat);
    const u32 k0 = word(k);
    const f32 t = global<be_f32>(kAnimTime);
    const auto scaled = [&](u32 off) { return t * as_f32(param_or(params, off, k0)); };  // fmuls
    be_u32* w = reinterpret_cast<be_u32*>(v);
    v[0] = scaled(0);
    v[1] = scaled(4);
    w[2] = param_or(params, 8, k0);
    w[3] = s2::kOne;
    v[4] = scaled(12);
    v[5] = scaled(16);
    w[6] = param_or(params, 20, k0);
    w[7] = s2::kOne;
    const f32 k1 = global<be_f32>(k + 4);
    for (u32 i : {2u, 6u}) {
        const f32 width = static_cast<f32>(config().width.get());  // fcfid, frsp (exact)
        v[i] = v[i] * (k1 / width);  // fdivs, fmuls
    }
}

// Binds `tex` to unit 0 (render state +52).
void bind_unit0(Texture* tex) {
    rs().texture[0] = tex;
    call(fn::cellGcmSetTexture, gcm(), 0u, at<GcmTexture>(tex, 16));
    const u32 levels = *at<u8>(tex, 17);
    call(fn::cellGcmSetTextureControl, gcm(), 0u, 1u, 0u, ((levels << 8) - 256) & 0xFFFF, 0u);
}

}  // namespace

// ScreenWarp: the warp vectors; unless g::config +152 bit 2 is set (then
// nothing more), fragment constants 1 and 2 and the screen copy
// (Draw_GetScreenCopy) on texture unit 0.
void ScreenWarp_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005AF50C;  // f32 pair in .text (0.0, 640.0)
    GuestFrame f(32);
    be_f32* v = f.as<be_f32>();
    warp_vectors(mat, k, v);
    if (((config_u8(152) >> 2) & 1) != 0) return;  // rlwinm 30,31,31
    fconst_set(1, v);
    fconst_set(2, v + 4);
    Texture* tex = Draw_GetScreenCopy();
    if (rs().texture[0].get() == tex) return;
    bind_unit0(tex);
}

// SlyScreenWarp: the warp vectors as fragment constants 1 and 2, then the
// resolved scene texture (Stats_ResolveTexture) on unit 0 (bound when it
// changed; filter min from table entry +229, mag from +223; clamp to
// edge).
void SlyScreenWarp_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005CD474;  // f32 pair in .text (0.0, 640.0)
    RenderState& st = rs();
    {
        GuestFrame f(32);
        be_f32* v = f.as<be_f32>();
        warp_vectors(mat, k, v);
        fconst_set(1, v);
        fconst_set(2, v + 4);
    }
    Texture* tex = Stats_ResolveTexture(&mem_stats());
    if (st.texture[0].get() != tex) bind_unit0(tex);
    call(fn::cellGcmSetTextureFilter, gcm(), 0u, 0u, static_cast<u32>(*at<u8>(&st, 229)), static_cast<u32>(*at<u8>(&st, 223)), 1u);
    emit({kTextureAddress0, s2::kClampToEdge3});
}

namespace {

// The parabolic sine approximation SlyDecal4 inlines (k: {0, 4/pi,
// -4/pi^2, 0.225, pi/2, pi, 2 pi}): y = x * k1 + x * k2 * |x|, refined
// to (y * |y| - y) * k3 + y; every step single precision.
f32 sine(u32 k, f32 x) {
    const be_f32* c = host<be_f32>(k);
    const f32 k1 = c[1], k2 = c[2], k3 = c[3];
    const f32 y = fmadds(x, k1, (x * k2) * std::fabs(x));
    return fmadds(fmsubs(y, std::fabs(y), y), k3, y);
}
// cos(x) = sine(x + pi/2), wrapped by `two_pi` (by default the f32 at k +
// 24) above pi.
f32 cosine(u32 k, f32 x, u32 two_pi = 0) {
    const be_f32* c = host<be_f32>(k);
    f32 a = x + c[4];  // fadds
    if (a > c[5]) a = a - global<be_f32>(two_pi != 0 ? two_pi : k + 24);  // fcmpu / bgt, fsubs
    return sine(k, a);
}

// `n` 2 x 2 rotation-scale rows (SlyDecal4, SlyTerrainSnowDecal) as vertex
// constants first..: row i (angle params + angle_off + 4i, scales
// params + scale_off + 8i / + 4; def_word for missing parameters) = {cos
// * sx, -(sin * sy), sin * sx, cos * sy}, sine constants at k.
void upload_rotations(const be_u32* params, u32 def_word, u32 k, u32 angle_off, u32 scale_off, u32 n, u32 first) {
    GuestFrame f(48);
    be_f32* v = f.as<be_f32>();
    for (u32 i = 0; i < n; ++i) {
        const f32 a = as_f32(param_or(params, angle_off + 4 * i, def_word));
        const f32 sx = as_f32(param_or(params, scale_off + 8 * i, def_word));
        const f32 sy = as_f32(param_or(params, scale_off + 8 * i + 4, def_word));
        const f32 s = sine(k, a), c = cosine(k, a);
        be_f32* row = v + 4 * i;
        row[0] = c * sx;     // fmuls
        row[1] = -(s * sy);  // fmuls, fneg
        row[2] = s * sx;
        row[3] = c * sy;
    }
    for (u32 i = 0; i < n; ++i) vconst(first + i, v + 4 * i);
}

}  // namespace

// SlyDecal4: three rotation-scale rows for the decal layers as vertex
// constants 4..6 (angles p+24.., scales p+0..; k0 for missing
// parameters).
void SlyDecal4_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005C49A4;  // f32[7] in .text: 0, then the sine constants
    upload_rotations(params_of(mat), word(k), k, 24, 0, 3, 4);
}

// SlyTerrainSnowDecal: the UV transform when animated (scales k), the two
// packed colours at parameters +0 / +4 as vertex constants 6 and 7 (/
// 255), then two rotation-scale rows (angles p+48.., scales p+32..; 0.0
// for missing parameters) as vertex constants 8 and 9.
void SlyTerrainSnowDecal_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005CFA40;  // f32 in .text: 1.0, 1/255, 0.0, then the sine constants
    if (const u32 anim = anim_of(mat); anim != 0) upload_uv(anim, 4, word(k), word(k));
    upload_colours(params_of(mat), 0, k + 4);
    upload_rotations(params_of(mat), word(k + 8), k + 8, 48, 32, 2, 8);
}

namespace {

// Render state +88 bit `bit` = on, then sent with method 0x1FF8.
void set_1ff8_bit(u32 bit, bool on) {
    RenderState& st = rs();
    GcmContext* c = gcm();
    const u32 w = (st.unk88 & ~(1u << bit)) | (static_cast<u32>(on) << bit);
    st.unk88 = w;
    call(s2::fn::cellGcmSetMethod1FF8, c, w);
}

}  // namespace

// SlyWater: render state +88 bit 0 = parameters +60 set; the UV transform
// when animated (vertex constants 9..10, scales k0); material +64 as
// vertex constant 12; {p+48, p+52, 0, 0} as vertex constant 13;
// TextureWarp's matrices (fragment constants 1, 2..4, 5..7); then {0,
// p+40, p+44, p+56} as fragment constant 9 (0.0 for missing parameters).
void SlyWater_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005D2354;  // f32 pair in .text (1.0, 0.0)
    set_1ff8_bit(0, param_set(params_of(mat), 60));
    if (const u32 anim = anim_of(mat); anim != 0) upload_uv(anim, 9, word(k), word(k));
    vconst(12, at<u8>(mat, 64));
    const u32 k1 = word(k + 4);
    // The original's sp+112..+335: vertex constant 13 (+0), the warp vec4
    // (+16), fragment constant 9 (+32), rows (+48..), matrices (+96, +160).
    GuestFrame f(224);
    be_u32* v13 = f.as<be_u32>(0);
    be_u32* vec = f.as<be_u32>(16);
    be_u32* c9 = f.as<be_u32>(32);
    be_u32* rows = f.as<be_u32>(48);
    be_u32* m1 = f.as<be_u32>(96);
    be_u32* m2 = f.as<be_u32>(160);
    const be_u32* p = params_of(mat);
    v13[0] = param_or(p, 48, k1);
    v13[1] = param_or(p, 52, k1);
    v13[2] = k1;
    v13[3] = k1;
    vconst(13, v13);
    TextureWarp_BuildMatrices(mat, reinterpret_cast<be_f32*>(m1), reinterpret_cast<be_f32*>(m2), vec);
    fconst_set(1, vec);
    send_rows(rows, m1, 2, false);
    send_rows(rows, m2, 5, false);
    p = params_of(mat);
    c9[0] = k1;
    c9[1] = param_or(p, 40, k1);
    c9[2] = param_or(p, 44, k1);
    c9[3] = param_or(p, 56, k1);
    fconst_set(9, c9);
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
void sly_water23_bind(Material* mat, u32 k, u32 splat, void (*waves)(Material*), bool waves_first, u32 scroll, u32 rows_first) {
    const u32 k0 = word(k);
    // The original's sp+112..+367: splat (+0), vertex constant 34 (+16),
    // the warp vec4 (+32), scroll (+48), fragment constant 1 (+64), rows
    // (+80..), matrices (+128, +192).
    GuestFrame f(256);
    be_u32* vs = f.as<be_u32>(0);
    be_u32* v34 = f.as<be_u32>(16);
    be_u32* vec = f.as<be_u32>(32);
    be_u32* vsc = f.as<be_u32>(48);
    be_u32* c1 = f.as<be_u32>(64);
    be_u32* rows = f.as<be_u32>(80);
    be_u32* m1 = f.as<be_u32>(128);
    be_u32* m2 = f.as<be_u32>(192);
    const be_u32* p = params_of(mat);
    const bool on = param_set(p, 60);
    const u32 s = param_or(p, 64, k0);
    for (u32 i = 0; i < 4; ++i) vs[i] = s;
    set_1ff8_bit(0, on);
    vconst(splat, vs);
    upload_uv(anim_of(mat), 13, word(k + 4), word(k + 4));
    vconst(33, at<u8>(mat, 64));
    p = params_of(mat);
    v34[0] = param_or(p, 48, k0);
    v34[1] = param_or(p, 52, k0);
    v34[2] = k0;
    v34[3] = k0;
    vconst(34, v34);
    if (waves_first) waves(mat);
    TextureWarp_BuildMatrices(mat, reinterpret_cast<be_f32*>(m1), reinterpret_cast<be_f32*>(m2), vec);
    if (!waves_first) waves(mat);
    p = params_of(mat);
    vsc[0] = param_or(p, 68, k0);
    vsc[1] = param_or(p, 80, k0);
    vsc[2] = s2::kOne;
    vsc[3] = s2::kOne;
    fconst_set(scroll, vsc);
    fconst_set(0, vec);
    send_rows(rows, m1, rows_first, true);
    send_rows(rows, m2, rows_first + 3, true);
    p = params_of(mat);
    c1[0] = k0;
    c1[1] = param_or(p, 40, k0);
    c1[2] = param_or(p, 44, k0);
    c1[3] = param_or(p, 56, k0);
    fconst_set(1, c1);
}

}  // namespace

void SlyWater2_Bind(ShaderPrograms*, Material* mat) {
    sly_water23_bind(mat, 0x005D33BC, 40, SlyWater2_UploadWaves, true, 4, 41);  // k: f32 pair in .text (0.0, 1.0)
}
void SlyWater3_Bind(ShaderPrograms*, Material* mat) {
    sly_water23_bind(mat, 0x005D4C74, 39, SlyWater3_UploadWaves, false, 5, 40);  // k: f32 pair in .text (0.0, 1.0)
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
void Water_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005B5958;  // .text: f32 1.0, 0.0, 2 pi, pi / 180; f64 pi, -pi; f32 sine constants (from +28)
    constexpr u32 kSine = k + 28;
    const be_f32* kc = host<be_f32>(k);
    const u32 k0 = word(k), zero = word(k + 4);
    MaterialAnim_UploadUVAlt(anim_of(mat), 9, as_f32(k0), as_f32(k0));
    set_1ff8_bit(0, param_set(params_of(mat), 40));
    // The original's sp+112..+367: the warp vec4 (+0), wave vectors (+16,
    // +48), rows (+80..), matrices (+128, +192).
    GuestFrame f(256);
    be_u32* vec = f.as<be_u32>(0);
    be_f32* wa = f.as<be_f32>(16);
    be_f32* wb = f.as<be_f32>(48);
    be_u32* rows = f.as<be_u32>(80);
    be_u32* m1 = f.as<be_u32>(128);
    be_u32* m2 = f.as<be_u32>(192);
    TextureWarp_BuildMatrices(mat, reinterpret_cast<be_f32*>(m1), reinterpret_cast<be_f32*>(m2), vec);
    fconst_set(1, vec);
    send_rows(rows, m1, 2, false);
    send_rows(rows, m2, 5, false);

    const be_u32* p = params_of(mat);
    const f32 fzero = as_f32(zero);
    const bool moving = as_f32(param_or(p, 44, zero)) > fzero || as_f32(param_or(p, 64, zero)) > fzero;  // fcmpu / bgt
    set_1ff8_bit(1, moving);

    const f32 t = global<be_f32>(kAnimTime);
    const f32 two_pi = kc[2], deg = kc[3];
    const f64 pi = at<be_f64>(kc, 16)->get(), neg_pi = at<be_f64>(kc, 24)->get();
    p = params_of(mat);
    for (u32 i = 0; i < 2; ++i) {
        const u32 o = 20 * i;
        be_f32* a = wa + 4 * i;
        be_f32* w = wb + 4 * i;
        be_u32* aw = reinterpret_cast<be_u32*>(a);
        be_u32* ww = reinterpret_cast<be_u32*>(w);
        const auto param = [&](u32 off) { return param_or(p, off + o, zero); };
        a[0] = two_pi / as_f32(param(52));  // fdivs
        aw[1] = param(44);
        a[2] = (as_f32(param(48)) * a[0]) * t;  // fmuls, fmuls
        aw[3] = param(56);
        const f32 ang = as_f32(param(60)) * deg;  // fmuls
        const f32 half = static_cast<f32>(fsel(ang, pi, neg_pi));  // fsel, frsp
        f32 x = half + ang;  // fadds
        const f32 turns = static_cast<f32>(static_cast<f64>(fctidz(x / two_pi)));  // fdivs, fctidz, fcfid, frsp
        x = fnmsubs(turns, two_pi, x);
        const f32 phase = x - half;  // fsubs
        w[0] = sine(kSine, phase);
        ww[1] = zero;
        w[2] = cosine(kSine, phase, k + 8);
        ww[3] = zero;
    }
    call(fn::cellGcmSetVertexProgramConstants, gcm(), 11u, 8u, wa);
    call(fn::cellGcmSetVertexProgramConstants, gcm(), 13u, 8u, wb);
}

// SprParticle: the scene depth texture on unit 1 and the depth range
// constant; then the depth bias (parameters +16, k1 = 0.0 when missing):
// none when it is 0.0; polygon offset off without depth test (render
// state +93) or depth target (+84), or for a depth format (target +24)
// other than 4 / 6 (bias * 65536) and 5 / 7 (bias * 2^24), which also
// sets g::depth_format_warned; else polygon offset on with {0.0, the
// scaled bias}.
void SprParticle_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005B069C;  // f32[4] in .text (1.0, 0.0, 65536.0, 16777216.0)
    const be_f32* kc = host<be_f32>(k);
    RenderState& st = rs();
    bind_depth(1, s2::kTexAddress1, k);
    const u32 k1 = word(k + 4);
    Target* target = st.depth;
    const f32 bias = as_f32(param_or(params_of(mat), 16, k1));
    if (bias == as_f32(k1)) return;  // fcmpu / beq
    bool off = st.dirty_b[1] == 0 || !target;
    f32 scaled = bias;
    if (!off) {
        const s32 format = target->format;
        if (format == 4 || format == 6) {
            scaled = bias * kc[2];  // fmuls
        } else if (format == 5 || format == 7) {
            scaled = bias * kc[3];
        } else {
            u8& warned = global<u8>(s2::g::depth_format_warned);
            if (warned == 0) warned = 1;
            off = true;
        }
    }
    if (off) {
        emit({s2::kPolyOffsetFill, 0});
        return;
    }
    emit({s2::kPolyOffsetFill, 1});
    emit({s2::kPolyOffset2, k1, bits(scaled)});
}

}  // namespace sly4::gfx

NATIVE(005BA7D0, sly4::gfx::SlyCaustics_Bind)
NATIVE(005C38C0, sly4::gfx::SlyDecal_Bind)
NATIVE(005B1E88, sly4::gfx::Diffuse_Bind)
NATIVE(005BD298, sly4::gfx::SlyCharacter2_Bind)
NATIVE(005AE9A0, sly4::gfx::TextureWarp_Bind)
NATIVE(005CE508, sly4::gfx::SlyTerrainSnow_Bind)
NATIVE(005C6D38, sly4::gfx::SlyHeightFog_Bind)
NATIVE(005D1628, sly4::gfx::SlyTextureWarp_Bind)
NATIVE(005AF518, sly4::gfx::ScreenWarp_Bind)
NATIVE(005CD480, sly4::gfx::SlyScreenWarp_Bind)
NATIVE(005C49C0, sly4::gfx::SlyDecal4_Bind)
NATIVE(005CFA68, sly4::gfx::SlyTerrainSnowDecal_Bind)
NATIVE(005D2360, sly4::gfx::SlyWater_Bind)
NATIVE(005D33C8, sly4::gfx::SlyWater2_Bind)
NATIVE(005D4C80, sly4::gfx::SlyWater3_Bind)
NATIVE(005B5990, sly4::gfx::Water_Bind)
NATIVE(005B06B0, sly4::gfx::SprParticle_Bind)
