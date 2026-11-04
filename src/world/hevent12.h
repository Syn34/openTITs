// Shared names for src/world/hevent12_*.cpp: callees named by the
// hevent11_*.cpp functions and more of the World event handler's remaining
// callees. Guest addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include "hevent11.h"

namespace sly4 {
namespace hevent12 {

namespace h12fn {
constexpr u32 Obj_FindComponent = 0x000D1288;  // (Object*, u32 type, 0) -> component or 0 (symbols.txt)
}  // namespace h12fn

// World +0x12820C: Entry*[] (u32 slots), +0x128214 u16 count; Entry +40 hash.
constexpr u32 kWorldEntries = 0x12820C, kWorldEntryCount = 0x128214, kWorldEntryHash = 40;

}  // namespace hevent12
}  // namespace sly4
