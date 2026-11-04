// World_HandleEvent callees, part 12f: the object notify used by
// Handle_Notify36742C when its b flag is set.
#include "hevent12.h"

using namespace sly4;
using namespace sly4::hevent12;

namespace {
// Object (as hevent12_a.cpp): +0 vtable (+12 activate), +8 Def*, +12 u32
// handle, +24 body (vt +40 (body, 1, 1)), +160 u32 (kind << 1 | bit 0), +328
// 16-byte component records {+4 component (vt +88 (c, bool)), +8 u32}, +336
// f32, +344 u32, +396 state.
constexpr u32 kObDef = 8, kObHandle = 12, kObBody = 24, kObKind = 160, kObComps = 328;
constexpr u32 kOb336 = 336, kOb344 = 344, kObState = 396;
constexpr u32 kObVtActivate = 12, kBodyVtShow = 40, kCompVtEnable = 88;
constexpr u32 kCompRecSize = 16, kCompRecObj = 4, kCompRecArg = 8;
constexpr u32 kStEnabled = 0x40000000u, kStA = 0x20000000u, kStB = 0x10000000u, kStLocked = 0x02000000u;
constexpr u32 kKindActive = 4;
constexpr u32 kMinusOneBits = 0xBF800000;  // -1.0f
// Def: +68 u32 key (when the object has no handle), +148 32-byte records
// (+24 binding source), +152 u32 record count.
constexpr u32 kDefKey = 68, kDefRecs = 148, kDefRecCount = 152, kDefRecSize = 32, kDefRecSource = 24;
constexpr u32 kObjSetState = 0x00362D28;  // Obj_SetState362D28 (o, s32 state, u8)
constexpr u32 kBindClear = 0x004E3168;    // Obj_Set4E3168 (const char* source, u32 key)
constexpr u32 kBindCreate = 0x004E2DB0;   // Obj_Attach4E2DB0 (source, key, u8, u8)
constexpr u32 kScaleName = 0x00ADCAA8;    // "Scale"
constexpr u32 kArrayReserve = 0x00558F3C; // Array_Reserve (Array*, u16 capacity)
// Map of objects waiting to be activated, keyed by handle / def key (a
// sorted pair table: +0 u16 capacity, +2 u16 count, +4 u16 high-water mark,
// +6 u16 flags (bit 0 unsorted), +8 {key, Object*}[]) (name?).
constexpr u32 kPendingMap = 0x0106C240;
constexpr u32 kTabCap = 0, kTabHwm = 4;
}  // namespace

// u32 Obj_NotifyA362F1C(Object* o, u8 a, u8 b) (name?): for an enabled,
// unlocked object (+396): kinds other than 0 and 4 become kind 4 (a / b into
// the state bits, +336 = -1, Obj_SetState 4, body vt +40, components vt +88
// and "Scale" bindings rebuilt from the def's records); a kind-0 object is
// activated (vt +12) when a or its a bit is set, else queued in the pending
// map (once) when b or its b bit is set. Clears +344 and returns 0 (the
// handle when the object is locked or disabled).
DECOMP(00362F1C, Obj_NotifyA362F1C) {
    const u32 o = arg<u32>(ctx, 0), ra = arg<u32>(ctx, 1), rb = arg<u32>(ctx, 2);
    const u32 a = ra & 0xFF, b = rb & 0xFF;
    const u32 st = load<u32>(o + kObState);
    const u32 handle = load<u32>(o + kObHandle);
    if ((st & kStLocked) != 0 || (st & kStEnabled) == 0) return ret(ctx, handle);
    const u32 kv = load<u32>(o + kObKind);
    const u32 d = load<u32>(o + kObDef);
    const u32 kind = kv >> 1;
    if (kind != 0 && kind != kKindActive) {
        store<u32>(o + kObState, (st & ~(kStA | kStB)) | ((ra & 1) << 29) | ((rb & 1) << 28));
        store<u32>(o + kOb336, kMinusOneBits);
        store<u32>(o + kObKind, (kv & 1) | (kKindActive << 1));
        call(ctx, kObjSetState, {o, kKindActive, 0});
        const u32 body = load<u32>(o + kObBody);
        if (body != 0) hevent::vcall(ctx, body, kBodyVtShow, {body, 1, 1});
        if (load<u32>(d + kDefRecCount) != 0) {
            for (u32 i = 0; i < load<u32>(d + kDefRecCount); ++i) {
                const u32 rec = load<u32>(o + kObComps) + i * kCompRecSize;
                const u32 c = load<u32>(rec + kCompRecObj);
                if (c == 0) continue;
                hevent::vcall(ctx, c, kCompVtEnable, {c, load<u32>(rec + kCompRecArg) != 0 ? 1u : 0u});
                if (load<u32>(load<u32>(d + kDefRecs) + i * kDefRecSize + kDefRecSource) == 0) continue;
                call(ctx, kBindClear, {kScaleName, load<u32>(rec + kCompRecObj)});
                call(ctx, kBindCreate, {load<u32>(load<u32>(d + kDefRecs) + i * kDefRecSize + kDefRecSource), load<u32>(rec + kCompRecObj), 1, 0});
            }
        }
    } else if (kind == 0 && ((st & kStA) != 0 || a != 0)) {
        hevent::vcall(ctx, o, kObVtActivate, {o});
    } else if (kind == 0 && ((st & kStB) != 0 || b != 0) && o != 0) {
        const u32 key = handle != 0 ? handle : (d != 0 ? load<u32>(d + kDefKey) : 0u);
        u32 found;
        {
            Stack frame(ctx, 16);
            store<u32>(frame.addr(), key);
            found = load<u32>(static_cast<u32>(call(ctx, hevent11::h11fn::SortedMap_Find3, {kPendingMap, frame.addr(), key})));
        }
        if (found == 0) {
            u32 k = load<u32>(o + kObHandle);
            if (k == 0) {
                const u32 d2 = load<u32>(o + kObDef);
                k = d2 != 0 ? load<u32>(d2 + kDefKey) : 0u;
            }
            const u32 t = kPendingMap;
            const u32 cap = load<u16>(t + kTabCap);
            if (load<u16>(t + hevent9::kTabCount) == cap) call(ctx, kArrayReserve, {t, (cap << 1) & 0xFFFE});
            const u32 data = load<u32>(t + hevent9::kTabEntries);
            store<u32>(data + ((load<u16>(t + hevent9::kTabCount) << 3) & 0x7FFF8), k);
            store<u32>(data + (load<u16>(t + hevent9::kTabCount) << 3) + 4, o);
            const u32 n = load<u16>(t + hevent9::kTabCount) + 1;
            const u32 hwm = load<u16>(t + kTabHwm);
            const u32 flags = load<u16>(t + hevent9::kTabFlags);
            store<u16>(t + hevent9::kTabCount, static_cast<u16>(n));
            store<u16>(t + kTabHwm, static_cast<u16>(static_cast<s32>(hwm) > static_cast<s32>(n & 0xFFFF) ? hwm : n));
            store<u16>(t + hevent9::kTabFlags, static_cast<u16>(flags | hevent9::kTabUnsorted));
        }
    }
    store<u32>(o + kOb344, 0);
    ret(ctx, 0u);
}
