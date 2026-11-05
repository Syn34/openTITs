// Game code, part 24: helpers shared by obj24*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from 22
// instructions up).
#pragma once

#include <bit>
#include <cmath>

#include "obj23.h"

namespace sly4 {
namespace obj24 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj23;
using namespace sly4::obj19;

namespace o24fn {
constexpr u32 Msg_Post = 0x00470660;           // (msg, key, a, b)
constexpr u32 Obj_InitVt_B20CC8 = 0x0042C9D0;  // (o, bool on): the 0x00B20CC8 base constructor
constexpr u32 Flag_Set8_40000000 = 0x0042CA60; // (o): o->+8 |= 0x40000000
constexpr u32 Mem_Free = 0x005611B0;           // (p)
constexpr u32 RefNode_Release = 0x004432DC;    // (node)
constexpr u32 String_Format = 0x005534D0;      // (fmt, ...)
constexpr u32 Ctor_Base4FBFA8 = 0x004FBFA8;    // (o, a): the base of the 0x00B0D9xx / 0x00B13xxx classes
}  // namespace o24fn

// fsel: c when a >= 0 (not NaN), else b.
inline f64 Fsel24(f64 a, f64 c, f64 b) { return a >= 0.0 ? c : b; }
// The f32 a fused multiply-subtract leaves: (f32)(a * c - b).
inline f32 Fms(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(c), -static_cast<f64>(b))); }
// The r13-relative (small data) word at off.
inline u32 Sda(const PPUContext& ctx, s32 off) { return load<u32>(static_cast<u32>(ctx.gpr[13]) + static_cast<u32>(off)); }
// The memory pool behind the r13-relative global at -28332 (its +44).
inline u32 Pool28332(const PPUContext& ctx) { return load<u32>(Sda(ctx, -28332) + 44); }

}  // namespace obj24
}  // namespace sly4
