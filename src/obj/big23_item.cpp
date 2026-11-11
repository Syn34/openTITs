// Game code, large-function sweep part 23: a gesture list item's step
// (callee of Obj_GesturePick41D158).
#include "big23.h"

using namespace sly4;
using namespace sly4::big23;

namespace {
constexpr u32 kItemK = 0x0041B1E8u;        // f32 0.4 (rate), 1.0, 0.25, 0.0, 0.1 (state 3's time limit)
constexpr u32 kItemClip = 0x42350C50u;     // the clip restarted when an item fires
constexpr u32 kItemComp = 0x87208BC1u;     // the effect component given the object's rows
constexpr u32 kItemFxHash = 0xA105407Bu;   // Effect_SetHash's key
constexpr s32 kItemDone = 4;

namespace ifn {
constexpr u32 Anim_Restart = 0x002775A8u;          // (an, key, ...; f32 x4) (12 arguments)
constexpr u32 Effect_FindComponent = 0x004BB4B4u;  // (Effect*, u32 hash) -> component or 0
constexpr u32 Effect_SetHash = 0x004CF988u;        // (Effect*, u32 hash)
}  // namespace ifn

// The item's state = done, then the owner's (+4) animator (vfn[15]) restarts
// kItemClip: Anim_Restart2775A8(an, key, 0, 0, -1, r8 as the animator call
// left it, 0, 0, the frame's sp+112 / sp+120 (never written), 0, -1; f32
// 1.0, 0.25, 0.0, 0.0).
void ItemFire(PPUContext& ctx, u32 it, u32 sp) {
    store<s32>(it + 80, kItemDone);
    const u32 o = load<u32>(it + 4);
    const u32 an = static_cast<u32>(VCall(ctx, o, 60, {o}));
    const u64 r8 = ctx.gpr[8];
    const u64 s9 = load<u64>(sp), s10 = load<u64>(sp + 8);
    const f64 a = Lf(kItemK + 4), b = Lf(kItemK + 8), z = Lf(kItemK + 12);
    callf(ctx, ifn::Anim_Restart, {an, kItemClip, 0, 0, ~0ull, r8, 0, 0, s9, s10, 0, ~0ull}, {a, b, z, z});
}

// The owner's (+4) vfn[68] effect: its component kItemComp, when that one's
// vfn[8] says 4, gets the owner's 4x3 rows (+96 / +112 / +128 / +144, x y z
// each) at +32 / +48 / +64 / +80; then Effect_SetHash(e, kItemFxHash).
void ItemSyncFx(PPUContext& ctx, u32 it) {
    const u32 o = load<u32>(it + 4);
    const u32 e = static_cast<u32>(VCall(ctx, o, 272, {o}));
    if (e == 0) return;
    const u32 comp = static_cast<u32>(call(ctx, ifn::Effect_FindComponent, {e, kItemComp}));
    if (comp != 0 && static_cast<u32>(VCall(ctx, comp, 32, {comp})) == 4) {
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) Cw(comp + 32 + 16 * r + 4 * k, o + 96 + 16 * r + 4 * k);
    }
    call(ctx, ifn::Effect_SetHash, {e, kItemFxHash});
}
}  // namespace

// bool Item_GestureStep41B200(Item* it, s32 mode): 0 unless it+160 == mode.
// The timer +84 += +88 * 0.4. State +80: 4 (done) returns 0; 1 and 2 fire
// (unless +164 is set) and sync the effect; 3 does the same while the timer
// is below 0.1 (always when +164 is set), else returns 0. After a step
// without +164 the list +76 counts it (+460) and sets the u8 +483 when the
// count crosses a third or two thirds of the total (+88 -> +56).
DECOMP(0041B200, Item_GestureStep41B200) {
    const u32 it = arg<u32>(ctx, 0);
    const u32 mode = arg<u32>(ctx, 1);
    if (load<u32>(it + 160) != mode) {
        ret(ctx, 0u);
        return;
    }
    Stack frame(ctx, 16);
    const u32 sp = frame.addr();
    Sf(it + 84, Fmadds(Lf(it + 88), Lf(kItemK), Lf(it + 84)));
    const s32 st = load<s32>(it + 80);
    if (st == kItemDone) {
        ret(ctx, 0u);
        return;
    }
    u32 r = 0;
    if (st == 1) {
        if (load<u8>(it + 164) == 0) ItemFire(ctx, it, sp);
        ItemSyncFx(ctx, it);
        r = 1;
    }
    if (load<s32>(it + 80) == 2) {
        if (load<u8>(it + 164) == 0) ItemFire(ctx, it, sp);
        ItemSyncFx(ctx, it);
        r = 1;
    }
    if (load<s32>(it + 80) == 3) {
        if (load<u8>(it + 164) != 0) {
            ItemSyncFx(ctx, it);
            r = 1;
        } else if (Lf(it + 84) < Lf(kItemK + 16)) {
            ItemFire(ctx, it, sp);
            ItemSyncFx(ctx, it);
            r = 1;
        } else {
            r = 0;
        }
    }
    if (r != 0 && load<u8>(it + 164) == 0) {
        const u32 list = load<u32>(it + 76);
        const s32 n = load<s32>(list + 460);
        const u32 def = load<u32>(list + 88);
        const s32 n1 = static_cast<s32>(static_cast<u32>(n) + 1);
        store<s32>(list + 460, n1);
        const s32 total = load<s32>(def + 56);
        const s32 third = total / 3;
        if (n1 >= third && n < third) store<u8>(list + 483, 1);
        const s32 two = static_cast<s32>(static_cast<u32>(total) << 1) / 3;
        if (n1 >= two && n < two) store<u8>(list + 483, 1);  // the same byte both times
    }
    ret(ctx, r);
}
