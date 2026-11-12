// View_GetCurrent, Mat4_Ortho and Matrix_ScaleRows.
#include "view2.h"

namespace sly4::view {
using namespace opentit::native;

// The view of the surface being drawn (current_surface +672).
void* View_GetCurrent() { return host<void>(gref<be_u32>(global<be_u32>(v2g::current_surface) + 672)); }

// An orthographic projection, row-major with the translation in row 3:
//   [0] = 2 / (r - l), [5] = 2 / (t - b), [10] = 1 / (n - f),
//   [12] = (l + r) / (l - r), [13] = (b + t) / (b - t), [14] = n / (n - f),
//   [15] = 1, everything else 0. (The original leaves f1 = [12]; no caller reads it.)
void Mat4_Ortho(be_f32* out, f64 l, f64 r, f64 b, f64 t, f64 n, f64 f) {
    const be_f32* c = host<be_f32>(v2g::ortho_consts);
    const f64 two = c[0], one = c[1];
    be_u32* w = reinterpret_cast<be_u32*>(out);
    const f64 nf = fsubs(n, f);
    const f64 sx = fdivs(two, fsubs(r, l));
    for (u32 i : {1u, 2u, 3u, 4u, 6u, 7u}) w[i] = 0u;
    const f64 sy = fdivs(two, fsubs(t, b));
    const f64 tz = fdivs(n, nf);
    const f64 ty = fdivs(fadds(b, t), fsubs(b, t));
    const f64 tx = fdivs(fadds(l, r), fsubs(l, r));
    out[0] = static_cast<f32>(sx);
    out[5] = static_cast<f32>(sy);
    const f64 sz = fdivs(one, nf);
    for (u32 i : {8u, 9u, 11u}) w[i] = 0u;
    w[15] = 0x3F800000u;  // 1.0f
    out[14] = static_cast<f32>(tz);
    out[12] = static_cast<f32>(tx);
    out[13] = static_cast<f32>(ty);
    out[10] = static_cast<f32>(sz);
}

// Scales the xyz of rows 0..2 of m by scale[0..2] (row i by scale[i]).
// Loads and stores in the original's order, so a scale inside m reads
// the already-scaled row 0 for rows 1 and 2.
void Matrix_ScaleRows(be_f32* m, const be_f32* s) {
    f64 k = s[0];
    const f64 m0 = m[0], m1 = m[1], m2 = m[2];
    const f64 r00 = fmuls(m0, k), r01 = fmuls(m1, k), r02 = fmuls(m2, k);
    const f64 m4 = m[4], m5 = m[5], m6 = m[6];
    const f64 m8 = m[8], m9 = m[9];
    m[0] = static_cast<f32>(r00);
    m[1] = static_cast<f32>(r01);
    m[2] = static_cast<f32>(r02);
    k = s[1];
    const f64 r10 = fmuls(m4, k), r11 = fmuls(m5, k), r12 = fmuls(m6, k);
    const f64 m10 = m[10];
    m[4] = static_cast<f32>(r10);
    m[5] = static_cast<f32>(r11);
    m[6] = static_cast<f32>(r12);
    k = s[2];
    m[8] = static_cast<f32>(fmuls(m8, k));
    m[9] = static_cast<f32>(fmuls(m9, k));
    m[10] = static_cast<f32>(fmuls(m10, k));
}

}  // namespace sly4::view

NATIVE(004FDBA8, sly4::view::View_GetCurrent)
NATIVE(0056A358, sly4::view::Mat4_Ortho)
NATIVE(00554394, sly4::view::Matrix_ScaleRows)
