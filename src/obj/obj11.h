// World objects, part 11: the spline follower (Peer_ApplyFollower /
// Follower_Update), the quaternion helpers, the definition destructors,
// the instance free-list returns and the view-sphere visit.
#pragma once

#include <bit>
#include <cmath>

#include "obj10.h"

namespace sly4 {
namespace obj11 {

using namespace opentit;

namespace o11fn {
constexpr u32 Matrix_Multiply = 0x005541F0;     // (Matrix* a_and_out, const Matrix* b): VMX (name?)
constexpr u32 Quat_FromMatrix = 0x00555478;     // (f32 out[4], const f32 m[4][4]) (name?)
constexpr u32 Spline_PointAt = 0x005602BC;      // (SplineData*, f1 t, f32 out[3], bool)
constexpr u32 Follower_Advance = 0x0057A770;    // (follower, -, f32* t, f32 out[3]; f1 dt) (name?)
constexpr u32 Follower_AdvanceAlt = 0x0057AF20; // the same for followers with +108 bit 0x40 (name?)
constexpr u32 View_GetCurrent = 0x004FDBA8;     // () -> View*
constexpr u32 Topology_Radius = 0x0050A268;     // (AglTopology*) -> f1 (name?)
constexpr u32 Obj_Visit583518 = 0x00583518;     // () (name?)
constexpr u32 Obj_Visit5832FC = 0x005832FC;     // (obj) (name?)
constexpr u32 Obj_Visit58303C = 0x0058303C;     // (obj) (name?)
constexpr u32 PtrTable_Remove = 0x007A8444;     // (PtrTable*, s32 index) (name?)
constexpr u32 MapE_Find = 0x0075C1DC;           // the emitter definition map's lookup (name?)
constexpr u32 MapE_Remove = 0x007A8250;         // (map, const u32* key) (name?)
constexpr u32 MapB_Remove = 0x007A7A80, MapC_Remove = 0x007A892C, MapD_Remove = 0x007A8B20;  // (map, const u32* key) (name?)
}  // namespace o11fn

// ---------------------------------------------------------------- spline followers (peer +136)
// +0 f32[4][4] the follower's matrix (rows 0-2 rotation, +48 position),
// +96 f32 the start delay, +100 u32 nonzero = active, +104 the spline
// record (96 bytes: +0 f32[4][4] its world matrix, +80 SplineData*),
// +108 u8: bit 0x40 picks Follower_AdvanceAlt.
constexpr u32 kFoPos = 48, kFoDelay = 96, kFoActive = 100, kFoRecord = 104, kFoBits = 108;
constexpr u32 kFoAlt = 0x40;
constexpr u32 kSrMatrix = 0, kSrSpline = 80;
constexpr u32 kFuConsts = 0x0057BA30;           // f32 {0, 1, 0.001, 0.0001}
constexpr u32 kFuZero = 0, kFuOne = 4, kFuParallel = 8, kFuTiny = 12;
constexpr u32 kUpAxis = 0x01164B60;             // f32[3]: the axis the follower turns away from (name?)
constexpr u32 kPaZero = 0x0057C0E0;             // f32 0 (+16 / +32: Peer_ApplyFollower's vperm masks)
constexpr u32 kPaPermA = 0x0057C0F0, kPaPermB = 0x0057C100;

// ---------------------------------------------------------------- quaternions
constexpr u32 kQmConsts = 0x00553A84;           // f32 {2, 1}

// ---------------------------------------------------------------- free lists
// The instance free-list headers are the class maps' holders (obj.h
// kMapHolderB / C / D): +0 allocator, +4 capacity, +16 the name-hash map.
constexpr u32 kGeomInstListVtbl = 0x00B0F008;
constexpr u32 kStrGeomInstList = 0x00AF4B7E;    // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = ApeGeomInstance]"
constexpr u32 kGeomInstanceSize = 352;
constexpr u32 kEdNameHash = 96;                 // u32: ApeEmitterDef's map key
constexpr u32 kInMap = 0x400;                   // obj2::kObjWord8 bit: in the class map
constexpr u32 kVfnDestruct = 0;

// ---------------------------------------------------------------- definition destructors
constexpr u32 kVtblEmitterDef = 0x00B247E0;
constexpr u32 kGdSubStride = 8;                 // obj2::kClassTopos entries
constexpr u32 kVfnDefRelease = 8;
constexpr u32 kVfnEdReleaseData = 12;           // ApeEmitterDef slot +12 (name?)

// ---------------------------------------------------------------- the view-sphere visit (0x00583864)
// The view's two sets of four planes, transposed: +368 / +384 / +400 the
// x / y / z coefficients and +416 the distances of planes 0-3, +432 /
// +448 / +464 / +480 those of planes 4-7.
constexpr u32 kViewPlanesA = 368, kViewPlanesB = 432;
constexpr u32 kObjBoundSphere = 240;            // f32[4]: centre, radius
constexpr u32 kObjSlotIndex = 284;              // u16: slot entry (0xFFFF = none)
constexpr u32 kSlotEntryPos = 48, kSlotEntryShift = 7;
constexpr u32 kCameraPos = 0x010DD780 + 304;    // f32[3] (name?)
constexpr u32 kClassTopo0 = 132;                // AglTopology*: the class's first topology

// ---------------------------------------------------------------- helpers
// Returns an instance (or definition) to its ApeTListAllocator free list:
// the object's destructor (vfunc +0), then alloc->Free(o). An object still
// in its class map (+8 bit 0x400) is first taken out when the map's entry
// for its name hash is the object itself (the keys on the stack at sp+112
// / sp+116 of the original's 160-byte frame), and the bit is cleared.
// The geometry-base destructor body the definition destructors end with:
// the base vtable (0x00B246B0), the +108 shared block (a reference
// dropped; the last frees its data, allocated 16 bytes in, and the
// block), then ApeRoot_Destruct.
void GeomBaseDestructBody(PPUContext& ctx, u32 d);

void FreeToList(PPUContext& ctx, u32 o, u32 hdr_ea, u32 vtbl, u32 name, u32 elem_size, u32 find, u32 remove,
                u32 key_off);

}  // namespace obj11
}  // namespace sly4
