// The helpers the shader Bind / Draw slots call: fog, UV transforms,
// fragment constants, the screen copy and depth texture, the warp
// matrices, the per-material vertex constants, DrawItem_Create and the
// skinning job set-up.
#include "shader3.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

namespace s2 = sly4::render::shader2;
namespace s3 = sly4::render::shader3;
namespace l4g = sly4::render::layers4::g;
constexpr u32 kCurrentSurface = 0x010DDA74;  // draw::g::current_surface

const be_u32* params_of(const Material* mat) { return host<be_u32>(or_zero_page(mat)->w124); }
// The material's parameter f32 at +off, or the f32 at k without parameters.
f32 param_f(const be_u32* p, u32 off, u32 k) { return as_f32(p ? at<be_u32>(p, off)->get() : global<be_u32>(k).get()); }
u32 param_w(const be_u32* p, u32 off, u32 k) { return p ? at<be_u32>(p, off)->get() : global<be_u32>(k).get(); }
void vconsts(u32 first, u32 count, const void* v) { call(fn::cellGcmSetVertexProgramConstants, gcm(), first, count, v); }

// fctidz / fcfid / frsp: x truncated toward zero through a 64-bit integer.
f32 trunc_single(f32 x) { return static_cast<f32>(static_cast<f64>(fctidz(x))); }

}  // namespace

// ---------------------------------------------------------------- fog

// void Draw_SetParticleState(u32 fog, bool enable, u32 fragment_constant)
// Really the view's fog: fog record `fog` of the current View (+128, 64
// bytes each) as vertex constant 32 = {start, 1 / (end - start), 0.0,
// enable && record +0 ? 1.0 : 0.0} and its colour (+2, u8[4]) / 255 as
// fragment constant `fragment_constant`. Notes the index (render state
// +216) and the flag (g::fog_enabled[fog]).
void Draw_SetParticleState(u32 fog, u8 enable, u32 fconst) {
    constexpr u32 k = 0x0050CF40;  // f32[3] in .text (0.0, 1.0, 1/255)
    const be_f32* kc = host<be_f32>(k);
    u8* surface = global<gp<u8>>(kCurrentSurface);
    rs().unk216 = fog;
    u8* view = *at<gp<u8>>(surface, s3::kSurfaceView);
    global<u8>(s3::g::fog_enabled + fog) = enable;
    const u8* rec = at<u8>(view, ((fog << 6) & 0x3FC0u) + s3::kViewFogs);  // rlwinm 6,18,25
    const bool on = enable != 0 && rec[0] != 0;
    const f32 start = *at<be_f32>(rec, 8), end = *at<be_f32>(rec, 12);
    GuestFrame f(32);  // the original's sp+112 (vertex constant) and sp+128 (colour)
    be_f32* v = f.as<be_f32>(0);
    be_f32* c = f.as<be_f32>(16);
    copy_word(v, rec + 8);
    v[1] = kc[1] / (end - start);  // fsubs, fdivs
    copy_word(v + 2, kc);
    copy_word(v + 3, on ? kc + 1 : kc);
    vconsts(32, 4, v);
    const f32 scale = kc[2];
    for (u32 i = 0; i < 4; ++i) c[i] = static_cast<f32>(rec[2 + i]) * scale;  // fcfid, frsp, fmuls
    FProgram_SetConstant(fconst, reinterpret_cast<const be_u32*>(c));
}

// void DrawItem_SetParticleState(DrawItem* item, u32 fragment_constant)
// The view fog with index item +68 bit 6 (rlwinm 26,31,31), enabled by
// the material's +100 bit 6.
void DrawItem_SetParticleState(DrawItem* item, u32 fconst) {
    const u32 fog = (item->flags68 >> 6) & 1;
    const u32 enable = (or_zero_page(item->mat.get())->b100 >> 6) & 1;  // cntlzw / xori
    Draw_SetParticleState(fog, static_cast<u8>(enable), fconst);
}

// ---------------------------------------------------------------- UV transforms

// void MaterialAnim_UploadUV(MaterialAnim* anim, u32 first, f32 su, f32 sv)
// Two vertex constants from `first`: the animation's UV rows
// (MaterialAnim_GetUV(anim; su, sv)), or {su, 0, 0, 0}, {0, sv, 0, 0}
// without an animation block.
void MaterialAnim_UploadUV(u32 anim, u32 first, f32 su, f32 sv) {
    if (anim != 0) {
        const u32 rows = call<u32>(s3::fn::MaterialAnim_GetUV, anim, su, sv);
        vconsts(first, 8, host<u8>(rows));
        return;
    }
    GuestFrame f(32);
    be_f32* v = f.as<be_f32>();
    for (u32 i = 0; i < 8; ++i) *reinterpret_cast<be_u32*>(v + i) = 0u;
    v[0] = su;
    v[5] = sv;
    vconsts(first, 8, v);
}

// void MaterialAnim_UploadUVAlt(MaterialAnim* anim, u32 first, f32 su, f32 sv)
// One vertex constant: the animation's vec4 (MaterialAnim_GetUVAlt) times
// {su, sv, k, k}, or {su, sv, 1.0, 1.0} without an animation block.
void MaterialAnim_UploadUVAlt(u32 anim, u32 first, f32 su, f32 sv) {
    constexpr u32 k = 0x0050D1D8;  // f32 in .text (1.0)
    const f32 kk = global<be_f32>(k);
    // The original's sp+112 (the animation's vec4) and sp+128 (the default).
    GuestFrame f(32);
    be_f32* v = f.as<be_f32>(0);
    if (anim != 0) {
        call(s3::fn::MaterialAnim_GetUVAlt, anim, v);
        const f32 x = v[0], y = v[1], z = v[2], w = v[3];
        v[0] = x * su;  // fmuls
        v[1] = y * sv;
        v[2] = z * kk;
        v[3] = w * kk;
        vconsts(first, 4, v);
        return;
    }
    be_f32* d = f.as<be_f32>(16);
    d[0] = su;
    d[1] = sv;
    *reinterpret_cast<be_u32*>(d + 2) = s2::kOne;
    *reinterpret_cast<be_u32*>(d + 3) = s2::kOne;
    vconsts(first, 4, d);
}

// ---------------------------------------------------------------- fragment constants

// void FProgram_SetConstant(u32 index, const f32 v[4])
// Into the current fragment program (render state +28): nothing when the
// program doesn't use constant `index` (record +592 + 16 * index, +0
// zero). Else the 16 bytes go to the record's cached slot (+12; at +48 +
// 16 * slot) and the slot's bit is set in the dirty mask (+16) when they
// changed or the program's stamp (+24) isn't render state +156.
void FProgram_SetConstant(u32 index, const be_u32* v) {
    RenderState& st = rs();
    FProgram* fp = st.fprogram;
    const FParam* rec = at<FParam>(fp, (index << 4) + s3::kFpConstants);
    if (rec->param == 0) return;
    const u32 slot = rec->index;
    const u64 n0 = reinterpret_cast<const be_u64*>(v)[0], n1 = reinterpret_cast<const be_u64*>(v)[1];
    be_u64* cache = at<be_u64>(fp, (slot << 4) + s3::kFpCache);  // rlwinm 4,0,27
    const u64 o0 = cache[0], o1 = cache[1];
    cache[0] = n0;
    cache[1] = n1;
    FProgram* f = or_zero_page(fp);
    const bool stale = f->stamp != st.stamp;
    const bool changed = ((o0 ^ n0) | (o1 ^ n1)) != 0;
    const u64 bit = (slot & 64) != 0 ? 0 : 1ull << (slot & 63);  // sld: shift counts 64..127 give 0
    f->dirty = f->dirty | ((changed || stale) ? bit : 0);
}

// ---------------------------------------------------------------- textures

// Texture* Draw_GetScreenCopy()
// The screen copy (Draw_CopyScreen), made at most once per g::config +76
// frame stamp.
Texture* Draw_GetScreenCopy() {
    const u32 stamp = config().layer_count;
    be_u32& copied = global<be_u32>(l4g::copy_stamp);
    gp<Texture>& copy = global<gp<Texture>>(l4g::copy_texture);
    if (copied == stamp) return copy;
    copied = stamp;
    Texture* tex = Draw_CopyScreen();
    copy = tex;
    return tex;
}

// Texture* Display_GetDepthTexture(MemStats* s)
// The scene depth texture: s +132 for the alternate buffer set (display
// +148 bit 7), else s +104 / +108 by g::mem_stats +184 with g::config +34
// set, else s +88.
Texture* Display_GetDepthTexture(MemStats* s) {
    if (alternate(or_zero_page(display()))) return *at<gp<Texture>>(s, 132);
    if (config().stereo == 0) return *at<gp<Texture>>(s, 88);
    const s32 eye = static_cast<s32>(mem_stats().buffer_select.get());
    return *at<gp<Texture>>(s, eye == 1 ? 104 : 108);
}

// ---------------------------------------------------------------- warp matrices

namespace {

// One warp matrix: Matrix_FromAxisAngle(m, g::rotation_axis, angle),
// then the scroll (su, sv, rotated by the matrix and offset by its row
// 3) is folded in: the rows are scaled by `scale`, and row 2 becomes
// {frac(x), frac(y), z} of the scrolled point.
void warp_matrix(be_f32* m, be_f32* tmp, be_u32* scale, f32 su, f32 sv, f32 angle) {
    call(s3::fn::Matrix_FromAxisAngle, m, s3::g::rotation_axis, angle);
    const be_f32* a = aligned16(m);  // lvx
    f32 r[4];
    for (u32 i = 0; i < 4; ++i) {
        f32 x = std::fmaf(a[i], su, 0.0f);  // vmaddfp
        x = std::fmaf(a[4 + i], sv, x);
        x = std::fmaf(a[8 + i], 0.0f, x);
        r[i] = a[12 + i] + x;  // vaddfp
    }
    for (u32 i = 0; i < 4; ++i) tmp[i] = r[i];  // stvx
    const f32 x = tmp[0], y = tmp[1];
    const f32 tx = trunc_single(x), ty = trunc_single(y);
    call(s3::fn::Matrix_ScaleRows, m, scale);
    m[8] = x - tx;  // fsubs
    m[9] = y - ty;
    copy_word(m + 10, tmp + 2);
}

// TextureWarp_BuildMatrices' body (k: the default f32 for missing
// parameters, 0.0). Parameters (material +124): +0 / +4 scroll speed and
// +32 angle of matrix 1, +12 / +16 and +36 of matrix 2, +24 / +28 the
// scale of both, +8 / +20 the vec4's x / y.
void build_warp_matrices(Material* mat, be_f32* m1, be_f32* m2, be_u32* v, u32 k) {
    const f32 t = global<be_f32>(s3::g::material_time);
    // The original's sp+112 (scroll point) and sp+128 (scale).
    GuestFrame f(32);
    be_f32* tmp = f.as<be_f32>(0);
    be_u32* scale = f.as<be_u32>(16);
    const be_u32* p = params_of(mat);
    scale[1] = param_w(p, 28, k);
    scale[0] = param_w(p, 24, k);
    copy_word(scale + 2, host<u8>(k));
    f32 su = t * param_f(p, 0, k);  // fmuls
    f32 sv = t * param_f(p, 4, k);
    warp_matrix(m1, tmp, scale, su, sv, param_f(p, 32, k));
    p = params_of(mat);
    su = t * param_f(p, 12, k);
    sv = t * param_f(p, 16, k);
    warp_matrix(m2, tmp, scale, su, sv, param_f(p, 36, k));
    v[0] = param_w(params_of(mat), 8, k);
    v[1] = param_w(params_of(mat), 20, k);
    copy_word(v + 2, host<u8>(k));
    copy_word(v + 3, host<u8>(k));
}

}  // namespace

// void TextureWarp_BuildMatrices(Material* mat, f32 m1[16], f32 m2[16], f32 v[4])
void TextureWarp_BuildMatrices(Material* mat, be_f32* m1, be_f32* m2, be_u32* v) {
    build_warp_matrices(mat, m1, m2, v, 0x005AE640);
}
// void SlyTextureWarp_BuildMatrices(Material* mat, f32 m1[16], f32 m2[16], f32 v[4]): the same code.
void SlyTextureWarp_BuildMatrices(Material* mat, be_f32* m1, be_f32* m2, be_u32* v) {
    build_warp_matrices(mat, m1, m2, v, 0x005D12C4);
}

// ---------------------------------------------------------------- per-material vertex constants

// void SlyHeightFog_UploadFog(Material* mat)
// Parameters (+124; k0 for each when missing): vertex constant 11 = {k2
// / p16, p12, k2 / p16 * p20 * t, k0}, 12 = {k2 / p40, p36, k2 / p40 *
// p44 * t, k0} (t: g::material_time), 13 = normalize(p0, k0, p4), 14 =
// normalize(p24, k0, p28) (each with w 1.0; k1 / length).
void SlyHeightFog_UploadFog(Material* mat) {
    constexpr u32 k = 0x005C6B14;  // f32[3] in .text
    const be_f32* kc = host<be_f32>(k);
    const be_u32* p = params_of(mat);
    const f32 k0 = kc[0], k1 = kc[1], k2 = kc[2];
    const f32 a0 = param_f(p, 0, k), a2 = param_f(p, 4, k);
    const f32 kk = k0 * k0;  // fmuls
    const f32 ia = k1 / std::sqrt(fmadds(a2, a2, fmadds(a0, a0, kk)));  // fsqrts, fdivs
    const f32 b0 = param_f(p, 24, k), b2 = param_f(p, 28, k);
    const f32 ib = k1 / std::sqrt(fmadds(b2, b2, fmadds(b0, b0, kk)));
    const f32 t = global<be_f32>(s3::g::material_time);
    // The original's sp+112 (constant 11), +128 (12), +144 (14), +160 (13).
    GuestFrame f(64);
    be_f32* v11 = f.as<be_f32>(0);
    be_f32* v12 = f.as<be_f32>(16);
    be_f32* v14 = f.as<be_f32>(32);
    be_f32* v13 = f.as<be_f32>(48);
    const auto raw = [](be_f32* dst, u32 w) { *reinterpret_cast<be_u32*>(dst) = w; };
    const f32 x11 = k2 / param_f(p, 16, k);  // fdivs
    raw(v11 + 1, param_w(p, 12, k));
    copy_word(v11 + 3, kc);
    raw(v12 + 1, param_w(p, 36, k));
    copy_word(v12 + 3, kc);
    const f32 x12 = k2 / param_f(p, 40, k);
    v11[0] = x11;
    v12[0] = x12;
    v11[2] = (x11 * param_f(p, 20, k)) * t;  // fmuls, fmuls
    v12[2] = (x12 * param_f(p, 44, k)) * t;
    vconsts(11, 4, v11);
    vconsts(12, 4, v12);
    raw(v13 + 3, s2::kOne);
    v13[0] = a0 * ia;
    v13[1] = k0 * ia;
    v13[2] = a2 * ia;
    vconsts(13, 4, v13);
    raw(v14 + 3, s2::kOne);
    v14[0] = b0 * ib;
    v14[1] = k0 * ib;
    v14[2] = b2 * ib;
    vconsts(14, 4, v14);
}

namespace {

// (x, k0, z) normalized unless its length is below k1 (fcmpu / blt: a NaN
// length divides).
void normalize_xz(f32& x, f32& y, f32& z, f32 kk, f32 k1) {
    const f32 len = std::sqrt(fmadds(z, z, fmadds(x, x, kk)));  // fsqrts
    if (std::fabs(len) < k1) return;
    x = x / len;
    y = y / len;
    z = z / len;
}

// SlyWater2 / SlyWater3 wave constants (k: f32[3] in .text; parameters
// +base.. a 32-byte wave block; first: the first vertex constant).
// Parameters (material +124; k0 for each when missing): constant first
// = {k2 / w+4, w+0, k2 / w+4 * w+8 * t, w+24}, first + 1 = {k2 / w+16,
// w+12, k2 / w+16 * w+20 * t, w+28} (t: g::material_time, w: parameters
// + base); first + 2 / + 3 = the directions (p0, k0, p4) and (p12, k0,
// p16), normalized when not below k1, with w 1.0.
void upload_waves(Material* mat, u32 k, u32 base, u32 first) {
    const be_f32* kc = host<be_f32>(k);
    const be_u32* p = params_of(mat);
    const f32 k0 = kc[0], k1 = kc[1], k2 = kc[2];
    const f32 kk = k0 * k0;  // fmuls
    f32 a0 = param_f(p, 0, k), a1 = k0, a2 = param_f(p, 4, k);
    normalize_xz(a0, a1, a2, kk, k1);
    f32 b0 = param_f(p, 12, k), b1 = k0, b2 = param_f(p, 16, k);
    normalize_xz(b0, b1, b2, kk, k1);
    const f32 t = global<be_f32>(s3::g::material_time);
    const f32 x0 = k2 / param_f(p, base + 4, k);  // fdivs
    const f32 x1 = k2 / param_f(p, base + 16, k);
    // The original's sp+112 (first), +128 (first + 1), +144 (first + 3), +160 (first + 2).
    GuestFrame f(64);
    be_f32* v0 = f.as<be_f32>(0);
    be_f32* v1 = f.as<be_f32>(16);
    be_f32* vb = f.as<be_f32>(32);
    be_f32* va = f.as<be_f32>(48);
    const auto raw = [](be_f32* dst, u32 w) { *reinterpret_cast<be_u32*>(dst) = w; };
    v0[0] = x0;
    raw(v0 + 1, param_w(p, base, k));
    v0[2] = (x0 * param_f(p, base + 8, k)) * t;  // fmuls, fmuls
    raw(v0 + 3, param_w(p, base + 24, k));
    v1[0] = x1;
    raw(v1 + 1, param_w(p, base + 12, k));
    v1[2] = (x1 * param_f(p, base + 20, k)) * t;
    raw(v1 + 3, param_w(p, base + 28, k));
    vconsts(first, 4, v0);
    vconsts(first + 1, 4, v1);
    va[0] = a0;
    va[1] = a1;
    va[2] = a2;
    raw(va + 3, s2::kOne);
    vconsts(first + 2, 4, va);
    vb[0] = b0;
    vb[1] = b1;
    vb[2] = b2;
    raw(vb + 3, s2::kOne);
    vconsts(first + 3, 4, vb);
}

}  // namespace

// void SlyWater2_UploadWaves(Material* mat): vertex constants 36..39, waves at parameters +100.
void SlyWater2_UploadWaves(Material* mat) { upload_waves(mat, 0x005D3174, 100, 36); }
// void SlyWater3_UploadWaves(Material* mat): vertex constants 35..38, waves at parameters +108.
void SlyWater3_UploadWaves(Material* mat) { upload_waves(mat, 0x005D4A28, 108, 35); }

// ---------------------------------------------------------------- draw items

// DrawItem* DrawItem_Create(DrawMesh* mesh, Material* mat, const ItemDesc* d)
// An 80-byte DrawItem from the frame arena (kArena +20), unzeroed: +0..
// the 4 x 3 matrix rows of d (+0 / +16 / +32 / +48, w not copied), +12
// mesh, +28 mat, +44 d +92, +60 d +80, +64 the item data (d +84, d +89
// bytes; copied into the arena, 16-byte padded, unless it already lies
// inside the arena's buffer), +68 flags (bit 0 kept as the arena held
// it; bits 1..4 the data size's low nibble; bits 5..7 from d +71), +69 0,
// +70 d +88, +71 d +69, +72 u8[4] d +72.
DrawItem* DrawItem_Create(DrawMesh* mesh, Material* mat, const u8* d) {
    be_u32* arena = host<be_u32>(s3::kArena);
    be_u32& cursor_ref = arena[s3::kArenaCursor / 4];
    DrawItem* item = host<DrawItem>(cursor_ref);
    cursor_ref = cursor_ref + s3::kDrawItemSize;
    item = or_zero_page(item);  // (unchecked by the original)
    item->mesh = mesh;
    item->mat = mat;
    for (u32 row = 0; row < 64; row += 16)
        for (u32 i = 0; i < 12; i += 4) copy_word(at<u8>(item, row + i), d + row + i);  // lfs / stfs
    for (u32 i = 0; i < 4; ++i) *at<u8>(item, 72 + i) = d[72 + i];
    u32 flags = item->flags68 & ~0x1Eu;  // rlwimi 0,27,30 with 0
    item->layers.ea = at<be_u32>(d, 92)->get();
    item->w64 = 0u;
    item->variant = 0;
    item->flags68 = static_cast<u8>(flags);
    for (const u32 bit : {0x80u, 0x40u, 0x20u}) {  // rlwimi 0,24,24 / 25,25 / 26,26, d +71 reloaded each time
        flags = (flags & ~bit) | (d[71] & bit);
        item->flags68 = static_cast<u8>(flags);
    }
    *at<u8>(item, 71) = d[69];
    item->bones.ea = at<be_u32>(d, 80)->get();
    *at<u8>(item, 70) = d[88];
    const u32 data = *at<be_u32>(d, 84);
    const u32 cursor = cursor_ref;
    if (data == 0) return item;
    const u32 size = d[89];
    if (data >= arena[s3::kArenaLo / 4] && data < arena[s3::kArenaHi / 4]) {  // cmplw
        item->w64 = data;
        item->flags68 = static_cast<u8>((flags & ~0x1Eu) | ((size << 1) & 0x1Eu));  // rlwimi 1,27,30
        return item;
    }
    cursor_ref = cursor + ((size + 15) & ~15u);
    const u32 fl = item->flags68;
    item->w64 = cursor;
    item->flags68 = static_cast<u8>((fl & ~0x1Eu) | ((size << 1) & 0x1Eu));
    call(s3::fn::memmove, cursor, data, size);
    return item;
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
void Skin_BuildJob(u8* job, const be_u32* desc, const u8* part, u32 bones, u32 flags) {
    const auto h = [part](u32 off) -> u32 { return *at<be_u16>(part, off); };
    const auto w = [part](u32 off) -> u32 { return *at<be_u32>(part, off); };
    const auto put = [job](u32 off, u32 v) { *at<be_u32>(job, off) = v; };
    const u32 p16 = w(16), p20 = h(20);
    put(52, p16);
    put(48, p20);
    put(60, p16 + p20);
    put(56, h(22));
    put(68, h(44) + bones);
    put(64, h(48));
    put(76, h(46) + bones);
    put(72, h(50));
    const u32 p56 = w(56), p52 = h(52);
    put(84, p56);
    put(80, p52);
    put(92, p56 + p52);
    put(88, h(54));
    const u32 p24 = w(24), p32 = h(32), p34 = h(34);
    put(100, p24);
    put(96, p32);
    put(108, p24 + p32);
    put(104, p34);
    put(116, p24 + p32 + p34);
    put(112, h(36));
    const u32 p28 = w(28), p38 = h(38), p40 = h(40);
    put(124, p28);
    put(120, p38);
    put(132, p28 + p38);
    put(128, p40);
    put(140, p28 + p38 + p40);
    put(136, h(42));
    put(148, desc[1]);
    put(144, 128);
    put(156, desc[2]);
    put(152, 48);
    put(164, guest(part));
    put(160, 16);
    put(176, desc[3]);

    const u32 n = *at<u8>(part, 1);
    GcmContext* c = or_zero_page(host<GcmContext>(desc[0]));
    const u32 words = n * 4 + 3;
    call(s3::fn::cellGcmReserveMethodSize, c, words);
    if (c->current + words * 4 > c->end) {  // cmplw / ble
        const u32 opd = c->callback;
        if (call_ptr<u32>(opd, c, words) != 0) return;  // cmpwi (r3 = the callback's result)
    }
    u32 cur = c->current;
    while ((cur & 15) != 0) {  // NOP padding to 16 bytes
        c->current = cur + 4;
        *host<be_u32>(cur) = 0u;
        cur = c->current;
    }
    const u32 space = cur, end = space + n * 16;
    GuestFrame f(16);  // the original's sp+112: the offset out-parameter
    be_u32* off = f.as<be_u32>();
    call(s3::fn::cellGcmAddressToOffset, space, off);
    u32 at_ = c->current;
    *reinterpret_cast<be_u32*>(mem::ptr(at_)) = *off | s3::kJumpFlag;
    u32 next = at_ + 4;
    c->current = next;
    for (u32 line = (at_ + 132) & ~127u; end > line; line = (at_ + 132) & ~127u) {  // cmplw / bgt
        rt::dcbz(line);
        c->current = line;
        call(s3::fn::cellGcmAddressToOffset, line, off);
        at_ = c->current;
        *reinterpret_cast<be_u32*>(mem::ptr(at_)) = *off | s3::kJumpFlag;
        next = at_ + 4;
        c->current = next;
    }
    c->current = n != 0 ? end : next;
    put(180, space);
    u32 mode = 1;
    if (global<u8>(s3::g::skin_mode_override) == 0) {
        const u32 kind = w(68);
        if (kind != 0) mode = kind == 2304 ? 2 : 3;
    }
    const u32 image = desc[4];
    put(184, mode | flags);
    *at<be_u16>(job, 10) = 120;
    *at<be_u16>(job, 28) = 0;
    *at<be_u64>(job, 0) = image;
    const u32 image_size = desc[5];
    put(12, 0);
    *at<be_u16>(job, 8) = static_cast<u16>(image_size);
    put(16, 1);
    const u32 p60 = w(60);
    put(24, 0);
    put(20, p60);
    const u32 p64 = w(64);
    put(32, 0);
    *at<be_u16>(job, 30) = static_cast<u16>(p64);
    put(36, 0);
}

}  // namespace sly4::gfx

NATIVE(0050CF50, sly4::gfx::Draw_SetParticleState)
NATIVE(0050D0E0, sly4::gfx::DrawItem_SetParticleState)
NATIVE(0050D124, sly4::gfx::MaterialAnim_UploadUV)
NATIVE(0050D1E0, sly4::gfx::MaterialAnim_UploadUVAlt)
NATIVE(0052F270, sly4::gfx::FProgram_SetConstant)
NATIVE(00512940, sly4::gfx::Draw_GetScreenCopy)
NATIVE(00537BB8, sly4::gfx::Display_GetDepthTexture)
NATIVE(005AE648, sly4::gfx::TextureWarp_BuildMatrices)
NATIVE(005D12C8, sly4::gfx::SlyTextureWarp_BuildMatrices)
NATIVE(005C6B20, sly4::gfx::SlyHeightFog_UploadFog)
NATIVE(005D3180, sly4::gfx::SlyWater2_UploadWaves)
NATIVE(005D4A38, sly4::gfx::SlyWater3_UploadWaves)
NATIVE(0079D6F8, sly4::gfx::DrawItem_Create)
NATIVE(00537824, sly4::gfx::Skin_BuildJob)
