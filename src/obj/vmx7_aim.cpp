// Game code, VMX sweep part 7 (see vmx7.h): aiming / targeting updates.
#include "../sly4.h"
#include "vmx7.h"

using namespace sly4;
using namespace sly4::vmx7;

namespace {
constexpr u32 kHk_SetF32At = 0x007EE08C;  // (f32* field, const f32* value) (Havok; name?)
// f32 {200, 0, 1, 0.447, 0.5, 0}, f64 1.0 (+24), f32 -1 (+32), f64 -1 (+40),
// f32 180/pi (+48), f64 0 (+56)
constexpr u32 kAimK23A8D8 = 0x0023A8D8;
constexpr u32 kHalfTabB1B2C0 = 0x00B1B2C0;  // u16[256]: half-float table indexed by a byte
}  // namespace

// u32 f(Aimer* o, Out* out; f32 dt) (from 0x0023C168): with d = o->+396 +
// 1272 and the target t = (o->+612)->+24:
// - when d->+88 is set, first Hk_SetF32At(t + 410, &v) with s = the half
//   table entry for t's byte +410 (0 when the entry is 0), g = d->+56 *
//   o->+1004 (0 when (o->+612)->+176 is set), v = s + max(fma(d+52 + g,
//   0.447, -s), -(fma(s, fma(s, d+100, d+96), d+92) * dt));
// - then Hk_SetF32At(t + 410, &w): w = 200 when (o->+612)->+28 is set and
//   none of its +32 (s8) parts (224-byte records at +72) has +36 set; else
//   the larger of 0.447 * (d+52 + d+56 * o->+1004 (0 when o->+1004 is 0
//   or +176 is set)) and t's +432 vector length (vrsqrtefp + Newton);
// - out->+20 = d+12 * (1 + ((d+72 - 1) * o->+1004, or 0 when +176 is
//   set)); when (o->+396)->+1264 is positive, out->+20 += d+16 *
//   clamp01(((o->+612)->+36)->+28 / it); when d+20 is positive, out->+20
//   += d+24 * clamp01(deg(Libm_Unary5E6578(clamp(o->+132, -1, 1))) (negated
//   when o->+980 is non-zero) / d+20).
// Returns the constant table address when either of the last two steps ran,
// else (o->+396) + 1264; f1 is the last out->+20 or d+20.
DECOMP(0023A918, Aim_UpdateTarget_0023A918) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const f64 dt = ctx.fpr[1];
    const u32 K = kAimK23A8D8;
    Stack frame(ctx, 208);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 d = load<u32>(o + 396) + 1272;
    if (load<u8>(d + 88) != 0) {
        const u32 c = load<u32>(o + 612);
        const u32 t = load<u32>(c + 24);
        f64 s = Lfs4(K + 4);
        const u32 h = load<u16>(kHalfTabB1B2C0 + 2 * load<u8>(t + 410));
        if (h != 0) s = std::bit_cast<f64>(LfsBits((h + 0x3B800u) << 12));
        f64 g = Lfs4(K + 4);
        if (load<u8>(c + 176) == 0) g = Muls(Lfs4(d + 56), Lfs4(o + 1004));
        const f32 a = Fmadds(s, Fmadds(s, Lfs4(d + 100), Lfs4(d + 96)), Lfs4(d + 92));
        const f32 b = Fmsubs(Adds(Lfs4(d + 52), g), Lfs4(K + 12), s);
        const f32 m = -Muls(a, dt);
        store<f32>(sp + 116, Adds(s, Subs(b, m) >= 0.0 ? b : m));
        call(ctx, kHk_SetF32At, {t + 410, sp + 116});
    }
    const u32 c = load<u32>(o + 612);
    bool idle = false;
    if (c != 0 && load<u32>(c + 28) != 0) {
        const u32 n = static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(load<u32>(c + 28) + 32))));
        idle = true;
        for (u32 i = 0; i < n; ++i)
            if (load<u32>(load<u32>(c + 72) + 224 * i + 36) != 0) { idle = false; break; }
    }
    if (idle) {
        store<u32>(sp + 112, load<u32>(K));
    } else {
        // c may be null here; the original reads through it regardless.
        const f64 w = Lfs4(o + 1004);
        f64 a = Lfs4(K + 4);
        if (w != a && load<u8>(c + 176) == 0) a = Muls(Lfs4(d + 56), w);
        const f32 f1 = Adds(Lfs4(d + 52), a);
        const V4 p = Lvx(load<u32>(c + 24) + 432);
        Stvx(sp + 128, SqrtNewton4(HSum3(Mul0(p, p)), SplatW(K + 16), SplatW(K + 8)));
        const f32 f3 = Muls(f1, Lfs4(K + 12));
        if (Subs(f3, Lfs4(sp + 128)) >= 0.0) store<f32>(sp + 112, f3);
        else store<u32>(sp + 112, load<u32>(sp + 128));
    }
    call(ctx, kHk_SetF32At, {load<u32>(c + 24) + 410, sp + 112});
    const u32 c2 = load<u32>(o + 612);
    const f64 d12 = Lfs4(d + 12), d20 = Lfs4(d + 20);
    const u32 q = load<u32>(o + 396);
    const f64 zero = Lfs4(K + 4);
    f64 f3 = zero;
    if (load<u8>(c2 + 176) == 0) f3 = Muls(Subs(Lfs4(d + 72), Lfs4(K + 8)), Lfs4(o + 1004));
    const f64 one = Lfs4(K + 8);
    const f32 out20 = Muls(d12, Adds(f3, one));
    store<f32>(out + 20, out20);
    u32 r3 = q + 1264;
    const f64 w = Lfs4(q + 1264);
    if (w > zero) {
        const f32 a = Divs(Lfs4(load<u32>(c2 + 36) + 28), w);
        f64 x = Subs(one, a) >= 0.0 ? static_cast<f64>(a) : Lfd(K + 24);
        x = x >= 0.0 ? x : Lfd(K + 56);
        store<f32>(out + 20, Fmadds(Lfs4(d + 16), std::fabs(x), out20));
        r3 = K;
    }
    ctx.fpr[1] = d20;
    if (d20 > zero) {
        const f64 x0 = Lfs4(o + 132);
        f64 x = Subs(one, x0) >= 0.0 ? x0 : Lfd(K + 24);
        x = static_cast<f32>(x);
        x = Subs(x, Lfs4(K + 32)) >= 0.0 ? x : Lfd(K + 40);
        x = static_cast<f32>(x);
        const f64 r = callf(ctx, v5fn::Libm_Unary5E6578, {}, {x});
        f32 deg = Muls(r, Lfs4(K + 48));
        if (Lfs4(o + 980) != zero) deg = -deg;
        const f32 tq = Divs(deg, Lfs4(d + 20));
        f64 y = Subs(one, tq) >= 0.0 ? static_cast<f64>(tq) : Lfd(K + 24);
        y = static_cast<f32>(y);
        y = y >= 0.0 ? y : Lfd(K + 56);
        y = static_cast<f32>(y);
        const f32 res = Fmadds(Lfs4(d + 24), y, Lfs4(out + 20));
        store<f32>(out + 20, res);
        ctx.fpr[1] = res;
        r3 = K;
    }
    ctx.gpr[3] = r3;
}
