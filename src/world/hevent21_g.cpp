// World_HandleEvent callees, part 21g: the effect def switch by name hash
// (Effect_Release / Effect_SetHash4CF988) and a slot ring reset.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kEffectLastDef = 0x011DE2A8;   // u32 def found by the last effect set lookup (name?)
constexpr u32 kEffectInstance4BBABC = 0x004BBABC;  // (Def*, Effect*) -> instance (name?)
constexpr u32 kEffectSetTime = 0x004CF710;   // (Effect*; f32) (symbols.txt Effect_SetTime)
constexpr u32 kFrameTime = 0x004CF980;       // f32 0x3C88CE70 (~1/60) given to Effect_SetTime

constexpr u32 kPtrVectorClear = 0x0054EC1C;  // (PtrVector*) (as hevent.h)
constexpr u32 kNop = 0x0054EC18;             // (x) (symbols.txt Nop)
constexpr u32 kSmallFree = 0x00552420;       // (p, size) (as hevent7.h)
constexpr u32 kMemFreeFrom = 0x0054EE6C;     // (p, pool) (symbols.txt Mem_FreeFrom)

// The effect's def set (def +24, map +12 in the hevent6.h layout) lookup of
// hash: returns the def found (0 for none). Stores as the original: the
// current node, and on a hit the global 0x011DE2A8.
u32 effect_lookup(u32 def, u32 hash) {
    const u32 set = load<u32>(def + 24);
    if (set == 0) return 0;
    const u32 m = set + 12;
    const u32 root = load<u32>(load<u32>(m));
    if (root == 0) {
        store<u32>(m + hevent6::kMapNode, 0);
        store<u32>(kEffectLastDef, 0);
        return 0;
    }
    const u32 n = hevent6::map_walk(m, root, hash);
    if (n == 0) return 0;
    store<u32>(m + hevent6::kMapNode, n);
    store<u32>(kEffectLastDef, load<u32>(n + hevent6::kNodeValue));
    const u32 set2 = load<u32>(def + 24);
    if (set2 == 0) return 0;
    const u32 cur = load<u32>(set2 + 12 + hevent6::kMapNode);
    return cur != 0 ? load<u32>(cur + hevent6::kNodeValue) : 0;
}

// The shared body: finds the def for hash and, when there is one, releases
// the current instance (+4, vt +8) and makes a new one. +136 bit 0x80 is
// cleared in any case. Returns whether a def was found.
bool effect_switch(PPUContext& ctx, u32 e, u32 hash) {
    const u32 nd = effect_lookup(load<u32>(e), hash);
    const u32 inst = load<u32>(e + 4);
    store<u32>(e + 136, load<u32>(e + 136) & ~0x80u);
    if (nd == 0) {
        ret(ctx, 0);
        return false;
    }
    if (inst != 0) vcall(ctx, inst, 8, {inst});
    store<u32>(e + 4, static_cast<u32>(call(ctx, kEffectInstance4BBABC, {nd, e})));
    return true;
}
}  // namespace

// bool Effect_Release(Effect* e, u32 hash) (name? it switches the effect
// to the def named hash in its def's set): see effect_switch.
DECOMP(004CF3C4, Effect_Release) {
    const u32 e = arg<u32>(ctx, 0);
    if (effect_switch(ctx, e, arg<u32>(ctx, 1))) ret(ctx, 1);
}

// bool Effect_SetHash4CF988(Effect* e, u32 hash): as Effect_Release, and a
// switched effect is restarted with Effect_SetTime(e; 1/60).
DECOMP(004CF988, Effect_SetHash4CF988) {
    const u32 e = arg<u32>(ctx, 0);
    if (!effect_switch(ctx, e, arg<u32>(ctx, 1))) return;
    callf(ctx, kEffectSetTime, {e}, {hevent7::lfs_f64(kFrameTime)});
    ret(ctx, 1);
}

// u32 Ring_Reset31E040(Ring* r) (name?): a ring of block pointers (+0
// begin, +4 end, +12 read cursor, +20 s32 count): each pending block
// (a PtrVector of 44-byte entries) has its entries cleared and freed
// (PtrVector_Clear, Nop, SmallFree(e, 44)) and is then cleared and freed
// itself (Mem_FreeFrom); the cursor advances with wrap-around. Then head
// (+8) and cursor = begin, count 0, +28 bit 31 = the owner's (+24) u8 +12
// bit 0. Returns that byte.
DECOMP(0031E040, Ring_Reset31E040) {
    const u32 r = arg<u32>(ctx, 0);
    u32 slot = load<u32>(r + 20) != 0 ? load<u32>(r + 12) : 0;
    u32 begin = load<u32>(r);
    while (slot != 0) {
        u32 v;
        for (u32 i = 0;; ++i) {
            v = load<u32>(slot);
            const u32 n = load<u16>(v + 8);
            if (!(i < n)) break;
            const u32 p = static_cast<s32>(i) < static_cast<s32>(n) ? load<u32>(v) + 4 * i : 0;
            const u32 ent = load<u32>(p);
            if (ent != 0) {
                call(ctx, kPtrVectorClear, {ent});
                call(ctx, kNop, {ent});
                call(ctx, kSmallFree, {ent, 44});
            }
        }
        if (v != 0) {
            call(ctx, kPtrVectorClear, {v});
            call(ctx, kNop, {v});
            call(ctx, kMemFreeFrom, {v, 0});
        }
        const u32 next = load<u32>(r + 12) + 4;
        const u32 end = load<u32>(r + 4);
        begin = load<u32>(r);
        const u32 count = load<u32>(r + 20) - 1;
        const u32 cur = static_cast<s32>(next - end) < 0 ? next : begin;
        store<u32>(r + 20, count);
        store<u32>(r + 12, cur);
        slot = count != 0 ? cur : 0;
    }
    const u32 owner = load<u32>(r + 24);
    const u32 fl = load<u32>(r + 28);
    store<u32>(r + 8, begin);
    store<u32>(r + 12, begin);
    const u32 b = load<u8>(owner + 12);
    store<u32>(r + 20, 0);
    store<u32>(r + 28, (fl & 0x7FFFFFFFu) | ((b & 1u) << 31));
    ret(ctx, b);
}
