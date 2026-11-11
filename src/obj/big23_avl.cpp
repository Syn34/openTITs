// Game code, large-function sweep part 23: the engine's AVL template
// removes (AVL<float, ApeAnimScriptEvent*> and the f32-keyed multimap
// variant), each with the rebalance inlined six times.
#include "big23.h"

using namespace sly4;
using namespace sly4::big23;

namespace {
// AVL node: +0 s16 balance (right - left height), +2 s16 height, +4 f32
// key, +8 value, +12 left, +16 right. AVL: +8 u8 rebalance flag.
constexpr u32 kBal = 0, kHeight = 2, kKey = 4, kLeft = 12, kRight = 16;
constexpr u32 kAvlRebalance = 8;

namespace afn {
constexpr u32 Avl_RemoveF32 = 0x0078FD34u;
constexpr u32 AvlF_RotateRight = 0x0078F180u;  // (Tree*, Node** link): the left child up
constexpr u32 AvlF_RotateLeft = 0x0078F2C8u;   // (Tree*, Node** link): the right child up
constexpr u32 AvlF_TakeMax = 0x0078FA2Cu;      // (Tree*, Node** slot, Node** left, u8* changed) (name?)
constexpr u32 Mem_Alloc = 0x0054ED38u;         // (size, 0, align)
constexpr u32 FreeList_Init = 0x00551A58u;     // (FreeList*, elem_size, count, heap or 0, bool)
constexpr u32 FreeList_Free = 0x00556D18u;     // (FreeList*, void*)
}  // namespace afn

// The AVL<float, ApeAnimScriptEvent*> node free list, made on first use.
constexpr u32 kEvNodeList = 0x00F8F0FCu;
constexpr u32 kEvNodeListVtbl = 0x00B22530u;
constexpr u32 kStrEvNodeList = 0x00AEEC21u;  // "static ApeTFreeList<...> &AVL<KeyType, ItemType>::GetAllocator() [with KeyType = float, ItemType = ApeAnimScriptEvent *]"
constexpr u32 kAvlNodeSize = 20, kNodesPerBlock = 64;

s32 Height(u32 n) { return n != 0 ? static_cast<s16>(load<u16>(n + kHeight)) : 0; }
s32 Bal(u32 n) { return static_cast<s16>(load<u16>(n + kBal)); }

// Height and balance of n from its children.
void Update(u32 n) {
    const s32 l = Height(load<u32>(n + kLeft)), r = Height(load<u32>(n + kRight));
    store<u16>(n + kHeight, static_cast<u16>((l > r ? l : r) + 1));
    store<u16>(n + kBal, static_cast<u16>(r - l));
}

// The rotations a template instance calls.
struct Rot {
    u32 right, left;
};

// The inlined rebalance of the subtree at slot: (optionally) the height
// update, then the right-heavy (> 1) and left-heavy (< -1) rotations. The
// multimap's copies skip the null check of the reloaded *slot before the
// left-heavy test (check_left = false).
void Rebalance(PPUContext& ctx, const Rot& rot, u32 avl, u32 slot, bool heights = true, bool check_left = true) {
    if (heights) {
        if (const u32 n = load<u32>(slot); n != 0) Update(n);
    }
    u32 n = load<u32>(slot);
    if (n == 0) return;
    if (Bal(n) > 1) {
        if (const u32 r = load<u32>(n + kRight); r != 0) {
            if (Bal(r) < 0) call(ctx, rot.right, {avl, n + kRight});
            call(ctx, rot.left, {avl, slot});
        }
    }
    n = load<u32>(slot);
    if ((check_left && n == 0) || Bal(n) >= -1) return;
    const u32 l = load<u32>(n + kLeft);
    if (l == 0) return;
    if (Bal(l) > 0) call(ctx, rot.left, {avl, n + kLeft});
    call(ctx, rot.right, {avl, slot});
}

// Returns n to the event node free list (made on first use; a failed
// Mem_Alloc leaves 0 and FreeList_Free is called on it). The two inlined
// copies leave r8 / r9 differently: {string, -} or {vtable, string}.
void FreeEvNode(PPUContext& ctx, u32 n, bool with_vtbl) {
    u32 list = load<u32>(kEvNodeList);
    if (list == 0) {
        const u32 l = static_cast<u32>(call(ctx, afn::Mem_Alloc, {60, 0, 16}));
        if (l != 0) {
            for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u}) store<u32>(l + off, 0);
            const u16 h54 = load<u16>(l + 54);
            for (u32 off : {32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(l + off, 0);
            store<u16>(l + 52, 0);
            store<u16>(l + 54, static_cast<u16>(h54 & 0x7FFF));
            store<u32>(l, kEvNodeListVtbl);
            store<u32>(l + 56, kStrEvNodeList);
            if (with_vtbl)
                call(ctx, afn::FreeList_Init, {l, kAvlNodeSize, kNodesPerBlock, 0, 0, kEvNodeListVtbl, kStrEvNodeList});
            else
                call(ctx, afn::FreeList_Init, {l, kAvlNodeSize, kNodesPerBlock, 0, 0, kStrEvNodeList});
        }
        list = l;
        store<u32>(kEvNodeList, list);
    }
    call(ctx, afn::FreeList_Free, {list, n});
}
}  // namespace

namespace {
namespace mfn {
constexpr u32 AvlMulti_RemoveF32 = 0x0074A210u;
constexpr u32 Avl_RotateR = 0x00749D40u;     // (?, Node** pp): the +12 child up
constexpr u32 Avl_RotateL = 0x00749E88u;     // (?, Node** pp): the +16 child up
constexpr u32 AvlMulti_TakeMax = 0x00749FD0u;  // (AVL*, Node** slot, Node** left, u8* changed) (name?)
constexpr u32 SmallFree = 0x00552420u;       // (p, size)
}  // namespace mfn
// The multimap's value chain: the tree node (24 bytes) holds the first
// value at +8 and links further 24-byte entries {+8 value, +20 next} at +20.
constexpr u32 kValue = 8, kNext = 20, kEntrySize = 24;
}  // namespace

// void Avl_RemoveF32_0078FD34(AVL* avl, Node** slot, -, u8* changed; f32
// key): recursive AVL delete of key below slot (the float twin of
// Map_Erase78D670). *changed = 0 when slot is empty; a node key > key
// recurses left, < key right, anything else (equal or NaN) deletes: the
// changed flag is 1 and the node goes back to the free list (one child: it
// takes the node's place; two: AvlF_TakeMax moves the left subtree's
// largest node there). Each changed level is rebalanced when the AVL's +8
// flag is set (the level of a removed one-child node always). r5 is passed
// through to the recursive calls untouched.
DECOMP(0078FD34, Avl_RemoveF32_0078FD34) {
    const u32 avl = arg<u32>(ctx, 0), slot = arg<u32>(ctx, 1);
    const u64 r5 = ctx.gpr[5];
    const u32 changed = arg<u32>(ctx, 3);
    const f64 key = farg(ctx, 0);
    constexpr Rot rot{afn::AvlF_RotateRight, afn::AvlF_RotateLeft};
    const u32 n = load<u32>(slot);
    if (n == 0) {
        store<u8>(changed, 0);
        return;
    }
    const f64 k = Lf(n + kKey);
    if (k > key || k < key) {
        callf(ctx, afn::Avl_RemoveF32, {avl, n + (k > key ? kLeft : kRight), r5, changed}, {key});
        if (load<u8>(changed) != 0 && load<u8>(avl + kAvlRebalance) != 0) Rebalance(ctx, rot, avl, slot);
        return;
    }
    const u32 right = load<u32>(n + kRight);
    if (right == 0) {
        store<u8>(changed, 1);
        store<u32>(slot, load<u32>(n + kLeft));
        FreeEvNode(ctx, n, false);
        Rebalance(ctx, rot, avl, slot);
        return;
    }
    if (load<u32>(n + kLeft) == 0) {
        store<u32>(slot, right);
        store<u8>(changed, 1);
        FreeEvNode(ctx, n, true);
        Rebalance(ctx, rot, avl, slot);
        return;
    }
    call(ctx, afn::AvlF_TakeMax, {avl, slot, n + kLeft, changed});
    if (load<u8>(changed) != 0 && load<u8>(avl + kAvlRebalance) != 0) {
        if (const u32 l = load<u32>(load<u32>(slot) + kLeft); l != 0) Update(l);
        Rebalance(ctx, rot, avl, load<u32>(slot) + kLeft, false);
        Rebalance(ctx, rot, avl, slot);
    }
    store<u8>(changed, 1);
}

// void AvlMulti_RemoveF32_74A210(AVL* avl, Node** slot, u32 value, -, u8*
// changed; f32 key): the f32-keyed multimap remove. Empty slot: *changed =
// 0. Node key > key / < key recurses left / right (r6 passed through). On
// the key: value 0 or the node's own value removes the head, another value
// is looked up in the chain (absent: nothing, *changed untouched) and
// unlinked and freed. A head with followers swaps in the first follower's
// value and link (the follower gets the old value and is freed); a lone
// head deletes the tree node as Avl_RemoveF32_0078FD34 does (SmallFree
// instead of the free list). The inlined rebalances read the reloaded
// *slot's balance without a null check after the right-heavy step.
DECOMP(0074A210, AvlMulti_RemoveF32_74A210) {
    const u32 avl = arg<u32>(ctx, 0), slot = arg<u32>(ctx, 1), value = arg<u32>(ctx, 2);
    const u64 r6 = ctx.gpr[6];
    const u32 changed = arg<u32>(ctx, 4);
    const f64 key = farg(ctx, 0);
    constexpr Rot rot{mfn::Avl_RotateR, mfn::Avl_RotateL};
    const u32 n = load<u32>(slot);
    if (n == 0) {
        store<u8>(changed, 0);
        return;
    }
    const f64 k = Lf(n + kKey);
    if (k > key || k < key) {
        callf(ctx, mfn::AvlMulti_RemoveF32, {avl, n + (k > key ? kLeft : kRight), value, r6, changed}, {key});
        if (load<u8>(changed) != 0 && load<u8>(avl + kAvlRebalance) != 0) Rebalance(ctx, rot, avl, slot, true, false);
        return;
    }
    u32 e = n, prev = 0;
    if (value != 0) {
        while (load<u32>(e + kValue) != value) {
            prev = e;
            e = load<u32>(e + kNext);
            if (e == 0) return;
        }
    }
    if (prev != 0) {
        store<u32>(prev + kNext, load<u32>(e + kNext));
        call(ctx, mfn::SmallFree, {e, kEntrySize});
        return;
    }
    if (const u32 next = load<u32>(n + kNext); next != 0) {
        const u32 link = load<u32>(next + kNext), v = load<u32>(n + kValue);
        store<u32>(n + kNext, link);
        store<u32>(n + kValue, load<u32>(next + kValue));
        store<u32>(next + kValue, v);
        call(ctx, mfn::SmallFree, {next, kEntrySize});
        return;
    }
    const u32 right = load<u32>(n + kRight);
    if (right == 0) {
        store<u8>(changed, 1);
        store<u32>(slot, load<u32>(n + kLeft));
        call(ctx, mfn::SmallFree, {n, kEntrySize});
        Rebalance(ctx, rot, avl, slot, true, false);
        return;
    }
    if (load<u32>(n + kLeft) == 0) {
        store<u32>(slot, right);
        store<u8>(changed, 1);
        call(ctx, mfn::SmallFree, {n, kEntrySize});
        Rebalance(ctx, rot, avl, slot, true, false);
        return;
    }
    call(ctx, mfn::AvlMulti_TakeMax, {avl, slot, n + kLeft, changed});
    if (load<u8>(changed) != 0 && load<u8>(avl + kAvlRebalance) != 0) {
        if (const u32 l = load<u32>(load<u32>(slot) + kLeft); l != 0) Update(l);
        Rebalance(ctx, rot, avl, load<u32>(slot) + kLeft, false, false);
        Rebalance(ctx, rot, avl, slot, true, false);
    }
    store<u8>(changed, 1);
}
