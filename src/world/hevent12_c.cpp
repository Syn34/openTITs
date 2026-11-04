// World_HandleEvent callees, part 12c: loader helpers named by
// Loader_Finish2E0498 (hevent11_d.cpp).
#include "hevent12.h"

using namespace sly4;
using namespace sly4::hevent12;

namespace {
// Loader: +40 Level*, +72 current user entry, +96 pending user id, +100 u32
// flags (bit 31: restart pending; 0x10000000 cleared on reset).
constexpr u32 kLdLevel = 40, kLdCurrent = 72, kLdPendingId = 96, kLdFlags = 100;
constexpr u32 kLdRestart = 0x80000000u, kLdResetBit = 0x10000000u;
// Level (as hevent11_c.cpp): +408 part names {+0 char[129][], +8 u16 count},
// +508 s32 state, +528 Vector* of holders (+0 Holder*[]); Holder +0 Request*,
// +8 data, +12 size; Request +32 s32 state (6: done).
constexpr u32 kLvParts = 408, kLvState = 508, kLvHolders = 528;
constexpr u32 kPartNameSize = 129;
constexpr u32 kHolderReq = 0, kHolderData = 8, kHolderSize = 12, kReqState = 32;
constexpr s32 kReqDone = 6;
constexpr u32 kSetFind = 0x004BCE90;        // Set_Find (u32 name) -> Set*
constexpr u32 kLevelSetName = 0x7719A548;   // as Entry_Prepare324F58's load kind (name?)
constexpr u32 kSetUsers = 8;                // Set +8 s32 users (name?)
constexpr u32 kLevelRelease = 0x002DA5E0;   // (Level*) (name?)
constexpr u32 kSetRemove = 0x004BDD08;      // (Set*, const char* name, 0) (name?)
constexpr u32 kSetAddArchive = 0x004BE374;  // (Set*, Archive*, 0) (name?)
constexpr u32 kArchiveOpenZip = 0x00567034; // Archive_OpenZip (data, size, name)
constexpr u32 kArchiveVtRelease = 8;
// User blocks (World +4208): +80 Entry*[], +88 u16 count. Entry +18 u8
// state, +20 / +24 u32, vtable +24 (e, u8, -1), +40 (e, u32).
constexpr u32 kUbEntries = 80, kUbCount = 88;
constexpr u32 kUeState = 18, kUeValue = 20, kUeSaved = 24;
constexpr u32 kUeVtSet = 24, kUeVtSelect = 40;
constexpr u8 kUeStateNext = 5, kUeStateCurrent = 4;
constexpr u32 kUserBlockFind = 0x00465F30;  // UserBlock_Find465F30 (UserBlock*, id)
constexpr u32 kLoaderStart = 0x002DEA50;    // Loader_Start2DEA50 (Loader*, Entry*, 0, 0)
}  // namespace

// void Loader_Stop2DD898(Loader* l) (name?): with a level and an unused level
// set (Set_Find 0x7719A548, +8 <= 0): releases the level when its state is 1,
// then for each part: state 0 removes the part's name from the set; otherwise
// a part whose request isn't done (state 6) gets an archive over its holder's
// data added to the set (and the set's reference dropped, vt +8). r3 is left
// as the original leaves it.
DECOMP(002DD898, Loader_Stop2DD898) {
    const u32 l = arg<u32>(ctx, 0);
    if (load<u32>(l + kLdLevel) == 0) return ret(ctx, 0u);
    const u32 set = static_cast<u32>(call(ctx, kSetFind, {kLevelSetName}));
    if (set == 0) return ret(ctx, 0u);
    const s32 users = static_cast<s32>(load<u32>(set + kSetUsers));
    if (users > 0) return ret(ctx, static_cast<u32>(users));
    u32 state = load<u32>(load<u32>(l + kLdLevel) + kLvState);
    if (state == 1) {
        call(ctx, kLevelRelease, {load<u32>(l + kLdLevel)});
        state = load<u32>(load<u32>(l + kLdLevel) + kLvState);
    }
    for (u32 i = 0;; ++i) {
        const u32 lev = load<u32>(l + kLdLevel);
        const u32 parts = lev + kLvParts;
        if (i >= load<u16>(parts + 8)) return ret(ctx, lev);
        const u32 name = load<u32>(parts) + i * kPartNameSize;
        if (state == 0) {
            call(ctx, kSetRemove, {set, name, 0});
            continue;
        }
        const u32 holder = load<u32>(load<u32>(load<u32>(lev + kLvHolders)) + i * 4);
        const u32 req = load<u32>(holder + kHolderReq);
        if (req != 0 && static_cast<s32>(load<u32>(req + kReqState)) == kReqDone) continue;
        const u32 zip = static_cast<u32>(call(ctx, kArchiveOpenZip, {load<u32>(holder + kHolderData), load<u32>(holder + kHolderSize), name}));
        call(ctx, kSetAddArchive, {set, zip, 0});
        hevent::vcall(ctx, zip, kArchiveVtRelease, {zip});
    }
}

// u32 Loader_Reset2DEDB4(Loader* l, u8 restart, u8 restore) (name?): with a
// level: a pending restart (+100 bit 31, restart set) is cleared and starts
// the pending user entry (+96, then 0); otherwise every user entry is reset
// (vt +24 (e, restart, -1); restore copies +24 to +20 first) and, when
// restarting with no entry in state 4, the entry after the last one in state
// 5 is selected (vt +40 with +20 & ~7, vt +24) and becomes the current one
// (+72). Clears +100 0x10000000; returns 0.
DECOMP(002DEDB4, Loader_Reset2DEDB4) {
    const u32 l = arg<u32>(ctx, 0);
    const u32 restart = arg<u32>(ctx, 1) & 0xFF, restore = arg<u32>(ctx, 2) & 0xFF;
    if (load<u32>(l + kLdLevel) == 0) return ret(ctx, 0u);
    const u32 f = load<u32>(l + kLdFlags);
    if (restart != 0 && (f & kLdRestart) != 0) {
        const u32 id = load<u32>(l + kLdPendingId);
        store<u32>(l + kLdFlags, f & ~kLdRestart);
        if (id != 0) {
            const u32 e = static_cast<u32>(call(ctx, kUserBlockFind, {load<u32>(hevent::kCurrentWorld) + hevent::kUserBlocks, id}));
            call(ctx, kLoaderStart, {l, e, 0, 0});
            store<u32>(l + kLdPendingId, 0);
        }
    } else {
        const u32 ub = load<u32>(hevent::kCurrentWorld) + hevent::kUserBlocks;
        bool current = false;
        s32 next = -1;
        u32 n;
        for (u32 i = 0; i < (n = load<u16>(ub + kUbCount)); ++i) {
            const u32 e = load<u32>(load<u32>(ub + kUbEntries) + i * 4);
            if (restore != 0) store<u32>(e + kUeValue, load<u32>(e + kUeSaved));
            hevent::vcall(ctx, e, kUeVtSet, {e, restart, 0xFFFFFFFFu});
            if (restart == 0) continue;
            const u8 st = load<u8>(e + kUeState);
            if (st == kUeStateNext) next = static_cast<s32>(i);
            if (!current && st == kUeStateCurrent) current = true;
        }
        if (next != -1 && !current && static_cast<u32>(next) < n - 1) {
            const u32 e = load<u32>(load<u32>(ub + kUbEntries) + static_cast<u32>(next + 1) * 4);
            hevent::vcall(ctx, e, kUeVtSelect, {e, load<u32>(e + kUeValue) & ~7u});
            hevent::vcall(ctx, e, kUeVtSet, {e, restart, 0xFFFFFFFFu});
            store<u32>(l + kLdCurrent, e);
        }
    }
    store<u32>(l + kLdFlags, load<u32>(l + kLdFlags) & ~kLdResetBit);
    ret(ctx, 0u);
}
