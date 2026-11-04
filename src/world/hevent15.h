// Shared names for src/world/hevent15_*.cpp: callees named by the
// hevent14_*.cpp functions and more of the World event handler's remaining
// callees. Guest addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include "hevent14.h"

namespace sly4 {
namespace hevent15 {

namespace h15fn {
constexpr u32 Obj_Dispatch362 = 0x0036325C;  // (Object*, u8 a, u8 alt, u8 b) (hevent_c.cpp)
}  // namespace h15fn

// The list of all objects (as hevent8_e.cpp kQueryList): +0 Object*[], +8 u16
// count.
constexpr u32 kAllObjects = 0x01013B34, kAllData = 0, kAllCount = 8;

}  // namespace hevent15
}  // namespace sly4
