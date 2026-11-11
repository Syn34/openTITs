// Game code, medium sweep part 10: helpers shared by mid10_*.cpp (still
// recompiled game functions of 120-400 instructions).
#pragma once

#include <bit>
#include <cmath>

#include "mid9.h"

namespace sly4 {
namespace mid10 {

using namespace opentit;
using namespace opentit::decomp;
using mid6::F32;
using mid6::W32;
using mid9::CopyW;
using obj19::VCall;
namespace m8fn = mid8::m8fn;
namespace m9fn = mid9::m9fn;
namespace m1fn = mid1::m1fn;
namespace xfn = mid5::xfn;

// The caller's stack word at r1 + off (stack arguments: a function that
// makes its own frame of n bytes reads them at n + off from its r1).
inline u32 SArg(const PPUContext& ctx, u32 off) { return load<u32>(static_cast<u32>(ctx.gpr[1]) + off); }

// A double argument register stored as a single (stfs).
inline void StoreF(u32 ea, f64 v) { store<f32>(ea, static_cast<f32>(v)); }

// The single at ea as the double an lfs leaves in a register.
inline f64 LoadF(u32 ea) { return static_cast<f64>(F32(load<u32>(ea))); }

// Bit IBM-b (0 = MSB) of w set to the low bit of v (rlwimi w, v, 31 - b, b, b).
inline u32 PutBit(u32 w, u32 b, u32 v) {
    const u32 m = 0x80000000u >> b;
    return (w & ~m) | ((v & 1u) ? m : 0u);
}

// f32 of an exact small int (extsw / std / lfd / fcfid / frsp).
inline u32 IntToF32Bits(s32 v) { return W32(static_cast<f32>(static_cast<f64>(static_cast<s64>(v)))); }

// The shared head of the two leaf def constructors (Def_Construct74FD80 /
// Def_Construct74596C): w12 / w16 / w20, bytes +24..+31 0, +32 0, f32 +36 /
// +40 / +72 -1, byte +44 0, +48..+68 0, +4 / +8 0, f32 +76 = f, +80 / +84 /
// +108..+116 0, u16 +120 = (old & 0xF) | 0x8200, u16 +122 0, +0 = 2, then
// f32 +88[5] = ((u16 +120 >> 4) & 0xF) - i (always -i: those bits were just
// cleared).
inline void DefHeadInit(u32 o, u32 a, u32 b, u32 c, f64 f) {
    const u32 h = (load<u16>(o + 120) & 0xFu) | 0x8200u;
    store<u32>(o + 12, a);
    store<u32>(o + 16, b);
    store<u32>(o + 20, c);
    for (u32 k = 24; k < 32; ++k) store<u8>(o + k, 0);
    store<u32>(o + 32, 0);
    store<u32>(o + 36, 0xBF800000u);
    store<u32>(o + 40, 0xBF800000u);
    store<u8>(o + 44, 0);
    for (u32 k = 48; k <= 68; k += 4) store<u32>(o + k, 0);
    store<u32>(o + 72, 0xBF800000u);
    store<u32>(o + 4, 0);
    store<u32>(o + 8, 0);
    StoreF(o + 76, f);
    store<u32>(o + 80, 0);
    store<u32>(o + 84, 0);
    store<u32>(o + 108, 0);
    store<u32>(o + 112, 0);
    store<u32>(o + 116, 0);
    store<u16>(o + 120, static_cast<u16>(h));
    store<u16>(o + 122, 0);
    store<u32>(o, 2);
    for (u32 i = 0; i < 5; ++i) {
        const s32 t = static_cast<s32>((load<u16>(o + 120) >> 4) & 0xFu) - static_cast<s32>(i);
        store<u32>(o + 88 + 4 * i, IntToF32Bits(t));
    }
}

}  // namespace mid10
}  // namespace sly4
