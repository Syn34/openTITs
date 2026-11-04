// World objects, part 16: the callees part 14 / 15 named but left
// recompiled (the sorted map instances, the input device and record
// helpers, ...) and the next still-recompiled functions of
// 0x00550000-0x005AA000 and 0x00730000-0x007A0000.
#pragma once

#include <bit>
#include <cmath>

#include "obj.h"
#include "obj15.h"

namespace sly4 {
namespace obj16 {

using namespace opentit;

// The default comparators (OPDs, unsigned u32 keys) the sorted map
// instances test for (obj.h layout) (name?).
constexpr u32 kCmpPointMap = 0x00F2E1B0;     // 0073343C / 007A8F34
constexpr u32 kCmpCameraDefMap = 0x00F2E1F8; // 0073374C / 007A805C
constexpr u32 kCmpSplineInst = 0x00F2E1F0;   // 007A92F4
constexpr u32 kCmpGeomDefMap = 0x00F2FB88;   // 007A7C74 (obj::kMapCompareU32B)
constexpr u32 kCmpLightDefMap = 0x00F3CBF0;  // 007A7E68 (Map_Find3's)
constexpr u32 kCmpPointDefMap = 0x00F3CC28;  // 007A6A64 / 007A8544
constexpr u32 kCmpCameraInst = 0x00F3CC48;   // 007A8D40
constexpr u32 kCmpSplineDefMap = 0x00F3A9B0; // 007936FC / 007A8738

}  // namespace obj16
}  // namespace sly4
