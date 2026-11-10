// Game code, medium sweep part 6: helpers shared by mid6_*.cpp (still
// recompiled game functions of 120-400 instructions: the query-spawn base
// constructor, XML element readers and other template families).
#pragma once

#include <bit>
#include <cmath>

#include "mid5.h"

namespace sly4 {
namespace mid6 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using obj19::VCall;
using mid5::XBit;
using mid5::XBool;
using mid5::XF32;
using mid5::XHash;
using mid5::XRead;
using mid5::XS32;
namespace xfn = mid5::xfn;

constexpr u32 XmlDef_ReadBase = 0x000CBE10;  // (a0, c, def): the shared part of the 0x000CBE10-family def readers (reads r4 / r5 only) (name?)

// The f32 field at p, defaulting to the f32 at def (passed in f1).
inline u32 XF32Def(PPUContext& ctx, u32 c, u32 hash, u32 p, u32 def) {
    callf(ctx, xfn::XmlCursor_ReadF32, {c, hash, p}, {std::bit_cast<f64>(LfsBits(load<u32>(def)))});
    return static_cast<u32>(ctx.gpr[3]);
}
// The f32[3] / f32[2] field at p, defaulting to itself.
inline u32 XVec3(PPUContext& ctx, u32 c, u32 hash, u32 p) { return XRead(ctx, xfn::XmlCursor_ReadVec3, c, hash, p, p); }
inline u32 XVec2(PPUContext& ctx, u32 c, u32 hash, u32 p) { return XRead(ctx, xfn::XmlCursor_ReadVec2, c, hash, p, p); }
// The three f32 at p scaled by the f32 at k (fmuls); f1 = the last.
inline void ScaleVec3(PPUContext& ctx, u32 p, u32 k) {
    const f32 s = std::bit_cast<f32>(load<u32>(k));
    f32 v[3];
    for (u32 i = 0; i < 3; ++i) v[i] = std::bit_cast<f32>(load<u32>(p + 4 * i)) * s;
    for (u32 i = 0; i < 3; ++i) store<u32>(p + 4 * i, std::bit_cast<u32>(v[i]));
    ctx.fpr[1] = static_cast<f64>(v[2]);
}

// f32 view of a guest word / word of an f32.
inline f32 F32(u32 w) { return std::bit_cast<f32>(w); }
inline u32 W32(f32 f) { return std::bit_cast<u32>(f); }

// Havok hkpWorldObject property list {+124 {u32 key, u32 pad, u64 value}[],
// +128 s32 count}: the index of key, or -1.
inline s32 HkPropFind(u32 ent, u32 key) {
    for (s32 i = 0; i < static_cast<s32>(load<u32>(ent + 128)); ++i)
        if (load<u32>(load<u32>(ent + 124) + 16 * static_cast<u32>(i)) == key) return i;
    return -1;
}

}  // namespace mid6
}  // namespace sly4
