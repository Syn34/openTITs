// The remaining shader Draw slots (Shader vtable slot 12):
// void <Shader>_Draw(Shader* s, DrawItem* item)
#include "shader5.h"
#include "shader_draw.h"

namespace sly4::gfx {

namespace {

using namespace sdraw;
namespace s4 = sly4::render::shader4;

constexpr u32 kLightDir = 0x010DD8A0;   // view block +288: the camera rotation's column 2 (shader4_draw.cpp kLightDir)
constexpr u32 kEye = 0x010DD8B0;        // view block +304: the camera position (render::g::camera_pos)
constexpr u32 kViewProj2 = 0x010DD800;  // view block +128: f32[4][4] (view +944) (name?)

constexpr u32 kCollectibleLight = 0x01196F80;  // f32[4] ambient, f32[4] diffuse, f32[3] direction (shader3_typeinit.cpp kColours) (name?)
constexpr u32 kViewRotation = 0x010DD840;      // view block +192 (shader4_draw.cpp)
constexpr u32 kViewColumns = 0x010DD880;       // view block +256: the camera rotation transposed (3 rows)
constexpr u32 Matrix_Normalize = 0x0056D268;   // (f32 dst[4][4], const f32 src[4][4]) (obj/obj10.cpp)

const be_f32* as_rows(const void* p) { return static_cast<const be_f32*>(p); }

// The camera in `space`'s frame (Listener_LocalDirection).
void eye_in(void* out, const void* space) { call(s4::fn::Listener_LocalDirection, out, kEye, space); }

// The item's 4x4 transposed (column j = word 0 of lvlx at item + 16i +
// 4j), row 3 the vector at `unit` (lvx: {0, 0, 0, 1}), into dst.
void item_columns(be_u32* dst, const DrawItem* item, u32 unit) {
    for (u32 j = 0; j < 3; ++j) {
        for (u32 i = 0; i < 4; ++i) dst[4 * j + i] = lvlx_word0(at<u8>(item, 16 * i + 4 * j));  // vmrghw, vperm
    }
    raw4(dst + 12, aligned16(host<u8>(unit)));
}

// SlyWater2 / SlyWater3. With variant bit 4 vertex constants 0..3 =
// kViewProj2 (SlyWater3 passes a stack copy) and 4..7 = the item
// transposed (item_columns), else the item transform; the view fog;
// vertex constants 8 = {the camera in the item's frame, 1}, 9 = {kLightDir
// in the item's frame (Vec3_RotateTo), 1}; with variant bits 3 or 5
// fragment constant 2 = constant 8's vector, then for bit 5 the layer
// list (unchecked) as vertex constants 16.. and for bit 3 fragment
// constant 3 (SlyWater2: material parameters +84.. or 0.0 each;
// SlyWater3: SlyWater3_UploadFragment); vertex constant 12 = {(0, 0, -1)
// rotated by the item's row 1 (Vec3_RotateTo(out, item +16, item)), 1};
// the stencil (with 0x20), then the mesh's draw.
void water_draw(DrawItem* item, u32 k, bool water3) {
    Material* mat = item_mat(item);
    if ((item->variant & 0x10) != 0) {  // rlwinm 0,27,27
        // The original's sp+240 (SlyWater2) / sp+224 (SlyWater3), and
        // SlyWater3's copy at sp+288.
        GuestFrame f(water3 ? 128 : 64);
        be_u32* cols = f.as<be_u32>();
        item_columns(cols, item, k + 16);
        const void* src = host<u8>(kViewProj2);
        if (water3) {
            be_u32* copy = cols + 16;
            for (u32 i = 0; i < 16; ++i) copy_word(copy + i, host<be_u32>(kViewProj2) + i);
            src = copy;
        }
        vconsts(0, 16, src);
        vconsts(4, 16, cols);
    } else {
        upload_item_matrix(item);
    }
    apply_fog(item);
    // The original's sp+128 (camera), +144 (light), +224 / +208 (constants
    // 8 / 9; SlyWater3 the same).
    GuestFrame f(64);
    be_f32* eye = f.as<be_f32>(0);
    be_f32* dir = f.as<be_f32>(16);
    be_f32* c8 = f.as<be_f32>(32);
    be_f32* c9 = f.as<be_f32>(48);
    eye_in(eye, item);
    Vec3_RotateTo(dir, host<be_f32>(kLightDir), as_rows(item));
    vec3_w1(c8, eye);
    vconsts(8, 4, c8);
    vec3_w1(c9, dir);
    vconsts(9, 4, c9);
    if ((item->variant & 0x28) != 0) {
        {
            GuestFrame c(16);  // the original's sp+192 (SlyWater3 sp+176)
            vec3_w1(c.as<u8>(), eye);
            fconst(2, c.as<u8>());
        }
        if ((item->variant & 0x20) != 0) upload_layer_list(mat, item->layers);  // rlwinm 0,26,26
        if ((item->variant & 0x08) != 0) {  // rlwinm 0,28,28
            if (water3) {
                SlyWater3_UploadFragment(mat);
            } else {
                GuestFrame c(16);  // the original's sp+176
                be_u32* v = c.as<be_u32>();
                for (u32 i = 0; i < 4; ++i) {
                    const u32 params = mat_params(mat);
                    copy_word(v + i, host<u8>(params != 0 ? params + 84 + 4 * i : k + 16));  // 0.0 without parameters
                }
                fconst(3, v);
            }
        }
    }
    {
        GuestFrame c(32);  // the original's sp+160 (direction) and sp+112 (constant 12)
        be_u32* down = c.as<be_u32>(0);
        be_f32* c12 = c.as<be_f32>(16);
        down[0] = 0u;
        down[1] = 0u;
        down[2] = 0xBF800000u;  // -1.0
        Vec3_RotateTo(reinterpret_cast<be_f32*>(down), at<be_f32>(item, 16), as_rows(item));
        vec3_w1(c12, down);
        vconsts(12, 4, c12);
    }
    item_stencil(item, item->flags68, 0x20);
    draw_mesh(item);
}

// For a lit item (variant +69 bit 0) the layer list (unchecked) as
// fragment constants 16.. when variant bit 1 is set, else as vertex
// constants 16..; with variant bit 2 fragment constant 1 = {the camera in
// the item's frame, 1}. (SlyDecal_Draw's block, shader4_draw.cpp.)
void lit_layers(DrawItem* item) {
    if ((item->variant & 1) == 0) return;
    if ((item->variant & 2) != 0) {
        upload_layers_fragment(item);
    } else {
        upload_layer_list(item_mat(item), item->layers);
    }
    if ((item->variant & 4) != 0) {
        GuestFrame f(32);  // the original's sp+112 / +128
        be_f32* dir = f.as<be_f32>(0);
        be_f32* c1 = f.as<be_f32>(16);
        eye_in(dir, item);
        vec3_w1(c1, dir);
        fconst(1, c1);
    }
}

// Vertex constants 0..3 = a stack copy of kViewProj2 and 4..7 = the item
// transposed (item_columns), into `frame` (128 bytes: the original's
// sp+208 / +272).
void upload_columns_copy(be_u32* frame, const DrawItem* item, u32 unit) {
    item_columns(frame, item, unit);
    for (u32 i = 0; i < 16; ++i) copy_word(frame + 16 + i, host<be_u32>(kViewProj2) + i);
    vconsts(0, 16, frame + 16);
    vconsts(4, 16, frame);
}

// The lit draw with the stencil reference ((stencil bit 7 | extra |
// material +101) & the write mask).
void lit_draw_bits(DrawItem* item, u32 k, u32 extra) {
    lit_draw_core(item, k, true, extra, 2, host<u8>(s3::g::character_fconst2), 0, true);
}

// The 3x3 rotation vmaddfp order: kViewColumns rows times (x, y, z).
void view_columns_times(be_f32* dst, f32 x, f32 y, f32 z) {
    const be_f32* m = host<be_f32>(kViewColumns);
    for (u32 k = 0; k < 4; ++k) {
        f32 v = std::fmaf(m[k], x, 0.0f);
        v = std::fmaf(m[4 + k], y, v);
        dst[k] = std::fmaf(m[8 + k], z, v);
    }
}

// SAC_EnvMap / EnvMap / SlyEnvMap (see SAC_EnvMap_Draw): no fog, the
// particle-state fog or the view fog.
enum class Fog { None, Particle, View };
void env_draw(DrawItem* item, u32 k, Fog fog) {
    upload_item_matrix(item);
    if (fog == Fog::Particle) DrawItem_SetParticleState(item, 32);
    if (fog == Fog::View) apply_fog(item);
    // The original's sp+160 (rows), +112 (camera), +128 (c).
    GuestFrame f(80);
    be_f32* rows = f.as<be_f32>(0);
    be_f32* eye = f.as<be_f32>(48);
    be_f32* c = f.as<be_f32>(64);
    Matrix_RotateRows(rows, as_rows(item), host<be_f32>(kViewRotation));
    eye_in(eye, item);
    vec3_w1(c, eye);
    const u32 variant = item->variant;
    if ((variant & 1) != 0 && (variant & 2) != 0) {
        upload_layers_fragment(item);
        for (u32 i = 0; i < 3; ++i) fconst(2 + i, rows + 4 * i);
        fconst(5, c);
    } else {
        if ((variant & 1) != 0) upload_layer_list(item_mat(item), item->layers);
        for (u32 i = 0; i < 3; ++i) call(s2::fn::cellGcmSetVertexProgramParameterBlock, gcm(), 7 + i, 1u, rows + 4 * i);
        vconsts(10, 4, c);
    }
    if (fog != Fog::View) {
        lit_draw(item, k, Stencil::Plain, 1);
        return;
    }
    // SlyEnvMap: fragment constant 1 = g::character_fconst2 with lane 2 =
    // material parameters +12 (k: 0.0 without), the same lane 2 in the
    // translucent constant.
    const u32 params = mat_params(item_mat(item));
    const u32 z = params != 0 ? host<be_u32>(params)[3].get() : global<be_u32>(k).get();
    GuestFrame first(16);  // the original's sp+144
    be_u32* v = first.as<be_u32>();
    raw4(v, host<u8>(s3::g::character_fconst2));
    v[2] = z;
    lit_draw_core(item, k + 4, true, 0, 1, v, z, true);
}

// The layer list of a lit item (variant +69 bit 0; unchecked) as
// fragment constants 16.. with variant bit 1, else vertex constants 16...
void lit_layer_list(DrawItem* item) {
    if ((item->variant & 2) != 0) {
        upload_layers_fragment(item);
    } else {
        upload_layer_list(item_mat(item), item->layers);
    }
}

// fsqrts as the original's code reaches it (a NaN for a negative square).
f32 sqrt_or_nan(f32 sq) { return sq >= 0.0f ? std::sqrt(sq) : static_cast<f32>(std::nan("")); }

// SlyTerrain's distance blend (variant bit 3): t = (params +12 - |the
// camera - the item position (+48)|) / params +16; u1 = clamp(2t, 0, 1),
// u2 = clamp(2(t - 0.5), 0, 1); fragment constant 0 = {u1 * params +0,
// params +4, u2 * params +8, 0} (0.0 for missing parameters).
void terrain_blend(DrawItem* item, u32 k) {
    const be_f32* eye = host<be_f32>(kEye);
    const f32 dy = eye[1] - item->pos[1];
    const f32 dx = eye[0] - item->pos[0];
    const f32 dz = eye[2] - item->pos[2];
    const u32 params = mat_params(item_mat(item));
    const auto p = [&](u32 off) { return host<be_f32>(params != 0 ? params + off : k); };
    const f32 sq = fmadds(dz, dz, fmadds(dx, dx, dy * dy));
    const f32 d = sqrt_or_nan(sq);  // fsqrts
    const f32 t = (*p(12) - d) / *p(16);
    const f32 half = global<be_f32>(k + 4), two = global<be_f32>(k + 8), one = global<be_f32>(k + 12);
    const f64 one_d = std::bit_cast<f64>(global<be_u64>(k + 16).get()), zero_d = std::bit_cast<f64>(global<be_u64>(k + 24).get());
    f32 a = t * two;
    f32 b = (t - half) * two;
    a = static_cast<f32>(fsel(one - a, a, one_d));  // fsel: min(.., 1)
    b = static_cast<f32>(fsel(one - b, b, one_d));
    a = static_cast<f32>(fsel(a, a, zero_d));  // fsel: max(.., 0)
    b = static_cast<f32>(fsel(b, b, zero_d));
    GuestFrame f(16);  // the original's sp+112
    be_f32* c = f.as<be_f32>();
    c[0] = a * *p(0);
    copy_word(c + 1, p(4));
    copy_word(c + 3, host<u8>(k));
    c[2] = b * *p(8);
    fconst(0, c);
}

}  // namespace

// SlyTerrain: the item transform and the view fog; with variant +69 bit
// 3 the distance blend (terrain_blend) as fragment constant 0; the lit
// layers (lit_layers), the lit draw (stencil without 0x40), then with
// bit 3 fragment constant 0 back to {params +0, +4, +8, 0}.
void SlyTerrain_Draw(ShaderPrograms*, DrawItem* item) {
    constexpr u32 k = 0x005CC218;  // .text: f32 0.0, 0.5, 2.0, 1.0; f64 1.0, 0.0; f32 1/255
    upload_item_matrix(item);
    apply_fog(item);
    if ((item->variant & 8) != 0) terrain_blend(item, k);  // rlwinm 0,28,28
    lit_layers(item);
    lit_draw(item, k + 32, Stencil::Plain);
    if ((item->variant & 8) == 0) return;
    const u32 params = mat_params(item_mat(item));
    GuestFrame f(16);  // the original's sp+160
    be_u32* c = f.as<be_u32>();
    for (u32 i = 0; i < 3; ++i) copy_word(c + i, host<u8>(params != 0 ? params + 4 * i : k));
    copy_word(c + 3, host<u8>(k));
    fconst(0, c);
}

// SlyCharacter: the item transform and the view fog; vertex constant
// parameter blocks 7..9 = the item rows through kViewRotation
// (Matrix_RotateRows), vertex constant 10 = c = {the camera in the item's
// frame, 1}; for a lit item (variant +69 bit 0) the layer list
// (lit_layer_list) and with variant bit 2 fragment constant 1 = c; then
// the lit draw, its stencil bit 0x40 set when the material parameters'
// word +28 is nonzero.
void SlyCharacter_Draw(ShaderPrograms*, DrawItem* item) {
    upload_item_matrix(item);
    apply_fog(item);
    // The original's sp+160 (rows), +112 (camera), +128 (c).
    GuestFrame f(80);
    be_f32* rows = f.as<be_f32>(0);
    be_f32* eye = f.as<be_f32>(48);
    be_f32* c = f.as<be_f32>(64);
    Matrix_RotateRows(rows, as_rows(item), host<be_f32>(kViewRotation));
    eye_in(eye, item);
    vec3_w1(c, eye);
    for (u32 i = 0; i < 3; ++i) call(s2::fn::cellGcmSetVertexProgramParameterBlock, gcm(), 7 + i, 1u, rows + 4 * i);
    vconsts(10, 4, c);
    if ((item->variant & 1) != 0) {
        lit_layer_list(item);
        if ((item->variant & 4) != 0) fconst(1, c);
    }
    const be_u32* params = host<be_u32>(mat_params(item_mat(item)));
    const u32 extra = params && params[7] != 0 ? 0x40u : 0u;  // read after the mirroring in the original
    lit_draw_bits(item, 0x005BB4D0, extra);  // .text: f32 1/255
}

// SlyCharacterCollectible: the item transform and the view fog; n =
// Matrix_Normalize(item); vertex constants 7..9 = n's rows through
// kViewRotation (Matrix_RotateRows, one parameter block each), 10 = {the
// camera in n's frame, 1}; for a lit item (variant +69 bit 0) the
// collectible light: {ambient * material colour (+64)}, {diffuse *
// colour}, {its direction through kViewColumns, then into n's frame
// (Vec3_RotateTo), 1} as fragment constants 16..18 with variant bit 1,
// else vertex constants 16..18, and with variant bit 2 fragment constant
// 1 = vertex constant 10's vector; then the lit draw with stencil bit
// 0x10.
void SlyCharacterCollectible_Draw(ShaderPrograms*, DrawItem* item) {
    upload_item_matrix(item);
    apply_fog(item);
    // The original's sp+368 (n), +224 (rows), +112 (camera), +128 (constant 10).
    GuestFrame frame(128);
    be_f32* n = frame.as<be_f32>(0);
    be_f32* rows = frame.as<be_f32>(64);
    be_f32* eye = frame.as<be_f32>(112);
    GuestFrame frame2(16);
    be_f32* c10 = frame2.as<be_f32>();
    call(Matrix_Normalize, n, item);
    Matrix_RotateRows(rows, n, host<be_f32>(kViewRotation));
    eye_in(eye, n);
    vec3_w1(c10, eye);
    for (u32 i = 0; i < 3; ++i) call(s2::fn::cellGcmSetVertexProgramParameterBlock, gcm(), 7 + i, 1u, rows + 4 * i);
    vconsts(10, 4, c10);
    if ((item->variant & 1) != 0) {
        const bool fragment = (item->variant & 2) != 0;  // rlwinm 0,30,30
        const be_f32* g = host<be_f32>(kCollectibleLight);
        const be_f32* colour = at<be_f32>(item_mat(item), 64);
        // The original's sp+320.. / +192 / +208 (fragment) or sp+272.. / +160 / +176.
        GuestFrame c(80);
        be_f32* v = c.as<be_f32>(0);
        be_f32* in = c.as<be_f32>(48);
        be_f32* out = c.as<be_f32>(64);
        for (u32 i = 0; i < 4; ++i) {
            v[i] = g[i] * colour[i];  // fmuls
            v[4 + i] = g[4 + i] * colour[i];
        }
        view_columns_times(in, lvlx_f32(g, 32), lvlx_f32(g, 36), lvlx_f32(g, 40));
        Vec3_RotateTo(out, in, n);
        vec3_w1(v + 8, out);
        if (fragment) {
            for (u32 i = 0; i < 3; ++i) fconst(16 + i, v + 4 * i);
        } else {
            vconsts(16, 12, v);
        }
        if ((item->variant & 4) != 0) fconst(1, c10);
    }
    lit_draw_bits(item, 0x005C2878, 0x10);  // .text: f32 1/255
}

// SlyHeightFog: vertex constants 0..7 as upload_columns_copy; the view
// fog; for a lit item (variant +69 bit 0) SlyHeightFog_BuildMatrices'
// two 3x4s as fragment constants 3..5 and 6..8 (xyz of each row; the w
// lanes are whatever the original's stack held at sp+172 / +188 / +204,
// never written: here whatever the guest stack holds); fragment constant
// 1 = {params +60, params +56, z, params +64} (0.0 for missing
// parameters), z 0.0, or with variant bit 1 the distance d from the item
// position to the current View's camera (+864): x = max(d, 1) raised to
// 0.2 with the bit trick (fcfid of its double's bits * 0.2 + K, fctid,
// the bits back as a double), z = max(0, min(params +68 (u32), x - 1));
// then the lit draw (stencil without 0x40).
void SlyHeightFog_Draw(ShaderPrograms*, DrawItem* item) {
    constexpr u32 k = 0x005C71C0;  // .text: f32 0.0, 1.0; f64 0.0; mask; {0, 0, 0, 1}; f64 1.0, 0.2, K; f32 1/255
    const auto kd = [](u32 ea) { return std::bit_cast<f64>(global<be_u64>(ea).get()); };
    GuestFrame cols(128);
    upload_columns_copy(cols.as<be_u32>(), item, k + 32);
    apply_fog(item);
    Material* mat = item_mat(item);
    if ((item->variant & 1) != 0) {
        // The original's sp+336 / +400 (the matrices) and +160.. (constants).
        GuestFrame frame(176);
        be_u32* a = frame.as<be_u32>(0);
        be_u32* b = frame.as<be_u32>(64);
        be_u32* c = frame.as<be_u32>(128);
        SlyHeightFog_BuildMatrices(mat, reinterpret_cast<be_f32*>(a), reinterpret_cast<be_f32*>(b));
        for (u32 n = 0; n < 2; ++n) {
            const be_u32* m = n == 0 ? a : b;
            for (u32 r = 0; r < 3; ++r) copy3(c + 4 * r, m + 4 * r);
            for (u32 r = 0; r < 3; ++r) fconst(3 + 3 * n + r, c + 4 * r);
        }
    }
    f32 z = global<be_f32>(k);
    if ((item->variant & 2) != 0) {  // rlwinm 0,30,30
        GuestFrame frame(16);  // the original's sp+128
        be_f32* pos = frame.as<be_f32>();
        const be_f32* rows = aligned16(item);
        f32 p[4];
        for (u32 i = 0; i < 4; ++i) {
            f32 v = std::fmaf(rows[i], 0.0f, 0.0f);  // vmaddfp with zero
            v = std::fmaf(rows[4 + i], 0.0f, v);
            v = std::fmaf(rows[8 + i], 0.0f, v);
            p[i] = rows[12 + i] + v;  // vaddfp
        }
        for (u32 i = 0; i < 4; ++i) pos[i] = p[i];
        const be_f32* cam = at<be_f32>(current_view(), 864);
        const f32 dy = cam[1] - pos[1];
        const f32 dx = cam[0] - pos[0];
        const f32 dz = cam[2] - pos[2];
        const f32 sq = fmadds(dz, dz, fmadds(dx, dx, dy * dy));
        f32 d = sqrt_or_nan(sq);  // fsqrts
        d = static_cast<f32>(fsel(kd(k + 8), d, d));
        const f32 one = global<be_f32>(k + 4);
        d = static_cast<f32>(fsel(d - one, d, kd(k + 48)));  // max(d, 1)
        f32 x = 0.0f;
        if (d != global<be_f32>(k)) {  // fcmpu (never equal: d >= 1)
            const f64 t = std::fma(rt::fcfid(std::bit_cast<u64>(f64{d})), kd(k + 56), kd(k + 64));  // fcfid, fmadd
            x = static_cast<f32>(std::bit_cast<f64>(rt::fctid(t, false)));  // fctid, frsp
        }
        const f32 limit = x - one;
        const u32 params = mat_params(mat);
        const f32 cap = static_cast<f32>(static_cast<f64>(params != 0 ? host<be_u32>(params)[17].get() : 0u));  // fcfid, frsp
        const f64 m = fsel(cap - limit, limit, cap);  // min
        z = static_cast<f32>(fsel(m, m, kd(k + 8)));  // max(.., 0)
    }
    {
        GuestFrame frame(16);  // the original's sp+112
        be_u32* c1 = frame.as<be_u32>();
        const auto param = [&](u32 off) {
            const u32 params = mat_params(mat);
            return params != 0 ? host<be_u32>(params + off)->get() : global<be_u32>(k).get();
        };
        c1[0] = param(60);
        c1[1] = param(56);
        reinterpret_cast<be_f32*>(c1)[2] = z;
        c1[3] = param(64);
        fconst(1, c1);
    }
    lit_draw(item, k + 72, Stencil::Plain);
}

// Diffuse: the item transform, the view fog (DrawItem_SetParticleState
// (item, 32)), the lit layers (lit_layers), then the lit draw (stencil
// without 0x40).
void Diffuse_Draw(ShaderPrograms*, DrawItem* item) {
    upload_item_matrix(item);
    DrawItem_SetParticleState(item, 32);
    lit_layers(item);
    lit_draw(item, 0x005B1F9C, Stencil::Plain);  // .text: f32 1/255
}

// SlyPanorama: the item transform, the view fog, the lit layers
// (lit_layers), then the lit draw (stencil without 0x40).
void SlyPanorama_Draw(ShaderPrograms*, DrawItem* item) {
    upload_item_matrix(item);
    apply_fog(item);
    lit_layers(item);
    lit_draw(item, 0x005C9940, Stencil::Plain);  // .text: f32 1/255
}

// SAC_EnvMap: the item transform (no fog); rows = the item rows through
// kViewRotation (Matrix_RotateRows), c = {the camera in the item's frame,
// 1}; for a lit item (variant +69 bit 0) with variant bit 1 the layer
// list (unchecked) as fragment constants 16.., then rows and c as
// fragment constants 2..5; otherwise (bit 1 clear: the layer list as
// vertex constants 16.. first) rows as vertex constant parameter blocks
// 7..9 and c as vertex constant 10; then the lit draw with fragment
// constant 1 (stencil without 0x40).
void SAC_EnvMap_Draw(ShaderPrograms*, DrawItem* item) { env_draw(item, 0x005B8EB8, Fog::None); }  // .text: f32 1/255

// EnvMap: SAC_EnvMap's draw with the view fog (DrawItem_SetParticleState
// (item, 32)) after the item transform.
void EnvMap_Draw(ShaderPrograms*, DrawItem* item) { env_draw(item, 0x005B3588, Fog::Particle); }  // .text: f32 1/255

// SlyEnvMap: SAC_EnvMap's draw with the view fog after the item
// transform and fragment constant 1 = g::character_fconst2 with lane 2
// from material parameters +12 (also lane 2 of the translucent
// constant).
void SlyEnvMap_Draw(ShaderPrograms*, DrawItem* item) { env_draw(item, 0x005C57A4, Fog::View); }  // .text: f32 0.0, 1/255

// SlyWater2: water_draw (fragment constant 3 from the material
// parameters).
void SlyWater2_Draw(ShaderPrograms*, DrawItem* item) { water_draw(item, 0x005D3730, false); }  // .text: vperm mask, {0, 0, 0, 1}

// SlyWater3: water_draw (fragment constant 3 from
// SlyWater3_UploadFragment).
void SlyWater3_Draw(ShaderPrograms*, DrawItem* item) { water_draw(item, 0x005D5200, true); }  // .text: vperm mask, {0, 0, 0, 1}

}  // namespace sly4::gfx

NATIVE(005CC240, sly4::gfx::SlyTerrain_Draw)
NATIVE(005BB4D8, sly4::gfx::SlyCharacter_Draw)
NATIVE(005C2880, sly4::gfx::SlyCharacterCollectible_Draw)
NATIVE(005C7210, sly4::gfx::SlyHeightFog_Draw)
NATIVE(005B1FA0, sly4::gfx::Diffuse_Draw)
NATIVE(005C9948, sly4::gfx::SlyPanorama_Draw)
NATIVE(005B8EC0, sly4::gfx::SAC_EnvMap_Draw)
NATIVE(005B3590, sly4::gfx::EnvMap_Draw)
NATIVE(005C57B0, sly4::gfx::SlyEnvMap_Draw)
NATIVE(005D3750, sly4::gfx::SlyWater2_Draw)
NATIVE(005D5220, sly4::gfx::SlyWater3_Draw)
