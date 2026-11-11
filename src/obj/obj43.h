// Game code, part 43: helpers shared by obj43*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from the remaining
// 91 instruction ones up).
#pragma once

#include <bit>
#include <cmath>

#include "obj42.h"

namespace sly4 {
namespace obj43 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj42;

// The raw f32 word of a register value (stfs).
inline u32 W43(f64 x) { return std::bit_cast<u32>(static_cast<f32>(x)); }
// lfs: the f32 at ea as a double.
inline f64 Lfs43(u32 ea) { return static_cast<f64>(load<f32>(ea)); }
// Single-precision rounding (the s-form result).
inline f64 F43(f64 x) { return static_cast<f64>(static_cast<f32>(x)); }

// One step of the game's 16-bit random generator (the inlined Rand16 of
// 0x01164888): returns the previous dword value (a u16) as an f32 and
// advances the state: x = (rotl(w, 4) + rotl(w, 16)) ^ 0x87654321 of the
// word w at +8; +8 = rotl(x, 16), the dword +0 = x & 0xFFFF.
inline f64 Rand16Step43() {
    const u32 g = 0x01164888u;
    const s64 prev = static_cast<s64>(load<u64>(g));
    const u32 w = load<u32>(g + 8);
    const u32 x = (std::rotl(w, 4) + std::rotl(w, 16)) ^ 0x87654321u;
    store<u32>(g + 8, std::rotl(x, 16));
    store<u64>(g, static_cast<u64>(x & 0xFFFFu));
    return static_cast<f64>(static_cast<f32>(static_cast<f64>(prev)));
}

}  // namespace obj43
}  // namespace sly4
