// Game code, large-function sweep part 31: helpers shared by big31_*.cpp.
#pragma once

#include <bit>
#include <cmath>

#include "big30.h"

namespace sly4 {
namespace big31 {

using namespace opentit;
using namespace opentit::decomp;
using big29::LfsBits;
using big29::SetFk;
using big29::StackArg;
using big19::VCall;
using big29::Adds;
using big29::Divs;
using big29::Fb;
using big29::Fmadds;
using big29::Fmsubs;
using big29::Lf;
using big29::LoadMasks3;
using big29::Muls;
using big29::RowSel;
using vmx2::RowSelW;
using big29::S;
using big29::Sqrts;
using big29::Stvx;
using big29::Subs;
using big29::Fill4;

namespace b31fn {
constexpr u32 Text_Lookup34A398 = 0x0034A398;   // (u32 id) -> text entry or 0
constexpr u32 Speaker_Say3514AC = 0x003514AC;   // (Speaker*, text, u8, -, -, -, r9, r10, voice; f1-f3)
constexpr u32 Obj_NotifyA362F1C = 0x00362F1C;   // (Obj*, u8 a, u8 b)
constexpr u32 SortedMap_Find3 = 0x0076611C;     // (map, const u32* key) -> value slot
constexpr u32 kHandleMap = 0x0106C228;          // SortedMap: handle -> Obj*
}  // namespace b31fn

// fsel: a >= 0 ? b : c (a NaN gives c).
inline f64 Fsel31(f64 a, f64 b, f64 c) { return a >= 0.0 ? b : c; }
// The f64 an lfd of the doubleword at ea leaves in an FPR.
inline f64 Lfd31(u32 ea) { return std::bit_cast<f64>(load<u64>(ea)); }

// The f64 an lfs of the word at ea leaves in an FPR.
inline f64 Lfs(u32 ea) { return std::bit_cast<f64>(LfsBits(load<u32>(ea))); }

}  // namespace big31
}  // namespace sly4
