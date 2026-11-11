// Game code, large-function sweep part 5: helpers shared by big5_*.cpp
// (still recompiled game functions of 400+ instructions and the callees
// part 4 named: the effect objects' bone matrix, geometry def and bone
// tables, ...).
#pragma once

#include <bit>
#include <cmath>

#include "big4.h"

namespace sly4 {
namespace big5 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using obj19::VCall;
using obj25::Sx;
using big4::CopyW;
using big4::CopyWords;

namespace b5fn {
using namespace big4::b4fn;
constexpr u32 kStringFormat = 0x005534D0;      // (fmt, ...) -> rotating static buffer
constexpr u32 kWorldObjSetName = 0x0057D368;   // (wo, const char* name, u8)
constexpr u32 kWorldObjInstance = 0x005A05E8;  // (wo, u32) -> a new instance
constexpr u32 kFindEntry = 0x00537B7C;         // (Entry* table, u32 key, u32 kind) -> Entry* or 0
constexpr u32 kMemGetDefaultStack = 0x005525FC;  // () -> ApeMemStack*
constexpr u32 kMemStackAlloc = 0x0055260C;     // (stack, size) -> p
constexpr u32 kMemDefaultAllocator = 0x00550EE8;  // () -> Allocator*
constexpr u32 kArrayInit = 0x00558F38;         // (Array*): empty
constexpr u32 kArrayReserve = 0x00558F3C;      // (Array*, u16 capacity)
constexpr u32 kArrayDestruct = 0x0054EC94;     // (Array*): empty
constexpr u32 kQsort = 0x0071EC28;             // (base, count, size, Opd* compare)
}  // namespace b5fn

// A VMX register's four f32 lanes (guest order).
struct V4 {
    f32 e[4];
};
// lvx: the aligned quadword at ea & ~15.
inline V4 Lvx(u32 ea) {
    V4 v;
    for (u32 k = 0; k < 4; ++k) v.e[k] = load<f32>((ea & ~15u) + 4 * k);
    return v;
}
// stvx: the four lanes to ea & ~15 (one 32-bit store per lane, in order).
inline void Stvx(u32 ea, const V4& v) {
    for (u32 k = 0; k < 4; ++k) store<f32>((ea & ~15u) + 4 * k, v.e[k]);
}
// The 12 x/y/z words of a 4x4 f32 matrix (rows at +0/+16/+32/+48; the w
// lanes are left alone), copied as raw words in row order.
inline void CopyMat43W(u32 dst, u32 src) {
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(dst + 16 * r + 4 * k, src + 16 * r + 4 * k);
}
// vmaddfp chain for one row: c2 * s2 + (c1 * s1 + (c0 * s0 + 0)), lane by
// lane (fused, single precision).
inline V4 Row3(const V4& c0, const V4& c1, const V4& c2, f32 s0, f32 s1, f32 s2) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) {
        f32 x = std::fma(c0.e[k], s0, 0.0f);
        x = std::fma(c1.e[k], s1, x);
        o.e[k] = std::fma(c2.e[k], s2, x);
    }
    return o;
}
// vaddfp a + b.
inline V4 Add4(const V4& a, const V4& b) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = a.e[k] + b.e[k];
    return o;
}

}  // namespace big5
}  // namespace sly4
