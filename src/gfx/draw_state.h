// Low-level draw helpers used by the post effects: texture units, the
// render target, depth / blend defaults and the full-screen quad.
//
// Render state at post::g::gcm_state (0x010F10A0), beyond effect_draw.h:
//   +32 u16 enabled vertex attribute mask, +36.. Target* per colour slot,
//   +52.. Target* per texture unit, +93 u8 bit 0 depth test,
//   +96 CellGcmSurface (type, antialias, colour format, target, location
//   per slot +100.., offsets +104.., pitches +120.., width/height/x/y
//   +148..+154), +222.. / +225.. texture filter tables.
#pragma once

#include "../sly4.h"

namespace sly4::draw {

namespace fn {
constexpr u32 Draw_SetTexture0 = 0x00536AC0;     // decompiled: src/gfx/draw_state.cpp
constexpr u32 Draw_SetTarget = 0x005373A8;       // decompiled: src/gfx/draw_state.cpp
constexpr u32 Draw_ResetState = 0x005355D8;      // decompiled: src/gfx/draw_state.cpp
constexpr u32 Draw_PrepareQuad = 0x0053539C;     // decompiled: src/gfx/draw_state.cpp
constexpr u32 Draw_RestoreDepth = 0x00535294;    // decompiled: src/gfx/draw_state.cpp
constexpr u32 Draw_BindScreenTargets = 0x00513038;  // decompiled: src/gfx/draw_state.cpp
// Still recompiled: game code.
constexpr u32 Draw_BindDisplay = 0x0053585C;   // () (name?)
constexpr u32 Draw_BindTargets = 0x00532090;   // (Target* depth, Target* colour, 0) (name?)
constexpr u32 FProgram_Upload = 0x0052F4F0;    // (FProgram** current) (name?)
// Still recompiled: libgcm.
constexpr u32 cellGcmSetTexture = 0x005E4514;         // (context, index, CellGcmTexture*) (name?)
constexpr u32 cellGcmSetTextureControl = 0x005E3F18;  // (context, index, enable, minlod, maxlod, aniso) (name?)
constexpr u32 cellGcmSetTextureFilter = 0x005E3E88;   // (context, index, bias, min, mag, conv) (name?)
constexpr u32 cellGcmSetSurface = 0x005E47A0;         // (context, CellGcmSurface*) (name?)
constexpr u32 cellGcmSetViewport = 0x005E4068;        // (context, x, y, w, h, min, max, scale, offset) (name?)
constexpr u32 cellGcmSetFragmentProgram = 0x005E42A0; // (context, program, 1) (name?)
constexpr u32 cellGcmSetDrawInlineArray = 0x005E51AC; // (context, mode 9, count, data) (name?)
constexpr u32 cellGcmSetColorMask = 0x005E5CB0;       // (context, mask, 4, f32[4]*) (name?)
}  // namespace fn

namespace g {
constexpr u32 render_state = 0x010F10A0;
constexpr u32 quad = 0x010F1020;           // +0.. 12-byte attribute list, +120 u16 vertex count, +126 u16 attribute mask
constexpr u32 quad_vertices = 0x00F671E4;  // vertex data for the quad draw
constexpr u32 viewport_consts = 0x00537398;  // f32 { 0.5, -0.5, 1.0, 0.0 }
constexpr u32 current_surface = 0x010DDA74;  // u32: object whose +512 is the surface being drawn (name?)
constexpr u32 display = 0x00F66FBC;
}  // namespace g

}  // namespace sly4::draw
