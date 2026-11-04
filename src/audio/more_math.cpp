// libm float logarithm (_FLog) and its _FDunscale helper, as compiled into
// the EBOOT. The constants come from the TOC, as in the original.
#include <bit>
#include <cmath>

#include "more.h"

using namespace sly4;

namespace {

// PPU single-precision ops as the recompiler emits them.
f32 fadds(f64 a, f64 b) { return static_cast<f32>(a + b); }
f32 fsubs(f64 a, f64 b) { return static_cast<f32>(a - b); }
f32 fmuls(f64 a, f64 c) { return static_cast<f32>(a * c); }
f32 fdivs(f64 a, f64 b) { return static_cast<f32>(a / b); }
f32 fmadds(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }   // a * c + b
f32 fmsubs(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, -b)); }  // a * c - b

f64 lf(u32 ea) { return static_cast<f64>(load<f32>(ea)); }
bool sign_of(f64 v) { return (std::bit_cast<u32>(static_cast<f32>(v)) >> 31) != 0; }  // stfs + bit 31

// TOC offsets (r2 = 0x00F50800).
constexpr u32 kTocInf = 16596, kTocNan = 16600;  // -> f32 +inf (0x00F8F050), NaN (0x00F8F060)
constexpr u32 kSqrtHalf = 16604, kOne = 16608;
constexpr u32 kP0 = 16612, kP1 = 16616, kP2 = 16620;      // series coefficients
constexpr u32 kLnLo = 16624, kLnHi = 16628;               // ln 2 split
constexpr u32 kL10a = 16632, kL10b = 16636, kL10c = 16640, kL10d = 16644, kL10e = 16648;
constexpr u32 kL2a = 16652, kL2b = 16656, kL2c = 16660;
constexpr u32 kFeDivByZero = 0x04000000, kFeInvalid = 0x20000000;

}  // namespace

// f32 Math_FLog(f32 x, s32 base)   (was named Snd_Log2)
// base 0: ln x, base > 0: log10 x, base < 0: log2 x. Zero raises
// divide-by-zero and gives -inf; negatives (and -inf) raise invalid and
// give NaN; NaN and +inf come back unchanged.
DECOMP(007062C8, Math_FLog) {
    const f64 x = farg(ctx, 0);
    const s32 base = arg<s32>(ctx, 1);
    const u32 toc = static_cast<u32>(ctx.gpr[2]);
    auto k = [&](u32 off) { return lf(toc + off); };

    Stack frame(ctx, 16);
    const u32 pex = frame.addr(), px = pex + 4;
    store<f32>(px, static_cast<f32>(x));
    const s32 cls = static_cast<s16>(static_cast<u32>(call(ctx, mfn::Math_FDunscale, {pex, px})) & 0xFFFF);

    auto invalid = [&] {
        call(ctx, mfn::Math_Feraise, {kFeInvalid});
        ret(ctx, lf(load<u32>(toc + kTocNan)));
    };
    if (cls == 1) {  // infinity
        const f64 v = lf(px);
        if (!sign_of(v)) return ret(ctx, v);
        return invalid();
    }
    if (cls == 2) return ret(ctx, lf(px));  // NaN
    if (cls == 0) {                          // zero
        call(ctx, mfn::Math_Feraise, {kFeDivByZero});
        return ret(ctx, -lf(load<u32>(toc + kTocInf)));
    }

    f64 m = lf(px);  // mantissa in [0.5, 1), exponent in *pex
    if (sign_of(m)) return invalid();
    u32 e;
    if (m < k(kSqrtHalf)) {
        m = fadds(m, m);
        e = static_cast<u32>(load<u16>(pex)) - 1;
        store<u16>(pex, static_cast<u16>(e));
        store<f32>(px, static_cast<f32>(m));
    } else {
        e = load<u16>(pex);
    }
    const s64 exponent = static_cast<s16>(e & 0xFFFF);
    const f32 num = fsubs(m, k(kOne));
    const f32 z = fdivs(num, fadds(m, k(kOne)));
    const f32 ef = static_cast<f32>(rt::fcfid(static_cast<u64>(exponent)));  // fcfid + frsp
    const f32 z2 = fmuls(z, z);
    const f32 p = fmadds(z2, fmadds(z2, k(kP0), k(kP1)), k(kP2));
    const f32 r = fmuls(z, fsubs(num, fmuls(z2, p)));

    if (base == 0) {
        const f32 t = fsubs(num, fmadds(ef, k(kLnLo), r));
        return ret(ctx, fmadds(ef, k(kLnHi), t));
    }
    if (base > 0) {
        const f32 t = fmadds(r, k(kL10c), fmuls(ef, k(kL10d)));
        const f32 u = fmsubs(num, k(kL10b), t);
        return ret(ctx, fmadds(ef, k(kL10e), fmadds(num, k(kL10a), u)));
    }
    const f32 t = fmadds(num, k(kL2b), fmuls(r, k(kL2c)));
    ret(ctx, fadds(ef, fmadds(num, k(kL2a), t)));
}

// s32 Math_FDunscale(s16* exp, f32* x)   (libm _FDunscale)
// Classifies *x: 2 NaN, 1 infinity, 0 zero (both with *exp = 0), else -1
// with *x scaled to [0.5, 1) (sign kept) and *exp the binary exponent.
// Denormals are normalised first by _FDnorm.
DECOMP(0071F668, Math_FDunscale) {
    const u32 pex = arg<u32>(ctx, 0), px = arg<u32>(ctx, 1);
    u32 hi = load<u16>(px);
    u32 xchar = (hi >> 7) & 0xFF;
    if (xchar == 255) {
        store<u16>(pex, 0);
        if ((load<u16>(px) & 0x7F) != 0) return ret(ctx, 2);
        return ret(ctx, load<u16>(px + 2) == 0 ? 1 : 2);
    }
    if (xchar == 0) {
        xchar = static_cast<u32>(call(ctx, mfn::Math_FDnorm, {px}));
        if (static_cast<s16>(xchar & 0xFFFF) > 0) {
            store<u16>(pex, 0);
            return ret(ctx, 0);
        }
        hi = load<u16>(px);
    }
    store<u16>(px, static_cast<u16>((hi & 0x807F) | 0x3F00));
    store<u16>(pex, static_cast<u16>(xchar - 126));
    ret(ctx, -1);
}
