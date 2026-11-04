// World objects, part 11: the quaternion helpers the peer animators and
// the spline follower use.
#include "obj11.h"

using namespace sly4;
using namespace sly4::obj9;
using namespace sly4::obj10;
using namespace sly4::obj11;

// void Quat_Multiply(f32 out[4], const f32 a[4], const f32 b[4])
// out = a * b for quaternions {x, y, z, w}: w = aw bw - a.b, v = aw b.v +
// bw a.v + a x b, as fused multiply-adds in the original's order. All loads
// come first, so out may alias a or b. f1 = out[1], as the original leaves it.
DECOMP(005556A4, Quat_Multiply) {
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const f32 b0 = Lf(b), a0 = Lf(a), b3 = Lf(b + 12), b2 = Lf(b + 8), b1 = Lf(b + 4);
    const f32 a3 = Lf(a + 12), a1 = Lf(a + 4), a2 = Lf(a + 8);
    f32 x = Fmuls(a0, b3), y = Fmuls(a0, b2), z = Fmuls(a0, b1), w = Fmuls(a0, b0);
    x = Fmadds(a3, b0, x);
    y = Fmsubs(a3, b1, y);
    z = Fmadds(a3, b2, z);
    w = Fmsubs(a3, b3, w);
    x = Fmadds(a1, b2, x);
    y = Fmadds(a1, b3, y);
    z = Fnmsubs(a1, b0, z);
    w = Fnmsubs(a1, b1, w);
    x = Fnmsubs(a2, b1, x);
    y = Fmadds(a2, b0, y);
    z = Fmadds(a2, b3, z);
    w = Fnmsubs(a2, b2, w);
    store<f32>(out, x);
    store<f32>(out + 4, y);
    store<f32>(out + 8, z);
    store<f32>(out + 12, w);
    ctx.fpr[1] = static_cast<f64>(y);
}

// void Quat_ToMatrix(f32 out[4][4], const f32 q[4], const f32 pos[3])
// The rotation matrix of the unit quaternion q {x, y, z, w} in rows 0-2
// and pos in row 3. The w lanes (+12 / +28 / +44 / +60) are not written.
// r6 = the constants' address and f1 = out[1][2], as the original leaves them.
DECOMP(00553A90, Quat_ToMatrix) {
    const u32 out = arg<u32>(ctx, 0), q = arg<u32>(ctx, 1), pos = arg<u32>(ctx, 2);
    const f32 y = Lf(q + 4), z = Lf(q + 8), x = Lf(q), w = Lf(q + 12);
    const f32 zz = Fmuls(z, z), yy = Fmuls(y, y);
    const f32 wz = Fmuls(w, z), wy = Fmuls(w, y), wx = Fmuls(w, x);
    const f32 two = Lf(kQmConsts), one = Lf(kQmConsts + 4);
    const f32 yyzz = Fmadds(y, y, zz), xxzz = Fmadds(x, x, zz);
    const f32 xy_wz = Fmsubs(x, y, wz), xz_wy = Fmadds(x, z, wy), xy__wz = Fmadds(x, y, wz);
    const f32 yz_wx = Fmsubs(y, z, wx), xz__wy = Fmsubs(x, z, wy), yz__wx = Fmadds(y, z, wx);
    const f32 xxyy = Fmadds(x, x, yy);
    const f32 m00 = Fnmsubs(yyzz, two, one), m11 = Fnmsubs(xxzz, two, one);
    const f32 m10 = Fmuls(xy_wz, two), m20 = Fmuls(xz_wy, two), m01 = Fmuls(xy__wz, two);
    const f32 m21 = Fmuls(yz_wx, two), m02 = Fmuls(xz__wy, two), m12 = Fmuls(yz__wx, two);
    const f32 m22 = Fnmsubs(xxyy, two, one);
    const u32 p0 = load<u32>(pos), p1 = load<u32>(pos + 4), p2 = load<u32>(pos + 8);
    store<f32>(out + 0, m00);
    store<f32>(out + 20, m11);
    store<u32>(out + 48, p0);
    store<f32>(out + 16, m10);
    store<f32>(out + 32, m20);
    store<f32>(out + 4, m01);
    store<f32>(out + 36, m21);
    store<f32>(out + 8, m02);
    store<f32>(out + 24, m12);
    store<f32>(out + 40, m22);
    store<u32>(out + 52, p1);
    store<u32>(out + 56, p2);
    ctx.gpr[6] = kQmConsts;
    ctx.fpr[1] = static_cast<f64>(m12);
}
