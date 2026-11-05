// World_HandleEvent callees, part 23g: an object's effect and child reset,
// a style lookup that copies a def's sub-records, and the world's return
// from a paused sequence.
#include "hevent22.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
// Obj_ResetEffects39CD68: +320 holder (+276 hash, +280), +340 effect,
// three 76-byte child tables from +80 ({Object*[18], +72 u32 count}),
// PtrVectors at +0 (u16 count +8) and +20 (u16 count +28).
constexpr u32 kEffectRelease = 0x004CF3C4, kEffectStart = 0x004C753C;
constexpr u32 kResetHash = 0xAB27550F, kReleaseArg = 0x00010CBB;
constexpr u32 kEffectFlags = 136, kEffectStopped = 0x200;
constexpr u32 kSlotRelease = 8, kSlotNotify = 40;

// Style_Apply0D6D28: the def table (0x01013B04), def {+32 u8 flags, +36 /
// +40 f32, +136 vector of 192-byte records (+0 data, +8 u16 count)}; the
// target's +16 record vector (created with a 192 stride), +120 def, +124 /
// +128, +132 bit 0x20000000; def vtable +28 (def, target).
constexpr u32 kDefTable = 0x01013B04;
constexpr u32 kTableFind74493C = 0x0074493C, kSmallAlloc = 0x0055238C, kPtrVectorConstruct = 0x005516E0;
constexpr u32 kVectorAdd733964 = 0x00733964;
constexpr u32 kRecSize = 192;
constexpr u32 kDefFlags = 32, kDefA = 36, kDefB = 40, kDefRecs = 136, kSlotApply = 28;
constexpr u32 kTgtRecs = 16, kTgtDef = 120, kTgtA = 124, kTgtB = 128, kTgtFlags = 132, kTgtFlag = 0x20000000;

// World_EndSequence47DFC8: the flag map (0x010BB738, hevent18) entry
// 0x846BF162 lists listeners {+0 obj, +4 next, +8 u8}; the 128-entry
// message queue at 0x010BA138 (20 bytes {+0, +4 obj, +8 hash, +12, +16
// u8}, u32 count at 0x010BA134).
constexpr u32 kWorldSetFlags = 0x00599024, kSortedMapFind786C0C = 0x00786C0C;
constexpr u32 kFlagMap = 0x010BB738, kSequenceKey = 0x846BF162;
constexpr u32 kQueueCount = 0x010BA134, kQueue = 0x010BA138, kQueueMax = 128, kQueueEntry = 20;
constexpr u32 kMsgSequenceEnd = 0x560A2252;
constexpr u32 kPendingFlush = 0x00367018, kPlayer2Reset = 0x00367290, kObjSetMode = 0x00470750;
constexpr u32 kPauseRelease = 0x0044BC10, kPauseSet = 0x002AB340;
constexpr u32 kPlayer2Mode = 0x00F9D1A8;
constexpr u32 kWorldSequenceObj = 0x1281F4, kWorldFlags45 = 45, kWorldUserFlags = 4144;
constexpr u32 kHudSlots = 0x010B19E0 + 176 + 28, kHudStride = 11968, kNumHuds = 2;
constexpr u32 kSlotShow = 36;
constexpr u32 kUserHidden = 0x10000000;
}  // namespace

// u32 Obj_ResetEffects39CD68(Object* o) (name?): the +320 holder's +280 /
// +276 get 0 / 0xAB27550F; +48 / +68 (u16) and +328 / +332 are cleared; a
// +340 effect is released (0x00010CBB), marked stopped (+136 |= 0x200),
// started and dropped; the three child tables' entries are released
// (vtable +8, slot cleared, count re-read) and their counts cleared; the
// +0 vector's objects are released (vtable +8) and the +20 vector's
// notified (vtable +40 (obj, 1)), both counts then cleared. Returns the
// last count read.
DECOMP(0039CD68, Obj_ResetEffects39CD68) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 holder = load<u32>(o + 320), e = load<u32>(o + 340);
    if (holder != 0) {
        store<u32>(holder + 280, 0);
        store<u32>(holder + 276, kResetHash);
    }
    store<u16>(o + 48, 0);
    store<u16>(o + 68, 0);
    store<u32>(o + 328, 0);
    store<u32>(o + 332, 0);
    if (e != 0) {
        call(ctx, kEffectRelease, {e, kReleaseArg});
        const u32 e2 = load<u32>(o + 340);
        store<u32>(e2 + kEffectFlags, load<u32>(e2 + kEffectFlags) | kEffectStopped);
        call(ctx, kEffectStart, {load<u32>(o + 340)});
        store<u32>(o + 340, 0);
    }
    for (u32 j = 0; j < 3; ++j) {
        const u32 t = o + 80 + 76 * j;
        u32 n = load<u32>(t + 72);
        for (u32 i = 0; n != 0 && i < n; ++i) {
            const u32 x = load<u32>(t + 4 * i);
            if (x == 0) continue;
            vcall(ctx, x, kSlotRelease, {x});
            store<u32>(t + 4 * i, 0);
            n = load<u32>(t + 72);
        }
        store<u32>(t + 72, 0);
    }
    for (u32 i = 0; i < load<u16>(o + 8); ++i) {
        const u32 x = load<u32>(load<u32>(o + 0) + 4 * i);
        if (x != 0) vcall(ctx, x, kSlotRelease, {x});
    }
    store<u16>(o + 8, 0);
    u32 n = 0;
    for (u32 i = 0; i < (n = load<u16>(o + 28)); ++i) {
        const u32 x = load<u32>(load<u32>(o + 20) + 4 * i);
        if (x != 0) vcall(ctx, x, kSlotNotify, {x, 1});
    }
    store<u16>(o + 28, 0);
    ret(ctx, n);
}

// u32 Style_Apply0D6D28(u32 key, Target* t) (name?): 0 for a zero key or
// one missing from the def table (0x01013B04). Otherwise t +120 = def, +124
// / +128 = def +36 / +40 (raw), t +132 bit 0x20000000 = def +32 bit 2;
// when the def has records (+136, u16 count +8) and t's +16 vector is
// missing or empty, each 192-byte record is added to it
// (Vector_Add733964; the vector is created with SmallAlloc(20) /
// PtrVector_Construct / +6 = 192 on first use, the def's vector re-read
// after each add). Returns the def's vtable +28 (def, t).
DECOMP(000D6D28, Style_Apply0D6D28) {
    const u32 key = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    if (key == 0) return ret(ctx, 0u);
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), key);
    const u32 d = load<u32>(static_cast<u32>(call(ctx, kTableFind74493C, {kDefTable, frame.addr()})));
    if (d == 0) return ret(ctx, 0u);
    store<u32>(t + kTgtDef, d);
    store<u32>(t + kTgtA, load<u32>(d + kDefA));
    store<u32>(t + kTgtB, load<u32>(d + kDefB));
    ctx.fpr[1] = hevent7::lfs_f64(d + kDefB);  // left in f1 by the lfs / stfs copy
    const u32 b = load<u8>(d + kDefFlags);
    store<u32>(t + kTgtFlags, (load<u32>(t + kTgtFlags) & ~kTgtFlag) | ((b & 4u) << 27));
    u32 v = load<u32>(d + kDefRecs);
    if (v != 0 && load<u16>(v + 8) != 0) {
        u32 vec = load<u32>(t + kTgtRecs);
        if (vec == 0 || load<u16>(vec + 8) == 0) {
            for (u32 i = 0, off = 0; i < (v != 0 ? load<u16>(v + 8) : 0u); ++i, off += kRecSize) {
                const u32 rec = load<u32>(v) + off;
                if (vec == 0) {
                    vec = static_cast<u32>(call(ctx, kSmallAlloc, {20}));
                    store<u32>(t + kTgtRecs, vec);
                    if (vec != 0) {
                        call(ctx, kPtrVectorConstruct, {vec});
                        store<u16>(vec + 6, static_cast<u16>(kRecSize));
                        vec = load<u32>(t + kTgtRecs);
                    }
                }
                call(ctx, kVectorAdd733964, {vec, rec});
                v = load<u32>(d + kDefRecs);
                vec = load<u32>(t + kTgtRecs);
            }
        }
    }
    vcall(ctx, d, kSlotApply, {d, t});
}

// u32 World_EndSequence47DFC8(World* w) (name?): World_SetFlags(255); the
// listeners of flag map entry 0x846BF162 (with an object or a zero byte)
// get a 0x560A2252 message queued while the queue holds under 128; the
// sequence object (+0x1281F4) gets vtable +40 (obj, 0); Pending_Flush,
// Player2_Reset unless 0x00F9D1A8 is 1; w +45 |= 8, Obj_SetMode470750(w,
// 0, 0), Pause_Release44BC10(0xFFBFFFFF), Pause_Set2AB340(0); both HUD
// slots (0x010B1AAC, stride 11968) get vtable +36 (hud, !(w +4144 &
// 0x10000000)). r3: the last HUD call's result or slot.
DECOMP(0047DFC8, World_EndSequence47DFC8) {
    const u32 w = arg<u32>(ctx, 0);
    call(ctx, kWorldSetFlags, {255});
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), kSequenceKey);
    const u32 list = load<u32>(static_cast<u32>(call(ctx, kSortedMapFind786C0C, {kFlagMap, frame.addr()})));
    if (list != 0 && load<u32>(list) != 0) {
        s32 count = static_cast<s32>(load<u32>(kQueueCount));
        u32 off = static_cast<u32>(count) * kQueueEntry;
        for (u32 n = load<u32>(list); n != 0; n = load<u32>(n + 4)) {
            const u32 obj = load<u32>(n + 0);
            const u8 b = load<u8>(n + 8);
            if (obj == 0 && b != 0) continue;
            if (!(count < static_cast<s32>(kQueueMax))) continue;
            const u32 q = kQueue + off;
            count += 1;
            off += kQueueEntry;
            store<u32>(q + 0, 0);
            store<u32>(kQueueCount, static_cast<u32>(count));
            store<u32>(q + 4, obj);
            store<u32>(q + 8, kMsgSequenceEnd);
            store<u32>(q + 12, 0);
            store<u8>(q + 16, b);
        }
    }
    const u32 seq = load<u32>(w + kWorldSequenceObj);
    if (seq != 0) vcall(ctx, seq, kSlotNotify, {seq, 0});
    call(ctx, kPendingFlush, {});
    if (static_cast<s32>(load<u32>(kPlayer2Mode)) != 1) call(ctx, kPlayer2Reset, {});
    store<u8>(w + kWorldFlags45, static_cast<u8>(load<u8>(w + kWorldFlags45) | 8u));
    call(ctx, kObjSetMode, {w, 0, 0});
    call(ctx, kPauseRelease, {0xFFBFFFFFu});
    call(ctx, kPauseSet, {0});
    u64 r3 = 0;
    for (u32 j = 0; j < kNumHuds; ++j) {
        const u32 shown = (load<u32>(w + kWorldUserFlags) & kUserHidden) == 0 ? 1u : 0u;
        const u32 hud = load<u32>(kHudSlots + j * kHudStride);
        r3 = hud;
        if (hud != 0) r3 = vcall(ctx, hud, kSlotShow, {hud, shown});
    }
    ctx.gpr[3] = r3;
}
