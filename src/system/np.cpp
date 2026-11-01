// PSN (NP) start-up.
#include "../sly4.h"

using namespace sly4;

namespace {
constexpr u32 CELL_SYSMODULE_SYSUTIL_NP = 0x0016;
// Bytes of user memory taken by loading system modules?
constexpr u32 kModuleMemoryUsed = g::mem_stats + 0x168;
}  // namespace

// bool Np_Init(u32 pool_size)
// Loads the NP module, records how much user memory that cost, allocates
// the pool for sceNpInit and starts NP. Returns true on success.
DECOMP(0056C0B4, Np_Init) {
    const u32 pool_size = arg<u32>(ctx, 0);

    Stack frame(ctx, 2 * sizeof(MemoryInfo));
    const u32 before = frame.addr(), after = frame.addr() + sizeof(MemoryInfo);

    syscall(ctx, sc::sys_memory_get_user_memory_size, {before});
    const s32 rc = static_cast<s32>(call(ctx, imp::cellSysmoduleLoadModule, {CELL_SYSMODULE_SYSUTIL_NP}));
    syscall(ctx, sc::sys_memory_get_user_memory_size, {after});
    ref<be_u32>(kModuleMemoryUsed) +=
        ref<MemoryInfo>(before).available_user_memory - ref<MemoryInfo>(after).available_user_memory;

    if (rc < 0) return ret(ctx, 0u);

    const u32 pool = static_cast<u32>(call(ctx, fn::Mem_Alloc, {pool_size, 0, 8}));
    store<u32>(g::np_pool, pool);
    store<u32>(g::np_pool_size, pool_size);
    const u32 np = static_cast<u32>(call(ctx, imp::sceNpInit, {pool_size, pool}));
    ret(ctx, np == 0 ? 1u : 0u);
}
