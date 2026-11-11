// Game code, large-function sweep part 19: helpers shared by big19_*.cpp.
#pragma once

#include <bit>
#include <cmath>

#include "big18.h"

namespace sly4 {
namespace big19 {

using namespace opentit;
using namespace opentit::decomp;
using big13::LfsX;
using big17::SetF1;
using mid1::LfsBits;
using obj19::VCall;

namespace b19fn {
constexpr u32 PtrVector_Construct = 0x005516E0;  // (vec)
constexpr u32 PtrVector_Reserve = 0x00551718;    // (vec, u32 n)
constexpr u32 SmallAlloc = 0x0055238C;           // (u32 size) -> p or 0
constexpr u32 Array_Reserve = 0x00558F3C;        // (array, u32 n)
constexpr u32 Hash_ToString = 0x0056A6E0;        // (u32 hash) -> const char*
constexpr u32 String_HashNoCase = 0x0056A710;    // (const char*) -> u32
constexpr u32 String_CopyN = 0x00553638;         // (dst, src, u32 n)
constexpr u32 Effect_FindComponent = 0x004BB4B4; // (Effect*, u32 hash) -> component or 0
constexpr u32 Set_Find = 0x004BCE90;             // (0) -> the global effect set or 0
constexpr u32 Effect_Start = 0x004C753C;         // (Effect*)
constexpr u32 Effect_Construct = 0x004C7A68;     // (Effect*, def)
constexpr u32 Effect_SetTime = 0x004CF710;       // (Effect*; f32)
}  // namespace b19fn

using big12::kSetSelected;
using big12::TreeSelect;
using big12::SetSelect;
using big12::NewEffect;
using obj23::LfsAt;
constexpr u32 kSubSelected = 0x011DE2A8u;  // a def's sub-set selected entry's value (node +8)

// u16 vector {+0 data, u16 +4 capacity, u16 +6 flags, u16 +8 count, u16
// +10 high-water mark} (PtrVector): append a word, growing to twice the
// capacity when full. Returns the value left in r3 by the original (the
// new count, or the mark when that is larger).
inline u32 PtrVectorPush(PPUContext& ctx, u32 v, u32 x) {
    u32 n = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (n == cap) {
        call(ctx, b19fn::PtrVector_Reserve, {v, (cap << 1) & 0xFFFFFFFEu});
        n = load<u16>(v + 8);
    }
    store<u32>(load<u32>(v) + (n << 2), x);
    const u32 n1 = load<u16>(v + 8) + 1;
    const u32 hi = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n1));
    const u32 r = static_cast<s32>(hi) > static_cast<s32>(n1 & 0xFFFFu) ? hi : n1;
    store<u16>(v + 10, static_cast<u16>(r));
    return r;
}

}  // namespace big19
}  // namespace sly4
