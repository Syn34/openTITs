// PPU thread creation for the engine's thread objects.
#include "thread.h"

using namespace sly4;
namespace th = sly4::thread;

// s32 Thread_Create(Thread* thread, entry opd, const ThreadParams* params)
// sys_ppu_thread_create(&thread->id, entry, (u64)thread, params->priority,
// 64 KiB stack, flags 0, thread->name). Without params the priority is 0x600.
DECOMP(0056F5BC, Thread_Create) {
    const u32 thread = arg<u32>(ctx, 0);
    const u32 entry = arg<u32>(ctx, 1);
    u32 params = arg<u32>(ctx, 2);

    Stack frame(ctx, sizeof(th::ThreadParams));
    th::ThreadParams& defaults = ref<th::ThreadParams>(frame.addr());
    defaults.priority = th::kDefaultPriority;
    defaults.unk4 = 0;
    defaults.unk8 = 1;
    if (params == 0) params = frame.addr();

    const u32 name = load<u32>(thread);
    const s32 priority = ref<th::ThreadParams>(params).priority;
    ctx.gpr[3] = call(ctx, th::imp::sys_ppu_thread_create,
                      {thread + 8, entry, thread, static_cast<u64>(static_cast<s64>(priority)), th::kStackSize, 0, name});
}
