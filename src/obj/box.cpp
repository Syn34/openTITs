// Axis-aligned box helpers: {f32 min[4], f32 max[4]}, 16-byte aligned, the
// w lanes unused. The VMX ones are written lane by lane (vmaddfp = fmaf).
#include <cmath>

#include "box.h"

using namespace sly4;
using namespace sly4::box;

namespace {

struct Vec4 {
    f32 v[4];
};

// lvx: the quadword at ea & ~15.
inline Vec4 LoadV(u32 ea) {
    Vec4 r;
    const u32 a = ea & ~15u;
    for (u32 i = 0; i < 4; ++i) r.v[i] = load<f32>(a + i * 4);
    return r;
}

// stvx: the quadword at ea & ~15.
inline void StoreV(u32 ea, const Vec4& x) {
    const u32 a = ea & ~15u;
    for (u32 i = 0; i < 4; ++i) store<f32>(a + i * 4, x.v[i]);
}

// vandc with the sign mask (vspltisw -1; vslw): |x| bitwise.
inline f32 AbsBits(f32 x) {
    u32 b;
    std::memcpy(&b, &x, 4);
    b &= 0x7FFFFFFFu;
    std::memcpy(&x, &b, 4);
    return x;
}

}  // namespace

// void Box_SetPoint(Box* b, const f32 p[3])
// min = max = p (raw words; p is read again for max). f1 is left as p.z.
DECOMP(00556F60, Box_SetPoint) {
    const u32 b = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    for (u32 k = 0; k < 12; k += 4) store<u32>(b + kBoxMin + k, load<u32>(p + k));
    for (u32 k = 0; k < 12; k += 4) store<u32>(b + kBoxMax + k, load<u32>(p + k));
    ctx.fpr[1] = static_cast<f64>(load<f32>(p + 8));  // lfs f1 left behind
}

// f32* Box_Center(const Box* b, f32 out[3])
// out = min + (max - min) * 0.5 per axis (fmadds); returns out, f1-f3 the
// three components as the original leaves them.
DECOMP(00556F98, Box_Center) {
    const u32 b = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const f64 half = static_cast<f64>(load<f32>(kHalf));
    f32 c[3];
    for (u32 i = 0; i < 3; ++i) {
        const f32 lo = load<f32>(b + kBoxMin + i * 4), hi = load<f32>(b + kBoxMax + i * 4);
        const f32 d = static_cast<f32>(static_cast<f64>(hi) - static_cast<f64>(lo));
        c[i] = static_cast<f32>(std::fma(static_cast<f64>(d), half, static_cast<f64>(lo)));
    }
    for (u32 i = 0; i < 3; ++i) store<f32>(out + i * 4, c[i]);
    for (u32 i = 0; i < 3; ++i) ctx.fpr[1 + i] = static_cast<f64>(c[i]);
    ret(ctx, out);
}

// void Box_Transform(Box* out, const Box* in, const Matrix* m)
// The box around `in` moved by the 4x4 row matrix (rows 0-2 the axes, row 3
// the translation): centre c = (max + min) * 0.5, half size h = (max - min)
// * 0.5; centre' = row3 + rows . c, half' = |rows| . h; out = centre' -/+
// half'. All four lanes are computed and stored (vmaddfp chains). Every
// input is read before the output is written.
DECOMP(005570F0, Box_Transform) {
    const u32 out = arg<u32>(ctx, 0), in = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    const Vec4 lo = LoadV(in), hi = LoadV(in + 16);
    const Vec4 r0 = LoadV(m), r1 = LoadV(m + 16), r2 = LoadV(m + 32), r3 = LoadV(m + 48);
    const f32 k = load<f32>(kHalfVmx);  // lvlx + vspltw 0
    f32 c[4], h[4];
    for (u32 i = 0; i < 4; ++i) {
        c[i] = std::fmaf(hi.v[i] + lo.v[i], k, 0.0f);
        h[i] = std::fmaf(hi.v[i] - lo.v[i], k, 0.0f);
    }
    Vec4 omin, omax;
    for (u32 i = 0; i < 4; ++i) {
        f32 p = std::fmaf(r0.v[i], c[0], r3.v[i]);
        f32 e = std::fmaf(AbsBits(r0.v[i]), h[0], 0.0f);
        p = std::fmaf(r1.v[i], c[1], p);
        e = std::fmaf(AbsBits(r1.v[i]), h[1], e);
        p = std::fmaf(r2.v[i], c[2], p);
        e = std::fmaf(AbsBits(r2.v[i]), h[2], e);
        omin.v[i] = p - e;
        omax.v[i] = p + e;
    }
    StoreV(out, omin);
    StoreV(out + 16, omax);
}

// bool Box_Overlaps(const Box* outer, const Box* inner)
// In fact a containment test: inner.min >= outer.min and outer.max >=
// inner.max on x, y and z (the w lanes masked off; NaN fails).
DECOMP(00557430, Box_Overlaps) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const Vec4 amin = LoadV(a), amax = LoadV(a + 16), bmin = LoadV(b), bmax = LoadV(b + 16);
    bool all = true;
    for (u32 i = 0; i < 3; ++i)
        if (!(bmin.v[i] >= amin.v[i]) || !(amax.v[i] >= bmax.v[i])) all = false;
    ret(ctx, all ? 1u : 0u);
}
