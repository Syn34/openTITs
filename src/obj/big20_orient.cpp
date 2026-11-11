// Game code, large-function sweep part 20: an object's orientation from a
// yaw, an optional carrier matrix and a smoothed up vector.
#include "big20.h"

using namespace sly4;
using namespace sly4::big20;

namespace {
namespace orient {
constexpr u32 kK = 0x00191110u;        // .text f32: 0, 1, 0.5, 0.04, 0.001, 1e-4
constexpr u32 kAxisA = 0x01164B70u;    // f32[3] (Constants_Init_0056B93C): yaw cos axis
constexpr u32 kAxisB = 0x01164B50u;    // f32[3]: yaw sin axis
constexpr u32 kUp = 0x01164B60u;       // f32[3]: the up axis
constexpr u32 Quat_ToMatrix = 0x00553A90;    // (f32 out[4][4], const f32 q[4], const f32 pos[3])
constexpr u32 Quat_FromMatrix = 0x00555478;  // (f32 out[4], const f32 m[4][4])
constexpr u32 Quat_Multiply = 0x005556A4;    // (f32 out[4], const f32 a[4], const f32 b[4])
constexpr u32 WorldObj_MarkDirty = 0x0057D000;
constexpr u32 WorldObj_UpdateLod = 0x0057EE00;  // (obj, unused, bool)

inline f64 K(u32 off) { return Lf(kK + off); }
inline f64 W(u32 w) { return mid13::LfsF(w); }

// Rows 0..3 (3 words each, 16 apart) copied from src to dst.
inline void CopyRows3(u32 dst, u32 src, u32 dstride, u32 sstride, u32 rows) {
    for (u32 r = 0; r < rows; ++r)
        for (u32 c = 0; c < 3; ++c) Cw(dst + dstride * r + 4 * c, src + sstride * r + 4 * c);
}

// The rotation taking the up axis v onto u (both unit, f32 math as
// compiled; nearly opposite: a half turn about an axis normal to v).
void ShortestArc(u32 out, f64 ux, f64 uy, f64 uz, f64 vx, f64 vy, f64 vz, u32 zeroW, f64 one) {
    f64 d = S(S(vy + uy) * vy);
    d = Fmadds(S(vx + ux), vx, d);
    d = Fmadds(S(vz + uz), vz, d);
    if (std::fabs(d) > K(16)) {
        const f64 cx = Fmsubs(vz, ux, S(vx * uz));
        const f64 cy = Fmsubs(vy, uz, S(vz * uy));
        const f64 cz = Fmsubs(vx, uy, S(vy * ux));
        f64 l = S(cx * cx);
        l = Fmadds(cy, cy, l);
        l = Fmadds(cz, cz, l);
        l = Fmadds(d, d, l);
        const f64 inv = S(one / S(std::sqrt(l)));
        store<u32>(out + 0, Fb(S(cy * inv)));
        store<u32>(out + 4, Fb(S(cx * inv)));
        store<u32>(out + 8, Fb(S(cz * inv)));
        store<u32>(out + 12, Fb(S(d * inv)));
        return;
    }
    if (vx > vy) {
        const f64 n = S(std::sqrt(Fmadds(vx, vx, S(vz * vz))));
        if (n < K(20)) {
            store<u32>(out + 0, zeroW);
            store<u32>(out + 4, zeroW);
            store<u32>(out + 8, zeroW);
            store<u32>(out + 12, 0x3F800000u);
            return;
        }
        const f64 inv = S(one / n);
        store<u32>(out + 4, zeroW);
        store<u32>(out + 12, zeroW);
        store<u32>(out + 8, Fb(S(inv * vx)));
        store<u32>(out + 0, Fb(-S(inv * vz)));
    } else {
        const f64 n = S(std::sqrt(Fmadds(vy, vy, S(vz * vz))));
        if (n < K(20)) {
            store<u32>(out + 0, zeroW);
            store<u32>(out + 4, zeroW);
            store<u32>(out + 8, zeroW);
            store<u32>(out + 12, 0x3F800000u);
            return;
        }
        const f64 inv = S(one / n);
        store<u32>(out + 0, zeroW);
        store<u32>(out + 12, zeroW);
        store<u32>(out + 4, Fb(S(inv * vz)));
        store<u32>(out + 8, Fb(-S(inv * vy)));
    }
}
}  // namespace orient
}  // namespace

// void f(Obj* o): with o +664 set, o's matrix (+96, rows of 3) is copied
// into its world object o +160 (+16 on) and WorldObj_MarkDirty. Else the
// matrix is built: rows from the yaw o +488 (cos * kAxisA + sin * kAxisB,
// normalized, as the forward row; up row (0, 1, 0)), position o +388's +288
// vector (lvx) when set; with a carrier o +616 (WorldObj_UpdateLod of o +624
// +256), its matrix (o +624 +80) replaces the rows and the position gets
// the carrier quaternion conjugated by a spin of o +656 / 2 about kUp
// applied to the quaternion o +640 (w 0) added. Without o +624 the smoothed
// up vector o +464 is moved 4% toward kUp, renormalized, and the shortest
// arc from kUp to it pre-multiplies the rotation. The normalized rotation
// becomes the matrix handed to vfunc 84 (o, f32[4][4]).
DECOMP(00191128, Obj_UpdateOrient191128) {
    using namespace orient;
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 664) != 0) {
        const u32 w = load<u32>(o + 160);
        CopyRows3(w + 16, o + 96, 16, 16, 4);
        call(ctx, WorldObj_MarkDirty, {w});
        return;
    }
    Stack frame(ctx, 496);
    const u32 sp = frame.addr();
    const u32 m = sp + 336;
    CopyRows3(m, kIdentity, 16, 16, 4);
    const u32 zeroW = load<u32>(kK);  // f31 / f27: lfs of 0.0 (stored raw)
    u32 w27 = zeroW, w28 = zeroW, w29 = zeroW;
    const f64 c = callf(ctx, b20fn::Math_Cosf, {}, {Lf(o + 488)});
    const f64 a0 = Lf(kAxisA), a1 = Lf(kAxisA + 4), a2 = Lf(kAxisA + 8);
    const f64 s = callf(ctx, b20fn::Math_Sinf, {}, {Lf(o + 488)});
    f64 fx = S(Lf(kAxisB) * s), fy = S(Lf(kAxisB + 4) * s), fz = S(Lf(kAxisB + 8) * s);
    fx = Fmadds(a0, c, fx);
    fy = Fmadds(a1, c, fy);
    fz = Fmadds(a2, c, fz);
    const f64 one = K(4);  // f30
    {
        f64 l = S(fy * fy);
        l = Fmadds(fx, fx, l);
        l = Fmadds(fz, fz, l);
        const f64 inv = S(one / S(std::sqrt(l)));
        fx = S(fx * inv);
        fy = S(fy * inv);
        fz = S(fz * inv);
    }
    // n = rows {fz, 0, -fx}, {0, 1, 0}, {fx, fy, fz}, kZeroVec at sp +272;
    // the same rows at m.
    const u32 n = sp + 272;
    for (u32 b : {n, m}) {
        store<u32>(b + 0, Fb(fz));
        store<u32>(b + 4, 0);
        store<u32>(b + 8, Fb(-fx));
        store<u32>(b + 16, 0);
        store<u32>(b + 20, 0x3F800000u);
        store<u32>(b + 24, 0);
        store<u32>(b + 32, Fb(fx));
        store<u32>(b + 36, Fb(fy));
        store<u32>(b + 40, Fb(fz));
        Cw(b + 48, kZeroVec);
        Cw(b + 52, kZeroVec + 4);
        Cw(b + 56, kZeroVec + 8);
    }
    const u32 posObj = load<u32>(o + 388);
    const u32 carrier = load<u32>(o + 616);
    const u32 lod = load<u32>(o + 624);
    if (posObj != 0) {
        const u32 v = (posObj + 288) & ~15u;  // lvx
        w27 = load<u32>(v);
        w28 = load<u32>(v + 4);
        w29 = load<u32>(v + 8);
    }
    if (carrier != 0) {
        call(ctx, WorldObj_UpdateLod, {load<u32>(lod + 256), 0, 1});
        CopyRows3(m, load<u32>(o + 624) + 80, 16, 16, 4);
        const f64 uy = Lf(kUp + 4), ux = Lf(kUp), uz = Lf(kUp + 8);
        f64 l = S(uy * uy);
        l = Fmadds(ux, ux, l);
        l = Fmadds(uz, uz, l);
        const f64 ln = S(std::sqrt(l));
        const f64 half = S(Lf(o + 656) * K(8));
        const f64 inv = S(one / ln);
        f64 qx = S(ux * inv), qy = S(uy * inv), qz = S(uz * inv);
        const f64 sh = callf(ctx, b20fn::Math_Sinf, {}, {half});
        qx = S(sh * qx);
        qy = S(sh * qy);
        qz = S(sh * qz);
        const f64 ch = callf(ctx, b20fn::Math_Cosf, {}, {half});
        const u32 q1 = sp + 160, q2 = sp + 224, q3 = sp + 208, q4 = sp + 240, q5 = sp + 256;
        store<u32>(q1 + 0, Fb(qx));
        store<u32>(q1 + 4, Fb(qy));
        store<u32>(q1 + 8, Fb(qz));
        store<u32>(q1 + 12, Fb(ch));
        Cw(q2 + 0, o + 640);
        Cw(q2 + 4, o + 644);
        Cw(q2 + 8, o + 648);
        store<u32>(q2 + 12, zeroW);
        call(ctx, Quat_Multiply, {q3, q1, q2});
        store<u32>(q4 + 12, load<u32>(q1 + 12));
        store<u32>(q4 + 0, load<u32>(q1 + 0) ^ 0x80000000u);  // fneg
        store<u32>(q4 + 4, load<u32>(q1 + 4) ^ 0x80000000u);
        store<u32>(q4 + 8, load<u32>(q1 + 8) ^ 0x80000000u);
        call(ctx, Quat_Multiply, {q5, q3, q4});
        w27 = Fb(S(Lf(m + 48) + Lf(q5 + 0)));
        w28 = Fb(S(Lf(m + 52) + Lf(q5 + 4)));
        w29 = Fb(S(Lf(m + 56) + Lf(q5 + 8)));
    }
    store<u32>(m + 48, w27);
    store<u32>(m + 52, w28);
    store<u32>(m + 56, w29);
    const u32 q = sp + 112;
    call(ctx, Quat_FromMatrix, {q, m});
    f64 x = Lf(q), y = Lf(q + 4), z = Lf(q + 8), w = Lf(q + 12);
    if (load<u32>(o + 624) == 0) {
        // Smoothed up vector o +464 moved 4% toward kUp, renormalized.
        const f64 cy = Lf(kUp + 4), cx = Lf(kUp), cz = Lf(kUp + 8);
        const f64 oy = Lf(o + 468), ox = Lf(o + 464), oz = Lf(o + 472);
        const f64 k = K(12);
        const f64 ny0 = Fmadds(S(cy - oy), k, oy);
        const f64 nx0 = Fmadds(S(cx - ox), k, ox);
        const f64 nz0 = Fmadds(S(cz - oz), k, oz);
        f64 l = S(ny0 * ny0);
        l = Fmadds(nx0, nx0, l);
        l = Fmadds(nz0, nz0, l);
        const f64 inv = S(one / S(std::sqrt(l)));
        f64 ux = S(nx0 * inv), uy = S(ny0 * inv), uz = S(nz0 * inv);
        store<u32>(o + 464, Fb(ux));
        store<u32>(o + 468, Fb(uy));
        store<u32>(o + 472, Fb(uz));
        const f64 vy0 = Lf(kUp + 4), vx0 = Lf(kUp), vz0 = Lf(kUp + 8);
        f64 lu = S(uy * uy);
        f64 lv = S(vy0 * vy0);
        lu = Fmadds(ux, ux, lu);
        lv = Fmadds(vx0, vx0, lv);
        lu = Fmadds(uz, uz, lu);
        lv = Fmadds(vz0, vz0, lv);
        const f64 iu = S(one / S(std::sqrt(lu)));
        const f64 iv = S(one / S(std::sqrt(lv)));
        uy = S(iu * uy);
        ux = S(iu * ux);
        uz = S(iu * uz);
        const f64 vy = S(iv * vy0), vx = S(iv * vx0), vz = S(iv * vz0);
        const u32 arc = sp + 176;
        ShortestArc(arc, ux, uy, uz, vx, vy, vz, zeroW, one);
        call(ctx, Quat_Multiply, {sp + 192, arc, q});
        for (u32 k4 = 0; k4 < 16; k4 += 4) Cw(q + k4, sp + 192 + k4);
        x = Lf(sp + 192);
        y = Lf(sp + 196);
        z = Lf(sp + 200);
        w = Lf(sp + 204);
    }
    {
        f64 l = S(y * y);
        l = Fmadds(x, x, l);
        l = Fmadds(z, z, l);
        l = Fmadds(w, w, l);
        const f64 inv = S(one / S(std::sqrt(l)));
        store<u32>(sp + 128, Fb(S(x * inv)));
        store<u32>(sp + 132, Fb(S(y * inv)));
        store<u32>(sp + 136, Fb(S(z * inv)));
        store<u32>(sp + 140, Fb(S(w * inv)));
    }
    call(ctx, Quat_ToMatrix, {n, sp + 128, kZeroVec});
    CopyRows3(m, n, 16, 16, 3);
    store<u32>(m + 48, w27);
    store<u32>(m + 52, w28);
    store<u32>(m + 56, w29);
    VCall(ctx, o, 84, {o, m});
}
