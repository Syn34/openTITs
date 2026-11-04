// Shared names for src/world/hevent11_*.cpp: callees named by the
// hevent10_*.cpp functions and more of the World event handler's remaining
// callees. Guest addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include "hevent10.h"

namespace sly4 {
namespace hevent11 {

namespace h11fn {
constexpr u32 SortedMap_Find3 = 0x0076611C;   // (map, const u32* key) -> value slot (as hevent9_b.cpp)
constexpr u32 Registry_Find = 0x00766C44;     // (map 0x010686A8, const u32* key) -> value slot (symbols.txt)
}  // namespace h11fn

constexpr u32 kRegistry = 0x010686A8;  // map searched with Registry_Find

// UserBlock +2232: per-slot {Holder*, u32} pairs; Holder +0 vector {+0
// Entry*[], +8 u16 count}. Entry +20 u32 hash / handle, +28 u32 flags.
constexpr u32 kUbSlots = 2232, kUbSlotSize = 8;
constexpr u32 kSlotEntryHash = 20, kSlotEntryFlags = 28;
constexpr u32 kSlotEntryStarted = 0x08000000;  // +28 bit 27

}  // namespace hevent11
}  // namespace sly4
