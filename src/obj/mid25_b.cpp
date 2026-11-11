// Game code, medium sweep part 25: the object reset handlers of the
// 0x0036xxxx-0x0038xxxx scripted objects that re-place the object at the
// default position, clear their mode state and, when flagged, queue the
// object in the 0x0106C240 {key, Object*} map.
#include "mid25.h"

using namespace sly4;
using namespace sly4::mid25;

namespace {
constexpr u32 kSortedMapFind3 = 0x0076611Cu;  // SortedMap_Find3(map, &key) -> slot
constexpr u32 kArrayReserve = 0x00558F3Cu;    // Array_Reserve(Array*, u16 capacity)
constexpr u32 kObjPlace = 0x003626E8u;        // Obj_Place3626E8(Object*, const vec3*, u8 snap)
constexpr u32 kDefaultPos = 0x01164B40u;      // f32[3] default position
constexpr u32 kQueueMap = 0x0106C240u;        // {u16 cap, u16 count, u16 high, u16 flags, data +8} of {key, Object*}
constexpr u32 kPending = 0x0106C2C0u;         // {Object** data, ..., u16 count +8} of pending objects
constexpr u32 kMinusOne = 0xBF800000u;        // f32 -1.0 bits

// o->vt[20](o, 0); Obj_Place3626E8(o, the default position, 1).
void PlaceDefault(PPUContext& ctx, u32 o) {
    VCall(ctx, o, 20, {o, 0});
    call(ctx, kObjPlace, {o, kDefaultPos, 1});
}

// The object's key: o->+12, else its def's (o->+8) +68, else 0.
u32 ObjKey(u32 o) {
    if (const u32 k = load<u32>(o + 12); k != 0) return k;
    const u32 d = load<u32>(o + 8);
    return d != 0 ? load<u32>(d + 68) : 0;
}

// With bit 0x10000000 of the flags word f (and o set): unless the object's
// key is already in the 0x0106C240 map, {key (re-read), o} is appended to
// it (grown to twice its u16 capacity by Array_Reserve when full; count +2,
// high-water +4, +6 |= 1).
void QueueIfFlagged(PPUContext& ctx, u32 o, u32 f) {
    if ((f & 0x10000000u) == 0 || o == 0) return;
    {
        Stack frame(ctx, 16);
        store<u32>(frame.addr(), ObjKey(o));
        const u32 slot = static_cast<u32>(call(ctx, kSortedMapFind3, {kQueueMap, frame.addr()}));
        if (load<u32>(slot) != 0) return;
    }
    const u32 k = ObjKey(o);
    const u32 cap = load<u16>(kQueueMap);
    if (load<u16>(kQueueMap + 2) == cap) call(ctx, kArrayReserve, {kQueueMap, (cap << 1) & 0xFFFEu});
    const u32 data = load<u32>(kQueueMap + 8);
    store<u32>(data + 8 * load<u16>(kQueueMap + 2), k);
    store<u32>(data + 8 * load<u16>(kQueueMap + 2) + 4, o);
    const u32 n = load<u16>(kQueueMap + 2) + 1;
    const u32 hi = load<u16>(kQueueMap + 4);
    const u32 fl = load<u16>(kQueueMap + 6);
    store<u16>(kQueueMap + 2, static_cast<u16>(n));
    store<u16>(kQueueMap + 4, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFF) ? hi : n));
    store<u16>(kQueueMap + 6, static_cast<u16>(fl | 1));
}

// o is taken out of the pending list 0x0106C2C0 (searched from the end; the
// later entries move down one, the u16 count +8 decreases). r3 = the new
// count when found, else -1.
void Unpend(PPUContext& ctx, u32 o) {
    const u32 cnt = load<u16>(kPending + 8);
    for (s32 i = static_cast<s32>(cnt) - 1; i >= 0; --i) {
        if (load<u32>(load<u32>(kPending) + 4 * static_cast<u32>(i)) != o) continue;
        u32 c = cnt;
        if (static_cast<u32>(i) != cnt - 1) {
            for (u32 j = static_cast<u32>(i) & 0xFFFF; static_cast<s32>(j) < static_cast<s32>(c - 1); j = (j + 1) & 0xFFFF) {
                const u32 d = load<u32>(kPending);
                store<u32>(d + 4 * j, load<u32>(d + 4 * j + 4));
                c = load<u16>(kPending + 8);
            }
        }
        store<u16>(kPending + 8, static_cast<u16>(c - 1));
        ctx.gpr[3] = c - 1;
        return;
    }
    ctx.gpr[3] = 0xFFFFFFFFFFFFFFFFull;
}

// The new flags word: bits m cleared; then with bit 0x20000000 set both it
// and 0x40000000 are cleared, else bit 0x40000000 takes bit 0x80000000.
u32 ClearMode(u32 f, u32 m = 0x0D800000u) {
    const u32 r = f & ~m;
    if ((r & 0x20000000u) != 0) return r & ~0x60000000u;
    return (r & ~0x40000000u) | ((r >> 1) & 0x40000000u);
}

// e->vt[40](e, 0, 1) when e is set.
void Off40(PPUContext& ctx, u32 e) {
    if (e != 0) VCall(ctx, e, 40, {e, 0, 1});
}
}  // namespace

// void T::Reset(o) (Obj_Reset_00381758, Thunk_00389C3C): o->vt[20](o, 0),
// placed at the default position; +344 = +340 = 0, +160 keeps bit 0,
// +336 = -1.0, the byte +164 = 0, +316 = 0, flags +396 updated (ClearMode);
// queued in the 0x0106C240 map when flag 0x10000000 is set; then taken out
// of the pending list. r3 = the new pending count, or -1.
DECOMP(0036BCAC, Obj_Reset_0036BCAC) {
    const u32 o = arg<u32>(ctx, 0);
    PlaceDefault(ctx, o);
    const u32 f = ClearMode(load<u32>(o + 396));
    store<u32>(o + 344, 0);
    store<u32>(o + 340, 0);
    store<u32>(o + 160, load<u32>(o + 160) & 1u);
    store<u32>(o + 336, kMinusOne);
    store<u8>(o + 164, 0);
    store<u32>(o + 316, 0);
    store<u32>(o + 396, f);
    QueueIfFlagged(ctx, o, f);
    Unpend(ctx, o);
}

// void T::Reset(o): o->vt[20](o, 0) (the def o->+8 read before), placed at
// the default position; +248 = +244 = 0, +240 = -1.0, the byte +164 = 0,
// +160 keeps bit 0, flags +252 updated (ClearMode); queued in the 0x0106C240 map
// when flag 0x10000000 is set; +256 = +264 = 0, +260 = def->+152, the u16s
// +408 / +316 = 0; the u16 +296 objects of o->+288 get vt[8]() and are
// cleared, then the count is cleared. r3 = the count read last.
DECOMP(0037965C, Obj_Reset_0037965C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 def = load<u32>(o + 8);
    PlaceDefault(ctx, o);
    const u32 f = ClearMode(load<u32>(o + 252));
    store<u32>(o + 248, 0);
    store<u32>(o + 244, 0);
    store<u32>(o + 240, kMinusOne);
    store<u8>(o + 164, 0);
    store<u32>(o + 160, load<u32>(o + 160) & 1u);
    store<u32>(o + 252, f);
    QueueIfFlagged(ctx, o, f);
    store<u32>(o + 256, 0);
    const u32 n = load<u32>(def + 152);
    store<u32>(o + 264, 0);
    store<u32>(o + 260, n);
    store<u16>(o + 408, 0);
    store<u16>(o + 316, 0);
    u32 i = 0;
    for (;; ++i) {
        const u32 c = load<u16>(o + 296);
        if (i >= c) {
            ctx.gpr[3] = c;
            break;
        }
        const u32 e = load<u32>(load<u32>(o + 288) + 4 * i);
        if (e == 0) continue;
        VCall(ctx, e, 8, {e});
        store<u32>(load<u32>(o + 288) + 4 * i, 0);
    }
    store<u16>(o + 296, 0);
}

// void T::Reset(o): o->vt[20](o, 0), placed at the default position;
// +204 = +200 = 0, +160 keeps bit 0, +196 = -1.0, flags +532 updated (ClearMode, bits
// 0x0CC00000);
// queued in the 0x0106C240 map when flag 0x10000000 is set; then
// vt[40](c, 0, 1) for the child slots +248 / +296 / +344 [12] (interleaved),
// +212 [9], +400 [32], then +396, +528 and +392. r3 = the last call's, or
// 0 when +528 and +392 are both empty.
DECOMP(003842D0, Obj_Reset_003842D0) {
    const u32 o = arg<u32>(ctx, 0);
    PlaceDefault(ctx, o);
    const u32 f = ClearMode(load<u32>(o + 532), 0x0CC00000u);
    store<u32>(o + 204, 0);
    store<u32>(o + 200, 0);
    store<u32>(o + 160, load<u32>(o + 160) & 1u);
    store<u32>(o + 196, kMinusOne);
    store<u32>(o + 532, f);
    QueueIfFlagged(ctx, o, f);
    for (u32 i = 0; i < 12; ++i) {
        Off40(ctx, load<u32>(o + 248 + 4 * i));
        Off40(ctx, load<u32>(o + 296 + 4 * i));
        Off40(ctx, load<u32>(o + 344 + 4 * i));
    }
    for (u32 i = 0; i < 9; ++i) Off40(ctx, load<u32>(o + 212 + 4 * i));
    for (u32 i = 0; i < 32; ++i) Off40(ctx, load<u32>(o + 400 + 4 * i));
    Off40(ctx, load<u32>(o + 396));
    const u32 a = load<u32>(o + 528);
    Off40(ctx, a);
    const u32 b = load<u32>(o + 392);
    Off40(ctx, b);
    if (a == 0 && b == 0) ctx.gpr[3] = 0;
}

// void T::Reset(o): o->vt[20](o, 0), placed at the default position;
// +252 = +248 = 0, +160 keeps bit 0, +244 = -1.0, the byte +164 = 0, flags
// +960 updated (ClearMode); queued in the 0x0106C240 map when flag 0x10000000 is
// set; Obj_BroadcastChildren385F00(o, 0); the emitter +956 is stopped
// (Emitter_Stop(h, 1)) and cleared. r3 = 0.
DECOMP(00387868, Obj_Reset_00387868) {
    const u32 o = arg<u32>(ctx, 0);
    PlaceDefault(ctx, o);
    const u32 f = ClearMode(load<u32>(o + 960));
    store<u32>(o + 252, 0);
    store<u32>(o + 248, 0);
    store<u32>(o + 160, load<u32>(o + 160) & 1u);
    store<u32>(o + 244, kMinusOne);
    store<u8>(o + 164, 0);
    store<u32>(o + 960, f);
    QueueIfFlagged(ctx, o, f);
    call(ctx, 0x00385F00u, {o, 0});
    call(ctx, 0x00443424u, {load<u32>(o + 956), 1});
    store<u32>(o + 956, 0);
    ctx.gpr[3] = 0;
}
