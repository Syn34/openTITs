// Game code, large-function sweep part 23: the engine's AVL template
// removes (AVL<float, ApeAnimScriptEvent*> and the f32-keyed multimap
// variant), each with the rebalance inlined six times.
#include "big23.h"

namespace sly4::obj {

namespace {
// AVL node: +0 s16 balance (right - left height), +2 s16 height, +4 f32
// key, +8 value, +12 left, +16 right.
using FNode = AvlNode<be_f32, be_u32>;
// The multimap's tree node (24 bytes) holds the first value at +8 and
// links further 24-byte entries {+8 value, +20 next} at +20.
struct MNode {
    be_s16 balance;    // +0
    be_s16 height;     // +2
    be_f32 key;        // +4
    be_u32 value;      // +8
    gp<MNode> left;    // +12
    gp<MNode> right;   // +16
    gp<MNode> next;    // +20
};
static_assert(sizeof(MNode) == 24);
constexpr u32 kEntrySize = 24;

constexpr u32 Avl_RemoveF32 = 0x0078FD34u;       // this file's Avl_RemoveF32_0078FD34
constexpr u32 AvlMulti_RemoveF32 = 0x0074A210u;  // this file's AvlMulti_RemoveF32_74A210
constexpr u32 AvlF_RotateRight = 0x0078F180u;  // (Tree*, Node** link): the left child up
constexpr u32 AvlF_RotateLeft = 0x0078F2C8u;   // (Tree*, Node** link): the right child up
constexpr u32 AvlF_TakeMax = 0x0078FA2Cu;      // (Tree*, Node** slot, Node** left, u8* changed) (name?)
constexpr u32 Avl_RotateR = 0x00749D40u;       // (?, Node** pp): the +12 child up
constexpr u32 Avl_RotateL = 0x00749E88u;       // (?, Node** pp): the +16 child up
constexpr u32 AvlMulti_TakeMax = 0x00749FD0u;  // (AVL*, Node** slot, Node** left, u8* changed) (name?)

// The AVL<float, ApeAnimScriptEvent*> node free list, made on first use.
constexpr u32 kEvNodeList = 0x00F8F0FCu;
constexpr u32 kEvNodeListVtbl = 0x00B22530u;
constexpr u32 kStrEvNodeList = 0x00AEEC21u;  // "static ApeTFreeList<...> &AVL<KeyType, ItemType>::GetAllocator() [with KeyType = float, ItemType = ApeAnimScriptEvent *]"

// The rotations a template instance calls.
struct Rot {
    u32 right, left;
};

// The inlined rebalance of the subtree at slot: (optionally) the height
// update, then the right-heavy (> 1) and left-heavy (< -1) rotations. The
// multimap's copies skip the null check of the reloaded *slot before the
// left-heavy test (check_left = false; a null node then reads address 0).
template <typename N>
void Rebalance(const Rot& rot, AvlTree* avl, gp<N>* slot, bool heights = true, bool check_left = true) {
    if (heights) {
        if (N* n = *slot) AvlFix(n);
    }
    N* n = *slot;
    if (n == nullptr) return;
    if (n->balance > 1) {
        if (N* r = n->right) {
            if (r->balance < 0) call(rot.right, avl, &n->right);
            call(rot.left, avl, slot);
        }
    }
    const u32 ne = slot->ea;
    if ((check_left && ne == 0) || gm<be_s16>(ne) >= -1) return;
    N& m = gm<N>(ne);
    N* l = m.left;
    if (l == nullptr) return;
    if (l->balance > 0) call(rot.left, avl, &m.left);
    call(rot.right, avl, slot);
}

// The removal of a node with two children: AvlF_TakeMax moves the left
// subtree's largest node into the slot; the changed levels are rebalanced
// when the tree's flag is set; *changed = 1.
template <typename N>
void TakeMax(u32 take, const Rot& rot, AvlTree* avl, gp<N>* slot, N* n, u8* changed, bool check_left) {
    call(take, avl, slot, &n->left, changed);
    if (*changed != 0 && avl->balanced != 0) {
        if (N* l = gm<N>(slot->ea).left) AvlFix(l);  // *slot unchecked
        Rebalance(rot, avl, &gm<N>(slot->ea).left, false, check_left);
        Rebalance(rot, avl, slot, true, check_left);
    }
    *changed = 1;
}
}  // namespace

// void Avl_RemoveF32_0078FD34(AVL* avl, Node** slot, -, u8* changed; f32
// key) (no caller reads r3 or f1): recursive AVL delete of key below slot
// (the float twin of Map_Erase78D670). *changed = 0 when slot is empty; a
// node key > key recurses left, < key right, anything else (equal or NaN)
// deletes: the changed flag is 1 and the node goes back to the free list
// (made on first use; a failed Mem_Alloc leaves 0 and FreeList_Free is
// called on it). One child: it takes the node's place; two: AvlF_TakeMax
// moves the left subtree's largest node there. Each changed level is
// rebalanced when the AVL's +8 flag is set (the level of a removed
// one-child node always). The third argument is passed through to the
// recursive calls untouched.
void Avl_RemoveF32_0078FD34(AvlTree* avl, gp<FNode>* slot, u32 r5, u8* changed, f64 key) {
    constexpr Rot rot{AvlF_RotateRight, AvlF_RotateLeft};
    FNode* n = *slot;
    if (n == nullptr) {
        *changed = 0;
        return;
    }
    const f64 k = lf(n->key);
    if (k > key || k < key) {
        // The recursion goes through the bridge (as the original's bl to
        // itself), so the harness checks one level at a time.
        call(Avl_RemoveF32, avl, k > key ? &n->left : &n->right, r5, changed, key);
        if (*changed != 0 && avl->balanced != 0) Rebalance(rot, avl, slot);
        return;
    }
    if (n->right.ea == 0u) {
        *changed = 1;
        *slot = n->left.get();
        call(fn::FreeList_Free, AvlPool(kEvNodeList, kEvNodeListVtbl, kStrEvNodeList), n);
        Rebalance(rot, avl, slot);
        return;
    }
    if (n->left.ea == 0u) {
        *slot = n->right.get();
        *changed = 1;
        call(fn::FreeList_Free, AvlPool(kEvNodeList, kEvNodeListVtbl, kStrEvNodeList), n);
        Rebalance(rot, avl, slot);
        return;
    }
    TakeMax(AvlF_TakeMax, rot, avl, slot, n, changed, true);
}

// void AvlMulti_RemoveF32_74A210(AVL* avl, Node** slot, u32 value, -, u8*
// changed; f32 key) (no caller reads r3 or f1): the f32-keyed multimap
// remove. Empty slot: *changed = 0. Node key > key / < key recurses left /
// right (the fourth argument passed through). On the key: value 0 or the
// node's own value removes the head, another value is looked up in the
// chain (absent: nothing, *changed untouched) and unlinked and freed. A
// head with followers swaps in the first follower's value and link (the
// follower gets the old value and is freed); a lone head deletes the tree
// node as Avl_RemoveF32_0078FD34 does (SmallFree instead of the free
// list). The inlined rebalances read the reloaded *slot's balance without
// a null check after the right-heavy step.
void AvlMulti_RemoveF32_74A210(AvlTree* avl, gp<MNode>* slot, u32 value, u32 r6, u8* changed, f64 key) {
    constexpr Rot rot{Avl_RotateR, Avl_RotateL};
    MNode* n = *slot;
    if (n == nullptr) {
        *changed = 0;
        return;
    }
    const f64 k = lf(n->key);
    if (k > key || k < key) {
        call(AvlMulti_RemoveF32, avl, k > key ? &n->left : &n->right, value, r6, changed, key);  // through the bridge
        if (*changed != 0 && avl->balanced != 0) Rebalance(rot, avl, slot, true, false);
        return;
    }
    MNode* e = n;
    MNode* prev = nullptr;
    if (value != 0) {
        while (e->value != value) {
            prev = e;
            e = e->next;
            if (e == nullptr) return;
        }
    }
    if (prev != nullptr) {
        prev->next = e->next.get();
        call(fn::SmallFree, e, kEntrySize);
        return;
    }
    if (MNode* next = n->next) {
        MNode* link = next->next;
        const u32 v = n->value;
        n->next = link;
        n->value = next->value.get();
        next->value = v;
        call(fn::SmallFree, next, kEntrySize);
        return;
    }
    if (n->right.ea == 0u) {
        *changed = 1;
        *slot = n->left.get();
        call(fn::SmallFree, n, kEntrySize);
        Rebalance(rot, avl, slot, true, false);
        return;
    }
    if (n->left.ea == 0u) {
        *slot = n->right.get();
        *changed = 1;
        call(fn::SmallFree, n, kEntrySize);
        Rebalance(rot, avl, slot, true, false);
        return;
    }
    TakeMax(AvlMulti_TakeMax, rot, avl, slot, n, changed, false);
}

}  // namespace sly4::obj

NATIVE(0078FD34, sly4::obj::Avl_RemoveF32_0078FD34)
NATIVE(0074A210, sly4::obj::AvlMulti_RemoveF32_74A210)
