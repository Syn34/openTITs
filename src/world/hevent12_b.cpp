// World_HandleEvent callees, part 12b: user-slot entry preparation, the
// level request constructor and one more sorted pair table lookup.
#include "hevent12.h"

using namespace sly4;
using namespace sly4::hevent7;
using namespace sly4::hevent9;
using namespace sly4::hevent12;

namespace {
// Slot entry (as hevent11.h): +20 u32 hash, +24 Effect*, +28 u32 flags.
constexpr u32 kEntryHash = hevent11::kSlotEntryHash, kEntryEffect = 24, kEntryFlags = hevent11::kSlotEntryFlags;
constexpr u32 kEfUnique = 0x80000000u;    // no effect kept in +24 (name?)
constexpr u32 kEfPrepared = 0x20000000u;  // set by Entry_Prepare324F58
constexpr u32 kEfPresent = 0x10000000u;   // effect started, or World_HasEntry46C750
constexpr u32 kEfStarted = hevent11::kSlotEntryStarted;  // 0x08000000
constexpr u32 kEntryStart = 0x002E9B08;     // Entry_Start2E9B08 (u32 hash)
constexpr u32 kRegistryNotify = 0x002E9BE4; // Registry_Notify (u32 hash) (symbols.txt)
constexpr u32 kEntryLoad = 0x00324B08;      // (UserBlock*, hash, kind, Entry*, bool unique, slot) -> Effect* or 0 (name?)
constexpr u32 kEntryLoadKind = 0x7719A548;  // hash passed to it (name?)
constexpr u32 kEffectSetTime = 0x004CF710;  // Effect_SetTime (Effect*; f32)
constexpr u32 kPrepareTimeConst = 0x00324F50;  // f32 0x3C88CE70 (~0.0167, one frame)
constexpr u32 kWorldHasEntry = 0x0046C750;  // World_HasEntry46C750 (World*, hash)
constexpr u32 kEffectFlags = 136, kEffectFlagAttached = 0x200;

// Level request (72 bytes, vtable 0x00B1D450).
constexpr u32 kRequestSize = 72, kRequestAlign = 16;
constexpr u32 kRequestVtable = 0x00B1D450;
constexpr u32 kRequestDone = 0x00F35F60;  // OPD stored at +16 (name?)
constexpr u32 kReqName = 12, kReqDone = 16, kReqOwner = 24, kReqHolder = 60;
constexpr u32 kReqVtLoad = 12, kReqVtUpdate = 8;
constexpr u32 kHolderNew = 0x00550E70;    // Holder_New550E70 () -> u32
constexpr u32 kStringDup = 0x00556038;    // String_Dup (const char*) -> copy
constexpr u32 kStringFind = 0x00556014;   // String_Thunk71EDAC (str, char) -> char* (strrchr?)

// Default compare OPD of Map_Find7668C4: plain u32 keys.
constexpr u32 kCompareU32_7668 = 0x00F35FB8;

// The sorted pair table lookup (as hevent11_b.cpp): sorts first when
// flagged, then a binary search, with plain unsigned keys for the default
// compare and the compare OPD on (entry, &key copy) otherwise. Returns
// &entry.value, or &t->miss (+16).
void table_find(PPUContext& ctx, u32 plain_compare) {
    const u32 t = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 flags = load<u16>(t + kTabFlags);
    if (flags & kTabUnsorted) {
        store<u16>(t + kTabFlags, static_cast<u16>(flags & 0xFFFE));
        const u32 n = load<u16>(t + kTabCount);
        if (n != 0) call(ctx, h7fn::qsort, {load<u32>(t + kTabEntries), n, 8, load<u32>(t + kTabCompare)});
    }
    s32 hi = static_cast<s32>(load<u16>(t + kTabCount)) - 1, lo = 0;
    u32 base = load<u32>(t + kTabEntries);
    s32 found = -1;
    if (load<u32>(t + kTabCompare) == plain_compare) {
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 want = load<u32>(key), k = load<u32>(base + static_cast<u32>(mid) * 8);
            if (k > want) hi = mid - 1;
            else if (k < want) lo = mid + 1;
            else { found = mid; break; }
        }
    } else {
        Stack frame(ctx, 16);  // the key copy at r1+112
        store<u32>(frame.addr(), load<u32>(key));
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const s32 c = static_cast<s32>(opdcall(ctx, load<u32>(t + kTabCompare), {base + static_cast<u32>(mid) * 8, frame.addr()}));
            if (c > 0) hi = mid - 1;
            else if (c < 0) lo = mid + 1;
            else {
                base = load<u32>(t + kTabEntries);
                found = mid;
                break;
            }
        }
    }
    ret(ctx, found >= 0 ? base + static_cast<u32>(found) * 8 + 4 : t + kTabMiss);
}
}  // namespace

// void Entry_Prepare324F58(UserBlock* ub, Entry* e, s32 slot) (name?): marks
// e prepared, starts it once (Entry_Start2E9B08), notifies the registry and
// loads its effect through 0x00324B08: a new effect is kept in +24 (unless
// +28 bit 31) with its attached flag cleared, set to one frame's time and
// marks the entry present; without one, presence comes from the world's
// entry list.
DECOMP(00324F58, Entry_Prepare324F58) {
    const u32 ub = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1), slot = arg<u32>(ctx, 2);
    u32 f = load<u32>(e + kEntryFlags) | kEfPrepared;
    store<u32>(e + kEntryFlags, f);
    if ((f & kEfStarted) == 0) {
        call(ctx, kEntryStart, {load<u32>(e + kEntryHash)});
        f = load<u32>(e + kEntryFlags) | kEfStarted;
        store<u32>(e + kEntryFlags, f);
    }
    // (f always has kEfStarted here; the original tests it again.)
    call(ctx, kRegistryNotify, {load<u32>(e + kEntryHash)});
    f = load<u32>(e + kEntryFlags);
    const u32 hash = load<u32>(e + kEntryHash);
    f &= ~kEfPresent;
    store<u32>(e + kEntryFlags, f);
    store<u32>(e + kEntryEffect, 0);
    const u32 fx = static_cast<u32>(call(ctx, kEntryLoad, {ub, hash, kEntryLoadKind, e, f >> 31, slot}));
    u32 g = load<u32>(e + kEntryFlags);
    if (fx != 0) {
        if ((g & kEfUnique) == 0) {
            store<u32>(e + kEntryEffect, fx);
            store<u32>(fx + kEffectFlags, load<u32>(fx + kEffectFlags) & ~kEffectFlagAttached);
        }
        callf(ctx, kEffectSetTime, {fx, kPrepareTimeConst}, {lfs_f64(kPrepareTimeConst)});
        g = load<u32>(e + kEntryFlags) | kEfPresent;
        store<u32>(e + kEntryFlags, g);
    }
    if ((g & kEfPresent) != 0) return ret(ctx, 1u);  // r3: the tested bit, as the original leaves it
    {
        const u32 has = static_cast<u32>(call(ctx, kWorldHasEntry, {load<u32>(hevent::kCurrentWorld), load<u32>(e + kEntryHash)}));
        store<u32>(e + kEntryFlags, (load<u32>(e + kEntryFlags) & ~kEfPresent) | ((has & 1) << 28));
    }
}

// void Request_Init2DA158(Request** slot, const char* name) (name?): a new
// 72-byte level request in *slot (owner slot, done OPD, holder), named by a
// copy of name whose extension's third character becomes 'b', then loaded
// (vt +12) and updated (vt +8, 0). After a failed allocation *slot is 0 and
// the +16 / +24 stores go to low memory, as in the original.
DECOMP(002DA158, Request_Init2DA158) {
    const u32 slot = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, fn::Mem_Alloc, {kRequestSize, 0, kRequestAlign}));
    u32 r = 0;
    if (p != 0) {
        for (u32 off = 4; off <= 24; off += 4) store<u32>(p + off, 0);
        store<u8>(p + 28, 0);
        store<u32>(p + 32, 0);
        store<u32>(p, kRequestVtable);
        store<u32>(p + 36, 0);
        store<u32>(p + 40, 0);
        store<u32>(p + 56, 0);
        store<u64>(p + 48, 0);
        store<u32>(p + 60, 0);
        store<u32>(p + 64, 0);
        store<u32>(p + kReqHolder, static_cast<u32>(call(ctx, kHolderNew, {})));
        r = p;
    }
    store<u32>(slot, r);
    store<u32>(r + kReqDone, kRequestDone);  // unchecked
    store<u32>(load<u32>(slot) + kReqOwner, slot);
    const u32 copy = static_cast<u32>(call(ctx, kStringDup, {name}));
    store<u32>(load<u32>(slot) + kReqName, copy);
    const u32 dot = static_cast<u32>(call(ctx, kStringFind, {copy, '.'}));
    store<u8>(dot + 3, 'b');
    const u32 req = load<u32>(slot);
    hevent::vcall(ctx, req, kReqVtLoad, {req});
    const u32 req2 = load<u32>(slot);
    hevent::vcall(ctx, req2, kReqVtUpdate, {req2, 0});
}

// void* Map_Find7668C4(Table* t, const u32* key) (name?): the sorted pair
// table lookup with default compare 0x00F35FB8.
DECOMP(007668C4, Map_Find7668C4) { table_find(ctx, kCompareU32_7668); }
