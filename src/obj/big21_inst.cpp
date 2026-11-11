// Game code, large-function sweep part 21: the effect instance base
// constructor (the XML children filed into the instance's four maps).
#include "big21.h"

using namespace sly4;
using namespace sly4::big21;

namespace {
constexpr u32 kInstVt = 0x00B21C68u;    // the base vtable
constexpr u32 kNameAttr = 0x00AEEFD8u;  // "name"
constexpr u32 kDefaultName = 0x19D1382Au;
constexpr u32 kAnimKind = 0x04B8CC71u;  // the child kind that makes an AnimInst
// The kind registries (BST root *(t+12), path t+16, s16 depth t+96,
// cursor t+100) and the globals their selects update.
constexpr u32 kKindsA = 0x010C27B8u;    // -> Tree_SeekA (map +12)
constexpr u32 kKindsB = 0x010C2820u;    // -> Tree_SeekB (array +28)
constexpr u32 kKindsC = 0x010C28F0u;    // -> 0x004B707C (map +16 / +20)
constexpr u32 kCurA = 0x011DE490u;
constexpr u32 kCurB = 0x011DE4A0u;
constexpr u32 kCurC = 0x011DE2ACu;
constexpr u32 kCurAnim = 0x011DE2A8u;   // the +24 map's select
constexpr u32 kCurF = 0x011DE494u;      // the f32-keyed +20 map's select
constexpr u32 kCurBase = 0x011E0000u;   // lis r5 the callees see

namespace ifn {
constexpr u32 FindAttrByName = 0x004F9628u;
constexpr u32 FirstChildAny = 0x004F9140u;
constexpr u32 NextSiblingAny = 0x004F8D74u;
constexpr u32 String_HashNoCase = 0x0056A710u;
constexpr u32 Hash_ToString = 0x0056A6E0u;
constexpr u32 SmallAlloc = 0x0055238Cu;
constexpr u32 PtrVector_Construct = 0x005516E0u;
constexpr u32 PtrVector_Reserve = 0x00551718u;
constexpr u32 Tree_SeekA = 0x004B6894u;
constexpr u32 Tree_SeekB = 0x004B6C88u;
constexpr u32 SeekC = 0x004B707Cu;
constexpr u32 AnimInst_Init = 0x004B69B8u;
constexpr u32 AvlVar_Insert = 0x00786468u;
constexpr u32 Avl_TreeInsert_E9D8 = 0x0078E9D8u;
constexpr u32 Avl_TreeInsert_EFEC = 0x0078EFECu;
constexpr u32 AvlF_Insert = 0x0078F898u;
}  // namespace ifn

// The inlined BST select (equality by cmpw, left +12 when the node's key is
// unsigned-greater, else right +16; each node passed pushed at t+16 + 4 *
// the s16 depth t+96). Found: cursor t+100 = node and *g = node->8, in the
// order the call site stores them; an empty tree zeroes both (also in site
// order); a miss leaves the cursor on the last node. Returns the node or 0.
u32 Select(u32 t, u32 g, u32 key, bool foundGFirst, bool emptyGFirst) {
    u32 node = load<u32>(load<u32>(t + 12));
    if (node == 0) {
        if (emptyGFirst) store<u32>(g, 0);
        store<u32>(t + 100, 0);
        if (!emptyGFirst) store<u32>(g, 0);
        return 0;
    }
    store<u16>(t + 96, 0);
    for (;;) {
        const u32 k = load<u32>(node + 4);
        if (k == key) {
            const u32 v = load<u32>(node + 8);
            if (foundGFirst) store<u32>(g, v);
            store<u32>(t + 100, node);
            if (!foundGFirst) store<u32>(g, v);
            return node;
        }
        const u32 next = load<u32>(node + (k > key ? 12 : 16));
        if (next == 0) {
            store<u32>(t + 100, node);
            return 0;
        }
        const s32 n = static_cast<s16>(load<u16>(t + 96));
        store<u16>(t + 96, static_cast<u16>(n + 1));
        store<u32>(t + 16 + 4u * static_cast<u32>(n), node);
        node = next;
    }
}

// The same on the f32-keyed map (fcmpu: equal found, greater left, else
// right); cursor first, then *g. `last` gets the last node examined (the
// r4 the original leaves), f1 the last key loaded.
u32 SelectF(PPUContext& ctx, u32 t, u32 g, f64 key, u32& last) {
    u32 node = load<u32>(load<u32>(t + 12));
    if (node == 0) {
        store<u32>(t + 100, 0);
        store<u32>(g, 0);
        last = kCurBase;
        return 0;
    }
    store<u16>(t + 96, 0);
    for (;;) {
        const f64 k = Lf(node + 4);
        ctx.fpr[1] = k;
        if (k == key) {
            store<u32>(t + 100, node);
            store<u32>(g, load<u32>(node + 8));
            last = node;
            return node;
        }
        const u32 next = load<u32>(node + (k > key ? 12 : 16));
        if (next == 0) {
            store<u32>(t + 100, node);
            last = node;
            return 0;
        }
        const s32 n = static_cast<s16>(load<u16>(t + 96));
        store<u16>(t + 96, static_cast<u16>(n + 1));
        store<u32>(t + 16 + 4u * static_cast<u32>(n), node);
        node = next;
    }
}

// A new empty 104-byte map (SmallAlloc) into *slot; returns *slot.
u32 NewMap(PPUContext& ctx, u32 slot, u32& r4) {
    const u32 t = static_cast<u32>(call(ctx, ifn::SmallAlloc, {104}));
    store<u32>(slot, t);
    r4 = static_cast<u32>(ctx.gpr[4]);
    if (t != 0) {
        store<u32>(t + 100, 0);
        store<u8>(t + 8, 1);
        store<u16>(t + 96, 0);
        store<u32>(t + 12, t);
        store<u32>(t + 4, 0);
        store<u32>(t + 0, 0);
        r4 = 1;
    }
    return load<u32>(slot);
}

// A 20-byte PtrVector (u16 +6 = 4) made in *slot when missing; returns *slot.
u32 VecAt(PPUContext& ctx, u32 slot) {
    u32 v = load<u32>(slot);
    if (v == 0) {
        v = static_cast<u32>(call(ctx, ifn::SmallAlloc, {20}));
        store<u32>(slot, v);
        if (v != 0) {
            call(ctx, ifn::PtrVector_Construct, {v});
            store<u16>(v + 6, 4);
            v = load<u32>(slot);
        }
    }
    return v;
}

// Push p onto the vector at v (data +0, u16 capacity +4, count +8,
// high-water +10), doubling the capacity when full.
void Push(PPUContext& ctx, u32 v, u32 p) {
    u32 cnt = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (cnt == cap) {
        call(ctx, ifn::PtrVector_Reserve, {v, (cap << 1) & 0xFFFFFFFEu});
        cnt = load<u16>(v + 8);
    }
    store<u32>(load<u32>(v) + ((cnt << 2) & 0xFFFFFFFCu), p);
    const u32 nc = load<u16>(v + 8) + 1;
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(nc));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(nc & 0xFFFFu) ? hw : nc));
}

// The XML node's hash (+20 when flag 0x80 of +24 is set, else 0).
u32 NodeHash(u32 c) { return (load<u8>(c + 24) & 0x80u) != 0 ? load<u32>(c + 20) : 0u; }
}  // namespace

// Instance_Base4B71BC(Instance* x, a, Effect* b, XmlCursor* xml): u16 x->4 =
// 1, vtable 0x00B21C68, x->8..28 = 0. With b and xml: x->8 = the "name"
// attribute's String_HashNoCase (0x19D1382A when missing or 0). The
// children whose hash is in registry B are counted to reserve the +28
// array. Then per child, by the first registry holding its hash: A ->
// Tree_SeekA(child) filed in the u32 map +12 (AvlVar_Insert; a duplicate key
// is released, vfn[8]); B -> Tree_SeekB(child) pushed onto the +28 array; C
// -> 0x004B707C(child): when its vfn[16] says so it is filed by its f32 key
// +28 in the map +20 (AvlF_Insert; a duplicate's u16-counted list +8/+16 is
// appended, refcounts +4 raised, to the existing entry's list +8 and the
// newcomer released), else by its key +32 in the map +16; the kind
// 0x04B8CC71 makes a 52-byte AnimInst (AnimInst_Init(p, a, b, child)) filed
// by its +8 in the map +24 (Avl_TreeInsert_0078E9D8; duplicates released).
// Maps are 104-byte nodes made on first use. r3 / f1 as the original leaves
// them.
DECOMP(004B71BC, Instance_Base4B71BC) {
    const u32 x = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), xml = arg<u32>(ctx, 3);
    store<u16>(x + 4, 1);
    store<u32>(x + 0, kInstVt);
    for (u32 k = 12; k <= 28; k += 4) store<u32>(x + k, 0);
    store<u32>(x + 8, 0);
    if (b == 0 || xml == 0) return;
    const u32 attr = static_cast<u32>(call(ctx, ifn::FindAttrByName, {xml, kNameAttr}));
    u32 name = load<u32>(x + 8);
    if (attr != 0) {
        name = static_cast<u32>(call(ctx, ifn::String_HashNoCase, {attr, name}));
        store<u32>(x + 8, name);
    }
    if (name == 0) store<u32>(x + 8, kDefaultName);
    u32 count = 0;
    for (u32 c = static_cast<u32>(call(ctx, ifn::FirstChildAny, {xml})); c != 0;
         c = static_cast<u32>(call(ctx, ifn::NextSiblingAny, {c})))
        if (Select(kKindsB, kCurB, NodeHash(c), true, true) != 0) ++count;
    u32 c = static_cast<u32>(call(ctx, ifn::FirstChildAny, {xml}));
    if (count != 0) call(ctx, ifn::PtrVector_Reserve, {VecAt(ctx, x + 28), count});
    for (; c != 0; c = static_cast<u32>(call(ctx, ifn::NextSiblingAny, {c}))) {
        if (const u32 n = Select(kKindsA, kCurA, NodeHash(c), false, true); n != 0) {
            const u32 p = static_cast<u32>(call(ctx, ifn::Tree_SeekA, {c, kKindsA, kCurBase, load<u32>(n + 8)}));
            if (p == 0) continue;
            const u32 key = load<u32>(p + 8);
            u32 t = load<u32>(x + 12);
            if (t != 0 && Select(t, kCurA, key, false, false) != 0) {
                call(ctx, ifn::Hash_ToString, {NodeHash(c)});
                VCall(ctx, p, 8, {p});
                continue;
            }
            u32 r4 = 0;
            if (load<u32>(x + 12) == 0) NewMap(ctx, x + 12, r4);
            call(ctx, ifn::AvlVar_Insert, {load<u32>(x + 12), key, p});
            continue;
        }
        if (const u32 n = Select(kKindsB, kCurB, NodeHash(c), false, true); n != 0) {
            const u32 p = static_cast<u32>(call(ctx, ifn::Tree_SeekB, {c, kKindsB, kCurBase, load<u32>(n + 8)}));
            if (p == 0) continue;
            Push(ctx, VecAt(ctx, x + 28), p);
            continue;
        }
        if (const u32 n = Select(kKindsC, kCurC, NodeHash(c), false, true); n != 0) {
            const u32 p = static_cast<u32>(call(ctx, ifn::SeekC, {c, kKindsC, kCurBase, load<u32>(n + 8)}));
            if (p == 0) continue;
            if (static_cast<u32>(VCall(ctx, p, 16, {p})) == 0) {
                const u32 key = load<u32>(p + 32);
                u32 r4 = 0;
                if (load<u32>(x + 16) == 0) NewMap(ctx, x + 16, r4);
                call(ctx, ifn::Avl_TreeInsert_EFEC, {load<u32>(x + 16), key, p});
                continue;
            }
            // f32-keyed map +20
            u32 r4 = static_cast<u32>(ctx.gpr[4]);
            f64 fk = Lf(p + 28);
            if (const u32 t = load<u32>(x + 20); t != 0) {
                if (SelectF(ctx, t, kCurF, fk, r4) != 0) {
                    // a duplicate key: append p's list to the entry found again
                    u32 dst = 0;
                    if (const u32 t2 = load<u32>(x + 20); t2 != 0) {
                        u32 dummy;
                        if (SelectF(ctx, t2, kCurF, fk, dummy) != 0) {
                            const u32 cur = load<u32>(t2 + 100);
                            dst = cur != 0 ? load<u32>(cur + 8) : 0u;
                        }
                    }
                    for (u32 j = 0, off = 0; j < load<u16>(p + 16); ++j, off += 4) {
                        const u32 e = load<u32>(load<u32>(p + 8) + off);
                        if (e == 0) continue;
                        store<u16>(e + 4, static_cast<u16>(load<u16>(e + 4) + 1));
                        u32 cnt = load<u16>(dst + 16);
                        const u32 cap = load<u16>(dst + 12);
                        if (cnt == cap) {
                            call(ctx, ifn::PtrVector_Reserve, {dst + 8, (cap << 1) & 0xFFFFFFFEu});
                            cnt = load<u16>(dst + 16);
                        }
                        store<u32>(load<u32>(dst + 8) + ((cnt << 2) & 0xFFFFFFFCu), e);
                        const u32 nc = load<u16>(dst + 16) + 1;
                        const u32 hw = load<u16>(dst + 18);
                        store<u16>(dst + 16, static_cast<u16>(nc));
                        store<u16>(dst + 18, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(nc & 0xFFFFu) ? hw : nc));
                    }
                    VCall(ctx, p, 8, {p});
                    continue;
                }
                fk = Lf(p + 28);
            }
            if (load<u32>(x + 20) == 0) NewMap(ctx, x + 20, r4);
            callf(ctx, ifn::AvlF_Insert, {load<u32>(x + 20), r4, p}, {fk});
            continue;
        }
        const u32 h = NodeHash(c);
        if (h != kAnimKind) {
            call(ctx, ifn::Hash_ToString, {h});
            continue;
        }
        const u32 p = static_cast<u32>(call(ctx, ifn::SmallAlloc, {52}));
        if (p == 0) continue;
        call(ctx, ifn::AnimInst_Init, {p, a, b, c});
        const u32 key = load<u32>(p + 8);
        if (const u32 t = load<u32>(x + 24); t != 0 && Select(t, kCurAnim, key, false, false) != 0) {
            call(ctx, ifn::Hash_ToString, {key});
            VCall(ctx, p, 8, {p});
            continue;
        }
        const u32 key2 = load<u32>(p + 8);
        u32 r4 = 0;
        if (load<u32>(x + 24) == 0) NewMap(ctx, x + 24, r4);
        call(ctx, ifn::Avl_TreeInsert_E9D8, {load<u32>(x + 24), key2, p});
    }
}
