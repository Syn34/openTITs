// Game code, large-function sweep part 12: set-entry selectors, HUD banner
// and vector helpers.
#include "big12.h"

namespace sly4::obj {

namespace {
inline be_u32& W(u32 base, u32 off) { return gm<be_u32>(base + off); }
inline f64 F(u32 ea) { return LfsW(gm<be_u32>(ea)); }
inline void St(u32 ea, f64 v) { gm<be_u32>(ea) = StfsBits(v); }
inline void CopyW(u32 dst, u32 src) { gm<be_u32>(dst) = gm<be_u32>(src).get(); }
constexpr u32 Effect_Start12 = 0x004C753C;         // (Effect*)
constexpr u32 Effect_SetTime12 = 0x004CF710;       // (Effect*; f32) (reads r3 / f1)
constexpr u32 Effect_New12 = 0x004CFB24;           // (u32 hash) -> Effect* or 0
constexpr u32 Effect_FindComponent12 = 0x004BB4B4; // (Effect*, u32 hash) -> component or 0
constexpr u32 Obj_EnableFirstN = 0x0036E9BC;       // (hud, s32 n)
constexpr u32 Acosf12 = 0x005E649C;                // (f1) -> f1
constexpr u32 Set_Find12 = 0x004BCE90;             // (u32 name) -> effect set
constexpr u32 kHud12 = 0x00F64CC0u;                // the HUD object
constexpr u32 kTierEffect = 0x00F6495Cu;           // u32 effect hash per tier
// The global effect set's entry for hash (Set_Find(0), then the select);
// the selected node or 0.
inline u32 SetSelectEntry(u32 hash) {
    const u32 set = call<u32>(Set_Find12, 0u);
    if (set == 0) return 0;
    if (EffectSetSelect(set, hash) == 0) return 0;
    return gm<be_u32>(set + 104);
}
// Effect_SetTime(e; the f32 at k), then bit 0x200 of e +136 set.
inline void EffectTimeFlag(u32 e, u32 k) {
    call(Effect_SetTime12, e, F(k));
    gm<be_u32>(e + 136) = gm<be_u32>(e + 136) | 0x200u;
}
}  // namespace

// void SetSelect_491000(?, u32 hash): selects the global set's entry for
// hash (SetSelectEntry); a found entry with a def (+8) gets an Effect built
// from it, started, timed (.text f32 at 0x00490FF8) and flagged 0x200.
// r3 (the first argument) is unread; no caller reads r3 or f1.
void SetSelect_491000(u32, u32 hash) {
    const u32 node = SetSelectEntry(hash);
    if (node == 0) return;
    const u32 def = gm<be_u32>(node + 8);
    if (def == 0) return;
    const u32 e = NewEffect140(def);
    call(Effect_Start12, e);
    EffectTimeFlag(e, 0x00490FF8u);
}

// void Hud_ShowBanner36E568(hud, u32 hash, s32 tier) (name?): for a non-zero
// hash, selects the set entry (SetSelectEntry); with a def it builds an
// Effect, stores tier at +20 of its component 0x044FA364 (when present),
// starts it and times it (.text f32 at 0x0036E560), flag 0x200. hud unread
// (no caller reads r3 or f1).
void Hud_ShowBanner36E568(u32, u32 hash, u32 tier) {
    if (hash == 0) return;
    const u32 node = SetSelectEntry(hash);
    if (node == 0) return;
    const u32 def = gm<be_u32>(node + 8);
    if (def == 0) return;
    const u32 e = NewEffect140(def);
    const u32 comp = call<u32>(Effect_FindComponent12, e, 0x044FA364u);
    if (comp != 0) gm<be_u32>(comp + 20) = tier;
    call(Effect_Start12, e);
    EffectTimeFlag(e, 0x0036E560u);
}

// void Player_SetTierKey16BA60(Player* p) (name?): with a HUD (kHud) whose
// +24 def has +164 bit 15 set, starts the tier effect (kTierEffect[p->668],
// timed by the .text f32 at 0x0016BA5C, flag 0x200) and enables the HUD's
// first p->660 items; otherwise HUD vfunc +20(1), the tier effect, then
// vfunc +20(0). Either way vfunc +20(0) again when the world's +31 is set
// (no caller reads r3 or f1).
void Player_SetTierKey16BA60(u32 p) {
    constexpr u32 kK = 0x0016BA5Cu;
    const u32 hud = gm<be_u32>(kHud12);
    if (hud == 0) return;
    const u32 def = gm<be_u32>(hud + 24);
    if (def != 0 && ((gm<be_u32>(def + 164) >> 15) & 1) != 0) {
        const u32 e = call<u32>(Effect_New12, gm<be_u32>(kTierEffect + 4 * gm<be_u32>(p + 668)));
        if (e != 0) EffectTimeFlag(e, kK);
        call(Obj_EnableFirstN, gm<be_u32>(kHud12), static_cast<s32>(gm<be_u32>(p + 660).get()));
    } else {
        vcall(host<u8>(hud), 20, 1);
        const u32 e = call<u32>(Effect_New12, gm<be_u32>(kTierEffect + 4 * gm<be_u32>(p + 668)));
        if (e != 0) EffectTimeFlag(e, kK);
        const u32 h = gm<be_u32>(kHud12);
        vcall(host<u8>(h), 20, 0);
    }
    if (gm<u8>(gm<be_u32>(kWorld) + 31) != 0) {
        const u32 h = gm<be_u32>(kHud12);
        vcall(host<u8>(h), 20, 0);
    }
}

// f32 Vec_SignedAngle16680(const vec3* a, const vec3* b) (name?): acosf of
// dot(a, b) clamped to [-1, 1] (by fsel), negated when dot(a, b x up) > 0
// (up = the vector at 0x01164B60); then one pi taken off above pi or added
// below -pi (the original's single-pi wrap). Constants: .text 0x00016658.
f64 Vec_SignedAngle16680(u32 a, u32 b) {
    constexpr u32 kK = 0x00016658u;   // f32 1, f32 0, f64 1, f32 -1, f64 -1 (+24), f32 pi (+32), f32 -pi (+36)
    constexpr u32 kUp = 0x01164B60u;
    f64 d = Muls(F(a + 4), F(b + 4));
    d = Fmadds(F(a + 0), F(b + 0), d);
    d = Fmadds(F(a + 8), F(b + 8), d);
    const f64 one = F(kK), mone = F(kK + 16), oneD = gm<be_f64>(kK + 8).get(), moneD = gm<be_f64>(kK + 24).get();
    f64 x = static_cast<f32>(Fsel(Subs(one, d), d, oneD));
    x = static_cast<f32>(Fsel(Subs(x, mone), x, moneD));
    x = static_cast<f32>(Fsel(Adds(x, one), x, moneD));
    x = static_cast<f32>(Fsel(Subs(x, one), oneD, x));
    f64 ang = call<f64>(Acosf12, x);
    const f64 bx = F(b), bz = F(b + 8), by = F(b + 4);
    const f64 u2 = F(kUp + 8), u1 = F(kUp + 4), u0 = F(kUp);
    const f64 c0 = Fmsubs(bz, u0, Muls(bx, u2));
    const f64 c1 = Fmsubs(by, u2, Muls(bz, u1));
    const f64 c2 = Fmsubs(bx, u1, Muls(by, u0));
    f64 s = Muls(F(a + 4), c0);
    s = Fmadds(F(a), c1, s);
    s = Fmadds(F(a + 8), c2, s);
    if (s > F(kK + 4)) ang = -ang;
    const f64 pi = F(kK + 32);
    if (ang > pi) ang = Subs(ang, pi);
    else if (ang < F(kK + 36)) ang = Adds(ang, pi);
    return ang;
}

}  // namespace sly4::obj

NATIVE(00491000, sly4::obj::SetSelect_491000)
NATIVE(0036E568, sly4::obj::Hud_ShowBanner36E568)
NATIVE(0016BA60, sly4::obj::Player_SetTierKey16BA60)
NATIVE(00016680, sly4::obj::Vec_SignedAngle16680)
