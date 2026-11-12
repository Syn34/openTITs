// Camera-side helpers: 4x4 matrix inverse / determinant / copy, the
// homogeneous vector transform, the view's backdrop gradient and the
// camera's per-split accessors (0x002AB9xx).
//
// View (see view2.h): +880 f32[16] view-projection matrix (the backdrop
// unprojects through its inverse).
#pragma once

#include "view.h"
#include "view2.h"

namespace sly4 {
namespace view {

namespace cfn {
constexpr u32 Mat4_Determinant = 0x00554A2C;  // f32 (const f32 m[16])            decompiled: src/view/camera_mat.cpp
constexpr u32 Mat4_Invert = 0x00554C28;       // (f32 out[16], f32 tmp[16], const f32 in[16])  decompiled: src/view/camera_mat.cpp
constexpr u32 Mat4_Copy = 0x007A430C;         // (f32 dst[16], const f32 src[16]) decompiled: src/view/camera_mat.cpp
constexpr u32 Vec4_Transform = 0x005539E4;    // (f32 v[4] in/out, const Matrix*) decompiled: src/view/camera_mat.cpp
constexpr u32 View_DrawBackdrop = 0x004583F8; // (World*, View*, const u8 top[4], const u8 bottom[4]) decompiled: src/view/camera_backdrop.cpp
// Still recompiled.
constexpr u32 Draw_SetTexture = 0x00523310;   // (FrameBuffer*, ShaderGroup*)
constexpr u32 Draw_SetState = 0x005218D8;     // (FrameBuffer*, const f32 matrix[16])
constexpr u32 Draw_BeginPrims = 0x005236A0;   // (Prims* out, FrameBuffer*, u32 type, u32 vertices) reads r7
constexpr u32 Draw_EndPrims = 0x00523748;     // (FrameBuffer*)
constexpr u32 Shader_LoadGroup = 0x0050F0F0;  // (s32 group) -> ShaderGroup*
}  // namespace cfn

namespace cg {
constexpr u32 current_world = 0x00F65CA0;  // u32 World*
constexpr u32 world_frame_ring = 0x1285E4; // World +: FrameRing*, current FrameBuffer at +12
constexpr u32 identity = 0x01164BD0;       // f32[4][4] identity
}  // namespace cg

f32 Mat4_Determinant(const be_f32* m);
void Mat4_Invert(be_f32* out, be_f32* tmp, const be_f32* in);
void Mat4_Copy(be_f32* dst, const be_f32* src);
void Vec4_Transform(be_f32* v, const void* m);
void View_DrawBackdrop(void* world, void* view, const u8* top, const u8* bottom);
void Camera_SetSplitStyle(void* cam, u32 index, u32 key);
u32 Camera_GetSplitA(void* cam, u32 index);
u32 Camera_GetSplitB(void* cam, u32 index);
u32 Camera_GetSplitXform(void* cam, u32 index);
void Camera_GetSplitColourA(u8* out, void* cam, u32 index);
void Camera_GetSplitColourB(u8* out, void* cam, u32 index);

}  // namespace view
}  // namespace sly4
