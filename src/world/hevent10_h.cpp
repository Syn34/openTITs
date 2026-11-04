// World_HandleEvent callees, part 10h: the treasure drop path (cubic
// Bezier position, eased slerp rotation).
#include <cmath>

#include "hevent10.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent10;

namespace sly4 {
namespace hevent10h {
namespace hfn10 {
constexpr u32 Math_Cosf = 0x007344E0;  // (f32) -> f32
constexpr u32 Math_Sinf = 0x007345F0;  // (f32) -> f32
constexpr u32 acosf = 0x005E649C;      // libm
}  // namespace hfn10

// Treasure (as Treasure_Tick426380): +0 / +16 / +32 / +48 vec3 Bezier
// control points, +64 quat from, +80 quat to, +148 f32 elapsed, +160 u32
// resting (non-zero: at the end point).
constexpr u32 kTrP0 = 0, kTrP1 = 16, kTrP2 = 32, kTrP3 = 48, kTrQ0 = 64, kTrQ1 = 80;
constexpr u32 kTrElapsed = 148, kTrResting = 160;
constexpr u32 kDropTime = 0x01089F70 + 72;  // f32 drop duration (tuning, name?)
// Constants (code segment): f32 0, 1, pi, 0.5, 0.99, 3, -1; f64 -1, 1.
constexpr u32 kConsts = 0x00425EE8;
}  // namespace hevent10h
}  // namespace sly4

using namespace sly4::hevent10h;

namespace {
f64 c32(u32 k) { return lfs_f64(kConsts + k); }
f64 fmadds(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }
f64 fnmsubs(f64 a, f64 c, f64 b) { return static_cast<f32>(-std::fma(a, c, -b)); }
f64 fnmadds(f64 a, f64 c, f64 b) { return static_cast<f32>(-std::fma(a, c, b)); }
f64 fsubs(f64 a, f64 b) { return static_cast<f32>(a - b); }
f64 fsel(f64 a, f64 c, f64 b) { return a >= 0.0 ? c : b; }
}  // namespace

// void Treasure_Place425F18(Treasure* t, quat* rot, vec3* pos) (name?):
// a resting treasure copies its end rotation and point. Otherwise, with s
// = elapsed / drop time clamped to [0, 1] and w = (1 + cos(pi - pi s)) / 2,
// pos = the cubic Bezier through +0..+48 at w and rot = q0 -> q1 at w (the
// shorter way): lerped above a dot of 0.99, slerped below. r3 / f1 as the
// original leaves them.
DECOMP(00425F18, Treasure_Place425F18) {
    const u32 t = arg<u32>(ctx, 0);
    const u32 rot = arg<u32>(ctx, 1);
    const u32 pos = arg<u32>(ctx, 2);
    const u32 resting = load<u32>(t + kTrResting);
    if (resting != 0) {
        for (u32 k = 0; k < 16; k += 4) store<u32>(rot + k, load<u32>(t + kTrQ1 + k));
        store<u32>(pos + 0, load<u32>(t + kTrP3 + 0));
        store<u32>(pos + 4, load<u32>(t + kTrP3 + 4));
        store<u32>(pos + 8, load<u32>(t + kTrP3 + 8));
        ctx.fpr[1] = lfs_f64(t + kTrP3 + 4);
        ctx.gpr[3] = resting;
        return;
    }
    const f64 zero = c32(0), one = c32(4);
    const f64 dur = lfs_f64(kDropTime);
    f64 u = one;
    if (dur > zero) u = static_cast<f32>(lfs_f64(t + kTrElapsed) / dur);
    f64 s = zero;
    {
        f64 m = one;
        if (u < m) m = u;
        if (m > zero) {
            s = one;
            if (u < s) s = u;
        }
    }
    const f64 pi = c32(8);
    const f64 c = callf(ctx, hfn10::Math_Cosf, {}, {fnmsubs(s, pi, pi)});
    const f64 w = static_cast<f32>(static_cast<f32>(c + one) * c32(12));  // fadds then fmuls by 0.5
    f64 q0[4], q1[4];
    for (u32 k = 0; k < 4; ++k) {
        q0[k] = lfs_f64(t + kTrQ0 + 4 * k);
        q1[k] = lfs_f64(t + kTrQ1 + 4 * k);
    }
    f64 dot = static_cast<f32>(q0[1] * q1[1]);
    dot = fmadds(q0[0], q1[0], dot);
    const f64 dot2 = fmadds(q0[2], q1[2], dot);
    dot = fmadds(q0[3], q1[3], dot2);
    f64 a[4], d[4], k_;  // result = a * k_ + d per component
    if (dot > c32(16)) {
        f64 q[4] = {q1[0], q1[1], q1[2], q1[3]};
        if (dot < zero)
            for (f64& x : q) x = -x;
        for (u32 k = 0; k < 4; ++k) {
            a[k] = fsubs(q[k], q0[k]);
            d[k] = q0[k];
        }
        k_ = w;
    } else {
        f64 cosang = dot;
        if (dot < zero) {
            cosang = fnmadds(q0[3], q1[3], dot2);
            for (f64& x : q1) x = -x;
        }
        f64 m = one;
        if (cosang < m) m = cosang;
        f64 cl = c32(24);
        if (m > cl) {
            cl = one;
            if (cosang < cl) cl = cosang;
        }
        // fsel clamps against the f64 constants -1 / 1 (frsp after each).
        f64 x = static_cast<f32>(fsel(static_cast<f32>(cl + one), cl, load<f64>(kConsts + 32)));
        x = static_cast<f32>(fsel(fsubs(x, one), load<f64>(kConsts + 40), x));
        const f64 ang = callf(ctx, hfn10::acosf, {}, {x});
        f64 perp[4];
        perp[1] = fnmsubs(lfs_f64(t + kTrQ0 + 4), cosang, q1[1]);
        perp[0] = fnmsubs(lfs_f64(t + kTrQ0 + 0), cosang, q1[0]);
        perp[2] = fnmsubs(lfs_f64(t + kTrQ0 + 8), cosang, q1[2]);
        perp[3] = fnmsubs(lfs_f64(t + kTrQ0 + 12), cosang, q1[3]);
        f64 len = static_cast<f32>(perp[1] * perp[1]);
        const f64 wa = static_cast<f32>(w * ang);
        len = fmadds(perp[0], perp[0], len);
        len = fmadds(perp[2], perp[2], len);
        len = fmadds(perp[3], perp[3], len);
        len = static_cast<f32>(std::sqrt(len));
        const f64 inv = static_cast<f32>(one / len);
        const f64 cw = callf(ctx, hfn10::Math_Cosf, {}, {wa});
        const f64 sw = callf(ctx, hfn10::Math_Sinf, {}, {wa});
        for (u32 k = 0; k < 4; ++k) d[k] = static_cast<f32>(static_cast<f32>(perp[k] * inv) * sw);
        for (u32 k = 0; k < 4; ++k) a[k] = lfs_f64(t + kTrQ0 + 4 * k);
        k_ = cw;
    }
    // 0x004260E8: rot = a * k + d.
    const f64 r0 = fmadds(a[0], k_, d[0]);
    const f64 v = fsubs(one, w);
    const f64 r1 = fmadds(a[1], k_, d[1]);
    const f64 r2 = fmadds(a[2], k_, d[2]);
    const f64 r3 = fmadds(a[3], k_, d[3]);
    store<f32>(rot + 0, static_cast<f32>(r0));
    store<f32>(rot + 4, static_cast<f32>(r1));
    store<f32>(rot + 8, static_cast<f32>(r2));
    store<f32>(rot + 12, static_cast<f32>(r3));
    // Cubic Bezier weights: (1-w)^3, 3(1-w)^2 w, 3(1-w) w^2, w^3.
    const f64 three = c32(20);
    const f64 w2 = static_cast<f32>(w * w);
    const f64 v2 = static_cast<f32>(v * v);
    const f64 b3 = static_cast<f32>(w2 * w);
    const f64 b0 = static_cast<f32>(v2 * v);
    const f64 b1 = static_cast<f32>(static_cast<f32>(v2 * three) * w);
    const f64 b2 = static_cast<f32>(static_cast<f32>(v * three) * w2);
    f64 p[3];
    for (u32 k = 0; k < 3; ++k) {
        p[k] = static_cast<f32>(b1 * lfs_f64(t + kTrP1 + 4 * k));
        p[k] = fmadds(b0, lfs_f64(t + kTrP0 + 4 * k), p[k]);
        p[k] = fmadds(b2, lfs_f64(t + kTrP2 + 4 * k), p[k]);
    }
    ctx.fpr[1] = p[2];
    for (u32 k = 0; k < 3; ++k) p[k] = fmadds(b3, lfs_f64(t + kTrP3 + 4 * k), p[k]);
    store<f32>(pos + 0, static_cast<f32>(p[0]));
    store<f32>(pos + 4, static_cast<f32>(p[1]));
    store<f32>(pos + 8, static_cast<f32>(p[2]));
    ctx.gpr[3] = kConsts;
}
