// Game code, large-function sweep part 15: helpers shared by big15_*.cpp
// (the remaining damage / collision def readers around
// Def_Construct74FD80 and further still-recompiled game functions).
#pragma once

#include <bit>
#include <cmath>

#include "big14.h"
#include "mid11.h"
#include "mid12.h"

namespace sly4 {
namespace big15 {

using namespace opentit;
using namespace opentit::decomp;
using big13::LfsX;
using big10::Stfs;
using big10::StfsW;
using big4::CopyW;
using mid11::DmgCtor;
using mid11::DmgDestroy;

namespace b15fn {
constexpr u32 DamageColDef_Read_000C85A8 = 0x000C85A8;  // (a0, c, x, tmpl, o): the damage def base reader
constexpr u32 XmlCursor_DocPath = 0x004F8944;           // (a0) (reads r3 only)
constexpr u32 XmlCursor_ReadBool = 0x004F986C;          // (c, hash, u8* out, u8 default) -> 0 ok / 1 missing / 2 wrong type
constexpr u32 XmlCursor_ReadS32 = 0x004F99BC;           // (c, hash, s32* out, s32 default) -> status
constexpr u32 XmlCursor_ReadF32 = 0x004F9B20;           // (c, hash, f32* out; f32 default) -> status
constexpr u32 XmlCursor_ReadVec3 = 0x004F9E0C;          // (c, hash, f32 out[3], const f32 default[3]) -> status
constexpr u32 XmlCursor_ReadHash = 0x004FA14C;          // (c, hash, u32* out, u32 default) -> status
}  // namespace b15fn

constexpr u32 kVecZero = 0x01164B40;    // f32[3] 0
constexpr u32 kStrAD13B8 = 0x00AD13B8;  // the damage def name (both string arguments)

// fsqrts / fdivs / fmuls / fmadds as the PPU rounds them (to single).
inline f64 Sqrts(f64 a) { return static_cast<f64>(static_cast<f32>(std::sqrt(a))); }
inline f64 Ms(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a * b)); }
inline f64 Ds(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a / b)); }
inline f64 Mas(f64 a, f64 c, f64 b) { return static_cast<f64>(static_cast<f32>(std::fma(a, c, b))); }

// XmlCursor_ReadF32(c, hash, out; f1 = the lfs of the f32 word at def).
inline u32 RdF32(PPUContext& ctx, u32 c, u32 hash, u32 out, u32 def) {
    callf(ctx, b15fn::XmlCursor_ReadF32, {c, hash, out}, {LfsX(def)});
    return static_cast<u32>(ctx.gpr[3]);
}

// The bool flag readers' default: the flag's bit of a word (as cntlzw /
// rlwinm 27 / xori 1 compute it).
inline u32 Bit(u32 w, u32 ibm) { return (w >> (31 - ibm)) & 1u; }

// PushBack of the u32 at ea into a PtrVector {+0 data, +4 u16 capacity,
// +8 u16 count, +10 u16 high-water mark} (inlined): PtrVector_Reserve to
// twice the capacity when full, the value read after it.
inline void PushFrom(PPUContext& ctx, u32 v, u32 ea) {
    u32 c = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (c == cap) {
        call(ctx, mid1::m1fn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
        c = load<u16>(v + 8);
    }
    store<u32>(load<u32>(v) + ((c << 2) & 0x3FFFCu), load<u32>(ea));
    const u32 n = (load<u16>(v + 8) + 1u) & 0xFFFFu;
    const u32 m = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(n) ? m : n));
}

// A u32 register argument as the PPU holds a signed 32-bit result (extsw /
// a negative subfc).
inline u64 Sx64(u32 v) { return static_cast<u64>(static_cast<s64>(static_cast<s32>(v))); }

}  // namespace big15
}  // namespace sly4
