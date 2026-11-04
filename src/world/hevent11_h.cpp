// World_HandleEvent callees, part 11h: the volume item's beam placement and
// the camera shake picker.
#include <cmath>

#include "hevent11.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent11;

namespace {
f64 F(u32 ea) { return lfs_f64(ea); }
f64 r32(f64 x) { return static_cast<f32>(x); }
f64 fmul_s(f64 a, f64 c) { return r32(a * c); }
f64 fadd_s(f64 a, f64 b) { return r32(a + b); }
f64 fsub_s(f64 a, f64 b) { return r32(a - b); }
f64 fdiv_s(f64 a, f64 b) { return r32(a / b); }
f64 fmadd_s(f64 a, f64 c, f64 b) { return r32(std::fma(a, c, b)); }
f64 fmsub_s(f64 a, f64 c, f64 b) { return r32(std::fma(a, c, -b)); }
void stfs(u32 ea, f64 v) { store<f32>(ea, static_cast<f32>(v)); }

constexpr u32 kAxisA = 0x01164B70;      // vec3 axis used unless dir is (anti)parallel to it
constexpr u32 kAxisB = 0x01164B60;      // vec3 fallback axis
constexpr u32 kBeamK = 0x00399A98;      // f32 {1.0, 1e-5, -1.0, 0.5, 2.0}
constexpr u32 kMatNormalize = 0x0056D374;  // Matrix_NormalizeInPlace (Mat34*)
constexpr u32 kOneBits = 0x3F800000;

// Camera shake record (96 bytes): +0 f32 end, +4 f32 start, +8 f32 strength,
// +12 f32 bias, +16 u32 curve hash, +20 f32 time, +48 f32 radius, +64 vec3
// centre, +80 f32 inner radius.
constexpr u32 kShakeRecs = 36, kShakeCount = 44, kShakeRecSize = 96;
constexpr u32 kShakeK = 0x002EE468;     // f32 zero, f32 pi-ish scale, f64 x2, f32 one at +24
constexpr u32 kCurveSquare = 0x17197D25, kCurveSine = 0x920469AE, kCurveLinear = 0x85D98325, kCurveAdd = 0x9F9D6710;
constexpr u32 kListenerGetPos = 284;    // Object vtable: (vec3* out, Object*)
constexpr u32 kObjFlags192 = 192, kObjPos = 144;
constexpr u32 kSinf = 0x007345F0;       // Math_Sinf
}  // namespace

// void Item_Submit399AB0(Mat34* out, vec3* scale, const vec3* from, const
// vec3* dir; f32 len, f32 radius) (name?): a beam basis with row 1 = dir,
// row 0 = dir x A (or B x dir when dir is within 1e-5 of +-A), row 2 = row 0
// x row 1, normalised; row 3 = from + dir * len * 0.5; scale = {1, len /
// (radius * 2), 1}. r3 is left as 0x3F800000, f1 as the scale y.
DECOMP(00399AB0, Item_Submit399AB0) {
    const u32 out = arg<u32>(ctx, 0), scale = arg<u32>(ctx, 1), from = arg<u32>(ctx, 2), dir = arg<u32>(ctx, 3);
    const f64 len = ctx.fpr[1], radius = ctx.fpr[2];
    const u32 r0 = out, r1 = out + 16, r2 = out + 32;
    const f64 dx = F(dir), dy = F(dir + 4), dz = F(dir + 8);
    const f64 dot = fmadd_s(dz, F(kAxisA + 8), fmadd_s(dx, F(kAxisA), fmul_s(dy, F(kAxisA + 4))));
    const f64 eps = F(kBeamK + 4);
    const bool parallel = std::fabs(fsub_s(std::fabs(dot), F(kBeamK))) < eps ||
                          std::fabs(fsub_s(dot, F(kBeamK + 8))) < eps;
    stfs(r1, dx);
    stfs(r1 + 4, F(dir + 4));
    stfs(r1 + 8, F(dir + 8));
    if (!parallel) {
        stfs(r0, fmsub_s(F(dir + 4), F(kAxisA + 8), fmul_s(F(dir + 8), F(kAxisA + 4))));
        stfs(r0 + 4, fmsub_s(F(r1 + 8), F(kAxisA), fmul_s(F(r1), F(kAxisA + 8))));
        stfs(r0 + 8, fmsub_s(F(r1), F(kAxisA + 4), fmul_s(F(r1 + 4), F(kAxisA))));
    } else {
        stfs(r0, fmsub_s(F(kAxisB + 4), F(dir + 8), fmul_s(F(kAxisB + 8), F(dir + 4))));
        stfs(r0 + 4, fmsub_s(F(kAxisB + 8), F(r1), fmul_s(F(kAxisB), F(r1 + 8))));
        stfs(r0 + 8, fmsub_s(F(kAxisB), F(r1 + 4), fmul_s(F(kAxisB + 4), F(r1))));
    }
    stfs(r2, fmsub_s(F(r0 + 4), F(r1 + 8), fmul_s(F(r0 + 8), F(r1 + 4))));
    stfs(r2 + 4, fmsub_s(F(r0 + 8), F(r1), fmul_s(F(r0), F(r1 + 8))));
    stfs(r2 + 8, fmsub_s(F(r0), F(r1 + 4), fmul_s(F(r0 + 4), F(r1))));
    call(ctx, kMatNormalize, {out});
    const f64 half = fmul_s(len, F(kBeamK + 12));
    const f64 px = fmadd_s(F(dir), half, F(from)), py = fmadd_s(F(dir + 4), half, F(from + 4)),
              pz = fmadd_s(F(dir + 8), half, F(from + 8));
    stfs(out + 48, px);
    stfs(out + 52, py);
    stfs(out + 56, pz);
    const f64 sy = fdiv_s(len, fmul_s(radius, F(kBeamK + 16)));
    store<u32>(scale, kOneBits);
    store<u32>(scale + 8, kOneBits);
    stfs(scale + 4, sy);
    ctx.fpr[1] = sy;
    ret(ctx, kOneBits);
}

// void Shake_Pick2EE488(ShakeSet* s, s32* index, f32* amplitude) (name?):
// the strongest shake: each record's strength, scaled by the listener's
// distance falloff (user 0's object, or its vtable +284 position) between
// the inner and outer radius, and while running (time < end) by its curve
// (square, linear, sine, biased linear or none; clamped to [0, 1], then
// 1 - x, and the bias moved for the biased curve). Stores the index and
// amplitude of the first maximum above 0. r3 / f1 are left as the
// original leaves them.
DECOMP(002EE488, Shake_Pick2EE488) {
    const u32 s = arg<u32>(ctx, 0), index = arg<u32>(ctx, 1), amp = arg<u32>(ctx, 2);
    store<u32>(index, 0);
    store<u32>(amp, 0);
    const f64 f27 = F(kShakeK), f29 = F(kShakeK + 4);
    const f64 f30 = std::bit_cast<f64>(load<u64>(kShakeK + 8)), f31 = std::bit_cast<f64>(load<u64>(kShakeK + 16));
    f64 f1 = ctx.fpr[1];
    u32 n;
    for (u32 i = 0; i < (n = load<u16>(s + kShakeCount)); ++i) {
        const u32 rec = load<u32>(s + kShakeRecs) + i * kShakeRecSize;
        f64 f26 = F(kShakeK + 24);
        const f64 f28 = f26;
        f64 f2 = F(rec + 48);
        if (f2 > f27) {
            const u32 o = load<u32>(load<u32>(kCurrentWorld) + kUserBlocks + 8);
            if (o != 0) {
                f64 px, py, pz;
                if ((load<u32>(o + kObjFlags192) & 1) != 0) {
                    Stack frame(ctx, 16);
                    vcall(ctx, o, kListenerGetPos, {frame.addr(), o});
                    px = F(frame.addr()), py = F(frame.addr() + 4), pz = F(frame.addr() + 8);
                    f2 = F(rec + 48);
                } else {
                    px = F(o + kObjPos), py = F(o + kObjPos + 4), pz = F(o + kObjPos + 8);
                }
                const f64 ey = fsub_s(py, F(rec + 68)), ex = fsub_s(px, F(rec + 64)), ez = fsub_s(pz, F(rec + 72));
                const f64 r2 = fmul_s(f2, f2);
                f1 = fmadd_s(ez, ez, fmadd_s(ex, ex, fmul_s(ey, ey)));
                if (f1 > r2) {
                    f26 = f27;
                } else {
                    const f64 inner = F(rec + 80);
                    if (inner >= f27) {
                        f2 = fsub_s(f2, inner);
                        if (f2 > f27) {
                            f1 = fsub_s(r32(std::sqrt(f1)), inner);
                            if (f1 > f27) {
                                f1 = fdiv_s(f1, f2);
                                f26 = fsub_s(f26, f1);
                            }
                        }
                    }
                }
            }
        }
        f2 = F(rec + 20);
        f1 = F(rec + 0);
        f26 = fmul_s(F(rec + 8), f26);
        if (f2 < f1) {
            u32 k = load<u32>(rec + 16);
            if (k == kCurveSquare) {
                f1 = fsub_s(f1, f2);
                f2 = fdiv_s(f1, fsub_s(F(rec + 4), f2));
                f2 = fmul_s(f2, f2);
            } else if (k == kCurveAdd) {
                f1 = fsub_s(f1, f2);
                f2 = fdiv_s(f1, fsub_s(F(rec + 4), f2));
                f1 = F(rec + 12);
                f26 = fsub_s(f26, f1);
                f2 = fsub_s(f28, f2);
            } else if (k == kCurveSine) {
                f1 = fsub_s(f1, f2);
                f2 = fsub_s(F(rec + 4), f2);
                f1 = fmul_s(fdiv_s(f1, f2), f29);
                ctx.gpr[3] = k;
                f1 = callf(ctx, kSinf, {}, {f1});
                f2 = f1;
                k = load<u32>(rec + 16);
            } else if (k == kCurveLinear) {
                f1 = fsub_s(f1, f2);
                f2 = fdiv_s(f1, fsub_s(F(rec + 4), f2));
            } else {
                f2 = f27;
            }
            f1 = fsub_s(f28, f2);
            f1 = r32(f1 >= 0.0 ? f2 : f30);  // fsel / frsp: clamp
            f1 = r32(f1 >= 0.0 ? f1 : f31);
            f1 = fsub_s(f28, f1);
            f26 = fmul_s(f26, f1);
            if (k == kCurveAdd) {
                f1 = F(rec + 12);
                f26 = fadd_s(f26, f1);
            }
        }
        f1 = F(amp);
        if (f26 > f1) {
            store<u32>(index, i);
            stfs(amp, f26);
        }
    }
    ctx.fpr[1] = f1;
    ret(ctx, n);
}
