// The level list: Scripts/Sly4Levels.xml, kept in a binary tree map with a
// built-in iterator. Guest addresses in the BCES01284 v01.02 EBOOT.
#pragma once

#include "../sly4.h"

namespace sly4 {
namespace levels {

// Map at 0x010C2CAC: +12 -> root pointer, +16 u32 path[] (iterator stack),
// +96 u16 depth, +100 current node. Node: +8 Level*, +12 left, +16 right.
constexpr u32 kLevelMap = 0x010C2CAC;
constexpr u32 kMapRoot = 12, kMapPath = 16, kMapDepth = 96, kMapCurrent = 100;
constexpr u32 kNodeValue = 8, kNodeLeft = 12, kNodeRight = 16;

// Level: +128 char path[] ("Dev/..." levels are skipped), +556 u8 loaded.
constexpr u32 kLevelPath = 128, kLevelLoaded = 556;

namespace lvfn {
constexpr u32 Levels_Load = 0x004EBE50;     // (const char* xml) -> u32
constexpr u32 Levels_Count = 0x004ECAC4;    // () -> u32
constexpr u32 Levels_At = 0x004ECC50;       // (s32 i) -> Level* or 0
constexpr u32 Level_Register = 0x004EBCF4;  // (Level*) -> u8 loaded
constexpr u32 Levels_Parse = 0x004EB904;    // (XmlElement*) -> u32 (name?)
constexpr u32 Xml_FindChild = 0x004F8C30;   // (node, const char* name) -> node (name?)
constexpr u32 Xml_FindChildren = 0x004F936C;  // (node, const char* name) -> element (name?)
constexpr u32 String_Append = 0x005566A8;   // strcat (name?)
}  // namespace lvfn

}  // namespace levels
}  // namespace sly4
