// More shader Draw slots (Shader vtable slot 12):
// void <Shader>_Draw(Shader* s, DrawItem* item)
#include "shader4_util.h"

using namespace sly4;
using namespace sly4::render::shader4::util;

namespace {

namespace s4 = sly4::render::shader4;

constexpr u32 kLightDir = 0x010DD8A0;  // layers3::g::block_dd780 +288: f32[3] the light direction (name?)
constexpr u32 kViewRotation = 0x010DD840;  // layers3::g::block_dd780 +192: f32[4][4] (name?)
constexpr u32 kEye = 0x010DD8B0;           // layers3::g::block_dd780 +304: the camera position
constexpr u32 kFurLight = 0x00F66FD0;      // u32: the fur shaders' light record (+32 f32[3] direction, +48 f32[4]) or 0 (name?)
constexpr u32 kProjectorBlock = 0x01196A00;  // passed to ProjTxr_BuildProjection (name?)

// Vertex constants 4..7 = the item rows through kViewRotation (lvx
// rows; row 3 adding its row 3 with vaddfp; w 0, 0, 0, 1) and 8 = {the
// item's row 1 (lvlx words) through it, 1}. (SlyWater / Water.)
void upload_rotated_rows(PPUContext& ctx, u32 item) {
    {
        f32 r[4][4];
        for (u32 i = 0; i < 4; ++i) {
            const u32 row = (item & ~15u) + 16 * i;
            rows_times(r[i], kViewRotation, load<f32>(row), load<f32>(row + 4), load<f32>(row + 8));
        }
        for (u32 k = 0; k < 4; ++k) r[3][k] = load<f32>(kViewRotation + 48 + 4 * k) + r[3][k];  // vaddfp
        Stack frame(ctx, 64);  // the original's sp+208 (SlyWater) / sp+176 (Water)
        const u32 v = frame.addr();
        for (u32 i = 0; i < 4; ++i) {
            for (u32 k = 0; k < 3; ++k) store<f32>(v + 16 * i + 4 * k, r[i][k]);
            store<u32>(v + 16 * i + 12, i == 3 ? s2::kOne : 0u);
        }
        set_vconsts(ctx, 4, 16, v);
    }
    f32 up[4];
    const auto word = [](u32 ea) { return std::bit_cast<f32>(lvlx_word0(ea)); };  // lvlx, vspltw 0
    rows_times(up, kViewRotation, word(item + 16), word(item + 20), word(item + 24));
    Stack frame(ctx, 16);  // the original's sp+112
    const u32 v = frame.addr();
    for (u32 k = 0; k < 3; ++k) store<f32>(v + 4 * k, up[k]);
    store<u32>(v + 12, s2::kOne);
    set_vconsts(ctx, 8, 4, v);
}

}  // namespace

// AngleIndex: the item transform, the view fog (DrawItem_SetParticleState
// (item, 32)), fragment constant 3 = {the light direction in the item's
// frame (Vec3_RotateTo), 1}, then the lit draw (stencil without 0x40).
DECOMP(005B5150, AngleIndex_Draw) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    call(ctx, s2::fn::DrawItem_SetParticleState, {item, 32});
    Stack frame(ctx, 32);  // the original's sp+112 (direction) and sp+144 (constant)
    const u32 dir = frame.addr(), c = dir + 16;
    call(ctx, s4::fn::Vec3_RotateTo, {dir, kLightDir, item});
    for (u32 i = 0; i < 12; i += 4) sp::copy_word(c + i, dir + i);
    store<u32>(c + 12, s2::kOne);
    call(ctx, d3::fn::FProgram_SetConstant, {3, c});
    lit_draw(ctx, item, 0x005B5148, Stencil::Plain);
}

// TextureWarp: the item transform, the view fog, the layers, then the
// lit draw with fragment constant 8 and neither mirroring nor stencil.
DECOMP(005AEB00, TextureWarp_Draw) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    call(ctx, s2::fn::DrawItem_SetParticleState, {item, 32});
    upload_layers(ctx, item);
    lit_draw(ctx, item, 0x005AEAFC, Stencil::None, 8, false);
}

// ProjTxr: the item transform and the view fog; the projector from the
// material parameters (+0.. three vec3s, +36 near, +40 far, +44 fov, +48
// aspect; all 0.0 without parameters): Matrix_Perspective, then
// ProjTxr_BuildProjection's matrix as vertex constants 4..7 and its
// position (w 1) as 8; the layers, then the mesh's draw.
DECOMP(005B0FB0, ProjTxr_Draw) {
    constexpr u32 kZero = 0x005B0FAC;  // f32 0.0 in .text
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    apply_fog(ctx, item);
    const u32 params = load<u32>(load<u32>(item + d3::kItemMaterial) + d3::kMatParams);
    const auto p = [&](u32 off) { return params != 0 ? params + off : kZero; };
    // The original's sp+112 / +128 / +144 (vectors), +160 (position), +176
    // (constant 8), +192 (matrix), +256 (projection).
    Stack frame(ctx, 208);
    const u32 a = frame.addr(), b = a + 16, c = a + 32, pos = a + 48, c8 = a + 64, m = a + 80, proj = a + 144;
    for (u32 i = 0; i < 12; i += 4) {
        sp::copy_word(a + i, p(i));
        sp::copy_word(b + i, p(12 + i));
        sp::copy_word(c + i, p(24 + i));
    }
    callf(ctx, s4::fn::Matrix_Perspective, {proj}, {sp::lfs(p(44)), sp::lfs(p(48)), sp::lfs(p(36)), sp::lfs(p(40))});
    call(ctx, s4::fn::ProjTxr_BuildProjection, {pos, m, a, b, c, proj, item, kProjectorBlock});
    set_vconsts(ctx, 4, 16, m);
    for (u32 i = 0; i < 12; i += 4) sp::copy_word(c8 + i, pos + i);
    store<u32>(c8 + 12, s2::kOne);
    set_vconsts(ctx, 8, 4, c8);
    upload_layers(ctx, item);
    draw_mesh(ctx, item);
}

// SelfLight: the item transform, the view fog, the light (SelfLight_GetLight)
// as vertex constants 20 / 23 (its direction, w 1) / 26 and
// fx::g::zero_vconsts as 16, then the lit draw (stencil without 0x40).
DECOMP(005B48E8, SelfLight_Draw) {
    const u32 s = arg<u32>(ctx, 0), item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    call(ctx, s2::fn::DrawItem_SetParticleState, {item, 32});
    Stack frame(ctx, 64);  // the original's sp+112 / +128 / +144 and +160 (constant 23)
    const u32 a = frame.addr(), dir = a + 16, b = a + 32, c23 = a + 48;
    call(ctx, s4::fn::SelfLight_GetLight, {s, item, a, dir, b});
    for (u32 i = 0; i < 12; i += 4) sp::copy_word(c23 + i, dir + i);
    store<u32>(c23 + 12, s2::kOne);
    set_vconsts(ctx, 20, 4, a);
    set_vconsts(ctx, 16, 4, fx::g::zero_vconsts);
    set_vconsts(ctx, 23, 4, c23);
    set_vconsts(ctx, 26, 4, b);
    lit_draw(ctx, item, 0x005B48E4, Stencil::Plain);
}

// SlyWater: the item transform and the view fog; for a lit item (variant
// +69 bit 0) fragment constants 17 / 18 = the layer list's +48 / +32
// (unchecked list) and 10 = {the camera in the item's frame, 1}; vertex
// constants 4..7 = the item rows through kViewRotation (row 3 adding its
// row 3; w 0, 0, 0, 1) and 8 = {the item's row 1 through it, 1}; the
// stencil reference ((stencil bit 7 | 0x20 | material +101) & the write
// mask), then the mesh's draw.
DECOMP(005D2614, SlyWater_Draw) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    apply_fog(ctx, item);
    if ((ref<u8>(item + d3::kItemVariant) & 1) != 0) {
        const u32 list = load<u32>(item + d3::kItemLayers);
        Stack frame(ctx, 64);  // the original's sp+144 / +160 / +176 / +192
        const u32 a = frame.addr(), b = a + 16, dir = a + 32, c10 = a + 48;
        raw4(a, list + 32);
        raw4(b, list + 48);
        call(ctx, d3::fn::FProgram_SetConstant, {17, b});
        call(ctx, d3::fn::FProgram_SetConstant, {18, a});
        call(ctx, s4::fn::Listener_LocalDirection, {dir, kEye, item});
        for (u32 i = 0; i < 12; i += 4) sp::copy_word(c10 + i, dir + i);
        store<u32>(c10 + 12, s2::kOne);
        call(ctx, d3::fn::FProgram_SetConstant, {10, c10});
    }
    upload_rotated_rows(ctx, item);
    const u32 flags = ref<u8>(item + kItemFlags);
    const u32 mat = load<u32>(item + d3::kItemMaterial);
    const u32 value = (((flags & kStencilHigh) != 0 ? 0x80u : 0u) | 0x20u | ref<u8>(mat + kMatStencil)) & ref<u8>(render::g::stencil_bytes + 1);
    if (value != ref<u8>(render::g::stencil_bytes)) set_stencil_ref(value);
    draw_mesh(ctx, item);
}

// Water: the item transform, the view fog, vertex constants 4..8 as
// SlyWater's, 10 = {the item position (+48), 1}, then the lit draw with
// fragment constant 8 and neither mirroring nor stencil.
DECOMP(005B5E00, Water_Draw) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    call(ctx, s2::fn::DrawItem_SetParticleState, {item, 32});
    upload_rotated_rows(ctx, item);
    {
        Stack frame(ctx, 16);  // the original's sp+160
        const u32 v = frame.addr();
        for (u32 i = 0; i < 12; i += 4) sp::copy_word(v + i, item + 48 + i);
        store<u32>(v + 12, s2::kOne);
        set_vconsts(ctx, 10, 4, v);
    }
    lit_draw(ctx, item, 0x005B5DFC, Stencil::None, 8, false);
}

// SlyShrub: the item transform and the view fog; vertex constants 4 =
// {1, 1, 1, alpha * k}, 5 = {the item position (+48), 1}, 6..8 = {params
// +4 + 8n, 1, params +8 + 8n, w} (w: g::material_time for 6, else k1;
// k1 for missing parameters); the alpha test (GREATER than parameters
// +0, s32) on while the draw runs when that is positive; the mirroring
// and stencil of the lit draw (without 0x40), then the mesh's draw.
DECOMP(005CDC38, SlyShrub_Draw) {
    constexpr u32 k = 0x005CDC2C;  // .text: f32 1/255, then k1
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    apply_fog(ctx, item);
    Stack frame(ctx, 48);  // the original's sp+112 (constant 4), +128 (6..8), +144 (5)
    const u32 c4 = frame.addr(), cn = c4 + 16, c5 = c4 + 32;
    for (u32 i = 0; i < 12; i += 4) store<u32>(c4 + i, s2::kOne);
    sp::stfs(c4 + 12, byte_scaled(ref<u8>(item + kItemAlpha), sp::lfs(k)));  // fcfid, frsp, fmuls
    set_vconsts(ctx, 4, 4, c4);
    for (u32 i = 0; i < 12; i += 4) sp::copy_word(c5 + i, item + 48 + i);
    store<u32>(c5 + 12, s2::kOne);
    set_vconsts(ctx, 5, 4, c5);
    const u32 mat = load<u32>(item + d3::kItemMaterial);
    for (u32 n = 0; n < 3; ++n) {
        const u32 params = load<u32>(mat + d3::kMatParams);
        sp::copy_word(cn, params != 0 ? params + 4 + 8 * n : k + 4);
        store<u32>(cn + 4, s2::kOne);
        sp::copy_word(cn + 8, params != 0 ? params + 8 + 8 * n : k + 4);
        sp::copy_word(cn + 12, n == 0 ? s3::g::material_time : k + 4);
        set_vconsts(ctx, 6 + n, 4, cn);
    }
    const u32 params = load<u32>(mat + d3::kMatParams);
    const s32 ref_alpha = params != 0 ? load<s32>(params) : 0;
    const u32 alpha_test = ref_alpha > 0 ? 1u : 0u;  // cmpwi
    if (alpha_test != 0) fx::emit({s2::kAlphaFunc2, s2::kGreater, static_cast<u32>(ref_alpha)});
    fx::emit({l4::kAlphaTest, alpha_test});
    u32 flags = ref<u8>(item + kItemFlags);
    if ((flags & kMirrored) != 0) {
        fx::emit({l4::kCullEnable, 1});
        fx::emit({l4::kFrontFace, l4::kCw});
        flags = ref<u8>(item + kItemFlags);
    }
    const u32 value = (((flags & kStencilHigh) != 0 ? 0x80u : 0u) | ref<u8>(load<u32>(item + d3::kItemMaterial) + kMatStencil)) & ref<u8>(render::g::stencil_bytes + 1);
    if (value != ref<u8>(render::g::stencil_bytes)) set_stencil_ref(value);
    draw_mesh(ctx, item);
    const bool mirrored = (ref<u8>(item + kItemFlags) & kMirrored) != 0;
    if (mirrored) {
        fx::emit({l4::kCullEnable, 1});
        fx::emit({l4::kFrontFace, l4::kCcw});
    }
    if (alpha_test != 0) fx::emit({l4::kAlphaTest, 0});
    ctx.gpr[3] = mirrored || alpha_test != 0 ? gcm() : 0;  // r3 as the original leaves it
}

// The stencil reference without 0x40: ((stencil bit 7 | material +101) &
// the write mask), as the lit draw's Stencil::Plain.
void plain_stencil(u32 item) {
    const u32 flags = ref<u8>(item + kItemFlags);
    const u32 value = (((flags & kStencilHigh) != 0 ? 0x80u : 0u) | ref<u8>(load<u32>(item + d3::kItemMaterial) + kMatStencil)) & ref<u8>(render::g::stencil_bytes + 1);
    if (value != ref<u8>(render::g::stencil_bytes)) set_stencil_ref(value);
}

// SlyCharacterFurFin: the item transform and the view fog; with a fur
// light (kFurLight) vertex constants 5 = its +48 and 6 = {its direction
// in the item's frame, 1} (fx::g::zero_vconsts for both without one);
// for a lit item (variant +69 bit 0) the layer list (unchecked) as vertex
// constants 16.. and 4 = {the camera in the item's frame, 1}; the stencil
// (without 0x40), then the mesh's draw.
DECOMP(005C13DC, SlyCharacterFurFin_Draw) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    apply_fog(ctx, item);
    if (const u32 light = load<u32>(kFurLight); light != 0) {
        Stack frame(ctx, 48);  // the original's sp+128 / +144 / +176
        const u32 in = frame.addr(), dir = in + 16, c6 = in + 32;
        for (u32 i = 0; i < 12; i += 4) sp::copy_word(in + i, light + 32 + i);
        call(ctx, s4::fn::Vec3_RotateTo, {dir, in, item});
        set_vconsts(ctx, 5, 4, light + 48);
        for (u32 i = 0; i < 12; i += 4) sp::copy_word(c6 + i, dir + i);
        store<u32>(c6 + 12, s2::kOne);
        set_vconsts(ctx, 6, 4, c6);
    } else {
        set_vconsts(ctx, 5, 4, fx::g::zero_vconsts);
        set_vconsts(ctx, 6, 4, fx::g::zero_vconsts);
    }
    if ((ref<u8>(item + d3::kItemVariant) & 1) != 0) {
        upload_layer_list(ctx, load<u32>(item + d3::kItemMaterial), load<u32>(item + d3::kItemLayers));
        Stack frame(ctx, 32);  // the original's sp+160 / +112
        const u32 dir = frame.addr(), c4 = dir + 16;
        call(ctx, s4::fn::Listener_LocalDirection, {dir, kEye, item});
        for (u32 i = 0; i < 12; i += 4) sp::copy_word(c4 + i, dir + i);
        store<u32>(c4 + 12, s2::kOne);
        set_vconsts(ctx, 4, 4, c4);
    }
    plain_stencil(item);
    draw_mesh(ctx, item);
}

// SlyTextureWarp: the item transform, the view fog, the layers, then the
// lit draw without mirroring (stencil without 0x40).
DECOMP(005D17E8, SlyTextureWarp_Draw) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    apply_fog(ctx, item);
    upload_layers(ctx, item);
    lit_draw(ctx, item, 0x005D17E0, Stencil::Plain, 2, false);
}

// SlyDecal: the item transform and the view fog; for a lit item (variant
// +69 bit 0) the layer list (unchecked) as fragment constants 16.. when
// variant bit 1 is set, else as vertex constants 16.., and with variant
// bit 2 fragment constant 1 = {the camera in the item's frame, 1}; the
// stencil (without 0x40), then the mesh's draw.
DECOMP(005C39C8, SlyDecal_Draw) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    apply_fog(ctx, item);
    if ((ref<u8>(item + d3::kItemVariant) & 1) != 0) {
        if ((ref<u8>(item + d3::kItemVariant) & 2) != 0) {
            upload_layers_fragment(ctx, item);
        } else {
            upload_layer_list(ctx, load<u32>(item + d3::kItemMaterial), load<u32>(item + d3::kItemLayers));
        }
        if ((ref<u8>(item + d3::kItemVariant) & 4) != 0) {
            Stack frame(ctx, 32);  // the original's sp+112 / +128
            const u32 dir = frame.addr(), c1 = dir + 16;
            call(ctx, s4::fn::Listener_LocalDirection, {dir, kEye, item});
            for (u32 i = 0; i < 12; i += 4) sp::copy_word(c1 + i, dir + i);
            store<u32>(c1 + 12, s2::kOne);
            call(ctx, d3::fn::FProgram_SetConstant, {1, c1});
        }
    }
    plain_stencil(item);
    draw_mesh(ctx, item);
}

// void Matrix_RotateRows(f32 out[3][4], const f32 rows[3][4], const f32 m[4][4])
// out row i = m0 * rows[i][0] + m1 * rows[i][1] + m2 * rows[i][2] per
// lane (vmaddfp; rows read as lvlx words, m and out 16-byte aligned with
// lvx / stvx, m re-read for each row).
DECOMP(00555370, Matrix_RotateRows) {
    const u32 out = arg<u32>(ctx, 0) & ~15u, rows = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    const auto word = [](u32 ea) { return std::bit_cast<f32>(lvlx_word0(ea)); };
    for (u32 i = 0; i < 3; ++i) {
        f32 r[4];
        rows_times(r, m, word(rows + 16 * i), word(rows + 16 * i + 4), word(rows + 16 * i + 8));
        for (u32 k = 0; k < 4; ++k) store<f32>(out + 16 * i + 4 * k, r[k]);
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
DECOMP(005B67C8, SAC_Skinned_Draw) {
    constexpr u32 k = 0x005B67BC;  // .text: f32 1.0, 1/255, 0.0
    constexpr u32 cfg = sly4::g::config;
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    call(ctx, d3::fn::Draw_SetParticleState, {0, 0, 32});
    // The original's sp+112 (23), +128 (16), +144 (20), +160 (direction),
    // +176 (camera), +192 (10), +224.. (rows).
    Stack frame(ctx, 160);
    const u32 c23 = frame.addr(), c16 = c23 + 16, c20 = c23 + 32, dir = c23 + 48, eye = c23 + 64, c10 = c23 + 80, rows = c23 + 96;
    for (u32 i = 0; i < 12; i += 4) {
        sp::copy_word(c16 + i, cfg + 176 + i);
        sp::copy_word(c20 + i, cfg + 192 + i);
        sp::copy_word(dir + i, cfg + 208 + i);
    }
    store<u32>(c20 + 12, s2::kOne);
    const u32 mat = load<u32>(item + d3::kItemMaterial);
    const bool layered = ((ref<u8>(mat + d3::kMatFlags) >> 7) & 1) != 0 && load<u32>(item + d3::kItemLayers) != 0;
    if (layered) {
        sp::copy_word(c16 + 12, k);
    } else {
        const f64 scale = sp::lfs(k + 4);
        for (u32 i = 0; i < 3; ++i) {
            sp::stfs(c16 + 4 * i, sp::sgl(sp::lfs(mat + 64 + 4 * i) * byte_scaled(ref<u8>(item + 72 + i), scale)));  // fmuls
        }
        sp::copy_word(c16 + 12, k + 8);
    }
    call(ctx, s4::fn::Matrix_RotateRows, {rows, item, kViewRotation});
    call(ctx, s4::fn::Listener_LocalDirection, {eye, kEye, item});
    for (u32 i = 0; i < 12; i += 4) sp::copy_word(c10 + i, eye + i);
    store<u32>(c10 + 12, s2::kOne);
    for (u32 i = 0; i < 3; ++i) call(ctx, s2::fn::cellGcmSetVertexProgramParameterBlock, {gcm(), 7 + i, 1, rows + 16 * i});
    set_vconsts(ctx, 10, 4, c10);
    call(ctx, s3::fn::Vec3_ToItemSpace, {dir, item});
    for (u32 i = 0; i < 12; i += 4) sp::copy_word(c23 + i, dir + i);
    store<u32>(c23 + 12, s2::kOne);
    set_vconsts(ctx, 23, 4, c23);
    set_vconsts(ctx, 16, 4, c16);
    set_vconsts(ctx, 20, 4, c20);
    lit_draw(ctx, item, k + 4, Stencil::Plain);
}
