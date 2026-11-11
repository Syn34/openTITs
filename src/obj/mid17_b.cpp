// Game code, medium sweep part 17: movers.
#include "mid17.h"

using namespace sly4;
using namespace sly4::mid17;
using mid15::kRand15;
using mid15::RandF15;
using mid15::RandMix15;

namespace {
inline f64 Len3(f64 x, f64 y, f64 z) { return Sqrts(Fmadds(z, z, Fmadds(x, x, S(y * y)))); }
}  // namespace

// void Mover_FleeNearest2D130(Mover* o; f32 dt)
// (called by 0002E130) Finds the nearest player object (game *(0x00F65CA0):
// count u8 +29, player i's object at +4216 + i * 0x20C80, position +144)
// to o's driver o->48 (+144). When one exists and is more than K+4 (K =
// 0x0002D10C: +0 0, +4 0.001, +8 1, +12 0.7, +16 0.2, +20 0.1, +24 1/65535,
// +28 2, +32 -1) away, steers away: the target is the player + away * o->320
// minus the driver, blended 0.1 with 0.7 * the driver's x/z (+128/+136)
// plus 0.2 * normalize(o->324 * {away.z, -away.x}) (the y parts use 0);
// the driver's heading w->484 = atan2f(x, z) of that, w->548 = 1.0, when
// the driver has flag 0x200 in +192. Then, if w->516 > 0 and the timer
// o->328 > 1 / w->516, Mover_Fn2CC64(o) re-picks the steer; and if o->356
// has bit 0x20000000, clears it, normalizes o->336..344
// and jitters the heading by 0.1 * a random unit vector (three steps of the
// generator at 0x01164888). Always ends with o->328 += dt. r3 is left as
// the compiled code leaves it.
DECOMP(0002D130, Mover_FleeNearest2D130) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    constexpr u32 K = 0x0002D10Cu;
    u32 r3 = 0;
    auto finish = [&]() {
        const f64 t = S(Lf(o + 328) + dt);
        Sf(o + 328, t);
        ctx.fpr[1] = t;
        ret(ctx, r3);
    };
    const u32 g = load<u32>(0x00F65CA0u);
    const u32 n = load<u8>(g + 29);
    if (n == 0) return finish();
    const f64 zero = Lf(K);
    {
        f64 best = zero;
        u32 gp = g;
        for (u32 i = 0; i < n; ++i, gp += 0x20C80u) {
            const u32 p = load<u32>(gp + 4216);
            if (p == 0) continue;
            const u32 w = load<u32>(o + 48) + 144;
            const f64 d = Len3(S(Lf(w) - Lf(p + 144)), S(Lf(w + 4) - Lf(p + 148)), S(Lf(w + 8) - Lf(p + 152)));
            if (r3 == 0) { best = d; r3 = p; }
            else if (d < best) { best = d; r3 = p; }
        }
    }
    if (r3 == 0) return finish();
    const u32 q = r3 + 144;
    u32 w = load<u32>(o + 48);
    const u32 m = w + 144;
    const f64 qx = Lf(q), qy = Lf(q + 4), qz = Lf(q + 8);
    const f64 mx = Lf(m), my = Lf(m + 4), mz = Lf(m + 8);
    f64 dy = S(my - qy), dx = S(mx - qx), dz = S(mz - qz);
    const f64 len = Len3(dx, dy, dz);
    r3 = K;
    if (!(len > Lf(K + 4))) return finish();
    const f64 one = Lf(K + 8), k01 = Lf(K + 20);
    const f64 inv = Divs(one, len);
    const f64 side = Lf(o + 324), push = Lf(o + 320);
    const f64 c07 = Lf(K + 12), c02 = Lf(K + 16);
    const f64 wx = Lf(w + 128), wz = Lf(w + 136);
    dx = S(dx * inv);
    dz = S(dz * inv);
    dy = S(dy * inv);
    const f64 sz = -S(side * dx), sx = S(side * dz);
    f64 ty = S(Fmadds(dy, push, qy) - my), tx = S(Fmadds(dx, push, qx) - mx), tz = S(Fmadds(dz, push, qz) - mz);
    {
        const f64 si = Divs(one, Sqrts(Fmadds(sz, sz, Fmadds(sx, sx, S(zero * zero)))));
        const f64 by = Fmadds(zero, c07, S(S(zero * si) * c02));
        const f64 bx = Fmadds(wx, c07, S(S(sx * si) * c02));
        const f64 bz = Fmadds(wz, c07, S(S(sz * si) * c02));
        ty = Fmadds(ty, k01, by);
        tx = Fmadds(tx, k01, bx);
        tz = Fmadds(tz, k01, bz);
    }
    const f64 ti = Divs(one, Len3(tx, ty, tz));
    const f64 h = callf(ctx, 0x005E665Cu, {}, {S(tx * ti), S(tz * ti)});  // atan2f
    r3 = static_cast<u32>(ctx.gpr[3]);
    w = load<u32>(o + 48);
    if (w == 0) return finish();
    r3 = load<u32>(w + 192) & 0x200u;
    if (r3 == 0) return finish();
    Sf(w + 484, h);
    store<u32>(w + 548, 0x3F800000u);
    {
        const f64 rate = Lf(w + 516);
        if (rate > zero && Lf(o + 328) > Divs(one, rate)) call(ctx, 0x0002CC64u, {o});  // Mover_Fn2CC64
    }
    r3 = load<u32>(o + 356);
    if ((r3 & 0x20000000u) == 0) return finish();
    f64 vy = Lf(o + 340), vx = Lf(o + 336), vz = Lf(o + 344);
    r3 &= ~0x20000000u;
    store<u32>(o + 356, r3);
    const f64 vl = Len3(vx, vy, vz);
    if (!(vl > Lf(K + 4))) return finish();
    const f64 vi = Divs(one, vl);
    const u64 old = load<u64>(kRand15);
    const u32 x1 = RandMix15(load<u32>(kRand15 + 8));
    const u32 x2 = RandMix15(std::rotl(x1, 16));
    const u32 x3 = RandMix15(std::rotl(x2, 16));
    vx = S(vx * vi);
    vy = S(vy * vi);
    vz = S(vz * vi);
    Sf(o + 336, vx);
    Sf(o + 340, vy);
    Sf(o + 344, vz);
    const f64 k24 = Lf(K + 24), k28 = Lf(K + 28), k32 = Lf(K + 32);
    f64 rx = Fmadds(S(RandF15(old) * k24), k28, k32);
    f64 ry = Fmadds(S(RandF15(x1 & 0xFFFFu) * k24), k28, k32);
    f64 rz = Fmadds(S(RandF15(x2 & 0xFFFFu) * k24), k28, k32);
    store<u32>(kRand15 + 8, std::rotl(x3, 16));
    store<u64>(kRand15, x3 & 0xFFFFu);
    {
        const f64 a = Divs(one, Len3(rx, ry, rz));
        ry = S(ry * a); rx = S(rx * a); rz = S(rz * a);
        const f64 b2 = Divs(one, Len3(rx, ry, rz));
        ry = S(ry * b2); rz = S(rz * b2); rx = S(rx * b2);
    }
    vy = Fmadds(ry, k01, vy);
    vz = Fmadds(rz, k01, vz);
    vx = Fmadds(rx, k01, vx);
    const f64 hi = Divs(one, Len3(vx, vy, vz));
    Sf(w + 484, callf(ctx, 0x005E665Cu, {}, {S(vx * hi), S(vz * hi)}));
    r3 = static_cast<u32>(ctx.gpr[3]);
    finish();
}
