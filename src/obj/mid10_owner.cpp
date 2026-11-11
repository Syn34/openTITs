// Game code, medium sweep part 10: the shared base constructor of the
// Owner_New family (001ADD08) and its effect-def binder.
#include "mid10.h"

using namespace sly4;
using namespace sly4::mid10;

namespace {

constexpr u32 Effect_FindComponent = 0x004BB4B4;  // (Effect*, u32 type hash) -> component or 0
constexpr u32 Thunk_000E40B4 = 0x000E40B4;        // (o, rec): the base constructor
constexpr u32 Node_PushF65B7C = 0x00446984;       // (a, b) -> node
constexpr u32 Owner_BindEffectDefs = 0x001ADB14;
constexpr u32 kSlotKeys = 0x00AD139C;  // the seven slot hashes
constexpr u32 kLastDefA8 = 0x011DE2A8;  // the def the last lookup found

// The inlined def lookup of key in the set of effect def table t
// (t->12->0 the root of a BST {+4 key, +8 def, +12 left (keys below), +16
// right}; the path in +16[] with its s16 length +96, the node found or the
// would-be parent in +100, the def found in kLastDefA8). Returns the def
// (0 when missing).
u32 DefLookupA8(u32 t, u32 key) {
    u32 n = load<u32>(load<u32>(t + 12));
    if (n == 0) {
        store<u32>(t + 100, 0);
        store<u32>(kLastDefA8, 0);
        return 0;
    }
    store<u16>(t + 96, 0);
    for (;;) {
        const u32 k = load<u32>(n + 4);
        if (k == key) break;
        const u32 next = load<u32>(n + (k > key ? 12 : 16));
        if (next == 0) {
            store<u32>(t + 100, n);
            return 0;
        }
        const s32 depth = static_cast<s16>(load<u16>(t + 96));
        store<u16>(t + 96, static_cast<u16>(depth + 1));
        store<u32>(t + 16 + 4 * static_cast<u32>(depth), n);  // unchecked, as the original
        n = next;
    }
    store<u32>(t + 100, n);
    store<u32>(kLastDefA8, load<u32>(n + 8));
    const u32 found = load<u32>(t + 100);
    return found != 0 ? load<u32>(found + 8) : 0;
}

}  // namespace

// u32 Owner_BindEffectDefs_001ADB14(Owner* o)
// Nothing when o->216 is 0. Else: o->536 = the 0xBBBC16FF component of the
// effect o->356 when its vtable slot 32 answers 1; then for each of the
// seven slot hashes (0x00AD139C) while o->356 is set, o->432 + 16 k = the
// def of that hash in the effect's def table (o->356->0->24; 0 when any
// link is missing). Returns 7.
DECOMP(001ADB14, Owner_BindEffectDefs_001ADB14) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 216) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 e = load<u32>(o + 356);
    if (e != 0) {
        const u32 c = static_cast<u32>(call(ctx, Effect_FindComponent, {e, 0xBBBC16FFu}));
        if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 1) store<u32>(o + 536, c);
    }
    for (u32 k = 0; k < 7; ++k) {
        const u32 eff = load<u32>(o + 356);
        if (eff == 0) continue;
        const u32 key = load<u32>(kSlotKeys + 4 * k);
        const u32 inst = load<u32>(eff);
        u32 def = 0;
        if (inst != 0) {
            const u32 t = load<u32>(inst + 24);
            if (t != 0 && DefLookupA8(t, key) != 0) {
                const u32 t2 = load<u32>(inst + 24);
                const u32 n = t2 != 0 ? load<u32>(t2 + 100) : 0;
                def = n != 0 ? load<u32>(n + 8) : 0;
            }
        }
        store<u32>(o + 432 + 16 * k, def);
    }
    ret(ctx, 7u);
}

// u32 Owner_Construct1ADD08(Owner* o, const OwnerRec* rec)
// The base constructor of the Owner_New family: Thunk_000E40B4(o, rec),
// vtables 0x00B159B8 / +380 0x00B15B8C / +384 0x00B15BB4, the seven 16-byte
// slots +424 (+4 / u16 +6 cleared), +536 0, +540 -1, f32 +544 = rec->236,
// +548 0, +552 bits 0 / 1 cleared; then each slot k: +432 0, +424 =
// Node_PushF65B7C(def->304[2k], def->384) (def = rec->32), +428 0, f32 +436
// = rec->208[k] when > the f32 at 0x001ADD04 else def->308[2k];
// Owner_BindEffectDefs; +548 = the index of def->388 among the slot hashes
// (when found); +552 bit 0 set and bit 2 = bit 7 of byte def->396. Returns
// the +552 word.
DECOMP(001ADD08, Owner_Construct1ADD08) {
    const u32 o = arg<u32>(ctx, 0), rec = arg<u32>(ctx, 1);
    call(ctx, Thunk_000E40B4, {o, rec});
    constexpr u32 kVt = 0x00B159B0;
    store<u32>(o, kVt + 8);
    store<u32>(o + 380, kVt + 476);
    store<u32>(o + 384, kVt + 516);
    const u32 f544 = load<u32>(rec + 236);
    for (u32 i = 0; i < 7; ++i) {
        store<u32>(o + 428 + 16 * i, 0);
        store<u16>(o + 430 + 16 * i, 0);
    }
    const u32 w552 = load<u32>(o + 552);
    store<u32>(o + 536, 0);
    store<u32>(o + 540, 0xFFFFFFFFu);
    store<u32>(o + 544, f544);
    store<u32>(o + 548, 0);
    store<u32>(o + 552, w552 & 0x3FFFFFFFu);
    const u32 def = load<u32>(rec + 32);
    const f32 lim = F32(load<u32>(0x001ADD04));
    for (u32 i = 0; i < 7; ++i) {
        const u32 s = o + 16 * i;
        store<u32>(s + 432, 0);
        const u32 n = static_cast<u32>(call(ctx, Node_PushF65B7C, {load<u32>(def + 304 + 8 * i), load<u32>(def + 384)}));
        store<u32>(s + 424, n);
        store<u32>(s + 428, 0);
        u32 v = load<u32>(rec + 208 + 4 * i);
        if (!(F32(v) > lim)) v = load<u32>(def + 308 + 8 * i);
        store<u32>(s + 436, v);
        ctx.fpr[1] = static_cast<f64>(F32(v));
    }
    call(ctx, Owner_BindEffectDefs, {o});
    const u32 w = load<u32>(o + 552);
    const u32 want = load<u32>(def + 388);
    for (u32 j = 0; j < 7; ++j)
        if (load<u32>(kSlotKeys + 4 * j) == want) {
            store<u32>(o + 548, j);
            break;
        }
    u32 r = w | 0x80000000u;
    store<u32>(o + 552, r);
    r = (r & ~0x20000000u) | ((load<u8>(def + 396) & 0x80u) ? 0x20000000u : 0u);
    store<u32>(o + 552, r);
    ret(ctx, r);
}
