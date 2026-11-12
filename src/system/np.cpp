// PSN (NP) start-up.
#include "opentit/native.h"
#include "../memory/mem.h"

namespace sly4::system {
using namespace opentit::native;

namespace {
constexpr u32 CELL_SYSMODULE_SYSUTIL_NP = 0x0016;
}  // namespace

// Loads the NP module, records how much user memory that cost (MemStats
// +0x168), allocates the pool for sceNpInit and starts NP. Returns true on
// success.
bool Np_Init(u32 pool_size) {
    GuestFrame frame(2 * sizeof(MemoryInfo));
    MemoryInfo* before = frame.as<MemoryInfo>(0);
    MemoryInfo* after = before + 1;

    lv2::syscall(sc::sys_memory_get_user_memory_size, before);
    const s32 rc = call<s32>(imp::cellSysmoduleLoadModule, CELL_SYSMODULE_SYSUTIL_NP);
    lv2::syscall(sc::sys_memory_get_user_memory_size, after);
    memory::mem_stats().module_memory += before->available_user_memory - after->available_user_memory;

    if (rc < 0) return false;

    void* pool = memory::Mem_Alloc(pool_size, 0, 8);
    global<be_u32>(g::np_pool) = guest(pool);
    global<be_u32>(g::np_pool_size) = pool_size;
    return call<u32>(imp::sceNpInit, pool_size, pool) == 0;
}

}  // namespace sly4::system

NATIVE(0056C0B4, sly4::system::Np_Init)
