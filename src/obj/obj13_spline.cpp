// World objects, part 13: the spline array's evaluations from a segment
// hint: the tangent (Spline_TangentEval) and the point with its tangent
// (Spline_PointTangent), both searching from the hinted segment.
#include "obj13.h"

using namespace sly4;
using namespace sly4::obj9;
using namespace sly4::obj13;

namespace {

constexpr u32 Bezier_Tangent = 0x005602F8;      // (f32 out[4], const f32 ctrl[4][4]; f1 u) (name?)
constexpr u32 kTeZero = 0x00560428;             // f32 0: u when no segment contains t
constexpr u32 kPtConsts = 0x005606B8;           // f32 {0, 3, 1}
constexpr u32 kPtZero = 0, kPtThree = 4, kPtOne = 8;
constexpr u32 kSaIndexMask = 0x7FFFu;           // obj9.h kSaAllocMask: the count these use
constexpr u32 kNoSegment = 0xFFFFFFFFu;
constexpr u32 kFrame = 208;                     // Spline_TangentEval's frame
constexpr u32 kFramePt = 240;                   // Spline_PointTangent's
constexpr u32 kTanOut = 112, kCtrl = 128;       // sp+112 the result, sp+128 four 16-byte control points

inline f32 FracSp(f64 x) { return Fsubs(x, static_cast<f32>(rt::fcfid(rt::fctid(x, true)))); }

inline void Copy3(u32 dst, u32 src) {
    for (u32 k = 0; k < 12; k += 4) store<u32>(dst + k, load<u32>(src + k));
}

struct Segment {
    u32 i;
    f32 u;
};

// The segment holding t, searched from `seg` forwards (or backwards)
// around the `last` segments, the points re-read from +4 every step. When
// found, +12 = its index and u = (t - its param) / (the next param - its
// param); otherwise the index is ~0 and u stays u0. Quirk kept: a forward
// search from a hint at or past `last` never comes back to it, so it
// loops forever when no segment holds t.
Segment FindSegment(u32 d, f64 t, u32 seg, bool back, u32 last, f32 u0) {
    u32 i = seg;
    do {
        const u32 pts = load<u32>(d + kSaPoints);
        const f32 a = Lf(pts + (i << 6) + kSpParam);
        if (t >= a && t <= Lf(pts + (i << 6) + kSpSize + kSpParam)) {
            store<u32>(d + kSaLastSeg, i);
            const f32 b = Lf(pts + ((i + 1) << 6) + kSpParam);
            return {i, Fdivs(Fsubs(t, a), Fsubs(b, a))};
        }
        if (!back) {
            ++i;
            if (!(i < last)) i = 0;
        } else {
            const u32 old = i--;
            if (old == 0) i = last;
        }
    } while (i != seg);
    return {kNoSegment, u0};
}

// The segment's Bezier control points at sp+128 (P0 the point, P1 + its
// outgoing handle, P2 the next point + its incoming handle, P3 the next
// point; the w lanes left as they were), each sum also stored at sp+112
// as the original does. With no segment the points are those at -1 and 0.
void Control(u32 sp, u32 pts, u32 i) {
    const u32 p = pts + (i << 6), q = pts + ((i + 1) << 6);
    const u32 c = sp + kCtrl;
    Copy3(c, p + kSpPos);
    for (u32 k = 0; k < 12; k += 4) {
        const f32 v = Fadds(Lf(p + kSpPos + k), Lf(p + kSpOut + k));
        store<f32>(sp + kTanOut + k, v);
        store<f32>(c + 16 + k, v);
    }
    for (u32 k = 0; k < 12; k += 4) {
        const f32 v = Fadds(Lf(q + kSpPos + k), Lf(q + kSpIn + k));
        store<f32>(sp + kTanOut + k, v);
        store<f32>(c + 32 + k, v);
    }
    Copy3(c + 48, q + kSpPos);
}

}  // namespace

// u32 Spline_TangentEval(SplineData* d, -, f32 out[4], u32 seg, bool back; f32 t)
// The tangent at t (wrapped into [0, 1) for closed splines) from the
// segment search starting at `seg` (Bezier_Tangent on the segment's
// control points). With fewer than two points out = the up axis
// (0x01164B60) and the result is 0; otherwise the segment found (~0 when
// none contains t: the points at -1 and 0 with u = 0). f1 = out's z.
DECOMP(00560430, Spline_TangentEval) {
    const u32 d = arg<u32>(ctx, 0), out = arg<u32>(ctx, 2), seg = arg<u32>(ctx, 3);
    const bool back = (arg<u32>(ctx, 4) & 0xFF) != 0;
    f64 t = farg(ctx, 0);  // a single only once wrapped
    const u32 bits = load<u32>(d + kSaBits);
    const u32 n = bits & kSaIndexMask;
    if (static_cast<s32>(n) <= 1) {
        Copy3(out, obj11::kUpAxis);
        ctx.gpr[3] = 0;
        ctx.fpr[1] = static_cast<f64>(Lf(obj11::kUpAxis + 8));
        return;
    }
    const f32 u0 = Lf(kTeZero);
    if ((bits & kSaClosed) != 0) t = FracSp(t);
    const Segment s = FindSegment(d, t, seg, back, n - 1, u0);
    Stack frame(ctx, kFrame);
    const u32 sp = frame.addr();
    Control(sp, load<u32>(d + kSaPoints), s.i);
    callf(ctx, Bezier_Tangent, {sp + kTanOut, sp + kCtrl}, {s.u});
    Copy3(out, sp + kTanOut);
    ctx.gpr[3] = s.i;
    ctx.fpr[1] = static_cast<f64>(Lf(sp + kTanOut + 8));
}

// u32 Spline_PointTangent(SplineData* d, -, f32 pos[4], f32 tan[4], u32 seg, bool back; f32 t)
// Spline_TangentEval that also gives the point: pos = the cubic Bezier at u
// ((1-u)^3 P0 + 3u(1-u)^2 P1 + 3u^2(1-u) P2 + u^3 P3, summed from P3 down
// with fused multiply-adds). With fewer than two points pos = the first
// point (the origin with none), tan = the up axis and the result is 0.
DECOMP(005606C8, Spline_PointTangent) {
    const u32 d = arg<u32>(ctx, 0), pos = arg<u32>(ctx, 2), tan = arg<u32>(ctx, 3), seg = arg<u32>(ctx, 4);
    const bool back = (arg<u32>(ctx, 5) & 0xFF) != 0;
    f64 t = farg(ctx, 0);  // a single only once wrapped
    const u32 bits = load<u32>(d + kSaBits);
    const u32 n = bits & kSaIndexMask;
    if (static_cast<s32>(n) <= 1) {
        Copy3(pos, n == 0 ? obj10::kOrigin : load<u32>(d + kSaPoints));
        Copy3(tan, obj11::kUpAxis);
        ctx.gpr[3] = 0;
        ctx.fpr[1] = static_cast<f64>(Lf(obj11::kUpAxis + 8));
        return;
    }
    const f32 u0 = Lf(kPtConsts + kPtZero);
    if ((bits & kSaClosed) != 0) t = FracSp(t);
    const Segment s = FindSegment(d, t, seg, back, n - 1, u0);
    Stack frame(ctx, kFramePt);
    const u32 sp = frame.addr();
    Control(sp, load<u32>(d + kSaPoints), s.i);
    const f32 u = s.u;
    const f32 u3u = Fmuls(u, Lf(kPtConsts + kPtThree));
    const f32 v = Fsubs(Lf(kPtConsts + kPtOne), u);
    const f32 b3 = Fmuls(Fmuls(u, u), u);           // u^3
    const f32 b2 = Fmuls(Fmuls(u, u3u), v);         // 3u^2(1-u)
    const f32 vv = Fmuls(v, v);
    const f32 b1 = Fmuls(vv, u3u);                  // 3u(1-u)^2
    const f32 b0 = Fmuls(v, vv);                    // (1-u)^3
    const u32 c = sp + kCtrl;
    f32 r[3];
    for (u32 k = 0; k < 3; ++k) {
        f32 a = Fmuls(Lf(c + 32 + 4 * k), b2);
        a = Fmadds(Lf(c + 48 + 4 * k), b3, a);
        a = Fmadds(Lf(c + 16 + 4 * k), b1, a);
        r[k] = Fmadds(Lf(c + 4 * k), b0, a);
    }
    for (u32 k = 0; k < 3; ++k) store<f32>(sp + kTanOut + 4 * k, r[k]);
    for (u32 k = 0; k < 3; ++k) store<f32>(pos + 4 * k, r[k]);
    callf(ctx, Bezier_Tangent, {sp + kTanOut, sp + kCtrl}, {u});
    Copy3(tan, sp + kTanOut);
    ctx.gpr[3] = s.i;
    ctx.fpr[1] = static_cast<f64>(Lf(sp + kTanOut + 8));
}
