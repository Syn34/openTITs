// The app-level text element class (vtable 0x00B1C9D0, type 100): derived
// from UiText (vtable 0x00B227D8), whose base is the element class
// (vtable 0x00B22710). It keeps an extra source string at +260.
#pragma once

#include "text_slots.h"

namespace sly4 {
namespace uielem {

namespace sfn2 {
constexpr u32 UiText_Construct = 0x004DC430;   // (TextElem*, const ElemDesc*) (name?)
constexpr u32 UiText2_Construct = 0x004DC5BC;  // (TextElem2*, const ElemDesc*) (name?)
constexpr u32 UiElem_CloneChildren = 0x004D7634;  // (Element* from, Element* to, u32) (name?)
constexpr u32 UiText_ReserveText = 0x004DB780;    // (TextElem*, u32 bytes) (name?)
constexpr u32 UiText_ReserveQuads = 0x004DB6CC;   // (TextElem*, u32 glyphs) (name?)
constexpr u32 UiText_Format = 0x004DB828;
constexpr u32 SmallAlloc = 0x0055238C;
constexpr u32 String_Assign = 0x00555F80;
constexpr u32 Mem_Alloc = 0x0054ED38;
}  // namespace sfn2

constexpr u32 kText2Vtbl = 0x00B1C9D0;
constexpr u32 kText2Source = 260;   // char*: the text as given (kept for clones)
constexpr u32 kText2Glyphs = 264;   // u32 extra glyphs reserved on SetText
constexpr u32 kText2SourceCap = 272;
constexpr u32 kTextSize = 240, kText2Size = 288;
constexpr u32 kVfnClone = 8;        // (name, parent, u32, bool deep) -> Element*

// Construction description (ElemDesc) both clones fill in.
constexpr u32 kDescName = 0, kDescMatrix = 16, kDescColor = 96, kDescFieldA = 100, kDescParent = 104;
constexpr u32 kDescArg = 108, kDescFieldB = 112, kDescFieldC = 116, kDescFlags = 120;  // u8[7]
constexpr u32 kDescFontSet = 128, kDescText = 132, kDescLineSpacing = 136, kDescCharSpacing = 140;
constexpr u32 kDescWrap = 144, kDescField26 = 148, kDescMonospace = 152, kDescGlyphMode = 156;
constexpr u32 kDescRevealRate = 160, kDescByte164 = 164, kDescSource = 168;

}  // namespace uielem
}  // namespace sly4
