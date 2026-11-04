// AglMaterial construction and shader registration.
#pragma once

#include "../sly4.h"

namespace sly4::material {

namespace fn {
constexpr u32 Material_Construct = 0x004FF0D8;  // decompiled: src/gfx/material.cpp
constexpr u32 Shader_Register = 0x0050AFEC;     // decompiled: src/gfx/material.cpp
// Still recompiled.
constexpr u32 PtrVector_Construct = 0x005516E0;
constexpr u32 PtrVector_Init = 0x00551598;      // (vector, capacity, 0, 0) as used here
constexpr u32 PtrVector_Reserve = 0x00551718;   // (vector, capacity)
}  // namespace fn

namespace g {
constexpr u32 vt_base0 = 0x00B242B8;          // written first, as by RenderSurface_Construct
constexpr u32 vt_material_base = 0x00B233B0;
constexpr u32 serial = 0x01164CD0;            // u32: shared with RenderSurface_Construct
constexpr u32 material_count = 0x010DD5C0;    // u32: materials made (id in +46)
constexpr u32 default_colour = 0x01164BB0;    // 4 x f32, copied to +64 and +80
constexpr u32 registry_made = 0x010F0BA0;     // u8  (see shadergroup.h)
constexpr u32 registry = 0x010F0BA8;          // PtrVector* of shaders
}  // namespace g

}  // namespace sly4::material
