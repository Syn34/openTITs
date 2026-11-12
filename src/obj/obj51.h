// Game code, part 51: helpers shared by obj51*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from the remaining
// 112 instruction ones up).
#pragma once

#include <bit>
#include <cmath>

#include "obj50.h"

namespace sly4 {
namespace obj51 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj50;

// Single precision rounding of a double (the s-forms).
inline f64 F51(f64 x) { return static_cast<f64>(static_cast<f32>(x)); }

// lfs as a double.
inline f64 Lfs51(u32 ea) { return static_cast<f64>(load<f32>(ea)); }

// o is taken out of the 8-byte {key, T*} entry array of 0x01056F1C (u16
// count +2, data +8; the count and the data pointer read once for the
// search): the later entries move down one (the count re-read after each
// move) and the count drops.
inline void Unlist1056F1C_51(u32 o) {
    constexpr u32 A = 0x01056F1Cu;
    const u32 n = load<u16>(A + 2);
    const u32 d = load<u32>(A + 8);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(d + 8 * i + 4) != o) continue;
        u32 c = n;
        if (static_cast<s32>(n) >= 2 && static_cast<s32>(i) < static_cast<s32>(n - 1)) {
            for (u32 j = i;;) {
                const u32 e = d + 8 * j;
                ++j;
                store<u32>(e, load<u32>(e + 8));
                store<u32>(e + 4, load<u32>(e + 12));
                c = load<u16>(A + 2);
                if (!(static_cast<s32>(j) < static_cast<s32>(c - 1))) break;
            }
        }
        store<u16>(A + 2, static_cast<u16>(c - 1));
        return;
    }
}

// The destructor body of 0x002B2A70 / 0x002B50D4 (void f(T* o)): vtables
// vt0 (+0) and vt24 (+24); the PtrVectors at +124, +104, +84 and +64
// cleared (ClearVec50); Obj_BaseDtor_0048F6B0(o + 24); vtable 0x00B1BE30;
// Unlist1056F1C_51(o) and List1013B5C_Remove(o) (r3 = its).
inline void Dtor4Vec51(PPUContext& ctx, u32 vt0, u32 vt24) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt0);
    store<u32>(o + 24, vt24);
    ClearVec50(ctx, o + 124);
    ClearVec50(ctx, o + 104);
    ClearVec50(ctx, o + 84);
    ClearVec50(ctx, o + 64);
    call(ctx, 0x0048F6B0u, {o + 24});
    store<u32>(o, 0x00B1BE30u);
    Unlist1056F1C_51(o);
    call(ctx, 0x000D6940u, {o});
}

// Appends {*keyea (read after any growth), v} to the 8-byte entry array a
// (u16 capacity +0, count +2, high-water +4, flags +6 |= 1, data +8;
// Array_Reserve(a, 2 * cap) when full).
inline void ArrInsert8_51(PPUContext& ctx, u32 a, u32 keyea, u32 v) {
    const u32 cap = load<u16>(a);
    if (load<u16>(a + 2) == cap) call(ctx, 0x00558F3Cu, {a, (cap << 1) & 0xFFFEu});
    const u32 key = load<u32>(keyea);
    const u32 d = load<u32>(a + 8);
    store<u32>(d + ((static_cast<u32>(load<u16>(a + 2)) << 3) & 0x7FFF8u), key);
    store<u32>(d + (static_cast<u32>(load<u16>(a + 2)) << 3) + 4, v);
    const u32 n = load<u16>(a + 2) + 1u;
    const u32 hi = load<u16>(a + 4);
    const u32 fl = load<u16>(a + 6);
    store<u16>(a + 2, static_cast<u16>(n));
    store<u16>(a + 4, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFFu) ? hi : n));
    store<u16>(a + 6, static_cast<u16>(fl | 1u));
}

// The 28-byte XML records of 0x002AF0D4 / 0x002B1628 (SmallAlloc(28); 0
// when that fails): vtable 0x00B0DAAC, +4 = String_HashNoCase(attr), +8 =
// 0, u8 +16 = 0, +12 = kind, entered as {+4, rec} in the 8-byte entry
// array 0x01056F04 (ArrInsert8_51), then vtable vt, +20 = 0 (and +24 = 0
// with clear24).
inline u32 Rec28New51(PPUContext& ctx, u32 attr, u32 kind, u32 vt, bool clear24) {
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {28}));
    if (p == 0) return 0;
    store<u32>(p, 0x00B0DAACu);
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {attr}));
    store<u32>(p + 8, 0);
    store<u8>(p + 16, 0);
    store<u32>(p + 4, h);
    store<u32>(p + 12, kind);
    ArrInsert8_51(ctx, 0x01056F04u, p + 4, p);
    store<u32>(p, vt);
    store<u32>(p + 20, 0);
    if (clear24) store<u32>(p + 24, 0);
    return p;
}

}  // namespace obj51
}  // namespace sly4
