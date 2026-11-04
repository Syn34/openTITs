// World objects, part 17: the callees part 16 named but left recompiled
// (the definition lookup by kind, the OBB helpers, the light ticks, ...)
// and the next still-recompiled functions of 0x00550000-0x005AA000 and
// 0x00730000-0x007A0000.
#pragma once

#include <bit>
#include <cmath>

#include "obj16.h"

namespace sly4 {
namespace obj17 {

using namespace opentit;
using obj8::Vec4;

namespace o17fn {
constexpr u32 HashMap_Find2 = 0x007399A4;      // geometry definitions (map, const u32* key) -> slot
constexpr u32 LightDefMap_Find = 0x007A68DC;
constexpr u32 CameraDefMap_Find = 0x0073374C;
constexpr u32 MapE_Find = 0x0075C1DC;          // emitter definitions
constexpr u32 PointDefMap_Find = 0x007A6A64;
constexpr u32 SplineDefMap_Find = 0x007936FC;
constexpr u32 Render_FrameAlloc = 0x004FD1E0;  // (u32 size): MemStack_Alloc(*(0x010DD600 +280), size) (name?)
}  // namespace o17fn

// The definition list headers (+16 the hash -> definition map) by kind.
constexpr u32 kGeomDefHolder = 0x00F8F12C;     // classes.h kGeomDefList
constexpr u32 kLightDefHolder = 0x00F8F130;
constexpr u32 kCameraDefHolder = 0x00F8F134;
constexpr u32 kEmitterDefHolder = 0x00F8F138;
constexpr u32 kPointDefHolder = 0x00F8F13C;
constexpr u32 kSplineDefHolder = 0x00F8F140;
constexpr u32 kDefHolderMap = 16;

}  // namespace obj17
}  // namespace sly4
