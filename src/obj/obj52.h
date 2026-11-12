// Game code, part 52: helpers shared by obj52*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from the remaining
// 115 instruction ones up).
#pragma once

#include <bit>
#include <cmath>

#include "obj51.h"

namespace sly4 {
namespace obj52 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj51;

// Single precision rounding of a double (the s-forms).
inline f64 F52(f64 x) { return static_cast<f64>(static_cast<f32>(x)); }

// lfs as a double.
inline f64 Lfs52(u32 ea) { return static_cast<f64>(load<f32>(ea)); }

// Raw 32-bit copy (an lfs / stfs pair).
inline void Cp52(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }

// fneg on an lfs / stfs pair: the f32 with its sign flipped (bit-exact).
inline u32 Neg52(u32 w) { return std::bit_cast<u32>(-std::bit_cast<f32>(w)); }

}  // namespace obj52
}  // namespace sly4
