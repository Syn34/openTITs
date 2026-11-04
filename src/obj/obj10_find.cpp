// World objects, part 10: the peer record lookups, the bone searches and
// three deleting destructors.
#include "obj10.h"

using namespace sly4;

namespace {

constexpr u32 kMem_FreeFrom = 0x0054EE6C;        // (block, 0)
constexpr u32 kWorldObj_DestructBase = 0x00580978;
constexpr u32 kGeomDef_Destruct = 0x00585678;    // ApeGeomDef +0 (name?)
constexpr u32 kEmitterDef_Destruct = 0x0059D360; // ApeEmitterDef +0 (name?)

constexpr u32 kObjPeerIndex = 164;   // u16: the peer record index (0xFFFF = none)
constexpr u32 kNoPeer = 0xFFFF;

// ApeSkeletonDef: +2 u16 bone count, +4 112-byte bone defs; a bone def's
// +64 is its name hash. ApeSkeleton: +28 u16 count, +32 16-byte entries
// (+0 the bone def).
constexpr u32 kSdCount = 2, kSdBones = 4, kBoneDefSize = 112, kBoneHash = 64;
constexpr u32 kSkCount = 28, kSkEntries = 32, kSkEntrySize = 16;

// The inlined peer lookup: records + index * 160, or 0 without one. r4..r6
// as the original leaves them.
void GetPeer(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 hdr = load<u32>(load<u32>(obj3::kSlotPeers));
    const u32 idx = load<u16>(o + kObjPeerIndex);
    ctx.gpr[5] = hdr;
    ctx.gpr[6] = kNoPeer;
    if (idx == kNoPeer) {
        ctx.gpr[4] = idx;
        ret(ctx, 0u);
        return;
    }
    const u32 recs = load<u32>(hdr);
    ctx.gpr[4] = recs;
    ret(ctx, recs + idx * obj4::kPeerSize);
}

// The index of the first of `count` entries (stride bytes apart from
// first) whose bone def's hash equals `hash` (signed compare), or -1. r5..r7
// as the original leaves them.
template <bool kIndirect>
void FindBone(PPUContext& ctx, u32 count, u32 first, u32 stride, u32 hash) {
    ctx.gpr[5] = count;
    ctx.gpr[6] = 0;
    u32 e = first;
    for (u32 i = 0; i < count; ++i, e += stride) {
        const u32 def = kIndirect ? load<u32>(e) : e;
        const u32 h = load<u32>(def + kBoneHash);
        ctx.gpr[6] = i;
        ctx.gpr[7] = h;
        if (static_cast<s32>(h) == static_cast<s32>(hash)) {
            ret(ctx, static_cast<s32>(i));
            return;
        }
        ctx.gpr[6] = i + 1;
    }
    ret(ctx, -1);
}

}  // namespace

// PeerRecord* WorldObj_GetPeer(Obj* o)  (two identical copies)
// The object's 160-byte peer record (+164 index), or 0.
DECOMP(0057A428, WorldObj_GetPeer) { GetPeer(ctx); }
DECOMP(0057FF50, WorldObj_GetPeer2) { GetPeer(ctx); }

// s32 SkeletonDef_FindBone(const ApeSkeletonDef* d, u32 hash)
DECOMP(00584888, SkeletonDef_FindBone) {
    const u32 d = arg<u32>(ctx, 0);
    const u32 n = load<u16>(d + kSdCount);
    FindBone<false>(ctx, n, n != 0 ? load<u32>(d + kSdBones) : 0, kBoneDefSize, arg<u32>(ctx, 1));
}

// s32 Skeleton_FindBone(const ApeSkeleton* s, u32 hash)
DECOMP(005A93D8, Skeleton_FindBone) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 n = load<u16>(s + kSkCount);
    FindBone<true>(ctx, n, n != 0 ? load<u32>(s + kSkEntries) : 0, kSkEntrySize, arg<u32>(ctx, 1));
}

// void XxxDeletingDestruct(o)  (slot +4): the destructor, then the block
// back to the heap; nothing for a null object.
DECOMP(00580AA4, WorldObjBase_DeletingDestruct) {  // vtable 0x00B24660
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    call(ctx, kWorldObj_DestructBase, {o});
    call(ctx, kMem_FreeFrom, {o, 0});
}
DECOMP(00585848, GeomDef_DeletingDestruct) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    call(ctx, kGeomDef_Destruct, {o});
    call(ctx, kMem_FreeFrom, {o, 0});
}
DECOMP(0059D4DC, EmitterDef_DeletingDestruct) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    call(ctx, kEmitterDef_Destruct, {o});
    call(ctx, kMem_FreeFrom, {o, 0});
}

namespace {

constexpr u32 kSplineRecs = 0x01194580 + 12;      // obj7.h kSplineList: 96-byte records
constexpr u32 kObjSplineIndex = 284, kSrBits = 95, kSrSize = 96;
constexpr u32 kSrFollowParent = 0x40;
constexpr u32 kIff_ReadChunks = 0x00569388;       // (reader, obj)
constexpr u32 kKindGeomDef = 20;
constexpr u32 kKindGeom = 8;
constexpr u32 kObjLight = 288, kObj292 = 292, kLightFlags212 = 212;
constexpr u32 kSkeletonWatchPtr = 0x01194590;     // {ApeSkeleton** data, u32 count, u32 capacity}* (obj4.h kSkeletonWatch)
constexpr u32 kSkelFlags = 31, kSkelWatched = 0x20;
constexpr u32 kResTableCount = 0x01193000;        // u16 (name?)

}  // namespace

// bool SplineInstance_FollowsParent(ApeSplineInstance* o)
// Its spline record's +95 bit 0x40; 0 without a record.
DECOMP(005A8BE4, SplineInstance_FollowsParent) {
    const s32 idx = static_cast<s32>(load<u32>(arg<u32>(ctx, 0) + kObjSplineIndex));
    if (idx == -1) {
        ret(ctx, 0u);
        return;
    }
    const u32 base = load<u32>(kSplineRecs);
    ctx.gpr[4] = base;
    ctx.gpr[5] = static_cast<u64>(static_cast<u32>(idx)) << 6;
    ret(ctx, (load<u8>(base + static_cast<u32>(idx) * kSrSize + kSrBits) & kSrFollowParent) != 0 ? 1u : 0u);
}

// void Peer_RewindAnims(PeerRecord* p)
// The rotation / position animators' times (+0..+8) to 0 and the spline
// follower's +64 to 0. r3..r5 as the original leaves them.
DECOMP(0057CDB8, Peer_RewindAnims) {
    const u32 p = arg<u32>(ctx, 0);
    for (u32 off : {obj10::kPrRotAnim, obj10::kPrPosAnim}) {
        const u32 a = load<u32>(p + off);
        ctx.gpr[4] = a;
        if (a == 0) continue;
        ctx.gpr[5] = 0;
        store<u32>(a + 8, 0);
        store<u32>(a + 4, 0);
        store<u32>(a + 0, 0);
    }
    const u32 f = load<u32>(p + obj10::kPrFollower);
    ctx.gpr[3] = f;
    if (f == 0) return;
    ctx.gpr[4] = 0;
    store<u32>(f + 64, 0);
}

// bool Iff_HandleGeomDef(IffReader* r, Obj* o)
// For an ApeGeomDef (kind 20) reads the child chunks into it and returns
// 1; otherwise 0.
DECOMP(005A9420, Iff_HandleGeomDef) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 kind = obj4::Kind(o);
    if (kind != kKindGeomDef) {
        ctx.gpr[4] = r;
        ctx.gpr[5] = kind;
        ret(ctx, 0u);
        return;
    }
    call(ctx, kIff_ReadChunks, {r, o, o});  // r5 = o as the original leaves it
    ret(ctx, 1u);
}

// void ResTable_Reset(ResTable* t)  (name?)
// Frees the block at +0 (and clears the pointer), then the u16 counts at
// +4, +5136 and 0x01193000 to 0.
DECOMP(00579BCC, ResTable_Reset) {
    const u32 t = arg<u32>(ctx, 0);
    const u32 b = load<u32>(t);
    if (b != 0) {
        call(ctx, kMem_FreeFrom, {b, 0});
        store<u32>(t, 0);
    }
    store<u16>(t + 4, 0);
    store<u16>(t + 5136, 0);
    store<u16>(kResTableCount, 0);
    ctx.gpr[3] = 0;
    ctx.gpr[4] = 0x01190000;
}

// bool GeomInstance_Replace292(Obj* o, u32 v)  (no direct callers)
// A non-geometry object (kind != 8) answers 1 untouched. A geometry
// object with a +292 value gets v there (and its light's +212 bit 0x80
// cleared, as WorldObj_Set292) and answers 1; otherwise 0. Quirk kept:
// the kind is read before the null check.
DECOMP(00582C10, GeomInstance_Replace292) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 kind = obj4::Kind(o);
    ctx.gpr[5] = kind;
    if (kind != kKindGeom) {
        ret(ctx, 1u);
        return;
    }
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 cur = load<u32>(o + kObj292);
    ctx.gpr[5] = cur;
    if (cur == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(o + kObj292, v);
    const u32 light = load<u32>(o + kObjLight);
    if (light != 0) {
        const u8 b = static_cast<u8>(load<u8>(light + kLightFlags212) & 0x7F);
        store<u8>(light + kLightFlags212, b);
        ctx.gpr[4] = 0;
        ctx.gpr[5] = b;
    }
    ret(ctx, 1u);
}

// void Skeleton_Watch(ApeSkeleton* s)
// Appends an unwatched skeleton (+31 bit 0x20 clear) to the watch list
// while it has room, and marks it watched.
DECOMP(005A9378, Skeleton_Watch) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 w = load<u32>(kSkeletonWatchPtr);
    ctx.gpr[4] = w;
    if ((load<u8>(s + kSkelFlags) & kSkelWatched) != 0) {
        ctx.gpr[5] = 1;
        return;
    }
    const u32 n = load<u32>(w + 4), cap = load<u32>(w + 8);
    ctx.gpr[5] = n;
    ctx.gpr[6] = cap;
    if (n >= cap) return;
    const u32 data = load<u32>(w);
    store<u32>(w + 4, n + 1);
    store<u32>(data + n * 4, s);
    const u8 b = static_cast<u8>(load<u8>(s + kSkelFlags) | kSkelWatched);
    store<u8>(s + kSkelFlags, b);
    ctx.gpr[4] = b;
    ctx.gpr[5] = w + 4;
    ctx.gpr[6] = n << 2;
    ctx.gpr[7] = n + 1;
}
