// Game code, large-function sweep part 23: a gesture list item's step
// (callee of Obj_GesturePick41D158).
#include "big23.h"

namespace sly4::obj {

namespace {
constexpr u32 kItemK = 0x0041B1E8u;        // f32 0.4 (rate), 1.0, 0.25, 0.0, 0.1 (state 3's time limit)
constexpr u32 kItemClip = 0x42350C50u;     // the clip restarted when an item fires
constexpr u32 kItemComp = 0x87208BC1u;     // the effect component given the object's rows
constexpr u32 kItemFxHash = 0xA105407Bu;   // Effect_SetHash's key
constexpr s32 kItemDone = 4;
constexpr u32 Anim_Restart = 0x002775A8u;          // (an, key, a, b; f, f, c, d; f, f, e, g)
constexpr u32 Effect_FindComponent = 0x004BB4B4u;  // (Effect*, u32 hash) -> component or 0

// The gesture list: +88 its def (+56 the total), +460 the count, u8 +483.
struct GestureList {
    pad<88> _0;
    be_u32 def;      // +88
    pad<368> _92;
    be_s32 count;    // +460
    pad<19> _464;
    u8 third;        // +483
};
static_assert(offsetof(GestureList, count) == 460 && offsetof(GestureList, third) == 483);
// The item's owner: +96 its 4x3 rows; vfn[60] its animator, vfn[272] its effect.
struct ItemOwner {
    pad<96> _0;
    Mtx44 m;         // +96
};

}  // namespace

// A gesture list item (declared in big23.h).
struct GestureItem {
    pad<4> _0;
    be_u32 owner;    // +4: ItemOwner*
    pad<68> _8;
    be_u32 list;     // +76: GestureList*
    be_s32 state;    // +80
    be_f32 timer;    // +84
    be_f32 rate;     // +88
    pad<68> _92;
    be_u32 mode;     // +160
    u8 quiet;        // +164
};
static_assert(offsetof(GestureItem, list) == 76 && offsetof(GestureItem, mode) == 160 && offsetof(GestureItem, quiet) == 164);

namespace {
f64 C(u32 off) { return lf(global<be_f32>(kItemK + off)); }

// The item's state = done, then the owner's (+4) animator (vfn[60])
// restarts kItemClip: Anim_Restart2775A8(an, key, 0, 0; 1.0, 0.25, 0, 0;
// 0.0, 0.0, 0, -1).
void ItemFire(GestureItem* it) {
    it->state = kItemDone;
    const u32 an = vcall<u32>(&gm<u8>(it->owner), 60);
    const f64 a = C(4), b = C(8), z = C(12);
    call(Anim_Restart, an, kItemClip, 0u, 0u, a, b, 0u, 0u, z, z, 0u, -1);
}

// The owner's (+4) vfn[272] effect: its component kItemComp, when that
// one's vfn[32] says 4, gets the owner's 4x3 rows (+96 / +112 / +128 /
// +144, x y z each) at +32 / +48 / +64 / +80; then Effect_SetHash(e,
// kItemFxHash).
void ItemSyncFx(GestureItem* it) {
    ItemOwner& o = gm<ItemOwner>(it->owner);
    const u32 e = vcall<u32>(&o, 272);
    if (e == 0) return;
    const u32 comp = call<u32>(Effect_FindComponent, e, kItemComp);
    if (comp != 0 && vcall<u32>(&gm<u8>(comp), 32) == 4) CopyMtxXYZ(gm<Mtx44>(comp + 32), o.m);
    call(fn::Effect_SetHash, e, kItemFxHash);
}
}  // namespace

// bool Item_GestureStep41B200(Item* it, s32 mode) (the caller reads r3):
// 0 unless it+160 == mode. The timer +84 += +88 * 0.4. State +80: 4
// (done) returns 0; 1 and 2 fire (unless +164 is set) and sync the effect;
// 3 does the same while the timer is below 0.1 (always when +164 is set),
// else returns 0. After a step without +164 the list +76 counts it (+460)
// and sets the u8 +483 when the count crosses a third or two thirds of the
// total (+88 -> +56).
bool Item_GestureStep41B200(GestureItem* it, u32 mode) {
    if (it->mode != mode) return false;
    it->timer = static_cast<f32>(Fmadds(lf(it->rate), C(0), lf(it->timer)));
    const s32 st = it->state;
    if (st == kItemDone) return false;
    bool r = false;
    if (st == 1) {
        if (it->quiet == 0) ItemFire(it);
        ItemSyncFx(it);
        r = true;
    }
    if (it->state == 2) {
        if (it->quiet == 0) ItemFire(it);
        ItemSyncFx(it);
        r = true;
    }
    if (it->state == 3) {
        if (it->quiet != 0) {
            ItemSyncFx(it);
            r = true;
        } else if (lf(it->timer) < C(16)) {
            ItemFire(it);
            ItemSyncFx(it);
            r = true;
        } else {
            r = false;
        }
    }
    if (r && it->quiet == 0) {
        GestureList& list = gm<GestureList>(it->list);
        const s32 n = list.count;
        const u32 def = list.def;
        const s32 n1 = static_cast<s32>(static_cast<u32>(n) + 1);
        list.count = n1;
        const s32 total = gm<be_s32>(def + 56);
        const s32 third = total / 3;
        if (n1 >= third && n < third) list.third = 1;
        const s32 two = static_cast<s32>(static_cast<u32>(total) << 1) / 3;
        if (n1 >= two && n < two) list.third = 1;  // the same byte both times
    }
    return r;
}

}  // namespace sly4::obj

NATIVE(0041B200, sly4::obj::Item_GestureStep41B200)
