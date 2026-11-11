// Game code, large-function sweep part 6: helpers shared by big6_*.cpp
// (still recompiled game functions of 400+ instructions and the callees
// part 5 named).
#pragma once

#include <bit>
#include <cmath>

#include "big5.h"

namespace sly4 {
namespace big6 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using obj19::VCall;
using obj25::Sx;
using big4::CopyW;
using big4::CopyWords;
using big5::V4;
using big5::Lvx;
using big5::Stvx;
using big5::Row3;
using big5::Add4;
using big5::CopyMat43W;

namespace b6fn {
using namespace big5::b5fn;
}  // namespace b6fn

// The value of an lfs of the word at ea, as f1 holds it afterwards.
inline void SetF1FromWord(PPUContext& ctx, u32 ea) { ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(ea))); }
inline void SetFFromWord(PPUContext& ctx, int f, u32 ea) { ctx.fpr[f] = std::bit_cast<f64>(LfsBits(load<u32>(ea))); }

// VMX lane helpers (V4 lanes in guest order).
inline u32 LaneW(const V4& v, u32 k) { return std::bit_cast<u32>(v.e[k]); }
inline V4 Splat(const V4& v, u32 k) { return V4{{v.e[k], v.e[k], v.e[k], v.e[k]}}; }
inline V4 Mrghw(const V4& a, const V4& b) { return V4{{a.e[0], b.e[0], a.e[1], b.e[1]}}; }
inline V4 Mrglw(const V4& a, const V4& b) { return V4{{a.e[2], b.e[2], a.e[3], b.e[3]}}; }
// vperm: byte c & 31 of the 32 bytes a:b, for each byte c of the mask.
inline V4 Perm(const V4& a, const V4& b, const V4& m) {
    u8 ab[32], mb[16];
    for (u32 k = 0; k < 4; ++k)
        for (u32 j = 0; j < 4; ++j) {
            ab[4 * k + j] = static_cast<u8>(LaneW(a, k) >> (24 - 8 * j));
            ab[16 + 4 * k + j] = static_cast<u8>(LaneW(b, k) >> (24 - 8 * j));
            mb[4 * k + j] = static_cast<u8>(LaneW(m, k) >> (24 - 8 * j));
        }
    V4 o;
    for (u32 k = 0; k < 4; ++k) {
        u32 w = 0;
        for (u32 j = 0; j < 4; ++j) w = (w << 8) | ab[mb[4 * k + j] & 31];
        o.e[k] = std::bit_cast<f32>(w);
    }
    return o;
}
// lvlx: the bytes from ea to the end of its 16-byte block, left-justified,
// the rest zero.
inline V4 Lvlx(u32 ea) {
    u8 b[16] = {};
    const u32 n = 16 - (ea & 15);
    for (u32 k = 0; k < n; ++k) b[k] = load<u8>(ea + k);
    V4 v;
    for (u32 k = 0; k < 4; ++k)
        v.e[k] = std::bit_cast<f32>((u32(b[4 * k]) << 24) | (u32(b[4 * k + 1]) << 16) | (u32(b[4 * k + 2]) << 8) | b[4 * k + 3]);
    return v;
}
// The 12 bytes of each of four 16-byte rows equal (the original compares
// them as a u64 and a u32 per row).
inline bool Rows12Equal(u32 a, u32 b) {
    for (u32 r = 0; r < 4; ++r)
        if (load<u64>(a + 16 * r) != load<u64>(b + 16 * r) || load<u32>(a + 16 * r + 8) != load<u32>(b + 16 * r + 8)) return false;
    return true;
}
// vsubfp a - b.
inline V4 Sub4(const V4& a, const V4& b) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = a.e[k] - b.e[k];
    return o;
}

}  // namespace big6
}  // namespace sly4
