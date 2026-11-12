// acosf, atan2f and their helpers from the game's C library.
#include "math.h"

namespace sly4::math {

// s32 _FDtest(const f32* p)
s32 FDtest(const be_u16* p) { return FDtestBits((u32(p[0]) << 16) | p[1]); }

f32 Sqrtf(f32 x) { return fsqrts(x); }

// _Atan(x, flags): see Atan_Kernel in math.h.
f32 Atan_KernelFn(f32 x, u32 flags) { return Atan_Kernel(x, flags); }

// |x| <= sqrt(1/2): atan(x / sqrt(1 - x^2)) offset to pi/2;
// otherwise atan(sqrt(1 - x^2) / x); NaN beyond 1 (and for NaN).
f32 Acosf(f32 xf) {
    const f64 x = xf;
    f64 a = x;
    u32 flags = 2;
    if (x < rodata(kZeroF)) {
        a = -x;
        flags = 4;
    }
    const f32 one = rodata(kOneF);
    if (a <= rodata(kSqrtHalf)) {
        const f32 s = fsqrts(fmuls(fsubs(one, a), fadds(a, one)));
        return Atan_Kernel(fdivs(a, s), flags & 6);
    }
    if (a <= one) {
        const f32 t = fsubs(one, a);
        const f32 s = fsqrts(fmadds(a, t, t));
        return Atan_Kernel(fdivs(s, a), (flags ^ 2) & 6);
    }
    return *global<gp<be_f32>>(kNanPtr);
}

// atan2f(a, b): atan(b / a) by quadrant. The original first stores both
// into the caller's parameter save area (r1 + 48, r1 + 56) and classifies
// them from there; that scratch space isn't observable, so this works on
// the values.
f32 Atan2f(f32 a, f32 b) {
    const s32 cb = static_cast<s16>(FDtestBits(std::bit_cast<u32>(b)));
    const s32 ca = static_cast<s16>(FDtestBits(std::bit_cast<u32>(a)));

    f32 x;
    u32 flags;
    if (cb > 0 || ca > 0) {
        // An infinity or NaN.
        if (ca == kFNaN) return a;
        if (cb == kFNaN) return b;
        x = cb == ca ? rodata(kAtan2One) : rodata(kAtan2Zero);
        flags = (std::bit_cast<u32>(a) & 0x80000000u) ? 8 : 0;
        if (std::bit_cast<u32>(b) & 0x80000000u) flags |= 6;
        if (ca == kFInf) flags ^= 2;
    } else {
        flags = 0;
        if (std::bit_cast<u32>(a) & 0x80000000u) {
            a = -a;
            flags = 8;
        }
        if (std::bit_cast<u32>(b) & 0x80000000u) {
            b = -b;
            flags ^= 6;
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
    return Atan_Kernel(x, flags & 0xFFFF);
}

}  // namespace sly4::math

NATIVE(005E68D0, sly4::math::FDtest)
NATIVE(005E693C, sly4::math::Sqrtf)
NATIVE(005E6830, sly4::math::Atan_KernelFn)
NATIVE(005E649C, sly4::math::Acosf)
NATIVE(005E665C, sly4::math::Atan2f)
