// World_HandleEvent callees, part 10b: probe switch, hashed-object flush,
// volume refresh and the layer object queue.
#include "hevent10.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent10;

namespace sly4 {
namespace hevent10b {
namespace bfn10 {
constexpr u32 Obj_Prepare3629D8 = 0x003629D8;     // (Object*): leaves its kind list
constexpr u32 Obj_AddToList36341C = 0x0036341C;   // (Object*)
constexpr u32 Hashed_Make = 0x003651F0;           // (u32 hash) -> Object*
constexpr u32 SortedMap_Remove = 0x00771DD0;      // (map, const u32* key)
constexpr u32 Effect_SetHash4CF988 = 0x004CF988;  // (effect, u32 hash) (as hevent.h sub_004CF988)
constexpr u32 Volume_Item3992AC = 0x003992AC;     // (item, u8, u8, u8) (name?)
constexpr u32 Volume_Notify112D8 = 0x000112D8;    // (listener, Volume*) (name?)
constexpr u32 Volume_Changed398FF8 = 0x00398FF8;  // (Volume*) (name?)
}  // namespace bfn10

// Probe: +0 Child*[] (+8 u16 count), +328 / +332 switchables (vtable +304:
// (obj, u8 on)), +340 effect, +356 flags (bit 31: on). Child vtable +40:
// (child, u8 on, 1).
constexpr u32 kProbeChildren = 0, kProbeChildCount = 8, kProbeSw0 = 328, kProbeSw1 = 332;
constexpr u32 kProbeEffect = 340, kProbeFlags = 356;
constexpr u32 kVSwitch = 304, kVChildOn = 40;
constexpr u32 kHashOn = 0xCADB1721, kHashOff = 0x3ECC2A7C;

// Hashed object pending a flush: the object, the hash it should have and a
// word mirrored into the object (+168, bit 7 of +172 = non-zero).
constexpr u32 kFlushObj = 0x00F64CB8;    // Object* (name?)
constexpr u32 kFlushHash = 0x0106C270;   // u32 (as hevent9_i.cpp kFlush365360)
constexpr u32 kFlushWord = 0x00F64CB4;   // u32 (as hevent9_i.cpp kFlag_F64CB4)
constexpr u32 kObjFlushWord = 168, kObjFlushBits = 172;
// Object handle: +12, else the def's (+8, +68).
constexpr u32 kObjDef = 8, kObjHandle = 12, kDefHandle = 68;
constexpr u32 kVRelease = 4;

// Volume: +64 owner (+12 u8 per-user flags, 4 apart), +72 flags (bit 30:
// on), +78 u16 item count, +84 8-byte items {+4 s16 index}. Item i lives at
// (*0x00F657FC)->+28 + 336 * index.
constexpr u32 kVolOwner = 64, kVolFlags = 72, kVolItemCount = 78, kVolItems = 84;
constexpr u32 kVolOn = 0x40000000u;
constexpr u32 kVolItemSize = 8, kVolItemIndex = 4;
constexpr u32 kItemTable = 0x00F657FC, kItemTableData = 28, kItemSize = 336;
constexpr u32 kOwnerFlags = 12;
constexpr u32 kUserState = 0x010AB5A8;   // +16 -> +64 u32 current user index (name?)
constexpr u32 kVolListener = 0x00F557EC; // u32 (name?)

// Layer (as hevent9_f.cpp): +4 Obj** queue (+12 u16 count), +32 f32 time.
// Queued object: +8 flags (bit 31: has update, bit 30: started); vtable +4
// release, +8 start, +12 update(dt) -> done.
constexpr u32 kLayerQueue = 4, kLayerQueueCount = 12, kLayerTime = 32;
constexpr u32 kQFlags = 8, kQHasUpdate = 0x80000000u, kQStarted = 0x40000000u;
constexpr u32 kQVRelease = 4, kQVStart = 8, kQVUpdate = 12;
}  // namespace hevent10b
}  // namespace sly4

using namespace sly4::hevent10b;

// void Probe_SetOn39D1D4(Probe* p, u8 on) (name?): switches both
// switchables (+328, +332, vtable +304), gives the effect (+340) the on / off
// hash, sets flag bit 31 to on's bit 0, then every child (vtable +40, (on,
// 1)). r3 as the original leaves it (the child count).
DECOMP(0039D1D4, Probe_SetOn39D1D4) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 raw = arg<u32>(ctx, 1);
    const u32 on = raw & 0xFF;
    for (const u32 off : {kProbeSw0, kProbeSw1}) {
        const u32 s = load<u32>(p + off);
        if (s != 0) vcall(ctx, s, kVSwitch, {s, on});
    }
    const u32 eff = load<u32>(p + kProbeEffect);
    if (eff != 0) call(ctx, bfn10::Effect_SetHash4CF988, {eff, on != 0 ? kHashOn : kHashOff});
    // rlwimi from the full register: only bit 0 of the argument counts.
    store<u32>(p + kProbeFlags, (load<u32>(p + kProbeFlags) & 0x7FFFFFFFu) | ((raw & 1u) << 31));
    u32 n;
    for (u32 i = 0;; ++i) {
        n = load<u16>(p + kProbeChildCount);
        if (!(i < n)) break;
        const u32 c = load<u32>(load<u32>(p + kProbeChildren) + i * 4);
        if (c != 0) vcall(ctx, c, kVChildOn, {c, on, 1u});
    }
    ctx.gpr[3] = n;
}

namespace {
u32 handle_of(u32 o) {
    const u32 h = load<u32>(o + kObjHandle);
    if (h != 0) return h;
    const u32 def = load<u32>(o + kObjDef);
    return def != 0 ? load<u32>(def + kDefHandle) : 0;
}
}  // namespace

// Object* Objects_Flush365360() (name?): when the pending object's handle
// differs from the wanted hash it leaves its kind list, its handle is
// removed from the handle map, it is released (vtable +4) and forgotten; a
// missing object is (re)made with Hashed_Make(hash) and listed. The object
// left gets bit 7 of +172 = (word != 0) and +168 = word. Returns it (or 0).
DECOMP(00365360, Objects_Flush365360) {
    u32 o = load<u32>(kFlushObj);
    bool make = o == 0;
    if (!make && load<u32>(kFlushHash) != handle_of(o)) {
        call(ctx, bfn10::Obj_Prepare3629D8, {o});
        Stack frame(ctx, 16);
        store<u32>(frame.addr(), handle_of(load<u32>(kFlushObj)));
        call(ctx, bfn10::SortedMap_Remove, {kHandleMap, frame.addr()});
        const u32 cur = load<u32>(kFlushObj);
        if (cur != 0) {
            vcall(ctx, cur, kVRelease, {cur});
            store<u32>(kFlushObj, 0);
        }
        make = true;
    }
    if (make) {
        o = static_cast<u32>(call(ctx, bfn10::Hashed_Make, {load<u32>(kFlushHash)}));
        store<u32>(kFlushObj, o);
        if (o != 0) {
            call(ctx, bfn10::Obj_AddToList36341C, {o});
            o = load<u32>(kFlushObj);
        }
    }
    if (o != 0) {
        const u8 b = load<u8>(o + kObjFlushBits);
        store<u8>(o + kObjFlushBits, static_cast<u8>((b & 0x7F) | (load<u32>(kFlushWord) != 0 ? 0x80 : 0)));
        o = load<u32>(kFlushObj);
        store<u32>(o + kObjFlushWord, load<u32>(kFlushWord));
    }
    ret(ctx, o);
}

// u32 Volume_Refresh399758(Volume* v, u8 on, u8 forced, u8 c) (name?):
// unless forced, nothing happens while v's on bit (+72 bit 30) already
// equals on. Otherwise the bit takes on's bit 0, every listed item (index
// != -1, non-null) gets 0x003992AC(item, on, forced, c), the listener at
// 0x00F557EC hears of v when its owner's flag for the current user is set,
// and an on volume finishes with 0x00398FF8(v). r3 as the original leaves it.
DECOMP(00399758, Volume_Refresh399758) {
    const u32 v = arg<u32>(ctx, 0);
    const u32 raw = arg<u32>(ctx, 1);
    const u32 on = raw & 0xFF, forced = arg<u32>(ctx, 2) & 0xFF, c = arg<u32>(ctx, 3) & 0xFF;
    if (forced == 0) {
        const u32 bit = (load<u32>(v + kVolFlags) & kVolOn) != 0 ? 1 : 0;
        if (bit == on) {
            ctx.gpr[3] = bit;
            return;
        }
    }
    // rlwimi from the full register: only bit 0 of the argument counts.
    store<u32>(v + kVolFlags, (load<u32>(v + kVolFlags) & ~kVolOn) | ((raw & 1u) << 30));
    for (u32 i = 0; i < load<u16>(v + kVolItemCount); ++i) {
        const s32 idx = static_cast<s16>(load<u16>(load<u32>(v + kVolItems) + i * kVolItemSize + kVolItemIndex));
        const u32 table = load<u32>(kItemTable);
        if (idx == -1) continue;
        const u32 item = load<u32>(table + kItemTableData) + static_cast<u32>(idx) * kItemSize;
        if (item != 0) call(ctx, bfn10::Volume_Item3992AC, {item, on, forced, c});
    }
    const u32 owner = load<u32>(v + kVolOwner);
    if (owner != 0) {
        const u32 user = load<u32>(load<u32>(kUserState + 16) + 64);
        if (load<u8>(load<u32>(owner + kOwnerFlags) + user * 4) != 0)
            call(ctx, bfn10::Volume_Notify112D8, {load<u32>(kVolListener), v});
    }
    u64 r3 = 0;
    if ((load<u32>(v + kVolFlags) & kVolOn) != 0) r3 = call(ctx, bfn10::Volume_Changed398FF8, {v});
    ctx.gpr[3] = r3;
}

// void Layer_Update42B5B0(Layer* l, f32 dt) (name?): advances the layer's
// time by dt, then works the object queue from the front: an object is
// started once (vtable +8, unless bit 30 is set); one with an update (bit
// 31) stays at the front while update(dt) (vtable +12) returns 0; otherwise
// it is released (vtable +4, when non-null) and the queue shifted down by
// one. Returns 0.
DECOMP(0042B5B0, Layer_Update42B5B0) {
    const u32 l = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const f32 t = static_cast<f32>(lfs_f64(l + kLayerTime) + dt);
    store<f32>(l + kLayerTime, t);
    ctx.fpr[1] = t;
    while (load<u16>(l + kLayerQueueCount) != 0) {
        const u32 o = load<u32>(load<u32>(l + kLayerQueue));
        u32 f = load<u32>(o + kQFlags);
        if ((f & kQStarted) == 0) {
            vcall(ctx, o, kQVStart, {o});
            f = load<u32>(o + kQFlags);
        }
        if ((f & kQHasUpdate) != 0) {
            const u32 opd = load<u32>(load<u32>(o) + kQVUpdate);
            ctx.gpr[4] = opd;
            ctx.gpr[5] = load<u32>(opd);
            ctx.fpr[1] = dt;
            if (static_cast<u32>(call_ptr(ctx, opd, {o})) == 0) break;
        }
        if (o != 0) vcall(ctx, o, kQVRelease, {o});
        for (u32 i = 0; i < static_cast<u32>(load<u16>(l + kLayerQueueCount)) - 1; ++i) {
            const u32 d = load<u32>(l + kLayerQueue);
            store<u32>(d + i * 4, load<u32>(d + (i + 1) * 4));
        }
        const u32 m = static_cast<u32>(load<u16>(l + kLayerQueueCount)) - 1;
        store<u16>(l + kLayerQueueCount, static_cast<u16>(m));
        // Only reachable with a count of 0 here (a release emptied the
        // queue): the original then copies entry 0xFFFF to entry -1.
        if (m != (m & 0xFFFF)) {
            const u32 d = load<u32>(l + kLayerQueue);
            store<u32>(d + m * 4, load<u32>(d + (m & 0xFFFF) * 4));
        }
    }
    ret(ctx, 0u);
}
