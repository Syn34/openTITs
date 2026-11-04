// World objects, part 13: the spline follower's distance-matched advance
// (Follower_AdvanceAlt, followers with +108 bit 0x40): the new parameter is
// searched so the step along the spline covers speed * dt in world space;
// followers that orient themselves then turn onto the direction 0.05
// ahead and back upright.
#include "obj13.h"

using namespace sly4;
using namespace sly4::obj8;
using namespace sly4::obj9;
using namespace sly4::obj10;
using namespace sly4::obj13;

namespace {

inline f32 Ac(u32 off) { return Lf(kAaConsts + off); }

inline f32 FracAa(f32 x) { return Fsubs(x, static_cast<f32>(rt::fcfid(rt::fctid(x, true)))); }

inline f32 Len3Aa(f32 x, f32 y, f32 z) { return Fsqrts(Fmadds(z, z, Fmadds(x, x, Fmuls(y, y)))); }

// rec rows 0-2 times v's x / y / z plus row 3: the original's vmaddfp chain.
Vec4 ToWorld(u32 rec, u32 v) {
    const Vec4 x = VSplat(VLoadLeft(v), 0), y = VSplat(VLoadLeft(v + 4), 0), z = VSplat(VLoadLeft(v + 8), 0);
    Vec4 a = VMadd(VLoad(rec), x, VZero());
    a = VMadd(VLoad(rec + 16), y, a);
    a = VMadd(VLoad(rec + 32), z, a);
    return VAdd(VLoad(rec + 48), a);
}

// Spline_PointAt(spline, -, out, 0; t), then out to the record's world space
// (stored aligned, w lane included). r4 is whatever the code before left
// (Spline_PointAt doesn't read it).
void PointWorld(PPUContext& ctx, u32 spline, u32 rec, u32 out, f32 t) {
    callf(ctx, obj11::o11fn::Spline_PointAt, {spline, 0, out, 0}, {t});
    VStore(out, ToWorld(rec, out));
}

// |a - b| over x / y / z.
f32 Dist(u32 a, u32 b) {
    const f32 dy = Fsubs(Lf(a + 4), Lf(b + 4)), dx = Fsubs(Lf(a), Lf(b)), dz = Fsubs(Lf(a + 8), Lf(b + 8));
    return Len3Aa(dx, dy, dz);
}

// The quaternion turning unit f onto unit d into q (Follower_Advance's
// QuatBetween with this function's constants): from the cross product and
// (f + d).f, or, when that is within 0.001 of 0, a half turn about an axis
// perpendicular to f (identity if f is within 1e-4 of that axis).
void TurnBetween(u32 q, f32 fx, f32 fy, f32 fz, f32 dx, f32 dy, f32 dz) {
    const f32 one = Ac(kAaOne);
    const f32 hy = Fadds(fy, dy), hx = Fadds(fx, dx), hz = Fadds(fz, dz);
    const f32 dot = Fmadds(hz, fz, Fmadds(hx, fx, Fmuls(hy, fy)));
    auto identity = [&] {
        store<u32>(q, 0);
        store<u32>(q + 4, 0);
        store<u32>(q + 8, 0);
        store<u32>(q + 12, 0x3F800000u);
    };
    if (std::fabs(dot) > Ac(kAaParallel)) {
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
        if (l < Ac(kAaTiny)) return identity();
        const f32 inv = Fdivs(one, l);
        store<u32>(q + 4, 0);
        store<u32>(q + 12, 0);
        store<f32>(q + 8, Fmuls(inv, fx));
        store<f32>(q, -Fmuls(inv, fz));
    } else {
        const f32 l = Fsqrts(Fmadds(fy, fy, Fmuls(fz, fz)));
        if (l < Ac(kAaTiny)) return identity();
        const f32 inv = Fdivs(one, l);
        store<u32>(q, 0);
        store<u32>(q + 12, 0);
        store<f32>(q + 4, Fmuls(inv, fz));
        store<f32>(q + 8, -Fmuls(inv, fy));
    }
}

// dst = src / |src| (x, y, z, w).
void QuatUnit(u32 dst, u32 src) {
    const f32 y = Lf(src + 4), x = Lf(src), z = Lf(src + 8), w = Lf(src + 12);
    const f32 inv = Fdivs(Ac(kAaOne), Fsqrts(Fmadds(w, w, Fmadds(z, z, Fmadds(x, x, Fmuls(y, y))))));
    store<f32>(dst, Fmuls(x, inv));
    store<f32>(dst + 4, Fmuls(y, inv));
    store<f32>(dst + 8, Fmuls(z, inv));
    store<f32>(dst + 12, Fmuls(w, inv));
}

// The x / y / z lanes of the four rows of m into the follower (lfs / stfs pairs).
void CopyRowsAa(u32 f, u32 m) {
    for (u32 off : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u, 48u, 52u, 56u}) store<u32>(f + off, load<u32>(m + off));
}

}  // namespace

// void Follower_AdvanceAlt(Follower* f, -, f32* t, f32 out[4]; f32 dt)
// The parameter is moved so the world-space distance from the current
// point (phase +64) is speed * scale * dt: starting from a step of dt / +88
// it tries phase + step, then (when that was no closer) phase + 1 - step
// (the step back, kept only when it stays above the old phase), halving the
// step after every miss, until the step is at most dt / (+88 * 32). The
// result is wrapped to [0, 1) into +64 (copying +92 to +96 when it passes
// 1), out = its point in the record's world space, *t = the new phase.
// With +108 bit 0x80 the direction to the point 0.05 ahead (moved on by up
// to ten 0.05 steps while it is under 0.001 away) turns the forward row
// onto it and then the up row onto 0x01164B60, as Follower_Advance does
// (Quat_FromMatrix / Quat_Multiply / Quat_ToMatrix from the origin).
DECOMP(0057AF20, Follower_AdvanceAlt) {
    const u32 f = arg<u32>(ctx, 0), tp = arg<u32>(ctx, 2), out = arg<u32>(ctx, 3);
    const f64 dt = farg(ctx, 0);
    Stack frame(ctx, 448);
    const u32 sp = frame.addr();
    const u32 prev = sp + 112, ahead = sp + 128;

    const u32 rec = load<u32>(f + kFoRecordA);
    const f32 speed = Fmuls(Lf(f + kFoScale), Lf(f + kFoSpeed));
    const u32 spline = load<u32>(rec + obj11::kSrSpline);
    PointWorld(ctx, spline, rec, prev, Lf(f + kFoPhase));

    // The search (f28 / f27 / f29 in the original).
    f32 step = Fdivs(dt, Lf(f + kFoLength));
    f32 best = Lf(f + kFoPhase);
    f32 best_err = Ac(kAaBig);
    auto more = [&] { return step > Fdivs(dt, Fmuls(Lf(f + kFoLength), Ac(kAaSteps))); };
    if (more()) {
        const f32 one = Ac(kAaOne), half = Ac(kAaHalf);
        do {
            const f32 fwd = Fadds(best, step);
            PointWorld(ctx, spline, rec, out, FracAa(fwd));
            f32 err = std::fabs(Fnmsubs(speed, dt, Dist(out, prev)));
            if (err < best_err) {
                best = fwd;
                best_err = err;
                continue;
            }
            PointWorld(ctx, spline, rec, out, FracAa(Fsubs(Fadds(best, one), step)));
            err = std::fabs(Fnmsubs(speed, dt, Dist(out, prev)));
            const f32 back = Fsubs(best, step);
            if (err < best_err && back > Lf(f + kFoPhase)) {
                best = back;
                best_err = err;
            } else {
                step = Fmuls(step, half);
            }
        } while (more());
    }

    const f32 ph = Lf(f + kFoPhase);
    const f32 t = Fadds(ph, Fsubs(best, ph));
    if (t > Ac(kAaOne)) store<u32>(f + kFoWrapDst, load<u32>(f + kFoWrapSrc));
    const f32 nt = FracAa(t);
    store<f32>(f + kFoPhase, nt);
    PointWorld(ctx, spline, rec, out, nt);

    if ((load<u8>(f + obj11::kFoBits) & kFoOrient) == 0) {
        ctx.gpr[3] = 0;  // the bit test, as the original returns
    } else {
        for (u32 k = 0; k < 12; k += 4) store<u32>(prev + k, load<u32>(out + k));
        const f32 look = Ac(kAaLook), eps = Ac(kAaParallel), one = Ac(kAaOne);
        f32 ta = FracAa(Fadds(Lf(f + kFoPhase), look));
        PointWorld(ctx, spline, rec, ahead, ta);
        f32 dx = Fsubs(Lf(ahead), Lf(prev)), dy = Fsubs(Lf(ahead + 4), Lf(prev + 4)), dz = Fsubs(Lf(ahead + 8), Lf(prev + 8));
        for (u32 i = 0; i < kAaLookTries; ++i) {
            if (Len3Aa(dx, dy, dz) < eps) {
                ta = FracAa(Fadds(ta, look));
                PointWorld(ctx, spline, rec, ahead, ta);
                dx = Fsubs(Lf(ahead), Lf(prev));
                dy = Fsubs(Lf(ahead + 4), Lf(prev + 4));
                dz = Fsubs(Lf(ahead + 8), Lf(prev + 8));
            }
        }
        const f32 len = Len3Aa(dx, dy, dz);
        if (!(len >= eps)) {
            ctx.gpr[3] = 128;  // the lvlx offset the original leaves
        } else {
            // Turn the forward row onto the direction.
            {
                const f32 il = Fdivs(one, len);
                const f32 fy = Lf(f + kFoForward + 4), fx = Lf(f + kFoForward), fz = Lf(f + kFoForward + 8);
                const f32 iff = Fdivs(one, Len3Aa(fx, fy, fz));
                const f32 ny = Fmuls(dy, il), nz = Fmuls(dz, il), nx = Fmuls(dx, il);
                const f32 id = Fdivs(one, Len3Aa(nx, ny, nz));
                TurnBetween(sp + 160, Fmuls(iff, fx), Fmuls(iff, fy), Fmuls(iff, fz), Fmuls(id, nx), Fmuls(id, ny),
                            Fmuls(id, nz));
                call(ctx, obj11::o11fn::Quat_FromMatrix, {sp + 176, f});
                call(ctx, o10fn::Quat_Multiply, {sp + 144, sp + 160, sp + 176});
                QuatUnit(sp + 160, sp + 144);
                call(ctx, o10fn::Quat_ToMatrix, {sp + 208, sp + 160, kOrigin});
                CopyRowsAa(f, sp + 208);
            }
            // Turn the up row onto the up axis.
            {
                const u32 up = obj11::kUpAxis;
                const f32 ry = Lf(f + kFoUp + 4), uy = Lf(up + 4), rx = Lf(f + kFoUp), ux = Lf(up);
                const f32 rz = Lf(f + kFoUp + 8), uz = Lf(up + 8);
                const f32 ir = Fdivs(one, Len3Aa(rx, ry, rz)), iu = Fdivs(one, Len3Aa(ux, uy, uz));
                TurnBetween(sp + 192, Fmuls(ir, rx), Fmuls(ir, ry), Fmuls(ir, rz), Fmuls(iu, ux), Fmuls(iu, uy),
                            Fmuls(iu, uz));
                call(ctx, obj11::o11fn::Quat_FromMatrix, {sp + 144, f});
                for (u32 k = 0; k < 16; k += 4) store<u32>(sp + 176 + k, load<u32>(sp + 144 + k));
                call(ctx, o10fn::Quat_Multiply, {sp + 144, sp + 192, sp + 176});
                QuatUnit(sp + 160, sp + 144);
                call(ctx, o10fn::Quat_ToMatrix, {sp + 208, sp + 160, kOrigin});
                CopyRowsAa(f, sp + 208);
                // r3 = Quat_ToMatrix's, as the original leaves it.
            }
        }
    }
    store<u32>(tp, load<u32>(f + kFoPhase));
    ctx.fpr[1] = static_cast<f64>(Lf(f + kFoPhase));
}
