// Game code, medium sweep part 15: aiming and trajectory math.
#include "mid15.h"

using namespace sly4;
using namespace sly4::mid15;

// bool Ballistic_Solve4A2D78(const Shot* o, f32 vel[3], f32* speed, f32 pos[3])
// (called by 00025708, 000EEB08, 001440C0) o {+0 origin, +16 base, +32
// target velocity, +48 f32 launch angle, +52 f32 max range, +56 f32
// gravity, +60 u32 iterations}. With no iterations, or a zero cosine,
// returns 0. Otherwise iterates (once when the target velocity is within
// 1e-5 of the global vec3 at 0x01164B40 in every axis) a fixed point on
// the flight time t (from 0): pos = base + target * t (clamped to the max
// range from the origin in x/z), d = the horizontal distance, h = |dy| -
// d tan (|dy| + d tan when pos is below the origin), speed = sqrt(|g d^2
// / (cos^2 h k)|) (k = -2 or 2; -1 when the divisor is zero), t = d / (cos
// speed) when speed > 0. vel = cos * the unit horizontal direction, then
// vel[1] = sin; pos[1] = 0 at the end of each pass. Constants at
// 0x004A2D5C. Returns 1 (f1 left as the cosine).
DECOMP(004A2D78, Ballistic_Solve4A2D78) {
    const u32 o = arg<u32>(ctx, 0), vel = arg<u32>(ctx, 1), outv = arg<u32>(ctx, 2), pos = arg<u32>(ctx, 3);
    if (load<u32>(o + 60) == 0) {
        ret(ctx, 0);
        return;
    }
    const f64 s = callf(ctx, m15fn::Math_Sinf, {}, {Lf(o + 48)});
    const f64 c = callf(ctx, m15fn::Math_Cosf, {}, {Lf(o + 48)});
    const u32 K = 0x004A2D5Cu;  // f32 0, 1e-5, 1, -2, -1, 2
    const f64 k0 = Lf(K);
    f64 tx = Lf(o + 32), ty = Lf(o + 36), tz = Lf(o + 40);
    u32 n = load<u32>(o + 60);
    if (c == k0) {
        ret(ctx, 0);
        return;
    }
    const u32 g = 0x01164B40u;
    const f64 k1 = Lf(K + 4);
    const f64 tn = S(s / c);
    if (std::fabs(S(tx - Lf(g))) < k1 && std::fabs(S(ty - Lf(g + 4))) < k1 && std::fabs(S(tz - Lf(g + 8))) < k1) n = 1;
    if (n != 0) {
        f64 t = k0;
        const f64 cc = S(c * c), k2 = Lf(K + 8), kk = S(t * t);
        for (u32 i = 0;;) {
            const f64 py = Fmadds(ty, t, Lf(o + 20)), px = Fmadds(tx, t, Lf(o + 16)), pz = Fmadds(tz, t, Lf(o + 24));
            Sf(pos + 4, py);
            Sf(pos, px);
            Sf(pos + 8, pz);
            const f64 oy = Lf(o + 4), ox = Lf(o), oz = Lf(o + 8), maxd = Lf(o + 52);
            const f64 dy = S(py - oy);
            const bool below = dy < k0;
            const f64 dx = S(px - ox), dz = S(pz - oz), ady = std::fabs(dy);
            f64 d = S(std::sqrt(Fmadds(dz, dz, Fmadds(dx, dx, kk))));
            if (maxd > k0 && d > maxd) {
                const f64 inv = S(k2 / d);
                d = maxd;
                Sf(pos, Fmadds(maxd, S(inv * dx), ox));
                Sf(pos + 4, Fmadds(maxd, S(inv * k0), oy));
                Sf(pos + 8, Fmadds(maxd, S(inv * dz), oz));
            }
            f64 hx = dx, hy = k0, hz = dz;
            if (d > k1 || d == k1) {
                const f64 inv = S(k2 / d);
                hx = S(inv * dx);
                hy = S(inv * k0);
                hz = S(inv * dz);
            }
            const f64 d2 = S(d * d);
            const f64 h = below ? Fmadds(d, tn, ady) : Fnmsubs(d, tn, ady);
            const f64 kf = below ? Lf(K + 20) : Lf(K + 12);
            const f64 den = S(cc * S(h * kf));
            const f64 num = S(std::fabs(Lf(o + 56)) * d2);
            f64 v = Lf(K + 16);
            if (den != k0) v = S(num / den);
            v = S(std::sqrt(std::fabs(v)));
            Sf(outv, v);
            if (v > k0) t = S(d / S(c * v));
            ++i;
            Sf(vel, S(c * hx));
            Sf(vel + 4, S(c * hy));
            Sf(vel + 8, S(c * hz));
            Sf(vel + 4, s);
            store<u32>(pos + 4, 0);
            tx = Lf(o + 32);
            ty = Lf(o + 36);
            tz = Lf(o + 40);
            if (i >= n) break;
        }
    }
    ret(ctx, 1);
}

// void Heading_Jitter2C9A8(Mover* o)
// (called by 0002E130) When o->48 is set and its +192 has 0x200, its
// heading +484 drifts by a random step (0.628 * r / 65535 - 0.314, wrapped
// into [0, 2pi)). With o+356 bit 0x20000000 set, the bit is cleared and
// the direction o+336 (vec3), when longer than 0.001, is normalised and
// stored, then nudged by 0.1 times a random unit vector (each axis 2 *
// r / 65535 - 1, normalised twice) and normalised again; o->48's +484
// becomes atan2f(x, z) of it (o->48 when the 0x200 test failed, which is
// then 0: the store goes to 484, the original's quirk). The generator
// (kRand15) steps once for the drift, three times for the vector.
// Constants at 0x0002C980. Leftover f1 as the original's.
DECOMP(0002C9A8, Heading_Jitter2C9A8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 K = 0x0002C980u;  // f32 1/65535, 0.628, -0.314, 0, 2pi, 0.001, 1, 2, -1, 0.1
    const u32 p = load<u32>(o + 48);
    u32 w = 0;
    if (p != 0 && (load<u32>(p + 192) & 0x200u) != 0) w = p;
    if (w != 0) {
        const u64 old = load<u64>(kRand15);
        const u32 x = RandMix15(load<u32>(kRand15 + 8));
        store<u32>(kRand15 + 8, std::rotl(x, 16));
        store<u64>(kRand15, x & 0xFFFFu);
        const f64 r = S(RandF15(old) * Lf(K));
        f64 h = S(Lf(w + 484) + Fmadds(r, Lf(K + 4), Lf(K + 8)));
        if (h < Lf(K + 12)) h = S(h + Lf(K + 16));
        const f64 tp = Lf(K + 16);
        if (h > tp) h = S(h - tp);
        Sf(w + 484, h);
        ctx.fpr[1] = tp;
    }
    const u32 fl = load<u32>(o + 356);
    if ((fl & 0x20000000u) == 0) return;
    f64 y = Lf(o + 340), x = Lf(o + 336), z = Lf(o + 344);
    store<u32>(o + 356, fl & ~0x20000000u);
    const f64 len = S(std::sqrt(Fmadds(z, z, Fmadds(x, x, S(y * y)))));
    ctx.fpr[1] = z;
    if (!(len > Lf(K + 20))) return;
    const f64 one = Lf(K + 24);
    const f64 inv = S(one / len);
    const u64 old = load<u64>(kRand15);
    const u32 x1 = RandMix15(load<u32>(kRand15 + 8));
    const u32 x2 = RandMix15(std::rotl(x1, 16));
    const u32 x3 = RandMix15(std::rotl(x2, 16));
    x = S(x * inv);
    y = S(y * inv);
    z = S(z * inv);
    Sf(o + 336, x);
    Sf(o + 340, y);
    Sf(o + 344, z);
    const f64 k0 = Lf(K), k7 = Lf(K + 28), k8 = Lf(K + 32);
    f64 a = Fmadds(S(RandF15(old) * k0), k7, k8);
    f64 b = Fmadds(S(RandF15(x1 & 0xFFFFu) * k0), k7, k8);
    f64 c = Fmadds(S(RandF15(x2 & 0xFFFFu) * k0), k7, k8);
    f64 il = S(one / S(std::sqrt(Fmadds(c, c, Fmadds(a, a, S(b * b))))));
    store<u32>(kRand15 + 8, std::rotl(x3, 16));
    store<u64>(kRand15, x3 & 0xFFFFu);
    const f64 k9 = Lf(K + 36);
    b = S(b * il);
    a = S(a * il);
    c = S(c * il);
    il = S(one / S(std::sqrt(Fmadds(c, c, Fmadds(a, a, S(b * b))))));
    b = S(b * il);
    a = S(a * il);
    c = S(c * il);
    const f64 ny = Fmadds(b, k9, y), nx = Fmadds(a, k9, x), nz = Fmadds(c, k9, z);
    const f64 in = S(one / S(std::sqrt(Fmadds(nz, nz, Fmadds(nx, nx, S(ny * ny))))));
    const f64 hd = callf(ctx, m15fn::Atan2f, {}, {S(nx * in), S(nz * in)});
    Sf(w + 484, hd);
}

// u32 Blend_ByAngle23AD48(Obj* o, Out* p)
// (called by 0023C168) d = o->396 + 1072. p+96 (vec3) = d+48 (or, with
// o->628 set, only x / y from d+60 / d+64). a = acosf(o+116 clamped to
// [-1, 1] by the fsel chain). Then for each of the u16 d+140 groups i
// (32-byte entries from d+132, each {+0 data (352-byte keys), +8 u16 n})
// and each key j with lo = +140 and hi = +152 both >= 0: t = clamp01((a -
// lo) / (hi - lo)) (fsel; a NaN picks 1 and 0), and the 40-byte output
// p->140[i * groups + j] gets +12 = lerp(+144, +156, t), +20 = lerp(+148,
// +160, t). Constants at 0x0023AD20. Returns 0 (f1 left as a).
DECOMP(0023AD48, Blend_ByAngle23AD48) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    u32 d = load<u32>(o + 396) + 1072;
    if (load<u32>(o + 628) != 0) {
        Cw(p + 96, d + 60);
        Cw(p + 100, d + 64);
    } else {
        Cw(p + 104, d + 56);
        Cw(p + 96, d + 48);
        Cw(p + 100, d + 52);
    }
    const u32 K = 0x0023AD20u;  // f32 1, f32 0, f64 1, f32 -1, f64 -1, f64 0
    const f64 one = Lf(K), d1 = Ld(K + 8), m1 = Lf(K + 16), dm1 = Ld(K + 24);
    const f64 x = Lf(o + 116);
    f64 c = S(Fsel15(S(one - x), x, d1));
    c = S(Fsel15(S(c - m1), c, dm1));
    c = S(Fsel15(S(c + one), c, dm1));
    c = S(Fsel15(S(c - one), d1, c));
    const f64 a = callf(ctx, m15fn::Acosf, {}, {c});
    const s32 ng = load<u16>(d + 140);
    if (ng > 0) {
        const f64 zero = Lf(K + 4), d0 = Ld(K + 32);
        u32 e = load<u32>(d + 132), base = 0;
        for (s32 i = 0; i < ng; ++i, base += static_cast<u32>(ng), e += 32) {
            const u32 n = load<u16>(e + 8);
            if (n == 0) continue;
            for (u32 j = 0; j < n; ++j) {
                const u32 k = load<u32>(e) + 352 * j;
                const f64 lo = Lf(k + 140);
                if (!(lo >= zero)) continue;
                const f64 hi = Lf(k + 152);
                if (!(hi >= zero)) continue;
                const f64 sp = S(hi - lo), dl = S(a - lo);
                const u32 out = load<u32>(p + 140) + (base + j) * 40;
                f64 t = S(S(one / sp) * dl);
                t = S(Fsel15(S(one - t), t, d1));
                t = S(Fsel15(t, t, d0));
                const f64 u = S(one - t);
                Sf(out + 12, Fmadds(u, Lf(k + 144), S(t * Lf(k + 156))));
                Sf(out + 20, Fmadds(u, Lf(k + 148), S(t * Lf(k + 160))));
            }
        }
    }
    ret(ctx, 0);
}

// bool Cam_SphereInView28A0E8(Cam* o, const f32 s[4])
// (called by 0028A300) With o->404 != 1 the byte 0x01068070 is set (once)
// and 0 returned. Otherwise Cam_UpdateViewCache_00289EC0(o), then the
// sphere s (centre, radius s[3]) against the view cone of o (position
// +420, axis +432, cos^2 terms +444 / +448): it fails when, measured from
// the apex pulled back by r * +452 along the axis, the centre is ahead
// and inside the +444 cone; or (behind the true apex) inside the +448
// cone and farther than r; or entirely behind the plane at the slot's
// +84 along the axis (slot = 128-byte entry o->240 of the table at
// 0x011950A0 +12). Returns 1 when visible; f1 is left as the original's.
DECOMP(0028A0E8, Cam_SphereInView28A0E8) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    if (load<u32>(o + 404) != 1) {
        if (load<u8>(0x01068070u) == 0) store<u8>(0x01068070u, 1);
        ret(ctx, 0);
        return;
    }
    const u32 slot = load<u32>(0x011950A0u + 12) + ((load<u32>(o + 240) << 7) & 0xFFFFFF80u);
    call(ctx, m15fn::Cam_UpdateViewCache_00289EC0, {o});
    const f64 r = Lf(s + 12);
    const f64 py = Lf(o + 424), px = Lf(o + 420), pz = Lf(o + 428);
    const f64 ay = Lf(o + 436), ax = Lf(o + 432), az = Lf(o + 440);
    const f64 sx = Lf(s), sy = Lf(s + 4), sz = Lf(s + 8);
    const f64 pb = S(r * Lf(o + 452));
    const f64 dy = S(sy - Fnmsubs(pb, ay, py)), dx = S(sx - Fnmsubs(pb, ax, px)), dz = S(sz - Fnmsubs(pb, az, pz));
    const f64 k0 = Lf(0x0028A0E0u);
    const f64 dot = Fmadds(dz, az, Fmadds(dx, ax, S(dy * ay)));
    const f64 ls = Fmadds(dz, dz, Fmadds(dx, dx, S(dy * dy)));
    ctx.fpr[1] = k0;
    if (!(dot > k0) || !(S(dot * dot) > S(ls * Lf(o + 444)) || S(dot * dot) == S(ls * Lf(o + 444)))) {
        ret(ctx, 0);
        return;
    }
    const f64 ey = S(sy - py), ex = S(sx - px), ez = S(sz - pz);
    const f64 nd = Fnmadds(ez, az, Fmadds(ex, ax, S(ey * ay)));  // -(e . axis)
    const f64 l2 = Fmadds(ez, ez, Fmadds(ex, ex, S(ey * ey)));
    bool vis = true;
    ctx.fpr[1] = nd;
    if (nd > k0) {
        const f64 n2 = S(nd * nd), c2 = S(l2 * Lf(o + 448));
        ctx.fpr[1] = n2;
        if (n2 > c2 || n2 == c2) {
            const f64 rr = S(r * r);
            ctx.fpr[1] = rr;
            vis = l2 < rr || l2 == rr;
        }
    }
    if (!vis) {
        ret(ctx, 0);
        return;
    }
    const f64 pl = Lf(slot + 84);
    const f64 qy = Fmadds(pl, ay, py), qx = Fmadds(pl, ax, px), qz = Fmadds(pl, az, pz);
    const f64 t1 = Fmadds(-az, sz, Fmadds(-ax, sx, S(-ay * sy)));
    const f64 t2 = Fnmadds(qz, -az, Fmadds(qx, -ax, S(qy * -ay)));
    const f64 v = S(t1 + t2);
    ctx.fpr[1] = v;
    ret(ctx, v < -r ? 0 : 1);
}

// void Timer_Advance4A0990(Timer* o, f32 dt)
// (called by Obj_UpdateCount_004A0D00, Obj_Blend_004A0E60,
// Obj_LerpPos_004A0FC8, 004AC820) o {+4 f32 t, +8 f32 duration, +12 f32
// eased output, +16 u32 modes: high u16 the easing, bits 1..15 the
// wrap, bit 0 (byte +19) the ping-pong direction}. With duration <= 0, t
// = 0. Wrap 0 clamps t + dt at the duration, 1 loops, 2 ping-pongs (bit 0
// set: forward), others leave t. Easing 0: out = t / d; 1: (cos(pi t / d)
// - 1) * -0.5; 2: with u = 2 t / d, u^2 up to 1, else 1 + sin(pi' * 4 *
// (2 - u^2) * 2)^2 * (1 - (2 - u^2)) (pi' = 0x40490E56); others none.
// Constants at 0x004A0970. f1 is left as the original's.
DECOMP(004A0990, Timer_Advance4A0990) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    const u32 K = 0x004A0970u;  // f32 0, 2, 1, 3.1415, 4, pi, -0.5
    const f64 dur = Lf(o + 8), k0 = Lf(K);
    if (dur < k0 || dur == k0) {
        store<u32>(o + 4, 0);
        ret(ctx, 0);
        return;
    }
    const u32 eas = load<u16>(o + 16), wrap = (load<u32>(o + 16) >> 1) & 0x7FFFu;
    f64 f = dt;
    ret(ctx, eas);  // leftover r3: the easing (the calls below replace it)
    if (wrap == 0) {
        f = S(Lf(o + 4) + dt);
        Sf(o + 4, f);
        if (f > dur) Sf(o + 4, dur);
    } else if (wrap == 1) {
        f = S(Lf(o + 4) + dt);
        Sf(o + 4, f);
        if (f > dur) {
            f = S(f - dur);
            Sf(o + 4, f);
        }
    } else if (wrap == 2) {
        const u32 b = load<u8>(o + 19);
        if ((b & 1u) != 0) {
            f = S(Lf(o + 4) + dt);
            Sf(o + 4, f);
            if (f > dur) {
                const f64 over = S(f - dur);
                store<u8>(o + 19, static_cast<u8>(b & 0xFEu));
                f = S(dur - over);
                Sf(o + 4, f);
            }
        } else {
            f = S(Lf(o + 4) - dt);
            Sf(o + 4, f);
            if (f < k0) {
                f = -f;
                store<u8>(o + 19, static_cast<u8>(b | 1u));
                Sf(o + 4, f);
            }
        }
    }
    if (eas == 0) {
        f = S(Lf(o + 4) / dur);
        Sf(o + 12, f);
    } else if (eas == 1) {
        const f64 c = callf(ctx, m15fn::Math_Cosf, {}, {S(S(Lf(o + 4) / dur) * Lf(K + 20))});
        f = S(S(c - Lf(K + 8)) * Lf(K + 24));
        Sf(o + 12, f);
    } else if (eas == 2) {
        const f64 two = Lf(K + 4), one = Lf(K + 8);
        ret(ctx, K);
        f64 u = S(S(Lf(o + 4) / dur) * two);
        u = S(u * u);
        if (u > one) {
            const f64 w = S(two - u);
            const f64 sn = callf(ctx, m15fn::Math_Sinf, {}, {S(S(S(w * two) * Lf(K + 12)) * Lf(K + 16))});
            f = S(one + S(S(sn * sn) * S(one - w)));
        } else {
            f = S(u + k0);
        }
        Sf(o + 12, f);
    }
    ctx.fpr[1] = f;
}

namespace {
// |a - b| for two vec3 as the original's fsubs (y, x, z) / fmuls / fmadds /
// fsqrts sequence.
inline f64 Dist15(u32 a, u32 b) {
    const f64 dy = S(Lf(a + 4) - Lf(b + 4)), dx = S(Lf(a) - Lf(b)), dz = S(Lf(a + 8) - Lf(b + 8));
    return S(std::sqrt(Fmadds(dz, dz, Fmadds(dx, dx, S(dy * dy)))));
}
// The nearest of the game's players (byte game+29 of them, each slot
// 0x20C80 bytes on with its object at +4216) to pos, or 0; `best` gets
// the distance (K0 when none was seen).
inline u32 NearestPlayer15(u32 pos, f64 k0) {
    const u32 g = load<u32>(mid14::kGlobF65CA0);
    const u32 n = load<u8>(g + 29);
    u32 best = 0;
    f64 bd = k0;
    for (u32 i = 0, s = g; i < n; ++i, s += 0x00020C80u) {
        const u32 p = load<u32>(s + 4216);
        if (p == 0) continue;
        const f64 d = Dist15(pos, p + 144);
        if (best == 0 || d < bd) {
            bd = d;
            best = p;
        }
    }
    return best;
}
}  // namespace

// void Mover_FacePlayer2DC20(Mover* o)
// (called by 0002E130) w = o->48 (needs +192 & 0x200 and a +508 id). The
// nearest player p (see NearestPlayer15) counts when w->348 >= 0 and the
// distance d from w+144 to p+144 is in (0.001, w->500), o+356 has
// 0x80000000 and w->160->208->100 differs from w->508: then w->480 =
// atan2f of the unit (x, z) of p - w and byte w+592 = 1, else 0.
// Constants at 0x0002DC14. Leftover r3 / f1 as the original's.
DECOMP(0002DC20, Mover_FacePlayer2DC20) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 48);
    if (w == 0 || (load<u32>(w + 192) & 0x200u) == 0) return;
    const u32 id = load<u32>(w + 508);
    if (id == 0) return;
    const u32 K = 0x0002DC14u;  // f32 0, 0.001, 1
    const f64 k0 = Lf(K);
    const u32 pos = w + 144;
    const u32 p = NearestPlayer15(pos, k0);
    ctx.fpr[1] = k0;
    u32 hit = 0;
    if (p != 0 && (Lf(w + 348) > k0 || Lf(w + 348) == k0)) {
        const u32 q = p + 144;
        const f64 ey = S(Lf(q + 4) - Lf(pos + 4)), ex = S(Lf(q) - Lf(pos)), ez = S(Lf(q + 8) - Lf(pos + 8));
        const f64 d = S(std::sqrt(Fmadds(ez, ez, Fmadds(ex, ex, S(ey * ey)))));
        ctx.fpr[1] = ez;
        if (d > Lf(K + 4) && d < Lf(w + 500)) {
            if ((load<u32>(o + 356) & 0x80000000u) == 0) {
                ret(ctx, 0);
            } else {
                const u32 c = load<u32>(load<u32>(load<u32>(w + 160) + 208) + 100);
                ret(ctx, c);
                if (id != c) {
                    const f64 inv = S(Lf(K + 8) / d);
                    Sf(w + 480, callf(ctx, m15fn::Atan2f, {}, {S(ex * inv), S(ez * inv)}));
                    hit = 1;
                }
            }
        }
    }
    store<u8>(w + 592, static_cast<u8>(hit));
}

namespace {
// The timer step both wobble functions end with: o+off += dt, less 4 when
// above 4 (k = 4). Leaves f1 the value and r3 the constants' address.
inline void WobbleTimer15(PPUContext& ctx, u32 o, u32 off, f64 dt, u32 K) {
    f64 t = S(Lf(o + off) + dt);
    Sf(o + off, t);
    const f64 lim = Lf(K + 4);
    if (t > lim) {
        t = S(t - lim);
        Sf(o + off, t);
    }
    ctx.fpr[1] = t;
    ret(ctx, K);
}
}  // namespace

// void Mover_WobbleAway2D618(Mover* o, f32 dt)
// (called by 0002E130) With the nearest player p (NearestPlayer15 from
// w+144, w = o->48) farther than 0.001 in x/z, w->484 (when w is set and
// its +192 has 0x200) = atan2f(-x, -z) of the unit away direction +
// sin(o->308 * 2pi * 0.5) * pi' * 0.25, wrapped into [0, 2pi], and w->548
// = 1.0f. Then o->308 += dt (wrapping at 4). Constants at 0x0002D5F4.
DECOMP(0002D618, Mover_WobbleAway2D618) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    const u32 K = 0x0002D5F4u;  // f32 0, 4, 0.001, 1, 2pi, 0.5, pi', 0.25
    const f64 k0 = Lf(K);
    const u32 p = NearestPlayer15(load<u32>(o + 48) + 144, k0);
    if (p != 0) {
        const u32 q = p + 144, wp = load<u32>(o + 48) + 144;
        const f64 dx = S(Lf(q) - Lf(wp)), dz = S(Lf(q + 8) - Lf(wp + 8));
        const f64 d = S(std::sqrt(Fmadds(dz, dz, Fmadds(dx, dx, S(k0 * k0)))));
        if (d > Lf(K + 8)) {
            const f64 inv = S(Lf(K + 12) / d);
            const f64 nz = S(dz * inv), nx = S(dx * inv);
            const f64 a = callf(ctx, m15fn::Atan2f, {}, {-nx, -nz});
            const f64 tp = Lf(K + 16);
            const f64 sn = callf(ctx, m15fn::Math_Sinf, {}, {S(S(Lf(o + 308) * tp) * Lf(K + 20))});
            f64 h = Fmadds(S(sn * Lf(K + 24)), Lf(K + 28), a);
            if (h < k0) h = S(h + tp);
            if (h > tp) h = S(h - tp);
            const u32 w = load<u32>(o + 48);
            if (w != 0 && (load<u32>(w + 192) & 0x200u) != 0) {
                Sf(w + 484, h);
                store<u32>(w + 548, 0x3F800000u);
            }
        }
    }
    WobbleTimer15(ctx, o, 308, dt, K);
}

// void Mover_WobbleToward2DE70(Mover* o, f32 dt)
// (called by 0002E130) As Mover_WobbleAway2D618 but toward the player
// (atan2f(x, z)) and on the timer o+312: within 5 the heading gets +
// sin(o->312 * 2pi) * pi' * 0.25 (wrapped into [0, 2pi]); at 5 or more
// the plain heading is used and the timer reset to 0. Constants at
// 0x0002DE50.
DECOMP(0002DE70, Mover_WobbleToward2DE70) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    const u32 K = 0x0002DE50u;  // f32 0, 4, 0.001, 1, 5, 2pi, pi', 0.25
    const f64 k0 = Lf(K);
    const u32 p = NearestPlayer15(load<u32>(o + 48) + 144, k0);
    if (p != 0) {
        const u32 q = p + 144, wp = load<u32>(o + 48) + 144;
        const f64 dx = S(Lf(q) - Lf(wp)), dz = S(Lf(q + 8) - Lf(wp + 8));
        const f64 d = S(std::sqrt(Fmadds(dz, dz, Fmadds(dx, dx, S(k0 * k0)))));
        if (d > Lf(K + 8)) {
            const f64 inv = S(Lf(K + 12) / d);
            f64 h = callf(ctx, m15fn::Atan2f, {}, {S(dx * inv), S(dz * inv)});
            if (d < Lf(K + 16)) {
                const f64 tp = Lf(K + 20);
                const f64 sn = callf(ctx, m15fn::Math_Sinf, {}, {S(Lf(o + 312) * tp)});
                h = Fmadds(S(sn * Lf(K + 24)), Lf(K + 28), h);
                if (h < k0) h = S(h + tp);
                if (h > tp) h = S(h - tp);
            } else {
                Sf(o + 312, k0);
            }
            const u32 w = load<u32>(o + 48);
            if (w != 0 && (load<u32>(w + 192) & 0x200u) != 0) {
                Sf(w + 484, h);
                store<u32>(w + 548, 0x3F800000u);
            }
        }
    }
    WobbleTimer15(ctx, o, 312, dt, K);
}

// bool Reach_CanTouch167F0(Obj* o, const Frame* t, const f32 p[3], f32 len,
//                          f32 ymin, f32 ymax, f32 maxDeg, ..., u8 useBase)
// (called by 0003AA78, 0003C208) The point c of the segment from t+48
// along t+32 (len long) nearest p (Seg_ClosestPoint_00013E40); q = p +
// unit(c - p) (unnormalised when shorter than 1e-5). The height (q.y, or
// p.y against o->180 with useBase) minus t+48's must be in [ymin, ymax]
// (it counts as 0 when o byte +305 has 0x40); then the offset from t+48 to
// q (x, h, z) must be shorter than len, and the angle
// (Vec_SignedAngle16680) between t+32 and its direction within half of
// maxDeg degrees. Constants at 0x000167D8. f1 is left as the original's.
DECOMP(000167F0, Reach_CanTouch167F0) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1), p = arg<u32>(ctx, 2);
    const u32 flag = arg<u32>(ctx, 7) & 0xFFu;
    const f64 len = ctx.fpr[1], ymin = ctx.fpr[2], ymax = ctx.fpr[3];
    const u32 K = 0x000167D8u;  // f32 pi/180, 1e-5, 0, 1, 0.5
    const u32 a = t + 48, dir = t + 32;
    const f64 lim = S(ctx.fpr[4] * Lf(K));
    Stack frame(ctx, 48);
    const u32 bp = frame.addr(), cp = bp + 16, vp = bp + 32;
    Sf(bp, Fmadds(Lf(dir), len, Lf(a)));
    Sf(bp + 4, Fmadds(Lf(dir + 4), len, Lf(a + 4)));
    Sf(bp + 8, Fmadds(Lf(dir + 8), len, Lf(a + 8)));
    call(ctx, m15fn::Seg_ClosestPoint_00013E40, {cp, o, a, bp, p});
    const f64 py = Lf(p + 4), px = Lf(p), pz = Lf(p + 8);
    f64 dy = S(Lf(cp + 4) - py), dx = S(Lf(cp) - px), dz = S(Lf(cp + 8) - pz);
    const f64 ax = Lf(a), az = Lf(a + 8);
    f64 ay = Lf(a + 4);
    const f64 dl = S(std::sqrt(Fmadds(dz, dz, Fmadds(dx, dx, S(dy * dy)))));
    if (dl > Lf(K + 4)) {
        dx = S(dx / dl);
        dy = S(dy / dl);
        dz = S(dz / dl);
    }
    const f64 qz = S(pz + dz), qx = S(px + dx);
    f64 h = S(py + dy);
    if (flag != 0) {
        h = Lf(p + 4);
        ay = Lf(o + 180);
    }
    h = S(h - ay);
    ctx.fpr[1] = qx;
    if (!(h > ymin || h == ymin) || !(h < ymax || h == ymax)) {
        ret(ctx, 0);
        return;
    }
    const f64 ex = S(qx - ax), ez = S(qz - az);
    if ((load<u8>(o + 305) & 0x40u) != 0) h = Lf(K + 8);
    const f64 l2 = Fmadds(ez, ez, Fmadds(ex, ex, S(h * h)));
    ctx.fpr[1] = ex;
    if (!(l2 < S(len * len))) {
        ret(ctx, 0);
        return;
    }
    const f64 inv = S(Lf(K + 12) / S(std::sqrt(l2)));
    Sf(vp, S(ex * inv));
    Sf(vp + 4, S(h * inv));
    Sf(vp + 8, S(ez * inv));
    const f64 ang = std::fabs(callf(ctx, m15fn::Vec_SignedAngle16680, {dir, vp}, {}));
    ctx.fpr[1] = ang;
    ret(ctx, ang < S(lim * Lf(K + 16)) ? 1 : 0);
}

namespace {
// The parabolic sine of the wave functions: x wrapped into [-pi, pi) as the
// original does it (the fsel bias +-pi, fmod by 2pi through a 64-bit
// truncation, unbias), then y = 4/pi x - 4/pi^2 x |x|, refined to y +
// 0.225 (y |y| - y). `cos` adds pi/2 (wrapping above pi) first.
inline f64 WaveSin15(u32 K, f64 x, bool cos) {
    const f64 bias = S(Fsel15(x, Ld(K + 8), Ld(K + 16)));
    const f64 v = S(bias + x), tp = Lf(K + 24);
    f64 w = S(Fnmsubs(Trunc15(S(v / tp)), tp, v) - bias);
    if (cos) {
        w = S(w + Lf(K + 28));
        if (w > Lf(K + 32)) w = S(w - tp);
    }
    const f64 p = S(S(w * Lf(K + 40)) * std::fabs(w));
    const f64 y = Fmadds(w, Lf(K + 36), p);
    return Fmadds(Fmsubs(y, std::fabs(y), y), Lf(K + 44), y);
}
}  // namespace

// f32 Wave_Eval4C0420(const Wave* o, f32 t)
// (called by Obj_SetFromSingleton_004C0728) o {+24 type, +28/+32 range,
// +36 amplitude, +40 frequency, +44 phase, +48 offset, +52 constant, +56 /
// +60 ramp ends, +64 ramp length}. 0: sine of (f t + phase), 1: cosine; 2:
// square (1 for the first half of each period, else -1), 3: triangle, 4/5:
// saw (frac(frac(t) f) * 2 - 1); all as offset + amplitude * wave; 6: the
// constant; 7: lerp of +56 -> +60 by (t - a) / len (wrapped to +32 - a
// when negative); others 0. Constants at 0x004C03E0. Returns f1.
DECOMP(004C0420, Wave_Eval4C0420) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 t = ctx.fpr[1];
    const u32 K = 0x004C03E0u;  // f32 0, f64 pi, f64 -pi, f32 2pi, pi/2, pi, 4/pi, -4/pi^2, 0.225, 0.5, 2, 1, -2
    f64 r = Lf(K);
    const u32 ty = load<u32>(o + 24);
    auto ph = [&] { return S(S(t - Trunc15(t)) * Lf(o + 40)); };  // frac(t) * f
    switch (ty) {
    case 0:
    case 1: {
        const f64 x = Fmadds(Lf(o + 40), t, Lf(o + 44));
        r = Fmadds(Lf(o + 36), WaveSin15(K, x, ty == 1), Lf(o + 48));
        break;
    }
    case 2: {
        const f64 a = ph();
        f64 v = r;
        if (S(a - Trunc15(a)) < Lf(K + 48)) v = Lf(K + 56);
        r = Fmadds(Lf(o + 36), Fmsubs(v, Lf(K + 52), Lf(K + 56)), Lf(o + 48));
        break;
    }
    case 3: {
        const f64 a = ph();
        f64 f = S(a - Trunc15(a));
        f64 m = Lf(K + 52);
        if (!(f < Lf(K + 48))) {
            f = S(f - Lf(K + 56));
            m = Lf(K + 60);
        }
        r = Fmadds(Lf(o + 36), Fmsubs(S(f * m), Lf(K + 52), Lf(K + 56)), Lf(o + 48));
        break;
    }
    case 4:
    case 5: {
        const f64 a = ph();
        r = Fmadds(Lf(o + 36), Fmsubs(S(a - Trunc15(a)), Lf(K + 52), Lf(K + 56)), Lf(o + 48));
        break;
    }
    case 6:
        r = Lf(o + 52);
        break;
    case 7: {
        const f64 a = Lf(o + 28);
        f64 d = S(t - a);
        if (d < r) d = S(Lf(o + 32) - a);
        const f64 u = S(d / Lf(o + 64));
        r = Fmadds(Lf(o + 56), S(Lf(K + 56) - u), S(Lf(o + 60) * u));
        break;
    }
    default:
        break;
    }
    ctx.fpr[1] = r;
}
