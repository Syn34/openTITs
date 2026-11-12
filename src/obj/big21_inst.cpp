// Game code, large-function sweep part 21: the effect instance base
// constructor (the XML children filed into the instance's four maps).
#include "big21.h"

namespace sly4::obj {

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
constexpr u32 FindAttrByName = 0x004F9628u;
constexpr u32 FirstChildAny = 0x004F9140u;
constexpr u32 NextSiblingAny = 0x004F8D74u;
constexpr u32 String_HashNoCase = 0x0056A710u;
constexpr u32 Hash_ToString = 0x0056A6E0u;
constexpr u32 PtrVector_Construct = 0x005516E0u;
constexpr u32 Tree_SeekA = 0x004B6894u;         // (XmlNode*) -> p or 0 (reads r3 only)
constexpr u32 Tree_SeekB = 0x004B6C88u;         // (XmlNode*) -> p or 0 (reads r3 only)
constexpr u32 SeekC = 0x004B707Cu;              // (XmlNode*) -> p or 0 (reads r3 only)
constexpr u32 AnimInst_Init = 0x004B69B8u;
constexpr u32 AvlVar_Insert = 0x00786468u;
constexpr u32 Avl_TreeInsert_E9D8 = 0x0078E9D8u;
constexpr u32 Avl_TreeInsert_EFEC = 0x0078EFECu;
constexpr u32 AvlF_Insert = 0x0078F898u;        // (map, -, p; f32 key) (reads r3, r5, f1)

// A BST node: +4 the key, +8 the value, +12 left, +16 right.
template <typename K>
struct BstNode {
    pad<4> _0;
    K key;                 // +4
    be_u32 value;          // +8
    gp<BstNode> left;      // +12
    gp<BstNode> right;     // +16
};
// A 104-byte map / registry: +8 u8, +12 the root slot, +16 the path,
// +96 the s16 depth, +100 the cursor.
struct BstMap {
    be_u32 w0;             // +0
    be_u32 w4;             // +4
    u8 b8;                 // +8
    pad<3> _9;
    be_u32 root;           // +12: the slot holding the root
    be_u32 path[20];       // +16
    be_s16 depth;          // +96
    pad<2> _98;
    be_u32 cursor;         // +100
};
static_assert(sizeof(BstMap) == 104 && offsetof(BstMap, depth) == 96);
struct Instance {
    be_u32 vt;             // +0
    be_u16 h4;             // +4
    pad<2> _6;
    be_u32 name;           // +8
    be_u32 mapA;           // +12
    be_u32 mapC;           // +16
    be_u32 mapF;           // +20
    be_u32 mapAnim;        // +24
    be_u32 vecB;           // +28
};
static_assert(sizeof(Instance) == 32);
// A filed object: +8 its key (or, for the f32 kind, the start of a
// PtrVec of refcounted entries), +28 its f32 key, +32 its u32 key.
struct InstPart {
    pad<8> _0;
    PtrVec<u8> list;       // +8 (+8 the u32 key word)
    pad<8> _20;
    be_f32 fkey;           // +28
    be_u32 ukey;           // +32
};
static_assert(offsetof(InstPart, fkey) == 28 && offsetof(InstPart, ukey) == 32);

// The inlined BST select (equality, left +12 when the node's key is
// greater (unsigned / f32), else right +16; each node passed pushed at
// t+16 + 4 * the s16 depth t+96). Found: cursor t+100 = node and *g =
// node->8; an empty tree zeroes both; a miss leaves the cursor on the last
// node. Returns the node or 0.
template <typename K, typename V>
u32 Select(u32 t, u32 g, V key) {
    BstMap& m = gm<BstMap>(t);
    BstNode<K>* node = &gm<BstNode<K>>(gm<be_u32>(m.root));
    if (guest(node) == 0) {
        m.cursor = 0u;
        global<be_u32>(g) = 0u;
        return 0;
    }
    m.depth = 0;
    for (;;) {
        const auto k = [&] {
            if constexpr (std::is_same_v<K, be_f32>) return lf(node->key);
            else return node->key.get();
        }();
        if (k == key) {
            m.cursor = guest(node);
            global<be_u32>(g) = node->value.get();
            return guest(node);
        }
        BstNode<K>* next = k > key ? node->left : node->right;
        if (next == nullptr) {
            m.cursor = guest(node);
            return 0;
        }
        const s32 n = m.depth;
        m.depth = static_cast<s16>(n + 1);
        gm<be_u32>(t + 16 + 4u * static_cast<u32>(n)) = guest(node);
        node = next;
    }
}
u32 SelectU(u32 t, u32 g, u32 key) { return Select<be_u32>(t, g, key); }
u32 SelectF(u32 t, u32 g, f64 key) { return Select<be_f32>(t, g, key); }

// A new empty 104-byte map (SmallAlloc) into *slot.
void NewMap(be_u32& slot) {
    const u32 t = call<u32>(fn::SmallAlloc, 104u);
    slot = t;
    if (t != 0) {
        BstMap& m = gm<BstMap>(t);
        m.cursor = 0u;
        m.b8 = 1;
        m.depth = 0;
        m.root = t;
        m.w4 = 0u;
        m.w0 = 0u;
    }
}

// A 20-byte PtrVector (u16 +6 = 4) made in *slot when missing; returns *slot.
u32 VecAt(be_u32& slot) {
    u32 v = slot;
    if (v == 0) {
        v = call<u32>(fn::SmallAlloc, 20u);
        slot = v;
        if (v != 0) {
            call(PtrVector_Construct, v);
            gm<be_u16>(v + 6) = 4;
            v = slot;
        }
    }
    return v;
}

// Push p onto the vector at v (data +0, u16 capacity +4, count +8,
// high-water +10), doubling the capacity when full.
void Push(PtrVec<u8>& v, u32 p) {
    u32 cnt = v.count;
    const u32 cap = v.cap;
    if (cnt == cap) {
        call(fn::PtrVector_Reserve, &v, (cap << 1) & 0xFFFFFFFEu);
        cnt = v.count;
    }
    gm<be_u32>(v.data.ea + ((cnt << 2) & 0xFFFFFFFCu)) = p;
    const u32 nc = v.count + 1;
    const u32 hw = v.mark;
    v.count = static_cast<u16>(nc);
    v.mark = static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(nc & 0xFFFFu) ? hw : nc);
}

// The XML node's hash (+20 when flag 0x80 of +24 is set, else 0).
u32 NodeHash(u32 c) { return (gm<u8>(c + 24) & 0x80u) != 0 ? gm<be_u32>(c + 20).get() : 0u; }
void Release(u32 p) { vcall(&gm<u8>(p), 8); }
}  // namespace

// void Instance_Base4B71BC(Instance* x, a, Effect* b, XmlCursor* xml) (no
// caller reads r3 or f1): u16 x->4 = 1, vtable 0x00B21C68, x->8..28 = 0. With b and xml: x->8 = the "name"
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
// Maps are 104-byte nodes made on first use.
void Instance_Base4B71BC(Instance* x, u32 a, u32 b, u32 xml) {
    x->h4 = 1;
    x->vt = kInstVt;
    x->mapA = 0u;
    x->mapC = 0u;
    x->mapF = 0u;
    x->mapAnim = 0u;
    x->vecB = 0u;
    x->name = 0u;
    if (b == 0 || xml == 0) return;
    const u32 attr = call<u32>(FindAttrByName, xml, kNameAttr);
    u32 name = x->name;
    if (attr != 0) {
        name = call<u32>(String_HashNoCase, attr, name);
        x->name = name;
    }
    if (name == 0) x->name = kDefaultName;
    u32 count = 0;
    for (u32 c = call<u32>(FirstChildAny, xml); c != 0; c = call<u32>(NextSiblingAny, c))
        if (SelectU(kKindsB, kCurB, NodeHash(c)) != 0) ++count;
    u32 c = call<u32>(FirstChildAny, xml);
    if (count != 0) call(fn::PtrVector_Reserve, VecAt(x->vecB), count);
    for (; c != 0; c = call<u32>(NextSiblingAny, c)) {
        if (SelectU(kKindsA, kCurA, NodeHash(c)) != 0) {
            const u32 p = call<u32>(Tree_SeekA, c);
            if (p == 0) continue;
            const u32 key = gm<be_u32>(p + 8);
            const u32 t = x->mapA;
            if (t != 0 && SelectU(t, kCurA, key) != 0) {
                call(Hash_ToString, NodeHash(c));
                Release(p);
                continue;
            }
            if (x->mapA == 0u) NewMap(x->mapA);
            call(AvlVar_Insert, x->mapA.get(), key, p);
            continue;
        }
        if (SelectU(kKindsB, kCurB, NodeHash(c)) != 0) {
            const u32 p = call<u32>(Tree_SeekB, c);
            if (p == 0) continue;
            Push(gm<PtrVec<u8>>(VecAt(x->vecB)), p);
            continue;
        }
        if (SelectU(kKindsC, kCurC, NodeHash(c)) != 0) {
            const u32 p = call<u32>(SeekC, c);
            if (p == 0) continue;
            InstPart& part = gm<InstPart>(p);
            if (vcall<u32>(&part, 16) == 0) {
                const u32 key = part.ukey;
                if (x->mapC == 0u) NewMap(x->mapC);
                call(Avl_TreeInsert_EFEC, x->mapC.get(), key, p);
                continue;
            }
            // f32-keyed map +20
            f64 fk = lf(part.fkey);
            if (const u32 t = x->mapF; t != 0) {
                if (SelectF(t, kCurF, fk) != 0) {
                    // a duplicate key: append p's list to the entry found again
                    u32 dst = 0;
                    if (const u32 t2 = x->mapF; t2 != 0 && SelectF(t2, kCurF, fk) != 0) {
                        const u32 cur = gm<BstMap>(t2).cursor;
                        dst = cur != 0 ? gm<be_u32>(cur + 8).get() : 0u;
                    }
                    for (u32 j = 0, off = 0; j < part.list.count; ++j, off += 4) {
                        const u32 e = gm<be_u32>(part.list.data.ea + off);
                        if (e == 0) continue;
                        gm<be_u16>(e + 4) = static_cast<u16>(gm<be_u16>(e + 4) + 1);
                        Push(gm<InstPart>(dst).list, e);
                    }
                    Release(p);
                    continue;
                }
                fk = lf(part.fkey);
            }
            if (x->mapF == 0u) NewMap(x->mapF);
            call(AvlF_Insert, x->mapF.get(), static_cast<f32>(fk), p);
            continue;
        }
        const u32 h = NodeHash(c);
        if (h != kAnimKind) {
            call(Hash_ToString, h);
            continue;
        }
        const u32 p = call<u32>(fn::SmallAlloc, 52u);
        if (p == 0) continue;
        call(AnimInst_Init, p, a, b, c);
        const u32 key = gm<be_u32>(p + 8);
        if (const u32 t = x->mapAnim; t != 0 && SelectU(t, kCurAnim, key) != 0) {
            call(Hash_ToString, key);
            Release(p);
            continue;
        }
        const u32 key2 = gm<be_u32>(p + 8);
        if (x->mapAnim == 0u) NewMap(x->mapAnim);
        call(Avl_TreeInsert_E9D8, x->mapAnim.get(), key2, p);
    }
}

}  // namespace sly4::obj

NATIVE(004B71BC, sly4::obj::Instance_Base4B71BC)
