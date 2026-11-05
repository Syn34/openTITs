// World_HandleEvent callees, part 18e: player stance animation set and the
// session event sink.
#include "hevent18.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent18;

namespace {
// Player: +184 u64 state flags (low word: bit 15 0x8000, bit 19 0x80000,
// bit 26 0x04000000, bit 2 0x4, bit 3 0x8), +216 def (+1376..+1392 stance
// animation sets), +432 s32 stance; vt +60 animator getter (+24 default
// set).
constexpr u32 kPlFlags = 184, kPlDef = 216, kPlStance = 432, kPlVtAnimator = 60, kAnimDefaultSet = 24;
constexpr u32 kFlagS4 = 0x8000, kFlagS5 = 0x80000, kFlagDefault = 0x04000000, kFlagS2 = 0x4, kFlagS3 = 0x8;
constexpr u32 kDefSet1 = 1384, kDefSet2 = 1380, kDefSet3 = 1376, kDefSet4 = 1388, kDefSet5 = 1392;
constexpr u32 kAnimCurrentSet297E3C = 0x00297E3C;  // (Animator*) -> Set* (name?)
constexpr u32 kAnimSetSet297EDC = 0x00297EDC;      // (Animator*, Set*) (name?)
constexpr u32 kAnimStop254814 = 0x00254814;        // (Animator*, key) (hevent17_b.cpp)

// Sink: +84 u32 last object, +100 u32 flags (bit 30 0x40000000: muted).
// Record: +0 s32 kind, +8 u32 name, +24 Object*, +28 u32 hash.
constexpr u32 kSinkLast = 84, kSinkFlags = 100, kSinkMuted = 0x40000000;
constexpr u32 kRecKind = 0, kRecName = 8, kRecObject = 24, kRecHash = 28;
// User block 0 (World +4208): +80 Object*[] users, +88 u16 count, +0x20074
// u32 per-entry states (top nibble 4: active); Object +2452 its entry index,
// vt +16 post(o, 0, rec).
constexpr u32 kUbObjects = 80, kUbObjectCount = 88, kUbStates = 0x20074, kObjEntryIndex = 2452;
constexpr u32 kStateActive = 4, kObjVtPost = 16;
constexpr u32 kObjCanShow2E2104 = 0x002E2104;  // (Sink*, Object*) -> bool (hevent8_b.cpp)
// The counted pickup: kind 1, name 0x021D9F0C, a player object (+184 bit 0)
// whose def (+52) is 0xDF0611F9, hash 0xA77EB5E9. Counted in the user's
// save slot byte +327 (saturating at 255); at the tuning threshold
// (0x01068470 +72) the trophy 0xADE2B55B is awarded (0x000B27B4).
constexpr u32 kPickupName = 0x021D9F0C, kPickupDef = 0xDF0611F9, kPickupHash = 0xA77EB5E9, kDefHash = 52;
constexpr u32 kUbUser = 4208, kSlotPickups = 327, kPickupTune = 0x01068470, kTuneThreshold = 72;
constexpr u32 kPickupTrophy = 0xADE2B55B, kAward0B27B4 = 0x000B27B4;
}  // namespace

// void Player_Settle11F0E4(Player* p, -, u8 restart) (name?): picks the
// stance from the state flags (stance 4 / 5 / 2 / 3 set from flags; 1 / 2
// kept; otherwise 0 and the animator's default set) and, when the stance's
// set differs from the animator's current one, switches to it (stopping
// every animation first when restart).
DECOMP(0011F0E4, Player_Settle11F0E4) {
    const u32 p = arg<u32>(ctx, 0), restart = arg<u8>(ctx, 2);
    const u32 lo = static_cast<u32>(load<u64>(p + kPlFlags));
    u32 d = load<u32>(p + kPlDef);
    bool fallback = false;
    if (lo & kFlagS4) {
        store<u32>(p + kPlStance, 4);
        d = load<u32>(d + kDefSet4);
    } else if (lo & kFlagS5) {
        store<u32>(p + kPlStance, 5);
        d = load<u32>(d + kDefSet5);
    } else if (lo & kFlagDefault) {
        store<u32>(p + kPlStance, 0);
        fallback = true;
    } else if (lo & kFlagS2) {
        store<u32>(p + kPlStance, 2);
        d = load<u32>(d + kDefSet2);
    } else if (lo & kFlagS3) {
        store<u32>(p + kPlStance, 3);
        d = load<u32>(d + kDefSet3);
    } else {
        const s32 s = load<s32>(p + kPlStance);
        if (s == 3 || s == 4 || s == 5) {
            store<u32>(p + kPlStance, 0);
            fallback = true;
        } else if (s == 2) {
            d = load<u32>(d + kDefSet2);
        } else if (s == 1) {
            d = load<u32>(d + kDefSet1);
        } else {
            fallback = true;
        }
    }
    if (fallback) d = load<u32>(static_cast<u32>(vcall(ctx, p, kPlVtAnimator, {p})) + kAnimDefaultSet);
    const u32 a = static_cast<u32>(vcall(ctx, p, kPlVtAnimator, {p}));
    if (d == static_cast<u32>(call(ctx, kAnimCurrentSet297E3C, {a}))) return;
    if (restart != 0) call(ctx, kAnimStop254814, {static_cast<u32>(vcall(ctx, p, kPlVtAnimator, {p})), 0});
    call(ctx, kAnimSetSet297EDC, {static_cast<u32>(vcall(ctx, p, kPlVtAnimator, {p})), d});
}

// void Sink_Post2E5180(Sink* s, u32 user, const Record* rec) (name?):
// unless muted, posts the record to every active user object the sink may
// show it to (kind 3 records first set +84 to their object; +84 ends 0).
// Then counts the pickup record towards its trophy.
DECOMP(002E5180, Sink_Post2E5180) {
    const u32 s = arg<u32>(ctx, 0), rec = arg<u32>(ctx, 2);
    if ((load<u32>(s + kSinkFlags) & kSinkMuted) == 0) {
        if (load<s32>(rec + kRecKind) == 3) store<u32>(s + kSinkLast, load<u32>(rec + kRecObject));
        const u32 ub = load<u32>(kCurrentWorld) + kUbUser;
        for (u32 i = 0; i < load<u16>(ub + kUbObjectCount); ++i) {
            const u32 o = load<u32>(load<u32>(ub + kUbObjects) + 4 * i);
            const u32 st = load<u32>(ub + (load<u32>(o + kObjEntryIndex) << 2) + kUbStates) >> 28;
            if (st != kStateActive) continue;
            if (call(ctx, kObjCanShow2E2104, {s, o}) == 0) continue;
            vcall(ctx, o, kObjVtPost, {o, 0, rec});
        }
        store<u32>(s + kSinkLast, 0);
    }
    if (load<s32>(rec + kRecKind) != 1 || load<u32>(rec + kRecName) != kPickupName) return;
    const u32 o = load<u32>(rec + kRecObject);
    if (o == 0 || (load<u64>(o + kPlFlags) & 1) == 0) return;
    const u32 def = load<u32>(o + kPlDef);
    if ((def != 0 ? load<u32>(def + kDefHash) : 0u) != kPickupDef) return;
    if (load<u32>(rec + kRecHash) != kPickupHash) return;
    const u32 w = load<u32>(kCurrentWorld);
    const u32 slot = w + load<u8>(w + kUbUser) * kSaveSlotSize + kSaveData;
    const s32 c = load<u8>(slot + kSlotPickups);
    if (c >= 255) return;
    store<u8>(slot + kSlotPickups, static_cast<u8>(c + 1));
    if (load<s32>(kPickupTune + kTuneThreshold) > ((c + 1) & 0xFF)) return;
    call(ctx, kAward0B27B4, {kPickupTrophy});
}
