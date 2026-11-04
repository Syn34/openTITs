// World_HandleEvent callees, part 12a: small helpers named by the
// hevent11_*.cpp functions (registry entries, targets, object notifies).
#include <bit>

#include "hevent12.h"

using namespace sly4;
using namespace sly4::hevent12;

namespace {
// Registry_Find value: an object whose vtable +12 starts it (with 0) and +16
// touches it.
constexpr u32 kRegVtStart = 12, kRegVtTouch = 16;
constexpr u32 kTargetComponent = 27;  // Obj_FindComponent type of a target's def (name?)

// Busy list at 0x010AA694: {+0 Busy*[], +8 u16 count}; Busy +0 u32 hash,
// +12 u16 pending.
constexpr u32 kBusyList = 0x010AA694;
constexpr u32 kBusyHash = 0, kBusyPending = 12;

// Target owner: +372 per-kind lists (u32 slots) of {+16 {u32 target, u32}[],
// +36 u32 count}.
constexpr u32 kOwnerLists = 372, kListItems = 16, kListCount = 36, kListItemSize = 8;

// Object +8 def (+152 u32: has a prompt), +328 component (+12 prompt), prompt
// +232 f32 fade rate.
constexpr u32 kObjDef = 8, kDefHasPrompt = 152, kObjComp = 328, kCompPrompt = 12, kPromptRate = 232;
constexpr f32 kFadeDefault = 3.0f;     // 0x00367744
constexpr u32 kFadeMinBits = 0x3727C5AC;  // 0x00367748: 1e-5
constexpr u32 kFadeMaxBits = 0x7F7FFFFF;  // 0x0036774C: FLT_MAX

// Geometry instance +208 -> +132 -> +48 -> +4 -> +128 record (0 for none);
// the record's +8 block gets +0 = 0 and +32 bits 20..27 = 2.
constexpr u32 kInstGeom = 208, kGeomA = 132, kGeomB = 48, kGeomC = 4, kGeomRec = 128, kRecBlock = 8;
constexpr u32 kBlockFlags = 32, kBlockModeMask = 0x0FF00000u, kBlockMode2 = 0x00200000u;

// Object +396 state bits.
constexpr u32 kObjState = 396, kObjKind160 = 160, kObjByte164 = 164;
constexpr u32 kStEnabled = 0x40000000u;  // must be set
constexpr u32 kStA = 0x20000000u;        // set from the a argument
constexpr u32 kStB = 0x10000000u;        // set from the b argument
constexpr u32 kStQueued = 0x08000000u;   // cleared on a change
constexpr u32 kStPending = 0x04000000u;  // set on a change; notifies are ignored while set
constexpr u32 kStLocked = 0x02000000u;   // notifies ignored (as Handle_Notify36742C)
}  // namespace

// void* Target_GetDef0D951C(Object* target) (name?): its type-27 component.
DECOMP(000D951C, Target_GetDef0D951C) {
    ret(ctx, call(ctx, h12fn::Obj_FindComponent, {arg<u32>(ctx, 0), kTargetComponent, 0}));
}

// bool World_HasEntry46C750(World* w, u32 hash) (name?): an entry of the
// world's list has +40 == hash.
DECOMP(0046C750, World_HasEntry46C750) {
    const u32 w = arg<u32>(ctx, 0), hash = arg<u32>(ctx, 1);
    const u32 n = load<u16>(w + kWorldEntryCount);
    const u32 items = load<u32>(w + kWorldEntries);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(load<u32>(items + 4 * i) + kWorldEntryHash) == hash) return ret(ctx, 1u);
    }
    ret(ctx, 0u);
}

// bool Entry_IsNew2E9B78(u32 hash) (name?): calls vtable +16 of the
// registry's object for hash; true when there is one.
DECOMP(002E9B78, Entry_IsNew2E9B78) {
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), arg<u32>(ctx, 0));
    const u32 obj = load<u32>(static_cast<u32>(call(ctx, hevent11::h11fn::Registry_Find, {hevent11::kRegistry, frame.addr()})));
    if (obj == 0) return ret(ctx, 0u);
    hevent::vcall(ctx, obj, kRegVtTouch, {obj});
    ret(ctx, 1u);
}

// bool Entry_Start2E9B08(u32 hash) (name?): calls vtable +12 (obj, 0) of the
// registry's object for hash; true when there is one.
DECOMP(002E9B08, Entry_Start2E9B08) {
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), arg<u32>(ctx, 0));
    const u32 obj = load<u32>(static_cast<u32>(call(ctx, hevent11::h11fn::Registry_Find, {hevent11::kRegistry, frame.addr()})));
    if (obj == 0) return ret(ctx, 0u);
    hevent::vcall(ctx, obj, kRegVtStart, {obj, 0});
    ret(ctx, 1u);
}

// void Prompt_Fade367758(Object* o; f32 time) (name?): sets the prompt's
// fade rate to 1 / time (time < 0: 3; time < 1e-5 or NaN: FLT_MAX). r3 is
// left as the original leaves it.
DECOMP(00367758, Prompt_Fade367758) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return ret(ctx, 0u);
    if (load<u32>(load<u32>(o + kObjDef) + kDefHasPrompt) == 0) return ret(ctx, o);
    const u32 p = load<u32>(load<u32>(o + kObjComp) + kCompPrompt);
    if (p == 0) return ret(ctx, 0u);
    f64 t = farg(ctx, 0);
    if (t < 0.0) {
        t = kFadeDefault;
        ctx.fpr[1] = t;  // the original reloads f1
    }
    u32 rate = kFadeMaxBits;
    if (t >= static_cast<f64>(std::bit_cast<f32>(kFadeMinBits))) rate = std::bit_cast<u32>(static_cast<f32>(1.0 / t));
    store<u32>(p + kPromptRate, rate);
    ret(ctx, p);
}

// Block* Obj_SetGeom36468C(Object*, Instance** slot, Instance* inst) (name?):
// with a geometry (+208) stores inst in *slot; its record block (if any) gets
// +0 = 0 and mode 2 in +32 bits 20..27. Returns the block, 0 when there is no
// geometry or record, or the untouched first argument when inst is 0.
DECOMP(0036468C, Obj_SetGeom36468C) {
    const u32 slot = arg<u32>(ctx, 1), inst = arg<u32>(ctx, 2);
    if (inst == 0) return;  // r3 left as the first argument
    if (load<u32>(inst + kInstGeom) == 0) return ret(ctx, 0u);
    store<u32>(slot, inst);
    const u32 geom = load<u32>(inst + kInstGeom);
    const u32 rec = load<u32>(load<u32>(load<u32>(load<u32>(geom + kGeomA) + kGeomB) + kGeomC) + kGeomRec);
    if (rec == 0) return ret(ctx, 0u);
    const u32 blk = rec + kRecBlock;
    if (blk == 0) return ret(ctx, 0u);
    const u32 f = load<u32>(blk + kBlockFlags);
    store<u32>(blk, 0);
    store<u32>(blk + kBlockFlags, (f & ~kBlockModeMask) | kBlockMode2);
    ret(ctx, blk);
}

// bool Entry_Busy42C22C(u32 hash) (name?): the busy list (0x010AA694) entry
// for a non-zero hash has a pending count (+12).
DECOMP(0042C22C, Entry_Busy42C22C) {
    const u32 hash = arg<u32>(ctx, 0);
    if (hash == 0) return ret(ctx, 0u);
    const u32 items = load<u32>(kBusyList);
    const u32 n = load<u16>(kBusyList + 8);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(load<u32>(items + 4 * i) + kBusyHash) == hash)
            return ret(ctx, load<u16>(load<u32>(items + 4 * i) + kBusyPending) != 0 ? 1u : 0u);
    }
    ret(ctx, 0u);
}

// Object* Target_Next271C08(Owner* from, s32 kind, Object* prev) (name?): the
// first non-null target of the kind's list after prev (from the start when
// prev is 0), or 0.
DECOMP(00271C08, Target_Next271C08) {
    const u32 from = arg<u32>(ctx, 0), kind = arg<u32>(ctx, 1), prev = arg<u32>(ctx, 2);
    const u32 list = load<u32>(from + kind * 4 + kOwnerLists);
    const u32 n = load<u32>(list + kListCount);
    if (n == 0) return ret(ctx, 0u);
    const u32 items = load<u32>(list + kListItems);
    bool passed = prev == 0;
    for (u32 i = 0; i < n; ++i) {
        const u32 t = load<u32>(items + i * kListItemSize);
        if (passed && t != 0) return ret(ctx, t);
        if (t == prev) passed = true;
    }
    ret(ctx, 0u);
}

// void Obj_NotifyB362B40(Object* o, u8 a, u8 b) (name?): for an enabled,
// unlocked object with no change pending (+396): kinds other than 0 and 4
// (+160 >> 1) take bit 0 of a and b into +396 0x20000000 / 0x10000000 and get
// a change pending (+164 = 0); kind 0 only gets the change pending, and only
// when a, b or a current a / b bit is set; kind 4 is left alone.
DECOMP(00362B40, Obj_NotifyB362B40) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 st = load<u32>(o + kObjState);
    if ((st & kStLocked) != 0 || (st & kStEnabled) == 0 || (st & kStPending) != 0) return;
    const u32 kind = load<u32>(o + kObjKind160) >> 1;
    if (kind != 0 && kind != 4) {
        u32 v = (st & ~kStQueued) | kStPending;
        store<u8>(o + kObjByte164, 0);
        v = (v & ~kStA) | ((a & 1) << 29);
        v = (v & ~kStB) | ((b & 1) << 28);
        store<u32>(o + kObjState, v);
        return;
    }
    if ((st & kStA) == 0 && (a & 0xFF) == 0 && (st & kStB) == 0 && (b & 0xFF) == 0) return;
    if (kind != 0) return;
    store<u8>(o + kObjByte164, 0);
    store<u32>(o + kObjState, (st & ~kStQueued) | kStPending);
}
