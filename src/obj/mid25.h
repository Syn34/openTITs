// Game code, medium sweep part 25: helpers shared by mid25_*.cpp (more
// still-recompiled game functions of 120-400 instructions: a family of
// world-object destructors and other candidates).
#pragma once

#include <bit>
#include <cmath>
#include <initializer_list>
#include <utility>

#include "mid24.h"

namespace sly4 {
namespace mid25 {

using namespace opentit;
using namespace opentit::decomp;
using mid14::ClampS;
using mid14::Cw;
using mid14::Fmadds;
using mid14::Fmsubs;
using mid14::Fnmadds;
using mid14::Fnmsubs;
using mid14::Lf;
using mid14::S;
using mid13::LfsF;
using mid15::Fb;
using mid15::Ld;
using mid15::Sf;
using mid16::Divs;
using mid16::Sqrts;
using mid17::CopyVec3;
using mid6::F32;
using mid6::W32;
using obj19::VCall;

namespace m25fn {
constexpr u32 Mem_Alloc = 0x0054ED38u;            // (size, pool, align)
constexpr u32 Mem_FreeFrom = 0x0054EE6Cu;         // (p, pool)
constexpr u32 SmallAlloc = 0x0055238Cu;           // (size)
constexpr u32 SmallFree = 0x00552420u;            // (p, size)
constexpr u32 PtrVector_Clear = 0x0054EC1Cu;      // (vec)
constexpr u32 Nop = 0x0054EC18u;                  // (p)
constexpr u32 World_ListRemove = 0x0046B7A4u;     // World_ListRemove_0046B7A4(world, kind, tbl, o, 0)
constexpr u32 Effect_Release = 0x004CF3C4u;       // (effect, hash)
constexpr u32 Effect_SetHash = 0x004CF988u;       // Effect_SetHash4CF988(effect, hash)
constexpr u32 DtorTrivial_B21B58 = 0x004A0F70u;   // VtB21B58_DtorTrivial_004A0F70(p)
constexpr u32 Obj_Set4E3168 = 0x004E3168u;        // (const char* source, u32 key)
}  // namespace m25fn

constexpr u32 kWorld25 = 0x00F65CA0u;  // the world whose lists objects are registered in

}  // namespace mid25
}  // namespace sly4
