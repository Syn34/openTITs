// Game code, large-function sweep part 11: player HUD counters (lives icons,
// the collected-item counter).
#include "big11.h"

using namespace sly4;
using namespace sly4::big11;

namespace {
constexpr u32 kFindByHash = 0x005801DCu;   // WorldObj_FindByHash (hash, 0)
constexpr u32 kWorldPtr = 0x00F65CA0u;
constexpr u32 kHudPtr = 0x00F64CC0u;       // the counter HUD object
constexpr u32 kHudLock = 0x010390B0u;      // u8: counter display held
constexpr u32 kTierKeys = 0x00F6495Cu;     // u32[11]: per-tier keys (f_0016BA60)
constexpr u32 kSetTier = 0x0016BA60u;      // (player, key) (name?)
constexpr u32 kAttachAll = 0x00364580u;    // Call_364408_If256 (hud)
constexpr u32 kHudShow = 0x0036E568u;      // (hud, hash, s32 tier) (name?)
constexpr u32 kAttach = 0x0036E714u;       // Obj_Attach264_0036E714 (hud, s32 i)
constexpr u32 kEnableN = 0x0036E9BCu;      // Obj_EnableFirstN_0036E9BC (hud, s32 n)

// Icon k (1..3) of a group: hidden (vfunc +40 (obj, 0, 0)) when n < k or
// when it has a +260 def whose +164 bit 0x8000 is clear, else shown (1, 0).
void Icon(PPUContext& ctx, u32 hash, u32 n, u32 k) {
    const u32 obj = static_cast<u32>(call(ctx, kFindByHash, {hash, 0}));
    if (obj == 0) return;
    bool show = false;
    if (n >= k) {
        const u32 def = load<u32>(obj + 260);
        show = def == 0 || (load<u32>(def + 164) & 0x8000u) != 0;
    }
    VCall(ctx, obj, 40, {obj, show ? 1u : 0u, 0});
}
}  // namespace

// u32 Player_AddScore(Player* p, u32 n) (name?): the lives count: +724 = n
// (capped at 3), the two groups of three icons (0x3704E0B1.. and
// 0x894F7ECF / 0x894F8290 / 0x894F8651) show the first n; outside the
// world's single-player mode (World +29 != 1) the other player's object
// (World +4208 + (1 - +772) * 0x20C80, +8) gets the same +724. r3 as the
// original leaves it.
DECOMP(0016B5F0, Player_AddScore16B5F0) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 n = arg<u32>(ctx, 1);
    store<u32>(p + 724, n);
    if (n > 3) store<u32>(p + 724, 3);
    static constexpr u32 kIcons[6] = {0x3704E0B1u, 0x3704E0B2u, 0x3704E0B3u, 0x894F7ECFu, 0x894F8290u, 0x894F8651u};
    for (u32 i = 0; i < 6; ++i) Icon(ctx, kIcons[i], n, i % 3 + 1);
    const u32 world = load<u32>(kWorldPtr);
    if (load<u8>(world + 29) == 1) {
        ctx.gpr[3] = world;
        return;
    }
    const u32 other = load<u32>(world + (1 - load<u32>(p + 772)) * 0x20C80u + 4208 + 8);
    ctx.gpr[3] = other;
    if (other == 0) return;
    // (the original's alternative for World +29 == 1, a write to address
    // 724, is unreachable here)
    store<u32>(other + 724, load<u32>(p + 724));
}

// u32 Player_SetCount(Player* p, u32 n) (name?): the collected-item counter:
// +660 = n; when it reaches +664 per tier (unless the HUD is held) the tier
// +668 goes up (max 10; the HUD flashes, World +0x1281F4's +261 flag when
// its +28 bit 0x20 is set, tier key and banner 0x9E447FFD / 0xE6CC47D7 by
// +772), +660 drops by +664 (0 at the top tier), the tier object +532[tier]
// is told (vfunc +8) and the HUD slot +664 - 1 attached; then the HUD
// slots for the new items are attached (all six at the top tier, all when
// the counter is 0), the +704 / +696 animation restarted (+716 = 4) when n
// grew, and the HUD shows the first +660 (5 at tier 10).
DECOMP(001753C0, Player_SetCount1753C0) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 n = arg<u32>(ctx, 1);
    const u32 hud = load<u32>(kHudPtr);
    u32 lo = 0, hi = 0;
    bool grew = false;
    const u32 old = load<u32>(p + 660);
    const u32 per = load<u32>(p + 664);
    if (n > old) {
        lo = old;
        hi = n;
        grew = true;
    }
    store<u32>(p + 660, n);
    u32 cur = n;
    if (!(n < per && load<u8>(kHudLock) == 0)) {
        if (hud != 0) {
            u32 c = load<u32>(p + 668);
            if (c + 1 < 10) {
                call(ctx, kAttachAll, {hud});
                c = load<u32>(p + 668);
            }
            c += 1;
            const u32 world = load<u32>(kWorldPtr);
            store<u32>(p + 668, c);
            const u32 fx = load<u32>(world + 0x1281F4u);
            if (fx != 0 && (load<u32>(fx + 28) & 0x20u) != 0) {
                store<u8>(fx + 261, 1);
                c = load<u32>(p + 668);
            }
            if (c > 10) {
                store<u32>(p + 668, 10);
            } else {
                call(ctx, kSetTier, {p, load<u32>(kTierKeys + 4 * c)});
                const u32 h = load<u32>(p + 772) == 0 ? 0x9E447FFDu : 0xE6CC47D7u;
                call(ctx, kHudShow, {hud, h, Sx(load<u32>(p + 668))});
            }
        }
        const u32 c = load<u32>(p + 668);
        const bool top = c >= 10;
        if (load<u8>(kHudLock) == 0 && !top)
            store<u32>(p + 660, load<u32>(p + 660) - load<u32>(p + 664));
        else if (!top)
            store<u32>(p + 660, 0);
        hi = load<u32>(p + 660);
        const u32 tobj = load<u32>(p + 532 + 4 * c);
        lo = 0;
        VCall(ctx, tobj, 8, {tobj, p});
        call(ctx, kAttach, {hud, Sx(load<u32>(p + 664) - 1)});
        grew = false;
        call(ctx, kSetTier, {p, load<u32>(kTierKeys + 4 * load<u32>(p + 668))});
        cur = load<u32>(p + 660);
    }
    if (load<u32>(p + 668) >= 10) {
        for (u32 k = 0; k < 6; ++k) call(ctx, kAttach, {hud, k});
    } else if (cur == 0) {
        call(ctx, kAttachAll, {hud});
    } else if (hi > lo) {
        do {
            if (hud != 0) call(ctx, kAttach, {hud, Sx(lo)});
            ++lo;
        } while (hi > lo);
    }
    if (grew) {
        const u32 a = load<u32>(p + 708), b = load<u32>(p + 700);
        store<u32>(p + 704, a);
        store<u32>(p + 696, b);
        store<u32>(p + 716, 0x40800000u);
        SetF1Bits(ctx, a);
    }
    if (static_cast<s32>(load<u32>(p + 668)) == 10)
        call(ctx, kEnableN, {load<u32>(kHudPtr), 5});
    else
        call(ctx, kEnableN, {load<u32>(kHudPtr), Sx(load<u32>(p + 660))});
}
