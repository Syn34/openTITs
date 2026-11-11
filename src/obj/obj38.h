// Game code, part 38: helpers shared by obj38*.cpp (the continued sweep of
// small still-recompiled game functions, in size order, from the last 76
// instruction ones up).
#pragma once

#include <bit>
#include <cmath>

#include "obj37.h"

namespace sly4 {
namespace obj38 {

using namespace opentit;
using namespace opentit::decomp;
using namespace sly4::obj37;

namespace o38fn {
constexpr u32 Hash_ToString = 0x0056A6E0;   // (u32 hash) -> const char*
constexpr u32 String_Format = 0x005534D0;   // (fmt, ...) -> char*
constexpr u32 Snd_Lock = 0x006EFD10;
constexpr u32 Snd_Unlock = 0x006EFD34;
}  // namespace o38fn

namespace o38imp {
constexpr u32 sys_net_accept = 0x00ABDDA4;  // (s32 socket, sockaddr* addr, u32* addrlen)
}  // namespace o38imp

// fsel: a >= 0 ? b : c (a NaN selects c).
inline f64 Fsel38(f64 a, f64 b, f64 c) { return a >= 0.0 ? b : c; }

// Single precision rounding of a double (the s-forms).
inline f64 F38(f64 x) { return static_cast<f64>(static_cast<f32>(x)); }

// The tail of the destructors 0x00280780 / 0x0028F2B0 (void f(T* o)):
// vtable vt, World_ListRemove_0046B7A4(*0x00F65CA0, list, tbl, o, o->+8),
// vtable 0x00B1BE30; o is taken out of the 8-byte entry array of
// 0x01056F1C (u16 count +2, data +8, the object in each entry's second
// word; the later entries moved down one, the count decreased) when found,
// then List1013B5C_Remove(o) (r3 = its).
inline void DtorWorldUnlist38(PPUContext& ctx, u32 vt, u32 list, u32 tbl) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt);
    call(ctx, 0x0046B7A4u, {load<u32>(0x00F65CA0u), list, tbl, o, load<u32>(o + 8)});
    constexpr u32 A = 0x01056F1Cu;
    store<u32>(o, 0x00B1BE30u);
    const u32 n = load<u16>(A + 2);
    const u32 d = load<u32>(A + 8);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(d + 8 * i + 4) != o) continue;
        u32 c = n;
        if (static_cast<s32>(n) >= 2 && static_cast<s32>(i) < static_cast<s32>(n - 1)) {
            for (u32 j = i;;) {
                const u32 e = d + 8 * j;
                ++j;
                store<u32>(d + 8 * (j - 1), load<u32>(e + 8));
                store<u32>(e + 4, load<u32>(e + 12));
                c = load<u16>(A + 2);
                if (!(static_cast<s32>(j) < static_cast<s32>(c - 1))) break;
            }
        }
        store<u16>(A + 2, static_cast<u16>(c - 1));
        break;
    }
    call(ctx, 0x000D6940u, {o});
}

// The pool setup of 0x00789280 / 0x0077A9AC / 0x0077AAE0 (void f(Pool*
// out, u32 n, u32 extra, Allocator* a), elements of esize bytes): for n
// != 0, one block of (esize * n + (extra ? 4 n : 0) + 31) & ~15 bytes from
// a->vt[0](a, size, 0, 16) (OPD / entry after the arguments): out->+0 =
// out->+4 = block; the block header (u16 +2, +4, +8 cleared when the block
// is not 0) gets u16 +0 = n (written even to a null block: a quirk), +8 =
// extra, +4 = size, +12 = a; out->+8 = block + 16 when extra (else 0) and
// out->+12 = block + 16 (+ 4 * n when extra). r3 = out->+12, or out for n
// = 0.
inline void PoolInit38(PPUContext& ctx, u32 esize) {
    const u32 out = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1), extra = arg<u32>(ctx, 2), a = arg<u32>(ctx, 3);
    if (n == 0) return;
    const u32 size = (esize * n + (extra != 0 ? n << 2 : 0u) + 31u) & ~15u;
    const u32 p = static_cast<u32>(VCall(ctx, a, 0, {a, size, 0, 16}));
    store<u32>(out, p);
    store<u32>(out + 4, p);
    if (p != 0) {
        store<u16>(p + 2, 0);
        store<u32>(p + 4, 0);
        store<u32>(p + 8, 0);
    }
    store<u16>(p, static_cast<u16>(n));
    const u32 h = load<u32>(out + 4);
    store<u32>(h + 8, extra);
    store<u32>(h + 4, size);
    store<u32>(h + 12, a);
    const u32 b = load<u32>(out);
    const u32 x = extra != 0 ? p + 16 : 0u;
    store<u32>(out + 8, x);
    const u32 r = x != 0 ? b + (static_cast<u32>(load<u16>(h)) << 2) + 16 : b + 16;
    store<u32>(out + 12, r);
    ret(ctx, r);
}

// The entity removal of 0x003A9D7C / 0x003A9C44 (u32 f(Owner* a,
// HkEntity* b)): the properties 0x0026E1FE and 0x516CC502 are removed
// from b when present in its 16-byte property array (+124, count +128;
// Hk_RemoveProperty93FED8 into a stack result, the array re-read after the
// first); b leaves the map a + moff (SortedMap_Remove7597DC on a stack
// copy of b), then 0x0091A228(b, a + o1) and 0x0091AE28(b, a + o2) (r3 =
// its).
inline void HkRemoveEntity38(PPUContext& ctx, u32 moff, u32 o1, u32 o2) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    Stack fr(ctx, 32);  // results +0 / +8, key +16 (the original's 112..131)
    s32 n = load<s32>(b + 128);
    u32 p = load<u32>(b + 124);
    for (s32 i = 0; i < n; ++i) {
        if (load<u32>(p + 16 * i) != 0x0026E1FEu) continue;
        call(ctx, 0x0093FED8u, {fr.addr(), b, 0x0026E1FEu});
        n = load<s32>(b + 128);
        p = load<u32>(b + 124);
        break;
    }
    for (s32 i = 0; i < n; ++i) {
        if (load<u32>(p + 16 * i) != 0x516CC502u) continue;
        call(ctx, 0x0093FED8u, {fr.addr() + 8, b, 0x516CC502u});
        break;
    }
    store<u32>(fr.addr() + 16, b);
    call(ctx, 0x007597DCu, {a + moff, fr.addr() + 16});
    call(ctx, 0x0091A228u, {b, a + o1});
    call(ctx, 0x0091AE28u, {b, a + o2});
}

// The close of 0x00405358 / 0x0041F0C0 (u32 f(T* o)): when the u8 o->+68
// is set: World_ListRemove_0046B7A4(*0x00F65CA0, 0, tbl, o, 0),
// Handle_Notify36742C(h1, 0, 1, 0), SetSelect_491000(o, h2), the view
// o->+voff gets vt[22](v, 0, 1, 1, 1) (OPD / entry after the arguments);
// then for the stack s = (o->vt[19](o))->+0x1281E0 (when set) holding
// o->+60 (Obj_HasWord368_000FC5DC): Obj_PopLayer(s, o->+60, o->+60; f1 =
// the f32 at kaddr, -1.0). r3 = 0, the vt[19] result (no stack) or the
// last call's.
inline void ObjClose38(PPUContext& ctx, u32 tbl, u32 h1, u32 h2, u32 voff, u32 kaddr) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(o + 68) == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x0046B7A4u, {load<u32>(0x00F65CA0u), 0, tbl, o, 0});
    call(ctx, 0x0036742Cu, {h1, 0, 1, 0});
    call(ctx, 0x00491000u, {o, h2});
    const u32 v = load<u32>(o + voff);
    VCall(ctx, v, 88, {v, 0, 1, 1, 1});
    const u32 g = static_cast<u32>(VCall(ctx, o, 76, {o}));
    const u32 s = load<u32>(g + 0x001281E0u);
    if (s == 0) {
        ret(ctx, g);
        return;
    }
    if (static_cast<u32>(call(ctx, 0x000FC5DCu, {s, load<u32>(o + 60)})) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 l = load<u32>(o + 60);
    callf(ctx, 0x000FBB88u, {s, l, l}, {Lfs32(kaddr)});
}

// The animation variable copy of 0x00391FF4 / 0x00393104 (words),
// 0x003925A4 (f32s) and 0x00392B54 (bytes) (u32 f(Cmd* c, Anim* o)): the
// 132-byte variable record i = s16 c->+6 of o (+152, u16 count +144; used
// while i is below the count and the record's +0 below 10, else address
// 0, read anyway) gives the s16 slot v at record + u16 *0x0106CF80 + 4;
// for v != -1, the value of the 128-byte entry pair (*0x00F65038)->+24 of
// s16 c->+4 / c->+2 (as in 0x0039366C: the byte +1 of the pair at entry
// c2 + u16 *0x0106CF7E selects a word at entry c4 + u16 *0x0106CF70 + 4 *
// it) is copied to the record of u16 o->+146 at its + u16 +6 + 4 v. r3 =
// 0.
inline void AnimVarCopy38(PPUContext& ctx, u32 size) {
    const u32 c = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const s32 i = static_cast<s16>(load<u16>(c + 6));
    u32 rec = o + 152 + static_cast<u32>(i) * 132u;
    if (!(i < static_cast<s32>(load<u16>(o + 144))) || !(load<s32>(rec) < 10)) rec = 0;
    const s32 v = static_cast<s16>(load<u16>(rec + load<u16>(0x0106CF80u) + 4));
    ctx.gpr[3] = 0;
    if (v == -1) return;
    const s32 a = static_cast<s16>(load<u16>(c + 4)), b = static_cast<s16>(load<u16>(c + 2));
    const u32 rec2 = o + 152 + static_cast<u32>(load<u16>(o + 146)) * 132u;
    const u32 g = load<u32>(0x00F65038u);
    const u32 ea = a != -1 ? load<u32>(g + 24) + (static_cast<u32>(a) << 7) : 0u;
    const u32 eb = b != -1 ? load<u32>(g + 24) + (static_cast<u32>(b) << 7) : 0u;
    const u32 pb = load<u8>(eb + 2) != 0 ? eb + load<u16>(0x0106CF7Eu) : 0u;
    const u32 src = ea + load<u16>(0x0106CF70u) + (static_cast<u32>(load<u8>(pb + 1)) << 2);
    const u32 dst = rec2 + load<u16>(rec2 + 6) + (static_cast<u32>(v) << 2);
    if (size == 1) {
        store<u8>(dst, load<u8>(src));
    } else {
        const u32 w = load<u32>(src);
        store<u32>(dst, w);
        if (size == 5) ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(w));  // the f32 form (lfs / stfs)
    }
}

}  // namespace obj38
}  // namespace sly4
