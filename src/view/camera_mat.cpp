// Scalar 4x4 matrix helpers the camera and view code use: determinant,
// inverse (by cofactors), copy, and the in-place vec4 transform.
#include "camera.h"

#include <bit>

namespace sly4::view {
using namespace opentit::native;

namespace {

constexpr u32 kOne = 0x00554C20;  // f32 1.0 (the inverse's numerator)

}  // namespace

// Laplace expansion along the last row, as 24 triple products fused into
// one running sum in the original's order (m(i) = m[i], row-major).
f32 Mat4_Determinant(const be_f32* src) {
    const auto m = [&](u32 i) -> f64 { return src[i]; };
    const auto q = [&](u32 a, u32 b, u32 c) { return fmuls(fmuls(m(a), m(b)), m(c)); };
    const f64 det =
        fmadds(q(0, 5, 10), m(15), fnmsubs(q(1, 4, 10), m(15), fnmsubs(q(0, 6, 9), m(15), fmadds(q(2, 4, 9), m(15),
        fmadds(q(1, 6, 8), m(15), fnmsubs(q(2, 5, 8), m(15), fnmsubs(q(0, 5, 11), m(14), fmadds(q(1, 4, 11), m(14),
        fmadds(q(0, 7, 9), m(14), fnmsubs(q(3, 4, 9), m(14), fnmsubs(q(1, 7, 8), m(14), fmadds(q(3, 5, 8), m(14),
        fmadds(q(0, 6, 11), m(13), fnmsubs(q(2, 4, 11), m(13), fnmsubs(q(0, 7, 10), m(13), fmadds(q(3, 4, 10), m(13),
        fmadds(q(2, 7, 8), m(13), fnmsubs(q(3, 6, 8), m(13), fnmsubs(q(1, 6, 11), m(12), fmadds(q(2, 5, 11), m(12),
        fmadds(q(1, 7, 10), m(12), fnmsubs(q(3, 5, 10), m(12), fmsubs(q(3, 6, 9), m(12),
        fmuls(q(2, 7, 9), m(12)))))))))))))))))))))))));
    return static_cast<f32>(det);
}

// void Mat4_Invert(f32 out[16], f32 tmp[16], const f32 in[16])
// tmp = adjugate(in) * (1 / det(in)) element by element (each cofactor
// a 6-term fused sum, in the original's order), then out = tmp
// (Mat4_Copy). A zero determinant gives inf / NaN, unchecked. Each
// element re-reads `in` after the previous element's store, as the
// original does, so tmp overlapping in behaves the same.
// (The original leaves f1 = out[15]; no caller reads it.)
void Mat4_Invert(be_f32* out, be_f32* tmp, const be_f32* src) {
    const f64 det = Mat4_Determinant(src);
    const f64 inv = fdivs(global<be_f32>(kOne), det);
    const auto s = [&](u32 i) -> f64 { return src[i]; };
    const auto p = [&](u32 a, u32 b) { return fmuls(s(a), s(b)); };
    const auto put = [&](u32 off, f64 v) { tmp[off / 4] = static_cast<f32>(v); };
    put(0, fmuls(fmadds(p(5, 10), s(15), fnmsubs(p(6, 9), s(15), fnmsubs(p(5, 11), s(14), fmadds(p(7, 9), s(14), fmsubs(p(6, 11), s(13), fmuls(p(7, 10), s(13))))))), inv));
    put(4, fmuls(fnmsubs(p(1, 10), s(15), fmadds(p(2, 9), s(15), fmadds(p(1, 11), s(14), fnmsubs(p(3, 9), s(14), fmsubs(p(3, 10), s(13), fmuls(p(2, 11), s(13))))))), inv));
    put(8, fmuls(fmadds(p(1, 6), s(15), fnmsubs(p(2, 5), s(15), fnmsubs(p(1, 7), s(14), fmadds(p(3, 5), s(14), fmsubs(p(2, 7), s(13), fmuls(p(3, 6), s(13))))))), inv));
    put(12, fmuls(fnmsubs(p(1, 6), s(11), fmadds(p(2, 5), s(11), fmadds(p(1, 7), s(10), fnmsubs(p(3, 5), s(10), fmsubs(p(3, 6), s(9), fmuls(p(2, 7), s(9))))))), inv));
    put(16, fmuls(fnmsubs(p(4, 10), s(15), fmadds(p(6, 8), s(15), fmadds(p(4, 11), s(14), fnmsubs(p(7, 8), s(14), fmsubs(p(7, 10), s(12), fmuls(p(6, 11), s(12))))))), inv));
    put(20, fmuls(fmadds(p(0, 10), s(15), fnmsubs(p(2, 8), s(15), fnmsubs(p(0, 11), s(14), fmadds(p(3, 8), s(14), fmsubs(p(2, 11), s(12), fmuls(p(3, 10), s(12))))))), inv));
    put(24, fmuls(fnmsubs(p(0, 6), s(15), fmadds(p(2, 4), s(15), fmadds(p(0, 7), s(14), fnmsubs(p(3, 4), s(14), fmsubs(p(3, 6), s(12), fmuls(p(2, 7), s(12))))))), inv));
    put(28, fmuls(fmadds(p(0, 6), s(11), fnmsubs(p(2, 4), s(11), fnmsubs(p(0, 7), s(10), fmadds(p(3, 4), s(10), fmsubs(p(2, 7), s(8), fmuls(p(3, 6), s(8))))))), inv));
    put(32, fmuls(fmadds(p(4, 9), s(15), fnmsubs(p(5, 8), s(15), fnmsubs(p(4, 11), s(13), fmadds(p(7, 8), s(13), fmsubs(p(5, 11), s(12), fmuls(p(7, 9), s(12))))))), inv));
    put(36, fmuls(fnmsubs(p(0, 9), s(15), fmadds(p(1, 8), s(15), fmadds(p(0, 11), s(13), fnmsubs(p(3, 8), s(13), fmsubs(p(3, 9), s(12), fmuls(p(1, 11), s(12))))))), inv));
    put(40, fmuls(fmadds(p(0, 5), s(15), fnmsubs(p(1, 4), s(15), fnmsubs(p(0, 7), s(13), fmadds(p(3, 4), s(13), fmsubs(p(1, 7), s(12), fmuls(p(3, 5), s(12))))))), inv));
    put(44, fmuls(fnmsubs(p(0, 5), s(11), fmadds(p(1, 4), s(11), fmadds(p(0, 7), s(9), fnmsubs(p(3, 4), s(9), fmsubs(p(3, 5), s(8), fmuls(p(1, 7), s(8))))))), inv));
    put(48, fmuls(fnmsubs(p(4, 9), s(14), fmadds(p(5, 8), s(14), fmadds(p(4, 10), s(13), fnmsubs(p(6, 8), s(13), fmsubs(p(6, 9), s(12), fmuls(p(5, 10), s(12))))))), inv));
    put(52, fmuls(fmadds(p(0, 9), s(14), fnmsubs(p(1, 8), s(14), fnmsubs(p(0, 10), s(13), fmadds(p(2, 8), s(13), fmsubs(p(1, 10), s(12), fmuls(p(2, 9), s(12))))))), inv));
    put(56, fmuls(fnmsubs(p(0, 5), s(14), fmadds(p(1, 4), s(14), fmadds(p(0, 6), s(13), fnmsubs(p(2, 4), s(13), fmsubs(p(2, 5), s(12), fmuls(p(1, 6), s(12))))))), inv));
    put(60, fmuls(fmadds(p(0, 5), s(10), fnmsubs(p(1, 4), s(10), fnmsubs(p(0, 6), s(9), fmadds(p(2, 4), s(9), fmsubs(p(1, 6), s(8), fmuls(p(2, 5), s(8))))))), inv));
    Mat4_Copy(out, tmp);
}

// Word by word, in order (lfs / stfs pairs). (The original leaves f1 = src[15].)
void Mat4_Copy(be_f32* dst, const be_f32* src) {
    be_u32* d = reinterpret_cast<be_u32*>(dst);
    const be_u32* s = reinterpret_cast<const be_u32*>(src);
    for (u32 i = 0; i < 16; ++i) d[i] = s[i].get();
}

// v = v * m through Vec4_TransformByMatrix, with v copied to the stack
// first (sp+112 of the original's 128-byte frame).
void Vec4_Transform(be_f32* v, const void* m) {
    GuestFrame frame(16);
    be_u32* copy = frame.as<be_u32>();
    const be_u32* src = reinterpret_cast<const be_u32*>(v);
    for (u32 i = 0; i < 4; ++i) copy[i] = src[i].get();
    call(vfn::Vec4_TransformByMatrix, v, copy, m);
}

}  // namespace sly4::view

NATIVE(00554A2C, sly4::view::Mat4_Determinant)
NATIVE(00554C28, sly4::view::Mat4_Invert)
NATIVE(007A430C, sly4::view::Mat4_Copy)
NATIVE(005539E4, sly4::view::Vec4_Transform)
