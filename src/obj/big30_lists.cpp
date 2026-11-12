// Game code, large-function sweep part 30: a section's readiness pass
// over its five child lists (+2232..+2264) and its entry references
// (+2272), choosing the mode it reports to its vfn[26].
#include "../sly4.h"
#include "big30.h"

using namespace sly4;
using namespace sly4::big30;

namespace {
constexpr u32 Obj_CanShow2E2104 = 0x002E2104;  // (ctx, Obj*) -> bool (reads r4 only)
constexpr u32 Ref_Resolve = 0x00321324;        // (Ref*, u8 warn)
constexpr u32 kShowCtx = 0x00F64C20;

// A list slot {Header** h, u32 status}: the u16 count +8 of *h (0 without
// one), stored as the u8 +4, then the status word with that count in its top
// byte and bit 23 = ready (two stores, as the original).
inline void ListStatus(u32 L, u32 n, bool ready) {
    store<u8>(L + 4, static_cast<u8>(n));
    u32 w = (load<u32>(L + 4) & 0x00FFFFFFu) | (n << 24);
    w = ready ? (w | 0x00800000u) : (w & ~0x00800000u);
    store<u32>(L + 4, w);
}
inline u32 ListCount(u32 L) {
    const u32 h = load<u32>(L);
    if (h == 0) return 0;
    const u32 a = load<u32>(h);
    return a != 0 ? load<u16>(a + 8) : 0;
}
// The word of o's +12 table selected by +2452 (its bit 5 / top nibble).
inline u32 StateWord(u32 o) { return load<u32>(o + 12) + (load<u32>(o + 2452) << 2) + 0x20074; }
}  // namespace

// void f(Section* o, u8 on, s32 k) (called by Section_SetChildren_003277E4,
// Recs_Resolve_0032C3F4, Obj_Start_0033F834).
// 1. For each of the five lists L = o + 2232 + 8 j: every element e (a
//    pointer array) gets +24 = 0 and in +28 bit 26 = bit 29 = (bits 30 and
//    29 were both set), bits 28 and 27 cleared; L's status gets the count
//    (0 unless every element was ready) and the ready bit.
// 2. With on, the top nibble m of the state word clears o->+20 bits 2 / 1 /
//    0 for m > 1 / 2 / 3, and m == 5 is remembered.
// 3. The mode: k == -1: 5 (on and m == 5, with +17 = +16) or else 4 / 3 /
//    2 / 1 by o->+20 bits 0..2 (+17 = 0); other k: +17 = (k == 5 ? +16 :
//    0), and k with flag = (k == 5 || on) unless k != 5, o->+20 bit 4 and
//    o->+18 == 5, which keep mode 0 and flag = on.
// 4. Each 20-byte entry of o->+2272 (count u16 +8, re-read) loses bits
//    31..30 of +8, which become 10 / 01 (u8 +9 set / clear) when its u8
//    +10 is set and (its u8 +11 is set or the mode is 5); +11, +12 and
//    +16..19 are zeroed and, when Obj_CanShow2E2104(*0x00F64C20, o),
//    Ref_Resolve(e, 0).
// 5. A non-zero mode goes to o->vfn[26](o, mode, flag). Then in state 5
//    (u8 +18) the lists +2232, +2248, +2240 (flag set: and the state
//    word's bit 26 is cleared) or +2256, +2264 (flag clear) are marked ready
//    with their counts. No caller reads r3 or other leftovers.
DECOMP(0032549C, Section_Ready_0032549C) {
    const u32 o = arg<u32>(ctx, 0);
    u32 flag = static_cast<u8>(ctx.gpr[4]);
    const s32 k = static_cast<s32>(ctx.gpr[5]);
    for (u32 j = 0; j < 5; ++j) {
        const u32 L = o + 2232 + 8 * j;
        bool notReady = false;
        u32 h = load<u32>(L);
        if (h != 0 && load<u32>(h) != 0 && load<u16>(load<u32>(h) + 8) != 0) {
            for (u32 i = 0, off = 0;; ++i, off += 4) {
                const u32 a = load<u32>(h);
                const u32 n = a != 0 ? load<u16>(a + 8) : 0;
                if (i >= n) break;
                const u32 e = load<u32>(load<u32>(a) + off);
                u32 w = load<u32>(e + 28);
                store<u32>(e + 24, 0);
                const bool rdy = (w & 0x20000000u) && (w & 0x40000000u);
                w &= ~0x3C000000u;
                if (rdy) w |= 0x24000000u;
                store<u32>(e + 28, w);
                if (!rdy) notReady = true;
                h = load<u32>(L);
            }
        }
        u32 n = 0;
        if (h != 0 && !notReady) {
            const u32 a = load<u32>(h);
            if (a != 0) n = load<u16>(a + 8);
        }
        ListStatus(L, n, !notReady);
    }
    bool m5 = false;
    if (flag != 0) {
        const u32 m = load<u32>(StateWord(o)) >> 28;
        u32 w = load<u32>(o + 20);
        if (m > 1) { w &= ~4u; store<u32>(o + 20, w); }
        if (m > 2) { w &= ~2u; store<u32>(o + 20, w); }
        if (m > 3) { w &= ~1u; store<u32>(o + 20, w); }
        m5 = m == 5;
    }
    u32 mode = 0;
    if (k == -1) {
        if (m5) {
            store<u8>(o + 17, load<u8>(o + 16));
            mode = 5;
            flag = 1;
        } else {
            const u32 w = load<u32>(o + 20);
            store<u8>(o + 17, 0);
            mode = 4;
            if (w & 1) mode = 3;
            if (w & 2) mode = 2;
            if (w & 4) mode = 1;
        }
    } else {
        store<u8>(o + 17, k == 5 ? load<u8>(o + 16) : 0);
        if (k == 5 || !(load<u32>(o + 20) & 0x10) || load<u8>(o + 18) != 5) {
            store<u8>(o + 17, k == 5 ? load<u8>(o + 16) : 0);  // stored again on this path
            mode = static_cast<u32>(k);
            flag = (k == 5 || flag != 0) ? 1 : 0;
        }
    }
    const u32 arr = load<u32>(o + 2272);
    if (arr != 0 && load<u16>(arr + 8) != 0) {
        const u32 show = static_cast<u32>(call(ctx, Obj_CanShow2E2104, {load<u32>(kShowCtx), o}));
        for (u32 i = 0, off = 0;; ++i, off += 20) {
            const u32 a = load<u32>(o + 2272);
            const u32 n = a != 0 ? load<u16>(a + 8) : 0;
            if (i >= n) break;
            const u32 e = load<u32>(a) + off;
            const u32 w = load<u32>(e + 8);
            store<u32>(e + 8, w & 0x3FFFFFFFu);
            if (((w & 0xFF) != 0 || mode == 5) && load<u8>(e + 10) != 0) {
                const u32 top = load<u8>(e + 9) != 0 ? 0x80000000u : 0x40000000u;
                store<u32>(e + 8, (load<u32>(e + 8) & 0x3FFFFFFFu) | top);
            }
            store<u8>(e + 11, 0);
            store<u8>(e + 12, 0);
            for (u32 b = 0; b < 4; ++b) store<u8>(e + 16 + b, 0);
            if (show != 0) call(ctx, Ref_Resolve, {e, 0});
        }
    }
    if (mode != 0) VCall(ctx, o, 104, {o, mode, flag});
    if (load<u8>(o + 18) != 5) return;
    if (flag != 0) {
        for (u32 off : {2232u, 2248u, 2240u}) ListStatus(o + off, ListCount(o + off), true);
        const u32 p = StateWord(o);
        store<u32>(p, load<u32>(p) & ~0x04000000u);
    } else {
        ListStatus(o + 2256, ListCount(o + 2256), true);
        ListStatus(o + 2264, ListCount(o + 2264), true);
    }
}
