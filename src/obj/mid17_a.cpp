// Game code, medium sweep part 17: facing and camera helpers.
#include "mid17.h"

using namespace sly4;
using namespace sly4::mid17;

// void Obj_SetFacing2C5140(Obj* o, const Vec3* facing)
// (called by 002C5780, 002C5E30, Obj_TurnToFocus2C6F78, 002C7340) Sets the
// facing o->208..216 from facing (raw words), then: when o->276's
// vfn[64]() returns an object whose +416 field (bits 1..3 = (w >> 28) & 7)
// is 3, rotates it by Cam_RotateToward1007D0 about row 2 (z) of o->276's
// matrix (+96, copied to the frame) by the angle o->168. When the turn
// rate o->256 is at least *(0x002C5130) (not NaN): if the target o->224
// finds an object (Obj_FindById), o->240..248 = row 2 of that object's
// matrix (+96 +32..40) with y = 0, normalized (below *(0x002C5134) squared
// length the constant 0x01164B70); then o->208 = Vec_TurnToward106770
// (o->240, o->208, o->256). Last, o->208 = Obj_SetRange_00101958(o->208,
// o->124, o->128). Returns what that last call returns; f1 = the copied z.
DECOMP(002C5140, Obj_SetFacing2C5140) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    Stack frame(ctx, 240);
    const u32 sp = frame.addr();
    CopyVec3(o + 208, v);
    ctx.fpr[1] = Lf(o + 216);  // f1 as the facing copy leaves it
    {
        const u32 x = load<u32>(o + 276);
        const u32 p = static_cast<u32>(VCall(ctx, x, 64, {x}));
        if (p != 0 && ((load<u32>(p + 416) >> 28) & 7) == 3) {
            CopyMat34(sp + 160, load<u32>(o + 276) + 96);
            callf(ctx, m17fn::Cam_RotateToward1007D0, {sp + 128, sp + 192, o + 208}, {Lf(o + 168)});
            CopyVec3(o + 208, sp + 128);
        }
    }
    if (Lf(o + 256) >= Lf(0x002C5130u)) {
        const u32 t = load<u32>(o + 224);
        if (t != 0) {
            ctx.fpr[1] = Lf(o + 256);  // f1 as left by the rate test
            const u32 r = static_cast<u32>(call(ctx, m17fn::Obj_FindById, {t}));
            if (r != 0) {
                const u32 m = r + 96;
                Cw(o + 244, m + 36);
                Cw(o + 240, m + 32);
                Cw(o + 248, m + 40);
                store<u32>(o + 244, 0);  // y = 0
                const f64 y = Lf(o + 244), x = Lf(o + 240), z = Lf(o + 248);
                const f64 l2 = Fmadds(z, z, Fmadds(x, x, S(y * y)));
                if (std::fabs(l2) < Lf(0x002C5134u)) {
                    CopyVec3(o + 240, 0x01164B70u);
                } else {
                    const f64 k = Divs(Lf(0x002C5138u), Sqrts(l2));
                    Sf(o + 240, S(x * k));
                    Sf(o + 244, S(y * k));
                    Sf(o + 248, S(z * k));
                }
            }
        }
        callf(ctx, m17fn::Vec_TurnToward106770, {sp + 144, o + 240, o + 208}, {Lf(o + 256)});
        CopyVec3(o + 208, sp + 144);
    }
    callf(ctx, m17fn::Obj_SetRange_00101958, {sp + 112, o + 208}, {Lf(o + 124), Lf(o + 128)});
    const u32 r = static_cast<u32>(ctx.gpr[3]);
    CopyVec3(o + 208, sp + 112);
    ctx.fpr[1] = Lf(sp + 120);
    ret(ctx, r);
}

// u32 Ring_ComputeProfile3EC3A0(-, Ring* o)
// (called by 003ECD50) Fills the 25-sample profile o->4[25] and its
// smoothed copy o->104[25]. With the constants K at 0x003EC35C (K+0 2, +4
// pi, +8 3, +12 0.1, +16 0.02, +20 2 pi, +24 25, +28 1, +32..+56 the eight
// sector starts 0, pi/4 .. 7pi/4, +60 the smoothing weight): w =
// cos(o->212 * 2 * pi / 3) * 0.1 + cos(o->216 * 2 * pi) * 0.02; for sample
// i at angle a = i * 2 pi / 25, sector k with neighbour links (o->232,
// 252, 244, 260, 236, 256, 240, 248 cyclically) adds t * 3 / (1 - t) * 3
// to the level o->204 and t / (1 - t) to the weight 1 for each linked
// neighbour that is not hidden (+268 bit 0x40000000), t = (pi/4 - (a -
// start)) / (pi/4); sample = min(3, s * level / weight + o->224 + w) * 2 /
// 3 + 1, s = (o->0->348 - 1) / (o->0->344 - 1). Then o->104[i] = (o->4[i +
// 1] + o->4[i - 1]) (cyclic) * K60 + o->4[i], times K60. Returns early
// (no writes) when o itself is hidden (o->268 bit 0x40000000), with f1 =
// w; else f1 = K60. r3 = 0 either way.
DECOMP(003EC3A0, Ring_ComputeProfile3EC3A0) {
    const u32 o = arg<u32>(ctx, 1);
    constexpr u32 K = 0x003EC35Cu;
    const f64 two = Lf(K), pi = Lf(K + 4), three = Lf(K + 8), one = Lf(K + 28), q = Lf(K + 36);
    const f64 c1 = callf(ctx, 0x007344E0u, {}, {Divs(S(S(Lf(o + 212) * two) * pi), three)});
    const f64 c2 = callf(ctx, 0x007344E0u, {}, {S(S(Lf(o + 216) * two) * pi)});
    const f64 w = Fmadds(c1, Lf(K + 12), S(c2 * Lf(K + 16)));
    // sector starts (the last sector has no end) and neighbour links
    const f64 B[9] = {Lf(K + 32), q, Lf(K + 40), Lf(K + 44), pi, Lf(K + 48), Lf(K + 52), Lf(K + 56), 0.0};
    static constexpr u32 L[9] = {232, 252, 244, 260, 236, 256, 240, 248, 232};
    auto shown = [&](u32 p) { return p != 0 && (load<u32>(p + 268) & 0x40000000u) == 0; };
    for (s32 i = 0; i < 25; ++i) {
        const f64 a = Divs(S(S(static_cast<f64>(i)) * Lf(K + 20)), Lf(K + 24));
        const u32 p = load<u32>(o);
        f64 lv = Lf(o + 204);
        const f64 hi = Lf(p + 348), lo = Lf(p + 344);
        if ((load<u32>(o + 268) & 0x40000000u) != 0) {
            ctx.fpr[1] = w;
            ret(ctx, 0);
            return;
        }
        const f64 s = Divs(S(hi - one), S(lo - one));
        f64 wt = one;
        for (u32 k = 0; k < 8; ++k) {
            if (!(a >= B[k]) || (k < 7 && !(a < B[k + 1]))) continue;
            const f64 t = Divs(S(q - S(a - B[k])), q);
            if (shown(load<u32>(o + L[k]))) {
                lv = Fmadds(t, three, lv);
                wt = S(t + wt);
            }
            if (shown(load<u32>(o + L[k + 1]))) {
                const f64 u = S(one - t);
                lv = Fmadds(u, three, lv);
                wt = S(u + wt);
            }
        }
        f64 v = S(S(Divs(S(s * lv), wt) + Lf(o + 224)) + w);
        if (v > three) v = three;
        Sf(o + 4 + 4 * i, S(Divs(S(v * two), three) + one));
    }
    const f64 m = Lf(K + 60);
    for (s32 i = 0; i < 25; ++i) {
        const s32 n = i + 1 < 25 ? i + 1 : 0, pv = i - 1 < 0 ? 24 : i - 1;
        const f64 sum = S(Lf(o + 4 + 4 * n) + Lf(o + 4 + 4 * pv));
        Sf(o + 104 + 4 * i, S(Fmadds(sum, m, Lf(o + 4 + 4 * i)) * m));
    }
    ctx.fpr[1] = m;
    ret(ctx, 0);
}

// u32 Obj_ForwardEulerPose2025B0(Obj* o, Obj* x)
// (called by Thunk_002027B0) Builds a position and a quaternion and hands
// them to World_Forward12847C_00489DF8(*(0x00F65CA0), x, o, -, id, &pos,
// &quat, 0x010549D0; f1 = o->216->440). By default pos and the Euler
// angles are the zero vector 0x01164B40 and id 0; when x has flag 0x8 in
// +192, they come from d = x->216: pos d+1536, angles d+1552, id d->1520.
// With h = *(0x002025AC) (0.5), a = angle * h and c/s = cos/sin of
// (y, z, x): quat = {s1 s2 c0 + c1 c2 s0, c2 s1 c0 + c1 s2 s0, c1 s2 c0 -
// c2 s1 s0, c1 c2 c0 - s1 s2 s0}. Returns what the forward call returns.
DECOMP(002025B0, Obj_ForwardEulerPose2025B0) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    Stack frame(ctx, 240);
    const u32 sp = frame.addr();
    constexpr u32 Z = 0x01164B40u;
    u32 id = 0;
    CopyVec3(sp + 128, Z);
    f64 ax = Lf(Z), ay = Lf(Z + 4), az = Lf(Z + 8);
    const f64 scale = Lf(load<u32>(o + 216) + 440);
    if (x != 0 && (load<u32>(x + 192) & 0x8u) != 0) {
        const u32 d = load<u32>(x + 216);
        id = load<u32>(d + 1520);
        CopyVec3(sp + 128, d + 1536);
        ax = Lf(d + 1552);
        ay = Lf(d + 1556);
        az = Lf(d + 1560);
    }
    const f64 h = Lf(0x002025ACu);
    const u32 g = load<u32>(0x00F65CA0u);
    const f64 a1 = S(ay * h), a2 = S(az * h), a0 = S(ax * h);
    const f64 c1 = callf(ctx, 0x007344E0u, {}, {a1});  // Math_Cosf
    const f64 c2 = callf(ctx, 0x007344E0u, {}, {a2});
    const f64 c0 = callf(ctx, 0x007344E0u, {}, {a0});
    const f64 s1 = callf(ctx, 0x007345F0u, {}, {a1});  // Math_Sinf
    const f64 s2 = callf(ctx, 0x007345F0u, {}, {a2});
    const f64 s0 = callf(ctx, 0x007345F0u, {}, {a0});
    const f64 p2 = S(s1 * s2), p3 = S(c1 * c2), p4 = S(c2 * s1), p5 = S(c1 * s2);
    Sf(sp + 124, Fmsubs(p3, c0, S(p2 * s0)));
    Sf(sp + 112, Fmadds(p2, c0, S(p3 * s0)));
    Sf(sp + 120, Fmsubs(p5, c0, S(p4 * s0)));
    Sf(sp + 116, Fmadds(p4, c0, S(p5 * s0)));
    ctx.fpr[1] = scale;
    call(ctx, 0x00489DF8u, {g, x, o, ctx.gpr[6], id, sp + 128, sp + 112, 0x010549D0u});  // World_Forward12847C_00489DF8 (r6 unread)
}

// u32 Obj_SwayTick5A388(Obj* o)
// (called by 0005A6A0) With the tuning values T(a) = the f32 at a + 72 and
// the constants c = 0x0005A374: s = |sinf(o->36 / T(0x00F99A94) * c[2])|,
// level = s * (T(0x00F99B78) - T(0x00F99B2C)) + T(0x00F99B2C), v =
// level * T(0x00F99CA8). When the timer o->5136 > T(0x00F999FC) / c[0], it
// is reset to 0 and a frame is built: the matrix 0x01164BD0 with row 3 =
// o->176 + row 1 * c[4], multiplied by the rotation about 0x01164B60 by
// T(0x00F99A48) * c[1] * o->36 + T(0x00F99BC4) * c[1] (Matrix_FromAxisAngle,
// zero translation), normalized; then 00058488(o, &frame; f1 = T(0x00F99AE0)
// + v) with {that, T(0x00F99AE0), v * c[3] + T(0x00F99AE0)} at sp+240 and
// {T(0x00F99BC4), T(0x00F99C10), T(0x00F99C5C)} * c[1] at sp+252.
DECOMP(0005A388, Obj_SwayTick5A388) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 C = 0x0005A374u;
    auto T = [](u32 a) { return Lf(a + 72); };
    Stack frame(ctx, 368);
    const u32 sp = frame.addr();
    const f64 a0 = Divs(Lf(o + 36), T(0x00F99A94u));
    const f64 lim = Divs(T(0x00F999FCu), Lf(C));
    const f64 c1 = Lf(C + 4);
    const f64 rate = S(T(0x00F99A48u) * c1);
    const f64 s = std::fabs(callf(ctx, 0x007345F0u, {}, {S(a0 * Lf(C + 8))}));  // Math_Sinf
    const f64 base = T(0x00F99B2Cu);
    const f64 span = S(T(0x00F99B78u) - base);
    const f64 tm = Lf(o + 5136);
    const f64 k = T(0x00F99CA8u), g = T(0x00F99AE0u);
    const f64 lv = Fmadds(s, span, base);
    const f64 ax = S(T(0x00F99BC4u) * c1), ay = S(T(0x00F99C10u) * c1), az = S(T(0x00F99C5Cu) * c1);
    const f64 v = S(lv * k);
    const f64 hi = S(g + v), mid = Fmadds(v, Lf(C + 12), g);
    if (!(tm > lim)) {
        ctx.fpr[1] = az;
        ret(ctx, 0x00F99BC4u);
        return;
    }
    store<u32>(o + 5136, 0);
    constexpr u32 X = 0x01164BD0u;
    const f64 c4 = Lf(C + 16);
    const u32 m = sp + 112, r = sp + 176;
    const f64 r3x = Fmadds(Lf(X + 16), c4, Lf(o + 176));
    const f64 r3y = Fmadds(Lf(X + 20), c4, Lf(o + 180));
    const f64 r3z = Fmadds(Lf(X + 24), c4, Lf(o + 184));
    for (u32 k3 = 0; k3 < 9; ++k3) Cw(m + MOff16(k3), X + MOff16(k3));
    Sf(m + 48, r3x);
    Sf(m + 52, r3y);
    Sf(m + 56, r3z);
    Sf(sp + 260, az);
    Sf(sp + 256, ay);
    Sf(sp + 252, ax);
    callf(ctx, 0x0056A410u, {r, 0x01164B60u}, {Fmadds(rate, Lf(o + 36), ax)});  // Matrix_FromAxisAngle
    CopyVec3(r + 48, 0x01164B40u);
    call(ctx, 0x005541F0u, {m, r});  // Matrix_Multiply
    call(ctx, 0x0056D374u, {m});     // Matrix_NormalizeInPlace
    Sf(sp + 248, mid);
    Sf(sp + 244, g);
    Sf(sp + 240, hi);
    callf(ctx, 0x00058488u, {o, m}, {hi});
}

// u32 Obj_FollowSpline3CD290(Obj* o; f32 dt)
// (called by 003CD4A0, 003CE2C0, 003CF0F8) Reads the stick of o's player
// (game *(0x00F65CA0), player o->84 (stride 0x20C80) index byte +4209 into
// the pads 0x010B19E0 + 11968 * i, channel block +176): s =
// Obj_GetChannel_0045B160(pad, 0); when |{c[0], s, ch1}| < c[1] (c =
// 0x003CD280) the pad's +8 flags 0x4 / 0x8 replace s with *(0x01164B80) /
// *(0x01164B50). With the spline object *(game + 0x1281E0) set: pos/tan =
// WorldObj_SplinePointTangentAt(o->108, -, &pos, &tan, 0; f1 = o->104),
// pos += tan * s * c[2], 003CCC68(o, &pos); and when dt > c[0], o->32..40
// = (o->16..24 - the old pos) * (c[3] / dt). Returns (r3) c with a dt
// above c[0], else the last call's r3 (0 without a spline object; f1 =
// the stick length then).
DECOMP(003CD290, Obj_FollowSpline3CD290) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    constexpr u32 C = 0x003CD280u;
    Stack frame(ctx, 208);
    const u32 sp = frame.addr();
    const u32 g = load<u32>(0x00F65CA0u);
    const u32 i = load<u8>(g + load<u32>(o + 84) * 0x20C80u + 4209);
    const u32 pad = 0x010B19E0u + i * 11968u + 176;
    call(ctx, 0x0045B160u, {pad, 0});  // Obj_GetChannel_0045B160
    f64 s = ctx.fpr[1];
    call(ctx, 0x0045B160u, {pad, 1});
    const f64 s1 = ctx.fpr[1];
    const f64 k0 = Lf(C);
    const f64 len = Sqrts(Fmadds(s1, s1, Fmadds(s, s, S(k0 * k0))));
    const u32 w = load<u32>(pad + 8);
    if (len < Lf(C + 4)) {
        if (w & 0x4u) s = Lf(0x01164B80u);
        if (w & 0x8u) s = Lf(0x01164B50u);
    }
    if (load<u32>(g + 0x1281E0u) == 0) {
        ctx.fpr[1] = len;
        ret(ctx, 0);
        return;
    }
    const u32 pos = sp + 128, tan = sp + 112;
    callf(ctx, 0x005A80D8u, {load<u32>(o + 108), 0x001281E0u, pos, tan, 0}, {Lf(o + 104)});  // WorldObj_SplinePointTangentAt
    const f64 tx = S(Lf(tan) * s), ty = S(Lf(tan + 4) * s), tz = S(Lf(tan + 8) * s);
    const f64 px = Lf(pos), py = Lf(pos + 4), pz = Lf(pos + 8);
    const f64 k2 = Lf(C + 8);
    const f64 nx = Fmadds(tx, k2, px), ny = Fmadds(ty, k2, py), nz = Fmadds(tz, k2, pz);
    Sf(pos, nx);
    Sf(pos + 4, ny);
    Sf(pos + 8, nz);
    ctx.fpr[1] = nx;
    ctx.fpr[2] = ny;
    ctx.fpr[3] = nz;
    call(ctx, 0x003CCC68u, {o, pos});
    if (dt > k0) {
        const f64 inv = Divs(Lf(C + 12), dt);
        Sf(o + 32, S(S(Lf(o + 16) - px) * inv));
        Sf(o + 36, S(S(Lf(o + 20) - py) * inv));
        const f64 z = S(S(Lf(o + 24) - pz) * inv);
        Sf(o + 40, z);
        ctx.fpr[1] = z;
        ret(ctx, C);  // r3 = the constant block address
    }
}

// u32 Params_Blend115900(Params* a, const Params* b; f32 t)
// (called by Obj_Update3_00115EB0) Eases a toward b where they differ by
// at least eps = *(0x001158FC) (or NaN): a->8 = 004A83D8(a->8, b->8,
// b->4, t); the vector a->16..24 (when any component differs) =
// Curve_Lerp3_004A8598(a+16, b+16; b->0, t); a->32 / 36 / 40 / 44 each =
// 004A83D8(a, b, b->0, t); then a->48 = b->48. r3 = the last call's (a
// without calls), f1 = b->48.
DECOMP(00115900, Params_Blend115900) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const f64 t = ctx.fpr[1];
    Stack frame(ctx, 160);
    const f64 eps = Lf(0x001158FCu);
    auto near = [&](u32 off) { return std::fabs(S(Lf(a + off) - Lf(b + off))) < eps; };
    auto ease = [&](u32 off, u32 k) {
        if (near(off)) return;
        Sf(a + off, callf(ctx, 0x004A83D8u, {}, {Lf(a + off), Lf(b + off), Lf(b + k), t}));
    };
    ease(8, 4);
    if (!near(16) || !near(20) || !near(24)) {
        const u32 sp = frame.addr();
        callf(ctx, 0x004A8598u, {sp + 112, a + 16, b + 16}, {Lf(b), t});  // Curve_Lerp3_004A8598
        CopyVec3(a + 16, sp + 112);
        ctx.fpr[1] = Lf(sp + 120);
    }
    for (u32 off = 32; off <= 44; off += 4) ease(off, 0);
    const u32 w = load<u32>(b + 48);
    store<u32>(a + 48, w);
    ctx.fpr[1] = LfsF(w);
}
