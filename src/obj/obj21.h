// Game code, part 21: helpers shared by the sweep of small functions in
// obj21*.cpp.
#pragma once

#include <bit>
#include <cmath>

#include "../sly4.h"

namespace sly4 {
namespace obj21 {

using namespace opentit;
using namespace opentit::decomp;

// rlwimi-style insert: the word at ea with the bits of `mask` taken from v.
inline void InsertBits32(u32 ea, u32 mask, u32 v) { store<u32>(ea, (load<u32>(ea) & ~mask) | (v & mask)); }
inline void InsertBits16(u32 ea, u32 mask, u32 v) { store<u16>(ea, static_cast<u16>((load<u16>(ea) & ~mask) | (v & mask))); }
inline void InsertBits8(u32 ea, u32 mask, u32 v) { store<u8>(ea, static_cast<u8>((load<u8>(ea) & ~mask) | (v & mask))); }

// ---------------------------------------------------------------- VMX (raw lanes)
struct V4 {
    u32 w[4];
    f32 f(u32 i) const { return std::bit_cast<f32>(w[i]); }
};
inline V4 VLd(u32 ea) {  // lvx: the aligned 16 bytes
    V4 r;
    for (u32 i = 0; i < 4; ++i) r.w[i] = load<u32>((ea & ~15u) + 4 * i);
    return r;
}
inline void VSt(u32 ea, const V4& v) {  // stvx
    for (u32 i = 0; i < 4; ++i) store<u32>((ea & ~15u) + 4 * i, v.w[i]);
}
// vmaddfp a*c + b, lane by lane, fused (as the runtime's vmx::vmaddfp)
inline V4 VMadd(const V4& a, const V4& c, const V4& b) {
    V4 r;
    for (u32 i = 0; i < 4; ++i) r.w[i] = std::bit_cast<u32>(std::fma(a.f(i), c.f(i), b.f(i)));
    return r;
}

}  // namespace obj21
}  // namespace sly4
