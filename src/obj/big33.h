// Game code, large-function sweep part 33: helpers shared by big33_*.cpp.
#pragma once

#include <bit>
#include <cmath>

#include "big32.h"

namespace sly4 {
namespace big33 {

using namespace opentit;
using namespace opentit::decomp;
using big31::Lfs;
using big32::W32f;
using big32::Copy3W;
using big32::Store3W;
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

// Addresses of callees used by several big33 files.
namespace b33fn {
constexpr u32 Nop = 0x0054EC18;                  // empty function (r3 left as passed)
constexpr u32 PtrVector_Clear = 0x0054EC1C;      // (vec)
constexpr u32 PtrVector_Construct = 0x005516E0;  // (vec)
constexpr u32 PtrVector_Reserve = 0x00551718;    // (vec, u32 n)
constexpr u32 String_FormatN = 0x00555CF4;       // (buf, size, fmt, ...)
constexpr u32 String_CopyN = 0x00553638;         // (dst, src, n)
}  // namespace b33fn

}  // namespace big33
}  // namespace sly4
