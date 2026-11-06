// Game code, medium sweep part 3: helpers shared by mid3_*.cpp (still
// recompiled game functions of 120-400 instructions, in coherent groups:
// the tuning-variable static constructors / destructors, XML readers, the
// slot-def factories and section helpers left over from part 2, ...).
#pragma once

#include <bit>
#include <cmath>

#include "mid2.h"

namespace sly4 {
namespace mid3 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using obj19::VCall;
using obj19::VecDtor;

namespace m3fn {
constexpr u32 Mem_Alloc = 0x0054ED38;            // (size, pool, align)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;         // (p, pool)
constexpr u32 SmallFree = 0x00552420;            // (p, size)
constexpr u32 Array_Destruct = 0x0054EC94;       // (Array*) (empty)
constexpr u32 Array_Init = 0x00558F38;           // (Array*)
constexpr u32 Mem_DefaultAllocator = 0x00550EE8;  // () -> Allocator*
constexpr u32 PtrVector_Construct = 0x005516E0;  // (vec)
constexpr u32 PtrVector_Reserve = 0x00551718;    // (vec, capacity)
constexpr u32 String_CopyN = 0x00553638;         // (dst, src, n)
constexpr u32 String_Length = 0x00555FA4;        // strlen
constexpr u32 String_Assign = 0x00555F80;        // strcpy
constexpr u32 String_HashNoCase = 0x0056A710;    // (s) -> u32
constexpr u32 TuningVar_ConstructBase = 0x004FBFA8;  // (TuningVar*, const char* name)
constexpr u32 TuningVar_DestructBase = 0x004FC1A0;   // (TuningVar*)
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;  // (cursor, name) -> const char* or 0
constexpr u32 SlotDefList_Read_0035C3E4 = 0x0035C3E4;  // (SlotDefList*, x, XmlCursor*) -> s
constexpr u32 Obj_BaseDtor_0048F6B0 = 0x0048F6B0;      // (o)
constexpr u32 Dtor_B219D0_00490B34 = 0x00490B34;       // (o): ~T() of vtable 0x00B219D0
}  // namespace m3fn

// ---------------------------------------------------------------- tuning variables

// The TuningVar classes (76 / 80-byte globals: base 0x004FBFA8, the value
// from +72), by the type of their value.
constexpr u32 kVtBool = 0x00B0D934;    // u8 +72
constexpr u32 kVtInt = 0x00B0D968;     // s32 +72
constexpr u32 kVtFloat = 0x00B0D99C;   // f32 +72
constexpr u32 kVtVec3 = 0x00B0D9D0;    // f32[3] +80
constexpr u32 kVtColor = 0x00B0DA04;   // u8[4] +72
constexpr u32 kVtString = 0x00B0DA38;  // char[128] +72
constexpr u32 kVtHash = 0x00B0DA6C;    // u32 hash +72
constexpr u32 kVtVec2 = 0x00B13940;    // f32[2] +72

// The inlined T::T(name) of a tuning-var global: the base constructor,
// then the vtable (the value is stored by the caller).
inline void TvNew(PPUContext& ctx, u32 v, u32 name, u32 vt) {
    call(ctx, m3fn::TuningVar_ConstructBase, {v, name});
    store<u32>(v, vt);
}
inline void TvBool(PPUContext& ctx, u32 v, u32 name, u8 x) {
    TvNew(ctx, v, name, kVtBool);
    store<u8>(v + 72, x);
}
inline void TvInt(PPUContext& ctx, u32 v, u32 name, s32 x) {
    TvNew(ctx, v, name, kVtInt);
    store<s32>(v + 72, x);
}
inline void TvHash(PPUContext& ctx, u32 v, u32 name, u32 x) {
    TvNew(ctx, v, name, kVtHash);
    store<u32>(v + 72, x);
}
// f32 values are given as their bit patterns (stored with stw).
inline void TvFloat(PPUContext& ctx, u32 v, u32 name, u32 bits) {
    TvNew(ctx, v, name, kVtFloat);
    store<u32>(v + 72, bits);
}
inline void TvVec3(PPUContext& ctx, u32 v, u32 name, u32 x, u32 y, u32 z) {
    TvNew(ctx, v, name, kVtVec3);
    store<u32>(v + 80, x);
    store<u32>(v + 84, y);
    store<u32>(v + 88, z);
}
// rgba packed as 0xRRGGBBAA, stored byte by byte.
inline void TvColor(PPUContext& ctx, u32 v, u32 name, u32 rgba) {
    TvNew(ctx, v, name, kVtColor);
    for (u32 k = 0; k < 4; ++k) store<u8>(v + 72 + k, static_cast<u8>(rgba >> (24 - 8 * k)));
}
inline void TvString(PPUContext& ctx, u32 v, u32 name, u32 s) {
    TvNew(ctx, v, name, kVtString);
    call(ctx, m3fn::String_CopyN, {v + 72, s, 127});
}

// The inlined ~T() of a tuning-var global: its vtable back, then the base.
inline void TvDtor(PPUContext& ctx, u32 v, u32 vt) {
    store<u32>(v, vt);
    call(ctx, m3fn::TuningVar_DestructBase, {v});
}

// ---------------------------------------------------------------- pool arrays

// The inlined ~Array() of a 16-byte pool array {+0 u16 element size, +6
// u16 capacity << 1, +8 data, +12 allocator}: the data handed back through
// the allocator's vfn[2](allocator, data, bytes), then Array_Destruct.
inline void PoolArrayDtor(PPUContext& ctx, u32 a) {
    const u32 bytes = static_cast<u32>(load<u16>(a)) * ((static_cast<u32>(load<u16>(a + 6)) >> 1) & 0x7FFFu);
    const u32 data = load<u32>(a + 8);
    if (data != 0) {
        const u32 al = load<u32>(a + 12);
        VCall(ctx, al, 8, {al, data, bytes});
    }
    call(ctx, m3fn::Array_Destruct, {a});
}

}  // namespace mid3
}  // namespace sly4
