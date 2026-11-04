// World_HandleEvent callees, part 11d: the per-slot loader tick and the
// loader's finish step.
#include "hevent11.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent11;

namespace {
// UserBlock slot pair: +0 holder, +4 u32: top byte cursor, bit 23 done.
constexpr u32 kSlotState = 4;
constexpr u32 kSlotDone = 0x00800000;
// Slot entry +24 Effect* (+136 flags, bit 9 attached), +28 flags:
constexpr u32 kSlotEntryEffect = 24, kEffectFlags = 136, kEffectFlagAttached = 0x200;
constexpr u32 kEntryAlways = 0x80000000;    // bit 31: ask the world first (name?)
constexpr u32 kEntryPrepared = 0x20000000;  // bit 29: 0x00324F58 ran
constexpr u32 kEntryWait = 0x10000000;      // bit 28: wait for the effect / entry
constexpr u32 kEntryStartedT = 0x08000000;  // bit 27 (kSlotEntryStarted)
constexpr u32 kEntryDone = 0x04000000;      // bit 26: finished, cursor moved on
constexpr u32 kEntryPrepare = 0x00324F58;   // (UserBlock*, Entry*, s32 slot) (name?)
constexpr u32 kEntryIsNew = 0x002E9B78;     // (u32 hash) -> non-zero (name?)
constexpr u32 kEntryBusy = 0x0042C22C;      // (u32 hash) -> non-zero while busy (name?)
constexpr u32 kWorldHasEntry = 0x0046C750;  // (World*, u32 hash) -> non-zero (name?)
constexpr u32 kEffectCheck = 0x004CF620;    // (Effect*, u8) -> u8 done (Effect_Check4CF620)
constexpr u32 kWorldDirty = 45;             // World +45 u8, bit 1 set for a new entry (name?)

// Loader: +40 u32 scene, +96 u32 entry id, +100 u32 flags (bit 31 started).
constexpr u32 kLdScene = 40, kLdEntryId = 96, kLdFlags = 100, kLdStarted = 0x80000000;
constexpr u32 kEntUser = 12, kEntId = 2560, kEntGetLevel = 80;  // Entry +12 UserBlock*, vtable +80
constexpr u32 kUbEntries = 80, kUbCount = 88;
constexpr u32 kWorldUsers = 29;             // World +29 u8 user count
constexpr u32 kWorldLevel = 0x00128114;     // World + this: u32 current level id
constexpr u32 kFadeSet = 0x7719A548;        // Set_Find name (name?)
constexpr u32 kSetFind = 0x004BCE90, kSetStop = 0x004BD1CC;  // Set_Find; (Set*) (name?)
constexpr u32 kUserBlockReset = 0x0045C7A8; // UserBlock_Reset45C7A8
constexpr u32 kLoaderBegin = 0x002E0008;    // (Loader*, scene) (name?)
constexpr u32 kLoaderStop = 0x002DD898;     // (Loader*) (name?)
constexpr u32 kLoaderStart = 0x002DEA50;    // (Loader*, Entry*, 0, 0) (name?)
constexpr u32 kLoaderReset = 0x002DEDB4;    // (Loader*, 0, 1) (name?)
constexpr u32 kWorldStartLevel = 0x004674C4;  // World_StartLevel (World*, id, u8)
constexpr u32 kWorldSetFlags44 = 0x004896F8;  // World_SetFlags44 (World*, 0)
}  // namespace

// u32 Loader_Tick3250A0(UserBlock* ub, s32 slot) (name?): steps the slot's
// entries from its cursor (slot +4 top byte): prepares each once
// (0x00324F58), and an entry that is done, or isn't waiting, or whose wait
// is over (world check 0x0046C750 for bit 31 entries, then its effect done
// and 0x0042C22C idle), gets its effect released (+136 bit 9) and marked
// done, and moves the cursor on (slot bit 23 set at the end). A started
// entry new to 0x002E9B78 sets World +45 bit 1. Stops at the first entry
// still waiting. r3 is left as the original leaves it.
DECOMP(003250A0, Loader_Tick3250A0) {
    const u32 ub = arg<u32>(ctx, 0);
    const s32 slot = arg<s32>(ctx, 1);
    if (slot == -1) return ret(ctx, ub);
    const u32 p = ub + static_cast<u32>(slot) * kUbSlotSize + kUbSlots;
    if ((load<u32>(p + kSlotState) & kSlotDone) != 0) return ret(ctx, 1u);
    {
        const u32 h = load<u32>(p);
        if (h == 0 || load<u32>(h) == 0) return ret(ctx, 0u);
        if (load<u16>(load<u32>(h) + 8) == 0) return ret(ctx, 0u);
    }
    for (;;) {
        const u32 cur = load<u8>(p + kSlotState);
        const u32 vec = load<u32>(load<u32>(p));
        if (!(cur < (vec != 0 ? load<u16>(vec + 8) : 0u))) return ret(ctx, vec);
        const u32 e = load<u32>(load<u32>(vec) + ((cur << 2) & 0x3FC));
        u32 f = load<u32>(e + kSlotEntryFlags);
        if ((f & kEntryPrepared) == 0) {
            call(ctx, kEntryPrepare, {ub, e, static_cast<u32>(slot)});
            f = load<u32>(e + kSlotEntryFlags);
        }
        if ((f & kEntryDone) == 0) {
            if ((f & kEntryPrepared) == 0) return ret(ctx, f);
            if ((f & kEntryWait) != 0) {
                bool wait = true;
                if ((f & kEntryAlways) != 0)
                    wait = static_cast<u32>(call(ctx, kWorldHasEntry, {load<u32>(kCurrentWorld), load<u32>(e + kSlotEntryHash)})) != 0;
                if (wait) {
                    const u32 fx = load<u32>(e + kSlotEntryEffect);
                    if (fx == 0) return ret(ctx, 0u);
                    if (static_cast<u32>(call(ctx, kEffectCheck, {fx, 0})) == 0) return ret(ctx, 0u);
                    const u32 busy = static_cast<u32>(call(ctx, kEntryBusy, {load<u32>(e + kSlotEntryHash)}));
                    if (busy != 0) return ret(ctx, busy);
                }
            }
        }
        // Release the effect, mark the entry done and advance the cursor.
        const u32 fx = load<u32>(e + kSlotEntryEffect);
        if (fx != 0) {
            store<u32>(fx + kEffectFlags, load<u32>(fx + kEffectFlags) | kEffectFlagAttached);
            store<u32>(e + kSlotEntryEffect, 0);
        }
        store<u32>(e + kSlotEntryFlags, load<u32>(e + kSlotEntryFlags) | kEntryDone);
        const u32 next = load<u8>(p + kSlotState) + 1;
        const u32 holder = load<u32>(p);
        u32 w = load<u32>(p + kSlotState);
        store<u8>(p + kSlotState, static_cast<u8>(next));
        const u32 v2 = load<u32>(holder);
        const u32 end = (next & 0xFF) < (v2 != 0 ? load<u16>(v2 + 8) : 0u) ? 0u : 1u;
        w = (w & 0x00FFFFFFu) | (next << 24);
        w = (w & ~kSlotDone) | (end << 23);
        store<u32>(p + kSlotState, w);
        if ((load<u32>(e + kSlotEntryFlags) & kEntryStartedT) == 0) continue;
        if (static_cast<u32>(call(ctx, kEntryIsNew, {load<u32>(e + kSlotEntryHash)})) == 0) continue;
        const u32 world = load<u32>(kCurrentWorld);
        store<u8>(world + kWorldDirty, load<u8>(world + kWorldDirty) | 2);
    }
}

// u32 Loader_Finish2E0498(Loader* ld, Entry* e, u8 force, u8 arg) (name?):
// resets every user block, begins the loader on its scene and asks e for
// its level (vtable +80). For no level, or the current one without force:
// stops the fade set (0x7719A548) if there is a scene and starts the loader
// on e. Otherwise resets the loader and starts the level (World_StartLevel,
// or World_SetFlags44 plus the fade stop when it is already current), then
// marks the loader started with e's id. The search of e in its user block's
// entries has no effect, as in the original.
DECOMP(002E0498, Loader_Finish2E0498) {
    const u32 ld = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const u32 force = arg<u32>(ctx, 2) & 0xFF, a = arg<u32>(ctx, 3) & 0xFF;
    if (e == 0) return ret(ctx, ld);
    for (u32 i = 0, off = 0; i < load<u8>(load<u32>(kCurrentWorld) + kWorldUsers); ++i, off += kUserBlockSize)
        call(ctx, kUserBlockReset, {load<u32>(kCurrentWorld) + off + kUserBlocks});
    call(ctx, kLoaderBegin, {ld, load<u32>(ld + kLdScene)});
    const u32 level = static_cast<u32>(vcall(ctx, e, kEntGetLevel, {e}));
    auto stop_fade = [&] {
        if (load<u32>(ld + kLdScene) == 0) return;
        const u32 s = static_cast<u32>(call(ctx, kSetFind, {kFadeSet}));
        if (s == 0) return;
        call(ctx, kSetStop, {s});
        call(ctx, kLoaderStop, {ld});
    };
    if (level == 0 || (force == 0 && load<u32>(load<u32>(kCurrentWorld) + kWorldLevel) == level)) {
        stop_fade();
        call(ctx, kLoaderStart, {ld, e, 0, 0});
        return;
    }
    call(ctx, kLoaderReset, {ld, 0, 1});
    const u32 world = load<u32>(kCurrentWorld);
    if (load<u32>(world + kWorldLevel) != level) {
        call(ctx, kWorldStartLevel, {world, level, a});
    } else {
        call(ctx, kWorldSetFlags44, {world, 0});
        stop_fade();
    }
    store<u32>(ld + kLdFlags, load<u32>(ld + kLdFlags) | kLdStarted);
    const u32 id = load<u32>(e + kEntId);
    store<u32>(ld + kLdEntryId, id);
    ret(ctx, id);
}
