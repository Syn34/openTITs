// Game code, part 47: helpers shared by obj47*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from the remaining
// 101 instruction ones up).
#pragma once

#include <bit>
#include <cmath>

#include "obj46.h"

namespace sly4 {
namespace obj47 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj46;

// f32 helpers: the bits of a double rounded to single, the double an lfs
// leaves, a single-precision add (fadds).
inline u32 W47(f64 x) { return std::bit_cast<u32>(static_cast<f32>(x)); }
inline f64 Lfs47(u32 ea) { return static_cast<f64>(load<f32>(ea)); }
inline f64 Fadds47(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a + b)); }
// A raw 32-bit copy (lfs/stfs or lwz/stw pairs).
inline void Cp47(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }
// The word / byte at r1 + off in the caller's frame (stack arguments).
inline u32 StkW47(const PPUContext& ctx, u32 off) { return load<u32>(static_cast<u32>(ctx.gpr[1]) + off); }
inline u8 StkB47(const PPUContext& ctx, u32 off) { return load<u8>(static_cast<u32>(ctx.gpr[1]) + off); }

// The XmlCursor readers (cur, hash, out, default).
constexpr u32 kReadHash47 = 0x004FA14Cu;  // XmlCursor_ReadHash (cur, hash, u32* out, u32 default)
constexpr u32 kReadBool47 = 0x004F986Cu;  // XmlCursor_ReadBool (cur, hash, u8* out, u8 default)
constexpr u32 kReadF32_47 = 0x004F9B20u;  // XmlCursor_ReadF32 (cur, hash, f32* out; f1 default)
inline u32 RdHash47(PPUContext& ctx, u32 x, u32 h, u32 out) {
    return static_cast<u32>(call(ctx, kReadHash47, {x, h, out, load<u32>(out)}));
}
inline u32 RdBool47(PPUContext& ctx, u32 x, u32 h, u32 out) {
    return static_cast<u32>(call(ctx, kReadBool47, {x, h, out, load<u8>(out)}));
}
inline void RdF32_47(PPUContext& ctx, u32 x, u32 h, u32 out) { callf(ctx, kReadF32_47, {x, h, out}, {Lfs47(out)}); }

// The base-object constructor of the XML-built command objects: m =
// Mem_Alloc(size, 0, 16) and, when that worked, Obj_InitVt_B21C98(m, x).
inline u32 NewXmlObj47(PPUContext& ctx, u32 size, u32 x) {
    const u32 m = static_cast<u32>(call(ctx, 0x0054ED38u, {size, 0, 16}));
    if (m != 0) call(ctx, 0x004BB3B0u, {m, x});
    return m;
}

}  // namespace obj47
}  // namespace sly4
