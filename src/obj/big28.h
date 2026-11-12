// Game code, large-function sweep part 28: helpers shared by big28_*.cpp
// (the Havok calls of the body / ray code).
#pragma once

#include "native/objtypes.h"

namespace sly4::obj {

namespace b28fn {
constexpr u32 HkBody_SetPosition_0091DFF0 = 0x0091DFF0;  // (hkpRigidBody*, const Vec4*) (through its motion +224 vfn[48]) (name?)
constexpr u32 HkBody_SetPosRot = 0x0091ECA8;    // (hkpRigidBody*, const Vec4* pos, const Vec4* rot)
constexpr u32 HkHits_Sort = 0x008871E8;         // (Collector*) sort the 48-byte hits
constexpr u32 Hk_Activate = 0x0091A4C8;         // (body)
constexpr u32 HkHeap = 0x011DF31C;              // Havok heap allocator object (vfn[20] (p, bytes) frees)
constexpr u32 VtHitCol = 0x00F1DE58;            // ray hit collector vtable (48-byte hit array)
constexpr u32 VtHitColEnd = 0x00B13C84;         // stored over it when the collector ends
}  // namespace b28fn

}  // namespace sly4::obj
