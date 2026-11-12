// Game code, part 48: helpers shared by obj48*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from the remaining
// 103 instruction ones up).
#pragma once

#include <bit>
#include <cmath>

#include "obj47.h"

namespace sly4 {
namespace obj48 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj47;

// Callees used throughout.
constexpr u32 kFindComp48 = 0x004BB4B4u;   // Effect_FindComponent (owner, kind hash)
constexpr u32 kHashStr48 = 0x0056A6E0u;    // Hash_ToString (hash): the debug name
constexpr u32 kGameRoot48 = 0x00F65CA0u;   // the game/world root pointer (name?)

// f32 helpers: the bits of a double rounded to single, the double an lfs
// leaves, single-precision rounding.
inline u32 W48(f64 x) { return std::bit_cast<u32>(static_cast<f32>(x)); }
inline f64 Lfs48(u32 ea) { return static_cast<f64>(load<f32>(ea)); }
inline f64 F48(f64 x) { return static_cast<f64>(static_cast<f32>(x)); }
// A raw 32-bit copy (lfs/stfs or lwz/stw pairs).
inline void Cp48(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }
// A 32-bit value sign-extended to the 64-bit register (extsw / lwa).
inline u64 Sx48(u32 v) { return static_cast<u64>(static_cast<s64>(static_cast<s32>(v))); }
// slw: the shift count is the low 6 bits; 32..63 give 0.
inline u32 Slw48(u32 v, u32 n) { n &= 63; return n >= 32 ? 0u : v << n; }

// Virtual calls with the OPD and its entry address left in the two registers
// after the arguments, in the order the original call sequence loads them:
// OE = OPD first (lwz rX, slot(vt); lwz rY, 0(rX)), EO = entry first.
inline u64 VcSeq48(PPUContext& ctx, u32 opd, std::initializer_list<u64> args, u64 e0, u64 e1) {
    u64 r[10] = {};
    u32 n = 0;
    for (u64 a : args) r[n++] = a;
    r[n++] = e0;
    r[n++] = e1;
    switch (n) {
    case 2: return call_ptr(ctx, opd, {r[0], r[1]});
    case 3: return call_ptr(ctx, opd, {r[0], r[1], r[2]});
    case 4: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3]});
    case 5: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4]});
    case 6: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4], r[5]});
    case 7: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4], r[5], r[6]});
    default: return call_ptr(ctx, opd, {r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]});
    }
}
// o's vtable slot (byte offset), OPD then entry.
inline u64 VcOE48(PPUContext& ctx, u32 o, u32 slot, std::initializer_list<u64> args) {
    const u32 opd = load<u32>(load<u32>(o) + slot);
    return VcSeq48(ctx, opd, args, opd, load<u32>(opd));
}
// vtable vt's slot, entry then OPD.
inline u64 VcEO48(PPUContext& ctx, u32 vt, u32 slot, std::initializer_list<u64> args) {
    const u32 opd = load<u32>(vt + slot);
    return VcSeq48(ctx, opd, args, load<u32>(opd), opd);
}

// XmlCursor_ReadS32 (cur, hash, s32* out, s32 default = *out, sign-extended).
constexpr u32 kReadS32_48 = 0x004F99BCu;
inline u32 RdS32_48(PPUContext& ctx, u32 x, u32 h, u32 out) {
    return static_cast<u32>(call(ctx, kReadS32_48, {x, h, out, Sx48(load<u32>(out))}));
}

}  // namespace obj48
}  // namespace sly4
