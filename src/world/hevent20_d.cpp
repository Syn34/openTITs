// World_HandleEvent callees, part 20d: a SPU job submission (the job
// descriptor builder and its queue wrapper) and an object constructor.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kJobQueueLock = 0x0056DFD4;    // () (symbols.txt JobQueue_Lock)
constexpr u32 kJobQueueGet = 0x0056E038;     // () -> JobQueue* (symbols.txt JobQueue_Get)
constexpr u32 kJobQueueUnlock = 0x0056E008;  // () (symbols.txt JobQueue_Unlock)
constexpr u32 kPtrVectorConstruct = 0x005516E0;  // (vec) (symbols.txt PtrVector_Construct)

// The job descriptor's fixed words: the SPU binary's EA, its size in 16-byte
// units (0x22FA8 bytes rounded up), and the two DMA list entry headers kept
// as u64 text constants at 0x00456798 (size 0xB0) / 0x004567A0 (size 0x10).
constexpr u32 kJobBinary = 0x00C36700;
constexpr u32 kJobBinarySize = 0x22FA8;
constexpr u32 kDmaHeaders = 0x00456798;

constexpr u32 kVtable2E8DFC = 0x00B1D4B8;    // the constructed object's vtable

// stvx: 16 zero bytes at ea & ~15.
inline void stvx_zero(u32 ea) {
    const u32 a = ea & ~15u;
    for (u32 k = 0; k < 16; k += 4) store<u32>(a + k, 0);
}
}  // namespace

// u64* Job_Build4567A8(JobRef* r, u32 in, u32 out) (name?): fills the job
// descriptor r +4: +0 u64 binary EA 0x00C36700, +8 u16 0x22FB (size / 16),
// +10 u16 16, +20 192, +44 u8 0, the 48 bytes zeroed (stvx) first; +48 the
// DMA entry {0xB0, in}; +56 {0x10, out} only when out is 16-byte aligned.
// Returns the address of +56.
DECOMP(004567A8, Job_Build4567A8) {
    const u32 j = load<u32>(arg<u32>(ctx, 0) + 4);
    const u32 in = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    stvx_zero(j);
    store<u16>(j + 10, 16);
    stvx_zero(j + 16);
    store<u32>(j + 20, 192);
    stvx_zero(j + 32);
    store<u8>(j + 44, 0);
    store<u16>(j + 8, static_cast<u16>((kJobBinarySize + 15) >> 4));
    store<u64>(j, kJobBinary);
    const u64 e0 = static_cast<u64>(in) | load<u64>(kDmaHeaders);
    store<u64>(j + 48, e0);
    ctx.gpr[4] = e0;
    if ((out & 0xF) == 0) {
        const u64 e1 = static_cast<u64>(out) | load<u64>(kDmaHeaders + 8);
        store<u64>(j + 56, e1);
        ctx.gpr[4] = e1;
    }
    ctx.gpr[7] = out & 0xF;
    ret(ctx, j + 56);
}

// void Job_Submit45685C(u32 in, u32 out) (name?): under JobQueue_Lock: the
// queue's vt +16 gives a job, Job_Build4567A8 fills it, vt +20 (queue, job)
// submits it.
DECOMP(0045685C, Job_Submit45685C) {
    const u32 in = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    call(ctx, kJobQueueLock, {in, out});
    const u32 q = static_cast<u32>(call(ctx, kJobQueueGet, {}));
    Stack frame(ctx, 16);
    const u32 ref = static_cast<u32>(ctx.gpr[1]);
    store<u32>(ref, q);
    const u32 job = static_cast<u32>(vcall(ctx, q, 16, {q}));
    store<u32>(ref + 4, job);
    call(ctx, 0x004567A8, {ref, in, out});
    const u32 q2 = load<u32>(ref), job2 = load<u32>(ref + 4);
    vcall(ctx, q2, 20, {q2, job2});
    call(ctx, kJobQueueUnlock, {});
}

// void Obj_Construct2E8DFC(Obj* o) (name?): vtable 0x00B1D4B8, +4..+48 and
// +72 / +96 / +100 zeroed, +56 / +64 u64 0, u8 +28 bits 0xE0 cleared, the
// PtrVector at +76 constructed with +82 u16 16, +104 = -1.
DECOMP(002E8DFC, Obj_Construct2E8DFC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u8>(o + 28) & ~0xE0u;
    for (u32 off = 4; off <= 24; off += 4) store<u32>(o + off, 0);
    store<u8>(o + 28, static_cast<u8>(b));
    store<u32>(o, kVtable2E8DFC);
    for (u32 off = 32; off <= 48; off += 4) store<u32>(o + off, 0);
    store<u64>(o + 56, 0);
    store<u64>(o + 64, 0);
    store<u32>(o + 72, 0);
    call(ctx, kPtrVectorConstruct, {o + 76, o + 76, 0});
    store<u32>(o + 96, 0);
    store<u32>(o + 100, 0);
    store<u16>(o + 82, 16);
    store<u32>(o + 104, 0xFFFFFFFFu);
    ctx.gpr[4] = 0xFFFFFFFFFFFFFFFFull;
    ctx.gpr[3] = 16;
}
