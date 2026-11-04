// The larger shader Draw slots (Shader vtable slot 12):
// void <Shader>_Draw(Shader* s, DrawItem* item)
//
// DrawItem fields these read beyond draw3_shader.h's: +68 u8 flags (bit
// 7: mirrored, drawn with clockwise front faces and culling; bit 5:
// stencil bit 7), +71 u8 alpha (255: opaque). Material: +101 u8 stencil
// bits, +104 bit 31: keeps its own blending when translucent, +112 bit
// 31: restores the depth mask (render state +93 bit 0) after a
// translucent draw.
#include "shader3.h"

#include <cmath>

using namespace sly4;

namespace {

namespace d3 = sly4::render::draw3;
namespace l4 = sly4::render::layers4;
namespace s2 = sly4::render::shader2;
namespace s3 = sly4::render::shader3;
namespace sp = sly4::render::pool;

constexpr u32 kMeshDraw = 16;  // Mesh vtable slot 16: Draw(mesh, item)
constexpr u32 kItemFlags = 68, kItemAlpha = 71;
constexpr u32 kMirrored = 0x80, kStencilHigh = 0x20;
constexpr u32 kMatStencil = 101, kMatOwnBlend = 104, kMatDepthRestore = 112;

// transpose(item rows * layers4::g::view_proj) as vertex constants 0..3
// (one SET_TRANSFORM_CONSTANT_LOAD of 17 words): rows read with lvx
// (address rounded down to 16), fused per lane in vmaddfp order, row 3
// adding view_proj's row 3. (shader2_draw.cpp's helper of the same name.)
void upload_item_transform(u32 item) {
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
    for (u32 k = 0; k < 4; ++k) {
        for (u32 i = 0; i < 4; ++i) store<f32>(cur + 8 + 16 * k + 4 * i, r[i][k]);  // stvlx / stvrx pairs
    }
}

// The view fog, as DrawItem_SetParticleState(item, 32) sets it but from
// the precomputed tables (s3::g::fog_vconsts / fog_colours): fog index
// item +68 bit 6, enabled by material +100 bit 6; vertex constant 32 is
// re-sent only when the index or its flag (render state +216 / +220..)
// changed; fragment constant 32 = the fog's colour.
void apply_fog(PPUContext& ctx, u32 item) {
    const u32 st = fx::g::state;
    const u32 fog = (ref<u8>(item + kItemFlags) >> 6) & 1;  // rlwinm 26,31,31
    const u32 enable = (ref<u8>(load<u32>(item + d3::kItemMaterial) + d3::kMatFlags) >> 6) & 1;
    const u32 colour = s3::g::fog_colours + 16 * fog;
    if (load<u32>(st + s3::kStFogIndex) != fog || ref<u8>(s3::g::fog_enabled + fog) != enable) {
        store<u32>(st + s3::kStFogIndex, fog);
        store<u8>(s3::g::fog_enabled + fog, static_cast<u8>(enable));
        call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(st), 32, 4, s3::g::fog_vconsts + 32 * fog + 16 * enable});
    }
    call(ctx, d3::fn::FProgram_SetConstant, {32, colour});
}

// The layer list as vertex constants 16..: {+16 * colour, +32 * colour,
// +48} and per layer entry (+0 u32 count, 48-byte entries from +64) {+0,
// +16 * colour, +32}, with colour the material's f32[4] at +64.
// (shader2_draw.cpp's Decal_Draw does the same.)
void upload_layer_list(PPUContext& ctx, u32 mat, u32 list) {
    const u32 colour = mat + 64;
    const auto tinted = [&](u32 dst, u32 src) {
        for (u32 i = 0; i < 16; i += 4) sp::stfs(dst + i, sp::sgl(sp::lfs(src + i) * sp::lfs(colour + i)));  // fmuls
    };
    const auto raw = [](u32 dst, u32 src) {
        for (u32 i = 0; i < 16; i += 4) sp::copy_word(dst + i, src + i);
    };
    // The original builds this at sp+112 of a 336-byte frame without a
    // bound: more than 3 entries overrun its saved registers. Here the
    // frame is sized for the entry count.
    Stack frame(ctx, 48 + 48 * load<u32>(list));
    const u32 v = frame.addr();
    tinted(v, list + 16);
    tinted(v + 16, list + 32);
    raw(v + 32, list + 48);
    u32 dst = v + 48;
    if (load<u32>(list) != 0) {
        u32 src = list + 64;
        for (u32 n = 0; n < load<u32>(list); ++n, src += 48, dst += 48) {  // cmplw
            raw(dst, src);
            tinted(dst + 16, src + 16);
            raw(dst + 32, src + 32);
        }
    }
    const u32 floats = static_cast<u32>(static_cast<s32>(dst - v) / 16) << 2;  // srawi / addze, rlwinm 2,0,29
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 16, floats, v});
}

// The layer list of a layered item (material +100 bit 7 and a list at item +44).
void upload_layers(PPUContext& ctx, u32 item) {
    const u32 mat = load<u32>(item + d3::kItemMaterial);
    if (((ref<u8>(mat + d3::kMatFlags) >> 7) & 1) == 0) return;  // rlwinm 25,31,31
    const u32 list = load<u32>(item + d3::kItemLayers);
    if (list != 0) upload_layer_list(ctx, mat, list);
}

// The mesh's draw (item +12, slot 16).
void draw_mesh(PPUContext& ctx, u32 item) {
    const u32 mesh = load<u32>(item + d3::kItemMesh);
    fx::vcall(ctx, mesh, kMeshDraw, {mesh, item});
}

// Top bit of a material word.
bool mat_bit31(u32 mat, u32 off) { return (load<u32>(mat + off) >> 31) != 0; }  // rlwinm 1,31,31

// Stencil function reference = value (cached at render::g::stencil_bytes[0]
// and render state +164), then the stencil write mask = stencil_bytes[1]
// (render state +168).
void set_stencil_ref(u32 value) {
    const u32 st = fx::g::state;
    store<u8>(render::g::stencil_bytes, static_cast<u8>(value));
    store<u32>(st + 164, value);
    fx::emit({sp::kStencilFunc, load<u32>(st + 160), value, load<u32>(st + 172)});
    const u32 mask = ref<u8>(render::g::stencil_bytes + 1);
    store<u32>(st + 168, mask);
    fx::emit({sp::kStencilMask, mask});
}

// The lit draw SlyCharacter2, the SAC_* shaders and SimpleColorTxrUV
// share (k: f32 1/255 in .text): fragment constant `fconst` (2; 0 for
// SimpleColorTxrUV) from g::character_fconst2; a translucent
// item (alpha below 255) draws
// without depth writes and with fragment constant 2 = {own blending ? 0 :
// 1, alpha / 255, 0, 0} (alpha blending on unless the material blends
// itself); a mirrored item flips the front face around the draw; the
// stencil reference becomes (stencil bit 7 | 0x40 | material +101) &
// the write mask (SlyCharacter2; the SAC_* shaders leave out 0x40,
// SimpleColorTxrUV leaves the stencil alone);
// after a translucent draw the depth mask and blending
// are restored as the material asks.
enum class Stencil { None, Plain, With40 };
void lit_draw(PPUContext& ctx, u32 item, u32 k, Stencil stencil, u32 fconst = 2) {
    call(ctx, d3::fn::FProgram_SetConstant, {fconst, s3::g::character_fconst2});
    if (ref<u8>(item + kItemAlpha) < 255) {  // cmpwi
        fx::emit({l4::kDepthMask, 0});
        Stack frame(ctx, 16);
        const u32 v = frame.addr();
        store<u32>(v, 0);
        store<u32>(v + 8, 0);
        store<u32>(v + 12, 0);
        sp::stfs(v + 4, sp::sgl(sp::sgl(static_cast<f64>(ref<u8>(item + kItemAlpha))) * sp::lfs(k)));  // fcfid, frsp, fmuls
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
    if (stencil != Stencil::None) {
        const u32 mat = load<u32>(item + d3::kItemMaterial);
        const u32 bits = ((flags & kStencilHigh) != 0 ? 0x80u : 0u) | (stencil == Stencil::With40 ? 0x40u : 0u) | ref<u8>(mat + kMatStencil);  // rlwinm 27,31,31
        const u32 value = bits & ref<u8>(render::g::stencil_bytes + 1);
        if (value != ref<u8>(render::g::stencil_bytes)) set_stencil_ref(value);
    }
    draw_mesh(ctx, item);
    if ((ref<u8>(item + kItemFlags) & kMirrored) != 0) {
        fx::emit({l4::kCullEnable, 1});
        fx::emit({l4::kFrontFace, l4::kCcw});
    }
    if (ref<u8>(item + kItemAlpha) >= 255) return;
    if (mat_bit31(load<u32>(item + d3::kItemMaterial), kMatDepthRestore)) {
        fx::emit({l4::kDepthMask, ref<u8>(fx::g::state + 93) & 1u});
    }
    if (!mat_bit31(load<u32>(item + d3::kItemMaterial), kMatOwnBlend)) fx::emit({l4::kBlendEnable, 0});
}

// SlyCharacter2_Draw's body: the character set-up
// (SlyCharacter2_SetupDraw), then the lit draw.
void draw_character(PPUContext& ctx, u32 k) {
    const u32 s = arg<u32>(ctx, 0), item = arg<u32>(ctx, 1);
    call(ctx, s3::fn::SlyCharacter2_SetupDraw, {s, item});
    lit_draw(ctx, item, k, Stencil::With40);
}

// The SAC_* shaders: the item transform, view fog 0 off
// (Draw_SetParticleState(0, false, 32)), the shader's own vertex
// constants (SacExtra), then the lit draw.
enum class SacExtra {
    None,
    Colour,  // SAC_Prop: the default material colour (s3::g::default_colour) as vertex constant 7
    Light,   // SAC_LitProp: g::config +176.. / +192.. as vertex constants 16 / 20 (w 1.0), the light direction +208.. in item space as 23
    Tint,    // SAC_FastProp: the material's packed colour (parameters +0; 0 without) times the item's u8[4] +72, / 256, / 255, as vertex constant 7
};
void draw_sac(PPUContext& ctx, u32 k, SacExtra extra) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    call(ctx, d3::fn::Draw_SetParticleState, {0, 0, 32});
    if (extra == SacExtra::Colour) {
        call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 7, 4, s3::g::default_colour});
    } else if (extra == SacExtra::Light) {
        constexpr u32 cfg = sly4::g::config;
        // The original's sp+112 (constant 23), +128 (16), +144 (20), +160 (the direction).
        Stack frame(ctx, 64);
        const u32 v23 = frame.addr(), v16 = v23 + 16, v20 = v23 + 32, dir = v23 + 48;
        for (u32 i = 0; i < 12; i += 4) {
            sp::copy_word(v16 + i, cfg + 176 + i);
            sp::copy_word(v20 + i, cfg + 192 + i);
            sp::copy_word(dir + i, cfg + 208 + i);
        }
        store<u32>(v16 + 12, s2::kOne);
        store<u32>(v20 + 12, s2::kOne);
        call(ctx, s3::fn::Vec3_ToItemSpace, {dir, item});
        for (u32 i = 0; i < 12; i += 4) sp::copy_word(v23 + i, dir + i);
        store<u32>(v23 + 12, s2::kOne);
        call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 23, 4, v23});
        call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 16, 4, v16});
        call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 20, 4, v20});
    } else if (extra == SacExtra::Tint) {
        const u32 params = load<u32>(load<u32>(item + d3::kItemMaterial) + d3::kMatParams);
        const u32 packed = params != 0 ? load<u32>(params) : 0;
        const f64 scale = sp::lfs(k);
        Stack frame(ctx, 16);
        const u32 v = frame.addr();
        for (u32 i = 0; i < 4; ++i) {
            const u32 channel = (packed >> (8 * i)) & 0xFF;  // byte 3 - i of the word
            const u32 x = ((channel * ref<u8>(item + 72 + i)) >> 8) & 0xFF;  // mullw, rlwinm 24,24,31
            sp::stfs(v + 4 * i, sp::sgl(sp::sgl(static_cast<f64>(x)) * scale));  // fcfid, frsp, fmuls
        }
        call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 7, 4, v});
    }
    lit_draw(ctx, item, k, Stencil::Plain);
}

// Sly2UV_Draw's body: the item transform, the fog, the layers (SlyDecal4:
// those of an item whose variant +69 bit 0 is set, the list unchecked),
// with `alpha_k` (SlyScreenWarp) vertex constant 4 = {1, 1, 1, item
// alpha / the f32 at alpha_k (255.0)}, the stencil reference ((stencil
// bit 7 | material +101) & the write mask), then the mesh's draw.
void draw_layered_stencil(PPUContext& ctx, bool by_variant = false, u32 alpha_k = 0) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    apply_fog(ctx, item);
    if (!by_variant) {
        upload_layers(ctx, item);
    } else if ((ref<u8>(item + d3::kItemVariant) & 1) != 0) {
        upload_layer_list(ctx, load<u32>(item + d3::kItemMaterial), load<u32>(item + d3::kItemLayers));
    }
    if (alpha_k != 0) {
        Stack frame(ctx, 16);
        const u32 v = frame.addr();
        for (u32 i = 0; i < 12; i += 4) store<u32>(v + i, s2::kOne);
        sp::stfs(v + 12, sp::sgl(sp::sgl(static_cast<f64>(ref<u8>(item + kItemAlpha))) / sp::lfs(alpha_k)));  // fcfid, frsp, fdivs
        call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 4, 4, v});
    }
    const u32 flags = ref<u8>(item + kItemFlags);
    const u32 mat = load<u32>(item + d3::kItemMaterial);
    const u32 value = (((flags & kStencilHigh) != 0 ? 0x80u : 0u) | ref<u8>(mat + kMatStencil)) & ref<u8>(render::g::stencil_bytes + 1);
    if (value != ref<u8>(render::g::stencil_bytes)) set_stencil_ref(value);
    draw_mesh(ctx, item);
}

// SimpleColorTxrUV: the item transform, vertex constant 6 = the item's
// colour (item +64, u8[4] / 255; the default material colour without
// one), then the lit draw with fragment constant 0 and no stencil.
void draw_colour_uv(PPUContext& ctx, u32 k) {
    const u32 item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    Stack frame(ctx, 16);
    const u32 v = frame.addr();
    if (const u32 colour = load<u32>(item + 64); colour != 0) {
        const f64 scale = sp::lfs(k);
        for (u32 i = 0; i < 4; ++i) sp::stfs(v + 4 * i, sp::sgl(sp::sgl(static_cast<f64>(ref<u8>(colour + i))) * scale));  // fcfid, frsp, fmuls
    } else {
        for (u32 i = 0; i < 16; i += 4) sp::copy_word(v + i, s3::g::default_colour + i);
    }
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 6, 4, v});
    lit_draw(ctx, item, k, Stencil::None, 0);
}

}  // namespace

DECOMP(005AE0F0, SimpleColorTxrUV_Draw) { draw_colour_uv(ctx, 0x005AE0EC); }

DECOMP(005B9D0C, Sly2UV_Draw) { draw_layered_stencil(ctx); }
DECOMP(005BA8BC, SlyCaustics_Draw) { draw_layered_stencil(ctx); }
DECOMP(005C4C3C, SlyDecal4_Draw) { draw_layered_stencil(ctx, true); }
DECOMP(005CD6F0, SlyScreenWarp_Draw) { draw_layered_stencil(ctx, false, 0x005CD6E8); }

DECOMP(005B7260, SAC_Environment_Draw) { draw_sac(ctx, 0x005B725C, SacExtra::None); }
DECOMP(005B78A8, SAC_Prop_Draw) { draw_sac(ctx, 0x005B78A4, SacExtra::Colour); }
DECOMP(005B7F18, SAC_LitProp_Draw) { draw_sac(ctx, 0x005B7F10, SacExtra::Light); }
DECOMP(005B8640, SAC_FastProp_Draw) { draw_sac(ctx, 0x005B863C, SacExtra::Tint); }

DECOMP(005BD3B0, SlyCharacter2_Draw) { draw_character(ctx, 0x005BD3AC); }
// SlyCharacterFakeFur: the same code (its own copy of k).
DECOMP(005C1EB8, SlyCharacterFakeFur_Draw) { draw_character(ctx, 0x005C1EB4); }
