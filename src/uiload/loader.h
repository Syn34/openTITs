// The UI file loader: resources, timelines and the element tree.
#pragma once

#include "uiload.h"

namespace sly4 {
namespace uiload {

namespace lofn {
constexpr u32 Ui_LoadElement = 0x004DE0F8;
constexpr u32 Ui_Find = 0x004D5544;
constexpr u32 XmlCursor_FirstChild = 0x004F92BC;     // (cursor, tag hash)
constexpr u32 XmlCursor_NextSibling = 0x004F8FC8;    // (cursor, tag hash)
constexpr u32 XmlCursor_FirstChildAny = 0x004F9140;  // (cursor) (name?)
constexpr u32 XmlCursor_NextSiblingAny = 0x004F8D74; // (cursor) (name?)
constexpr u32 XmlCursor_ReadVec3 = 0x004F9E0C;
constexpr u32 XmlCursor_ReadColor = 0x004F9F9C;
constexpr u32 String_CompareN = 0x00555EB0;          // (a, b, n) -> 0 when equal (name?)
constexpr u32 Matrix_FromAxisAngle = 0x0056A410;     // (f32 out[16], const f32 axis[3]; f1 = angle)
// The element builders: (doc, node, const ElemDesc* base, Element* tmpl) -> Element*.
constexpr u32 UiElem9_FromXml = 0x004D7910;   // tag 0xAA8CFA44, type 9
constexpr u32 UiElem8_FromXml = 0x004D8200;   // tag 0x30CF0025, type 8
constexpr u32 UiElem7_FromXml = 0x004D8460;   // tag 0x040EFE5F, type 7
constexpr u32 UiElem6_FromXml = 0x004D9DA0;   // tag 0x00274AAE, type 6
constexpr u32 UiText5_FromXml = 0x004DCC04;   // tag 0x937B5F0A, type 5
constexpr u32 UiText_FromXml = 0x004DCE7C;    // tag 0x044D86D4, type 3
constexpr u32 UiElem2_FromXml = 0x004DD1F0;   // tag 0x33FDE730, type 2
constexpr u32 UiElem1_FromXml = 0x004DD800;   // tag 0x0428B13B, type 1
constexpr u32 UiElem0_FromXml = 0x004DDD2C;   // tag 0x0400B14D, type 0
// The file loader's sections.
constexpr u32 Texture_FromXml = 0x004D5A4C;   // (doc, node, include)
constexpr u32 Font_FromXml = 0x004D601C;
constexpr u32 FontSet_FromXml = 0x004D71D0;
constexpr u32 Timeline_FromXml = 0x004DF88C;  // (doc, node, include) (name?)
constexpr u32 Bank_Create = 0x005903F4;       // (0) -> object, other subsystem (name?)
constexpr u32 Bank_Open = 0x005904EC;         // (object, const char* name, 0) (name?)
constexpr u32 Bank_Start = 0x005964B4;        // (object) (name?)
}  // namespace lofn

constexpr u32 kTagInstance = 0x03D3E53D;      // copies a "from=" element and builds one of its type
constexpr u32 kUiElementHook = 0x00F66894;    // OPD*: builder for unknown tags (doc, node, desc, tmpl)
constexpr u32 kUiInstanceHook = 0x00F66898;   // OPD*: builder for instances of unknown types
constexpr u32 kStrFrom = 0x00AEF81C, kStrBlend = 0x00AEF840, kStrAnchor = 0x00AEF858, kStrAlign = 0x00AEF850;
constexpr u32 kBlendNames = 0x00AEF828;       // "av", "ad", "su" (8 bytes apart)
constexpr u32 kHashCloneChildren = 0x242AA215;
constexpr u32 kHashScale = 0x04B1112A, kHashColor = 0x03D53743, kHashPos = 0x5B388929, kHashRot = 0x2C3EFF7E;
constexpr u32 kHashFlags[7] = {0x469D7C52, 0xFFD1EC91, 0x7ABBD29A, 0x2A3FC52F, 0xDF472DF2, 0x29466D43, 0x11272E87};
constexpr u32 kDefaultColor = 0x01164CA0;     // u8[4]
constexpr u32 kIdentity = 0x01164BD0;         // f32[16]
constexpr u32 kOneVec3 = 0x01164B30;          // f32[3] (the default scale)
constexpr u32 kRotAxis = 0x01164B70;          // f32[3]
constexpr u32 kLoaderConsts = 0x004DE0EC;     // f32 { 0.0, pi / 180 }

// Ui_LoadFile section tags.
constexpr u32 kTagResources = 0x73A36783, kTagTexture = 0xD64960FB, kTagFont = 0x447D9F5E, kTagFontSet = 0x0021044F;
constexpr u32 kTagTimelines = 0x9D9FD4E8, kTagTimeline = 0x9178C4CB, kTagElements = 0xA7D40E5E;
constexpr u32 kIncludeBank = 92;              // Include +92: Bank_Create's object

// Native functions (src/uiload/ctor_elem.cpp, loader.cpp).
text::UiElem* UiElem9_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const text::UiElem* tmpl);
text::UiElem* UiElem8_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const text::UiElem* tmpl);
text::UiElem* UiElem7_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const text::UiElem* tmpl);
text::UiElem* UiElem6_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const text::UiElem* tmpl);
text::UiElem* UiElem2_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const text::UiElem* tmpl);
text::UiElem* UiElem1_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const text::UiElem* tmpl);
text::UiElem* UiElem0_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const text::UiElem* tmpl);
text::UiElem* Ui_LoadElement(XmlCursor* doc, XmlCursor* node, text::UiElem* parent, u32 arg);
u32 Ui_LoadFile(void* inc, XmlCursor* doc, XmlCursor* root);

}  // namespace uiload
}  // namespace sly4
