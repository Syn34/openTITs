// World objects, part 14: the callees part 13 left recompiled (the
// Bezier tangent, the draw record list insert, ...) and the next
// still-recompiled functions of 0x00550000-0x005AA000.
#pragma once

#include <bit>
#include <cmath>

#include "obj13.h"

namespace sly4 {
namespace obj14 {

using namespace opentit;

// PtrVector: +0 the items, +4 u16 the capacity, +8 u16 the count, +10 u16
// the highest count (name?).
constexpr u32 kPvItems = 0, kPvCap = 4, kPvCount = 8, kPvPeak = 10;

}  // namespace obj14
}  // namespace sly4
