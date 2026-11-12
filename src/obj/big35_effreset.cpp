// Game code, large-function sweep part 35: emptying the effect system's
// deferred list and its five keyed registries.
#include "../sly4.h"
#include "big35.h"

using namespace sly4;
using namespace sly4::big35;

namespace {
constexpr u32 kDeferredReady = 0x010C2960;  // u8 flag (hevent20_f), read and cleared here as a doubleword
constexpr u32 kDeferred = 0x010C2968;       // PtrVector of the deferred effects

// The registries: AVL trees {+0 Node* root, +4 s32 count} with an iterator
// at +12 (hevent6.h layout: +12 Tree* (+0 root), +16 Node*[] path stack,
// +96 s16 depth, +100 Node*). Nodes: +4 key, +8 value, +12 left, +16 right.
constexpr u32 kGlobalComponents = 0x010C2750;  // hevent6_g
constexpr u32 kRegistry27B8 = 0x010C27B8;
constexpr u32 kRegistry28F0 = 0x010C28F0;
constexpr u32 kRegistry2820 = 0x010C2820;
constexpr u32 kSetRegistry = 0x010C2888;  // the effect set registry (hevent6_a)

// (Tree*, key, u32* out, Node* root) -> found (r3 is not read) and
// (Tree*, Node** slot, key, u8* changed): per-instance template copies.
constexpr u32 Tree_Find78E260 = 0x0078E260, Map_Erase78D670 = 0x0078D670;
constexpr u32 TreeFind_0078D300 = 0x0078D300, Avl_Remove_0078C710 = 0x0078C710;
constexpr u32 TreeFind_0078C110 = 0x0078C110, Avl_Remove_0078B520 = 0x0078B520;
constexpr u32 TreeFind_0078B1B0 = 0x0078B1B0, Avl_Remove_0078A5C0 = 0x0078A5C0;

// One registry: until a pass finds the tree empty, walk to its leftmost
// node (pushing the path on the iterator stack, +100 = the node), look its
// key up (out = the value, unused) and, when found, erase it and decrement
// the count. A failed lookup just starts the next pass. The push is
// unchecked: a path deeper than 20 runs into the +96 depth (as in the
// original).
void DrainRegistry(PPUContext& ctx, u32 m, u32 find, u32 erase, u32 out, u32 changed) {
    for (;;) {
        const u32 tree = load<u32>(m + 12);
        store<u16>(m + 96, 0);
        u32 n = load<u32>(tree);
        u32 node = 0;
        u32 key = 0;
        if (n != 0) {
            for (;;) {
                node = n;
                const u32 l = load<u32>(node + 12);
                if (l == 0) break;
                const s32 d = static_cast<s16>(load<u16>(m + 96));
                store<u16>(m + 96, static_cast<u16>(d + 1));
                store<u32>(m + 16 + (static_cast<u32>(d) << 2), n);
                n = l;
            }
            key = load<u32>(node + 4);
        }
        store<u32>(m + 100, node);
        if (node == 0) return;
        store<u8>(changed, 0);
        if (static_cast<u32>(call(ctx, find, {m, key, out, load<u32>(m)})) == 0) continue;
        call(ctx, erase, {m, m, key, changed});
        store<u32>(m + 4, load<u32>(m + 4) - 1);
    }
}
}  // namespace

// u32 Effects_ResetRegistries4D05E8() (called through a pointer): when the
// deferred list is built (the doubleword at 0x010C2960 non-zero), clears it
// (PtrVector_Clear, then Nop on it) and zeroes that doubleword; then drains
// the registries 0x010C2750, 0x010C27B8, 0x010C28F0, 0x010C2820 and
// 0x010C2888 in that order. Returns 0.
DECOMP(004D05E8, Effects_ResetRegistries4D05E8) {
    if (load<u64>(kDeferredReady) != 0) {
        call(ctx, b35fn::PtrVector_Clear, {kDeferred});
        call(ctx, b35fn::Nop, {kDeferred});
        store<u64>(kDeferredReady, 0);
    }
    // sp+112..+143 in the original: the lookup outputs at +16/+12/+8/+4/+0
    // and the erase flags at +24/+23/+22/+21/+20
    Stack frame(ctx, 32);
    const u32 s = frame.addr();
    DrainRegistry(ctx, kGlobalComponents, Tree_Find78E260, Map_Erase78D670, s + 16, s + 24);
    DrainRegistry(ctx, kRegistry27B8, Tree_Find78E260, Map_Erase78D670, s + 12, s + 23);
    DrainRegistry(ctx, kRegistry28F0, TreeFind_0078D300, Avl_Remove_0078C710, s + 8, s + 22);
    DrainRegistry(ctx, kRegistry2820, TreeFind_0078C110, Avl_Remove_0078B520, s + 4, s + 21);
    DrainRegistry(ctx, kSetRegistry, TreeFind_0078B1B0, Avl_Remove_0078A5C0, s + 0, s + 20);
    ret(ctx, 0u);
}
