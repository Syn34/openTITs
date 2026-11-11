// Game code, VMX (AltiVec) function sweep part 3: helpers shared by
// vmx3_*.cpp (game functions the size sweeps skipped because they use VMX).
// Builds on vmx2.h (Madd4 / Nmsub4 / SplatW / Fill4 / Zero4 / RowSel /
// ProfMark) and, through it, vmx1.h and the big5..big17 lane helpers.
#pragma once

#include <bit>
#include <cmath>

#include "vmx2.h"

namespace sly4 {
namespace vmx3 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::vmx2;

constexpr u32 kV3WorldPtr = 0x00F65CA0;   // World*
constexpr u32 kV3WorldCam = 0x001281E0;   // World: the camera (+96 its matrix rows) or 0
constexpr u32 kV3Zero3 = 0x01164B40;      // f32[3] the zero vector

namespace v3fn {
constexpr u32 Cursor_ReadInput = 0x002411B0;  // void (Ui*, f32 out[3])
constexpr u32 Matrix_NearlyEqual = 0x005545B4;  // bool (const f32 a[4][4], const f32 b[4][4]; f1 eps)
}  // namespace v3fn

// fsel(a, b, c) then frsp.
inline f32 Fsels(f64 a, f64 b, f64 c) { return static_cast<f32>(a >= 0.0 ? b : c); }
// f32 vector length from its three words (fmuls y*y, fmadds x, fmadds z, fsqrts).
inline f32 Len3s(f32 x, f32 y, f32 z) { return Sqrts(Fmadds(z, z, Fmadds(x, x, Muls(y, y)))); }

// vrsqrtefp as the recompiler evaluates it (1.0f / sqrtf, lane by lane).
inline V4 Rsqrte4(const V4& a) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = 1.0f / std::sqrt(a.e[k]);
    return o;
}
// vcmpgefp a >= b: all-ones lanes where true.
inline V4 Cmpge4(const V4& a, const V4& b) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = std::bit_cast<f32>(a.e[k] >= b.e[k] ? 0xFFFFFFFFu : 0u);
    return o;
}
// vcmpeqfp a == b: all-ones lanes where true.
inline V4 Cmpeq4(const V4& a, const V4& b) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = std::bit_cast<f32>(a.e[k] == b.e[k] ? 0xFFFFFFFFu : 0u);
    return o;
}
// vor.
inline V4 Or4(const V4& a, const V4& b) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = std::bit_cast<f32>(LaneW(a, k) | LaneW(b, k));
    return o;
}
// The compiler's vector length from a splatted |v|^2 l2 (rsqrte e, s = e *
// l2, s + (1 - e * s) * (s * half); l2 itself where l2 or e is 0, 0 where l2
// <= 0). half / one are the splatted pool words.
inline V4 SqrtNewton4(const V4& l2, const V4& half, const V4& one) {
    const V4 z = Zero4();
    const V4 e = Rsqrte4(l2);
    const V4 s = Madd4(e, l2, z);
    const V4 pass = Or4(Cmpeq4(l2, z), Cmpeq4(e, z));
    const V4 r = Madd4(Nmsub4(e, s, one), Madd4(s, half, z), s);
    return Andc(Sel(r, l2, pass), Cmpge4(z, l2));
}
// The compiler's lane-rotation cross product a x b: with yzx / zxy the two
// vperm masks, -(a.zxy * b.yzx - a.yzx * b.zxy) (vmaddfp from +0, then
// vnmsubfp).
inline V4 Cross4(const V4& a, const V4& b, const V4& yzx, const V4& zxy) {
    return Nmsub4(Perm(a, a, zxy), Perm(b, b, yzx), Madd4(Perm(a, a, yzx), Perm(b, b, zxy), Zero4()));
}
// Lanes 0 + 1 + 2 of a, splatted (vspltw x3, vaddfp, vaddfp).
inline V4 HSum3(const V4& a) { return Add4(Add4(Splat(a, 0), Splat(a, 1)), Splat(a, 2)); }

}  // namespace vmx3
}  // namespace sly4
