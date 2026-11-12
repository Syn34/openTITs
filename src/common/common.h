// Engine plumbing shared by many subsystems: the heap/allocator virtual
// functions, the small-block allocator front end, ApeTFreeList<T>'s
// allocate/free/destroy, and thin C library wrappers.
// Addresses are guest addresses in the BCES01284 v01.02 EBOOT. A "?" in a
// comment marks a guess from how the code is used.
#pragma once

#include "opentit/native.h"
#include "../container/container.h"
#include "../memory/mem.h"

namespace sly4 {
namespace common {

namespace cmfn {
// decompiled in src/common/*.cpp
constexpr u32 Mem_FreeFrom = 0x0054EE6C;      // (void* p, s32 pool): Mem_FreePool(p, pool, 0)
constexpr u32 SmallAlloc = 0x0055238C;        // (u32 size, ...) -> 00551CC0 (name?)
constexpr u32 SmallFree = 0x00552420;         // (void* p, u32 size)
constexpr u32 FreeList_Alloc = 0x00556C04;    // (FreeList*) -> item
constexpr u32 FreeList_Free = 0x00556D18;     // (FreeList*, void* item)
constexpr u32 FreeList_Destroy = 0x005568FC;  // (FreeList*)
// still recompiled
constexpr u32 SmallAlloc_Impl = 0x00551CC0;   // 435 instructions: size-class table at kSizeClasses (name?)
constexpr u32 FreeList_AddBlock = 0x005518FC; // (FreeList*, u32 flag) src/container/freelist.cpp
constexpr u32 FreeList_Init = 0x00551A58;     // (FreeList*, elem_size, count, heap, bool) src/game/load_util.cpp
constexpr u32 PtrVector_Init = 0x00551598;    // src/container/vector.cpp
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 strcpy = 0x007239D8;
constexpr u32 memalign_r = 0x0072A470;        // _memalign_r(reent, align, size)
constexpr u32 free_r = 0x007277D0;            // _free_r(reent, p)
constexpr u32 malloc_r = 0x00728608;          // _malloc_r(reent, size)
constexpr u32 Np_Stub1 = 0x0056DF78;          // () -> 1 (src/common/misc.cpp)
}  // namespace cmfn

constexpr u32 kReent = 0x011DDE98;  // the C library's struct _reent (from the TOC)

// LinearHeap (mem.h) vtable 0x00B242D0: +0 destructor, +4 deleting destructor, +8 Free(p).
constexpr u32 kLinearHeapVtbl = 0x00B242D0;

// ApeTFreeList<T> (60 bytes). Items carry a 16-byte header { next, prev, ... }.
constexpr u32 kFlUsedHead = 4, kFlUsedTail = 8, kFlUsedCount = 12;
constexpr u32 kFlFreeHead = 16, kFlFreeTail = 20, kFlFreeCount = 24;
constexpr u32 kFlBlockOldest = 28, kFlBlockNewest = 32, kFlBlockCount = 36;
constexpr u32 kFlHeap = 40, kFlElemSize = 44, kFlUsedPeak = 48;
constexpr u32 kItemHeader = 16;
constexpr u32 kFreeListBaseVtbl = 0x00B24298;   // ApeTFreeList<T> base, set by the destructors
constexpr u32 kRtFreeListVtbl = 0x00B23DC0;     // ApeTFreeList<AglRenderTarget>
constexpr u32 kInputFreeListVtbl = 0x00B243F8;  // ApeTFreeList<ApeInputDevice>
constexpr u32 kRtSize = 1840, kInputDeviceSize = 296;
constexpr u32 kHeapVfnFree = 8;  // heap->Free(p, size)

// The registry of every free list (see FreeList_Init, src/game/load_util.cpp).
constexpr u32 kFreeLists = 0x01165028, kFreeListsGuard = 0x01165020;
constexpr u32 kDefaultHeapPtr = 0x00F88700, kDefaultHeap = 0x01165048, kDefaultHeapGuard = 0x01165040;
constexpr u32 kDefaultHeapVtbl = 0x00B24278;

// Small-block allocator size classes: 10 entries of 80 bytes.
constexpr u32 kSizeClasses = 0x00F67C74;
constexpr u32 kSizeClassCount = 10, kSizeClassStride = 80;
constexpr u32 kClassLimit = 0;      // u32 largest size served
constexpr u32 kClassList = 8;       // FreeList*: vtable +16 = Free(p)
constexpr u32 kClassSlack = 12;     // u32 adjusted by limit - size on every free (name?)
constexpr u32 kSmallSlackTotal = 0x00F67F94;  // u32, the same over every class (name?)
constexpr u32 kListVfnFree = 16;

// Native functions (src/common).
using container::FreeList;
void* FreeList_Alloc(FreeList* list);
void FreeList_Free(FreeList* list, void* p);
void FreeList_Destroy(FreeList* list);
void Mem_FreeFrom(void* p, s32 pool);
void* SmallAlloc(u32 size);
void SmallFree(void* p, u32 size);
char* String_Assign(char* dst, const char* src);

inline u32 vfunc(u32 obj, u32 byte_offset) { return load<u32>(load<u32>(obj) + byte_offset); }
inline u32 call32(PPUContext& ctx, u32 addr, std::initializer_list<u64> args = {}) {
    return static_cast<u32>(call(ctx, addr, args));
}

}  // namespace common
}  // namespace sly4
