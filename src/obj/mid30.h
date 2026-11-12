// Game code, medium sweep part 30: helpers shared by mid30_*.cpp (more
// still-recompiled game functions of 120-400 instructions).
#pragma once

#include <bit>
#include <cmath>
#include <initializer_list>
#include <utility>

#include "mid29.h"
#include "obj6.h"

namespace sly4 {
namespace mid30 {

using namespace opentit;
using namespace opentit::decomp;
using mid14::Lf;
using mid14::LfsF;
using mid6::W32;
using obj19::VCall;
using mid28::Sgl28;
using mid28::Bits28;
using mid28::Fsel28;

namespace m30fn {
using namespace mid29::m29fn;
constexpr u32 XmlFirst30 = 0x004F92BCu;  // XmlCursor_FirstChild(cursor, tag)
constexpr u32 XmlNext30 = 0x004F8FC8u;   // XmlCursor_NextSibling(cursor, tag)
constexpr u32 XmlBool30 = 0x004F986Cu;   // XmlCursor_ReadBool(cursor, key, u8* out, default)
constexpr u32 XmlS32_30 = 0x004F99BCu;   // XmlCursor_ReadS32(cursor, key, s32* out, default)
constexpr u32 XmlF32_30 = 0x004F9B20u;   // XmlCursor_ReadF32(cursor, key, f32* out; f1 default)
constexpr u32 XmlHash30 = 0x004FA14Cu;   // XmlCursor_ReadHash(cursor, key, u32* out, default)
constexpr u32 kGame30 = 0x00F557F0u;     // the game-state pointer
}  // namespace m30fn

// lfd: the f64 at ea.
inline f64 Ld30(u32 ea) { return std::bit_cast<f64>(load<u64>(ea)); }

}  // namespace mid30
}  // namespace sly4
