// The shaders' last helpers (the animated UV matrix, the projector, the
// self-lit light, the view constants block) and the remaining Draw (slot
// 12) slots in shader5_draw*.cpp. (Shader / ShaderType layouts:
// draw3_shader.h.)
//
// MaterialAnim (material +128) as MaterialAnim_GetMatrix reads it, beyond
// shader4.h's: +128 / +132 f32 rotation pivot (U, V), +108 f32 rotation
// steps (0: continuous), +140 f32 rotation (turns).
//
// The view constants block (layers3::g::block_dd780, 0x010DD780) as
// View_SetConstants fills it from the View: +0 view +1008.. (f32[4][4]),
// +64 view +880.. (layers4::g::view_proj, f32[4][4]), +128 view +944..
// (f32[4][4]), +192 / +208 / +224 / +240 view +752 / +768 / +784 / +800
// (f32[3] each: the camera rows and position), +256.. the 3x3 camera
// rotation transposed and +304 the camera position through it negated
// (render::g::camera_pos), +320.. view +1200.. and +384.. view +1072..
// (f32[3] rows), +512 / +528 view +144 / +208, +544 / +560 / +576 /
// +592 view +160 / +176 / +224 / +240 (f32[4] each), +612 f32 min(1,
// tan(fov / 2) / tan(k)), +616 u32 draw::g::current_surface +4, +624..
// g::config's 6 vec4s at 0x010C5B00 times g::config +320, +720 {0, 1,
// 0, 0}.
#pragma once

#include "shader4_util.h"

namespace sly4::render {
namespace shader5 {

namespace fn {
// Decompiled in shader5_util.cpp: shader4::fn::MaterialAnim_GetMatrix,
// Matrix_Perspective, ProjTxr_BuildProjection, SelfLight_GetLight and
// View_SetConstants (0x005207D0).
constexpr u32 View_SetConstants = 0x005207D0;  // (View*) the view constants block (name?)
// Still recompiled.
constexpr u32 Material_ParamSlot = 0x0050C018;  // (Material*, u32 index) -> u32*: &params[index], growing material +124 (u8 capacity +121) (name?)
constexpr u32 Map_Find = 0x007A68DC;            // (map, const u32* key) -> value* (sorted, qsort on demand) (name?)
constexpr u32 Matrix_LookAt = 0x0056A0D0;       // (f32 out[16], eye, target, up) (world/world.h)
constexpr u32 Matrix_MultiplyTo = 0x0055499C;   // (f32 out[16], const f32 a[16], const f32 b[16]): rows of a through b (Vec4_TransformByMatrix x4) (name?)
// libm.
constexpr u32 tanf = 0x005E62E8;  // f1 -> f1 (name?)
}  // namespace fn

namespace g {
constexpr u32 anim_matrix = 0x010C5C40;      // f32[4][4]: MaterialAnim_GetMatrix's result (name?)
constexpr u32 anim_identity = 0x01164BD0;    // f32[4][4] the identity the anim matrices start from (name?)
constexpr u32 anim_pivot_out = 0x010F0BE0;   // f32[4][4]: identity with row 2 (-pivot, 1) (name?)
constexpr u32 anim_pivot_out_set = 0x010F0BD0;  // u8: anim_pivot_out initialised (name?)
constexpr u32 anim_pivot_in = 0x010F0C20;    // f32[4][4]: identity with row 2 (pivot, 1) (name?)
constexpr u32 anim_pivot_in_set = 0x010F0BD8;   // u8: anim_pivot_in initialised (name?)
constexpr u32 light_map = 0x00F8F130;        // u32: object whose +16 is the map SelfLight_GetLight resolves its light in (name?)
constexpr u32 view_block = 0x010DD780;       // layers3::g::block_dd780
constexpr u32 fov_ref_set = 0x010F0DC8;      // u8: fov_ref computed (name?)
constexpr u32 fov_ref = 0x010F0DD0;          // f32: tan(k), View_SetConstants' reference (name?)
constexpr u32 config_vec4s = 0x010C5B00;     // g::config +224: f32[6][4] (name?)
}  // namespace g

}  // namespace shader5
}  // namespace sly4::render
