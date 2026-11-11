// Game code, large-function sweep part 22: helpers shared by big22_*.cpp.
#pragma once

#include <bit>
#include <cmath>

#include "big21.h"

namespace sly4 {
namespace big22 {

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

namespace b22fn {
constexpr u32 Thunk_000E02D8 = 0x000E02D8;         // (o, QueryEx*): the base object constructor (0x000E01C0)
constexpr u32 StateMachine_SetState = 0x00464D1C;  // (o, level, id)
constexpr u32 HkBody_SetName = 0x007CE950;         // (body + 120, const char*)
constexpr u32 HkBody_AddListener = 0x0091A82C;     // (body, listener)
constexpr u32 Hk_SetTransform = 0x0091EAB8;        // (body, const f32 m[4][4])
constexpr u32 Hk_AddProperty = 0x0093FE00;         // (entity, u32 key, u64 value)
}  // namespace b22fn

// A 16-byte vector row {x, y, z, 0} from three words at src (the lvlx /
// vspltw / vsel / vandc sequence with the masks {0,~0,0,0}, {0,0,~0,0},
// {0,0,0,~0}).
inline void Row3(u32 dst, u32 src) {
    Cw(dst, src);
    Cw(dst + 4, src + 4);
    Cw(dst + 8, src + 8);
    store<u32>(dst + 12, 0);
}

}  // namespace big22
}  // namespace sly4
