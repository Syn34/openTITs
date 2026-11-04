// World objects, part 3: link / destruct helpers and the remaining slots
// of the WorldObj vtable (0x00B24858).
#include "obj3.h"

using namespace sly4;
using namespace sly4::obj;
using namespace sly4::obj2;
using namespace sly4::obj3;

namespace {

inline u32 Kind(u32 o) { return (load<u32>(o + kObjWord4) >> 10) & 0x3F; }

u64 Vcall(PPUContext& ctx, u32 o, u32 slot) {
    const u32 opd = load<u32>(load<u32>(o) + slot);
    return call_ptr(ctx, opd, {o, opd, load<u32>(opd)});  // r4 / r5 = OPD and entry
}

}  // namespace

// void WorldObj_InitLinks(Obj* o)
// The tail of the base constructor: clears the extra / class pointers and
// links, slot 0xFFFF, +296 = -1, white colour, +328 = 2, and the +320
// bitfields (top byte and bits 18-23 cleared, +322 bit 0x80 set, 0x40 and
// bits 9-12 cleared). The byte stores are overwritten by the final word,
// as in the original.
DECOMP(00580658, WorldObj_InitLinks) {
    const u32 o = arg<u32>(ctx, 0);
    u32 r7 = load<u32>(o + kObjGroupWord);
    r7 &= ~0xFF000000u;
    r7 &= ~0x00FC0000u;
    store<u8>(o + kObjGroupWord, 0);
    store<u32>(o + kObjExtra288, 0);
    u32 r8 = (r7 >> 16) & 0xFE;
    u32 r9 = (r7 >> 8) & 0xFF;
    store<u16>(o + kObjWord284, 0xFFFF);
    r8 &= ~2u;
    store<u32>(o + 292, 0);
    u32 r6 = r7;
    store<u32>(o + 296, 0xFFFFFFFFu);
    r9 |= 0x80;
    store<u32>(o + kObjGroupWord, r7);
    r6 = (r6 & ~0x00FF0000u) | ((r8 << 16) & 0x00FF0000u);
    store<u32>(o + 300, 0);
    r9 &= ~0x40u;
    store<u32>(o + 304, 0);
    store<u32>(o + 308, 0);
    r6 = (r6 & ~0x0000FF00u) | ((r9 << 8) & 0x0000FF00u);
    store<u8>(o + kObjGroupWord + 1, static_cast<u8>(r8));
    store<u8>(o + kObjGroupWord + 2, static_cast<u8>(r9));
    r6 &= ~0x00001E00u;
    store<u32>(o + kObjGroupWord, r6);
    for (u32 c = 0; c < 4; ++c) store<u8>(o + kObjColor + c, load<u8>(0x01164CA0 + c));
    store<u32>(o + 324, 0);
    store<u16>(o + kObjU16_328, 2);
    store<u32>(o + 332, 0);
    store<u32>(o + 336, 0);
}

// u32 SlotEntry_UpdatePeer(u16 slot)
// For the object in slot `slot`: when it has a peer record (+164 !=
// 0xFFFF), sets the record's +144 to the object's own slot entry if it is
// a kind-8 / kind-10 object with a slot, else 0. Returns 0 (0xFFFF when
// there is no record), as the original leaves r3.
DECOMP(0057A46C, SlotEntry_UpdatePeer) {
    const u32 slot = arg<u32>(ctx, 0);
    const u32 table = load<u32>(kSlotTable);
    const u32 o = load<u32>(table + ((slot << 7) & 0xFFFFFF80u) + 96);
    const u32 idx = load<u16>(o + 164);
    if (idx == 0xFFFF) return ret(ctx, 0xFFFFu);
    const u32 rec = load<u32>(load<u32>(load<u32>(kSlotPeers))) + idx * 160;
    const u32 kind = Kind(o);
    u32 v = 0;
    if (kind == 10 || kind == 8) {
        const u32 s = load<u16>(o + kObjWord284);
        if (s != 0xFFFF) v = table + ((s << 7) & 0xFFFFFF80u);
    }
    store<u32>(rec + 144, v);
    ret(ctx, 0u);
}

// void WorldObj_Unlink(Obj* o)
// A kind-8 / kind-10 object flagged as listed (+321 bit 0x02) is removed
// from the kind list (the last entry moves into its place); a kind-10
// object then clears its class's +120.
DECOMP(0057FF98, WorldObj_Unlink) {
    const u32 o = arg<u32>(ctx, 0);
    u32 kind = Kind(o);
    if (static_cast<s32>(kind) < 8 || static_cast<s32>(kind) > 14) return;
    if (((load<u8>(o + kObjByte321) >> 1) & 1) != 1) return;
    kind = Kind(o);
    if (kind != 10 && kind != 8) return;
    const u32 n = load<u16>(kKindList + 8);
    if (n == 0) return;
    s32 i = static_cast<s32>(n) - 1;
    while (load<u32>(load<u32>(kKindList) + static_cast<u32>(i) * 4) != o) {
        if (i == 0) return;
        --i;
    }
    const u32 count = load<u16>(kKindList + 8) - 1;
    store<u16>(kKindList + 8, static_cast<u16>(count));
    if (i != static_cast<s32>(count & 0xFFFF)) {
        const u32 data = load<u32>(kKindList);
        store<u32>(data + static_cast<u32>(i) * 4, load<u32>(data + ((count << 2) & 0x3FFFC)));
    }
    if (Kind(o) != 10) return;
    const u32 cls = load<u32>(o + 208);
    store<u8>(cls + 120, 0);
    ret(ctx, cls);
}

// void WorldObj_DestructBase(Obj* o)
// Releases the +288 / +300 extras, drops the object from the dirty list
// if it is queued (0x400), then detaches it and runs the root destructor.
DECOMP(00580978, WorldObj_DestructBase) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 extra = load<u32>(o + kObjExtra288);
    store<u32>(o, kVtblBase2);
    if (extra != 0) {
        call(ctx, o3fn::Texture_ReleaseRef, {extra});
        store<u32>(o + kObjExtra288, 0);
    }
    const u32 extra2 = load<u32>(o + kObjExtra300);
    if (extra2 != 0) {
        call(ctx, o3fn::WorldObj_FreeExtra300, {extra2});
        store<u32>(o + kObjExtra300, 0);
    }
    store<u32>(o, kVtblBase1);
    if ((load<u32>(o + kObjFlags164) & kObjQueuedFlag) != 0) {
        const u32 v = load<u32>(kDirtyList);
        const u32 n = load<u16>(v + 8);
        const u32 last = n - 1;
        if (n != 0) {
            for (s32 i = static_cast<s32>(last);; --i) {
                const u32 data = load<u32>(v);
                if (load<u32>(data + static_cast<u32>(i) * 4) == o) {
                    store<u16>(v + 8, static_cast<u16>(last));
                    if (i != static_cast<s32>(last & 0xFFFF))
                        store<u32>(data + static_cast<u32>(i) * 4, load<u32>(data + ((last << 2) & 0x3FFFC)));
                    store<u32>(o + kObjFlags164, load<u32>(o + kObjFlags164) & ~kObjQueuedFlag);
                    break;
                }
                if (i == 0) break;
            }
        }
    }
    call(ctx, o3fn::WorldObj_DetachAll, {o});
    call(ctx, o3fn::WorldObj_SetParent, {o, 0});
    call(ctx, o3fn::WorldObj_DestructRoot, {o});
}

// void WorldObj_SetHidden(Obj* o, u8 hidden, bool recurse)  (slot +40) (name?)
// Unless the object's group (+320 bits 9-12) is 4, sets bit 0x8000 of +164
// from bit 0 of `hidden`; optionally the same for the children (their
// +40); vfunc +36 when the stored bit differed from `hidden`.
DECOMP(00583CC8, WorldObj_SetHidden) {
    const u32 o = arg<u32>(ctx, 0), hidden_arg = arg<u32>(ctx, 1);
    const u32 a = hidden_arg & 0xFF, b = arg<u32>(ctx, 2) & 0xFF;
    const u32 w = load<u32>(o + kObjGroupWord);
    const u32 first = load<u32>(o + kObjFirstChild);
    bool changed = false;
    if (((w >> 9) & 0xF) != 4) {
        u32 f = load<u32>(o + kObjFlags164);
        const u32 old = (f >> 15) & 1;
        f = (f & ~0x8000u) | ((hidden_arg << 15) & 0x8000u);
        changed = old != a;
        store<u32>(o + kObjFlags164, f);
    }
    if (b != 0)
        for (u32 c = first; c != 0; c = load<u32>(c + kObjNextSibling)) {
            const u32 opd = load<u32>(load<u32>(c) + 40);
            call_ptr(ctx, opd, {c, a, b, opd, load<u32>(opd)});  // r6 / r7 = OPD and entry
        }
    if (changed) Vcall(ctx, o, kObjVfnChanged);
}

// void WorldObj_Refresh(Obj* o, s32 depth, u32 flags)  (slot +60) (name?)
// depth <= 0: vfunc +12 of the object and of its class (results unused).
// Otherwise refreshes itself one level down (flags | 1) and, while depth
// remains, the tree of the +28 object.
DECOMP(005A0BCC, WorldObj_Refresh) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 depth = arg<s32>(ctx, 1);
    const u64 flags = ctx.gpr[5];
    if (depth <= 0) {
        Vcall(ctx, o, 12);
        Vcall(ctx, load<u32>(o + 208), 12);
        return;
    }
    const s32 d1 = depth - 1;
    call(ctx, o3fn::WorldObj_RefreshSelf, {o, static_cast<u64>(static_cast<s64>(d1)), flags | 1});
    if (d1 <= 0) return;
    if (static_cast<u32>(Vcall(ctx, o, 28)) == 0) return;
    const u32 opd = load<u32>(load<u32>(o) + 28);
    const u32 r = static_cast<u32>(call_ptr(ctx, opd, {o, opd, load<u32>(opd)}));
    // r5 = the entry address of that vcall here in the original (not read).
    call(ctx, o3fn::WorldObj_RefreshTree, {r, static_cast<u64>(static_cast<s64>(d1 - 1)), load<u32>(opd)});
}

namespace {

// The ApeTFreeList header every lazily made free list starts with (as in
// Texture_Alloc / Light_Alloc), then FreeList_Init(list, size, count, 0, 0).
u32 MakeFreeList(PPUContext& ctx, u32 vtbl, u32 name, u32 elem_size, u32 count) {
    const u32 obj = static_cast<u32>(call(ctx, o2fn::Mem_Alloc, {60, 0, 16}));
    if (obj == 0) return 0;
    for (u32 off : {8u, 4u, 12u, 20u, 16u}) store<u32>(obj + off, 0);
    const u16 h54 = load<u16>(obj + 54);
    for (u32 off : {24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(obj + off, 0);
    store<u16>(obj + 52, 0);
    store<u16>(obj + 54, static_cast<u16>(h54 & 0x7FFF));
    store<u32>(obj, vtbl);
    store<u32>(obj + 56, name);
    // r8 / r9 = the vtable and the name here in the original; FreeList_Init reads r3..r7 only.
    call(ctx, o2fn::FreeList_Init, {obj, elem_size, count, 0, 0});
    return obj;
}

constexpr u32 kSkelDefList = 0x011DE4E0;     // ApeTFreeList<ApeSkeletonDef>* (64 x 8 bytes)
constexpr u32 kSkelDefListVtbl = 0x00B24E18;
constexpr u32 kStrSkelDefList = 0x00AF493E;  // "static ApeTFreeList<T> &ApeTMemClassFL<T, size>::GetAllocator() [with T = ApeSkeletonDef, size = 64U]"
constexpr u32 kDeferState = 0x010DDC88;      // +0 free-list header*, +4 u32 frame, +8 u8 defer
constexpr u32 kDeferListVtbl = 0x00B23EA0;
constexpr u32 kStrDeferList = 0x00AF145A;    // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglDeferredDestructionObj]"

}  // namespace

// void SkeletonDef_Release(SkeletonDef* d)  (the "WorldRes_Release" guess)
// Drops a reference (u16 at +0); on the last one frees its +4 block and
// gives it back to its free list (made on first use). As in the original,
// the count is decremented before the null check.
DECOMP(00584DFC, SkeletonDef_Release) {
    const u32 d = arg<u32>(ctx, 0);
    const u16 refs = static_cast<u16>(load<u16>(d) - 1);
    store<u16>(d, refs);
    if (static_cast<s16>(refs) > 0 || d == 0) return;
    const u32 block = load<u32>(d + 4);
    if (block != 0) {
        call(ctx, o2fn::Mem_FreeFrom, {block, 0});
        store<u32>(d + 4, 0);
    }
    u32 alloc = load<u32>(kSkelDefList);
    if (alloc == 0) {
        alloc = MakeFreeList(ctx, kSkelDefListVtbl, kStrSkelDefList, 8, 64);
        store<u32>(kSkelDefList, alloc);  // 0 if Mem_Alloc failed: the original then calls through 0
    }
    const u32 opd = load<u32>(load<u32>(alloc) + 16);
    call_ptr(ctx, opd, {alloc, d, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry
}

// void DeferredDestroy(void* obj, const Opd* destroy)
// Calls destroy(obj) now, or, while destruction is deferred (+8 of
// 0x010DDC88), queues {obj, frame + 2, destroy} on the deferred list.
DECOMP(005082F4, DeferredDestroy) {
    const u32 obj = arg<u32>(ctx, 0), destroy = arg<u32>(ctx, 1);
    if (load<u8>(kDeferState + 8) == 0) {
        const u32 entry = load<u32>(destroy);
        call_ptr(ctx, destroy, {obj, entry});  // r4 = the entry address, as in the original
        return;
    }
    const u32 header = load<u32>(kDeferState);
    const u32 due = load<u32>(kDeferState + 4) + 2;
    u32 alloc = load<u32>(header);
    if (alloc == 0) {
        alloc = MakeFreeList(ctx, kDeferListVtbl, kStrDeferList, 12, load<u32>(header + 4));
        store<u32>(header, alloc);
    }
    const u32 n = static_cast<u32>(Vcall(ctx, alloc, 12));
    if (n == 0) return;
    store<u32>(n, obj);
    store<u32>(n + 4, due);
    store<u32>(n + 8, destroy);
}

// u32 SlotEntry_Move(u16 to, u16 from)
// Slot entries are 128 bytes at *0x01193004 (+96 = the object, which
// keeps its slot in +284). Detaches the object in `to` (+284 = 0xFFFF);
// when from == to the entry is reset (identity transform, 0x01164BC0 at
// +64, +96 = 0xFEEDF00D), otherwise `from` is copied over it and its
// object told its new slot. r3 / f1 are left as the original leaves them.
DECOMP(0057A1D8, SlotEntry_Move) {
    const u32 to = arg<u32>(ctx, 0), from = arg<u32>(ctx, 1);
    const u32 off = (to << 7) & 0x007FFF80u;
    const u32 dst = load<u32>(kSlotTable) + off;
    store<u16>(load<u32>(dst + 96) + kObjWord284, 0xFFFF);
    if (((to ^ from) & 0xFFFF) == 0) {
        constexpr u32 kIdentity = 0x01164BD0, kResetVec4 = 0x01164BC0;
        for (u32 r = 0; r < 4; ++r)
            for (u32 c = 0; c < 3; ++c) store<u32>(dst + 16 * r + 4 * c, load<u32>(kIdentity + 16 * r + 4 * c));
        for (u32 c = 0; c < 4; ++c) store<u32>(dst + 64 + 4 * c, load<u32>(kResetVec4 + 4 * c));
        store<u32>(dst + 92, 0);
        store<u32>(dst + 96, 0xFEEDF00Du);
        store<u32>(dst + 100, 0);
        store<u16>(dst + 124, 0);
        store<u32>(dst + 12, 0);
        store<u32>(dst + 28, 0);
        store<u32>(dst + 44, 0);
        ctx.fpr[1] = static_cast<f64>(load<f32>(kResetVec4 + 12));
        return ret(ctx, 0xFEEDF00Du);
    }
    const u32 src = load<u32>(kSlotTable) + ((from << 7) & 0x007FFF80u);
    // lvx / stvx: the first 64 bytes move as 16-byte aligned blocks.
    for (u32 k = 0; k < 64; k += 4) store<u32>((dst & ~15u) + k, load<u32>((src & ~15u) + k));
    for (u32 k = 64; k < 108; k += 4) store<u32>(dst + k, load<u32>(src + k));
    store<u16>(dst + 108, load<u16>(src + 108));
    store<u16>(dst + 110, load<u16>(src + 110));
    for (u32 k = 112; k < 124; k += 4) store<u32>(dst + k, load<u32>(src + k));
    store<u16>(dst + 124, load<u16>(src + 124));
    store<u8>(dst + 126, load<u8>(src + 126));
    store<u8>(dst + 127, load<u8>(src + 127));
    ctx.fpr[1] = static_cast<f64>(load<f32>(src + 120));
    const u32 o = load<u32>(load<u32>(kSlotTable) + off + 96);
    store<u16>(o + kObjWord284, static_cast<u16>(to));
    ret(ctx, o);
}

// u32 WorldObj_InitTransform(Obj* o)
// Identity local (+16) and world (+80) matrices, unit scale, +164 = {u16
// 0xFFFF, 0x8000 set, 0x7F00 cleared}, +168 top byte 0xFF and next byte
// cleared, +160 = 0x01193010, +240 = 0x01164BC0, parent = itself, cleared
// links, and the name "ApeObj::Unknown" with its hash. Returns the hash.
DECOMP(0057D688, WorldObj_InitTransform) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kIdentity = 0x01164BD0, kVec4 = 0x01164BC0, kStrUnknown = 0x00AF4EBC;
    for (u32 base : {16u, 80u})
        for (u32 r = 0; r < 4; ++r)
            for (u32 c = 0; c < 3; ++c) store<u32>(o + base + 16 * r + 4 * c, load<u32>(kIdentity + 16 * r + 4 * c));
    u32 f = load<u32>(o + kObjFlags164);
    f = (f & 0x0000FFFFu) | 0xFFFF0000u;
    f &= ~0x4000u;
    f |= 0x8000u;
    f &= ~(0x0800u | 0x0400u | 0x0200u | 0x2000u | 0x1000u | 0x0100u);
    for (u32 c = 0; c < 3; ++c) store<u32>(o + kObjScale + 4 * c, load<u32>(kOneVec3 + 4 * c));
    const u32 w168 = (load<u32>(o + 168) & 0x0000FFFFu) | 0xFF000000u;
    store<u16>(o + kObjFlags164, 0xFFFF);
    store<u8>(o + 168, 0xFF);
    store<u32>(o + kObjFlags164, f);
    store<u32>(o + 160, 0x01193010u);
    store<u32>(o + kObjNameHash, 0);
    store<u32>(o + 168, w168);
    store<u32>(o + 208, 0);
    for (u32 c = 0; c < 4; ++c) store<u32>(o + 240 + 4 * c, load<u32>(kVec4 + 4 * c));
    store<u32>(o + kObjParent, o);
    store<u32>(o + kObjFirstChild, 0);
    store<u32>(o + kObjNextSibling, 0);
    store<u32>(o + kObjQueued, 0);
    store<u16>(o + 264, 0);
    call(ctx, ofn::String_Copy, {o + kObjName, kStrUnknown, 32});
    const u32 h = static_cast<u32>(call(ctx, ofn::String_HashNoCase, {kStrUnknown}));
    if (load<u32>(o + kObjNameHash) != h) store<u32>(o + kObjNameHash, h);
    ret(ctx, h);
}
