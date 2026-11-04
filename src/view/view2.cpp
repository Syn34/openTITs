// View_GetCurrent, Mat4_Ortho and Matrix_ScaleRows.
#include "view2.h"

using namespace sly4;
using namespace sly4::view;

// View* View_GetCurrent()
// The view of the surface being drawn (current_surface +672).
DECOMP(004FDBA8, View_GetCurrent) {
    ret(ctx, load<u32>(load<u32>(v2g::current_surface) + 672));
}

// void Mat4_Ortho(f32 out[16]; f1 left, f2 right, f3 bottom, f4 top, f5 near, f6 far)
// An orthographic projection, row-major with the translation in row 3:
//   [0] = 2 / (r - l), [5] = 2 / (t - b), [10] = 1 / (n - f),
//   [12] = (l + r) / (l - r), [13] = (b + t) / (b - t), [14] = n / (n - f),
//   [15] = 1, everything else 0.
// Leaves f1 = (l + r) / (l - r) as the original does.
DECOMP(0056A358, Mat4_Ortho) {
    const u32 out = arg<u32>(ctx, 0);
    const f64 l = farg(ctx, 0), r = farg(ctx, 1), b = farg(ctx, 2), t = farg(ctx, 3);
    const f64 n = farg(ctx, 4), f = farg(ctx, 5);
    const f64 two = load<f32>(v2g::ortho_consts), one = load<f32>(v2g::ortho_consts + 4);
    const f64 nf = fsubs(n, f);
    const f64 sx = fdivs(two, fsubs(r, l));
    store<u32>(out + 4, 0);
    store<u32>(out + 8, 0);
    store<u32>(out + 12, 0);
    store<u32>(out + 16, 0);
    store<u32>(out + 24, 0);
    store<u32>(out + 28, 0);
    const f64 sy = fdivs(two, fsubs(t, b));
    const f64 tz = fdivs(n, nf);
    const f64 ty = fdivs(fadds(b, t), fsubs(b, t));
    const f64 tx = fdivs(fadds(l, r), fsubs(l, r));
    store<f32>(out + 0, static_cast<f32>(sx));
    store<f32>(out + 20, static_cast<f32>(sy));
    const f64 sz = fdivs(one, nf);
    store<u32>(out + 32, 0);
    store<u32>(out + 36, 0);
    store<u32>(out + 44, 0);
    store<u32>(out + 60, 0x3F800000);  // 1.0f
    store<f32>(out + 56, static_cast<f32>(tz));
    store<f32>(out + 48, static_cast<f32>(tx));
    store<f32>(out + 52, static_cast<f32>(ty));
    store<f32>(out + 40, static_cast<f32>(sz));
    ctx.fpr[1] = tx;
}

// void Matrix_ScaleRows(Matrix* m, const f32 scale[3])
// Scales the xyz of rows 0..2 of m by scale[0..2] (row i by scale[i]).
// Loads and stores in the original's order, so a scale inside m reads
// the already-scaled row 0 for rows 1 and 2.
DECOMP(00554394, Matrix_ScaleRows) {
    const u32 m = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    f64 k = load<f32>(s);
    const f64 m0 = load<f32>(m), m1 = load<f32>(m + 4), m2 = load<f32>(m + 8);
    const f64 r00 = fmuls(m0, k), r01 = fmuls(m1, k), r02 = fmuls(m2, k);
    const f64 m4 = load<f32>(m + 16), m5 = load<f32>(m + 20), m6 = load<f32>(m + 24);
    const f64 m8 = load<f32>(m + 32), m9 = load<f32>(m + 36);
    store<f32>(m, static_cast<f32>(r00));
    store<f32>(m + 4, static_cast<f32>(r01));
    store<f32>(m + 8, static_cast<f32>(r02));
    k = load<f32>(s + 4);
    const f64 r10 = fmuls(m4, k), r11 = fmuls(m5, k), r12 = fmuls(m6, k);
    const f64 m10 = load<f32>(m + 40);
    store<f32>(m + 16, static_cast<f32>(r10));
    store<f32>(m + 20, static_cast<f32>(r11));
    store<f32>(m + 24, static_cast<f32>(r12));
    k = load<f32>(s + 8);
    const f64 r20 = fmuls(m8, k), r21 = fmuls(m9, k), r22 = fmuls(m10, k);
    store<f32>(m + 32, static_cast<f32>(r20));
    store<f32>(m + 36, static_cast<f32>(r21));
    store<f32>(m + 40, static_cast<f32>(r22));
    ctx.fpr[1] = r22;
}
