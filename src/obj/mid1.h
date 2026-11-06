// Game code, medium sweep part 1: helpers shared by mid1_*.cpp (still
// recompiled game functions of 120-400 instructions, picked in coherent
// groups: the name-binding owner, the effect list, destructors, ...).
#pragma once

#include <bit>
#include <cmath>

#include "obj25.h"

namespace sly4 {
namespace mid1 {

using namespace opentit;
using namespace opentit::decomp;
using obj19::VCall;
using obj25::Sx;
using obj23::kWorldPtr;

namespace m1fn {
constexpr u32 Mem_Alloc = 0x0054ED38;          // (size, pool, align)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;       // (p, pool)
constexpr u32 SmallAlloc = 0x0055238C;         // (size)
constexpr u32 SmallFree = 0x00552420;          // (p, size)
constexpr u32 PtrVector_Construct = 0x005516E0;  // (vec)
constexpr u32 PtrVector_Init = 0x00551598;     // (vec, capacity, a, b)
constexpr u32 PtrVector_Reserve = 0x00551718;  // (vec, capacity)
constexpr u32 PtrVector_Clear = 0x0054EC1C;    // (vec)
constexpr u32 Nop = 0x0054EC18;                // (p)
constexpr u32 String_CopyN = 0x00553638;       // (dst, src, n)
constexpr u32 String_HashNoCase = 0x0056A710;  // (s) -> u32
constexpr u32 String_Format = 0x005534D0;      // (fmt, ...) -> a temporary string
constexpr u32 String_Length = 0x00555FA4;      // strlen
constexpr u32 String_Assign = 0x00555F80;      // strcpy
constexpr u32 Node_Bind = 0x004D3088;          // (owner, name, kind, k6, offset, value) -> binding
constexpr u32 Set_Find = 0x004BCE90;           // (u32 name) -> effect set
constexpr u32 Effect_Construct = 0x004C7A68;   // (Effect* 140 bytes, Def*)
constexpr u32 Effect_Destruct = 0x004C8E80;    // (Effect*)
constexpr u32 Set_Insert_0079B55C = 0x0079B55C;  // (out, set, const T* key): insert (name?)
constexpr u32 World_ReleaseChild1281F4 = 0x0047C490;  // (world)
constexpr u32 Ptr_FreeAndClear_00321240 = 0x00321240;  // (void** pp)
constexpr u32 Obj_BaseDtor_0048F6B0 = 0x0048F6B0;  // the base destructor under VtB1E1F8 (name?)
constexpr u32 Obj_Flag13Off_0048F298 = 0x0048F298;  // (o, 1) when +176 has 0x2000 (name?)
constexpr u32 WorldObj_MarkDirty = 0x0057D000;  // (obj)
constexpr u32 Dtor_B219D0_00464AD0 = 0x00464AD0;  // the base destructor (vtable 0x00B219D0)
}  // namespace m1fn

// The formats Node_Bind prints values with.
constexpr u32 kFmtBindInt = 0x00B225C0;    // "%d"
constexpr u32 kFmtBindFloat = 0x00B225C3;  // "%.2f"

// The effect list (a PtrVector of every live Effect) and its guard byte
// (a function-local static: built on first use by Effect_New / Effect_Delete).
constexpr u32 kEffectListInit = 0x010C2960;
constexpr u32 kEffectList = 0x010C2968;
// The def of the node the last Effect_New lookup found.
constexpr u32 kEffectLastDef = 0x011DE288;

// lfs: the double a single-precision word loads as, bit for bit (NaNs keep
// their payload and are not quieted, as on the PPU).
inline u64 LfsBits(u32 w) {
    if ((w & 0x7F800000u) == 0x7F800000u && (w & 0x007FFFFFu) != 0)
        return (static_cast<u64>(w >> 31) << 63) | (0x7FFull << 52) | (static_cast<u64>(w & 0x007FFFFFu) << 29);
    return std::bit_cast<u64>(static_cast<f64>(std::bit_cast<f32>(w)));
}

// PushBack of a u32 PtrVector {+0 data, +4 u16 capacity, +8 u16 count,
// +10 u16 high-water mark}: grown to twice the capacity when full, the value
// stored, the count and the mark updated.
inline void PtrVecPush(PPUContext& ctx, u32 v, u32 value) {
    u32 c = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (c == cap) {
        call(ctx, m1fn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
        c = load<u16>(v + 8);
    }
    store<u32>(load<u32>(v) + ((c << 2) & 0x3FFFCu), value);
    const u32 n = (load<u16>(v + 8) + 1u) & 0xFFFFu;
    const u32 m = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(m) > static_cast<s32>(n) ? m : n));
}

// The inlined PtrVector constructor: construct, +6 = the element size (4
// for pointers), PtrVector_Init(v, capacity, 0, last).
inline void PtrVecNew(PPUContext& ctx, u32 v, u32 capacity, u16 elem = 4, u32 last = 0) {
    call(ctx, m1fn::PtrVector_Construct, {v});
    store<u16>(v + 6, elem);
    call(ctx, m1fn::PtrVector_Init, {v, capacity, 0, last});
}

// The inlined delete of a heap PtrVector: PtrVector_Clear, Nop, then
// SmallFree(the pointer read again from slot, 20).
inline void PtrVecDeleteSmall(PPUContext& ctx, u32 slot) {
    const u32 v = load<u32>(slot);
    if (v == 0) return;
    call(ctx, m1fn::PtrVector_Clear, {v});
    call(ctx, m1fn::Nop, {v});
    call(ctx, m1fn::SmallFree, {load<u32>(slot), 20});
}

// Mem_FreeFrom(*slot, 0) and *slot = 0 when the slot is set.
inline void FreeSlot(PPUContext& ctx, u32 slot) {
    const u32 p = load<u32>(slot);
    if (p == 0) return;
    call(ctx, m1fn::Mem_FreeFrom, {p, 0});
    store<u32>(slot, 0);
}

}  // namespace mid1
}  // namespace sly4
