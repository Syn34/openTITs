// The renderer's remaining small helpers (gfx7*.cpp): matrix leaves the
// shader Draw slots call, and the fur shell count.
#pragma once

#include "shader6.h"

namespace sly4::render {
namespace gfx7 {

namespace fn {
// Decompiled in gfx7_math.cpp.
constexpr u32 Matrix_SplitScale = 0x00554858;  // (const f32 m[16], f32 rot[16], f32 scale[3]) (name?)
constexpr u32 Matrix_RotationZ = 0x00553F18;   // (f32 out[16]; f1 angle) -> f1 cos (name?)
constexpr u32 Fur_ShellCount = 0x005BFB50;     // (Shader*, DrawItem*, f32* fade) -> u16 shells (name?)
}  // namespace fn

// Matrix_RotationZ's .text constants (0x00553EF0): f64 pi / -pi (as
// floats), f32 2pi, 4/pi, -4/pi^2, 0.225, pi/2, pi.
constexpr u32 kRotZConsts = 0x00553EF0;
// Matrix_SplitScale's 1.0f.
constexpr u32 kSplitScaleOne = 0x00554850;
// Fur_ShellCount's .text constants (0x005BFB10): f32 12.0 (near), f64 1.0,
// f32 1.0, f64 0.0, f32 18.0 (+32, fade range), f32 2.0 (+56), f32 0.5 (+60).
constexpr u32 kFurConsts = 0x005BFB10;

}  // namespace gfx7
}  // namespace sly4::render
