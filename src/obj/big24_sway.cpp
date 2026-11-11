// Game code, large-function sweep part 24: a swaying / tilting prop's tick.
#include "../sly4.h"
#include "big24.h"

using namespace sly4;
using namespace sly4::big24;

namespace {
constexpr u32 kSinf = 0x007345F0;           // f1 -> f1
constexpr u32 kCosf = 0x007344E0;           // f1 -> f1
constexpr u32 kMat34FromRotPos = 0x00553B4C;  // (Mat34* out, const Mat33*, const Vec3*)
constexpr u32 kVec3ToItemSpace = 0x00553468;  // (f32 v[3] in/out, const f32 m[3][4])
constexpr u32 kQuatToMatrix = 0x00553A90;   // (Mat44* out, const Quat*, const Vec3* pos)
constexpr u32 kMatrixMulRot = 0x00553FF0;   // (out, a, b)
constexpr u32 kSetEffectHash = 0x000DAEC8;  // Obj_SetEffectHash_000DAEC8 (Obj*, hash)
constexpr u32 kC = 0x003FA0D0;              // .text f32 table (2pi, 8.6, 1.2, 0.2, 11.7, 2.2, 7, 3.7, 5, 0.5, 0, -pi, 180, 1, 0.001, 1e-4)
constexpr u32 kZeroVec = 0x01164B40;
constexpr u32 kUp = 0x01164B60;
constexpr u32 kGame = 0x00F65CA0;           // Game* ; +0x1281E0 the reference object (player?)
constexpr u32 kHashTiltUp = 0x603D2AEEu;    // effect when the tilt speed turns positive
constexpr u32 kHashTiltDown = 0xA7BE93F5u;  // ... negative

f64 C(u32 off) { return Lf(kC + off); }
}  // namespace

// void f(Prop* o; f32 dt) (called by 003FC468, 003FD8D0). o->+88 = params
// (+32 bob amplitude, +44 tilt limit), o->+256 the body.
// 1. Bob: three phases +144/+148/+152 (periods 8.6, 11.7, 7; each phase
//    advances by dt and wraps by subtracting 8.6, 3.7 (sic) and 7 when
//    past its period) give y = sum of amp * sin(2pi * p / T + phase)
//    weighted 1, 0.2, 0.2 (as fmadds) plus +136; the body (vfn[84]) gets
//    Mat34_FromRotPos(its rows +96/+112/+128, o->+208 with y added).
// 2. Tilt: +124 angle, +132 speed: target clamp(+140 * limit, +-limit)
//    pulls the speed with gain 5 (acc = (target - angle) * 5 - speed),
//    angle += speed dt + acc dt^2 / 2 (fmadds order kept); clamped to the
//    limit, the speed zeroed when pushing outwards. A sign change of the
//    speed (u32 +368 state 1 / -1) fires the effect 0x603D2AEE /
//    0xA7BE93F5 on the body when its vfn[272] is set. +136 / +140 are
//    cleared.
// 3. With the game's reference object r (0x00F65CA0 -> +0x1281E0): a =
//    r->+96 * cos(-pi angle / 180) + up * sin(...), b = r->+96, both into
//    o's frame +160 (Vec3_ToItemSpace); the rotation from b to a
//    (normalized; half-vector quaternion, or a perpendicular axis when
//    they're opposite) goes through Quat_ToMatrix / Matrix_MulRot(+160)
//    to the body's vfn[84] with the body's position +144.
DECOMP(003FA110, Prop_SwayTick_003FA110) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    Stack frame(ctx, 496);
    const u32 sp = frame.addr();
    const u32 pos = sp + 112, q = sp + 160, rot = sp + 176, qm = sp + 224, m2 = sp + 288, m1 = sp + 352;
    const u32 va = sp + 128, vb = sp + 144;

    // 1. Bob.
    const f64 s1 = callf(ctx, kSinf, {}, {Adds(S(S(Lf(o + 144) * C(0)) / C(4)), C(8))});
    const f64 w = C(12);
    f64 acc = S(s1 * Lf(load<u32>(o + 88) + 32));
    const f64 s2 = callf(ctx, kSinf, {}, {Adds(S(S(Lf(o + 148) * C(0)) / C(16)), C(20))});
    const f64 t2 = S(S(s2 * Lf(load<u32>(o + 88) + 32)) * w);
    const f64 a3 = S(S(Lf(o + 152) * C(0)) / C(24));
    acc = Fmadds(acc, w, t2);
    const f64 s3 = callf(ctx, kSinf, {}, {a3});
    const f64 p0 = Adds(Lf(o + 144), dt), p1 = Adds(Lf(o + 148), dt), p2 = Adds(Lf(o + 152), dt);
    const f64 amp = Lf(load<u32>(o + 88) + 32);
    const f64 base_y = Lf(o + 136);
    const u32 body = load<u32>(o + 256);
    Sf(o + 144, p0);
    Sf(o + 148, p1);
    Sf(o + 152, p2);
    const f64 y = Adds(Fmadds(S(s3 * amp), w, acc), base_y);
    if (p0 > C(4)) Sf(o + 144, Subs(p0, C(4)));
    if (p1 > C(16)) Sf(o + 148, Subs(p1, C(28)));  // quirk: wraps by 3.7, not by the 11.7 period
    if (p2 > C(24)) Sf(o + 152, Subs(p2, C(24)));
    Cw(pos, o + 208);
    Sf(pos + 4, Adds(Lf(o + 212), y));
    Cw(pos + 8, o + 216);
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k) Cw(rot + 16 * r + 4 * k, body + 96 + 16 * r + 4 * k);
    call(ctx, kMat34FromRotPos, {m1, rot, pos});
    VCall(ctx, body, 84, {body, m1});

    // 2. Tilt.
    const f64 lim = Lf(load<u32>(o + 88) + 44);
    f64 tgt = S(Lf(o + 140) * lim);
    const f64 ang0 = Lf(o + 124), spd0 = Lf(o + 132);
    const u32 body2 = load<u32>(o + 256);
    if (tgt > lim) tgt = lim;
    const f64 nlim = -lim;
    if (tgt < nlim) tgt = nlim;
    f64 ang = Fmadds(spd0, dt, ang0);
    const f64 a = Fmsubs(S(tgt - ang0), C(32), spd0);
    ang = Fmadds(S(S(a * C(36)) * dt), dt, ang);
    f64 spd = Fmadds(a, dt, spd0);
    Sf(o + 124, ang);
    Sf(o + 132, spd);
    if (ang > lim) {
        ang = lim;
        Sf(o + 124, lim);
        if (spd > C(40)) {
            spd = C(40);
            store<u32>(o + 132, 0);
        }
    }
    if (ang < nlim) {
        Sf(o + 124, nlim);
        if (spd < C(40)) {
            spd = C(40);
            store<u32>(o + 132, 0);
        }
    }
    ctx.fpr[1] = C(40);  // as the original leaves f1 for the calls below
    if (spd > C(40)) {
        if (static_cast<s32>(load<u32>(o + 368)) < 0 && VCall(ctx, body2, 272, {body2}) != 0)
            call(ctx, kSetEffectHash, {load<u32>(o + 256), kHashTiltUp});
        store<u32>(o + 368, 1);
    } else if (spd < C(40)) {
        if (static_cast<s32>(load<u32>(o + 368)) > 0 && VCall(ctx, body2, 272, {body2}) != 0)
            call(ctx, kSetEffectHash, {load<u32>(o + 256), kHashTiltDown});
        store<u32>(o + 368, 0xFFFFFFFFu);
    }
    store<u32>(o + 136, 0);
    store<u32>(o + 140, 0);
    const u32 game = load<u32>(kGame);
    const u32 ref = load<u32>(game + 0x1281E0u);
    if (ref == 0) {
        ctx.gpr[3] = game;  // as the original leaves r3
        return;
    }

    // 3. Lean towards the reference object.
    const f64 k_pi = C(44), k_180 = C(48);
    const f64 rx = Lf(ref + 96), ry = Lf(ref + 100), rz = Lf(ref + 104);
    const f64 c = callf(ctx, kCosf, {}, {S(S(Lf(o + 124) * k_pi) / k_180)});
    const f64 s = callf(ctx, kSinf, {}, {S(S(Lf(o + 124) * k_pi) / k_180)});
    const u32 frm = o + 160;
    Cw(vb, ref + 96);
    Cw(vb + 4, ref + 100);
    Cw(vb + 8, ref + 104);
    Sf(va, Fmadds(rx, c, S(Lf(kUp) * s)));
    Sf(va + 4, Fmadds(ry, c, S(Lf(kUp + 4) * s)));
    Sf(va + 8, Fmadds(rz, c, S(Lf(kUp + 8) * s)));
    call(ctx, kVec3ToItemSpace, {va, frm});
    call(ctx, kVec3ToItemSpace, {vb, frm});
    const f64 bx0 = Lf(vb), by0 = Lf(vb + 4), bz0 = Lf(vb + 8);
    const f64 ax0 = Lf(va), ay0 = Lf(va + 4), az0 = Lf(va + 8);
    const f64 lb = Fmadds(bz0, bz0, Fmadds(bx0, bx0, S(by0 * by0)));
    const f64 la = Fmadds(az0, az0, Fmadds(ax0, ax0, S(ay0 * ay0)));
    const f64 one = C(52);
    const f64 ib = S(one / S(std::sqrt(lb))), ia = S(one / S(std::sqrt(la)));
    const f64 by = S(ib * by0), bz = S(ib * bz0), bx = S(ib * bx0);
    const f64 ay = S(ia * ay0), ax = S(ia * ax0), az = S(ia * az0);
    const f64 hy = Adds(by, ay), hx = Adds(bx, ax), hz = Adds(bz, az);
    const f64 d = Fmadds(hz, bz, Fmadds(hx, bx, S(hy * by)));
    if (std::fabs(d) > C(56)) {
        const f64 qy = Fmsubs(bz, ax, S(bx * az));
        const f64 qx = Fmsubs(by, az, S(bz * ay));
        const f64 qz = Fmsubs(bx, ay, S(by * ax));
        const f64 l = Fmadds(d, d, Fmadds(qz, qz, Fmadds(qx, qx, S(qy * qy))));
        const f64 inv = S(one / S(std::sqrt(l)));
        Sf(q, S(qx * inv));
        Sf(q + 4, S(qy * inv));
        Sf(q + 8, S(qz * inv));
        Sf(q + 12, S(d * inv));
    } else {
        // b and a opposite: a quarter turn about an axis perpendicular to b.
        const bool xbig = bx > by;
        const f64 l = S(std::sqrt(Fmadds(xbig ? bx : by, xbig ? bx : by, S(bz * bz))));
        if (l < C(60)) {
            store<u32>(q, 0);
            store<u32>(q + 4, 0);
            store<u32>(q + 8, 0);
            store<u32>(q + 12, 0x3F800000u);
        } else if (xbig) {
            const f64 inv = S(one / l);
            store<u32>(q + 4, 0);
            store<u32>(q + 12, 0);
            Sf(q + 8, S(inv * bx));
            Sf(q, -S(inv * bz));
        } else {
            const f64 inv = S(one / l);
            store<u32>(q, 0);
            store<u32>(q + 12, 0);
            Sf(q + 4, S(inv * bz));
            Sf(q + 8, -S(inv * by));
        }
    }
    call(ctx, kQuatToMatrix, {qm, q, kZeroVec});
    call(ctx, kMatrixMulRot, {m2, qm, frm});
    const u32 body3 = load<u32>(o + 256);
    Cw(m2 + 48, body3 + 144);
    Cw(m2 + 52, body3 + 148);
    Cw(m2 + 56, body3 + 152);
    ctx.fpr[1] = Lf(body3 + 152);  // the original copies the position through f1
    VCall(ctx, body3, 84, {body3, m2});
}
