// Post-effect start-up: the initialisers PostProc_InitEffects runs, which
// load each effect's vertex / fragment programs into fixed globals.
#pragma once

#include "../sly4.h"

namespace sly4::effect {

// The initialisers (ThermalVision_Init 0x00546334, Bloom_Init 0x0053D800,
// Dof_Init 0x00540824, Effect_InitNone 0x0053EC4C, Monochrome_Init 0x0053ECE0,
// Warp_Init 0x0054588C, GodRays_Init 0x00543774, MotionTrail_Init 0x005447F4,
// BloomNoise_Init 0x005420CC, GodRays_Load 0x0051EE94, MotionTrail_Load
// 0x0051F0FC) are native: src/gfx/effect_init.cpp, declared in rsx.h.

namespace g {
constexpr u32 thermal_shader = 0x011640F0;  // static shader object registered by ThermalVision_Init
}  // namespace g

}  // namespace sly4::effect
