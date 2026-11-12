// Game code, large-function sweep part 3: helpers shared by big3_*.cpp
// (still recompiled game functions of 400+ instructions).
#pragma once

#include <bit>
#include <cmath>

#include "big2.h"

namespace sly4 {
namespace big3 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using obj19::VCall;
using obj25::Sx;

namespace b3fn {
using namespace big2::b2fn;
}  // namespace b3fn

// ------------------------------------------------------- property sets
//
// A property owner's +12 points to a block of up to four typed property
// sets (the pointers at +4..+16, a null ending the list early). A set
// has +8 its type hash, +12 a u16 count, +16 the sorted (ascending,
// unsigned) array of property name hashes, +20 a u16 offset per
// property and +24 the data base: property k lives at +24 + off[k].

// The set of `type` among the four, or -1 (the inlined search: stops at
// the first null pointer).
inline s32 PropSetIndex(u32 block, u32 type) {
    for (u32 k = 0; k < 4; ++k) {
        const u32 s = load<u32>(block + 4 + 4 * k);
        if (s == 0) return -1;
        if (load<u32>(s + 8) == type) return static_cast<s32>(k);
    }
    return -1;
}

// The address of property `hash` in set s, or 0: a linear search of
// the sorted hashes that gives up at the first larger (unsigned) one.
inline u32 PropFind(u32 s, u32 hash) {
    const s32 n = load<u16>(s + 12);
    u32 h = load<u32>(s + 16);
    for (s32 i = 0; i < n; ++i, h += 4) {
        const u32 x = load<u32>(h);
        if (x == hash) return load<u32>(s + 24) + load<u16>(load<u32>(s + 20) + 2 * static_cast<u32>(i));
        if (x > hash) return 0;
    }
    return 0;
}

}  // namespace big3
}  // namespace sly4

// ---------------------------------------------------------------- native
#include "native/objtypes.h"
