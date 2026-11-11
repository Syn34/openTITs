// Game code, VMX sweep part 4 (see vmx4.h): sound emitters.
#include "../sly4.h"
#include "vmx4.h"

using namespace sly4;
using namespace sly4::vmx4;

namespace {
constexpr u32 kK213820 = 0x00213820;  // three vperm masks, then f32 {1.0, 0.0, 1e-5}, f64 {1.0, 2^62 - 2^52 (the 1.0 bias)}

// The fast pow the code inlines: x^p from the bit pattern of the double x
// (fcfid of its bits, * p + (1 - p) * bias, fctid back to bits, frsp).
inline f32 FastPow(f64 x, f64 p) {
    const f64 n = rt::fcfid(std::bit_cast<u64>(x));
    const f64 c = (std::bit_cast<f64>(load<u64>(kK213820 + 64)) - p) * std::bit_cast<f64>(load<u64>(kK213820 + 72));
    return static_cast<f32>(std::bit_cast<f64>(rt::fctid(std::fma(n, p, c), false)));
}
// The shared tail: with t (< one), one - t^p, where |t - one| < 1e-5 gives
// one - t, p == 0 gives one - one, and t == 0 gives one - 0.
inline f32 Falloff(f32 t, f32 one, f32 p) {
    const f32 zero = F(kK213820 + 52);
    f32 v;
    if (std::fabs(static_cast<f64>(Subs(t, one))) < static_cast<f64>(F(kK213820 + 56))) v = t;
    else if (p == zero) v = one;
    else if (t == zero) v = zero;
    else v = FastPow(t, p);
    return Subs(one, v);
}
}  // namespace

// f32 f(Emitter* o, const f32 p[3]) (called by 00213C78, 002140C0,
// Emitter_Spatialise, Emitter_Update): the emitter's gain at the world
// point p. p is taken into the emitter's frame (the rigid inverse of the
// 4x3 rows at +96, built with lvlx / vmrghw / vperm as the original does;
// stored on the stack). The falloff exponent is (o->+216 ->+180)->+4, else
// 1.0. Shape +428 == 3 (box +496 min / +512 max, fade widths +560 / +576):
// 1.0 inside (Box_ContainsPoint); else t = the largest per-axis distance
// past a face over its width (0 when none), 0.0 when t >= 1, else the
// falloff 1 - t^exp. Shape 0 (sphere, radii +488 / +492): 1.0 within the
// inner radius, else t = (d - inner) / (outer - inner) and the same. Any
// other shape: 0.0. r3 is left as the original leaves it.
DECOMP(00213870, Emitter_Gain_00213870) {
    const u32 o = arg<u32>(ctx, 0), pt = arg<u32>(ctx, 1);
    const u32 snd = load<u32>(o + 216);
    const u32 shape = load<u32>(o + 428);
    const u32 curve = snd != 0 ? load<u32>(snd + 180) : 0;
    Stack frame(ctx, 224);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 loc = sp + 112;
    const V4 a96 = Lvlx(o + 96), a100 = Lvlx(o + 100), a104 = Lvlx(o + 104), a112 = Lvlx(o + 112), a116 = Lvlx(o + 116),
             a120 = Lvlx(o + 120), a128 = Lvlx(o + 128), a132 = Lvlx(o + 132), a136 = Lvlx(o + 136), a144 = Lvlx(o + 144),
             a148 = Lvlx(o + 148), a152 = Lvlx(o + 152);
    const V4 p0 = Lvlx(pt), p1 = Lvlx(pt + 4), p2 = Lvlx(pt + 8);
    const V4 stale = Lvx(sp + 176);  // the original merges in a stack vector it never wrote (lane 3 only)
    for (u32 k = 0; k < 3; ++k) CopyW(loc + 4 * k, pt + 4 * k);
    const V4 m0 = Lvx(kK213820), m1 = Lvx(kK213820 + 16), m2 = Lvx(kK213820 + 32);
    const V4 t = Perm(Perm(a144, a148, m0), Perm(a152, stale, m0), m1);
    const V4 nt = Sub4(Zero4(), t);
    const V4 c0 = Perm(Mrghw(a96, a112), Mrghw(a128, a116), m2);
    const V4 c1 = Perm(Mrghw(a100, a116), Mrghw(a132, a96), m2);
    const V4 c2 = Perm(Mrghw(a104, a120), Mrghw(a136, a104), m2);
    const V4 rot = Madd4(c2, Splat(p2, 0), Madd4(c1, Splat(p1, 0), Madd4(c0, Splat(p0, 0), Zero4())));
    const V4 tr = Madd4(c2, Splat(nt, 2), Madd4(c1, Splat(nt, 1), Madd4(c0, Splat(nt, 0), Zero4())));
    Stvx(loc, Add4(tr, rot));
    const f32 one = F(kK213820 + 48), zero = F(kK213820 + 52);
    const f32 ex = curve != 0 ? F(curve + 4) : one;
    const f32 lx = F(loc), ly = F(loc + 4), lz = F(loc + 8);
    if (shape == 3) {
        if (call(ctx, v4fn::Box_ContainsPoint, {o + 496, loc}) != 0) {
            ctx.fpr[1] = one;
            return;
        }
        f32 m = zero;
        for (u32 k = 0; k < 3; ++k) {
            f32 v = zero;
            const f32 w = F(o + 560 + 4 * k), x = F(loc + 4 * k);
            bool done = false;
            if (w > zero) {
                const f32 hi = F(o + 512 + 4 * k);
                if (x > hi) {
                    v = Divs(Subs(x, hi), w);
                    done = true;
                }
            }
            if (!done) {
                const f32 w2 = F(o + 576 + 4 * k);
                if (w2 < zero) {
                    const f32 lo = F(o + 496 + 4 * k);
                    if (x < lo) v = Divs(Subs(x, lo), w2);
                }
            }
            if (v > m) m = v;
        }
        if (m >= one) {
            ctx.fpr[1] = zero;
            ctx.gpr[3] = loc + 12;
            return;
        }
        ctx.fpr[1] = Falloff(m, one, ex);
        ctx.gpr[3] = kK213820;
        return;
    }
    if (shape != 0) {
        ctx.fpr[1] = zero;
        ctx.gpr[3] = kK213820;
        return;
    }
    const f32 inner = F(o + 488);
    const f32 d = Len3s(lx, ly, lz);
    ctx.gpr[3] = 112;
    if (d <= inner) {
        ctx.fpr[1] = one;
        return;
    }
    const f32 tt = Divs(Subs(d, inner), Subs(F(o + 492), inner));
    ctx.gpr[3] = kK213820;
    if (tt >= one) {
        ctx.fpr[1] = zero;
        return;
    }
    ctx.fpr[1] = Falloff(tt, one, ex);
}
