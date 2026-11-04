// Shared names for src/world/hevent10_*.cpp: callees named by the
// hevent9_*.cpp functions and more of the World event handler's remaining
// callees. Guest addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include "hevent9.h"

namespace sly4 {
namespace hevent10 {

namespace h10fn {
constexpr u32 WorldObj_SetScale = 0x0057D100;  // (WorldObj*, const vec3*, u8) (symbols.txt)
}  // namespace h10fn

// Object +340: the attach vector (as Obj_Attach0D15C0): +0 240-byte records,
// +8 u16 count. Record +192: the attached Object*.
constexpr u32 kObjAttach = 340, kAttachData = 0, kAttachCount = 8;
constexpr u32 kAttachRecSize = 240, kAttachRecObject = 192;
constexpr u32 kObjWorldObj = 160;  // Object +160: WorldObj* or 0

}  // namespace hevent10
}  // namespace sly4
