// World objects, part 12: matrix and quaternion helpers.
#include "obj12.h"

using namespace sly4;
using namespace sly4::obj8;
using namespace sly4::obj9;
using namespace sly4::obj10;
using namespace sly4::obj12;

namespace {

constexpr u32 kMmPerm = 0x005541D0;   // u8[2][16]: the row-gathering vperm masks
constexpr u32 kQfConsts = 0x00555464; // f32 {0, 0.5, 0.25, 1}
constexpr u32 kQfZero = 0, kQfHalf = 4, kQfQuarter = 8, kQfOne = 12;

}  // namespace

// void Matrix_Multiply(Matrix* a, const Matrix* b)
// a = b x a in the row-vector sense the transform code uses (obj8.h
// MatCombine): row i of the result is a0 * b_i.x + a1 * b_i.y + a2 * b_i.z
// (+ a3 for i = 3). b is read aligned; a's rows are gathered unaligned
// (lvlx), and each row's w lane comes from the stack below the caller's
// frame (sp+60 / +76 / +92 / +108 of this function's 144-byte frame), so
// the result's w lanes depend on what the previous calls left there: read
// at the same addresses to keep that. The result is stored aligned.
// r4..r11 as the original leaves them.
DECOMP(005541F0, Matrix_Multiply) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 sp = static_cast<u32>(ctx.gpr[1]) - 144;
    const Vec4 m0 = VLoad(kMmPerm), m1 = VLoad(kMmPerm + 16);
    Vec4 rows[4], bm[4], out[4];
    for (u32 i = 0; i < 4; ++i) {
        const u32 r = a + 16 * i;
        const Vec4 lo = VPerm(VLoadLeft(r), VLoadLeft(r + 4), m0);
        const Vec4 hi = VPerm(VLoadLeft(r + 8), VLoad(sp + 48 + 16 * i), m0);
        rows[i] = VPerm(lo, hi, m1);
    }
    MatLoad(b, bm);
    MatCombine(rows, bm, out);
    MatStore(a, out);
    ctx.gpr[4] = a + 16;
    ctx.gpr[5] = a + 32;
    ctx.gpr[6] = a + 48;
    ctx.gpr[7] = 56;
    ctx.gpr[8] = 16;
    ctx.gpr[9] = 32;
    ctx.gpr[10] = 80;
    ctx.gpr[11] = 36;
}

// void Quat_FromMatrix(f32 out[4], const f32 m[4][4])
// The rotation's quaternion {x, y, z, w}: from the trace when it is
// positive, otherwise from the largest diagonal element (s = sqrt(1 + ...)
// / 2, the others (m_ij +- m_ji) * 0.25 / s). Each element is loaded after
// the previous store, as in the original, so out may alias m. r4..r9 / f1
// as the original leaves them.
DECOMP(00555478, Quat_FromMatrix) {
    const u32 out = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const f32 m00 = Lf(m), m11 = Lf(m + 20), m22 = Lf(m + 40);
    const f32 zero = Lf(kQfConsts + kQfZero), half = Lf(kQfConsts + kQfHalf);
    const f32 quarter = Lf(kQfConsts + kQfQuarter), one = Lf(kQfConsts + kQfOne);
    const f32 trace = Fadds(Fadds(m00, m11), m22);
    // m(i, j) at m + 16 i + 4 j
    auto e = [&](u32 i, u32 j) { return Lf(m + 16 * i + 4 * j); };
    auto put = [&](u32 k, f32 v) { store<f32>(out + 4 * k, v); };
    f32 f, last;
    u32 r4, r9;
    if (trace > zero) {
        const f32 s = Fmuls(Fsqrts(Fadds(trace, one)), half);
        f = Fdivs(quarter, s);
        put(3, s);
        put(0, Fmuls(Fsubs(e(1, 2), e(2, 1)), f));
        put(1, Fmuls(Fsubs(e(2, 0), e(0, 2)), f));
        put(2, last = Fmuls(Fsubs(e(0, 1), e(1, 0)), f));
        r4 = m + 4, r9 = m + 8;
    } else if (m00 > m11 && m00 > m22) {
        const f32 s = Fmuls(Fsqrts(Fadds(Fsubs(Fsubs(m00, m11), m22), one)), half);
        f = Fdivs(quarter, s);
        put(0, s);
        put(1, Fmuls(Fadds(e(1, 0), e(0, 1)), f));
        put(2, Fmuls(Fadds(e(2, 0), e(0, 2)), f));
        put(3, last = Fmuls(Fsubs(e(1, 2), e(2, 1)), f));
        r4 = m + 8, r9 = m + 4;
    } else if (m11 > m22) {
        const f32 s = Fmuls(Fsqrts(Fadds(Fsubs(Fsubs(m11, m00), m22), one)), half);
        f = Fdivs(quarter, s);
        put(1, s);
        put(0, Fmuls(Fadds(e(1, 0), e(0, 1)), f));
        put(2, Fmuls(Fadds(e(2, 1), e(1, 2)), f));
        put(3, last = Fmuls(Fsubs(e(2, 0), e(0, 2)), f));
        r4 = m + 8, r9 = m + 4;
    } else {
        const f32 s = Fmuls(Fsqrts(Fadds(Fsubs(Fsubs(m22, m00), m11), one)), half);
        f = Fdivs(quarter, s);
        put(2, s);
        put(0, Fmuls(Fadds(e(2, 0), e(0, 2)), f));
        put(1, Fmuls(Fadds(e(2, 1), e(1, 2)), f));
        put(3, last = Fmuls(Fsubs(e(0, 1), e(1, 0)), f));
        r4 = m + 4, r9 = m + 8;
    }
    ctx.fpr[1] = static_cast<f64>(last);
    ctx.gpr[4] = r4;
    ctx.gpr[5] = m + 16;
    ctx.gpr[6] = m + 24;
    ctx.gpr[7] = m + 36;
    ctx.gpr[8] = m + 32;
    ctx.gpr[9] = r9;
}
