// Game code, part 53: helpers shared by obj53*.cpp (the last 119
// instruction functions of the small-function sweep, then the small
// functions that use VMX, smallest first).
#pragma once

#include <bit>
#include <cmath>

#include "obj52.h"

namespace sly4 {
namespace obj53 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj52;

// Single precision rounding of a double (the s-forms).
inline f64 F53(f64 x) { return static_cast<f64>(static_cast<f32>(x)); }

// lfs as a double.
inline f64 Lfs53(u32 ea) { return static_cast<f64>(load<f32>(ea)); }

// Raw 32-bit copy (an lfs / stfs pair).
inline void Cp53(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }

// The 9th argument of a call (the low word of the doubleword at sp + 112).
inline u32 Arg9_53(PPUContext& ctx) { return load<u32>(static_cast<u32>(ctx.gpr[1]) + 116); }

// The u16-counted vector of 8-byte entries at v (data +0, capacity +4,
// count +8, high-water mark +10): grows to twice the capacity
// (PtrVector_Reserve) when full, copies the 8 bytes at src to the end as
// two raw words, count + 1 (u16) and the mark = max(mark, count) (signed).
inline void Push8_53(PPUContext& ctx, u32 v, u32 src) {
    const u32 cap = load<u16>(v + 4);
    if (load<u16>(v + 8) == cap) call(ctx, 0x00551718u, {v, cap << 1});
    const u32 d = load<u32>(v) + (static_cast<u32>(load<u16>(v + 8)) << 3);
    Cp53(d, src);
    Cp53(d + 4, src + 4);
    const u32 n = static_cast<u16>(load<u16>(v + 8) + 1);
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n) ? hw : n));
}

// The game's handle table (*0x00F557F8): a handle h = idx | slot << 16
// names the 16-byte record tab->+260 + 16 * slot when idx != 0, slot <
// (s32) tab->+256, ((rec +12 >> 15) & 0xFFFF) == idx and bit 0x80 of byte
// rec +12 is clear. Returns the record (+8 the object) or 0.
inline u32 HandleRec53(u32 h) {
    const u32 tab = load<u32>(0x00F557F8u);
    const u32 idx = h & 0xFFFFu;
    if (idx == 0) return 0;
    const u32 slot = h >> 16;
    if (!(static_cast<s32>(slot) < static_cast<s32>(load<u32>(tab + 256)))) return 0;
    const u32 e = load<u32>(tab + 260) + slot * 16;
    if (((load<u32>(e + 12) >> 15) & 0xFFFFu) != idx) return 0;
    if ((load<u8>(e + 12) & 0x80u) != 0) return 0;
    return e;
}

}  // namespace obj53
}  // namespace sly4
