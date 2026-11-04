// World_HandleEvent callees, part 10l: the track ring reset.
#include <cmath>

#include "hevent10.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent8;
using namespace sly4::hevent10;

namespace sly4 {
namespace hevent10l {
namespace lfn10 {
constexpr u32 Group_Start31D740 = 0x0031D740;   // (Group*, Def*, Track*, -, u8 first, u8; f32 time) (name?)
constexpr u32 Keys_Find768C48 = 0x00768C48;     // (map, const key*) -> value* (name?)
constexpr u32 Body_Add931D10 = 0x00931D10;      // (world, body): Havok range (name?)
constexpr u32 Hash_ToString = 0x0056A6E0;
constexpr u32 Mem_Alloc = 0x0054ED38;          // (size, pool, align)
}  // namespace lfn10

// Track set (as hevent10_c.cpp): +2 u16 item count, +8 8-byte items {+4
// Track*}, +24 map key -> def list, +48 / +52 / +56 the current key.
// Track: a ring of Group* {+0 base, +4 end, +8 write, +12 read, +16 s32
// capacity, +20 s32 count, +24 -> name hash, +28 flags (bit 31: grown)}.
// Group: PtrVector of 44-byte groups {+0 data, +4 u16 cap, +8 u16 count,
// +10 u16 high-water mark}. Def: +20 / +24 / +28 / +32 ring {base, end,
// write, read} of Element*, +36 s32 keep, +40 s32 count, +136 Track*.
// Element: +20 32-byte records (+28 u16 count) {+16 Obj*}; Obj +416 body
// (+8 world); Obj vtable +304 (obj, u8 on).
constexpr u32 kTsItemCount = 2, kTsItems = 8, kTsKeys = 24, kTsOwner = 48, kTsGroup = 52, kTsKey = 56;
constexpr u32 kTrBase = 0, kTrEnd = 4, kTrWrite = 8, kTrRead = 12, kTrCap = 16, kTrCount = 20, kTrName = 24, kTrFlags = 28;
constexpr u32 kDefBase = 20, kDefEnd = 24, kDefWrite = 28, kDefRead = 32, kDefKeep = 36, kDefCount = 40, kDefTrack = 136;
constexpr u32 kGrSize = 44, kVecSize = 20;
constexpr u32 kWorldPhys = 0x1281D4;     // World +0x1281D4: physics (+4 hkpWorld*) (name?)
constexpr u32 kConsts = 0x00320188;      // f32 FLT_MAX, 0.5 (code segment)
}  // namespace hevent10l
}  // namespace sly4

using namespace sly4::hevent10l;

namespace {
u32 ring_next(u32 p, u32 end, u32 base) { return static_cast<s32>(p + 4 - end) < 0 ? p + 4 : base; }

// PtrVector push_back (inlined): doubles a full vector, then data[count++]
// = x and the high-water mark follows.
void push(PPUContext& ctx, u32 v, u32 x) {
    if (load<u16>(v + 8) == load<u16>(v + 4))
        call(ctx, h8fn::PtrVector_Reserve, {v, (static_cast<u32>(load<u16>(v + 4)) << 1) & 0xFFFFFFFEu});
    store<u32>(load<u32>(v) + (static_cast<u32>(load<u16>(v + 8)) << 2), x);
    const u32 n = static_cast<u32>(load<u16>(v + 8)) + 1;
    const u32 high = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(high) > static_cast<s32>(n & 0xFFFF) ? high : n));
}
}  // namespace

// void Tracks_Reset320190(Set* s, u8 flag) (name?): for every track of the
// set, finds the earliest finish (time - 0.5 * length) of its newest group
// and, for each def of the current key that belongs to the track, makes a
// 44-byte group: one element is taken from the def's ring (rotated back in
// unless the count reached the keep mark), its bodies added to the world
// and its objects switched on; 0x0031D740 starts the group, which is then
// appended to the track's newest group slot (a new slot, 20-byte vector,
// is pushed onto a track without one; a full ring logs its name hash and
// ends the reset). r3 / f1 as the original leaves them.
DECOMP(00320190, Tracks_Reset320190) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 flag = arg<u32>(ctx, 1) & 0xFF;
    f64 f1 = ctx.fpr[1];
    u64 r3 = 0;
    for (s32 i = 0;; ++i) {
        r3 = load<u16>(s + kTsItemCount);
        if (!(i < static_cast<s32>(r3))) break;
        const u32 t = load<u32>(load<u32>(s + kTsItems) + static_cast<u32>(i) * 8 + 4);
        if (t == 0) continue;
        u32 slot = 0;
        const u32 cnt = load<u32>(t + kTrCount);
        if (cnt != 0) {
            slot = load<u32>(t + kTrRead) + (cnt - 1) * 4;
            if (!(slot < load<u32>(t + kTrEnd))) slot -= load<u32>(t + kTrCap) * 4;
        }
        const u32 newest = slot != 0 ? load<u32>(slot) : 0;
        f64 best = lfs_f64(kConsts);
        if (newest != 0) {
            const f64 half = lfs_f64(kConsts + 4);
            for (u32 k = 0; k < load<u16>(newest + 8); ++k) {
                const u32 g = load<u32>(load<u32>(newest) + k * 4);
                const u32 src = load<u32>(g + 20);
                const f64 v = static_cast<f32>(-std::fma(lfs_f64(load<u32>(src + 132) + 44), half, -lfs_f64(g + 28)));
                f1 = v;
                if (v < best) best = v;
            }
        }
        const u32 grp = load<u32>(load<u32>(load<u32>(s + kTsOwner) + 8)) + load<u32>(s + kTsGroup) * 8;
        const u32 key = load<u32>(load<u32>(grp)) + load<u32>(s + kTsKey) * 8;
        const u32 defs = load<u32>(static_cast<u32>(call(ctx, lfn10::Keys_Find768C48, {s + kTsKeys, key})));
        for (u32 j = 0; j < load<u16>(defs + 8); ++j) {
            const u32 d = load<u32>(load<u32>(defs) + j * 4);
            if (d == 0 || load<u32>(d + kDefTrack) != t) continue;
            u32 gr = 0;
            const u32 blk = static_cast<u32>(call(ctx, h7fn::SmallAlloc, {kGrSize}));
            if (blk != 0) {
                call(ctx, hevent9::h9fn::PtrVector_Construct, {blk});
                store<u32>(blk + 20, 0);
                store<u32>(blk + 24, 0);
                store<u32>(blk + 28, 0);
                store<u32>(blk + 32, 0);
                store<u16>(blk + 6, 4);
                store<u32>(blk + 36, 0);
                store<u32>(blk + 40, 1);
                gr = blk;
            }
            const u32 c = load<u32>(d + kDefCount);
            if (c != 0) {
                const u32 rd = load<u32>(d + kDefRead);
                const u32 end = load<u32>(d + kDefEnd), base = load<u32>(d + kDefBase);
                store<u32>(d + kDefCount, c - 1);
                const bool keep = static_cast<s32>(c - 1) == static_cast<s32>(load<u32>(d + kDefKeep));
                store<u32>(d + kDefRead, ring_next(rd, end, base));
                const u32 popped = load<u32>(rd);
                if (!keep) {
                    // Rotate: the element goes back in at the write end.
                    const u32 wr = load<u32>(d + kDefWrite);
                    store<u32>(d + kDefCount, c);
                    store<u32>(d + kDefWrite, ring_next(wr, end, base));
                    if (wr != 0) store<u32>(wr, popped);
                }
                const u32 el = rd != 0 ? load<u32>(rd) : 0;
                if (el != 0) {
                    for (u32 k = 0; k < load<u16>(el + 28); ++k) {
                        const u32 rec = load<u32>(el + 20) + k * 32;
                        u32 obj = load<u32>(rec + 16);
                        const u32 body = load<u32>(obj + 416);
                        if (load<u32>(body + 8) == 0) {
                            call(ctx, lfn10::Body_Add931D10, {load<u32>(load<u32>(load<u32>(kCurrentWorld) + kWorldPhys) + 4), body});
                            obj = load<u32>(rec + 16);
                        }
                        vcall(ctx, obj, 304, {obj, 1u});
                    }
                    push(ctx, gr, el);
                }
            }
            // r6 is not an argument of 0x0031D740 (it reads r7 / r8).
            callf(ctx, lfn10::Group_Start31D740, {gr, d, t, 0u, newest == 0 ? 1u : 0u, flag}, {best});
            f1 = best;
            if (slot == 0) {
                const u32 tc = load<u32>(t + kTrCount);
                if (tc != load<u32>(t + kTrCap)) {
                    slot = load<u32>(t + kTrWrite);
                    const u32 end = load<u32>(t + kTrEnd);
                    const u32 base = load<u32>(t + kTrBase);
                    store<u32>(t + kTrCount, tc + 1);
                    store<u32>(t + kTrWrite, ring_next(slot, end, base));
                }
                if (slot == 0) {
                    ctx.gpr[3] = call(ctx, lfn10::Hash_ToString, {load<u32>(load<u32>(t + kTrName))});
                    ctx.fpr[1] = f1;
                    return;
                }
                u32 nv = 0;
                const u32 m = static_cast<u32>(call(ctx, lfn10::Mem_Alloc, {kVecSize, 0u, 16u}));
                if (m != 0) {
                    call(ctx, hevent9::h9fn::PtrVector_Construct, {m});
                    store<u16>(m + 6, 4);
                    nv = m;
                }
                store<u32>(slot, nv);
                store<u32>(t + kTrFlags, load<u32>(t + kTrFlags) | 0x80000000u);
            }
            push(ctx, load<u32>(slot), gr);
        }
    }
    ctx.gpr[3] = r3;
    ctx.fpr[1] = f1;
}

namespace {
// The newest group of track t (its ring's last slot), or 0.
u32 newest_group(u32 t) {
    const u32 cnt = load<u32>(t + kTrCount);
    if (cnt == 0) return 0;
    u32 slot = load<u32>(t + kTrRead) + (cnt - 1) * 4;
    if (!(slot < load<u32>(t + kTrEnd))) slot -= load<u32>(t + kTrCap) * 4;
    return slot != 0 ? load<u32>(slot) : 0;
}
}  // namespace

// void Track_Start3208B0(Set* s, Track* t, s32 pending, u8 flag) (name?):
// pending times (t non-null, pending > 0), for the defs of the current key
// that belong to t (a missing def list logs the key hash): as in
// Tracks_Reset320190, a 44-byte group gets one element from each def's
// ring and is started (0x0031D740 with the newest group's earliest finish
// and first = (t was empty or flag)); the groups of one pass go into a new
// 20-byte slot vector pushed onto t's ring (a full ring logs t's name hash
// and stops). r3 / f1 as the original leaves them.
DECOMP(003208B0, Track_Start3208B0) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 t = arg<u32>(ctx, 1);
    const s32 pending = static_cast<s32>(arg<u32>(ctx, 2));
    const u32 flag = arg<u32>(ctx, 3) & 0xFF;
    if (!(pending > 0 && t != 0)) {
        ctx.gpr[3] = s;
        return;
    }
    f64 f1 = ctx.fpr[1];
    const u32 grp = load<u32>(load<u32>(load<u32>(s + kTsOwner) + 8)) + load<u32>(s + kTsGroup) * 8;
    const u32 key = load<u32>(load<u32>(grp)) + load<u32>(s + kTsKey) * 8;
    const u32 defs = load<u32>(static_cast<u32>(call(ctx, lfn10::Keys_Find768C48, {s + kTsKeys, key})));
    if (defs == 0) {
        ctx.gpr[3] = call(ctx, lfn10::Hash_ToString, {load<u32>(key)});
        return;
    }
    const u32 first = (load<u32>(t + kTrCount) == 0 || flag != 0) ? 1u : 0u;
    u64 r3 = 0;
    for (s32 pass = 0; pass < pending; ++pass) {
        const u32 newest = newest_group(t);
        f64 best = lfs_f64(0x003208A4);
        // Unlike Tracks_Reset320190 this only checks the ring's count: a
        // null newest group is read through (count at 8, data at 0).
        if (load<u32>(t + kTrCount) != 0) {
            const f64 half = lfs_f64(0x003208A8);
            for (u32 k = 0; k < load<u16>(newest + 8); ++k) {
                const u32 g = load<u32>(load<u32>(newest) + k * 4);
                const u32 src = load<u32>(g + 20);
                const f64 v = static_cast<f32>(-std::fma(lfs_f64(load<u32>(src + 132) + 44), half, -lfs_f64(g + 28)));
                f1 = v;
                if (v < best) best = v;
            }
        }
        u32 slot = 0, vec = 0;
        for (u32 j = 0;; ++j) {
            r3 = load<u16>(defs + 8);
            if (!(j < r3)) break;
            const u32 d = load<u32>(load<u32>(defs) + j * 4);
            if (d == 0 || load<u32>(d + kDefTrack) != t) continue;
            u32 gr = 0;
            const u32 blk = static_cast<u32>(call(ctx, h7fn::SmallAlloc, {kGrSize}));
            if (blk != 0) {
                call(ctx, hevent9::h9fn::PtrVector_Construct, {blk});
                store<u32>(blk + 20, 0);
                store<u32>(blk + 24, 0);
                store<u32>(blk + 28, 0);
                store<u16>(blk + 6, 4);
                store<u32>(blk + 32, 0);
                store<u32>(blk + 36, 0);
                store<u32>(blk + 40, 1);
                gr = blk;
            }
            const u32 c = load<u32>(d + kDefCount);
            if (c != 0) {
                const u32 rd = load<u32>(d + kDefRead);
                const u32 end = load<u32>(d + kDefEnd), base = load<u32>(d + kDefBase);
                store<u32>(d + kDefCount, c - 1);
                const bool keep = static_cast<s32>(c - 1) == static_cast<s32>(load<u32>(d + kDefKeep));
                store<u32>(d + kDefRead, ring_next(rd, end, base));
                const u32 popped = load<u32>(rd);
                if (!keep) {
                    const u32 wr = load<u32>(d + kDefWrite);
                    store<u32>(d + kDefCount, c);
                    store<u32>(d + kDefWrite, ring_next(wr, end, base));
                    if (wr != 0) store<u32>(wr, popped);
                }
                const u32 el = rd != 0 ? load<u32>(rd) : 0;
                if (el != 0) {
                    for (u32 k = 0; k < load<u16>(el + 28); ++k) {
                        const u32 rec = load<u32>(el + 20) + k * 32;
                        u32 obj = load<u32>(rec + 16);
                        const u32 body = load<u32>(obj + 416);
                        if (load<u32>(body + 8) == 0) {
                            call(ctx, lfn10::Body_Add931D10, {load<u32>(load<u32>(load<u32>(kCurrentWorld) + kWorldPhys) + 4), body});
                            obj = load<u32>(rec + 16);
                        }
                        vcall(ctx, obj, 304, {obj, 1u});
                    }
                    push(ctx, gr, el);
                }
            }
            // r6 is not an argument of 0x0031D740 (it reads r7 / r8).
            callf(ctx, lfn10::Group_Start31D740, {gr, d, t, 0u, newest == 0 ? 1u : 0u, first}, {best});
            f1 = best;
            if (slot == 0) {
                const u32 tc = load<u32>(t + kTrCount);
                if (tc != load<u32>(t + kTrCap)) {
                    slot = load<u32>(t + kTrWrite);
                    const u32 end = load<u32>(t + kTrEnd);
                    const u32 base = load<u32>(t + kTrBase);
                    store<u32>(t + kTrCount, tc + 1);
                    store<u32>(t + kTrWrite, ring_next(slot, end, base));
                }
                if (slot == 0) {
                    ctx.gpr[3] = call(ctx, lfn10::Hash_ToString, {load<u32>(load<u32>(t + kTrName))});
                    ctx.fpr[1] = f1;
                    return;
                }
                vec = 0;
                const u32 m = static_cast<u32>(call(ctx, lfn10::Mem_Alloc, {kVecSize, 0u, 16u}));
                if (m != 0) {
                    call(ctx, hevent9::h9fn::PtrVector_Construct, {m});
                    store<u16>(m + 6, 4);
                    vec = m;
                }
                store<u32>(slot, vec);
                store<u32>(t + kTrFlags, load<u32>(t + kTrFlags) | 0x80000000u);
            }
            // The slot vector stays in a register for the rest of the pass.
            push(ctx, vec, gr);
        }
    }
    ctx.gpr[3] = r3;
    ctx.fpr[1] = f1;
}
