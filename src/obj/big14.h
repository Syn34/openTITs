// Game code, large-function sweep part 14: helpers shared by big14_*.cpp
// (object matrix setters, the player's link driving and further
// still-recompiled game functions).
#pragma once

#include <bit>
#include <cmath>

#include "big13.h"

namespace sly4 {
namespace big14 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using obj19::VCall;
using obj23::LfsAt;
using obj25::Sx;
using big4::CopyW;
using big7::F;
using big7::Fmadds;
using big7::Fmsubs;
using big7::Fnmsubs;
using big7::SetF1Bits;
using big10::StfsW;
using big10::Stfs;
using big11::Adds;
using big11::Subs;
using big11::Muls;
using big11::Divs;
using big12::Fsel;
using big12::Lfd;
using big13::LfsX;

namespace b14fn {
constexpr u32 WorldObj_MarkDirty = 0x0057D000;   // (obj): reads r3 only
constexpr u32 WorldObj_UpdateLod = 0x0057EE00;   // (obj, -, s32): reads r3 / r5
constexpr u32 Quat_ToMatrix = 0x00553A90;        // (f32 out[4][4], const f32 q[4], const f32 pos[3]); w lanes untouched
}  // namespace b14fn

// The f32 word at ea, bit for bit.
inline f32 Fw(u32 ea) { return std::bit_cast<f32>(load<u32>(ea)); }

// vmaddfp lane: a * c + b, one single rounding.
inline f32 Vmadd(f32 a, f32 c, f32 b) { return std::fma(a, c, b); }

}  // namespace big14
}  // namespace sly4
