// Game code, medium sweep part 22: helpers shared by mid22_*.cpp (still
// recompiled game functions of 120-400 instructions, plus the callees part 21
// identified).
#pragma once

#include <bit>
#include <cmath>
#include <initializer_list>
#include <utility>

#include "mid21.h"

namespace sly4 {
namespace mid22 {

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
using mid16::Sqrts;
using mid17::CopyVec3;
using mid6::F32;
using mid6::W32;
using obj19::VCall;

}  // namespace mid22
}  // namespace sly4
