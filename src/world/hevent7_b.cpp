// World_HandleEvent callees, part 7b: sound state, effect creation, the
// sorted table lookup and the FIOS read-by-path start.
#include "../file/fsop.h"
#include "hevent7.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;

namespace sly4 {
namespace hevent7b {
constexpr u32 kEffectSize = 140;
constexpr u32 kEffectFlags = 136, kEffectAutoStart = 0x200;
constexpr u32 kCompSkin = 0x0027469E;              // component type hash (name?)
constexpr u32 kCompKindSkin = 5;                   // its virtual +32 kind
constexpr u32 kCompRes = 28, kCompArg = 36;        // ref-counted resource (u16 refs at +4) and value
constexpr u32 kResRefs = 4;

// Table: +2 u16 count, +6 u16 flags (bit 0: unsorted), +8 {u32 key, value}[],
// +16 "not found" entry, +20 compare OPD (0x00F31A10: plain unsigned keys).
constexpr u32 kTabCount = 2, kTabFlags = 6, kTabEntries = 8, kTabMiss = 16, kTabCompare = 20;
constexpr u32 kTabUnsorted = 1;
constexpr u32 kCompareU32 = 0x00F31A10;

// FIOS attribute block (as file/fioswrap.cpp).
constexpr u32 kFlagTable = 0x00AF425C;             // { u32 mask; u32 fios_flag; } [7]
constexpr u32 kCallback = 0x00F3C2D8;              // completion callback OPD
constexpr u64 kDeadlineNever = 0x7FFFFFFFFFFFFFFFull;
constexpr u32 kFlagExtra = 0x20;
}  // namespace hevent7b
}  // namespace sly4

using namespace sly4::hevent7b;

// bool Sound_IsDone4431A0(Sound* s) (name?): 0 while not playing (+97).
// With sub-voices (the emitter node's voice, count from Voice_GetValue):
// 1 unless one of them reports a 0 first value (Sub_GetInfo). Without:
// done when there is no sample (+0 -> +0) or its +88 is not positive.
DECOMP(004431A0, Sound_IsDone4431A0) {
    const u32 s = arg<u32>(ctx, 0);
    if (load<u8>(s + kSndPlaying) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 idx = load<u16>(s + kSndEmitter);
    const u32 slot = load<u32>(kEmitters + kPoolNodes) + idx * kNodeSize + kNodeVoice;
    const u32 n = static_cast<u32>(call(ctx, h7fn::Voice_GetValue, {load<u32>(slot)}));
    if (n != 0) {
        Stack frame(ctx, 16);
        for (u32 i = 0; i < n; ++i) {
            const u64 sub = call(ctx, h7fn::Voice_GetSub, {load<u32>(slot), static_cast<u64>(static_cast<s64>(static_cast<s16>(i)))});
            call(ctx, h7fn::Sub_GetInfo, {sub, frame.addr(), 0, 0, 0});
            if (load<u32>(frame.addr()) == 0) {
                ret(ctx, 0u);
                return;
            }
        }
        ret(ctx, 1u);
        return;
    }
    const u32 sample = load<u32>(s + kSndSample);
    if (sample == 0) {
        ret(ctx, 1u);
        return;
    }
    const u32 data = load<u32>(sample + kSampleData);
    ret(ctx, data == 0 || load<s32>(data + kDataPending) <= 0 ? 1u : 0u);
}

// Effect* Effect_Create4A5E58(def, Resource* res, u32 value, u8 start)
// (name?): a new 140-byte effect for def (0 without one). With res, its
// skin component (type 0x0027469E, kind 5) drops its old resource (virtual
// +8), takes res (one more reference) and value (+36). With start, flag
// 0x200 at +136 and Effect_Start. The component lookup runs even when the
// allocation failed (quirk).
DECOMP(004A5E58, Effect_Create4A5E58) {
    const u32 def = arg<u32>(ctx, 0), res = arg<u32>(ctx, 1), value = arg<u32>(ctx, 2);
    const u32 start = arg<u32>(ctx, 3);
    if (def == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 e = static_cast<u32>(call(ctx, h7fn::SmallAlloc, {kEffectSize}));
    if (e != 0) call(ctx, h7fn::Effect_Construct, {e, def});
    if (res != 0) {
        const u32 c = static_cast<u32>(call(ctx, h7fn::Effect_FindComponent, {e, kCompSkin}));
        if (c != 0 && static_cast<s32>(hevent::vcall(ctx, c, 32, {c})) == static_cast<s32>(kCompKindSkin)) {
            if (const u32 old = load<u32>(c + kCompRes); old != 0) hevent::vcall(ctx, old, 8, {old});
            store<u32>(c + kCompRes, res);
            if (res != 0) store<u16>(res + kResRefs, static_cast<u16>(load<u16>(res + kResRefs) + 1));
            store<u32>(c + kCompArg, value);
        }
    }
    if ((start & 0xFF) != 0) {
        store<u32>(e + kEffectFlags, load<u32>(e + kEffectFlags) | kEffectAutoStart);
        call(ctx, h7fn::Effect_Start, {e});
    }
    ret(ctx, e);
}

// void* Table_Find74493C(Table* t, const u32* key) (name?): sorts the
// entries first when flagged (qsort with the table's compare), then a binary
// search: plain unsigned keys for the default compare, else the compare OPD
// on (entry, &key copy). Returns &entry.value, or &t->miss (+16).
DECOMP(0074493C, Table_Find74493C) {
    const u32 t = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 flags = load<u16>(t + kTabFlags);
    if (flags & kTabUnsorted) {
        const u32 n = load<u16>(t + kTabCount);
        store<u16>(t + kTabFlags, static_cast<u16>(flags & 0xFFFE));
        if (n != 0) call(ctx, h7fn::qsort, {load<u32>(t + kTabEntries), n, 8, load<u32>(t + kTabCompare)});
    }
    s32 hi = static_cast<s32>(load<u16>(t + kTabCount)) - 1, lo = 0;
    u32 base = load<u32>(t + kTabEntries);
    s32 found = -1;
    if (load<u32>(t + kTabCompare) == kCompareU32) {
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 k = load<u32>(base + static_cast<u32>(mid) * 8), want = load<u32>(key);
            if (k > want) hi = mid - 1;
            else if (k < want) lo = mid + 1;
            else { found = mid; break; }
        }
    } else {
        Stack frame(ctx, 16);
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

// bool FsOp_Start571A00(FsOp* op, const char* path, void* buf, s64 size,
// options in r7-r9) (name?): the FIOS read of a file by path into buf: the
// attribute block (as Fios_ReadFile's: deadline / priority, callback when
// options w2, fios flags from w0, user = op, w4 with flag 0x20) and the
// request on the scheduler; op->+0 = the fios_op. Returns whether it was made.
DECOMP(00571A00, FsOp_Start571A00) {
    const u32 op = arg<u32>(ctx, 0);
    const u64 path = ctx.gpr[4], buf = ctx.gpr[5], size = ctx.gpr[6];
    const u32 w0 = static_cast<u32>(ctx.gpr[7] >> 32), w1 = static_cast<u32>(ctx.gpr[7]);
    const u32 w2 = static_cast<u32>(ctx.gpr[8] >> 32), w4 = static_cast<u32>(ctx.gpr[9] >> 32);
    u32 flags = 0;
    for (u32 i = 0; i < 7; ++i) {
        const u32 mask = load<u32>(kFlagTable + 8 * i);
        if ((mask & w0) == mask) flags |= load<u32>(kFlagTable + 8 * i + 4);
    }
    const bool high = static_cast<s32>(w1) == 1;
    Stack frame(ctx, 48);  // fios_opattr, 40 bytes
    const u32 a = frame.addr();
    store<u64>(a + 0, high ? 1 : kDeadlineNever);
    store<u8>(a + 8, high ? 127 : 0x80);
    // Bytes 9-11 are uninitialised stack in the original (as fioswrap.cpp).
    store<u8>(a + 9, 0);
    store<u8>(a + 10, 0);
    store<u8>(a + 11, 0);
    store<u32>(a + 12, w2 != 0 ? kCallback : 0);
    store<u32>(a + 16, 0);
    store<u32>(a + 20, flags);
    store<u32>(a + 24, 0);
    store<u32>(a + 28, op);
    store<u32>(a + 32, (w0 & kFlagExtra) ? w4 : 0);
    store<u32>(a + 36, 0);
    // r8-r10 as the original leaves them (the callee reads r3-r7).
    const u32 sched = load<u32>(fsop::kScheduler);
    const u32 fop = static_cast<u32>(call(ctx, h7fn::fios_ReadPath5FF4F4,
                                          {sched, a, path, buf, size, sched, buf, 0x00F90000u /* lis r10 of the scheduler address */}));
    store<u32>(op, fop);
    ret(ctx, fop != 0 ? 1u : 0u);
}
