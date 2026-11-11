// Game code, medium sweep part 13: helpers shared by mid13_*.cpp (still
// recompiled game functions of 120-400 instructions: def readers, factories
// and other game code).
#pragma once

#include <bit>
#include <cmath>
#include <initializer_list>
#include <utility>

#include "mid12.h"

namespace sly4 {
namespace mid13 {

using namespace opentit;
using namespace opentit::decomp;
using mid10::LoadF;
using mid10::PutBit;
using mid10::SArg;
using mid10::StoreF;
using mid12::CopyOne;
using mid12::CopySeq;
using mid12::Cp;
using mid12::MulS;
using mid12::NodeTag;
using mid12::SetF1;
using mid6::F32;
using mid6::W32;
using mid9::CopyW;
namespace m1fn = mid1::m1fn;
namespace m11fn = mid11::m11fn;
namespace m12fn = mid12::m12fn;
namespace xfn = mid5::xfn;

namespace m13fn {
constexpr u32 Damage_Read0C3910 = 0x000C3910;   // (a0, c, x, tmpl, o): the damage def base reader
constexpr u32 Comp_GetF32_922918 = 0x00922918;  // (body + 224) -> f1
constexpr u32 Xml_ReadEnum = 0x004FA458;        // (c, hash, u32* out, const u32* table, n, u32 default)
constexpr u32 ReturnK_004440E8 = 0x004440E8;    // (x) -> 0x00AE4674
}  // namespace m13fn

constexpr u32 kStrEmpty637C = 0x00AC637C;  // "" (one of several empty strings)

// The double an lfs of the word w leaves (bit exact, NaNs not quieted).
inline f64 LfsF(u32 w) { return std::bit_cast<f64>(mid1::LfsBits(w)); }

// f32[3] *= k for a just-read vec3 (three fmuls, stored in order).
inline void ScaleVec3(u32 v, f64 k) {
    const f64 a = MulS(LoadF(v), k), b = MulS(LoadF(v + 4), k), c = MulS(LoadF(v + 8), k);
    StoreF(v, a);
    StoreF(v + 4, b);
    StoreF(v + 8, c);
}

}  // namespace mid13
}  // namespace sly4
