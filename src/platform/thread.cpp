// PPU thread creation for the engine's thread objects.
#include "thread.h"

namespace sly4::thread {
using namespace opentit::native;

// sys_ppu_thread_create(&thread->id, entry, (u64)thread, params->priority,
// 64 KiB stack, flags 0, thread->name). Without params the priority is 0x600.
void Thread_Create(void* thread, u32 entry, const ThreadParams* params) {
    GuestFrame frame(sizeof(ThreadParams));
    ThreadParams* defaults = frame.as<ThreadParams>();
    defaults->priority = kDefaultPriority;
    defaults->unk4 = 0;
    defaults->unk8 = 1;
    if (!params) params = defaults;

    const u32 name = gref<be_u32>(guest(thread));  // unchecked, as in the original
    const s32 priority = params->priority;
    call(imp::sys_ppu_thread_create, guest(thread) + 8u, entry, thread, priority, kStackSize, 0u, name);
}

}  // namespace sly4::thread

NATIVE(0056F5BC, sly4::thread::Thread_Create)
