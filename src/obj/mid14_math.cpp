// Game code, medium sweep part 14: geometry.
#include "mid14.h"

using namespace sly4;
using namespace sly4::mid14;

// f32 Seg_DistSq4A44C8(const f32 p0[3], const f32 p1[3], const f32 q0[3], const f32 q1[3])
// (called by 001F4DC0, 001F4F38, 004A4704) Squared distance between the
// segments p0-p1 and q0-q1 (the clamped closest-point solve: d1 = p1 - p0,
// d2 = q1 - q0, r = p0 - q0, a = d1.d1, b = d1.d2, e = d2.d2, c = d1.r,
// f = d2.r; near-parallel below 1e-5 (0x004A44B0) uses s = 0, t = f / e).
// The fused single-precision steps follow the original exactly. Returns f1.
DECOMP(004A44C8, Seg_DistSq4A44C8) {
    const u32 p0 = arg<u32>(ctx, 0), p1 = arg<u32>(ctx, 1), q0 = arg<u32>(ctx, 2), q1 = arg<u32>(ctx, 3);
    const f64 d1y = S(Lf(p1 + 4) - Lf(p0 + 4)), d2y = S(Lf(q1 + 4) - Lf(q0 + 4));
    const f64 d1x = S(Lf(p1) - Lf(p0)), d2x = S(Lf(q1) - Lf(q0));
    const f64 d1z = S(Lf(p1 + 8) - Lf(p0 + 8)), d2z = S(Lf(q1 + 8) - Lf(q0 + 8));
    const f64 ry = S(Lf(p0 + 4) - Lf(q0 + 4)), rz = S(Lf(p0 + 8) - Lf(q0 + 8)), rx = S(Lf(p0) - Lf(q0));
    const u32 kk = 0x004A44B0;  // f32 1e-5, f32 0, f32 1, (pad), f64 0
    const f64 eps = Lf(kk), zero = Lf(kk + 4), one = Lf(kk + 8);
    const f64 dzero = std::bit_cast<f64>(load<u64>(kk + 16));
    const f64 a = Fmadds(d1z, d1z, Fmadds(d1x, d1x, S(d1y * d1y)));
    const f64 b = Fmadds(d1z, d2z, Fmadds(d1x, d2x, S(d1y * d2y)));
    const f64 e = Fmadds(d2z, d2z, Fmadds(d2x, d2x, S(d2y * d2y)));
    const f64 cp = Fmadds(d1x, rx, S(d1y * ry));  // c without its z term
    const f64 c = Fmadds(d1z, rz, cp);
    const f64 f = Fmadds(d2z, rz, Fmadds(d2x, rx, S(d2y * ry)));
    const f64 den = Fmsubs(a, e, S(b * b));
    f64 sN, sD, tN, tD;
    if (den < eps) {
        sN = zero;
        sD = one;
        tN = f;
        tD = e;
    } else {
        sN = Fmsubs(b, f, S(e * c));
        tN = Fmsubs(a, f, S(b * c));
        sD = den;
        tD = den;
        if (sN < dzero) {
            sN = zero;
            tN = f;
            tD = e;
        } else if (den < sN) {
            sN = den;
            tN = S(b + f);
            tD = e;
        }
    }
    if (tN < dzero) {
        const f64 nc = Fnmadds(d1z, rz, cp);  // -c
        tN = zero;
        if (nc < dzero) sN = zero;
        else if (nc > a) sN = sD;
        else {
            sN = nc;
            sD = a;
        }
    } else if (tD < tN) {
        const f64 bc = S(b - c);
        tN = tD;
        if (bc < dzero) sN = zero;
        else if (bc > a) sN = sD;
        else {
            sN = bc;
            sD = a;
        }
    }
    const f64 sc = std::fabs(sN) < eps ? zero : S(sN / sD);
    const f64 tc = std::fabs(tN) < eps ? zero : S(tN / tD);
    const f64 vy = Fnmsubs(tc, d2y, Fmadds(sc, d1y, ry));
    const f64 vx = Fnmsubs(tc, d2x, Fmadds(sc, d1x, rx));
    const f64 vz = Fnmsubs(tc, d2z, Fmadds(sc, d1z, rz));
    ctx.fpr[1] = Fmadds(vz, vz, Fmadds(vx, vx, S(vy * vy)));
    ret(ctx, kk);  // leftover r3: the constants' address
}

namespace {
// |a - b| for two f32[3] points, as the original's fsubs / fmuls / fmadds /
// fsqrts sequence (y first).
inline f64 Dist3(u32 a, u32 b) {
    const f64 dy = S(Lf(a + 4) - Lf(b + 4)), dx = S(Lf(a) - Lf(b)), dz = S(Lf(a + 8) - Lf(b + 8));
    return S(std::sqrt(Fmadds(dz, dz, Fmadds(dx, dx, S(dy * dy)))));
}
}  // namespace

// u32 Trail_AddPoint17FCE8(Vector* v, const Obj* o)
// (called by 0018A570) Trail of 16-byte points {+0 data, +4 u16 capacity,
// +8 u16 count, +10 high-water}: +52 = 0, the last point = o+144 (vec3).
// When it is farther than k (f64 0x0017FCD8) from the one before, o+144
// is appended again (grown to twice the capacity when full). Then, with
// more than two points, every inner point i closer than or as close as
// f32 0x0017FCE0 to the point before it is removed (the rest moved down,
// count - 1; a count wrapping below 0 copies one point from data +
// 0xFFFF0, the original's quirk). Returns the last count value it held.
DECOMP(0017FCE8, Trail_AddPoint17FCE8) {
    const u32 v = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1) + 144;
    const u32 px = load<u32>(p), py = load<u32>(p + 4), pz = load<u32>(p + 8);
    const u32 n = load<u16>(v + 8);
    const u32 last = load<u32>(v) + ((n - 1) << 4);
    store<u32>(v + 52, 0);
    store<u32>(last, px);
    store<u32>(last + 4, py);
    store<u32>(last + 8, pz);
    u32 c = load<u16>(v + 8);
    u32 data = load<u32>(v);
    const f64 d = Dist3(data + ((c - 1) << 4), data + ((c - 2) << 4));
    ctx.fpr[1] = d;
    if (d > std::bit_cast<f64>(load<u64>(0x0017FCD8u))) {
        const u32 cap = load<u16>(v + 4);
        if (c == cap) {
            call(ctx, m1fn::PtrVector_Reserve, {v, (cap << 1) & 0x0001FFFEu});
            c = load<u16>(v + 8);
            data = load<u32>(v);
        }
        const u32 q = data + ((c << 4) & 0x000FFFF0u);
        store<u32>(q, px);
        store<u32>(q + 4, py);
        store<u32>(q + 8, pz);
        const u32 c1 = load<u16>(v + 8) + 1;
        const u32 hw = load<u16>(v + 10);
        store<u16>(v + 8, static_cast<u16>(c1));
        store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c1 & 0xFFFFu) ? hw : c1));
        c = c1;
    }
    u32 r3 = c, r7 = c & 0xFFFFu;
    if (r7 > 2) {
        const f64 kmin = Lf(0x0017FCE0u);
        ctx.fpr[1] = kmin;
        u32 i = 1, off = 16;
        while (i < r7 - 1) {
            const u32 dt = load<u32>(v);
            const f64 dd = Dist3(dt + off, dt + off - 16);
            if (!(dd < kmin || dd == kmin)) {
                off += 16;
                ++i;
                continue;
            }
            u32 j = i, o2 = off, r5;
            for (;;) {
                r5 = (r3 & 0xFFFFu) - 1;
                if (!(j < r5)) break;
                const u32 d0 = load<u32>(v);
                ++j;
                const u32 dst = d0 + o2;
                o2 += 16;
                const u32 src = d0 + o2;
                Cw(dst, src);
                Cw(dst + 4, src + 4);
                Cw(dst + 8, src + 8);
                r3 = load<u16>(v + 8);
            }
            r3 = r5;
            store<u16>(v + 8, static_cast<u16>(r5));
            r7 = r3 & 0xFFFFu;
            if (r3 != r7) {  // the count wrapped below 0
                const u32 d0 = load<u32>(v);
                const u32 dst = d0 + (r5 << 4), src = d0 + ((r5 << 4) & 0x000FFFF0u);
                Cw(dst, src);
                Cw(dst + 4, src + 4);
                Cw(dst + 8, src + 8);
                r7 = load<u16>(v + 8);
                r3 = r7;
            }
        }
    }
    ret(ctx, r3);
}
