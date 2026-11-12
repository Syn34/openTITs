// Game code, large-function sweep part 35: helpers shared by big35_*.cpp.
#pragma once

#include <bit>
#include <cmath>

#include "big34.h"

namespace sly4 {
namespace big35 {

using namespace opentit;
using namespace opentit::decomp;
using big31::Lfs;
using big32::W32f;
using big29::Adds;
using big29::Divs;
using big29::Fmadds;
using big29::Fmsubs;
using big29::Muls;
using big29::S;
using big29::Sqrts;
using big29::Subs;
using big29::StackArg;
using big19::VCall;
using big34::NodeParamAddr;

// Addresses of callees used by several big35 files.
namespace b35fn {
constexpr u32 PtrVector_Clear = 0x0054EC1C;  // (vec)
constexpr u32 Nop = 0x0054EC18;              // (x): returns
}  // namespace b35fn

}  // namespace big35
}  // namespace sly4
