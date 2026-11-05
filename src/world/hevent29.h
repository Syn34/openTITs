// Shared names for src/world/hevent29_*.cpp: the callees of the geometry
// scene load (Geom_LoadScene4F05A8) and more of the World event handler's
// remaining callees. Guest addresses in the BCES01284 v01.02 EBOOT;
// "(name?)" marks a guess.
#pragma once

#include "hevent28.h"

namespace sly4 {
namespace hevent29 {

namespace h29fn {
constexpr u32 View_SetFov = 0x00520368;    // (View*, f32) (name?)
constexpr u32 View_SetF338 = 0x00520338;   // (View*, f32) (View_Set520338, name?)
constexpr u32 View_SetF2C8 = 0x005202C8;   // (View*, f32) (SetFactor_005202C8, name?)
}  // namespace h29fn

// lfs: a single's bits as the double the register then holds (NaNs kept
// as they are, which a C++ float -> double conversion would quiet).
inline f64 lfs_reg(u32 w) {
    if ((w & 0x7F800000u) == 0x7F800000u && (w & 0x007FFFFFu) != 0)
        return std::bit_cast<f64>((static_cast<u64>(w >> 31) << 63) | (u64{0x7FF} << 52) | (static_cast<u64>(w & 0x007FFFFFu) << 29));
    return static_cast<f64>(std::bit_cast<f32>(w));
}
inline f64 lfs_at(u32 ea) { return lfs_reg(load<u32>(ea)); }

// Property tables (read by Geom_LoadScene4F05A8): table +4 holds one Set*
// per index; a set has u16 count at +12, sorted u32 keys at +16, u16 value
// offsets at +20 (one per key) and the value block at +24.
constexpr u32 kPropSets = 4;
constexpr u32 kPropCount = 12, kPropKeys = 16, kPropOffsets = 20, kPropValues = 24;

}  // namespace hevent29
}  // namespace sly4
