// Game code, part 46: helpers shared by obj46*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from the remaining
// 99 instruction ones up).
#pragma once

#include <bit>
#include <cmath>

#include "obj45.h"

namespace sly4 {
namespace obj46 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj45;

// The string table *(0x00F65038): rows of 128 bytes at +24, row -1 = none
// (address 0, read anyway by the callers).
inline u32 TblRow46(u32 tbl, s32 i) { return i != -1 ? load<u32>(tbl + 24) + static_cast<u32>(i) * 128u : 0u; }
// A row's sub-block: row + u16 *(0x0106CF7E) when the byte row->+2 is
// nonzero, else 0.
inline u32 TblSub46(u32 row) { return load<u8>(row + 2) != 0 ? row + load<u16>(0x0106CF7Eu) : 0u; }
// The cell of (row a, column row b's sub-block byte +k): row a + u16
// *(0x0106CF70) + 4 * that byte.
inline u32 TblCell46(u32 tbl, s32 a, s32 b, u32 k) {
    return TblRow46(tbl, a) + load<u16>(0x0106CF70u) + 4u * load<u8>(TblSub46(TblRow46(tbl, b)) + k);
}

// Binary tree search (u32 key +4, left +12 when the key is above the
// wanted one, right +16) from the root *(holder), pushing the path into
// the s16-counted stack (count at cnt, slots at slots, both reset first).
// Returns the hit or, when missing, the last node visited (hit = false).
inline u32 TreeSeek46(u32 root, u32 key, u32 cnt, u32 slots, bool& hit) {
    store<u16>(cnt, 0);
    for (u32 n = root;;) {
        const u32 k = load<u32>(n + 4);
        if (k == key) {
            hit = true;
            return n;
        }
        const u32 nx = k > key ? load<u32>(n + 12) : load<u32>(n + 16);
        if (nx == 0) {
            hit = false;
            return n;
        }
        const s32 i = static_cast<s16>(load<u16>(cnt));
        store<u16>(cnt, static_cast<u16>(i + 1));
        store<u32>(slots + static_cast<u32>(i) * 4u, n);
        n = nx;
    }
}

}  // namespace obj46
}  // namespace sly4
