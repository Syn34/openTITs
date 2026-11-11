// Game code, medium sweep part 15: frames and bases.
#include "mid15.h"

using namespace sly4;
using namespace sly4::mid15;

// void Frame_SetForward1CD8C0(Obj* o, const f32 dir[3])
// (called by 001CE2A8) d = dir (y = 0 when byte +784 has 0x80); within
// 1e-5 of the global vec3 0x01164B40 in every axis the previous forward
// o+592 is kept, otherwise d is normalised. With the global up U
// (0x01164B60): side c = unit(U x d) -> +560 and +624, up' = d x c ->
// +576 and +640, d -> +592, +656 and +720; +608 is rewritten in place and
// copied to +672. Then +640 = U, +656 = (+624) x (+640) (re-read), and
// +672 gets back the value it held on entry. Constants at 0x001CD8B4.
// f1 is left as d.y.
DECOMP(001CD8C0, Frame_SetForward1CD8C0) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 K = 0x001CD8B4u;  // f32 0, 1e-5, 1
    f64 y = Lf(v + 4), x = Lf(v), z = Lf(v + 8);
    if ((load<u8>(o + 784) & 0x80u) != 0) y = Lf(K);
    const u32 G = 0x01164B40u;
    const f64 eps = Lf(K + 4);
    if (std::fabs(S(x - Lf(G))) < eps && std::fabs(S(y - Lf(G + 4))) < eps && std::fabs(S(z - Lf(G + 8))) < eps) {
        x = Lf(o + 592);
        y = Lf(o + 596);
        z = Lf(o + 600);
    } else {
        const f64 inv = S(Lf(K + 8) / S(std::sqrt(Fmadds(z, z, Fmadds(x, x, S(y * y))))));
        x = S(x * inv);
        y = S(y * inv);
        z = S(z * inv);
    }
    const u32 U = 0x01164B60u;
    const f64 u0 = Lf(U), u1 = Lf(U + 4), u2 = Lf(U + 8);
    f64 cy = Fmsubs(u2, x, S(u0 * z));
    f64 cx = Fmsubs(u1, z, S(u2 * y));
    f64 cz = Fmsubs(u0, y, S(u1 * x));
    const f64 one = Lf(K + 8);
    const u32 w0 = load<u32>(o + 608), w1 = load<u32>(o + 612), w2 = load<u32>(o + 616);
    Sf(o + 592, x);
    Sf(o + 596, y);
    Sf(o + 600, z);
    store<u32>(o + 608, w0);
    store<u32>(o + 612, w1);
    store<u32>(o + 616, w2);
    const f64 il = S(one / S(std::sqrt(Fmadds(cz, cz, Fmadds(cx, cx, S(cy * cy))))));
    cx = S(cx * il);
    cy = S(cy * il);
    cz = S(cz * il);
    Sf(o + 560, cx);
    Sf(o + 564, cy);
    Sf(o + 568, cz);
    const f64 ez = Fmsubs(x, cy, S(y * cx));
    const f64 ex = Fmsubs(y, cz, S(z * cy));
    const f64 ey = Fmsubs(z, cx, S(x * cz));
    Sf(o + 584, ez);
    Sf(o + 576, ex);
    Sf(o + 580, ey);
    const u32 s0 = load<u32>(o + 672), s1 = load<u32>(o + 676), s2 = load<u32>(o + 680);
    Sf(o + 624, cx);
    Sf(o + 628, cy);
    Sf(o + 632, cz);
    Sf(o + 640, ex);
    Sf(o + 644, ey);
    Sf(o + 648, ez);
    Sf(o + 656, x);
    Sf(o + 660, y);
    Sf(o + 664, z);
    store<u32>(o + 672, w0);
    store<u32>(o + 676, w1);
    store<u32>(o + 680, w2);
    Cw(o + 640, U);
    Cw(o + 644, U + 4);
    Cw(o + 648, U + 8);
    Sf(o + 656, Fmsubs(Lf(o + 628), Lf(U + 8), S(Lf(o + 632) * Lf(U + 4))));
    Sf(o + 660, Fmsubs(Lf(o + 632), Lf(o + 640), S(Lf(o + 624) * Lf(o + 648))));
    Sf(o + 664, Fmsubs(Lf(o + 624), Lf(o + 644), S(Lf(o + 628) * Lf(o + 640))));
    store<u32>(o + 672, s0);
    store<u32>(o + 676, s1);
    store<u32>(o + 680, s2);
    Sf(o + 720, x);
    Sf(o + 724, y);
    Sf(o + 728, z);
    ctx.fpr[1] = y;
}

// Vert* Grid_EmitQuad429260(void*, Vert* out)
// (called by 00429950) Emits a (nj + 1) x (ni + 1) grid of 24-byte
// vertices {+0 pos, +12 colour bytes from 0x01164CA0, +16 uv} over the
// quad held at 0x010AA1B0 (u16 +82 ni, +84 nj; corners A +160, B +112, C
// +128, D +144): row i at s = i / nj runs from A + s (B - A) to D + s (C -
// D), vertex j at t = j / ni along it; uv = (0 + s*0, 0 + s*1) to (+1,
// ..) as the original's fused steps compute them. Counters are u16,
// compared signed. Returns the end pointer (f1 left as the row's u).
DECOMP(00429260, Grid_EmitQuad429260) {
    u32 out = arg<u32>(ctx, 1);
    const u32 K = 0x00429258u;  // f32 0, 1
    const u32 Q = 0x010AA1B0u;
    const f64 k0 = Lf(K), k1 = Lf(K + 4);
    const u32 nj = load<u16>(Q + 84);
    const s32 nrow = static_cast<s32>(nj + 1);
    const u32 A = Q + 160, B = Q + 112, C = Q + 128, D = Q + 144;
    const f64 e0 = S(k0 - k0), e1 = S(k1 - k0), e2 = S(k1 - k1);
    const u32 ni = load<u16>(Q + 82);
    const s32 ncol = static_cast<s32>(ni + 1);
    const f64 ax = Lf(A), ay = Lf(A + 4), az = Lf(A + 8);
    const f64 bx = S(Lf(B) - ax), by = S(Lf(B + 4) - ay), bz = S(Lf(B + 8) - az);
    const f64 dx0 = Lf(D), dy0 = Lf(D + 4), dz0 = Lf(D + 8);
    const f64 cx = S(Lf(C) - dx0), cy = S(Lf(C + 4) - dy0), cz = S(Lf(C + 8) - dz0);
    const f64 fnj = S(static_cast<f64>(nj)), fni = S(static_cast<f64>(ni));
    const u32 col = 0x01164CA0u;
    f64 u1 = ctx.fpr[1];
    for (u32 i = 0; static_cast<s32>(i) < nrow; i = (i + 1) & 0xFFFFu) {
        const f64 s = S(S(static_cast<f64>(i)) / fnj);
        const f64 px = Fmadds(bx, s, ax), py = Fmadds(by, s, ay), pz = Fmadds(bz, s, az);
        u1 = Fmadds(e0, s, k0);
        const f64 v1 = Fmadds(e1, s, k0), u2 = Fmadds(e2, s, k1);
        if (ncol <= 0) continue;
        const f64 qx = Fmadds(cx, s, dx0), qy = Fmadds(cy, s, dy0), qz = Fmadds(cz, s, dz0);
        const f64 du = S(u2 - u1), dv = S(v1 - v1);
        const f64 ex = S(qx - px), ey = S(qy - py), ez = S(qz - pz);
        for (u32 j = 0; static_cast<s32>(j) < ncol; j = (j + 1) & 0xFFFFu) {
            const f64 t = S(S(static_cast<f64>(j)) / fni);
            Sf(out + 16, Fmadds(du, t, u1));
            Sf(out + 20, Fmadds(dv, t, v1));
            Sf(out + 0, Fmadds(ex, t, px));
            Sf(out + 4, Fmadds(ey, t, py));
            Sf(out + 8, Fmadds(ez, t, pz));
            for (u32 k = 0; k < 4; ++k) store<u8>(out + 12 + k, load<u8>(col + k));
            out += 24;
        }
    }
    ctx.fpr[1] = u1;
    ret(ctx, out);
}

namespace {
// The cross product a x g (g a global vec3) as the original's fmuls /
// fmsubs pairs, storing x then y to out as it goes (a is re-read after
// each store, so an aliasing out behaves as in the original). Returns z.
inline f64 CrossStoreXY(u32 out, u32 a, u32 g, f64& cx, f64& cy) {
    cx = Fmsubs(Lf(a + 4), Lf(g + 8), S(Lf(a + 8) * Lf(g + 4)));
    Sf(out, cx);
    cy = Fmsubs(Lf(a + 8), Lf(g), S(Lf(a) * Lf(g + 8)));
    Sf(out + 4, cy);
    return Fmsubs(Lf(a), Lf(g + 4), S(Lf(a + 4) * Lf(g)));
}
}  // namespace

// f32 Vec_AxisAngle284528(const f32 a[3], const f32 b[3], f32 axis[3])
// (called by 00284860) With |a| and |b| both >= 1e-5: axis = unit(a x b)
// (when that is shorter than 1e-5, a x Y (0x01164B60) and then a x X
// (0x01164B50) are tried; the last one is normalised regardless) and
// returns acosf of a.b / (|a| |b|) clamped to [-1, 1] by the fsel chain.
// Otherwise axis = X (and f1 = 0, k[0]). Every partial result is stored as the
// original stores it, the inputs re-read after each store. Constants at
// 0x00284500.
DECOMP(00284528, Vec_AxisAngle284528) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    const u32 K = 0x00284500u;  // f32 0, 1e-5, 1, (pad), f64 1, f32 -1, (pad), f64 -1
    const f64 a1 = Lf(a + 4), a0 = Lf(a), a2 = Lf(a + 8), b1 = Lf(b + 4);
    const f64 la = S(std::sqrt(Fmadds(a2, a2, Fmadds(a0, a0, S(a1 * a1)))));
    const f64 b0 = Lf(b), eps = Lf(K + 4), b2 = Lf(b + 8);
    const f64 lb = S(std::sqrt(Fmadds(b2, b2, Fmadds(b0, b0, S(b1 * b1)))));
    ret(ctx, out);
    const u32 X = 0x01164B50u, Y = 0x01164B60u;
    if (!(la >= eps) || !(lb >= eps)) {
        Cw(out, X);
        Cw(out + 4, X + 4);
        Cw(out + 8, X + 8);
        ctx.fpr[1] = Lf(K);  // the exit's fmr f1, f8 (f8 = k[0])
        return;
    }
    f64 cx = Fmsubs(a1, b2, S(a2 * b1));
    Sf(out, cx);
    f64 cy = Fmsubs(Lf(a + 8), Lf(b), S(Lf(a) * Lf(b + 8)));
    Sf(out + 4, cy);
    f64 cz = Fmsubs(Lf(a), Lf(b + 4), S(Lf(a + 4) * Lf(b)));
    f64 lc = S(std::sqrt(Fmadds(cz, cz, Fmadds(cx, cx, S(cy * cy)))));
    if (!(lc >= eps)) {
        Sf(out + 8, cz);
        cz = CrossStoreXY(out, a, Y, cx, cy);
        const f64 l2 = S(std::sqrt(Fmadds(cz, cz, Fmadds(cx, cx, S(cy * cy)))));
        if (l2 < eps) {
            Sf(out + 8, cz);
            cz = CrossStoreXY(out, a, X, cx, cy);
        }
        lc = S(std::sqrt(Fmadds(cz, cz, Fmadds(cx, cx, S(cy * cy)))));
    }
    const f64 lab = S(la * lb);
    const f64 one = Lf(K + 8);
    const f64 inv = S(one / lc);
    Sf(out + 8, S(cz * inv));
    Sf(out + 4, S(cy * inv));
    Sf(out, S(cx * inv));
    const f64 dot = Fmadds(Lf(a + 8), Lf(b + 8), Fmadds(Lf(a), Lf(b), S(Lf(a + 4) * Lf(b + 4))));
    f64 c = S(dot / lab);
    const f64 d1 = Ld(K + 16), m1 = Lf(K + 24), dm1 = Ld(K + 32);
    c = S(Fsel15(S(one - c), c, d1));
    c = S(Fsel15(S(c - m1), c, dm1));
    c = S(Fsel15(S(c + one), c, dm1));
    c = S(Fsel15(S(c - one), d1, c));
    callf(ctx, m15fn::Acosf, {}, {c});
}

// u32 Chain_UpdateFrames1A8010(Chain* o)
// (called by 001A8678) For each of the u16 o+1890 links i (96-byte frames
// at o+352: +0 right, +16 dir, +32 up, +48 pos, +80 world object), with
// the points (64 bytes apart) at o->1896->116: pos = point i, dir = point
// i - point i+1, right = dir x up, up = right x dir (stored component by
// component, re-read), Matrix_NormalizeInPlace, the frame's four rows
// copied to the world object's +16.. and WorldObj_MarkDirty. Returns the
// count handled (f1 left as the last point's z).
DECOMP(001A8010, Chain_UpdateFrames1A8010) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a0 = load<u32>(load<u32>(o + 1896) + 116);
    u32 px = load<u32>(a0), py = load<u32>(a0 + 4), pz = load<u32>(a0 + 8);
    ctx.fpr[1] = LfsF(pz);
    u32 i = 0;
    ret(ctx, 0);
    if (load<u16>(o + 1890) == 0) return;
    do {
        const u32 m = o + 352 + 96 * i;
        const u32 np = load<u32>(load<u32>(o + 1896) + 116) + 64 * (i + 1);
        const u32 qx = load<u32>(np), qy = load<u32>(np + 4), qz = load<u32>(np + 8);
        const f64 dx = S(LfsF(px) - LfsF(qx)), dy = S(LfsF(py) - LfsF(qy)), dz = S(LfsF(pz) - LfsF(qz));
        store<u32>(m + 48, px);
        store<u32>(m + 52, py);
        store<u32>(m + 56, pz);
        Sf(m + 16, dx);
        Sf(m + 20, dy);
        Sf(m + 24, dz);
        const u32 d = m + 16, u = m + 32;
        Sf(m, Fmsubs(dy, Lf(u + 8), S(dz * Lf(u + 4))));
        Sf(m + 4, Fmsubs(Lf(d + 8), Lf(u), S(Lf(d) * Lf(u + 8))));
        const f64 rz = Fmsubs(Lf(d), Lf(u + 4), S(Lf(d + 4) * Lf(u)));
        Sf(m + 8, rz);
        Sf(u, Fmsubs(Lf(m + 4), Lf(d + 8), S(rz * Lf(d + 4))));
        Sf(u + 4, Fmsubs(Lf(m + 8), Lf(d), S(Lf(m) * Lf(d + 8))));
        Sf(u + 8, Fmsubs(Lf(m), Lf(d + 4), S(Lf(m + 4) * Lf(d))));
        call(ctx, m15fn::Matrix_NormalizeInPlace, {m});
        const u32 w = load<u32>(m + 80);
        for (u32 r = 0; r < 64; r += 16)
            for (u32 k = 0; k < 12; k += 4) Cw(w + 16 + r + k, m + r + k);
        call(ctx, m15fn::WorldObj_MarkDirty, {w});
        ++i;
        px = qx;
        py = qy;
        pz = qz;
        ctx.fpr[1] = LfsF(pz);
        ret(ctx, i);
    } while (i < load<u16>(o + 1890));
}

// void Quat_AngularVel0F8E30(f32 out[3], -, const f32 q0[4], const f32 q1[4], f32 dt)
// (called by 000F90F8, 000FB450) The angular velocity taking q0 to q1 in
// dt: with dt <= 0, or a rotation axis length (sqrt(1 - w^2) of the unit
// relative quaternion) below 1e-4, out = the global vec3 0x01164B40.
// Otherwise r = q1 * q0^-1 (q0 negated first when q0.q1 < 0; the inverse
// is the conjugate over |q0|^2), normalised; out = unit(r.xyz / s) * 2
// acosf(r.w clamped by the fsel chain) / dt. Constants at 0x000F8DF8.
// Leftover r3 / f1 as the original's.
DECOMP(000F8E30, Quat_AngularVel0F8E30) {
    const u32 out = arg<u32>(ctx, 0), q0 = arg<u32>(ctx, 2), q1 = arg<u32>(ctx, 3);
    const f64 dt = ctx.fpr[1];
    const u32 K = 0x000F8DF8u;  // f32 0, 1, f64 1, f32 -1, f64 -1, f32 2, f64 0, f32 1e-4
    const u32 G = 0x01164B40u;
    const f64 k0 = Lf(K);
    auto zero = [&] {
        ret(ctx, 0x01160000u);
        Cw(out, G);
        Cw(out + 4, G + 4);
        Cw(out + 8, G + 8);
        ctx.fpr[1] = Lf(G + 8);
    };
    if (dt < k0 || dt == k0) {
        zero();
        return;
    }
    f64 x = Lf(q0);
    const f64 d = Fmadds(Lf(q0 + 12), Lf(q1 + 12), Fmadds(Lf(q0 + 8), Lf(q1 + 8), Fmadds(x, Lf(q1), S(Lf(q0 + 4) * Lf(q1 + 4)))));
    f64 y = Lf(q0 + 4), z = Lf(q0 + 8), w = Lf(q0 + 12);
    if (d < k0) {
        x = -x;
        y = -y;
        z = -z;
        w = -w;
    }
    const f64 n = Fmadds(w, w, Fmadds(z, z, Fmadds(x, x, S(y * y))));
    Stack frame(ctx, 32);
    const u32 rq = frame.addr(), iq = rq + 16;
    Sf(iq, S(-x / n));
    Sf(iq + 4, S(-y / n));
    Sf(iq + 8, S(-z / n));
    Sf(iq + 12, S(w / n));
    call(ctx, m15fn::Quat_Multiply, {rq, q1, iq});
    const f64 ry = Lf(rq + 4), rx = Lf(rq), rz = Lf(rq + 8), rw = Lf(rq + 12);
    const f64 one = Lf(K + 4);
    const f64 inv = S(one / S(std::sqrt(Fmadds(rw, rw, Fmadds(rz, rz, Fmadds(rx, rx, S(ry * ry)))))));
    const f64 d1 = Ld(K + 8), m1 = Lf(K + 16), dm1 = Ld(K + 24);
    const f64 cw = S(rw * inv);
    f64 c = S(Fsel15(S(one - cw), cw, d1));
    c = S(Fsel15(S(c - m1), c, dm1));
    c = S(Fsel15(S(c + one), c, dm1));
    c = S(Fsel15(S(c - one), d1, c));
    const f64 ac = callf(ctx, m15fn::Acosf, {}, {c});
    const f64 s2 = Fnmsubs(cw, cw, one);
    const f64 s = S(std::sqrt(S(Fsel15(s2, s2, Ld(K + 40)))));
    const f64 ang = S(ac * Lf(K + 32));
    ctx.fpr[1] = ang;
    if (s < Lf(K + 48)) {
        zero();
        return;
    }
    const f64 rate = S(ang / dt);
    f64 ax = S(S(ry * inv) / s);
    f64 bx = S(S(rx * inv) / s);
    f64 cz = S(S(rz * inv) / s);
    const f64 il = S(one / S(std::sqrt(Fmadds(cz, cz, Fmadds(bx, bx, S(ax * ax))))));
    bx = S(il * bx);
    ax = S(il * ax);
    cz = S(il * cz);
    Sf(out, S(rate * bx));
    Sf(out + 4, S(rate * ax));
    const f64 oz = S(rate * cz);
    Sf(out + 8, oz);
    ctx.fpr[1] = oz;
}

// Mat34* Obj_LookAt1D0B40(Mat34* m, Obj* o, const f32 target[3])
// (called by Obj_ApplyPose_001D0E58, 001D3810) m = identity (0x01164BD0)
// with forward (row 2) = unit(target - o+688) (target = o+848 when o->884
// is set; with a squared distance below 1e-5 the old forward o+672 is
// kept), right (row 0) = unit(up x forward), up (row 1) = forward x
// right, each component stored and re-read as in the original; right and
// up are negated when right . (o+640) <= 0. Row 3 = o+688, and the four
// rows are copied back to o+640. Constants at 0x001D0B30. Returns m (f1
// left as the last word copied).
DECOMP(001D0B40, Obj_LookAt1D0B40) {
    const u32 m = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    u32 t = arg<u32>(ctx, 2);
    const u32 K = 0x001D0B30u;  // f32 1e-5, 1, 0
    if (load<u32>(o + 884) != 0) t = o + 848;
    const u32 pos = o + 688;
    const f64 dx = S(Lf(t) - Lf(pos)), dy = S(Lf(t + 4) - Lf(pos + 4)), dz = S(Lf(t + 8) - Lf(pos + 8));
    const f64 l2 = Fmadds(dz, dz, Fmadds(dx, dx, S(dy * dy)));
    f64 fx, fy, fz;
    if (l2 > Lf(K) || l2 == Lf(K)) {
        const f64 inv = S(Lf(K + 4) / S(std::sqrt(l2)));
        fx = S(inv * dx);
        fy = S(inv * dy);
        fz = S(inv * dz);
    } else {
        fx = Lf(o + 672);
        fy = Lf(o + 676);
        fz = Lf(o + 680);
    }
    const u32 I = 0x01164BD0u;
    for (u32 r = 0; r < 64; r += 16)
        for (u32 k = 0; k < 12; k += 4) Cw(m + r + k, I + r + k);
    const u32 u = m + 16, f = m + 32;
    Sf(f, fx);
    Sf(f + 4, fy);
    Sf(f + 8, fz);
    Sf(m, Fmsubs(Lf(u + 4), fz, S(Lf(u + 8) * fy)));
    Sf(m + 4, Fmsubs(Lf(u + 8), Lf(f), S(Lf(u) * Lf(f + 8))));
    const f64 rz = Fmsubs(Lf(u), Lf(f + 4), S(Lf(u + 4) * Lf(f)));
    const f64 rx = Lf(m), ry = Lf(m + 4);
    Sf(m + 8, rz);
    const f64 one = Lf(K + 4);
    const f64 rz2 = Lf(m + 8);
    const f64 il = S(one / S(std::sqrt(Fmadds(rz2, rz2, Fmadds(rx, rx, S(ry * ry))))));
    Sf(m + 4, S(ry * il));
    Sf(m + 8, S(rz2 * il));
    Sf(m, S(rx * il));
    Sf(u, Fmsubs(Lf(f + 4), Lf(m + 8), S(Lf(f + 8) * Lf(m + 4))));
    Sf(u + 4, Fmsubs(Lf(f + 8), Lf(m), S(Lf(f) * Lf(m + 8))));
    Sf(u + 8, Fmsubs(Lf(f), Lf(m + 4), S(Lf(f + 4) * Lf(m))));
    const f64 dt = Fmadds(Lf(m + 8), Lf(o + 648), Fmadds(Lf(m), Lf(o + 640), S(Lf(m + 4) * Lf(o + 644))));
    if (dt < Lf(K + 8) || dt == Lf(K + 8)) {
        for (u32 b : {m, u}) {  // lfs x3, fneg, stfs x3: sign flips
            const u32 a0 = load<u32>(b), a1 = load<u32>(b + 4), a2 = load<u32>(b + 8);
            store<u32>(b, a0 ^ 0x80000000u);
            store<u32>(b + 4, a1 ^ 0x80000000u);
            store<u32>(b + 8, a2 ^ 0x80000000u);
        }
    }
    Cw(m + 48, pos);
    Cw(m + 52, pos + 4);
    Cw(m + 56, pos + 8);
    for (u32 r = 0; r < 64; r += 16)
        for (u32 k = 0; k < 12; k += 4) Cw(o + 640 + r + k, m + r + k);
    ctx.fpr[1] = Lf(m + 56);
    ret(ctx, m);
}
