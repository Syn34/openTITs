// cellSysutil wrappers. The game guards every callback call with one
// lightweight mutex so the main loop and other threads don't race.
#include "opentit/native.h"
#include "../sly4.h"

namespace sly4::system {
using namespace opentit::native;

// Polled once per frame by the main loop and by loading screens.
s32 Sysutil_CheckCallback() {
    call(imp::sys_lwmutex_lock, g::sysutil_mutex, kSysutilMutexTimeoutUs);
    const s32 result = call<s32>(imp::cellSysutilCheckCallback);
    call(imp::sys_lwmutex_unlock, g::sysutil_mutex);
    return result;
}

// int Sysutil_RegisterCallback(int slot, CellSysutilCallback func, void* userdata)
s32 Sysutil_RegisterCallback(s32 slot, u32 func, u32 userdata) {
    call(imp::sys_lwmutex_lock, g::sysutil_mutex, kSysutilMutexTimeoutUs);
    const s32 result = call<s32>(imp::cellSysutilRegisterCallback, slot, func, userdata);
    call(imp::sys_lwmutex_unlock, g::sysutil_mutex);
    return result;
}

}  // namespace sly4::system

NATIVE(0056C184, sly4::system::Sysutil_CheckCallback)
NATIVE(0056C1E8, sly4::system::Sysutil_RegisterCallback)
