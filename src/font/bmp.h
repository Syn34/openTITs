// Font_Load: a bitmap font's pages (materials on its texture) and its
// glyph table, found by scanning a BMP file for the cell outlines drawn
// around each glyph.
//
// The BMP (the font resource's path) is a picture of the font's texture
// with markers. Scanning rows from the top, the first row with a pixel of
// blue >= 240 is a baseline row; along it, each pixel with green >= 240
// starts a glyph cell whose outline (green >= 240) runs up the left side,
// along the top and down the right side. A pixel with red >= 240 (and
// green < 240) right after a glyph widens that glyph to more cells. A
// baseline row without glyphs ends the scan.
#pragma once

#include "res.h"

namespace sly4 {
namespace bmpfont {

using namespace opentit;

namespace bfn {
constexpr u32 Bmp_GetPixel = 0x004D65EC;  // (u8 out[4], ...) see below
constexpr u32 Tex_GetDefault = 0x004FEF30;
constexpr u32 Tex_Acquire = 0x00523B7C;   // (Texture*) -> Material*
constexpr u32 File_LoadWhole = 0x00560B90;
constexpr u32 Mem_Free = 0x005611B0;
constexpr u32 Mem_Alloc = 0x0054ED38;
constexpr u32 memmove = 0x0071C3F0;
}  // namespace bfn

// Font fields (see src/font/font.cpp, src/text/text.h).
constexpr u32 kFontPath = 6;         // char[] (UiRes path): the BMP
constexpr u32 kFontTexture = 92;     // UiTexture*
constexpr u32 kFontGlyphCount = 100; // u8
constexpr u32 kFontFlags = 101;      // u8: 0x40 depth func LEQUAL (else ALWAYS), 0x20 depth test
constexpr u32 kFontLineHeight = 102; // u16
constexpr u32 kFontGlyphs = 104;     // Glyph[]
constexpr u32 kFontPages = 112;      // Material*[3], one per blend index

// Glyph (8 bytes).
constexpr u32 kGlyphAbove = 0;   // u8 rows from the cell top to the baseline row
constexpr u32 kGlyphBelow = 1;   // u8 rows from the baseline row to the cell bottom
constexpr u32 kGlyphWidth = 2;   // u8 (the advance)
constexpr u32 kGlyphCells = 3;   // u8 1, more when widened
constexpr u32 kGlyphX = 4;       // u16 left
constexpr u32 kGlyphY = 6;       // u16 top
constexpr u32 kGlyphSize = 8;
constexpr u32 kMaxGlyphs = 256;  // a full table is stored as a count of 0 (u8), as in the original

constexpr u8 kMarker = 240;      // channel value that marks outlines

// BMP header (little endian).
constexpr u32 kBmpDataOffset = 10, kBmpHeaderSize = 14, kBmpWidth = 18, kBmpHeight = 22, kBmpBpp = 28;

}  // namespace bmpfont
}  // namespace sly4
