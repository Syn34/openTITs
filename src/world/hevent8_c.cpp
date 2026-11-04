// World_HandleEvent callees, part 8c: the AVL map's take-max step (used by
// Map_Erase78D670 in hevent7_d.cpp).
#include "hevent8.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent8;

namespace sly4 {
namespace hevent8c {
namespace cfn {
constexpr u32 Avl_TakeMax78D368 = 0x0078D368;
constexpr u32 Avl_RotateRight785D58 = 0x00785D58;
constexpr u32 Avl_RotateLeft785EA0 = 0x00785EA0;
constexpr u32 Mem_Alloc = 0x0054ED38;      // (size, 0, align)
constexpr u32 FreeList_Init = 0x00551A58;  // (FreeList*, elem_size, count, heap or 0, bool)
constexpr u32 FreeList_Free = 0x00556D18;  // (FreeList*, void*)
}  // namespace cfn

// AVL node (as hevent7_d.cpp): +0 s16 balance, +2 s16 height, +4 key, +8
// value, +12 left, +16 right. AVL: +8 u8 rebalance.
constexpr u32 kBal = 0, kHeight = 2, kKey = 4, kValue = 8, kLeft = 12, kRight = 16;
constexpr u32 kAvlRebalance = 8;
constexpr u32 kNodeList = 0x00F8F0D8;      // the node free list, made on first use
constexpr u32 kNodeListVtbl = 0x00B0EFA8;
constexpr u32 kStrNodeList = 0x00AEEAC5;   // "static ApeTFreeList<AVL<...>::AVLNode<...>> &...GetAllocator() ..."
constexpr u32 kAvlNodeSize = 20, kNodesPerBlock = 64;
}  // namespace hevent8c
}  // namespace sly4

using namespace sly4::hevent8c;

namespace {

s32 avl_height(u32 n) { return n != 0 ? static_cast<s16>(load<u16>(n + kHeight)) : 0; }

void avl_fix(u32 n) {
    const s32 l = avl_height(load<u32>(n + kLeft)), r = avl_height(load<u32>(n + kRight));
    store<u16>(n + kHeight, static_cast<u16>((l > r ? l : r) + 1));
    store<u16>(n + kBal, static_cast<u16>(r - l));
}

// The inlined rebalance of the subtree at slot (as hevent7_d.cpp's): the
// height update, then the right-heavy (> 1) and left-heavy (< -1)
// rotations, double when the child leans the other way.
void avl_rebalance(PPUContext& ctx, u32 avl, u32 slot) {
    if (const u32 n = load<u32>(slot); n != 0) avl_fix(n);
    u32 n = load<u32>(slot);
    if (n == 0) return;
    if (static_cast<s16>(load<u16>(n + kBal)) > 1) {
        if (const u32 r = load<u32>(n + kRight); r != 0) {
            if (static_cast<s16>(load<u16>(r + kBal)) < 0) call(ctx, cfn::Avl_RotateRight785D58, {avl, n + kRight});
            call(ctx, cfn::Avl_RotateLeft785EA0, {avl, slot});
        }
    }
    n = load<u32>(slot);
    if (n == 0 || static_cast<s16>(load<u16>(n + kBal)) >= -1) return;
    const u32 l = load<u32>(n + kLeft);
    if (l == 0) return;
    if (static_cast<s16>(load<u16>(l + kBal)) > 0) call(ctx, cfn::Avl_RotateLeft785EA0, {avl, n + kLeft});
    call(ctx, cfn::Avl_RotateRight785D58, {avl, slot});
}

}  // namespace

// void Avl_TakeMax78D368(AVL* avl, Node** dst, Node** slot, u8* changed)
// (name?): removes the largest node below slot and moves its key and value
// into *dst. With a right child it recurses there and, when something
// changed and the AVL's +8 flag is set, rebalances slot; otherwise the
// node's left child takes its place, the node goes back to the free list
// (made on first use; a failed Mem_Alloc leaves 0 and FreeList_Free is
// called on it) and *changed = 1.
DECOMP(0078D368, Avl_TakeMax78D368) {
    const u32 avl = arg<u32>(ctx, 0), dst = arg<u32>(ctx, 1), slot = arg<u32>(ctx, 2), changed = arg<u32>(ctx, 3);
    const u32 n = load<u32>(slot);
    if (load<u32>(n + kRight) != 0) {
        call(ctx, cfn::Avl_TakeMax78D368, {avl, dst, n + kRight, changed});
        if (load<u8>(changed) == 0) return;
        if (load<u8>(avl + kAvlRebalance) == 0) return;
        avl_rebalance(ctx, avl, slot);
        return;
    }
    store<u32>(load<u32>(dst) + kKey, load<u32>(n + kKey));
    store<u32>(load<u32>(dst) + kValue, load<u32>(load<u32>(slot) + kValue));
    const u32 node = load<u32>(slot);
    store<u32>(slot, load<u32>(node + kLeft));
    u32 list = load<u32>(kNodeList);
    if (list == 0) {
        const u32 l = static_cast<u32>(call(ctx, cfn::Mem_Alloc, {60, 0, 16}));
        if (l != 0) {
            for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u}) store<u32>(l + off, 0);
            const u16 h54 = load<u16>(l + 54);
            for (u32 off : {32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(l + off, 0);
            store<u16>(l + 52, 0);
            store<u16>(l + 54, static_cast<u16>(h54 & 0x7FFF));
            store<u32>(l, kNodeListVtbl);
            store<u32>(l + 56, kStrNodeList);
            // r8 / r9 are left as the vtable and the name, as in the original.
            call(ctx, cfn::FreeList_Init, {l, kAvlNodeSize, kNodesPerBlock, 0, 0, kNodeListVtbl, kStrNodeList});
        }
        list = l;
        store<u32>(kNodeList, list);
    }
    call(ctx, cfn::FreeList_Free, {list, node});
    store<u8>(changed, 1);
    ctx.gpr[3] = 1;
}

namespace sly4 {
namespace hevent8c {
namespace cfn {
constexpr u32 EffectDef_Find = 0x004BDEDC;       // (hash a, hash b, 1) -> def or 0
constexpr u32 Hash_ToString = 0x0056A6E0;        // (hash)
constexpr u32 PtrVector_Construct = 0x005516E0;  // (vec)
constexpr u32 PtrVector_Init = 0x00551598;       // (vec, capacity, 0, 1)
}  // namespace cfn

constexpr u32 kSceneEnabled = 0x010B8B48;     // u8: scenes may start (name?)
constexpr u32 kSceneCapacity = 0x00F65C7C;    // u32 initial argument capacity (name?)
constexpr u32 kSession = 0x00F64C20;          // u32 session object, +76 checked (name?)
// World +0x12820C: vector of queued scenes {+0 Scene**, +4 u16 capacity,
// +8 u16 count, +10 u16 high-water}, at most 4; +0x128220..22 u8 flags.
constexpr u32 kWScenes = 0x0012820C, kWSceneCap = 0x00128210, kWSceneCount = 0x00128214, kWSceneHigh = 0x00128216;
constexpr u32 kWSceneFlagA = 0x00128220, kWSceneFlagB = 0x00128221, kWSceneFirst = 0x00128222;
constexpr u32 kMaxScenes = 4;
// Scene (52 bytes): an argument vector (+0..+11, element size at +6 = 76)
// then +20 e, +24 def, +28/+32 0, +36 f32 1.0, +40 hash a, +44 d,
// +48 flags (bit 31 c & 1, bit 30 g & 1, bit 29 cleared).
constexpr u32 kSceneSize = 52;
constexpr u32 kSE = 20, kSDef = 24, kSRate = 36, kSHash = 40, kSD = 44, kSFlags = 48;
}  // namespace hevent8c
}  // namespace sly4

// void World_StartScene46C3B8(World* w, u32 a, u32 b, u32 c, ArgVector*
// args, u32 d, u32 e, u8 f, u8 g (stack), u8 h (stack)) (name?): queues
// the effect scene (a, b) on the world: nothing when scenes are disabled
// (0x010B8B48), a is 0 or 4 are queued; an unknown def logs both hashes
// (Hash_ToString). The first one sets w +0x128222; f (or the session's +76)
// and h are ORed into the flag bytes +0x128220 / +0x128221. The scene gets a
// copy of args. A failed Mem_Alloc leaves the scene 0 and its fields are
// still written at 0 + offset (as in the original).
DECOMP(0046C3B8, World_StartScene46C3B8) {
    const u32 w = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    const u32 args = arg<u32>(ctx, 4), d = arg<u32>(ctx, 5), e = arg<u32>(ctx, 6), f = arg<u32>(ctx, 7) & 0xFF;
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 g = load<u32>(sp + 116), h = load<u32>(sp + 124) & 0xFF;  // 9th and 10th arguments
    const u32 enabled = load<u8>(kSceneEnabled);
    ctx.gpr[3] = enabled;
    if (enabled == 0 || a == 0) return;
    const u32 def = static_cast<u32>(call(ctx, cfn::EffectDef_Find, {a, b, 1}));
    if (def == 0) {
        call(ctx, cfn::Hash_ToString, {a});
        call(ctx, cfn::Hash_ToString, {b});
        return;
    }
    const u32 n = load<u16>(w + kWSceneCount);
    ctx.gpr[3] = n;
    if (!(n < kMaxScenes)) return;
    if (n == 0) store<u8>(w + kWSceneFirst, 1);
    const u32 flag = f != 0 || load<u32>(load<u32>(kSession) + 76) != 0 ? 1 : 0;
    store<u8>(w + kWSceneFlagA, (load<u8>(w + kWSceneFlagA) | flag) != 0 ? 1 : 0);
    store<u8>(w + kWSceneFlagB, (load<u8>(w + kWSceneFlagB) | h) != 0 ? 1 : 0);
    const u32 s0 = static_cast<u32>(call(ctx, cfn::Mem_Alloc, {kSceneSize, 0, 16}));
    u32 s = 0;
    if (s0 != 0) {
        const u32 cap = load<u32>(kSceneCapacity);
        call(ctx, cfn::PtrVector_Construct, {s0});
        store<u16>(s0 + 6, kArgSize);
        call(ctx, cfn::PtrVector_Init, {s0, cap, 0, 1});
        const u32 fl = load<u32>(s0 + kSFlags);
        store<u32>(s0 + kSE, 0);
        store<u32>(s0 + kSDef, 0);
        store<u32>(s0 + 28, 0);
        store<u32>(s0 + 32, 0);
        store<u32>(s0 + kSRate, 0x3F800000u);  // 1.0f
        store<u32>(s0 + kSHash, 0);
        store<u32>(s0 + kSD, 0);
        store<u32>(s0 + kSFlags, (fl | 0x80000000u) & ~0x60000000u);
        s = s0;
    }
    const u32 fl = load<u32>(s + kSFlags);
    store<u32>(s + kSHash, a);
    store<u32>(s + kSDef, def);
    store<u32>(s + kSD, d);
    store<u32>(s + kSE, e);
    store<u32>(s + kSFlags, (fl & 0x3FFFFFFFu) | ((c & 1) << 31) | ((g & 1) << 30));
    if (args != 0) {
        u32 off = 0;
        for (u32 i = 0; i < load<u16>(args + 8); ++i, off += kArgSize) {
            const u32 src = load<u32>(args) + off;
            u32 cnt = load<u16>(s + 8);
            const u32 cap = load<u16>(s + 4);
            if (cnt == cap) {
                call(ctx, h8fn::PtrVector_Reserve, {s, cap << 1});
                cnt = load<u16>(s + 8);
            }
            call(ctx, h8fn::Memmove, {load<u32>(s) + cnt * kArgSize, src, kArgCopy});
            const u32 count = load<u16>(s + 8) + 1;
            const u32 hw = load<u16>(s + 10);
            store<u16>(s + 8, static_cast<u16>(count));
            store<u16>(s + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(count & 0xFFFF) ? hw : count));
        }
    }
    u32 cnt = load<u16>(w + kWSceneCount);
    const u32 cap = load<u16>(w + kWSceneCap);
    if (cnt == cap) {
        call(ctx, h8fn::PtrVector_Reserve, {w + kWScenes, (cap << 1) & 0x1FFFEu});
        cnt = load<u16>(w + kWSceneCount);
    }
    store<u32>(load<u32>(w + kWScenes) + ((cnt << 2) & 0x3FFFCu), s);
    const u32 count = load<u16>(w + kWSceneCount) + 1;
    const u32 hw = load<u16>(w + kWSceneHigh);
    store<u16>(w + kWSceneCount, static_cast<u16>(count));
    const u32 r = static_cast<s32>(hw) > static_cast<s32>(count & 0xFFFF) ? hw : count;
    store<u16>(w + kWSceneHigh, static_cast<u16>(r));
    ctx.gpr[3] = r;
}
