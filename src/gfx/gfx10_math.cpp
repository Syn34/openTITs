// The renderer range's float and VMX leaves of ~60 to ~300 instructions:
// render target matrices, the 64-byte clones and a point integrator.
// Names carry the address where the role is unclear.
#include <bit>
#include <cmath>

#include "opentit/recomp.h"

#include "fx.h"
#include "gfx10.h"

using namespace sly4;

namespace {

inline f32 lf10(u32 a) { return load<f32>(a); }
inline f32 mul10(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) * b); }               // fmuls
inline f32 sub10(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) - b); }               // fsubs
inline f32 madd10(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), c, b)); }    // fmadds a*c+b
inline f32 nmsub10(f32 a, f32 c, f32 b) { return static_cast<f32>(-std::fma(static_cast<f64>(a), c, -static_cast<f64>(b))); }  // fnmsubs b-a*c

// The 64-byte clone (vtable 00B231B0 / 00B231D0 slot 3): p =
// SmallAlloc(64, 0); a new p gets +4 u16 1, +8..+32 cleared, vtable vt
// and +48..+56 = the vec3 at 0x01164B40 scaled by the f32 at k; then
// self's +8, +12, +20, +24, +28, u8 +16, +32, +48..+56 are copied in
// (lfs / stfs pairs: raw words), to address 0 when the allocation failed
// (the original's null writes). Returns p; f1 the last word copied.
void clone64(PPUContext& ctx, u32 vt, u32 k) {
    const u32 self = arg<u32>(ctx, 0);
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu /* SmallAlloc */, {64, 0}));
    if (p != 0) {
        store<u16>(p + 4, 1);
        store<u32>(p + 8, 0);
        store<u32>(p + 12, 0);
        store<u8>(p + 16, 0);
        for (u32 off = 20; off <= 32; off += 4) store<u32>(p + off, 0);
        store<u32>(p, vt);
        constexpr u32 kVec = 0x01164B40;
        const f32 s = lf10(k);
        const f32 x = mul10(lf10(kVec), s), y = mul10(lf10(kVec + 4), s), z = mul10(lf10(kVec + 8), s);
        store<f32>(p + 48, x);
        store<f32>(p + 52, y);
        store<f32>(p + 56, z);
        ctx.gpr[6] = 0x01160000u;  // left (lis)
    }
    for (u32 off : {8u, 12u, 20u, 24u, 28u}) store<u32>(p + off, load<u32>(self + off));
    store<u8>(p + 16, load<u8>(self + 16));
    for (u32 off : {32u, 48u, 52u, 56u}) store<u32>(p + off, load<u32>(self + off));
    ctx.fpr[1] = static_cast<f64>(lf10(self + 56));
    ctx.gpr[4] = self;
    ctx.gpr[5] = p;
    ret(ctx, p);
}

// A render target matrix pair: the rows of m (3 floats each, w not
// written) to this+dst..; the inverse of the rigid matrix m (transposed
// rotation, -t rotated) to this+inv.. with VMX: columns {m0.x, m1.x,
// m2.x, m1.y}, {m0.y, m1.y, m2.y, m0.x}, {m0.z, m1.z, m2.z, m0.z} and
// their sum scaled by -t (fused per lane, 0 - t); u8 +52 bit 0x80 set.
void set_matrix_pair10(PPUContext& ctx, u32 dst, u32 inv) {
    const u32 self = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 3; ++k) store<u32>(self + dst + 16 * row + 4 * k, load<u32>(m + 16 * row + 4 * k));
    const u32 a = m & ~15u;  // lvx
    f32 r[4][4];
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 4; ++k) r[row][k] = lf10(a + 16 * row + 4 * k);
    const f32 c0[4] = {r[0][0], r[1][0], r[2][0], r[1][1]};
    const f32 c1[4] = {r[0][1], r[1][1], r[2][1], r[0][0]};
    const f32 c2[4] = {r[0][2], r[1][2], r[2][2], r[0][2]};
    const f32 t0 = 0.0f - r[3][0], t1 = 0.0f - r[3][1], t2 = 0.0f - r[3][2];
    const u32 o = (self + inv) & ~15u;  // stvx
    for (u32 k = 0; k < 4; ++k) {
        store<f32>(o + 4 * k, c0[k]);
        store<f32>(o + 16 + 4 * k, c1[k]);
        store<f32>(o + 32 + 4 * k, c2[k]);
    }
    for (u32 k = 0; k < 4; ++k) {
        f32 v = std::fmaf(c0[k], t0, 0.0f);
        v = std::fmaf(c1[k], t1, v);
        store<f32>(o + 48 + 4 * k, std::fmaf(c2[k], t2, v));
    }
    store<u8>(self + 52, static_cast<u8>(load<u8>(self + 52) | 0x80));
    ctx.gpr[4] = load<u8>(self + 52);  // left as the original leaves them
    ctx.gpr[5] = self + inv + 32;
    ctx.gpr[6] = self + inv + 48;
    ctx.fpr[1] = static_cast<f64>(lf10(m + 56));  // the last lfs
}

}  // namespace

// Obj* Clone_0079F850(this) [vtable 00B231B0[3]]: clone64 (scale 0.5 at 0x0079F848)
DECOMP(0079F850, Clone_0079F850) { clone64(ctx, 0x00B231B0u, 0x0079F848u); }

// Obj* Clone_0079FE68(this) [vtable 00B231D0[3]]: clone64 (scale 0.5 at 0x0079FE60)
DECOMP(0079FE68, Clone_0079FE68) { clone64(ctx, 0x00B231D0u, 0x0079FE60u); }

// void RenderTarget_SetMatrix(RenderTarget* rt, const f32 m[16]): the
// camera matrix to +752.., its inverse (the view matrix) to +816..
DECOMP(005201B0, RenderTarget_SetMatrix) { set_matrix_pair10(ctx, 752, 816); }

// void (RenderTarget* rt, const f32 m[16]): the view matrix to +816..,
// its inverse (the camera matrix) to +752.. (name?)  [called by f_00056ED0]
DECOMP(00522630, RenderTarget_SetViewMatrix_00522630) { set_matrix_pair10(ctx, 816, 752); }

// f32 (Point* a, Particle* p, const f32 c[3], const f32 n[3]; f1 s): one
// integration step of p (+0 position, +12 time, +16 velocity) against
// a's time (+8) and a->+4->+120->+16 acceleration q: dt = a+8 - p+12,
// v = p+16 + q dt, v += n (n.v) K0 (K at 0x00505C24: -2.0, 0.5, 1/60),
// v *= s; p+16 = v - q dt; p+0 = (c - (dt (v - q dt) + q dt^2 K1)) +
// v K2. Returns f1 = the new z  [called by f_00505E90] (name?)
DECOMP(00505C30, Particle_Integrate_00505C30) {
    const u32 a = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2), n = arg<u32>(ctx, 3);
    const f64 s = farg(ctx, 0);  // used as the double in f1
    constexpr u32 kK = 0x00505C24;
    const f32 dt = sub10(lf10(a + 8), lf10(p + 12));
    const u32 q = load<u32>(load<u32>(a + 4) + 120) + 16;
    const f32 q0 = lf10(q), q1 = lf10(q + 4), q2 = lf10(q + 8);
    f32 vy = madd10(dt, q1, lf10(p + 20));
    f32 vx = madd10(dt, q0, lf10(p + 16));
    const f32 vz0 = madd10(dt, q2, lf10(p + 24));
    const f32 n0 = lf10(n), n1 = lf10(n + 4), n2 = lf10(n + 8);
    const f32 dt2 = mul10(dt, dt);
    f32 d = mul10(n1, vy);
    d = madd10(n0, vx, d);
    d = madd10(n2, vz0, d);
    d = mul10(d, lf10(kK));
    vx = static_cast<f32>(s * madd10(n0, d, vx));
    vy = static_cast<f32>(s * madd10(n1, d, vy));
    const f32 vz = static_cast<f32>(s * madd10(n2, d, vz0));
    const f32 wx = nmsub10(dt, q0, vx), wy = nmsub10(dt, q1, vy), wz = nmsub10(dt, q2, vz);
    store<f32>(p + 16, wx);
    store<f32>(p + 20, wy);
    store<f32>(p + 24, wz);
    const f32 h = mul10(dt2, lf10(kK + 4));
    const f32 c0 = lf10(c), c1 = lf10(c + 4), c2 = lf10(c + 8);
    const f32 k2 = lf10(kK + 8);
    const f32 ex = sub10(c0, madd10(dt, wx, mul10(h, q0)));
    const f32 ey = sub10(c1, madd10(dt, wy, mul10(h, q1)));
    const f32 ez = sub10(c2, madd10(dt, wz, mul10(h, q2)));
    const f32 x = madd10(vx, k2, ex), y = madd10(vy, k2, ey), z = madd10(vz, k2, ez);
    store<f32>(p, x);
    store<f32>(p + 4, y);
    store<f32>(p + 8, z);
    ctx.fpr[1] = static_cast<f64>(z);
    ctx.gpr[3] = q;  // r3 as left: the acceleration vector
}

namespace {
// lfs: a single's bits as the double the register holds (NaNs kept as they are).
inline f64 lfs_bits10(u32 w) {
    if ((w & 0x7F800000u) == 0x7F800000u && (w & 0x007FFFFFu) != 0)
        return std::bit_cast<f64>((static_cast<u64>(w >> 31) << 63) | (u64{0x7FF} << 52) | (static_cast<u64>(w & 0x007FFFFFu) << 29));
    return static_cast<f64>(std::bit_cast<f32>(w));
}
inline u32 bswap10(u32 w) { return (w >> 24) | ((w >> 8) & 0xFF00u) | ((w << 8) & 0xFF0000u) | (w << 24); }
}  // namespace

// void* Floats_ByteSwap_00525174(u32 count, u32* p, u32 comps, u32 stride):
// the first comps (1..4; any value counts as at least 1) words of each of
// count elements byte-reversed in place (through the stack, lfs / stfs).
// Returns the pointer past the last element; f1 the last word.  [called
// by f_0052528C] (name?)
DECOMP(00525174, Floats_ByteSwap_00525174) {
    u32 n = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    const u32 comps = arg<u32>(ctx, 2), stride = arg<u32>(ctx, 3);
    while (n != 0) {
        for (u32 k = 0; k < 4; ++k) {
            if (k != 0 && comps < k + 1) break;  // cmplwi
            const u32 w = bswap10(load<u32>(p + 4 * k));
            store<u32>(p + 4 * k, w);
            ctx.fpr[1] = lfs_bits10(w);
        }
        p += stride;
        --n;
    }
    ret(ctx, p);
}

// void (): the 8 unit directions at 0x010DFA40 (16 bytes each: -sin a,
// 0, cos a for a = i * 45 degrees), from the parabola sine approximation
// (constants at 0x005271C0: 45, pi/180, pi, -pi, 2pi, 4/pi, -4/pi^2,
// 0.225, .., pi/2, pi): a wrapped to [-pi, pi), s = 4/pi a - 4/pi^2 a|a|,
// refined by 0.225 (s|s| - s)  [called by f_00527484] (name?)
DECOMP(005271F8, Directions_Init_005271F8) {
    constexpr u32 kK = 0x005271C0, kOut = 0x010DFA40;
    const f32 k0 = lf10(kK), k1 = lf10(kK + 4);
    const f64 d8 = load<f64>(kK + 8), d16 = load<f64>(kK + 16);
    const f32 k24 = lf10(kK + 24), k28 = lf10(kK + 28), k32 = lf10(kK + 32), k36 = lf10(kK + 36);
    const f32 k40 = lf10(kK + 40), k44 = lf10(kK + 44), k48 = lf10(kK + 48);
    auto approx = [&](f32 x) {
        f32 y = madd10(x, k28, mul10(mul10(x, k32), std::fabs(x)));
        const f32 e = static_cast<f32>(std::fma(static_cast<f64>(y), std::fabs(y), -static_cast<f64>(y)));  // fmsubs
        y = madd10(e, k36, y);
        return mul10(y, k40);
    };
    for (u32 i = 0; i < 8; ++i) {
        f32 a = mul10(mul10(static_cast<f32>(i), k0), k1);
        const f32 off = static_cast<f32>(a >= 0.0f ? d8 : d16);  // fsel + frsp
        a = static_cast<f32>(static_cast<f64>(off) + a);
        const f32 q = static_cast<f32>(std::trunc(static_cast<f32>(static_cast<f64>(a) / k24)));  // fdivs, fctidz, fcfid, frsp
        a = nmsub10(q, k24, a);
        const f32 x = sub10(a, off);
        const f32 sn = approx(x);
        f32 c = static_cast<f32>(static_cast<f64>(x) + k44);
        if (c > k48) c = sub10(c, k24);
        const u32 e = kOut + 16 * i;
        store<u32>(e + 4, 0);
        store<f32>(e, -sn);
        store<f32>(e + 8, approx(c));
    }
    ctx.gpr[3] = 0;  // r3 as left
    ctx.fpr[1] = static_cast<f64>(k48);
}

// void Copy_007A18F4(Obj* dst, const Obj* src) [called through a
// pointer]: the assignment body: u8 +4, the three 24-byte blocks at +20,
// bytes +92..+95, then +96, +100, +108..+128, +104, +144 / +132 .. +160
// (raw words, in the original's store order). f1 the last word.
DECOMP(007A18F4, Copy_007A18F4) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    store<u8>(d + 4, load<u8>(s + 4));
    for (u32 k = 0; k < 72; k += 4) store<u32>(d + 20 + k, load<u32>(s + 20 + k));
    for (u32 k = 0; k < 4; ++k) store<u8>(d + 92 + k, load<u8>(s + 92 + k));
    for (u32 off : {96u, 100u, 108u, 112u, 116u, 120u, 124u, 104u, 128u, 144u, 132u, 148u, 152u, 160u})
        store<u32>(d + off, load<u32>(s + off));
    ctx.fpr[1] = lfs_bits10(load<u32>(s + 160));
    ctx.gpr[5] = load<u32>(s + 132);  // left as the original leaves it
}

namespace {
// r = a * b with VMX, as Matrix_MulAffine_0079DE98: row i = b0 a[i].x +
// b1 a[i].y + b2 a[i].z (+ b3 for row 3), fused per lane in vmaddfp
// order; a's w column unused. Addresses rounded down to 16 (lvx).
void mul_affine10(u32 a, u32 b, f32 r[4][4]) {
    a &= ~15u;
    b &= ~15u;
    for (u32 i = 0; i < 4; ++i) {
        const f32 x = lf10(a + 16 * i), y = lf10(a + 16 * i + 4), z = lf10(a + 16 * i + 8);
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fmaf(lf10(b + 4 * k), x, i == 3 ? lf10(b + 48 + 4 * k) : 0.0f);
            v = std::fmaf(lf10(b + 16 + 4 * k), y, v);
            r[i][k] = std::fmaf(lf10(b + 32 + 4 * k), z, v);
        }
    }
}

// The two record-stream matrix steps (00515784 / 005158B4, the same
// body): (x, const f32 a[16], const f32 b[16]): a * b transposed into a
// stack block, then cellGcmSetVertexProgramConstants(context, 0, 16, it).
void set_mvp10(PPUContext& ctx) {
    f32 r[4][4];
    mul_affine10(arg<u32>(ctx, 1), arg<u32>(ctx, 2), r);
    Stack frame(ctx, 64);
    const u32 t = frame.addr();
    for (u32 k = 0; k < 4; ++k)
        for (u32 i = 0; i < 4; ++i) store<f32>(t + 16 * k + 4 * i, r[i][k]);
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(0x010F10A0u), 0, 16, t});
}
}  // namespace

// void (x, const f32 a[16], const f32 b[16]): set_mvp10  [called by f_005159E8] (name?)
DECOMP(00515784, VpConst_SetMvp_00515784) { set_mvp10(ctx); }

// void (x, const f32 a[16], const f32 b[16]): set_mvp10 (a second copy)  [called by f_005159E8] (name?)
DECOMP(005158B4, VpConst_SetMvp_005158B4) { set_mvp10(ctx); }

// void Particle_PositionAt_00505D50(Point* a, const f32 p[8], f32 out[3];
// f1 t, f2 s): out = p's position (+0) + the offset at time t: along the
// spline a->+4->+136 (Spline_PointAt(spline + 16, a->+4->+120, offset,
// 0; f1 s * t), the offset preset to the vec3 at 0x01164B40) or, without
// one, ballistic: dt = max(t - q->+32, 0) (fsel), offset = p+16 (the
// velocity) dt + q+16.. dt^2 / 2 (q = a->+4->+120; 0.0 / 0.5 at
// 0x00505D40). f1 = out.y  [called by f_00505E90] (name?)
DECOMP(00505D50, Particle_PositionAt_00505D50) {
    const u32 a = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    const f64 t = farg(ctx, 0), s = farg(ctx, 1);
    const f32 px = lf10(p), py = lf10(p + 4), pz = lf10(p + 8);
    const f32 vx = lf10(p + 16), vy = lf10(p + 20), vz = lf10(p + 24);
    const u32 h = load<u32>(a + 4);
    const u32 spline = load<u32>(h + 136), q = load<u32>(h + 120);
    Stack frame(ctx, 16);
    const u32 b = frame.addr();
    for (u32 k = 0; k < 12; k += 4) store<u32>(b + k, load<u32>(0x01164B40u + k));
    f32 ox, oy, oz;
    if (spline != 0) {
        callf(ctx, 0x005602BCu /* Spline_PointAt */, {spline + 16, q, b, 0}, {static_cast<f64>(static_cast<f32>(s * t))});
        ox = lf10(b);
        oy = lf10(b + 4);
        oz = lf10(b + 8);
    } else {
        constexpr u32 kK = 0x00505D40;
        const f32 d = static_cast<f32>(t - lf10(q + 32));  // fsubs
        const f32 dt = static_cast<f32>(d >= 0.0f ? static_cast<f64>(d) : load<f64>(kK));  // fsel + frsp
        const f32 h2 = mul10(mul10(dt, dt), lf10(kK + 8));
        const f32 ax = mul10(lf10(q + 16), h2);
        const f32 ay = mul10(lf10(q + 20), h2);
        ox = madd10(vx, dt, ax);
        oy = madd10(vy, dt, ay);
        oz = madd10(lf10(q + 24), h2, mul10(vz, dt));
        ctx.gpr[3] = kK;  // r3 as left (the spline path leaves Spline_PointAt's)
    }
    const f32 x = static_cast<f32>(static_cast<f64>(px) + ox), y = static_cast<f32>(static_cast<f64>(py) + oy),
              z = static_cast<f32>(static_cast<f64>(pz) + oz);
    store<f32>(out, x);
    store<f32>(out + 4, y);
    store<f32>(out + 8, z);
    ctx.fpr[1] = static_cast<f64>(y);
}

// u32 Lines_AddCircle_00518A20(LineBuf* o, Batch* out, const f32 c[3]; f1
// r): 8 36-byte line records at o's cursor (+4; +324 = the start) round
// the circle of radius r about c * o->+316 in the plane of the 8 unit
// directions at 0x010DFA40: {0, 2.0f, 0, p_i, p_(i+1)%8}, p = dir r + c s
// (fused). The cursor is left at the start; u16 out->+4 = the record
// count (8). Returns it  [called by f_00518E78] (name?)
DECOMP(00518A20, Lines_AddCircle_00518A20) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2);
    const f64 r = farg(ctx, 0);
    constexpr u32 kDirs = 0x010DFA40;
    const u32 start = load<u32>(o + 4);
    const f32 s = lf10(o + 316);
    const f32 cx = mul10(lf10(c), s), cy = mul10(lf10(c + 4), s), cz = mul10(lf10(c + 8), s);
    store<u32>(o + 324, start);
    auto pt = [&](u32 d, u32 k, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(lf10(d + 4 * k)), r, b)); };
    u32 e = start;
    for (u32 i = 0; i < 8; ++i) {
        const u32 di = kDirs + 16 * i, dj = kDirs + 16 * ((i + 1) % 8);
        const f32 a0 = pt(di, 0, cx), a1 = pt(di, 1, cy), a2 = pt(di, 2, cz);
        const f32 b0 = pt(dj, 0, cx), b1 = pt(dj, 1, cy), b2 = pt(dj, 2, cz);
        store<u32>(e, 0);
        store<u32>(e + 4, 0x40000000u);  // 2.0f
        store<u32>(e + 8, 0);
        store<f32>(e + 12, a0);
        store<f32>(e + 16, a1);
        store<f32>(e + 20, a2);
        store<f32>(e + 24, b0);
        store<f32>(e + 28, b1);
        store<f32>(e + 32, b2);
        e += 36;
        store<u32>(o + 4, e);
    }
    store<u32>(o + 4, start);
    const s32 n = static_cast<s32>(e - start) / 12 / 3;  // mulhw magic, truncating
    store<u16>(out + 4, static_cast<u16>(n));
    ret(ctx, n);
}

// void (Proj* o, const Obj* x, const Cam* cam, y): unless bit 0x80 of u8
// x->+96: o+12 / +16 = o+4 / +8 * cam+212 + cam+208 (fused) and
// f_0052B688(o, x + 32, s32 x->+24, cam, y); then o+352 = cam+64 *
// (o+48 * 0x010F0A90) (Matrix_MultiplyTo twice, through the stack),
// transposed in place (VMX)  [called by f_0052BC9C] (name?)
DECOMP(0052BB60, Proj_Update_0052BB60) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), cam = arg<u32>(ctx, 2), y = arg<u32>(ctx, 3);
    if ((load<u8>(x + 96) & 0x80) == 0) {
        store<f32>(o + 12, madd10(lf10(o + 4), lf10(cam + 212), lf10(cam + 208)));
        const f32 b = lf10(o + 8);
        const f32 v = madd10(b, lf10(cam + 212), lf10(cam + 208));
        store<f32>(o + 16, v);
        ctx.fpr[1] = static_cast<f64>(v);  // left in f1 / f2 (not read by the callee)
        ctx.fpr[2] = static_cast<f64>(b);
        call(ctx, 0x0052B688u, {o, x + 32, static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(x + 24)))), cam, y});
    }
    Stack frame(ctx, 64);
    call(ctx, 0x0055499Cu /* Matrix_MultiplyTo */, {frame.addr(), o + 48, 0x010F0A90u});
    call(ctx, 0x0055499Cu /* Matrix_MultiplyTo */, {o + 352, cam + 64, frame.addr()});
    const u32 m = (o + 352) & ~15u;  // lvx / stvx
    u32 w[4][4];
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 4; ++k) w[i][k] = load<u32>(m + 16 * i + 4 * k);
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 4; ++k) store<u32>(m + 16 * i + 4 * k, w[k][i]);
    ctx.gpr[3] = o + 352;  // left as the original leaves them
    ctx.gpr[4] = o + 368;
    ctx.gpr[5] = o + 400;
    ctx.gpr[6] = o + 384;
}
