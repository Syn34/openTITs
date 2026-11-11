// Game code, large-function sweep part 10: helpers shared by big10_*.cpp
// (callees part 9 named and further still-recompiled game functions of
// 400+ instructions).
#pragma once

#include <bit>
#include <cmath>

#include "big9.h"

namespace sly4 {
namespace big10 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using obj19::VCall;
using obj25::Sx;
using big4::CopyW;
using big5::V4;
using big7::F;
using big7::Fmadds;
using big7::Fmsubs;
using big7::SetF1Bits;
using big9::FBits;
using mid1::PtrVecPush;

// stfs of an FPR: rounded to single, NaNs keep their top payload bits.
inline u32 StfsW(f64 v) {
    const u64 d = std::bit_cast<u64>(v);
    if (std::isnan(v)) return static_cast<u32>(((d >> 63) << 31) | (0xFFu << 23) | ((d >> 29) & 0x7FFFFF));
    return std::bit_cast<u32>(static_cast<f32>(v));
}
inline void Stfs(u32 ea, f64 v) { store<u32>(ea, StfsW(v)); }

// ++ of a u16 reference count at +178 (CamTarget's referenced objects).
inline void AddRef178(u32 o) { store<u16>(o + 178, static_cast<u16>(load<u16>(o + 178) + 1)); }

}  // namespace big10
}  // namespace sly4
