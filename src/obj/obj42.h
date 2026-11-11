// Game code, part 42: helpers shared by obj42*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from the remaining
// 89 instruction ones up).
#pragma once

#include <bit>
#include <cmath>

#include "obj41.h"

namespace sly4 {
namespace obj42 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj41;

// The raw f32 word of a register value (stfs).
inline u32 W42(f64 x) { return std::bit_cast<u32>(static_cast<f32>(x)); }
// lfs: the f32 at ea as a double.
inline f64 Lfs42(u32 ea) { return static_cast<f64>(load<f32>(ea)); }
// Single-precision rounding (the s-form result).
inline f64 F42(f64 x) { return static_cast<f64>(static_cast<f32>(x)); }

// The inlined ~Array(a) (u16 count +0, u16 element size << 1 at +6, data
// +8, allocator +12): the data freed through the allocator's slot 8
// (allocator, data, bytes), then Array_Destruct(a).
inline void ArrayDtor42(PPUContext& ctx, u32 a) {
    const u32 bytes = static_cast<u32>(load<u16>(a)) * ((static_cast<u32>(load<u16>(a + 6)) >> 1) & 0x7FFFu);
    const u32 data = load<u32>(a + 8);
    if (data != 0) {
        const u32 al = load<u32>(a + 12);
        VCall(ctx, al, 8, {al, data, bytes});
    }
    call(ctx, 0x0054EC94u, {a});
}

// ~T() of the two volume-shape classes 0x0021F088 / 0x0022499C: with a
// body at o->+boff the vtable vt0 goes in first and bit 9 of body +136 is
// set; then vtable 0x00B19558, Volume_Notify(*0x00F557EC, o + 424),
// Shape_Release(o + 424), the arrays +388 / +364 and the PtrVector +344
// destroyed, and the base Thunk_000D729C.
inline void DtorBodyShape42(PPUContext& ctx, u32 vt0, u32 boff) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u32>(o + boff);
    if (b != 0) {
        store<u32>(o, vt0);
        store<u32>(b + 136, load<u32>(b + 136) | 0x200u);
    }
    store<u32>(o, 0x00B19558u);
    call(ctx, 0x000112D8u, {load<u32>(0x00F557ECu), o + 424});
    call(ctx, 0x00638498u, {o + 424});
    ArrayDtor42(ctx, o + 388);
    ArrayDtor42(ctx, o + 364);
    VecDtor(ctx, o + 344);
    call(ctx, 0x000D729Cu, {o});
}

}  // namespace obj42
}  // namespace sly4
