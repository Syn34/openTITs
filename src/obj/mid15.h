// Game code, medium sweep part 15: helpers shared by mid15_*.cpp (still
// recompiled game functions of 120-400 instructions).
#pragma once

#include <bit>
#include <cmath>
#include <initializer_list>
#include <utility>

#include "mid14.h"

namespace sly4 {
namespace mid15 {

using namespace opentit;
using namespace opentit::decomp;
using mid14::BitCopy;
using mid14::ClampS;
using mid14::Cw;
using mid14::Fmadds;
using mid14::Fmsubs;
using mid14::Fnmadds;
using mid14::Fnmsubs;
using mid14::Lf;
using mid14::S;
using mid14::VecPushTail;
using mid14::VecSlot;
using mid10::SArg;
using mid13::LfsF;
using mid6::F32;
using mid6::W32;
namespace m1fn = mid1::m1fn;

namespace m15fn {
constexpr u32 Math_Cosf = 0x007344E0;                     // f1 -> f1
constexpr u32 Math_Sinf = 0x007345F0;                     // f1 -> f1
constexpr u32 Acosf = 0x005E649C;                         // acosf: f1 -> f1
constexpr u32 Atan2f = 0x005E665C;                        // atan2f: (f1 y, f2 x) -> f1
constexpr u32 Shape_Reset63778C = 0x0063778C;             // (Shape*) (middleware)
constexpr u32 Cam_UpdateViewCache_00289EC0 = 0x00289EC0;  // (Cam*)
constexpr u32 Emitter_Stop = 0x00443424;                  // (handle, bool fade)
constexpr u32 Matrix_NormalizeInPlace = 0x0056D374;       // (Mat34*)
constexpr u32 WorldObj_MarkDirty = 0x0057D000;            // (WorldObj*)
constexpr u32 Seg_ClosestPoint_00013E40 = 0x00013E40;     // (Vec3* out, -, const Vec3* a, const Vec3* b, const Vec3* p)
constexpr u32 Vec_SignedAngle16680 = 0x00016680;          // (const vec3* a, const vec3* b) -> f1
constexpr u32 Set_Find = 0x004BCE90;                      // (u32 name) -> Set*
constexpr u32 Quat_Multiply = 0x005556A4;                 // (f32 out[4], const f32 a[4], const f32 b[4])
constexpr u32 Obj_Construct_00769BD8 = 0x00769BD8;        // (o, x) (reads r3 / r4 only)
constexpr u32 QueryEx_Construct_000DFDF8 = 0x000DFDF8;    // (q, ... 7 GPR + 11 stack args; r7 = Def*)
}  // namespace m15fn

// The f32 bits an stfs of the (already single-rounded) double x writes.
inline u32 Fb(f64 x) { return std::bit_cast<u32>(static_cast<f32>(x)); }
// stfs of x to ea.
inline void Sf(u32 ea, f64 x) { store<u32>(ea, Fb(x)); }
// The double an lfd at ea leaves.
inline f64 Ld(u32 ea) { return std::bit_cast<f64>(load<u64>(ea)); }

// fsel d, a, c, b: a >= 0 (not NaN) ? c : b.
inline f64 Fsel15(f64 a, f64 c, f64 b) { return a >= 0.0 ? c : b; }

// fctidz + fcfid + frsp: the f32 of x truncated through a 64-bit integer.
inline f64 Trunc15(f64 x) { return S(rt::fcfid(rt::fctid(x, true))); }

// The root of a parent chain linked at +12 (the compiled walk re-reads
// each link; the loads have no side effects).
inline u32 Root12(u32 x) {
    while (load<u32>(x + 12) != 0) x = load<u32>(x + 12);
    return x;
}

// The game's inlined random generator on 0x01164888 {+0 u64 the low 16
// bits of the last mix, +8 u32 seed}: mix(s) = (rotl4(s) + rotl16(s)) ^
// 0x87654321, the next seed rotl16(mix). The value a step hands out is the
// float of the u64 before it (fcfid + frsp).
constexpr u32 kRand15 = 0x01164888u;
inline u32 RandMix15(u32 s) { return (std::rotl(s, 4) + std::rotl(s, 16)) ^ 0x87654321u; }
inline f64 RandF15(u64 v) { return S(static_cast<f64>(static_cast<s64>(v))); }

}  // namespace mid15
}  // namespace sly4
