// Game code, medium sweep part 31: effect-slot binding and small list /
// lookup helpers.
#include "mid31.h"

using namespace sly4;
using namespace sly4::mid31;

namespace {
using namespace m31fn;
constexpr u32 HashStr31 = 0x0056A710u;  // String_HashNoCase(str)
}  // namespace

// bool f(Bind* o, Slot* def): binds o's effect +44 to a slot once (u8 +41
// set; nothing when +40 and +41 are both set already). The name +8 picks
// the slot of the owner (((+4)->0)->+32)->+76: +540 for 0x04B59DBC /
// 0x0028A808, +612 for 0x044DC6AB, else def. With a slot and an effect:
// +48 = slot, the slot's old effect +52 releases it (Effect_Release); in
// the slot's holder +56, the matching row i (+900 + 4i, i < 3) passes the
// effect to its controllers +916 + 8i (only when holder +432 is 1) and
// +920 + 8i (vslot 128(c, e, 1)). r3 = 0 after a slot was taken, else 1.
DECOMP(002FE230, EffectSlot_Bind_002FE230) {
    const u32 o = arg<u32>(ctx, 0);
    u32 slot = arg<u32>(ctx, 1);
    if (load<u8>(o + 40) != 0 && load<u8>(o + 41) != 0) {
        ret(ctx, 1u);
        return;
    }
    store<u8>(o + 41, 1);
    const u32 s = o + 8;
    auto owner = [&]() { return load<u32>(load<u32>(load<u32>(load<u32>(o + 4)) + 32) + 76); };
    if (static_cast<u32>(call(ctx, HashStr31, {s})) == 0x04B59DBCu) slot = owner() + 540;
    else if (static_cast<u32>(call(ctx, HashStr31, {s})) == 0x044DC6ABu) slot = owner() + 612;
    else if (static_cast<u32>(call(ctx, HashStr31, {s})) == 0x0028A808u) slot = owner() + 540;
    const u32 e = load<u32>(o + 44);
    if (slot == 0 || e == 0) {
        ret(ctx, 1u);
        return;
    }
    store<u32>(o + 48, slot);
    const u32 old = load<u32>(slot + 52);
    if (old != 0) call(ctx, 0x004CF3C4u, {old, e});  // Effect_Release
    const u32 y = load<u32>(slot + 56);
    if (y != 0) {
        for (u32 i = 0; i < 3; ++i) {
            if (load<u32>(y + 900 + 4 * i) != slot) continue;
            if (load<u32>(y + 432) == 1) {
                const u32 c = load<u32>(y + 916 + 8 * i);
                if (c != 0) VCall(ctx, c, 128, {c, e, 1});
            }
            const u32 c = load<u32>(y + 920 + 8 * i);
            if (c != 0) VCall(ctx, c, 128, {c, e, 1});
            break;
        }
    }
    ret(ctx, 0u);
}

// void f(T* o): o's six controllers +752 / +764 (three each, read in
// pairs) get vslot 28(c, 1); a pending object +896 is pushed on the
// u16-counted vector +900 (capacity u16 +904 doubled by PtrVector_Reserve,
// high-water u16 +910) and +896 cleared; then the vector is emptied from the
// back: each object gets vslot 40(x, 0) and, with +172 bit 0x40000, vslot
// 40(x, 1); count u16 +908 = 0 at the end (also r3).
DECOMP(00304298, Obj_FlushPending_00304298) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 i = 0; i < 3; ++i) {
        const u32 a = load<u32>(o + 752 + 4 * i);
        if (a != 0) VCall(ctx, a, 28, {a, 1});
        const u32 b = load<u32>(o + 764 + 4 * i);
        if (b != 0) VCall(ctx, b, 28, {b, 1});
    }
    u32 v = load<u32>(o + 896);
    if (v != 0) {
        u32 c = load<u16>(o + 908);
        const u32 cap = load<u16>(o + 904);
        if (c == cap) {
            call(ctx, 0x00551718u, {o + 900, (cap << 1) & 0x1FFFEu});  // PtrVector_Reserve
            v = load<u32>(o + 896);
            c = load<u16>(o + 908);
        }
        const u32 base = load<u32>(o + 900);
        const u32 c2 = load<u16>(o + 908);
        const u32 hw0 = load<u16>(o + 910);
        store<u32>(base + ((c << 2) & 0x3FFFCu), v);
        const u32 n = c2 + 1;
        store<u16>(o + 908, static_cast<u16>(n));
        store<u16>(o + 910, static_cast<u16>(static_cast<s32>(hw0) > static_cast<s32>(n & 0xFFFF) ? hw0 : n));
    }
    store<u32>(o + 896, 0);
    for (;;) {
        const u32 cnt = load<u16>(o + 908);
        if (cnt == 0) break;
        const u32 at = (cnt - 1) << 2;
        u32 x = load<u32>(load<u32>(o + 900) + at);
        VCall(ctx, x, 40, {x, 0});
        x = load<u32>(load<u32>(o + 900) + at);
        if ((load<u32>(x + 172) & 0x40000u) != 0) VCall(ctx, x, 40, {x, 1});
        const u32 base = load<u32>(o + 900);
        const u32 c = load<u16>(o + 908);
        const u32 idx = c - 1;
        store<u16>(o + 908, static_cast<u16>(idx));
        if (idx != (idx & 0xFFFF)) store<u32>(base + (idx << 2), load<u32>(base + ((idx << 2) & 0x3FFFCu)));  // only when the count was re-read as 0
    }
    store<u16>(o + 908, 0);
    ret(ctx, 0u);
}

// void f(T* o; f32 dt): a power meter tick (o +384 bit 0x80000000 set, u16
// +536 zero, else nothing). p = the player record ((*0x00F65CA0) + 4208 +
// 0x20C80 * (s8) u8 +172), its energy f32 +108; boost = flag 0x100 of the
// mode row (0x010B19E0 + 11968 * u8 p +1) +184. The exhausted flag
// 0x80000000 of +564 clears once the energy is <= 0 again and, unless
// boost, sets when it reaches def (+216) +940. The active state f32 +556
// (0 / 1) turns on when not exhausted and (being already on, or the time
// +560 has passed def +932) or boost holds, and drops when it was on and
// def +936 is below +560 (clearing the exhausted flag). While on, energy
// -= def +924 * dt (+ def +920 on the first tick; floored at 0; p +0 |=
// 0x40) and +560 += dt; off, +560 = 0. Turning on / off sends component
// 23 (0x000D1288(o, 23, 0)) Obj_SetField20_00276558(c, 6 / 0).
DECOMP(002428E0, PowerMeter_Tick_002428E0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    if ((load<u32>(o + 384) & 0x80000000u) == 0 || load<u16>(o + 536) != 0) {
        ret(ctx, o);
        return;
    }
    constexpr u32 K = 0x002428C8u;  // f32 0, f64 0 at +8, f32 1 at +16
    const u32 zw = load<u32>(K);
    const f64 z = LfsF(zw);
    const u32 p = load<u32>(0x00F65CA0u) + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(o + 172))) * 0x20C80) + 4208;
    const u32 d = load<u32>(o + 216);
    const bool boost = (load<u32>(0x010B19E0u + load<u8>(p + 1) * 11968u + 176 + 8) & 0x100u) != 0;
    u32 fl = load<u32>(o + 564);
    const bool ex0 = (fl & 0x80000000u) != 0;
    if (ex0 && (Lf(p + 108) < z || Lf(p + 108) == z)) {
        fl &= 0x7FFFFFFFu;
        store<u32>(o + 564, fl);
    } else if (!ex0 && !boost) {
        const f64 lim = Lf(d + 940);
        if (lim < Lf(p + 108) || lim == Lf(p + 108)) {
            fl |= 0x80000000u;
            store<u32>(o + 564, fl);
        }
    }
    const f64 t0 = Lf(o + 556);
    bool on;  // the new state is 1 (else 0)
    bool drop = false;
    if ((fl & 0x80000000u) != 0) {
        if (!(t0 == z) && Lf(d + 936) > z && Lf(d + 936) < Lf(o + 560)) drop = true;
    }
    if (drop) {
        fl &= 0x7FFFFFFFu;
        store<u32>(o + 564, fl);
        on = false;
    } else if ((fl & 0x80000000u) == 0) {
        on = false;
    } else {
        on = (!(t0 == z) && Lf(d + 932) > Lf(o + 560)) || boost;
    }
    const u32 tw = on ? load<u32>(K + 16) : zw;
    store<u32>(o + 556, tw);
    const f64 t1 = LfsF(tw);
    f64 f1 = z;
    u32 f1w = zw;
    if (!(t1 == z)) {
        f64 base = z;
        const f64 rate = Lf(d + 924);
        if (t0 == z) base = Lf(d + 920);
        const f64 use = Fmadds(rate, dt, base);
        const f64 e = Sgl28(Lf(p + 108) - use);
        store<u32>(p + 0, load<u32>(p + 0) | 0x40u);
        store<u32>(p + 108, W32(static_cast<f32>(Sgl28(Fsel28(-e, Ld30(K + 8), e)))));
        f1 = Sgl28(Lf(o + 560) + dt);
        f1w = W32(static_cast<f32>(f1));
    }
    store<u32>(o + 560, f1w);
    ctx.fpr[1] = f1;
    const bool wasOn = !(t0 == z), isOn = !(Lf(o + 556) == z);
    if (wasOn == isOn) {
        ret(ctx, o);
        return;
    }
    const u32 c = static_cast<u32>(call(ctx, 0x000D1288u, {o, 23, 0}));
    if (c != 0) call(ctx, 0x00276558u, {c, isOn ? 6u : 0u});  // Obj_SetField20_00276558
}
