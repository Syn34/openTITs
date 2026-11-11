// Game code, part 44: helpers shared by obj44*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from the remaining
// 93 instruction ones up).
#pragma once

#include <bit>
#include <cmath>

#include "obj43.h"

namespace sly4 {
namespace obj44 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj43;

// The raw f32 word of a register value (stfs).
inline u32 W44(f64 x) { return std::bit_cast<u32>(static_cast<f32>(x)); }
// lfs: the f32 at ea as a double.
inline f64 Lfs44(u32 ea) { return static_cast<f64>(load<f32>(ea)); }
// Single-precision rounding (the s-form result).
inline f64 F44(f64 x) { return static_cast<f64>(static_cast<f32>(x)); }
// fmadds / fmsubs: (f32)(a * c + b) / (f32)(a * c - b), fused.
inline f64 Fma44(f64 a, f64 c, f64 b) { return F44(std::fma(a, c, b)); }
inline f64 Fms44(f64 a, f64 c, f64 b) { return F44(std::fma(a, c, -b)); }
// lfs / stfs pair: copies the raw word.
inline void Cp44(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }

// The destructor body of 0x0026F914 / 0x0028E9F8 (void f(T* o)): vtable
// vt; for each of the n pointers at o + off that is set, p->vt[slot](p)
// (OPD / entry after the argument); vtable 0x00B1BE30, o taken out of the
// entry array of 0x01056F1C (Unlist1056F1C39) and List1013B5C_Remove(o)
// (r3 = its).
inline void SlotsDtorUnlist44(PPUContext& ctx, u32 vt, u32 off, u32 n, u32 slot) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt);
    for (u32 k = 0; k < n; ++k) {
        const u32 p = load<u32>(o + off + 4 * k);
        if (p != 0) VCall(ctx, p, slot, {p});
    }
    store<u32>(o, 0x00B1BE30u);
    sly4::obj39::Unlist1056F1C39(o);
    call(ctx, 0x000D6940u, {o});
}

}  // namespace obj44
}  // namespace sly4
