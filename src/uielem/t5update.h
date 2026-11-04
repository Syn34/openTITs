// UiText5_Update: the type-5 text element's per-frame update (vtable
// 0x00B22B78 +152). While a layout is pending it lays the text out on one
// line, clipped to the element's width (+240) and scrolled by +248; with
// scrolling on (+256 bit 0x80, speed +236) it keeps the layout pending so
// the text runs every frame, wrapping the scroll at the text's width.
#pragma once

#include "../text/text.h"

namespace sly4 {
namespace t5upd {

using namespace opentit;

namespace ufn {
constexpr u32 Utf8_Next = 0x004DA380;  // (e, const char** p, u32* len, bool advance) -> code point
}

constexpr u32 kConsts = 0x004E7090;  // f32 { 0.5, 0.0, -0.5, 3.0, 0.25 }, f64 4294967295.0 at +24

// Element fields (kElemFlags, kTextLineCount, kTextGlyphCount, kVfnGetScale: text.h).
constexpr u32 kElemSize = 140;          // f32[2]: the size as last measured (x = the text's width here)
constexpr u32 kElemFirstChild = 152, kElemNext = 156;
constexpr u32 kText5Width = 240;        // f32: glyphs from x > width aren't drawn
constexpr u32 kText5Scroll = 248;       // f32
constexpr u32 kText5Speed = 236;        // f32 scroll speed (units per second)
constexpr u32 kText5Flags = 256;        // u8: 0x80 scrolls
constexpr u32 kTextReveal = 224, kTextRevealElapsed = 228;  // f32 seconds per glyph, f32 elapsed
constexpr u32 kTextLayoutPending = 0x40000000;
constexpr u32 kElemSizeDirty = 0x02000000;

// Virtual functions.
constexpr u32 kVfnUpdate = 152;   // (f1 dt)
constexpr u32 kVfnColor = 172;    // sret u8[4]
constexpr u32 kVfnGetSize = 140;  // sret f32[2]
constexpr u32 kVfnFieldB = 68;    // () -> u32 (the result is not used here)

// Font fields.
constexpr u32 kFontFlags = 101;   // u8: 0x80 -> quad bit 19
constexpr u32 kFontPages = 112;   // Material*[] by blend index

}  // namespace t5upd
}  // namespace sly4
