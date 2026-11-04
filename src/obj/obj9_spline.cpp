// World objects, part 9: the spline definition (ApeSplineDef) constructor
// and its point array (SplineArray, at the definition's +112): set-up,
// evaluation and the nearest-point search.
#include "../heap/heap.h"
#include "obj9.h"

using namespace sly4;
using namespace sly4::obj9;

namespace {

constexpr u32 kKindSplineDef = 25;
constexpr u32 kVtblSplineDef = 0x00B24830;
constexpr u32 kEvalConsts = 0x0056004C;     // f32 {0, 3, 1} (Spline_Eval)
constexpr u32 kNearestConsts = 0x0055EFFC;  // f32 {0, FLT_MAX, 1} (Spline_NearestSeg)
constexpr u32 kNearestIterations = 8;

// The inlined release of the definition's shared block (+108): drops a
// reference; at zero frees its data (allocated 16 bytes in) and the block.
// Returns what the original leaves in r3 (`none` when there is no block).
u64 ReleaseShared(PPUContext& ctx, u32 b, u32 d, u64 none) {
    if (b == 0) return none;
    store<u32>(d + obj8::kSdShared, 0);
    const u64 refs = static_cast<u64>(load<u16>(b)) - 1;
    store<u16>(b, static_cast<u16>(refs));
    if (static_cast<s16>(refs) > 0) return refs;
    const u32 data = load<u32>(b + 4);
    if (data != 0) {
        call(ctx, o9fn::Mem_FreeFrom, {data - 16, 0});
        store<u32>(b + 4, 0);
    }
    return call(ctx, o9fn::Mem_FreeFrom, {b, 0});
}

}  // namespace

// void SplineArray_Reset(SplineArray* a)
// Clears all four words (the points are not freed).
DECOMP(0055F244, SplineArray_Reset) {
    const u32 a = arg<u32>(ctx, 0);
    store<u32>(a + 4, 0);
    store<u32>(a + 8, 0);
    store<u32>(a + 12, 0);
    store<u32>(a, 0);  // the original masks every field of the loaded word out
    ctx.gpr[4] = 0;
    ctx.gpr[5] = 0;
}

// void SplineDef_Construct(ApeSplineDef* d)
// The root header as kind 25 with a new object serial and one reference,
// an empty box collapsed to the origin, the default sphere, no name or
// shared block (both constructor stages, vtables 0x00B246B0 and
// 0x00B24830), and an empty point array.
DECOMP(0059ECF8, SplineDef_Construct) {
    const u32 d = arg<u32>(ctx, 0);
    const u32 w4 = (load<u32>(d + 4) & 0x3FFu) | 0x00010000u | (kKindSplineDef << 10);
    store<u16>(d + 4, 1);
    store<u32>(d + 4, w4);
    const u32 serial = load<u32>(obj2::kObjSerial) + 1;
    store<u32>(d, obj4::kVtblRoot);
    store<u32>(obj2::kObjSerial, serial);
    u32 w8 = load<u32>(d + 8);
    store<u32>(d + 32, obj4::kFltMinNeg);
    store<u32>(d, cls::kVtblGeomBase);
    w8 = ((w8 & 0x7FFu) | (serial << 11)) & ~0x400u & ~0x3C0u;
    store<u32>(d + 36, obj4::kFltMinNeg);
    store<u32>(d + 16, obj4::kFltMax);
    store<u32>(d + 20, obj4::kFltMax);
    store<u32>(d + 24, obj4::kFltMax);
    store<u32>(d + 40, obj4::kFltMinNeg);
    store<u32>(d + obj8::kSdShared, 0);
    store<u32>(d + 12, 0);
    store<u32>(d + 8, w8);
    call(ctx, cls::cfn::Box_SetPoint, {d + 16, cls::kOrigin});
    const u32 sph = cls::kDefaultSphere;
    store<u32>(d + 48, load<u32>(sph));
    store<u32>(d + 52, load<u32>(sph + 4));
    u32 b = load<u32>(d + obj8::kSdShared);
    store<u32>(d + 56, load<u32>(sph + 8));
    store<u32>(d + 96, 0);
    store<u32>(d + 60, load<u32>(sph + 12));
    store<u32>(d + 100, 0);
    store<u8>(d + 64, 0);
    store<u32>(d + 104, 0);
    ReleaseShared(ctx, b, d, 0);

    store<u32>(d, kVtblSplineDef);
    call(ctx, o9fn::SplineArray_Reset, {d + obj8::kSdData});
    call(ctx, cls::cfn::Box_SetPoint, {d + 16, cls::kOrigin});
    b = load<u32>(d + obj8::kSdShared);
    store<u32>(d + 48, load<u32>(sph));
    store<u32>(d + 52, load<u32>(sph + 4));
    store<u32>(d + 96, 0);
    store<u32>(d + 56, load<u32>(sph + 8));
    store<u32>(d + 100, 0);
    store<u32>(d + 60, load<u32>(sph + 12));
    store<u8>(d + 64, 0);
    store<u32>(d + 104, 0);
    ctx.fpr[1] = static_cast<f64>(Lf(sph + 12));
    ctx.gpr[3] = ReleaseShared(ctx, b, d, 0x01160000u);  // r3 / f1 as the original leaves them
}

// u32 SplineArray_Init(SplineArray* a, u32 count, bool closed, bool flag30)
// Sets the count and the two flags, frees the old points (pool 0) and,
// for a nonzero count, allocates and zeroes count (+1 when closed) 64-byte
// points from pool 0. The allocated count (bits 0-14) is then set to the
// point count; returns the new header word.
// The original inlines the memory manager's lazy set-up before the
// allocation (heap::MemState_EnsureInit).
DECOMP(0055F2BC, SplineArray_Init) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 count = arg<u32>(ctx, 1);
    u32 w = load<u32>(a + kSaBits);
    const u32 old = load<u32>(a + kSaPoints);
    w = (w & kSaAllocMask) | ((count << kSaCountShift) & kSaCountMask) | ((arg<u32>(ctx, 2) & 1) << 31) |
        ((arg<u32>(ctx, 3) & 1) << 30);
    store<u32>(a + kSaBits, w);
    if (old != 0) {
        call(ctx, o9fn::Mem_FreePool, {old, 0, 0});
        w = load<u32>(a + kSaBits);
        store<u32>(a + kSaPoints, 0);
    }
    u32 n = (w >> kSaCountShift) & 0x7FFF;
    if (n != 0) {
        if ((w & kSaClosed) != 0) {
            ++n;
            w = (w & ~kSaCountMask) | ((n << kSaCountShift) & kSaCountMask);
            store<u32>(a + kSaBits, w);
        }
        const u32 size = ((w >> kSaCountShift) & 0x7FFF) * kSpSize;
        heap::MemState_EnsureInit(ctx);
        const u32 p = static_cast<u32>(call(ctx, o9fn::Mem_AllocPool, {16, size, 0, 0}));
        store<u32>(a + kSaPoints, p);
        call(ctx, obj4::o4fn::memset, {p, 0, size});
        w = load<u32>(a + kSaBits);
    }
    w = (w & ~kSaAllocMask) | ((w >> kSaCountShift) & 0x7FFF);
    store<u32>(a + kSaBits, w);
    ret(ctx, w);
}

// f32 Spline_NearestSeg(const SplineArray* a, const f32 p[3], u32 first, u32 end, u32* seg, f32* dist)
// The segments first..end-1 as Bezier curves (point i, i + its outgoing
// handle, point i+1 + its incoming handle, point i+1), each searched by
// 0x0055ED60 (8 iterations); for the closest one the curve parameter at
// the found position (point parameters interpolated) clamped to [0, 1],
// with its index in *seg (-1 = none) and its distance in *dist (FLT_MAX =
// none). Returns 0 when no segment is closer than FLT_MAX.
DECOMP(0055F008, Spline_NearestSeg) {
    const u32 a = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 end = arg<u32>(ctx, 3), seg = arg<u32>(ctx, 4), dist = arg<u32>(ctx, 5);
    const f32 lo = Lf(kNearestConsts), hi = Lf(kNearestConsts + 8);
    f32 result = lo;
    u32 best_bits = load<u32>(kNearestConsts + 4);
    u32 best_seg = ~0u;
    u64 r3 = a;  // r3 as the original leaves it
    Stack frame(ctx, 288);
    const u32 sp = frame.addr();
    const u32 ctrl = sp + 112, out = sp + 176;
    for (u32 i = arg<u32>(ctx, 2); i < end; ++i) {
        const u32 off = i << 6;
        u32 base = load<u32>(a + kSaPoints);
        const u32 pt = base + off, nx = base + off + kSpSize;
        const f32 x = Lf(pt), y = Lf(pt + 4), z = Lf(pt + 8);
        store<f32>(ctrl, x);
        store<f32>(ctrl + 4, y);
        store<f32>(ctrl + 8, z);
        store<f32>(ctrl + 16, Fadds(x, Lf(pt + kSpOut)));
        store<f32>(ctrl + 20, Fadds(y, Lf(pt + kSpOut + 4)));
        store<f32>(ctrl + 24, Fadds(z, Lf(pt + kSpOut + 8)));
        const f32 nx0 = Lf(nx), nx1 = Lf(nx + 4), nx2 = Lf(nx + 8);
        const f32 s0 = Fadds(nx0, Lf(nx + kSpIn)), s1 = Fadds(nx1, Lf(nx + kSpIn + 4)), s2 = Fadds(nx2, Lf(nx + kSpIn + 8));
        store<f32>(ctrl + 48, nx0);
        store<f32>(ctrl + 52, nx1);
        store<f32>(ctrl + 56, nx2);
        store<f32>(ctrl + 32, s0);
        store<f32>(ctrl + 36, s1);
        store<f32>(ctrl + 40, s2);
        r3 = call(ctx, o9fn::SplineSeg_Nearest, {ctrl, p, out, kNearestIterations});
        const f64 t = ctx.fpr[1];
        const u32 d_bits = load<u32>(out);
        if (!(std::bit_cast<f32>(d_bits) < std::bit_cast<f32>(best_bits))) continue;
        best_bits = d_bits;
        base = load<u32>(a + kSaPoints);
        const f32 t0 = Lf(base + off + kSpParam);
        const f32 v = Fmadds(t, Fsubs(Lf(base + off + kSpSize + kSpParam), t0), t0);
        r3 = base + off + kSpSize + kSpParam;
        const f32 c = v < hi ? v : hi;
        result = c > lo ? c : lo;
        best_seg = i;
    }
    if (seg != 0) store<u32>(seg, best_seg);
    if (dist != 0) store<u32>(dist, best_bits);
    ctx.gpr[3] = r3;
    ret(ctx, result);
}

// void Spline_Eval(SplineArray* a, f32 t, f32 out[3], u32 start, bool backward)
// The position at curve parameter t (wrapped into [0, 1) for a closed
// array): from segment `start` the segments are searched forwards
// (wrapping to 0 after the last) or backwards for one whose point
// parameters bracket t; the found index goes to +12 and the segment is
// evaluated as a cubic Bezier. With one point that point, with none the
// origin. r3 = the segment (0 for those).
// Quirk kept: when no segment brackets t the original still evaluates
// "segment -1" at u = 0 (the 64 bytes before the points), and r3 = -1.
DECOMP(00560058, Spline_Eval) {
    const u32 a = arg<u32>(ctx, 0), out = arg<u32>(ctx, 2);
    f64 t = farg(ctx, 0);
    const u32 w = load<u32>(a + kSaBits);
    const u32 n = w & 0x7FFF;
    if (static_cast<s32>(n) <= 1) {
        const u32 src = n == 0 ? cls::kOrigin : load<u32>(a + kSaPoints);
        store<u32>(out, load<u32>(src));
        store<u32>(out + 4, load<u32>(src + 4));
        store<u32>(out + 8, load<u32>(src + 8));
        ctx.fpr[1] = static_cast<f64>(Lf(src + 8));
        ctx.gpr[3] = 0;
        return;
    }
    f32 u = Lf(kEvalConsts);
    if ((w & kSaClosed) != 0) t = Fsubs(t, static_cast<f32>(rt::fcfid(rt::fctid(t, true))));
    const u32 start = arg<u32>(ctx, 3);
    const bool backward = (arg<u32>(ctx, 4) & 0xFF) != 0;
    const u32 last = n - 1;
    u32 i = start, seg = ~0u, pts = 0;
    do {
        const u32 off = i << 6;
        pts = load<u32>(a + kSaPoints);
        const f32 t0 = Lf(pts + off + kSpParam);
        if (t >= t0 && t <= Lf(pts + off + kSpSize + kSpParam)) {
            store<u32>(a + kSaLastSeg, i);
            seg = i;
            u = Fdivs(Fsubs(t, t0), Fsubs(Lf(pts + ((i + 1) << 6) + kSpParam), t0));
            break;
        }
        if (!backward) {
            ++i;
            if (i >= last) i = 0;
        } else {
            i = i == 0 ? last : i - 1;
        }
    } while (i != start);

    // B(u) = P s^3 + (P + out) 3 u s^2 + (Q + in) 3 u^2 s + Q u^3, s = 1 - u.
    const f32 a3u = Fmuls(u, Lf(kEvalConsts + 4));
    const f32 s = Fsubs(Lf(kEvalConsts + 8), u);
    const f32 u2 = Fmuls(u, u);
    const f32 s2 = Fmuls(s, s);
    const f32 u3 = Fmuls(u2, u);
    const f32 wq = Fmuls(Fmuls(u, a3u), s);  // 3 u^2 s
    const f32 wp = Fmuls(s2, a3u);            // 3 u s^2
    const f32 s3 = Fmuls(s, s2);
    const u32 P = pts + (seg << 6), Q = pts + ((seg + 1) << 6);
    f32 r[3];
    for (u32 k = 0; k < 3; ++k) {
        const f32 q = Lf(Q + 4 * k), p = Lf(P + 4 * k);
        const f32 qh = Fmuls(Fadds(q, Lf(Q + kSpIn + 4 * k)), wq);
        const f32 ph = Fadds(p, Lf(P + kSpOut + 4 * k));
        r[k] = Fmadds(p, s3, Fmadds(ph, wp, Fmadds(q, u3, qh)));
    }
    store<f32>(out, r[0]);
    store<f32>(out + 4, r[1]);
    store<f32>(out + 8, r[2]);
    ctx.fpr[1] = static_cast<f64>(r[2]);
    ctx.gpr[3] = seg;
}
