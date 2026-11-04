// World objects, part 4: the WorldObj vtable's (0x00B24858) child links
// (+20 / +24), change notification (+36), class swap (+44) and clone (+56),
// and the base class swap they build on.
#include "obj4.h"

using namespace sly4;
using namespace sly4::obj;
using namespace sly4::obj4;

// void WorldObj_AddChild(Obj* o, Obj* child, u16 tag)  (slot +20)
// Unless either object is dead (refs <= 0) or the child already has a
// parent: links the child in front of o's children (the reference-counted
// links are released / added as the original does), marks o dirty and
// gives the child's subtree the new root: the topmost ancestor of o below
// a kind-19 object. r3 is left as the original leaves it on early exits.
DECOMP(0057F794, WorldObj_AddChild) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), tag = arg<u32>(ctx, 2);
    const s32 orefs = static_cast<s16>(load<u16>(o + kObjRefs));
    if (orefs <= 0) return ret(ctx, orefs);
    const s32 crefs = static_cast<s16>(load<u16>(c + kObjRefs));
    if (crefs <= 0) return ret(ctx, crefs);
    const u32 cur = load<u32>(c + kObjParentLink);
    if (cur != 0) return ret(ctx, cur);
    store<u32>(c + kObjParentLink, o);
    const u32 old_next = load<u32>(c + kObjNext);
    store<u16>(c + kObjChildTag, static_cast<u16>(tag));
    const u32 first = load<u32>(o + kObjFirst);
    if (old_next != 0) Release(ctx, old_next);
    store<u32>(c + kObjNext, first);
    if (first != 0) AddRef(first);
    const u32 old_first = load<u32>(o + kObjFirst);
    if (old_first != 0) Release(ctx, old_first);
    store<u32>(o + kObjFirst, c);
    AddRef(c);  // c != 0: its refs were read above
    MarkDirtyInline(ctx, o);
    u32 top = o;
    for (u32 p; (p = load<u32>(top + kObjParentLink)) != 0 && Kind(p) != kKindScene;) top = p;
    call(ctx, o4fn::WorldObj_ForEach, {c, kSetRootOpd, top, 1});
}

// void WorldObj_RemoveChild(Obj* o, Obj* child)  (slot +24)
// Unlinks a child of o (the links are released / added as the original
// does), clears its parent and tag, drops o's reference to it and makes it
// the root of its own subtree. Not o's child: only the two names are
// fetched (vfunc +12, an assert message in the original build).
DECOMP(0057F9C0, WorldObj_RemoveChild) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    if (load<u32>(c + kObjParentLink) != o) {
        Vcall0(ctx, c, kVfnGetName);
        Vcall0(ctx, o, kVfnGetName);
        return;
    }
    // The shared tail: clear the links and drop the parent's reference.
    auto detach = [&] {
        const u32 next = load<u32>(c + kObjNext);
        store<u32>(c + kObjParentLink, 0);
        store<u16>(c + kObjChildTag, 0);
        if (next != 0) Release(ctx, next);
        store<u32>(c + kObjNext, 0);
        Release(ctx, c);
    };
    const u32 first = load<u32>(o + kObjFirst);
    if (first == c) {
        const u16 refs = load<u16>(c + kObjRefs);
        const u32 cur = load<u32>(o + kObjFirst);
        const u32 next = load<u32>(c + kObjNext);
        store<u16>(c + kObjRefs, static_cast<u16>(refs + 1));
        if (cur != 0) Release(ctx, cur);
        store<u32>(o + kObjFirst, next);
        if (next != 0) AddRef(next);
        detach();
    } else if (first != 0) {
        const u16 refs = load<u16>(c + kObjRefs);
        u32 prev = first;
        while (load<u32>(prev + kObjNext) != c) {
            prev = load<u32>(prev + kObjNext);
            if (prev == 0) break;
        }
        if (prev != 0) {
            store<u16>(c + kObjRefs, static_cast<u16>(refs + 1));
            const u32 cur = load<u32>(prev + kObjNext);
            const u32 next = load<u32>(c + kObjNext);
            if (cur != 0) Release(ctx, cur);
            store<u32>(prev + kObjNext, next);
            if (next != 0) AddRef(next);
            detach();
        }
    }
    call(ctx, o4fn::WorldObj_ForEach, {c, kSetRootOpd, c, 1});
}

// void WorldObj_Changed(Obj* o)  (slot +36)
// With slot entries on (0x0119300E): gives a visible object without one a
// slot entry and updates its entry. Otherwise puts the object on the
// changed list (0x0116BD08, 0x200 in +164) if there is room. Either way an
// object with a peer record copies its +240 vec4 to the record's vec4.
DECOMP(005A0CD4, WorldObj_Changed) { ChangedBody(ctx, arg<u32>(ctx, 0)); }

void sly4::obj4::ChangedBody(PPUContext& ctx, u32 o) {
    if (load<u8>(kSlotTableBase + 10) != 0) {
        u32 slot = load<u16>(o + obj2::kObjWord284);
        if (slot == 0xFFFF && (load<u32>(o + kObjFlags) & kObjFlagVisible) != 0) {
            call(ctx, o4fn::SlotEntry_Alloc, {o});
            slot = load<u16>(o + obj2::kObjWord284);
        }
        if (slot != 0xFFFF) {
            const u32 entry = load<u32>(kSlotTableBase) + ((slot << 7) & 0xFFFFFF80u);
            call(ctx, o4fn::SlotEntry_Update, {entry, o});
        }
    } else {
        const u32 f = load<u32>(o + kObjFlags);
        if ((f & kObjFlagChanged) != 0) {
            store<u32>(o + kObjFlags, f | kObjFlagChanged);
        } else {
            const u32 n = load<u32>(kChangedList + 4);
            if (n < load<u32>(kChangedList + 8)) {
                const u32 data = load<u32>(kChangedList);
                store<u32>(kChangedList + 4, n + 1);
                store<u32>(data + ((n << 2) & 0xFFFFFFFCu), o);
                store<u32>(o + kObjFlags, load<u32>(o + kObjFlags) | kObjFlagChanged);
            }
        }
    }
    CopyAnchorToPeer(ctx, o);
}

void sly4::obj4::CopyAnchorToPeer(PPUContext& ctx, u32 o) {
    const u32 idx = load<u16>(o + kObjFlags);
    if (idx == 0xFFFF) return;
    const u32 peers = load<u32>(load<u32>(obj3::kSlotPeers));
    const u32 base = load<u32>(peers);
    const u32 rec = base + idx * kPeerSize;
    // The record's index, recomputed as (rec - base) / 160 (signed).
    const s32 i = static_cast<s32>(rec - load<u32>(peers)) / static_cast<s32>(kPeerSize);
    const u32 dst = load<u32>(peers + 4) + (static_cast<u32>(i) << 5);
    CopyVec4(dst, o + kObjAnchor);
    ctx.fpr[1] = static_cast<f64>(load<f32>(o + kObjAnchor + 12));  // lfs / stfs left f1
    ret(ctx, dst);
}

// void ApeObj_SetClass(Obj* o, WorldClass* cls)  (the base +44; "WorldObj_SetParent" in symbols.txt)
// Releases the old class and takes a reference to the new one, copying its
// +48 vec4 to +224 and marking the object dirty (no class: the vec4 at
// 0x01164BC0, not dirty), then vfunc +36.
DECOMP(0057D8AC, ApeObj_SetClass) {
    const u32 o = arg<u32>(ctx, 0), cls = arg<u32>(ctx, 1);
    const u32 old = load<u32>(o + kObjClassPtr);
    if (old != 0) Release(ctx, old);
    store<u32>(o + kObjClassPtr, cls);
    if (cls != 0) {
        AddRef(cls);
        CopyVec4(o + 224, cls + 48);
        MarkDirtyInline(ctx, o);
    } else {
        CopyVec4(o + 224, 0x01164BC0);
    }
    Vcall0(ctx, o, kVfnChanged);
}

// void WorldObj_SetClass(Obj* o, WorldClass* cls)  (slot +44)
// The base swap, then: with a skeleton definition (class +164) and no
// skeleton yet, a new skeleton (the old one released) and vfunc +36; the
// +322 bit 0x40 from the first topology's +54 bit 0x80; the bounding
// radius; vfunc +36. No class: radius 0, bit cleared, vfunc +36.
DECOMP(005A0A28, WorldObj_SetClass) {
    const u32 o = arg<u32>(ctx, 0), cls_arg = arg<u32>(ctx, 1);
    if (cls_arg == load<u32>(o + kObjClassPtr)) return;
    call(ctx, o4fn::WorldObj_SetClassBase, {o, cls_arg});
    const u32 cls = load<u32>(o + kObjClassPtr);
    if (cls == 0) {
        store<u32>(o + kObjRadiusSq, 0);
        store<u8>(o + kObjBits322, static_cast<u8>(load<u8>(o + kObjBits322) & ~0x40u));
        Vcall0(ctx, o, kVfnChanged);
        return;
    }
    if (load<u32>(cls + obj2::kClassRes) != 0 && static_cast<u32>(Vcall0(ctx, o, kVfnGetSkeleton)) == 0) {
        const u32 sk = static_cast<u32>(call(ctx, o4fn::Skeleton_Create, {load<u32>(cls + obj2::kClassRes)}));
        const u32 old = load<u32>(o + kObjSkeleton);
        if (old != sk) {
            if (old != 0) call(ctx, o4fn::Skeleton_Release, {old});
            store<u32>(o + kObjSkeleton, sk);
            Vcall0(ctx, o, kVfnChanged);
        }
    }
    const u32 topo = load<u32>(cls + obj2::kClassTopos);
    const u32 bit = (load<u8>(topo + 54) >> 7) & 1;
    store<u8>(o + kObjBits322, static_cast<u8>((load<u8>(o + kObjBits322) & ~0x40u) | (bit << 6)));
    StoreRadius(o, load<u32>(o + kObjClassPtr));
    Vcall0(ctx, o, kVfnChanged);
}

// Obj* WorldObj_Clone(const Obj* o, u32 flags)  (slot +56)
// A new instance of o's class (of a copy of it with kCloneOwnClass) with
// o's state copied; kCloneChildren clones the children too (each clone
// added with tag 0xFFFF). Unless o has a parent or kCloneNoWorld is set,
// the clone is added to the world.
DECOMP(005A089C, WorldObj_Clone) {
    const u32 o = arg<u32>(ctx, 0), flags = arg<u32>(ctx, 1);
    const u32 cls = load<u32>(o + kObjClassPtr);
    u32 n;
    if ((flags & kCloneOwnClass) != 0) {
        const u32 copy = static_cast<u32>(call(ctx, o4fn::WorldClass_Copy, {cls}));
        n = static_cast<u32>(call(ctx, o4fn::WorldObj_Instance, {copy, 0}));
        Release(ctx, copy);
        call(ctx, o4fn::WorldObj_CopyFrom, {n, o, 1});
    } else {
        n = static_cast<u32>(call(ctx, o4fn::WorldObj_Instance, {cls, 0}));
        call(ctx, o4fn::WorldObj_CopyFrom, {n, o, 0});
    }
    if ((flags & kCloneChildren) != 0) {
        for (u32 c = load<u32>(o + kObjFirst); c != 0; c = load<u32>(c + kObjNext)) {
            u32 opd = load<u32>(load<u32>(c) + kVfnClone);
            const u32 cc = static_cast<u32>(call_ptr(ctx, opd, {c, flags, opd, load<u32>(opd)}));  // r5 / r6
            opd = load<u32>(load<u32>(n) + kVfnAddChild);
            call_ptr(ctx, opd, {n, cc, ~0ull, opd, load<u32>(opd)});  // tag -1; r6 / r7 = OPD and entry
            Release(ctx, cc);
        }
    }
    if (load<u32>(o + kObjParentLink) == 0 && (flags & kCloneNoWorld) == 0) call(ctx, o4fn::WorldObj_AddToWorld, {n});
    ret(ctx, n);
}

// void WorldObj_SetBoundRadius(Obj* o, f32 radius)  (the base +64; "WorldObj_ApplyClass" in symbols.txt)
// Gives the class's bounding sphere the radius (centre kept) and makes its
// box the cube of that half-size around the old box's centre; the object
// copies the sphere (+224). Then marks the object dirty and 0x0057EE00(o,
// 0, 0). WorldObj_ClassChanged calls it with whatever f1 its caller left.
DECOMP(0058049C, WorldObj_SetBoundRadius) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 r = ctx.fpr[1];
    const u32 cls = load<u32>(o + kObjClassPtr);
    const u32 cx = load<u32>(cls + kClassSphere), cy = load<u32>(cls + kClassSphere + 4),
              cz = load<u32>(cls + kClassSphere + 8);
    Stack frame(ctx, 16);  // sp+112: the box centre
    const u32 c = static_cast<u32>(call(ctx, o4fn::Box_Center, {cls + kClassBoxMin, frame.addr()}));
    const f64 x = load<f32>(c), y = load<f32>(c + 4), z = load<f32>(c + 8);
    const f32 rs = static_cast<f32>(r);
    store<u32>(cls + kClassSphere, cx);
    store<u32>(cls + kClassSphere + 4, cy);
    store<u32>(cls + kClassSphere + 8, cz);
    store<f32>(cls + kClassSphere + 12, rs);
    store<u32>(o + kObjSphere, cx);
    store<u32>(o + kObjSphere + 4, cy);
    store<u32>(o + kObjSphere + 8, cz);
    store<f32>(o + kObjSphere + 12, rs);
    store<f32>(cls + kClassBoxMin, static_cast<f32>(x - r));
    store<f32>(cls + kClassBoxMin + 4, static_cast<f32>(y - r));
    store<f32>(cls + kClassBoxMin + 8, static_cast<f32>(z - r));
    store<f32>(cls + kClassBoxMax, static_cast<f32>(x + r));
    store<f32>(cls + kClassBoxMax + 4, static_cast<f32>(y + r));
    store<f32>(cls + kClassBoxMax + 8, static_cast<f32>(z + r));
    MarkDirtyInline(ctx, o);
    call(ctx, o4fn::WorldObj_UpdateLod, {o, 0, 0});
}

namespace {

// 1 << b as slw computes it: shifts of 32-63 give 0.
inline u32 Bit(u32 b) { return (b & 0x20) != 0 ? 0u : 1u << (b & 0x1F); }

}  // namespace

// void WorldObj_UpdateZoneMask(Obj* o, bool lookup)  ("WorldObj_ShowChildren" in symbols.txt)
// Recomputes +324, the mask of the zones (*0x01193084) the object is in,
// calling vfunc +36 when it changes. With +168 bit 0x00800000 only a lookup
// does anything: the bit of the zone whose +176 equals the current mask
// (else 0). Otherwise a lookup asks 0x00577338, and a plain update tests
// the object's world box against each zone's box (and its shape when the
// zone's +2 bit 0 is set). WorldObj_ShowAndNotify passes lookup = 0.
DECOMP(00582C6C, WorldObj_UpdateZoneMask) {
    const u32 o = arg<u32>(ctx, 0);
    const bool lookup = (arg<u32>(ctx, 1) & 0xFF) != 0;
    const u32 zones = load<u32>(kZones);
    if (zones == 0) return;
    u32 mask;
    if (((load<u32>(o + 168) >> 23) & 1) != 0) {
        if (!lookup) return;
        const u32 cur = load<u32>(o + kObjZoneMask);
        const u32 n = load<u8>(zones + 16);
        u32 hit = 0;
        for (u32 i = 0, z = zones + 32; i < n; ++i, z += kZoneSize)
            if (load<u32>(z + 176) == cur) {
                hit = z;
                break;
            }
        mask = hit != 0 ? Bit(load<u8>(hit)) : 0;
        if (cur == mask) return;
    } else if (lookup) {
        mask = static_cast<u32>(call(ctx, o4fn::Zones_MaskFor, {zones, o}));
        if (load<u32>(o + kObjZoneMask) == mask) return;
    } else {
        Stack frame(ctx, 48);  // sp+112: the world box {min[3], -, max[3], -}
        const u32 box = frame.addr();
        for (u32 k = 0; k < 12; k += 4) store<u32>(box + k, kFltMax);
        for (u32 k = 16; k < 28; k += 4) store<u32>(box + k, kFltMinNeg);
        call(ctx, o4fn::Box_Transform, {box, load<u32>(o + kObjClassPtr) + kClassBoxMin, o + kObjMatrix + 64});
        mask = 0;
        u32 z = zones + 32;
        for (u32 i = 0; i < load<u8>(zones + 16); ++i, z += kZoneSize) {
            if (static_cast<u32>(call(ctx, o4fn::Box_Overlaps, {z + 16, box})) == 0) continue;
            if ((load<u8>(z + 2) & 1) != 0 && static_cast<u32>(call(ctx, o4fn::Zone_Overlaps, {z + 48, box})) == 0)
                continue;
            mask |= Bit(load<u8>(z));
        }
        if (load<u32>(o + kObjZoneMask) == mask) return;
    }
    store<u32>(o + kObjZoneMask, mask);
    Vcall0(ctx, o, kVfnChanged);
}

// void WorldObj_ReleasePeers(Obj* o)  ("WorldObj_DetachAll" in symbols.txt)
// For o and every descendant: removes its peer record (index +164 high
// half). The record's owner gets index 0xFFFF and its +128 / +132 / +136
// blocks go back to the peers' allocator (+136 is not cleared); later
// records (and their 32-byte vectors) move down one, their owners taking
// the new index, or the last record is zeroed. Then the count drops.
DECOMP(0057DA68, WorldObj_ReleasePeers) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 idx = load<u16>(o + kObjFlags);
    if (idx != 0xFFFF) {
        const u32 off = idx * kPeerSize;
        const u32 peers = load<u32>(load<u32>(obj3::kSlotPeers));
        const u32 last32 = load<u16>(peers + kPeerCount) - 1u;
        const u32 last = last32 & 0xFFFF;
        const u32 rec = load<u32>(peers) + off;
        auto free_block = [&](u32 b) {
            const u32 alloc = load<u32>(load<u32>(obj3::kSlotPeers) + 4);
            const u32 opd = load<u32>(load<u32>(alloc) + 16);
            call_ptr(ctx, opd, {alloc, b, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry
        };
        const u32 owner = load<u32>(rec + 148);
        const u32 b128 = load<u32>(rec + 128);
        store<u16>(owner + kObjFlags, 0xFFFF);
        if (b128 != 0) {
            const u32 b132 = load<u32>(rec + 132);
            store<u32>(rec + 128, 0);
            if (b132 != 0) free_block(b132);
        }
        store<u32>(rec + 132, 0);
        const u32 b136 = load<u32>(rec + 136);
        if (b136 != 0) free_block(b136);
        if (((idx ^ last32) & 0xFFFF) != 0) {
            const u32 tail = last - idx;
            const u32 dst = load<u32>(peers) + off;
            call(ctx, o4fn::memmove, {dst, dst + kPeerSize, tail * kPeerSize});
            const u32 vdst = load<u32>(peers + 4) + ((idx << 5) & 0x001FFFE0u);
            call(ctx, o4fn::memmove, {vdst, vdst + kPeerVecSize, (tail << 5) & 0xFFFFFFE0u});
            if (static_cast<s32>(idx) < static_cast<s32>(last)) {
                const u32 base = load<u32>(peers);
                for (u32 i = idx; static_cast<s32>(i) < static_cast<s32>(last); i = (i + 1) & 0xFFFF)
                    store<u16>(load<u32>(base + i * kPeerSize + 148) + kObjFlags, static_cast<u16>(i));
            }
        } else {
            call(ctx, fn::memset, {load<u32>(peers) + off, 0, kPeerSize});
            const u32 v = load<u32>(peers + 4) + ((idx << 5) & 0xFFFFFFE0u);
            for (u32 k = 0; k < 16; k += 4) store<u32>((v & ~15u) + k, 0);         // stvx
            for (u32 k = 0; k < 16; k += 4) store<u32>(((v + 16) & ~15u) + k, 0);  // stvx
        }
        store<u16>(peers + kPeerCount, static_cast<u16>(load<u16>(peers + kPeerCount) - 1));
    }
    for (u32 c = load<u32>(o + kObjFirst); c != 0; c = load<u32>(c + kObjNext))
        call(ctx, 0x0057DA68, {c});
}

// void WorldObj_DumpTree(Obj* o, s32 depth, u32 flags)  ("WorldObj_RefreshSelf" in symbols.txt)
// The body of the debug tree dump (the printing is compiled out): fetches
// the object's and its class's names (vfunc +12), dumps the class (its +24
// with 0) while depth >= 1, then each child's +60 (depth, flags).
DECOMP(00580094, WorldObj_DumpTree) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 depth = ctx.gpr[4], flags = ctx.gpr[5];
    Vcall0(ctx, o, kVfnGetName);
    u32 cls = load<u32>(o + kObjClassPtr);
    if (cls != 0) {
        Vcall0(ctx, cls, kVfnGetName);
        cls = load<u32>(o + kObjClassPtr);
    }
    if (!(static_cast<s32>(depth) < 1 || cls == 0)) {
        const u32 opd = load<u32>(load<u32>(cls) + 24);
        call_ptr(ctx, opd, {cls, 0, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry
    }
    for (u32 c = load<u32>(o + kObjFirst); c != 0; c = load<u32>(c + kObjNext)) {
        const u32 opd = load<u32>(load<u32>(c) + 60);
        call_ptr(ctx, opd, {c, depth, flags, opd, load<u32>(opd)});  // r6 / r7 = OPD and entry
    }
}

// void Skeleton_DumpTree(ApeSkeleton* s, s32 depth)  ("WorldObj_RefreshTree" in symbols.txt)
// The skeleton half of the debug dump: with depth > 0 and bones (+28 u16,
// 16-byte entries at +32 whose +0 bone has its parent at +100), dumps each
// root bone (parent -1) through 0x00583FBC with a scratch bit set from the
// memory stack; bit 0 of its first word says another root follows.
DECOMP(00584160, Skeleton_DumpTree) {
    const u32 s = arg<u32>(ctx, 0);
    const s32 depth = arg<s32>(ctx, 1);
    if (depth <= 0 || load<u16>(s + 28) == 0) return;
    const u32 n0 = load<u16>(s + 28);
    const u32 bytes = ((static_cast<s32>(n0 + 31) / 32) << 2) & 0xFFFFFFFCu;
    const u32 ms = static_cast<u32>(call(ctx, o4fn::Mem_GetDefaultStack));
    const u32 bits = static_cast<u32>(call(ctx, o4fn::MemStack_Alloc, {ms, bytes}));
    call(ctx, fn::memset, {bits, 0, bytes});
    u32 n = load<u16>(s + 28);
    if (n != 0) {
        const u64 d1 = static_cast<u64>(static_cast<s64>(depth - 1));
        auto parent = [&](u32 i) {
            return static_cast<s16>(load<u16>(load<u32>(load<u32>(s + 32) + ((i << 4) & 0xFFFFFFF0u)) + 100));
        };
        for (u32 i = 0; i < n; ++i) {
            if (parent(i) != -1) continue;
            const u32 w = load<u32>(bits);
            const u32 count = load<u16>(s + 28);
            store<u32>(bits, w & ~1u);
            for (u32 j = i + 1; j < count; ++j)
                if (parent(j) == -1) {
                    store<u32>(bits, w | 1u);
                    break;
                }
            call(ctx, o4fn::Skeleton_DumpBone, {s, i, d1, bits});
            n = load<u16>(s + 28);
        }
    }
    const u32 ms2 = static_cast<u32>(call(ctx, o4fn::Mem_GetDefaultStack));
    const u32 opd = load<u32>(load<u32>(ms2) + 8);
    call_ptr(ctx, opd, {ms2, bits, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry
}
