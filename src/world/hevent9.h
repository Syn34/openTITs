// Shared names for src/world/hevent9_*.cpp: the callees named by the
// hevent8_*.cpp helpers and more of the World event handler's remaining
// callees. Guest addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include "hevent8.h"

namespace sly4 {
namespace hevent9 {

namespace h9fn {
constexpr u32 JobQueue_Lock = 0x0056DFD4;       // () (as hevent2_a.cpp)
constexpr u32 JobQueue_Get = 0x0056E038;        // () -> JobQueue*
constexpr u32 JobQueue_Unlock = 0x0056E008;     // ()
constexpr u32 Rec80_Find = 0x005848CC;          // (list, key) -> 80-byte record or 0
constexpr u32 Skeleton_FindBone = 0x005A93D8;   // (skeleton, u32 hash) -> s32 index or -1
constexpr u32 WorldObj_ForEach = 0x0057F5D8;    // (obj, fn OPD, arg, bool recurse)
constexpr u32 PtrVector_Construct = 0x005516E0; // (vec)
constexpr u32 Attach_Add7450E4 = 0x007450E4;    // (vector of 240-byte attach records, AttachDesc*) (name?)
}  // namespace h9fn

// The sorted pair table (as hevent8_b.cpp): +2 u16 count, +6 u16 flags (bit
// 0: unsorted), +8 {key, value}[] entries, +16 "not found" value, +20
// compare OPD.
constexpr u32 kTabCount = 2, kTabFlags = 6, kTabEntries = 8, kTabMiss = 16, kTabCompare = 20;
constexpr u32 kTabUnsorted = 1;

}  // namespace hevent9
}  // namespace sly4
