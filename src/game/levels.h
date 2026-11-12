// The level list: Scripts/Sly4Levels.xml, kept in a binary tree map with a
// built-in iterator. Guest addresses in the BCES01284 v01.02 EBOOT.
#pragma once

#include <cstddef>

#include "opentit/native.h"
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

// ---------------------------------------------------------------- native structures
using opentit::native::gp;
using opentit::native::pad;

struct Level {
    pad<128> _0;
    char path[428];   // +128 ("Dev/..." levels are skipped)
    u8 loaded;        // +556
};
static_assert(offsetof(Level, loaded) == kLevelLoaded);

struct LevelNode {
    pad<8> _0;
    gp<Level> value;      // +8
    gp<LevelNode> left;   // +12
    gp<LevelNode> right;  // +16
};
static_assert(offsetof(LevelNode, right) == kNodeRight);

struct LevelMap {
    pad<12> _0;
    gp<gp<LevelNode>> root;  // +12: points at the root pointer
    be_u32 path[20];         // +16: iterator stack (LevelNode*)
    be_u16 depth;            // +96
    pad<2> _98;
    gp<LevelNode> current;   // +100
};
static_assert(offsetof(LevelMap, depth) == kMapDepth && offsetof(LevelMap, current) == kMapCurrent);

u32 Levels_Count();
Level* Levels_At(s32 i);
u32 Levels_Load(const char* xml);
u8 Level_Register(Level* level);

}  // namespace levels
}  // namespace sly4
