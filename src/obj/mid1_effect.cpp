// Game code, medium sweep part 1: creating and deleting effects by def key,
// through the list of live effects at 0x010C2968 (a PtrVector, built on
// first use).
#include "mid1.h"

using namespace sly4;
using namespace sly4::mid1;

namespace {
// The function-local static: the effect list built (capacity 16) the first
// time it is needed.
void EffectListEnsure(PPUContext& ctx) {
    if (load<u8>(kEffectListInit) != 0) return;
    PtrVecNew(ctx, kEffectList, 16);
    store<u8>(kEffectListInit, 1);
}
}  // namespace

// Effect* Effect_New(u32 key): looks key up in the def tree of the default
// effect set (Set_Find(0): +16 the root of a BST {+4 key, +8 def, +12 left
// (keys below), +16 right}; the path walked is kept in +20[] with its length
// in the s16 +100, the last node visited in +104, and the def found in
// kEffectLastDef). When a def is found, a new 140-byte Effect is built on it
// and pushed onto the effect list (0 is pushed when the allocation fails).
// Returns the effect, or 0 when the key or its def is missing.
DECOMP(004CFB24, Effect_New) {
    const u32 key = arg<u32>(ctx, 0);
    const u32 s = static_cast<u32>(call(ctx, m1fn::Set_Find, {0}));
    u32 n = load<u32>(load<u32>(s + 16));
    if (n == 0) {
        store<u32>(s + 104, 0);
        store<u32>(kEffectLastDef, 0);
        ret(ctx, 0u);
        return;
    }
    store<u16>(s + 100, 0);
    for (;;) {
        const u32 k = load<u32>(n + 4);
        if (k == key) break;
        const u32 next = load<u32>(n + (k > key ? 12 : 16));
        if (next == 0) {
            store<u32>(s + 104, n);  // the would-be parent
            ret(ctx, 0u);
            return;
        }
        const s32 depth = static_cast<s16>(load<u16>(s + 100));
        store<u16>(s + 100, static_cast<u16>(depth + 1));
        store<u32>(s + 20 + 4 * static_cast<u32>(depth), n);  // unchecked, as the original
        n = next;
    }
    store<u32>(s + 104, n);
    store<u32>(kEffectLastDef, load<u32>(n + 8));
    const u32 found = load<u32>(s + 104);
    if (found == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 def = load<u32>(found + 8);
    if (def == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {140}));
    u32 e = 0;
    if (p != 0) {
        call(ctx, m1fn::Effect_Construct, {p, def});
        e = p;
    }
    EffectListEnsure(ctx);
    PtrVecPush(ctx, kEffectList, e);
    ret(ctx, e);
}

// void Effect_Delete(Effect* e): when e is on the effect list, it is
// swapped with the last entry, destroyed and freed (140 bytes) when set,
// and the last entry popped. The list is re-read after the destructor,
// which may delete other effects.
DECOMP(004D0280, Effect_Delete) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 flag = load<u8>(kEffectListInit);
    EffectListEnsure(ctx);
    u32 i = 0;
    for (;; ++i) {
        if (!(i < load<u16>(kEffectList + 8))) {
            ret(ctx, flag != 0 ? flag : 1u);  // r3 = the guard as the original holds it
            return;
        }
        if (load<u32>(load<u32>(kEffectList) + 4 * i) == e) break;
    }
    const u32 last = load<u32>(load<u32>(kEffectList) + 4 * (load<u16>(kEffectList + 8) - 1u));
    store<u32>(load<u32>(kEffectList) + ((i << 2) & 0xFFFFFFFCu), last);
    store<u32>(load<u32>(kEffectList) + 4 * (load<u16>(kEffectList + 8) - 1u), e);
    if (e != 0) {
        call(ctx, m1fn::Effect_Destruct, {e});
        call(ctx, m1fn::SmallFree, {e, 140});
        EffectListEnsure(ctx);
    }
    // Pop the last entry; with the count already 0 (the destructor emptied
    // the list), the count wraps and data[-1] takes data[0xFFFF].
    const u32 c = load<u16>(kEffectList + 8) - 1u;
    store<u16>(kEffectList + 8, static_cast<u16>(c));
    if (c != (c & 0xFFFFu)) {
        const u32 data = load<u32>(kEffectList);
        store<u32>(data + (c << 2), load<u32>(data + ((c << 2) & 0x3FFFCu)));
    }
    ret(ctx, static_cast<s32>(c));
}
