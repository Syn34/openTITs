// Game code, large-function sweep part 30: helpers shared by big30_*.cpp.
#pragma once

#include <bit>
#include <cmath>

#include "big29.h"

namespace sly4 {
namespace big30 {

using namespace opentit;
using namespace opentit::decomp;
using big29::SetFk;
using big29::StackArg;
using big19::VCall;

namespace b30fn {
constexpr u32 Obj_FindComp34CC34 = 0x0034CC34;  // (owner, key) -> component or 0
constexpr u32 Entry_Stop353CC0 = 0x00353CC0;    // (Entry*, u8 release)
constexpr u32 Obj_StopVoiceTree = 0x000DC314;   // (Obj*, key, u8 release)
}  // namespace b30fn

}  // namespace big30
}  // namespace sly4
