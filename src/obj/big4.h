// Game code, large-function sweep part 4: helpers shared by big4_*.cpp
// (still recompiled game functions of 400+ instructions, the pool-kind
// constructors, ...).
#pragma once

#include <bit>
#include <cmath>

#include "big3.h"

namespace sly4 {
namespace big4 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using obj19::VCall;
using obj25::Sx;
using big3::PropSetIndex;
using big3::PropFind;

namespace b4fn {
using namespace big3::b3fn;
constexpr u32 kMemAlloc = 0x0054ED38;     // (size, pool, align) -> p or 0
constexpr u32 kMemFreeFrom = 0x0054EE6C;  // (p, pool)
constexpr u32 kMemmove = 0x0071C3F0;      // (dst, src, n)
constexpr u32 kSmallAlloc = 0x0055238C;   // (size) -> p or 0
constexpr u32 Skeleton_FindBone = 0x005A93D8;  // (skeleton, u32 hash) -> s32 index or -1
constexpr u32 WorldObj_ForEach = 0x0057F5D8;   // (obj, Opd* fn(node, arg) -> bool, arg, bool recurse)
constexpr u32 HashMap_Find2 = 0x007399A4;      // (map, const u32* key) -> value slot
constexpr u32 kGeomDefClass = 0x00F8F12C;      // u32: the geometry class (+16 its map by name hash)
}  // namespace b4fn

// Copy the raw 32-bit word at src to dst (an lfs / stfs pair).
inline void CopyW(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }
// Copy n consecutive words.
inline void CopyWords(u32 dst, u32 src, u32 n) {
    for (u32 k = 0; k < n; ++k) CopyW(dst + 4 * k, src + 4 * k);
}

// The address of set k of the property block (block->+4 + 4k).
inline u32 PropSetAt(u32 block, s32 k) { return load<u32>(block + 4 + 4 * static_cast<u32>(k)); }

}  // namespace big4
}  // namespace sly4

// ---------------------------------------------------------------- native
#include "native/objtypes.h"
