// Game code, large-function sweep part 33: a value registered under a dotted
// name ("A.B.C") in the name tree rooted at 0x010C2BC0, one 124-byte node per
// path element, each node holding a sorted map of its children.
#include "../sly4.h"
#include "big33.h"

using namespace sly4;
using namespace sly4::big33;

namespace {
constexpr u32 kPathFmt = 0x00AEFD0C;       // "%s"
constexpr u32 kNameTree = 0x010C2BC0;      // root sorted map (see Lists_Init_004F32A4)
constexpr u32 kTreeLastValue = 0x011DE4A8;  // u32: the value of the last map lookup
constexpr u32 Avl_TreeInsert = 0x007992E0;  // (map, u32 key, value)

// The inlined sorted-map find over m (+12 the root slot, +16 the path stack of
// s16 depth +96, +100 the cursor) for key h: the found node's value (+8), else
// 0. The root map and a child map store the "empty" result in opposite orders.
u32 MapFind(u32 m, u32 h, bool root) {
    u32 node = load<u32>(load<u32>(m + 12));
    if (node == 0) {
        if (root) {
            store<u32>(kTreeLastValue, 0);
            store<u32>(m + 100, 0);
        } else {
            store<u32>(m + 100, 0);
            store<u32>(kTreeLastValue, 0);
        }
        return 0;
    }
    store<u16>(m + 96, 0);
    for (;;) {
        const u32 key = load<u32>(node + 4);
        if (key == h) {
            store<u32>(m + 100, node);
            store<u32>(kTreeLastValue, load<u32>(node + 8));
            if (root) return load<u32>(node + 8);
            const u32 cur = load<u32>(m + 100);
            return cur != 0 ? load<u32>(cur + 8) : 0;
        }
        const u32 next = key > h ? load<u32>(node + 12) : load<u32>(node + 16);
        if (next == 0) {
            store<u32>(m + 100, node);  // cursor left at the last node visited
            return 0;
        }
        const s16 d = static_cast<s16>(load<u16>(m + 96));
        store<u16>(m + 96, static_cast<u16>(d + 1));
        store<u32>(m + 16 + static_cast<u32>(static_cast<s32>(d)) * 4, node);
        node = next;
    }
}
}  // namespace

// void f(const char* name, u32 value, u8 flag) (called by World_Construct and
// f_004EC730). name is formatted ("%s", 99 bytes) into a stack buffer and
// split at '.' (at most 100 characters, each '.' overwritten by 0) into a
// PtrVector of element pointers. Every element but the last is looked up
// (String_HashNoCase key) from the root map, then in the map at +12 of each
// value found; any miss clears the vector and returns. Then a 124-byte node
// (Mem_Alloc(124, 0, 16)) gets an empty child map at +12 (+24 = +12, +16 = 0,
// +20 = 1, +108 = 0, cursor +112), value at +4, flag at +8, is inserted under
// the last element's hash into its parent's map (the root map when there are
// no parents) and gets the parent at +116. A failed allocation still writes
// +4 / +8 / +116 of address 0 (quirk). The vector is cleared and passed to
// Nop, whose r3 (the vector) is what returns.
DECOMP(004EC20C, NameTree_AddPath_004EC20C) {
    const u32 name = arg<u32>(ctx, 0), value = arg<u32>(ctx, 1);
    const u8 flag = arg<u8>(ctx, 2);
    Stack frame(ctx, 128);  // sp+112: PtrVector (12 bytes), sp+136: char[100]
    const u32 vec = frame.addr(), buf = vec + 24;
    call(ctx, b33fn::String_FormatN, {buf, 99u, kPathFmt, name});
    call(ctx, b33fn::PtrVector_Construct, {vec});
    store<u16>(vec + 6, 4);
    store<u8>(buf + 99, 0);

    // c3 / c4: the element count as cached in r3 / r4 between pushes.
    u32 c3 = load<u16>(vec + 8), c4 = c3;
    // Push buf + start: grow (twice the capacity) when the cached count is
    // the capacity, store, count + 1, high-water mark at +10.
    auto push = [&](u32 seg, bool maxFirst) {
        const u32 cap = load<u16>(vec + 4);
        if ((c3 & 0xFFFF) == cap) {
            call(ctx, b33fn::PtrVector_Reserve, {vec, (cap << 1) & 0x1FFFEu});
            c4 = load<u16>(vec + 8);
        }
        const u32 idx = c4 & 0xFFFF;
        const u32 data = load<u32>(vec);
        const u32 n = load<u16>(vec + 8) + 1u;
        u32 mx = maxFirst ? load<u16>(vec + 10) : 0;
        store<u32>(data + idx * 4, seg);
        if (!maxFirst) mx = load<u16>(vec + 10);
        store<u16>(vec + 8, static_cast<u16>(n));
        store<u16>(vec + 10, static_cast<u16>(static_cast<s32>(mx) > static_cast<s32>(n & 0xFFFF) ? mx : n));
        c3 = n;
        c4 = n;
    };
    u32 k = 0, start = 0;
    for (; k < 100; ++k) {
        const u8 c = load<u8>(buf + k);
        if (c == 0) break;
        if (c != '.') continue;
        store<u8>(buf + k, 0);
        // The capacity is read before the 0 is written; neither aliases.
        push(buf + start, true);
        start = k + 1;
    }
    if (k != start) push(buf + start, false);

    u32 parent = 0;
    for (u32 i = 0, off = 0; i < static_cast<u32>(load<u16>(vec + 8) - 1); ++i, off += 4) {
        const u32 h = static_cast<u32>(call(ctx, fn::String_HashNoCase, {load<u32>(load<u32>(vec) + off)}));
        parent = parent == 0 ? MapFind(kNameTree, h, true) : MapFind(parent + 12, h, false);
        if (parent == 0) {
            call(ctx, b33fn::PtrVector_Clear, {vec});
            call(ctx, b33fn::Nop, {vec});
            return;
        }
    }

    const u32 obj = static_cast<u32>(call(ctx, fn::Mem_Alloc, {124u, 0u, 16u}));
    u32 node = 0;
    if (obj != 0) {
        store<u32>(obj + 12, 0);
        store<u32>(obj + 24, obj + 12);
        store<u32>(obj + 16, 0);
        store<u8>(obj + 20, 1);
        // The child map's begin(): the leftmost node from the root just
        // cleared, so always 0 (the walk is kept as compiled).
        const u32 first = load<u32>(obj + 12);
        store<u16>(obj + 108, 0);
        u32 cur = 0;
        if (first != 0) {
            u32 p = first;
            for (;;) {
                cur = p;
                if (load<u32>(p + 12) == 0) break;
                const s16 d = static_cast<s16>(load<u16>(obj + 108));
                store<u16>(obj + 108, static_cast<u16>(d + 1));
                store<u32>(obj + 28 + static_cast<u32>(static_cast<s32>(d)) * 4, p);
                p = load<u32>(p + 12);
                if (p == 0) break;
            }
        }
        store<u32>(obj + 112, cur);
        node = obj;
        store<u8>(obj + 20, 1);
    }
    const u32 cnt = load<u16>(vec + 8);
    store<u32>(node + 4, value);
    store<u8>(node + 8, flag);
    const u32 last = load<u32>(load<u32>(vec) + (cnt - 1) * 4);
    const u32 h = static_cast<u32>(call(ctx, fn::String_HashNoCase, {last}));
    call(ctx, Avl_TreeInsert, {parent == 0 ? kNameTree : parent + 12, h, node});
    store<u32>(node + 116, parent);
    call(ctx, b33fn::PtrVector_Clear, {vec});
    call(ctx, b33fn::Nop, {vec});
}
