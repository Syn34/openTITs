// Game code, large-function sweep part 3: the launch-direction solver
// 004A2690 (9 callers: thrown / fired projectiles of the character and
// gadget code).
#include "big3.h"

using namespace sly4;
using namespace sly4::big3;

namespace b3a {
constexpr u32 Libm_Unary5E6578 = 0x005E6578;  // f32 (f32 x in [-1, 1]) (asinf?)
constexpr u32 Atan2f = 0x005E665C;            // (f1 y, f2 x) -> f1
constexpr u32 Math_Cosf = 0x007344E0;         // (f1 x) -> f1
constexpr u32 Math_Sinf = 0x007345F0;         // (f1 x) -> f1
constexpr u32 kVecZero = 0x01164B40;          // f32[3]: the vector "no motion" is compared with
constexpr u32 kDirDefault = 0x01164B60;       // f32[3]: the direction given for no iterations
// The .text constant table 0x004A2660.
constexpr u32 kConsts = 0x004A2660;
constexpr u32 kZero = 0, kEps = 4, kOne = 8, kHalf = 12, kMinusOne = 16, kMinDiv = 20, kTwo = 24, kMaxSpeedRatio = 28,
              kHalfPi = 32, kFour = 36, kRootEps = 40;
}  // namespace b3a

namespace {

inline f64 F(u32 a) { return std::bit_cast<f64>(LfsBits(load<u32>(a))); }
inline f64 K(u32 off) { return F(b3a::kConsts + off); }
inline f64 S(f64 x) { return static_cast<f64>(static_cast<f32>(x)); }
inline f64 Muls(f64 a, f64 b) { return S(a * b); }
inline f64 Divs(f64 a, f64 b) { return S(a / b); }
inline f64 Adds(f64 a, f64 b) { return S(a + b); }
inline f64 Subs(f64 a, f64 b) { return S(a - b); }
inline f64 Madds(f64 a, f64 c, f64 b) { return S(std::fma(a, c, b)); }
inline f64 Msubs(f64 a, f64 c, f64 b) { return S(std::fma(a, c, -b)); }
inline f64 Nmsubs(f64 a, f64 c, f64 b) { return S(-std::fma(a, c, -b)); }
inline f64 Sqrts(f64 x) { return S(std::sqrt(x)); }
// stfs: a NaN keeps its payload's top bits (no quieting), as the hardware.
inline void StoreF(u32 a, f64 v) {
    if (std::isnan(v)) {
        const u64 b = std::bit_cast<u64>(v);
        store<u32>(a, static_cast<u32>((b >> 32) & 0x80000000u) | 0x7F800000u | static_cast<u32>((b >> 29) & 0x7FFFFFu));
        return;
    }
    store<u32>(a, std::bit_cast<u32>(static_cast<f32>(v)));
}
inline f64 Fn1(PPUContext& ctx, u32 fn, f64 x) { return callf(ctx, fn, {}, {x}); }

}  // namespace

// bool Aim_SolveLaunch(const AimQuery* q, f32 dir[3], f32* time): the unit
// launch direction from q's origin (+0) that reaches a target moving from
// +32 by velocity +48 (minus the shooter's velocity +16), at speed +64
// under gravity +68, refined over +72 iterations (+76: the high arc).
// *time gets the flight time. With no gravity it aims straight at the
// predicted target; with both velocities zero (equal to kVecZero within
// 1e-5) it solves the ballistic equation directly. Returns false when the
// target is out of reach. f1 is left as the original leaves it on each
// exit (the last time, speed or intermediate value).
DECOMP(004A2690, Aim_SolveLaunch) {
    using namespace b3a;
    const u32 q = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1), tout = arg<u32>(ctx, 2);
    const auto done = [&](u32 r3, f64 f1) {
        ctx.fpr[1] = f1;
        ret(ctx, r3);
    };
    const auto put = [&](f64 x, f64 y, f64 z) {
        StoreF(out + 0, x);
        StoreF(out + 4, y);
        StoreF(out + 8, z);
    };
    const f64 eps = K(kEps);
    store<u32>(tout, 0);
    // Both velocities "zero"?
    bool still = true;
    for (const u32 v : {48u, 16u})
        for (u32 k = 0; k < 12 && still; k += 4) still = std::fabs(Subs(F(q + v + k), F(kVecZero + k))) < eps;
    f64 x = F(kDirDefault), y = F(kDirDefault + 4), z = F(kDirDefault + 8);  // f28 / f7 / f29
    if (load<u32>(q + 72) == 0) {
        put(x, y, z);
        done(1, K(kZero));
        return;
    }
    const f64 sq0 = Muls(K(kZero), K(kZero));  // (f23: 0 * 0)
    f64 t = K(kZero);                          // f1: the flight time
    f64 ang = K(kZero);                        // f31: the elevation angle
    Stack frame(ctx, 16);
    const u32 roots = frame.addr() + 4;  // the original's r1+112 (r1+108 below it)
    for (u32 it = 0;;) {
        // The target relative to the origin at time t.
        const f64 nt = -t;
        const f64 py = Madds(F(q + 20), nt, Madds(F(q + 52), t, F(q + 36)));
        const f64 px = Madds(F(q + 16), nt, Madds(F(q + 48), t, F(q + 32)));
        const f64 pz = Madds(F(q + 24), nt, Madds(F(q + 56), t, F(q + 40)));
        const f64 dy = Subs(py, F(q + 4)), dx = Subs(px, F(q + 0)), dz = Subs(pz, F(q + 8));
        const f64 dist = Sqrts(Madds(dz, dz, Madds(dx, dx, Muls(dy, dy))));
        const f64 g = F(q + 68);
        if (g == 0.0) {
            t = Divs(dist, F(q + 64));
            StoreF(tout, t);
            if (dist >= eps) {
                const f64 inv = Divs(K(kOne), dist);
                x = Muls(dx, inv);
                y = Muls(dy, inv);
                z = Muls(dz, inv);
            }
            if (still) {
                put(x, y, z);
                done(dist >= eps ? 1u : 0u, t);
                return;
            }
        } else {
            f64 nx = dx, nz = dz;  // f29 / f28
            f64 h = Sqrts(Madds(dz, dz, Madds(dx, dx, sq0)));
            if (h >= eps) {
                const f64 inv = Divs(K(kOne), h);
                nx = Muls(nx, inv);
                nz = Muls(nz, inv);
            }
            if (still) {
                // Direct solution of the ballistic equation.
                const f64 spd = F(q + 64), gneg = -g;
                const f64 v2 = Muls(spd, spd);
                const f64 hg = Muls(h, gneg);
                const f64 disc = Msubs(v2, v2, Muls(gneg, Madds(h, hg, Muls(Muls(dy, v2), K(kTwo)))));
                if (disc < 0.0) { done(0, hg); return; }
                const f64 r = Sqrts(disc);
                const f64 num = load<u8>(q + 76) != 0 ? Adds(v2, r) : Subs(v2, r);
                const f64 a = callf(ctx, Atan2f, {}, {num, hg});
                const f64 c = Fn1(ctx, Math_Cosf, a);
                const f64 ox = Muls(c, nx), oz = Muls(c, nz);
                const f64 s = Fn1(ctx, Math_Sinf, a);
                f64 time;
                if (std::fabs(c) >= eps) {
                    time = Divs(h, Muls(F(q + 64), c));
                } else {
                    // Straight up or down: the flight time from the vertical
                    // quadratic, the earlier root (the later one for the high
                    // arc). With no root the slot read was never written: the
                    // original reads stale stack there (r1+112, or r1+108 for
                    // the high arc), as this does in its own frame.
                    const f64 half_g = Muls(gneg, K(kHalf));
                    const f64 b = Muls(s, F(q + 64));
                    const f64 d = Msubs(b, b, Muls(Muls(half_g, K(kFour)), -dy));
                    s32 n = 0;
                    if (!(d < 0.0)) {
                        const f64 rt = Sqrts(d);
                        const f64 den = Muls(half_g, K(kTwo));
                        const f64 m = Divs(b, den), w = Divs(rt, den);
                        if (std::fabs(w) < K(kRootEps)) {
                            StoreF(roots, -m);
                            n = 1;
                        } else {
                            const f64 r0 = Subs(w, m), r1 = Subs(-m, w);
                            StoreF(roots, r0);
                            StoreF(roots + 4, r1);
                            if (r0 > r1) {
                                StoreF(roots, r1);
                                StoreF(roots + 4, r0);
                            }
                            n = 2;
                        }
                    }
                    const s32 idx = load<u8>(q + 76) != 0 ? n - 1 : 0;
                    time = F(roots + static_cast<u32>(4 * idx));
                }
                StoreF(tout, time);
                put(ox, s, oz);
                done(1, s);
                return;
            }
            if (load<u8>(q + 76) == 0) {
                // Low arc: refine the angle from the time.
                f64 tt = t;
                if (it == 0) {
                    tt = Divs(dist, F(q + 64));
                    ang = K(kZero);
                    StoreF(tout, tt);
                    if (dist >= eps) {
                        ang = Fn1(ctx, Libm_Unary5E6578, Divs(dy, dist));
                        tt = F(tout);
                    }
                }
                if (h >= eps) {
                    const f64 vy = Nmsubs(Muls(g, tt), K(kHalf), Divs(dy, tt));
                    const f64 spd = F(q + 64);
                    if (std::fabs(vy) <= spd) {
                        ang = Fn1(ctx, Libm_Unary5E6578, Divs(vy, spd));
                    } else {
                        if (!(it < load<u32>(q + 72) - 1)) { done(0, spd); return; }
                        const f64 r = Divs(vy, spd);
                        const f64 lo = r < K(kOne) ? r : K(kOne);
                        const f64 cl = lo < K(kMinusOne) ? K(kMinusOne) : (r < K(kOne) ? r : K(kOne));
                        ang = Fn1(ctx, Libm_Unary5E6578, cl);
                    }
                    const f64 c = Fn1(ctx, Math_Cosf, ang);
                    const f64 t3 = F(tout);
                    const f64 hv = Muls(Muls(t3, F(q + 64)), c);
                    const f64 den = hv > K(kMinDiv) ? hv : K(kMinDiv);
                    StoreF(tout, Muls(t3, Divs(h, den)));
                }
            } else {
                // High arc: the time from the angle, then the angle from
                // the time.
                if (it == 0) ang = K(kHalfPi);
                const f64 s = Fn1(ctx, Math_Sinf, ang);
                const f64 v = Muls(F(q + 64), s);
                const f64 g2 = Muls(F(q + 68), K(kTwo));
                const f64 v2 = Muls(v, v);
                const f64 disc = Madds(g2, dy, v2);
                const f64 t0 = Divs(-v, F(q + 68));
                if (!(disc >= 0.0)) { done(0, t0); return; }
                const f64 r = Divs(Sqrts(disc), F(q + 68));
                const f64 ta = Subs(t0, r), tb = Adds(t0, r);
                const f64 tt = ta > tb ? ta : tb;
                h = Divs(h, tt);
                StoreF(tout, tt);
                ang = callf(ctx, Atan2f, {}, {v, h});
                if (!(it < load<u32>(q + 72) - 1)) {
                    const f64 ratio = Divs(Sqrts(Adds(Madds(h, h, v2), sq0)), F(q + 64));
                    if (ratio > K(kMaxSpeedRatio)) { done(0, ratio); return; }
                }
            }
            const f64 c = Fn1(ctx, Math_Cosf, ang);
            const f64 ox = Muls(c, nx), oz = Muls(c, nz);
            x = ox;
            z = oz;
            y = Fn1(ctx, Math_Sinf, ang);
            t = F(tout);
        }
        ++it;
        if (!(it < load<u32>(q + 72))) break;
    }
    put(x, y, z);
    done(1, t);
}
