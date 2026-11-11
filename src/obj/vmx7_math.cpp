// Game code, VMX sweep part 7 (see vmx7.h): vector math.
#include "../sly4.h"
#include "vmx7.h"

using namespace sly4;
using namespace sly4::vmx7;

namespace {
constexpr u32 kVec_RotateToward = 0x001007D0;  // (f32 out[3], const f32 from[3], const f32 to[3]; f32 maxAngle)
constexpr u32 kAcosf = 0x005E649C;  // f1 -> f1
// f32 1 (+0), 0 (+4), f64 1 (+8), 0 (+16), f32 1e-5 (+24), -1 (+28), f64 -1 (+32)
constexpr u32 kTurnK = 0x001026A8;

// The compiler's acos-argument clamp to [-1, 1] (four fsel steps, each
// rounded to single).
f64 TurnClamp(f64 x, u32 K) {
    const f64 one = Lfs4(K), d1 = Lfd(K + 8), dm1 = Lfd(K + 32);
    x = static_cast<f32>(Subs(one, x) >= 0.0 ? x : d1);
    x = static_cast<f32>(Subs(x, Lfs4(K + 28)) >= 0.0 ? x : dm1);
    x = static_cast<f32>(Adds(x, one) >= 0.0 ? x : dm1);
    x = static_cast<f32>(Subs(x, one) >= 0.0 ? d1 : x);
    return x;
}
}  // namespace

// u32 f(f32 out[3], const f32 a[3], const f32 b[3]; f32 t) (from
// 0x00102BE8): out = a normalised, turned toward b normalised by at most
// acos(..) * clamp01(t). When both are nearly vertical (|y| < 1) their
// horizontal parts are first lined up: out is rotated about the up axis
// (Matrix_RotationY of the signed horizontal angle, vmaddfp, stvx to out)
// when they point apart. Nothing more when out's and b's y differ by less
// than 1e-5. Else, with neither y at +-1 or beyond: Vec_RotateToward(out,
// b' = out's horizontal direction scaled to b's horizontal length, with
// b's y; angle acos(out . b') * t); otherwise toward b itself.
DECOMP(001026D0, Vec_TurnToward_001026D0) {
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const f64 t = ctx.fpr[1];
    const u32 K = kTurnK;
    Stack frame(ctx, 336);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const f64 a0 = Lfs4(a), a1 = Lfs4(a + 4), a2 = Lfs4(a + 8), b0 = Lfs4(b), b1 = Lfs4(b + 4), b2 = Lfs4(b + 8);
    const f64 one = Lfs4(K), zero = Lfs4(K + 4);
    const f32 la = Sqrts(Fmadds(a2, a2, Fmadds(a0, a0, Muls(a1, a1))));
    const f32 lb = Sqrts(Fmadds(b2, b2, Fmadds(b0, b0, Muls(b1, b1))));
    f64 tc = static_cast<f32>(Subs(one, t) >= 0.0 ? t : Lfd(K + 8));
    tc = static_cast<f32>(tc >= 0.0 ? tc : Lfd(K + 16));
    const f32 ia = Divs(one, la), ib = Divs(one, lb);
    f64 na1 = Muls(a1, ia), na0 = Muls(a0, ia), na2 = Muls(a2, ia);
    const f32 nb0 = Muls(b0, ib), nb1 = Muls(b1, ib), nb2 = Muls(b2, ib);
    store<f32>(sp + 112, nb0);
    store<f32>(sp + 116, nb1);
    store<f32>(sp + 120, nb2);
    store<f32>(out, static_cast<f32>(na0));
    store<f32>(out + 4, static_cast<f32>(na1));
    store<f32>(out + 8, static_cast<f32>(na2));
    if (std::fabs(na1) < one && std::fabs(nb1) < one) {
        const f64 z = Muls(zero, zero);
        const f32 sa = Divs(one, Sqrts(Fmadds(na2, na2, Fmadds(na0, na0, z))));
        const f32 sb = Divs(one, Sqrts(Fmadds(nb2, nb2, Fmadds(nb0, nb0, z))));
        const f32 hay = Muls(zero, sa), hax = Muls(na0, sa), haz = Muls(na2, sa);
        const f32 hby = Muls(zero, sb), hbx = Muls(nb0, sb), hbz = Muls(nb2, sb);
        const f32 dot = Fmadds(haz, hbz, Fmadds(hax, hbx, Muls(hay, hby)));
        if (dot < one) {
            f64 ang = callf(ctx, kAcosf, {}, {TurnClamp(dot, K)});
            const u32 I = kAttachIdentity;
            const f64 ux = Lfs4(I + 16), uy = Lfs4(I + 20), uz = Lfs4(I + 24);
            const f32 cy = Fmsubs(uz, hax, Muls(ux, haz));
            const f32 cx = Fmsubs(uy, haz, Muls(uz, hay));
            const f32 cz = Fmsubs(ux, hay, Muls(uy, hax));
            if (Fmadds(cz, hbz, Fmadds(cx, hbx, Muls(cy, hby))) < zero) ang = -ang;
            CopyRowsXyz7(sp + 176, I, 4);
            callf(ctx, v5fn::Matrix_RotationY, {sp + 176}, {Muls(ang, tc)});
            const V4 r0 = Lvx(sp + 176), r1 = Lvx(sp + 192), r2 = Lvx(sp + 208), r3 = Lvx(sp + 224);
            Stvx(out, Add4(r3, Madd4(r2, big7::Splat1(F(out + 8)), Madd4(r1, big7::Splat1(F(out + 4)), Mul0(r0, big7::Splat1(F(out)))))));
            na0 = Lfs4(out);
            na1 = Lfs4(out + 4);
            na2 = Lfs4(out + 8);
        }
    }
    const f64 o1 = Lfs4(out + 4);
    if (std::fabs(Subs(o1, nb1)) < Lfs4(K + 24)) {
        ctx.fpr[1] = nb0;
        ctx.gpr[3] = out + 4;
        return;
    }
    const f32 yy = Muls(na1, nb1);
    u32 res;
    if (std::fabs(o1) >= one || std::fabs(nb1) >= one) {
        const f32 dot = Fmadds(na2, nb2, Fmadds(na0, nb0, yy));
        const f64 ang = callf(ctx, kAcosf, {}, {TurnClamp(dot, K)});
        callf(ctx, kVec_RotateToward, {sp + 128, out, sp + 112}, {Muls(ang, tc)});
        res = sp + 128;
    } else {
        const f64 z = Muls(zero, zero);
        store<f32>(sp + 164, nb1);
        const f32 lo = Sqrts(Fmadds(na2, na2, Fmadds(na0, na0, z)));
        const f32 lh = Sqrts(Fmadds(nb2, nb2, Fmadds(nb0, nb0, z)));
        const f32 s = Divs(one, lo);
        const f32 tx = Muls(Muls(na0, s), lh), tz = Muls(Muls(na2, s), lh);
        store<f32>(sp + 160, tx);
        store<f32>(sp + 168, tz);
        const f32 dot = Fmadds(na2, tz, Fmadds(na0, tx, yy));
        const f64 ang = callf(ctx, kAcosf, {}, {TurnClamp(dot, K)});
        callf(ctx, kVec_RotateToward, {sp + 144, out, sp + 160}, {Muls(ang, tc)});
        res = sp + 144;
    }
    CopyW6(out, res, 3);
    ctx.fpr[1] = Lfs4(out + 8);
}
