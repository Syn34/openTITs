// The shaders' shared helpers that the Bind / Draw slots call (fog,
// UV transforms, fragment constants, warp matrices, wave constants), and
// the shader registry's set-up.
//
// View fog (Draw_SetParticleState): the current View (the object at
// draw::g::current_surface +672) holds 64-byte fog records from +128:
// +0 u8 enabled, +2 u8[4] colour, +8 f32 start, +12 f32 end.
//
// FProgram (the current fragment program, render state +28), as
// FProgram_SetConstant reads it: +16 u64 dirty mask (bit n: cached slot
// n changed), +24 u32 stamp (compared with render state +156), +48..
// 16-byte cached constant slots, +592.. per-constant records of 16 bytes
// (+0 nonzero: used, +12 u32 slot).
#pragma once

#include "shader2.h"

namespace sly4::render {
namespace shader3 {

namespace fn {
// Still recompiled.
constexpr u32 MaterialAnim_GetUV = 0x0050BE5C;     // (anim block; f1 su, f2 sv) -> const f32[8]: two UV rows (name?)
constexpr u32 MaterialAnim_GetUVAlt = 0x0050BF48;  // (anim block, f32 out[4]) MaterialAnim_UploadUVAlt's source (name?)
constexpr u32 Matrix_FromAxisAngle = 0x0056A410;   // (f32 out[16], const f32 axis[3]; f1 = angle)
constexpr u32 Matrix_ScaleRows = 0x00554394;       // (f32 rows[3][4], const f32 scale[3]) (decompiled: view/view2.cpp)
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 Vec3_ToItemSpace = 0x00553468;  // (f32 v[3] in / out, DrawItem*) a direction into the item's space (name?)
constexpr u32 SlyCharacter2_SetupDraw = 0x005BC8F8;  // (Shader*, DrawItem*) the item's transforms and lighting constants (name?)
// libgcm.
constexpr u32 cellGcmReserveMethodSize = 0x005E08E8;  // (context, u32 words): the callback (+12) when the buffer can't hold them (name?)
constexpr u32 cellGcmAddressToOffset = 0x00ABD5E4;    // import stub
}  // namespace fn

namespace g {
constexpr u32 material_time = 0x010DD5B0;  // f32 (layers3::g::block_dd5a8 +8): the clock material animations scroll by (name?)
constexpr u32 rotation_axis = 0x01164B70;  // f32[3] (uielem kRotationAxis)
constexpr u32 fog_enabled = 0x010F117C;    // u8[]: per fog index, the enable flag last set (render state +220) (name?)
constexpr u32 fog_colours = 0x010DD980;  // f32[2][4] (layers3::g::block_dd780 +512): per fog index, its colour / 255 (name?)
constexpr u32 fog_vconsts = 0x010DD9A0;  // f32[2][2][4] (+544): per fog index and enable flag, fog vertex constant 32 (name?)
constexpr u32 default_colour = 0x01164BB0;  // f32[4] (material.h material::g::default_colour)
constexpr u32 character_fconst2 = 0x010DDA50;  // f32[4] (layers3::g::block_dd780 +720): SlyCharacter2's fragment constant 2 (name?)
constexpr u32 skin_mode_override = 0x010F1380;  // u8 (render state +736): Skin_BuildJob's mode 1 for every part (name?)
}  // namespace g

// Render state (fx::g::state) fields.
constexpr u32 kStFragmentProgram = 28, kStProgramStamp = 156, kStFogIndex = 216;
// View fields.
constexpr u32 kViewFogs = 128, kFogSize = 64;
constexpr u32 kSurfaceView = 672;  // current surface +672: the current View
// The frame arena DrawItem_Create takes items from (layers3::g::block_dd600):
// +20 u32 cursor, +24 / +28 the current buffer's bounds.
constexpr u32 kArena = 0x010DD600, kArenaCursor = 20, kArenaLo = 24, kArenaHi = 28;
constexpr u32 kDrawItemSize = 80;
// A libgcm context: +0 begin, +4 end, +8 current, +12 callback OPD.
constexpr u32 kGcmEnd = 4, kGcmCurrent = 8, kGcmCallback = 12;
constexpr u32 kJumpFlag = 0x20000000;  // CELL_GCM_METHOD_FLAG_JUMP
// FProgram fields.
constexpr u32 kFpDirty = 16, kFpStamp = 24, kFpCache = 48, kFpConstants = 592;

// The shader registry's types (ShaderType, draw3_shader.h).
namespace types {
namespace fn {
// Still recompiled.
constexpr u32 Shader_Construct = 0x0050AFDC;     // (Shader*) the base Shader vtable 0x00B233A0 (name?)
constexpr u32 Shader_Load = 0x0053A9D8;          // (Shader*, vertex program name, fragment program name or 0, vparams, fparams) (name?)
constexpr u32 Shader_SetFpControl = 0x0053ABB8;  // (Shader*, u32 bits 20..25 of the fragment program's control word) (name?)
constexpr u32 PtrVector_Reserve = 0x00551718;    // (PtrVector*, u32 capacity)
constexpr u32 PtrVector_Clear = 0x0054EC1C;      // (PtrVector*)
constexpr u32 PtrVector_Construct = 0x005516E0;  // (PtrVector*)
constexpr u32 PtrVector_Init = 0x00551598;       // (PtrVector*, u32 capacity, 0, 0)
constexpr u32 Nop = 0x0054EC18;
constexpr u32 Mem_FreeFrom = 0x0054EE6C;         // (p, pool)
}  // namespace fn
constexpr u32 kShaders = 12, kCapacity = 16, kCount = 20, kHighWater = 22;  // ShaderType: PtrVector of Shader*
constexpr u32 kShaderSize = 44;
constexpr u32 kBaseVtable = 0x00B23F00;  // ShaderType's own vtable
}  // namespace types

namespace g {
constexpr u32 panorama_created = 0x011DE4E8;  // u8: SAC_PanoramaType_Create ran (name?)
// The four null-terminated ShaderType* tables Shader_RegisterAll walks: the
// first always, the others unless g::config +156 / +157 / +158 is set.
constexpr u32 type_tables[4] = {0x00F88A08, 0x00F88A4C, 0x00F88A68, 0x00F88A8C};
constexpr u32 group_texture_cursor = 0x00F66FB0;  // u32: shgroup::g::group_textures +8, + 56 (name?)
}  // namespace g

// The registry (shgroup::g::registry, made on first use): PtrVector of
// ShaderType*: +0 data, +4 u16 capacity, +6 u16, +8 u16 count, +10 u16
// high-water mark. A registered type's +24 gets bit 1.
constexpr u32 kRegData = 0, kRegCapacity = 4, kRegCount = 8, kRegHighWater = 10;
constexpr u32 kTypeFlags = 24, kTypeRegistered = 2;

}  // namespace shader3
}  // namespace sly4::render
