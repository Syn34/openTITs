// World objects, part 10: Matrix_Normalize, SplineSeg_Nearest (the Bezier
// segment search Spline_NearestSeg runs) and SplineMap_Find (the spline
// definitions' map lookup).
#include "obj10.h"

using namespace sly4;
using namespace sly4::obj9;
using namespace sly4::obj10;
using obj8::Vec4;
using obj8::VLoad;
using obj8::VStore;

namespace {

constexpr u32 kNormConsts = 0x0056D240;  // two vperm masks, then f32 0.5 (+32) and 1.0 (+36)
constexpr u32 kNcHalf = 32, kNcOne = 36;

constexpr u32 kSegConsts = 0x0055ED4C;   // f32 {FLT_MAX, 0, 1, 3, 1e-5}
constexpr u32 kScMax = 0, kScZero = 4, kScOne = 8, kScThree = 12, kScEps = 16;

constexpr u32 kSplineMapCompare = 0x00F2E1F0;  // the map's default (unsigned key) comparator OPD

}  // namespace

// void Matrix_Normalize(f32 dst[4][4], const f32 src[4][4])
// dst = src with rows 0-2 scaled to unit length (all four lanes of each
// row are scaled; row 3 is copied). The inverse length is the vrsqrtefp
// estimate refined by one Newton step (a NaN step keeps the estimate).
// As the VMX code: squares and products are fused with a +0 addend (so a
// -0 lane becomes +0); row 0's length sums (x + y) + z, rows 1 and 2
// x + (y + z); the estimate is 1 / sqrt in single precision, as the
// runtime's vrsqrtefp computes it.
DECOMP(0056D268, Matrix_Normalize) {
    const u32 dst = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);
    Vec4 rows[3];
    for (u32 r = 0; r < 3; ++r) rows[r] = VLoad(src + r * 16);
    const f32 half = Lf(kNormConsts + kNcHalf), one = Lf(kNormConsts + kNcOne);
    f32 k[3];
    for (u32 r = 0; r < 3; ++r) {
        f32 sq[3];
        for (u32 j = 0; j < 3; ++j) sq[j] = std::fmaf(rows[r].f(j), rows[r].f(j), 0.0f);
        const f32 len2 = r == 0 ? (sq[0] + sq[1]) + sq[2] : sq[0] + (sq[1] + sq[2]);
        const f32 e = 1.0f / std::sqrt(len2);
        const f32 es = std::fmaf(e, len2, 0.0f);
        const f32 eh = std::fmaf(e, half, 0.0f);
        const f32 step = -std::fmaf(e, es, -one);  // 1 - e * e * len2
        const f32 n = std::fmaf(step, eh, e);
        k[r] = n == n ? n : e;
    }
    const Vec4 w = VLoad(src + 48);
    for (u32 r = 0; r < 3; ++r) {
        Vec4 o;
        for (u32 j = 0; j < 4; ++j) o.w[j] = std::bit_cast<u32>(std::fmaf(rows[r].f(j), k[r], 0.0f));
        VStore(dst + r * 16, o);
    }
    VStore(dst + 48, w);
}

// f32 SplineSeg_Nearest(const f32 ctrl[4][4], const f32 p[3], f32* dist, u32 steps)
// The cubic Bezier ctrl[0..3] cut into `steps` chords (the last one ends
// exactly at ctrl[3]); p is projected onto each chord (clamped to its
// ends; a chord shorter than 1e-5 is not normalised) and the closest
// projection wins. Returns its curve parameter (the chord's start plus
// its share of 1 / steps) and stores its distance in *dist (FLT_MAX and
// t = 0 for zero steps).
DECOMP(0055ED60, SplineSeg_Nearest) {
    const u32 c = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), dist = arg<u32>(ctx, 2);
    const u32 steps = arg<u32>(ctx, 3);
    const f32 zero = Lf(kSegConsts + kScZero), one = Lf(kSegConsts + kScOne);
    f32 best = Lf(kSegConsts + kScMax), best_t = zero;
    const f32 dt = Fdivs(one, static_cast<f32>(static_cast<f64>(steps)));
    if (steps != 0) {
        const f32 three = Lf(kSegConsts + kScThree), eps = Lf(kSegConsts + kScEps);
        const f32 px = Lf(p), py = Lf(p + 4), pz = Lf(p + 8);
        const f32 c0[3] = {Lf(c), Lf(c + 4), Lf(c + 8)};
        f32 prev[3] = {c0[0], c0[1], c0[2]};
        f32 t = zero;
        for (u32 i = 0; i < steps; ++i) {
            f32 cur[3];
            if (i < steps - 1) {
                // Bernstein weights at u: (1-u)^3, 3u(1-u)^2, 3u^2(1-u), u^3.
                const f32 u = Fadds(t, dt);
                const f32 u3 = Fmuls(u, three);
                const f32 v = Fsubs(one, u);
                const f32 uu = Fmuls(u, u);
                const f32 vv = Fmuls(v, v);
                const f32 b3 = Fmuls(uu, u);
                const f32 b2 = Fmuls(Fmuls(u, u3), v);
                const f32 b1 = Fmuls(vv, u3);
                const f32 b0 = Fmuls(v, vv);
                for (u32 k = 0; k < 3; ++k) {
                    const f32 a = Fmadds(Lf(c + 48 + 4 * k), b3, Fmuls(Lf(c + 32 + 4 * k), b2));
                    cur[k] = Fmadds(c0[k], b0, Fmadds(Lf(c + 16 + 4 * k), b1, a));
                }
            } else {
                for (u32 k = 0; k < 3; ++k) cur[k] = Lf(c + 48 + 4 * k);
            }
            f32 dx = Fsubs(cur[0], prev[0]), dy = Fsubs(cur[1], prev[1]), dz = Fsubs(cur[2], prev[2]);
            const f32 len = Fsqrts(Fmadds(dz, dz, Fmadds(dx, dx, Fmuls(dy, dy))));
            if (len >= eps) {
                const f32 inv = Fdivs(one, len);
                dx = Fmuls(dx, inv);
                dy = Fmuls(dy, inv);
                dz = Fmuls(dz, inv);
            }
            f32 ex = Fsubs(px, cur[0]), ey = Fsubs(py, cur[1]), ez = Fsubs(pz, cur[2]);
            f32 tt;
            if (!(Fmadds(dz, ez, Fmadds(dx, ex, Fmuls(dy, ey))) < zero)) {
                tt = Fadds(t, dt);  // beyond the chord's end
            } else {
                ex = Fsubs(px, prev[0]);
                ey = Fsubs(py, prev[1]);
                ez = Fsubs(pz, prev[2]);
                const f32 d = Fmadds(dz, ez, Fmadds(dx, ex, Fmuls(dy, ey)));
                if (d > zero) {
                    const f32 off = Fdivs(Fmuls(dt, d), len);
                    ex = Fsubs(px, Fmadds(dx, d, prev[0]));
                    ey = Fsubs(py, Fmadds(dy, d, prev[1]));
                    ez = Fsubs(pz, Fmadds(dz, d, prev[2]));
                    tt = Fadds(t, off);
                } else {
                    tt = t;  // before the chord's start
                }
            }
            const f32 dd = Fsqrts(Fmadds(ez, ez, Fmadds(ex, ex, Fmuls(ey, ey))));
            if (dd < best) {
                best = dd;
                best_t = tt;
            }
            for (u32 k = 0; k < 3; ++k) prev[k] = cur[k];
            t = Fadds(t, dt);
        }
    }
    store<f32>(dist, best);
    ret(ctx, best_t);
}

// ApeSplineDef** SplineMap_Find(Map* m, const u32* name_hash)
// The spline definitions' map (*(*0x00F8F158) +16): the value slot for
// *key, sorting the entries first if they changed, or the default slot.
// The same template as the class maps' lookups (the comparator re-read
// from the map on every probe).
DECOMP(007335C4, SplineMap_Find) {
    using namespace sly4::obj;
    const u32 m = arg<u32>(ctx, 0), keyp = arg<u32>(ctx, 1);
    const u16 flags = load<u16>(m + kMapFlags);
    if ((flags & 1) != 0) {
        store<u16>(m + kMapFlags, static_cast<u16>(flags & 0xFFFE));
        const u32 n = load<u16>(m + kMapCount);
        if (static_cast<s32>(n) > 0)
            call(ctx, ofn::qsort, {load<u32>(m + kMapEntries), n, kMapEntrySize, load<u32>(m + kMapCompare)});
    }
    s32 hi = static_cast<s32>(load<u16>(m + kMapCount)) - 1, lo = 0;
    u32 entries = load<u32>(m + kMapEntries);
    s32 found = -1;
    if (load<u32>(m + kMapCompare) == kSplineMapCompare) {
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 key = load<u32>(keyp);
            const u32 k = load<u32>(entries + static_cast<u32>(mid * 8));
            if (k > key) {
                hi = mid - 1;
            } else if (k < key) {
                lo = mid + 1;
            } else {
                found = mid;
                break;
            }
        }
    } else {
        Stack frame(ctx, 160 - 112);  // + call()'s linkage area = the original's 160-byte frame
        const u32 key = frame.addr();  // sp+112
        store<u32>(key, load<u32>(keyp));
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 cmp = load<u32>(m + kMapCompare);
            const s32 r = static_cast<s32>(
                call_ptr(ctx, cmp, {entries + static_cast<u32>(mid * 8), key, cmp, load<u32>(cmp)}));  // r5 / r6 = OPD and entry
            if (r > 0) {
                hi = mid - 1;
            } else if (r < 0) {
                lo = mid + 1;
            } else {
                entries = load<u32>(m + kMapEntries);
                found = mid;
                break;
            }
        }
    }
    ret(ctx, found < 0 ? m + kMapDefault : entries + static_cast<u32>(found * 8) + 4);
}
