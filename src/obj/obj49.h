// Game code, part 49: helpers shared by obj49*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from the remaining
// 107 instruction ones up).
#pragma once

#include <bit>
#include <cmath>

#include "obj48.h"

namespace sly4 {
namespace obj49 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj48;

// Callees used throughout.
constexpr u32 kSkelRelease49 = 0x007D4EA8u;  // AnimSkel_Release7D4EA8 (p)

// Single-precision helpers: fsubs, fmuls, fmadds (fused), fsqrts.
inline f64 Fs49(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a - b)); }
inline f64 Fm49(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a * b)); }
inline f64 Fma49(f64 a, f64 c, f64 b) { return static_cast<f64>(static_cast<f32>(std::fma(a, c, b))); }
inline f64 Sqrt49(f64 x) { return static_cast<f64>(static_cast<f32>(std::sqrt(x))); }
// |a - b| for two vec3 in memory, as the original computes it: dy*dy, then
// dx*dx + that, then dz*dz + that (fused, single), then fsqrts.
inline f64 Dist49(u32 a, u32 b) {
    const f64 dy = Fs49(Lfs48(a + 4), Lfs48(b + 4));
    const f64 dx = Fs49(Lfs48(a), Lfs48(b));
    const f64 dz = Fs49(Lfs48(a + 8), Lfs48(b + 8));
    return Sqrt49(Fma49(dz, dz, Fma49(dx, dx, Fm49(dy, dy))));
}

// Index of the 16-byte property with key k in the Havok body s (count +128,
// array +124), scanning from 0; -1 when absent.
inline s32 HkProp_Find49(u32 s, u32 k, bool count_once) {
    const s32 n0 = static_cast<s32>(load<u32>(s + 128));
    for (s32 j = 0;; ++j) {
        const s32 n = count_once ? n0 : static_cast<s32>(load<u32>(s + 128));
        if (!(j < n)) return -1;
        if (load<u32>(load<u32>(s + 124) + 16 * static_cast<u32>(j)) == k) return j;
    }
}

}  // namespace obj49
}  // namespace sly4
