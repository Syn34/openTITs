// Shared names for src/world/hevent18_*.cpp: callees named by the
// hevent17_*.cpp functions and more of the World event handler's remaining
// callees. Guest addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include "hevent17.h"

namespace sly4 {
namespace hevent18 {

namespace h18fn {
constexpr u32 SortedMap_Find786C0C = 0x00786C0C;  // (map, const u32* key) -> value slot (symbols.txt)
}  // namespace h18fn

// The flag map searched by Flag_Get48FC68.
constexpr u32 kFlagMap = 0x010BB738;

// The engine's inlined PtrVector push: +0 u32 data[], +4 u16 capacity, +8
// u16 count, +10 u16 high-water mark. A full vector is first grown to twice
// its capacity. Stores: the slot, the count, the high-water mark.
constexpr u32 kPvData = 0, kPvCap = 4, kPvStep = 6, kPvCount = 8, kPvHigh = 10;
inline void pv_push(PPUContext& ctx, u32 v, u32 value) {
    u32 n = load<u16>(v + kPvCount);
    const u32 cap = load<u16>(v + kPvCap);
    if (n == cap) {
        call(ctx, hevent8::h8fn::PtrVector_Reserve, {v, (cap << 1) & 0xFFFFFFFEu});
        n = load<u16>(v + kPvCount);
    }
    const u32 data = load<u32>(v + kPvData);
    const u32 n1 = load<u16>(v + kPvCount) + 1;
    const u32 hi = load<u16>(v + kPvHigh);
    store<u32>(data + (n << 2), value);
    store<u16>(v + kPvCount, static_cast<u16>(n1));
    store<u16>(v + kPvHigh, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n1 & 0xFFFF) ? hi : n1));
}

}  // namespace hevent18
}  // namespace sly4
