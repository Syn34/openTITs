// Game code, medium sweep part 31: helpers shared by mid31_*.cpp (more
// still-recompiled game functions of 120-400 instructions).
#pragma once

#include <bit>
#include <cmath>
#include <initializer_list>
#include <utility>

#include "mid30.h"

namespace sly4 {
namespace mid31 {

using namespace opentit;
using namespace opentit::decomp;
using mid14::Lf;
using mid14::LfsF;
using mid14::Fmadds;
using mid14::Fmsubs;
using mid14::Fnmadds;
using mid14::Fnmsubs;
using mid6::W32;
using obj19::VCall;
using mid28::Sgl28;
using mid28::Bits28;
using mid28::Fsel28;
using mid30::Ld30;

namespace m31fn {
using namespace mid30::m30fn;
constexpr u32 ObjCheck31 = 0x001AEA50u;     // Obj_Check_001AEA50(o; f1 dt) -> bool
constexpr u32 ObjById31 = 0x000D6A28u;      // Obj_FindById(id) -> object or 0
constexpr u32 FindComp31 = 0x004BB4B4u;     // Effect_FindComponent(e, hash)
constexpr u32 SplMapFind31 = 0x007335C4u;   // SplineMap_Find(map, const u32* key) -> entry*
constexpr u32 kSplineSlot31 = 0x00F8F158u;  // g_SplineInstanceAllocSlot (+16: the spline map)
constexpr u32 RelChild31 = 0x001ED85Cu;     // Obj_ReleaseChild_001ED85C(o, child)
}  // namespace m31fn

// fmuls: the product of two singles, rounded once to single.
inline f64 Fmuls31(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a * b)); }

}  // namespace mid31
}  // namespace sly4
