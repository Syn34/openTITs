// Game code, medium sweep part 14: helpers shared by mid14_*.cpp (still
// recompiled game functions of 120-400 instructions).
#pragma once

#include <bit>
#include <cmath>
#include <initializer_list>
#include <utility>

#include "mid13.h"

namespace sly4 {
namespace mid14 {

using namespace opentit;
using namespace opentit::decomp;
using mid10::LoadF;
using mid10::PutBit;
using mid10::SArg;
using mid10::StoreF;
using mid12::MulS;
using mid12::NodeTag;
using mid12::SetF1;
using mid13::LfsF;
using mid6::F32;
using mid6::W32;
using obj19::VCall;
namespace m1fn = mid1::m1fn;
namespace m2fn = mid2::m2fn;
namespace xfn = mid5::xfn;

namespace m14fn {
constexpr u32 Thunk_000C3078 = 0x000C3078;          // (Def* p, const Record* rec): a def base constructor
constexpr u32 Obj_FindName_0034654C = 0x0034654C;   // (table, u32 hash) -> object or 0
constexpr u32 Obj_InitVt_B21C98 = 0x004BB3B0;       // (o, c): the action base constructor
constexpr u32 Hk_RemoveEntity92FFC8 = 0x0092FFC8;   // (hkResult* out, world, entity) (Havok)
constexpr u32 Hk_SetMotion_0091D470 = 0x0091D470;   // (body, 5, 1, 0) (Havok; name?)
constexpr u32 Hk_SetMass_0091D618 = 0x0091D618;     // (body; f1) (Havok; name?)
constexpr u32 Hk_SetF32At_007EE08C = 0x007EE08C;    // (f32* field, const f32* value) (Havok; name?)
}  // namespace m14fn

constexpr u32 kGlobF65CA0 = 0x00F65CA0;  // the game pointer (+0x1281D4: the named-object table, +4208 a level byte)

// Single-precision results: the s-forms round once to f32.
inline f64 S(f64 x) { return static_cast<f64>(static_cast<f32>(x)); }
inline f64 Fmadds(f64 a, f64 c, f64 b) { return S(std::fma(a, c, b)); }
inline f64 Fmsubs(f64 a, f64 c, f64 b) { return S(std::fma(a, c, -b)); }
inline f64 Fnmadds(f64 a, f64 c, f64 b) { return S(-std::fma(a, c, b)); }
inline f64 Fnmsubs(f64 a, f64 c, f64 b) { return S(-std::fma(a, c, -b)); }
// The double an lfs of the word at ea leaves.
inline f64 Lf(u32 ea) { return LfsF(load<u32>(ea)); }

// Copy a 32-bit word (an lfs / stfs or lwz / stw pair).
inline void Cw(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }

// The clamps the compiled code spells out with compare chains.
inline s32 ClampS(s32 v, s32 lo, s32 hi) {
    const s32 m = v < hi ? v : hi;
    return m > lo ? m : lo;
}

// The slot for a push onto a vector {+0 data, +4 u16 capacity, +8 u16
// count, +10 u16 high-water} of `size`-byte elements: PtrVector_Reserve to
// twice the capacity (masked 0x1FFFE) when full, count re-read after it.
inline u32 VecSlot(PPUContext& ctx, u32 v, u32 size) {
    u32 n = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (n == cap) {
        call(ctx, m1fn::PtrVector_Reserve, {v, (cap << 1) & 0x0001FFFEu});
        n = load<u16>(v + 8);
    }
    return load<u32>(v) + n * size;
}

// The end of such a push: count + 1, the high-water mark raised to it;
// returns the new element's index.
inline u32 VecPushTail(u32 v) {
    const u32 c = (load<u16>(v + 8) + 1) & 0xFFFFu;
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(c));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(c) ? hw : c));
    return c - 1;
}

// A bitfield copy of the top n bits of the word at s into the word at d,
// one bit (and one store) at a time, the source re-read for each.
inline void BitCopy(u32 d, u32 s, u32 n) {
    u32 r = (load<u32>(d) & 0x7FFFFFFFu) | (load<u32>(s) & 0x80000000u);
    store<u32>(d, r);
    for (u32 b = 1; b < n; ++b) {
        const u32 m = 0x80000000u >> b;
        r = (r & ~m) | (load<u32>(s) & m);
        store<u32>(d, r);
    }
}

}  // namespace mid14
}  // namespace sly4
