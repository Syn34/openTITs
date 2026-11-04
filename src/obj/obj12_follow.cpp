// World objects, part 12: the spline follower's advance (Follower_Advance):
// the new parameter, the position on the spline and, for followers that
// orient themselves, the rotation turned onto the tangent and back upright.
#include "obj12.h"

using namespace sly4;
using namespace sly4::obj8;
using namespace sly4::obj9;
using namespace sly4::obj10;
using namespace sly4::obj12;

namespace {

// The follower as Follower_Advance sees it (the pointer it is passed):
// +0 f32[4][4] the matrix (+16 row 1 = up, +32 row 2 = forward), +64 f32
// the phase, +68 f32 the phase offset, +72 / +76 f32 the speed and its
// scale, +92 / +96 a word copied when the phase wraps, +100 the spline
// record (96 bytes: +0 its world matrix, +80 SplineData*), +108 u8 bit
// 0x80 = orient along the tangent, +109 u8 the segment hint. (obj11.h
// reads +100 as the active word and +104 as the record (name?).)
constexpr u32 kFaPhase = 64, kFaOffset = 68, kFaSpeed = 72, kFaScale = 76;
constexpr u32 kFaWrapSrc = 92, kFaWrapDst = 96, kFaRecord = 100, kFaBits = 108, kFaSegHint = 109;
constexpr u32 kFaOrient = 0x80;
constexpr u32 kFaConsts = 0x0057A760;       // f32 {1, 1e-4, 0.001}
constexpr u32 kFaOne = 0, kFaTiny = 4, kFaParallel = 8;
constexpr u32 kSpline_PointTangent = 0x005606C8;  // (SplineData*, -, f32 pos[4], f32 tan[4], u8 hint, 0; f1 t) -> u8 hint (name?)
constexpr u32 kQuat_FromMatrix = 0x00555478;
constexpr u32 kOneBits = 0x3F800000;

inline f32 Frac(f32 x) { return Fsubs(x, static_cast<f32>(rt::fcfid(rt::fctid(x, true)))); }

inline f32 Len3(f32 x, f32 y, f32 z) { return Fsqrts(Fmadds(z, z, Fmadds(x, x, Fmuls(y, y)))); }

// rec rows 0-2 times v's x / y / z (+ row 3 with translate), the original's
// vmaddfp chain.
Vec4 Transform(u32 rec, u32 v, bool translate) {
    const Vec4 x = VSplat(VLoadLeft(v), 0), y = VSplat(VLoadLeft(v + 4), 0), z = VSplat(VLoadLeft(v + 8), 0);
    Vec4 a = VMadd(VLoad(rec), x, VZero());
    a = VMadd(VLoad(rec + 16), y, a);
    a = VMadd(VLoad(rec + 32), z, a);
    return translate ? VAdd(VLoad(rec + 48), a) : a;
}

// The quaternion turning unit f onto unit d into q: from the cross product
// and (f + d).f, or, when that is within 0.001 of 0 (opposite vectors), a
// half turn about an axis perpendicular to f (identity if f is too close to
// that axis).
void QuatBetween(u32 q, f32 fx, f32 fy, f32 fz, f32 dx, f32 dy, f32 dz) {
    const f32 one = Lf(kFaConsts + kFaOne), tiny = Lf(kFaConsts + kFaTiny);
    const f32 hy = Fadds(fy, dy), hx = Fadds(fx, dx), hz = Fadds(fz, dz);
    const f32 dot = Fmadds(hz, fz, Fmadds(hx, fx, Fmuls(hy, fy)));
    auto identity = [&] {
        store<u32>(q, 0);
        store<u32>(q + 4, 0);
        store<u32>(q + 8, 0);
        store<u32>(q + 12, kOneBits);
    };
    if (std::fabs(dot) > Lf(kFaConsts + kFaParallel)) {
        const f32 cy = Fmsubs(fz, dx, Fmuls(fx, dz));
        const f32 cx = Fmsubs(fy, dz, Fmuls(fz, dy));
        const f32 cz = Fmsubs(fx, dy, Fmuls(fy, dx));
        const f32 inv = Fdivs(one, Fsqrts(Fmadds(dot, dot, Fmadds(cz, cz, Fmadds(cx, cx, Fmuls(cy, cy))))));
        store<f32>(q, Fmuls(cx, inv));
        store<f32>(q + 4, Fmuls(cy, inv));
        store<f32>(q + 8, Fmuls(cz, inv));
        store<f32>(q + 12, Fmuls(dot, inv));
    } else if (fx > fy) {
        const f32 l = Fsqrts(Fmadds(fx, fx, Fmuls(fz, fz)));
        if (l < tiny) return identity();
        const f32 inv = Fdivs(one, l);
        store<u32>(q + 4, 0);
        store<u32>(q + 12, 0);
        store<f32>(q + 8, Fmuls(inv, fx));
        store<f32>(q, -Fmuls(inv, fz));
    } else {
        const f32 l = Fsqrts(Fmadds(fy, fy, Fmuls(fz, fz)));
        if (l < tiny) return identity();
        const f32 inv = Fdivs(one, l);
        store<u32>(q, 0);
        store<u32>(q + 12, 0);
        store<f32>(q + 4, Fmuls(inv, fz));
        store<f32>(q + 8, -Fmuls(inv, fy));
    }
}

// dst = src / |src| (x, y, z, w).
void QuatNormalize(u32 dst, u32 src) {
    const f32 y = Lf(src + 4), x = Lf(src), z = Lf(src + 8), w = Lf(src + 12);
    const f32 inv = Fdivs(Lf(kFaConsts + kFaOne), Fsqrts(Fmadds(w, w, Fmadds(z, z, Fmadds(x, x, Fmuls(y, y))))));
    store<f32>(dst, Fmuls(x, inv));
    store<f32>(dst + 4, Fmuls(y, inv));
    store<f32>(dst + 8, Fmuls(z, inv));
    store<f32>(dst + 12, Fmuls(w, inv));
}

// The x / y / z lanes of the four rows of m into the follower.
void CopyRows(u32 f, u32 m) {
    for (u32 off : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u, 48u, 52u, 56u}) store<u32>(f + off, load<u32>(m + off));
}

}  // namespace

// void Follower_Advance(Follower* f, -, f32* t, f32 out[4]; f32 dt)
// phase += speed * scale * dt (wrapped to [0, 1), copying +92 to +96 when it
// passes 1); *t = frac(phase + offset); out = the spline point at *t in the
// record's world space (stored aligned, w lane included). With +108 bit
// 0x80 the segment hint is updated and, when the world tangent is at least
// 1e-4 long, the rotation is turned so the forward row (+32) follows it and
// then so the up row (+16) points along 0x01164B60, each as
// normalise(turn * Quat_FromMatrix(f)) rebuilt through Quat_ToMatrix (the
// position row too, from the origin) (name?).
DECOMP(0057A770, Follower_Advance) {
    const u32 f = arg<u32>(ctx, 0), tp = arg<u32>(ctx, 2), out = arg<u32>(ctx, 3);
    const f64 dt = farg(ctx, 0);
    const f32 one = Lf(kFaConsts + kFaOne);
    const u32 rec = load<u32>(f + kFaRecord);
    const f32 off = Lf(f + kFaOffset);
    const u32 spline = load<u32>(rec + obj11::kSrSpline);
    const f32 t = Fmadds(Fmuls(Lf(f + kFaScale), Lf(f + kFaSpeed)), dt, Lf(f + kFaPhase));
    if (t > one) store<u32>(f + kFaWrapDst, load<u32>(f + kFaWrapSrc));
    const f32 ph = Frac(t);
    store<f32>(f + kFaPhase, ph);
    const f32 t2 = Frac(Fadds(ph, off));
    store<f32>(tp, t2);
    if ((load<u8>(f + kFaBits) & kFaOrient) == 0) {
        callf(ctx, obj11::o11fn::Spline_PointAt, {spline, 0, out, 0}, {t2});  // r4 = 0 (the bit test)
        VStore(out, Transform(rec, out, true));
        return;
    }
    Stack frame(ctx, 384);
    const u32 sp = frame.addr();
    const u32 tan = sp + 112;
    callf(ctx, kSpline_PointTangent, {spline, 1, out, tan, load<u8>(f + kFaSegHint), 0}, {t2});
    store<u8>(f + kFaSegHint, static_cast<u8>(ctx.gpr[3]));
    VStore(out, Transform(rec, out, true));
    VStore(tan, Transform(rec, tan, false));
    const f32 ty = Lf(tan + 4), tx = Lf(tan), tz = Lf(tan + 8);
    const f32 len = Len3(tx, ty, tz);
    if (!(len >= Lf(kFaConsts + kFaTiny))) return;
    const f32 il = Fdivs(one, len);
    const f32 dx = Fmuls(tx, il), dy = Fmuls(ty, il), dz = Fmuls(tz, il);
    store<f32>(tan, dx);
    store<f32>(tan + 4, dy);
    store<f32>(tan + 8, dz);

    // Turn the forward row onto the tangent.
    {
        const f32 fy = Lf(f + 36), fx = Lf(f + 32), fz = Lf(f + 40);
        const f32 id = Fdivs(one, Len3(dx, dy, dz)), iff = Fdivs(one, Len3(fx, fy, fz));
        QuatBetween(sp + 144, Fmuls(iff, fx), Fmuls(iff, fy), Fmuls(iff, fz), Fmuls(id, dx), Fmuls(id, dy),
                    Fmuls(id, dz));
        call(ctx, kQuat_FromMatrix, {sp + 160, f});
        call(ctx, o10fn::Quat_Multiply, {sp + 128, sp + 144, sp + 160});
        QuatNormalize(sp + 144, sp + 128);
        call(ctx, o10fn::Quat_ToMatrix, {sp + 208, sp + 144, kOrigin});
        CopyRows(f, sp + 208);
    }
    // Turn the up row onto the up axis.
    {
        const u32 up = obj11::kUpAxis;
        const f32 ry = Lf(f + 20), uy = Lf(up + 4), rx = Lf(f + 16), ux = Lf(up), rz = Lf(f + 24), uz = Lf(up + 8);
        const f32 ir = Fdivs(one, Len3(rx, ry, rz)), iu = Fdivs(one, Len3(ux, uy, uz));
        QuatBetween(sp + 192, Fmuls(ir, rx), Fmuls(ir, ry), Fmuls(ir, rz), Fmuls(iu, ux), Fmuls(iu, uy), Fmuls(iu, uz));
        call(ctx, kQuat_FromMatrix, {sp + 176, f});
        for (u32 k = 0; k < 4; ++k) store<u32>(sp + 160 + 4 * k, load<u32>(sp + 176 + 4 * k));
        call(ctx, o10fn::Quat_Multiply, {sp + 176, sp + 192, sp + 160});
        QuatNormalize(sp + 144, sp + 176);
        call(ctx, o10fn::Quat_ToMatrix, {sp + 272, sp + 144, kOrigin});
        CopyRows(f, sp + 272);
    }
}
