// World_HandleEvent callees, part 27d: episode completion percentage.
#include <cmath>

#include "hevent27.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent27;

namespace {
constexpr u32 kPlayerCount = 0x002DAC24;   // (Episode*, SaveSlot*) -> u32 (Player_Count2DAC24)
constexpr u32 kStringFormat = 0x005534D0;  // (fmt, ...) (String_Format)
constexpr u32 kFmtEpisodeIndex = 0x00B1D308;  // "Episode Index is out of bounds '%i'! Range is [0 - %i]! ..."
constexpr u32 kSession = 0x00F64C20;       // Session*: +20 860-byte level records (+724 id), +24 count
constexpr u32 kLevelRecSize = 860, kLevelRecId = 724;
// {key, Item*} arrays (u16 +2 count, +8 entries; item +0 u8 level index)
// of two collectible kinds.
constexpr u32 kItemsA = 0x0106819C, kItemsB = 0x01068184;
constexpr u32 kEpisodeIndexCount = 12;

f32 fadd(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) + static_cast<f64>(b)); }
f32 fdiv(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) / static_cast<f64>(b)); }
f32 fmul(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) * static_cast<f64>(b)); }
f32 fmadd(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(c), static_cast<f64>(b))); }
f32 u2f(u32 v) { return static_cast<f32>(static_cast<f64>(v)); }  // fcfid of the zero-extended value, frsp

// Index of the session's level record whose +724 id is key, or -1.
s32 find_level(u32 key) {
    const u32 g = load<u32>(kSession);
    const u32 n = load<u32>(g + 24);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(load<u32>(g + 20) + kLevelRecSize * i + kLevelRecId) == key) return static_cast<s32>(i);
    }
    return -1;
}
// Items of the array whose level index is lvl.
u32 count_items(u32 arr, s32 lvl) {
    u32 c = 0;
    for (s32 j = 0; j < static_cast<s32>(load<u16>(arr + 2)); ++j) {
        if (static_cast<s32>(load<u8>(load<u32>(load<u32>(arr + 8) + 8 * static_cast<u32>(j) + 4))) == lvl) ++c;
    }
    return c;
}
u32 popcount(u32 v) {
    u32 n = 0;
    for (; v != 0; v &= v - 1) ++n;
    return n;
}
}  // namespace

// ---------------------------------------------------------------- 0x002DB040
// f32 Episode_Percent2DB040(Episode* ep, SaveSlot* save) (name?): the
// weighted completion of an episode in percent: sum(weight * done /
// total) / sum(weight) * 100 over
//   - jobs (u8 +720 records of 536 bytes at +716, done when save +464
//     [record +502] is 2), weight +836;
//   - collectibles of kind A in the episode's level (0x0106819C), done =
//     Player_Count2DAC24, weight +840 (skipped without any);
//   - the u8 +821 items, done = the bits of save +52[+823] (+823 = 255:
//     none; 12 or more: logs an error, none), weight +844 (when +821 is
//     set);
//   - collectibles of kind B in the level (0x01068184; done = the bits of
//     the u16 save +124[+825], 255: none), weight +848, and +852 added to
//     the total weight (and to the sum too when save +416[+822] & 7 == 3).
// The sections without items add nothing.
DECOMP(002DB040, Episode_Percent2DB040) {
    const u32 ep = arg<u32>(ctx, 0), save = arg<u32>(ctx, 1);
    const f32 zero = 0.0f;  // lfs 0x002DB034
    f32 bonus = zero, total = zero, sum = zero;
    const u32 njobs = load<u8>(ep + 720);
    if (static_cast<s32>(njobs) > 0) {
        u32 done = 0;
        u32 rec = load<u32>(ep + 716);
        for (u32 i = 0; i < njobs; ++i, rec += 536) {
            if (load<u8>(save + load<u8>(rec + 502) + 464) == 2) ++done;
        }
        const f32 w = lf(ep + 836);
        total = fadd(total, w);
        sum = fmadd(fdiv(u2f(done), u2f(njobs)), w, sum);
    }
    if (count_items(kItemsA, find_level(load<u32>(ep + 724))) != 0) {
        const f32 got = u2f(static_cast<u32>(call(ctx, kPlayerCount, {ep, save})));
        const u32 all = count_items(kItemsA, find_level(load<u32>(ep + 724)));
        const f32 w = lf(ep + 840);
        total = fadd(total, w);
        sum = fmadd(fdiv(got, u2f(all)), w, sum);
    }
    const u32 nspecial = load<u8>(ep + 821);
    if (nspecial != 0) {
        const u32 idx = load<u8>(ep + 823);
        u32 got = 0;
        if (idx != 255) {
            if (idx < kEpisodeIndexCount) {
                got = popcount(load<u32>(save + 4 * idx + 52));
            } else {
                call(ctx, kStringFormat, {kFmtEpisodeIndex, idx, kEpisodeIndexCount});
            }
        }
        const u32 n = load<u8>(ep + 821);  // re-read after the log call (same value otherwise)
        const f32 w = lf(ep + 844);
        total = fadd(total, w);
        sum = fmadd(fdiv(u2f(got), u2f(idx < kEpisodeIndexCount || idx == 255 ? nspecial : n)), w, sum);
    }
    const u32 key = load<u32>(ep + 724);
    if (count_items(kItemsB, find_level(key)) != 0) {
        const u32 b = load<u8>(ep + 825);
        const u32 got = b != 255 ? popcount(load<u16>(save + 124 + ((b << 1) & 0x1FE))) : 0;
        const u32 all = count_items(kItemsB, find_level(key));
        const u32 kind = load<u16>(save + 4 * load<u8>(ep + 822) + 416) & 7;
        const f32 w = lf(ep + 848);
        const f32 r = fdiv(u2f(got), u2f(all));
        const f32 t = fadd(total, w);
        sum = fmadd(r, w, sum);
        if (kind == 3) bonus = lf(ep + 852);
        sum = fadd(sum, bonus);
        total = fadd(t, lf(ep + 852));
    }
    ctx.fpr[1] = fmul(fdiv(sum, total), 100.0f);  // lfs 0x002DB038
    ctx.gpr[3] = 0x002DB034;  // r3 is left holding the constants' address
}
