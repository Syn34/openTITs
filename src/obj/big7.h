// Game code, large-function sweep part 7: helpers shared by big7_*.cpp
// (the callees part 6 named and further still-recompiled game functions of
// 400+ instructions).
#pragma once

#include <bit>
#include <cmath>

#include "big6.h"

namespace sly4 {
namespace big7 {

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
using big6::LaneW;
using big6::Splat;
using big6::Mrghw;
using big6::Mrglw;
using big6::Perm;
using big6::Lvlx;
using big6::Sub4;
using big6::SetF1FromWord;
using big6::Rows12Equal;

namespace b7fn {
using namespace big5::b5fn;
constexpr u32 kStateMachineUpdate = 0x00464BD4;  // (o, f1 dt) -> bool
constexpr u32 kStateMachineTick = 0x00464CBC;    // (o, f1 dt) -> bool
constexpr u32 kShapeInit = 0x000110EC;           // (Shape* out, system, const f32 m[4][4], const f32 halfExt[3], u32, u16)
constexpr u32 kEffectSetHash = 0x004CF988;       // (Effect*, u32 hash)
constexpr u32 kEffectCheck = 0x004CF620;         // (Effect*, 0) -> bool
constexpr u32 kQueuePush = 0x004705F8;           // (queue, o, u32 hash, 0, 1)
constexpr u32 kMsgPost = 0x00470660;             // (o, u32, u32, u32)
constexpr u32 kEventInit = 0x00464ED4;           // (Event* e, o, 0, 0, 0, 0, 0)
constexpr u32 kWorldQuery = 0x00482D14;          // (world, s32 kind, Event*)
constexpr u32 kQuatToMatrix = 0x00553A90;        // (f32 out[4][4], const f32 q[4], const f32 pos[3])
constexpr u32 kMatrixMultiply = 0x005541F0;      // (Matrix* a, const Matrix* b): a = a x b
constexpr u32 kObjBoneMatrix = 0x000CE688;       // (f32 out[4][4], Object*, s32 bone, u8 world, u8 raw)
}  // namespace b7fn

inline f32 F(u32 a) { return load<f32>(a); }
inline V4 Splat1(f32 x) { return V4{{x, x, x, x}}; }
// vsel: b's bits where m is set, a's elsewhere.
inline V4 Sel(const V4& a, const V4& b, const V4& m) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = std::bit_cast<f32>((LaneW(a, k) & ~LaneW(m, k)) | (LaneW(b, k) & LaneW(m, k)));
    return o;
}
// vandc a & ~m.
inline V4 Andc(const V4& a, const V4& m) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = std::bit_cast<f32>(LaneW(a, k) & ~LaneW(m, k));
    return o;
}
// vxor.
inline V4 Xor(const V4& a, const V4& b) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = std::bit_cast<f32>(LaneW(a, k) ^ LaneW(b, k));
    return o;
}
// vminfp / vmaxfp as the runtime's vmx.h evaluates them: a NaN in a, else
// a NaN in b, else std::min / std::max (a on ties, so +0 / -0 keep a).
inline V4 Min4(const V4& a, const V4& b) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) {
        const f32 x = a.e[k], y = b.e[k];
        o.e[k] = std::isnan(x) ? x : std::isnan(y) ? y : (y < x ? y : x);
    }
    return o;
}
inline V4 Max4(const V4& a, const V4& b) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) {
        const f32 x = a.e[k], y = b.e[k];
        o.e[k] = std::isnan(x) ? x : std::isnan(y) ? y : (x < y ? y : x);
    }
    return o;
}
// vmaddfp a * c + 0, lane by lane.
inline V4 Mul0(const V4& a, const V4& c) {
    V4 o;
    for (u32 k = 0; k < 4; ++k) o.e[k] = std::fma(a.e[k], c.e[k], 0.0f);
    return o;
}
// fmadds / fmsubs / fnmsubs.
inline f32 Fmadds(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }
inline f32 Fmsubs(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, -b)); }
inline f32 Fnmsubs(f64 a, f64 c, f64 b) { return static_cast<f32>(-std::fma(a, c, -b)); }
// The 12 x/y/z words of a 4x4 matrix copied as raw words (CopyMat43W);
// returns the last one (the +56 word), which the original leaves in f1.
inline u32 CopyMat43Last(u32 dst, u32 src) {
    u32 w = 0;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) {
            w = load<u32>(src + 16 * r + 4 * k);
            store<u32>(dst + 16 * r + 4 * k, w);
        }
    return w;
}
inline void SetF1Bits(PPUContext& ctx, u32 w) { ctx.fpr[1] = std::bit_cast<f64>(LfsBits(w)); }

}  // namespace big7
}  // namespace sly4

// ---------------------------------------------------------------- native
#include "native/objtypes.h"
