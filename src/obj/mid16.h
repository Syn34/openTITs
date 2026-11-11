// Game code, medium sweep part 16: helpers shared by mid16_*.cpp (still
// recompiled game functions of 120-400 instructions).
#pragma once

#include <bit>
#include <cmath>
#include <initializer_list>
#include <utility>

#include "mid15.h"

namespace sly4 {
namespace mid16 {

using namespace opentit;
using namespace opentit::decomp;
using mid14::ClampS;
using mid14::Cw;
using mid14::Fmadds;
using mid14::Fmsubs;
using mid14::Fnmadds;
using mid14::Fnmsubs;
using mid14::Lf;
using mid14::S;
using mid13::LfsF;
using mid15::Fb;
using mid15::Ld;
using mid15::Sf;
using mid6::F32;
using mid6::W32;

namespace m16fn {
constexpr u32 Matrix_NormalizeInPlace = 0x0056D374;  // (Mat34*)
constexpr u32 Quat_AngularVel0F8E30 = 0x000F8E30;    // (f32 out[3], -, q0, q1; f1 dt)
constexpr u32 Obj_GetChannel_0045B160 = 0x0045B160;  // (const T* o, s32 i) -> f1
}  // namespace m16fn

// The 3x3 + translation word k (0..11) of a Mat34: row k / 3, column k % 3.
inline u32 MOff16(u32 k) { return 16 * (k / 3) + 4 * (k % 3); }

// fdivs / fsqrts / fabs on singles.
inline f64 Divs(f64 a, f64 b) { return S(a / b); }
inline f64 Sqrts(f64 x) { return S(std::sqrt(x)); }

}  // namespace mid16
}  // namespace sly4
