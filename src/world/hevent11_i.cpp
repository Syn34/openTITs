// World_HandleEvent callees, part 11i: switching a volume item on or off.
#include "hevent11.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent11;

namespace {
// Volume item: +76 / +80 two 96-byte switch slots ({+76 u32, +80 holder
// (+28 switchable)}), +240 vec3 sound position, +276 s32 mode (1: toggle
// mode), +288 child switchable, +304 sound desc, +308 emitter handle,
// +316 / +320 f32 timers, +328 flags.
constexpr u32 kItSlotA = 76, kItSlotB = 80, kItSlotSize = 96, kHolderSwitch = 28;
constexpr u32 kItSoundPos = 240, kItMode = 276, kItChild = 288, kItSound = 304, kItEmitter = 308;
constexpr u32 kItTimerA = 316, kItTimerB = 320, kItFlags = 328;
constexpr s32 kModeToggle = 1;
constexpr u32 kFlagIdle = 0x80000000;     // bit 31: off and settled (name?)
constexpr u32 kFlagFired = 0x40000000;    // bit 30: switched by a trigger
constexpr u32 kFlagForceOn = 0x20000000;  // bit 29: switchables stay on
constexpr u32 kFlagShown = 0x10000000;    // bit 28
constexpr u32 kFlagOn = 0x08000000;       // bit 27
constexpr u32 kSwitchSet = 40;            // switchable vtable: (obj, u8 on, u8 1)
constexpr u32 kTimerTuneA = 0x0106D584, kTimerTuneB = 0x0106D5D0, kTuneValue = 72;  // tuning objects (as Probe_Prepare39D054)
constexpr u32 kPtrDerefNonZero = 0x00446960;  // Ptr_DerefNonZero (p) -> *p != 0
constexpr u32 kSoundPlay = 0x0044C474;        // Sound_Play44C474 (11 arguments)

u32 bit(u32 f, u32 m) { return (f & m) != 0 ? 1u : 0u; }

// Sets both switch slots' switchables (when present) to on.
void switch_slots(PPUContext& ctx, u32 it, u32 on) {
    for (u32 j = 0; j < 2; ++j) {
        const u32 base = it + j * kItSlotSize;
        if (load<u32>(base + kItSlotA) == 0) continue;
        const u32 h = load<u32>(base + kItSlotB);
        if (h == 0) continue;
        const u32 d = load<u32>(h + kHolderSwitch);
        if (d == 0) continue;
        vcall(ctx, d, kSwitchSet, {d, on, 1});
    }
}

// Stops the item's emitter when it has a sound.
bool stop_sound(PPUContext& ctx, u32 it) {
    if (static_cast<u32>(call(ctx, kPtrDerefNonZero, {load<u32>(it + kItSound)})) == 0) return false;
    call(ctx, hfn::Emitter_Stop, {load<u32>(it + kItEmitter), 1});
    store<u32>(it + kItEmitter, 0);
    return true;
}
}  // namespace

// u32 Volume_Item3992AC(Item* it, u8 on, u8 forced, u8 fired) (name?):
// unless unchanged (and not forced), stores the on bit (27, bit 28 copied,
// bit 31 cleared, bit 30 for a trigger). Normal mode: the child and the
// switch slots follow (on, or always with bit 29), the sound is restarted
// at +240 when on. Toggle mode (+276 == 1; a repeat trigger is ignored):
// everything switches off (slots on with bit 29), and on a change the
// timers restart from the tuning, bit 28 becomes !on, the child and slots
// follow !on and an off item is marked idle (bit 31). r3 is left as the
// original leaves it.
DECOMP(003992AC, Volume_Item3992AC) {
    const u32 it = arg<u32>(ctx, 0);
    const u32 on_arg = arg<u32>(ctx, 1);
    const u32 on = on_arg & 0xFF, forced = arg<u32>(ctx, 2) & 0xFF, fired = arg<u32>(ctx, 3) & 0xFF;
    const u32 f0 = load<u32>(it + kItFlags);
    const u32 was = bit(f0, kFlagOn);
    if (forced == 0 && was == on) return ret(ctx, was);
    u32 f = (f0 & ~kFlagOn) | ((on_arg & 1) << 27);
    const u32 changed = was - on;  // non-zero when the bit changed (the original's subfc)
    store<u32>(it + kItFlags, f);
    if (fired != 0) {
        if ((f & kFlagFired) != 0 && load<s32>(it + kItMode) == kModeToggle) return ret(ctx, f);
        f |= kFlagFired;
    }
    const u32 now = bit(f, kFlagOn);
    f = (f & ~kFlagIdle & ~kFlagShown) | (now << 28);
    store<u32>(it + kItFlags, f);
    if (load<s32>(it + kItMode) != kModeToggle) {
        u32 child = load<u32>(it + kItChild);
        if (child != 0) {
            vcall(ctx, child, kSwitchSet, {child, now, 1});
            f = load<u32>(it + kItFlags);
        }
        switch_slots(ctx, it, ((f & kFlagForceOn) != 0 || now != 0) ? 1u : 0u);
        if (static_cast<u32>(call(ctx, kPtrDerefNonZero, {load<u32>(it + kItSound)})) == 0) return ret(ctx, 0u);
        call(ctx, hfn::Emitter_Stop, {load<u32>(it + kItEmitter), 1});
        store<u32>(it + kItEmitter, 0);
        if (now == 0) return ret(ctx, 0u);
        // Arguments 9-11 (on the stack) are 0 too.
        Stack frame(ctx, 16);
        call(ctx, kSoundPlay, {frame.addr(), load<u32>(it + kItSound), it + kItSoundPos, 0, 0, 0, 0, 0, 0, 0, 0});
        const u32 h = load<u32>(frame.addr());
        store<u32>(it + kItEmitter, h);
        return ret(ctx, h);
    }
    // Toggle mode.
    const u32 child = load<u32>(it + kItChild);
    if (child != 0) {
        vcall(ctx, child, kSwitchSet, {child, 0, 1});
        f = load<u32>(it + kItFlags);
    }
    switch_slots(ctx, it, (f & kFlagForceOn) != 0 ? 1u : 0u);
    stop_sound(ctx, it);
    if (changed == 0) return ret(ctx, 0u);
    f = load<u32>(it + kItFlags);
    store<u32>(it + kItTimerA, load<u32>(kTimerTuneA + kTuneValue));
    const u32 on2 = bit(f, kFlagOn);
    f = (f & ~kFlagShown) | ((on2 == 0 ? 1u : 0u) << 28);
    store<u32>(it + kItTimerB, load<u32>(kTimerTuneB + kTuneValue));
    ctx.fpr[1] = hevent7::lfs_f64(kTimerTuneB + kTuneValue);  // left in f1 by the original's copy
    store<u32>(it + kItFlags, f);
    const u32 child2 = load<u32>(it + kItChild);
    if (child2 != 0) {
        vcall(ctx, child2, kSwitchSet, {child2, on2 == 0 ? 1u : 0u, 1});
        f = load<u32>(it + kItFlags);
    }
    switch_slots(ctx, it, ((f & kFlagForceOn) != 0 || on2 == 0) ? 1u : 0u);
    f = load<u32>(it + kItFlags);
    if ((f & kFlagOn) != 0) return ret(ctx, f);
    store<u32>(it + kItFlags, f | kFlagIdle);
    ret(ctx, f | kFlagIdle);
}
