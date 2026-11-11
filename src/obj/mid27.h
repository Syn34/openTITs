// Game code, medium sweep part 27: helpers shared by mid27_*.cpp (more
// still-recompiled game functions of 120-400 instructions).
#pragma once

#include <bit>
#include <cmath>
#include <initializer_list>
#include <utility>

#include "mid26.h"

namespace sly4 {
namespace mid27 {

using namespace opentit;
using namespace opentit::decomp;
using mid14::Lf;
using mid6::F32;
using mid6::W32;
using obj19::VCall;
using mid26::FreeSlot26;
using mid26::RegistryRemove26;
using mid26::RegistryRemoveB26;
using mid26::Vt8Slot;

namespace m27fn {
using namespace mid26::m26fn;
constexpr u32 ListRemove = 0x000D6940u;        // List1013B5C_Remove(o)
constexpr u32 PtrVecClear = 0x0054EC1Cu;       // PtrVector_Clear(v)
constexpr u32 PtrVecNop = 0x0054EC18u;         // Nop(v)
constexpr u32 MemAlloc = 0x0054ED38u;          // Mem_Alloc(size, pool, align)
constexpr u32 SmallFree27 = 0x00552420u;       // SmallFree(p, size)
constexpr u32 ArrReserve = 0x00558F3Cu;        // Array_Reserve(a, u16 cap)
constexpr u32 IffReadU32 = 0x005642ACu;        // Iff_ReadU32(r, u32* out, name)
constexpr u32 ReadByte = 0x0056998Cu;          // FileReader_ReadByte(r, u8* out, name)
constexpr u32 ReadF32 = 0x005698F4u;           // FileReader_ReadF32Swapped(r, f32* out, name)
constexpr u32 EffRelease = 0x004CF3C4u;        // Effect_Release(e, id)
constexpr u32 RefNodeRelease = 0x004432DCu;    // RefNode_Release(node)
constexpr u32 PoolDestructPart = 0x0052EFE0u;  // Pool_DestructPart(part)
}  // namespace m27fn

constexpr u32 kRegBaseVt27 = 0x00B1BE30u;  // registered-object base vtable

// The registered-object base part: vtable 0x00B1BE30, o removed from the
// registry 0x01056F1C (count and data read once for the search, or with
// `reread` both re-read each step), then List1013B5C_Remove(o) (r3 = its).
inline void RegBase27(PPUContext& ctx, u32 o, bool reread = false) {
    store<u32>(o, kRegBaseVt27);
    if (reread) RegistryRemoveB26(o);
    else RegistryRemove26(o);
    call(ctx, m27fn::ListRemove, {o});
}

// Frees the Havok array at o + d (capacity and flags at o + c) when it owns
// its storage (flag 0x80000000 clear): the thread allocator 0x011DF31C gets
// vt[20](0x011DF31C, data, bytes); bytes = cap * elem (the low word,
// sign-extended). r6 / r7 carry the OPD and entry as the original call
// sequence leaves them: OPD then entry, or with `entryFirst` the reverse.
constexpr u32 kHkAlloc27 = 0x011DF31Cu;
inline void HkArrFree27(PPUContext& ctx, u32 o, u32 d, u32 c, u32 elem, bool entryFirst) {
    const u32 cap = load<u32>(o + c);
    if ((cap & 0x80000000u) != 0) return;
    const u32 data = load<u32>(o + d);
    const u32 opd = load<u32>(load<u32>(kHkAlloc27) + 20);
    const u32 ent = load<u32>(opd);
    const u64 n = static_cast<u64>(static_cast<s64>(static_cast<s32>(cap * elem)));
    if (entryFirst) call_ptr(ctx, opd, {kHkAlloc27, data, n, ent, opd});
    else call_ptr(ctx, opd, {kHkAlloc27, data, n, opd, ent});
}

// PtrVector_Clear(v) then Nop(v), the inlined PtrVector destructor.
inline void PtrVecDtor27(PPUContext& ctx, u32 v) {
    call(ctx, m27fn::PtrVecClear, {v});
    call(ctx, m27fn::PtrVecNop, {v});
}

// Appends {key, val} to the 8-byte array a (u16 cap +0, count +2, peak +4,
// flags +6, data +8): Array_Reserve(a, cap * 2) when full; flags bit 0
// set; the peak is kept when above the new count (signed compare of the
// u16s). `countFirst` stores the count before the value word (else after).
inline void PairPush27(PPUContext& ctx, u32 a, u32 key, u32 val, bool countFirst) {
    const u32 cap = load<u16>(a);
    if (load<u16>(a + 2) == cap) call(ctx, m27fn::ArrReserve, {a, (cap << 1) & 0xFFFEu});
    const u32 c = load<u16>(a + 2);
    const u32 d = load<u32>(a + 8);
    store<u32>(d + ((c << 3) & 0x7FFF8u), key);
    const u32 n = c + 1;
    if (countFirst) store<u16>(a + 2, static_cast<u16>(n));
    store<u32>(d + 8 * c + 4, val);
    if (!countFirst) store<u16>(a + 2, static_cast<u16>(n));
    const u32 peak = load<u16>(a + 4);
    store<u16>(a + 6, static_cast<u16>(load<u16>(a + 6) | 1u));
    store<u16>(a + 4, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(n & 0xFFFFu) ? peak : n));
}

}  // namespace mid27
}  // namespace sly4
