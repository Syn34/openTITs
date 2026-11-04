// The remaining shader Draw slots (Shader vtable slot 12):
// void <Shader>_Draw(Shader* s, DrawItem* item)
#include "shader5.h"

using namespace sly4;
using namespace sly4::render::shader4::util;

namespace {

namespace s4 = sly4::render::shader4;
namespace s5 = sly4::render::shader5;
namespace l4 = sly4::render::layers4;

constexpr u32 kLightDir = 0x010DD8A0;   // view block +288: the camera rotation's column 2 (shader4_draw.cpp kLightDir)
constexpr u32 kEye = 0x010DD8B0;        // view block +304: the camera position (render::g::camera_pos)
constexpr u32 kViewProj2 = 0x010DD800;  // view block +128: f32[4][4] (view +944) (name?)

constexpr u32 SlyWater3_UploadFragment = 0x005D4FF0;  // (Material*) SlyWater3's fragment constant 3 (name?)
constexpr u32 SlyHeightFog_BuildMatrices = 0x005C6EB8;  // (Material*, f32 a[3][4], f32 b[3][4]) (Matrix_FromAxisAngle, Matrix_ScaleRows) (name?)
constexpr u32 kCollectibleLight = 0x01196F80;  // f32[4] ambient, f32[4] diffuse, f32[3] direction (shader3_typeinit.cpp kColours) (name?)
constexpr u32 kViewRotation = 0x010DD840;      // view block +192 (shader4_draw.cpp)
constexpr u32 kViewColumns = 0x010DD880;       // view block +256: the camera rotation transposed (3 rows)
constexpr u32 Matrix_Normalize = 0x0056D268;   // (f32 dst[4][4], const f32 src[4][4]) (obj/obj10.cpp)
constexpr u32 View_GetCurrent = 0x004FDBA8;  // () -> View* (+864 f32[3] its camera position)

// {xyz of src, 1.0} into dst.
void vec_w1(u32 dst, u32 src) {
    for (u32 i = 0; i < 12; i += 4) sp::copy_word(dst + i, src + i);
    store<u32>(dst + 12, s2::kOne);
}

// The item's 4x4 transposed (column j = word 0 of lvlx at item + 16i +
// 4j), row 3 the vector at `unit` (lvx: {0, 0, 0, 1}), into dst.
void item_columns(u32 dst, u32 item, u32 unit) {
    for (u32 j = 0; j < 3; ++j) {
        for (u32 i = 0; i < 4; ++i) store<u32>(dst + 16 * j + 4 * i, lvlx_word0(item + 16 * i + 4 * j));  // vmrghw, vperm
    }
    raw4(dst + 48, unit & ~15u);
}

// The stencil reference with 0x20: ((stencil bit 7 | 0x20 | material
// +101) & the write mask).
void stencil_with20(u32 item) {
    const u32 flags = ref<u8>(item + kItemFlags);
    const u32 mat = load<u32>(item + d3::kItemMaterial);
    const u32 value = (((flags & kStencilHigh) != 0 ? 0x80u : 0u) | 0x20u | ref<u8>(mat + kMatStencil)) & ref<u8>(render::g::stencil_bytes + 1);
    if (value != ref<u8>(render::g::stencil_bytes)) set_stencil_ref(value);
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
void water_draw(PPUContext& ctx, u32 item, u32 k, bool water3) {
    const u32 mat = load<u32>(item + d3::kItemMaterial);
    if ((ref<u8>(item + d3::kItemVariant) & 0x10) != 0) {  // rlwinm 0,27,27
        // The original's sp+240 (SlyWater2) / sp+224 (SlyWater3), and
        // SlyWater3's copy at sp+288.
        Stack frame(ctx, water3 ? 128 : 64);
        const u32 cols = frame.addr();
        item_columns(cols, item, k + 16);
        u32 src = kViewProj2;
        if (water3) {
            src = cols + 64;
            for (u32 i = 0; i < 64; i += 4) sp::copy_word(src + i, kViewProj2 + i);
        }
        set_vconsts(ctx, 0, 16, src);
        set_vconsts(ctx, 4, 16, cols);
    } else {
        upload_item_transform(item);
    }
    apply_fog(ctx, item);
    // The original's sp+128 (camera), +144 (light), +224 / +208 (constants
    // 8 / 9; SlyWater3 the same).
    Stack frame(ctx, 64);
    const u32 eye = frame.addr(), dir = eye + 16, c8 = eye + 32, c9 = eye + 48;
    call(ctx, s4::fn::Listener_LocalDirection, {eye, kEye, item});
    call(ctx, s4::fn::Vec3_RotateTo, {dir, kLightDir, item});
    vec_w1(c8, eye);
    set_vconsts(ctx, 8, 4, c8);
    vec_w1(c9, dir);
    set_vconsts(ctx, 9, 4, c9);
    if ((ref<u8>(item + d3::kItemVariant) & 0x28) != 0) {
        {
            Stack c(ctx, 16);  // the original's sp+192 (SlyWater3 sp+176)
            vec_w1(c.addr(), eye);
            call(ctx, d3::fn::FProgram_SetConstant, {2, c.addr()});
        }
        if ((ref<u8>(item + d3::kItemVariant) & 0x20) != 0) {  // rlwinm 0,26,26
            upload_layer_list(ctx, mat, load<u32>(item + d3::kItemLayers));
        }
        if ((ref<u8>(item + d3::kItemVariant) & 0x08) != 0) {  // rlwinm 0,28,28
            if (water3) {
                call(ctx, SlyWater3_UploadFragment, {mat});
            } else {
                Stack c(ctx, 16);  // the original's sp+176
                for (u32 i = 0; i < 16; i += 4) {
                    const u32 params = load<u32>(mat + d3::kMatParams);
                    sp::copy_word(c.addr() + i, params != 0 ? params + 84 + i : k + 16);  // 0.0 without parameters
                }
                call(ctx, d3::fn::FProgram_SetConstant, {3, c.addr()});
            }
        }
    }
    {
        Stack c(ctx, 32);  // the original's sp+160 (direction) and sp+112 (constant 12)
        const u32 down = c.addr(), c12 = down + 16;
        store<u32>(down, 0);
        store<u32>(down + 4, 0);
        store<u32>(down + 8, 0xBF800000u);  // -1.0
        call(ctx, s4::fn::Vec3_RotateTo, {down, item + 16, item});
        vec_w1(c12, down);
        set_vconsts(ctx, 12, 4, c12);
    }
    stencil_with20(item);
    draw_mesh(ctx, item);
}

// For a lit item (variant +69 bit 0) the layer list (unchecked) as
// fragment constants 16.. when variant bit 1 is set, else as vertex
// constants 16..; with variant bit 2 fragment constant 1 = {the camera in
// the item's frame, 1}. (SlyDecal_Draw's block, shader4_draw.cpp.)
void lit_layers(PPUContext& ctx, u32 item) {
    if ((ref<u8>(item + d3::kItemVariant) & 1) == 0) return;
    if ((ref<u8>(item + d3::kItemVariant) & 2) != 0) {
        upload_layers_fragment(ctx, item);
    } else {
        upload_layer_list(ctx, load<u32>(item + d3::kItemMaterial), load<u32>(item + d3::kItemLayers));
    }
    if ((ref<u8>(item + d3::kItemVariant) & 4) != 0) {
        Stack frame(ctx, 32);  // the original's sp+112 / +128
        const u32 dir = frame.addr(), c1 = dir + 16;
        call(ctx, s4::fn::Listener_LocalDirection, {dir, kEye, item});
        vec_w1(c1, dir);
        call(ctx, d3::fn::FProgram_SetConstant, {1, c1});
    }
}

// Vertex constants 0..3 = a stack copy of kViewProj2 and 4..7 = the item
// transposed (item_columns), into `frame` (128 bytes: the original's
// sp+208 / +272).
void upload_columns_copy(PPUContext& ctx, u32 frame, u32 item, u32 unit) {
    item_columns(frame, item, unit);
    for (u32 i = 0; i < 64; i += 4) sp::copy_word(frame + 64 + i, kViewProj2 + i);
    set_vconsts(ctx, 0, 16, frame + 64);
    set_vconsts(ctx, 4, 16, frame);
}

// lit_draw (shader4_util.h) with the stencil reference ((stencil bit 7 |
// extra | material +101) & the write mask), fragment constant `fconst`
// first set from `first`, and the translucent constant's lane 2 = z_word
// (lit_draw: 0).
void lit_draw_ex(PPUContext& ctx, u32 item, u32 k, u32 extra, u32 fconst, u32 first, u32 z_word) {
    call(ctx, d3::fn::FProgram_SetConstant, {fconst, first});
    if (ref<u8>(item + kItemAlpha) < 255) {  // cmpwi
        fx::emit({l4::kDepthMask, 0});
        Stack frame(ctx, 16);
        const u32 v = frame.addr();
        store<u32>(v, 0);
        store<u32>(v + 8, z_word);
        store<u32>(v + 12, 0);
        sp::stfs(v + 4, byte_scaled(ref<u8>(item + kItemAlpha), sp::lfs(k)));  // fcfid, frsp, fmuls
        if (!mat_bit31(load<u32>(item + d3::kItemMaterial), kMatOwnBlend)) {
            fx::blend_on(fx::kSrcAlpha2, fx::kOneMinusSrcAlpha2);
            store<u32>(v, s2::kOne);
        }
        call(ctx, d3::fn::FProgram_SetConstant, {fconst, v});
    }
    u32 flags = ref<u8>(item + kItemFlags);
    if ((flags & kMirrored) != 0) {  // rlwinm 25,31,31
        fx::emit({l4::kCullEnable, 1});
        fx::emit({l4::kFrontFace, l4::kCw});
        flags = ref<u8>(item + kItemFlags);
    }
    const u32 mat = load<u32>(item + d3::kItemMaterial);
    const u32 value = (((flags & kStencilHigh) != 0 ? 0x80u : 0u) | extra | ref<u8>(mat + kMatStencil)) & ref<u8>(render::g::stencil_bytes + 1);
    if (value != ref<u8>(render::g::stencil_bytes)) set_stencil_ref(value);
    draw_mesh(ctx, item);
    if ((ref<u8>(item + kItemFlags) & kMirrored) != 0) {
        fx::emit({l4::kCullEnable, 1});
        fx::emit({l4::kFrontFace, l4::kCcw});
    }
    // r3 as the original leaves it (lit_draw).
    if (ref<u8>(item + kItemAlpha) >= 255) {
        ctx.gpr[3] = ref<u8>(item + kItemAlpha);
        return;
    }
    if (mat_bit31(load<u32>(item + d3::kItemMaterial), kMatDepthRestore)) {
        fx::emit({l4::kDepthMask, ref<u8>(fx::g::state + 93) & 1u});
    }
    if (!mat_bit31(load<u32>(item + d3::kItemMaterial), kMatOwnBlend)) {
        fx::emit({l4::kBlendEnable, 0});
        ctx.gpr[3] = gcm();
    } else {
        ctx.gpr[3] = 1;
    }
}

void lit_draw_bits(PPUContext& ctx, u32 item, u32 k, u32 extra) { lit_draw_ex(ctx, item, k, extra, 2, s3::g::character_fconst2, 0); }

// The 3x3 rotation vmaddfp order: kViewColumns rows times (x, y, z).
void view_columns_times(u32 dst, f32 x, f32 y, f32 z) {
    for (u32 k = 0; k < 4; ++k) {
        f32 v = std::fmaf(load<f32>(kViewColumns + 4 * k), x, 0.0f);
        v = std::fmaf(load<f32>(kViewColumns + 16 + 4 * k), y, v);
        store<f32>(dst + 4 * k, std::fmaf(load<f32>(kViewColumns + 32 + 4 * k), z, v));
    }
}

// SAC_EnvMap / EnvMap / SlyEnvMap (see SAC_EnvMap_Draw): no fog, the
// particle-state fog or the view fog.
enum class Fog { None, Particle, View };
void env_draw(PPUContext& ctx, u32 item, u32 k, Fog fog) {
    upload_item_transform(item);
    if (fog == Fog::Particle) call(ctx, s2::fn::DrawItem_SetParticleState, {item, 32});
    if (fog == Fog::View) apply_fog(ctx, item);
    // The original's sp+160 (rows), +112 (camera), +128 (c).
    Stack frame(ctx, 80);
    const u32 rows = frame.addr(), eye = rows + 48, c = rows + 64;
    call(ctx, s4::fn::Matrix_RotateRows, {rows, item, kViewRotation});
    call(ctx, s4::fn::Listener_LocalDirection, {eye, kEye, item});
    vec_w1(c, eye);
    const u32 variant = ref<u8>(item + d3::kItemVariant);
    if ((variant & 1) != 0 && (variant & 2) != 0) {
        upload_layers_fragment(ctx, item);
        for (u32 i = 0; i < 3; ++i) call(ctx, d3::fn::FProgram_SetConstant, {2 + i, rows + 16 * i});
        call(ctx, d3::fn::FProgram_SetConstant, {5, c});
    } else {
        if ((variant & 1) != 0) upload_layer_list(ctx, load<u32>(item + d3::kItemMaterial), load<u32>(item + d3::kItemLayers));
        for (u32 i = 0; i < 3; ++i) call(ctx, s2::fn::cellGcmSetVertexProgramParameterBlock, {gcm(), 7 + i, 1, rows + 16 * i});
        set_vconsts(ctx, 10, 4, c);
    }
    if (fog != Fog::View) {
        lit_draw(ctx, item, k, Stencil::Plain, 1);
        return;
    }
    // SlyEnvMap: fragment constant 1 = g::character_fconst2 with lane 2 =
    // material parameters +12 (k: 0.0 without), the same lane 2 in the
    // translucent constant.
    const u32 params = load<u32>(load<u32>(item + d3::kItemMaterial) + d3::kMatParams);
    const u32 z = params != 0 ? load<u32>(params + 12) : load<u32>(k);
    Stack first(ctx, 16);  // the original's sp+144
    raw4(first.addr(), s3::g::character_fconst2);
    store<u32>(first.addr() + 8, z);
    lit_draw_ex(ctx, item, k + 4, 0, 1, first.addr(), z);
}

// The layer list of a lit item (variant +69 bit 0; unchecked) as
// fragment constants 16.. with variant bit 1, else vertex constants 16...
void lit_layer_list(PPUContext& ctx, u32 item) {
    if ((ref<u8>(item + d3::kItemVariant) & 2) != 0) {
        upload_layers_fragment(ctx, item);
    } else {
        upload_layer_list(ctx, load<u32>(item + d3::kItemMaterial), load<u32>(item + d3::kItemLayers));
    }
}

// SlyTerrain's distance blend (variant bit 3): t = (params +12 - |the
// camera - the item position (+48)|) / params +16; u1 = clamp(2t, 0, 1),
// u2 = clamp(2(t - 0.5), 0, 1); fragment constant 0 = {u1 * params +0,
// params +4, u2 * params +8, 0} (0.0 for missing parameters).
void terrain_blend(PPUContext& ctx, u32 item, u32 k) {
    const u32 mat = load<u32>(item + d3::kItemMaterial), pos = item + 48;
    const f64 dy = sp::sgl(sp::lfs(kEye + 4) - sp::lfs(pos + 4));
    const f64 dx = sp::sgl(sp::lfs(kEye) - sp::lfs(pos));
    const f64 dz = sp::sgl(sp::lfs(kEye + 8) - sp::lfs(pos + 8));
    const u32 params = load<u32>(mat + d3::kMatParams);
    const auto p = [&](u32 off) { return params != 0 ? params + off : k; };
    const f64 sq = sp::fmadds(dz, dz, sp::fmadds(dx, dx, sp::sgl(dy * dy)));
    const f64 d = sq >= 0.0 ? sp::sgl(std::sqrt(sq)) : std::nan("");  // fsqrts
    const f64 t = sp::sgl(sp::sgl(sp::lfs(p(12)) - d) / sp::lfs(p(16)));
    const f64 half = sp::lfs(k + 4), two = sp::lfs(k + 8), one = sp::lfs(k + 12);
    const f64 one_d = load<f64>(k + 16), zero_d = load<f64>(k + 24);
    f64 a = sp::sgl(t * two);
    f64 b = sp::sgl(sp::sgl(t - half) * two);
    a = sp::sgl(sp::sel_ge(sp::sgl(one - a), a, one_d));  // fsel: min(.., 1)
    b = sp::sgl(sp::sel_ge(sp::sgl(one - b), b, one_d));
    a = sp::sgl(sp::sel_ge(a, a, zero_d));  // fsel: max(.., 0)
    b = sp::sgl(sp::sel_ge(b, b, zero_d));
    Stack frame(ctx, 16);  // the original's sp+112
    const u32 c = frame.addr();
    sp::stfs(c, sp::sgl(a * sp::lfs(p(0))));
    sp::copy_word(c + 4, p(4));
    store<u32>(c + 12, load<u32>(k));
    sp::stfs(c + 8, sp::sgl(b * sp::lfs(p(8))));
    call(ctx, d3::fn::FProgram_SetConstant, {0, c});
}

}  // namespace

// SlyTerrain: the item transform and the view fog; with variant +69 bit
// 3 the distance blend (terrain_blend) as fragment constant 0; the lit
// layers (lit_layers), the lit draw (stencil without 0x40), then with
// bit 3 fragment constant 0 back to {params +0, +4, +8, 0}.
DECOMP(005CC240, SlyTerrain_Draw) {
    constexpr u32 k = 0x005CC218;  // .text: f32 0.0, 0.5, 2.0, 1.0; f64 1.0, 0.0; f32 1/255
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    apply_fog(ctx, item);
    if ((ref<u8>(item + d3::kItemVariant) & 8) != 0) terrain_blend(ctx, item, k);  // rlwinm 0,28,28
    lit_layers(ctx, item);
    lit_draw(ctx, item, k + 32, Stencil::Plain);
    if ((ref<u8>(item + d3::kItemVariant) & 8) == 0) {
        ctx.gpr[3] = 0;  // as the original leaves it
        return;
    }
    const u32 params = load<u32>(load<u32>(item + d3::kItemMaterial) + d3::kMatParams);
    Stack frame(ctx, 16);  // the original's sp+160
    const u32 c = frame.addr();
    for (u32 i = 0; i < 12; i += 4) sp::copy_word(c + i, params != 0 ? params + i : k);
    store<u32>(c + 12, load<u32>(k));
    call(ctx, d3::fn::FProgram_SetConstant, {0, c});
}

// SlyCharacter: the item transform and the view fog; vertex constant
// parameter blocks 7..9 = the item rows through kViewRotation
// (Matrix_RotateRows), vertex constant 10 = c = {the camera in the item's
// frame, 1}; for a lit item (variant +69 bit 0) the layer list
// (lit_layer_list) and with variant bit 2 fragment constant 1 = c; then
// the lit draw, its stencil bit 0x40 set when the material parameters'
// word +28 is nonzero.
DECOMP(005BB4D8, SlyCharacter_Draw) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    apply_fog(ctx, item);
    // The original's sp+160 (rows), +112 (camera), +128 (c).
    Stack frame(ctx, 80);
    const u32 rows = frame.addr(), eye = rows + 48, c = rows + 64;
    call(ctx, s4::fn::Matrix_RotateRows, {rows, item, kViewRotation});
    call(ctx, s4::fn::Listener_LocalDirection, {eye, kEye, item});
    vec_w1(c, eye);
    for (u32 i = 0; i < 3; ++i) call(ctx, s2::fn::cellGcmSetVertexProgramParameterBlock, {gcm(), 7 + i, 1, rows + 16 * i});
    set_vconsts(ctx, 10, 4, c);
    if ((ref<u8>(item + d3::kItemVariant) & 1) != 0) {
        lit_layer_list(ctx, item);
        if ((ref<u8>(item + d3::kItemVariant) & 4) != 0) call(ctx, d3::fn::FProgram_SetConstant, {1, c});
    }
    const u32 params = load<u32>(load<u32>(item + d3::kItemMaterial) + d3::kMatParams);
    const u32 extra = params != 0 && load<u32>(params + 28) != 0 ? 0x40u : 0u;  // read after the mirroring in the original
    lit_draw_bits(ctx, item, 0x005BB4D0, extra);  // .text: f32 1/255
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
DECOMP(005C2880, SlyCharacterCollectible_Draw) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    apply_fog(ctx, item);
    // The original's sp+368 (n), +224 (rows), +112 (camera), +128 (constant 10).
    Stack frame(ctx, 128);
    const u32 n = frame.addr(), rows = n + 64, eye = n + 112;
    Stack frame2(ctx, 16);
    const u32 c10 = frame2.addr();
    call(ctx, Matrix_Normalize, {n, item});
    call(ctx, s4::fn::Matrix_RotateRows, {rows, n, kViewRotation});
    call(ctx, s4::fn::Listener_LocalDirection, {eye, kEye, n});
    vec_w1(c10, eye);
    for (u32 i = 0; i < 3; ++i) call(ctx, s2::fn::cellGcmSetVertexProgramParameterBlock, {gcm(), 7 + i, 1, rows + 16 * i});
    set_vconsts(ctx, 10, 4, c10);
    if ((ref<u8>(item + d3::kItemVariant) & 1) != 0) {
        const bool fragment = (ref<u8>(item + d3::kItemVariant) & 2) != 0;  // rlwinm 0,30,30
        const u32 g = kCollectibleLight;
        const u32 colour = load<u32>(item + d3::kItemMaterial) + kMatColour;
        // The original's sp+320.. / +192 / +208 (fragment) or sp+272.. / +160 / +176.
        Stack c(ctx, 80);
        const u32 v = c.addr(), in = v + 48, out = v + 64;
        for (u32 i = 0; i < 16; i += 4) {
            sp::stfs(v + i, sp::sgl(sp::lfs(g + i) * sp::lfs(colour + i)));  // fmuls
            sp::stfs(v + 16 + i, sp::sgl(sp::lfs(g + 16 + i) * sp::lfs(colour + i)));
        }
        const auto word = [](u32 ea) { return std::bit_cast<f32>(lvlx_word0(ea)); };  // lvlx, vspltw 0
        view_columns_times(in, word(g + 32), word(g + 36), word(g + 40));
        call(ctx, s4::fn::Vec3_RotateTo, {out, in, n});
        vec_w1(v + 32, out);
        if (fragment) {
            for (u32 i = 0; i < 3; ++i) call(ctx, d3::fn::FProgram_SetConstant, {16 + i, v + 16 * i});
        } else {
            set_vconsts(ctx, 16, 12, v);
        }
        if ((ref<u8>(item + d3::kItemVariant) & 4) != 0) call(ctx, d3::fn::FProgram_SetConstant, {1, c10});
    }
    lit_draw_bits(ctx, item, 0x005C2878, 0x10);  // .text: f32 1/255
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
DECOMP(005C7210, SlyHeightFog_Draw) {
    constexpr u32 k = 0x005C71C0;  // .text: f32 0.0, 1.0; f64 0.0; mask; {0, 0, 0, 1}; f64 1.0, 0.2, K; f32 1/255
    const u32 item = arg<u32>(ctx, 1);
    Stack cols(ctx, 128);
    upload_columns_copy(ctx, cols.addr(), item, k + 32);
    apply_fog(ctx, item);
    const u32 mat = load<u32>(item + d3::kItemMaterial);
    if ((ref<u8>(item + d3::kItemVariant) & 1) != 0) {
        // The original's sp+336 / +400 (the matrices) and +160.. (constants).
        Stack frame(ctx, 176);
        const u32 a = frame.addr(), b = a + 64, c = a + 128;
        call(ctx, SlyHeightFog_BuildMatrices, {mat, a, b});
        for (u32 n = 0; n < 2; ++n) {
            const u32 m = n == 0 ? a : b;
            for (u32 r = 0; r < 3; ++r) {
                for (u32 i = 0; i < 12; i += 4) sp::copy_word(c + 16 * r + i, m + 16 * r + i);
            }
            for (u32 r = 0; r < 3; ++r) call(ctx, d3::fn::FProgram_SetConstant, {3 + 3 * n + r, c + 16 * r});
        }
    }
    f64 z = sp::lfs(k);
    if ((ref<u8>(item + d3::kItemVariant) & 2) != 0) {  // rlwinm 0,30,30
        Stack frame(ctx, 16);  // the original's sp+128
        const u32 pos = frame.addr();
        f32 p[4];
        for (u32 i = 0; i < 4; ++i) {
            const auto row = [&](u32 r) { return load<f32>((item & ~15u) + 16 * r + 4 * i); };
            f32 v = std::fmaf(row(0), 0.0f, 0.0f);  // vmaddfp with zero
            v = std::fmaf(row(1), 0.0f, v);
            v = std::fmaf(row(2), 0.0f, v);
            p[i] = row(3) + v;  // vaddfp
        }
        for (u32 i = 0; i < 4; ++i) store<f32>(pos + 4 * i, p[i]);
        const u32 cam = static_cast<u32>(call(ctx, View_GetCurrent)) + 864;
        const f64 dy = sp::sgl(sp::lfs(cam + 4) - sp::lfs(pos + 4));
        const f64 dx = sp::sgl(sp::lfs(cam) - sp::lfs(pos));
        const f64 dz = sp::sgl(sp::lfs(cam + 8) - sp::lfs(pos + 8));
        const f64 sq = sp::fmadds(dz, dz, sp::fmadds(dx, dx, sp::sgl(dy * dy)));
        f64 d = sq >= 0.0 ? sp::sgl(std::sqrt(sq)) : (sq == 0.0 ? sq : std::nan(""));  // fsqrts
        d = sp::sgl(sp::sel_ge(load<f64>(k + 8), d, d));
        const f64 one = sp::lfs(k + 4);
        d = sp::sgl(sp::sel_ge(sp::sgl(d - one), d, load<f64>(k + 48)));  // max(d, 1)
        f64 x = 0.0;
        if (d != sp::lfs(k)) {  // fcmpu (never equal: d >= 1)
            const f64 t = std::fma(rt::fcfid(std::bit_cast<u64>(d)), load<f64>(k + 56), load<f64>(k + 64));  // fcfid, fmadd
            x = sp::sgl(std::bit_cast<f64>(rt::fctid(t, false)));  // fctid, frsp
        }
        const f64 limit = sp::sgl(x - one);
        const u32 params = load<u32>(mat + d3::kMatParams);
        const f64 cap = sp::sgl(static_cast<f64>(params != 0 ? load<u32>(params + 68) : 0u));  // fcfid, frsp
        const f64 m = sp::sel_ge(sp::sgl(cap - limit), limit, cap);  // min
        z = sp::sgl(sp::sel_ge(m, m, load<f64>(k + 8)));  // max(.., 0)
    }
    {
        Stack frame(ctx, 16);  // the original's sp+112
        const u32 c1 = frame.addr();
        const auto param = [&](u32 off) {
            const u32 params = load<u32>(mat + d3::kMatParams);
            return params != 0 ? load<u32>(params + off) : load<u32>(k);
        };
        store<u32>(c1, param(60));
        store<u32>(c1 + 4, param(56));
        sp::stfs(c1 + 8, z);
        store<u32>(c1 + 12, param(64));
        call(ctx, d3::fn::FProgram_SetConstant, {1, c1});
    }
    lit_draw(ctx, item, k + 72, Stencil::Plain);
}

// Diffuse: the item transform, the view fog (DrawItem_SetParticleState
// (item, 32)), the lit layers (lit_layers), then the lit draw (stencil
// without 0x40).
DECOMP(005B1FA0, Diffuse_Draw) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    call(ctx, s2::fn::DrawItem_SetParticleState, {item, 32});
    lit_layers(ctx, item);
    lit_draw(ctx, item, 0x005B1F9C, Stencil::Plain);  // .text: f32 1/255
}

// SlyPanorama: the item transform, the view fog, the lit layers
// (lit_layers), then the lit draw (stencil without 0x40).
DECOMP(005C9948, SlyPanorama_Draw) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    apply_fog(ctx, item);
    lit_layers(ctx, item);
    lit_draw(ctx, item, 0x005C9940, Stencil::Plain);  // .text: f32 1/255
}

// SAC_EnvMap: the item transform (no fog); rows = the item rows through
// kViewRotation (Matrix_RotateRows), c = {the camera in the item's frame,
// 1}; for a lit item (variant +69 bit 0) with variant bit 1 the layer
// list (unchecked) as fragment constants 16.., then rows and c as
// fragment constants 2..5; otherwise (bit 1 clear: the layer list as
// vertex constants 16.. first) rows as vertex constant parameter blocks
// 7..9 and c as vertex constant 10; then the lit draw with fragment
// constant 1 (stencil without 0x40).
DECOMP(005B8EC0, SAC_EnvMap_Draw) {
    env_draw(ctx, arg<u32>(ctx, 1), 0x005B8EB8, Fog::None);  // .text: f32 1/255
}

// EnvMap: SAC_EnvMap's draw with the view fog (DrawItem_SetParticleState
// (item, 32)) after the item transform.
DECOMP(005B3590, EnvMap_Draw) {
    env_draw(ctx, arg<u32>(ctx, 1), 0x005B3588, Fog::Particle);  // .text: f32 1/255
}

// SlyEnvMap: SAC_EnvMap's draw with the view fog after the item
// transform and fragment constant 1 = g::character_fconst2 with lane 2
// from material parameters +12 (also lane 2 of the translucent
// constant).
DECOMP(005C57B0, SlyEnvMap_Draw) {
    env_draw(ctx, arg<u32>(ctx, 1), 0x005C57A4, Fog::View);  // .text: f32 0.0, 1/255
}

// SlyWater2: water_draw (fragment constant 3 from the material
// parameters).
DECOMP(005D3750, SlyWater2_Draw) {
    water_draw(ctx, arg<u32>(ctx, 1), 0x005D3730, false);  // .text: vperm mask, {0, 0, 0, 1}
}

// SlyWater3: water_draw (fragment constant 3 from
// SlyWater3_UploadFragment).
DECOMP(005D5220, SlyWater3_Draw) {
    water_draw(ctx, arg<u32>(ctx, 1), 0x005D5200, true);  // .text: vperm mask, {0, 0, 0, 1}
}
