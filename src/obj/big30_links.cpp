// Game code, large-function sweep part 30: a section's link mode, which
// registers or forgets its keys in the 128-byte records of its holder.
#include "../sly4.h"
#include "big30.h"

using namespace sly4;
using namespace sly4::big30;

namespace {
constexpr u32 Obj_Notify24 = 0x000D949C;            // (holder) -> record owner or 0 (reads r3 only)
constexpr u32 Rec128_Alloc = 0x0029F760;            // (owner) -> Rec* or 0 (reads r3 only)
constexpr u32 Records_RemoveKey = 0x0029F7A0;       // (owner, key)
constexpr u32 kZeroVec = 0x01164B40;
constexpr u32 kRecTag = 0x427BF52D;

inline u32 Owner(PPUContext& ctx, u32 o) { return static_cast<u32>(call(ctx, Obj_Notify24, {load<u32>(load<u32>(o + 12) + 8)})); }

// A fresh record for key: +8 key, +16..24 the zero vector (raw), +40 the
// tag, +36 = 1, +120 bits 28 / 27 / 25 / 23 cleared and bit 26 set.
inline void AddKey(PPUContext& ctx, u32 owner, u32 key) {
    const u32 r = static_cast<u32>(call(ctx, Rec128_Alloc, {owner}));
    if (r == 0) return;
    const u32 w = load<u32>(r + 120);
    store<u32>(r + 8, key);
    store<u32>(r + 16, load<u32>(kZeroVec));
    store<u32>(r + 40, kRecTag);
    store<u32>(r + 36, 1);
    store<u32>(r + 20, load<u32>(kZeroVec + 4));
    store<u32>(r + 24, load<u32>(kZeroVec + 8));
    store<u32>(r + 120, (w & ~0x1A800000u) | 0x04000000u);
}
// Each key of the u32 list at o + base (u16 count at o + cnt, re-read):
// with an owner, added (add) or removed.
inline void ForList(PPUContext& ctx, u32 o, u32 base, u32 cnt, bool add) {
    for (u32 i = 0, off = 0; i < load<u16>(o + cnt); ++i, off += 4) {
        const u32 key = load<u32>(load<u32>(o + base) + off);
        const u32 w = Owner(ctx, o);
        if (w == 0) continue;
        if (add) AddKey(ctx, w, key);
        else call(ctx, Records_RemoveKey, {w, key});
    }
}
}  // namespace

// void f(Section* o, s32 mode, u8 force) (called by
// Section_ReleaseAll334114, Section_AcquireAll334B74, f_00334D88). Nothing
// when mode is the current +2676 and force is clear. The keys are the list
// A (+2652, u16 count +2660), the list B (+2632, count +2640) and the
// single key +2672; the record owner is Obj_Notify24(o->+12->+8), asked
// per key. Mode 0 removes A, B and the single key; 1 adds A and removes B
// and the single key; 2 removes A, adds B, removes the single key; 3 (only
// with a single key, read on entry) removes A and B and adds the single
// key. Then +2676 = mode (also for other modes). No caller reads r3 or
// other leftovers.
DECOMP(00333BC8, Section_SetLinkMode_00333BC8) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 mode = static_cast<s32>(ctx.gpr[4]);
    const u32 single = load<u32>(o + 2672);
    if (static_cast<u32>(mode) == load<u32>(o + 2676) && static_cast<u8>(ctx.gpr[5]) == 0) return;
    auto single_key = [&](bool add) {
        const u32 k = load<u32>(o + 2672);
        if (!add && k == 0) return;
        const u32 w = Owner(ctx, o);
        if (w == 0) return;
        if (add) AddKey(ctx, w, k);
        else call(ctx, Records_RemoveKey, {w, k});
    };
    switch (mode) {
    case 0:
        ForList(ctx, o, 2652, 2660, false);
        ForList(ctx, o, 2632, 2640, false);
        single_key(false);
        break;
    case 1:
        ForList(ctx, o, 2652, 2660, true);
        ForList(ctx, o, 2632, 2640, false);
        single_key(false);
        break;
    case 2:
        ForList(ctx, o, 2652, 2660, false);
        ForList(ctx, o, 2632, 2640, true);
        single_key(false);
        break;
    case 3:
        if (single == 0) break;
        ForList(ctx, o, 2652, 2660, false);
        ForList(ctx, o, 2632, 2640, false);
        single_key(true);
        break;
    default:
        break;
    }
    store<u32>(o + 2676, static_cast<u32>(mode));
}
