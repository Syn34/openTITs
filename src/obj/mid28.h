// Game code, medium sweep part 28: helpers shared by mid28_*.cpp (more
// still-recompiled game functions of 120-400 instructions).
#pragma once

#include <bit>
#include <cmath>
#include <initializer_list>
#include <utility>

#include "mid27.h"

namespace sly4 {
namespace mid28 {

using namespace opentit;
using namespace opentit::decomp;
using mid14::Lf;
using mid6::W32;
using obj19::VCall;

namespace m28fn {
using namespace mid27::m27fn;
constexpr u32 SetHash = 0x004CF988u;      // Effect_SetHash4CF988(e, hash)
constexpr u32 VecReserve = 0x00551718u;   // PtrVector_Reserve(v, u16 cap)
constexpr u32 SmallAlloc28 = 0x0055238Cu; // SmallAlloc(size)
constexpr u32 ReadU8 = 0x005643C4u;       // Iff_ReadU8(r, u8* out, name)
constexpr u32 ReadVec3 = 0x005699F0u;     // FileReader_ReadVec3Swapped(r, f32 out[3], name)
}  // namespace m28fn

// The PPU single-precision result of a double-precision operation.
inline f64 Sgl28(f64 x) { return static_cast<f64>(static_cast<f32>(x)); }
// The f32 bit pattern of x rounded to single (stfs).
inline u32 Bits28(f64 x) { return std::bit_cast<u32>(static_cast<f32>(x)); }
// fsel: a >= 0 (false for NaN) ? b : c.
inline f64 Fsel28(f64 a, f64 b, f64 c) { return a >= 0.0 ? b : c; }

// Appends w to the 4-byte vector at o + d (data +d, u16 cap +d+4, count
// +d+8, peak +d+10): PtrVector_Reserve(v, cap * 2) when full, the count
// re-read, data[count] = w, count + 1 stored, then peak = max(peak, count)
// (signed compare of the u16s).
inline void VecPush28(PPUContext& ctx, u32 o, u32 d, u32 w) {
    const u32 cap = load<u16>(o + d + 4);
    if (load<u16>(o + d + 8) == cap) call(ctx, m28fn::VecReserve, {o + d, (cap << 1) & 0x1FFFEu});
    const u32 c = load<u16>(o + d + 8);
    store<u32>(load<u32>(o + d) + ((c << 2) & 0x3FFFCu), w);
    const u32 n = (load<u16>(o + d + 8) + 1) & 0xFFFFu;
    store<u16>(o + d + 8, static_cast<u16>(n));
    const u32 peak = load<u16>(o + d + 10);
    store<u16>(o + d + 10, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(n) ? peak : n));
}

// The inlined binary-tree lookup of a record e (root slot *(e + root),
// nodes {+4 key, +8 value, +12 / +16 children by unsigned key}): the nodes
// passed are pushed to the path at e + path (s16 depth at e + depth, zeroed
// first); e + node = the node found (and *glob = its value) or the last
// node visited; an empty tree stores 0 to both. True when found.
inline bool TreeFind28(u32 e, u32 k, u32 root, u32 path, u32 depth, u32 node, u32 glob) {
    u32 cur = load<u32>(load<u32>(e + root));
    if (cur == 0) {
        store<u32>(e + node, 0);
        store<u32>(glob, 0);
        return false;
    }
    store<u16>(e + depth, 0);
    for (;;) {
        const u32 ck = load<u32>(cur + 4);
        if (ck == k) {
            store<u32>(e + node, cur);
            store<u32>(glob, load<u32>(cur + 8));
            return true;
        }
        const u32 next = load<u32>(cur + (ck > k ? 12 : 16));
        if (next == 0) {
            store<u32>(e + node, cur);
            return false;
        }
        const s32 dd = static_cast<s16>(load<u16>(e + depth));
        store<u16>(e + depth, static_cast<u16>(dd + 1));
        store<u32>(e + path + 4 * static_cast<u32>(dd), cur);
        cur = next;
    }
}

}  // namespace mid28
}  // namespace sly4
