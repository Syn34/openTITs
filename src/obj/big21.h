// Game code, large-function sweep part 21: helpers shared by big21_*.cpp.
#pragma once

#include <bit>
#include <cmath>

#include "big20.h"
#include "mid16.h"

namespace sly4 {
namespace big21 {

using namespace opentit;
using namespace opentit::decomp;
using big19::VCall;
using big20::kRand;
using big20::Rng;
using mid14::Fnmsubs;
using mid15::Cw;
using mid15::Fb;
using mid15::Fmadds;
using mid15::Fmsubs;
using mid15::Lf;
using mid15::S;
using mid15::Sf;
using mid16::Divs;
using mid16::Sqrts;

namespace b21fn {
constexpr u32 Anim_Restart = 0x002775A8;        // (an, u32 key, ...12 args; f1-f4)
constexpr u32 Effect_FindComponent = 0x004BB4B4;  // (Effect*, u32 hash) -> component or 0
constexpr u32 Effect_SetHash = 0x004CF988;      // (Effect*, u32 hash)
constexpr u32 Mat34_FromRotPos = 0x00553B4C;    // (out, const f32 rows[3][4], const Vec3* pos)
constexpr u32 Quat_FromMatrix = 0x00555478;     // (Quat* out, const Mat34*)
constexpr u32 SplineMap_Find = 0x007335C4;      // (map, const u32* key) -> entry* (entry->0 the spline)
constexpr u32 CameraDefMap_Find = 0x0073374C;   // (map, const u32* key) -> entry* (entry->0 the def)
}  // namespace b21fn

}  // namespace big21
}  // namespace sly4
