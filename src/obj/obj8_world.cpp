// World objects, part 8: World_AddObjects, which gives every object a
// collector gathered a peer record in the world.
#include "obj8.h"

using namespace sly4;
using namespace sly4::obj;
using namespace sly4::obj4;
using namespace sly4::obj8;

namespace {

constexpr u32 kPeer_Bind = 0x0057BEE0;        // (peer, obj) (name?)
constexpr u32 kPeer_GetMatrix = 0x0057C408;   // (peer, f1, f32 out[4][4]) -> bool: copy instead of multiply (name?)
constexpr u32 kWorld_AddGroup = 0x005922A8;   // (world, the group's +152 object): r3 not read (name?)
constexpr u32 kSpline_Nearest = 0x00560A40;   // (spline data, const f32 p[3], 0, 0) -> f1 (name?)
constexpr u32 kSpline_PointAt = 0x005602BC;   // (spline data, f1 t, f32 out[3], 0): r4 not read (name?)
constexpr u32 kAddConsts = 0x005947CC;        // f32 0.0
constexpr u32 kScaleEps = 0x0057BED8;         // f32 1e-5 (Peer_Bind)
constexpr u32 kString_Format = 0x005534D0;
constexpr u32 kStrHasScale = 0x00AF4DFC;      // "ProcAnim: Object has scale:%s - (%f,%f,%f)"
constexpr u32 kPrOwner = 148;                 // Obj*
constexpr u32 kListener_LocalDirection = 0x005532E4;  // (f32 out[3], const f32 p[3], const Matrix*)

// The 160-byte peer record (obj3.h's kSlotPeers +0, u16 count at +8).
constexpr u32 kPeersCount = 8;
constexpr u32 kPrMatrix = 0;                  // f32[4][4]: +48 the position
constexpr u32 kPrOwned0 = 128, kPrOwned1 = 132;
constexpr u32 kPrFollow = 136;                // a spline follower: +68 f32 the parameter, +80 the spline object (name?)
constexpr u32 kFwSpline = 80, kFwParam = 68;
constexpr u32 kPrMatrixSrc = 140;             // f32*: the object's matrix (+16 or +80)
constexpr u32 kPrSlotEntry = 144;             // SlotEntry* for kinds 8 / 10 (0 otherwise)
constexpr u32 kPrGroupObj = 152;              // the object's +256 (root)
constexpr u32 kPrFlags = 156;                 // u8: 0x80 / 0x40 / 0x20 / 0x10 (name?)
constexpr u32 kPrFlagAttached = 0x80, kPrFlagGroup = 0x40, kPrFlagGrouped = 0x20, kPrFlag10 = 0x10;
// The 32-byte vector record (PeerVec): +16 / +20 / +24 the range terms,
// +28 bits, +29 u8 the group's extra record count.
constexpr u32 kVrRange = 16, kVrBits = 28, kVrGroupCount = 29;
constexpr u32 kVrRangeDefault = 0x41C80000, kVrRangeSqDefault = 0x441C4000, kVrInnerSqDefault = 0x43C80000;  // 25, 625, 400
constexpr u32 kObjBits321 = 321;              // u8: 0x02 forces vector bit 0x10 (name?)
constexpr u32 kFrame = 400;
constexpr u32 kSpA = 112, kSpB = 128, kSpDir = 144, kSpMatrix = 160;

inline u32 RecOf(u32 peers, u32 n) { return load<u32>(peers) + n * obj4::kPeerSize; }

// p = W * p + W row 3 for the point at sp_ea (W = the object's world matrix).
void TransformPoint(u32 obj, u32 sp_ea) {
    const u32 w = obj + obj6::kObjWorld;
    Vec4 a = VMadd(VLoad(w), VSplat(VLoadLeft(sp_ea), 0), VZero());
    a = VMadd(VLoad(w + 16), VSplat(VLoadLeft(sp_ea + 4), 0), a);
    a = VMadd(VLoad(w + 32), VSplat(VLoadLeft(sp_ea + 8), 0), a);
    a = VAdd(VLoad(w + 48), a);
    VStore(sp_ea, a);
}

}  // namespace

// void Peer_Bind(PeerRecord* rec, Obj* o)
// The record takes o's local matrix rows (x, y, z of each), o itself
// (+148), its world matrix (+140) and its sphere (+240, into the vector
// record); an object whose scale is not (1, 1, 1) within 1e-5 gets the
// "ProcAnim: Object has scale" debug message.
DECOMP(0057BEE0, Peer_Bind) {
    const u32 rec = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 l = o + obj6::kObjLocal;
    for (u32 k : {0u, 4u, 8u, 16u, 20u}) store<u32>(rec + k, load<u32>(l + k));
    store<u32>(rec + kPrOwner, o);
    for (u32 k : {24u, 32u, 36u, 40u}) store<u32>(rec + k, load<u32>(l + k));
    store<u32>(rec + kPrMatrixSrc, o + obj6::kObjWorld);
    for (u32 k : {48u, 52u, 56u}) store<u32>(rec + k, load<u32>(l + k));
    const u32 vr = PeerVec(rec);
    for (u32 k = 0; k < 16; k += 4) store<u32>(vr + k, load<u32>(o + 240 + k));
    const f32 eps = load<f32>(kScaleEps);
    bool unit = true;
    for (u32 k = 0; k < 12 && unit; k += 4) {
        const f32 d = static_cast<f32>(static_cast<f64>(load<f32>(o + kObjScale + k)) - static_cast<f64>(load<f32>(kOneVec3 + k)));
        unit = std::fabs(d) < eps;
    }
    if (unit) {
        ctx.gpr[3] = kOneVec3;  // as the original leaves it
        return;
    }
    const u32 name = static_cast<u32>(Vcall0(ctx, o, kVfnGetName));
    // The scale goes as three doubles in r5..r7 (varargs).
    const auto bits = [&](u32 k) { return std::bit_cast<u64>(static_cast<f64>(load<f32>(o + kObjScale + k))); };
    call(ctx, kString_Format, {kStrHasScale, name, bits(0), bits(4), bits(8)});
}

// void World_AddObjects(World* w, Collector* col, bool attach_all)
// For each collected object (col +16, its flag byte at col +20): the next
// peer record (its index stored at the object's +164 high half) and its
// vector record are reset, the root is marked dirty and its LOD updated,
// the record bound (0x0057BEE0), given the slot entry of a kind-8 / 10
// object and its component properties (Comps_Query). Bit 0x10 of the
// vector record marks shadow casters (or +321 bit 0x02); a nonzero flag
// starts a group (record bit 0x20; the previous group head gets the
// group's OR of bit 0x40). Top-level childless objects and flagged ones
// remember their root (+152); flagged ones keep flag - 1 extra records
// (vector +29) and are attached (bit 0x80) when that is nonzero or
// attach_all; an unattached single child of a top-level object takes its
// world matrix from its parent. Spline followers (+136) move their
// position by the spline's offset. Then the record's matrix comes from
// 0x0057C408 (copied, or multiplied into the object's matrix). Last, for
// each group of records from the first one: the group's root (0x005922A8)
// and vector bits 0x08 (no record of the group owns anything) / 0x04.
DECOMP(005947D0, World_AddObjects) {
    const u32 world = arg<u32>(ctx, 0), col = arg<u32>(ctx, 1);
    const u32 attach_all = arg<u32>(ctx, 2) & 0xFF;
    if (load<u32>(col + obj5::kColCount) == 0) return;
    Stack frame(ctx, kFrame);
    const u32 sp = frame.addr();
    const f32 zero = load<f32>(kAddConsts);
    u32 prev = 0, first = 0;
    bool group = false;
    u32 count = 0;
    for (u32 i = 0;;) {
        const u32 obj = load<u32>(load<u32>(col + obj5::kColData) + i * 4);
        const u32 fl = load<u8>(load<u32>(col + obj5::kColFlags) + i);
        const u32 peers = load<u32>(world);
        const u32 n = load<u16>(peers + kPeersCount);
        store<u16>(obj + obj::kObjFlags, static_cast<u16>(n));
        const u32 comps = load<u32>(obj + obj6::kObjComps);
        const u32 rec0 = RecOf(peers, n);
        if (rec0 != 0) {
            const u32 b = load<u8>(rec0 + kPrFlags);
            for (u32 off : {kPrOwned0, kPrOwned1, kPrFollow, kPrMatrixSrc, kPrOwner, kPrGroupObj}) store<u32>(rec0 + off, 0);
            store<u8>(rec0 + kPrFlags, static_cast<u8>(b & ~(kPrFlagAttached | kPrFlagGroup | kPrFlag10)));
        }
        const u32 vr0 = load<u32>(peers + 4) + (n << 5);
        if (vr0 != 0) {
            const u32 b = load<u8>(vr0 + kVrBits);
            store<u32>(vr0 + kVrRange, kVrRangeDefault);
            store<u32>(vr0 + kVrRange + 4, kVrRangeSqDefault);
            store<u32>(vr0 + kVrRange + 8, kVrInnerSqDefault);
            store<u8>(vr0 + kVrGroupCount, 0);
            store<u8>(vr0 + kVrBits, static_cast<u8>(b & 0x03));
        }
        const u32 base = load<u32>(peers);
        const u32 n1 = (load<u16>(peers + kPeersCount) + 1) & 0xFFFF;
        store<u16>(peers + kPeersCount, static_cast<u16>(n1));
        const u32 rec = base + n1 * obj4::kPeerSize - obj4::kPeerSize;  // wraps with the count, as in the original
        MarkDirtyInline(ctx, load<u32>(obj + kObjRoot));
        call(ctx, o4fn::WorldObj_UpdateLod, {load<u32>(obj + kObjRoot), 0, 0});
        call(ctx, kPeer_Bind, {rec, obj});

        const u32 kind = Kind(obj);
        u32 slot = 0;
        if (kind == 8 || kind == 10) {
            const u32 idx = load<u16>(obj + obj7::kObjSplineIndex);
            if (idx != 0xFFFF) slot = load<u32>(kSlotTableBase) + (idx << 7);
        }
        store<u32>(rec + kPrSlotEntry, slot);
        call(ctx, obj6::o6fn::Comps_Query, {rec, comps, 0});

        u32 vr = PeerVec(rec);
        const u32 vb = load<u8>(vr + kVrBits);
        bool shadow = (vb & 0x10) != 0;
        if (obj != 0 && ((load<u8>(obj + kObjBits321) >> 1) & 1) == 1) shadow = true;
        store<u8>(vr + kVrBits, static_cast<u8>((vb & ~0x10u) | (shadow ? 0x10 : 0)));
        u32 rb = (load<u8>(rec + kPrFlags) & ~kPrFlagGrouped) | (fl != 0 ? kPrFlagGrouped : 0);
        store<u8>(rec + kPrFlags, static_cast<u8>(rb));
        if (fl != 0) {
            if (prev != 0) {
                SetBit8(PeerVec(prev) + kVrBits, kPrFlagGroup, group);
                SetBit8(prev + kPrFlags, kPrFlagGroup, group);
                group = false;
                rb = load<u8>(rec + kPrFlags);
            }
            prev = rec;
        }
        group = group || (rb & kPrFlagGroup) != 0;

        if (load<u32>(obj + kObjParentLink) == 0 && load<u32>(obj + kObjFirst) == 0) {
            store<u32>(rec + kPrGroupObj, load<u32>(obj + kObjRoot));
            SetBit8(PeerVec(rec) + kVrBits, kPrFlagAttached, false);
            const u32 b = load<u8>(rec + kPrFlags) & ~kPrFlagAttached;
            store<u8>(rec + kPrFlags, static_cast<u8>(b));
            if ((b & kPrFlagGroup) != 0) store<u32>(rec + kPrMatrixSrc, obj + obj6::kObjLocal);
        } else {
            store<u32>(rec + kPrMatrixSrc, obj + obj6::kObjLocal);
            if (fl != 0) {
                store<u32>(rec + kPrGroupObj, load<u32>(obj + kObjRoot));
                const u32 extra = (fl - 1) & 0xFF;
                store<u8>(PeerVec(rec) + kVrGroupCount, static_cast<u8>(extra));
                u32 b;
                if (attach_all != 0) {
                    SetBit8(PeerVec(rec) + kVrBits, kPrFlagAttached, true);
                    b = load<u8>(rec + kPrFlags) | kPrFlagAttached;
                    store<u8>(rec + kPrFlags, static_cast<u8>(b));
                } else {
                    SetBit8(PeerVec(rec) + kVrBits, kPrFlagAttached, extra != 0);
                    b = (load<u8>(rec + kPrFlags) & ~kPrFlagAttached) | (extra != 0 ? kPrFlagAttached : 0);
                    store<u8>(rec + kPrFlags, static_cast<u8>(b));
                }
                const u32 p = load<u32>(obj + kObjParentLink);
                if ((b & kPrFlagAttached) == 0 && p != 0 && load<u32>(p + kObjParentLink) == 0 &&
                    load<u32>(obj + kObjFirst) == 0 && fl == 1 && attach_all == 0) {
                    // The record's matrix = the local matrix moved into the parent's world.
                    Vec4 pw[4], l[4], out[4];
                    MatLoad(p + obj6::kObjWorld, pw);
                    MatLoad(obj + obj6::kObjLocal, l);
                    MatCombine(pw, l, out);
                    MatStore(rec + kPrMatrix, out);
                    store<u32>(rec + kPrMatrixSrc, obj + obj6::kObjWorld);
                }
            }
        }

        if (load<u32>(rec + kPrFollow) != 0) {
            const u32 f = load<u32>(rec + kPrFollow);
            call(ctx, o4fn::WorldObj_UpdateLod, {load<u32>(load<u32>(f + kFwSpline) + kObjRoot), 0, 0});
            const u32 fo = load<u32>(load<u32>(rec + kPrFollow) + kFwSpline);
            const u32 pos = rec + kPrMatrix + 48;
            call(ctx, kListener_LocalDirection, {sp + kSpDir, pos, fo + obj6::kObjWorld});
            const f64 t = callf(ctx, kSpline_Nearest, {load<u32>(fo + kObjClassPtr) + obj7::kSplineDefData, sp + kSpDir, 0, 0}, {});
            const u32 f2 = load<u32>(rec + kPrFollow);
            u32 so = load<u32>(f2 + kFwSpline);
            store<f32>(f2 + kFwParam, static_cast<f32>(t));
            // r4 is left over from the last call (not read).
            callf(ctx, kSpline_PointAt, {load<u32>(so + kObjClassPtr) + obj7::kSplineDefData, ctx.gpr[4], sp + kSpA, 0},
                  {static_cast<f64>(zero)});
            TransformPoint(so, sp + kSpA);
            so = load<u32>(load<u32>(rec + kPrFollow) + kFwSpline);
            // r4 = so + 128 here in the original (the transform's last row; not read).
            callf(ctx, kSpline_PointAt, {load<u32>(so + kObjClassPtr) + obj7::kSplineDefData, so + 128, sp + kSpB, 0}, {t});
            TransformPoint(so, sp + kSpB);
            const auto sub = [](u32 a, u32 b) {
                return static_cast<f32>(static_cast<f64>(load<f32>(a)) - static_cast<f64>(load<f32>(b)));
            };
            const f32 x = sub(pos, sp + kSpB), y = sub(pos + 4, sp + kSpB + 4), z = sub(pos + 8, sp + kSpB + 8);
            store<f32>(pos, x);
            store<f32>(pos + 4, y);
            store<f32>(pos + 8, z);
        }
        if (first == 0) first = rec;
        // r4 is left over from the last call (not read).
        callf(ctx, kPeer_GetMatrix, {rec, ctx.gpr[4], sp + kSpMatrix}, {static_cast<f64>(zero)});
        const u32 copy = static_cast<u32>(ctx.gpr[3]);
        count = load<u32>(col + obj5::kColCount);
        const u32 dst = load<u32>(rec + kPrMatrixSrc);
        if (copy != 0) {
            for (u32 row = 0; row < 64; row += 16)
                for (u32 k = 0; k < 12; k += 4) store<u32>(dst + row + k, load<u32>(sp + kSpMatrix + row + k));
        } else {
            Vec4 r[4], m[4], out[4];
            MatLoad(rec + kPrMatrix, r);
            MatLoad(sp + kSpMatrix, m);
            MatCombine(r, m, out);
            MatStore(dst, out);
        }
        i = i + 1;
        if (i >= count) break;
    }

    if (prev != 0) {
        SetBit8(PeerVec(prev) + kVrBits, kPrFlagGroup, group);
        SetBit8(prev + kPrFlags, kPrFlagGroup, group);
        count = load<u32>(col + obj5::kColCount);
    }
    ctx.gpr[3] = count;
    if (first == 0) return;
    u32 vf = PeerVec(first);
    if (count == 0) return;
    for (u32 j = 0;;) {
        if ((load<u8>(first + kPrFlags) & kPrFlagGrouped) != 0 && load<u32>(first + kPrGroupObj) != 0)
            call(ctx, kWorld_AddGroup, {world, load<u32>(first + kPrGroupObj)});
        const s32 n = static_cast<s32>(load<u8>(vf + kVrGroupCount)) + 1;
        bool none = true;
        for (s32 k = 0; k < n; ++k) {
            const u32 r = first + static_cast<u32>(k) * obj4::kPeerSize;
            if (load<u32>(r + kPrFollow) != 0 || (load<u32>(r + kPrOwned0) != 0 && load<u32>(r + kPrOwned1) != 0)) none = false;
        }
        store<u8>(vf + kVrBits, static_cast<u8>((load<u8>(vf + kVrBits) & ~0x0Cu) | (none ? 0x08 : 0) | (none ? 0 : 0x04)));
        j += static_cast<u32>(n);
        vf += static_cast<u32>(n) * 32;
        first += static_cast<u32>(n) * obj4::kPeerSize;
        count = load<u32>(col + obj5::kColCount);
        ctx.gpr[3] = count;
        if (j >= count) break;
    }
}
