// Game code, large-function sweep part 11: helpers shared by big11_*.cpp
// (the camera rig's mode handlers part 10 named, and further still-recompiled
// game functions of 400+ instructions).
#pragma once

#include <bit>
#include <cmath>

#include "big10.h"

namespace sly4 {
namespace big11 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using obj19::VCall;
using obj23::LfsAt;
using obj25::Sx;
using big4::CopyW;
using big7::F;
using big7::Fmadds;
using big7::Fmsubs;
using big7::Fnmsubs;
using big7::SetF1Bits;
using big7::CopyMat43Last;
using big10::StfsW;
using big10::Stfs;

// fadds / fsubs / fmuls / fdivs: the double operation rounded to single.
inline f32 Adds(f64 a, f64 b) { return static_cast<f32>(a + b); }
inline f32 Subs(f64 a, f64 b) { return static_cast<f32>(a - b); }
inline f32 Muls(f64 a, f64 b) { return static_cast<f32>(a * b); }
inline f32 Divs(f64 a, f64 b) { return static_cast<f32>(a / b); }

}  // namespace big11
}  // namespace sly4

// ---------------------------------------------------------------- native
#include "native/objtypes.h"
