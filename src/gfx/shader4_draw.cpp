// More shader Draw slots (Shader vtable slot 12):
// void <Shader>_Draw(Shader* s, DrawItem* item)
#include "shader4.h"
#include "shader_draw.h"

namespace sly4::gfx {

namespace {

using namespace sdraw;
namespace s4 = sly4::render::shader4;

constexpr u32 kLightDir = 0x010DD8A0;      // layers3::g::block_dd780 +288: f32[3] the light direction (name?)
constexpr u32 kViewRotation = 0x010DD840;  // layers3::g::block_dd780 +192: f32[4][4] (name?)
constexpr u32 kEye = 0x010DD8B0;           // layers3::g::block_dd780 +304: the camera position
constexpr u32 kFurLight = 0x00F66FD0;      // u32: the fur shaders' light record (+32 f32[3] direction, +48 f32[4]) or 0 (name?)
constexpr u32 kProjectorBlock = 0x01196A00;  // passed to ProjTxr_BuildProjection (name?)
constexpr u32 kZeroVconsts = 0x01164BC0;   // f32[4] (fx::g::zero_vconsts)

// Vertex constants 4..7 = the item rows through kViewRotation (lvx
// rows; row 3 adding its row 3 with vaddfp; w 0, 0, 0, 1) and 8 = {the
// item's row 1 (lvlx words) through it, 1}. (SlyWater / Water.)
void upload_rotated_rows(DrawItem* item) {
    const be_f32* vr = host<be_f32>(kViewRotation);
    {
        f32 r[4][4];
        const be_f32* rows = aligned16(item);
        for (u32 i = 0; i < 4; ++i) rows_times(r[i], vr, rows[4 * i], rows[4 * i + 1], rows[4 * i + 2]);
        for (u32 k = 0; k < 4; ++k) r[3][k] = vr[12 + k] + r[3][k];  // vaddfp
        GuestFrame f(64);  // the original's sp+208 (SlyWater) / sp+176 (Water)
        be_f32* v = f.as<be_f32>();
        for (u32 i = 0; i < 4; ++i) {
            for (u32 k = 0; k < 3; ++k) v[4 * i + k] = r[i][k];
            reinterpret_cast<be_u32*>(v)[4 * i + 3] = i == 3 ? kOne : 0u;
        }
        vconsts(4, 16, v);
    }
    f32 up[4];
    rows_times(up, vr, lvlx_f32(item, 16), lvlx_f32(item, 20), lvlx_f32(item, 24));
    GuestFrame f(16);  // the original's sp+112
    be_f32* v = f.as<be_f32>();
    for (u32 k = 0; k < 3; ++k) v[k] = up[k];
    reinterpret_cast<be_u32*>(v)[3] = kOne;
    vconsts(8, 4, v);
}

// {the camera in the item's frame (Listener_LocalDirection into `dir`), 1} into c.
void eye_w1(be_f32* dir, be_f32* c, DrawItem* item) {
    call(s4::fn::Listener_LocalDirection, dir, kEye, item);
    vec3_w1(c, dir);
}

// The stencil reference without 0x40: ((stencil bit 7 | material +101) &
// the write mask), as the lit draw's Stencil::Plain.
void plain_stencil(DrawItem* item) { item_stencil(item, item->flags68, 0); }

}  // namespace

// AngleIndex: the item transform, the view fog (DrawItem_SetParticleState
// (item, 32)), fragment constant 3 = {the light direction in the item's
// frame (Vec3_RotateTo), 1}, then the lit draw (stencil without 0x40).
void AngleIndex_Draw(ShaderPrograms*, DrawItem* item) {
    upload_item_matrix(item);
    DrawItem_SetParticleState(item, 32);
    GuestFrame f(32);  // the original's sp+112 (direction) and sp+144 (constant)
    be_f32* dir = f.as<be_f32>();
    be_f32* c = f.as<be_f32>(16);
    Vec3_RotateTo(dir, host<be_f32>(kLightDir), reinterpret_cast<const be_f32*>(item));
    vec3_w1(c, dir);
    fconst(3, c);
    lit_draw(item, 0x005B5148, Stencil::Plain);
}

// TextureWarp: the item transform, the view fog, the layers, then the
// lit draw with fragment constant 8 and neither mirroring nor stencil.
void TextureWarp_Draw(ShaderPrograms*, DrawItem* item) {
    upload_item_matrix(item);
    DrawItem_SetParticleState(item, 32);
    upload_layers(item);
    lit_draw(item, 0x005AEAFC, Stencil::None, 8, false);
}

// ProjTxr: the item transform and the view fog; the projector from the
// material parameters (+0.. three vec3s, +36 near, +40 far, +44 fov, +48
// aspect; all 0.0 without parameters): Matrix_Perspective, then
// ProjTxr_BuildProjection's matrix as vertex constants 4..7 and its
// position (w 1) as 8; the layers, then the mesh's draw.
void ProjTxr_Draw(ShaderPrograms*, DrawItem* item) {
    constexpr u32 kZero = 0x005B0FAC;  // f32 0.0 in .text
    upload_item_matrix(item);
    apply_fog(item);
    const u32 params = mat_params(item_mat(item));
    const auto p = [&](u32 off) { return host<be_f32>(params != 0 ? params + off : kZero); };
    // The original's sp+112 / +128 / +144 (vectors), +160 (position), +176
    // (constant 8), +192 (matrix), +256 (projection).
    GuestFrame f(208);
    be_f32* a = f.as<be_f32>(0);
    be_f32* b = f.as<be_f32>(16);
    be_f32* c = f.as<be_f32>(32);
    be_f32* pos = f.as<be_f32>(48);
    be_f32* c8 = f.as<be_f32>(64);
    be_f32* m = f.as<be_f32>(80);
    be_f32* proj = f.as<be_f32>(144);
    for (u32 i = 0; i < 12; i += 4) {
        copy_word(at<u8>(a, i), p(i));
        copy_word(at<u8>(b, i), p(12 + i));
        copy_word(at<u8>(c, i), p(24 + i));
    }
    Matrix_Perspective(proj, *p(44), *p(48), *p(36), *p(40));
    ProjTxr_BuildProjection(pos, m, a, b, c, proj, reinterpret_cast<const be_f32*>(item), host<be_f32>(kProjectorBlock));
    vconsts(4, 16, m);
    vec3_w1(c8, pos);
    vconsts(8, 4, c8);
    upload_layers(item);
    draw_mesh(item);
}

// SelfLight: the item transform, the view fog, the light (SelfLight_GetLight)
// as vertex constants 20 / 23 (its direction, w 1) / 26 and
// fx::g::zero_vconsts as 16, then the lit draw (stencil without 0x40).
void SelfLight_Draw(ShaderPrograms* s, DrawItem* item) {
    upload_item_matrix(item);
    DrawItem_SetParticleState(item, 32);
    GuestFrame f(64);  // the original's sp+112 / +128 / +144 and +160 (constant 23)
    be_f32* a = f.as<be_f32>(0);
    be_f32* dir = f.as<be_f32>(16);
    be_f32* b = f.as<be_f32>(32);
    be_f32* c23 = f.as<be_f32>(48);
    SelfLight_GetLight(s, item, reinterpret_cast<be_u32*>(a), reinterpret_cast<be_u32*>(dir), reinterpret_cast<be_u32*>(b));
    vec3_w1(c23, dir);
    vconsts(20, 4, a);
    vconsts(16, 4, host<u8>(kZeroVconsts));
    vconsts(23, 4, c23);
    vconsts(26, 4, b);
    lit_draw(item, 0x005B48E4, Stencil::Plain);
}

// SlyWater: the item transform and the view fog; for a lit item (variant
// +69 bit 0) fragment constants 17 / 18 = the layer list's +48 / +32
// (unchecked list) and 10 = {the camera in the item's frame, 1}; vertex
// constants 4..7 = the item rows through kViewRotation (row 3 adding its
// row 3; w 0, 0, 0, 1) and 8 = {the item's row 1 through it, 1}; the
// stencil reference ((stencil bit 7 | 0x20 | material +101) & the write
// mask), then the mesh's draw.
void SlyWater_Draw(ShaderPrograms*, DrawItem* item) {
    upload_item_matrix(item);
    apply_fog(item);
    if ((item->variant & 1) != 0) {
        const be_u32* list = item->layers;
        GuestFrame f(64);  // the original's sp+144 / +160 / +176 / +192
        be_f32* a = f.as<be_f32>(0);
        be_f32* b = f.as<be_f32>(16);
        be_f32* dir = f.as<be_f32>(32);
        be_f32* c10 = f.as<be_f32>(48);
        raw4(a, at<u8>(list, 32));
        raw4(b, at<u8>(list, 48));
        fconst(17, b);
        fconst(18, a);
        eye_w1(dir, c10, item);
        fconst(10, c10);
    }
    upload_rotated_rows(item);
    item_stencil(item, item->flags68, 0x20);
    draw_mesh(item);
}

// Water: the item transform, the view fog, vertex constants 4..8 as
// SlyWater's, 10 = {the item position (+48), 1}, then the lit draw with
// fragment constant 8 and neither mirroring nor stencil.
void Water_Draw(ShaderPrograms*, DrawItem* item) {
    upload_item_matrix(item);
    DrawItem_SetParticleState(item, 32);
    upload_rotated_rows(item);
    {
        GuestFrame f(16);  // the original's sp+160
        be_f32* v = f.as<be_f32>();
        vec3_w1(v, item->pos);
        vconsts(10, 4, v);
    }
    lit_draw(item, 0x005B5DFC, Stencil::None, 8, false);
}

// SlyShrub: the item transform and the view fog; vertex constants 4 =
// {1, 1, 1, alpha * k}, 5 = {the item position (+48), 1}, 6..8 = {params
// +4 + 8n, 1, params +8 + 8n, w} (w: g::material_time for 6, else k1;
// k1 for missing parameters); the alpha test (GREATER than parameters
// +0, s32) on while the draw runs when that is positive; the mirroring
// and stencil of the lit draw (without 0x40), then the mesh's draw.
void SlyShrub_Draw(ShaderPrograms*, DrawItem* item) {
    constexpr u32 k = 0x005CDC2C;  // .text: f32 1/255, then k1
    upload_item_matrix(item);
    apply_fog(item);
    GuestFrame f(48);  // the original's sp+112 (constant 4), +128 (6..8), +144 (5)
    be_u32* c4 = f.as<be_u32>(0);
    be_u32* cn = f.as<be_u32>(16);
    be_u32* c5 = f.as<be_u32>(32);
    for (u32 i = 0; i < 3; ++i) c4[i] = kOne;
    reinterpret_cast<be_f32*>(c4)[3] = byte_scaled(item_alpha(item), global<be_f32>(k));  // fcfid, frsp, fmuls
    vconsts(4, 4, c4);
    vec3_w1(c5, item->pos);
    vconsts(5, 4, c5);
    Material* mat = item_mat(item);
    for (u32 n = 0; n < 3; ++n) {
        const u32 params = mat_params(mat);
        copy_word(cn, host<u8>(params != 0 ? params + 4 + 8 * n : k + 4));
        cn[1] = kOne;
        copy_word(cn + 2, host<u8>(params != 0 ? params + 8 + 8 * n : k + 4));
        copy_word(cn + 3, host<u8>(n == 0 ? s3::g::material_time : k + 4));
        vconsts(6 + n, 4, cn);
    }
    const u32 params = mat_params(mat);
    const s32 ref_alpha = params != 0 ? static_cast<s32>(*host<be_u32>(params)) : 0;
    const u32 alpha_test = ref_alpha > 0 ? 1u : 0u;  // cmpwi
    if (alpha_test != 0) emit({s2::kAlphaFunc2, s2::kGreater, static_cast<u32>(ref_alpha)});
    emit({kAlphaTestEnable, alpha_test});
    u32 flags = item->flags68;
    if ((flags & kMirrored) != 0) {
        emit({kCullFaceEnable, 1});
        emit({kFrontFace, kGlCw});
        flags = item->flags68;
    }
    item_stencil(item, flags, 0);
    draw_mesh(item);
    if ((item->flags68 & kMirrored) != 0) {
        emit({kCullFaceEnable, 1});
        emit({kFrontFace, kGlCcw});
    }
    if (alpha_test != 0) emit({kAlphaTestEnable, 0});
}

// SlyCharacterFurFin: the item transform and the view fog; with a fur
// light (kFurLight) vertex constants 5 = its +48 and 6 = {its direction
// in the item's frame, 1} (fx::g::zero_vconsts for both without one);
// for a lit item (variant +69 bit 0) the layer list (unchecked) as vertex
// constants 16.. and 4 = {the camera in the item's frame, 1}; the stencil
// (without 0x40), then the mesh's draw.
void SlyCharacterFurFin_Draw(ShaderPrograms*, DrawItem* item) {
    upload_item_matrix(item);
    apply_fog(item);
    if (u8* light = global<gp<u8>>(kFurLight)) {
        GuestFrame f(48);  // the original's sp+128 / +144 / +176
        be_f32* in = f.as<be_f32>(0);
        be_f32* dir = f.as<be_f32>(16);
        be_f32* c6 = f.as<be_f32>(32);
        copy3(in, light + 32);
        Vec3_RotateTo(dir, in, reinterpret_cast<const be_f32*>(item));
        vconsts(5, 4, light + 48);
        vec3_w1(c6, dir);
        vconsts(6, 4, c6);
    } else {
        vconsts(5, 4, host<u8>(kZeroVconsts));
        vconsts(6, 4, host<u8>(kZeroVconsts));
    }
    if ((item->variant & 1) != 0) {
        upload_layer_list(item_mat(item), item->layers);
        GuestFrame f(32);  // the original's sp+160 / +112
        be_f32* dir = f.as<be_f32>(0);
        be_f32* c4 = f.as<be_f32>(16);
        eye_w1(dir, c4, item);
        vconsts(4, 4, c4);
    }
    plain_stencil(item);
    draw_mesh(item);
}

// SlyTextureWarp: the item transform, the view fog, the layers, then the
// lit draw without mirroring (stencil without 0x40).
void SlyTextureWarp_Draw(ShaderPrograms*, DrawItem* item) {
    upload_item_matrix(item);
    apply_fog(item);
    upload_layers(item);
    lit_draw(item, 0x005D17E0, Stencil::Plain, 2, false);
}

// SlyDecal: the item transform and the view fog; for a lit item (variant
// +69 bit 0) the layer list (unchecked) as fragment constants 16.. when
// variant bit 1 is set, else as vertex constants 16.., and with variant
// bit 2 fragment constant 1 = {the camera in the item's frame, 1}; the
// stencil (without 0x40), then the mesh's draw.
void SlyDecal_Draw(ShaderPrograms*, DrawItem* item) {
    upload_item_matrix(item);
    apply_fog(item);
    if ((item->variant & 1) != 0) {
        if ((item->variant & 2) != 0) {
            upload_layers_fragment(item);
        } else {
            upload_layer_list(item_mat(item), item->layers);
        }
        if ((item->variant & 4) != 0) {
            GuestFrame f(32);  // the original's sp+112 / +128
            be_f32* dir = f.as<be_f32>(0);
            be_f32* c1 = f.as<be_f32>(16);
            eye_w1(dir, c1, item);
            fconst(1, c1);
        }
    }
    plain_stencil(item);
    draw_mesh(item);
}

// void Matrix_RotateRows(f32 out[3][4], const f32 rows[3][4], const f32 m[4][4])
// out row i = m0 * rows[i][0] + m1 * rows[i][1] + m2 * rows[i][2] per
// lane (vmaddfp; rows read as lvlx words, m and out 16-byte aligned with
// lvx / stvx, m re-read for each row).
void Matrix_RotateRows(be_f32* out, const be_f32* rows, const be_f32* m) {
    be_f32* o = aligned16_out(out);
    for (u32 i = 0; i < 3; ++i) {
        f32 r[4];
        rows_times(r, m, lvlx_f32(rows, 16 * i), lvlx_f32(rows, 16 * i + 4), lvlx_f32(rows, 16 * i + 8));
        for (u32 k = 0; k < 4; ++k) *at<be_f32>(o, 16 * i + 4 * k) = r[k];
    }
}

// SAC_Skinned: the item transform, view fog 0 off (Draw_SetParticleState
// (0, false, 32)); vertex constants 16 = the light colour (g::config
// +176.., w k0 = 1.0; for an item that is not layered, the material
// colour (+64) times the item's tint (+72, u8[3] / 255), w 0.0), 20 =
// g::config +192.. (w 1), 7..9 = the item rows through kViewRotation
// (Matrix_RotateRows, one parameter block each), 10 = {the camera in the
// item's frame, 1}, 23 = {the light direction g::config +208.. in item
// space, 1}; then the lit draw (k = 1.0, stencil without 0x40).
void SAC_Skinned_Draw(ShaderPrograms*, DrawItem* item) {
    constexpr u32 k = 0x005B67BC;  // .text: f32 1.0, 1/255, 0.0
    const u8* cfg = reinterpret_cast<const u8*>(&config());
    upload_item_matrix(item);
    Draw_SetParticleState(0, 0, 32);
    // The original's sp+112 (23), +128 (16), +144 (20), +160 (direction),
    // +176 (camera), +192 (10), +224.. (rows).
    GuestFrame f(160);
    be_f32* c23 = f.as<be_f32>(0);
    be_f32* c16 = f.as<be_f32>(16);
    be_f32* c20 = f.as<be_f32>(32);
    be_f32* dir = f.as<be_f32>(48);
    be_f32* eye = f.as<be_f32>(64);
    be_f32* c10 = f.as<be_f32>(80);
    be_f32* rows = f.as<be_f32>(96);
    copy3(c16, cfg + 176);
    copy3(c20, cfg + 192);
    copy3(dir, cfg + 208);
    reinterpret_cast<be_u32*>(c20)[3] = kOne;
    Material* mat = item_mat(item);
    const bool layered = ((mat->b100 >> 7) & 1) != 0 && item->layers.ea != 0;
    if (layered) {
        copy_word(c16 + 3, host<u8>(k));
    } else {
        const f32 scale = global<be_f32>(k + 4);
        for (u32 i = 0; i < 3; ++i) c16[i] = *at<be_f32>(mat, 64 + 4 * i) * byte_scaled(*at<u8>(item, 72 + i), scale);  // fmuls
        copy_word(c16 + 3, host<u8>(k + 8));
    }
    Matrix_RotateRows(rows, reinterpret_cast<const be_f32*>(item), host<be_f32>(kViewRotation));
    call(s4::fn::Listener_LocalDirection, eye, kEye, item);
    vec3_w1(c10, eye);
    for (u32 i = 0; i < 3; ++i) call(s2::fn::cellGcmSetVertexProgramParameterBlock, gcm(), 7 + i, 1u, rows + 4 * i);
    vconsts(10, 4, c10);
    Vec3_ToItemSpace(dir, reinterpret_cast<const be_f32*>(item));
    vec3_w1(c23, dir);
    vconsts(23, 4, c23);
    vconsts(16, 4, c16);
    vconsts(20, 4, c20);
    lit_draw(item, k + 4, Stencil::Plain);
}

}  // namespace sly4::gfx

NATIVE(005B5150, sly4::gfx::AngleIndex_Draw)
NATIVE(005AEB00, sly4::gfx::TextureWarp_Draw)
NATIVE(005B0FB0, sly4::gfx::ProjTxr_Draw)
NATIVE(005B48E8, sly4::gfx::SelfLight_Draw)
NATIVE(005D2614, sly4::gfx::SlyWater_Draw)
NATIVE(005B5E00, sly4::gfx::Water_Draw)
NATIVE(005CDC38, sly4::gfx::SlyShrub_Draw)
NATIVE(005C13DC, sly4::gfx::SlyCharacterFurFin_Draw)
NATIVE(005D17E8, sly4::gfx::SlyTextureWarp_Draw)
NATIVE(005C39C8, sly4::gfx::SlyDecal_Draw)
NATIVE(00555370, sly4::gfx::Matrix_RotateRows)
NATIVE(005B67C8, sly4::gfx::SAC_Skinned_Draw)
