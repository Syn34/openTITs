// World_HandleEvent callees, part 13h: Session_UpdateEntries2E4040, the
// per-frame pass over user block 0's entries (counter texts, prepared
// entries, the next entry to start).
#include "hevent13.h"

using namespace sly4;
using namespace sly4::hevent13;

namespace {
// Session (r3): +72 Entry* current, +100 u32 flags (bit 27 counters shown,
// bit 28 an entry started this frame).
constexpr u32 kSesCurrent = 72, kSesFlags = 100;
constexpr u32 kSesFlagCounters = 0x08000000u, kSesFlagStarted = 0x10000000u;
// UserBlock 0 (World +4208): +8 player, +80 Entry*[], +88 u16 count,
// +0x20074 u32 state per entry slot (top nibble 4: shown; bit 26 prepared;
// bit 27 started).
constexpr u32 kUbPlayer = 8, kUbEntries = 80, kUbEntryCount = 88, kUbStates = 0x20074;
constexpr u32 kStPrepared = 0x04000000u, kStStarted = 0x08000000u;
constexpr u32 kStNibbleShown = 4;
// Entry: +18 u8 kind (4: counter, 5: done), +20 u32 flags (bit 3 waits for
// others), +2276 u32[4] hashes it waits for, +2452 s32 state slot, +2560 u32
// hash; vtable +16 update(e, 0, Ctx*), +24 (e, 0, -1), +36 (e, 0), +40 (e,
// flags), +52 counter(e, u32[3]* out) -> bool, +80 (e) -> u32 level.
constexpr u32 kEnKind = 18, kEnFlags = 20, kEnWaits = 2276, kEnSlot = 2452, kEnHash = 2560;
constexpr u8 kKindCounter = 4, kKindDone = 5;
constexpr u32 kEnFlagWaits = 0x8;
constexpr u32 kEnVtUpdate = 16, kEnVt24 = 24, kEnVt36 = 36, kEnVt40 = 40, kEnVtCounter = 52, kEnVtLevel = 80;
constexpr u32 kNumWaits = 4;
// Counter target object: +4 bit 0 live, +8 (+152 u32, +68 hash), +12 hash,
// +396 bit 30 text slot (else an anim).
constexpr u32 kObjLive = 4, kObjSub = 8, kSub152 = 152, kSubHash = 68, kObjHash = 12, kObjFlags396 = 396, kObj396Text = 0x40000000u;
constexpr u32 kWSceneCount = 0x128214, kWLevel = 0x128114, kW1281F4 = 0x1281F4, kWFlags44 = 44, kWFlag44Bit7 = 0x80, kW72 = 72;

constexpr u32 kObjNotify26 = 0x000D94CC, kObjField244Clear = 0x0029A40C, kSortedMapFind3 = 0x0076611C;
constexpr u32 kHashToString = 0x0056A6E0, kObjSetText = 0x00367530, kObjSetAnim = 0x00363A3C, kPlayerSet = 0x003632C8;
constexpr u32 kCtxInit4651A0 = 0x004651A0;    // (Ctx*, Object* player; f32 dt) (name?)
constexpr u32 kHandleNotify = 0x0036742C, kSessionStart2E3AEC = 0x002E3AEC;  // (Session*, Entry*, 0) (name?)
constexpr u32 kSave0AE418 = 0x000AE418;       // (SaveSlot*, u8*) (name?)
constexpr u32 kStringFormat = 0x005534D0;
constexpr u32 kFmtD = 0x00AD8B78, kFmtDD = 0x00AD8B80;  // "%d", "%d/%d"
constexpr u32 kAnimRate = 0x002E4038;         // f32 -1.0
constexpr u32 kAnim9 = 0x01164CA4;            // r9 of Obj_SetAnim (name?)
}  // namespace

// u32 Session_UpdateEntries2E4040(Session* s; f32 dt) (name?): for every
// entry of user block 0: (until one starts this frame) shown entries and
// prepared ones get the player's Notify26 and, with counters on, their
// counter text ("%d" / "%d/%d" on the counter's object, or two values for a
// two-slot object), then an update (vt +16) with a context for dt. With no
// scene queued a prepared entry becomes started; a finished one (kind 5)
// notifies its handle, and (if none started yet) the first waiting entry
// whose awaited entries are all done is started (vt +40/+24/+36 when its
// level differs, s +72, 0x002E3AEC, s +100 bit 28, 0x000AE418, World
// +0x1281F4 vt +72). Finally bit 28 is cleared (World +72 = 2 first when it is
// 0 and World +44 bit 7). Returns s +100.
DECOMP(002E4040, Session_UpdateEntries2E4040) {
    const u32 s = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    Stack frame(ctx, 64);  // r1+112 .. r1+163
    const u32 key = frame.addr(), cnt = frame.addr() + 4, cnt2 = frame.addr() + 16, cx = frame.addr() + 28;
    const u32 ub = load<u32>(hevent::kCurrentWorld) + hevent::kUserBlocks;
    bool started = false;
    for (u32 i = 0; i < load<u16>(ub + kUbEntryCount); ++i) {
        const u32 e = load<u32>(load<u32>(ub + kUbEntries) + 4 * i);
        const u32 st = ub + (load<u32>(e + kEnSlot) << 2) + kUbStates;
        bool update = false;
        if (!started) {
            const u32 v = load<u32>(st);
            update = (v >> 28) == kStNibbleShown || (v & kStPrepared) != 0;
        }
        if (update) {
            const u32 p = load<u32>(ub + kUbPlayer);
            u32 r = 0;
            if (p != 0) r = static_cast<u32>(call(ctx, kObjNotify26, {p}));
            bool counters = (load<u32>(s + kSesFlags) & kSesFlagCounters) != 0;
            if (counters && r != 0) counters = call(ctx, kObjField244Clear, {r}) != 0;
            if (counters) {
                store<u32>(cnt, 0);
                store<u32>(cnt + 4, 0);
                store<u32>(cnt + 8, 0);
                u32 o = 0;
                if (load<u8>(e + kEnKind) == kKindCounter && hevent::vcall(ctx, e, kEnVtCounter, {e, cnt}) != 0 &&
                    load<u32>(cnt) != 0) {
                    store<u32>(key, load<u32>(cnt));
                    o = load<u32>(static_cast<u32>(call(ctx, kSortedMapFind3, {hevent::kHandleMap, key})));
                }
                if (o != 0 && (load<u32>(o + kObjLive) & 1) != 0) {
                    u32 text, second = 0;
                    if (load<u32>(cnt + 8) == 0) {
                        text = static_cast<u32>(call(ctx, kStringFormat, {kFmtD, load<u32>(cnt + 4)}));
                    } else {
                        const u32 sub = load<u32>(o + kObjSub);
                        if (sub != 0 && load<u32>(sub + kSub152) > 1) {
                            text = static_cast<u32>(call(ctx, kStringFormat, {kFmtD, load<u32>(cnt + 4)}));
                            second = static_cast<u32>(call(ctx, kStringFormat, {kFmtD, load<u32>(cnt + 8)}));
                        } else {
                            u32 h = load<u32>(o + kObjHash);
                            if (h == 0) h = sub != 0 ? load<u32>(sub + kSubHash) : 0;
                            call(ctx, kHashToString, {h});  // result unused
                            text = static_cast<u32>(call(ctx, kStringFormat, {kFmtDD, load<u32>(cnt + 4), load<u32>(cnt + 8)}));
                        }
                    }
                    if ((load<u32>(o + kObjFlags396) & kObj396Text) != 0) {
                        call(ctx, kObjSetText, {o, text, 0});
                    } else {
                        callf(ctx, kObjSetAnim, {o, text, 1, kAnimRate, 0, 0, kAnim9}, {hevent7::lfs_f64(kAnimRate)});
                    }
                    if (second != 0) call(ctx, kPlayerSet, {o, second, 1});
                }
            }
            for (u32 k = 0; k < 24; k += 4) store<u32>(cx + k, 0);
            callf(ctx, kCtxInit4651A0, {cx, load<u32>(ub + kUbPlayer)}, {dt});
            hevent::vcall(ctx, e, kEnVtUpdate, {e, 0, cx});
        }
        if (load<u16>(load<u32>(hevent::kCurrentWorld) + kWSceneCount) != 0) continue;
        const u32 v = load<u32>(st);
        if ((v & kStPrepared) == 0) continue;
        store<u32>(st, (v & ~kStPrepared) | kStStarted);
        if (load<u8>(e + kEnKind) != kKindDone) continue;
        store<u32>(cnt2, 0);
        store<u32>(cnt2 + 4, 0);
        store<u32>(cnt2 + 8, 0);
        if (hevent::vcall(ctx, e, kEnVtCounter, {e, cnt2}) != 0) call(ctx, kHandleNotify, {load<u32>(cnt2), 1, 1, 0});
        if (started) continue;
        // The first waiting entry whose awaited entries (found ones) are done.
        for (u32 j = 0; j < load<u16>(ub + kUbEntryCount); ++j) {
            const u32 data = load<u32>(ub + kUbEntries);
            const u32 n = load<u16>(ub + kUbEntryCount);
            const u32 e2 = load<u32>(data + 4 * j);
            if (load<u8>(e2 + kEnKind) >= kKindCounter || (load<u32>(e2 + kEnFlags) & kEnFlagWaits) == 0) continue;
            bool all = true;
            for (u32 k = 0; k < kNumWaits; ++k) {
                const u32 h = load<u32>(e2 + kEnWaits + 4 * k);
                if (h == 0) break;
                for (u32 m = 0; m < n; ++m) {
                    if (load<u32>(load<u32>(data + 4 * m) + kEnHash) != h) continue;
                    if (load<u8>(load<u32>(data + 4 * m) + kEnKind) != kKindDone) all = false;
                    break;
                }
                if (!all) break;
            }
            if (!all) continue;
            const u32 lv = static_cast<u32>(hevent::vcall(ctx, e2, kEnVtLevel, {e2}));
            if (lv == 0 || static_cast<u32>(hevent::vcall(ctx, e2, kEnVtLevel, {e2})) == load<u32>(load<u32>(hevent::kCurrentWorld) + kWLevel)) {
                hevent::vcall(ctx, e2, kEnVt40, {e2, load<u32>(e2 + kEnFlags) & 0xFFFFFFF8u});
                hevent::vcall(ctx, e2, kEnVt24, {e2, 0, 0xFFFFFFFFu});
                hevent::vcall(ctx, e2, kEnVt36, {e2, 0});
            }
            store<u32>(s + kSesCurrent, e2);
            started = true;
            call(ctx, kSessionStart2E3AEC, {s, e2, 0});
            store<u32>(s + kSesFlags, load<u32>(s + kSesFlags) | kSesFlagStarted);
            const u32 w = load<u32>(hevent::kCurrentWorld);
            const u32 u = load<u8>(w + hevent::kUserBlocks);
            call(ctx, kSave0AE418, {w + u * hevent::kSaveSlotSize + hevent::kSaveData, w + 2 * u + 4137});
            const u32 x = load<u32>(load<u32>(hevent::kCurrentWorld) + kW1281F4);
            if (x != 0) hevent::vcall(ctx, x, 72, {x});
            break;
        }
    }
    u32 f = load<u32>(s + kSesFlags);
    if ((f & kSesFlagStarted) == 0) return ret(ctx, f);
    const u32 w = load<u32>(hevent::kCurrentWorld);
    if ((load<u8>(w + kWFlags44) & kWFlag44Bit7) != 0 && load<u32>(w + kW72) == 0) {
        store<u32>(w + kW72, 2);
        f = load<u32>(s + kSesFlags);
    }
    f &= ~kSesFlagStarted;
    store<u32>(s + kSesFlags, f);
    ret(ctx, f);
}
