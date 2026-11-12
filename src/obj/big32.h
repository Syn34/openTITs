// Game code, large-function sweep part 32: helpers shared by big32_*.cpp.
#pragma once

#include <bit>
#include <cmath>

#include "big31.h"

namespace sly4 {
namespace big32 {

using namespace opentit;
using namespace opentit::decomp;
using big31::Fsel31;
using big31::Lfd31;
using big31::Lfs;
using big29::Adds;
using big29::Divs;
using big29::Fb;
using big29::Fmadds;
using big29::Fmsubs;
using big29::Muls;
using big29::S;
using big29::Sqrts;
using big29::Subs;
using big29::StackArg;
using big5::Add4;
using big5::Lvx;
using big5::Row3;
using big5::Stvx;
using big5::V4;
using big19::VCall;

// The f32 a 32-bit load of ea holds (a VMX lane).
inline f32 W32f(u32 ea) { return std::bit_cast<f32>(load<u32>(ea)); }
// The 12 bytes from src to dst as three raw words (lfs / stfs pairs).
inline void Copy3W(u32 dst, u32 src) {
    for (u32 k = 0; k < 12; k += 4) store<u32>(dst + k, load<u32>(src + k));
}
// The first three lanes of v to dst as raw words (stvx to the stack, then
// lfs / stfs pairs).
inline void Store3W(u32 dst, const V4& v) {
    for (u32 k = 0; k < 3; ++k) store<u32>(dst + 4 * k, std::bit_cast<u32>(v.e[k]));
}

}  // namespace big32
}  // namespace sly4
