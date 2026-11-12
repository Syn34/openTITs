// Game code, large-function sweep part 19: effect slots started from the
// global effect set.
#include "big19.h"

namespace sly4::obj {

namespace {
constexpr u32 kComp27469E = 0x0027469Eu;  // the component hash looked up on a new effect
constexpr u32 kKey9DF = 2527;             // sub-set keys read into +1368 / +1372
constexpr u32 kKey10CBB = 0x00010CBBu;
constexpr u32 kStartTime = 0x002464ECu;   // .text f32 passed to Effect_SetTime
constexpr u32 Effect_FindComponent = 0x004BB4B4; // (Effect*, u32 hash) -> component or 0
constexpr u32 Set_Find = 0x004BCE90;             // (0) -> the global effect set or 0
constexpr u32 Effect_Start = 0x004C753C;         // (Effect*)
constexpr u32 Effect_SetTime = 0x004CF710;       // (Effect*; f32)

// The owner of the effect slots.
struct FxOwner {
    pad<160> _0;
    be_u32 w160;        // +160
    pad<52> _164;
    be_u32 owner;       // +216 (+872 / +876 / +880 the set keys)
    pad<136> _220;
    be_u32 a356;        // +356
    pad<992> _360;
    be_u32 slots[4];    // +1352 / +1356 / +1360 / +1364
    be_u32 sub[2];      // +1368 / +1372
    be_u32 w1376;       // +1376
};
static_assert(offsetof(FxOwner, owner) == 216 && offsetof(FxOwner, a356) == 356 && offsetof(FxOwner, slots) == 1352);
static_assert(offsetof(FxOwner, w1376) == 1376);
constexpr u32 kSlot1352 = 0, kSlot1360 = 2, kSlot1364 = 3;

// The value of a def's sub-set entry `key`: def +24 is the sub-set (root
// **(+12), path +16, count +96, selected +100, global kEffSubSelected);
// when found, the selected node's +8 re-read through def +24 (else 0).
u32 SubValue(u32 def, u32 key) {
    if (def == 0) return 0;
    const u32 s2 = gm<be_u32>(def + 24);
    if (s2 == 0) return 0;
    if (BstSelect(s2, 12, 96, 16, 100, kEffSubSelected, key) == 0) return 0;
    const u32 t = gm<be_u32>(def + 24);
    if (t == 0) return 0;
    const u32 sel = gm<be_u32>(t + 100);
    return sel != 0 ? gm<be_u32>(sel + 8).get() : 0u;
}

// Effect slot k from the set's selected node (+104, its +8 the def):
// nothing without one. New effect (0 when the allocation fails) stored in
// the slot; with `nullcheck` a failed one stops here. Its component
// kComp27469E, when vfunc 32 says 5, has its +28 replaced by o +160 (the
// old one's vfunc 8 called first, the new one's u16 +4 bumped). Returns
// false when nothing was made (or the null check failed).
bool MakeSlot(FxOwner* o, u32 set, u32 k, bool nullcheck) {
    const u32 node = gm<be_u32>(set + 104);
    if (node == 0) return false;
    const u32 def = gm<be_u32>(node + 8);
    if (def == 0) return false;
    const u32 e = NewEffect140(def);
    o->slots[k] = e;
    if (nullcheck && e == 0) return false;
    const u32 c = call<u32>(Effect_FindComponent, e, kComp27469E);
    if (c != 0 && vcall<u32>(&gm<u8>(c), 32) == 5) {
        const u32 q = gm<be_u32>(c + 28);
        const u32 w = o->w160;
        if (q != 0) vcall(&gm<u8>(q), 8);
        gm<be_u32>(c + 28) = w;
        if (w != 0) gm<be_u16>(w + 4) = static_cast<u16>(gm<be_u16>(w + 4) + 1);
    }
    return true;
}

// e +136 &= ~0x200, Effect_Start(e) (e re-read from the slot each time).
void StartSlot(FxOwner* o, u32 k) {
    const u32 e = o->slots[k];
    gm<be_u32>(e + 136) = gm<be_u32>(e + 136) & ~0x200u;
    call(Effect_Start, o->slots[k].get());
}

// Effect_SetTime(slot effect; the .text f32 kStartTime).
void TimeSlot(FxOwner* o, u32 k) { call(Effect_SetTime, o->slots[k].get(), lf(global<be_f32>(kStartTime))); }
}  // namespace

// void f(Obj* o) (no caller reads r3): o +216 is the owner (none: nothing). o +1376 = the
// value (+8) of o +356's entry +4 (0 without one) when +356 is set. With
// the global effect set (Set_Find(0); none: returns 0), each empty effect
// slot is filled from the set entry named by the owner: +1364 by owner
// +880 (then +1368 / +1372 = its def's sub-set values for 2527 / 0x10CBB,
// bit 0x200 of +136 cleared and Effect_Start), +1352 by +872 and +1360 by
// +876 (each: bit 0x200 cleared, Effect_Start, Effect_SetTime(.text f32)).
// The selects are the inlined path-recording BST walks (BstSelect).
void Owner_StartEffects_002464F0(FxOwner* o) {
    const u32 own = o->owner;
    if (own == 0) return;
    if (const u32 a = o->a356; a != 0) {
        const u32 b = gm<be_u32>(a + 4);
        o->w1376 = b != 0 ? gm<be_u32>(b + 8).get() : 0u;
    }
    const u32 set = call<u32>(Set_Find, 0u);
    if (set == 0) return;
    if (o->slots[kSlot1364] == 0u) {
        const u32 key = gm<be_u32>(own + 880);
        if (EffectSetSelect(set, key) != 0 && MakeSlot(o, set, kSlot1364, false)) {
            o->sub[0] = SubValue(gm<be_u32>(o->slots[kSlot1364]), kKey9DF);
            o->sub[1] = SubValue(gm<be_u32>(o->slots[kSlot1364]), kKey10CBB);
            StartSlot(o, kSlot1364);
        }
    }
    if (o->slots[kSlot1352] == 0u) {
        const u32 key = gm<be_u32>(own + 872);
        if (EffectSetSelect(set, key) != 0 && MakeSlot(o, set, kSlot1352, true)) {
            StartSlot(o, kSlot1352);
            TimeSlot(o, kSlot1352);
        }
    }
    if (o->slots[kSlot1360] != 0u) return;
    const u32 key = gm<be_u32>(own + 876);
    if (EffectSetSelect(set, key) == 0) return;
    if (!MakeSlot(o, set, kSlot1360, false)) return;
    StartSlot(o, kSlot1360);
    TimeSlot(o, kSlot1360);
}

}  // namespace sly4::obj

NATIVE(002464F0, sly4::obj::Owner_StartEffects_002464F0)
