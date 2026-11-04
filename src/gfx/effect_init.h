// Post-effect start-up: the initialisers PostProc_InitEffects runs, which
// load each effect's vertex / fragment programs into fixed globals.
#pragma once

#include "../sly4.h"

namespace sly4::effect {

namespace fn {
constexpr u32 ThermalVision_Init = 0x00546334;  // decompiled: src/gfx/effect_init.cpp
constexpr u32 Bloom_Init = 0x0053D800;          // decompiled: src/gfx/effect_init.cpp
constexpr u32 Dof_Init = 0x00540824;            // decompiled: src/gfx/effect_init.cpp
constexpr u32 Effect_InitNone = 0x0053EC4C;     // decompiled: src/gfx/effect_init.cpp (empty)
constexpr u32 Monochrome_Init = 0x0053ECE0;     // decompiled: src/gfx/effect_init.cpp
constexpr u32 Warp_Init = 0x0054588C;           // decompiled: src/gfx/effect_init.cpp
constexpr u32 GodRays_Init = 0x00543774;        // decompiled: src/gfx/effect_init.cpp
constexpr u32 MotionTrail_Init = 0x005447F4;    // decompiled: src/gfx/effect_init.cpp
constexpr u32 BloomNoise_Init = 0x005420CC;     // decompiled: src/gfx/effect_init.cpp
constexpr u32 GodRays_Load = 0x0051EE94;        // decompiled: src/gfx/effect_init.cpp
constexpr u32 MotionTrail_Load = 0x0051F0FC;    // decompiled: src/gfx/effect_init.cpp
constexpr u32 Shader_Register = 0x0050AFEC;     // decompiled: src/gfx/material.cpp
}  // namespace fn

namespace g {
constexpr u32 thermal_shader = 0x011640F0;  // static shader object registered by ThermalVision_Init
}  // namespace g

}  // namespace sly4::effect
