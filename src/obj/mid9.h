// Game code, medium sweep part 9: helpers shared by mid9_*.cpp (still
// recompiled game functions of 120-400 instructions: the callees part 8
// named, def readers on Def_ReadBase20BFE8, damage / collision def
// readers, slot objects).
#pragma once

#include <bit>
#include <cmath>

#include "mid8.h"

namespace sly4 {
namespace mid9 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using mid5::XBool;
using mid5::XF32;
using mid5::XHash;
using mid5::XRead;
using mid5::XS32;
using mid6::F32;
using mid6::W32;
using mid6::XF32Def;
using mid6::XVec3;
using mid7::XBoolVia;
namespace xfn = mid5::xfn;
namespace m7fn = mid7::m7fn;
namespace m8fn = mid8::m8fn;

namespace m9fn {
constexpr u32 Set_Find = 0x004BCE90;                // (u32 name) -> effect set (reads r3 only)
constexpr u32 Rec_CopyFields_000C387C = 0x000C387C;  // (a0, c, x, tmpl, o) (reads r5 / r7 only)
constexpr u32 Obj_Construct_00769BD8 = 0x00769BD8;   // (o, x) (reads r3 / r4 only)
constexpr u32 PtrVector_Reserve = 0x00551718;       // (vec, capacity)
constexpr u32 Def_ReadBase20BFE8 = 0x0020BFE8;      // (a0, c, x, tmpl, o)
constexpr u32 Thunk_000C2B90 = 0x000C2B90;          // (o, const Record* r): the trigger def base constructor (reads r3 / r4)
constexpr u32 Obj_BaseDtor_000C2DA0 = 0x000C2DA0;   // (o) (reads r3)
}  // namespace m9fn

// The def of the node the last effect-def lookup found (see mid1).
constexpr u32 kEffectLastDef = 0x011DE288;

// The inlined lookup of key in the def tree of the default effect set
// (Set_Find(0): +16 the root of a BST {+4 key, +8 def, +12 left (keys
// below), +16 right}; the path walked kept in +20[] with its length in the
// s16 +100, the last node visited in +104, the def found in
// kEffectLastDef). Returns the def found (0 when the key is missing).
// s is the set Set_Find returned (not 0).
inline u32 EffectDefLookup(u32 s, u32 key) {
    u32 n = load<u32>(load<u32>(s + 16));
    if (n == 0) {
        store<u32>(s + 104, 0);
        store<u32>(kEffectLastDef, 0);
        return 0;
    }
    store<u16>(s + 100, 0);
    for (;;) {
        const u32 k = load<u32>(n + 4);
        if (k == key) break;
        const u32 next = load<u32>(n + (k > key ? 12 : 16));
        if (next == 0) {
            store<u32>(s + 104, n);  // the would-be parent
            return 0;
        }
        const s32 depth = static_cast<s16>(load<u16>(s + 100));
        store<u16>(s + 100, static_cast<u16>(depth + 1));
        store<u32>(s + 20 + 4 * static_cast<u32>(depth), n);  // unchecked, as the original
        n = next;
    }
    store<u32>(s + 104, n);
    store<u32>(kEffectLastDef, load<u32>(n + 8));
    const u32 found = load<u32>(s + 104);
    return found != 0 ? load<u32>(found + 8) : 0;
}

// {key, value} appended to a {+0 u16 cap, +2 u16 count, +4 u16 high-water,
// +6 u16 flags, +8 {u32, u32}[]} registry with the store order key, value,
// count, high-water, flags |= 1 (the count re-read between the stores).
// Returns the flags stored.
inline u32 RegPushKV2(PPUContext& ctx, u32 R, u32 key, u32 value) {
    const u32 cap = load<u16>(R);
    if (load<u16>(R + 2) == cap) call(ctx, m8fn::Array_Reserve, {R, (cap << 1) & 0xFFFEu});
    const u32 data = load<u32>(R + 8);
    store<u32>(data + ((load<u16>(R + 2) << 3) & 0x7FFF8u), key);
    store<u32>(data + (load<u16>(R + 2) << 3) + 4, value);
    const u32 n1 = load<u16>(R + 2) + 1u, hw = load<u16>(R + 4), fl = load<u16>(R + 6) | 1u;
    store<u16>(R + 2, static_cast<u16>(n1));
    store<u16>(R + 4, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFF) ? hw : n1));
    store<u16>(R + 6, static_cast<u16>(fl));
    return fl;
}

// The trigger defs (the 0020BFE8 family, vtable 0x00B19510 over the
// Thunk_000C2B90 base): the record they are built from, as the inlined
// default constructors fill it on the stack: +0 flags, +4 w4, +8 0, +12
// x->12, +16 / +20 the empty string 0x00AD366C, bytes +24..+31 0, +32 w32,
// f32 +36 / +40 -1, byte +44 0, +48..+68 0, f32 +72 -1, +76 the objPrefix
// default, bools +80 1, +81..+83 0.
constexpr u32 kTrigVt = 0x00B19510;
constexpr u32 kStrEmpty366C = 0x00AD366C;
inline void TrigRecInit(u32 rec, u32 x, u32 flags, u32 w4, u32 w32, u32 prefix) {
    store<u32>(rec, flags);
    store<u32>(rec + 4, w4);
    store<u32>(rec + 8, 0);
    store<u32>(rec + 12, load<u32>(x + 12));
    store<u32>(rec + 16, kStrEmpty366C);
    store<u32>(rec + 20, kStrEmpty366C);
    for (u32 k = 24; k < 32; ++k) store<u8>(rec + k, 0);
    store<u32>(rec + 32, w32);
    store<u32>(rec + 36, 0xBF800000u);
    store<u32>(rec + 40, 0xBF800000u);
    store<u8>(rec + 44, 0);
    for (u32 k = 48; k <= 68; k += 4) store<u32>(rec + k, 0);
    store<u32>(rec + 72, 0xBF800000u);
    store<u32>(rec + 76, prefix);
    store<u8>(rec + 80, 1);
    store<u8>(rec + 81, 0);
    store<u8>(rec + 82, 0);
    store<u8>(rec + 83, 0);
}
// The inlined trigger def constructor from rec: Thunk_000C2B90(o, rec),
// bools +172..+175 = rec+80..83 (the vtable 0x00B19510 stored after the
// first vtAfter of them), the objPrefix rec+76 copied (31 chars) to +140.
inline void TrigFromRec(PPUContext& ctx, u32 o, u32 rec, u32 vtAfter) {
    call(ctx, m9fn::Thunk_000C2B90, {o, rec});
    for (u32 k = 0; k < 4; ++k) {
        if (k == vtAfter) store<u32>(o, kTrigVt);
        store<u8>(o + 172 + k, load<u8>(rec + 80 + k));
    }
    call(ctx, m8fn::String_CopyN, {o + 140, load<u32>(rec + 76), 31});
}

// The VMX vector {a, b, c, 0} as the original builds it: three splats
// merged with vsel through the masks at m + 0 / m + 16, then vandc with the
// mask at m + 32 (normally {0, ~0, 0, 0}, {0, 0, ~0, 0}, {0, 0, 0, ~0}).
inline void Vec3Sel(u32 dst, u32 m, u32 a, u32 b, u32 c) {
    for (u32 i = 0; i < 4; ++i) {
        const u32 m1 = load<u32>(m + 4 * i), m2 = load<u32>(m + 16 + 4 * i), m3 = load<u32>(m + 32 + 4 * i);
        u32 w = (a & ~m1) | (b & m1);
        w = (w & ~m2) | (c & m2);
        store<u32>(dst + 4 * i, w & ~m3);
    }
}

// The u32 word at src copied to dst (an lfs / stfs pair).
inline void CopyW(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }

}  // namespace mid9
}  // namespace sly4
