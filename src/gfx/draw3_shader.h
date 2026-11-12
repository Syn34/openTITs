// The shader registry's types and their shaders, as the normal item draw
// (DrawItem_Draw) uses them.
//
// ShaderType (57 statics of 32 bytes from 0x01196A80, listed in four
// null-terminated tables at 0x00F88A08 / 0x00F88A4C / 0x00F88A68 /
// 0x00F88A8C; constructed by 0x005D9810): +0 vtable, +4 const char* name,
// +8 u32 name hash, +12 PtrVector of Shader* (+12 data, +16 u16 capacity,
// +20 u16 count). A material's +16 is its ShaderType. Slots: 0 Create
// (makes the shaders), 8 ~, 12 delete, 16 PickShader(type, material,
// item) -> Shader*.
//
// Shader (44 bytes): +0 vtable, +4 VProgram*, +8 FProgram*. Slots: 0 / 4
// small housekeeping, 8 Bind(shader, material), 12 Draw(shader, item), 16
// End(shader, 0).
//
// What the pickers read: material +44 u8 texture count, +100 u8 flags
// (bit 4: projected; item's material bit 7: layered), +120 u32 (top
// nibble: blend mode), +124 per-material parameters, +128 animation
// block; item +12 Mesh* (+84 bone palette: skinned, +112 skin), +28
// Material*, +44 layer list (+0 u32 layer index), +64, +69 u8 variant the
// picker writes back.
#pragma once

#include "draw3.h"

namespace sly4::render {
namespace draw3 {

namespace fn {
// Still recompiled.
constexpr u32 MaterialAnim_UploadUV = 0x0050D124;  // native (rsx.h) (anim block or 0, u32 first constant, f32 su [f1], f32 sv [f2]) 2 vertex constants (name?)
constexpr u32 FProgram_SetConstant = 0x0052F270;  // native (rsx.h) (u32 index, const f32 v[4]) into the current fragment program, marking it dirty on change (name?)
constexpr u32 Draw_SetParticleState = 0x0050CF50;  // native (rsx.h) (0, bool, 32); also called by the SAC_* Draw slots (unknown, name?)
constexpr u32 SlyCharacter2_Bind = 0x005BD298;  // native (rsx.h) shader vtable 0x00B25958 slot 8 (name?)
}  // namespace fn

namespace g {
constexpr u32 panorama_used = 0x011DE4EA;  // u8: set by SAC_Panorama's picker (name?)
}  // namespace g

// Material / item fields.
constexpr u32 kMatType = 16, kMatTextureCount = 44, kMatFlags = 100, kMatBlend = 120, kMatParams = 124, kMatAnim = 128;
constexpr u32 kItemMesh = 12, kItemMaterial = 28, kItemLayers = 44, kItemVariant = 69;
constexpr u32 kMeshPalette = 84, kMeshSkin = 112;
constexpr u32 kTypeShaders = 12;

}  // namespace draw3
}  // namespace sly4::render
