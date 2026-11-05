// World_HandleEvent callees, part 19i: stopping a restarted slot's
// finished tracks (Track_Sync276BA4).
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent19;

namespace {
// Animator +32 current set handle, +40 eight 40-byte slots {+0 key, +4
// Def*, +20 u32 tag}. Def +54 u16 track count. Set +0 parent, +24 key, +36
// track set. Track +12 s32 remaining, +28 s32 state (2: done), +32 u32 tag.
constexpr u32 kAnimCurrent = 32, kSlots = 40, kSlotSize = 40, kNumSlots = 8, kSlotDef = 4, kSlotTag = 20;
constexpr u32 kDefTracks = 54, kSetParent = 0, kSetKey = 24, kSetTracks = 36;
constexpr u32 kTrackRemaining = 12, kTrackState = 28, kTrackTag = 32, kStateDone = 2;
constexpr u32 kTrackFind = 0x004B3624, kTrackStop = 0x00254210, kAnimHandle = 0x00254438;
}  // namespace

// void Track_Sync276BA4(Animator* a, u32 key) (name?): for each track of
// the key slot's def, finds it in the current set (or, through held
// handles, in its ancestors) and stops it when it is done (state 2, tag =
// the slot's, nothing remaining). r3 is not kept as the original leaves it
// (its only caller, Anim_Restart2775A8, ignores it).
DECOMP(00276BA4, Track_Sync276BA4) {
    const u32 a = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    u32 slot = 0;
    for (u32 i = 0; i < kNumSlots; ++i)
        if (load<u32>(a + kSlots + kSlotSize * i) == key) {
            slot = a + kSlots + kSlotSize * i;
            break;
        }
    if (slot == 0) return;
    const u32 def = load<u32>(slot + kSlotDef);
    if (def == 0) return;
    u32 obj = handle_object(load<u32>(kAnimPool), load<u32>(a + kAnimCurrent));
    if (obj == 0) return;
    const u32 n = load<u16>(def + kDefTracks);
    if (n == 0) return;
    Stack frame(ctx, 16);  // r1 +112 / +116: handles
    const u32 h112 = frame.addr(), h116 = frame.addr() + 4;
    for (u32 i = 0; i < n; ++i) {
        const u64 idx = static_cast<u64>(static_cast<s64>(static_cast<s16>(static_cast<u16>(i))));
        u32 t = static_cast<u32>(call(ctx, kTrackFind, {def, idx, load<u32>(load<u32>(obj) + kSetTracks)}));
        // Not in this set: try its parent, then follow the parent's handle
        // (an unresolved one reads the set from address 0, as the original).
        while (t == 0) {
            const u32 parent = load<u32>(load<u32>(obj) + kSetParent);
            if (parent == 0) break;
            t = static_cast<u32>(call(ctx, kTrackFind, {def, idx, load<u32>(parent + kSetTracks)}));
            call(ctx, kAnimHandle, {h112, a, load<u32>(load<u32>(load<u32>(obj) + kSetParent) + kSetKey)});
            if ((load<u32>(h112) & 0xFFFF) == 0) break;
            const u32 set = load<u32>(obj);
            const u32 pool = load<u32>(kAnimPool);
            call(ctx, kAnimHandle, {h116, a, load<u32>(load<u32>(set + kSetParent) + kSetKey)});
            obj = handle_object(pool, load<u32>(h116));
        }
        if (t != 0 && load<u32>(t + kTrackTag) == load<u32>(slot + kSlotTag) && static_cast<s32>(load<u32>(t + kTrackState)) == static_cast<s32>(kStateDone) &&
            static_cast<s32>(load<u32>(t + kTrackRemaining)) <= 0)
            call(ctx, kTrackStop, {a, obj, t});
    }
}
