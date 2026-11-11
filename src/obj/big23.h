// Game code, large-function sweep part 23: helpers shared by big23_*.cpp.
#pragma once

#include <bit>
#include <cmath>

#include "big22.h"

namespace sly4 {
namespace big23 {

using namespace opentit;
using namespace opentit::decomp;
using big19::VCall;
using mid15::Cw;
using mid15::Fb;
using mid15::Fmadds;
using mid15::Fmsubs;
using mid15::Lf;
using mid15::S;
using mid15::Sf;
using mid16::Divs;
using mid16::Sqrts;

namespace b23fn {
constexpr u32 Acosf = 0x005E649Cu;          // acosf: f1 -> f1
constexpr u32 Curve_Lerp3 = 0x004A8598u;    // (Vec3* out, const Vec3* a, const Vec3* b; f32 t, f32 u)
constexpr u32 Spring_Step3 = 0x000F8C70u;   // (Spring3* s; f32 k, f32 damping, f32 dt) (reads r3 f1 f2 f3)
}  // namespace b23fn

// fsel d, a, c, b: a >= 0 (not NaN) ? c : b.
inline f64 Fsel(f64 a, f64 c, f64 b) { return a >= 0.0 ? c : b; }
// fadds / fsubs / fmuls (one rounding to single).
inline f64 Adds(f64 a, f64 b) { return S(a + b); }
inline f64 Subs(f64 a, f64 b) { return S(a - b); }
inline f64 Muls(f64 a, f64 b) { return S(a * b); }
// The double an lfd at ea leaves.
inline f64 Lfd(u32 ea) { return std::bit_cast<f64>(load<u64>(ea)); }

}  // namespace big23
}  // namespace sly4
