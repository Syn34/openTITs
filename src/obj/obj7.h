// World objects, part 7: the base ApeObj slots, the point / spline
// instance vtables (0x00B24998 / 0x00B249E8) and the callees the earlier
// parts left recompiled.
#pragma once

#include "box.h"
#include "obj6.h"

namespace sly4 {
namespace obj7 {

using namespace opentit;

namespace o7fn {
constexpr u32 ApeObj_Destruct = 0x0057DCF0;           // the base +0 (still recompiled)
constexpr u32 WorldObj_ReleasePeers = 0x0057DA68;     // (obj) ("WorldObj_DetachAll" in obj3.h)
constexpr u32 ApeRoot_Destruct = 0x00558CE8;          // (obj) ("WorldObj_DestructRoot" in obj3.h)
constexpr u32 PointInstance_New = 0x005A68EC;         // (ApePointDef*) -> ApePointInstance* (name?)
constexpr u32 SplineDef_Copy = 0x0059F248;            // (ApeSplineDef*) -> a copy (name?)
constexpr u32 SplineInstance_New = 0x005A7CB4;        // (ApeSplineDef*) -> ApeSplineInstance* (name?)
constexpr u32 SplineInstance_SetMode = 0x005A7B38;    // (inst, u8 the record's +94) (name?)
constexpr u32 SplineInstance_SyncRecord = 0x005A782C; // (inst)
constexpr u32 SplineInstance_Destruct = 0x005A7680;   // (inst)
constexpr u32 SplineList_Remove = 0x007A9128;         // (list 0x01194580, s32 index) (name?)
}  // namespace o7fn

// ---------------------------------------------------------------- ApeSplineInstance (kind 13)
// The records of obj5.h's kK13List (96 bytes): +0 f32[4][3] rows of the
// world matrix, +64 the object's +240 vec4, +80 the spline data (class
// +112), +84 its +4, +88 the owner, +92 u16 its +0 & 0x7FFF, +94 u8 mode,
// +95 u8: 0x80 active, 0x40 follows its kind-13 parent (name?).
constexpr u32 kSplineList = obj5::kK13List;
constexpr u32 kSrData = 80, kSrData4 = 84, kSrOwner = 88, kSrCount = 92, kSrMode = 94, kSrBits = 95;
constexpr u32 kSrActive = 0x80, kSrFollowParent = 0x40;
constexpr u32 kSplineDefData = 112;          // ApeSplineDef +112: {u32 (count in bits 0-14), u32}
constexpr u32 kObjSplineIndex = 284;         // s32: the record index (-1 = none)
constexpr u32 kSplineDestroyOpd = 0x00F3C9F8;
constexpr u32 kPointDestroyOpd = 0x00F3C9A8;

// ---------------------------------------------------------------- Light_Accepts
constexpr u32 kCompTypeLight = 0x00F88954;   // u32 type hash (after obj6.h's three)

// ---------------------------------------------------------------- shared helpers
// The ApeObj destructor body the point / spline destructors inline: the
// base vtable, off the dirty list when queued (the last entry moved into
// its place), then the peers, the class and the root destructor.
void ApeObjDestructBody(PPUContext& ctx, u32 o);

// The Release slot of the point / spline instances: drops a reference;
// at zero unlinks, removes the children (vfunc +24), releases the
// resources and defers the destructor `destroy_opd`.
void InstanceRelease(PPUContext& ctx, u32 o, u32 destroy_opd);

// The inlined peer copy of the Changed slots: an object with a peer record
// (+164 high half != 0xFFFF) copies its +240 vec4 to the record's vec4.
// Returns the destination (0 when there is none).
u32 CopyAnchor(u32 o);

}  // namespace obj7
}  // namespace sly4
