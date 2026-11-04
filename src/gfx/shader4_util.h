// The pieces the shader Draw slots inline, as shader4*.cpp share them.
// (shader2_draw.cpp / shader3_draw.cpp have file-local copies of the
// first few; these are the same code, kept in one place for shader4.)
//
// DrawItem fields beyond draw3_shader.h's: +68 u8 flags (bit 7: mirrored;
// bit 6: fog index; bit 5: stencil bit 7), +69 u8 variant (SlyCharacter2:
// bit 0 lit, bit 1 layers as fragment constants), +71 u8 alpha.
#pragma once

#include <cmath>

#include "shader4.h"

namespace sly4::render {
namespace shader4 {
namespace util {

namespace d3 = sly4::render::draw3;
namespace l4 = sly4::render::layers4;
namespace s2 = sly4::render::shader2;
namespace s3 = sly4::render::shader3;
namespace sp = sly4::render::pool;

constexpr u32 kMeshDraw = 16;  // Mesh vtable slot 16: Draw(mesh, item)
constexpr u32 kItemFlags = 68, kItemAlpha = 71, kItemColour = 64;
constexpr u32 kMirrored = 0x80, kStencilHigh = 0x20;
constexpr u32 kMatColour = 64, kMatStencil = 101, kMatOwnBlend = 104, kMatDepthRestore = 112;

inline u32 gcm() { return load<u32>(fx::g::state); }

inline void set_vconsts(PPUContext& ctx, u32 first, u32 floats, u32 src) {
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {gcm(), first, floats, src});
}

// transpose(item rows * layers4::g::view_proj) as vertex constants 0..3
// (one SET_TRANSFORM_CONSTANT_LOAD of 17 words): rows read with lvx
// (address rounded down to 16), fused per lane in vmaddfp order, row 3
// adding view_proj's row 3.
inline void upload_item_transform(u32 item) {
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
    const u32 c = gcm();
    const u32 cur = load<u32>(c + 8);
    store<u32>(c + 8, cur + 72);
    store<u32>(cur, l4::kVertexConst17);
    store<u32>(cur + 4, 0);  // first constant
    for (u32 k = 0; k < 4; ++k) {
        for (u32 i = 0; i < 4; ++i) store<f32>(cur + 8 + 16 * k + 4 * i, r[i][k]);  // stvlx / stvrx pairs
    }
}

// The view fog from the precomputed tables (s3::g::fog_vconsts /
// fog_colours): fog index item +68 bit 6, enabled by material +100 bit
// 6; vertex constant 32 re-sent only when the index or its flag (render
// state +216 / +220..) changed; fragment constant 32 = the fog's colour.
inline void apply_fog(PPUContext& ctx, u32 item) {
    const u32 st = fx::g::state;
    const u32 fog = (ref<u8>(item + kItemFlags) >> 6) & 1;  // rlwinm 26,31,31
    const u32 enable = (ref<u8>(load<u32>(item + d3::kItemMaterial) + d3::kMatFlags) >> 6) & 1;
    const u32 colour = s3::g::fog_colours + 16 * fog;
    if (load<u32>(st + s3::kStFogIndex) != fog || ref<u8>(s3::g::fog_enabled + fog) != enable) {
        store<u32>(st + s3::kStFogIndex, fog);
        store<u8>(s3::g::fog_enabled + fog, static_cast<u8>(enable));
        set_vconsts(ctx, 32, 4, s3::g::fog_vconsts + 32 * fog + 16 * enable);
    }
    call(ctx, d3::fn::FProgram_SetConstant, {32, colour});
}

inline void tinted4(u32 dst, u32 src, u32 colour) {
    for (u32 i = 0; i < 16; i += 4) sp::stfs(dst + i, sp::sgl(sp::lfs(src + i) * sp::lfs(colour + i)));  // fmuls
}
inline void raw4(u32 dst, u32 src) {
    for (u32 i = 0; i < 16; i += 4) sp::copy_word(dst + i, src + i);
}

// The layer list as vertex constants 16..: {+16 * colour, +32 * colour,
// +48} and per layer entry (+0 u32 count, 48-byte entries from +64) {+0,
// +16 * colour, +32}, with colour the material's f32[4] at +64. The
// original builds it in a fixed stack area without a bound (more entries
// overrun its frame); here the frame is sized for the count.
inline void upload_layer_list(PPUContext& ctx, u32 mat, u32 list) {
    const u32 colour = mat + kMatColour;
    Stack frame(ctx, 48 + 48 * load<u32>(list));
    const u32 v = frame.addr();
    tinted4(v, list + 16, colour);
    tinted4(v + 16, list + 32, colour);
    raw4(v + 32, list + 48);
    u32 dst = v + 48;
    if (load<u32>(list) != 0) {
        u32 src = list + 64;
        for (u32 n = 0; n < load<u32>(list); ++n, src += 48, dst += 48) {  // cmplw
            raw4(dst, src);
            tinted4(dst + 16, src + 16, colour);
            raw4(dst + 32, src + 32);
        }
    }
    const u32 floats = static_cast<u32>(static_cast<s32>(dst - v) / 16) << 2;  // srawi / addze, rlwinm 2,0,29
    set_vconsts(ctx, 16, floats, v);
}

// The layer list (item +44, unchecked) as fragment constants 16..: {+16 *
// colour}, {+32 * colour}, {+48}, and per entry 3 more {+0}, {+16 *
// colour}, {+32}, colour the material's f32[4] at +64. The original
// builds the entries in a fixed stack area without a bound (more
// entries overrun its frame: SlyCharacter2_SetupDraw from sp+480 of 752
// bytes, SlyDecal from sp+384 of 608); here the frame is sized for the
// count.
inline void upload_layers_fragment(PPUContext& ctx, u32 item) {
    const u32 list = load<u32>(item + d3::kItemLayers);
    const u32 colour = load<u32>(item + d3::kItemMaterial) + kMatColour;
    Stack head(ctx, 48);
    const u32 v = head.addr();
    tinted4(v, list + 16, colour);
    call(ctx, d3::fn::FProgram_SetConstant, {16, v});
    tinted4(v + 16, list + 32, colour);
    raw4(v + 32, list + 48);
    call(ctx, d3::fn::FProgram_SetConstant, {17, v + 16});
    call(ctx, d3::fn::FProgram_SetConstant, {18, v + 32});
    if (load<u32>(list) == 0) return;
    Stack entries(ctx, 48 * load<u32>(list));
    u32 dst = entries.addr(), src = list + 64;
    s32 index = 19;
    for (u32 n = 0; n < load<u32>(list); ++n, dst += 48, src += 48, index += 3) {  // cmplw
        raw4(dst, src);
        tinted4(dst + 16, src + 16, colour);
        raw4(dst + 32, src + 32);
        call(ctx, d3::fn::FProgram_SetConstant, {static_cast<u64>(index), dst});
        call(ctx, d3::fn::FProgram_SetConstant, {static_cast<u64>(index + 1), dst + 16});
        call(ctx, d3::fn::FProgram_SetConstant, {static_cast<u64>(index + 2), dst + 32});
    }
}

// The layer list of a layered item (material +100 bit 7 and a list at item +44).
inline void upload_layers(PPUContext& ctx, u32 item) {
    const u32 mat = load<u32>(item + d3::kItemMaterial);
    if (((ref<u8>(mat + d3::kMatFlags) >> 7) & 1) == 0) return;  // rlwinm 25,31,31
    const u32 list = load<u32>(item + d3::kItemLayers);
    if (list != 0) upload_layer_list(ctx, mat, list);
}

// The mesh's draw (item +12, slot 16).
inline u64 draw_mesh(PPUContext& ctx, u32 item) {
    const u32 mesh = load<u32>(item + d3::kItemMesh);
    return fx::vcall(ctx, mesh, kMeshDraw, {mesh, item});
}

// Top bit of a material word.
inline bool mat_bit31(u32 mat, u32 off) { return (load<u32>(mat + off) >> 31) != 0; }  // rlwinm 1,31,31

// Stencil function reference = value (cached at render::g::stencil_bytes[0]
// and render state +164), then the stencil write mask = stencil_bytes[1]
// (render state +168).
inline void set_stencil_ref(u32 value) {
    const u32 st = fx::g::state;
    store<u8>(render::g::stencil_bytes, static_cast<u8>(value));
    store<u32>(st + 164, value);
    fx::emit({sp::kStencilFunc, load<u32>(st + 160), value, load<u32>(st + 172)});
    const u32 mask = ref<u8>(render::g::stencil_bytes + 1);
    store<u32>(st + 168, mask);
    fx::emit({sp::kStencilMask, mask});
}

// Word 0 of lvlx(ea): the bytes from ea to the end of its 16-byte block,
// zero filled.
inline u32 lvlx_word0(u32 ea) {
    u32 w = 0;
    for (u32 i = 0; i < 4; ++i) w = (w << 8) | ((ea & 15) + i < 16 ? ref<u8>(ea + i) : 0u);
    return w;
}

// m (4 rows of f32[4], read with lvx) times the 3-vector (x, y, z) per
// lane, in vmaddfp order: m0 * x + 0, + m1 * y, + m2 * z.
inline void rows_times(f32 out[4], u32 m, f32 x, f32 y, f32 z) {
    m &= ~15u;
    for (u32 k = 0; k < 4; ++k) {
        f32 v = std::fmaf(load<f32>(m + 4 * k), x, 0.0f);
        v = std::fmaf(load<f32>(m + 16 + 4 * k), y, v);
        out[k] = std::fmaf(load<f32>(m + 32 + 4 * k), z, v);
    }
}

// u8 -> f32 through fcfid / frsp, times a single.
inline f64 byte_scaled(u32 b, f64 k) { return sp::sgl(sp::sgl(static_cast<f64>(b)) * k); }

// The translucent / mirrored / stencilled mesh draw the lit shaders end
// with (shader3_draw.cpp's lit_draw): fragment constant `fconst` from
// g::character_fconst2; a translucent item (alpha below 255) draws
// without depth writes and with fragment constant `fconst` = {own
// blending ? 0 : 1, alpha * k, 0, 0} (alpha blending on unless the
// material blends itself, material +104 bit 31); a mirrored item flips
// the front face around the draw; the stencil reference becomes (stencil
// bit 7 | extra | material +101) & the write mask (Stencil::None leaves
// it; TextureWarp leaves out the mirroring and the stencil, `mirror`
// false); after a translucent draw the depth mask (material +112 bit 31)
// and blending are restored.
enum class Stencil { None, Plain, With40 };
inline void lit_draw(PPUContext& ctx, u32 item, u32 k, Stencil stencil, u32 fconst = 2, bool mirror = true) {
    call(ctx, d3::fn::FProgram_SetConstant, {fconst, s3::g::character_fconst2});
    if (ref<u8>(item + kItemAlpha) < 255) {  // cmpwi
        fx::emit({l4::kDepthMask, 0});
        Stack frame(ctx, 16);
        const u32 v = frame.addr();
        store<u32>(v, 0);
        store<u32>(v + 8, 0);
        store<u32>(v + 12, 0);
        sp::stfs(v + 4, byte_scaled(ref<u8>(item + kItemAlpha), sp::lfs(k)));  // fcfid, frsp, fmuls
        if (!mat_bit31(load<u32>(item + d3::kItemMaterial), kMatOwnBlend)) {
            fx::blend_on(fx::kSrcAlpha2, fx::kOneMinusSrcAlpha2);
            store<u32>(v, s2::kOne);
        }
        call(ctx, d3::fn::FProgram_SetConstant, {fconst, v});
    }
    u32 flags = ref<u8>(item + kItemFlags);
    if (mirror && (flags & kMirrored) != 0) {  // rlwinm 25,31,31
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
    if (mirror && (ref<u8>(item + kItemFlags) & kMirrored) != 0) {
        fx::emit({l4::kCullEnable, 1});
        fx::emit({l4::kFrontFace, l4::kCcw});
    }
    // r3 as the original leaves it: the alpha byte, the own-blending bit
    // or the gcm context.
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

}  // namespace util
}  // namespace shader4
}  // namespace sly4::render
