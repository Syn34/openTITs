// Game code, large-function sweep part 25: helpers shared by big25_*.cpp.
#pragma once

#include <bit>

#include "native/objtypes.h"

namespace sly4::obj {

// fmuls with the PPC NaN order: with both operands NaN, frA's comes out
// (the host may swap the operands of a plain multiply).
inline f64 MulsA(f64 a, f64 c) {
    if (a != a) return S(std::bit_cast<f64>(std::bit_cast<u64>(a) | 0x0008000000000000ull));
    return S(a * c);
}
// fneg then fadds / fmuls / fsubs: (-a) op b. A NaN a comes out with its
// sign flipped (PPC returns frA quieted); spelled out because the host
// compiler may turn -a + b into b - a, which keeps a NaN's sign (only the
// NaN case needs it: the folds are exact for every other input; b / c
// non-NaN for the last two).
inline f64 NegNaN(f64 a) { return S(std::bit_cast<f64>((std::bit_cast<u64>(a) ^ 0x8000000000000000ull) | 0x0008000000000000ull)); }
inline f64 NegAdds(f64 a, f64 b) { return a != a ? NegNaN(a) : S(-a + b); }
inline f64 NegMuls(f64 a, f64 c) { return a != a ? NegNaN(a) : S(-a * c); }
inline f64 NegSubs(f64 a, f64 c) { return a != a ? NegNaN(a) : S(-a - c); }

}  // namespace sly4::obj
