// Game code, large-function sweep part 2: helpers shared by big2_*.cpp
// (still recompiled game functions of 400+ instructions: the remaining
// SlotDef constructors, ...).
#pragma once

#include <bit>
#include <cmath>

#include "big1.h"

namespace sly4 {
namespace big2 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using obj19::VCall;
using obj25::Sx;
using big1::ReadHash;
using big1::ReadBool;
using big1::ReadF32;
using big1::ReadColor;

namespace b2fn {
using namespace big1::b1fn;
constexpr u32 XmlCursor_ReadVec3 = 0x004F9E0C;  // (cursor, hash, f32 out[3], const f32 default[3])
}  // namespace b2fn

// XmlCursor_ReadS32 with the field's current value as the default.
inline void ReadS32(PPUContext& ctx, u32 cur, u32 hash, u32 p) {
    call(ctx, b2fn::XmlCursor_ReadS32, {cur, hash, p, Sx(load<u32>(p))});
}

}  // namespace big2
}  // namespace sly4

// ---------------------------------------------------------------- native
#include "native/objtypes.h"
