// Engine thread objects: { const char* name; u32; u64 sys_ppu_thread_t id; ... }.
#pragma once

#include "opentit/native.h"
#include "../sly4.h"

namespace sly4::thread {

namespace imp {
constexpr u32 sys_ppu_thread_create = 0x00ABF144;  // (u64* id, entry opd, u64 arg, s32 prio, u32 stack, u64 flags, name)
}

namespace fn {
constexpr u32 Thread_Create = 0x0056F5BC;  // decompiled: src/platform/thread.cpp
}

struct ThreadParams {
    be_s32 priority;
    be_u32 unk4;  // 0 in the defaults
    be_u32 unk8;  // 1 in the defaults
};
static_assert(sizeof(ThreadParams) == 12);

constexpr s32 kDefaultPriority = 0x600;
constexpr u32 kStackSize = 0x10000;

void Thread_Create(void* thread, u32 entry, const ThreadParams* params);

}  // namespace sly4::thread
