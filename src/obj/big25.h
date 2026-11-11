// Game code, large-function sweep part 25: helpers shared by big25_*.cpp.
#pragma once

#include <bit>
#include <cmath>

#include "big24.h"

namespace sly4 {
namespace big25 {

using namespace opentit;
using namespace opentit::decomp;
using big19::VCall;
using big23::Adds;
using big23::Subs;
using big23::Muls;
using big24::Sw;
using big5::V4;
using big5::Lvx;
using big5::Stvx;
using big5::Row3;
using big5::Add4;
using big6::Lvlx;
using mid15::Cw;
using mid15::Fmadds;
using mid15::Fmsubs;
using mid15::Fb;
using mid15::Lf;
using mid15::S;
using mid15::Sf;
using mid16::Divs;
using mid16::Sqrts;

namespace b25fn {
constexpr u32 Math_Sinf = 0x007345F0;          // f1 -> f1
constexpr u32 Math_Cosf = 0x007344E0;          // f1 -> f1
constexpr u32 Quat_ToMatrix = 0x00553A90;      // (Mat44* out, const Quat*, const Vec3* pos)
constexpr u32 Matrix_MulRot = 0x00553FF0;      // (out, a, b)
constexpr u32 PointMap_Find = 0x0073343C;      // (map, const u32* key) -> value slot*
constexpr u32 Effect_SetHash = 0x004CF988;     // Effect_SetHash4CF988 (Effect*, u32 hash)
constexpr u32 Obj_SetEffectHash = 0x000DAEC8;  // Obj_SetEffectHash_000DAEC8 (Obj*, u32 hash)
constexpr u32 Obj_FindByHash = 0x000DB290;     // Obj_FindByHash_000DB290 (Obj*, u32 hash) -> child or 0
constexpr u32 MapHolderPoint = 0x00F8F154;     // header*; +16 the map PointMap_Find searches
constexpr u32 Game = 0x00F65CA0;               // Game*
constexpr u32 ZeroVec = 0x01164B40;
constexpr u32 UpVec = 0x01164B60;
constexpr u32 AxisVec = 0x01164B70;            // the global vector after UpVec (lever hinge axis)
}  // namespace b25fn

// fmuls with the PPC NaN order: with both operands NaN, frA's comes out
// (the host may swap the operands of a plain multiply).
inline f64 MulsA(f64 a, f64 c) {
    if (a != a) return S(std::bit_cast<f64>(std::bit_cast<u64>(a) | 0x0008000000000000ull));
    return S(a * c);
}

// fneg then fadds: (-a) + b. A NaN a comes out with its sign flipped (PPC
// returns frA quieted); spelled out because the host compiler may turn
// -a + b into b - a, which keeps a NaN's sign (only the NaN case needs it:
// the folds the compiler may make are exact for every other input).
inline f64 NegAdds(f64 a, f64 b) {
    if (a != a) return S(std::bit_cast<f64>((std::bit_cast<u64>(a) ^ 0x8000000000000000ull) | 0x0008000000000000ull));
    return S(-a + b);
}

// fneg then fmuls: (-a) * c for a non-NaN c (same NaN reasoning).
inline f64 NegMuls(f64 a, f64 c) {
    if (a != a) return S(std::bit_cast<f64>((std::bit_cast<u64>(a) ^ 0x8000000000000000ull) | 0x0008000000000000ull));
    return S(-a * c);
}
// fneg then fsubs: (-a) - c for a non-NaN c.
inline f64 NegSubs(f64 a, f64 c) {
    if (a != a) return S(std::bit_cast<f64>((std::bit_cast<u64>(a) ^ 0x8000000000000000ull) | 0x0008000000000000ull));
    return S(-a - c);
}

// The word in lane 0 of an lvlx at ea, as vspltw 0 leaves it in every lane.
inline f32 SplatLvlx(u32 ea) { return Lvlx(ea).e[0]; }

}  // namespace big25
}  // namespace sly4
