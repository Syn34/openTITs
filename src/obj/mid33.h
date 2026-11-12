// Game code, medium sweep part 33: helpers shared by mid33_*.cpp (more
// still-recompiled game functions of 120-400 instructions).
#pragma once

#include <bit>
#include <cmath>
#include <initializer_list>
#include <utility>

#include "mid32.h"

namespace sly4 {
namespace mid33 {

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
using mid31::Fmuls31;

namespace m33fn {
using namespace mid32::m32fn;
constexpr u32 Memset33 = 0x00726E54u;         // memset(p, c, n)
constexpr u32 TrophyHas33 = 0x004FB56Cu;      // Trophy_Has(trophy, i) -> bool
constexpr u32 TrophyUnlock33 = 0x004FB4A0u;   // Trophy_Unlock4FB4A0(trophy, i)
constexpr u32 StrAssign33 = 0x00555F80u;      // String_Assign (strcpy)
constexpr u32 StrCompare33 = 0x00555D94u;     // String_Compare (strcmp)
constexpr u32 StrCopyN33 = 0x00553638u;       // String_CopyN(d, s, n)
constexpr u32 kTrophyPtr33 = 0x00F668E8u;     // the Trophy object pointer
constexpr u32 FindComp33 = 0x004BB4B4u;       // Effect_FindComponent(set, hash) -> component or 0
constexpr u32 SetHash33 = 0x004CF988u;        // Effect_SetHash4CF988(set, hash)
constexpr u32 ObjFindComp33 = 0x000D1288u;    // Obj_FindComponent(o, type, after) -> next or 0
constexpr u32 SetField20_33 = 0x00276558u;    // Obj_SetField20_00276558(o, k)
constexpr u32 kWorld33 = 0x00F65CA0u;         // the world pointer
}  // namespace m33fn

// The inlined random step on 0x01164888 (+0 u64 last 16-bit value, +8 u32
// seed): the seed becomes rotl16(mix), the u64 mix & 0xFFFF (stw +8 then
// std +0). Returns the old u64.
inline u64 RandStep33() {
    const u64 old = load<u64>(0x01164888u);
    const u32 s = load<u32>(0x01164890u);
    const u32 x = (std::rotl(s, 4) + std::rotl(s, 16)) ^ 0x87654321u;
    store<u32>(0x01164890u, std::rotl(x, 16));
    store<u64>(0x01164888u, x & 0xFFFFu);
    return old;
}

}  // namespace mid33
}  // namespace sly4
