// Game code, medium sweep part 2: the SlotList inserts (template
// instances): a value pushed at the back or the front of a handle-based
// doubly linked list of 16-byte slots (layout in mid2.h), the new slot's
// handle written to *out (0 when the list is full and not growable).
#include "mid2.h"

using namespace sly4;
using namespace sly4::mid2;

namespace {

// void PushBack(Handle* out, SlotList* l, const u32* value): the slot
// taken from the free list's head and linked after the tail (the position
// handle is the tail's, taken before any growth); a full list doubles its
// capacity through grow when +32 is set. key_first: the value is read
// before the free head is written back (the 0079B55C instance's order).
// r3 = the new slot's +12 address (the capacity when full).
template <u32 kGrow, bool kKeyFirst>
void TSlotListPushBack(PPUContext& ctx) {
    const u32 out = arg<u32>(ctx, 0), l = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2);
    u32 count = load<u32>(l + 16), cap = load<u32>(l + 24);
    const u32 pos = count != 0 ? SlotHandle(l, load<u32>(l + 12)) : 0;
    if (count == cap) {
        if (!SlotListMakeRoom(ctx, l, cap, kGrow)) {
            store<u32>(out, 0);
            ctx.gpr[3] = cap;
            return;
        }
        count = load<u32>(l + 16);
        cap = load<u32>(l + 24);
    }
    u32 f, h, fl;
    if (count == 0) {
        // Unchecked: the free head's successor's prev cleared unless the
        // capacity is 1 (not "unless it has none"), as the original.
        if (cap != 1) store<u32>(SlotAt(l, load<u32>(SlotAt(l, load<u32>(l)) + 4)), 0xFFFFFFFFu);
        f = load<u32>(l);
        const u32 e = SlotAt(l, f);
        store<u32>(l + 8, f);
        store<u32>(l + 12, f);
        store<u32>(l, load<u32>(e + 4));
        store<u32>(e + 4, 0xFFFFFFFFu);
        store<u32>(e, 0xFFFFFFFFu);
        store<u32>(e + 8, load<u32>(key));
        store<u8>(e + 12, static_cast<u8>(load<u8>(e + 12) & 0x7F));
        const u32 c = load<u32>(l + 16), fc = load<u32>(l + 20);
        store<u32>(l + 16, c + 1);
        store<u32>(l + 20, fc - 1);
        const u32 g = SlotListNextGen();
        f = load<u32>(l + 8);
        h = SlotStamp(l, f, g, &fl);
    } else {
        const u32 fc0 = load<u32>(l + 20);
        f = load<u32>(l);
        u32 nf;
        if (fc0 == 1) {
            store<u32>(l + 4, 0xFFFFFFFFu);
            nf = 0xFFFFFFFFu;
        } else {
            nf = load<u32>(SlotAt(l, f) + 4);
        }
        u32 v = 0;
        if (kKeyFirst) v = load<u32>(key);
        store<u32>(l, nf);
        if (!kKeyFirst) v = load<u32>(key);
        const u32 p = (pos >> 16) & 0xFFFFu;
        const u32 e = SlotAt(l, f);
        store<u32>(e + 8, v);
        store<u8>(e + 12, static_cast<u8>(load<u8>(e + 12) & 0x7F));
        const u32 s = load<u32>(l + 28);
        const u32 fo = (f << 4) & 0xFFFFFFF0u, po = p << 4;
        store<u32>(s + fo, p);  // prev = the old tail
        const u32 tail = load<u32>(l + 12), c = load<u32>(l + 16), fc = load<u32>(l + 20);
        const bool at_tail = p == tail;
        store<u32>(s + fo + 4, at_tail ? 0xFFFFFFFFu : load<u32>(s + po + 4));
        if (!at_tail) store<u32>(s + ((load<u32>(s + po + 4) << 4) & 0xFFFFFFF0u), f);
        store<u32>(s + po + 4, f);
        if (at_tail) store<u32>(l + 12, f);
        store<u32>(l + 16, c + 1);
        store<u32>(l + 20, fc - 1);
        h = SlotStamp(l, f, SlotListNextGen(), &fl);
    }
    store<u32>(out, h);
    ctx.gpr[3] = fl;
}

// void PushFront(Handle* out, SlotList* l, const u32* value): as PushBack,
// linked before the head (the position handle is the head's). r3 = the
// new slot's +12 address, or the generation when the list was empty (the
// capacity when full).
template <u32 kGrow>
void TSlotListPushFront(PPUContext& ctx) {
    const u32 out = arg<u32>(ctx, 0), l = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2);
    u32 count = load<u32>(l + 16);
    const u32 cap = load<u32>(l + 24);
    const u32 pos = count != 0 ? SlotHandle(l, load<u32>(l + 8)) : 0;
    if (count == cap) {
        if (!SlotListMakeRoom(ctx, l, cap, kGrow)) {
            store<u32>(out, 0);
            ctx.gpr[3] = cap;
            return;
        }
        count = load<u32>(l + 16);
    }
    u32 h, fl;
    if (count == 0) {
        const u32 f = load<u32>(l), s = load<u32>(l + 28);
        const u32 e = s + ((f << 4) & 0xFFFFFFF0u);
        u32 nx = load<u32>(e + 4);
        if (nx != 0xFFFFFFFFu) {
            store<u32>(s + ((nx << 4) & 0xFFFFFFF0u), 0xFFFFFFFFu);
            nx = load<u32>(e + 4);
        }
        store<u32>(l + 8, f);
        store<u32>(l + 12, f);
        store<u32>(l, nx);
        store<u32>(e + 4, 0xFFFFFFFFu);
        store<u32>(e, 0xFFFFFFFFu);
        store<u32>(e + 8, load<u32>(key));
        store<u8>(e + 12, static_cast<u8>(load<u8>(e + 12) & 0x7F));
        const u32 c = load<u32>(l + 16), fc = load<u32>(l + 20);
        store<u32>(l + 16, c + 1);
        store<u32>(l + 20, fc - 1);
        const u32 g = SlotListNextGen();
        h = SlotStamp(l, load<u32>(l + 8), g, &fl);
        store<u32>(out, h);
        ctx.gpr[3] = g;
        return;
    }
    const u32 fc0 = load<u32>(l + 20), f = load<u32>(l);
    u32 nf;
    if (fc0 == 1) {
        store<u32>(l + 4, 0xFFFFFFFFu);
        nf = 0xFFFFFFFFu;
    } else {
        nf = load<u32>(SlotAt(l, f) + 4);
    }
    store<u32>(l, nf);
    const u32 fo = (f << 4) & 0xFFFFFFF0u;
    const u32 e = load<u32>(l + 28) + fo;
    const u32 v = load<u32>(key);
    const u32 p = (pos >> 16) & 0xFFFFu, po = p << 4;
    store<u32>(e + 8, v);
    store<u8>(e + 12, static_cast<u8>(load<u8>(e + 12) & 0x7F));
    const u32 s = load<u32>(l + 28);
    store<u32>(s + fo + 4, p);  // next = the old head
    const u32 head = load<u32>(l + 8), c = load<u32>(l + 16), fc = load<u32>(l + 20);
    const bool at_head = p == head;
    store<u32>(s + fo, at_head ? 0xFFFFFFFFu : load<u32>(s + po));
    if (!at_head) store<u32>(s + ((load<u32>(s + po) << 4) & 0xFFFFFFF0u) + 4, f);
    store<u32>(s + po, f);
    if (at_head) store<u32>(l + 8, f);
    store<u32>(l + 16, c + 1);
    store<u32>(l + 20, fc - 1);
    h = SlotStamp(l, f, SlotListNextGen(), &fl);
    store<u32>(out, h);
    ctx.gpr[3] = fl;
}

}  // namespace

// void Set_Insert_0079B55C(Handle* out, SlotList* l, const u32* v): PushBack
// (the Obj_RegisterParam set at o + 48; grows with SlotList_Grow_0079B3B0).
DECOMP(0079B55C, Set_Insert_0079B55C) { TSlotListPushBack<0x0079B3B0, true>(ctx); }

// void Queue_Push79BD1C(Handle* out, SlotList* l, const s32* v) (grows with
// SlotList_Grow_0079BB70).
DECOMP(0079BD1C, Queue_Push79BD1C) { TSlotListPushBack<0x0079BB70, false>(ctx); }

// void SlotList_PushFront_00785190(Handle* out, SlotList* l, const u32* v)
// (grows with SlotList_Grow_00784FE4).
DECOMP(00785190, SlotList_PushFront_00785190) { TSlotListPushFront<0x00784FE4>(ctx); }

// void SlotList_PushFront_0079A6D4(Handle* out, SlotList* l, const u32* v)
// (grows with SlotList_Grow_0079A528).
DECOMP(0079A6D4, SlotList_PushFront_0079A6D4) { TSlotListPushFront<0x0079A528>(ctx); }
