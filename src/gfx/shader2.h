// The larger shader Bind (slot 8) slots: names and the helpers they share.
// (Shader / ShaderType layouts: draw3_shader.h.)
//
// Material parameters (material +124, a per-type record; 0 when the
// material has none) as these Binds read them: SlyCaustics +0 / +4 UV
// scales, +8 / +12 divisors; SlyDecal +0 / +4 UV scales, +8.. fragment
// constant 0; Diffuse +0.. fragment constant 0, +12 s32 alpha reference
// (alpha test on when positive); SlyCharacter2 +0.. fragment constant 0,
// +40 / +48 vertex constant 33; SlyTerrainSnow +12 / +16 packed RGB
// colours; TextureWarp (TextureWarp_BuildMatrices) +0.. +36.
#pragma once

#include <bit>

#include "draw3_shader.h"

namespace sly4::render {
namespace shader2 {

namespace fn {
// Still recompiled.
constexpr u32 TextureWarp_BuildMatrices = 0x005AE648;  // (Material*, f32 m1[16], f32 m2[16], f32 v[4]) two UV warp matrices and a vec4 (name?)
constexpr u32 Display_GetDepthTexture = 0x00537BB8;    // (g::mem_stats) the scene depth texture for the current buffer set (name?)
constexpr u32 SlyHeightFog_UploadFog = 0x005C6B20;     // (Material*) fog vertex constants (name?)
constexpr u32 SlyTextureWarp_BuildMatrices = 0x005D12C8;  // TextureWarp_BuildMatrices' twin for SlyTextureWarp (name?)
constexpr u32 Draw_GetScreenCopy = 0x00512940;            // () Draw_CopyScreen once per frame (layers4::g::copy_stamp / copy_texture) (name?)
constexpr u32 MaterialAnim_UploadUVAlt = 0x0050D1E0;      // (anim block or 0, u32 first constant, f32 su [f1], f32 sv [f2]) Water's variant of MaterialAnim_UploadUV (name?)
constexpr u32 DrawItem_SetParticleState = 0x0050D0E0;    // (DrawItem*, u32) Draw_SetParticleState(item +68 bit 5, material +100 bit 5, arg) (name?)
constexpr u32 SlyWater2_UploadWaves = 0x005D3180;         // (Material*) vertex constants (name?)
constexpr u32 SlyWater3_UploadWaves = 0x005D4A38;         // (Material*) vertex constants (name?)
// libgcm.
constexpr u32 cellGcmSetMethod1FF8 = 0x005E3F80;          // (context, u32 value) one-word NV4097 method 0x1FF8 (render state +88 caches it) (name?)
constexpr u32 cellGcmSetVertexProgramParameterBlock = 0x005E4124;  // (context, first constant, vec4 count, const f32*) method 0x1EFC (name?)
}  // namespace fn

namespace g {
constexpr u32 depth_format_warned = 0x011DE2C4;  // u8: SprParticle_Bind met a depth target format it has no bias scale for (name?)
}  // namespace g

// RSX methods.
constexpr u32 kPolyOffsetFill = 0x00040A68;  // POLYGON_OFFSET_FILL_ENABLE
constexpr u32 kPolyOffset2 = 0x00080A78;     // POLYGON_OFFSET_SCALE_FACTOR, _BIAS
constexpr u32 kTexAddress1 = 0x00041A28;     // TEXTURE_ADDRESS, unit 1
constexpr u32 kAlphaFunc2 = 0x00080308;  // ALPHA_FUNC, ALPHA_REF
constexpr u32 kGreater = 0x0206;
constexpr u32 kTexAddress4 = 0x00041A88;  // TEXTURE_ADDRESS, unit 4
constexpr u32 kClampToEdge3 = 0x00030303;  // wrap s / t / r: CLAMP_TO_EDGE
constexpr u32 kOne = 0x3F800000;           // f32 1.0

// The four words of a vector built from optional material parameters:
// params + off when params is set, else the default word.
inline u32 param_or(u32 params, u32 off, u32 def_word) { return params != 0 ? load<u32>(params + off) : def_word; }

// Top nibble of material +120 (rlwinm 4,28,31).
inline u32 blend_mode(u32 mat) { return load<u32>(mat + draw3::kMatBlend) >> 28; }

// fdivs on two raw f32 words.
inline u32 fdivs_bits(u32 a, u32 b) {
    const f64 q = std::bit_cast<f32>(a) / static_cast<f64>(std::bit_cast<f32>(b));
    return std::bit_cast<u32>(static_cast<f32>(q));
}

}  // namespace shader2
}  // namespace sly4::render
