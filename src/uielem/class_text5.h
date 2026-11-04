// The type-5 element class (vtable 0x00B22B78, 272 bytes): a UiText that
// clips its width (to its parent's, or to the text it shows) and scrolls.
#pragma once

#include "class.h"

namespace sly4 {
namespace uielem {

constexpr u32 kText5Size = 272;
constexpr u32 kText5_236 = 236;      // f32 = desc +168 (name?)
constexpr u32 kText5Width = 240;     // f32 = desc +172, 0 = the parent's width; clipped to the text's
constexpr u32 kText5_244 = 244;      // f32 = desc +176 (name?)
constexpr u32 kText5Scroll = 248;    // f32: reset to kText5Start by Invalidate (name?)
constexpr u32 kText5Start = 252;     // f32 = desc +180 (name?)
constexpr u32 kText5Flags = 256;     // u8: 0x80 and 0x40 both = desc +184 bit 0 (name?)

// Description fields after the text ones (desc +0..+164, see slots.h).
constexpr u32 kDesc5_168 = 168, kDesc5Width = 172, kDesc5_176 = 176, kDesc5Start = 180, kDesc5Flag = 184;
constexpr u32 kDesc5Size = 188;

namespace t5fn {
constexpr u32 UiText5_Construct = 0x004DC5E0;
constexpr u32 UiText5_GetSize = 0x004DA4C0;  // (sret f32[2], e) [+140]
constexpr u32 UiText_Destruct = 0x004E1604;
constexpr u32 UiText_CountGlyphs = 0x004E2178;
constexpr u32 Mem_Alloc = 0x0054ED38;
}  // namespace t5fn

constexpr u32 kText5Consts = 0x004DC5DC;  // f32 0.0

}  // namespace uielem
}  // namespace sly4
