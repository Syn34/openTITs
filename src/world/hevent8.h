// Shared names for src/world/hevent8_*.cpp: more callees of the World event
// handler's helpers (hevent7_*.cpp) and of the handler itself. Guest
// addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include "hevent7.h"

namespace sly4 {
namespace hevent8 {

namespace h8fn {
constexpr u32 SubMesh_ConstructBase2 = 0x004FCE88;  // (SubMesh*) (name?)
constexpr u32 Treasure_Release = 0x00425BA8;        // (Treasure*, u8 lost)
constexpr u32 Memset = 0x00726E54;                  // libc memset
constexpr u32 Memmove = 0x0071C3F0;                 // libc memmove
constexpr u32 PtrVector_Reserve = 0x00551718;       // (vector, new capacity)
}  // namespace h8fn

// A vector of 76-byte event argument records (74 bytes copied):
//   +0 u32 data, +4 u16 capacity, +8 u16 count, +10 u16 high-water mark.
// Record: +0 u32 value, +68 u32 name hash, +72 u8 type, +73 u8 (0).
constexpr u32 kArgSize = 76, kArgCopy = 74;
constexpr u32 kArgValue = 0, kArgHash = 68, kArgType = 72, kArgType2 = 73;
constexpr u8 kArgObject = 2, kArgInt = 13;

// fneg of an lfs'd single, as the raw word (sign flip; NaN payloads kept).
inline u32 fneg_bits(u32 w) { return w ^ 0x80000000u; }

}  // namespace hevent8
}  // namespace sly4
