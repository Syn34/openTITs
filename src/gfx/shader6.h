// The last shader helpers (material parameter slots, the sorted map
// lookup, the 4x4 product, SlyWater3's fragment constants, SlyHeightFog's
// matrices) and the remaining Draw (slot 12) slots in shader6_draw*.cpp.
// (Shader / ShaderType layouts: draw3_shader.h.)
//
// Map (as Map_Find3 reads it): +2 u16 count, +6 u16 flags (bit 0: unsorted),
// +8 8-byte entries {u32 key, value}, +16 the value returned for a missing
// key, +20 compare OPD (int(const Entry*, const u32* key); also qsort's).
// With the default compare (kMapCompareU32) the keys are compared inline,
// unsigned.
#pragma once

#include "shader5.h"

namespace sly4::render {
namespace shader6 {

namespace fn {
// Decompiled in shader6_util.cpp: shader5::fn::Material_ParamSlot,
// Map_Find (as Map_Find3), Matrix_MultiplyTo, SlyWater3_UploadFragment (0x005D4FF0),
// SlyHeightFog_BuildMatrices (0x005C6EB8).
constexpr u32 SlyWater3_UploadFragment = 0x005D4FF0;    // (Material*) fragment constants 3 / 4 and the screen copy on unit 5 (name?)
constexpr u32 SlyHeightFog_BuildMatrices = 0x005C6EB8;  // (Material*, f32 a[4][4], f32 b[4][4]) two scrolled, rotated UV matrices (name?)
// Still recompiled.
constexpr u32 SmallAlloc = 0x0055238C;              // (u32 size) (common.h)
constexpr u32 SmallFree = 0x00552420;               // (p, u32 size) (common.h)
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 qsort = 0x0071EC28;                   // (base, count, size, compare OPD)
constexpr u32 Vec4_TransformByMatrix = 0x00553910;  // (f32 out[4], const f32 v[4], const f32 m[16]) (VMX) (name?)
// libgcm.
constexpr u32 cellGcmSetTexture = 0x005E4514;         // (context, index, CellGcmTexture*) (draw_state.h)
constexpr u32 cellGcmSetTextureControl = 0x005E3F18;  // (context, index, enable, minlod, maxlod, aniso) (draw_state.h)
}  // namespace fn

constexpr u32 kMapCompareU32 = 0x00F3CBF0;  // OPD: the default Map compare (unsigned u32 keys) (name?)
constexpr u32 kMapCount = 2, kMapFlags = 6, kMapEntries = 8, kMapMissing = 16, kMapCompare = 20;
constexpr u32 kMatParamCapacity = 121;  // u8: material +124's capacity in words

}  // namespace shader6
}  // namespace sly4::render
