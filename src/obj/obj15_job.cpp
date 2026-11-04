// World objects, part 15: the SPURS job queue helpers (the descriptors
// the draw record list and the other workers submit).
#include "obj15.h"
#include "../obj/obj8.h"

using namespace sly4;
using namespace sly4::obj15;

namespace {

constexpr u32 kJobMutex = 0x0116B580;           // sys_lwmutex_t (JobQueue_Lock)
constexpr u32 kJobQueue = 0x0116B600;           // JobQueue_Get(): +0 vtable
constexpr u32 kLockTimeout = 100000;            // usec
constexpr u32 kVfnQueueAcquire = 16;            // (queue) -> job descriptor handle (name?)
constexpr u32 kVfnQueueSubmit = 20;             // (queue, handle) (name?)
constexpr u32 JobDesc_Init = 0x0056EBB8;        // obj14
constexpr u32 imp_cellSpursRunJobChain = 0x00ABE264;
constexpr u32 imp_cellSpursEventFlagWait = 0x00ABDFE4;
constexpr u32 imp_cellSpursJobGuardNotify = 0x00ABE244;

// The DMA size words (u64 size << 32) the job builders OR the addresses into.
constexpr u32 kDmaWordsE4C0 = 0x0056E4B8;       // {16 << 32}
constexpr u32 kDmaWordsE710 = 0x0056E700;       // {224 << 32, 16 << 32}
constexpr u32 kDmaWordsE970 = 0x0056E960;       // {0xD90 << 32, 16 << 32}

// The job chain owner (0056E1F0): +256 the event flag, +384 the job
// guard, +520 the CellSpursJobChain*, +524 the command list, +528 u32
// the pending command index, +536 u8 done (name?).
constexpr u32 kJcFlag = 256, kJcGuard = 384, kJcChain = 520, kJcCmds = 524, kJcPending = 528, kJcDone = 536;
constexpr u32 kJcNopCount = 511 * 4;            // the 8-byte commands reset to 127 (NOP)
constexpr u64 kJcCmdNop = 127;
constexpr u32 kJcNextOffset = 16352;            // the command after the NOPs: "next" (low bits 3)

// The atomic ++*counter the submit paths inline (lwarx / stwcx.).
void AtomicInc(u32 counter) {
    for (;;) {
        const u32 old = load<u32>(counter);
        if (mem::cas32(counter, old, old + 1)) break;
    }
}

// The shared body of WorkQueue_Fn56E4C0 / Fn56E710: an 80-byte job
// descriptor (zeroed, binary / size, +10 32, +44 0, +20 the rounded
// *pb * *pc + extra), then the DMA entries: +48 (size << 32 | a) when the
// size is DMA-able and a is aligned to it, +56 / +64 / +72 the pointers
// pb, pc and d (16-aligned only) with their size words.
void JobDmaInit(u32 j, u64 binary, u16 qwords, u32 extra, u32 a, u32 pb, u32 pc, u32 d,
                u64 wb, u64 wc, u64 wd, bool check_d) {
    obj8::VStore(j, obj8::VZero());
    obj8::VStore(j + 16, obj8::VZero());
    obj8::VStore(j + 32, obj8::VZero());
    store<u16>(j + 8, qwords);
    store<u64>(j, binary);
    store<u8>(j + 44, 0);
    const u32 size = load<u32>(pb) * load<u32>(pc) + 15;
    store<u16>(j + 10, 32);
    store<u32>(j + 20, (size & ~15u) + extra);
    const u32 s = size & 0xFFF0;
    bool skip = static_cast<s32>(s) > 16384;
    if (!skip && static_cast<s32>(s) > 16 && (s & 15) != 0) skip = true;
    if (!skip && ((s & (s - 1)) & 15) != 0) skip = true;
    if (!skip && s != 0 && ((a & (s - 1)) & 15) != 0) skip = true;
    if (!skip) store<u64>(j + 48, (static_cast<u64>(s) << 32) | a);
    if ((pb & 15) == 0) store<u64>(j + 56, pb | wb);
    if ((pc & 15) == 0) store<u64>(j + 64, pc | wc);
    if (!check_d || (d & 15) == 0) store<u64>(j + 72, d | wd);
}

// lock; handle = queue->Acquire(); build(handle); ++*counter;
// queue->Submit(handle); unlock.
u32 QueueAcquire(PPUContext& ctx) {
    call(ctx, imp::sys_lwmutex_lock, {kJobMutex, kLockTimeout});
    const u32 opd = load<u32>(load<u32>(kJobQueue) + kVfnQueueAcquire);
    return static_cast<u32>(call_ptr(ctx, opd, {kJobQueue, opd, load<u32>(opd)}));
}

void QueueSubmit(PPUContext& ctx, u32 q, u32 h) {
    const u32 opd = load<u32>(load<u32>(q) + kVfnQueueSubmit);
    call_ptr(ctx, opd, {q, h, opd, load<u32>(opd)});
    call(ctx, imp::sys_lwmutex_unlock, {kJobMutex});
}

}  // namespace

// void WorkQueue_Fn56E4C0(QueueRef* r, u32 first, u32* stride, u32* count, u32* counter)
// DrawRecordList_Flush's job: binary 0x00CE4C80 (0x1A89 qwords), +20
// size + 48, the four DMA entries (all 16 bytes, d only when aligned) (name?).
DECOMP(0056E4C0, WorkQueue_Fn56E4C0) {
    const u32 j = load<u32>(arg<u32>(ctx, 0) + 4);
    const u32 a = arg<u32>(ctx, 1), pb = arg<u32>(ctx, 2), pc = arg<u32>(ctx, 3), d = arg<u32>(ctx, 4);
    const u64 w = load<u64>(kDmaWordsE4C0);
    JobDmaInit(j, 0x00CE4C80u, 0x1A89, 48, a, pb, pc, d, w, w, w, true);
    ctx.gpr[3] = (d & 15) == 0 ? (d | w) : kDmaWordsE4C0;  // as the original leaves it
}

// void WorkQueue_Fn56E710(QueueRef* r, u32 a, u32* pb, u32* pc, u32 d)
// The same descriptor for binary 0x00DCDC80 (0x3142 qwords), +20 size +
// 256, d's entry 224 bytes and always written (name?).
DECOMP(0056E710, WorkQueue_Fn56E710) {
    const u32 j = load<u32>(arg<u32>(ctx, 0) + 4);
    const u32 a = arg<u32>(ctx, 1), pb = arg<u32>(ctx, 2), pc = arg<u32>(ctx, 3), d = arg<u32>(ctx, 4);
    JobDmaInit(j, 0x00DCDC80u, 0x3142, 256, a, pb, pc, d, load<u64>(kDmaWordsE710 + 8),
               load<u64>(kDmaWordsE710 + 8), load<u64>(kDmaWordsE710), false);
    ctx.gpr[3] = d | load<u64>(kDmaWordsE710);  // as the original leaves it
}

// void WorkQueue_Submit56E710(u32 a, u32* pb, u32* pc, Obj* d)
// Under the job mutex: a descriptor from the queue, WorkQueue_Fn56E710
// on it, ++*(d->+208), then submitted (name?).
DECOMP(0056E854, WorkQueue_Submit56E710) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    Stack frame(ctx, 16);
    const u32 ref = frame.addr();  // {queue, handle}
    store<u32>(ref, kJobQueue);
    store<u32>(ref + 4, QueueAcquire(ctx));
    call(ctx, 0x0056E710u, {ref, a, b, c, d});
    AtomicInc(load<u32>(d + 208));
    QueueSubmit(ctx, load<u32>(ref), load<u32>(ref + 4));
}

// void WorkQueue_Fn56E970(Job* j, Worker* w, u32 c, u32 d, u32 n, u32 e)
// Binary 0x00CFF580 (0x1358 qwords), +16 1, +30 1024, +20 the rounded
// n * 16 + n * 4 * w->+3208 + 3488; DMA +48 w (0xD90 bytes), +56 e (16),
// +64 c, +72 d, +80 n (all 64 bits) (name?).
DECOMP(0056E970, WorkQueue_Fn56E970) {
    const u32 j = arg<u32>(ctx, 0), w = arg<u32>(ctx, 1);
    const u32 n = static_cast<u32>(ctx.gpr[7]);
    const u32 per = ((n << 2) + 15) & ~15u;
    const u32 a = (((n << 4) & ~15u) + 15) & ~15u;
    const u32 total = a + per * load<u32>(w + 3208) + 3488;
    const u64 w0 = load<u64>(kDmaWordsE970), w1 = load<u64>(kDmaWordsE970 + 8);
    obj8::VStore(j, obj8::VZero());
    obj8::VStore(j + 32, obj8::VZero());
    obj8::VStore(j + 16, obj8::VZero());
    store<u16>(j + 10, 16);
    store<u32>(j + 16, 1);
    store<u32>(j + 20, total);
    store<u16>(j + 30, 1024);
    store<u8>(j + 44, 0);
    store<u16>(j + 8, 0x1358);
    store<u64>(j, 0x00CFF580u);
    store<u64>(j + 48, w | w0);
    store<u64>(j + 56, static_cast<u32>(ctx.gpr[8]) | w1);
    store<u64>(j + 64, static_cast<u32>(ctx.gpr[5]));
    store<u64>(j + 72, static_cast<u32>(ctx.gpr[6]));
    store<u64>(j + 80, ctx.gpr[7]);
}

// void WorkQueue_Submit56E970(Worker* w, u32 b, u32 c, u32 n, u32 e)
// Under the job mutex: WorkQueue_Fn56E970(handle, w, b, c, n, e),
// ++*(w->+3212), submitted (name?).
DECOMP(0056EA8C, WorkQueue_Submit56E970) {
    const u32 w = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2), n = arg<u32>(ctx, 3),
              e = arg<u32>(ctx, 4);
    const u32 h = QueueAcquire(ctx);
    call(ctx, 0x0056E970u, {h, w, b, c, n, e});
    AtomicInc(load<u32>(w + 3212));
    QueueSubmit(ctx, kJobQueue, h);
}

// void WorkQueue_SubmitJobDesc(JobOwner* o, u32 b, u32 c, u64 d)
// Under the job mutex: JobDesc_Init({queue, handle}, o, b, c, d),
// ++*(o->+104), submitted (name?).
DECOMP(0056EC80, WorkQueue_SubmitJobDesc) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2);
    const u64 d = ctx.gpr[6];
    Stack frame(ctx, 16);
    const u32 ref = frame.addr();
    store<u32>(ref, kJobQueue);
    store<u32>(ref + 4, QueueAcquire(ctx));
    call(ctx, JobDesc_Init, {ref, o, b, c, d});
    AtomicInc(load<u32>(o + 104));
    QueueSubmit(ctx, load<u32>(ref), load<u32>(ref + 4));
}

// void JobChain_Kick(JobChainOwner* o)
// Takes the pending command index (atomically zeroed), writes the "next"
// command at it (cmds + 16352, low bits 3), runs the chain, waits on the
// event flag (bit 1), resets the 2044 commands to NOP (127), notifies the
// job guard and sets +536 (name?).
DECOMP(0056E1F0, JobChain_Kick) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = o + kJcPending;
    u32 idx;
    for (;;) {  // lwarx / stwcx. exchange with 0
        idx = load<u32>(p);
        if (mem::cas32(p, idx, 0)) break;
    }
    const u32 cmds = load<u32>(o + kJcCmds);
    store<u64>(cmds + ((idx << 3) & ~7u), (static_cast<u64>(cmds + kJcNextOffset) & ~7ull) | 3);
    call(ctx, imp_cellSpursRunJobChain, {load<u32>(o + kJcChain)});
    Stack frame(ctx, 16);
    store<u16>(frame.addr(), 1);
    call(ctx, imp_cellSpursEventFlagWait, {o + kJcFlag, frame.addr(), 1});
    for (u32 i = 0; i < kJcNopCount * 8; i += 32) {
        store<u64>(load<u32>(o + kJcCmds) + i, kJcCmdNop);
        store<u64>(load<u32>(o + kJcCmds) + i + 8, kJcCmdNop);
        const u32 c = load<u32>(o + kJcCmds);
        store<u64>(c + i + 16, kJcCmdNop);
        store<u64>(c + i + 24, kJcCmdNop);
    }
    call(ctx, imp_cellSpursJobGuardNotify, {o + kJcGuard});
    store<u8>(o + kJcDone, 1);
    ctx.gpr[3] = 1;  // as the original leaves it
}

namespace {
constexpr u32 memset_ = 0x00726E54;
constexpr u32 kJobBinaryED88 = 0x00E25000;      // (name?)
constexpr u16 kJobSizeED88 = (0x00020000u - 9192 + 15) >> 4;
}  // namespace

// void WorkQueue_SubmitED88(u32 a, u64 b, u32* counter)
// Under the job mutex: a descriptor from the queue cleared (48 bytes),
// binary 0x00E25000 (0x1DC2 qwords), +16 1, +48 a, +56 b, +64 counter;
// ++*counter, then submitted (name?).
DECOMP(0056ED88, WorkQueue_SubmitED88) {
    const u32 a = arg<u32>(ctx, 0);
    const u64 b = ctx.gpr[4];
    const u32 counter = arg<u32>(ctx, 2);
    const u32 j = QueueAcquire(ctx);
    call(ctx, memset_, {j, 0, 48});
    store<u16>(j + 8, kJobSizeED88);
    store<u64>(j + 48, a);
    store<u64>(j, kJobBinaryED88);
    store<u8>(j + 44, 0);
    store<u32>(j + 20, 0);
    store<u32>(j + 16, 1);
    store<u16>(j + 10, 0);
    store<u64>(j + 56, b);
    store<u64>(j + 64, counter);
    AtomicInc(counter);
    QueueSubmit(ctx, kJobQueue, j);
}

namespace {
constexpr u32 kDmaWordsEEF0 = 0x0056EEE0;       // {0xA0 << 32, 16 << 32}
}  // namespace

// void WorkQueue_Fn56EEF0(QueueRef* r, u32 a, u32 b, u32 n, u32 c)
// Binary 0x00DA0980 (0x2D2B qwords), +10 24, +20 the rounded n * 144 +
// 176; DMA +48 a (0xA0 bytes), +56 (n * 144 rounded, b) when DMA-able,
// +64 c (16, when aligned), +72 n (64 bits) (name?).
DECOMP(0056EEF0, WorkQueue_Fn56EEF0) {
    const u32 j = load<u32>(arg<u32>(ctx, 0) + 4);
    const u32 a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 4);
    const u64 n = ctx.gpr[6];
    const u32 size = static_cast<u32>(n) * 144 + 15;
    obj8::VStore(j, obj8::VZero());
    store<u16>(j + 10, 24);
    obj8::VStore(j + 16, obj8::VZero());
    store<u32>(j + 20, (size & ~15u) + 176);
    obj8::VStore(j + 32, obj8::VZero());
    store<u8>(j + 44, 0);
    store<u16>(j + 8, 0x2D2B);
    store<u64>(j, 0x00DA0980u);
    store<u64>(j + 48, a | load<u64>(kDmaWordsEEF0));
    const u32 s = size & 0xFFF0;
    bool skip = static_cast<s32>(s) > 16384;
    if (!skip && static_cast<s32>(s) > 16 && (s & 15) != 0) skip = true;
    if (!skip && ((s & (s - 1)) & 15) != 0) skip = true;
    if (!skip && s != 0 && ((b & (s - 1)) & 15) != 0) skip = true;
    if (!skip) store<u64>(j + 56, (static_cast<u64>(s) << 32) | b);
    if ((c & 15) == 0) store<u64>(j + 64, c | load<u64>(kDmaWordsEEF0 + 8));
    store<u64>(j + 72, n);
    ctx.gpr[3] = j;  // as the original leaves it
}

namespace {
constexpr u32 kDmaWordF158 = 0x0056F150;        // {0x70 << 32}
constexpr u32 kDmaWordF2C8 = 0x0056F2C0;        // {0x210 << 32}
}  // namespace

// void WorkQueue_SubmitF158(Worker* w)
// Under the job mutex: a cleared descriptor, binary 0x00D73D80 (0x2CBC
// qwords), +30 64, +10 8, +20 128, +48 w (0x70 bytes); ++*(w->+80),
// submitted (name?).
DECOMP(0056F158, WorkQueue_SubmitF158) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 j = QueueAcquire(ctx);
    call(ctx, memset_, {j, 0, 48});
    store<u16>(j + 30, 64);
    store<u16>(j + 10, 8);
    store<u32>(j + 20, 128);
    store<u8>(j + 44, 0);
    store<u16>(j + 8, 0x2CBC);
    store<u64>(j, 0x00D73D80u);
    store<u64>(j + 48, w | load<u64>(kDmaWordF158));
    AtomicInc(load<u32>(w + 80));
    QueueSubmit(ctx, kJobQueue, j);
}

// void WorkQueue_SubmitF2C8(Worker* w)
// The same for binary 0x00DFF100 (0x25E6 qwords), +10 8, +16 1, +20 528,
// +48 w (0x210 bytes); ++*(w->+380) (name?).
DECOMP(0056F2C8, WorkQueue_SubmitF2C8) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 j = QueueAcquire(ctx);
    call(ctx, memset_, {j, 0, 48});
    store<u16>(j + 10, 8);
    store<u32>(j + 16, 1);
    store<u8>(j + 44, 0);
    store<u32>(j + 20, 528);
    store<u16>(j + 8, 0x25E6);
    store<u64>(j, 0x00DFF100u);
    store<u64>(j + 48, w | load<u64>(kDmaWordF2C8));
    AtomicInc(load<u32>(w + 380));
    QueueSubmit(ctx, kJobQueue, j);
}

namespace {
constexpr u32 kJcMutex = 544;                   // sys_lwmutex_t
constexpr u32 kJcRing = 2044;                   // commands in the ring (the index wraps after 2043)
}  // namespace

// void JobChain_Push(JobChainOwner* o, u32 cmd)
// Under o's mutex: the next ring index (atomically, wrapping after 2043);
// at index 0 the ring is first reset (after waiting for the chain unless
// +536 says it is done; the job guard notified when it was not) and +536
// cleared; then the command (low 3 bits cleared) goes in and the chain is
// run (name?).
DECOMP(0056E088, JobChain_Push) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 cmd = static_cast<u64>(static_cast<u32>(ctx.gpr[4])) & ~7ull;
    call(ctx, imp::sys_lwmutex_lock, {o + kJcMutex, kLockTimeout});
    const u32 p = o + kJcPending;
    u32 idx;
    for (;;) {  // lwarx / stwcx.: idx = (*p)++, wrapping
        idx = load<u32>(p);
        if (mem::cas32(p, idx, idx == kJcRing - 1 ? 0 : idx + 1)) break;
    }
    if (idx == 0) {
        if (load<u8>(o + kJcDone) == 0) {
            Stack frame(ctx, 16);
            store<u16>(frame.addr(), 1);
            call(ctx, imp_cellSpursEventFlagWait, {o + kJcFlag, frame.addr(), 1});
        }
        for (u32 i = 0; i < kJcNopCount * 8; i += 32)
            for (u32 k = 0; k < 32; k += 8) store<u64>(load<u32>(o + kJcCmds) + i + k, kJcCmdNop);
        if (load<u8>(o + kJcDone) == 0) call(ctx, imp_cellSpursJobGuardNotify, {o + kJcGuard});
        store<u8>(o + kJcDone, 0);
    }
    const u32 cmds = load<u32>(o + kJcCmds);
    const u32 chain = load<u32>(o + kJcChain);
    store<u64>(cmds + ((idx << 3) & ~7u), cmd);
    call(ctx, imp_cellSpursRunJobChain, {chain});
    call(ctx, imp::sys_lwmutex_unlock, {o + kJcMutex});
}

namespace {
constexpr u32 kDmaWordF430 = 0x0056F428;        // {0x1D0 << 32}
}  // namespace

// void WorkQueue_SubmitWorkerRecords(Worker* w, Rec* recs, u64 count)
// Under the job mutex: a cleared descriptor, binary 0x00D12B00 (0x30FD
// qwords), +20 464, +10 8, +48 w (0x1D0 bytes), +56 recs, +64 count;
// ++*(w->+288), submitted (name?).
DECOMP(0056F430, WorkQueue_SubmitWorkerRecords) {
    const u32 w = arg<u32>(ctx, 0), recs = arg<u32>(ctx, 1);
    const u64 count = ctx.gpr[5];
    const u32 j = QueueAcquire(ctx);
    call(ctx, memset_, {j, 0, 48});
    store<u64>(j, 0x00D12B00u);
    store<u16>(j + 8, 0x30FD);
    store<u8>(j + 44, 0);
    store<u32>(j + 20, 464);
    store<u64>(j + 48, w | load<u64>(kDmaWordF430));
    store<u16>(j + 10, 8);
    store<u64>(j + 56, recs);
    store<u64>(j + 64, count);
    AtomicInc(load<u32>(w + 288));
    QueueSubmit(ctx, kJobQueue, j);
}

namespace {
constexpr u32 memmove_ = 0x0071C3F0;
constexpr u32 kRsxCopyFlag = 0x0116B580 + 768;  // the copy job's event flag (name?)
constexpr u32 kRsxCopyMin = 64;                 // smaller copies are done in place
}  // namespace

// void Rsx_Copy(const void* src, void* dst, u64 size, u32 flags)
// Up to 64 bytes: memmove. Otherwise a copy job (binary 0x00D67A80, 0x164
// qwords, +30 1024, +48 src, +56 dst, +64 size, with flags bit 0 also +72
// the event flag and +80 0x8000) under the job mutex, then (bit 0) waits
// for its event bit (name?).
DECOMP(0056E2EC, Rsx_Copy) {
    const u32 src = arg<u32>(ctx, 0), dst = arg<u32>(ctx, 1);
    const u64 size = ctx.gpr[5];
    const u32 flags = arg<u32>(ctx, 3);
    if (!(static_cast<u32>(size) > kRsxCopyMin)) {
        call(ctx, memmove_, {dst, src, size});
        return;
    }
    const u32 j = QueueAcquire(ctx);
    const bool wait = (flags & 1) != 0;
    const u32 ef = wait ? kRsxCopyFlag : 0;
    const u32 bits = wait ? 0x8000 : 0;
    Stack frame(ctx, 16);
    store<u16>(frame.addr(), static_cast<u16>(bits));
    call(ctx, memset_, {j, 0, 48});
    store<u16>(j + 8, 0x164);
    store<u8>(j + 44, 0);
    store<u64>(j + 56, dst);
    store<u64>(j + 64, size);
    store<u64>(j, 0x00D67A80u);
    store<u16>(j + 30, 1024);
    store<u64>(j + 48, src);
    store<u64>(j + 72, ef);
    store<u64>(j + 80, bits);
    QueueSubmit(ctx, kJobQueue, j);
    if (wait) call(ctx, imp_cellSpursEventFlagWait, {ef, frame.addr(), 1});
}
