// World objects, part 8: the small vector / box / sphere / spline helpers
// WorldObj_UpdateLod and World_AddObjects call.
#include "obj8.h"

using namespace sly4;
using namespace sly4::obj8;

namespace {

constexpr u32 kHalf = 0x005577D8;           // f32 0.5 (Sphere_FromBox)
constexpr u32 kNoParam = 0x00560A38;        // f32 0.0 (Spline_Nearest with fewer than two points)
constexpr u32 kSpline_NearestSeg = 0x0055F008;  // (data, const f32 p[3], 0, u32 last, u32* seg, f32* dist) -> f1 (name?)
constexpr u32 kSpline_Eval = 0x00560058;    // (data, f1 t, f32 out[3], data +12, bool) (name?)

inline f32 F(u32 bits) { return std::bit_cast<f32>(bits); }
inline f32 Lf(u32 ea) { return load<f32>(ea); }
inline f32 Adds(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) + static_cast<f64>(b)); }
inline f32 Subs(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) - static_cast<f64>(b)); }
inline f32 Muls(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) * static_cast<f64>(b)); }
inline f32 Madds(f32 a, f32 c, f32 b) {
    return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(c), static_cast<f64>(b)));
}
inline f32 Sqrts(f32 a) { return static_cast<f32>(std::sqrt(static_cast<f64>(a))); }
// fsel(d >= 0 ? x : y) on raw words (the picked value is stored unchanged).
inline u32 Sel(f32 d, u32 x, u32 y) { return d >= 0.0f ? x : y; }

// sqrt(b^2 + a^2 + c^2) in the original's order (b*b, then a and c fused in).
inline f32 Length3(f32 a, f32 b, f32 c) { return Sqrts(Madds(c, c, Madds(a, a, Muls(b, b)))); }

}  // namespace

// void Vec3_Add(f32 dst[3], const f32 src[3])
DECOMP(00557A20, Vec3_Add) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    const f32 x = Adds(Lf(d), Lf(s)), y = Adds(Lf(d + 4), Lf(s + 4)), z = Adds(Lf(d + 8), Lf(s + 8));
    store<f32>(d, x);
    store<f32>(d + 4, y);
    store<f32>(d + 8, z);
}

// void Box_AddPoint(f32 box[8], const f32 p[3])
// Grows the box {min[4], max[4]} to take in p (fsel, so a NaN difference
// keeps the old bound for min and takes p for max).
DECOMP(00556FE8, Box_AddPoint) {
    const u32 b = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    for (u32 k = 0; k < 12; k += 4) {
        const u32 lo = load<u32>(b + k), v = load<u32>(p + k);
        store<u32>(b + k, Sel(Subs(F(lo), F(v)), v, lo));
    }
    for (u32 k = 0; k < 12; k += 4) {
        const u32 hi = load<u32>(b + 16 + k), v = load<u32>(p + k);
        store<u32>(b + 16 + k, Sel(Subs(F(hi), F(v)), hi, v));
    }
}

// void Sphere_FromBox(f32 sphere[4], const f32 box[8])
// The centre (min + (max - min) * 0.5) and the distance from it to max.
DECOMP(005577E0, Sphere_FromBox) {
    const u32 s = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const f32 h = Lf(kHalf);
    const f32 cy = Madds(Subs(Lf(b + 20), Lf(b + 4)), h, Lf(b + 4));
    const f32 cx = Madds(Subs(Lf(b + 16), Lf(b)), h, Lf(b));
    const f32 cz = Madds(Subs(Lf(b + 24), Lf(b + 8)), h, Lf(b + 8));
    store<f32>(s + 4, cy);
    store<f32>(s, cx);
    store<f32>(s + 8, cz);
    const u32 mx = b + 16;
    const f32 dy = Subs(Lf(mx + 4), Lf(s + 4)), dx = Subs(Lf(mx), Lf(s)), dz = Subs(Lf(mx + 8), Lf(s + 8));
    store<f32>(s + 12, Length3(dx, dy, dz));
}

// void Sphere_Transform(f32 sphere[4], const Matrix* m, bool scale_radius)
// The centre through m (rows 0-2 and the translation row); with
// scale_radius the radius times the longest of rows 0-2.
DECOMP(00557900, Sphere_Transform) {
    const u32 s = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    const u32 scale = arg<u32>(ctx, 2) & 0xFF;
    Stack frame(ctx, 64);
    const u32 sp = frame.addr();
    for (u32 k = 0; k < 12; k += 4) store<u32>(sp + 48 + k, load<u32>(s + k));
    Vec4 a = VMadd(VLoad(m), VSplat(VLoadLeft(s), 0), VZero());
    a = VMadd(VLoad(m + 16), VSplat(VLoadLeft(s + 4), 0), a);
    a = VMadd(VLoad(m + 32), VSplat(VLoadLeft(s + 8), 0), a);
    a = VAdd(VLoad(m + 48), a);
    VStore(sp + 48, a);
    for (u32 k = 0; k < 12; k += 4) store<u32>(s + k, load<u32>(sp + 48 + k));
    if (scale == 0) return;
    const f32 l0 = Length3(Lf(m), Lf(m + 4), Lf(m + 8));
    const f32 l1 = Length3(Lf(m + 16), Lf(m + 20), Lf(m + 24));
    const f32 l2 = Length3(Lf(m + 32), Lf(m + 36), Lf(m + 40));
    const f32 r = Lf(s + 12);
    u32 big = Sel(Subs(l0, l1), std::bit_cast<u32>(l0), std::bit_cast<u32>(l1));
    big = Sel(Subs(l2, F(big)), std::bit_cast<u32>(l2), big);
    store<f32>(s + 12, Muls(r, F(big)));
}

// Obj* WorldObj_UpdateSphere(Obj* o)  -> &o->sphere (+240)
// The world bounding sphere from the class's (+224): the centre scaled by
// the largest scale component (+144) and moved by the world matrix, the
// radius (+252) scaled. Goes through the original's stack words.
DECOMP(0057E14C, WorldObj_UpdateSphere) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 80);
    const u32 sp = frame.addr();
    const u32 sx = load<u32>(o + obj::kObjScale), sy = load<u32>(o + obj::kObjScale + 4), sz = load<u32>(o + obj::kObjScale + 8);
    u32 s = Sel(Subs(F(sx), F(sy)), sx, sy);
    s = Sel(Subs(F(sz), F(s)), sz, s);
    const f32 k = F(s);
    const u32 c = o + obj4::kObjSphere;
    store<f32>(o + 252, Muls(Lf(c + 12), k));
    const f32 x = Muls(Lf(c), k);
    for (u32 off : {64u, 48u, 68u}) store<f32>(sp + off, x);
    const f32 y = Muls(Lf(c + 4), k);
    store<f32>(sp + 72, x);
    store<f32>(sp + 76, x);
    store<f32>(sp + 52, y);
    const Vec4 vx = VLoad(sp + 64);
    const f32 z = Muls(Lf(c + 8), k);
    for (u32 off : {64u, 68u, 72u, 76u}) store<f32>(sp + off, y);
    store<f32>(sp + 56, z);
    const u32 w = o + obj6::kObjWorld;
    Vec4 a = VMadd(VLoad(w), vx, VZero());
    const Vec4 vy = VLoad(sp + 64);
    for (u32 off : {64u, 68u, 72u, 76u}) store<f32>(sp + off, z);
    a = VMadd(VLoad(w + 16), vy, a);
    a = VMadd(VLoad(w + 32), VLoad(sp + 64), a);
    a = VAdd(VLoad(w + 48), a);
    VStore(sp + 48, a);
    for (u32 k4 = 0; k4 < 12; k4 += 4) store<u32>(o + 240 + k4, load<u32>(sp + 48 + k4));
    ret(ctx, o + 240);
}

// f32 Spline_PointAt(SplineData* d, f32 t, f32 out[3], bool flag)
// 0x00560058(d, t, out, d +12, flag).
DECOMP(005602BC, Spline_PointAt) {
    callf(ctx, kSpline_Eval, {ctx.gpr[3], ctx.gpr[4], ctx.gpr[5], load<u32>(arg<u32>(ctx, 0) + 12), ctx.gpr[6] & 0xFF},
          {farg(ctx, 0)});
}

// f32 Spline_Nearest(SplineData* d, const f32 p[3], u32* seg, f32* dist)
// The parameter of the spline point nearest p (0x0055F008 over the
// segments); with fewer than two points 0, seg 0 and the distance to the
// first point.
DECOMP(00560A40, Spline_Nearest) {
    const u32 d = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), seg = arg<u32>(ctx, 2), dist = arg<u32>(ctx, 3);
    const u32 n = load<u32>(d) & 0x7FFF;
    if (static_cast<s32>(n) > 1) {
        callf(ctx, kSpline_NearestSeg, {d, p, 0, n - 1, seg, dist}, {});
        return;
    }
    if (seg != 0) store<u32>(seg, 0);
    if (dist != 0) {
        const u32 pt = load<u32>(d + 4);
        const f32 dy = Subs(Lf(pt + 4), Lf(p + 4)), dx = Subs(Lf(pt), Lf(p)), dz = Subs(Lf(pt + 8), Lf(p + 8));
        store<f32>(dist, Length3(dx, dy, dz));
    }
    ret(ctx, static_cast<f64>(Lf(kNoParam)));
    ctx.gpr[3] = kNoParam;  // r3 = the constant's address, as in the original
}
