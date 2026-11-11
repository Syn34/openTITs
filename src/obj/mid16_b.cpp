// Game code, medium sweep part 16: splines, slerp, aim and contacts.
#include "mid16.h"

using namespace sly4;
using namespace sly4::mid16;

namespace {

constexpr u32 SplineInstance_FindNearest = 0x005A8CF8;     // (pos, f32* t, f32* dist, filter opd, arg) -> inst
constexpr u32 WorldObj_SplinePointTangentAt = 0x005A80D8;  // (inst, -, f32 pos[4], f32 tan[4], bool back; f1 t)
constexpr u32 Obj_Fn2C5140 = 0x002C5140;                   // (Obj*, const Vec3*)
constexpr u32 Hk_SetF32At_007EE08C = 0x007EE08C;           // (f32* field, const f32*)

}  // namespace

// bool Obj_SnapToSpline20100(Obj* o)
// (called by 00020310) Looks for the nearest spline (filter OPD
// 0x00F2D998) to o->176 + o->160 * K4 (K = 0x000200F0); none -> 0.
// Otherwise one step of the game random (kRand15) picks t = rand * K8 and
// WorldObj_SplinePointTangentAt(inst, -, o->336, sp+128, 0) fills o's
// position and the tangent; o->352 = tangent x 0x01164B60 (up), flipped
// when (o->336 - o->176) . that < K0. Returns 1.
DECOMP(00020100, Obj_SnapToSpline20100) {
    using namespace mid15;
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 K = 0x000200F0u;
    Stack frame(ctx, 208);
    const u32 sp = frame.addr();
    const f64 k0 = Lf(K), k4 = Lf(K + 4);
    store<u32>(sp + 152, 0);
    store<u32>(sp + 156, 0);
    const f64 px = Fmadds(Lf(o + 160), k4, Lf(o + 176)), py = Fmadds(Lf(o + 164), k4, Lf(o + 180)),
              pz = Fmadds(Lf(o + 168), k4, Lf(o + 184));
    Sf(sp + 112, px);
    Sf(sp + 116, py);
    Sf(sp + 120, pz);
    ctx.fpr[1] = pz;  // left in f1 across the call
    const u32 inst = static_cast<u32>(call(ctx, SplineInstance_FindNearest, {sp + 112, sp + 152, sp + 156, 0x00F2D998u, 0}));
    if (inst == 0) {
        ret(ctx, 0);
        return;
    }
    const u64 old = load<u64>(kRand15);
    store<u64>(sp + 144, old);
    const f64 k8 = Lf(K + 8);
    const u32 x = RandMix15(load<u32>(kRand15 + 8));
    store<u32>(kRand15 + 8, std::rotl(x, 16));
    store<u64>(kRand15, x & 0xFFFFu);
    const f64 t = S(RandF15(old) * k8);
    Sf(sp + 152, t);
    callf(ctx, WorldObj_SplinePointTangentAt, {inst, 0x01160000u, o + 336, sp + 128, 0}, {t});  // r4: left by the random step
    constexpr u32 U = 0x01164B60u;
    const f64 t2 = Lf(sp + 136), t1 = Lf(sp + 132), t0 = Lf(sp + 128);
    const f64 u1 = Lf(U + 4), u2 = Lf(U + 8);
    const f64 m8 = S(t2 * u1);
    const f64 p1 = Lf(o + 340), p0 = Lf(o + 336);
    const f64 n0 = Fmsubs(t1, u2, m8);
    Sf(o + 352, n0);
    const f64 u2b = Lf(U + 8), u0 = Lf(U);
    const f64 m7 = S(t0 * u2b);
    const f64 p2 = Lf(o + 344);
    const f64 n1 = Fmsubs(t2, u0, m7);
    Sf(o + 356, n1);
    const f64 u0b = Lf(U), u1b = Lf(U + 4);
    const f64 m6 = S(t1 * u0b);
    const f64 n2 = Fmsubs(t0, u1b, m6);
    Sf(o + 360, n2);
    const f64 e1 = S(p1 - Lf(o + 180)), e0 = S(p0 - Lf(o + 176)), e2 = S(p2 - Lf(o + 184));
    const f64 d = Fmadds(e2, n2, Fmadds(e0, n0, S(e1 * n1)));
    ctx.fpr[1] = t0;
    if (d < k0) {
        Sf(o + 352, Fnmsubs(t1, u2, m8));
        Sf(o + 356, Fnmsubs(t2, u0, m7));
        const f64 f = Fnmsubs(t0, u1b, m6);
        Sf(o + 360, f);
        ctx.fpr[1] = f;
    }
    ret(ctx, 1);
}

// f32 Obj_TurnToFocus2C6F78(Obj* o, f32 dt)
// (called by 002C7340) With no focus object (*(0x00F65CA0)->0x1281E0)
// returns the f32 at 0x002C6F20 (r3 = 0x002C6F20). Otherwise the unit
// direction from the focus's +144 to o->480 (the focus's +128 when the
// squared length is <= K4, K = 0x002C6F20) is compared with o's facing
// o->208: angle = acosf(clamp(dot)); o->464 += dt / o->392; o->468 is
// clamped to [K56, K40]; u = clamp01(o->464 / o->468) eased by
// (cosf(K80 - u * K80) + 1) * K84 (clamped to [0, 1]); the facing moves
// toward the direction by that fraction (Obj_Fn2C5140(o, sp+112)).
// Returns the angle (r3 = that call's).
DECOMP(002C6F78, Obj_TurnToFocus2C6F78) {
    using mid15::Fsel15;
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    constexpr u32 K = 0x002C6F20u;
    Stack frame(ctx, 208);
    const u32 sp = frame.addr();
    const u32 p = load<u32>(load<u32>(0x00F65CA0u) + 0x1281E0u);
    if (p == 0) {
        ctx.fpr[1] = Lf(K);
        ret(ctx, K);
        return;
    }
    const f64 dy = S(Lf(o + 484) - Lf(p + 148)), dx = S(Lf(o + 480) - Lf(p + 144)), dz = S(Lf(o + 488) - Lf(p + 152));
    const f64 one = Lf(K + 8);
    f64 vx = Lf(p + 128), vy = Lf(p + 132), vz = Lf(p + 136);
    const f64 l2 = Fmadds(dz, dz, Fmadds(dx, dx, S(dy * dy)));
    if (!(l2 <= Lf(K + 4))) {
        const f64 inv = Divs(one, Sqrts(l2));
        vx = S(inv * dx);
        vy = S(inv * dy);
        vz = S(inv * dz);
    }
    const f64 d16 = Ld(K + 16), d32 = Ld(K + 32);
    f64 c = Fmadds(vz, Lf(o + 216), Fmadds(vx, Lf(o + 208), S(vy * Lf(o + 212))));
    c = S(Fsel15(S(one - c), c, d16));
    c = S(Fsel15(S(c - Lf(K + 24)), c, d32));
    c = S(Fsel15(S(c + one), c, d32));
    c = S(Fsel15(S(c - one), d16, c));
    const f64 ang = callf(ctx, mid15::m15fn::Acosf, {}, {c});
    const f64 t464 = Fmadds(Divs(one, Lf(o + 392)), dt, Lf(o + 464));
    const f64 h = Lf(o + 468);
    Sf(o + 464, t464);
    f64 dur = S(Fsel15(S(Lf(K + 40) - h), h, Ld(K + 48)));
    dur = S(Fsel15(S(dur - Lf(K + 56)), dur, Ld(K + 64)));
    f64 u = Divs(t464, dur);
    Sf(o + 468, dur);
    const f64 d72 = Ld(K + 72), k80 = Lf(K + 80);
    u = S(Fsel15(S(one - u), u, d16));
    u = S(Fsel15(u, u, d72));
    const f64 cs = callf(ctx, mid15::m15fn::Math_Cosf, {}, {Fnmsubs(u, k80, k80)});
    f64 e = S(S(cs + one) * Lf(K + 84));
    const f64 fx = Lf(o + 208), fy = Lf(o + 212), fz = Lf(o + 216);
    e = S(Fsel15(S(one - e), e, d16));
    e = S(Fsel15(e, e, d72));
    Sf(sp + 112, Fmadds(S(vx - fx), e, fx));
    Sf(sp + 116, Fmadds(S(vy - fy), e, fy));
    const f64 z = Fmadds(S(vz - fz), e, fz);
    Sf(sp + 120, z);
    ctx.fpr[1] = z;
    call(ctx, Obj_Fn2C5140, {o, sp + 112});
    ctx.fpr[1] = ang;
}

// void Quat_Slerp74C048(f32 out[4], const f32 a[4], const f32 b[4], f32 t)
// (called by 00134BB8, 00139B90 and others) c = a . b. Above K0 (0.99,
// K = 0x0074C028) a straight lerp a + (b - a) * t, b negated when the
// recomputed dot is < K4 (0) (r3 = K). Otherwise b is negated when c < 0;
// theta = acosf of min(c, 1) - but of K12 (-1) itself when that min is
// not above -1 - clamped to [-1, 1]; out = a cos(t theta) + normalize(b - a
// c) sin(t theta) (r3 = Math_Sinf's). f1 = out[3].
DECOMP(0074C048, Quat_Slerp74C048) {
    using mid15::Fsel15;
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const f64 t = ctx.fpr[1];
    constexpr u32 K = 0x0074C028u;
    const f64 a0 = Lf(a), b0 = Lf(b), a1 = Lf(a + 4), b1 = Lf(b + 4), a2 = Lf(a + 8), b2 = Lf(b + 8), a3 = Lf(a + 12),
              b3 = Lf(b + 12);
    const f64 m1 = S(a1 * b1);
    const f64 p7 = Fmadds(a2, b2, Fmadds(a0, b0, m1));
    f64 c = Fmadds(a3, b3, p7);
    if (c > Lf(K)) {
        f64 q0 = b0, q1 = b1, q2 = b2, q3 = b3;
        const f64 c2 = Fmadds(a3, b3, Fmadds(a2, b2, Fmadds(Lf(a), Lf(b), m1)));
        if (c2 < Lf(K + 4)) q0 = -b0, q1 = -b1, q2 = -b2, q3 = -b3;
        const f64 s0 = S(q0 - a0), s1 = S(q1 - Lf(a + 4)), s2 = S(q2 - Lf(a + 8)), s3 = S(q3 - Lf(a + 12));
        Sf(out, Fmadds(s0, t, a0));
        Sf(out + 4, Fmadds(s1, t, Lf(a + 4)));
        Sf(out + 8, Fmadds(s2, t, Lf(a + 8)));
        const f64 r = Fmadds(s3, t, Lf(a + 12));
        Sf(out + 12, r);
        ctx.fpr[1] = r;
        ret(ctx, K);
        return;
    }
    f64 q0 = b0, q1 = b1, q2 = b2, q3 = b3;
    if (c < Lf(K + 4)) {
        c = Fnmadds(a3, b3, p7);
        q0 = -q0, q1 = -q1, q2 = -q2, q3 = -q3;
    }
    const f64 one = Lf(K + 8);
    f64 x = one;
    if (c < one) x = c;
    if (!(x > Lf(K + 12))) x = Lf(K + 12);  // the compiled code jumps past the reload with f1 = K12
    x = S(Fsel15(S(x + one), x, Ld(K + 16)));
    x = S(Fsel15(S(x - one), Ld(K + 24), x));
    const f64 th = callf(ctx, mid15::m15fn::Acosf, {}, {x});
    const f64 r1 = Fnmsubs(Lf(a + 4), c, q1), r0 = Fnmsubs(Lf(a), c, q0), r2 = Fnmsubs(Lf(a + 8), c, q2),
              r3 = Fnmsubs(Lf(a + 12), c, q3);
    const f64 ang = S(t * th);
    const f64 inv = Divs(one, Sqrts(Fmadds(r3, r3, Fmadds(r2, r2, Fmadds(r0, r0, S(r1 * r1))))));
    const f64 cs = callf(ctx, mid15::m15fn::Math_Cosf, {}, {ang});
    const f64 c0 = S(Lf(a) * cs), c1 = S(Lf(a + 4) * cs), c2 = S(Lf(a + 8) * cs), c3 = S(Lf(a + 12) * cs);
    const f64 sn = callf(ctx, mid15::m15fn::Math_Sinf, {}, {ang});
    Sf(out, Fmadds(S(r0 * inv), sn, c0));
    Sf(out + 4, Fmadds(S(r1 * inv), sn, c1));
    Sf(out + 8, Fmadds(S(r2 * inv), sn, c2));
    const f64 r = Fmadds(S(r3 * inv), sn, c3);
    Sf(out + 12, r);
    ctx.fpr[1] = r;
}

// u32 Contact_Update106B80(Contact* c, Event* ev)
// (called by AdjThunk_00106F20) Needs ev->20, ev->24, c->20, c->24 and
// c->24->8 (else returns: r3 = 0 or ev->24). Zeroes ev->24's f32 +12
// (Hk_SetF32At_007EE08C with a 0.0 on the stack). Unless ev->28 is set
// and ev->29 clear, re-reads the frame: c->64 = c->24 + 288, c->80 = c->20
// + 288, c->96..152 = c->24's matrix (+240, rows of 16), c->160 = its row
// 2 . (c->80 - c->64). d = row 2 . (ev->20 - c->64); d <= c->1032 ->
// c->1024 = max(c->1024, c->1008 . ev->20+16, with the z term subtracted).
// d < c->160: ev->24->15 bit 0x8 cleared and ++c->164 (returned);
// otherwise bit 0x8 set (r3 = ev->24). f1 = d.
DECOMP(00106B80, Contact_Update106B80) {
    using mid15::Fsel15;
    const u32 c = arg<u32>(ctx, 0), ev = arg<u32>(ctx, 1);
    {
        u32 r3 = load<u32>(ev + 20);
        if (r3 != 0) r3 = load<u32>(ev + 24);
        if (r3 == 0 || load<u32>(c + 20) == 0 || load<u32>(c + 24) == 0 || load<u32>(load<u32>(c + 24) + 8) == 0) {
            ret(ctx, r3);
            return;
        }
    }
    Stack frame(ctx, 208);
    const u32 sp = frame.addr();
    store<u32>(sp + 176, 0);
    call(ctx, Hk_SetF32At_007EE08C, {load<u32>(ev + 24) + 12, sp + 176, 0});
    if (load<u8>(ev + 28) == 0 || load<u8>(ev + 29) != 0) {
        const u32 b = load<u32>(c + 24), a = load<u32>(c + 20);
        const f64 p0 = Lf(b + 288), p1 = Lf(b + 292), p2 = Lf(b + 296);
        Sf(c + 64, p0);
        Sf(c + 68, p1);
        Sf(c + 72, p2);
        const f64 q0 = Lf(a + 288), q1 = Lf(a + 292), q2 = Lf(a + 296);
        Sf(c + 80, q0);
        Sf(c + 84, q1);
        Sf(c + 88, q2);
        u32 m[12];
        for (u32 k = 0; k < 12; ++k) m[k] = load<u32>(b + 240 + 16 * (k / 3) + 4 * (k % 3));
        for (u32 k = 0; k < 12; ++k) store<u32>(c + 96 + 16 * (k / 3) + 4 * (k % 3), m[k]);
        const f64 d1 = S(q1 - p1), d0 = S(q0 - p0), d2 = S(q2 - p2);
        Sf(c + 160, Fmadds(Lf(c + 136), d2, Fmadds(Lf(c + 128), d0, S(Lf(c + 132) * d1))));
    }
    const u32 e = load<u32>(ev + 20);
    const f64 dx = S(Lf(e) - Lf(c + 64)), dy = S(Lf(e + 4) - Lf(c + 68)), dz = S(Lf(e + 8) - Lf(c + 72));
    const f64 d = Fmadds(Lf(c + 136), dz, Fmadds(Lf(c + 128), dx, S(Lf(c + 132) * dy)));
    if (d <= Lf(c + 1032)) {
        f64 v = Fmadds(Lf(c + 1008), Lf(e + 16), S(Lf(c + 1012) * Lf(e + 20)));
        v = Fnmadds(Lf(c + 1016), Lf(e + 24), v);
        const f64 cur = Lf(c + 1024);
        Sf(c + 1024, Fsel15(S(cur - v), cur, v));
    }
    ctx.fpr[1] = d;
    if (d < Lf(c + 160)) {
        const u32 t = load<u32>(ev + 24);
        store<u8>(t + 15, load<u8>(t + 15) & ~0x8u);
        const u32 n = load<u32>(c + 164) + 1;
        store<u32>(c + 164, n);
        ret(ctx, n);
    } else {
        const u32 t = load<u32>(ev + 24);
        store<u8>(t + 15, load<u8>(t + 15) | 0x8u);
        ret(ctx, t);
    }
}

// void Body_SpringRot0F90F8(Body* s, f32 k, f32 d, f32 dt)
// (called by 000FB450) Implicit spring step of s's angular velocity
// s->32 toward the target vector (s->64 when the byte s->84 is set, else
// 0x01164B40): with K = 0x000F90E0, a = k² K0, b = k d K4, den = 1 / (K8 +
// dt b + dt² a), w += dt ((b + dt a) den (target - w) + a den v) where v =
// Quat_AngularVel0F8E30(sp+112, s, s+16, s+48, K8); then the rotation dt w
// (above K12 in length) turns s->16: q = {axis sin(|dt w| K16), cos(...)}
// and s->16 = Quat_Multiply(q, s->16). f1 is left holding the last f32.
DECOMP(000F90F8, Body_SpringRot0F90F8) {
    const u32 s = arg<u32>(ctx, 0);
    const f64 k = ctx.fpr[1], dmp = ctx.fpr[2], dt = ctx.fpr[3];
    constexpr u32 K = 0x000F90E0u;
    const u32 tg = load<u8>(s + 84) != 0 ? s + 64 : 0x01164B40u;
    Stack frame(ctx, 224);
    const u32 sp = frame.addr();
    const f64 a = S(S(k * k) * Lf(K)), b = S(S(dmp * k) * Lf(K + 4)), one = Lf(K + 8);
    const f64 adt = S(dt * a);
    const f64 den = Divs(one, Fmadds(adt, dt, Fmadds(dt, b, one)));
    const f64 c29 = S(a * den), c28 = S(S(b + adt) * den);
    callf(ctx, m16fn::Quat_AngularVel0F8E30, {sp + 112, s, s + 16, s + 48}, {one});
    const f64 y = Fmadds(c28, S(Lf(tg + 4) - Lf(s + 36)), S(c29 * Lf(sp + 116)));
    const f64 x = Fmadds(c28, S(Lf(tg) - Lf(s + 32)), S(c29 * Lf(sp + 112)));
    const f64 z = Fmadds(c28, S(Lf(tg + 8) - Lf(s + 40)), S(c29 * Lf(sp + 120)));
    const f64 ny = Fmadds(dt, y, Lf(s + 36)), nx = Fmadds(dt, x, Lf(s + 32)), nz = Fmadds(dt, z, Lf(s + 40));
    const f64 ry = S(ny * dt), rx = S(nx * dt);
    Sf(s + 32, nx);
    const f64 rz = S(nz * dt);
    Sf(s + 36, ny);
    Sf(s + 40, nz);
    const f64 len = Sqrts(Fmadds(rz, rz, Fmadds(rx, rx, S(ry * ry))));
    if (!(len > Lf(K + 12))) {
        ctx.fpr[1] = rz;
        return;
    }
    const f64 inv = Divs(one, len);
    const f64 half = S(len * Lf(K + 16));
    const f64 uy = S(inv * ry), ux = S(inv * rx), uz = S(inv * rz);
    const f64 n = Divs(one, Sqrts(Fmadds(uz, uz, Fmadds(ux, ux, S(uy * uy)))));
    const f64 qx = S(n * ux), qy = S(n * uy), qz = S(n * uz);
    const f64 sn = callf(ctx, mid15::m15fn::Math_Sinf, {}, {half});
    const f64 sx = S(sn * qx), sy = S(sn * qy), sz = S(sn * qz);
    const f64 cs = callf(ctx, mid15::m15fn::Math_Cosf, {}, {half});
    Sf(sp + 144, sx);
    Sf(sp + 148, sy);
    Sf(sp + 152, sz);
    Sf(sp + 156, cs);
    call(ctx, mid15::m15fn::Quat_Multiply, {sp + 128, sp + 144, s + 16});
    for (u32 j = 0; j < 4; ++j) Cw(s + 16 + 4 * j, sp + 128 + 4 * j);
    ctx.fpr[1] = Lf(sp + 136);
}

// void Mover_Steer2CEB0(Mover* o, f32 dt)
// (called by 0002E130) K = 0x0002CE8C. With a driver w = o->48 that has
// flag 0x200 in +192: w->548 = rand * K0 * K4 + K8 (one step of kRand15);
// when w->516 > K12 and o->328 > K16 / w->516, Obj_Fn2CC64(o) (re-picks
// the steer); o->328 += dt. Then, when o->356 has flag 0x20000000, it
// clears it and normalizes the direction o->336 (when
// longer than K20); if that points behind the heading (sin h, 0, cos h)
// of w->484 (dot < K12 = 0) the heading becomes atan2f of the direction
// reflected off it (dir - heading * dot * K24, when longer than K28).
// Quirk: with no driver the heading is still read from address 0 + 484
// (and written back there). Leaves r3 / f1 as the compiled exits do.
DECOMP(0002CEB0, Mover_Steer2CEB0) {
    using namespace mid15;
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    constexpr u32 K = 0x0002CE8Cu;
    Stack frame(ctx, 160);
    const u32 sp = frame.addr();
    u32 w = 0;
    {
        const u32 p = load<u32>(o + 48);
        if (p != 0 && (load<u32>(p + 192) & 0x200u) != 0) w = p;
    }
    if (w != 0) {
        const u64 old = load<u64>(kRand15);
        store<u64>(sp + 112, old);
        const u32 x = RandMix15(load<u32>(kRand15 + 8));
        store<u32>(kRand15 + 8, std::rotl(x, 16));
        store<u64>(kRand15, x & 0xFFFFu);
        const f64 r = RandF15(old);
        const f64 v516 = Lf(w + 516);
        Sf(w + 548, Fmadds(S(r * Lf(K)), Lf(K + 4), Lf(K + 8)));
        if (v516 > Lf(K + 12)) {
            if (Lf(o + 328) > Divs(Lf(K + 16), v516)) call(ctx, 0x0002CC64u, {o});
        }
        const f64 t = S(Lf(o + 328) + dt);
        Sf(o + 328, t);
        ctx.fpr[1] = t;
    }
    const u32 f356 = load<u32>(o + 356);
    if ((f356 & 0x20000000u) == 0) {
        ret(ctx, f356);
        return;
    }
    const f64 y = Lf(o + 340), x = Lf(o + 336), z = Lf(o + 344);
    store<u32>(o + 356, f356 & ~0x20000000u);
    const f64 len = Sqrts(Fmadds(z, z, Fmadds(x, x, S(y * y))));
    ctx.fpr[1] = y;
    ret(ctx, K);
    if (!(len > Lf(K + 20))) return;
    const f64 one = Lf(K + 16);
    const f64 inv = Divs(one, len);
    Sf(o + 336, S(x * inv));
    Sf(o + 340, S(y * inv));
    Sf(o + 344, S(z * inv));
    const f64 sn = callf(ctx, mid15::m15fn::Math_Sinf, {}, {Lf(w + 484)});
    const f64 cs = callf(ctx, mid15::m15fn::Math_Cosf, {}, {Lf(w + 484)});
    const f64 k12 = Lf(K + 12);
    const f64 dy = Lf(o + 340), dx = Lf(o + 336), dz = Lf(o + 344);
    const f64 d = Fmadds(cs, dz, Fmadds(sn, dx, S(k12 * dy)));
    ctx.fpr[1] = d;
    if (!(d < k12)) return;
    const f64 k24 = Lf(K + 24);
    const f64 ry = Fnmsubs(S(dy * d), k24, k12), rx = Fnmsubs(S(dx * d), k24, sn), rz = Fnmsubs(S(dz * d), k24, cs);
    const f64 l2 = Sqrts(Fmadds(rz, rz, Fmadds(rx, rx, S(ry * ry))));
    ret(ctx, K);
    ctx.fpr[1] = rx;
    if (!(l2 > Lf(K + 28))) return;
    const f64 i2 = Divs(one, l2);
    const f64 h = callf(ctx, mid15::m15fn::Atan2f, {}, {S(rx * i2), S(rz * i2)});
    Sf(w + 484, h);
}

// void Cursor_ReadInput2411B0(Ui* o, f32 out[3])
// (called by 00241500) Reads the pad block at 0x010B1A90 (K = 0x00241180):
// the d-pad bits 0x8 / 0x4 / 0x2 / 0x1 of +8 give the axes (K4 per bit);
// the stick (Obj_GetChannel_0045B160 channels 0 / 1) is rescaled from the
// dead zone c->792 (c = o->216): s = clamp((|stick, z| - c->792) / (K4 -
// c->792), 0, 1) with z = the f32 at 0x01164B48, scaled by c->808 / c->804
// and dropped when a d-pad axis (times c->800 / c->796) is non-zero; out[0]
// / out[1] = clamp(d-pad + stick, -1, 1). With the byte o->604 set and o->600
// > 0, out = normalize(clamp(View_ProjectPoint(view, sp+112, o->576), 0, 1)
// - K40, 0) instead. Leaves r3 / f1 as the compiled exits do.
DECOMP(002411B0, Cursor_ReadInput2411B0) {
    using mid15::Fsel15;
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    constexpr u32 K = 0x00241180u, P = 0x010B1A90u;
    Stack frame(ctx, 240);
    const u32 sp = frame.addr();
    const f64 k0 = Lf(K);
    const u32 c = load<u32>(o + 216);
    const u32 btn = load<u32>(P + 8);
    const f64 zz = Lf(0x01164B48u);
    const f64 b8 = (btn & 0x8u) ? Lf(K + 4) : k0, b4 = (btn & 0x4u) ? Lf(K + 4) : k0;
    const f64 b2 = (btn & 0x2u) ? Lf(K + 4) : k0, b1 = (btn & 0x1u) ? Lf(K + 4) : k0;
    call(ctx, m16fn::Obj_GetChannel_0045B160, {P, 0});
    const f64 sx_ = ctx.fpr[1];
    call(ctx, m16fn::Obj_GetChannel_0045B160, {P, 1});
    const f64 sy = ctx.fpr[1];
    const f64 one = Lf(K + 4), d8 = Ld(K + 8), d16 = Ld(K + 16);
    const f64 dz = Lf(c + 792);
    const f64 mag = Sqrts(Fmadds(zz, zz, Fmadds(sx_, sx_, S(sy * sy))));
    const f64 ax = S(S(b8 - b4) * Lf(c + 800));
    f64 s = Divs(S(mag - dz), S(one - dz));
    const f64 ay = S(S(b2 - b1) * Lf(c + 796));
    s = S(Fsel15(S(one - s), s, d8));
    s = S(Fsel15(s, s, d16));
    f64 px = S(S(sx_ * s) * Lf(c + 808)), py = S(S(sy * s) * Lf(c + 804));
    if (!(ax == k0) || !(ay == k0)) px = py = k0;
    f64 x = S(ax + px), y = S(ay + py);
    const f64 k24 = Lf(K + 24), d32 = Ld(K + 32);
    x = S(Fsel15(S(one - x), x, d8));
    y = S(Fsel15(S(one - y), y, d8));
    x = S(Fsel15(S(x - k24), x, d32));
    y = S(Fsel15(S(y - k24), y, d32));
    Sf(out, x);
    Sf(out + 4, y);
    ctx.fpr[1] = y;
    const u8 on = load<u8>(o + 604);
    ret(ctx, on);
    if (on == 0) return;
    const f64 t600 = Lf(o + 600);
    ctx.fpr[1] = t600;
    if (!(t600 > k0)) return;
    for (u32 k = 0; k < 4; ++k) Cw(sp + 112 + 4 * k, 0x01164BC0u + 4 * k);
    const u32 view = load<u32>(load<u32>(0x00F65CA0u) + 0x128168u);
    call(ctx, 0x00522750u, {view, sp + 112, o + 576});
    f64 v1 = Lf(sp + 116), v0 = Lf(sp + 112);
    v1 = S(Fsel15(S(one - v1), v1, d8));
    v0 = S(Fsel15(S(one - v0), v0, d8));
    const f64 h = Lf(K + 40), w = S(k0 - k0);
    v1 = S(Fsel15(v1, v1, d16));
    v0 = S(Fsel15(v0, v0, d16));
    v1 = S(v1 - h);
    v0 = S(v0 - h);
    const f64 inv = Divs(one, Sqrts(Fmadds(w, w, Fmadds(v0, v0, S(v1 * v1)))));
    Sf(out, S(inv * v0));
    const f64 r1 = S(inv * v1);
    Sf(out + 4, r1);
    Sf(out + 8, S(inv * w));
    ctx.fpr[1] = r1;
    ret(ctx, K);
}

// void Cursor_ReadInput1DFFB8(Ui* o, f32 out[2])
// (called by 001E0468) Like Cursor_ReadInput2411B0 with both sticks (K =
// 0x001DFF88, c = o->216): each channel v of 0..3 becomes sign(v) (from
// v > K8 / v < K12, as an integer) * clamp((|v| - c->536) / (K4 - c->536),
// 0, 1); out[0] = d-pad (bits 0x2 - 0x1) * c->544 + ch1 * c->552 + ch3 *
// c->560, out[1] = d-pad (0x8 - 0x4) * c->540 + ch0 * c->548 + ch2 * c->556,
// the stick terms of channels 1 / 0 dropped when a d-pad axis is non-zero;
// each clamped to [-1, 1] and squared keeping its sign; the invert flags of
// *(0x00F65CA0)->4144 (0x40000000: out[1], 0x20000000: out[0]) negate.
// Leaves r3 / f1 as the compiled exits do.
DECOMP(001DFFB8, Cursor_ReadInput1DFFB8) {
    using mid15::Fsel15;
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    constexpr u32 K = 0x001DFF88u, P = 0x010B1A90u;
    Stack frame(ctx, 240);
    const u32 sp = frame.addr();
    const u32 c = load<u32>(o + 216);
    const f64 k0 = Lf(K), k4 = Lf(K + 4);
    const u32 btn = load<u32>(P + 8);
    const f64 b2 = (btn & 0x2u) ? k4 : k0, b1 = (btn & 0x1u) ? k4 : k0;
    const f64 b8 = (btn & 0x8u) ? k4 : k0, b4 = (btn & 0x4u) ? k4 : k0;
    f64 ch[4];
    const u32 order[4] = {1, 0, 3, 2};
    for (u32 j = 0; j < 4; ++j) {
        call(ctx, m16fn::Obj_GetChannel_0045B160, {P, order[j]});
        ch[order[j]] = ctx.fpr[1];
    }
    const f64 k8 = Lf(K + 8), k12 = Lf(K + 12), dz = Lf(c + 536), d16 = Ld(K + 16), d24 = Ld(K + 24);
    const f64 den = S(k4 - dz);
    auto axis = [&](f64 v) {
        const s32 sg = static_cast<s32>(static_cast<u32>(v > k8) - static_cast<u32>(v < k12));
        store<u64>(sp + 112, static_cast<u64>(static_cast<s64>(sg)));
        const f64 sf = S(static_cast<f64>(static_cast<s64>(sg)));
        f64 m = Divs(S(std::fabs(v) - dz), den);
        m = S(Fsel15(S(k4 - m), m, d16));
        m = S(Fsel15(m, m, d24));
        return S(sf * m);
    };
    const f64 v1 = axis(ch[1]), v0 = axis(ch[0]), v3 = axis(ch[3]), v2 = axis(ch[2]);
    const f64 ax = S(S(b2 - b1) * Lf(c + 544)), ay = S(S(b8 - b4) * Lf(c + 540));
    f64 s1 = S(v1 * Lf(c + 552)), s0 = S(v0 * Lf(c + 548));
    if (!(ax == k0) || !(ay == k0)) s1 = s0 = k0;
    f64 x = Fmadds(v3, Lf(c + 560), S(ax + s1));
    f64 y = Fmadds(v2, Lf(c + 556), S(ay + s0));
    const f64 k32 = Lf(K + 32), d40 = Ld(K + 40);
    x = S(Fsel15(S(k4 - x), x, d16));
    y = S(Fsel15(S(k4 - y), y, d16));
    x = S(Fsel15(S(x - k32), x, d40));
    y = S(Fsel15(S(y - k32), y, d40));
    Sf(out, x);
    Sf(out + 4, y);
    {
        const f64 a = Lf(out);
        Sf(out, S(a * std::fabs(a)));
    }
    const f64 b = Lf(out + 4);
    f64 f1 = std::fabs(b);
    const f64 bb = S(b * f1);
    Sf(out + 4, bb);
    const u32 fl = load<u32>(0x00F65CA0u) + 4144;
    u32 w = load<u32>(fl);
    if ((w & 0x40000000u) != 0) {
        f1 = -bb;
        Sf(out + 4, f1);
        w = load<u32>(fl);
    }
    const u32 inv0 = (w >> 29) & 1u;
    if (inv0 != 0) {
        f1 = -Lf(out);
        Sf(out, f1);
    }
    ctx.fpr[1] = f1;
    ret(ctx, inv0);
}

// u32 Obj_AlignToTarget0F1E00(Obj* o, f32 s)
// (callback) No target o->388 -> returns 0. When the def o->216 lacks flag
// 0x00400000 in +372, o's matrix o->96..152 becomes a copy of the
// target's +240 matrix (r3 = target + 296). Otherwise a frame is built on
// the stack (sp+176): z = the target's +432 (o->128 when that is within
// the f32 at 0x000F1DF8 of 0x01164B40 on every axis), x = o's y row (+112)
// x z (when that is ~0 too: the least-aligned axis 0x01164B50 / B60 / B70
// x z), y = z x x, translation = the target's matrix row 3;
// Matrix_NormalizeInPlace(frame), Matrix_RotationZ(sp+240, def->256 * s),
// Matrix_Multiply(frame, sp+240), and o->96..152 = the frame. f1 is left
// holding the last f32 copied.
DECOMP(000F1E00, Obj_AlignToTarget0F1E00) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 sc = ctx.fpr[1];
    Stack frame(ctx, 416);
    const u32 sp = frame.addr();
    const u32 T = load<u32>(o + 388), def = load<u32>(o + 216);
    if (T == 0) {
        ret(ctx, 0);
        return;
    }
    if ((load<u32>(def + 372) & 0x00400000u) == 0) {
        for (u32 k = 0; k < 12; ++k) Cw(sp + 304 + MOff16(k), T + 240 + MOff16(k));
        for (u32 k = 0; k < 12; ++k) Cw(o + 96 + MOff16(k), sp + 304 + MOff16(k));
        ctx.fpr[1] = Lf(sp + 360);
        ret(ctx, T + 296);
        return;
    }
    const u32 F = sp + 176;
    for (u32 k = 0; k < 6; ++k) Cw(F + MOff16(k), o + 96 + MOff16(k));
    const f64 yx = Lf(o + 112), yy = Lf(o + 116), yz = Lf(o + 120);
    for (u32 k = 0; k < 12; ++k) Cw(sp + 112 + MOff16(k), T + 240 + MOff16(k));
    Cw(F + 52, sp + 164);
    Cw(F + 56, sp + 168);
    Cw(F + 48, sp + 160);
    f64 zx = Lf(T + 432), zy = Lf(T + 436), zz = Lf(T + 440);
    Sf(F + 32, zx);
    Sf(F + 36, zy);
    Sf(F + 40, zz);
    constexpr u32 V = 0x01164B40u;
    const f64 eps = Lf(0x000F1DF8u);
    auto near0 = [&](f64 a, f64 b, f64 c) {
        return std::fabs(S(a - Lf(V))) < eps && std::fabs(S(b - Lf(V + 4))) < eps && std::fabs(S(c - Lf(V + 8))) < eps;
    };
    if (near0(zx, zy, zz)) {
        zx = Lf(o + 128);
        Sf(F + 32, zx);
        zy = Lf(o + 132);
        Sf(F + 36, zy);
        zz = Lf(o + 136);
        Sf(F + 40, zz);
    }
    f64 xx = Fmsubs(yy, zz, S(yz * zy)), xy = Fmsubs(yz, zx, S(yx * zz)), xz = Fmsubs(yx, zy, S(yy * zx));
    Sf(F, xx);
    Sf(F + 4, xy);
    Sf(F + 8, xz);
    if (near0(xx, xy, xz)) {
        const f64 az = std::fabs(zz), ay = std::fabs(zy);
        u32 A = 0x01164B50u;
        if (az < ay && az < std::fabs(zx)) A = 0x01164B70u;
        else if (ay < std::fabs(zx)) A = 0x01164B60u;
        const f64 ax_ = Lf(A), ay_ = Lf(A + 4), az_ = Lf(A + 8);
        xx = Fmsubs(ay_, zz, S(az_ * zy));
        xy = Fmsubs(az_, zx, S(ax_ * zz));
        xz = Fmsubs(ax_, zy, S(ay_ * zx));
        Sf(F, xx);
        Sf(F + 4, xy);
        Sf(F + 8, xz);
    }
    Sf(F + 16, Fmsubs(zy, xz, S(zz * xy)));
    Sf(F + 20, Fmsubs(zz, xx, S(zx * xz)));
    Sf(F + 24, Fmsubs(zx, xy, S(zy * xx)));
    call(ctx, m16fn::Matrix_NormalizeInPlace, {F});
    callf(ctx, 0x00553F18u, {sp + 240}, {S(Lf(def + 256) * sc)});  // Matrix_RotationZ
    call(ctx, 0x005541F0u, {F, sp + 240});                           // Matrix_Multiply
    for (u32 k = 0; k < 12; ++k) Cw(o + 96 + MOff16(k), F + MOff16(k));
    ctx.fpr[1] = Lf(F + 56);
}
