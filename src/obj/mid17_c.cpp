// Game code, medium sweep part 17: effect-state switches.
#include "mid17.h"

using namespace sly4;
using namespace sly4::mid17;

namespace {
constexpr u32 Obj_EffectHash356_000E0B64 = 0x000E0B64u;  // (Obj*, u32 hash, 1)

// o->1376 = h; Obj_EffectHash356_000E0B64(o, h, 1).
inline void SetFx(PPUContext& ctx, u32 o, u32 off, u32 h) {
    store<u32>(o + off, h);
    call(ctx, Obj_EffectHash356_000E0B64, {o, h, 1});
}
// The shared tail: when want differs from cur (= o->off on entry), cur's
// transition plays first (0xCE2E6B3D -> 0x9B04BE36, 0x594B024B ->
// 0x273F0E72, 0xE15D38C8 -> 0x4090D96F, 0xAB6ADA7A -> 0x32AB4EB3), then
// want is stored and played unless o->off already holds it (r3 = o->off).
inline void FxSwitch(PPUContext& ctx, u32 o, u32 off, u32 cur, u32 want) {
    if (want != cur) {
        if (cur == 0xCE2E6B3Du) SetFx(ctx, o, off, 0x9B04BE36u);
        else if (cur == 0x594B024Bu) SetFx(ctx, o, off, 0x273F0E72u);
        else if (cur == 0xE15D38C8u) SetFx(ctx, o, off, 0x4090D96Fu);
        else if (cur == 0xAB6ADA7Au) SetFx(ctx, o, off, 0x32AB4EB3u);
    }
    const u32 c = load<u32>(o + off);
    if (c == want) {
        ret(ctx, c);
        return;
    }
    SetFx(ctx, o, off, want);
}

// The same switch for an Effect* at o->eoff with the current hash at
// o->coff, played with Effect_SetHash4CF988(e, h); a fifth transition
// 0x7D22E5CD -> 0x7A29A6B4. The transitions use e0 (the effect read on
// entry) when reload is false, else re-read o->eoff and skip a null one.
// The final play re-reads o->eoff and skips a null one (r3 = o->coff as
// read before the store).
inline void FxSwitchE(PPUContext& ctx, u32 o, u32 eoff, u32 coff, u32 cur, u32 want, u32 e0, bool reload) {
    auto play = [&](u32 h) {
        const u32 e = reload ? load<u32>(o + eoff) : e0;
        store<u32>(o + coff, h);
        if (e != 0) call(ctx, 0x004CF988u, {e, h});  // Effect_SetHash4CF988
        else ret(ctx, 0);
    };
    if (want != cur) {
        if (cur == 0xCE2E6B3Du) play(0x9B04BE36u);
        else if (cur == 0x594B024Bu) play(0x273F0E72u);
        else if (cur == 0xE15D38C8u) play(0x4090D96Fu);
        else if (cur == 0xAB6ADA7Au) play(0x32AB4EB3u);
        else if (cur == 0x7D22E5CDu) play(0x7A29A6B4u);
    }
    const u32 c = load<u32>(o + coff);
    if (c == want) {
        ret(ctx, c);
        return;
    }
    store<u32>(o + coff, want);
    const u32 e = load<u32>(o + eoff);
    if (e == 0) {
        ret(ctx, c);
        return;
    }
    call(ctx, 0x004CF988u, {e, want});
}

// "Busy": o->612 = p and p->28 = q are set and one of the (s8) q->32
// entries of p->72 (stride 224; the count compared unsigned) has a non-zero
// +36. A null p or q counts as busy.
inline bool Busy612(u32 o) {
    const u32 p = load<u32>(o + 612);
    if (p == 0) return true;
    const u32 q = load<u32>(p + 28);
    if (q == 0) return true;
    const u32 n = static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(q + 32))));
    u32 e = load<u32>(p + 72);
    for (u32 i = 0; i < n; ++i, e += 224)
        if (load<u32>(e + 36) != 0) return true;
    return false;
}

// The wanted hash from Busy612 and o->972 / o->980 against k.
inline u32 StanceWant(u32 o, f64 k) {
    if (Busy612(o)) return Lf(o + 972) != k ? 0xCE2E6B3Du : Lf(o + 980) == k ? 0x002237D4u : 0x594B024Bu;
    return Lf(o + 972) != k ? 0xE15D38C8u : Lf(o + 980) == k ? 0x0D94B291u : 0xAB6ADA7Au;
}

// The table pick of 248BB0 / 248958 (see there).
inline u32 TableWant(u32 o, u32 sp, u32 s) {
    if (load<u8>(o + 500) != 0 || load<u8>(o + 501) != 0) {
        store<u32>(sp + 112, 0x002237D4u);
        store<u32>(sp + 116, 0xCE2E6B3Du);
        store<u32>(sp + 120, 0x594B024Bu);
        return load<u32>(sp + 112 + 4 * s);  // unchecked index
    }
    store<u32>(sp + 124, 0xE15D38C8u);
    store<u32>(sp + 128, 0xE15D38C8u);
    store<u32>(sp + 132, 0xAB6ADA7Au);
    return load<u32>(sp + 124 + 4 * s);  // unchecked index
}
}  // namespace

// u32 Obj_UpdateStanceFx248BB0(Obj* o)
// (called by 0024BA28, Obj_SetActive_0024DC98) Picks the wanted effect
// hash for o->1217 (u8 index) from a 3-entry table built on the stack:
// {0x002237D4, 0xCE2E6B3D, 0x594B024B} at sp+112 when o->500 or o->501 is
// set, else {0xE15D38C8, 0xE15D38C8, 0xAB6ADA7A} at sp+124 (the index is
// not range-checked: larger values read on up the stack). When it differs
// from the current o->1376, the current one first plays its transition
// (0xCE2E6B3D -> 0x9B04BE36, 0x594B024B -> 0x273F0E72, 0xE15D38C8 ->
// 0x4090D96F, 0xAB6ADA7A -> 0x32AB4EB3; each stored to o->1376 and played
// with Obj_EffectHash356_000E0B64(o, h, 1)), then the wanted one is
// stored and played unless it is already current. Returns the call's r3,
// or o->1376 when nothing was played last.
DECOMP(00248BB0, Obj_UpdateStanceFx248BB0) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 160);
    const u32 sp = frame.addr();
    const u32 cur = load<u32>(o + 1376);
    const u32 s = load<u8>(o + 1217);
    const u32 want = TableWant(o, sp, s);
    FxSwitch(ctx, o, 1376, cur, want);
}

// u32 Obj_UpdateStanceFx2360A8(Obj* o)
// (called by 00238F78) The same effect switch on o->1012: "busy" when
// o->612 = p and p->28 = q are set and one of the (s8) q->32 entries of
// p->72 (stride 224; the count compared unsigned) has a non-zero +36.
// With K = *(0x002360A0): busy -> 0xCE2E6B3D if o->972 != K, else
// 0x002237D4 if o->980 == K, else 0x594B024B; not busy -> 0xE15D38C8 if
// o->972 != K, else 0x0D94B291 if o->980 == K, else 0xAB6ADA7A.
DECOMP(002360A8, Obj_UpdateStanceFx2360A8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 cur = load<u32>(o + 1012);
    FxSwitch(ctx, o, 1012, cur, StanceWant(o, Lf(0x002360A0u)));
}

// u32 Obj_UpdateEffectFx248958(Obj* o)
// (called by 0024BA28) The Effect version for e = o->1352 (nothing when
// null; r3 = 0), current hash o->1356: the wanted hash is 0x7D22E5CD when
// o->1216 is 0, else the table pick of Obj_UpdateStanceFx248BB0 by o->1217.
// Transitions play on the effect read on entry.
DECOMP(00248958, Obj_UpdateEffectFx248958) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 160);
    const u32 e = load<u32>(o + 1352);
    const u32 s = load<u8>(o + 1217);
    if (e == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 cur = load<u32>(o + 1356);
    const u32 want = load<u8>(o + 1216) == 0 ? 0x7D22E5CDu : TableWant(o, frame.addr(), s);
    FxSwitchE(ctx, o, 1352, 1356, cur, want, e, false);
}

// u32 Obj_UpdateEffectFx235DC0(Obj* o)
// (called by 00238F78) The Effect version for o->768 (re-read and
// null-checked for every play), current hash o->772: with K =
// *(0x00235DBC), the wanted hash is 0x7D22E5CD when o->1004 == K, else the
// pick of Obj_UpdateStanceFx2360A8 against K.
DECOMP(00235DC0, Obj_UpdateEffectFx235DC0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 k = Lf(0x00235DBCu);
    const u32 cur = load<u32>(o + 772);
    const u32 want = Lf(o + 1004) == k ? 0x7D22E5CDu : StanceWant(o, k);
    FxSwitchE(ctx, o, 768, 772, cur, want, 0, true);
}

namespace {
// One pass of the tree clear in Trees_Clear4F30AC: until the tree t is
// empty, walk from the root (*(t->12)) down the +12 links to the leftmost
// node, pushing each node passed onto the s16-counted stack t->16[] (count
// t->96, reset to 0 per walk), set t->100 = that node, look up its key
// (+4) with find(t, key, &out, t->0) and, when found, remove(t, t, key,
// &flag) (flag byte 0) and --t->4. The final (empty) pass sets t->100 = 0.
inline void ClearTree(PPUContext& ctx, u32 t, u32 find, u32 remove, u32 out, u32 flag) {
    for (;;) {
        store<u16>(t + 96, 0);
        u32 n = load<u32>(load<u32>(t + 12));
        if (n == 0) {
            store<u32>(t + 100, 0);  // the empty exit passes the t->100 store too
            return;
        }
        u32 last;
        do {
            last = n;
            const u32 l = load<u32>(n + 12);
            if (l != 0) {
                const s16 i = static_cast<s16>(load<u16>(t + 96));
                store<u16>(t + 96, static_cast<u16>(i + 1));
                store<u32>(t + 16 + 4 * static_cast<s32>(i), n);
            }
            n = l;
        } while (n != 0);
        const u32 key = load<u32>(last + 4);
        store<u32>(t + 100, last);
        store<u8>(flag, 0);
        if (static_cast<u32>(call(ctx, find, {t, key, out, load<u32>(t)})) == 0) continue;
        call(ctx, remove, {t, t, key, flag});
        store<u32>(t + 4, load<u32>(t + 4) - 1);
    }
}
}  // namespace

// u32 Trees_Clear4F30AC()
// (no direct callers: called through a pointer) Empties the two AVL trees at 0x010C2CAC
// (TreeFind_00798DF8 / Avl_Remove_00798208) and 0x010C2BC0
// (TreeFind_00797C08 / Avl_Remove_00797018) one leftmost node at a time.
// Returns 0.
DECOMP(004F30AC, Trees_Clear4F30AC) {
    Stack frame(ctx, 160);
    const u32 sp = frame.addr();
    ClearTree(ctx, 0x010C2CACu, 0x00798DF8u, 0x00798208u, sp + 116, sp + 121);
    ClearTree(ctx, 0x010C2BC0u, 0x00797C08u, 0x00797018u, sp + 112, sp + 120);
    ret(ctx, 0);
}

// u32 Obj_Destroy70894(Obj* o)
// (called by Delete_00070B00) Destructor body: o->0 = the vtable
// 0x00B0F3B0, then empties the five AVL trees at o + 440, 332, 224, 116 and
// 12 (as Trees_Clear4F30AC, with TreeFind_0073AF5C / Avl_Remove 0x0073A36C)
// and runs VtB11B20_DtorTrivial_004BB41C(o); returns its r3.
DECOMP(00070894, Obj_Destroy70894) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 192);
    const u32 sp = frame.addr();
    store<u32>(o, 0x00B0F3B0u);
    for (u32 k = 0; k < 4; ++k) ClearTree(ctx, o + 440 - 108 * k, 0x0073AF5Cu, 0x0073A36Cu, sp + 116, sp + 121);
    ClearTree(ctx, o + 12, 0x0073AF5Cu, 0x0073A36Cu, sp + 112, sp + 120);
    call(ctx, 0x004BB41Cu, {o});  // VtB11B20_DtorTrivial_004BB41C
}
