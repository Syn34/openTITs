// Game code, large-function sweep part 32: move an object's matrix to a
// target point with a Havok sweep, then turn its forward axis toward the
// distance actually moved.
#include "../sly4.h"
#include "big32.h"

using namespace sly4;
using namespace sly4::big32;

namespace {
constexpr u32 kSweepK = 0x000EB400;  // .text: f32 1e-5, 1.0, 1000.0; f64 1000.0 at +16, f64 1.0 at +24
constexpr u32 kTurnDiv = 0x01021CD0;  // f32: the turn divisor (clamped to [1, 1000])
constexpr u32 kZero3 = 0x01164B40;    // f32[3] the zero vector
constexpr u32 kAxisX = 0x01164B50;    // f32[3] the fallback axes
constexpr u32 kAxisY = 0x01164B60;
constexpr u32 kAxisZ = 0x01164B70;
constexpr u32 Hk_SweepToPoint_004A41C0 = 0x004A41C0;  // bool (T* o, const f32 m[4][4], const Vec3* p, f32 out[4][4])
constexpr u32 Matrix_NormalizeInPlace56D374 = 0x0056D374;  // (Mat34*)

// Each of (x, y, z) within eps of kZero3 (fsubs, fabs, fcmpu lt; the zero
// vector re-read each time).
bool NearZero(f64 x, f64 y, f64 z, f64 eps) {
    return std::fabs(Subs(x, Lfs(kZero3))) < eps && std::fabs(Subs(y, Lfs(kZero3 + 4))) < eps &&
           std::fabs(Subs(z, Lfs(kZero3 + 8))) < eps;
}
}  // namespace

// u32 f(Obj* o, const f32 m[4][4], const Vec3* p, f32 out[4][4], u8 turn)
// (called by f_000F45E0 and f_000F5518). out = m (x / y / z of the four
// rows) with translation p. With the Havok holder +388 (and its +8) set:
// Hk_SweepToPoint_004A41C0(+388, m, p, out); on a hit with flag 0x400 of
// (+216)->+372, o->vfn[352](o, opd, 0, 0, entry; f1 = o +344). Then, with
// flag 0x400000 and turn set and the translation moved (out.t - m.t not
// within 1e-5 of m.t nor of zero): f = normalize(m.row2 + normalize(d) /
// clamp(kTurnDiv, 1, 1000)) (out.row2 when that is ~zero), row0 = out.row1
// x f (or, when ~zero, the least-aligned of kAxisX / Y / Z x f), row1 =
// f x row0; the matrix at sp + 112 (translation out.t) goes through
// Matrix_NormalizeInPlace and back to out. r3 / f1 are left as the
// original leaves them on each return.
DECOMP(000EB420, Obj_SweepTurnToMove_000EB420) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1), p = arg<u32>(ctx, 2), out = arg<u32>(ctx, 3);
    const u32 turn = arg<u32>(ctx, 4) & 0xFF;
    for (u32 r = 0; r < 64; r += 16) Copy3W(out + r, m + r);
    Copy3W(out + 48, p);
    ctx.gpr[3] = p;
    ctx.fpr[1] = Lfs(p + 8);  // the last lfs of the copies
    const u32 hk = load<u32>(o + 388);
    const u32 def = load<u32>(o + 216);
    if (hk == 0 || load<u32>(hk + 8) == 0) return;
    if (static_cast<u32>(call(ctx, Hk_SweepToPoint_004A41C0, {hk, m, p, out})) != 0 && (load<u32>(def + 372) & 0x400u) != 0) {
        ctx.fpr[1] = Lfs(o + 344);
        const u32 opd = load<u32>(load<u32>(o) + 352);
        call_ptr(ctx, opd, {o, opd, 0, 0, load<u32>(opd)});  // OPD in r4, entry in r7 (as the original loads them)
    }
    if ((load<u32>(def + 372) & 0x400000u) == 0 || turn == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const f64 eps = Lfs(kSweepK);
    const u32 mt = m + 48;
    const f64 ox = Lfs(out + 48), mx = Lfs(mt);
    if (std::fabs(Subs(mx, ox)) < eps && std::fabs(Subs(Lfs(mt + 4), Lfs(out + 52))) < eps &&
        std::fabs(Subs(Lfs(mt + 8), Lfs(out + 56))) < eps) {
        ctx.gpr[3] = mt;
        ctx.fpr[1] = mx;
        return;
    }
    const f64 dx = Subs(ox, mx), dy = Subs(Lfs(out + 52), Lfs(mt + 4)), dz = Subs(Lfs(out + 56), Lfs(mt + 8));
    if (NearZero(dx, dy, dz, eps)) {
        ctx.gpr[3] = kZero3;
        ctx.fpr[1] = dy;
        return;
    }
    const f64 len = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy))));
    const f64 g = Lfs(kTurnDiv), one = Lfs(kSweepK + 4);
    f64 div = S(Fsel31(Subs(Lfs(kSweepK + 8), g), g, Lfd31(kSweepK + 16)));
    div = S(Fsel31(Subs(div, one), div, Lfd31(kSweepK + 24)));
    const f64 idiv = Divs(one, div), il = Divs(one, len);
    f64 fy = Fmadds(Muls(dy, il), idiv, Lfs(m + 36));
    f64 fx = Fmadds(Muls(dx, il), idiv, Lfs(m + 32));
    f64 fz = Fmadds(Muls(dz, il), idiv, Lfs(m + 40));
    Stack frame(ctx, 64);
    const u32 sp = frame.addr() - 112;  // the original's frame (offsets below are its sp offsets)
    for (u32 k = 0; k < 12; k += 4) {
        store<u32>(sp + 112 + k, load<u32>(out + k));
        store<u32>(sp + 128 + k, load<u32>(out + 16 + k));
        store<u32>(sp + 160 + k, load<u32>(out + 48 + k));
    }
    const f64 r1x = Lfs(out + 16), r1y = Lfs(out + 20), r1z = Lfs(out + 24);
    const f64 il2 = Divs(one, Sqrts(Fmadds(fz, fz, Fmadds(fx, fx, Muls(fy, fy)))));
    fx = Muls(fx, il2);
    fy = Muls(fy, il2);
    fz = Muls(fz, il2);
    store<u32>(sp + 144, Fb(fx));
    store<u32>(sp + 148, Fb(fy));
    store<u32>(sp + 152, Fb(fz));
    if (NearZero(fx, fy, fz, eps)) {
        Copy3W(sp + 144, out + 32);
        fx = Lfs(sp + 144);
        fy = Lfs(sp + 148);
        fz = Lfs(sp + 152);
    }
    f64 c0 = Fmsubs(r1y, fz, Muls(r1z, fy)), c1 = Fmsubs(r1z, fx, Muls(r1x, fz)), c2 = Fmsubs(r1x, fy, Muls(r1y, fx));
    if (NearZero(c0, c1, c2, eps)) {
        u32 ax = kAxisX;
        const f64 az = std::fabs(fz), ay = std::fabs(fy);
        if (az < ay && az < std::fabs(fx)) ax = kAxisZ;
        else if (ay < std::fabs(fx)) ax = kAxisY;
        const f64 vx = Lfs(ax), vy = Lfs(ax + 4), vz = Lfs(ax + 8);
        c0 = Fmsubs(vy, fz, Muls(vz, fy));
        c1 = Fmsubs(vz, fx, Muls(vx, fz));
        c2 = Fmsubs(vx, fy, Muls(vy, fx));
    }
    store<u32>(sp + 112, Fb(c0));
    store<u32>(sp + 116, Fb(c1));
    store<u32>(sp + 120, Fb(c2));
    store<u32>(sp + 128, Fb(Fmsubs(fy, c2, Muls(fz, c1))));
    store<u32>(sp + 132, Fb(Fmsubs(fz, c0, Muls(fx, c2))));
    store<u32>(sp + 136, Fb(Fmsubs(fx, c1, Muls(fy, c0))));
    call(ctx, Matrix_NormalizeInPlace56D374, {sp + 112});
    for (u32 r = 0; r < 64; r += 16) Copy3W(out + r, sp + 112 + r);
    ctx.fpr[1] = Lfs(sp + 168);
}
