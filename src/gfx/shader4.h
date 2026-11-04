// The shaders' remaining small helpers (Shader set-up, UV animation
// sources, a 3x3 transform) and the Draw (slot 12) slots in
// shader4_draw*.cpp. (Shader / ShaderType layouts: draw3_shader.h.)
//
// MaterialAnim (material +128), as MaterialAnim_GetUVAlt reads it: +4 u8
// wrap flag, +12 f32 U steps (0: continuous), +44 f32 U offset, +60 f32 V
// steps, +92 f32 V offset.
#pragma once

#include "shader3.h"

namespace sly4::render {
namespace shader4 {

namespace fn {
// Decompiled in shader4.cpp: shader3::fn::MaterialAnim_GetUV,
// MaterialAnim_GetUVAlt, Vec3_ToItemSpace, SlyCharacter2_SetupDraw,
// shader3::types::fn::Shader_Construct, Shader_Load, Shader_SetFpControl.
// Still recompiled.
constexpr u32 MaterialAnim_GetMatrix = 0x0050B980;  // (MaterialAnim*) -> const f32*: the animated UV matrix (rows of 4) (name?)
constexpr u32 Vec3_RotateTo = 0x005533EC;  // (f32 out[3], const f32 v[3], const f32 m[3][4]) out = m rows . v (decompiled: shader4.cpp) (name?)
constexpr u32 Listener_LocalDirection = 0x005532E4;  // (f32 out[3], const f32 pos[3], const f32 m[16]) (pos - m row 3) in m's frame (decompiled: src/audio/lib_math.cpp)
constexpr u32 Matrix_Perspective = 0x0056A200;  // (f32 out[16]; f1 fov, f2 aspect, f3 near, f4 far): 1 / tan(fov * k) based, w = -z (name?)
constexpr u32 ProjTxr_BuildProjection = 0x005B0CF0;  // (f32 pos_out[3], f32 m_out[16], const f32 a[3], b[3], c[3], const f32 proj[16], DrawItem*, 0x01196A00) (name?)
constexpr u32 SelfLight_GetLight = 0x005B47F4;  // (Shader*, DrawItem*, f32 a[4], f32 dir[4], f32 b[4]): vertex constants 20 / 23 / 26 (name?)
constexpr u32 Matrix_RotateRows = 0x00555370;  // (f32 out[3][4], const f32 rows[3][4], const f32 m[4][4]) out_i = m^T-style rows_times(m, rows_i) (decompiled: shader4_draw.cpp) (name?)
// libgcm Cg helpers.
constexpr u32 Cg_GetIOMask = 0x005E6028;  // (CgProgram*) a vertex program's attribute output mask (+16) / a fragment program's input mask (+4) (name?)
constexpr u32 Cg_SetIOMask = 0x005E608C;  // (CgProgram*, u32) the same word written (name?)
}  // namespace fn

namespace g {
constexpr u32 uv_rows = 0x010C5C80;  // f32[8]: MaterialAnim_GetUV's result (name?)
}  // namespace g

constexpr u32 kShaderName = 12;          // Shader +12: char[32] vertex program name
constexpr u32 kFpLoadOffset = 1136;      // FProgram +1136 (post.h)
constexpr u32 kTexInputBits = 0x00000FC0;  // rlwimi 0,20,25

}  // namespace shader4
}  // namespace sly4::render
