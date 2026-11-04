// World_HandleEvent callees, part 9e: the track set update.
#include "hevent9.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent9;

namespace sly4 {
namespace hevent9e {
namespace efn {
constexpr u32 Group_Free31DB24 = 0x0031DB24;   // (Item*) before its 44-byte block is freed (name?)
constexpr u32 Item_Update31DCC0 = 0x0031DCC0;  // (Item*; f32 dt) -> still running (name?)
constexpr u32 Tracks_Reset320190 = 0x00320190; // (Set*, u8) (name?)
constexpr u32 Tracks_Advance320708 = 0x00320708;  // (Set*; f32, u8) (r4 unused) (name?)
constexpr u32 Track_Start3208B0 = 0x003208B0;  // (Set*, Track*, s32 pending, u8) (name?)
constexpr u32 PtrVector_Clear = 0x0054EC1C;
constexpr u32 Nop = 0x0054EC18;
constexpr u32 Mem_FreeFrom = 0x0054EE6C;       // (p, pool)
}  // namespace efn

// The set: +2 u16 count, +8 {key, Track*}[], +60 f32, +64 f32 time scale,
// +68 flags (bit 31: reset pending, bit 30: its argument, bit 29: one more
// update while paused).
constexpr u32 kSetCount = 2, kSetEntries = 8, kSet60 = 60, kSetScale = 64, kSetFlags = 68;
constexpr u32 kFlagReset = 0x80000000u, kFlagResetArg = 0x40000000u, kFlagStep = 0x20000000u;
// Track: a ring of group pointers, +0 first slot, +4 end, +8 tail, +12
// head, +16 s32 target, +20 s32 count, +28 flags (bit 31: keep going).
constexpr u32 kRingFirst = 0, kRingEnd = 4, kRingTail = 8, kRingHead = 12, kTrackTarget = 16, kTrackCount = 20;
constexpr u32 kTrackFlags = 28;
// Group: PtrVector {+0 Item**, +8 u16 count}; items are 44-byte blocks.
constexpr u32 kItemSize = 44;
constexpr u32 kZero = 0x00320E64;  // f32 0.0
}  // namespace hevent9e
}  // namespace sly4

using namespace sly4::hevent9e;

namespace {
// srawi rX, (a - b), 31: all ones when a - b is negative (as s32).
inline u32 neg_mask(u32 a, u32 b) { return static_cast<u32>(static_cast<s32>(a - b) >> 31); }

// The slot after p in the track's ring (wrapping at +4 to +0), branch-free
// as in the original.
inline u32 ring_next(u32 t, u32 p) {
    const u32 n = p + 4, m = neg_mask(n, load<u32>(t + kRingEnd));
    return (n & m) | (load<u32>(t + kRingFirst) & ~m);
}
}  // namespace

// void Tracks_Update320E68(Set* s, f32 dt) (name?): unless the time scale
// (+64) is <= 0 without the one-step flag (bit 29, cleared here), runs every
// track's groups from the head to the tail with dt * scale; a group whose
// items all report done is freed (items: 0x0031DB24, Clear, 44-byte free;
// the group: Clear, Mem_FreeFrom) and the head popped. Then a pending reset
// (bit 31, its argument in bit 30) is applied; every track with a pending
// count gets 0x003208B0, and the first one without +28 bit 31 ends the
// update; else, when +60 >= 0, 0x00320708 with the scaled dt. r3 / f1 as
// the original leaves them.
DECOMP(00320E68, Tracks_Update320E68) {
    const u32 s = arg<u32>(ctx, 0);
    const f64 zero = lfs_f64(kZero);
    f64 t = lfs_f64(s + kSetScale);
    if (t <= zero && (load<u32>(s + kSetFlags) & kFlagStep) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(s + kSetFlags, load<u32>(s + kSetFlags) & ~kFlagStep);
    t = fmuls(t, farg(ctx, 0));

    for (s32 i = 0; i < static_cast<s32>(load<u16>(s + kSetCount)); ++i) {
        const u32 tr = load<u32>(load<u32>(s + kSetEntries) + static_cast<u32>(i) * 8 + 4);
        if (tr == 0) continue;
        u32 p = load<u32>(tr + kTrackCount) != 0 ? load<u32>(tr + kRingHead) : 0;
        while (p != 0) {
            u32 next = ring_next(tr, p);
            const u32 tail = load<u32>(tr + kRingTail);
            next &= neg_mask(next, tail) | neg_mask(tail, next);  // 0 at the tail
            bool done = true;
            for (u32 j = 0; j < load<u16>(load<u32>(p) + 8); ++j) {
                const u32 item = load<u32>(load<u32>(load<u32>(p)) + j * 4);
                if (item == 0) continue;
                callf(ctx, efn::Item_Update31DCC0, {item}, {t});
                if (static_cast<u32>(ctx.gpr[3]) != 0) done = false;  // r3, not f1
            }
            if (done) {
                u32 g;
                for (u32 k = 0;; ++k) {
                    g = load<u32>(p);
                    if (!(k < load<u16>(g + 8))) break;
                    const u32 item = load<u32>(load<u32>(g) + k * 4);
                    if (item != 0) {
                        call(ctx, efn::Group_Free31DB24, {item});
                        call(ctx, efn::PtrVector_Clear, {item});
                        call(ctx, efn::Nop, {item});
                        call(ctx, h7fn::SmallFree, {item, kItemSize});
                    }
                }
                if (g != 0) {
                    call(ctx, efn::PtrVector_Clear, {g});
                    call(ctx, efn::Nop, {g});
                    call(ctx, efn::Mem_FreeFrom, {g, 0});
                }
                const u32 head = load<u32>(tr + kRingHead);
                store<u32>(tr + kTrackCount, load<u32>(tr + kTrackCount) - 1);
                store<u32>(tr + kRingHead, ring_next(tr, head));
            }
            p = next;
        }
    }

    u32 flag = 0;
    const u32 fl = load<u32>(s + kSetFlags);
    if ((fl & kFlagReset) != 0) {
        flag = (fl & kFlagResetArg) != 0 ? 1 : 0;
        store<u32>(s + kSetFlags, fl & ~(kFlagReset | kFlagResetArg));
        call(ctx, efn::Tracks_Reset320190, {s, flag});
        // (Set*, f32 0.0, u8): the float takes r4's slot (left as is).
        callf(ctx, efn::Tracks_Advance320708, {s, ctx.gpr[4], flag}, {zero});
    }
    for (s32 i = 0;; ++i) {
        const u32 n = load<u16>(s + kSetCount);
        if (!(i < static_cast<s32>(n))) {
            ctx.gpr[3] = n;
            break;
        }
        const u32 tr = load<u32>(load<u32>(s + kSetEntries) + static_cast<u32>(i) * 8 + 4);
        if (tr == 0) continue;
        const s32 pending = static_cast<s32>(load<u32>(tr + kTrackTarget) - load<u32>(tr + kTrackCount));
        if (pending > 0) call(ctx, efn::Track_Start3208B0, {s, tr, static_cast<u32>(pending), flag});
        if ((load<u32>(tr + kTrackFlags) & 0x80000000u) == 0) {
            ctx.gpr[3] = 0;
            return;
        }
    }
    const f64 v = lfs_f64(s + kSet60);
    ctx.fpr[1] = v;  // f1 as the original leaves it
    if (v > zero || v == zero) callf(ctx, efn::Tracks_Advance320708, {s, ctx.gpr[4], 0}, {t});
}
