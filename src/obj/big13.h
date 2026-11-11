// Game code, large-function sweep part 13: helpers shared by big13_*.cpp
// (the player's rig-part tuning helpers and further still-recompiled game
// functions).
#pragma once

#include <bit>
#include <cmath>

#include "big12.h"

namespace sly4 {
namespace big13 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using obj19::VCall;
using obj23::LfsAt;
using obj25::Sx;
using big4::CopyW;
using big7::F;
using big7::Fmadds;
using big7::Fmsubs;
using big7::Fnmsubs;
using big7::SetF1Bits;
using big10::StfsW;
using big10::Stfs;
using big11::Adds;
using big11::Subs;
using big11::Muls;
using big11::Divs;
using big12::Fsel;
using big12::Lfd;

namespace b13fn {
constexpr u32 HkArray_Reserve = 0x007CCE88;   // (allocator, hkArray*, s32 n, s32 elem size)
constexpr u32 MapE_Find = 0x0075C1DC;         // (map, const u32* key) -> slot*
constexpr u32 EmitterInstance_New = 0x005A3FFC;  // (def, bool) -> instance
constexpr u32 String_Format = 0x005534D0;     // (fmt, ...) -> rotating static buffer
constexpr u32 Def_SetName = 0x00584BDC;       // (def, name, bool hash)
constexpr u32 PtrVector_Reserve = 0x00551718; // (PtrVector*, capacity)
constexpr u32 Hk_Curve9FDF0C = 0x009FDF0C;    // (f1..f4) -> f1 (reads only f1..f4) (name?)
}  // namespace b13fn

// lfs of the word at ea as a double, bit for bit (signalling NaNs kept).
inline f64 LfsX(u32 ea) { return std::bit_cast<f64>(LfsBits(load<u32>(ea))); }

constexpr u32 kHkArrayAlloc = 0x011DF31Cu;    // the allocator passed to HkArray_Reserve

// hkArray {+0 data, +4 s32 size, +8 u32 capacity | flags (top two bits)}:
// grows to max(2 * capacity, n) when the capacity (low 30 bits) is below n
// (signed compares; a negative n never grows). The size is not written.
// Returns whether it called HkArray_Reserve.
inline bool HkReserve(PPUContext& ctx, u32 arr, s32 n, u32 elem) {
    const u32 cap = load<u32>(arr + 8);
    if (static_cast<s32>(cap & 0x3FFFFFFFu) >= n) return false;
    const s32 dbl = static_cast<s32>((cap << 1) & 0x7FFFFFFEu);
    call(ctx, b13fn::HkArray_Reserve, {kHkArrayAlloc, arr, static_cast<u64>(static_cast<s64>(dbl > n ? dbl : n)), elem});
    return true;
}

// The player tuning's link groups (tuning +1072: +132 the 32-byte groups,
// +140 the u16 group count; each group {+0 the 352-byte items, +8 u16
// count}): f(g, i, item) for every item j of every group g, with i = g *
// group count + j (the rig parts keep a square table, one row per group).
// The counts and pointers are re-read as the original does.
template <typename Fn>
inline u32 ForLinkItems(u32 links, Fn f) {
    const u32 ng = load<u16>(links + 140);
    for (u32 g = 0; g < ng; ++g) {
        const u32 grp = load<u32>(links + 132) + 32 * g;
        for (u32 j = 0; j < load<u16>(grp + 8); ++j) f(g, g * ng + j, load<u32>(grp) + 352 * j);
    }
    return ng;
}

// {w(a), w(b), w(c), 0} stored as the original's vsel / vandc merge of
// three splatted words to the 16-byte block holding dst.
inline void StoreVec3W(u32 dst, u32 wa, u32 wb, u32 wc) {
    const u32 q = dst & ~15u;
    store<u32>(q + 0, wa);
    store<u32>(q + 4, wb);
    store<u32>(q + 8, wc);
    store<u32>(q + 12, 0);
}

}  // namespace big13
}  // namespace sly4
