// Game code, large-function sweep part 11: player HUD counters (lives icons,
// the collected-item counter).
#include "big11.h"
#include "big12.h"

namespace sly4::obj {

namespace {
inline be_u32& W(u32 base, u32 off) { return gm<be_u32>(base + off); }
inline f64 F(u32 ea) { return LfsW(gm<be_u32>(ea)); }
inline void St(u32 ea, f64 v) { gm<be_u32>(ea) = StfsBits(v); }
inline void CopyW(u32 dst, u32 src) { gm<be_u32>(dst) = gm<be_u32>(src).get(); }
}  // namespace

namespace {
constexpr u32 kFindByHash = 0x005801DCu;   // WorldObj_FindByHash (hash, 0)
constexpr u32 kWorldPtr = 0x00F65CA0u;
constexpr u32 kHudPtr = 0x00F64CC0u;       // the counter HUD object
constexpr u32 kHudLock = 0x010390B0u;      // u8: counter display held
constexpr u32 kTierKeys = 0x00F6495Cu;     // u32[11]: per-tier keys (f_0016BA60)
constexpr u32 kAttachAll = 0x00364580u;    // Call_364408_If256 (hud)
constexpr u32 kAttach = 0x0036E714u;       // Obj_Attach264_0036E714 (hud, s32 i)
constexpr u32 kEnableN = 0x0036E9BCu;      // Obj_EnableFirstN_0036E9BC (hud, s32 n)

// Icon k (1..3) of a group: hidden (vfunc +40 (obj, 0, 0)) when n < k or
// when it has a +260 def whose +164 bit 0x8000 is clear, else shown (1, 0).
void Icon(u32 hash, u32 n, u32 k) {
    const u32 obj = call<u32>(kFindByHash, hash, 0);
    if (obj == 0) return;
    bool show = false;
    if (n >= k) {
        const u32 def = gm<be_u32>(obj + 260);
        show = def == 0 || (gm<be_u32>(def + 164) & 0x8000u) != 0;
    }
    vcall(host<u8>(obj), 40, show ? 1u : 0u, 0u);
}
}  // namespace

// u32 Player_AddScore(Player* p, u32 n) (name?): the lives count: +724 = n
// (capped at 3), the two groups of three icons (0x3704E0B1.. and
// 0x894F7ECF / 0x894F8290 / 0x894F8651) show the first n; outside the
// world's single-player mode (World +29 != 1) the other player's object
// (World +4208 + (1 - +772) * 0x20C80, +8) gets the same +724. r3 as the
// original leaves it.
void Player_AddScore16B5F0(u32 p, u32 n) {
    gm<be_u32>(p + 724) = n;
    if (n > 3) gm<be_u32>(p + 724) = 3;
    static constexpr u32 kIcons[6] = {0x3704E0B1u, 0x3704E0B2u, 0x3704E0B3u, 0x894F7ECFu, 0x894F8290u, 0x894F8651u};
    for (u32 i = 0; i < 6; ++i) Icon(kIcons[i], n, i % 3 + 1);
    const u32 world = gm<be_u32>(kWorldPtr);
    if (gm<u8>(world + 29) == 1) return;
    const u32 other = gm<be_u32>(world + (1 - gm<be_u32>(p + 772)) * 0x20C80u + 4208 + 8);
    if (other == 0) return;
    // (the original's alternative for World +29 == 1, a write to address
    // 724, is unreachable here)
    gm<be_u32>(other + 724) = gm<be_u32>(p + 724);
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
void Player_SetCount1753C0(u32 p, u32 n) {
    const u32 hud = gm<be_u32>(kHudPtr);
    u32 lo = 0, hi = 0;
    bool grew = false;
    const u32 old = gm<be_u32>(p + 660);
    const u32 per = gm<be_u32>(p + 664);
    if (n > old) {
        lo = old;
        hi = n;
        grew = true;
    }
    gm<be_u32>(p + 660) = n;
    u32 cur = n;
    if (!(n < per && gm<u8>(kHudLock) == 0)) {
        if (hud != 0) {
            u32 c = gm<be_u32>(p + 668);
            if (c + 1 < 10) {
                call(kAttachAll, hud);
                c = gm<be_u32>(p + 668);
            }
            c += 1;
            const u32 world = gm<be_u32>(kWorldPtr);
            gm<be_u32>(p + 668) = c;
            const u32 fx = gm<be_u32>(world + 0x1281F4u);
            if (fx != 0 && (gm<be_u32>(fx + 28) & 0x20u) != 0) {
                gm<u8>(fx + 261) = 1;
                c = gm<be_u32>(p + 668);
            }
            if (c > 10) {
                gm<be_u32>(p + 668) = 10;
            } else {
                Player_SetTierKey16BA60(p);  // (the key in r4 is not read)
                const u32 h = gm<be_u32>(p + 772) == 0 ? 0x9E447FFDu : 0xE6CC47D7u;
                Hud_ShowBanner36E568(hud, h, gm<be_u32>(p + 668));
            }
        }
        const u32 c = gm<be_u32>(p + 668);
        const bool top = c >= 10;
        if (gm<u8>(kHudLock) == 0 && !top)
            gm<be_u32>(p + 660) = gm<be_u32>(p + 660) - gm<be_u32>(p + 664);
        else if (!top)
            gm<be_u32>(p + 660) = 0;
        hi = gm<be_u32>(p + 660);
        const u32 tobj = gm<be_u32>(p + 532 + 4 * c);
        lo = 0;
        vcall(host<u8>(tobj), 8, p);
        call(kAttach, hud, static_cast<s32>(gm<be_u32>(p + 664) - 1));
        grew = false;
        Player_SetTierKey16BA60(p);  // (the key in r4 is not read)
        cur = gm<be_u32>(p + 660);
    }
    if (gm<be_u32>(p + 668) >= 10) {
        for (u32 k = 0; k < 6; ++k) call(kAttach, hud, k);
    } else if (cur == 0) {
        call(kAttachAll, hud);
    } else if (hi > lo) {
        do {
            if (hud != 0) call(kAttach, hud, static_cast<s32>(lo));
            ++lo;
        } while (hi > lo);
    }
    if (grew) {
        const u32 a = gm<be_u32>(p + 708), b = gm<be_u32>(p + 700);
        gm<be_u32>(p + 704) = a;
        gm<be_u32>(p + 696) = b;
        gm<be_u32>(p + 716) = 0x40800000u;
    }
    if (static_cast<s32>(gm<be_u32>(p + 668)) == 10)
        call(kEnableN, gm<be_u32>(kHudPtr), 5);
    else
        call(kEnableN, gm<be_u32>(kHudPtr), static_cast<s32>(gm<be_u32>(p + 660)));
}

}  // namespace sly4::obj

NATIVE(0016B5F0, sly4::obj::Player_AddScore16B5F0)
NATIVE(001753C0, sly4::obj::Player_SetCount1753C0)
