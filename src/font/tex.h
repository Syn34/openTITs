// The texture resource (vtable 0x00B22618, 112 bytes) and the included
// UI file resource (vtable 0x00B22678).
#pragma once

#include "load.h"

namespace sly4 {
namespace font {

constexpr u32 kTexFlags = 92;    // u32: 0x80000000 / 0x40000000 from two bools (name?)
constexpr u32 kTexHandle = 96;   // Ref*: the loaded texture
constexpr u32 kTexPages = 100;   // Ref*[3]
constexpr u32 kTexSize = 112;
constexpr u32 kStrFileName = 0x00AEF4C8;  // "filename"

namespace tfn2 {
constexpr u32 Tex_FindLoaded = 0x0050C398;   // (u32 file hash) -> Texture* or 0 (name?)
constexpr u32 Tex_LoadFile = 0x0051E68C;     // (const char* path, 0) -> Texture* (name?)
constexpr u32 Texture_MakePages = 0x004D5C64;// (UiTexture*) (name?)
constexpr u32 XmlDoc_Construct = 0x004FA888;
constexpr u32 XmlDoc_Parse = 0x004FAF10;
constexpr u32 XmlDoc_Destruct = 0x004FA9C0;
constexpr u32 Ui_LoadFile = 0x004DFA60;
constexpr u32 Bank_Destroy = 0x00596E3C;     // (name?)
}  // namespace tfn2

constexpr u32 kIncBank = 92;
constexpr u32 kTagUi = 0x0001151B;        // the file's root element
constexpr u32 kXmlDocSize = 2240;
constexpr u32 kUiTimelineList = 0x00F6689C;   // Timeline*, linked by +52, owner at +40 (name?)
constexpr u32 kUiRootElements = 0x00F66868;   // Element*, linked by +156, owner (+160) checked

}  // namespace font
}  // namespace sly4
