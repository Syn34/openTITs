// Game code, large-function sweep part 30: a section's readiness pass
// over its five child lists (+2232..+2264) and its entry references
// (+2272), choosing the mode it reports to its vfn[26].
#include "big30.h"

namespace sly4::obj {

namespace {
constexpr u32 Obj_CanShow2E2104 = 0x002E2104;  // (ctx, Obj*) -> u32 (reads r4 only)
constexpr u32 kShowCtx = 0x00F64C20;

// An element of a child list: +24 cleared, +28 the ready bits.
struct ReadyElem {
    pad<24> _0;
    be_u32 w24;    // +24
    be_u32 flags;  // +28: bits 30 / 29 ready inputs, 29..26 the status
};
static_assert(offsetof(ReadyElem, flags) == 28);
// A list slot {ListHdr** h, status: u8 count +4 overlapping the word's
// top byte, bit 23 ready}; *h is a PtrVec of ReadyElem.
struct ReadyList {
    gp<gp<PtrVec<ReadyElem>>> h;  // +0
    WordBytes status;             // +4
};
static_assert(sizeof(ReadyList) == 8);
// The section: +12 the state table (words at +0x20074, indexed by +2452),
// u8s +16..+18, +20 the request bits, five lists, the entry references.
struct ReadySection {
    pad<12> _0;
    be_u32 states;          // +12
    u8 b16;                 // +16
    u8 b17;                 // +17
    u8 state;               // +18
    pad<1> _19;
    be_u32 req;             // +20
    pad<2208> _24;
    ReadyList lists[5];     // +2232
    gp<RefVec> refs;        // +2272
    pad<176> _2276;
    be_u32 state_idx;       // +2452
};
static_assert(offsetof(ReadySection, lists) == 2232 && offsetof(ReadySection, refs) == 2272);
static_assert(offsetof(ReadySection, state_idx) == 2452);

// The count stored as the u8 +4, then the status word with that count in
// its top byte and bit 23 = ready (two stores, as the original).
void ListStatus(ReadyList& L, u32 n, bool ready) {
    L.status.b[0] = static_cast<u8>(n);
    u32 w = (L.status.w & 0x00FFFFFFu) | (n << 24);
    w = ready ? (w | 0x00800000u) : (w & ~0x00800000u);
    L.status.w = w;
}
// The u16 count of the list's vector (0 without one).
u32 ListCount(const ReadyList& L) {
    const u32 h = L.h.ea;
    if (h == 0) return 0;
    const u32 a = gm<be_u32>(h);
    return a != 0 ? gm<PtrVec<ReadyElem>>(a).count : 0u;
}
// The word of o's state table selected by +2452 (its top nibble / bit 26).
be_u32& StateWord(const ReadySection* o) { return gm<be_u32>(o->states + (o->state_idx << 2) + 0x20074u); }
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
void Section_Ready_0032549C(ReadySection* o, u8 on, s32 k) {
    u32 flag = on;
    for (u32 j = 0; j < 5; ++j) {
        ReadyList& L = o->lists[j];
        bool notReady = false;
        u32 h = L.h.ea;
        if (h != 0 && gm<be_u32>(h) != 0u && gm<PtrVec<ReadyElem>>(gm<be_u32>(h)).count != 0) {
            for (u32 i = 0, off = 0;; ++i, off += 4) {
                const u32 a = gm<be_u32>(h);
                const u32 n = a != 0 ? gm<PtrVec<ReadyElem>>(a).count : 0u;
                if (i >= n) break;
                ReadyElem& e = gm<ReadyElem>(gm<be_u32>(gm<PtrVec<ReadyElem>>(a).data.ea + off));
                u32 w = e.flags;
                e.w24 = 0u;
                const bool rdy = (w & 0x20000000u) && (w & 0x40000000u);
                w &= ~0x3C000000u;
                if (rdy) w |= 0x24000000u;
                e.flags = w;
                if (!rdy) notReady = true;
                h = L.h.ea;
            }
        }
        u32 n = 0;
        if (h != 0 && !notReady) {
            const u32 a = gm<be_u32>(h);
            if (a != 0) n = gm<PtrVec<ReadyElem>>(a).count;
        }
        ListStatus(L, n, !notReady);
    }
    bool m5 = false;
    if (flag != 0) {
        const u32 m = StateWord(o) >> 28;
        u32 w = o->req;
        if (m > 1) { w &= ~4u; o->req = w; }
        if (m > 2) { w &= ~2u; o->req = w; }
        if (m > 3) { w &= ~1u; o->req = w; }
        m5 = m == 5;
    }
    u32 mode = 0;
    if (k == -1) {
        if (m5) {
            o->b17 = o->b16;
            mode = 5;
            flag = 1;
        } else {
            const u32 w = o->req;
            o->b17 = 0;
            mode = 4;
            if (w & 1) mode = 3;
            if (w & 2) mode = 2;
            if (w & 4) mode = 1;
        }
    } else {
        o->b17 = k == 5 ? o->b16 : 0;
        if (k == 5 || !(o->req & 0x10) || o->state != 5) {
            o->b17 = k == 5 ? o->b16 : 0;  // stored again on this path
            mode = static_cast<u32>(k);
            flag = (k == 5 || flag != 0) ? 1 : 0;
        }
    }
    const u32 arr = o->refs.ea;
    if (arr != 0 && gm<RefVec>(arr).count != 0) {
        const u32 show = call<u32>(Obj_CanShow2E2104, global<be_u32>(kShowCtx), o);
        for (u32 i = 0, off = 0;; ++i, off += 20) {
            const u32 a = o->refs.ea;
            const u32 n = a != 0 ? gm<RefVec>(a).count : 0u;
            if (i >= n) break;
            Ref& e = gm<Ref>(gm<RefVec>(a).data.ea + off);
            const u32 w = e.u.w;
            e.u.w = w & 0x3FFFFFFFu;
            if (((w & 0xFF) != 0 || mode == 5) && e.u.b[2] != 0) {
                const u32 top = e.u.b[1] != 0 ? 0x80000000u : 0x40000000u;
                e.u.w = (e.u.w & 0x3FFFFFFFu) | top;
            }
            e.u.b[3] = 0;
            e.pending = 0;
            e.target = 0u;
            if (show != 0) Ref_Resolve_00321324(&e, 0);
        }
    }
    if (mode != 0) vcall(o, 104, mode, flag);
    if (o->state != 5) return;
    if (flag != 0) {
        for (u32 j : {0u, 2u, 1u}) ListStatus(o->lists[j], ListCount(o->lists[j]), true);  // +2232, +2248, +2240
        be_u32& p = StateWord(o);
        p = p & ~0x04000000u;
    } else {
        ListStatus(o->lists[3], ListCount(o->lists[3]), true);
        ListStatus(o->lists[4], ListCount(o->lists[4]), true);
    }
}

}  // namespace sly4::obj

NATIVE(0032549C, sly4::obj::Section_Ready_0032549C)
