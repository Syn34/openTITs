// World_HandleEvent callees, part 7d: the AVL map erase (the engine's
// AVL<u32, ApeAnimScriptVar*>, 764 instructions with the rebalance inlined
// six times).
#include "hevent7.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;

namespace sly4 {
namespace hevent7d {
namespace dfn {
constexpr u32 Map_Erase78D670 = 0x0078D670;
constexpr u32 Avl_RotateRight785D58 = 0x00785D58;  // (AVL*, Node** slot): left child up (name?)
constexpr u32 Avl_RotateLeft785EA0 = 0x00785EA0;   // (AVL*, Node** slot): right child up (name?)
constexpr u32 Avl_TakeMax78D368 = 0x0078D368;      // (AVL*, Node** slot, Node** left, u8* changed):
                                                   // moves the left subtree's largest node into slot (name?)
constexpr u32 Mem_Alloc = 0x0054ED38;              // (size, 0, align)
constexpr u32 FreeList_Init = 0x00551A58;          // (FreeList*, elem_size, count, heap or 0, bool)
constexpr u32 FreeList_Free = 0x00556D18;          // (FreeList*, void*)
}  // namespace dfn

// AVL node (20 bytes): +0 s16 balance (right - left height), +2 s16 height,
// +4 u32 key, +8 value, +12 left, +16 right. AVL: +8 u8 rebalance.
constexpr u32 kBal = 0, kHeight = 2, kKey = 4, kLeft = 12, kRight = 16;
constexpr u32 kAvlRebalance = 8;
// The node free list, made on first use (as Prim_Build52374C's).
constexpr u32 kNodeList = 0x00F8F0D8;
constexpr u32 kNodeListVtbl = 0x00B0EFA8;
constexpr u32 kStrNodeList = 0x00AEEAC5;  // "static ApeTFreeList<AVL<KeyType, ItemType>::AVLNode<...>> &...GetAllocator() [with KeyType = unsigned int, ItemType = ApeAnimScriptVar *]"
constexpr u32 kAvlNodeSize = 20, kNodesPerBlock = 64;
}  // namespace hevent7d
}  // namespace sly4

using namespace sly4::hevent7d;

namespace {

s32 height(u32 n) { return n != 0 ? static_cast<s16>(load<u16>(n + kHeight)) : 0; }

// Height and balance of n from its children.
void update(u32 n) {
    const s32 l = height(load<u32>(n + kLeft)), r = height(load<u32>(n + kRight));
    store<u16>(n + kHeight, static_cast<u16>((l > r ? l : r) + 1));
    store<u16>(n + kBal, static_cast<u16>(r - l));
}

// The inlined rebalance of the subtree at slot: (optionally) the height
// update, then the right-heavy (> 1) and left-heavy (< -1) rotations.
void rebalance(PPUContext& ctx, u32 avl, u32 slot, bool heights = true) {
    if (heights) {
        if (const u32 n = load<u32>(slot); n != 0) update(n);
    }
    u32 n = load<u32>(slot);
    if (n == 0) return;
    if (static_cast<s16>(load<u16>(n + kBal)) > 1) {
        if (const u32 r = load<u32>(n + kRight); r != 0) {
            if (static_cast<s16>(load<u16>(r + kBal)) < 0) call(ctx, dfn::Avl_RotateRight785D58, {avl, n + kRight});
            call(ctx, dfn::Avl_RotateLeft785EA0, {avl, slot});
        }
    }
    n = load<u32>(slot);
    if (n == 0 || static_cast<s16>(load<u16>(n + kBal)) >= -1) return;
    const u32 l = load<u32>(n + kLeft);
    if (l == 0) return;
    if (static_cast<s16>(load<u16>(l + kBal)) > 0) call(ctx, dfn::Avl_RotateLeft785EA0, {avl, n + kLeft});
    call(ctx, dfn::Avl_RotateRight785D58, {avl, slot});
}

// Returns n to the node free list (made on first use; a failed Mem_Alloc
// leaves 0 and FreeList_Free is called on it). The two inlined copies leave
// r8 / r9 differently: {vtable, string} or {string, -}.
void free_node(PPUContext& ctx, u32 n, bool copy_a) {
    u32 list = load<u32>(kNodeList);
    if (list == 0) {
        const u32 l = static_cast<u32>(call(ctx, dfn::Mem_Alloc, {60, 0, 16}));
        if (l != 0) {
            for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u}) store<u32>(l + off, 0);
            const u16 h54 = load<u16>(l + 54);
            for (u32 off : {32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(l + off, 0);
            store<u16>(l + 52, 0);
            store<u16>(l + 54, static_cast<u16>(h54 & 0x7FFF));
            store<u32>(l, kNodeListVtbl);
            store<u32>(l + 56, kStrNodeList);
            if (copy_a)
                call(ctx, dfn::FreeList_Init, {l, kAvlNodeSize, kNodesPerBlock, 0, 0, kNodeListVtbl, kStrNodeList});
            else
                call(ctx, dfn::FreeList_Init, {l, kAvlNodeSize, kNodesPerBlock, 0, 0, kStrNodeList});
        }
        list = l;
        store<u32>(kNodeList, list);
    }
    call(ctx, dfn::FreeList_Free, {list, n});
}

}  // namespace

// void Map_Erase78D670(AVL* avl, Node** slot, u32 key, u8* changed)
// (name?): recursive AVL delete of key below slot. *changed = 0 when the key
// is absent; otherwise 1 and the node goes back to the free list (one child:
// it takes the node's place; two: Avl_TakeMax78D368 moves the left
// subtree's largest node there). On the way back up each changed level is
// rebalanced when the AVL's +8 flag is set (the level of the removed node
// always, but its left subtree only with the flag).
DECOMP(0078D670, Map_Erase78D670) {
    const u32 avl = arg<u32>(ctx, 0), slot = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2);
    const u32 changed = arg<u32>(ctx, 3);
    const u32 n = load<u32>(slot);
    if (n == 0) {
        store<u8>(changed, 0);
        return;
    }
    const u32 k = load<u32>(n + kKey);
    if (k != key) {
        call(ctx, dfn::Map_Erase78D670, {avl, n + (k > key ? kLeft : kRight), key, changed});
        if (load<u8>(changed) != 0 && load<u8>(avl + kAvlRebalance) != 0) rebalance(ctx, avl, slot);
        return;
    }
    const u32 right = load<u32>(n + kRight);
    if (right == 0) {
        store<u8>(changed, 1);
        store<u32>(slot, load<u32>(n + kLeft));
        free_node(ctx, n, true);
        rebalance(ctx, avl, slot);
        return;
    }
    if (load<u32>(n + kLeft) == 0) {
        store<u32>(slot, right);
        store<u8>(changed, 1);
        free_node(ctx, n, false);
        rebalance(ctx, avl, slot);
        return;
    }
    call(ctx, dfn::Avl_TakeMax78D368, {avl, slot, n + kLeft, changed});
    if (load<u8>(changed) != 0 && load<u8>(avl + kAvlRebalance) != 0) {
        if (const u32 l = load<u32>(load<u32>(slot) + kLeft); l != 0) update(l);
        rebalance(ctx, avl, load<u32>(slot) + kLeft, false);
        rebalance(ctx, avl, slot);
    }
    store<u8>(changed, 1);
}
