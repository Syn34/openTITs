// Game code, medium sweep part 17: helpers shared by mid17_*.cpp (still
// recompiled game functions of 120-400 instructions).
#pragma once

#include <bit>
#include <cmath>
#include <initializer_list>
#include <utility>

#include "mid16.h"
#include "obj19.h"

namespace sly4 {
namespace mid17 {

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
using mid16::Divs;
using mid16::MOff16;
using mid16::Sqrts;
using mid6::F32;
using mid6::W32;
using obj19::VCall;

namespace m17fn {
constexpr u32 Obj_FindById = 0x000D6A28;            // (u32 id) -> Obj* or 0
constexpr u32 Cam_RotateToward1007D0 = 0x001007D0;  // (Vec3* out, const Vec3* axis, const Vec3* v; f1 angle)
constexpr u32 Obj_SetRange_00101958 = 0x00101958;   // (Vec3* out, const Vec3* v; f1, f2)
constexpr u32 Vec_TurnToward106770 = 0x00106770;    // (Vec3* out, const Vec3* to, const Vec3* from; f1 rate)
}  // namespace m17fn

// Copies the 3x3 + translation of the Mat34 at s into d (the x/y/z words of
// rows 0..3; the w words are left alone), as raw words.
inline void CopyMat34(u32 d, u32 s) {
    for (u32 k = 0; k < 12; ++k) Cw(d + MOff16(k), s + MOff16(k));
}

// Copies a Vec3 (raw words).
inline void CopyVec3(u32 d, u32 s) {
    for (u32 k = 0; k < 3; ++k) Cw(d + 4 * k, s + 4 * k);
}

}  // namespace mid17
}  // namespace sly4
