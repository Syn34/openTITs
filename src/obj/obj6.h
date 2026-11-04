// World objects, part 6: skeleton set-up and pose copies, the object
// collector AddToWorld uses, the base-state copy, box corners and the
// emitter-system binding.
#pragma once

#include "obj5.h"

namespace sly4 {
namespace obj6 {

using namespace opentit;

namespace o6fn {
constexpr u32 SmallAlloc = 0x0055238C;          // (size) -> block
constexpr u32 Skeleton_Layout = 0x0058142C;     // (ApeSkeleton*, block, size): carves the bone arrays (name?)
constexpr u32 Bone_InitEntry = 0x005812F0;      // (entry, bone def, local, world, vec3*) (name?)
constexpr u32 Skeleton_Compose = 0x00580EA0;    // (ApeSkeleton*, matrices +44): still recompiled (name?)
constexpr u32 ObjComps_Assign = 0x00558E24;     // (dst, src): ref-counted copy of +12 (name?)
constexpr u32 String_Assign = 0x00555F80;       // (String* dst, const String* src)
constexpr u32 Collector_Accepts = 0x005A9914;   // (obj, bool visible_only) -> bool (name?)
constexpr u32 Comps_Query = 0x00593DA8;         // (0, comps, 1) -> bool: still recompiled (name?)
constexpr u32 EmitPropsRef_Set = 0x0079CB94;    // (AglEmitProperties** slot, AglEmitProperties*) ref-counted (name?)
constexpr u32 EmitProps_Destruct = 0x004FFA98;  // (AglEmitProperties*) (name?)
constexpr u32 EmitProps_Construct = 0x004FFFF0; // (AglEmitProperties*) (name?)
constexpr u32 EmitProps_CopyFrom = 0x004FE3DC;  // (dst, src) (name?)
constexpr u32 EmitterDef_Construct = 0x0058DF94; // (ApeEmitterDef*) (name?)
constexpr u32 PtrTable_Grow = 0x007A78C4;       // (table, u32 by) (name?)
constexpr u32 Mem_Alloc = 0x0054ED38;           // (size, pool, align)
constexpr u32 FreeList_Init = 0x00551A58;       // (list, elem_size, count, heap, bool)
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 EmitProps_BlendKind = 0x0050061C; // (const u32 state[2]) -> 0 / 2 / 3 / 4 (name?)
constexpr u32 Iff_NextChunk = 0x00569108;       // (IffReader*) -> chunk header* or 0 (name?)
constexpr u32 SortedMap_Find = 0x007A46EC;      // (map, const u32* key) -> ChunkDesc** slot
constexpr u32 SystemState_CallHook = 0x005DBB60; // ()
constexpr u32 Matrix_ScaleRows = 0x00554394;    // (f32 rows[3][4], const f32 scale[3])
constexpr u32 WorldObj_UpdateKind8 = 0x00582F3C; // (obj): kinds 8 / 10 after a transform update (name?)
constexpr u32 Topology_Construct = 0x00508690;  // (AglTopology*) (name?)
constexpr u32 Topology_AddDraw = 0x00508B94;    // (AglTopology*, u32, AglMaterial*) (name?)
constexpr u32 Material_Alloc = 0x004FF2E8;      // () -> AglMaterial*
constexpr u32 Material_CopyFrom = 0x00509090;   // (dst, src)
constexpr u32 TopoShared_Release = 0x005084CC;  // (the +88 object) (name?)
constexpr u32 TopoRecord_Set = 0x0079C668;      // (record, its +0 from the source) (name?)
constexpr u32 Light_Accepts = 0x00591948;       // (LightInstance*, 0) -> bool (name?)
constexpr u32 LightAnim_Init = 0x00591820;      // (anim record, LightInstance*) (name?)
}  // namespace o6fn

// ---------------------------------------------------------------- ApeSkeleton layout (Skeleton_Layout)
constexpr u32 kSkelBlock = 20, kSkelBlockSize = 24;   // the SmallAlloc'd block and its size
constexpr u32 kSkelSerial = 30;                       // u8: the skeleton serial modulo the config's count
constexpr u32 kSkelFlags = 31;                        // u8: 0xE0 cleared by Skeleton_Init
constexpr u32 kSkelLocalArr = 36, kSkelWorldArr = 40; // 64-byte matrices, one per bone
constexpr u32 kSkelMatArr = 44;                       // 64-byte matrices (Skeleton_Compose's output)
constexpr u32 kSkelVecArr = 48;                       // 16-byte vec3s, set to (1, 1, 1)
constexpr u32 kSkelParents = 52;                      // s16 per bone (the defs' +100)
constexpr u32 kSkelEntryVec = 12;                     // entry +12: the bone's vec3*
constexpr u32 kSkeletonSerial = 0x01194594;           // u8: bumped by every Skeleton_Init (name?)
constexpr u32 kConfigSkelMod = 0x010C5A20 + 154;      // u8 in g::config: the serial's modulus (name?)
constexpr u32 kSkelDefBoneCount = 2;                  // u16
constexpr u32 kSkelDefBones = 4;                      // 112-byte bone defs: +0 the bind matrix (4 rows)
constexpr u32 kBoneDefSize = 112;
constexpr u32 kMatrixSize = 64;
constexpr u32 kPermBoneY = 0x005812D0, kPermBoneZ = 0x005812E0;  // Bone_InitEntry's vperm masks

// ---------------------------------------------------------------- WorldObj_CopyBase flags
constexpr u32 kBaseKeepClass = 0x01;   // don't SetClass(src class)
constexpr u32 kBaseKeepFlags = 0x02;   // don't copy the +164 / +168 bits
constexpr u32 kBaseKeepComps = 0x04;   // don't copy +12
constexpr u32 kBaseKeepMatrix = 0x08;  // don't copy +16..+152
constexpr u32 kBaseKeepName = 0x20;    // don't copy the name / hash
constexpr u32 kObjComps = 12;          // {u16 refs, ..., +4 4 component pointers (+8 a type hash)}* (name?)
constexpr u32 kObj160 = 160;
constexpr u32 kObj160Default = 0x01193010;  // stored at +160 with the matrix (name?)
constexpr u32 kObjLocal = 16;          // f32[4][4] rows: the local matrix
constexpr u32 kObjWorld = 80;          // f32[4][4] rows: the world matrix (WorldObj_UpdateTransforms)
constexpr u32 kObjOffsetMtx = kObj160; // Matrix*: applied to the local matrix first (default 0x01193010) (name?)

// ---------------------------------------------------------------- the collector (Collector_Accepts)
constexpr u32 kCompTypeA = 0x00F88948, kCompTypeB = 0x00F8894C, kCompTypeC = 0x00F88950;  // u32 type hashes
constexpr u32 kCompSlots = 4;
constexpr u32 kObjType320Skip = 1;     // +320 bits 9-12: type 1 is never collected

// ---------------------------------------------------------------- emitter systems
constexpr u32 kEsProps = 4;            // AglEmitProperties*
constexpr u32 kEsFlags = 64;           // u8: bit 0x02 = additive-ish blend (kind 3), 0x04 cleared
constexpr u32 kEpStatic = 68;          // u8 (props): nonzero = leave the flags
constexpr u32 kEpMaterial = 72;        // (props): +104 the blend state words

// ---------------------------------------------------------------- AglEmitProperties (140 bytes, u16 refs at +0)
constexpr u32 kEmitPropsList = 0x00F8F128;      // header*: +0 allocator, +4 capacity
constexpr u32 kEmitPropsListVtbl = 0x00B23E80;
constexpr u32 kStrEmitPropsList = 0x00AF1407;   // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglEmitProperties]"
constexpr u32 kEmitPropsSize = 140;
constexpr u32 kEpShape = 108;                   // object: u16 refs at +4, vfunc +12 clone, +4 destroy (name?)
constexpr u32 kEpCurve = 112;                   // 84-byte object (vtable 0x00B23200): u16 refs at +4 (name?)
constexpr u32 kSubRefs = 4;
constexpr u32 kVtblEmitCurve = 0x00B23200;
constexpr u32 kEmitCurveSize = 84;
constexpr u32 kCurveBig = 0x4CBEBC20;           // 1e8f at +12 / +16
constexpr u32 kVfnSubDestroy = 4, kVfnSubClone = 12;

// ---------------------------------------------------------------- ApeEmitterDef (128 bytes)
constexpr u32 kEmitterDefList = 0x00F8F138;     // header*: +0 allocator, +4 capacity
constexpr u32 kEmitterDefListVtbl = 0x00B24E78;
constexpr u32 kStrEmitterDefList = 0x00AF4A8E;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = ApeEmitterDef]"
constexpr u32 kEmitterDefSize = 128;
constexpr u32 kEdIndex = 122;                   // u16: the def's index in the table (0xFFFF when it was full)
constexpr u32 kEmitterDefTable = 0x01194570 + 12;  // PtrTable*: +4 {u16 cap, u16 count}*, +8 optional Obj**, +12 Obj** (name?)
constexpr u32 kTableGrowBy = 128;
constexpr u32 kVfnSystemRelease = 24;

// ---------------------------------------------------------------- IFF reading (Iff_ReadChunks)
constexpr u32 kChunkMap = 0x01164E30;          // map<u32 fourcc, ChunkDesc*>
constexpr u32 kChunkHandler = 4;               // ChunkDesc: the handler OPD
constexpr u32 kUnknownChunkHook = 0x00F8870C;  // OPD*: (reader, fourcc) for unregistered chunks (name?)
constexpr u32 kIrStack = 0, kIrStream = 4, kIrPos = 8;  // IffReader: chunk stack*, stream*, u32 position
constexpr u32 kIsTop = 4;                      // chunk stack: s32 top index (-1 = empty)
constexpr u32 kIsEntries = 8, kIsEntrySize = 12;  // {?, u32 start, u32 size}
constexpr u32 kIeStart = 4, kIeSize = 8;
constexpr u32 kVfnStreamSeek = 20;             // (stream, u32 pos, 0)
constexpr u32 kStreamPos = 264;                // u64

// ---------------------------------------------------------------- AglTopology (112 bytes, Topology_Copy)
constexpr u32 kTopologyList = 0x00F8F124;      // header*: +0 allocator, +4 capacity
constexpr u32 kTopologyListVtbl = 0x00B23E00;
constexpr u32 kStrTopologyList = 0x00AF12C9;   // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglTopology]"
constexpr u32 kTopologySize = 112;
constexpr u32 kTopoDraws = 48;                 // {u32, AglMaterial*}[]
constexpr u32 kTopoDrawCount = 52;             // u16
constexpr u32 kTopoFlags = 54;                 // u8: 0x80 / 0x40 / 0x20 copied
constexpr u32 kTopoShared = 88;                // {u16 refs, ...}*: released through 0x005084CC
constexpr u32 kTopoRecord = 92;                // 16-byte record*: +4 back to the topology (name?)
constexpr u32 kTopoRecordSize = 16;
constexpr u32 kCopyTableCount = obj2::kCopyTableCount;  // u32 at +1280: {src, copy} pairs before it

// ---------------------------------------------------------------- light animation records (LightInstance_Register)
// 80 bytes each at *(*0x01194EC0) (cls::kLightRecs), u16 count at +4.
constexpr u32 kLrCount = 4;
constexpr u32 kLaFlags = 12;      // u8: 0x80 cleared
constexpr u32 kLaPhase = 32;      // f32
constexpr u32 kLaPeriod = 36;     // f32
constexpr u32 kLaType = 40;       // s32: 0 sawtooth, 1 bounce (sine), 2 random, 3 clamp (name?)
constexpr u32 kLaDir = 44;        // u8: the bounce direction
constexpr u32 kLaNoise = 48;      // f32: noise amplitude
constexpr u32 kLaRandA = 52, kLaRandB = 56;  // f32: random values (type 2 blends them)
constexpr u32 kLaBase = 60, kLaAmp = 64;     // f32
constexpr u32 kLaOwner = 68;      // LightInstance*
constexpr u32 kLa25 = 0x41C80000, kLa625 = 0x441C4000, kLa400 = 0x43C80000;  // +0 / +4 / +8 defaults
constexpr u32 kLaConsts = 0x005A24E8;        // the function's constants:
constexpr u32 kLcInv65535 = 0, kLcZero = 4, kLcHalf = 8, kLcTwo = 12, kLcTwoPi = 16;
constexpr u32 kLcPiD = 24, kLcNegPiD = 32;   // f64 +-pi
constexpr u32 kLcHalfPi = 40, kLcPi = 44, kLc4OverPi = 48, kLcNeg4OverPi2 = 52, kLcSinP = 56, kLcOne = 60;
constexpr u32 kRandomLast = 0x01164888;      // u64: low 16 bits of the last mix
constexpr u32 kRandomState = 0x01164890;     // u32
constexpr u32 kRandomMix = 0x87654321;

// ---------------------------------------------------------------- shared helpers
// The inlined ApeTListAllocator<T>::GetAllocator(): the allocator behind
// the header at hdr_ea, made on first use (a 60-byte ApeTFreeList with
// `count` = header +4 entries). As in the original a failed Mem_Alloc
// stores a null allocator, which the caller then calls through. r8 / r9 =
// the vtable and the name in the original; FreeList_Init reads r3..r7 only.
inline u32 ListAllocator(PPUContext& ctx, u32 hdr_ea, u32 vtbl, u32 name, u32 elem_size) {
    const u32 hdr = load<u32>(hdr_ea);
    u32 alloc = load<u32>(hdr);
    if (alloc != 0) return alloc;
    const u32 obj = static_cast<u32>(call(ctx, o6fn::Mem_Alloc, {60, 0, 16}));
    if (obj != 0) {
        const u32 count = load<u32>(hdr + 4);
        for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u}) store<u32>(obj + off, 0);
        const u16 h54 = load<u16>(obj + 54);
        for (u32 off : {32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(obj + off, 0);
        store<u16>(obj + 52, 0);
        store<u16>(obj + 54, static_cast<u16>(h54 & 0x7FFF));
        store<u32>(obj, vtbl);
        store<u32>(obj + 56, name);
        call(ctx, o6fn::FreeList_Init, {obj, elem_size, count, 0, 0});
    }
    store<u32>(hdr, obj);
    return obj;
}

// alloc->Free(p) (slot +16): r5 / r6 = the OPD and entry.
inline void ListFree(PPUContext& ctx, u32 alloc, u32 p) {
    const u32 opd = load<u32>(load<u32>(alloc) + 16);
    call_ptr(ctx, opd, {alloc, p, opd, load<u32>(opd)});
}

}  // namespace obj6
}  // namespace sly4
