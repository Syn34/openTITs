// The renderer range's next recompiled functions (gfx21*.cpp): the tuning
// variable base class, the trophy manager and sub-mesh base destructors,
// the pad rumble update and the next undone functions of 0x004FB000 -
// 0x005E0000.
#pragma once

#include "gfx20.h"

namespace sly4::render {
namespace gfx21 {

namespace fn {
// Still recompiled or decompiled elsewhere (called from gfx21*.cpp).
constexpr u32 String_Assign = 0x00555F80;              // (String*, const char*)
constexpr u32 String_HashNoCase = 0x0056A710;          // (const char*) -> u32
constexpr u32 Mem_FreeFrom = 0x0054EE6C;               // (p, pool)
constexpr u32 Array_Destruct = 0x0054EC94;             // (Array*)
constexpr u32 Pool_DestructPart = 0x0052EFE0;          // (part*) (gfx/layers4.cpp)
constexpr u32 ApeRoot_Destruct = 0x00558CE8;           // (ApeRoot*) (obj/obj4_res.cpp)
constexpr u32 Thread_Join = 0x005E760C;                // (thread, u32* exit_value) -> s32 (pthread_join-like; SDK)
constexpr u32 Event_Destroy = 0x005E6DB8;              // (event*) (SDK sync object destroy, name?)
constexpr u32 InputDevice_IntegrateAxes = 0x00562C58;  // (InputDevice*) (obj/obj15_small.cpp)
constexpr u32 Mem_Alloc = 0x0054ED38;                  // (size, pool, align) -> p
constexpr u32 FreeList_Init = 0x00551A58;              // (FreeList*, size, heap, 0, 0)
constexpr u32 Box_SetPoint = 0x00556F60;               // (f32 box[6], const f32 p[3])
constexpr u32 Iff_ReadChunks = 0x00569388;             // (IffReader*, obj)
constexpr u32 Array_Reserve = 0x00558F3C;              // (Array*, u32 capacity)
constexpr u32 GeomDef_Reset = 0x00586148;              // (ApeGeomDef*) (obj/obj15_small.cpp)
constexpr u32 Bank_BuildViewJob = 0x0058D920;          // (-, per, index) -> Job* (obj/obj18.cpp)
constexpr u32 Plus12080_005382A8 = 0x005382A8;         // (p) -> p + 0x12080 (gfx7_stubs.cpp)
constexpr u32 Render_FrameAlloc = 0x004FD1E0;          // (u32 size) (obj/obj22.cpp)
constexpr u32 View_GetCurrent = 0x004FDBA8;            // () -> View* (view/view2.cpp)
constexpr u32 WorldObj_AnimMatrix = 0x00572AB0;        // (obj) -> f32[4][4]* (obj/obj13_draw.cpp)
constexpr u32 DrawPacket_SetTexture = 0x0050CD58;      // (packet, u32 tex +44, u16 tex +28)
constexpr u32 Matrix_Scale3 = 0x00554414;              // (f32 m[4][4]; f1 s): rows 0-2 scaled (obj/obj14_small.cpp)
constexpr u32 cellGcmSetVertexProgramConstants = 0x005E5CB0;  // (ctx, first, count, const f32*)
constexpr u32 DrawItem_Create = 0x0079D6F8;            // (shader, 0, packet) -> DrawItem* (gfx/shader3.cpp)
constexpr u32 DrawItem_DrawBank_005393FC = 0x005393FC; // (Mesh*, DrawItem*, Bank*) (gfx/gfx18.cpp)
constexpr u32 SlotEntry_Update = 0x00579C50;           // (f32 slot[4][4]*, obj) (obj/obj5_slot.cpp)
constexpr u32 SlotEntry_Alloc = 0x0057A53C;            // (obj) (obj/obj5_slot.cpp)
constexpr u32 Skeleton_Compose = 0x00580EA0;           // (skeleton, out matrices) (obj/obj7_skel.cpp)
constexpr u32 GroupList_SetLevels = 0x005902B8;        // ({f32 level, u32 count, Obj** items}*) (obj/obj15_small.cpp)
constexpr u32 EventSlot_Set = 0x00565304;              // (u32 slot, u64 {OPD, arg}) (obj/obj14_small.cpp)
constexpr u32 EventSlot_Wait = 0x0055E2AC;             // (u32 slot) (obj/obj14_small.cpp)
constexpr u32 PtrVector_Reserve = 0x00551718;          // (PtrVector*, u32 capacity) (container/vector.cpp)
constexpr u32 Sphere_FrustumFine = 0x007A6EFC;         // (planes[6][4], c[3], e[3], m[3][4]) -> bool (obj/obj17.cpp)
constexpr u32 Pack_Entry_007A4874 = 0x007A4874;        // (Pack*, s32 i, u32* size) -> data (gfx/gfx13.cpp)
constexpr u32 MemStack_Alloc = 0x0055260C;             // (MemStack*, u32 size) (heap/stack_alloc.cpp)
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 PtrVector_Init = 0x00551598;             // (PtrVector*, data, count, capacity) (container/vector.cpp)
constexpr u32 EmitterData_Release = 0x005003F4;        // (EmitterData*) (obj/obj6_emitter.cpp)
constexpr u32 SmallAlloc = 0x0055238C;                 // (u32 size) (common/alloc.cpp)
constexpr u32 Thunk_005008D8 = 0x005008D8;             // (inst, data): the emitter instance base ctor (gfx7_thunks.cpp)
constexpr u32 Min256_00500534 = 0x00500534;            // (data) -> min(u16 +40, 256) (gfx7_stubs.cpp)
constexpr u32 GetForThread_00522C98 = 0x00522C98;      // () -> the thread's LOD parameters
constexpr u32 Obj_GetField4 = 0x00556BF0;              // (p) -> p +4 (obj/obj13_misc.cpp)
constexpr u32 Plane_StraddlesBox = 0x00577680;         // (const f32 n[3], const f32 p[3], const f32 h[3]) -> bool
constexpr u32 DrawRecordList_Init = 0x004FD214;        // (list) (obj/obj13_small.cpp)
constexpr u32 DrawRecord_Submit = 0x004FDBB8;          // (packet, record) (obj/obj13_draw.cpp)
constexpr u32 PushMarker_0051300C = 0x0051300C;        // (stream): {-1, 0} at its cursor +8 (gfx7_thunks.cpp)
constexpr u32 Layer0_FlushMarker_0051A6D4 = 0x0051A6D4;  // ()
constexpr u32 EmitterJobs_Submit_00528130 = 0x00528130;  // () (gfx/gfx16.cpp)
constexpr u32 WorkQueue_SubmitJobDesc = 0x0056EC80;    // (owner, desc, entries, count) (obj/obj15_*)
constexpr u32 GeomInstance_DrawLod = 0x0058AC70;       // (obj, u8 lod, u8, u8 kind) (obj/obj12.cpp)
constexpr u32 memset = 0x00726E54;
constexpr u32 Matrix_LookAt = 0x0056A0D0;              // (f32 out[4][4], eye[3], dir[3], up[3]) (world/render.cpp)
constexpr u32 Matrix_PerspectiveWH = 0x0056A2D8;       // (f32 m[4][4]; f1 w, f2 h, f3 n, f4 f) (obj/obj14_small.cpp)
constexpr u32 Mat4_Ortho = 0x0056A358;                 // (out; f1 l, f2 r, f3 b, f4 t, f5 n, f6 f) (view/view2.cpp)
constexpr u32 Matrix_MultiplyTo = 0x0055499C;          // (out[16], a[16], b[16])
constexpr u32 Frustum_FromMatrix = 0x004FDEA0;         // (Frustum*, const f32 m[16])
}  // namespace fn

namespace imp {
constexpr u32 sceNpTrophyDestroyHandle = 0x00ABE684;
constexpr u32 sceNpTrophyDestroyContext = 0x00ABE624;
constexpr u32 sceNpTrophyTerm = 0x00ABE6C4;
constexpr u32 cellPadSetActDirect = 0x00ABDF04;
}  // namespace imp

// Tuning variables: every one registers itself in a global table
// (gameload's lg::kTuningVars / kTuningVarCount) on construction and
// removes itself on destruction. Base layout: +0 vtable, +4 String name,
// +68 u32 String_HashNoCase(name).
constexpr u32 kTuningVarBaseVtbl = 0x00B23110;
constexpr u32 kTuningVars = 0x010C2DBC;      // TuningVar*[]
constexpr u32 kTuningVarCount = 0x010C597C;  // u32
constexpr u32 kTvName = 4, kTvNameHash = 68;

// SubMesh base (vtable 0x00B23148): +16 owned object (deleted through
// vtable slot +8), +24 / +44 pool parts (Pool_DestructPart), +72 / +84
// buffers (Mem_FreeFrom).
constexpr u32 kSubMeshBaseVtbl = 0x00B23148;
constexpr u32 kSmOwned = 16, kSmPart24 = 24, kSmPart44 = 44, kSmBuf72 = 72, kSmBuf84 = 84;

// Pad device (InputDevice subclass) rumble fields: +12 u8 port, +132 /
// +136 the two axes (InputDevice_IntegrateAxes), +148 u8 active, +284
// CellPadActParam {u8 motor[2]}, +292 f32 the small motor's accumulator.
constexpr u32 kPadPort = 12, kPadAxis0 = 132, kPadAxis1 = 136, kPadActive = 148;
constexpr u32 kPadAct = 284, kPadAccum = 292;
constexpr u32 kPadRumbleConsts = 0x0056F9C8;  // f32 1, 0, 255
// CellPadInfo2 (gameload::lg::kPadInfo) +68: u32 device_capability[7].
constexpr u32 kPadCapability = 0x0116B2EC;
constexpr u32 kPadCapActuator = 0x10;  // CELL_PAD_CAPABILITY_ACTUATOR

// The world definition classes read by the IFF chunk handlers (the same
// body as gfx20's GeomDef_ReadChunks_0059B788 with GeomDef_New inlined):
// each class has a global {+0 ApeTFreeList*, +4 heap, +8 load hook OPD,
// +12 hook argument, +16 the named map}. A new def gets the root header
// (kind << 10 at +4, a new serial at +8), the geometry base (vtable
// 0x00B246B0: empty box collapsed to the origin, the default sphere, no
// name), then its own vtable and fields.
struct DefClass {
    u32 global;     // the class pointer's address
    u32 size;       // item size of its free list
    u32 list_vtbl;  // ApeTFreeList<T> vtable
    u32 list_name;  // its GetAllocator() __PRETTY_FUNCTION__
    u32 kind;       // root header kind
    u32 vtbl;       // the def's own vtable
};
constexpr DefClass kLightDef = {0x00F8F130, 176, 0x00B24E58, 0x00AF49F0, 21, 0x00B24700};
constexpr DefClass kCameraDef = {0x00F8F134, 192, 0x00B140D0, 0x00AF4A3E, 23, 0x00B247B8};
constexpr DefClass kPointDef = {0x00F8F13C, 112, 0x00B24E98, 0x00AF4AE0, 24, 0x00B24808};
constexpr u32 kGeomBaseVtbl = 0x00B246B0;
constexpr u32 kObjSerial = 0x01164CD0;
constexpr u32 kOrigin = 0x01164B40;         // f32[3]
constexpr u32 kDefaultSphere = 0x01164BC0;  // f32[4]
constexpr u32 kIdentity3x4 = 0x01164BD0;    // f32[4][4]: the camera def's frame
constexpr u32 kFltMax = 0x7F7FFFFF, kFltMinNeg = 0xFF7FFFFF;
constexpr u32 kDefShared = 108, kDefName = 96, kDefRegistered = 0x400;

// SkinBankJob_Build_0059A134: the skinning fields it adds to the view job
// Bank_BuildViewJob makes (job +288 .. +448), from draw3.h's skin buffers.
constexpr u32 kVjWork = 0x0118E080;  // +8 the view items (+4 Item*[]), +12 u16 their count
constexpr u32 kArena = 0x010DD600;   // +4 u8, +20 u32, +256 u32 (layers3 block_dd600)
constexpr u32 kSkinMatrixCount = 0x01163F00, kSkinMatrixFrame = 0x01163E90, kSkinMatrices = 0x01151A80;
constexpr u32 kSkinJobCount = 0x01151900, kSkinJobFrame = 0x01151880, kSkinJobs = 0x010F1880;
constexpr u32 kSkinImage = 0x00D69100, kSkinEntries = 0x300;
constexpr u32 kSkinImageQuads = ((0x10000u - 27360u + 15u) >> 4) & 0xFFFF;  // 0x952
constexpr u32 kSkinWord368 = 0x010F0E08;  // u32 copied to job +368 (name?)
constexpr u32 kSkinLocks = 0x01192C00, kSkinSlotsA = 0x01192A00, kSkinSlotsB = 0x01192800;  // 128 bytes per item index

// Item_Submit_0058C53C: the world item fields it reads. +80 own matrix,
// +164 flags (bits 6-7 animated, bit 14), +168 u8, +208 the sink (vfunc
// +20 takes the packet), +240 f32[3] the bound centre, +252 its radius,
// +256 / +260 a second object and its enable, +284 u16 slot matrix index
// (0xFFFF: own), +288 / +296 words, +316 u8[4] colour, +320 u8 / bits
// 18-23, +321 bit 0, +322 bit 7, +340 the occluder set (+8: {80-byte
// boxes*, u16 count}).
constexpr u32 kSlotMatrices = 0x01193004;  // f32[4][4][] (128 bytes each)
constexpr u32 kItemPassArg = 0x010DDA78;   // u32 passed to vfunc +32 (name?)

// Bank_DrawItem: the bank's draw list {Obj*, Model*, f32 scale, -} (16
// bytes, from +416 to the cursor +512, at most 256), its matrix at +48;
// the object's u8 +168 is a signed fade (- 128, / 127). Model: +48
// {Mesh*, Mesh*}[] (+0 drawn, +4 flags at +100 bit 5), +52 u16 count.
constexpr u32 kBankDrawList = 416, kBankDrawCursor = 512, kBankMatrix = 48;
constexpr u32 kBankDrawConsts = 0x0058D080;  // f32 127, 1e-5
constexpr u32 kGcmContext = 0x010F10A0;      // CellGcmContextData*

// Bank_Stop596A8C: the bank's per-frame bookkeeping lists.
constexpr u32 kChangedList = 0x0116BD08;     // {Obj** data, u32 count, u32 -, u32 reset value}
constexpr u32 kSkeletonWatch = 0x01194590;   // u32 -> {ApeSkeleton** data, u32 count}
constexpr u32 kGroupLevel = 0x01194560;      // f32
constexpr u32 kGroupStamp = 0x01194564, kGroupStampSeen = 0x01194568;  // u32: levels redone when they differ
constexpr u32 kGroupHolder = 0x0119457C;     // u32 -> {+4 -> {u16 +2 count}, +12 Obj** items}
constexpr u32 kGroupJobOpd = 0x00F3C618;     // OPD run on event slot 0 with the first part of the list
constexpr u32 kGroupSplit = 0x200;           // more objects than this: half (- 32) go to the worker
constexpr u32 kFlaggedList = 0x011944C4;     // {Obj** data, -, u16 +8 count}
constexpr u32 kFlaggedOut = 0x011944D8;      // u32 -> PtrVector {data, u16 +4 capacity, +8 count, +10 peak}
constexpr u32 kFrameCounter = 0x010C5A80;    // u32 bumped by modes 1 and 2 (config +96)

// Boxes_PickNearest_005843C0: a set of oriented boxes {+0 u32 boxes (80
// bytes: f32 extents[3] +0, axes [3][4] +16, centre +64), +4 u16 count,
// +8 u32 output (80 bytes each), +12 u16 picked, +16 f32 radius}.
constexpr u32 kPickConsts = 0x00584390;  // f32 FLT_MAX, 0; +16 / +32 the two vperm masks
constexpr u32 kPickMax = 2;

// PackEntry_Unpack_00563368: the memory stack it allocates from.
constexpr u32 kUnpackStack = 0x0116489C;  // u32 MemStack* (0x01164898 +4)

// Constants_Init_0056B93C: the math / colour constants of 0x01164888 -
// 0x01164CD4 (names from their values), then the chunk vector and map.
namespace k {
constexpr u32 kZero64 = 0x01164888;  // u64 0, then u32 0 at +8
constexpr u32 kVec2One = 0x01164AF4, kVec2Zero = 0x01164AFC, kVec2X = 0x01164B04, kVec2Y = 0x01164B0C;
constexpr u32 kVec2NegX = 0x01164B14, kVec2NegY = 0x01164B1C;
// Vec3 in 16-byte slots: one, zero (kOrigin), +x, +y, +z, -x, -y, -z.
constexpr u32 kVec3One = 0x01164B30;
constexpr u32 kVec4One = 0x01164BB0, kVec4Zero = 0x01164BC0;
constexpr u32 kMatrix3x4 = 0x01164BD0;   // identity rows (w lanes untouched)
constexpr u32 kMatrix4x4 = 0x01164C10;   // identity
constexpr u32 kMatrix3x3 = 0x01164C50;   // identity rows (w lanes untouched)
constexpr u32 kVec4Zero2 = 0x01164C80, kQuatIdentity = 0x01164C90;
constexpr u32 kColours = 0x01164CA0;     // u8[12][4] RGBA
constexpr u32 kWord4CD4 = 0x01164CD4;
constexpr u32 kChunkVector = 0x01164E14; // PtrVector, element size 4
constexpr u32 kChunkPair = 0x01164E28;   // {u32 0, u32 256}
constexpr u32 kChunkMap = 0x01164E30;    // sorted map (obj6.h kChunkMap): 256 entries of 8 bytes
constexpr u32 kDefaultAllocatorPtr = 0x00F88704, kDefaultAllocator = 0x01165018;
constexpr u32 kDefaultAllocatorGuard = 0x01165010, kDefaultAllocatorVtable = 0x00B24368;
constexpr u32 kDefaultCompare = 0x00F3C250;
}  // namespace k

// SimpleEmitter_SetData_0059D6BC.
constexpr u32 kSimpleEmitterBaseVtbl = 0x00B23308, kSimpleEmitterVtbl = 0x00B23338;
constexpr u32 kRingFreeList = 0x011DE4B4;      // ApeTMemClassFL<ApeTRingBuf<AglSimplePtl>, 1024>
constexpr u32 kRingFreeListVtbl = 0x00B0EF68;
constexpr u32 kRingFreeListName = 0x00AF48CB;  // its GetAllocator() __PRETTY_FUNCTION__

// GeomLod_Update_005908A0: the instance and def classes' free lists.
constexpr u32 kGeomInstClass = 0x00F8F144, kGeomInstListVtbl = 0x00B0F008, kGeomInstListName = 0x00AF4B7E;
constexpr u32 kGeomDefClass = 0x00F8F12C, kGeomDefListVtbl = 0x00B24E38, kGeomDefListName = 0x00AF49A4;
constexpr u32 kLodConsts = 0x00590894;  // f32 100, 1e-5

// GeomJobs_Submit_00599864: the geometry draw jobs.
namespace gj {
constexpr u32 kQueuePtr = 0x0117EE9C;     // u32 -> u32 the queue's end (entries of 16 bytes from kQueue)
constexpr u32 kQueue = 0x0116E110;
constexpr u32 kConfig = 0x010C5A20;       // +48 f32, +52 u32 copied into the job
constexpr u32 kJobBlock = 0x010DD780;
constexpr u32 kPassInfo = 0x010DDA74;     // +16 u32; +24 (0x010DDA8C) u32 stream count, +28 Stream*[]
constexpr u32 kStreamCount = 0x010DDA8C, kStreams = 0x010DDA90;
constexpr u32 kDone = 0x01192680;         // u32, atomic: non-zero while the jobs run
constexpr u32 kLodOut = 0x0118E0E0;       // {Obj*, u8 lod, u8} entries (16 bytes), end at kLodOutEnd
constexpr u32 kLodOutEnd = 0x0118E500;
constexpr u32 kRecords = 0x0118E580;      // 112-byte draw records, end at kRecordsEnd
constexpr u32 kRecordsEnd = 0x01191D80;
constexpr u32 kCursors = 0x01191E00;      // u32 per stream, 128 bytes apart
constexpr u32 kRecList = 0x011DE300, kRecListGuard = 0x011DE480;
constexpr u32 kMinPer = 32, kMaxPer = 80;
constexpr u32 kSleepUs = 300;             // sys_timer_usleep while the jobs run
constexpr u32 kSysTimerUsleep = 141;
}  // namespace gj

// SlotsWork_Prepare_005894F8: the slots job's work record and its inputs.
namespace sw {
constexpr u32 kWork = 0x0117E280;        // 3228 bytes (name?)
constexpr u32 kPassArg = 0x010DDA78;
constexpr u32 kViewBlock = 0x010DD780;   // +256 3x4 rows, +612 f32
constexpr u32 kBank = 0x01193060;        // +36 -> {+20 u32}; +5264 / +5280 / +5312 f32[3] rows
constexpr u32 kGrid = 0x011945A0;        // u32, u16 +4; +16 / +272 passed by address
constexpr u32 kConfig = 0x010C5A20;
constexpr u32 kOutA = 0x0117E110, kOutB = 0x0117E180, kOutBInit = 0x0116E110;
constexpr u32 kRing = 0x01183080, kRingStart = 0x0117F020, kRingSize = 16384;
constexpr u32 kCounter = 0x0117E200;     // u32, atomic, cleared
constexpr u32 kLights = 0x01195120;      // 144-byte light copies (gfx12 light_copies)
constexpr u32 kLightFirst = 0x011957E2, kLightCount = 0x011957E4;  // u16
constexpr u32 kOrthoHalf = 0x005894F0;   // f32 -0.5, 0.5
constexpr u32 kLightBuf = 2048, kLightBlock = 128;
}  // namespace sw

}  // namespace gfx21
}  // namespace sly4::render
