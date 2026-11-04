// The display render surface (the AglRenderSurface at g::display) and the
// frame-buffer surface records bound to it.
//
// Display object (from the AglRenderSurface free list):
//   +16..  Target* colour targets (Target_Init), +24 depth target
//   +44    u32 clear mask? (240 colour, 243 with depth, 3 depth only)
//   +52    u32 bits 28..29 colour target count, 27 depth, 26 colour,
//          25 targets made at creation (Display_CreateTargets flags & 1),
//          22..24 extra eye-pair count (Display_Finish)
//   +56    u32 width, +60 u32 height
//   +64..  Target* screen-size copies, +92.. 3D copies (Display_Finish)
//   +137   u8 set once the video mode is configured
//   +148   u8 bit 7: use the alternate buffer set
#pragma once

#include "../sly4.h"

namespace sly4::disp {

namespace fn {
constexpr u32 Display_CreateTargets = 0x0051B228;  // decompiled: src/gfx/display.cpp
constexpr u32 Display_Finish = 0x0053074C;         // decompiled: src/gfx/display.cpp
constexpr u32 Display_SetSize = 0x00533054;        // decompiled: src/gfx/display.cpp
// Still recompiled.
constexpr u32 RenderSurface_Alloc = 0x0051AF28;  // () -> AglRenderSurface* from the free list (name?)
constexpr u32 Target_InitDepth = 0x00530A9C;     // (Target** out, w, h, 7, 0) depth target with tile + zcull (name?)
constexpr u32 Surface_Create = 0x005304D4;       // decompiled: src/gfx/target.cpp
constexpr u32 Target_Init = 0x00530DB4;          // decompiled: src/gfx/target.cpp
constexpr u32 Target_Attach = 0x00532E70;        // decompiled: src/gfx/target.cpp
constexpr u32 Surface_SetBuffer = 0x00532D5C;    // decompiled: src/gfx/target.cpp
}  // namespace fn

namespace g {
constexpr u32 display = 0x00F66FBC;            // AglRenderSurface*: the display
constexpr u32 surface_list_made = 0x00F670D9;  // u8
constexpr u32 surface_list = 0x00F8F110;       // 16-byte AglRenderSurface free-list header: +4 capacity
// 52-byte surface records with their lazy-construction guards.
constexpr u32 attach_target = 0x011647A8;  // Target*: the full-width 720-row copy (Display_SetSize)
constexpr u32 wide_surface = 0x011646E8, wide_guard = 0x011646E0;
constexpr u32 eye_surface0 = 0x011646A8, eye_guard0 = 0x011646A0;
constexpr u32 eye_surface1 = 0x01164668, eye_guard1 = 0x01164660;
constexpr u32 pair_a = 0x011641F0, pair_a_guard = 0x01164658;  // two records, 52 bytes apart
constexpr u32 pair_b = 0x01164258, pair_b_guard = 0x01164650;  // two records, 52 bytes apart
}  // namespace g

}  // namespace sly4::disp
