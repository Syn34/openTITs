// World objects, part 12: the callees the follower / definition code of
// part 11 left recompiled (IFF field reads, the class maps' removes, the
// pointer table, the topology queries), and the math helpers of
// 0x00550000-0x00579000.
#pragma once

#include <bit>
#include <cmath>

#include "obj11.h"

namespace sly4 {
namespace obj12 {

using namespace opentit;

namespace o12fn {
constexpr u32 Spline_TangentEval = 0x00560430;  // (SplineData*, -, f32 out[4], u32 last seg, bool; f1 t) (name?)
constexpr u32 Topology_DumpDraw = 0x00539874;   // (draw record's +0) (name?)
// The object light cache's update steps (all (obj)) (name?).
constexpr u32 ObjLight_Collect = 0x00581D48, ObjLight_Sort = 0x00582290;
constexpr u32 ObjLight_Blend = 0x00582848, ObjLight_Apply = 0x00582A84;
constexpr u32 Topology_Apply = 0x005099B8;      // (AglTopology*, draw packet*) (name?)
constexpr u32 DrawPacket_Begin = 0x0079D920;    // (draw packet*) (name?)
constexpr u32 DrawRecord_Submit = 0x004FDBB8;   // (topology record*, draw packet*) (name?)
constexpr u32 PtrVector_Construct = 0x005516E0;
constexpr u32 DrawItem_Create = 0x0079D6F8;     // (draw record, AglMaterial*, draw packet*) -> item
constexpr u32 LightSlot_CellRange = 0x005820E0;  // (slot, s32* x0, s32* x1, s32* y0, s32* y1) (name?)
constexpr u32 DrawRecordList_Init = 0x004FD214;  // (list 0x011DE300) (name?)
constexpr u32 Topology_ApplyRecv = 0x00509CEC;  // (AglTopology*, draw packet*, receiver) (name?)
constexpr u32 Topology_ApplyTo = 0x00509F08;    // (AglTopology*, draw packet*, receiver**) (name?)
constexpr u32 WorldObj_AnimMatrix = 0x00572AB0;  // (obj) -> f32[4][4]*: for +164 bits 6-7 (name?)
constexpr u32 DrawPacket_SetTexture = 0x0050CD58; // (packet, +44 word, u16 +28) of the vfunc +28 result (name?)
}  // namespace o12fn

// ---------------------------------------------------------------- IFF reader (obj6.h kIr* / kIs*)
constexpr u32 kVfnStreamRead = 12;              // (stream, buf, size, 0) -> bytes read

// ---------------------------------------------------------------- AglTopology (obj6.h kTopo*)
constexpr u32 kTopoDrawStride = 8;              // {record*, AglMaterial*}
constexpr u32 kTopoNoRadius = 0x0050A260;       // f32 0
constexpr u32 kTsRadius = 16;                   // the shared block's f32 radius

// ---------------------------------------------------------------- pointer tables (obj6.h kEmitterDefTable)
// +4 {u16 capacity, u16 count, -, OPD* moved callback}*, +8 optional Obj**
// (the callback's array), +12 Obj**.
constexpr u32 kPtHeader = 4, kPtAux = 8, kPtData = 12;
constexpr u32 kPhCount = 2, kPhMoved = 8;

// ---------------------------------------------------------------- the class maps (obj.h kMap*)
constexpr u32 kMapCompareE = 0x00F34E50;        // the emitter definition map's default comparator

// ---------------------------------------------------------------- the object light cache (WorldObj_UpdateLight)
// obj +288 the AglLight cache record (+212 u8 bit 0x80 = still settling,
// +213 u8 the slot countdown, +216 u32 the frame it was updated), +292
// nonzero = lit; g::config +96 u32 the frame, +100 / +104 / +108 u32 frame
// thresholds, +128 / +132 f32 the near / far squared distances, +153 u8
// the slots per frame (obj2.h kConfigLightSlots) (name?).
constexpr u32 kObjLit = 292;
constexpr u32 kLcFlags = 212, kLcCountdown = 213, kLcStamp = 216;
constexpr u32 kLcSettling = 0x80;
constexpr u32 kCfgFrame = 96, kCfgStampA = 100, kCfgStampB = 104, kCfgStampC = 108;
constexpr u32 kCfgNear2 = 128, kCfgFar2 = 132, kCfgSlots = 153;
constexpr u32 kViewCamPos = 864;                // View: f32[3]
constexpr u32 kObjLightPos = 128;               // obj: f32[3] the position the light cache uses (name?)
constexpr u32 kVfnLightChanged = 36;            // obj vfunc (obj) (name?)

// ---------------------------------------------------------------- the draw packet (WorldObj_SubmitDraw)
// Built at sp+112 of a 240-byte frame: +0 f32[4][4] the matrix (x / y / z
// lanes; the w lanes are left as they were), +64 the obj +288 word, +68
// u8 obj +320, +69 u8 obj +168, +70 u8 obj +320 word bits 18-23, +71 u8
// flags (0x80 obj +164 bit 0x4000, 0x40 obj +321 bit 0, 0x10 obj +322 bit
// 7), +72 u8[4] obj +316 (the colour), +76 obj +296, +80 / +84 u32 0, +88
// / +89 u8 0, +92 u32 0 (name?).
constexpr u32 kDpSize = 96;
constexpr u32 kObjAnimBits = 0xC0;              // obj +164: bits 6-7 = take the matrix from WorldObj_AnimMatrix
constexpr u32 kObjDpFlag = 0x4000;              // obj +164
constexpr u32 kObjOwnMatrix = 80;               // f32[4][4]: the matrix without a slot entry (name?)
constexpr u32 kObjDrawCb = 332, kObjDrawCbArg = 336;   // OPD* (obj, {0, arg, packet*}*) (name?)
constexpr u32 kVfnDrawTexture = 28;             // obj vfunc (obj) -> {+28 u16, +44 word}* or 0 (name?)
constexpr u32 kVfnClassSubmit = 20;             // class vfunc (class, packet*) -> bool drawn (name?)
constexpr u32 kVfnDrawn = 32;                   // obj vfunc (obj, frame) (name?)
constexpr u32 kDrawFrame = 0x010DDA74 + 4;      // u32 (name?)
constexpr u32 kCfgFinerLod = 34;                // g::config u8: draw one LOD finer when vfunc +28 has a record (name?)

// ---------------------------------------------------------------- Topology_Apply
// The function-local static draw-record list at 0x011DE300 (+0 u8 enabled,
// +4 PtrVector (u16 +10 = 160), +32 u32, +128 / +256 u32 atomics), guarded
// by the u8 at 0x011DE480 (name?).
constexpr u32 kTrList = 0x011DE300, kTrListGuard = 0x011DE480;
constexpr u32 kDrawBuckets = 0x010DDA8C + 4;    // obj*[]: the draw buckets by kind, vfunc +8 (bucket, item, key) (name?)
constexpr u32 kDpLayerMask = 76;                // the packet's s32 layer mask (-1 = all) (name?)
constexpr u32 kDpKind = 70, kDpAlpha = 69, kDpLayer = 68;
constexpr u32 kDrLayer = 108;                   // draw record u8: its layer bit (name?)
constexpr u32 kMatFlags100 = 100, kMatVisible = 0x20;   // AglMaterial u8 (name?)
constexpr u32 kMatKind = 45, kMatWord104 = 104;          // u8 the forced bucket / u32 bit 31 = bucket 4 (name?)

// ---------------------------------------------------------------- the light grid (LightGrid_Rebuild)
// classes.h kLightSlotLists: +0 the 64-byte light slots (+62 u8 bit 0x80
// = active), +4 u16 their count (0xFFFF = none), +16 {u16 start, u16
// count}[8][8] the cells, +272 u16[1024] the cells' slot indices, +2320
// u32 the frame the grid was built (name?).
constexpr u32 kLgSlots = 0, kLgCount = 4, kLgCells = 16, kLgList = 272, kLgFrame = 2320;
constexpr u32 kLgGrid = 8, kLgListMax = 1024, kLgCellBytes = 256;
constexpr u32 kLsFlags62 = 62, kLsActive = 0x80;
constexpr u32 kLightSlotSize = 64;

}  // namespace obj12
}  // namespace sly4
