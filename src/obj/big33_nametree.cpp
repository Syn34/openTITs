// Game code, large-function sweep part 33: a value registered under a dotted
// name ("A.B.C") in the name tree rooted at 0x010C2BC0, one 124-byte node per
// path element, each node holding a sorted map of its children.
#include "big33.h"

namespace sly4::obj {

using NameAvl = AvlNode<be_u32, be_u32>;  // key: the element hash, value: a NameNode

// A sorted map of a name tree: +0 the root node, +8 the AVL "rebalance"
// flag, +12 the slot holding the root (its own +0), +16 the path stack of
// the last walk (s16 depth +96), +100 the cursor.
struct NameMap {
    gp<NameAvl> root;       // +0
    be_u32 w4;              // +4
    u8 balanced;            // +8
    pad<3> _9;
    gp<gp<NameAvl>> slot;   // +12
    be_u32 path[20];        // +16
    be_s16 depth;           // +96
    pad<2> _98;
    gp<NameAvl> cursor;     // +100
};
static_assert(offsetof(NameMap, slot) == 12 && offsetof(NameMap, depth) == 96 && sizeof(NameMap) == 104);
struct NameNode {
    pad<4> _0;
    be_u32 value;            // +4
    u8 flag;                 // +8
    pad<3> _9;
    NameMap children;        // +12
    gp<NameNode> parent;     // +116
    pad<4> _120;
};
static_assert(offsetof(NameNode, parent) == 116 && sizeof(NameNode) == 124);
// The element pointers (a PtrVector; +6 is set to 4 after construction).
struct NamePathVec {
    gp<gp<char>> data;  // +0
    be_u16 cap;         // +4
    be_u16 h6;          // +6
    be_u16 count;       // +8
    be_u16 mark;        // +10
};

namespace {
constexpr u32 kPathFmt = 0x00AEFD0C;       // "%s"
constexpr u32 kNameTree = 0x010C2BC0;      // NameMap: the root (see Lists_Init_004F32A4)
constexpr u32 kTreeLastValue = 0x011DE4A8;  // u32: the value of the last map lookup
constexpr u32 Avl_TreeInsert = 0x007992E0;  // (map, u32 key, value)

// The inlined sorted-map find over m for key h: the found node's value,
// else 0. The root map and a child map store the "empty" result in
// opposite orders.
u32 MapFind(NameMap& m, u32 h, bool root) {
    be_u32& last = global<be_u32>(kTreeLastValue);
    NameAvl* node = gm<gp<NameAvl>>(m.slot.ea);  // the slot unchecked
    if (node == nullptr) {
        if (root) {
            last = 0u;
            m.cursor = nullptr;
        } else {
            m.cursor = nullptr;
            last = 0u;
        }
        return 0;
    }
    m.depth = 0;
    for (;;) {
        const u32 key = node->key;
        if (key == h) {
            m.cursor = node;
            last = node->value.get();
            if (root) return node->value;
            const NameAvl* cur = m.cursor;
            return cur != nullptr ? cur->value.get() : 0u;
        }
        NameAvl* next = key > h ? node->left : node->right;
        if (next == nullptr) {
            m.cursor = node;  // cursor left at the last node visited
            return 0;
        }
        const s16 d = m.depth;
        m.depth = static_cast<s16>(d + 1);
        gm<be_u32>(guest(&m) + 16 + static_cast<u32>(static_cast<s32>(d)) * 4) = guest(node);  // path[d], unbounded
        node = next;
    }
}
}  // namespace

// void f(const char* name, u32 value, u8 flag) (called by World_Construct and
// f_004EC730). name is formatted ("%s", 99 bytes) into a stack buffer and
// split at '.' (at most 100 characters, each '.' overwritten by 0) into a
// PtrVector of element pointers. Every element but the last is looked up
// (String_HashNoCase key) from the root map, then in the child map of each
// node found; any miss clears the vector and returns. Then a 124-byte node
// (Mem_Alloc(124, 0, 16)) gets an empty child map, value and flag, is
// inserted under the last element's hash into its parent's map (the root
// map when there are no parents) and gets its parent. A failed allocation
// still writes value / flag / parent at address 0 (quirk). The vector is
// cleared and passed to Nop.
void NameTree_AddPath_004EC20C(const char* name, u32 value, u8 flag) {
    GuestFrame frame(128);  // sp+112: PtrVector (12 bytes), sp+136: char[100]
    NamePathVec* vec = frame.as<NamePathVec>(0);
    char* buf = frame.as<char>(24);
    const u32 bufea = guest(buf);
    call(b33fn::String_FormatN, buf, 99u, kPathFmt, name);
    call(b33fn::PtrVector_Construct, vec);
    vec->h6 = 4;
    buf[99] = 0;

    // c3 / c4: the element count as cached in r3 / r4 between pushes.
    u32 c3 = vec->count, c4 = c3;
    // Push buf + start: grow (twice the capacity) when the cached count is
    // the capacity, store, count + 1, high-water mark.
    auto push = [&](u32 seg, bool maxFirst) {
        const u32 cap = vec->cap;
        if ((c3 & 0xFFFF) == cap) {
            call(fn::PtrVector_Reserve, vec, (cap << 1) & 0x1FFFEu);
            c4 = vec->count;
        }
        const u32 idx = c4 & 0xFFFF;
        const u32 data = vec->data.ea;
        const u32 n = vec->count + 1u;
        u32 mx = maxFirst ? vec->mark : 0u;
        gm<be_u32>(data + idx * 4) = seg;
        if (!maxFirst) mx = vec->mark;
        vec->count = static_cast<u16>(n);
        vec->mark = static_cast<u16>(static_cast<s32>(mx) > static_cast<s32>(n & 0xFFFF) ? mx : n);
        c3 = n;
        c4 = n;
    };
    u32 k = 0, start = 0;
    for (; k < 100; ++k) {
        const char c = buf[k];
        if (c == 0) break;
        if (c != '.') continue;
        buf[k] = 0;
        // The capacity is read before the 0 is written; neither aliases.
        push(bufea + start, true);
        start = k + 1;
    }
    if (k != start) push(bufea + start, false);

    u32 parent = 0;
    for (u32 i = 0, off = 0; i < static_cast<u32>(vec->count - 1); ++i, off += 4) {
        const u32 h = call<u32>(b33fn::String_HashNoCase, gm<be_u32>(vec->data.ea + off).get());
        parent = parent == 0 ? MapFind(global<NameMap>(kNameTree), h, true) : MapFind(gm<NameNode>(parent).children, h, false);
        if (parent == 0) {
            call(b33fn::PtrVector_Clear, vec);
            call(b33fn::Nop, vec);
            return;
        }
    }

    NameNode* obj = call<NameNode*>(fn::Mem_Alloc, 124u, 0u, 16u);
    u32 node = 0;
    if (obj != nullptr) {
        NameMap& m = obj->children;
        m.root = nullptr;
        m.slot = &m.root;
        m.w4 = 0u;
        m.balanced = 1;
        // The child map's begin(): the leftmost node from the root just
        // cleared, so always 0 (the walk is kept as compiled).
        NameAvl* first = m.root;
        m.depth = 0;
        NameAvl* cur = nullptr;
        if (first != nullptr) {
            NameAvl* p = first;
            for (;;) {
                cur = p;
                if (!p->left) break;
                const s16 d = m.depth;
                m.depth = static_cast<s16>(d + 1);
                gm<be_u32>(guest(&m) + 16 + static_cast<u32>(static_cast<s32>(d)) * 4) = guest(p);
                p = p->left;
                if (p == nullptr) break;
            }
        }
        m.cursor = cur;
        node = guest(obj);
        m.balanced = 1;
    }
    NameNode& nn = gm<NameNode>(node);
    const u32 cnt = vec->count;
    nn.value = value;
    nn.flag = flag;
    const u32 last = gm<be_u32>(vec->data.ea + (cnt - 1) * 4);
    const u32 h = call<u32>(b33fn::String_HashNoCase, last);
    call(Avl_TreeInsert, parent == 0 ? kNameTree : guest(&gm<NameNode>(parent).children), h, node);
    nn.parent.ea = parent;
    call(b33fn::PtrVector_Clear, vec);
    call(b33fn::Nop, vec);
}

}  // namespace sly4::obj

NATIVE(004EC20C, sly4::obj::NameTree_AddPath_004EC20C)
