// Game code, large-function sweep part 12: set-entry selectors, HUD banner
// and vector helpers.
#include "big12.h"

using namespace sly4;
using namespace sly4::big12;

// void SetSelect_491000(?, u32 hash): selects the global set's entry for
// hash (SetSelectEntry); a found entry with a def (+8) gets an Effect built
// from it, started, timed (.text f32 at 0x00490FF8) and flagged 0x200.
// r3 (the first argument) is unread.
DECOMP(00491000, SetSelect_491000) {
    const u32 hash = arg<u32>(ctx, 1);
    u32 r3;
    const u32 node = SetSelectEntry(ctx, hash, r3);
    if (node == 0) { ctx.gpr[3] = r3; return; }
    const u32 def = load<u32>(node + 8);
    if (def == 0) { ctx.gpr[3] = r3; return; }
    const u32 e = NewEffect(ctx, def);
    call(ctx, b12fn::Effect_Start, {e});
    ctx.gpr[3] = EffectTimeFlag(ctx, e, 0x00490FF8u);
}

// void Hud_ShowBanner36E568(hud, u32 hash, s32 tier) (name?): for a non-zero
// hash, selects the set entry (SetSelectEntry); with a def it builds an
// Effect, stores tier at +20 of its component 0x044FA364 (when present),
// starts it and times it (.text f32 at 0x0036E560), flag 0x200. hud unread.
DECOMP(0036E568, Hud_ShowBanner36E568) {
    const u32 hash = arg<u32>(ctx, 1);
    const u32 tier = arg<u32>(ctx, 2);
    if (hash == 0) return;   // r3 = hud
    u32 r3;
    const u32 node = SetSelectEntry(ctx, hash, r3);
    if (node == 0) { ctx.gpr[3] = r3; return; }
    const u32 def = load<u32>(node + 8);
    if (def == 0) { ctx.gpr[3] = r3; return; }
    const u32 e = NewEffect(ctx, def);
    const u32 comp = static_cast<u32>(call(ctx, b12fn::Effect_FindComponent, {e, 0x044FA364u}));
    if (comp != 0) store<u32>(comp + 20, tier);
    call(ctx, b12fn::Effect_Start, {e});
    ctx.gpr[3] = EffectTimeFlag(ctx, e, 0x0036E560u);
}

// void Player_SetTierKey16BA60(Player* p) (name?): with a HUD (kHud) whose
// +24 def has +164 bit 15 set, starts the tier effect (kTierEffect[p->668],
// timed by the .text f32 at 0x0016BA5C, flag 0x200) and enables the HUD's
// first p->660 items; otherwise HUD vfunc +20(1), the tier effect, then
// vfunc +20(0). Either way vfunc +20(0) again when the world's +31 is set.
DECOMP(0016BA60, Player_SetTierKey16BA60) {
    const u32 p = arg<u32>(ctx, 0);
    constexpr u32 kK = 0x0016BA5Cu;
    const u32 hud = load<u32>(kHud);
    if (hud == 0) { ctx.gpr[3] = 0; return; }
    const u32 def = load<u32>(hud + 24);
    if (def != 0 && ((load<u32>(def + 164) >> 15) & 1) != 0) {
        const u32 e = static_cast<u32>(call(ctx, b12fn::Effect_New, {load<u32>(kTierEffect + 4 * load<u32>(p + 668))}));
        if (e != 0) EffectTimeFlag(ctx, e, kK);
        call(ctx, b12fn::Obj_EnableFirstN, {load<u32>(kHud), Sx(load<u32>(p + 660))});
    } else {
        VCall(ctx, hud, 20, {hud, 1});
        const u32 e = static_cast<u32>(call(ctx, b12fn::Effect_New, {load<u32>(kTierEffect + 4 * load<u32>(p + 668))}));
        if (e != 0) EffectTimeFlag(ctx, e, kK);
        const u32 h = load<u32>(kHud);
        VCall(ctx, h, 20, {h, 0});
    }
    ctx.gpr[3] = load<u8>(load<u32>(kWorld) + 31);
    if (ctx.gpr[3] != 0) {
        const u32 h = load<u32>(kHud);
        VCall(ctx, h, 20, {h, 0});
    }
}

// f32 Vec_SignedAngle16680(const vec3* a, const vec3* b) (name?): acosf of
// dot(a, b) clamped to [-1, 1] (by fsel), negated when dot(a, b x up) > 0
// (up = the vector at 0x01164B60); then one pi taken off above pi or added
// below -pi (the original's single-pi wrap). Constants: .text 0x00016658.
DECOMP(00016680, Vec_SignedAngle16680) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 b = arg<u32>(ctx, 1);
    constexpr u32 kK = 0x00016658u;   // f32 1, f32 0, f64 1, f32 -1, f64 -1 (+24), f32 pi (+32), f32 -pi (+36)
    constexpr u32 kUp = 0x01164B60u;
    f64 d = Muls(F(a + 4), F(b + 4));
    d = Fmadds(F(a + 0), F(b + 0), d);
    d = Fmadds(F(a + 8), F(b + 8), d);
    const f64 one = F(kK), mone = F(kK + 16), oneD = Lfd(kK + 8), moneD = Lfd(kK + 24);
    f64 x = static_cast<f32>(Fsel(Subs(one, d), d, oneD));
    x = static_cast<f32>(Fsel(Subs(x, mone), x, moneD));
    x = static_cast<f32>(Fsel(Adds(x, one), x, moneD));
    x = static_cast<f32>(Fsel(Subs(x, one), oneD, x));
    f64 ang = callf(ctx, b12fn::acosf, {}, {x});
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
    ctx.fpr[1] = ang;
    ctx.gpr[3] = kK;   // the original's last constant base
}
