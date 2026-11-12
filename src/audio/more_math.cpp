// libm float logarithm (_FLog) and its _FDunscale helper, as compiled into
// the EBOOT. The constants come from the TOC, as in the original.
#include <bit>
#include <cmath>

#include "more.h"
#include "snd.h"

namespace sly4::snd {
using namespace opentit::native;

namespace {

f32 fmadds(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }   // a * c + b
f32 fmsubs(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, -b)); }  // a * c - b
bool sign_of(f64 v) { return (std::bit_cast<u32>(static_cast<f32>(v)) >> 31) != 0; }  // stfs + bit 31

// TOC offsets (r2 = 0x00F50800).
constexpr u32 kToc = 0x00F50800;
constexpr u32 kTocInf = 16596, kTocNan = 16600;  // -> f32 +inf (0x00F8F050), NaN (0x00F8F060)
constexpr u32 kSqrtHalf = 16604, kOne = 16608;
constexpr u32 kP0 = 16612, kP1 = 16616, kP2 = 16620;      // series coefficients
constexpr u32 kLnLo = 16624, kLnHi = 16628;               // ln 2 split
constexpr u32 kL10a = 16632, kL10b = 16636, kL10c = 16640, kL10d = 16644, kL10e = 16648;
constexpr u32 kL2a = 16652, kL2b = 16656, kL2c = 16660;
constexpr u32 kFeDivByZero = 0x04000000, kFeInvalid = 0x20000000;

f64 k(u32 off) { return static_cast<f64>(global<be_f32>(kToc + off).get()); }
f64 kp(u32 off) { return static_cast<f64>(global<be_f32>(global<be_u32>(kToc + off)).get()); }

// The float being unscaled: two halfwords (sign/exponent/top mantissa, rest).
struct FloatHalves {
    be_u16 hi;  // +0
    be_u16 lo;  // +2
};

}  // namespace

// f32 Math_FLog(f32 x, s32 base)   (was named Snd_Log2)
// base 0: ln x, base > 0: log10 x, base < 0: log2 x. Zero raises
// divide-by-zero and gives -inf; negatives (and -inf) raise invalid and
// give NaN; NaN and +inf come back unchanged.
f32 Math_FLog(f32 x, s32 base) {
    GuestFrame frame(16);
    be_u16* pex = frame.as<be_u16>();
    be_f32* px = frame.as<be_f32>(4);
    *px = x;
    const s32 cls = static_cast<s16>(static_cast<u32>(Math_FDunscale(pex, reinterpret_cast<be_u16*>(px))) & 0xFFFF);

    auto invalid = [&] {
        call(mfn::Math_Feraise, kFeInvalid);
        return static_cast<f32>(kp(kTocNan));
    };
    if (cls == 1) {  // infinity
        const f64 v = px->get();
        if (!sign_of(v)) return static_cast<f32>(v);
        return invalid();
    }
    if (cls == 2) return px->get();  // NaN
    if (cls == 0) {                  // zero
        call(mfn::Math_Feraise, kFeDivByZero);
        return static_cast<f32>(-kp(kTocInf));
    }

    f64 m = px->get();  // mantissa in [0.5, 1), exponent in *pex
    if (sign_of(m)) return invalid();
    u32 e;
    if (m < k(kSqrtHalf)) {
        m = fadds(m, m);
        e = static_cast<u32>(pex->get()) - 1;
        *pex = static_cast<u16>(e);
        *px = static_cast<f32>(m);
    } else {
        e = *pex;
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
        return fmadds(ef, k(kLnHi), t);
    }
    if (base > 0) {
        const f32 t = fmadds(r, k(kL10c), fmuls(ef, k(kL10d)));
        const f32 u = fmsubs(num, k(kL10b), t);
        return fmadds(ef, k(kL10e), fmadds(num, k(kL10a), u));
    }
    const f32 t = fmadds(num, k(kL2b), fmuls(r, k(kL2c)));
    return fadds(ef, fmadds(num, k(kL2a), t));
}

// s32 Math_FDunscale(s16* exp, f32* x)   (libm _FDunscale)
// Classifies *x: 2 NaN, 1 infinity, 0 zero (both with *exp = 0), else -1
// with *x scaled to [0.5, 1) (sign kept) and *exp the binary exponent.
// Denormals are normalised first by _FDnorm.
s32 Math_FDunscale(be_u16* pex, be_u16* pxw) {
    FloatHalves& x = gref(reinterpret_cast<FloatHalves*>(pxw));
    be_u16& ex = gref(pex);
    u32 hi = x.hi;
    u32 xchar = (hi >> 7) & 0xFF;
    if (xchar == 255) {
        ex = 0;
        if ((x.hi & 0x7F) != 0) return 2;
        return x.lo == 0 ? 1 : 2;
    }
    if (xchar == 0) {
        xchar = call<u32>(mfn::Math_FDnorm, pxw);
        if (static_cast<s16>(xchar & 0xFFFF) > 0) {
            ex = 0;
            return 0;
        }
        hi = x.hi;
    }
    x.hi = static_cast<u16>((hi & 0x807F) | 0x3F00);
    ex = static_cast<u16>(xchar - 126);
    return -1;
}

}  // namespace sly4::snd

NATIVE(007062C8, sly4::snd::Math_FLog)
NATIVE(0071F668, sly4::snd::Math_FDunscale)
