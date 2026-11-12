// Game code, large-function sweep part 32: a sensor phantom's AABB set to
// the 30-degree wedge in front of its owner, then a scan of what it
// overlaps for a grab-flagged shape.
#include "../sly4.h"
#include "big32.h"

using namespace sly4;
using namespace sly4::big32;

namespace {
constexpr u32 kWedgeK = 0x002D0220;  // .text: f32 -0 (sign mask), 0.0, 1e-5, 1.0, 0.5, pi/6, 8.0, -8.0 at +0..+28; vperm and
                                     // lane masks at +32..+176
constexpr u32 kFltMax4 = 0x00B2C3A0;  // f32[4] 0x7F7FFFEE (the AABB min seed; max seeds with its negation)
constexpr u32 kUp = 0x01164B60;       // f32[3] world up
constexpr u32 kPropGrab = 0x418F096F;  // shape property: grab flag (as Ledge_Probe2CCFF0)
constexpr u32 Math_Cosf7344E0 = 0x007344E0;   // f1 -> f1
constexpr u32 Math_Sinf7345F0 = 0x007345F0;   // f1 -> f1
constexpr u32 Hk_SetAabb924B90 = 0x00924B90;  // (body, const hkAabb*)

// One lane of p transformed by the rows c (lane k) / up / d and the
// translation t: vmaddfp chain from +0, then vaddfp.
f32 XLane(f32 c, f32 u, f32 d, f32 t, f32 px, f32 py, f32 pz) {
    f32 a = std::fma(c, px, 0.0f);
    a = std::fma(u, py, a);
    a = std::fma(d, pz, a);
    return a + t;
}
}  // namespace

// bool f(Sensor* s) (called by f_00131798 and f_00132AC0). With the phantom
// body at +120, the owner at +8 and the body's +8 all non-zero: the owner's
// forward axis (+128 / +136 of its matrix at +96, y taken as 0) is
// normalized (returns 0 when its squared length is below 1e-5, f1 left at
// the x read); the frame d = that axis, row 1 = kUp, row 0 = kUp x d,
// origin = owner +144 + kUp * 0.5 transforms the five points (0, 0, 8),
// (8 sin, 0, 8 cos), (-8 sin, 0, 8 cos) (30 degrees), (0, 0, 0), (0, 1, 0);
// their min / max (w lanes 0; seeded with +-0x7F7FFFEE) go to an hkAabb at
// sp + 128 and Hk_SetAabb924B90(+120, aabb) is called (f1 is left at
// +0.0, the last point's z). Then for each overlapped collidable of the body (+192 the
// array, +196 the count, read once): a shape whose byte +24 is 1 with a
// non-zero property block (shape + s8 at +16) whose kPropGrab value (low
// word) is non-zero gives 1. Otherwise 0.
DECOMP(002D02E0, Sensor_WedgeHasGrab_002D02E0) {
    const u32 s = arg<u32>(ctx, 0);
    if (load<u32>(s + 120) == 0) return ret(ctx, 0u);
    const u32 own = load<u32>(s + 8);
    if (own == 0) return ret(ctx, 0u);
    if (load<u32>(load<u32>(s + 120) + 8) == 0) return ret(ctx, 0u);
    const u32 m = own + 96;
    const f64 zero = Lfs(kWedgeK + 4);
    const f64 x = Lfs(m + 32), z = Lfs(m + 40);
    f64 len2 = Fmadds(x, x, Muls(zero, zero));
    len2 = Fmadds(z, z, len2);
    if (std::fabs(len2) < Lfs(kWedgeK + 8)) {
        ctx.fpr[1] = x;
        return ret(ctx, 0u);
    }
    const f64 len = Sqrts(len2);
    const f64 half = Lfs(kWedgeK + 16), ang = Lfs(kWedgeK + 20);
    const f64 ux = Lfs(kUp), uy = Lfs(kUp + 4), uz = Lfs(kUp + 8);
    const f64 tx = Fmadds(ux, half, Lfs(m + 48)), ty = Fmadds(uy, half, Lfs(m + 52)), tz = Fmadds(uz, half, Lfs(m + 56));
    const f64 inv = Divs(Lfs(kWedgeK + 12), len);
    const f64 dy = Muls(zero, inv), dx = Muls(x, inv), dz = Muls(z, inv);
    const f64 cx = Fmsubs(uy, dz, Muls(uz, dy));
    const f64 cz = Fmsubs(ux, dy, Muls(uy, dx));
    const f64 cy = Fmsubs(uz, dx, Muls(ux, dz));
    const f64 cs = callf(ctx, Math_Cosf7344E0, {}, {ang});
    const f64 sn = callf(ctx, Math_Sinf7345F0, {}, {ang});
    const f64 eight = Lfs(kWedgeK + 24);
    const f32 sn8 = static_cast<f32>(Muls(sn, eight)), c8 = static_cast<f32>(Muls(cs, eight));
    const f64 ms8 = Muls(sn, Lfs(kWedgeK + 28));
    const f32 pts[5][3] = {{0.0f, 0.0f, 8.0f}, {sn8, 0.0f, c8}, {static_cast<f32>(ms8), 0.0f, c8}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    const f32 c[3] = {static_cast<f32>(cx), static_cast<f32>(cy), static_cast<f32>(cz)};
    const f32 u[3] = {W32f(kUp), W32f(kUp + 4), W32f(kUp + 8)};
    const f32 d[3] = {static_cast<f32>(dx), static_cast<f32>(dy), static_cast<f32>(dz)};
    const f32 t[3] = {static_cast<f32>(tx), static_cast<f32>(ty), static_cast<f32>(tz)};
    const u32 body = load<u32>(s + 120);
    Stack frame(ctx, 48);
    const u32 sp = frame.addr() - 128;  // the original's frame (offsets below are its sp offsets)
    V4 mn = Lvx(kFltMax4), mx = big7::Xor(mn, V4{{-0.0f, -0.0f, -0.0f, -0.0f}});
    for (u32 i = 0; i < 5; ++i) {
        V4 p{{0.0f, 0.0f, 0.0f, 0.0f}};  // the w lane cleared by vandc
        for (u32 k = 0; k < 3; ++k) p.e[k] = XLane(c[k], u[k], d[k], t[k], pts[i][0], pts[i][1], pts[i][2]);
        mn = big7::Min4(mn, p);
        mx = big7::Max4(mx, p);
    }
    Stvx(sp + 144, mx);
    Stvx(sp + 128, mn);
    ctx.fpr[1] = 0.0;  // f1 as the original leaves it for the callee: the last point's z (its per-point lfs copies)
    call(ctx, Hk_SetAabb924B90, {body, sp + 128});
    const u32 lst = load<u32>(s + 120) + 192;
    const s32 n = load<s32>(lst + 4);
    for (s32 i = 0; i < n; ++i) {
        const u32 sh = load<u32>(load<u32>(lst) + 4 * static_cast<u32>(i));
        if (static_cast<s8>(load<u8>(sh + 24)) != 1) continue;
        const u32 props = sh + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(sh + 16))));
        if (props == 0) continue;
        u32 v = 0;
        for (s32 j = 0; j < load<s32>(props + 128); ++j) {
            const u32 e = load<u32>(props + 124) + 16 * static_cast<u32>(j);
            if (load<u32>(e) == kPropGrab) {
                v = load<u32>(load<u32>(props + 124) + (static_cast<u32>(j) << 4) + 12);
                break;
            }
        }
        if (v != 0) return ret(ctx, 1u);
    }
    ret(ctx, 0u);
}
