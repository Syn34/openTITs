// The UI file loader: element descriptions read from XML.
#pragma once

#include <cstddef>

#include "opentit/native.h"
#include "../sly4.h"
#include "../text/text.h"
#include "../xml/xml.h"

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

// ---------------------------------------------------------------- native structures
using native::gp;
using native::pad;
using xml::XmlCursor;

// The description an element is built from (Ui_LoadElement fills it; the
// builders copy it into their own and add their type's fields).
struct ElemDesc {
    gp<const char> name;       // +0
    pad<12> _4;
    be_f32 matrix[4][4];       // +16: rows' xyz (w not used)
    be_f32 scale[3];           // +80
    pad<4> _92;
    u8 color[4];               // +96
    be_u32 blend;              // +100 (field A)
    gp<text::UiElem> parent;   // +104
    be_u32 arg;                // +108
    be_u32 align;              // +112 (field B)
    be_u32 anchor;             // +116 (field C)
    u8 flags[7];               // +120
    u8 _127;
};
static_assert(sizeof(ElemDesc) == 128 && offsetof(ElemDesc, color) == 96 && offsetof(ElemDesc, flags) == 120);

struct TextDesc {
    ElemDesc base;                 // +0
    gp<text::FontSet> font_set;    // +128
    gp<const char> text;           // +132
    be_f32 line_spacing;           // +136
    be_f32 char_spacing;           // +140
    be_u32 wrap;                   // +144
    be_u32 f148;                   // +148
    u8 monospace;                  // +152
    pad<3> _153;
    be_u32 glyph_mode;             // +156
    be_f32 reveal_rate;            // +160
    u8 b164;                       // +164
    pad<3> _165;
};
static_assert(sizeof(TextDesc) == 168 && offsetof(TextDesc, glyph_mode) == 156);

struct Text5Desc {
    TextDesc text;   // +0
    be_f32 f168;     // +168
    be_f32 width;    // +172
    be_f32 f176;     // +176
    be_f32 start;    // +180
    u8 flag;         // +184
    pad<3> _185;
};
static_assert(sizeof(Text5Desc) == 188);

// Type-8 description (UiElem8_FromXml -> UiElem8_Construct).
struct Elem8Desc {
    ElemDesc base;          // +0
    gp<const char> name;    // +128 objName
    be_u32 obj;             // +132 an object to use
    be_u32 cls;             // +136 a class to instance
    u8 use_obj;             // +140
    pad<3> _141;
};
static_assert(offsetof(Elem8Desc, use_obj) == 140);

// A loaded resource (textures, fonts, font sets; linked from kResList).
using font::UiRes;

// Native functions (src/uiload).
const char* XmlCursor_DocPath(XmlCursor* c);
u32 Tex_GetDefault(s32 n);
void UiTextDesc_FromXml(XmlCursor* doc, XmlCursor* node, const text::TextElem* tmpl, TextDesc* d);
text::TextElem* UiText_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const text::TextElem* tmpl);
void UiText5Desc_FromXml(XmlCursor* doc, XmlCursor* node, const text::UiElem* tmpl, Text5Desc* d);
text::UiElem* UiText5_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const text::UiElem* tmpl);

}  // namespace uiload
}  // namespace sly4
