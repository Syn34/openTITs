// The object classes after the WorldObj vtable (0x00B24858) and the
// geometry definition objects instance from.
#pragma once

#include "obj4.h"

namespace sly4 {
namespace cls {

using namespace opentit;

namespace cfn {
constexpr u32 Box_SetPoint = 0x00556F60;      // (f32 box[8], const f32 p[3]): min = max = p
constexpr u32 Mem_FreeFrom = 0x0054EE6C;      // (block, 0)
constexpr u32 SkeletonDef_Release = 0x00584DFC;
}  // namespace cfn

// ---------------------------------------------------------------- ApeGeomDef ("WorldClass")
// 176 bytes from the free list at *0x00F8F12C, kind 20.
constexpr u32 kGeomDefList = 0x00F8F12C;      // header*: +0 allocator, +4 capacity
constexpr u32 kGeomDefListVtbl = 0x00B24E38;
constexpr u32 kStrGeomDefList = 0x00AF49A4;   // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = ApeGeomDef]"
constexpr u32 kGeomDefSize = 176;
constexpr u32 kKindGeomDef = 20;
constexpr u32 kVtblGeomBase = 0x00B246B0, kVtblGeomDef = 0x00B246D8;
constexpr u32 kOrigin = 0x01164B40;           // f32[3] zero point (name?)
constexpr u32 kDefaultSphere = 0x01164BC0;    // f32[4]
constexpr u32 kGeomShared = 108;              // {u16 refs, u16, void* data (allocated 16 bytes in)}*
constexpr u32 kGeomEpsilon = 0x3727C5AC;      // 1e-5f at +120


// ---------------------------------------------------------------- instances (shared)
namespace ifn {
constexpr u32 WorldObj_DestructBase = 0x00580978;
constexpr u32 WorldObj_Unlink = 0x0057FF98;
constexpr u32 WorldObj_ReleaseRes = 0x00558F34;
constexpr u32 DeferredDestroy = 0x005082F4;     // (obj, destroy opd)
constexpr u32 ApeObj_SetClass = 0x0057D8AC;
constexpr u32 WorldObj_CopyFrom = 0x00581B34;   // (dst, src, bool own_class) (name?)
constexpr u32 WorldObj_AddToWorld = 0x005A0394; // (obj) (name?)
constexpr u32 WorldObj_DumpTree = 0x00580094;   // (obj, s32 depth, flags)
constexpr u32 Skeleton_DumpTree = 0x00584160;   // (skeleton, s32 depth)
constexpr u32 WorldObj_SetBoundRadius = 0x0058049C;  // (obj, f32 radius in f1)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;
constexpr u32 memset = 0x00726E54;
constexpr u32 memmove = 0x0071C9B4;
}  // namespace ifn

constexpr u32 kVfnRemoveChild = 24;
constexpr u32 kVfnDump = 60;

// ---------------------------------------------------------------- ApeLightInstance (kind 9, vtable 0x00B248A8)
namespace lfn {
constexpr u32 LightInstance_New = 0x005A2BA8;      // (ApeLightDef*) -> instance (name?)
constexpr u32 LightInstance_Register = 0x005A2528; // (instance) (name?)
constexpr u32 LightInstance_Update = 0x005A31C8;   // (instance) (name?)
constexpr u32 LightSlot_Move = 0x005A2024;         // (u16 to, u16 from) (name?)
}  // namespace lfn
constexpr u32 kVtblLightInstance = 0x00B248A8;
constexpr u32 kLightDestroyOpd = 0x00F3C8D8;
constexpr u32 kLightSlotLists = 0x011945A0;   // +4 u16: end of the type-2 slots; +6 u16: start of the type-0/1 slots (name?)
constexpr u32 kLightRecs = 0x01194EC0;        // **: {80-byte records (+16 vec4, +68 owner), u16 count at +4}
constexpr u32 kLightRecSize = 80;
constexpr u32 kConfigLights = 0x010C5A20;     // g::config: +96 copied to +100 (modes 0/1) or +104 (others) (name?)
constexpr u32 kLightColor = 352;              // f32[3] (def +112)
constexpr u32 kLightType = 368;               // s32 (def +128): 2 = spot? (name?)
constexpr u32 kLight372 = 372;                // u32 (def +132)
constexpr u32 kLightRangeSq = 376;            // f32 (def +136 squared)
constexpr u32 kLight380 = 380;                // u8 (def +140)
constexpr u32 kLight384 = 384;                // f32[3] (def +144)
constexpr u32 kLightRec = 382;                // u16: index into kLightRecs, 0xFFFF = none
constexpr u32 kOneF = 0x3F800000;

// Clone's child loop: each child's clone (vfunc +56 with the same flags)
// added to n with tag 0xFFFF, then released.
inline void CloneChildren(PPUContext& ctx, u32 o, u32 n, u32 flags) {
    for (u32 c = load<u32>(o + obj4::kObjFirst); c != 0; c = load<u32>(c + obj4::kObjNext)) {
        u32 opd = load<u32>(load<u32>(c) + obj4::kVfnClone);
        const u32 cc = static_cast<u32>(call_ptr(ctx, opd, {c, flags, opd, load<u32>(opd)}));  // r5 / r6
        opd = load<u32>(load<u32>(n) + obj4::kVfnAddChild);
        call_ptr(ctx, opd, {n, cc, ~0ull, opd, load<u32>(opd)});  // tag -1; r6 / r7 = OPD and entry
        obj4::Release(ctx, cc);
    }
}

// Release's child loop: o->RemoveChild(c) (vfunc +24) for each child, the
// next one read first.
inline void RemoveChildren(PPUContext& ctx, u32 o) {
    for (u32 c = load<u32>(o + obj4::kObjFirst); c != 0;) {
        const u32 next = load<u32>(c + obj4::kObjNext);
        const u32 opd = load<u32>(load<u32>(o) + kVfnRemoveChild);
        call_ptr(ctx, opd, {o, c, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry
        c = next;
    }
}

// The Dump slot (+60) of the light / emitter / camera instances: depth <=
// 0 only fetches the name; otherwise the tree one level down (flags | 1)
// and, while depth remains, the skeleton (vfunc +28).
inline void DumpInstance(PPUContext& ctx, u32 o, u64 depth, u64 flags) {
    if (static_cast<s32>(depth) <= 0) {
        obj4::Vcall0(ctx, o, obj4::kVfnGetName);
        return;
    }
    const s64 d1 = static_cast<s32>(depth - 1);
    call(ctx, ifn::WorldObj_DumpTree, {o, static_cast<u64>(d1), flags | 1});
    if (d1 <= 0) return;
    if (static_cast<u32>(obj4::Vcall0(ctx, o, obj4::kVfnGetSkeleton)) == 0) return;
    const u32 sk = static_cast<u32>(obj4::Vcall0(ctx, o, obj4::kVfnGetSkeleton));
    // r5 = the entry address of that vcall here (Skeleton_DumpTree reads r3 / r4 only).
    call(ctx, ifn::Skeleton_DumpTree, {sk, static_cast<u64>(static_cast<s64>(static_cast<s32>(d1 - 1)))});
}

// Removes record `idx` from a packed record array {data, u16 count at
// count_off} whose records keep their owner at owner_off and owners keep
// their index (u16) at back_off: the owner gets 0xFFFF, later records move
// down one (their owners renumbered), or the last one is zeroed; then the
// count drops.
inline void RemoveRecord(PPUContext& ctx, u32 arr, u32 idx, u32 size, u32 count_off, u32 owner_off, u32 back_off) {
    const u32 last32 = load<u16>(arr + count_off) - 1u;
    const u32 last = last32 & 0xFFFF;
    const u32 rec = load<u32>(arr) + idx * size;
    store<u16>(load<u32>(rec + owner_off) + back_off, 0xFFFF);
    if (((idx ^ last32) & 0xFFFF) == 0) {
        call(ctx, ifn::memset, {rec, 0, size});
    } else {
        call(ctx, ifn::memmove, {rec, rec + size, (last - idx) * size});
        if (static_cast<s32>(idx) < static_cast<s32>(last)) {
            const u32 base = load<u32>(arr);
            for (u32 i = idx; static_cast<s32>(i) < static_cast<s32>(last); i = (i + 1) & 0xFFFF)
                store<u16>(load<u32>(base + i * size + owner_off) + back_off, static_cast<u16>(i));
        }
    }
    store<u16>(arr + count_off, static_cast<u16>(load<u16>(arr + count_off) - 1));
}

}  // namespace cls
}  // namespace sly4
