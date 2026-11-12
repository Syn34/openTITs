// Game code, medium sweep part 33: an area object's enter / leave actions.
//
// Area layout as these use it: +344 (u16 +352 the team), +428 the effect
// component set, +436 bit 0 "set child mode on enter", +444 / +448 the
// enter delay range, +452 / +456 the leave delay range, +460 the record
// key, +464 the f32 delay picked, +468 bit 31 entered once / bit 30 left
// once (the first enter / leave also sets a one-shot effect hash).
// The entering object x: +160 a refcounted handle (u16 count +4), +164
// a word, +172 bit 0x40000 "has typed children".
#include "mid33.h"

using namespace sly4;
using namespace sly4::mid33;

namespace {
using namespace m33fn;
constexpr u32 RecInit33 = 0x0049E8A8u;    // Obj_Clear68Set_0049E8A8(rec, key)
constexpr u32 RecUpdate33 = 0x002D98DCu;  // Rec_Update_002D98DC(table, rec)
constexpr u32 kRecGlobal33 = 0x00F64C20u;  // global whose +40 is the record table

// The holder ("area link" components, type 5) of the set's component h:
// its +28 handle's +280 when that handle's +276 is 0x02235C33, else 0.
u32 AreaHeld33(PPUContext& ctx, u32 set, u32 h) {
    const u32 c = static_cast<u32>(call(ctx, FindComp33, {set, h}));
    if (c == 0) return 0;
    if (static_cast<s32>(VCall(ctx, c, 32, {c})) != 5) return 0;
    const u32 p = load<u32>(c + 28);
    if (p == 0) return 0;
    return load<u32>(p + 276) == 0x02235C33u ? load<u32>(p + 280) : 0;
}

// When component h of o->+428 is a type 5 link: release its handle +28
// (slot 8) and clear +28 / +36.
void AreaUnlink33(PPUContext& ctx, u32 o, u32 h) {
    const u32 set = load<u32>(o + 428);
    if (set == 0) return;
    const u32 c = static_cast<u32>(call(ctx, FindComp33, {set, h}));
    if (c == 0 || static_cast<s32>(VCall(ctx, c, 32, {c})) != 5) return;
    const u32 p = load<u32>(c + 28);
    if (p != 0) VCall(ctx, p, 8, {p});
    store<u32>(c + 28, 0);
    store<u32>(c + 36, 0);
}

// The shared middle part of both actions (o->+428 non-zero on entry):
//  - component hClear: unlinked when it currently holds x;
//  - component hSet (type 5): +28 = x->+160 (old handle released, new one's
//    u16 +4 count incremented), +36 = x->+164 (both 0 without x);
//  - component 0xA212C671 (type 2): +20 = 1 when x is the world's
//    +0x1280F0, 2 when +0x1280F4, else 0; 0 when the world's +0x1280F4 is 0;
//  - component 0x1BE4F82D (type 2): +20 = the team u16 o->+352.
// The last two steps are skipped when +428 has become 0.
void AreaLinks33(PPUContext& ctx, u32 o, u32 x, u32 hClear, u32 hSet) {
    const u32 held = AreaHeld33(ctx, load<u32>(o + 428), hClear);
    if (x == held) AreaUnlink33(ctx, o, hClear);
    u32 set = load<u32>(o + 428);
    if (set != 0) {
        const u32 c = static_cast<u32>(call(ctx, FindComp33, {set, hSet}));
        if (c != 0 && static_cast<s32>(VCall(ctx, c, 32, {c})) == 5) {
            const u32 hnd = x != 0 ? load<u32>(x + 160) : 0;
            const u32 old = load<u32>(c + 28);
            if (old != 0) VCall(ctx, old, 8, {old});
            store<u32>(c + 28, hnd);
            if (hnd != 0) store<u16>(hnd + 4, static_cast<u16>(load<u16>(hnd + 4) + 1));
            store<u32>(c + 36, x != 0 ? load<u32>(x + 164) : 0);
        }
    }
    set = load<u32>(o + 428);
    if (set == 0) return;
    const u32 t = static_cast<u32>(call(ctx, FindComp33, {set, 0xA212C671u}));
    if (t != 0 && static_cast<s32>(VCall(ctx, t, 32, {t})) == 2) {
        const u32 w = load<u32>(kWorld33);
        u32 v = 1;
        if (x != load<u32>(w + 0x1280F0u)) v = x == load<u32>(w + 0x1280F4u) ? 2 : 0;
        store<u32>(t + 20, v);
        if (load<u32>(load<u32>(kWorld33) + 0x1280F4u) == 0) store<u32>(t + 20, 0);
    }
    const u32 m = static_cast<u32>(call(ctx, FindComp33, {load<u32>(o + 428), 0x1BE4F82Du}));
    if (m != 0 && static_cast<s32>(VCall(ctx, m, 32, {m})) == 2) store<u32>(m + 20, load<u16>(o + 352));
}

// The delay picked from [lo, hi]: lo + (hi - lo) * (rand * k), k the f32
// at kAddr (1 / 65535); stored to o->+464.
void AreaDelay33(PPUContext& ctx, u32 o, u32 lo, u32 kAddr, bool enter) {
    const f64 a = Lf(o + lo);
    const f64 d = Sgl28(Lf(o + lo + 4) - a);
    const f64 k = Lf(kAddr);
    const f64 r = Sgl28(static_cast<f64>(static_cast<s64>(RandStep33())));
    const f64 v = Fmadds(d, Fmuls31(r, k), a);
    if (!enter) store<u32>(o + 468, load<u32>(o + 468) | 0x40000000u);
    store<f32>(o + 464, static_cast<f32>(v));
    ctx.fpr[1] = v;  // f1 as left
}
}  // namespace

// Area enter action (o, x): records o->+460 in the global record table
// (u8 +72 = 13, +0 = 1), links x into the area's effect components (unlink
// 0x1ABD6576 if it held x, link 0x2C73353E), one-shot hash 0xD126CD47 then
// 0xDC175839, picks the enter delay, puts x's type 23 children whose mode
// ((+24 >> 25) & 31) is 1 into mode 2 when +436 bit 0 and x's bit 0x40000
// are set, and sets +468 bit 31.
DECOMP(0021F4E8, Area_OnEnter_0021F4E8) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    Stack frame(ctx, 240);
    const u32 rec = frame.addr() + 120;
    const u32 key = load<u32>(o + 460);
    if (key != 0) {
        const u32 tab = load<u32>(load<u32>(kRecGlobal33) + 40);
        if (tab != 0) {
            call(ctx, RecInit33, {rec, key});
            store<u8>(rec + 72, 13);
            store<u32>(rec, 1);
            call(ctx, RecUpdate33, {tab, rec});
        }
    }
    if (load<u32>(o + 428) != 0) {
        AreaLinks33(ctx, o, x, 0x1ABD6576u, 0x2C73353Eu);
        if ((load<u32>(o + 468) & 0x80000000u) == 0) call(ctx, SetHash33, {load<u32>(o + 428), 0xD126CD47u});
        call(ctx, SetHash33, {load<u32>(o + 428), 0xDC175839u});
    }
    AreaDelay33(ctx, o, 444, 0x0021F4E0u, true);
    if ((load<u32>(o + 436) & 1) != 0 && x != 0 && (load<u32>(x + 172) & 0x40000u) != 0) {
        u32 c = static_cast<u32>(call(ctx, ObjFindComp33, {x, 23, 0}));
        while (c != 0) {
            if (((load<u32>(c + 24) >> 25) & 31) == 1) call(ctx, SetField20_33, {c, 2});
            c = static_cast<u32>(call(ctx, ObjFindComp33, {x, 23, c}));
        }
    }
    const u32 f = load<u32>(o + 468) | 0x80000000u;
    store<u32>(o + 468, f);
    ctx.gpr[3] = 0xFFFFFFFF00000000ull | f;  // r3 as left (lis 0x8000 sign-extends)
}

// Area leave action (o, x): the mirror of the enter action (unlink
// 0x2C73353E if it held x, link 0x1ABD6576), one-shot hash 0x0F0164AF then
// 0x8B3AB37D, the leave delay, and +468 bit 30.
DECOMP(0021FB38, Area_OnLeave_0021FB38) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    if (load<u32>(o + 428) != 0) {
        AreaLinks33(ctx, o, x, 0x2C73353Eu, 0x1ABD6576u);
        if ((load<u32>(o + 468) & 0x40000000u) == 0) call(ctx, SetHash33, {load<u32>(o + 428), 0x0F0164AFu});
        call(ctx, SetHash33, {load<u32>(o + 428), 0x8B3AB37Du});
    }
    AreaDelay33(ctx, o, 452, 0x0021FB34u, false);
    ctx.gpr[3] = load<u32>(o + 468);  // r3 as left: the new flags word
}

namespace {
constexpr u32 AllIdle33 = 0x00235D48u;  // Obj_AllIdle_00235D48(o) -> bool
}  // namespace

// Area interaction kind (-, x, u32* out) -> bool: 0 without x. A player x
// (+192 bit 0x8) with a body (slot 64) reports by its state (slot 32 with
// 0; layer 1 = slot 32 with 1): 0 -> 8 (layer 1 == 10) / 1; 1 -> 4
// (layer 1 == 2) / 2; 3 -> 1 (layer 1 == 11) / 4; 4..6 -> 1; 7 -> 4;
// others 0. Otherwise x with +200 bit 0x800 reports 2 when
// Obj_AllIdle_00235D48(x), else 1; with +200 bit 0x400 reports 1. The
// result is 1 when *out was written.
DECOMP(0021612C, Area_InteractKind_0021612C) {
    const u32 x = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    if (x == 0) return ret(ctx, 0u);
    if ((load<u32>(x + 192) & 8u) != 0 && static_cast<u32>(VCall(ctx, x, 64, {x})) != 0) {
        const s32 s = static_cast<s32>(VCall(ctx, x, 32, {x, 0}));
        u32 v = 0;
        if (s == 0) v = static_cast<s32>(VCall(ctx, x, 32, {x, 1})) == 10 ? 8 : 1;
        else if (s == 1) v = static_cast<s32>(VCall(ctx, x, 32, {x, 1})) == 2 ? 4 : 2;
        else if (s == 3) v = static_cast<s32>(VCall(ctx, x, 32, {x, 1})) == 11 ? 1 : 4;
        else if (s >= 4 && s <= 6) v = 1;
        else if (s == 7) v = 4;
        store<u32>(out, v);
        return ret(ctx, 1u);
    }
    if ((load<u32>(x + 200) & 0x800u) != 0) {
        const u32 v = call(ctx, AllIdle33, {x}) != 0 ? 2 : 1;
        store<u32>(out, v);
        return ret(ctx, 1u);
    }
    if ((load<u32>(x + 200) & 0x400u) != 0) {
        store<u32>(out, 1);
        return ret(ctx, 1u);
    }
    ret(ctx, 0u);
}
