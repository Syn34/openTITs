// Video-out set-up: choosing the resolution (Video_SelectMode) and
// configuring the output and the display buffers (Video_Configure).
//
// g::mem_stats fields used here (cellVideoOutGetState writes +0..+15):
//   +8 u8 resolution id, +11 u8 aspect, +14 u16 refresh rates,
//   +16 CellVideoOutResolution 2D, +20 CellVideoOutResolution 3D,
//   +24 u32 3D resolution id, +28 u32 2D resolution id,
//   +56 f32 1.0, +160 f32 screen size (inches), +184 u32 eye (1 = left?).
// g::config fields: +4/+6 u16 width/height, +8/+10 u16 0,
//   +20 f32 refresh rate, +24/+26 u16 5% safe-area margins,
//   +28/+30 u16 the size inside them, +33 u8 aspect is not 4:3,
//   +34 u8 3D in use, +35 u8 3D available.
#pragma once

#include "../sly4.h"

namespace sly4::vsetup {

namespace imp {
constexpr u32 cellGcmResetFlipStatus = 0x00ABD804;
constexpr u32 cellGcmSetFlipMode = 0x00ABD644;
constexpr u32 cellGcmSetDisplayBuffer = 0x00ABD784;
constexpr u32 cellVideoOutConfigure = 0x00ABD9A4;
constexpr u32 cellVideoOutGetState = 0x00ABDB24;
constexpr u32 cellVideoOutGetResolutionAvailability = 0x00ABDB84;
constexpr u32 cellVideoOutGetResolution = 0x00ABDBC4;
constexpr u32 cellVideoOutGetScreenSize = 0x00ABE3C4;
}  // namespace imp

namespace fn {
constexpr u32 Video_SelectMode = 0x0052FD2C;  // native: src/gfx/setup.cpp (declared in rsx.h)
constexpr u32 Video_Configure = 0x005312E4;   // native: src/gfx/setup.cpp (declared in rsx.h)
// Still recompiled.
constexpr u32 printf = 0x007240F4;
constexpr u32 Gcm_Sub5E0E50 = 0x005E0E50;  // (context) libgcm? (unknown)
constexpr u32 Gcm_Sub5E3B84 = 0x005E3B84;  // (context, frame count) libgcm? (unknown)
}  // namespace fn

namespace g {
constexpr u32 mode_names = 0x00F67840;    // 24 x {u32 resolution id, const char* name}
constexpr u32 aspect_names = 0x00F67900;  // 3 x {u32 aspect id, const char* name}
constexpr u32 gcm_context = 0x010F10A0;   // u32 passed to 005E0E50 / 005E3B84 (name?)
constexpr u32 frame_count = 0x00F67918;   // u32
}  // namespace g

}  // namespace sly4::vsetup
