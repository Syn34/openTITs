// World objects, part 5: the tree walk, the root callback and the state
// copy clones use.
#include <cmath>

#include "obj5.h"

using namespace sly4;
using namespace sly4::obj4;
using namespace sly4::obj5;

// bool WorldObj_SetRootCb(Obj* node, Obj* root)  (OPD 0x00F3C488)
// The WorldObj_ForEach callback AddChild / RemoveChild use.
DECOMP(0057F5CC, WorldObj_SetRootCb) {
    store<u32>(arg<u32>(ctx, 0) + kObjRoot, arg<u32>(ctx, 1));
    ret(ctx, 1u);
}

// bool WorldObj_ForEach(Obj* o, bool (*fn)(Obj*, void*), void* arg, bool self)
// Calls fn(node, arg) on o (only with `self`) and then on every descendant,
// depth first (a node, its children, then its next siblings), stopping
// with 0 as soon as fn returns 0. The original walks with a 10-entry stack
// (a 0 sentinel under the first child): a push that would go past it is
// dropped, so very deep or wide trees lose subtrees (kept). r5 = fn's entry
// address at each call, as the original leaves it.
DECOMP(0057F5D8, WorldObj_ForEach) {
    const u32 o = arg<u32>(ctx, 0), fn = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2);
    const bool self = (arg<u32>(ctx, 3) & 0xFF) != 0;
    auto visit = [&](u32 node) { return static_cast<u32>(call_ptr(ctx, fn, {node, a, load<u32>(fn)})) != 0; };
    if (self && !visit(o)) return ret(ctx, 0u);
    const u32 first = load<u32>(o + kObjFirst);
    if (first == 0) return ret(ctx, 1u);
    const s32 cap = static_cast<s32>(kWalkStackCap);
    u32 stack[kWalkStackCap + 1];  // sp+120: indices 0..cap
    stack[0] = 0;
    s32 top = 1;
    stack[1] = first;
    for (;;) {
        u32 node;
        s32 below;
        if (top > -1) {
            node = stack[top];
            below = top - 1;
            top = below;
        } else {
            node = stack[0];  // unreachable: popping the sentinel returns
            below = top;
        }
        if (node == 0) return ret(ctx, 1u);
        if (!visit(node)) return ret(ctx, 0u);
        s32 at = below;
        const u32 next = load<u32>(node + kObjNext);
        if (next != 0 && below < cap) {
            top = below + 1;
            stack[top] = next;
            at = top;
        }
        const u32 child = load<u32>(node + kObjFirst);
        if (child != 0 && at < cap) {
            top = at + 1;
            stack[top] = child;
        }
    }
}

// void WorldObj_CopyFrom(Obj* dst, const Obj* src, u32 flags)
// Copies the instance state a clone takes: the base state (0x0057DEEC),
// +292 (unless flags 0x40), +296, the +316 colour, the +320 / +321 bytes
// and the 0x9E00 bits of the +320 word (a type of 1 or 2 in 0x1E00 also
// sets dst +168 bit 0x00200000), the skeleton pose (a new skeleton from
// the source's definition when dst has none), an AglLight at +288 when the
// source has +292 (then vfunc +36) or none when it hasn't, and the zone
// mask +324 (unless flags 8). r3 is left as the original leaves it.
DECOMP(00581B34, WorldObj_CopyFrom) {
    const u32 dst = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1), flags = arg<u32>(ctx, 2);
    call(ctx, o5fn::WorldObj_CopyBase, {dst, src, flags});
    if ((flags & kCopyKeep292) == 0) store<u32>(dst + kObj292, load<u32>(src + kObj292));
    store<u32>(dst + kObj296, load<u32>(src + kObj296));
    for (u32 k = 0; k < 4; ++k) store<u8>(dst + obj3::kObjColor + k, load<u8>(src + obj3::kObjColor + k));
    const u32 d = load<u32>(dst + kObjBits320);
    const u32 s = load<u32>(src + kObjBits320);
    const u32 f168 = load<u32>(dst + 168);
    store<u32>(dst + kObjBits320, (s & 0xFFFF9E00u) | (d & 0x000061FFu));
    const u32 type = (s >> 9) & 0xF;
    if (type == 1 || type == 2) store<u32>(dst + 168, f168 | kObj168Type12);
    const u32 sk = load<u32>(src + kObjSkeleton);
    if (sk != 0) {
        const u32 dsk = load<u32>(dst + kObjSkeleton);
        if (dsk == 0) {
            const u32 n = static_cast<u32>(call(ctx, o4fn::Skeleton_Create, {load<u32>(sk + 16)}));
            call(ctx, o5fn::Skeleton_CopyPose, {n, sk});
            store<u32>(dst + kObjSkeleton, n);
        } else {
            call(ctx, o5fn::Skeleton_CopyPose, {dsk, sk});
        }
    }
    if (load<u32>(src + kObj292) != 0) {
        if (load<u32>(dst + kObjLight) == 0) {
            store<u32>(dst + kObjLight, static_cast<u32>(call(ctx, o5fn::Light_Alloc)));
            Vcall0(ctx, dst, kVfnChanged);
        }
    } else {
        const u32 l = load<u32>(dst + kObjLight);
        if (l != 0) {
            call(ctx, o5fn::Light_Free, {l});
            store<u32>(dst + kObjLight, 0);
        }
    }
    if ((flags & kCopyKeepZones) != 0) return ret(ctx, flags & kCopyKeepZones);
    const u32 z = load<u32>(src + kObjZoneMask);
    store<u32>(dst + kObjZoneMask, z);
    ret(ctx, z);
}

namespace {

struct Quad {
    f32 v[4];
};

// stvx: the quadword at ea & ~15.
inline void StoreQuad(u32 ea, const Quad& q) {
    const u32 a = ea & ~15u;
    for (u32 i = 0; i < 4; ++i) store<f32>(a + i * 4, q.v[i]);
}

// vmaddfp with a splatted scalar: a * s + b lane by lane.
inline Quad Madd(const Quad& a, f32 s, const Quad& b) {
    Quad r;
    for (u32 i = 0; i < 4; ++i) r.v[i] = std::fmaf(a.v[i], s, b.v[i]);
    return r;
}

}  // namespace

// ApeSkeleton* Skeleton_Create(ApeSkeletonDef* def)
// A 64-byte ApeSkeleton from the free list at 0x011DE4E4 (the list made on
// first use: 128 entries), set up from the definition (0x005814A0). Then
// every bone with a parent (bone def +100 != -1) gets its local matrix
// (entry +4) = its world matrix (entry +8) times the inverse of the
// parent's world matrix (rotation transposed, translation subtracted), as
// the original's vmaddfp chains compute it, all four lanes. Quirks kept: a
// failed list allocation stores a null allocator and calls through it, and
// an empty list still reads the bone count at address 28.
DECOMP(005816F0, Skeleton_Create) {
    const u32 def = arg<u32>(ctx, 0);
    u32 list = load<u32>(kSkeletonList);
    if (list == 0) {
        const u32 obj = static_cast<u32>(call(ctx, o4fn::Mem_Alloc, {60, 0, 16}));
        if (obj != 0) {
            for (u32 off : {8u, 4u, 12u, 20u, 16u}) store<u32>(obj + off, 0);
            const u16 h54 = load<u16>(obj + 54);
            for (u32 off : {24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(obj + off, 0);
            store<u16>(obj + 52, 0);
            store<u16>(obj + 54, static_cast<u16>(h54 & 0x7FFF));
            store<u32>(obj, kSkeletonListVtbl);
            store<u32>(obj + 56, kStrSkeletonList);
            // r8 / r9 = the vtable and the string here; FreeList_Init reads r3..r7 only.
            call(ctx, o4fn::FreeList_Init, {obj, 64, 128, 0, 0});
        }
        list = obj;
        store<u32>(kSkeletonList, list);
    }
    const u32 got = static_cast<u32>(Vcall0(ctx, list, 12));
    u32 sk = 0;
    if (got != 0) {
        call(ctx, o5fn::Skeleton_Init, {got, def});
        sk = got;
    }
    for (u32 i = 0; i < load<u16>(sk + kSkelBoneCount); ++i) {
        const u32 entry = load<u32>(sk + kSkelBones) + i * kSkelEntrySize;
        const s32 parent = static_cast<s16>(load<u16>(load<u32>(entry) + kBoneParent));
        if (parent == -1) continue;
        const u32 pentry = load<u32>(sk + kSkelBones) + static_cast<u32>(parent) * kSkelEntrySize;
        const u32 p = load<u32>(pentry + kSkelWorld);
        auto P = [&](u32 off) { return load<f32>(p + off); };  // lvlx word 0
        // The parent's rotation transposed (the w lanes are what the merges leave).
        const Quad c0{{P(0), P(16), P(32), P(20)}};
        const Quad c1{{P(4), P(20), P(36), P(0)}};
        const Quad c2{{P(8), P(24), P(40), P(8)}};
        const f32 tx = 0.0f - P(48), ty = 0.0f - P(52), tz = 0.0f - P(56);  // vsubfp from 0
        // (the original also spills P(48..56) to sp+160; nothing reads them)
        const u32 l = load<u32>(entry + kSkelWorld);
        auto L = [&](u32 off) { return load<f32>(l + off); };
        const Quad zero{{0.0f, 0.0f, 0.0f, 0.0f}};
        const Quad r0 = Madd(c2, L(8), Madd(c1, L(4), Madd(c0, L(0), zero)));
        const Quad r1 = Madd(c2, L(24), Madd(c1, L(20), Madd(c0, L(16), zero)));
        const Quad r2 = Madd(c2, L(40), Madd(c1, L(36), Madd(c0, L(32), zero)));
        const Quad ta = Madd(c2, tz, Madd(c1, ty, Madd(c0, tx, zero)));
        const Quad tb = Madd(c2, L(56), Madd(c1, L(52), Madd(c0, L(48), zero)));
        Quad r3;
        for (u32 k = 0; k < 4; ++k) r3.v[k] = ta.v[k] + tb.v[k];
        const u32 out = load<u32>(entry + kSkelLocal);
        StoreQuad(out, r0);
        StoreQuad(out + 16, r1);
        StoreQuad(out + 32, r2);
        StoreQuad(out + 48, r3);
    }
    ret(ctx, sk);
}

// void WorldObj_AddToWorld(Obj* o)
// For an unparented object: collects o and its descendants (WorldObj_ForEach
// with the collector at 0x005A9AA8) into a 32-entry list on the stack, hands
// the list to the world (0x005947D0 on *0x01194598, 1), marks o dirty and
// registers o itself (0x00597814). The collector's heap path (a null
// inline buffer) can't happen; its frees are kept for the inline flag.
// r3 is left as the original leaves it (o +260, or the inline flag).
DECOMP(005A0394, WorldObj_AddToWorld) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 parent = load<u32>(o + kObjParentLink);
    if (parent != 0) return ret(ctx, parent);
    Stack frame(ctx, kCollectorFrame);  // sp+112: the collector, its pointers and flags after it
    const u32 c = frame.addr();
    store<u32>(c + kColCount, 0);
    store<u32>(c + kColCapacity, kColInline);
    store<u8>(c + kColOption, 1);
    store<u32>(c + kColData, c + kColInlineData);
    store<u8>(c + kColIsInline, 1);
    store<u32>(c + kColFlags, c + kColInlineFlags);
    store<u32>(c + kColRun, 0);
    call(ctx, o4fn::WorldObj_ForEach, {o, kCollectOpd, c, 1});
    const u32 run = load<u32>(c + kColRun);
    if (run != 0) store<u8>(load<u32>(c + kColFlags) + (load<u32>(c + kColCount) - run), static_cast<u8>(run));
    auto free_heap = [&] {
        call(ctx, cls::ifn::Mem_FreeFrom, {load<u32>(c + kColData), 0});
        call(ctx, cls::ifn::Mem_FreeFrom, {load<u32>(c + kColFlags), 0});
    };
    if (load<u32>(c + kColCount) == 0) {
        if (load<u8>(c + kColIsInline) != 0) return ret(ctx, 1u);
        return free_heap();
    }
    call(ctx, o5fn::World_AddObjects, {load<u32>(obj3::kSlotPeers), c, 1});
    MarkDirtyInline(ctx, o);
    call(ctx, o5fn::World_AddObject, {load<u32>(obj3::kSlotPeers), o});
    if (load<u8>(c + kColIsInline) != 0) return ret(ctx, 1u);
    free_heap();
}

// void Skeleton_DumpBone(ApeSkeleton* s, u32 bone, s32 depth, u32* last_bits)
// The bone half of the debug tree dump (printing compiled out): bumps the
// dump level (0x00F889F4), "prints" the bone (0x0055498C, an empty stub)
// when depth != 0, then dumps each child bone (parent == bone) one level
// down with the same depth. last_bits gets the level's bit cleared, or set
// when another child follows, before each child's dump.
DECOMP(00583FBC, Skeleton_DumpBone) {
    const u32 s = arg<u32>(ctx, 0), bone = arg<u32>(ctx, 1);
    const u64 depth = ctx.gpr[5];
    const u32 bits = arg<u32>(ctx, 3);
    store<u32>(kDumpLevel, load<u32>(kDumpLevel) + 1);
    auto bone_def = [&](u32 i) { return load<u32>(load<u32>(s + kSkelBones) + ((i << 4) & 0xFFFFFFF0u)); };
    if (static_cast<s32>(depth) != 0) call(ctx, o5fn::Debug_PrintBone, {bone_def(bone)});
    auto parent = [&](u32 i) { return static_cast<s32>(static_cast<s16>(load<u16>(bone_def(i) + kBoneParent))); };
    // The level's word in last_bits: (level >> 5) words in (signed).
    auto word = [&](u32 level) { return bits + (static_cast<u32>(static_cast<s32>(level) >> 5) << 2); };
    for (u32 i = 0; i < load<u16>(s + kSkelBoneCount); ++i) {
        if (parent(i) != static_cast<s32>(bone)) continue;
        const u32 level = load<u32>(kDumpLevel);
        const u32 n = load<u16>(s + kSkelBoneCount);
        store<u32>(word(level), load<u32>(word(level)) & ~(1u << (level & 31)));
        for (u32 j = i + 1; j < n; ++j)
            if (parent(j) == static_cast<s32>(bone)) {
                store<u32>(word(level), load<u32>(word(level)) | (1u << (level & 31)));
                break;
            }
        call(ctx, 0x00583FBC, {s, i, depth, bits});
    }
    store<u32>(kDumpLevel, load<u32>(kDumpLevel) - 1);
}
