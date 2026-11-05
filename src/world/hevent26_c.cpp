// World_HandleEvent callees, part 26c: the second object class's notify
// (the +960-flags sibling of Obj_NotifyA362F1C).
#include <bit>

#include "hevent22.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kSpeakerSayId = 0x004A5C90;     // (Speaker*, u32 text id, u8; f32) -> bool (Speaker_SayId4A5C90)
constexpr u32 kEffectSetHash = 0x004CF988;    // (Effect*, u32 hash) -> bool (Effect_SetHash4CF988)
constexpr u32 kBroadcastChildren = 0x00385F00;  // (Object*, u8) (Obj_BroadcastChildren385F00)
constexpr u32 kObjNotifyA = 0x00362F1C;       // (Object*, u8, u8) (Obj_NotifyA362F1C)
constexpr u32 kBindClear = 0x004E3168;        // (const char* source, u32 key) (Obj_Set4E3168)
constexpr u32 kBindCreate = 0x004E2DB0;       // (source, key, u8, u8) (Obj_Attach4E2DB0)
constexpr u32 kSortedMapFind3 = 0x0076611C;   // (map, const u32* key) -> value slot
constexpr u32 kScaleName = 0x00ADCAA8;        // source name given to the binding calls (as hevent12_f.cpp)
constexpr u32 kPendingMap = 0x0106C240;       // {handle, Object*} map of objects waiting for activation
constexpr u32 kNotifyHandle = 0x00F64CE4;     // u32 handle of an object notified after the switch (name?)
constexpr u32 kSayVolume = 0x0038613C;        // f32 1.0
constexpr u32 kModeHash386140 = 0x00F65034;   // u32 effect hash (0x00F65018 +28)

// Object (this class): +8 Def*, +12 handle, +20 Effect*, +24 body, +28
// Speaker*, +160 bit 0 kept / bits 1..31 kind, +228 text id, +236 16-byte
// component records {+4 Object*, +8 u32 arg}, +244 f32, +252 u32, +960
// flags. Def: +68 key, +148 36-byte records (+24 binding source), +152
// count.
constexpr u32 kObDef = 8, kObHandle = 12, kObEffect = 20, kObBody = 24, kObSpeaker = 28, kObKind = 160;
constexpr u32 kObTextId = 228, kObComps = 236, kOb244 = 244, kOb252 = 252, kObFlags = 960;
constexpr u32 kFlEnabled = 0x40000000u, kFlA = 0x20000000u, kFlB = 0x10000000u, kFlLocked = 0x02000000u,
              kFlSaid = 0x01000000u, kFlEffect = 0x00800000u;
constexpr u32 kKindActive = 7;
constexpr u32 kDefKey = 68, kDefRecs = 148, kDefRecCount = 152, kDefRecSize = 36, kDefRecSource = 24;
constexpr u32 kCompRecSize = 16, kCompRecObj = 4, kCompRecArg = 8;
}  // namespace

// ---------------------------------------------------------------- 0x00386140
// u32 Obj_NotifyB386140(Object* o, u8 a, u8 b) (name?): nothing (returns
// +12) when +960 bit 25 is set or bit 30 clear. Kinds other than 0 / 7:
// +960 bits 29 / 28 = a / b, +244 = -1.0f, kind 7, the speaker says the
// +228 text (bit 24 = said), the +20 effect gets hash *0x00F65034 (bit 23),
// body vt +40 (1, 1), Obj_BroadcastChildren385F00(o, 0), each component vt
// +88 (arg != 0) and its def binding re-created; then the object of handle
// *0x00F64CE4 (if live, +4 bit 0, and +396 bit 25 clear) gets
// Obj_NotifyA362F1C(.., 0, 0). Kind 0: with bit 29 or a: vt +12; else
// with bit 28 or b: queued in 0x0106C240 under its handle (or def +68)
// unless already there. +252 = 0, returns 0.
DECOMP(00386140, Obj_NotifyB386140) {
    const u32 o = arg<u32>(ctx, 0), ra = arg<u32>(ctx, 1), rb = arg<u32>(ctx, 2);
    const u32 a = ra & 0xFF, b = rb & 0xFF;
    const u32 fl = load<u32>(o + kObFlags);
    const u32 vt = load<u32>(o);
    const u32 handle = load<u32>(o + kObHandle);
    if ((fl & kFlLocked) != 0 || (fl & kFlEnabled) == 0) return ret(ctx, handle);
    const u32 kv = load<u32>(o + kObKind);
    const u32 d = load<u32>(o + kObDef);
    const u32 kind = kv >> 1;
    if (kind != 0 && kind != kKindActive) {
        store<u32>(o + kObFlags, (fl & ~(kFlA | kFlB)) | ((ra & 1) << 29) | ((rb & 1) << 28));
        store<u32>(o + kOb244, 0xBF800000u);  // -1.0f
        store<u32>(o + kObKind, (kv & 1) | (kKindActive << 1));
        const f64 vol = static_cast<f64>(std::bit_cast<f32>(load<u32>(kSayVolume)));
        callf(ctx, kSpeakerSayId, {load<u32>(o + kObSpeaker), load<u32>(o + kObTextId), 0}, {vol});
        const u32 said = static_cast<u32>(ctx.gpr[3]);
        const u32 eff = load<u32>(o + kObEffect);
        store<u32>(o + kObFlags, (load<u32>(o + kObFlags) & ~kFlSaid) | ((said & 1) << 24));
        if (eff != 0) {
            const u32 r = static_cast<u32>(call(ctx, kEffectSetHash, {eff, load<u32>(kModeHash386140)}));
            store<u32>(o + kObFlags, (load<u32>(o + kObFlags) & ~kFlEffect) | ((r & 1) << 23));
        }
        const u32 body = load<u32>(o + kObBody);
        if (body != 0) vcall(ctx, body, 40, {body, 1, 1});
        call(ctx, kBroadcastChildren, {o, 0});
        if (load<u32>(d + kDefRecCount) != 0) {
            u32 i = 0;
            do {
                const u32 rec = load<u32>(o + kObComps) + i * kCompRecSize;
                const u32 c = load<u32>(rec + kCompRecObj);
                if (c != 0) {
                    const u32 opd = load<u32>(load<u32>(c) + 88);
                    const u32 on = load<u32>(rec + kCompRecArg) != 0 ? 1u : 0u;
                    ctx.gpr[5] = load<u32>(opd);  // as the original sequence: entry in r5, OPD in r6
                    ctx.gpr[6] = opd;
                    call_ptr(ctx, opd, {c, on});
                    if (load<u32>(load<u32>(d + kDefRecs) + i * kDefRecSize + kDefRecSource) != 0) {
                        call(ctx, kBindClear, {kScaleName, load<u32>(rec + kCompRecObj)});
                        call(ctx, kBindCreate, {load<u32>(load<u32>(d + kDefRecs) + i * kDefRecSize + kDefRecSource), load<u32>(rec + kCompRecObj), 1, 0});
                    }
                }
                ++i;
            } while (i < load<u32>(d + kDefRecCount));
        }
        const u32 h = load<u32>(kNotifyHandle);
        if (h != 0) {
            Stack frame(ctx, 16);
            store<u32>(frame.addr() + 4, h);
            const u32 x = load<u32>(static_cast<u32>(call(ctx, kSortedMapFind3, {kHandleMap, frame.addr() + 4, h})));
            if (x != 0 && (load<u32>(x + 4) & 1u) != 0 && ((load<u32>(x + 396) >> 25) & 1u) == 0) call(ctx, kObjNotifyA, {x, 0, 0});
        }
    } else if (kind == 0 && ((fl & kFlA) != 0 || a != 0)) {
        const u32 opd = load<u32>(vt + 12);
        ctx.gpr[4] = opd;
        ctx.gpr[5] = load<u32>(opd);
        call_ptr(ctx, opd, {o});
    } else if (kind == 0 && ((fl & kFlB) != 0 || b != 0) && o != 0) {
        const u32 key = handle != 0 ? handle : (d != 0 ? load<u32>(d + kDefKey) : 0u);
        u32 found;
        {
            Stack frame(ctx, 16);
            store<u32>(frame.addr(), key);
            found = load<u32>(static_cast<u32>(call(ctx, kSortedMapFind3, {kPendingMap, frame.addr(), key})));
        }
        if (found == 0) {
            u32 k = load<u32>(o + kObHandle);
            if (k == 0) {
                const u32 d2 = load<u32>(o + kObDef);
                k = d2 != 0 ? load<u32>(d2 + kDefKey) : 0u;
            }
            hevent22::map_append(ctx, kPendingMap, k, o);
        }
    }
    store<u32>(o + kOb252, 0);
    ret(ctx, 0u);
}
