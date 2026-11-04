// UiText_BuildGlyph: one glyph quad of a text element.
#pragma once

#include "text.h"

namespace sly4 {
namespace text {

// A glyph quad (32 bytes, the element's +208 buffer).
constexpr u32 kQuadX = 0, kQuadY = 4, kQuadW = 8, kQuadH = 12;
constexpr u32 kQuadBits = 16;  // u64 bit-fields, see below
constexpr u32 kQuadPage = 24;  // u32: the font's page for the element's field A (Font +112[4])
constexpr u32 kQuad28 = 28;    // u32, set to 0

// kQuadBits fields (bit numbers from the least significant bit).
constexpr u32 kBitsTexU = 53;     // 11 bits: glyph +4 (u16)
constexpr u32 kBitsTexV = 42;     // 11 bits: glyph +6 (u16)
constexpr u32 kBitsWidth = 31;    // 11 bits: glyph +2 (advance in texels)
constexpr u32 kBitsHeight = 20;   // 11 bits: glyph +0 + glyph +1 + 1
constexpr u32 kBitsFontFlag = 19; // 1 bit: Font +101 bit 0x80
constexpr u32 kBitsChannel = 15;  // 4 bits: glyph +3 (name?)
constexpr u32 kBitsChannel2 = 11; // 4 bits: (glyph +3) - 1 when text flags bits 22-25 >= 3, else 0
constexpr u32 kBitsLine = 0;      // 10 bits: the line index
// bit 10 is kept as it was.

// Glyph (8 bytes): +0 u8 top, +1 u8 rows, +2 u8 advance, +3 u8 channel?, +4 u16 u, +6 u16 v.
constexpr u32 kFontFlags = 101;   // u8: 0x80 copied to kBitsFontFlag
constexpr u32 kFontPages = 112;   // u32[4] by element field A (+136 bits 22-23)

// kTextMonospace (0x80000000 of +180) is in text.h.

}  // namespace text
}  // namespace sly4
