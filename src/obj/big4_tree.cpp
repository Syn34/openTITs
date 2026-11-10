// Game code, large-function sweep part 4: vfn[12] of the classes with
// vtables 0x00B21C68 / 0x00B21C80 / 0x00B21CC0 (004B8EE4): a visit of
// every child, with the map iterator inlined.
#include "big4.h"

using namespace sly4;
using namespace sly4::big4;

namespace {

// The inlined map iterator (an in-order walk of a binary tree with an
// explicit stack of ancestors). it: +12 -> the root pointer, +16 the
// stack (u32[], s16-indexed), u16 +96 its depth, +100 the current node.
// A node: +8 the value, +12 the left child, +16 the right child.
inline u32 Slot(u32 it, u32 depth) { return it + 16 + 4 * static_cast<u32>(static_cast<s32>(static_cast<s16>(depth))); }

// Begin: the leftmost node, its ancestors on the stack (each push stores
// the depth before the slot).
void IterBegin(u32 it) {
    store<u16>(it + 96, 0);
    u32 n = load<u32>(load<u32>(it + 12));
    if (n != 0) {
        while (load<u32>(n + 12) != 0) {
            const u32 d = load<u16>(it + 96);
            store<u16>(it + 96, static_cast<u16>(d + 1));
            store<u32>(Slot(it, d), n);
            n = load<u32>(n + 12);
        }
    }
    store<u32>(it + 100, n);
}

// Next (the current node is non-null): the leftmost node of the right
// subtree, or the first ancestor reached from its left child (0 at the
// end).
void IterNext(u32 it) {
    const u32 cur = load<u32>(it + 100);
    u32 r = load<u32>(cur + 16);
    if (r != 0) {
        const u32 d = load<u16>(it + 96);
        store<u16>(it + 96, static_cast<u16>(d + 1));
        store<u32>(Slot(it, d), cur);
        while (load<u32>(r + 12) != 0) {  // (these pushes store the slot first)
            const u32 d2 = load<u16>(it + 96);
            store<u32>(Slot(it, d2), r);
            r = load<u32>(r + 12);
            store<u16>(it + 96, static_cast<u16>(d2 + 1));
        }
        store<u32>(it + 100, r);
        return;
    }
    u32 d = load<u16>(it + 96);
    u32 child = cur;
    for (;;) {
        if ((d & 0xFFFFu) == 0) {
            store<u32>(it + 100, 0);
            return;
        }
        d -= 1;
        store<u16>(it + 96, static_cast<u16>(d));
        const u32 p = load<u32>(Slot(it, d));
        store<u32>(it + 100, p);
        if (child == load<u32>(p + 12)) return;
        child = p;
    }
}

// Walks the map at o->+field (re-read after each visit), calling
// visit(value) for each node; returns the iterator as last read.
template <typename F>
u32 ForEachValue(u32 o, u32 field, F visit) {
    u32 it = load<u32>(o + field);
    if (it != 0) IterBegin(it);
    for (;;) {
        if (it == 0 || load<u32>(it + 100) == 0) return it;
        visit(load<u32>(load<u32>(it + 100) + 8));
        it = load<u32>(o + field);
        if (it == 0 || load<u32>(it + 100) == 0) continue;
        IterNext(it);
    }
}

}  // namespace

// void VtB21C80_VisitChildren(T* o): vfn[20] of each element of the
// array at +28 (+0 the elements, u16 +8 the count, re-read each time),
// vfn[12] of each value of the map at +24, then for each value of the
// maps at +16 and +20 (a list: +8 the elements, u16 +16 the count),
// vfn[20] of its elements. Each map's iterator is begun in place (the
// iterator object is the map field's target). r3 is left as the last
// map pointer read (the +20 one).
DECOMP(004B8EE4, VtB21C80_VisitChildren) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 i = 0;; ++i) {
        const u32 list = load<u32>(o + 28);
        const u32 n = list != 0 ? load<u16>(list + 8) : 0u;
        if (!(i < n)) break;
        const u32 e = load<u32>(load<u32>(list) + 4 * i);
        VCall(ctx, e, 20, {e});
    }
    ForEachValue(o, 24, [&](u32 v) { VCall(ctx, v, 12, {v}); });
    auto visit_list = [&](u32 v) {
        for (u32 j = 0; j < load<u16>(v + 16); ++j) {
            const u32 e = load<u32>(load<u32>(v + 8) + 4 * j);
            VCall(ctx, e, 20, {e});
        }
    };
    ForEachValue(o, 16, visit_list);
    ctx.gpr[3] = ForEachValue(o, 20, visit_list);
}
