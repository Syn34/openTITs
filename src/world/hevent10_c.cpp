// World_HandleEvent callees, part 10c: probe reset, track group release,
// layer removal and the track clock.
#include "hevent10.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent10;

namespace sly4 {
namespace hevent10c {
namespace cfn10 {
constexpr u32 Effect_SetHash4CF988 = 0x004CF988;  // (effect, u32 hash) (as hevent.h sub_004CF988)
constexpr u32 Body_Remove931F00 = 0x00931F00;     // (world, body): Havok-range code (name?)
constexpr u32 PtrVector_Clear = 0x0054EC1C;
constexpr u32 Nop = 0x0054EC18;
constexpr u32 SmallFree = 0x00552420;             // (void* p, u32 size)
constexpr u32 Layer_Removed42B420 = 0x0042B420;
constexpr u32 Tracks_Reset320190 = 0x00320190;    // (Set*, u8) (name?)
}  // namespace cfn10

// Probe (as hevent10_b.cpp): +0 Child*[] (+8 u16 count), +20 Part*[] (+28
// u16 count), +316 s32 mode, +340 effect, +344 / +348 f32, +356 flags (bit
// 31: on, bit 28: prepared). Child vtable +40: (child, u8 on, 1); part
// vtable +272: () -> effect or 0.
constexpr u32 kPrChildren = 0, kPrChildCount = 8, kPrParts = 20, kPrPartCount = 28;
constexpr u32 kPrMode = 316, kPrEffect = 340, kPrF344 = 344, kPrF348 = 348, kPrFlags = 356;
constexpr u32 kPrOn = 0x80000000u, kPrPrepared = 0x10000000u;
constexpr u32 kVChildOn = 40, kVPartEffect = 272;
constexpr u32 kHashPart = 0x942B571C, kHashProbe = 0x066F8774;
// Two tuning blocks whose +72 singles seed +344 / +348 (name?).
constexpr u32 kTune106D584 = 0x0106D584, kTune106D5D0 = 0x0106D5D0, kTuneValue = 72;

// Track group (44-byte block): +0 Sub*[] (+8 u16 count), +28 / +32 / +36
// u32, +40 u32. Sub: +0 Child*[] (+8 u16 count), +20 32-byte records (+28
// u16 count) {+16 Obj*}. Obj +416 -> {+8 world}; Obj vtable +304: (obj,
// u8 on).
constexpr u32 kGrSubs = 0, kGrSubCount = 8, kGr28 = 28, kGr32 = 32, kGr36 = 36, kGr40 = 40;
constexpr u32 kSubChildren = 0, kSubChildCount = 8, kSubRecs = 20, kSubRecCount = 28;
constexpr u32 kRecSize = 32, kRecObj = 16, kObjBody = 416, kBodyWorld = 8;
constexpr u32 kVSwitch = 304;

// The layer PtrVector (as hevent9_f.cpp kLayers): +0 Layer**, +8 u16 count.
// Layer: +4 PtrVector of objects (+12 u16 count), +28 parent layer.
constexpr u32 kLayers = 0x010AA694, kLayerParent = 28, kLayerObjs = 4, kLayerObjCount = 12;
constexpr u32 kLayerSize = 36;

// Track set: +2 u16 item count, +8 8-byte items {+4 Item*}, +48 owner
// (+8 -> group table, 8-byte groups {+0 -> {+0 8-byte keys {+4 f32 time},
// +8 u16 count}}), +52 u32 group, +56 u32 key, +60 f32 time left.
// Item: +24 -> {+12 u8}, +28 flags (bit 31 mirrors that byte's bit 0).
constexpr u32 kTsItemCount = 2, kTsItems = 8, kTsOwner = 48, kTsGroup = 52, kTsKey = 56, kTsTime = 60;
constexpr u32 kTsItemPtr = 4, kItemSrc = 24, kItemFlags = 28, kSrcBit = 12;
constexpr u32 kZero320704 = 0x00320704;  // f32 0.0 (in the code segment)
}  // namespace hevent10c
}  // namespace sly4

using namespace sly4::hevent10c;

// Effect* Probe_Prepare39D054(Probe* p) (name?): marks p prepared (+356 bit
// 28); in mode 1 it also clears the on bit, switches every child off
// (vtable +40, (0, 1)), gives each part's effect (part vtable +272) and
// p's own effect their hashes and reloads +344 / +348 from the tuning
// blocks. r3 / f1 as the original leaves them.
DECOMP(0039D054, Probe_Prepare39D054) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 f = load<u32>(p + kPrFlags) | kPrPrepared;
    store<u32>(p + kPrFlags, f);
    if (static_cast<s32>(load<u32>(p + kPrMode)) != 1) {
        ctx.gpr[3] = p;
        return;
    }
    store<u32>(p + kPrFlags, f & ~kPrOn);
    for (u32 i = 0; i < load<u16>(p + kPrChildCount); ++i) {
        const u32 c = load<u32>(load<u32>(p + kPrChildren) + i * 4);
        if (c != 0) vcall(ctx, c, kVChildOn, {c, 0u, 1u});
    }
    for (u32 i = 0; i < load<u16>(p + kPrPartCount); ++i) {
        const u32 part = load<u32>(load<u32>(p + kPrParts) + i * 4);
        if (part == 0) continue;
        const u32 eff = static_cast<u32>(vcall(ctx, part, kVPartEffect, {part}));
        if (eff != 0) call(ctx, cfn10::Effect_SetHash4CF988, {eff, kHashPart});
    }
    const u32 eff = load<u32>(p + kPrEffect);
    if (eff != 0) call(ctx, cfn10::Effect_SetHash4CF988, {eff, kHashProbe});
    store<u32>(p + kPrF344, load<u32>(kTune106D584 + kTuneValue));
    store<u32>(p + kPrF348, load<u32>(kTune106D5D0 + kTuneValue));
    ctx.fpr[1] = lfs_f64(kTune106D5D0 + kTuneValue);
    ctx.gpr[3] = kTune106D584;
}

// u32 Group_Free31DB24(Group* g) (name?): for every sub-group, switches its
// children off (vtable +40, (0, 1)) and, for each record, removes the
// object's body from its world (0x00931F00, when it has one) and switches
// the object off (vtable +304, 0). Then the sub count, +28, +32 and +36 are
// cleared and +40 set to 1. Returns 1.
DECOMP(0031DB24, Group_Free31DB24) {
    const u32 g = arg<u32>(ctx, 0);
    for (u32 i = 0; i < load<u16>(g + kGrSubCount); ++i) {
        const u32 sub = load<u32>(load<u32>(g + kGrSubs) + i * 4);
        if (sub == 0) continue;
        for (u32 j = 0; j < load<u16>(sub + kSubChildCount); ++j) {
            const u32 c = load<u32>(load<u32>(sub + kSubChildren) + j * 4);
            if (c != 0) vcall(ctx, c, kVChildOn, {c, 0u, 1u});
        }
        for (u32 k = 0; k < load<u16>(sub + kSubRecCount); ++k) {
            const u32 rec = load<u32>(sub + kSubRecs) + k * kRecSize;
            u32 obj = load<u32>(rec + kRecObj);
            const u32 body = load<u32>(obj + kObjBody);
            const u32 world = load<u32>(body + kBodyWorld);
            if (world != 0) {
                call(ctx, cfn10::Body_Remove931F00, {world, body});
                obj = load<u32>(rec + kRecObj);
            }
            vcall(ctx, obj, kVSwitch, {obj, 0u});
        }
    }
    store<u16>(g + kGrSubCount, 0);
    store<u32>(g + kGr28, 0);
    store<u32>(g + kGr32, 0);
    store<u32>(g + kGr36, 0);
    store<u32>(g + kGr40, 1);
    ret(ctx, 1u);
}

// void Layer_Removed42B420(Layer* l) (name?): every layer whose parent
// (+28) is l is swap-removed from the layer vector; a non-null one has its
// objects released (vtable +4), its object vector emptied (count 0,
// PtrVector_Clear, Nop) and its 36-byte block freed; then (freed or null)
// it goes through Layer_Removed itself. A null entry's +28 is read through
// as in the original. r3 as the original leaves it.
DECOMP(0042B420, Layer_Removed42B420) {
    const u32 l = arg<u32>(ctx, 0);
    u64 r3 = l;
    for (s32 i = 0;; ++i) {
        const u32 n = load<u16>(kLayers + 8);
        if (!(static_cast<u32>(i) < n)) break;
        const u32 d = load<u32>(kLayers);
        r3 = d;
        const u32 e = load<u32>(d + static_cast<u32>(i) * 4);
        if (load<u32>(e + kLayerParent) != l) continue;
        u32 cnt = n, data = d;
        if (e != 0) {
            for (u32 j = 0; j < load<u16>(e + kLayerObjCount); ++j) {
                const u32 o = load<u32>(load<u32>(e + kLayerObjs) + j * 4);
                if (o != 0) vcall(ctx, o, 4, {o});
            }
            store<u16>(e + kLayerObjCount, 0);
            call(ctx, cfn10::PtrVector_Clear, {e + kLayerObjs});
            call(ctx, cfn10::Nop, {e + kLayerObjs});
            call(ctx, cfn10::SmallFree, {e, kLayerSize});
            cnt = load<u16>(kLayers + 8);
            data = load<u32>(kLayers);
        }
        const u32 last = cnt - 1;
        store<u16>(kLayers + 8, static_cast<u16>(last));
        if (i != static_cast<s32>(last & 0xFFFF))
            store<u32>(data + static_cast<u32>(i) * 4, load<u32>(data + (last & 0xFFFF) * 4));
        // e was freed above; the original still recurses with it.
        r3 = call(ctx, cfn10::Layer_Removed42B420, {e});
        --i;
    }
    ctx.gpr[3] = r3;
}

// void Tracks_Advance320708(Set* s, f32 dt, u8 flag) (name?): counts the
// time left (+60) down by dt; once it reaches 0 the key index (+56) steps
// round the current group's keys, each key's time becoming the time left,
// with Tracks_Reset320190(s, flag) whenever a key's time is 0, until a key
// with a non-zero time (or the starting index) comes up. Then every item's
// flag bit 31 is refreshed from its source byte. r3 / f1 as the original
// leaves them.
DECOMP(00320708, Tracks_Advance320708) {
    const u32 s = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const u32 flag = arg<u32>(ctx, 2) & 0xFF;
    const f64 zero = lfs_f64(kZero320704);
    const f32 t = static_cast<f32>(lfs_f64(s + kTsTime) - dt);
    store<f32>(s + kTsTime, t);
    ctx.fpr[1] = t;
    u32 owner = load<u32>(s + kTsOwner), group = load<u32>(s + kTsGroup);
    const u32 slot = load<u32>(load<u32>(owner + 8)) + group * 8;
    if (!(t < zero || t == zero)) {
        ctx.gpr[3] = group;
        return;
    }
    const u32 start = load<u32>(s + kTsKey);
    u32 key = start;
    f64 f1;
    for (;;) {
        key += 1;
        store<u32>(s + kTsKey, key);
        // The key count comes from the group the clock started in.
        const u32 v = load<u32>(slot);
        const u32 cnt = v != 0 ? load<u16>(v + 8) : 0;
        if (!(key < cnt)) {
            key = 0;
            store<u32>(s + kTsKey, 0);
        }
        const u32 e = load<u32>(load<u32>(load<u32>(load<u32>(owner + 8)) + group * 8)) + key * 8;
        store<u32>(s + kTsTime, load<u32>(e + 4));
        f1 = lfs_f64(e + 4);
        if (f1 == zero) {
            ctx.fpr[1] = f1;
            call(ctx, cfn10::Tracks_Reset320190, {s, flag});
            owner = load<u32>(s + kTsOwner);
            group = load<u32>(s + kTsGroup);
            key = load<u32>(s + kTsKey);
            f1 = lfs_f64(s + kTsTime);
        }
        if (start == key) break;
        if (!(f1 == zero)) break;
    }
    ctx.fpr[1] = f1;
    s32 i = 0;
    for (; i < static_cast<s32>(load<u16>(s + kTsItemCount)); ++i) {
        const u32 item = load<u32>(load<u32>(s + kTsItems) + static_cast<u32>(i) * 8 + kTsItemPtr);
        if (item == 0) continue;
        const u32 b = load<u8>(load<u32>(item + kItemSrc) + kSrcBit);
        store<u32>(item + kItemFlags, (load<u32>(item + kItemFlags) & 0x7FFFFFFFu) | ((b & 1u) << 31));
    }
    ctx.gpr[3] = static_cast<u32>(i);
}
