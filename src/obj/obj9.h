// World objects, part 9: the spline definition and its point array
// (construction, evaluation, nearest-point search), the peer matrix query,
// the component animation queries, the skeleton update and the child LOD
// update.
#pragma once

#include <bit>
#include <cmath>

#include "obj8.h"

namespace sly4 {
namespace obj9 {

using namespace opentit;

namespace o9fn {
constexpr u32 SplineArray_Reset = 0x0055F244;  // (SplineArray*): all four words 0 (name?)
constexpr u32 SplineSeg_Nearest = 0x0055ED60;  // (const f32 ctrl[4][4], const f32 p[3], f32* dist, s32 iterations) -> f1 t (name?)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;       // (block, 0)
constexpr u32 Mem_AllocPool = 0x0056C514;      // (align, size, pool, tag)
constexpr u32 Mem_FreePool = 0x0056C730;       // (p, pool, tag)
}  // namespace o9fn

// ---------------------------------------------------------------- SplineArray (ApeSplineDef +112)
// +0 u32: bit 31 closed (one extra point, Spline_Eval wraps t into [0, 1)),
// bit 30 a second flag (name?), bits 15-29 the point count, bits 0-14 the
// allocated count; +4 the 64-byte points; +12 u32 the last segment found.
constexpr u32 kSaBits = 0, kSaPoints = 4, kSa8 = 8, kSaLastSeg = 12;
constexpr u32 kSaClosed = 0x80000000u, kSaFlag30 = 0x40000000u;
constexpr u32 kSaCountMask = 0x3FFF8000u, kSaCountShift = 15, kSaAllocMask = 0x7FFFu;
// A point: +0 the position, +16 the incoming handle, +32 the outgoing
// handle (both relative to the position), +48 f32 the curve parameter.
constexpr u32 kSpPos = 0, kSpIn = 16, kSpOut = 32, kSpParam = 48, kSpSize = 64;

// ---------------------------------------------------------------- single-precision arithmetic
// The s-forms: computed in double, rounded once to single.
inline f32 Fadds(f64 a, f64 b) { return static_cast<f32>(a + b); }
inline f32 Fsubs(f64 a, f64 b) { return static_cast<f32>(a - b); }
inline f32 Fmuls(f64 a, f64 b) { return static_cast<f32>(a * b); }
inline f32 Fdivs(f64 a, f64 b) { return static_cast<f32>(a / b); }
inline f32 Fmadds(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }  // a * c + b
inline f32 Lf(u32 ea) { return load<f32>(ea); }

}  // namespace obj9
}  // namespace sly4
