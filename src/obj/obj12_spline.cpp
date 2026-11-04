// World objects, part 12: the spline point array's arc-length
// parameterisation (SplineArray_Finish) and the Bezier helpers behind it.
#include "obj12.h"

using namespace sly4;
using namespace sly4::obj9;
using namespace sly4::obj10;
using namespace sly4::obj12;

namespace {

constexpr u32 kBezier_Split = 0x0055F8D0;
constexpr u32 kBezier_Length = 0x0055FA30;

constexpr u32 kBsHalf = 0x0055F8C8;            // f32 0.5
constexpr u32 kBlConsts = 0x0055FA20;          // f32 {0, 0.5, 1e-5}
constexpr u32 kBlZero = 0, kBlHalf = 4, kBlMinTol = 8;
constexpr u32 kSfConsts = 0x0055FB80;          // f32 {0, 1e-5}
constexpr u32 kSfZero = 0, kSfTol = 4;
constexpr u32 kSfCount = 0x7FFF;               // SplineArray +0: the point count SplineArray_Finish uses
constexpr u32 kSaLength = 8;                   // f32: the total length
constexpr u32 kOneBits = 0x3F800000;           // 1.0f

// A control polygon: f32[4][4] (x / y / z lanes used) at c.
inline f32 Dist3(u32 p, u32 n) {
    const f32 dy = Fsubs(Lf(n + 4), Lf(p + 4)), dx = Fsubs(Lf(n), Lf(p)), dz = Fsubs(Lf(n + 8), Lf(p + 8));
    return Fsqrts(Fmadds(dz, dz, Fmadds(dx, dx, Fmuls(dy, dy))));
}

inline void Copy32(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }

}  // namespace

// void Bezier_Split(const f32 c[4][4], f32 left[4][4], f32 right[4][4])
// De Casteljau at t = 0.5 on the x / y / z lanes, except that both halves
// share (c1 + c2) / 2 as their joining point instead of the curve's
// midpoint (a quirk of the original, kept). The stores and reloads are in
// the original's order (left[3] is written early and read back for
// right[0]).
DECOMP(0055F8D0, Bezier_Split) {
    const u32 c = arg<u32>(ctx, 0), l = arg<u32>(ctx, 1), r = arg<u32>(ctx, 2);
    Copy32(l, c);
    Copy32(l + 4, c + 4);
    Copy32(l + 8, c + 8);
    const f32 h = Lf(kBsHalf);
    f32 ax = Fadds(Lf(c), Lf(c + 16)), ay = Fadds(Lf(c + 4), Lf(c + 20)), az = Fadds(Lf(c + 8), Lf(c + 24));
    ax = Fmuls(ax, h), ay = Fmuls(ay, h), az = Fmuls(az, h);
    store<f32>(l + 16, ax);
    store<f32>(l + 20, ay);
    store<f32>(l + 24, az);
    f32 bx = Fadds(Lf(c + 16), Lf(c + 32)), by = Fadds(Lf(c + 20), Lf(c + 36)), bz = Fadds(Lf(c + 24), Lf(c + 40));
    bx = Fmuls(bx, h), by = Fmuls(by, h), bz = Fmuls(bz, h);
    const f32 mx = Fadds(ax, bx), my = Fadds(ay, by), mz = Fadds(az, bz);
    store<f32>(l + 52, by);
    store<f32>(l + 56, bz);
    store<f32>(l + 48, bx);
    store<f32>(l + 32, Fmuls(mx, h));
    store<f32>(l + 36, Fmuls(my, h));
    store<f32>(l + 40, Fmuls(mz, h));
    store<f32>(r, bx);
    const u32 jy = load<u32>(l + 52);
    store<u32>(r + 4, jy);
    const u32 jz = load<u32>(l + 56);
    store<u32>(r + 8, jz);
    f32 cx = Fadds(Lf(c + 32), Lf(c + 48)), cy = Fadds(Lf(c + 36), Lf(c + 52)), cz = Fadds(Lf(c + 40), Lf(c + 56));
    cx = Fmuls(cx, h), cy = Fmuls(cy, h), cz = Fmuls(cz, h);
    const f32 nx = Fadds(bx, cx), ny = Fadds(std::bit_cast<f32>(jy), cy), nz = Fadds(std::bit_cast<f32>(jz), cz);
    store<f32>(r + 32, cx);
    store<f32>(r + 36, cy);
    store<f32>(r + 40, cz);
    store<f32>(r + 16, Fmuls(nx, h));
    store<f32>(r + 20, Fmuls(ny, h));
    const u32 ex = load<u32>(c + 48);
    store<f32>(r + 24, Fmuls(nz, h));
    const u32 ey = load<u32>(c + 52);
    store<u32>(r + 48, ex);
    const u32 ez = load<u32>(c + 56);
    store<u32>(r + 52, ey);
    store<u32>(r + 56, ez);
}

// f32 Bezier_Length(const f32 c[4][4], f32 tolerance)
// The control polygon's length when its squared difference from the
// chord is within the tolerance; otherwise the halves' lengths (Bezier_Split
// into sp+112 / sp+176) with the tolerance halved, down to 1e-5.
DECOMP(0055FA30, Bezier_Length) {
    const u32 c = arg<u32>(ctx, 0);
    const f64 tol0 = farg(ctx, 0);
    f32 poly = Lf(kBlConsts + kBlZero);
    for (u32 k = 0; k < 3; ++k) poly = Fadds(poly, Dist3(c + 16 * k, c + 16 * k + 16));
    const f32 chord = Dist3(c, c + 48);
    f32 e = Fsubs(poly, chord);
    e = Fmuls(e, e);
    if (!(tol0 < e)) return ret(ctx, static_cast<f64>(poly));
    Stack frame(ctx, 272 - 112);
    const u32 l = frame.addr(), r = l + 64;  // sp+112 / sp+176
    call(ctx, kBezier_Split, {c, l, r});
    f32 tol = Fmuls(tol0, Lf(kBlConsts + kBlHalf));
    const f32 lo = Lf(kBlConsts + kBlMinTol);
    if (tol < lo) tol = lo;
    const f32 a = static_cast<f32>(callf(ctx, kBezier_Length, {l}, {tol}));
    const f32 b = static_cast<f32>(callf(ctx, kBezier_Length, {r}, {tol}));
    ret(ctx, static_cast<f64>(Fadds(a, b)));
}

// void SplineArray_Finish(SplineArray* sa)
// Arc-length parameterisation: each point's +48 gets the length so far
// (Bezier_Length of the segment to the next point, tolerance 1e-5), then
// all are divided by the total (or set to i / (n - 1) when it is 0) and
// the last is set to 1. +8 = the total, +12 (the last segment found) = 0.
// The count (+0 bits 0-14) is re-read on every step. r3 = 0.
DECOMP(0055FB88, SplineArray_Finish) {
    const u32 sa = arg<u32>(ctx, 0);
    const f32 zero = Lf(kSfConsts + kSfZero);
    f32 total = zero;
    u32 n = load<u32>(sa + kSaBits) & kSfCount;
    if (static_cast<s32>(n) > 1) {
        u32 i = 0;
        if (n - 1 != 0) {
            const f32 tol = Lf(kSfConsts + kSfTol);
            Stack frame(ctx, 256 - 112);
            const u32 cp = frame.addr();  // sp+112: the segment's control polygon
            do {
                const u32 p = load<u32>(sa + kSaPoints) + i * kSpSize, q = p + kSpSize;
                store<f32>(p + kSpParam, total);
                for (u32 k = 0; k < 3; ++k) Copy32(cp + 4 * k, p + 4 * k);
                for (u32 k = 0; k < 3; ++k) store<f32>(cp + 16 + 4 * k, Fadds(Lf(p + 4 * k), Lf(p + kSpOut + 4 * k)));
                for (u32 k = 0; k < 3; ++k) Copy32(cp + 48 + 4 * k, q + 4 * k);
                for (u32 k = 0; k < 3; ++k) store<f32>(cp + 32 + 4 * k, Fadds(Lf(q + 4 * k), Lf(q + kSpIn + 4 * k)));
                total = Fadds(total, callf(ctx, kBezier_Length, {cp}, {tol}));
                ++i;
                n = load<u32>(sa + kSaBits) & kSfCount;
            } while (i < n - 1);
        }
        const u32 pts = load<u32>(sa + kSaPoints);
        store<f32>(pts + (i << 6) + kSpParam, total);
        u32 bits = load<u32>(sa + kSaBits);
        if ((bits & kSfCount) != 0) {
            u32 j = 0;
            do {
                f32 num, den;
                if (total == zero) {
                    num = static_cast<f32>(static_cast<f64>(j));
                    den = static_cast<f32>(static_cast<f64>(static_cast<s32>((bits & kSfCount) - 1)));
                } else {
                    num = Lf(pts + j * kSpSize + kSpParam);
                    den = total;
                }
                store<f32>(pts + j * kSpSize + kSpParam, Fdivs(num, den));
                ++j;
                bits = load<u32>(sa + kSaBits);
            } while (j < (bits & kSfCount));
        }
        store<u32>(pts + ((bits << 6) & 0x001FFFC0u) - 16, kOneBits);  // the last point's parameter
    }
    store<f32>(sa + kSaLength, total);
    store<u32>(sa + kSaLastSeg, 0);
    ret(ctx, 0u);
}

namespace {

constexpr u32 kSbConsts = 0x0055FDF0;          // f32 {FLT_MAX, -FLT_MAX, 0}
constexpr u32 kSbMax = 0, kSbMin = 4, kSbZero = 8;

inline f32 Fsel(f32 a, f32 c, f32 b) { return a >= 0.0f ? c : b; }

}  // namespace

// void SplineArray_Bounds(const SplineArray* sa, f32 box[8], f32 sphere[4])
// The points' box (min at +0, max at +16, x / y / z) and a sphere around
// their average (radius to the farthest point). One point: the box grown
// to include it (fsel, so a NaN keeps the box's value) and the point with
// radius 0. No points: nothing. The count is re-read before the average
// (and the points pointer on every step of the radius loop).
DECOMP(0055FE00, SplineArray_Bounds) {
    const u32 sa = arg<u32>(ctx, 0), box = arg<u32>(ctx, 1), sph = arg<u32>(ctx, 2);
    const u32 n = load<u32>(sa + kSaBits) & kSfCount;
    if (static_cast<s32>(n) <= 1) {
        if (n == 0) return;
        const u32 p = load<u32>(sa + kSaPoints);
        // The mins (and each max) are loaded before the store before them.
        const f32 lo0 = Lf(box), v0 = Lf(p), lo1 = Lf(box + 4), lo2 = Lf(box + 8);
        const f32 hi0 = Lf(box + 16);
        store<f32>(box, Fsel(Fsubs(lo0, v0), v0, lo0));
        const f32 v1 = Lf(p + 4);
        const f32 hi1 = Lf(box + 20);
        store<f32>(box + 4, Fsel(Fsubs(lo1, v1), v1, lo1));
        const f32 v2 = Lf(p + 8);
        const f32 hi2 = Lf(box + 24);
        store<f32>(box + 8, Fsel(Fsubs(lo2, v2), v2, lo2));
        const f32 hi[3] = {hi0, hi1, hi2};
        for (u32 k = 0; k < 3; ++k) {
            const f32 v = Lf(p + 4 * k);
            store<f32>(box + 16 + 4 * k, Fsel(Fsubs(hi[k], v), hi[k], v));
        }
        const u32 x = load<u32>(p), y = load<u32>(p + 4), z = load<u32>(p + 8);
        store<u32>(sph, x);
        store<u32>(sph + 4, y);
        store<u32>(sph + 8, z);
        store<u32>(sph + 12, 0);
        ctx.gpr[3] = p;  // as the original leaves it
        return;
    }
    const f32 big = Lf(kSbConsts + kSbMax), small = Lf(kSbConsts + kSbMin);
    f32 lo[3] = {big, big, big}, hi[3] = {small, small, small};
    f32 sum[3] = {Lf(kOrigin), Lf(kOrigin + 4), Lf(kOrigin + 8)};
    u32 p = load<u32>(sa + kSaPoints);
    for (u32 k = n; k != 0; --k, p += kSpSize) {
        const f32 v[3] = {Lf(p), Lf(p + 4), Lf(p + 8)};
        for (u32 j = 0; j < 3; ++j) {
            const f32 dl = Fsubs(lo[j], v[j]), dh = Fsubs(hi[j], v[j]);
            sum[j] = Fadds(sum[j], v[j]);
            lo[j] = Fsel(dl, v[j], lo[j]);
            hi[j] = Fsel(dh, hi[j], v[j]);
        }
    }
    for (u32 j = 0; j < 3; ++j) store<f32>(box + 4 * j, lo[j]);
    for (u32 j = 0; j < 3; ++j) store<f32>(box + 16 + 4 * j, hi[j]);
    const u32 n2 = load<u32>(sa + kSaBits) & kSfCount;
    const f32 fn = static_cast<f32>(static_cast<f64>(n2));
    const f32 cx = Fdivs(sum[0], fn), cy = Fdivs(sum[1], fn), cz = Fdivs(sum[2], fn);
    f32 r2 = Lf(kSbConsts + kSbZero);
    for (u32 j = 0; j < n2; ++j) {
        const u32 q = load<u32>(sa + kSaPoints) + j * kSpSize;
        const f32 dy = Fsubs(Lf(q + 4), cy), dx = Fsubs(Lf(q), cx), dz = Fsubs(Lf(q + 8), cz);
        const f32 d2 = Fmadds(dz, dz, Fmadds(dx, dx, Fmuls(dy, dy)));
        if (d2 > r2) r2 = d2;
    }
    store<f32>(sph, cx);
    store<f32>(sph + 4, cy);
    store<f32>(sph + 8, cz);
    store<f32>(sph + 12, Fsqrts(r2));
}
