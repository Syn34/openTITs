// World objects, part 4: child links, change notification, class swaps
// and cloning (the last WorldObj vtable slots), plus the helpers those use.
#pragma once

#include "obj3.h"

namespace sly4 {
namespace obj4 {

using namespace opentit;

namespace o4fn {
constexpr u32 PtrVector_Reserve = 0x00551718;    // (vector, capacity)
constexpr u32 WorldObj_ForEach = 0x0057F5D8;     // (obj, Opd* fn(node, arg) -> bool, arg, bool recurse) (name?)
constexpr u32 SlotEntry_Alloc = 0x0057A53C;      // (obj): gives a visible object a slot entry (name?)
constexpr u32 SlotEntry_Update = 0x00579C50;     // (SlotEntry*, obj) (name?)
constexpr u32 WorldObj_SetClassBase = 0x0057D8AC; // (obj, cls): the base +44 ("WorldObj_SetParent" in symbols.txt)
constexpr u32 Skeleton_Release = 0x005807D8;     // (ApeSkeleton*) ("WorldObj_FreeExtra300" in symbols.txt)
constexpr u32 Skeleton_Create = 0x005816F0;      // (ApeSkeletonDef*) -> ApeSkeleton* (name?)
constexpr u32 WorldClass_Copy = 0x005858D4;      // (cls) -> copy
constexpr u32 WorldObj_Instance = 0x005A05E8;    // (cls, bool copy_class) -> obj
constexpr u32 WorldObj_CopyFrom = 0x00581B34;    // (dst, src, bool own_class) (name?)
constexpr u32 WorldObj_AddToWorld = 0x005A0394;  // (obj) (name?)
constexpr u32 Box_Center = 0x00556F98;           // (const f32 box[8], f32 out[3]) -> out
constexpr u32 WorldObj_UpdateLod = 0x0057EE00;   // (obj, 0, 0) (name?)
constexpr u32 Box_Transform = 0x005570F0;        // (f32 out[8], const f32 in[8], const Matrix*) (name?)
constexpr u32 Box_Overlaps = 0x00557430;         // (const box*, const box*) -> bool (name?)
constexpr u32 Zone_Overlaps = 0x00576E28;        // (const zone shape*, const box*) -> bool (name?)
constexpr u32 Zones_MaskFor = 0x00577338;        // (zones, obj) -> u32 mask (name?)
constexpr u32 memset = 0x00726E54;
constexpr u32 memmove = 0x0071C9B4;              // the vectorised libc move (0x0071C3F0 is "memmove" in symbols.txt)
constexpr u32 Mem_Alloc = 0x0054ED38;            // (size, pool, align)
constexpr u32 Mem_FreePool = 0x0056C730;         // (p, pool, tag)
constexpr u32 FreeList_Init = 0x00551A58;        // (list, elem_size, count, heap, bool)
constexpr u32 SmallFree = 0x00552420;            // (p, size)
constexpr u32 Mem_GetDefaultStack = 0x005525FC;  // () -> ApeMemStack*
constexpr u32 MemStack_Alloc = 0x0055260C;       // (stack, size)
constexpr u32 Skeleton_DumpBone = 0x00583FBC;    // (skeleton, bone, s32 depth, u32* last_bits) (name?)
}  // namespace o4fn

// ---------------------------------------------------------------- bounds (class)
constexpr u32 kClassBoxMin = 16, kClassBoxMax = 32;  // f32[4] each
constexpr u32 kClassSphere = 48;                     // f32[3] centre, +60 radius
constexpr u32 kObjSphere = 224;                      // f32[4]: the class's sphere, copied
constexpr u32 kObjZoneMask = 324;                    // u32: one bit per overlapping zone (name?)
constexpr u32 kObjFlag168Static = 0x00800000;        // +168 bit: keep the zone mask (name?)
constexpr u32 kZones = 0x01193060 + 36;              // Zones*: +16 u8 count, +32 192-byte entries (name?)
constexpr u32 kZoneSize = 192;                       // +0 u8 bit, +2 u8 flags (1: precise test), +16 box, +48 shape, +176 s32
constexpr u32 kFltMax = 0x7F7FFFFF, kFltMinNeg = 0xFF7FFFFF;

// ---------------------------------------------------------------- peer records
constexpr u32 kPeerSize = 160;      // +128 / +132 / +136 owned blocks, +148 the owner object
constexpr u32 kPeerVecSize = 32;    // the parallel array at peers +4
constexpr u32 kPeerCount = 8;       // u16 in the peers header

// ---------------------------------------------------------------- free lists
constexpr u32 kSkeletonList = 0x011DE4E4;  // ApeTFreeList<ApeSkeleton> (64 bytes x 128)
constexpr u32 kSkeletonListVtbl = 0x00B24ED8;
constexpr u32 kStrSkeletonList = 0x00AF4D77;  // "static ApeTFreeList<T> &ApeTMemClassFL<T, size>::GetAllocator() [with T = ApeSkeleton, size = 128U]"
constexpr u32 kSkeletonWatch = 0x01194590;    // {ApeSkeleton** data, u32 count}: cleared on free when +31 bit 0x20
constexpr u32 kRootDestroyHook = 0x01164CD4;  // Opd*: called for objects with +8 bits 6-9 set (name?)
constexpr u32 kVtblRoot = 0x00B242B8;
constexpr u32 kVtblTexture = 0x00B23380;      // (name?)
constexpr u32 kTextureSerial = 0x010DD594;    // u32

// ---------------------------------------------------------------- links
constexpr u32 kObjRefs = 4;           // u16: reference count (high half of +4)
constexpr u32 kObjRoot = 256;         // obj*: topmost ancestor below a kind-19 object (itself when unparented)
constexpr u32 kObjParentLink = 260;   // obj*: the parent (obj.h's "kObjQueued")
constexpr u32 kObjChildTag = 264;     // u16: AddChild's third argument
constexpr u32 kObjNext = 268, kObjFirst = 272;
constexpr u32 kObjClassPtr = 208;     // WorldClass*
constexpr u32 kObjSkeleton = 300;     // ApeSkeleton*
constexpr u32 kObjRadiusSq = 312;     // f32
constexpr u32 kObjBits322 = 322;      // u8: 0x40 = the class's first topology has bit 0x80 at +54
constexpr u32 kObjPerView = 304;     // u32[]: one value per view (name?)
constexpr u32 kObjAnchor = 240;       // f32[4]: copied to the peer's vec4 on change (name?)
constexpr u32 kKindScene = 19;        // ancestors of this kind stop the root walk (name?)

constexpr u32 kSetRootOpd = 0x00F3C488;  // OPD of 0x0057F5CC: node->+256 = arg; return 1

// ---------------------------------------------------------------- vtable slots
constexpr u32 kVfnRelease = 8, kVfnGetName = 12, kVfnAddChild = 20, kVfnGetSkeleton = 28;
constexpr u32 kVfnChanged = 36, kVfnClone = 56;

// ---------------------------------------------------------------- change lists
constexpr u32 kSlotTableBase = 0x01193004;  // +0 SlotEntry*, +10 u8: objects use slot entries (name?)
constexpr u32 kChangedList = 0x0116BD08;    // {obj** data, u32 count, u32 capacity}
constexpr u32 kObjFlagVisible = 0x8000;     // +164 bit (SetHidden's bit)
constexpr u32 kObjFlagChanged = 0x200;      // +164: on kChangedList

// ---------------------------------------------------------------- Clone flags
constexpr u32 kCloneChildren = 1, kCloneOwnClass = 2, kCloneNoWorld = 8;

// ---------------------------------------------------------------- shared helpers
inline u32 Kind(u32 o) { return (load<u32>(o + obj2::kObjWord4) >> 10) & 0x3F; }

// A virtual call with no arguments: r4 / r5 = the OPD and entry, as the
// original call sequence leaves them.
inline u64 Vcall0(PPUContext& ctx, u32 o, u32 slot) {
    const u32 opd = load<u32>(load<u32>(o) + slot);
    return call_ptr(ctx, opd, {o, opd, load<u32>(opd)});
}

inline void Release(PPUContext& ctx, u32 o) { Vcall0(ctx, o, kVfnRelease); }
inline void AddRef(u32 o) { store<u16>(o + kObjRefs, static_cast<u16>(load<u16>(o + kObjRefs) + 1)); }

// The WorldObj_MarkDirty sequence the callers inline: unless o is dirty,
// marks its root dirty (queueing the root on the dirty list first when it
// is neither dirty nor queued, has no parent and queueing is on), then o.
inline void MarkDirtyInline(PPUContext& ctx, u32 o) {
    const u32 w = load<u32>(o + obj::kObjFlags);
    if ((w & obj::kObjDirty) != 0) return;
    const u32 p = load<u32>(o + kObjRoot);
    u32 pw = load<u32>(p + obj::kObjFlags);
    u32 ow = w;  // the root-already-dirty path keeps the first read
    if ((pw & obj::kObjDirty) == 0) {
        if ((pw & obj::kObjQueuedFlag) == 0 && load<u32>(p + kObjParentLink) == 0 && load<u8>(obj::kDirtyListOff) == 0) {
            const u32 v = load<u32>(obj::kDirtyList);
            if (load<u16>(v + 8) == load<u16>(v + 4))
                call(ctx, o4fn::PtrVector_Reserve, {v, (static_cast<u32>(load<u16>(v + 4)) << 1) & ~1u});
            store<u32>(load<u32>(v) + load<u16>(v + 8) * 4u, p);
            const u32 n = load<u16>(v + 8) + 1u;
            const u32 hi = load<u16>(v + 10);
            store<u16>(v + 8, static_cast<u16>(n));
            store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFF) ? hi : n));
            pw = load<u32>(p + obj::kObjFlags) | obj::kObjQueuedFlag;
        }
        store<u32>(p + obj::kObjFlags, pw | obj::kObjDirty);
        ow = load<u32>(o + obj::kObjFlags);
    }
    store<u32>(o + obj::kObjFlags, ow | obj::kObjDirty);
}

inline void CopyVec4(u32 dst, u32 src) {
    for (u32 k = 0; k < 16; k += 4) store<u32>(dst + k, load<u32>(src + k));
}

// The bounding radius the class gives: (class +60 + the last topology's
// f32)^2, as WorldObj_ClassChanged computes it.
inline void StoreRadius(u32 o, u32 cls) {
    const f64 a = static_cast<f64>(load<f32>(cls + 60));
    const u32 last = cls + ((static_cast<u32>(load<u8>(cls + obj2::kClassTopoCount)) << 3) & ~7u) + 124;
    const f64 r = static_cast<f64>(static_cast<f32>(static_cast<f64>(load<f32>(last + 4)) + a));
    store<f32>(o + kObjRadiusSq, static_cast<f32>(r * r));
}

// An object with a peer record (+164 high half) copies its +240 vec4 to
// the record's vec4 (the inlined tail of the Changed slots). Leaves r3 / f1
// as the original does.
void CopyAnchorToPeer(PPUContext& ctx, u32 o);

// WorldObj_Changed's body (the emitter's +36 is the same code).
void ChangedBody(PPUContext& ctx, u32 o);

}  // namespace obj4
}  // namespace sly4
