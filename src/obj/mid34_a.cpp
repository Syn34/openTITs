// Game code, medium sweep part 34: AVL tree instances.
//
// Nodes: {+0 s16 balance, +2 s16 height, +4 key, +8 value, +12 left,
// +16 right}; the maps' 20-byte nodes come from a lazily made per-type
// free list (AvlPool), the float multimap's 24-byte nodes (+20 the next
// node with the same key) from SmallAlloc.
#include "mid34.h"

namespace sly4::obj {

using AvlNodeUF = AvlNode<be_u32, be_f32>;  // AVL<u32, float> (the "Scale" maps)
using AvlNodeFU = AvlNode<be_f32, be_u32>;  // AVL<float, u32>

// The float-keyed multimap's node.
struct AvlMultiNode {
    be_s16 balance;            // +0
    be_s16 height;             // +2
    be_f32 key;                // +4
    be_u32 value;              // +8
    gp<AvlMultiNode> left;     // +12
    gp<AvlMultiNode> right;    // +16
    gp<AvlMultiNode> next;     // +20: the next node with the same key
};
static_assert(sizeof(AvlMultiNode) == 24);

namespace {
// AVL<u32, float>
constexpr u32 kScalePool34 = 0x00F8F0F0u;
constexpr u32 kScalePoolVt34 = 0x00B20C90u;
constexpr u32 kScalePoolName34 = 0x00ADF00Au;
constexpr u32 ScaleRotR34 = 0x0077B570u;      // Avl_RotateR_0077B570(tree, Node** pp)
constexpr u32 ScaleRotL34 = 0x0077B6B8u;      // Avl_RotateL_0077B6B8(tree, Node** pp)
// AVL<float, u32>
constexpr u32 kFKeyPool34 = 0x00F8F0FCu;
constexpr u32 kFKeyPoolVt34 = 0x00B22530u;
constexpr u32 kFKeyPoolName34 = 0x00AEEC21u;
constexpr u32 FKeyRotR34 = 0x0078F180u;       // Avl_RotateR_0078F180
constexpr u32 FKeyRotL34 = 0x0078F2C8u;       // Avl_RotateL_0078F2C8
// the float-keyed multimap
constexpr u32 MultiRotR34 = 0x00749D40u;      // Avl_RotateR_00749D40
constexpr u32 MultiRotL34 = 0x00749E88u;      // Avl_RotateL_00749E88

// RemoveMax(tree, Node** target, Node** pp, u8* changed): the right-most
// node below *pp unlinked (*pp = its left child); its key / value copied
// into *target (raw words), the node given back to the free list,
// *changed = 1. With a right child: the recursion, then the rebalance
// when *changed.
template <typename N>
void RemoveMax34(AvlTree* t, gp<N>* target, gp<N>* pp, u8* changed, void (*self)(AvlTree*, gp<N>*, gp<N>*, u8*),
                 u32 pool_ea, u32 vt, u32 name, u32 rotl, u32 rotr) {
    N* n = *pp;
    if (n->right) {
        self(t, target, &n->right, changed);
        if (*changed == 0) return;
        AvlRebal34(t, pp, rotl, rotr, true);
        return;
    }
    (*target)->key = n->key;
    N* m = *pp;
    (*target)->value = m->value;
    N* node = *pp;
    *pp = node->left.get();
    FreeList* pool = AvlPool(pool_ea, vt, name);
    call(fn::FreeList_Free, pool, node);
    *changed = 1;
}
}  // namespace

// Avl_RemoveMax(tree, Node** target, Node** pp, u8* changed) of the
// AVL<u32, float> maps (callee of Avl_Remove_0077BB08).
void AvlScale_RemoveMax77B800(AvlTree* t, gp<AvlNodeUF>* target, gp<AvlNodeUF>* pp, u8* changed) {
    RemoveMax34(t, target, pp, changed, &AvlScale_RemoveMax77B800, kScalePool34, kScalePoolVt34, kScalePoolName34,
                ScaleRotL34, ScaleRotR34);
}

// The same for the AVL<float, u32> maps.
void AvlFKey_RemoveMax78FA2C(AvlTree* t, gp<AvlNodeFU>* target, gp<AvlNodeFU>* pp, u8* changed) {
    RemoveMax34(t, target, pp, changed, &AvlFKey_RemoveMax78FA2C, kFKeyPool34, kFKeyPoolVt34, kFKeyPoolName34,
                FKeyRotL34, FKeyRotR34);
}

// Insert(tree, u32 key, -, Node** pp, float value) of the AVL<u32, float>
// maps: unsigned key descent (an equal key changes nothing), a new leaf
// {0, 1, key, value} from the free list's slot 12 at the empty child (the
// null from a failed allocation stored as it is), then the rebalance.
void AvlScale_Insert77C760(AvlTree* t, u32 key, u32 a5, gp<AvlNodeUF>* pp, f32 v) {
    AvlNodeUF* n = *pp;
    const u32 nk = n->key;
    gp<AvlNodeUF>* child = key < nk ? &n->left : key > nk ? &n->right : nullptr;
    if (child != nullptr) {
        if (*child) {
            AvlScale_Insert77C760(t, key, a5, child, v);
        } else {
            FreeList* pool = AvlPool(kScalePool34, kScalePoolVt34, kScalePoolName34);
            AvlNodeUF* node = vcall<AvlNodeUF*>(pool, 12);
            AvlNodeUF* m = *pp;
            if (node != nullptr) {
                node->balance = 0;
                node->height = 1;
                node->key = key;
                node->value = v;
                node->left = nullptr;
                node->right = nullptr;
            }
            (key < nk ? m->left : m->right) = node;
        }
    }
    AvlRebal34(t, pp, ScaleRotL34, ScaleRotR34, true);
}

// AvlMulti_Insert(tree, -, value, Node** pp, float key) of the float-keyed
// multimap (callee of f_0017A3F0): descent by the f32 key (fcmpu: less
// left, greater right); a new 24-byte leaf {0, 1, key, value} at an empty
// child. An equal (or unordered) key: a new node appended to the end of
// the node's +20 chain; the node keeps the new value and the appended one
// takes the old (a failed SmallAlloc links null and writes the old value
// to address 8, as the original does). Then the rebalance.
void AvlMulti_Insert74AD48(AvlTree* t, u32, u32 val, gp<AvlMultiNode>* pp, f32 key) {
    AvlMultiNode* n = *pp;
    const f64 nk = lf(n->key);
    const f64 k = key;
    auto init = [&](AvlMultiNode* node) {
        node->balance = 0;
        node->height = 1;
        node->key = key;
        node->value = val;
        node->left = nullptr;
        node->right = nullptr;
        node->next = nullptr;
    };
    if (k < nk || k > nk) {
        const bool less = k < nk;
        AvlMultiNode* c = less ? n->left : n->right;
        if (c != nullptr) {
            AvlMulti_Insert74AD48(t, guest(c), val, less ? &n->left : &n->right, key);
        } else {
            AvlMultiNode* node = call<AvlMultiNode*>(fn::SmallAlloc, 24u);
            AvlMultiNode* m = *pp;
            if (node != nullptr) init(node);
            (less ? m->left : m->right) = node;
        }
    } else {
        AvlMultiNode* node = call<AvlMultiNode*>(fn::SmallAlloc, 24u);
        AvlMultiNode* x = *pp;
        if (node != nullptr) init(node);
        const u32 old = x->value;
        while (x->next) x = x->next;
        x->next = node;
        (*pp)->value = val;
        gm<AvlMultiNode>(guest(node)).value = old;  // quirk: address 8 when the allocation failed
    }
    AvlRebal34(t, pp, MultiRotL34, MultiRotR34, false);
}

// AvlMulti_TakeMax(tree, Node** target, Node** pp, u8* changed): the
// multimap's delete-max helper (callee of AvlMulti_RemoveF32_74A210): the
// right-most node below *pp unlinked, its f32 key and value copied into
// *target (raw words), the node SmallFree'd (24 bytes), *changed = 1;
// with a right child the recursion, then the rebalance when *changed.
void AvlMulti_TakeMax749FD0(AvlTree* t, gp<AvlMultiNode>* target, gp<AvlMultiNode>* pp, u8* changed) {
    AvlMultiNode* n = *pp;
    if (n->right) {
        AvlMulti_TakeMax749FD0(t, target, &n->right, changed);
        if (*changed == 0) return;
        AvlRebal34(t, pp, MultiRotL34, MultiRotR34, false);
        return;
    }
    (*target)->key = n->key;
    (*target)->value = (*pp)->value;
    AvlMultiNode* node = *pp;
    *pp = node->left.get();
    call(fn::SmallFree, node, 24u);
    *changed = 1;
}

}  // namespace sly4::obj

NATIVE(0077B800, sly4::obj::AvlScale_RemoveMax77B800)
NATIVE(0078FA2C, sly4::obj::AvlFKey_RemoveMax78FA2C)
NATIVE(0077C760, sly4::obj::AvlScale_Insert77C760)
NATIVE(0074AD48, sly4::obj::AvlMulti_Insert74AD48)
NATIVE(00749FD0, sly4::obj::AvlMulti_TakeMax749FD0)
