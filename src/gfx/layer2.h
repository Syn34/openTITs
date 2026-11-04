// Render layer internals still left by render_layer.cpp: the item draw,
// the layer end, the two entry sorts and the layer table set-up.
//
// Draw binding (g::binding, 0x010DC840): +0 Material* bound, +4 Shader*
// bound, +8 u32 (cleared with them).
// DrawItem: +12 (sort key low bits at +80), +28 Material* (+16 an object
// whose slot 16 picks the Shader for (material, item)).
// Shader vtable: slot 12 Draw(shader, item), slot 16 End(shader, 0).
#pragma once

#include "render.h"

namespace sly4::render {
namespace layer2 {

namespace fn {
constexpr u32 Material_Bind = 0x0079CD0C;  // (Material*, Shader*) (name?)
constexpr u32 memmove = 0x0071C3F0;
}  // namespace fn

namespace g {
constexpr u32 binding = 0x010DC840;  // {Material* +0, Shader* +4, u32 +8} (name?)
}  // namespace g

constexpr u32 kShaderDraw = 12, kShaderEnd = 16, kPickShader = 16;

}  // namespace layer2
}  // namespace sly4::render
