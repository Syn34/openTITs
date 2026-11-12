// Game code, large-function sweep part 16: helpers shared by big16_*.cpp
// (the last damage / collision def readers of the Def_Construct74FD80
// family and further still-recompiled large game functions).
#pragma once

#include <bit>
#include <cmath>

#include "big15.h"

namespace sly4 {
namespace big16 {

using namespace opentit;
using namespace opentit::decomp;
using big10::Stfs;
using big10::StfsW;
using big12::Fsel;
using big12::Lfd;
using big13::LfsX;
using big15::Bit;
using big15::kVecZero;
using big15::Ds;
using big15::Mas;
using big15::Ms;
using big15::RdF32;
using big15::Sqrts;
using big15::Sx64;
using big4::CopyW;
using mid11::DmgCtor;
using mid11::DmgDestroy;

namespace b16fn {
using namespace big15::b15fn;
constexpr u32 XmlCursor_ReadVec2 = 0x004F9C9C;        // (c, hash, f32 out[2], const f32 default[2]) -> status
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;  // (c, const char* name) -> value string or 0
constexpr u32 String_Length = 0x00555FA4;             // strlen
constexpr u32 String_Alloc = 0x0055578C;              // (n) -> char*
constexpr u32 String_CopyN = 0x00553638;              // (dst, src, n)
constexpr u32 Mem_FreeIfSet = 0x00555CC0;             // (p)
constexpr u32 Math_Cosf = 0x007344E0;                 // f1 -> f1
constexpr u32 Math_Sinf = 0x007345F0;                 // f1 -> f1
}  // namespace b16fn

// (f32)(a - b), an fsubs.
inline f64 Ss(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a - b)); }
// frsp of a double (no NaN handling needed where used: the inputs are the
// results of an fsel against a constant).
inline f64 Frsp(f64 a) { return static_cast<f64>(static_cast<f32>(a)); }

// The f32 at ea scaled by k (an lfs / fmuls / stfs).
inline void ScaleAt(u32 ea, f64 k) { Stfs(ea, Ms(LfsX(ea), k)); }

}  // namespace big16
}  // namespace sly4

// ---------------------------------------------------------------- native
#include "native/objtypes.h"
