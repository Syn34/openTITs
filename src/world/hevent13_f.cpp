// World_HandleEvent callees, part 13f: Tree_Erase792490, the AVL tree erase
// of the engine's AVL<u32, ApeAnimScript*> (used by Set_Stop4BD1CC).
#include "hevent13.h"

using namespace sly4;
using namespace sly4::hevent13;

namespace {
// Tree (r3): +8 u8 rebalance. Node (20 bytes): +0 s16 balance (right height
// - left height), +2 s16 height, +4 u32 key, +8 value, +12 left, +16 right.
constexpr u32 kTrRebalance = 8;
constexpr u32 kAnBalance = 0, kAnHeight = 2, kAnKey = 4, kAnLeft = 12, kAnRight = 16;
constexpr u32 kRotateLeft = 0x00792040;   // (Tree*, Node** link) (name?)
constexpr u32 kRotateRight = 0x00791EF8;  // (Tree*, Node** link) (name?)
constexpr u32 kEraseTwo = 0x00792188;     // (Tree*, Node** link, Node** left, u8* done): node with two children (name?)
constexpr u32 kTreeErase = 0x00792490;
// The node allocator (AVL<...>::GetAllocator(), created on first use): a
// 60-byte ApeTFreeList of 64 x 20-byte nodes.
constexpr u32 kNodeAllocator = 0x00F8F0DC;  // ApeTFreeList*
constexpr u32 kFreeListVtable = 0x00B0EFC8;
constexpr u32 kAllocatorName = 0x00AEEE29;  // "static ApeTFreeList<AVL<KeyType, ItemType>::AVLNode<...>> &...GetAllocator() [...]"
constexpr u32 kMemAlloc = 0x0054ED38, kFreeListInit = 0x00551A58, kFreeListFree = 0x00556D18;
constexpr u32 kFlSize = 60, kFlAlign = 16, kFlElem = 20, kFlCount = 64;

s32 height(u32 n) { return n != 0 ? static_cast<s16>(load<u16>(n + kAnHeight)) : 0; }
// Recomputes x's height and balance; returns the balance (r3).
u64 update(u32 x) {
    const s32 hl = height(load<u32>(x + kAnLeft)), hr = height(load<u32>(x + kAnRight));
    store<u16>(x + kAnHeight, static_cast<u16>((hl > hr ? hl : hr) + 1));
    store<u16>(x + kAnBalance, static_cast<u16>(hr - hl));
    return static_cast<u64>(static_cast<s64>(hr) - hl);
}
s32 balance(u32 n) { return static_cast<s16>(load<u16>(n + kAnBalance)); }
// Rebalances *link (non-zero): right-heavy first, then left-heavy (on the
// node then at *link). Returns r3 as the original leaves it.
u64 rebalance(PPUContext& ctx, u32 t, u32 link) {
    const u32 x = load<u32>(link);
    if (balance(x) > 1) {
        const u32 r = load<u32>(x + kAnRight);
        if (r != 0) {
            if (balance(r) < 0) call(ctx, kRotateRight, {t, x + kAnRight});
            call(ctx, kRotateLeft, {t, link});
        }
    }
    const u32 y = load<u32>(link);
    if (y == 0 || balance(y) >= -1) return y;
    const u32 l = load<u32>(y + kAnLeft);
    if (l == 0) return y;
    if (balance(l) > 0) call(ctx, kRotateLeft, {t, y + kAnLeft});
    return call(ctx, kRotateRight, {t, link});
}
// Returns n to the node allocator (creating it first). Returns r3.
u64 free_node(PPUContext& ctx, u32 n) {
    u32 fl = load<u32>(kNodeAllocator);
    if (fl == 0) {
        const u32 a = static_cast<u32>(call(ctx, kMemAlloc, {kFlSize, 0, kFlAlign}));
        if (a != 0) {
            for (u32 o : {8u, 4u, 12u, 20u, 16u, 24u}) store<u32>(a + o, 0);
            const u32 f54 = load<u16>(a + 54);
            for (u32 o : {32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(a + o, 0);
            store<u16>(a + 52, 0);
            store<u16>(a + 54, static_cast<u16>(f54 & ~0x8000u));
            store<u32>(a + 0, kFreeListVtable);
            store<u32>(a + 56, kAllocatorName);
            ctx.gpr[8] = kFreeListVtable;  // left in r8 / r9 by the original
            ctx.gpr[9] = kAllocatorName;
            call(ctx, kFreeListInit, {a, kFlElem, kFlCount, 0, 0});
            fl = a;
        }
        store<u32>(kNodeAllocator, fl);
    }
    return call(ctx, kFreeListFree, {fl, n});
}
}  // namespace

// void Tree_Erase792490(Tree* t, Node** link, u32 key, u8* done) (name?):
// recursive AVL erase of key below *link; *done is set when a node was
// removed (0 when the key is missing). On the way back up, with t +8, the
// heights and balances are updated and the subtree rebalanced.
DECOMP(00792490, Tree_Erase792490) {
    const u32 t = arg<u32>(ctx, 0), link = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2), done = arg<u32>(ctx, 3);
    const u32 n = load<u32>(link);
    if (n == 0) {
        store<u8>(done, 0);
        return ret(ctx, 0u);
    }
    const u32 k = load<u32>(n + kAnKey);
    if (k != key) {
        call(ctx, kTreeErase, {t, (k > key ? n + kAnLeft : n + kAnRight), key, done});
        if (load<u8>(done) == 0 || load<u8>(t + kTrRebalance) == 0) return ret(ctx, 0u);
        const u32 x = load<u32>(link);
        if (x == 0) return ret(ctx, load<u8>(t + kTrRebalance));
        update(x);
        ctx.gpr[3] = rebalance(ctx, t, link);
        return;
    }
    const u32 right = load<u32>(n + kAnRight);
    if (right == 0 || load<u32>(n + kAnLeft) == 0) {
        if (right == 0) {
            const u32 left = load<u32>(n + kAnLeft);
            store<u8>(done, 1);
            store<u32>(link, left);
        } else {
            store<u32>(link, right);
            store<u8>(done, 1);
        }
        const u64 r = free_node(ctx, n);
        const u32 x = load<u32>(link);
        if (x == 0) {
            ctx.gpr[3] = r;
            return;
        }
        update(x);
        ctx.gpr[3] = rebalance(ctx, t, link);
        return;
    }
    // Two children.
    call(ctx, kEraseTwo, {t, link, n + kAnLeft, done});
    if (load<u8>(done) != 0 && load<u8>(t + kTrRebalance) != 0) {
        const u32 l = load<u32>(load<u32>(link) + kAnLeft);
        if (l != 0) update(l);
        const u32 llink = load<u32>(link) + kAnLeft;
        if (load<u32>(llink) != 0) rebalance(ctx, t, llink);
        const u32 x = load<u32>(link);
        if (x != 0) {
            update(x);
            if (load<u32>(link) != 0) rebalance(ctx, t, link);
        }
    }
    store<u8>(done, 1);
    ret(ctx, 1u);
}
