// World_HandleEvent callees, part 6: the effect definition lookup and the
// effect destructor (both built on the engine's inlined binary-tree map).
#include "hevent6.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent6;

namespace sly4 {
namespace hevent6a {
namespace afn {
constexpr u32 Effect_Destruct = 0x004C8E80;
constexpr u32 SmallFree = 0x00552420;          // (p, size)
constexpr u32 Nop = 0x0054EC18;
constexpr u32 Tree_Find = 0x0078E260;          // (Map*, u32 key, u32* out value, Node* root) -> found (name?)
constexpr u32 Map_Erase = 0x0078D670;          // (Map*, Map*, u32 key, u8* out) (name?)
}  // namespace afn

// The effect set registry: a map at +12 of 0x010C2888, keyed by set name; the
// values are sets with their own map at +16, keyed by effect name hash.
constexpr u32 kSets = 0x010C2888 + 12;
constexpr u32 kLastSet = 0x011DE4A4;         // u32 set found by the last lookup (name?)

// Effect layout used by the destructor.
constexpr u32 kEffectRef0 = 0, kEffectRef1 = 4;  // refcounted objects (vfunc +8 release)
constexpr u32 kEffectPool = 8, kEffectCount = 12; // +8 root node (what the +20 map points to), u32 count
constexpr u32 kEffectMap = 20;               // map of components
constexpr u32 kEffectChildren = 112;         // PtrVector of child effects
constexpr u32 kEffectChildCount = 120;       // u16
constexpr u32 kEffectSize = 140;

}  // namespace hevent6a
}  // namespace sly4

using namespace sly4::hevent6a;

// EffectDef* EffectDef_Find(u32 hash, u32 set_name, u8 search_all) (name?):
// looks hash up in the effect set named set_name (registry 0x010C2888 +12;
// the set at 0x011DE4A4, 0 when the registry is empty). The set lookup is
// done twice, as the inlined find + get of the original. Not found there (or
// no such set) and search_all: every set in key order is searched, first
// match wins. Each set search leaves its node / def as set_lookup describes.
DECOMP(004BDEDC, EffectDef_Find) {
    const u32 hash = arg<u32>(ctx, 0);
    const u32 set_name = arg<u32>(ctx, 1);
    const u32 search_all = arg<u32>(ctx, 2) & 0xFF;
    const u32 root = load<u32>(load<u32>(kSets));
    if (root == 0) {
        store<u32>(kSets + kMapNode, 0);
        store<u32>(kLastSet, 0);
    } else if (const u32 n = map_walk(kSets, root, set_name); n != 0) {
        store<u32>(kSets + kMapNode, n);
        store<u32>(kLastSet, load<u32>(n + kNodeValue));
        // Second (identical) search; on a miss the set is 0 and the original
        // reads its map from address 16.
        u32 set = 0;
        const u32 root2 = load<u32>(load<u32>(kSets));
        if (root2 == 0) {
            store<u32>(kLastSet, 0);
            store<u32>(kSets + kMapNode, 0);
        } else if (const u32 n2 = map_walk(kSets, root2, set_name); n2 != 0) {
            store<u32>(kSets + kMapNode, n2);
            store<u32>(kLastSet, load<u32>(n2 + kNodeValue));
            set = load<u32>(n2 + kNodeValue);
        }
        const u32 def = set_lookup(set, hash);
        if (def != 0) {
            ret(ctx, def);
            return;
        }
    }
    if (search_all == 0) {
        ret(ctx, 0);
        return;
    }
    u32 node = map_first(kSets);
    while (node != 0) {
        const u32 def = set_lookup(load<u32>(node + kNodeValue), hash);
        if (def != 0) {
            ret(ctx, def);
            return;
        }
        node = load<u32>(kSets + kMapNode);
        if (node == 0) break;
        node = map_next(kSets, node);
    }
    ret(ctx, 0);
}

// void Effect_Destruct(Effect* e) (name?): destroys and frees (SmallFree, 140
// bytes) every non-null child in +112 (count +120 re-read after each one;
// +120 = 0 after), calls vfunc +48 then (in a second pass) vfunc +8 on every
// component in the +20 map, erases the map's entries one by one (smallest key
// first) through +8 with +12 decremented each time, releases +0 and +4
// (vfunc +8), clears the +112 vector, and runs the map clear once more (the
// map member's destructor). Returns 0 in r3 like the original.
DECOMP(004C8E80, Effect_Destruct) {
    const u32 e = arg<u32>(ctx, 0);
    u32 count = load<u16>(e + kEffectChildCount);
    if (count != 0) {
        for (u32 i = 0; i < count; ++i) {
            const u32 child = load<u32>(load<u32>(e + kEffectChildren) + 4 * i);
            if (child == 0) continue;
            call(ctx, afn::Effect_Destruct, {child});
            call(ctx, afn::SmallFree, {child, kEffectSize});
            count = load<u16>(e + kEffectChildCount);
        }
        store<u16>(e + kEffectChildCount, 0);
    }
    const u32 m = e + kEffectMap;
    for (const u32 slot : {48u, 8u}) {
        for (u32 node = map_first(m); node != 0;) {
            const u32 c = load<u32>(node + kNodeValue);
            vcall(ctx, c, slot, {c});
            node = load<u32>(m + kMapNode);
            if (node == 0) break;
            node = map_next(m, node);
        }
    }
    Stack frame(ctx, 16);  // r1+112..127: out values / flags of the erase loops
    const u32 pool = e + kEffectPool;
    auto clear = [&](u32 out, u32 flag) {
        for (;;) {
            const u32 node = map_first(m);
            if (node == 0) return;
            const u32 key = load<u32>(node + kNodeKey);
            store<u8>(flag, 0);
            // A failed find retries forever, as in the original.
            if (call(ctx, afn::Tree_Find, {pool, key, out, load<u32>(pool)}) == 0) continue;
            call(ctx, afn::Map_Erase, {pool, pool, key, flag});
            store<u32>(e + kEffectCount, load<u32>(e + kEffectCount) - 1);
        }
    };
    clear(frame.addr() + 4, frame.addr() + 9);
    for (const u32 off : {kEffectRef0, kEffectRef1}) {
        const u32 o = load<u32>(e + off);
        if (o != 0) vcall(ctx, o, 8, {o});
    }
    call(ctx, hfn::PtrVector_Clear, {e + kEffectChildren});
    call(ctx, afn::Nop, {e + kEffectChildren});
    clear(frame.addr(), frame.addr() + 8);
    ret(ctx, 0);
}
