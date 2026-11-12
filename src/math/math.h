// Single-precision math library functions (the game's C library: _FDtest,
// atanf kernel, acosf, atan2f). Guest addresses in the BCES01284 v01.02 EBOOT.
//
// Every operation mirrors the recompiler's semantics for the PPU float
// instructions: single forms are computed in double and rounded to float.
#pragma once

#include <bit>
#include <cmath>

#include "opentit/native.h"
#include "../sly4.h"

namespace sly4::math {
using namespace opentit::native;

inline f32 fadds(f64 a, f64 b) { return static_cast<f32>(a + b); }
inline f32 fsubs(f64 a, f64 b) { return static_cast<f32>(a - b); }
inline f32 fmuls(f64 a, f64 c) { return static_cast<f32>(a * c); }
inline f32 fdivs(f64 a, f64 b) { return static_cast<f32>(a / b); }
inline f32 fsqrts(f64 a) { return static_cast<f32>(std::sqrt(a)); }
inline f32 fmadds(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }   // a * c + b
inline f32 fmsubs(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, -b)); }  // a * c - b

namespace mfn {
constexpr u32 FDtest = 0x005E68D0;      // (const f32*) -> 2 NaN, 1 inf, 0 zero, -1 finite
constexpr u32 Atan_Kernel = 0x005E6830;  // (f32 x, u32 flags) -> f32
constexpr u32 Sqrtf = 0x005E693C;
constexpr u32 Acosf = 0x005E649C;
constexpr u32 Atan2f = 0x005E665C;
}  // namespace mfn

enum FClass : s32 { kFNaN = 2, kFInf = 1, kFZero = 0, kFFinite = -1 };

// Constants in .rodata (TOC 0x00F50800).
inline f32 rodata(u32 ea) { return global<be_f32>(ea); }
constexpr u32 kZeroF = 0x00F4892C, kSqrtHalf = 0x00F48920, kOneF = 0x00F48924;
constexpr u32 kNanPtr = 0x00F48928;     // u32 -> f32 NaN (0x7FC00000)
constexpr u32 kTan15 = 0x00F48950, kSqrt3 = 0x00F48954, kOneK = 0x00F48958;
constexpr u32 kEpsPtr = 0x00F4895C;     // u32 -> f32 2^-12
constexpr u32 kC2 = 0x00F48960, kC0 = 0x00F48964, kC1 = 0x00F48968;
constexpr u32 kOffsetsPtr = 0x00F4896C; // u32 -> f32[8] multiples of pi/6
constexpr u32 kAtan2One = 0x00F48940, kAtan2Zero = 0x00F48944;

// _FDtest on a float's bits (the original reads its two big-endian halves).
inline s32 FDtestBits(u32 bits) {
    const u32 hi = bits >> 16, lo = bits & 0xFFFF;
    const u32 exponent = hi & 0x7F80;
    if (exponent == 0x7F80) {
        if ((hi & 0x7F) != 0) return kFNaN;
        return lo == 0 ? kFInf : kFNaN;
    }
    if ((hi & 0x7FFF) != 0) return kFFinite;
    return lo == 0 ? kFZero : kFFinite;
}

// The atanf kernel: x in [0, 1] (reduced by the callers), flags bit 1 =
// negate before the offset, bits 0-2 index the offset table (bit 0 set
// here for x > tan 15deg), bit 3 = negate the result.
inline f32 Atan_Kernel(f32 x, u32 flags) {
    f64 f1 = x;
    if (f1 > rodata(kTan15)) {
        flags |= 1;
        const f32 sqrt3 = rodata(kSqrt3), one = rodata(kOneK);
        const f32 den = fadds(f1, sqrt3);
        const f32 num = fmsubs(f1, sqrt3, one);
        f1 = fdivs(num, den);
    }
    const f32 eps = *global<gp<be_f32>>(kEpsPtr);
    const f64 neg_eps = -static_cast<f64>(eps);
    if (neg_eps > f1 || f1 > eps) {
        const f32 x2 = fmuls(f1, f1);
        const f32 x3 = fmuls(f1, x2);
        const f32 p = fmadds(x2, rodata(kC0), rodata(kC1));
        const f32 num = fmadds(x3, rodata(kC2), f1);
        const f32 den = fmadds(x2, p, rodata(kOneK));
        f1 = fdivs(num, den);
    }
    if (flags & 2) f1 = -f1;
    const u32 offset = (flags << 2) & 0x1C;
    const be_f32* offsets = global<gp<be_f32>>(kOffsetsPtr);
    f32 r = fadds(f1, offsets[offset / 4]);
    if (flags & 8) r = -r;
    return r;
}

s32 FDtest(const be_u16* p);
f32 Sqrtf(f32 x);
f32 Acosf(f32 x);
f32 Atan2f(f32 a, f32 b);

}  // namespace sly4::math
