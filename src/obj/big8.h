// Game code, large-function sweep part 8: helpers shared by big8_*.cpp
// (callees part 7 named and further still-recompiled game functions of
// 400+ instructions).
#pragma once

#include <bit>
#include <cmath>

#include "big7.h"

namespace sly4 {
namespace big8 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using obj19::VCall;
using obj25::Sx;
using big4::CopyW;
using big4::CopyWords;
using big5::V4;
using big5::Lvx;
using big5::Stvx;
using big5::Row3;
using big5::Add4;
using big6::LaneW;
using big6::Lvlx;
using big7::F;
using big7::Splat1;
using big7::Sel;
using big7::Andc;
using big7::Fmadds;
using big7::Fmsubs;
using big7::SetF1Bits;

namespace b8fn {
using namespace big7::b7fn;
constexpr u32 kHkCastRay = 0x0092E510u;      // hkpWorld::castRay(world, const hkpWorldRayCastInput*, hkpRayHitCollector*) (Havok)
constexpr u32 kSortHits = 0x00887E28u;       // (hkpAllRayHitCollector*): sort hits by fraction (Havok)
constexpr u32 kEventInitB = 0x0046582Cu;     // (Event* e, o, 0, 0, 0)
}  // namespace b8fn

constexpr u32 kWorldPtr = 0x00F65CA0u;       // World*
constexpr u32 kWorldPhys = 0x001281D4u;      // World: physics holder (+4 hkpWorld*)
constexpr u32 kWorldCamera = 0x001281E0u;    // World: camera (+144 position) or 0
constexpr u32 kZero3 = 0x01164B40u;          // the zero vector
constexpr u32 kIdentity = 0x01164BD0u;       // identity matrix (+16 the up row)
constexpr u32 kCollectorVt = 0x00F1DE80u;    // vtable of the all-hits ray collector (hkpAllRayHitCollector)
constexpr u32 kHeapAlloc = 0x011DF31Cu;      // allocator object: vfunc +20 free(p, size)
constexpr u32 kObjProp = 0x91C7CF5Bu;        // entity property key: the game object

// The entity a hit's collidable belongs to (its +24 type 1, owner at
// +(s8)+16), or 0.
inline u32 HitEntity(u32 col) {
    if (static_cast<s8>(load<u8>(col + 24)) != 1) return 0;
    return col + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(col + 16))));
}

}  // namespace big8
}  // namespace sly4

// ---------------------------------------------------------------- native
#include "native/objtypes.h"
