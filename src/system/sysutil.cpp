// cellSysutil wrappers. The game guards every callback call with one
// lightweight mutex so the main loop and other threads don't race.
#include "../sly4.h"

using namespace sly4;

// int Sysutil_CheckCallback()
// Polled once per frame by the main loop and by loading screens.
DECOMP(0056C184, Sysutil_CheckCallback) {
    call(ctx, imp::sys_lwmutex_lock, {g::sysutil_mutex, kSysutilMutexTimeoutUs});
    const u64 result = call(ctx, imp::cellSysutilCheckCallback);
    call(ctx, imp::sys_lwmutex_unlock, {g::sysutil_mutex});
    ctx.gpr[3] = result;
}

// int Sysutil_RegisterCallback(int slot, CellSysutilCallback func, void* userdata)
DECOMP(0056C1E8, Sysutil_RegisterCallback) {
    const u64 slot = ctx.gpr[3], func = ctx.gpr[4], userdata = ctx.gpr[5];
    call(ctx, imp::sys_lwmutex_lock, {g::sysutil_mutex, kSysutilMutexTimeoutUs});
    const u64 result = call(ctx, imp::cellSysutilRegisterCallback, {slot, func, userdata});
    call(ctx, imp::sys_lwmutex_unlock, {g::sysutil_mutex});
    ctx.gpr[3] = result;
}
