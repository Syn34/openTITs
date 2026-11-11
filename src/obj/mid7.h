// Game code, medium sweep part 7: helpers shared by mid7_*.cpp (still
// recompiled game functions of 120-400 instructions: the spawn-query def
// readers of the 0x000CBE10 family, volume setup, XML element readers and
// other template families).
#pragma once

#include <bit>
#include <cmath>

#include "mid6.h"

namespace sly4 {
namespace mid7 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using obj19::VCall;
using mid5::XBool;
using mid5::XF32;
using mid5::XHash;
using mid5::XRead;
using mid5::XS32;
using mid6::F32;
using mid6::W32;
using mid6::XF32Def;
using mid6::XVec2;
using mid6::XVec3;
namespace xfn = mid5::xfn;

namespace m7fn {
constexpr u32 XmlCursor_FirstChildAny = 0x004F9140;   // (c) -> the first child element, or 0
constexpr u32 XmlCursor_NextSiblingAny = 0x004F8D74;  // (node) -> the next sibling element, or 0
constexpr u32 SmallAlloc = 0x0055238C;                // (size) -> p or 0
constexpr u32 Mem_Alloc = 0x0054ED38;                 // (size, pool, align)
}  // namespace m7fn

// The u8 bool field at p read through the byte tmp (default its value).
inline u32 XBoolVia(PPUContext& ctx, u32 c, u32 hash, u32 p, u32 tmp) {
    const u32 r = XRead(ctx, xfn::XmlCursor_ReadBool, c, hash, tmp, load<u8>(p));
    store<u8>(p, load<u8>(tmp));
    return r;
}

// The quaternion of the euler angles (degrees) at s, the shared inline code
// of several XML readers: the angles are scaled by k[0] (deg -> rad, stored
// back), halved with k[1], then Math_Cosf / Math_Sinf of y, z, x (in that
// order, cos first); q = {x, y, z, w} as the original computes them
// (fmuls / fmadds / fmsubs in single precision).
struct EulerQuat {
    f32 x, y, z, w;
};
inline EulerQuat EulerToQuat(PPUContext& ctx, u32 s, u32 k) {
    constexpr u32 kCosf = 0x007344E0, kSinf = 0x007345F0;
    auto mul = [](f64 a, f64 b) { return static_cast<f32>(a * b); };
    auto madd = [](f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); };
    auto msub = [](f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, -b)); };
    const f64 d2r = F32(load<u32>(k)), half = F32(load<u32>(k + 4));
    const f32 ax = mul(F32(load<u32>(s)), d2r), ay = mul(F32(load<u32>(s + 4)), d2r), az = mul(F32(load<u32>(s + 8)), d2r);
    store<u32>(s, W32(ax));
    store<u32>(s + 4, W32(ay));
    store<u32>(s + 8, W32(az));
    const f32 hy = mul(ay, half);
    const f64 cy = callf(ctx, kCosf, {}, {hy});
    const f32 hz = mul(az, half);
    const f64 cz = callf(ctx, kCosf, {}, {hz});
    const f32 hx = mul(ax, half);
    const f64 cx = callf(ctx, kCosf, {}, {hx});
    const f64 sy = callf(ctx, kSinf, {}, {hy});
    const f64 sz = callf(ctx, kSinf, {}, {hz});
    const f64 sx = callf(ctx, kSinf, {}, {hx});
    const f32 f2 = mul(sy, sz), f3 = mul(cy, cz), f4 = mul(cz, sy), f5 = mul(cy, sz);
    const f32 f6 = mul(f2, sx), f7 = mul(f3, sx), f8 = mul(f4, sx), f1 = mul(f5, sx);
    return {madd(f2, cx, f7), madd(f4, cx, f1), msub(f5, cx, f8), msub(f3, cx, f6)};
}

}  // namespace mid7
}  // namespace sly4
