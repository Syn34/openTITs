// World_HandleEvent callees, part 23b: the effect instance's timed event
// pass (named by hevent22_c.cpp Instance_SetTime4BC7C8).
#include "hevent22.h"

#include <bit>

using namespace sly4;
using namespace sly4::hevent;

namespace {
// Instance +20: AVL<float, ApeAnimScriptEvent*> iterator object (hevent6 map
// layout from +12; node +4 holds an f32 key), +44 the event list fired last.
// Event list: +8 Event*[] (each called through vtable +12 (event, effect,
// instance)), +16 u16 count. The tree's last-found value and key globals.
constexpr u32 kInstEvents = 20, kInstLastFired = 44;
constexpr u32 kTreeMap = 12;
constexpr u32 kListData = 8, kListCount = 16;
constexpr u32 kEventFire = 12;
constexpr u32 kEventLastValue = 0x011DE498, kEventLastKey = 0x011DE49C;

f64 key_of(u32 n) { return static_cast<f64>(std::bit_cast<f32>(load<u32>(n + sly4::hevent6::kNodeKey))); }
}  // namespace

// bool Instance_Apply4BC36C(Instance* inst, Effect* e, f32 from, f32 to)
// (name?): positions the +20 event tree's iterator on the first key after
// from (an exact key match stays on it; the walk and the successor step
// set 0x011DE49C / 0x011DE498 to the key / value found), then, while the
// current key is <= to, fires every event of the current node's list
// (vtable +12 (event, e, inst); skipped when the list is the one at +44,
// which then becomes this list) and steps to the in-order successor.
// Returns 1; f1 is left as the last key loaded, as in the original.
DECOMP(004BC36C, Instance_Apply4BC36C) {
    using namespace sly4::hevent6;
    const u32 inst = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const f64 from = farg(ctx, 0), to = farg(ctx, 1);
    const u32 t = load<u32>(inst + kInstEvents);
    if (t != 0) {
        const u32 m = t + kTreeMap;
        const u32 root = load<u32>(load<u32>(m));
        bool exact = false;
        u32 cur = 0;
        if (root == 0) {
            store<u32>(m + kMapNode, 0);
            store<u32>(kEventLastValue, 0);
            cur = load<u32>(m + kMapNode);
        } else {
            store<u16>(m + kMapDepth, 0);
            for (u32 n = root;;) {
                const f64 k = key_of(n);
                if (k == from) {
                    store<u32>(m + kMapNode, n);
                    store<u32>(kEventLastValue, load<u32>(n + kNodeValue));
                    exact = true;
                    break;
                }
                const u32 next = k > from ? load<u32>(n + kNodeLeft) : load<u32>(n + kNodeRight);
                if (next == 0) {
                    store<u32>(m + kMapNode, n);
                    cur = n;
                    break;
                }
                map_push(m, n);
                n = next;
            }
        }
        if (!exact) {
            bool after = false;
            if (cur != 0) {
                store<u32>(kEventLastKey, load<u32>(cur + kNodeKey));
                store<u32>(kEventLastValue, load<u32>(cur + kNodeValue));
                after = key_of(cur) > from;
            }
            if (!after) {
                const u32 c = load<u32>(m + kMapNode);
                if (c == 0) {
                    store<u32>(kEventLastValue, 0);
                } else {
                    const u32 n = sly4::hevent22::map_next_lf(m, c);
                    if (n != 0) {
                        ctx.fpr[1] = key_of(n);
                        store<u32>(kEventLastKey, load<u32>(n + kNodeKey));
                        store<u32>(kEventLastValue, load<u32>(n + kNodeValue));
                    } else {
                        store<u32>(kEventLastValue, 0);
                    }
                }
            }
        }
    }
    for (;;) {
        const u32 tr = load<u32>(inst + kInstEvents);
        if (tr == 0) break;
        const u32 cur = load<u32>(tr + kTreeMap + kMapNode);
        if (cur == 0) break;
        const f64 k = key_of(cur);
        ctx.fpr[1] = k;
        if (!(k < to || k == to)) break;
        const u32 list = load<u32>(cur + kNodeValue);
        if (list != load<u32>(inst + kInstLastFired)) {
            for (u32 i = 0; i < load<u16>(list + kListCount); ++i) {
                const u32 ev = load<u32>(load<u32>(list + kListData) + 4 * i);
                vcall(ctx, ev, kEventFire, {ev, e, inst});
            }
        }
        store<u32>(inst + kInstLastFired, list);
        const u32 t2 = load<u32>(inst + kInstEvents);
        if (t2 == 0) continue;
        const u32 c = load<u32>(t2 + kTreeMap + kMapNode);
        if (c == 0) continue;
        sly4::hevent22::map_next_lf(t2 + kTreeMap, c);
    }
    ret(ctx, 1u);
}
