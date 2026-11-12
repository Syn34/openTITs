// Game code, part 50: helpers shared by obj50*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from the remaining
// 110 instruction ones up).
#pragma once

#include <bit>
#include <cmath>

#include "obj49.h"

namespace sly4 {
namespace obj50 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj49;

// Raw 32-bit copy (an lfs / stfs or lwz / stw pair).
inline void Cp50(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }

// PtrVector_Clear + Nop on the vector at v.
inline void ClearVec50(PPUContext& ctx, u32 v) {
    call(ctx, 0x0054EC1Cu, {v});
    call(ctx, 0x0054EC18u, {v});
}

// The record teardown of the spawn helpers: the lazily allocated 20-byte
// PtrVector at rec+0 is cleared and freed (SmallFree(rec->+0, 20), reloaded).
inline void FreeVec0_50(PPUContext& ctx, u32 rec) {
    if (const u32 p = load<u32>(rec); p != 0) {
        ClearVec50(ctx, p);
        call(ctx, 0x00552420u, {load<u32>(rec), 20});
    }
}

// Appends v to the PtrVector of 32-bit entries at l (+0 array, u16 +4 cap,
// +8 count, +10 high-water count), doubling it (PtrVector_Reserve) when
// full; returns the old high-water count (the original's r3).
inline u32 PtrVec_Push50(PPUContext& ctx, u32 l, u32 v) {
    const u32 cap = load<u16>(l + 4);
    if (load<u16>(l + 8) == cap) call(ctx, 0x00551718u, {l, (cap << 1) & 0x1FFFEu});
    store<u32>(load<u32>(l) + ((static_cast<u32>(load<u16>(l + 8)) << 2) & 0x3FFFCu), v);
    const u32 n = load<u16>(l + 8) + 1u;
    const u32 hi = load<u16>(l + 10);
    store<u16>(l + 8, static_cast<u16>(n));
    store<u16>(l + 10, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFFu) ? hi : n));
    return hi;
}

// PtrVec_Push50 as compiled without the masks (rlwinm ..,0,30 / ..,0,29:
// plain shifts of the u16 cap and count, the same values).
inline u32 PtrVec_PushNm50(PPUContext& ctx, u32 l, u32 v) {
    const u32 cap = load<u16>(l + 4);
    if (load<u16>(l + 8) == cap) call(ctx, 0x00551718u, {l, cap << 1});
    store<u32>(load<u32>(l) + (static_cast<u32>(load<u16>(l + 8)) << 2), v);
    const u32 n = load<u16>(l + 8) + 1u;
    const u32 hi = load<u16>(l + 10);
    store<u16>(l + 8, static_cast<u16>(n));
    store<u16>(l + 10, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFFu) ? hi : n));
    return hi;
}

}  // namespace obj50
}  // namespace sly4
