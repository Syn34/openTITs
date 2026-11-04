// World objects, part 5: tree walks, state copies, skeletons, slot entries
// and adding objects to the world.
#pragma once

#include "classes.h"

namespace sly4 {
namespace obj5 {

using namespace opentit;

namespace o5fn {
constexpr u32 WorldObj_CopyBase = 0x0057DEEC;    // (dst, src, flags): matrix / name / ... (name?)
constexpr u32 Skeleton_CopyPose = 0x00580D70;    // (dst skeleton, src skeleton): the bone transforms (name?)
constexpr u32 Light_Alloc = 0x00526494;          // () -> AglLight*
constexpr u32 Light_Free = 0x0050CAE8;           // (AglLight*)
constexpr u32 Skeleton_Init = 0x005814A0;        // (ApeSkeleton*, ApeSkeletonDef*) (name?)
constexpr u32 World_AddObjects = 0x005947D0;     // (world = *0x01194598, Collector*, 1) (name?)
constexpr u32 World_AddObject = 0x00597814;      // (world, obj) (name?)
constexpr u32 Debug_PrintBone = 0x0055498C;      // (bone def): an empty stub (name?)
constexpr u32 WorldObj_Construct = 0x00580708;   // (obj, kind)
constexpr u32 EmitterDef_Copy = 0x0058E3E4;      // (def, bool) -> a copy (name?)
constexpr u32 EmitterDef_New = 0x0058E140;       // (0, 0) -> a new ApeEmitterDef (name?)
constexpr u32 EmitterData_Copy = 0x005000C8;     // (data, bool) -> a copy (name?)
constexpr u32 EmitterData_Release = 0x005003F4;  // (data) (name?)
constexpr u32 EmitterSystem_Bind = 0x00500714;   // (system, data) (name?)
constexpr u32 PointInstance_Load = 0x00569388;   // (src, PointInstance*) (name?)
constexpr u32 Array_Reserve = 0x00558F3C;        // (Array*, u16 capacity)
constexpr u32 Collector_Add = 0x005A9AA8;        // ForEach callback (node, Collector*) (name?)
}  // namespace o5fn

// ---------------------------------------------------------------- WorldObj_ForEach
constexpr u32 kWalkStackCap = 10;  // the explicit stack's capacity: deeper pushes are dropped

// ---------------------------------------------------------------- WorldObj_AddToWorld
// The object collector it builds on the stack (sp+112).
constexpr u32 kCollectOpd = 0x00F3CA20;   // OPD of Collector_Add
constexpr u32 kColCount = 0, kColCapacity = 4;
constexpr u32 kColRun = 8;                // u32: the collector's run counter
constexpr u32 kColIsInline = 12;          // u8: the buffers are the inline ones (don't free)
constexpr u32 kColOption = 13;            // u8: 1 (Collector_Add's second argument is !this)
constexpr u32 kColData = 16;              // Obj**
constexpr u32 kColFlags = 20;             // u8*
constexpr u32 kColInline = 32;
constexpr u32 kColInlineData = 24;        // sp+136: 32 pointers
constexpr u32 kColInlineFlags = 152;      // sp+264: 32 bytes
constexpr u32 kCollectorFrame = 184;      // sp+112..sp+296

// ---------------------------------------------------------------- WorldObj_CopyFrom
constexpr u32 kCopyKeep292 = 0x40;   // flags: don't copy +292
constexpr u32 kCopyKeepZones = 0x08; // flags: don't copy +324 (the zone mask)
constexpr u32 kObjLight = 288;       // AglLight*: allocated when the source has +292 (name?)
constexpr u32 kObj292 = 292, kObj296 = 296;
constexpr u32 kObjBits320 = 320;     // u32 bitfield: +320 / +321 bytes, 0x00009E00 copied, 0x00001E00 a 4-bit type
constexpr u32 kObj168Type12 = 0x00200000;  // +168 bit set when that type is 1 or 2 (name?)

// ---------------------------------------------------------------- ApeSkeleton (64 bytes)
constexpr u32 kSkelDef = 16;         // ApeSkeletonDef*
constexpr u32 kSkelBoneCount = 28;   // u16
constexpr u32 kSkelBones = 32;       // 16-byte entries: +0 bone def, +4 local matrix*, +8 world matrix*
constexpr u32 kSkelEntrySize = 16;
constexpr u32 kSkelLocal = 4, kSkelWorld = 8;
constexpr u32 kBoneParent = 100;     // s16 in the bone def: -1 = a root
constexpr u32 kDumpLevel = 0x00F889F4;  // s32: the debug dump's nesting level (name?)

// ---------------------------------------------------------------- light slots (64 bytes at *0x011945A0)
constexpr u32 kLsOwner = 48;               // LightInstance*
constexpr u32 kLsNoOwner = 0xFEEDF00D;     // a reset slot's owner
constexpr u32 kLightConsts = 0x005A31B8;   // f32 {2.0, 1.0, 0.0}

// ---------------------------------------------------------------- instance lists
constexpr u32 kVfnSetClass = 44;
constexpr u32 kLightInstanceSize = 400, kEmitterInstanceSize = 352, kCameraInstanceSize = 352;
constexpr u32 kLightInstListVtbl = 0x00B13920;
constexpr u32 kStrLightInstList = 0x00AF4BCF;    // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = ApeLightInstance]"
constexpr u32 kEmitterInstListVtbl = 0x00B10EF0;
constexpr u32 kStrEmitterInstList = 0x00AF4C23;  // "... [with T = ApeEmitterInstance]"
constexpr u32 kCameraInstListVtbl = 0x00B24EB8;
constexpr u32 kStrCameraInstList = 0x00AF4C77;   // "... [with T = ApeCameraInstance]"
constexpr u32 kVtblEmitterInstance = 0x00B248F8;
constexpr u32 kVtblCameraInstance = 0x00B24948;
constexpr u32 kEmitter340 = 340;
constexpr u32 kEdData = 112, kEdSystem = 116;    // ApeEmitterDef: the emitter data / the particle system

// ---------------------------------------------------------------- ApePointInstance (kind 12)
constexpr u32 kKindPoint = 12;
constexpr u32 kMapHolderPoint = 0x00F8F154;      // header*: +0 list, +4 capacity, +8 callback OPD, +12 its argument, +16 map
constexpr u32 kPointInstanceSize = 288;
constexpr u32 kPointInstListVtbl = 0x00B13900;
constexpr u32 kStrPointInstList = 0x00AF4CCD;    // "... [with T = ApePointInstance]"
constexpr u32 kVtblPointInstance = 0x00B24998;
constexpr u32 kPointOffset = 0x011944E0;         // f32[3] added to unparented points (name?)

// ---------------------------------------------------------------- kind 13 (vtable 0x00B249E8, unidentified)
constexpr u32 kKind13 = 13;
constexpr u32 kVtblKind13 = 0x00B249E8;
constexpr u32 kK13List = 0x01194580;   // +4 {u16 cap, u16 count}*, +8 Obj**, +12 96-byte records (name?)
constexpr u32 kK13RecSize = 96;

}  // namespace obj5
}  // namespace sly4
