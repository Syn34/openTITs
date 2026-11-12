// Game code, medium sweep part 33: two update steps of the object family
// driven by f_00056468 (both look up the local player: the world's
// +4208 slot +8 when its +192 bit 0x2 is set, else 0).
#include "mid33.h"

using namespace sly4;
using namespace sly4::mid33;

namespace {
using namespace m33fn;
// The local player (world +4208 +8 with +192 bit 0x2) or 0.
u32 LocalPlayer33() {
    const u32 p = load<u32>(load<u32>(kWorld33) + 4208 + 8);
    if (p != 0 && (load<u32>(p + 192) & 2u) != 0) return p;
    return 0;
}
}  // namespace

// Proximity trigger step (o; f1 dt): +6204 += dt * T(0x00F9ADDC); past
// T(0x00F9AEC0) the u8 +6208 is cleared. Otherwise the point o->+176 is
// pushed K[3] towards the player's position in xz (y = K[0], or the
// +224 owner's +4 +132 + K[1] unless +308 is 9) by the elapsed +6204
// (the push direction keeps K[2] as y); when the player is above it by
// more than K[4] but less than T(0x00F9AE28), and within T(0x00F9AE74)
// in xz (with y K[2]), the player is sent message 0x34F5E34C (slot 344)
// with the sender +60. K = the f32 table 0x000545C8, T(x) = x +72 (a
// tunable). Leaves r3 / f1 as the original does.
DECOMP(000545E0, Prox_Step_000545E0) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 192);
    const u32 sp = frame.addr();
    constexpr u32 K = 0x000545C8u;
    const u32 kind = load<u32>(o + 308);
    const f64 t0 = Lf(o + 6204);
    const u32 snd = load<u32>(o + 60);
    const u32 own = load<u32>(o + 224);
    u32 fl = load<u16>(sp + 126);  // never written before: the stack's leftover half-word
    const u32 pl = LocalPlayer33();
    const f64 t = Fmadds(farg(ctx, 0), Lf(0x00F9ADDCu), t0);
    store<f32>(o + 6204, static_cast<f32>(t));
    if (t > Lf(0x00F9AEC0u)) {
        store<u8>(o + 6208, 0);
        ctx.fpr[1] = t;
        ctx.gpr[3] = o;
        return;
    }
    const f64 px = Lf(pl + 144), py = Lf(pl + 148), pz = Lf(pl + 152);
    const f64 hmax = Lf(0x00F9AE28u), rmax = Lf(0x00F9AE74u);
    const u32 arg8 = load<u32>(0x00F9AF0Cu);
    const f64 ox = Lf(o + 176), oz = Lf(o + 184);
    f64 y = Lf(K);
    if (kind != 9) y = Sgl28(Lf(load<u32>(own + 4) + 132) + Lf(K + 4));
    const f64 k2 = Lf(K + 8);
    const f64 k22 = Fmuls31(k2, k2);
    const f64 dx = Sgl28(px - ox), dz = Sgl28(pz - oz);
    store<u32>(sp + 116, snd);
    fl = (fl & ~0xE000u) | 0x6000u;
    store<u16>(sp + 124, 0);
    store<u32>(sp + 120, 0x34F5E34Cu);
    f64 n = Fmadds(dx, dx, k22);
    n = Fmadds(dz, dz, n);
    fl &= ~0x0F00u;
    n = Sgl28(Lf(K + 12) / Sgl28(std::sqrt(n)));
    const f64 qx = Fmadds(Fmuls31(dx, n), t, ox);
    const f64 qy = Fmadds(Fmuls31(k2, n), t, y);
    const f64 qz = Fmadds(Fmuls31(dz, n), t, oz);
    u32 bit = 0;
    if (snd != 0) {
        store<u16>(sp + 126, static_cast<u16>(fl));
        if ((load<u64>(snd + 184) & 1u) != 0) bit = 1;
    }
    const f64 dy = Sgl28(py - qy);
    fl = (fl & ~0x1000u) | (bit << 12);
    store<u16>(sp + 126, static_cast<u16>(fl));
    store<u32>(sp + 112, 0x00B0DADCu);
    store<u32>(sp + 128, load<u32>(0x01164B60u));
    store<u32>(sp + 132, load<u32>(0x01164B64u));
    const u32 vz = load<u32>(0x01164B68u);
    ctx.gpr[3] = fl;
    ctx.fpr[1] = qz;
    if (!(dy > Lf(K + 16)) || !(dy < hmax)) return;
    const f64 ex = Sgl28(px - qx), ez = Sgl28(pz - qz);
    f64 d2 = Fmadds(ex, ex, k22);
    d2 = Fmadds(ez, ez, d2);
    ctx.fpr[1] = d2;
    if (!(d2 < Fmuls31(rmax, rmax))) return;
    store<u32>(sp + 148, 0);
    store<u32>(sp + 144, arg8);
    store<u32>(sp + 136, vz);
    VCall(ctx, pl, 344, {pl, sp + 112});
}

// Contact step (o; f1 dt): +5136 += dt; for each object of the list
// +5144 (count +6168): one without bit 0x40000 of +172 is released (slot
// 40 with 0) and replaced by the last entry; otherwise, unless +4 is 5,
// when it is within sqrt(K[0]) of the player (slot 80 positions) the
// player is sent message 0x34F5E34C (slot 336) with the sender +48, o's
// position +176, the xz direction from the player to it, the vector
// 0x01164B40 and the f32 0x00F9AF58. K = the f32 table 0x00056090.
// Leaves r3 / f1 as the original does.
DECOMP(000560A0, Contact_Step_000560A0) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 352);
    const u32 sp = frame.addr();
    constexpr u32 K = 0x00056090u;
    f64 f1 = Sgl28(Lf(o + 5136) + farg(ctx, 0));
    store<f32>(o + 5136, static_cast<f32>(f1));
    const u32 pl = LocalPlayer33();
    const u32 pp = sp + 112, xp = sp + 128, msg = sp + 144;
    VCall(ctx, pl, 80, {pp, pl});
    u32 cnt = load<u32>(o + 6168);
    if (static_cast<s32>(cnt) > 0) {
        const f64 k0 = Lf(K), k1 = Lf(K + 4), k2 = Lf(K + 8);
        const f64 k11 = Fmuls31(k1, k1);
        for (s32 i = 0; i < static_cast<s32>(cnt); ++i, cnt = load<u32>(o + 6168)) {
            const u32 slot = o + 5144 + 4 * static_cast<u32>(i);
            const u32 x = load<u32>(slot);
            if ((load<u32>(x + 172) & 0x40000u) == 0) {
                VCall(ctx, x, 40, {x, 0});
                const u32 n = load<u32>(o + 6168) - 1;
                store<u32>(o + 6168, n);
                store<u32>(slot, load<u32>(o + 5144 + 4 * n));
                --i;
                continue;
            }
            if (load<u32>(o + 4) == 5) continue;
            VCall(ctx, x, 80, {xp, x});
            const f64 dy = Sgl28(Lf(xp + 4) - Lf(pp + 4));
            const f64 dx = Sgl28(Lf(xp) - Lf(pp));
            const f64 x1 = Lf(pp);
            const f64 z1 = Lf(pp + 8);
            f1 = z1;
            const f64 dz = Sgl28(Lf(xp + 8) - z1);
            u32 w216 = load<u32>(sp + 216);  // the previous message's word (or the stack's leftover)
            f64 d = Fmadds(dx, dx, Fmuls31(dy, dy));
            d = Fmadds(dz, dz, d);
            if (!(d < k0)) continue;
            const u32 ox = load<u32>(o + 176), oy = load<u32>(o + 180), oz = load<u32>(o + 184);
            const f64 ex = Sgl28(LfsF(ox) - x1), ez = Sgl28(LfsF(oz) - z1);
            f64 n = Fmadds(ex, ex, k11);
            n = Fmadds(ez, ez, n);
            const f64 len = Sgl28(std::sqrt(n));
            const u32 f208 = load<u32>(0x00F9AF58u);
            u32 fl = load<u16>(sp + 158);  // the previous message's flags (or the stack's leftover)
            const u32 snd = load<u32>(o + 48);
            fl = (fl & ~0xE000u) | 0x2000u;
            store<u32>(msg + 8, 0x34F5E34Cu);
            store<u32>(msg + 4, snd);
            fl &= ~0x0E00u;
            store<u16>(msg + 12, 0);
            fl &= ~0x0100u;
            const f64 inv = Sgl28(k2 / len);
            u32 bit = 0;
            if (snd != 0) {
                store<u16>(msg + 14, static_cast<u16>(fl));
                if ((load<u64>(snd + 184) & 1u) != 0) bit = 1;
            }
            fl = (fl & ~0x1000u) | (bit << 12);
            w216 &= 0x3FFFFFFFu;
            store<u16>(msg + 14, static_cast<u16>(fl));
            store<u32>(msg, 0x00B0DACCu);
            store<u32>(msg + 16, ox);
            store<u32>(msg + 20, oy);
            store<u32>(msg + 24, oz);
            store<f32>(msg + 32, static_cast<f32>(Fmuls31(ex, inv)));
            store<f32>(msg + 36, static_cast<f32>(Fmuls31(k1, inv)));
            store<f32>(msg + 40, static_cast<f32>(Fmuls31(ez, inv)));
            store<u32>(msg + 48, load<u32>(0x01164B40u));
            store<u32>(msg + 52, load<u32>(0x01164B44u));
            const u32 v2 = load<u32>(0x01164B48u);
            store<u32>(msg + 56, v2);
            f1 = LfsF(v2);
            store<u32>(msg + 64, f208);
            store<u32>(msg + 68, 0);
            store<u32>(msg + 72, w216);
            store<u32>(msg + 76, 0);
            VCall(ctx, pl, 336, {pl, msg});
            store<u32>(msg, 0x00B0DABCu);  // the message's base vtable (destructor)
        }
    }
    ctx.fpr[1] = f1;
    ctx.gpr[3] = load<u32>(o + 6168);
}
