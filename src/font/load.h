// Building fonts and font sets from a UI file's XML.
#pragma once

#include "res.h"

namespace sly4 {
namespace font {

namespace lfn {
constexpr u32 XmlCursor_DocPath = 0x004F8944;   // (cursor) -> char* or 0; the loaders call it on errors
constexpr u32 XmlCursor_FirstChild = 0x004F92BC;
constexpr u32 XmlCursor_NextSibling = 0x004F8FC8;
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;
constexpr u32 XmlCursor_ReadBool = 0x004F986C;
constexpr u32 XmlCursor_ReadF32 = 0x004F9B20;
constexpr u32 XmlCursor_ReadVec2 = 0x004F9C9C;
constexpr u32 Mem_Alloc = 0x0054ED38;
constexpr u32 memset = 0x00726E54;
}  // namespace lfn

constexpr u32 kStrName = 0x00AEF4C0;      // "name"
constexpr u32 kEmptyPath = 0x00B2266C;    // ""
constexpr u32 kHashFontTag = 0x00255A8F;  // child tag of a font set ("font"?)
constexpr u32 kHashScale = 0x04B1112A;    // vec2 on the set, f32 on each font ("scale"?)
constexpr u32 kHashKerning = 0x7A600EB7;  // f32 per font (the text's char spacing attribute too)
constexpr u32 kHashPreload = 0x17CFF9E9;  // bool: load the set right away (name?)
constexpr u32 kFontSetSize = 296;
constexpr u32 kFontSetLoadConsts = 0x004D71C8;  // f32 { 1.0, 0.0 }

}  // namespace font
}  // namespace sly4

namespace sly4 {
namespace font {

namespace lfn2 {
constexpr u32 XmlCursor_Text = 0x004FA744;      // (cursor) -> const char* (the element's text?) (name?)
constexpr u32 Mem_GetDefaultStack = 0x005525FC;
constexpr u32 MemStack_Alloc = 0x0055260C;      // (stack, size)
constexpr u32 sscanf = 0x0071EC78;
constexpr u32 memmove = 0x0071C3F0;
}  // namespace lfn2

constexpr u32 kStrTexture = 0x00AEF4E4, kStrMapFile = 0x00AEF4EC, kStrFirstChar = 0x00AEF4FC;
constexpr u32 kStrHexFormat = 0x00AEF50C;   // "%x"
constexpr u32 kHashFontFlag80 = 0x04704F4C; // bool -> Font +101 0x80 (name?)
constexpr u32 kHashFontFlag40 = 0x051B7FEC; // bool -> 0x40 (name?)
constexpr u32 kHashFontFlag20 = 0x2E8401A3; // bool -> 0x20 (name?)
constexpr u32 kHashFontMapTag = 0x0001293C; // child element whose text lists the characters
constexpr u32 kFontSize = 124;
constexpr u32 kFontMapSize = 0x10000;
constexpr u32 kFontMapMaxGlyphs = 255;

}  // namespace font
}  // namespace sly4
