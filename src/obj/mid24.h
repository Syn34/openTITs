// Game code, medium sweep part 24: helpers shared by mid24_*.cpp (more
// still-recompiled game functions of 120-400 instructions: the remaining
// QueryEx / Obj_Init22 spawners, then other candidates).
#pragma once

#include <bit>
#include <cmath>
#include <initializer_list>
#include <utility>

#include "mid23.h"

namespace sly4 {
namespace mid24 {

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

}  // namespace mid24
}  // namespace sly4
