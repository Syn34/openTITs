// The UI file loader: element descriptions read from XML.
#pragma once

#include "../sly4.h"

namespace sly4 {
namespace uiload {

using namespace opentit;

namespace ufn {
constexpr u32 XmlCursor_DocPath = 0x004F8944;
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;
constexpr u32 XmlCursor_ReadBool = 0x004F986C;
constexpr u32 XmlCursor_ReadS32 = 0x004F99BC;
constexpr u32 XmlCursor_ReadF32 = 0x004F9B20;
constexpr u32 XmlCursor_ReadHash = 0x004FA14C;
constexpr u32 String_HashNoCase = 0x0056A710;
}  // namespace ufn

constexpr u32 kResList = 0x00F66864;  // UiRes*, linked by +88, name hash at +76
constexpr u32 kTexDefaults = 0x010DDADC;  // Texture*[]: Tex_GetDefault (name?)

// Text description attributes (UiTextDesc_FromXml).
constexpr u32 kStrFont = 0x00AEF7BC, kStrString = 0x00AEF7D0;
constexpr u32 kHashLineSpacing = 0x2DCBF6E6;
constexpr u32 kHashCharSpacing = 0x7A600EB7;
constexpr u32 kHashWrap = 0x0028C84A;
constexpr u32 kHashMonospace = 0x8403BAD2;
constexpr u32 kHashRevealRate = 0xFF478E71;
constexpr u32 kHashGlyphMode = 0xFF457B34;
constexpr u32 kGlyphModeHashes = 0x00AEF7D8;  // u32[5]: the glyph mode names' hashes, by mode
constexpr u32 kGlyphModeCount = 5;
constexpr u32 kEmptyString = 0x00AEF4A0;      // ""
constexpr u32 kTextDescDefaults = 0x004DC714; // f32 { 1.0 line spacing / reveal numerator, 0.0 char spacing, 3.0 reveal rate }

}  // namespace uiload
}  // namespace sly4

namespace sly4 {
namespace uiload {

namespace efn {
constexpr u32 UiTextDesc_FromXml = 0x004DC720;
constexpr u32 UiText5Desc_FromXml = 0x004DCA08;
constexpr u32 UiText_Construct = 0x004DC430;
constexpr u32 UiText5_Construct = 0x004DC5E0;
constexpr u32 Mem_Alloc = 0x0054ED38;
}  // namespace efn

// Type-5 attributes (UiText5Desc_FromXml).
constexpr u32 kHashT5_168 = 0xFC87E57A, kHashT5Width = 0xB75E2EC9, kHashT5_176 = 0x77B9554D;
constexpr u32 kHashT5Start = 0x911A2AA2, kHashT5Flag = 0x280D85C8;
constexpr u32 kText5DescConsts = 0x004DCA00;  // f32 { 0.0, 1.0 }
constexpr u32 kVfnType = 12;                  // element: () -> s32 (5 for this class)

}  // namespace uiload
}  // namespace sly4
