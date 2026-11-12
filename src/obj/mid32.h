// Game code, medium sweep part 32: helpers shared by mid32_*.cpp (more
// still-recompiled game functions of 120-400 instructions).
#pragma once

#include <bit>
#include <cmath>
#include <initializer_list>
#include <utility>

#include "mid31.h"

namespace sly4 {
namespace mid32 {

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

namespace m32fn {
using namespace mid31::m31fn;
constexpr u32 MemAlloc32 = 0x0054ED38u;       // Mem_Alloc(size, pool, align)
constexpr u32 MemFree32 = 0x0054EE6Cu;        // Mem_FreeFrom(p, pool)
constexpr u32 StrLen32 = 0x00555FA4u;         // String_Length(s)
constexpr u32 FormatN32 = 0x00555CF4u;        // String_FormatN(buf, size, fmt, ...)
constexpr u32 NameIfSet32 = 0x00571DA0u;      // Obj_NameIfSet(handle) -> name or 0
constexpr u32 Qsort32 = 0x0071EC28u;          // qsort(base, n, size, cmp OPD)
}  // namespace m32fn

}  // namespace mid32
}  // namespace sly4
