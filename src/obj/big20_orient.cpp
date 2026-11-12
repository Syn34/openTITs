// Game code, large-function sweep part 20: an object's orientation from a
// yaw, an optional carrier matrix and a smoothed up vector.
#include "big20.h"

namespace sly4::obj {

namespace {
constexpr u32 kK = 0x00191110u;        // .text f32: 0, 1, 0.5, 0.04, 0.001, 1e-4
constexpr u32 kAxisA = 0x01164B70u;    // f32[3] (Constants_Init_0056B93C): yaw cos axis
constexpr u32 kAxisB = 0x01164B50u;    // f32[3]: yaw sin axis
constexpr u32 kUp = 0x01164B60u;       // f32[3]: the up axis
constexpr u32 kIdentity = 0x01164BD0u; // f32[4][4] identity
constexpr u32 Quat_ToMatrix = 0x00553A90;    // (f32 out[4][4], const f32 q[4], const f32 pos[3])
constexpr u32 Quat_FromMatrix = 0x00555478;  // (f32 out[4], const f32 m[4][4])
constexpr u32 Quat_Multiply = 0x005556A4;    // (f32 out[4], const f32 a[4], const f32 b[4])
constexpr u32 WorldObj_MarkDirty = 0x0057D000;
constexpr u32 WorldObj_UpdateLod = 0x0057EE00;  // (obj, unused, bool)
constexpr u32 Math_Cosf = 0x007344E0;
constexpr u32 Math_Sinf = 0x007345F0;

struct OrientObj {
    pad<96> _0;
    Mtx44 m;            // +96
    be_u32 world;       // +160 (+16 its rows)
    pad<224> _164;
    be_u32 posobj;      // +388 (+288 a vector)
    pad<72> _392;
    Vec3 up;            // +464
    pad<12> _476;
    be_f32 yaw;         // +488
    pad<124> _492;
    be_u32 carrier;     // +616
    pad<4> _620;
    be_u32 lod;         // +624 (+80 its matrix, +256 its world object)
    pad<12> _628;
    Vec3 q640;          // +640
    pad<4> _652;
    be_f32 spin;        // +656
    pad<4> _660;
    be_u32 direct;      // +664
};
static_assert(offsetof(OrientObj, world) == 160 && offsetof(OrientObj, posobj) == 388 && offsetof(OrientObj, up) == 464);
static_assert(offsetof(OrientObj, yaw) == 488 && offsetof(OrientObj, carrier) == 616 && offsetof(OrientObj, q640) == 640);
static_assert(offsetof(OrientObj, spin) == 656 && offsetof(OrientObj, direct) == 664);
// The original's locals (sp offsets - 112).
struct OrientLocals {
    Vec4 q;             // sp+112
    Vec4 qn;            // sp+128
    Vec4 _144;
    Vec4 q1;            // sp+160
    Vec4 arc;           // sp+176
    Vec4 prod;          // sp+192
    Vec4 q3;            // sp+208
    Vec4 q2;            // sp+224
    Vec4 q4;            // sp+240
    Vec4 q5;            // sp+256
    Mtx44 n;            // sp+272
    Mtx44 m;            // sp+336
};
static_assert(offsetof(OrientLocals, n) == 160 && offsetof(OrientLocals, m) == 224);

f64 K(u32 off) { return lf(global<be_f32>(kK + off)); }
void SetW(be_f32& d, u32 w) { set_bits(d, w); }
void SetF(be_f32& d, f64 x) { d = static_cast<f32>(x); }

// The rotation taking the up axis v onto u (both unit, f32 math as
// compiled; nearly opposite: a half turn about an axis normal to v).
void ShortestArc(Vec4& out, f64 ux, f64 uy, f64 uz, f64 vx, f64 vy, f64 vz, u32 zeroW, f64 one) {
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
        SetF(out.x, S(cy * inv));
        SetF(out.y, S(cx * inv));
        SetF(out.z, S(cz * inv));
        SetF(out.w, S(d * inv));
        return;
    }
    const bool xbig = vx > vy;
    const f64 a = xbig ? vx : vy;
    const f64 n = S(std::sqrt(Fmadds(a, a, S(vz * vz))));
    if (n < K(20)) {
        SetW(out.x, zeroW);
        SetW(out.y, zeroW);
        SetW(out.z, zeroW);
        SetW(out.w, 0x3F800000u);
        return;
    }
    const f64 inv = S(one / n);
    if (xbig) {
        SetW(out.y, zeroW);
        SetW(out.w, zeroW);
        SetF(out.z, S(inv * vx));
        SetF(out.x, -S(inv * vz));
    } else {
        SetW(out.x, zeroW);
        SetW(out.w, zeroW);
        SetF(out.y, S(inv * vz));
        SetF(out.z, -S(inv * vy));
    }
}
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
// becomes the matrix handed to vfunc 84 (o, f32[4][4]). No caller reads r3.
void Obj_UpdateOrient191128(OrientObj* o) {
    if (o->direct != 0u) {
        const u32 w = o->world;
        CopyMtxXYZ(gm<Mtx44>(w + 16), o->m);
        call(WorldObj_MarkDirty, w);
        return;
    }
    GuestFrame frame(496);
    OrientLocals& L = *frame.as<OrientLocals>(112);
    Mtx44& m = L.m;
    CopyMtxXYZ(m, global<Mtx44>(kIdentity));
    const u32 zeroW = global<be_u32>(kK);  // f31 / f27: lfs of 0.0 (stored raw)
    u32 w27 = zeroW, w28 = zeroW, w29 = zeroW;
    const f64 c = call<f64>(Math_Cosf, lf(o->yaw));
    const Vec3& A = global<Vec3>(kAxisA);
    const Vec3& B = global<Vec3>(kAxisB);
    const f64 a0 = lf(A.x), a1 = lf(A.y), a2 = lf(A.z);
    const f64 s = call<f64>(Math_Sinf, lf(o->yaw));
    f64 fx = S(lf(B.x) * s), fy = S(lf(B.y) * s), fz = S(lf(B.z) * s);
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
    // n = rows {fz, 0, -fx}, {0, 1, 0}, {fx, fy, fz}, kZeroVec; the same rows at m.
    for (Mtx44* b : {&L.n, &m}) {
        SetF(b->r[0].x, fz);
        SetW(b->r[0].y, 0u);
        SetF(b->r[0].z, -fx);
        SetW(b->r[1].x, 0u);
        SetW(b->r[1].y, 0x3F800000u);
        SetW(b->r[1].z, 0u);
        SetF(b->r[2].x, fx);
        SetF(b->r[2].y, fy);
        SetF(b->r[2].z, fz);
        CopyXYZ(b->r[3], global<Vec3>(kZeroVec));
    }
    const u32 posObj = o->posobj;
    const u32 carrier = o->carrier;
    const u32 lod = o->lod;
    if (posObj != 0) {
        const Quad& v = gm<Quad>((posObj + 288) & ~15u);  // lvx
        w27 = v.w[0];
        w28 = v.w[1];
        w29 = v.w[2];
    }
    if (carrier != 0) {
        call(WorldObj_UpdateLod, gm<be_u32>(lod + 256).get(), 0u, 1u);
        CopyMtxXYZ(m, gm<Mtx44>(o->lod + 80));
        const Vec3& up = global<Vec3>(kUp);
        const f64 uy = lf(up.y), ux = lf(up.x), uz = lf(up.z);
        f64 l = S(uy * uy);
        l = Fmadds(ux, ux, l);
        l = Fmadds(uz, uz, l);
        const f64 ln = S(std::sqrt(l));
        const f64 half = S(lf(o->spin) * K(8));
        const f64 inv = S(one / ln);
        f64 qx = S(ux * inv), qy = S(uy * inv), qz = S(uz * inv);
        const f64 sh = call<f64>(Math_Sinf, half);
        qx = S(sh * qx);
        qy = S(sh * qy);
        qz = S(sh * qz);
        const f64 ch = call<f64>(Math_Cosf, half);
        SetF(L.q1.x, qx);
        SetF(L.q1.y, qy);
        SetF(L.q1.z, qz);
        SetF(L.q1.w, ch);
        CopyXYZ(L.q2, o->q640);
        SetW(L.q2.w, zeroW);
        call(Quat_Multiply, &L.q3, &L.q1, &L.q2);
        SetW(L.q4.w, bits(L.q1.w));
        SetW(L.q4.x, bits(L.q1.x) ^ 0x80000000u);  // fneg
        SetW(L.q4.y, bits(L.q1.y) ^ 0x80000000u);
        SetW(L.q4.z, bits(L.q1.z) ^ 0x80000000u);
        call(Quat_Multiply, &L.q5, &L.q3, &L.q4);
        w27 = std::bit_cast<u32>(static_cast<f32>(S(lf(m.r[3].x) + lf(L.q5.x))));
        w28 = std::bit_cast<u32>(static_cast<f32>(S(lf(m.r[3].y) + lf(L.q5.y))));
        w29 = std::bit_cast<u32>(static_cast<f32>(S(lf(m.r[3].z) + lf(L.q5.z))));
    }
    SetW(m.r[3].x, w27);
    SetW(m.r[3].y, w28);
    SetW(m.r[3].z, w29);
    call(Quat_FromMatrix, &L.q, &m);
    f64 x = lf(L.q.x), y = lf(L.q.y), z = lf(L.q.z), w = lf(L.q.w);
    if (o->lod == 0u) {
        // Smoothed up vector o +464 moved 4% toward kUp, renormalized.
        const Vec3& up = global<Vec3>(kUp);
        const f64 cy = lf(up.y), cx = lf(up.x), cz = lf(up.z);
        const f64 oy = lf(o->up.y), ox = lf(o->up.x), oz = lf(o->up.z);
        const f64 k = K(12);
        const f64 ny0 = Fmadds(S(cy - oy), k, oy);
        const f64 nx0 = Fmadds(S(cx - ox), k, ox);
        const f64 nz0 = Fmadds(S(cz - oz), k, oz);
        f64 l = S(ny0 * ny0);
        l = Fmadds(nx0, nx0, l);
        l = Fmadds(nz0, nz0, l);
        const f64 inv = S(one / S(std::sqrt(l)));
        f64 ux = S(nx0 * inv), uy = S(ny0 * inv), uz = S(nz0 * inv);
        SetF(o->up.x, ux);
        SetF(o->up.y, uy);
        SetF(o->up.z, uz);
        const f64 vy0 = lf(up.y), vx0 = lf(up.x), vz0 = lf(up.z);
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
        ShortestArc(L.arc, ux, uy, uz, vx, vy, vz, zeroW, one);
        call(Quat_Multiply, &L.prod, &L.arc, &L.q);
        L.q = L.prod;
        x = lf(L.prod.x);
        y = lf(L.prod.y);
        z = lf(L.prod.z);
        w = lf(L.prod.w);
    }
    {
        f64 l = S(y * y);
        l = Fmadds(x, x, l);
        l = Fmadds(z, z, l);
        l = Fmadds(w, w, l);
        const f64 inv = S(one / S(std::sqrt(l)));
        SetF(L.qn.x, S(x * inv));
        SetF(L.qn.y, S(y * inv));
        SetF(L.qn.z, S(z * inv));
        SetF(L.qn.w, S(w * inv));
    }
    call(Quat_ToMatrix, &L.n, &L.qn, kZeroVec);
    for (u32 r = 0; r < 3; ++r) CopyXYZ(m.r[r], L.n.r[r]);
    SetW(m.r[3].x, w27);
    SetW(m.r[3].y, w28);
    SetW(m.r[3].z, w29);
    vcall(o, 84, &m);
}

}  // namespace sly4::obj

NATIVE(00191128, sly4::obj::Obj_UpdateOrient191128)
