// Game code, large-function sweep part 19: effect slots started from the
// global effect set.
#include "big19.h"

using namespace sly4;
using namespace sly4::big19;

namespace {
namespace fx {
constexpr u32 kComp27469E = 0x0027469Eu;  // the component hash looked up on a new effect
constexpr u32 kKey9DF = 2527;             // sub-set keys read into +1368 / +1372
constexpr u32 kKey10CBB = 0x00010CBBu;
constexpr u32 kStartTime = 0x002464ECu;   // .text f32 passed to Effect_SetTime

// The value of a def's sub-set entry `key`: def +24 is the sub-set (root
// **(+12), path +16, count +96, selected +100, global kSubSelected); when
// found, the selected node's +8 re-read through def +24 (else 0).
u32 SubValue(u32 def, u32 key) {
    if (def == 0) return 0;
    const u32 s2 = load<u32>(def + 24);
    if (s2 == 0) return 0;
    if (TreeSelect(s2, 12, 96, 16, 100, kSubSelected, key) == 0) return 0;
    const u32 t = load<u32>(def + 24);
    if (t == 0) return 0;
    const u32 sel = load<u32>(t + 100);
    return sel != 0 ? load<u32>(sel + 8) : 0;
}

// Effect slot o +slot from the set's selected node (+104, its +8 the def):
// nothing without one. New effect (0 when the allocation fails) stored in
// the slot; with `nullcheck` a failed one stops here. Its component
// kComp27469E, when vfunc 32 says 5, has its +28 replaced by o +160 (the
// old one's vfunc 8 called first, the new one's u16 +4 bumped). Returns
// false when nothing was made (or the null check failed).
bool MakeSlot(PPUContext& ctx, u32 o, u32 set, u32 slot, bool nullcheck) {
    using namespace b19fn;
    const u32 node = load<u32>(set + 104);
    if (node == 0) return false;
    const u32 def = load<u32>(node + 8);
    if (def == 0) return false;
    const u32 e = NewEffect(ctx, def);
    store<u32>(o + slot, e);
    if (nullcheck && e == 0) return false;
    const u32 c = static_cast<u32>(call(ctx, Effect_FindComponent, {e, kComp27469E}));
    if (c != 0 && static_cast<u32>(VCall(ctx, c, 32, {c})) == 5) {
        const u32 q = load<u32>(c + 28);
        const u32 k = load<u32>(o + 160);
        if (q != 0) VCall(ctx, q, 8, {q});
        store<u32>(c + 28, k);
        if (k != 0) store<u16>(k + 4, static_cast<u16>(load<u16>(k + 4) + 1));
    }
    return true;
}

// e +136 &= ~0x200, Effect_Start(e) (e re-read from the slot each time).
void StartSlot(PPUContext& ctx, u32 o, u32 slot) {
    const u32 e = load<u32>(o + slot);
    store<u32>(e + 136, load<u32>(e + 136) & ~0x200u);
    call(ctx, b19fn::Effect_Start, {load<u32>(o + slot)});
}

// Effect_SetTime(slot effect; the .text f32 kStartTime), r4 / r5 as the
// original leaves them. Returns its r3.
u64 TimeSlot(PPUContext& ctx, u32 o, u32 slot) {
    const u32 e = load<u32>(o + slot);
    callf(ctx, b19fn::Effect_SetTime, {e, e, kStartTime}, {LfsAt(kStartTime)});
    return ctx.gpr[3];
}
}  // namespace fx
}  // namespace

// u32 f(Obj* o): o +216 is the owner (none: returns o). o +1376 = the
// value (+8) of o +356's entry +4 (0 without one) when +356 is set. With
// the global effect set (Set_Find(0); none: returns 0), each empty effect
// slot is filled from the set entry named by the owner: +1364 by owner
// +880 (then +1368 / +1372 = its def's sub-set values for 2527 / 0x10CBB,
// bit 0x200 of +136 cleared and Effect_Start), +1352 by +872 and +1360 by
// +876 (each: bit 0x200 cleared, Effect_Start, Effect_SetTime(.text f32)).
// The selects are the inlined path-recording BST walks (TreeSelect).
// Returns the last Effect_SetTime's r3, else what the original's r3 holds
// at the exit (+1360 when it was set, 0 when the last select fails).
DECOMP(002464F0, Owner_StartEffects_002464F0) {
    using namespace fx;
    using namespace b19fn;
    const u32 o = arg<u32>(ctx, 0);
    const u32 own = load<u32>(o + 216);
    if (own == 0) { ret(ctx, o); return; }
    const u32 a = load<u32>(o + 356);
    if (a != 0) {
        const u32 b = load<u32>(a + 4);
        store<u32>(o + 1376, b != 0 ? load<u32>(b + 8) : 0);
    }
    const u32 set = static_cast<u32>(call(ctx, Set_Find, {0}));
    if (set == 0) { ret(ctx, 0); return; }

    if (load<u32>(o + 1364) == 0) {
        const u32 key = load<u32>(own + 880);
        if (SetSelect(set, key) != 0 && MakeSlot(ctx, o, set, 1364, false)) {
            store<u32>(o + 1368, SubValue(load<u32>(load<u32>(o + 1364)), kKey9DF));
            store<u32>(o + 1372, SubValue(load<u32>(load<u32>(o + 1364)), kKey10CBB));
            StartSlot(ctx, o, 1364);
        }
    }
    if (load<u32>(o + 1352) == 0) {
        const u32 key = load<u32>(own + 872);
        if (SetSelect(set, key) != 0 && MakeSlot(ctx, o, set, 1352, true)) {
            StartSlot(ctx, o, 1352);
            TimeSlot(ctx, o, 1352);
        }
    }
    const u32 e3 = load<u32>(o + 1360);
    if (e3 != 0) { ret(ctx, e3); return; }
    const u32 key = load<u32>(own + 876);
    const u32 node = SetSelect(set, key);
    if (node == 0) { ret(ctx, 0); return; }
    if (!MakeSlot(ctx, o, set, 1360, false)) { ret(ctx, load<u32>(node + 8)); return; }
    StartSlot(ctx, o, 1360);
    ret(ctx, TimeSlot(ctx, o, 1360));
}
