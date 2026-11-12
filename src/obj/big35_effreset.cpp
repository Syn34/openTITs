// Game code, large-function sweep part 35: emptying the effect system's
// deferred list and its five keyed registries.
#include "native/objtypes.h"

namespace sly4::obj {

using RegNode = AvlNode<be_u32, be_u32>;
// A registry: an AVL tree {+0 root, +4 s32 count} with an iterator at +12
// (+12 the tree's root slot, +16 the path stack, +96 s16 depth, +100 the
// node).
struct EffectRegistry {
    gp<RegNode> root;             // +0
    be_s32 count;                 // +4
    pad<4> _8;
    gp<gp<RegNode>> iter_tree;    // +12: the root slot the iterator walks
    be_u32 path[20];              // +16: Node* stack
    be_s16 depth;                 // +96
    pad<2> _98;
    gp<RegNode> cur;              // +100
};
static_assert(offsetof(EffectRegistry, depth) == 96 && offsetof(EffectRegistry, cur) == 100);

namespace {
constexpr u32 kDeferredReady = 0x010C2960;  // u8 flag (hevent20_f), read and cleared here as a doubleword
constexpr u32 kDeferred = 0x010C2968;       // PtrVector of the deferred effects
constexpr u32 PtrVector_Clear = 0x0054EC1C;  // (vec)
constexpr u32 Nop = 0x0054EC18;              // (x): returns

constexpr u32 kGlobalComponents = 0x010C2750;  // hevent6_g
constexpr u32 kRegistry27B8 = 0x010C27B8;
constexpr u32 kRegistry28F0 = 0x010C28F0;
constexpr u32 kRegistry2820 = 0x010C2820;
constexpr u32 kSetRegistry = 0x010C2888;  // the effect set registry (hevent6_a)

// (Tree*, key, u32* out, Node* root) -> found and (Tree*, Node** slot,
// key, u8* changed): per-instance template copies.
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
void DrainRegistry(u32 mea, u32 find, u32 erase, be_u32* out, u8* changed) {
    EffectRegistry* m = host<EffectRegistry>(mea);
    for (;;) {
        gp<RegNode>* tree = m->iter_tree;
        m->depth = 0;
        RegNode* n = *tree;
        RegNode* node = nullptr;
        u32 key = 0;
        if (n != nullptr) {
            for (;;) {
                node = n;
                RegNode* l = node->left;
                if (l == nullptr) break;
                const s32 d = m->depth;
                m->depth = static_cast<s16>(d + 1);
                gm<be_u32>(mea + 16 + (static_cast<u32>(d) << 2)) = guest(n);
                n = l;
            }
            key = node->key;
        }
        m->cur = node;
        if (node == nullptr) return;
        *changed = 0;
        if (call<u32>(find, m, key, out, m->root.get()) == 0) continue;
        call(erase, m, m, key, changed);
        m->count = m->count - 1;
    }
}
}  // namespace

// u32 Effects_ResetRegistries4D05E8() (called through a pointer): when the
// deferred list is built (the doubleword at 0x010C2960 non-zero), clears it
// (PtrVector_Clear, then Nop on it) and zeroes that doubleword; then drains
// the registries 0x010C2750, 0x010C27B8, 0x010C28F0, 0x010C2820 and
// 0x010C2888 in that order. Returns 0.
u32 Effects_ResetRegistries4D05E8() {
    be_u64& ready = global<be_u64>(kDeferredReady);
    if (ready != 0) {
        call(PtrVector_Clear, kDeferred);
        call(Nop, kDeferred);
        ready = 0;
    }
    // the lookup outputs at +16/+12/+8/+4/+0 and the erase flags at
    // +24/+23/+22/+21/+20 of a stack block
    GuestFrame f(32);
    auto out = [&](u32 off) { return f.as<be_u32>(off); };
    auto flag = [&](u32 off) { return f.as<u8>(off); };
    DrainRegistry(kGlobalComponents, Tree_Find78E260, Map_Erase78D670, out(16), flag(24));
    DrainRegistry(kRegistry27B8, Tree_Find78E260, Map_Erase78D670, out(12), flag(23));
    DrainRegistry(kRegistry28F0, TreeFind_0078D300, Avl_Remove_0078C710, out(8), flag(22));
    DrainRegistry(kRegistry2820, TreeFind_0078C110, Avl_Remove_0078B520, out(4), flag(21));
    DrainRegistry(kSetRegistry, TreeFind_0078B1B0, Avl_Remove_0078A5C0, out(0), flag(20));
    return 0;
}

}  // namespace sly4::obj

NATIVE(004D05E8, sly4::obj::Effects_ResetRegistries4D05E8)
