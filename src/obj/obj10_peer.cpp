// World objects, part 10: Peer_GetMatrix, the procedural animation of a
// peer record: its rotation / position animators (+128 / +132) and its
// spline follower (+136).
#include "obj10.h"

using namespace sly4;
using namespace sly4::obj9;
using namespace sly4::obj10;

namespace {

// The original's frame offsets (400 bytes).
constexpr u32 kFrame = 400;
constexpr u32 kSpQuatXY = 112, kSpQuatX = 128, kSpQuatY = 144, kSpQuatZ = 160, kSpQuat = 176;
constexpr u32 kSpMatrix = 192, kSpPosition = 256, kSpAngles = 268;

f32 Pc(u32 off) { return Lf(kPgConsts + off); }

void CopyWord(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }

// The rows' x, y, z of a 4x4 matrix (the w lanes are left alone).
void CopyMatrix3(u32 dst, u32 src) {
    for (u32 r = 0; r < 64; r += 16)
        for (u32 k = 0; k < 12; k += 4) CopyWord(dst + r + k, src + r + k);
}

// Advances the three axis times of an animator by dt. A "once" axis whose
// period + delay is 0 stops the loop (the later axes are not advanced).
void AnimAdvance(u32 a, f64 dt) {
    const f32 zero = Pc(kPgZero);
    for (u32 i = 0; i < 3; ++i) {
        const u32 t = a + 4 * i;
        const f32 period = Lf(t + kAnPeriod);
        if (period == zero) continue;
        const u32 mb = a + kAnMode + i;
        const u8 bits = load<u8>(mb);
        const s32 mode = AnimMode(bits);
        if (mode == kAnOnce) {
            const f32 lim = Fadds(period, Lf(t + kAnDelay));
            if (lim == zero) break;  // quirk kept
            const f32 x = Fadds(Lf(t), dt);
            store<f32>(t, x);
            if (x > lim) store<f32>(t, lim);
        } else if (mode == kAnLoop) {
            f32 x = Fadds(Lf(t), dt);
            store<f32>(t, x);
            const f32 lim = Fadds(Lf(t + kAnPeriod), Lf(t + kAnDelay));
            while (x > lim) {
                x = Fsubs(x, lim);
                store<f32>(t, x);
            }
        } else if (mode == kAnPingPong) {
            const f32 lim = Fadds(period, Lf(t + kAnDelay));
            if ((bits & kAnBackwards) != 0) {
                const f32 x = Fsubs(Lf(t), dt);
                store<f32>(t, x);
                if (x < zero) {
                    store<f32>(t, -x);
                    store<u8>(mb, static_cast<u8>(load<u8>(mb) & ~kAnBackwards));
                }
            } else {
                f32 x = Fadds(Lf(t), dt);
                store<f32>(t, x);
                if (x > lim) {
                    do {
                        x = Fsubs(x, lim);
                        store<f32>(t, x);
                    } while (x > lim);
                    store<f32>(t, Fsubs(lim, x));
                    store<u8>(mb, static_cast<u8>(load<u8>(mb) | kAnBackwards));
                }
            }
        }
    }
}

// x wrapped into [-pi, pi]: (x + sign(x) pi) less its whole turns, less
// sign(x) pi (the sign from fsel: NaN counts as negative).
f32 WrapPi(f32 x) {
    const f64 sd = static_cast<f64>(x) >= 0.0 ? load<f64>(kPgConsts + kPgPiD) : load<f64>(kPgConsts + kPgNegPiD);
    const f32 s = static_cast<f32>(sd);
    const f32 a = Fadds(s, x);
    const f32 turns = static_cast<f32>(rt::fcfid(rt::fctid(Fdivs(a, Pc(kPgTwoPi)), true)));
    return Fsubs(Fnmsubs(turns, Pc(kPgTwoPi), a), s);
}

// The parabolic sine of x in [-pi, pi]: y = 4/pi x - 4/pi^2 x|x|, then
// y + 0.225 (y|y| - y).
f32 FastSin(f32 x) {
    const f32 y = Fmadds(x, Pc(kPgSinA), Fmuls(Fmuls(x, Pc(kPgSinB)), std::fabs(x)));
    return Fmadds(Fmsubs(y, std::fabs(y), y), Pc(kPgSinC), y);
}

// The cosine argument of a wrapped x: x + pi/2, less 2pi above pi.
f32 CosArg(f32 x) {
    const f32 c = Fadds(x, Pc(kPgHalfPi));
    return c > Pc(kPgPi) ? Fsubs(c, Pc(kPgTwoPi)) : c;
}

}  // namespace

// bool Peer_GetMatrix(PeerRecord* p, f32 dt, f32 out[4][4])
// out starts as the identity (rows' x, y, z). With a rotation animator
// (+128): its times advance by dt, then each axis angle is base + amp *
// ramp (time - delay, at least 0, over the period) or, for a ping-pong
// axis, base + amp * cos(2pi (time - delay) / period) (a parabolic
// approximation), in degrees; an axis with period <= 0 keeps its base
// unconverted. out = the rotation X * Y * Z of the half angles (as
// quaternions through Quat_Multiply / Quat_ToMatrix, translation from the
// origin). With a position animator (+132): its times advance likewise
// and out's translation row = base + amp * ramp or amp * Math_Cosf(...).
// With a spline follower (+136): Peer_ApplyFollower(p, follower, out,
// rotated, moved) and returns 1; otherwise returns 0.
DECOMP(0057C408, Peer_GetMatrix) {
    const u32 p = arg<u32>(ctx, 0), out = arg<u32>(ctx, 2);
    const f64 dt = farg(ctx, 0);  // used as it comes (fmr into f31)
    Stack frame(ctx, kFrame - 112);  // + the 112-byte linkage area call() adds = the original's frame
    const u32 sp = frame.addr() - 112;
    CopyMatrix3(out, kIdentity);
    const f32 zero = Pc(kPgZero);

    const u32 rot = load<u32>(p + kPrRotAnim);
    if (rot != 0) {
        AnimAdvance(rot, dt);
        const u32 ang = sp + kSpAngles;
        for (u32 i = 0; i < 3; ++i) {
            const u32 t = rot + 4 * i;
            f32 x = Fsubs(Lf(t), Lf(t + kAnDelay));
            CopyWord(ang + 4 * i, t + kAnBase);
            f32 v = Lf(t + kAnBase);
            if (x < zero) x = zero;
            const f32 period = Lf(t + kAnPeriod);
            if (!(period > zero)) continue;
            if (AnimMode(load<u8>(rot + kAnMode + i)) == kAnPingPong) {
                const f32 w = FastSin(CosArg(WrapPi(Fdivs(Fmuls(x, Pc(kPgTwoPi)), period))));
                v = Fmadds(Lf(t + kAnAmp), w, v);
            } else {
                v = Fadds(v, Fdivs(Fmuls(Lf(t + kAnAmp), x), period));
            }
            store<f32>(ang + 4 * i, Fmuls(Fmuls(v, Pc(kPgDegToRad)), Pc(kPgHalf)));
        }
        // One quaternion per axis: {sin h along the axis, cos h}.
        const u32 q[3] = {sp + kSpQuatX, sp + kSpQuatY, sp + kSpQuatZ};
        for (u32 i = 0; i < 3; ++i) {
            const f32 x = WrapPi(Lf(ang + 4 * i));
            for (u32 k = 0; k < 3; ++k) store<u32>(q[i] + 4 * k, 0);
            store<f32>(q[i] + 4 * i, FastSin(x));
            store<f32>(q[i] + 12, FastSin(CosArg(x)));
        }
        call(ctx, o10fn::Quat_Multiply, {sp + kSpQuatXY, q[0], q[1]});
        call(ctx, o10fn::Quat_Multiply, {sp + kSpQuat, sp + kSpQuatXY, q[2]});
        call(ctx, o10fn::Quat_ToMatrix, {sp + kSpMatrix, sp + kSpQuat, kOrigin});
        CopyMatrix3(out, sp + kSpMatrix);
    }

    const u32 pos = load<u32>(p + kPrPosAnim);
    if (pos != 0) {
        AnimAdvance(pos, dt);
        const u32 v3 = sp + kSpPosition;
        for (u32 i = 0; i < 3; ++i) {
            const u32 t = pos + 4 * i;
            f32 x = Fsubs(Lf(t), Lf(t + kAnDelay));
            CopyWord(v3 + 4 * i, t + kAnBase);
            const f32 base = Lf(t + kAnBase);
            if (x < zero) x = zero;
            const f32 period = Lf(t + kAnPeriod);
            if (!(period > zero)) continue;
            f32 v;
            if (AnimMode(load<u8>(pos + kAnMode + i)) == kAnPingPong) {
                const f64 w = callf(ctx, o10fn::Math_Cosf, {}, {Fdivs(Fmuls(x, Pc(kPgTwoPi)), period)});
                v = Fmuls(Lf(t + kAnAmp), w);
            } else {
                v = Fdivs(Fmuls(Lf(t + kAnAmp), x), period);
            }
            store<f32>(v3 + 4 * i, Fadds(base, v));
        }
        for (u32 k = 0; k < 12; k += 4) CopyWord(out + 48 + k, v3 + k);
    }

    const u32 fol = load<u32>(p + kPrFollower);
    if (fol == 0) {
        ret(ctx, 0u);
        return;
    }
    // r5 = cntlzw(rot) is left over from the original's bool computation
    // (Peer_ApplyFollower reads r3, r4, r6, r7, r8 and f1 only).
    callf(ctx, o10fn::Peer_ApplyFollower,
          {p, fol, static_cast<u64>(std::countl_zero(rot)), out, rot != 0 ? 1u : 0u, pos != 0 ? 1u : 0u}, {dt});
    ret(ctx, 1u);
}
