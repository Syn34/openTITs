// World objects as the UI's type-8 element uses them, and the sorted
// key -> value maps the game looks them up in.
#pragma once

#include "../sly4.h"

namespace sly4 {
namespace obj {

using namespace opentit;

namespace ofn {
constexpr u32 qsort = 0x0071EC28;              // (base, count, size, compare opd)
constexpr u32 PtrVector_Reserve = 0x00551718;  // (vector, capacity)
constexpr u32 String_Copy = 0x00553684;        // (dst, src, size): always terminates
constexpr u32 String_HashNoCase = 0x0056A710;
constexpr u32 WorldObj_SetScale = 0x0057D100;
constexpr u32 WorldObj_SetGroup = 0x00583E54;
constexpr u32 WorldObj_MarkDirty = 0x0057D000;     // (obj) marks it and its parent dirty (0x800)
constexpr u32 UiElem8_Attach = 0x004D7CF8;
// The four class maps' lookups (same template as Map_Find, still recompiled).
constexpr u32 MapA_Find = 0x007A6D74, MapB_Find = 0x007A6BEC, MapC_Find = 0x00793884, MapD_Find = 0x00793A0C;
}  // namespace ofn

// ---------------------------------------------------------------- sorted map
// Entries are {u32 key, u32 value} pairs kept sorted by key; a lookup sorts
// them first when they were changed. A miss returns the default slot.
constexpr u32 kMapCount = 2;       // u16
constexpr u32 kMapFlags = 6;       // u16: bit 0 = needs sorting
constexpr u32 kMapEntries = 8;     // {u32 key, u32 value}[]
constexpr u32 kMapDefault = 16;    // u32: the value slot returned on a miss
constexpr u32 kMapCompare = 20;    // OPD*: (const entry*, const u32* key) -> int
constexpr u32 kMapEntrySize = 8;
constexpr u32 kMapCompareU32A = 0x00F3BB80;  // the default comparators (unsigned keys) of the two instances
constexpr u32 kMapCompareU32B = 0x00F2FB88;

// ---------------------------------------------------------------- world object
constexpr u32 kObjMatrix = 16;      // f32[3][4] rows; +64 / +68 / +72 the translation row
constexpr u32 kObjScale = 144;      // f32[3]
constexpr u32 kObjFlags = 164;      // u32: 0x2000 non-unit scale, 0x800 dirty, 0x400 queued
constexpr u32 kObjName = 170;       // char[32]
constexpr u32 kObjNameHash = 204;   // u32
constexpr u32 kObjParent = 256;     // obj*
constexpr u32 kObjQueued = 260;     // u32 (nonzero: don't queue)
constexpr u32 kObjNextSibling = 268, kObjFirstChild = 272;
constexpr u32 kObjGroupWord = 320;  // u32: bits 18-23 group (name?)
constexpr u32 kObjVfnChanged = 36;  // ()
constexpr u32 kObjVfnSet40 = 40;    // (0, 0) (name?)

constexpr u32 kObjNonUnitScale = 0x2000, kObjDirty = 0x800, kObjQueuedFlag = 0x400;
constexpr u32 kScaleEpsilon = 0x0057D0FC;  // f32 1e-5
constexpr u32 kOneVec3 = 0x01164B30;
constexpr u32 kDirtyList = 0x0116BD00;     // PtrVector*: {data, u16 cap at +4, u16 count at +8, u16 high-water at +10}
constexpr u32 kDirtyListOff = 0x0116BD04;  // u8: nonzero = don't queue

// Class maps searched by WorldObj_FindByHash: holder objects with the map at +16.
constexpr u32 kMapHolderA = 0x00F8F150, kMapHolderB = 0x00F8F144, kMapHolderC = 0x00F8F148, kMapHolderD = 0x00F8F14C;

}  // namespace obj
}  // namespace sly4
