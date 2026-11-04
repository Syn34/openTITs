// World_HandleEvent callees, part 2: the identified helpers (trophy bit,
// timed callbacks, the three SPURS job launchers, the two sorted-map
// lookups, the push-stack blend) and the 54..55-instruction callees.
#include "hevent.h"
#include "../resource/tags.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent2 {
namespace afn {
constexpr u32 JobQueue_Lock = 0x0056DFD4;     // () sys_lwmutex_lock(0x0116B580, 100000) (name?)
constexpr u32 JobQueue_Get = 0x0056E038;      // () -> 0x0116B600 (name?)
constexpr u32 JobQueue_Unlock = 0x0056E008;   // () sys_lwmutex_unlock(0x0116B580) (name?)
constexpr u32 memset = 0x00726E54;
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 qsort = 0x0071EC28;
constexpr u32 PtrVector_Reserve = 0x00551718;
constexpr u32 sys_ppu_thread_create = 0x00ABF144;  // import stub
constexpr u32 PushStack_Blend = 0x00447860;   // (f32 t) (name?)
constexpr u32 Hash_ToString = 0x0056A6E0;     // (u32) -> "0x%x"
constexpr u32 SortedMap_Find3 = 0x0076611C;
constexpr u32 sub_00363890 = 0x00363890;      // (Object*, 0, ?, 0, 0, u32, f32) (name?)
constexpr u32 sub_000CF534 = 0x000CF534;      // (Object*, f32 dt)
constexpr u32 sub_0026F798 = 0x0026F798;      // ()
}  // namespace afn

// Trophy manager: +8 u8 ready, +168 u32 bits[].
constexpr u32 kTrophyReady = 8, kTrophyBits = 168;

// World_AddTimedCallback lists: World +0x128480 + kind * 20, PtrVector-like
// {+0 data, +4 u16 capacity, +8 u16 count, +10 u16 high water} of 12-byte entries.
constexpr u32 kWorldLists = 0x128480, kWorldListStride = 20, kTimedEntry = 12;

// Default comparators of the two sorted maps (keys compared directly when set).
constexpr u32 kRegistryDefaultCompare = 0x00F36070;
constexpr u32 kTableDefaultCompare = 0x00F3B118;

// The push stack (PushStack_Push / _Remove) and its three 31-value blocks.
// Every block (and every pushed item) is {u32; {s32 op; f32 value}[31]}: op
// 1 = set, 2 = add, 3 = multiply, anything else = leave.
constexpr u32 kPushStack = 0x010AFB84;   // u32 items[4]
constexpr u32 kPushCur = 0x010AFB94;     // the values in use
constexpr u32 kPushTarget = 0x010AFC90;  // the defaults with every pushed item applied
constexpr u32 kPushFrom = 0x010AFD8C;    // kPushCur when the blend started
constexpr u32 kPushTime = 0x010AFE88;    // f32 blend time
constexpr u32 kPushElapsed = 0x010AFE8C; // u32
constexpr u32 kPushCount = 0x010AFE90;   // u32
constexpr u32 kPushDefaults = 0x00F65B78;  // u32 pointer to the default block
constexpr u32 kPushValues = 31;
constexpr u32 kPushEps = 0x00447B64;     // f32 1e-5: shorter blends snap
constexpr u32 kPushZero = 0x00447B68;    // f32 0.0

// The save threads' shared state (see SaveTaskPoll in hevent_g.cpp).
constexpr u32 kSaveTaskState = 0x00F9D1A8;
constexpr u32 kSaveTaskResult = 0x00F9D1B8;  // u32 cleared when LoadGameFromList starts (name?)
constexpr u32 kSaveTaskBuffer = 0x010127A0;
constexpr u32 kLoadGameFromListEntry = 0x00F30FB8;  // thread entry OPD
constexpr u32 kLoadGameFromListName = 0x00AC5FB8;   // "LoadGameFromList"
constexpr u32 kSaveTaskPrio = 1001, kSaveTaskStack = 16384;

// World +0x128224: the running cutscene (+60 f32 time scale).
constexpr u32 kWorldCutscene = 0x128224;
constexpr u32 kSystemStateTimeScale = 4;  // f32 in system_state::object (name?)

// Loading-screen UI (name?): +88 Show(ui, bool), +188 SetText(ui, const char*, 0) (name?)
constexpr u32 kLoadingUi = 0x00F558E8, kLoadingText1 = 0x00F558EC, kLoadingText2 = 0x00F558F0;
constexpr u32 kUiVfnShow = 88, kUiVfnSetText = 188;
constexpr u32 kEmptyString = 0x00AC5E64;  // ""

// Job C's results (see Objects_RunJobC): pending SPU jobs, the object list.
constexpr u32 kJobCPending = 0x01021F00, kJobCCount = 0x0101DBE0, kJobCObjects = 0x0101DBF0;
constexpr u32 kUsleepWait = 300;

// SaveSlot_MarkSeen: objects with a u32 hash at +96 (name?).
constexpr u32 kHashedObjects = 0x010681FC;  // {u32 data, u16 count +8}
constexpr u32 kCurrentUser = kUserBlocks;   // World u8 (user block 0 +0): the active save slot (name?)
constexpr u32 kSlotSeenBits = 3940;         // SaveSlot (World +80 + i * 4056) + : bitset by list index (name?)

// Player2_Activate: the second player handle, the handle map.
constexpr u32 kPlayer2Handle = 0x00F64D44;
constexpr u32 kConstMinusOne = 0x003671AC;  // f32 -1.0
constexpr u32 kTable1164CA4 = 0x01164CA4;   // (name?)

// The DMA list entry's size half (the 8 bytes before each launcher hold it).
constexpr u64 kJobDmaEntry = 0x0000002000000000ull;

// One SPURS job launch (CellSpursJob-like descriptor, 48 bytes of header
// then one DMA list entry {u32 size, u32 ea}).
void JobRun(PPUContext& ctx, u32 image, u32 image_size) {
    const u32 record = arg<u32>(ctx, 0);
    call(ctx, afn::JobQueue_Lock);
    const u32 q = static_cast<u32>(call(ctx, afn::JobQueue_Get));
    const u32 job = static_cast<u32>(vcall(ctx, q, 16, {q}));  // allocate
    call(ctx, afn::memset, {job, 0, 48});
    store<u16>(job + 30, 1024);
    store<u16>(job + 10, 8);    // DMA list size
    store<u32>(job + 20, 32);   // input size: the 32-byte record
    ref<u8>(job + 44) = 0;
    store<u16>(job + 8, static_cast<u16>(((image_size + 15) >> 4) & 0x0FFFFFFF));  // in 16-byte units
    store<u64>(job + 0, image);
    store<u64>(job + 48, kJobDmaEntry | record);  // {32, record}
    // lwarx / stwcx. loop in the original: atomic ++ of the job's pending count.
    const u32 pending = load<u32>(record + 16);
    for (;;) {
        const u32 old = mem::r32(pending);
        if (mem::cas32(pending, old, old + 1)) break;
    }
    vcall(ctx, q, 20, {q, job});  // push
    call(ctx, afn::JobQueue_Unlock);
}

// SortedMap find with an inline direct search for the map's default comparator
// (as SortedMap_Find 0x007A46EC does with its own default).
void MapFind(PPUContext& ctx, u32 default_compare) {
    const u32 m = arg<u32>(ctx, 0), key_ptr = arg<u32>(ctx, 1);
    tags::SortedMap& map = ref<tags::SortedMap>(m);
    const u16 flags = map.flags;
    if (flags & tags::kUnsorted) {
        map.flags = static_cast<u16>(flags & 0xFFFE);
        const u16 n = map.count;
        if (static_cast<s32>(n) > 0) call(ctx, afn::qsort, {map.entries.get(), n, tags::kEntrySize, map.compare.get()});
    }
    s32 hi = static_cast<s32>(map.count) - 1, lo = 0;
    u32 entries = map.entries;
    s32 found = -1;
    if (map.compare != default_compare) {
        Stack frame(ctx, 16);  // the key copy at r1+112
        store<u32>(frame.addr(), load<u32>(key_ptr));
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 opd = map.compare;
            ctx.gpr[5] = opd;
            ctx.gpr[6] = load<u32>(opd);
            const s32 c = static_cast<s32>(call_ptr(ctx, opd, {entries + static_cast<u32>(mid) * 8, frame.addr()}));
            if (c > 0) hi = mid - 1;
            else if (c < 0) lo = mid + 1;
            else {
                entries = map.entries;  // reloaded on a match
                found = mid;
                break;
            }
        }
    } else {
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 key = load<u32>(key_ptr);
            const u32 e = load<u32>(entries + static_cast<u32>(mid) * 8);
            if (e > key) hi = mid - 1;
            else if (e < key) lo = mid + 1;
            else {
                found = mid;
                break;
            }
        }
    }
    if (found < 0) return ret(ctx, m + 16);
    ret(ctx, entries + static_cast<u32>(found) * 8 + 4);
}

// The thread poll of "LoadGameFromList": 0 starts the thread (state 1); 3 and
// 2 are returned once and reset to 0, 2 first copying the SaveSlot (4056 bytes)
// to dst; other states are returned as they are.
void LoadGameFromListPoll(PPUContext& ctx) {
    const u32 dst = arg<u32>(ctx, 0);
    const s32 s = static_cast<s32>(load<u32>(kSaveTaskState));
    if (s == 3) {
        store<u32>(kSaveTaskState, 0);
        ctx.gpr[4] = 0x00FA0000u;
        ctx.gpr[5] = 0;
        return ret(ctx, 3);
    }
    if (s == 2) {
        store<u32>(kSaveTaskState, 0);
        ctx.gpr[6] = 0x00FA0000u;
        ctx.gpr[7] = 0;
        call(ctx, afn::memmove, {dst, kSaveTaskBuffer, kSaveSlotSize});
        return ret(ctx, 2);
    }
    if (s != 0) {
        ctx.gpr[4] = static_cast<u64>(static_cast<s64>(s));
        return ret(ctx, s);
    }
    store<u32>(kSaveTaskResult, 0);
    store<u32>(kSaveTaskState, 1);
    Stack frame(ctx, 16);  // sys_ppu_thread_t out (r1+112)
    call(ctx, afn::sys_ppu_thread_create,
         {frame.addr(), kLoadGameFromListEntry, 0, kSaveTaskPrio, kSaveTaskStack, 0, kLoadGameFromListName});
    const s32 now = static_cast<s32>(load<u32>(kSaveTaskState));
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(now));
    ret(ctx, now);
}

// Copy the 31 values (not the ops) of one push block to another.
void PushCopyValues(u32 dst, u32 src) {
    for (u32 i = 0; i < kPushValues; ++i) store<u32>(dst + 8 + i * 8, load<u32>(src + 8 + i * 8));
}
}  // namespace hevent2
}  // namespace sly4

using namespace sly4::hevent2;

// bool Trophy_Has(Trophy* t, s32 i): bit i of the unlocked bitset at +168, false
// until the manager is ready (+8).
DECOMP(004FB56C, Trophy_Has) {
    const u32 t = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1);
    const u32 ready = ref<u8>(t + kTrophyReady);
    if (ready == 0) {
        ctx.gpr[5] = 0;
        return ret(ctx, 0u);
    }
    const u32 bit = 1u << (i & 31);
    ctx.gpr[4] = bit;
    ctx.gpr[5] = 1;
    ret(ctx, (load<u32>(t + kTrophyBits + ((i >> 3) & 0x1FFFFFFCu)) & bit) != 0 ? 1u : 0u);
}

// void World_AddTimedCallback(World* w, u32 kind, u32 a, u32 b, u32 c): appends
// the 12-byte entry {a, b, c} to the World's list `kind` (+0x128480 + kind * 20),
// doubling its capacity when full, and raises its high-water mark (name?).
// Leaves r3 = the old high-water mark and r4 = the new count, as the original.
DECOMP(0046B5F8, World_AddTimedCallback) {
    const u32 w = arg<u32>(ctx, 0), kind = arg<u32>(ctx, 1);
    const u32 a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3), c = arg<u32>(ctx, 4);
    const u32 v = w + kind * kWorldListStride + kWorldLists;
    const u32 cap = load<u16>(v + 4);
    if (load<u16>(v + 8) == cap) call(ctx, afn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
    const u32 n = load<u16>(v + 8);
    const u32 e = load<u32>(v) + n * kTimedEntry;
    store<u32>(e + 0, a);
    store<u32>(e + 4, b);
    store<u32>(e + 8, c);
    const u32 count = (load<u16>(v + 8) + 1) & 0xFFFF;
    const u32 high = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(count));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(high) > static_cast<s32>(count) ? high : count));
    ctx.gpr[4] = count;
    ret(ctx, high);
}

// void Job_Run456A60(JobRecord* r) / Job_Run456BA8 / Job_Run456CF0: launches the
// SPU job image at 0x00C86C80 / 0x00CA0600 / 0x00CBC000 on the 32-byte record
// built by Objects_RunJobA / B / C, bumping the record's pending count (+16)
// (name?). r3 on return is JobQueue_Unlock's.
DECOMP(00456A60, Job_Run456A60) { JobRun(ctx, 0x00C86C80, 0x20000 - 26328); }
DECOMP(00456BA8, Job_Run456BA8) { JobRun(ctx, 0x00CA0600, 0x20000 - 18008); }
DECOMP(00456CF0, Job_Run456CF0) { JobRun(ctx, 0x00CBC000, 0x20000 - 18008); }

// u32* Registry_Find(SortedMap* m, const u32* key) / Table_Find: the value slot
// of `key`, or &m->null_value; the same code as SortedMap_Find with another
// default comparator (name?)
DECOMP(00766C44, Registry_Find) { MapFind(ctx, kRegistryDefaultCompare); }
DECOMP(00799474, Table_Find) { MapFind(ctx, kTableDefaultCompare); }

// void PushStack_Changed(f32 time): rebuilds the target block from the defaults
// with every pushed item applied in order, then blends to it over `time`
// (PushStack_Blend(0)); shorter than 1e-5 it snaps (name?). PushStack_Push and
// _Remove reach it with whatever their callers left in f1.
DECOMP(00447B70, PushStack_Changed) {
    const f64 time = farg(ctx, 0);
    const bool snap = std::fabs(time) < static_cast<f64>(load<f32>(kPushEps));
    if (!snap) {
        PushCopyValues(kPushFrom, kPushCur);
        store<u32>(kPushElapsed, 0);
    }
    for (u32 i = 0; i < kPushValues; ++i) {
        const u32 out = kPushTarget + 8 + i * 8;
        f32 v = load<f32>(load<u32>(kPushDefaults) + 8 + i * 8);
        store<f32>(out, v);
        const u32 n = load<u32>(kPushCount);
        for (u32 j = 0; j < n; ++j) {
            const u32 e = load<u32>(kPushStack + j * 4) + 4 + i * 8;
            const s32 op = static_cast<s32>(load<u32>(e));
            const f32 x = load<f32>(e + 4);
            if (op == 1) v = x;
            else if (op == 2) v = static_cast<f32>(static_cast<f64>(v) + static_cast<f64>(x));
            else if (op == 3) v = fmuls(v, x);
            else continue;
            store<f32>(out, v);
        }
    }
    if (snap) PushCopyValues(kPushCur, kPushTarget);
    store<f32>(kPushTime, static_cast<f32>(time));
    callf(ctx, afn::PushStack_Blend, {}, {static_cast<f64>(load<f32>(kPushZero))});
}

// s32 LoadGameFromList_Poll(SaveSlot* dst): the "LoadGameFromList" save thread (name?)
DECOMP(000AEEC0, LoadGameFromList_Poll) { LoadGameFromListPoll(ctx); }

// f32 World_GetCutsceneTimeScale(World* w): the running cutscene's +60 (or the
// SystemState's +4 without one), times the World time scale while it is on (name?)
DECOMP(004843A4, World_GetCutsceneTimeScale) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 cs = load<u32>(w + kWorldCutscene);
    f32 r;
    if (cs != 0) r = load<f32>(cs + 60);
    else r = load<f32>(system_state::get(ctx) + kSystemStateTimeScale);
    if ((load<u32>(w + wevent::kWorldTimeScaleOn) >> 31) != 0) r = fmuls(r, load<f32>(w + 0x1282AC));
    ret(ctx, r);
}

// void LoadingUi_Hide(): with both the UI at 0x00F558E8 and the text at
// 0x00F558EC: Show(ui, 0) and SetText("") on 0x00F558EC and (if set) 0x00F558F0 (name?)
DECOMP(000AC39C, LoadingUi_Hide) {
    const u32 ui = load<u32>(kLoadingUi);
    if (ui == 0 || load<u32>(kLoadingText1) == 0) return;
    vcall(ctx, ui, kUiVfnShow, {ui, 0});
    const u32 t1 = load<u32>(kLoadingText1);
    vcall(ctx, t1, kUiVfnSetText, {t1, kEmptyString, 0});
    const u32 t2 = load<u32>(kLoadingText2);
    if (t2 != 0) vcall(ctx, t2, kUiVfnSetText, {t2, kEmptyString, 0});
}

// void Objects_FinishJobC(f32 dt): waits (300 us sleeps) for job C's SPU work,
// then sub_000CF534(obj, dt) for each object it listed; clears the list and
// calls sub_0026F798() (name?)
DECOMP(000CF898, Objects_FinishJobC) {
    const f64 dt = farg(ctx, 0);
    while (load<u32>(kJobCPending) != 0) syscall(ctx, kSysTimerUsleep, {kUsleepWait});
    u32 r3 = load<u32>(kJobCCount);
    if (r3 != 0) {
        for (u32 i = 0; i < load<u32>(kJobCCount); ++i)
            callf(ctx, afn::sub_000CF534, {load<u32>(kJobCObjects + i * 4)}, {dt});
        r3 = load<u32>(kJobCCount);
    }
    store<u32>(kJobCCount, 0);
    call(ctx, afn::sub_0026F798, {r3});  // r3 is a leftover (the count); the callee ignores it
}

// void SaveSlot_MarkSeen(?, u32 hash): sets the bit of the object with this
// hash (+96) in the list at 0x010681FC in the active SaveSlot's bitset
// (+3940); unknown hashes go to Hash_ToString (result unused) (name?)
DECOMP(002E2874, SaveSlot_MarkSeen) {
    const u32 hash = arg<u32>(ctx, 1);
    const u32 w = load<u32>(kCurrentWorld);
    const u32 slot = w + kSaveData + ref<u8>(w + kCurrentUser) * kSaveSlotSize;
    const u32 data = load<u32>(kHashedObjects);
    const u32 n = load<u16>(kHashedObjects + 8);
    u32 index = 0, entry = 0, p = data;
    for (u32 i = 0; i < n; ++i, p += 4) {
        if (load<u32>(load<u32>(p) + 96) == hash) {
            index = i;
            entry = load<u32>(data + i * 4);
            break;
        }
    }
    if (entry == 0) {
        call(ctx, afn::Hash_ToString, {hash, p});
        return;
    }
    const u32 word = slot + kSlotSeenBits + ((index >> 3) & 0x1FFFFFFCu);
    const u32 old = load<u32>(word);
    const u32 now = old | (1u << (index & 31));
    store<u32>(word, now);
    ctx.gpr[4] = now;
    ctx.gpr[5] = old;
    ret(ctx, word);
}

// void Player2_Activate(): the object of the handle at 0x00F64D44 (flags bit
// 0x1) gets +396 |= 0x40040000, vfunc +20(obj, 1) and sub_00363890(obj, 0, ...,
// -1.0) (name?). Quirk: without one the original goes on with obj = 0.
DECOMP(003671B0, Player2_Activate) {
    Stack frame(ctx, 16);  // the handle copy at r1+112
    const u32 h = load<u32>(kPlayer2Handle);
    store<u32>(frame.addr(), h);
    u32 obj = 0;
    if (h != 0) {
        obj = load<u32>(static_cast<u32>(call(ctx, afn::SortedMap_Find3, {kHandleMap, frame.addr()})));
        if (obj != 0 && (load<u32>(obj + 4) & 1u) == 0) obj = 0;
    }
    store<u32>(obj + 396, load<u32>(obj + 396) | 0x40040000u);
    vcall(ctx, obj, 20, {obj, 1});
    ctx.gpr[5] = kConstMinusOne;  // left over from loading the constant; not read by the callee
    callf(ctx, afn::sub_00363890, {obj, 0, kConstMinusOne, 0, 0, kTable1164CA4},
          {static_cast<f64>(load<f32>(kConstMinusOne))});
}
