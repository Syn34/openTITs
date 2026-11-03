// acosf, atan2f and their helpers from the game's C library.
#include "math.h"

using namespace sly4;
using namespace sly4::math;

// s32 _FDtest(const f32* p)
DECOMP(005E68D0, FDtest) { ret(ctx, math::FDtest(arg<u32>(ctx, 0))); }

// f32 sqrtf(f32)
DECOMP(005E693C, Sqrtf) { ctx.fpr[1] = fsqrts(farg(ctx, 0)); }

// f32 _Atan(f32 x, u32 flags): see math::Atan_Kernel.
DECOMP(005E6830, Atan_Kernel) {
    ctx.fpr[1] = math::Atan_Kernel(static_cast<f32>(farg(ctx, 0)), arg<u32>(ctx, 1));
}

// f32 acosf(f32 x)
// |x| <= sqrt(1/2): atan(x / sqrt(1 - x^2)) offset to pi/2;
// otherwise atan(sqrt(1 - x^2) / x); NaN beyond 1 (and for NaN).
DECOMP(005E649C, Acosf) {
    const f64 x = farg(ctx, 0);
    f64 a = x;
    u32 flags = 2;
    if (x < rodata(kZeroF)) {
        a = -x;
        flags = 4;
    }
    const f32 one = rodata(kOneF);
    if (a <= rodata(kSqrtHalf)) {
        const f32 s = fsqrts(fmuls(fsubs(one, a), fadds(a, one)));
        ctx.fpr[1] = math::Atan_Kernel(fdivs(a, s), flags & 6);
    } else if (a <= one) {
        const f32 t = fsubs(one, a);
        const f32 s = fsqrts(fmadds(a, t, t));
        ctx.fpr[1] = math::Atan_Kernel(fdivs(s, a), (flags ^ 2) & 6);
    } else {
        ctx.fpr[1] = load<f32>(load<u32>(kNanPtr));
    }
}

// f32 atan2f(f32 a, f32 b)  (a in f1, b in f2; atan(b / a) by quadrant)
// Both are first stored into the caller's parameter save area (r1 + 48,
// r1 + 56), as the original does, and classified from there.
DECOMP(005E665C, Atan2f) {
    const u32 pa = static_cast<u32>(ctx.gpr[1]) + 48, pb = static_cast<u32>(ctx.gpr[1]) + 56;
    store<f32>(pa, static_cast<f32>(farg(ctx, 0)));
    store<f32>(pb, static_cast<f32>(farg(ctx, 1)));
    const s32 cb = static_cast<s16>(math::FDtest(pb));
    const s32 ca = static_cast<s16>(math::FDtest(pa));

    f32 x;
    u32 flags;
    if (cb > 0 || ca > 0) {
        // An infinity or NaN.
        if (ca == kFNaN) return void(ctx.fpr[1] = load<f32>(pa));
        if (cb == kFNaN) return void(ctx.fpr[1] = load<f32>(pb));
        x = cb == ca ? rodata(kAtan2One) : rodata(kAtan2Zero);
        flags = (load<u16>(pa) & 0x8000) ? 8 : 0;
        if (load<u16>(pb) & 0x8000) flags |= 6;
        if (ca == kFInf) flags ^= 2;
    } else {
        f32 a = load<f32>(pa);
        flags = 0;
        if (std::bit_cast<u32>(a) & 0x80000000u) {
            a = -a;
            flags = 8;
            store<f32>(pa, a);
        }
        f32 b = load<f32>(pb);
        if (std::bit_cast<u32>(b) & 0x80000000u) {
            b = -b;
            flags ^= 6;
            store<f32>(pb, b);
        }
        if (b < a) {
            flags ^= 2;
            x = fdivs(b, a);
        } else if (cb == kFZero) {
            x = rodata(kAtan2Zero);
        } else {
            x = fdivs(a, b);
        }
    }
    ctx.fpr[1] = math::Atan_Kernel(x, flags & 0xFFFF);
}
