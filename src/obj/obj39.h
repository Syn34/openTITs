// Game code, part 39: helpers shared by obj39*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from the 80
// instruction ones up).
#pragma once

#include <bit>
#include <cmath>

#include "obj38.h"

namespace sly4 {
namespace obj39 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj38;

// Single precision rounding of a double (the s-forms).
inline f64 F39(f64 x) { return static_cast<f64>(static_cast<f32>(x)); }

// Sign extension of a 32-bit value to the 64-bit register (extsw).
inline u64 Sx39(u32 v) { return static_cast<u64>(static_cast<s64>(static_cast<s32>(v))); }

// o is taken out of the 8-byte entry array of 0x01056F1C (u16 count +2,
// data +8, the object in each entry's second word; the later entries move
// down one and the count drops) when found (0x002CBEAC / 0x0027BB20).
inline void Unlist1056F1C39(u32 o) {
    constexpr u32 A = 0x01056F1Cu;
    for (u32 i = 0; i < load<u16>(A + 2); ++i) {
        const u32 d = load<u32>(A + 8);
        if (load<u32>(d + 8 * i + 4) != o) continue;
        u32 c = load<u16>(A + 2);
        if (static_cast<s32>(c) >= 2 && static_cast<s32>(i) < static_cast<s32>(c - 1)) {
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

// The deleting destructor body of 0x0027CF60 / 0x0027EA94 / 0x00280244
// and their adjustor thunks (o = this - 80): vtables vt0 (+0) and vt80
// (+80), Dtor_B219D0_00464AD0(o + 80), vtable 0x00B1BE30,
// Unlist1056F1C39(o), List1013B5C_Remove(o) and SmallFree(o, 112) (r3 =
// its).
inline void DeletePair112Body39(PPUContext& ctx, u32 o, u32 vt0, u32 vt80) {
    store<u32>(o, vt0);
    store<u32>(o + 80, vt80);
    call(ctx, 0x00464AD0u, {o + 80});
    store<u32>(o, 0x00B1BE30u);
    Unlist1056F1C39(o);
    call(ctx, 0x000D6940u, {o});
    call(ctx, 0x00552420u, {o, 112});
}
// void f(T* o): DeletePair112Body39 for o != 0 (r3 = 0 otherwise).
inline void DeletePair112_39(PPUContext& ctx, u32 vt0, u32 vt80) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    DeletePair112Body39(ctx, o, vt0, vt80);
}
// void f(Base80* p): the thunk through the base at +80: o = p - 80, then
// DeletePair112Body39 for o != 0 (r3 = p otherwise).
inline void AdjDeletePair112_39(PPUContext& ctx, u32 vt0, u32 vt80) {
    const u32 o = arg<u32>(ctx, 0) - 80;
    if (o == 0) return;
    DeletePair112Body39(ctx, o, vt0, vt80);
}

// The XML node constructor of 0x0008B21C / 0x0008C9A4 (T* f(?, XmlCursor*
// x)): a 28-byte node (Mem_Alloc(28, 0, 16); r3 = 0 on failure) set up by
// Obj_InitVt_B21C98(p, x), +12 / +16 / +20 = 0, u8 +24 / +25 = 0, vtable
// vt; hashes 0x8A93973F -> +12 (default 0), 0x7A30D7E3 -> +16, 0xCCEB9AF9
// -> +20, bools 0xCADB1721 -> +24, 0xF5897C08 -> +25. r3 = p.
inline void XmlNode28_39(PPUContext& ctx, u32 vt) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {28, 0, 16}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, x});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u8>(p + 24, 0);
    store<u8>(p + 25, 0);
    store<u32>(p, vt);
    call(ctx, 0x004FA14Cu, {x, 0x8A93973Fu, p + 12, 0});
    call(ctx, 0x004FA14Cu, {x, 0x7A30D7E3u, p + 16, load<u32>(p + 16)});
    call(ctx, 0x004FA14Cu, {x, 0xCCEB9AF9u, p + 20, load<u32>(p + 20)});
    call(ctx, 0x004F986Cu, {x, 0xCADB1721u, p + 24, load<u8>(p + 24)});
    call(ctx, 0x004F986Cu, {x, 0xF5897C08u, p + 25, load<u8>(p + 25)});
    ret(ctx, p);
}

}  // namespace obj39
}  // namespace sly4
