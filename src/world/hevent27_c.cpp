// World_HandleEvent callees, part 27c: the Havok job system set up on level
// entry.
#include "hevent27.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent27;

namespace {
// The thread's Havok memory router: *(r13 - 28332) +44 allocator, vt +8
// alloc(size) (as hevent22_d.cpp).
constexpr u32 kTlsRouter = 28332, kRouterAllocator = 44;
constexpr u32 kSpursHandle = 0x00F64C8C;   // u32 copied from 0x010F0E6C (name?)
constexpr u32 kSpursSource = 0x010F0E6C;
constexpr u32 kJobThreadPool = 0x00F64C90; // 56-byte thread pool (Hk_StepDeltaTime92F0C8's third argument)
constexpr u32 kJobQueue = 0x00F64C94;      // 1984-byte job queue (Hk_StepDeltaTime92F0C8's second argument)
// The container allocator object (vtable at +0, vt +20 free(p, bytes)).
constexpr u32 kContainerAlloc = 0x011DF31C;
constexpr u32 kSpuUtilCtor = 0x007DC758;      // (SpuUtil* 40 bytes, handle) (name?)
constexpr u32 kSpuUtilInit = 0x007DC510;      // (SpuUtil*) (name?)
constexpr u32 kPoolCinfoCtor = 0x007E2080;    // (Cinfo* 24 bytes) (name?)
constexpr u32 kThreadPoolCtor = 0x007E20D8;   // (ThreadPool* 56 bytes, const Cinfo*) (name?)
constexpr u32 kRemoveReference = 0x007D4EA8;  // (ReferencedObject*) (AnimSkel_Release7D4EA8)
constexpr u32 kQueueCinfoCtor = 0x007E2A70;   // (QueueCinfo* 40 bytes) (name?)
constexpr u32 kJobQueueCtor = 0x007E9630;     // (JobQueue* 1984 bytes, const QueueCinfo*) (name?)
constexpr u32 kJobQueueRegister = 0x00888104; // (JobQueue*) (name?)
constexpr u32 kArrayLocked = 0x80000000u;     // hkArray capacity flag: storage not owned

u32 router_alloc(PPUContext& ctx, u32 size) {
    const u32 a = load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13]) - kTlsRouter) + kRouterAllocator);
    return static_cast<u32>(vcall(ctx, a, 8, {a, size}));
}
u64 sx(u32 v) { return static_cast<u64>(static_cast<s64>(static_cast<s32>(v))); }
}  // namespace

// ---------------------------------------------------------------- 0x00341A78
// u32 Havok_InitJobSystem341A78() (name?): copies the SPURS handle
// (0x010F0E6C -> 0x00F64C8C), builds the 40-byte SPU helper (0x007DC758,
// 0x007DC510), the 56-byte job thread pool from a {helper, 6} cinfo
// (0x00F64C90; vt +32 (pool, 6)), drops the helper reference, then the
// 1984-byte job queue from a cinfo whose +28 is 21 (0x00F64C94,
// registered with 0x00888104), and destroys that cinfo's nested arrays.
// Allocations go through the thread's memory router; the size is stored at
// +4 of each block even when the allocation failed.
DECOMP(00341A78, Havok_InitJobSystem341A78) {
    store<u32>(kSpursHandle, load<u32>(kSpursSource));
    const u32 util = router_alloc(ctx, 40);
    store<u16>(util + 4, 40);  // unchecked (address 4 for a failed allocation)
    u32 helper = 0;
    if (util != 0) {
        call(ctx, kSpuUtilCtor, {util, load<u32>(kSpursHandle)});
        helper = util;
    }
    call(ctx, kSpuUtilInit, {helper});
    Stack frame(ctx, 64);
    const u32 pcinfo = frame.addr(), qcinfo = frame.addr() + 24;
    call(ctx, kPoolCinfoCtor, {pcinfo});
    store<u32>(pcinfo + 0, helper);
    store<u32>(pcinfo + 4, 6);
    const u32 pmem = router_alloc(ctx, 56);
    store<u16>(pmem + 4, 56);  // unchecked
    u32 pool = 0;
    if (pmem != 0) {
        call(ctx, kThreadPoolCtor, {pmem, pcinfo});
        pool = pmem;
    }
    store<u32>(kJobThreadPool, pool);
    vcall(ctx, pool, 32, {pool, 6});  // unchecked
    call(ctx, kRemoveReference, {helper});
    call(ctx, kQueueCinfoCtor, {qcinfo});
    store<u32>(qcinfo + 28, 21);
    const u32 qmem = router_alloc(ctx, 1984);
    u32 queue = 0;
    if (qmem != 0) {
        call(ctx, kJobQueueCtor, {qmem, qcinfo});
        queue = qmem;
    }
    store<u32>(kJobQueue, queue);
    call(ctx, kJobQueueRegister, {queue});
    // ~QueueCinfo: hkArray<hkArray<s32>> at +16 {data, size, capacity | flags}.
    const u32 n = load<u32>(qcinfo + 20);
    u32 data = load<u32>(qcinfo + 16);
    for (s32 k = static_cast<s32>(n - 1); k >= 0; --k) {
        const u32 e = data + 12 * static_cast<u32>(k);
        const u32 cap = load<u32>(e + 8);
        store<u32>(e + 4, 0);
        if ((cap & kArrayLocked) == 0) vcall(ctx, kContainerAlloc, 20, {kContainerAlloc, load<u32>(e + 0), sx(cap << 2)});
        store<u32>(e + 0, 0);
        store<u32>(e + 8, kArrayLocked);
    }
    data = load<u32>(qcinfo + 16);
    const u32 cap = load<u32>(qcinfo + 24);
    u64 r3 = data;
    if ((cap & kArrayLocked) == 0) {
        store<u32>(qcinfo + 20, 0);
        const u32 opd = load<u32>(load<u32>(kContainerAlloc) + 20);
        ctx.gpr[6] = load<u32>(opd);  // this call sequence leaves the entry in r6 and the OPD in r7
        ctx.gpr[7] = opd;
        r3 = call_ptr(ctx, opd, {kContainerAlloc, data, sx((cap & 0x3FFFFFFF) * 12)});
    }
    ctx.gpr[3] = r3;
}
