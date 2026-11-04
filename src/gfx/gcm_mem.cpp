// Small helpers of Gcm_Init: system module loading with memory accounting,
// the RSX local / mapped main memory pools, the render command buffer and
// a bump region. (Pool_Create is the memory manager's, but only Gcm_Init's
// helpers call it.)
#include "gcm.h"

#include <atomic>

using namespace sly4;
namespace gc = sly4::gcm;

namespace {

constexpr u32 kStats = g::mem_stats;
constexpr u32 kModuleMemory = kStats + 360;
constexpr u32 kMemPoolA = 0x0116B340, kMemPoolB = 0x0116B328;  // pools 4 and 5 (src/memory/mem.h)
constexpr u32 cellSysmoduleLoadModule = 0x00ABD944;
constexpr u32 cellGcmMapMainMemory = 0x00ABD764;
constexpr u32 cellGcmAddressToOffset = 0x00ABD5E4;
constexpr u32 sys_memory_get_user_memory_size = 352;
constexpr u32 Pool_Create = 0x00565AA8;
constexpr u32 init_user_mstate = 0x005504F4;  // dlmalloc (name?)
constexpr u32 kMemset = 0x00726E54;

// dlmalloc globals.
constexpr u32 kMparams = 0x011648A8;  // { magic; page_size; granularity; mmap_threshold; trim_threshold; default_mflags }
constexpr u32 kGlobalMstate = 0x011648C0;
constexpr u32 kMagic = 0x58585858;

void atomic_store(u32 ea, u32 v) {  // lwarx / stwcx. loop in the original
    std::atomic_ref<u32>(*reinterpret_cast<u32*>(mem::ptr(ea))).store(bswap32(v));
}

}  // namespace

// bool Sysmodule_Require(u16 id)
// cellSysmoduleLoadModule, adding the user memory it took to the stats.
DECOMP(0056C020, Sysmodule_Require) {
    const u32 id = arg<u32>(ctx, 0) & 0xFFFF;
    Stack frame(ctx, 16);
    const u32 before = frame.addr(), after = frame.addr() + 8;
    syscall(ctx, sys_memory_get_user_memory_size, {before});
    const s32 rc = static_cast<s32>(call(ctx, cellSysmoduleLoadModule, {id}));
    syscall(ctx, sys_memory_get_user_memory_size, {after});
    store<u32>(kModuleMemory, load<u32>(kModuleMemory) + (load<u32>(before + 4) - load<u32>(after + 4)));
    ret(ctx, rc < 0 ? 0u : 1u);
}

// void RsxLocal_Init(u32 address, u32 size)
// Records RSX local memory and makes it pool 4 (16 bytes kept back).
DECOMP(0056C428, RsxLocal_Init) {
    const u32 address = arg<u32>(ctx, 0), size = arg<u32>(ctx, 1);
    store<u32>(kStats + 60, address);
    store<u32>(kStats + 64, size);
    call(ctx, Pool_Create, {kMemPoolA, address, size - 16, 0});
    store<u32>(kStats + 212, size);  // pool_a_free
    store<u32>(kStats + 216, size);  // pool_a_min_free
    store<u32>(kStats + 208, 0);     // pool_a_used
}

// void RsxMain_Init(u32 address, u32 size)
// Maps main memory for the RSX and makes it pool 5.
DECOMP(0056C49C, RsxMain_Init) {
    const u32 address = arg<u32>(ctx, 0), size = arg<u32>(ctx, 1);
    {
        Stack frame(ctx, 8);
        call(ctx, cellGcmMapMainMemory, {address, size, frame.addr()});
    }
    call(ctx, Pool_Create, {kMemPoolB, address, size, 0});
    store<u32>(kStats + 196, 0);     // pool_b_used
    store<u32>(kStats + 200, size);  // pool_b_free
    store<u32>(kStats + 204, size);  // pool_b_min_free
}

// void Heap_InitRegion(Region* r, u32 base, u32 size): { base, cursor = base, size }.
DECOMP(0056DF80, Heap_InitRegion) {
    const u32 r = arg<u32>(ctx, 0), base = arg<u32>(ctx, 1);
    store<u32>(r, base);
    store<u32>(r + 8, arg<u32>(ctx, 2));
    store<u32>(r + 4, base);
}

// void CommandBuffer_Construct(CommandBuffer* cb, u32 size)
// Two RSX command buffers of size (rounded to 4) + 4 KiB from pool 5,
// with their RSX offsets, cleared; then the get/put words.
DECOMP(0052F670, CommandBuffer_Construct) {
    const u32 cb = arg<u32>(ctx, 0);
    const u32 size = (arg<u32>(ctx, 1) + 3) & ~3u;
    const u32 alloc = size + 4096;
    store<u32>(cb + 256, cb);
    store<u32>(cb + 260, cb + 128);
    store<u32>(cb + 284, 0);
    store<u32>(cb + 264, size);
    for (u32 i = 0; i < 2; ++i) {
        const u32 p = static_cast<u32>(call(ctx, gc::fn::Mem_AllocPool, {128, alloc, 5, 7}));
        store<u32>(cb + 268 + 4 * i, p);
        store<u32>(cb + 276 + 4 * i, 0);
        call(ctx, cellGcmAddressToOffset, {load<u32>(cb + 268 + 4 * i), cb + 276 + 4 * i});
        call(ctx, kMemset, {load<u32>(cb + 268 + 4 * i), 0, alloc});
    }
    atomic_store(load<u32>(cb + 256), 0);
    const u32 words = load<u32>(cb + 264) >> 2;
    atomic_store(load<u32>(cb + 260), words);
}

// void Pool_Create(MemPool* pool, void* base, u32 size, bool reset)
// dlmalloc create_mspace_with_base (init_mparams inlined) for a MemPool:
// { base, size, mspace, used = 0, free = size }. No mspace (0) if the
// region is too small or too large.
DECOMP(00565AA8, Pool_Create) {
    const u32 pool = arg<u32>(ctx, 0), base = arg<u32>(ctx, 1), size = arg<u32>(ctx, 2);
    if (arg<u8>(ctx, 3) != 0 && field<u32>(pool, 8) != 0) field<u32>(pool, 8) = 0;
    u32 page = field<u32>(kMparams, 4);
    if (page == 0) {
        field<u32>(kMparams, 12) = 0xFFFFFFFFu;
        field<u32>(kMparams, 16) = 0x200000;
        field<u32>(kMparams, 20) = 4;
        if (field<u32>(kMparams, 0) == 0) {
            field<u32>(kMparams, 0) = kMagic;
            field<u32>(kGlobalMstate, 436) = 4;
        }
        page = 4096;
        field<u32>(kMparams, 4) = page;
        field<u32>(kMparams, 8) = 0x10000;
    }
    u32 mspace = 0;
    if (size > 0x208 && size < static_cast<u32>(-static_cast<s32>(page + 520))) {
        mspace = static_cast<u32>(call(ctx, init_user_mstate, {base, size}));
        field<u32>(mspace, 452) = 8;
    }
    field<u32>(pool, 8) = mspace;
    field<u32>(pool, 0) = base;
    field<u32>(pool, 4) = size;
    field<u32>(pool, 12) = 0;
    field<u32>(pool, 16) = size;
}
