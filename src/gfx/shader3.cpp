// The helpers the shader Bind / Draw slots call: fog, UV transforms,
// fragment constants, the screen copy and depth texture, the warp
// matrices and the per-material vertex constants.
#include "shader3.h"

#include "draw_state.h"

using namespace sly4;

namespace {

namespace d3 = sly4::render::draw3;
namespace s2 = sly4::render::shader2;
namespace s3 = sly4::render::shader3;
namespace sp = sly4::render::pool;

constexpr u32 kSetVertexConstants = fx::fn::cellGcmSetVertexProgramConstants;

u32 gcm() { return load<u32>(fx::g::state); }

// The material's parameter f32 at +off, or the f32 at k without parameters.
f64 param_f(u32 params, u32 off, u32 k) { return sp::lfs(params != 0 ? params + off : k); }
u32 param_w(u32 params, u32 off, u32 k) { return load<u32>(params != 0 ? params + off : k); }

// fctidz / fcfid / frsp: x truncated toward zero through a 64-bit integer.
f64 trunc_single(f64 x) { return sp::sgl(rt::fcfid(rt::fctid(x, true))); }

}  // namespace

// ---------------------------------------------------------------- fog

// void Draw_SetParticleState(u32 fog, bool enable, u32 fragment_constant)
// Really the view's fog: fog record `fog` of the current View (+128, 64
// bytes each) as vertex constant 32 = {start, 1 / (end - start), 0.0,
// enable && record +0 ? 1.0 : 0.0} and its colour (+2, u8[4]) / 255 as
// fragment constant `fragment_constant`. Notes the index (render state
// +216) and the flag (g::fog_enabled[fog]).
DECOMP(0050CF50, Draw_SetParticleState) {
    constexpr u32 k = 0x0050CF40;  // f32[3] in .text (0.0, 1.0, 1/255)
    const u32 fog = arg<u32>(ctx, 0);
    const u32 enable = arg<u32>(ctx, 1) & 0xFF;  // rlwinm 0,24,31
    const u32 fconst = arg<u32>(ctx, 2);
    const u32 surface = load<u32>(draw::g::current_surface);
    store<u32>(fx::g::state + s3::kStFogIndex, fog);
    const u32 view = load<u32>(surface + s3::kSurfaceView);
    store<u8>(s3::g::fog_enabled + fog, static_cast<u8>(enable));
    const u32 rec = view + ((fog << 6) & 0x3FC0u) + s3::kViewFogs;  // rlwinm 6,18,25
    const bool on = enable != 0 && ref<u8>(rec) != 0;
    const f64 start = sp::lfs(rec + 8), end = sp::lfs(rec + 12);
    Stack frame(ctx, 32);  // the original's sp+112 (vertex constant) and sp+128 (colour)
    const u32 v = frame.addr(), c = v + 16;
    sp::copy_word(v, rec + 8);
    sp::stfs(v + 4, sp::sgl(sp::lfs(k + 4) / sp::sgl(end - start)));  // fsubs, fdivs
    sp::copy_word(v + 8, k);
    sp::copy_word(v + 12, on ? k + 4 : k);
    call(ctx, kSetVertexConstants, {gcm(), 32, 4, v});
    const f64 scale = sp::lfs(k + 8);
    for (u32 i = 0; i < 4; ++i) sp::stfs(c + 4 * i, sp::sgl(static_cast<f64>(ref<u8>(rec + 2 + i)) * scale));  // fcfid, frsp, fmuls
    call(ctx, d3::fn::FProgram_SetConstant, {fconst, c});
}

// void DrawItem_SetParticleState(DrawItem* item, u32 fragment_constant)
// The view fog with index item +68 bit 6 (rlwinm 26,31,31), enabled by
// the material's +100 bit 6.
DECOMP(0050D0E0, DrawItem_SetParticleState) {
    const u32 item = arg<u32>(ctx, 0), fconst = arg<u32>(ctx, 1);
    const u32 fog = (ref<u8>(item + 68) >> 6) & 1;
    const u32 enable = (ref<u8>(load<u32>(item + d3::kItemMaterial) + d3::kMatFlags) >> 6) & 1;  // cntlzw / xori
    call(ctx, d3::fn::Draw_SetParticleState, {fog, enable, fconst});
}

// ---------------------------------------------------------------- UV transforms

// void MaterialAnim_UploadUV(MaterialAnim* anim, u32 first, f32 su, f32 sv)
// Two vertex constants from `first`: the animation's UV rows
// (MaterialAnim_GetUV(anim; su, sv)), or {su, 0, 0, 0}, {0, sv, 0, 0}
// without an animation block.
DECOMP(0050D124, MaterialAnim_UploadUV) {
    const u32 anim = arg<u32>(ctx, 0), first = arg<u32>(ctx, 1);
    const f64 su = farg(ctx, 0), sv = farg(ctx, 1);
    if (anim != 0) {
        callf(ctx, s3::fn::MaterialAnim_GetUV, {anim}, {su, sv});
        const u32 rows = static_cast<u32>(ctx.gpr[3]);
        call(ctx, kSetVertexConstants, {gcm(), first, 8, rows});
        return;
    }
    Stack frame(ctx, 32);
    const u32 v = frame.addr();
    for (u32 i = 0; i < 32; i += 4) store<u32>(v + i, 0);
    sp::stfs(v, su);
    sp::stfs(v + 20, sv);
    call(ctx, kSetVertexConstants, {gcm(), first, 8, v});
}

// void MaterialAnim_UploadUVAlt(MaterialAnim* anim, u32 first, f32 su, f32 sv)
// One vertex constant: the animation's vec4 (MaterialAnim_GetUVAlt) times
// {su, sv, k, k}, or {su, sv, 1.0, 1.0} without an animation block.
DECOMP(0050D1E0, MaterialAnim_UploadUVAlt) {
    constexpr u32 k = 0x0050D1D8;  // f32 in .text (1.0)
    const u32 anim = arg<u32>(ctx, 0), first = arg<u32>(ctx, 1);
    const f64 su = farg(ctx, 0), sv = farg(ctx, 1);
    const f64 kk = sp::lfs(k);
    // The original's sp+112 (the animation's vec4) and sp+128 (the default).
    Stack frame(ctx, 32);
    const u32 v = frame.addr();
    if (anim != 0) {
        call(ctx, s3::fn::MaterialAnim_GetUVAlt, {anim, v});
        const f64 x = sp::lfs(v), y = sp::lfs(v + 4), z = sp::lfs(v + 8), w = sp::lfs(v + 12);
        sp::stfs(v, sp::sgl(x * su));  // fmuls
        sp::stfs(v + 4, sp::sgl(y * sv));
        sp::stfs(v + 8, sp::sgl(z * kk));
        sp::stfs(v + 12, sp::sgl(w * kk));
        call(ctx, kSetVertexConstants, {gcm(), first, 4, v});
        return;
    }
    const u32 d = v + 16;
    sp::stfs(d, su);
    sp::stfs(d + 4, sv);
    store<u32>(d + 8, s2::kOne);
    store<u32>(d + 12, s2::kOne);
    call(ctx, kSetVertexConstants, {gcm(), first, 4, d});
}

// ---------------------------------------------------------------- fragment constants

// u64 FProgram_SetConstant(u32 index, const f32 v[4])
// Into the current fragment program (render state +28): nothing when the
// program doesn't use constant `index` (record +592 + 16 * index, +0
// zero). Else the 16 bytes go to the record's cached slot (+12; at +48 +
// 16 * slot) and the slot's bit is set in the dirty mask (+16) when they
// changed or the program's stamp (+24) isn't render state +156. Returns
// the dirty mask (render state when unused).
DECOMP(0052F270, FProgram_SetConstant) {
    const u32 index = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 st = fx::g::state;
    const u32 fp = load<u32>(st + s3::kStFragmentProgram);
    const u32 rec = fp + (index << 4) + s3::kFpConstants;
    if (load<u32>(rec) == 0) {
        ctx.gpr[3] = st;
        return;
    }
    const u32 slot = load<u32>(rec + 12);
    const u64 n0 = load<u64>(v), n1 = load<u64>(v + 8);
    const u32 cache = fp + (slot << 4) + s3::kFpCache;  // rlwinm 4,0,27
    const u64 o0 = load<u64>(cache), o1 = load<u64>(cache + 8);
    store<u64>(cache, n0);
    store<u64>(cache + 8, n1);
    const bool stale = load<u32>(fp + s3::kFpStamp) != load<u32>(st + s3::kStProgramStamp);
    const bool changed = ((o0 ^ n0) | (o1 ^ n1)) != 0;
    const u64 bit = (slot & 64) != 0 ? 0 : 1ull << (slot & 63);  // sld: shift counts 64..127 give 0
    const u64 dirty = load<u64>(fp + s3::kFpDirty) | ((changed || stale) ? bit : 0);
    store<u64>(fp + s3::kFpDirty, dirty);
    ctx.gpr[3] = dirty;
}

// ---------------------------------------------------------------- textures

// Texture* Draw_GetScreenCopy()
// The screen copy (Draw_CopyScreen), made at most once per g::config +76
// frame stamp.
DECOMP(00512940, Draw_GetScreenCopy) {
    namespace l4 = render::layers4;
    const u32 stamp = load<u32>(sly4::g::config + 76);
    if (load<u32>(l4::g::copy_stamp) == stamp) {
        ctx.gpr[4] = stamp;  // left in r4
        ret(ctx, load<u32>(l4::g::copy_texture));
        return;
    }
    store<u32>(l4::g::copy_stamp, stamp);
    // Draw_CopyScreen takes no arguments; r3 still holds the stamp.
    const u32 tex = static_cast<u32>(call(ctx, l4::fn::Draw_CopyScreen, {stamp}));
    store<u32>(l4::g::copy_texture, tex);
    ctx.gpr[4] = l4::g::copy_texture & 0xFFFF0000u;  // left in r4
    ret(ctx, tex);
}

// Texture* Display_GetDepthTexture(MemStats* s)
// The scene depth texture: s +132 for the alternate buffer set (display
// +148 bit 7), else s +104 / +108 by s +184 with g::config +34 set, else
// s +88.
DECOMP(00537BB8, Display_GetDepthTexture) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 alt = (ref<u8>(load<u32>(draw::g::display) + 148) >> 7) & 1;  // rlwinm 25,31,31
    if (alt != 0) {
        ctx.gpr[4] = alt;  // left in r4
        ret(ctx, load<u32>(s + 132));
        return;
    }
    const u32 split = ref<u8>(sly4::g::config + 34);
    if (split == 0) {
        ctx.gpr[4] = 0;  // left in r4
        ret(ctx, load<u32>(s + 88));
        return;
    }
    const u32 eye = load<u32>(sly4::g::mem_stats + 184);
    ctx.gpr[4] = eye;  // left in r4
    ret(ctx, load<u32>(s + (static_cast<s32>(eye) == 1 ? 104 : 108)));
}

// ---------------------------------------------------------------- warp matrices

namespace {

// One warp matrix: Matrix_FromAxisAngle(m, g::rotation_axis, angle),
// then the scroll (su, sv, rotated by the matrix and offset by its row
// 3) is folded in: the rows are scaled by `scale`, and row 2 becomes
// {frac(x), frac(y), z} of the scrolled point.
void warp_matrix(PPUContext& ctx, u32 m, u32 tmp, u32 scale, f64 su, f64 sv, f64 angle) {
    callf(ctx, s3::fn::Matrix_FromAxisAngle, {m, s3::g::rotation_axis}, {angle});
    const u32 a = m & ~15u;  // lvx
    f32 r[4];
    for (u32 i = 0; i < 4; ++i) {
        f32 x = std::fmaf(load<f32>(a + 4 * i), static_cast<f32>(su), 0.0f);  // vmaddfp
        x = std::fmaf(load<f32>(a + 16 + 4 * i), static_cast<f32>(sv), x);
        x = std::fmaf(load<f32>(a + 32 + 4 * i), 0.0f, x);
        r[i] = load<f32>(a + 48 + 4 * i) + x;  // vaddfp
    }
    for (u32 i = 0; i < 4; ++i) store<f32>(tmp + 4 * i, r[i]);  // stvx
    const f64 x = sp::lfs(tmp), y = sp::lfs(tmp + 4);
    const f64 tx = trunc_single(x), ty = trunc_single(y);
    call(ctx, s3::fn::Matrix_ScaleRows, {m, scale});
    sp::stfs(m + 32, sp::sgl(x - tx));  // fsubs
    sp::stfs(m + 36, sp::sgl(y - ty));
    sp::copy_word(m + 40, tmp + 8);
}

// TextureWarp_BuildMatrices' body (k: the default f32 for missing
// parameters, 0.0). Parameters (material +124): +0 / +4 scroll speed and
// +32 angle of matrix 1, +12 / +16 and +36 of matrix 2, +24 / +28 the
// scale of both, +8 / +20 the vec4's x / y.
void build_warp_matrices(PPUContext& ctx, u32 k) {
    const u32 mat = arg<u32>(ctx, 0), m1 = arg<u32>(ctx, 1), m2 = arg<u32>(ctx, 2), v = arg<u32>(ctx, 3);
    const f64 t = sp::lfs(s3::g::material_time);
    // The original's sp+112 (scroll point) and sp+128 (scale).
    Stack frame(ctx, 32);
    const u32 tmp = frame.addr(), scale = tmp + 16;
    u32 p = load<u32>(mat + d3::kMatParams);
    store<u32>(scale + 4, param_w(p, 28, k));
    store<u32>(scale, param_w(p, 24, k));
    sp::copy_word(scale + 8, k);
    f64 su = sp::sgl(t * param_f(p, 0, k));  // fmuls
    f64 sv = sp::sgl(t * param_f(p, 4, k));
    warp_matrix(ctx, m1, tmp, scale, su, sv, param_f(p, 32, k));
    p = load<u32>(mat + d3::kMatParams);
    su = sp::sgl(t * param_f(p, 12, k));
    sv = sp::sgl(t * param_f(p, 16, k));
    warp_matrix(ctx, m2, tmp, scale, su, sv, param_f(p, 36, k));
    store<u32>(v, param_w(load<u32>(mat + d3::kMatParams), 8, k));
    store<u32>(v + 4, param_w(load<u32>(mat + d3::kMatParams), 20, k));
    sp::copy_word(v + 8, k);
    sp::copy_word(v + 12, k);
    ctx.gpr[3] = v + 4;  // left in r3..r5 (the last stores' addresses)
    ctx.gpr[4] = v + 8;
    ctx.gpr[5] = v + 12;
}

}  // namespace

// void TextureWarp_BuildMatrices(Material* mat, f32 m1[16], f32 m2[16], f32 v[4])
DECOMP(005AE648, TextureWarp_BuildMatrices) { build_warp_matrices(ctx, 0x005AE640); }
// void SlyTextureWarp_BuildMatrices(Material* mat, f32 m1[16], f32 m2[16], f32 v[4]): the same code.
DECOMP(005D12C8, SlyTextureWarp_BuildMatrices) { build_warp_matrices(ctx, 0x005D12C4); }

// ---------------------------------------------------------------- per-material vertex constants

// void SlyHeightFog_UploadFog(Material* mat)
// Parameters (+124; k0 for each when missing): vertex constant 11 = {k2
// / p16, p12, k2 / p16 * p20 * t, k0}, 12 = {k2 / p40, p36, k2 / p40 *
// p44 * t, k0} (t: g::material_time), 13 = normalize(p0, k0, p4), 14 =
// normalize(p24, k0, p28) (each with w 1.0; k1 / length).
DECOMP(005C6B20, SlyHeightFog_UploadFog) {
    constexpr u32 k = 0x005C6B14;  // f32[3] in .text
    const u32 p = load<u32>(arg<u32>(ctx, 0) + d3::kMatParams);
    const f64 k0 = sp::lfs(k), k1 = sp::lfs(k + 4), k2 = sp::lfs(k + 8);
    const f64 a0 = param_f(p, 0, k), a2 = param_f(p, 4, k);
    const f64 kk = sp::sgl(k0 * k0);  // fmuls
    const f64 ia = sp::sgl(k1 / sp::sgl(std::sqrt(sp::fmadds(a2, a2, sp::fmadds(a0, a0, kk)))));  // fsqrts, fdivs
    const f64 b0 = param_f(p, 24, k), b2 = param_f(p, 28, k);
    const f64 ib = sp::sgl(k1 / sp::sgl(std::sqrt(sp::fmadds(b2, b2, sp::fmadds(b0, b0, kk)))));
    const f64 t = sp::lfs(s3::g::material_time);
    // The original's sp+112 (constant 11), +128 (12), +144 (14), +160 (13).
    Stack frame(ctx, 64);
    const u32 v11 = frame.addr(), v12 = v11 + 16, v14 = v11 + 32, v13 = v11 + 48;
    const f64 x11 = sp::sgl(k2 / param_f(p, 16, k));  // fdivs
    store<u32>(v11 + 4, param_w(p, 12, k));
    sp::copy_word(v11 + 12, k);
    store<u32>(v12 + 4, param_w(p, 36, k));
    sp::copy_word(v12 + 12, k);
    const f64 x12 = sp::sgl(k2 / param_f(p, 40, k));
    sp::stfs(v11, x11);
    sp::stfs(v12, x12);
    sp::stfs(v11 + 8, sp::sgl(sp::sgl(x11 * param_f(p, 20, k)) * t));  // fmuls, fmuls
    sp::stfs(v12 + 8, sp::sgl(sp::sgl(x12 * param_f(p, 44, k)) * t));
    call(ctx, kSetVertexConstants, {gcm(), 11, 4, v11});
    call(ctx, kSetVertexConstants, {gcm(), 12, 4, v12});
    store<u32>(v13 + 12, s2::kOne);
    sp::stfs(v13, sp::sgl(a0 * ia));
    sp::stfs(v13 + 4, sp::sgl(k0 * ia));
    sp::stfs(v13 + 8, sp::sgl(a2 * ia));
    call(ctx, kSetVertexConstants, {gcm(), 13, 4, v13});
    store<u32>(v14 + 12, s2::kOne);
    sp::stfs(v14, sp::sgl(b0 * ib));
    sp::stfs(v14 + 4, sp::sgl(k0 * ib));
    sp::stfs(v14 + 8, sp::sgl(b2 * ib));
    call(ctx, kSetVertexConstants, {gcm(), 14, 4, v14});
}

namespace {

// (x, k0, z) normalized unless its length is below k1 (fcmpu / blt: a NaN
// length divides).
void normalize_xz(f64& x, f64& y, f64& z, f64 kk, f64 k1) {
    const f64 len = sp::sgl(std::sqrt(sp::fmadds(z, z, sp::fmadds(x, x, kk))));  // fsqrts
    if (std::fabs(len) < k1) return;
    x = sp::sgl(x / len);
    y = sp::sgl(y / len);
    z = sp::sgl(z / len);
}

// SlyWater2 / SlyWater3 wave constants (k: f32[3] in .text; parameters
// +base.. a 32-byte wave block; first: the first vertex constant).
// Parameters (material +124; k0 for each when missing): constant first
// = {k2 / w+4, w+0, k2 / w+4 * w+8 * t, w+24}, first + 1 = {k2 / w+16,
// w+12, k2 / w+16 * w+20 * t, w+28} (t: g::material_time, w: parameters
// + base); first + 2 / + 3 = the directions (p0, k0, p4) and (p12, k0,
// p16), normalized when not below k1, with w 1.0.
void upload_waves(PPUContext& ctx, u32 k, u32 base, u32 first) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + d3::kMatParams);
    const f64 k0 = sp::lfs(k), k1 = sp::lfs(k + 4), k2 = sp::lfs(k + 8);
    const f64 kk = sp::sgl(k0 * k0);  // fmuls
    f64 a0 = param_f(p, 0, k), a1 = k0, a2 = param_f(p, 4, k);
    normalize_xz(a0, a1, a2, kk, k1);
    f64 b0 = param_f(p, 12, k), b1 = k0, b2 = param_f(p, 16, k);
    normalize_xz(b0, b1, b2, kk, k1);
    const f64 t = sp::lfs(s3::g::material_time);
    const f64 x0 = sp::sgl(k2 / param_f(p, base + 4, k));  // fdivs
    const f64 x1 = sp::sgl(k2 / param_f(p, base + 16, k));
    // The original's sp+112 (first), +128 (first + 1), +144 (first + 3), +160 (first + 2).
    Stack frame(ctx, 64);
    const u32 v0 = frame.addr(), v1 = v0 + 16, vb = v0 + 32, va = v0 + 48;
    sp::stfs(v0, x0);
    store<u32>(v0 + 4, param_w(p, base, k));
    sp::stfs(v0 + 8, sp::sgl(sp::sgl(x0 * param_f(p, base + 8, k)) * t));  // fmuls, fmuls
    store<u32>(v0 + 12, param_w(p, base + 24, k));
    sp::stfs(v1, x1);
    store<u32>(v1 + 4, param_w(p, base + 12, k));
    sp::stfs(v1 + 8, sp::sgl(sp::sgl(x1 * param_f(p, base + 20, k)) * t));
    store<u32>(v1 + 12, param_w(p, base + 28, k));
    call(ctx, kSetVertexConstants, {gcm(), first, 4, v0});
    call(ctx, kSetVertexConstants, {gcm(), first + 1, 4, v1});
    sp::stfs(va, a0);
    sp::stfs(va + 4, a1);
    sp::stfs(va + 8, a2);
    store<u32>(va + 12, s2::kOne);
    call(ctx, kSetVertexConstants, {gcm(), first + 2, 4, va});
    sp::stfs(vb, b0);
    sp::stfs(vb + 4, b1);
    sp::stfs(vb + 8, b2);
    store<u32>(vb + 12, s2::kOne);
    call(ctx, kSetVertexConstants, {gcm(), first + 3, 4, vb});
}

}  // namespace

// void SlyWater2_UploadWaves(Material* mat): vertex constants 36..39, waves at parameters +100.
DECOMP(005D3180, SlyWater2_UploadWaves) { upload_waves(ctx, 0x005D3174, 100, 36); }
// void SlyWater3_UploadWaves(Material* mat): vertex constants 35..38, waves at parameters +108.
DECOMP(005D4A38, SlyWater3_UploadWaves) { upload_waves(ctx, 0x005D4A28, 108, 35); }

// ---------------------------------------------------------------- draw items

// DrawItem* DrawItem_Create(Mesh* mesh, Material* mat, const ItemDesc* d)
// An 80-byte DrawItem from the frame arena (kArena +20), unzeroed: +0..
// the 4 x 3 matrix rows of d (+0 / +16 / +32 / +48, w not copied), +12
// mesh, +28 mat, +44 d +92, +60 d +80, +64 the item data (d +84, d +89
// bytes; copied into the arena, 16-byte padded, unless it already lies
// inside the arena's buffer), +68 flags (bit 0 kept as the arena held
// it; bits 1..4 the data size's low nibble; bits 5..7 from d +71), +69 0,
// +70 d +88, +71 d +69, +72 u8[4] d +72.
DECOMP(0079D6F8, DrawItem_Create) {
    const u32 mesh = arg<u32>(ctx, 0), mat = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    const u32 a = s3::kArena;
    const u32 item = load<u32>(a + s3::kArenaCursor);
    store<u32>(a + s3::kArenaCursor, item + s3::kDrawItemSize);
    store<u32>(item + d3::kItemMesh, mesh);
    store<u32>(item + d3::kItemMaterial, mat);
    for (u32 row = 0; row < 64; row += 16) {
        for (u32 i = 0; i < 12; i += 4) sp::copy_word(item + row + i, d + row + i);  // lfs / stfs
    }
    for (u32 i = 0; i < 4; ++i) store<u8>(item + 72 + i, ref<u8>(d + 72 + i));
    u32 flags = ref<u8>(item + 68) & ~0x1Eu;  // rlwimi 0,27,30 with 0
    store<u32>(item + d3::kItemLayers, load<u32>(d + 92));
    store<u32>(item + 64, 0);
    store<u8>(item + d3::kItemVariant, 0);
    store<u8>(item + 68, static_cast<u8>(flags));
    for (const u32 bit : {0x80u, 0x40u, 0x20u}) {  // rlwimi 0,24,24 / 25,25 / 26,26, d +71 reloaded each time
        flags = (flags & ~bit) | (ref<u8>(d + 71) & bit);
        store<u8>(item + 68, static_cast<u8>(flags));
    }
    store<u8>(item + 71, ref<u8>(d + 69));
    store<u32>(item + 60, load<u32>(d + 80));
    store<u8>(item + 70, ref<u8>(d + 88));
    const u32 data = load<u32>(d + 84);
    const u32 cursor = load<u32>(a + s3::kArenaCursor);
    ret(ctx, item);
    if (data == 0) return;
    const u32 size = ref<u8>(d + 89);
    if (data >= load<u32>(a + s3::kArenaLo) && data < load<u32>(a + s3::kArenaHi)) {  // cmplw
        store<u32>(item + 64, data);
        store<u8>(item + 68, static_cast<u8>((flags & ~0x1Eu) | ((size << 1) & 0x1Eu)));  // rlwimi 1,27,30
        return;
    }
    store<u32>(a + s3::kArenaCursor, cursor + ((size + 15) & ~15u));
    const u32 f = ref<u8>(item + 68);
    store<u32>(item + 64, cursor);
    store<u8>(item + 68, static_cast<u8>((f & ~0x1Eu) | ((size << 1) & 0x1Eu)));
    call(ctx, s3::fn::memmove, {cursor, data, size});
    ret(ctx, item);
}

// ---------------------------------------------------------------- skinning

// void Skin_BuildJob(SkinJob* job, const SkinDesc* desc, const SkinPart* part, u32 bones, u32 flags)
// Fills the 256-byte SPU job record for one skinned part (descriptor:
// draw3.h) and reserves the part's command space in the gcm context
// (desc +0): part +1 u8 n lines of 16 bytes at a 16-byte boundary (NOP
// padded), each 128-byte line starting with a jump to itself so the RSX
// waits there until the SPU job overwrites it. Gives up after the
// buffer's callback fails. The job record: +0 u64 desc +16 (SPU image),
// +8 u16 desc +20 (its size), +10 u16 120, +12 0, +16 1, +20 part +60,
// +24 0, +28 u16 0, +30 u16 part +64, +32 / +36 0; +48.. a DMA list of
// {u32 size, u32 address} pairs (the part's streams, its bone matrices
// at bones + part +44 / +46, desc +4 / +8 and the part record); +176
// desc +12, +180 the reserved command space, +184 the mode (1 with
// g::skin_mode_override or no part +68, 2 for part +68 == 2304, else 3)
// | flags.
DECOMP(00537824, Skin_BuildJob) {
    const u32 job = arg<u32>(ctx, 0), desc = arg<u32>(ctx, 1), part = arg<u32>(ctx, 2);
    const u32 bones = arg<u32>(ctx, 3), flags = arg<u32>(ctx, 4);
    const auto h = [part](u32 off) -> u32 { return load<u16>(part + off); };
    const auto w = [part](u32 off) -> u32 { return load<u32>(part + off); };
    const u32 p16 = w(16), p20 = h(20);
    store<u32>(job + 52, p16);
    store<u32>(job + 48, p20);
    store<u32>(job + 60, p16 + p20);
    store<u32>(job + 56, h(22));
    store<u32>(job + 68, h(44) + bones);
    store<u32>(job + 64, h(48));
    store<u32>(job + 76, h(46) + bones);
    store<u32>(job + 72, h(50));
    const u32 p56 = w(56), p52 = h(52);
    store<u32>(job + 84, p56);
    store<u32>(job + 80, p52);
    store<u32>(job + 92, p56 + p52);
    store<u32>(job + 88, h(54));
    const u32 p24 = w(24), p32 = h(32), p34 = h(34);
    store<u32>(job + 100, p24);
    store<u32>(job + 96, p32);
    store<u32>(job + 108, p24 + p32);
    store<u32>(job + 104, p34);
    store<u32>(job + 116, p24 + p32 + p34);
    store<u32>(job + 112, h(36));
    const u32 p28 = w(28), p38 = h(38), p40 = h(40);
    store<u32>(job + 124, p28);
    store<u32>(job + 120, p38);
    store<u32>(job + 132, p28 + p38);
    store<u32>(job + 128, p40);
    store<u32>(job + 140, p28 + p38 + p40);
    store<u32>(job + 136, h(42));
    store<u32>(job + 148, load<u32>(desc + 4));
    store<u32>(job + 144, 128);
    store<u32>(job + 156, load<u32>(desc + 8));
    store<u32>(job + 152, 48);
    store<u32>(job + 164, part);
    store<u32>(job + 160, 16);
    store<u32>(job + 176, load<u32>(desc + 12));

    const u32 n = ref<u8>(part + 1);
    const u32 c = load<u32>(desc);
    const u32 words = n * 4 + 3;
    call(ctx, s3::fn::cellGcmReserveMethodSize, {c, words});
    if (load<u32>(c + s3::kGcmCurrent) + words * 4 > load<u32>(c + s3::kGcmEnd)) {  // cmplw / ble
        const u32 opd = load<u32>(c + s3::kGcmCallback);
        if (static_cast<u32>(call_ptr(ctx, opd, {c, words})) != 0) return;  // cmpwi (r3 = the callback's result)
    }
    u32 cur = load<u32>(c + s3::kGcmCurrent);
    while ((cur & 15) != 0) {  // NOP padding to 16 bytes
        store<u32>(c + s3::kGcmCurrent, cur + 4);
        store<u32>(cur, 0);
        cur = load<u32>(c + s3::kGcmCurrent);
    }
    const u32 space = cur, end = space + n * 16;
    Stack frame(ctx, 16);  // the original's sp+112: the offset out-parameter
    const u32 off = frame.addr();
    call(ctx, s3::fn::cellGcmAddressToOffset, {space, off});
    u32 at = load<u32>(c + s3::kGcmCurrent);
    store<u32>(at, load<u32>(off) | s3::kJumpFlag);
    u32 next = at + 4;
    store<u32>(c + s3::kGcmCurrent, next);
    for (u32 line = (at + 132) & ~127u; end > line; line = (at + 132) & ~127u) {  // cmplw / bgt
        rt::dcbz(line);
        store<u32>(c + s3::kGcmCurrent, line);
        call(ctx, s3::fn::cellGcmAddressToOffset, {line, off});
        at = load<u32>(c + s3::kGcmCurrent);
        store<u32>(at, load<u32>(off) | s3::kJumpFlag);
        next = at + 4;
        store<u32>(c + s3::kGcmCurrent, next);
    }
    store<u32>(c + s3::kGcmCurrent, n != 0 ? end : next);
    store<u32>(job + 180, space);
    u32 mode = 1;
    if (ref<u8>(s3::g::skin_mode_override) == 0) {
        const u32 kind = w(68);
        if (kind != 0) mode = kind == 2304 ? 2 : 3;
    }
    const u32 image = load<u32>(desc + 16);
    store<u32>(job + 184, mode | flags);
    store<u16>(job + 10, 120);
    store<u16>(job + 28, 0);
    store<u64>(job, image);
    const u32 image_size = load<u32>(desc + 20);
    store<u32>(job + 12, 0);
    store<u16>(job + 8, static_cast<u16>(image_size));
    store<u32>(job + 16, 1);
    const u32 p60 = w(60);
    store<u32>(job + 24, 0);
    store<u32>(job + 20, p60);
    const u32 p64 = w(64);
    store<u32>(job + 32, 0);
    store<u16>(job + 30, static_cast<u16>(p64));
    store<u32>(job + 36, 0);
    ctx.gpr[3] = p64;  // left in r3
}
