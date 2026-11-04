// Scalar matrix helpers and the sorted map the UI keeps resources in.
#pragma once

#include "../sly4.h"

namespace sly4 {
namespace view {

namespace mfn {
constexpr u32 qsort = 0x0071EC28;   // (base, count, size, cmp OPD)
}  // namespace mfn

// SortedMap (HashMap_Find): +2 u16 count, +6 u16 flags (bit 0: needs sorting),
// +8 entries { u32 key; u32 value; }, +16 the value slot returned on a miss,
// +20 compare OPD (kMapCompareU32 = plain unsigned keys).
constexpr u32 kMapCount = 2, kMapFlags = 6, kMapEntries = 8, kMapMiss = 16, kMapCompare = 20;
constexpr u32 kMapCompareU32 = 0x00F3AD88;
constexpr u16 kMapUnsorted = 1;
constexpr u32 kMapEntrySize = 8;

inline f64 sgl(f64 v) { return static_cast<f64>(static_cast<f32>(v)); }
inline f64 fmuls(f64 a, f64 b) { return sgl(a * b); }
inline f64 fadds(f64 a, f64 b) { return sgl(a + b); }
inline f64 fsubs(f64 a, f64 b) { return sgl(a - b); }
inline f64 fdivs(f64 a, f64 b) { return sgl(a / b); }
inline f64 fmadds(f64 a, f64 c, f64 b) { return sgl(std::fma(a, c, b)); }
inline f64 fmsubs(f64 a, f64 c, f64 b) { return sgl(std::fma(a, c, -b)); }
inline f64 fnmsubs(f64 a, f64 c, f64 b) { return sgl(-std::fma(a, c, -b)); }

}  // namespace view
}  // namespace sly4
