// World_HandleEvent callees, part 14b: the AVL<u32, ApeAnimScript*> rotations
// and two-children erase under Tree_Erase792490, the user spawn point and
// the end of the effect scenes.
#include "hevent14.h"

using namespace sly4;
using namespace sly4::hevent13;
using namespace sly4::hevent14;

namespace {
// Tree (r3): +8 u8 rebalance. Node (20 bytes, as hevent13_f.cpp): +0 s16
// balance (right height - left height), +2 s16 height, +4 u32 key, +8
// value, +12 left, +16 right.
constexpr u32 kTrRebalance = 8;
constexpr u32 kAnBalance = 0, kAnHeight = 2, kAnKey = 4, kAnValue = 8, kAnLeft = 12, kAnRight = 16;
// The node allocator (as hevent13_f.cpp): a 60-byte ApeTFreeList of 64 x
// 20-byte nodes, created on first use.
constexpr u32 kNodeAllocator = 0x00F8F0DC;
constexpr u32 kFreeListVtable = 0x00B0EFC8;
constexpr u32 kAllocatorName = 0x00AEEE29;  // "static ApeTFreeList<AVL<KeyType, ItemType>::AVLNode<...>> &...GetAllocator() [...]"
constexpr u32 kMemAlloc = 0x0054ED38, kFreeListInit = 0x00551A58, kFreeListFree = 0x00556D18;
constexpr u32 kFlSize = 60, kFlAlign = 16, kFlElem = 20, kFlCount = 64;

s32 height(u32 n) { return n != 0 ? static_cast<s16>(load<u16>(n + kAnHeight)) : 0; }
s32 balance(u32 n) { return static_cast<s16>(load<u16>(n + kAnBalance)); }
// Recomputes x's height and balance; returns the balance as r3 holds it.
u64 update(u32 x) {
    const s32 hl = height(load<u32>(x + kAnLeft)), hr = height(load<u32>(x + kAnRight));
    store<u16>(x + kAnHeight, static_cast<u16>((hl > hr ? hl : hr) + 1));
    store<u16>(x + kAnBalance, static_cast<u16>(hr - hl));
    return static_cast<u64>(static_cast<s64>(hr) - hl);
}
// After a rotation at link: updates the new subtree's children, then its
// root (each when present). Returns r3 (*link).
u64 fix_after_rotate(u32 link) {
    if (const u32 l = load<u32>(load<u32>(link) + kAnLeft); l != 0) update(l);
    if (const u32 r = load<u32>(load<u32>(link) + kAnRight); r != 0) update(r);
    const u32 x = load<u32>(link);
    if (x != 0) update(x);
    return x;
}
// Rebalances *link (non-zero), as Tree_Erase792490 does. Returns r3.
u64 rebalance(PPUContext& ctx, u32 t, u32 link) {
    const u32 x = load<u32>(link);
    if (balance(x) > 1) {
        const u32 r = load<u32>(x + kAnRight);
        if (r != 0) {
            if (balance(r) < 0) call(ctx, h14fn::Avl_RotateRight, {t, x + kAnRight});
            call(ctx, h14fn::Avl_RotateLeft, {t, link});
        }
    }
    const u32 y = load<u32>(link);
    if (y == 0 || balance(y) >= -1) return y;
    const u32 l = load<u32>(y + kAnLeft);
    if (l == 0) return y;
    if (balance(l) > 0) call(ctx, h14fn::Avl_RotateLeft, {t, y + kAnLeft});
    return call(ctx, h14fn::Avl_RotateRight, {t, link});
}
// Returns n to the node allocator (creating it first), as hevent13_f.cpp.
void free_node(PPUContext& ctx, u32 n) {
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
    call(ctx, kFreeListFree, {fl, n});
}

// Spawn table: 76-byte records at 0x010B86B8, +72 the point hash; the
// default position (vec3) at 0x010B89C0.
constexpr u32 kSpawnTable = 0x010B86B8, kSpawnSize = 76, kSpawnHash = 72;
constexpr u32 kDefaultPos = 0x010B89C0;

// World +0x1285A5: scenes ended flag; +0x12847C: the cutscene object (+164
// bit 31: playing); user block 0 +8: the player.
constexpr u32 kWScenesEnded = 0x1285A5, kWCutscene = 0x12847C, kCutPlaying = 164;
constexpr u32 kUb0Player = 4208 + 8;
constexpr u32 kScenesFinish48B330 = 0x0048B330;   // (World*, u8) (name?)
constexpr u32 kCutscene_Stop4269E4 = 0x004269E4;  // (cutscene, u8) (name?)
constexpr u32 kObjectsRelease = 0x0035B984;       // Objects_Release35B984 (0)
constexpr u32 kObjNotify26 = 0x000D94CC;          // Obj_Notify26 (Object*) -> u32
constexpr u32 kSub2A8AE0 = 0x002A8AE0;            // (x, 0, 1, 0) (name?)
constexpr u32 kGameSetPaused = 0x00365A54;        // Game_SetPaused (1, 0)
constexpr u32 kUiFind = 0x004D5544;               // (path, parent) -> Element*
constexpr u32 kBindClear = 0x004E3168;            // Obj_Set4E3168 (const char* source, u32 key)
constexpr u32 kTimelinePlay = 0x004E2EF0;         // (name, Element*, u8, u8)
constexpr u32 kStrLetterbox = 0x00AE8AF8;         // "InGame.Cutscene.Letterbox"
constexpr u32 kStrEmpty = 0x00AE65E4;             // ""
constexpr u32 kStrLetterboxShutdown = 0x00AE91D4; // "Letterbox_Shutdown"
}  // namespace

// Node* Avl_RotateLeft792040(Tree* t, Node** link) (name?): left rotation at
// *link (nothing but *link = 0 without a right child), then the heights and
// balances. Returns *link (the old node when there was no right child).
DECOMP(00792040, Avl_RotateLeft792040) {
    const u32 link = arg<u32>(ctx, 1);
    const u32 n = load<u32>(link);
    const u32 r = load<u32>(n + kAnRight);
    store<u32>(link, r);
    if (r == 0) return ret(ctx, n);
    store<u32>(n + kAnRight, load<u32>(r + kAnLeft));
    store<u32>(load<u32>(link) + kAnLeft, n);
    ctx.gpr[3] = fix_after_rotate(link);
}

// Node* Avl_RotateRight791EF8(Tree* t, Node** link) (name?): the mirror of
// Avl_RotateLeft792040.
DECOMP(00791EF8, Avl_RotateRight791EF8) {
    const u32 link = arg<u32>(ctx, 1);
    const u32 n = load<u32>(link);
    const u32 l = load<u32>(n + kAnLeft);
    store<u32>(link, l);
    if (l == 0) return ret(ctx, n);
    store<u32>(n + kAnLeft, load<u32>(l + kAnRight));
    store<u32>(load<u32>(link) + kAnRight, n);
    ctx.gpr[3] = fix_after_rotate(link);
}

// void Avl_EraseTwo792188(Tree* t, Node** link, Node** sub, u8* done)
// (name?): erase of *link when it has two children. Walks to the right-most
// node of the left subtree (*sub), moves its key and value into *link,
// unlinks and frees it (*done = 1), rebalancing on the way back up.
DECOMP(00792188, Avl_EraseTwo792188) {
    const u32 t = arg<u32>(ctx, 0), link = arg<u32>(ctx, 1), sub = arg<u32>(ctx, 2), done = arg<u32>(ctx, 3);
    const u32 n = load<u32>(sub);
    if (load<u32>(n + kAnRight) != 0) {
        call(ctx, h14fn::Avl_EraseTwo, {t, link, n + kAnRight, done});
        if (load<u8>(done) == 0) return ret(ctx, 0u);
        const u8 rb = load<u8>(t + kTrRebalance);
        if (rb == 0) return ret(ctx, 0u);
        u64 r3 = rb;
        u32 m = load<u32>(sub);
        if (m != 0) {
            r3 = update(m);
            m = load<u32>(sub);
        }
        if (m == 0) {
            ctx.gpr[3] = r3;
            return;
        }
        ctx.gpr[3] = rebalance(ctx, t, sub);
        return;
    }
    store<u32>(load<u32>(link) + kAnKey, load<u32>(n + kAnKey));
    store<u32>(load<u32>(link) + kAnValue, load<u32>(load<u32>(sub) + kAnValue));
    const u32 x = load<u32>(sub);
    store<u32>(sub, load<u32>(x + kAnLeft));
    free_node(ctx, x);
    store<u8>(done, 1);
    ret(ctx, 1u);
}

// void World_SpawnPoint486460(World*, u32 user, Mat* out) (name?): the 3x4
// matrix of the named point in the user's spawn record (0x010B86B8 [user]
// +72), or identity at the default position when there is none.
DECOMP(00486460, World_SpawnPoint486460) {
    const u32 user = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    const u32 hash = load<u32>(kSpawnTable + user * kSpawnSize + kSpawnHash);
    if (hash != 0) {
        Stack frame(ctx, 16);  // r1+112: the key
        const u32 pt = find_point(ctx, frame.addr(), hash);
        if (pt != 0) {
            copy_mat(ctx, out, pt + kPointMatrix);
            ret(ctx, pt + kPointMatrix);
            return;
        }
    }
    copy_mat(ctx, out, kIdentity);
    for (u32 k = 0; k < 12; k += 4) store<u32>(out + 48 + k, load<u32>(kDefaultPos + k));
    ctx.fpr[1] = hevent7::lfs_f64(kDefaultPos + 8);
    ret(ctx, out + 48);
}

// void World_ScenesEnd48B674(World* w, u8 keep) (name?): unless already done
// (+0x1285A5), finishes the scenes (0x0048B330). With keep: stops a playing
// cutscene (u8 1), releases objects and, for a player with +192 bit 3,
// passes its Obj_Notify26 result on to 0x002A8AE0. Without: stops it (0),
// unpauses the game and plays "Letterbox_Shutdown" on the letterbox.
DECOMP(0048B674, World_ScenesEnd48B674) {
    const u32 w = arg<u32>(ctx, 0);
    const u8 keep = arg<u8>(ctx, 1);
    const u8 ended = load<u8>(w + kWScenesEnded);
    if (ended != 0) return ret(ctx, static_cast<u32>(ended));
    call(ctx, kScenesFinish48B330, {w, keep});
    const u32 cut = load<u32>(w + kWCutscene);
    const bool playing = cut != 0 && (load<u32>(cut + kCutPlaying) & 0x80000000u) != 0;
    if (keep != 0) {
        if (playing) call(ctx, kCutscene_Stop4269E4, {cut, 1});
        call(ctx, kObjectsRelease, {0});
        const u32 p = load<u32>(w + kUb0Player);
        if (p == 0) return ret(ctx, w + 4208);
        if ((load<u32>(p + kObjFlags192) & kObjFlag192Bit3) == 0) return ret(ctx, 0u);
        if (static_cast<u32>(call(ctx, kObjNotify26, {p})) == 0) return;  // r3 = 0
        const u32 x = static_cast<u32>(call(ctx, kObjNotify26, {p}));
        call(ctx, kSub2A8AE0, {x, 0, 1, 0});
        return;
    }
    if (playing) call(ctx, kCutscene_Stop4269E4, {cut, 0});
    call(ctx, kGameSetPaused, {1, 0});
    const u32 ui = static_cast<u32>(call(ctx, kUiFind, {kStrLetterbox, 0}));
    if (ui == 0) return;  // r3 = 0
    call(ctx, kBindClear, {kStrEmpty, ui});
    call(ctx, kTimelinePlay, {kStrLetterboxShutdown, ui, 1, 0});
}
