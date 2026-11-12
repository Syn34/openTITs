// Memory manager: the allocator front ends (Mem_Alloc, operator new,
// Mem_Free), the two dlmalloc-style pools and the accounting block at
// g::mem_stats, and the 36-byte linear heap object built by Heap_Construct.
#pragma once

#include <cstddef>

#include "opentit/native.h"
#include "../sly4.h"

namespace sly4 {

namespace fn {
// Decompiled in src/memory/alloc.cpp; the addresses for call().
constexpr u32 Mem_AllocPool = 0x0056C514;  // (u32 align, u32 size, s32 pool, s32 tag) -> pointer or 0
constexpr u32 Mem_FreePool = 0x0056C730;   // (void* p, s32 pool, s32 tag)
constexpr u32 Mem_InitSystemStats = 0x0056C2E4;  // () once: records/prints user memory, touches it
constexpr u32 Pool_Alloc = 0x00565BD0;     // (MemPool*, u32 align, u32 size) -> pointer or 0
constexpr u32 Pool_Free = 0x00565C50;      // (MemPool*, void* p)
// Still recompiled.
constexpr u32 Mspace_Memalign = 0x00550690;  // (mspace, u32 align, u32 size) -> pointer (dlmalloc)
constexpr u32 Mspace_Free = 0x0054EE9C;      // (mspace, void* p) (dlmalloc)
}  // namespace fn

// C library (still recompiled).
namespace libc {
constexpr u32 memalign = 0x0072A88C;  // (u32 align, u32 size)
constexpr u32 free = 0x0072A8FC;      // (void* p)
constexpr u32 malloc = 0x0072A928;    // (u32 size)
constexpr u32 printf = 0x007240F4;    // (const char* fmt, ...)
}  // namespace libc

// The allocator's pool ids (Mem_Alloc's second argument). Anything other
// than 4 and 5 goes to the C library's memalign.
enum MemPoolId : s32 {
    kPoolSystem = 0,     // 0..3 and >5: memalign/free (Mem_Free passes 1, the lazy heap 3)
    kPoolA = 4,          // MemPool at kMemPoolA (name?)
    kPoolB = 5,          // MemPool at kMemPoolB (name?)
};
constexpr u32 kMemPoolA = 0x0116B340;  // MemPool for pool 4
constexpr u32 kMemPoolB = 0x0116B328;  // MemPool for pool 5

// Allocations carry dlmalloc chunk headers: the size word sits just before
// the user pointer, with the low 4 bits used as flags.
inline u32 chunk_size(const void* p) { return *native::host<be_u32>(native::guest(p) - 4) & ~0xFu; }

// A dlmalloc mspace with byte accounting.
struct MemPool {
    be_u32 base;     // +0 start of the pool memory
    be_u32 size;     // +4 bytes
    gptr mspace;      // +8 (dlmalloc mspace)
    be_u32 used;      // +12 bytes allocated
    be_u32 free;      // +16 bytes still free (no lower bound check)
    be_u32 peak_used; // +20 high-water mark of `used`
};
static_assert(sizeof(MemPool) == 24);

// g::mem_stats: byte counts kept by the allocator front ends. Tags index
// the per-tag arrays; every caller seen so far passes tag 0.
struct MemStats {
    u8 unk0[60];
    be_u32 unk60;              // +60, zeroed by Mem_InitSystemStats
    be_u32 unk64;              // +64, zeroed by Mem_InitSystemStats
    u8 unk68[8];
    gptr spurs_core;           // +76  CellSpurs "ApeCore" (5 SPUs)
    gptr spurs_1spu;           // +80  CellSpurs "ApeSpurs_1SPU"
    u8 unk84[104];
    be_u32 system_used;        // +188 memalign bytes
    be_u32 system_free;        // +192 starts as available_user_memory
    be_u32 pool_b_used;        // +196
    be_u32 pool_b_free;        // +200
    be_u32 pool_b_min_free;    // +204 low-water mark of pool_b_free
    be_u32 pool_a_used;        // +208
    be_u32 pool_a_free;        // +212
    be_u32 pool_a_min_free;    // +216 low-water mark of pool_a_free
    be_u32 tag_used[8];        // +220 every pool
    be_u32 tag_system[8];      // +252
    be_u32 tag_pool_a[8];      // +284
    be_u32 tag_pool_b[8];      // +316
    be_u32 image_size;         // +348 ELF image: 0x10000 .. end of bss
    be_u32 code_size;          // +352
    be_u32 data_size;          // +356 data + bss
    be_u32 module_memory;      // +360 user memory taken by cellSysmoduleLoadModule
};
static_assert(sizeof(MemStats) == 364);

// The 36-byte heap object Heap_Construct builds (vtable 0x00B242D0). It
// owns one block taken from the allocator and hands it out from `cursor`
// (its virtual free pops the cursor back), so it looks like a linear/stack
// heap. LinearHeap is our name.
struct LinearHeap {
    gptr vtable;     // +0
    gptr base;       // +4  block from Mem_Alloc(size, pool, 16), or 0
    gptr end;        // +8  base + size (0 if the allocation failed)
    gptr cursor;     // +12 starts at base
    be_u32 unk16;    // +16 not set by the constructor
    be_u32 size;     // +20
    be_s32 pool;     // +24 pool the block came from (the destructor frees it there)
    u8 owns;         // +28 (name?) the lazy heap passes 1, Main_InitSystems 0
    u8 pad29[3];
    be_u32 min_free; // +32 lowest free space seen (starts at size)
};
static_assert(sizeof(LinearHeap) == 36);
constexpr u32 kLinearHeapVtable = 0x00B242D0;

// Mem_Alloc / operator new set up a default heap on first use.
struct MemState {
    u8 initialised;  // +0
    u8 pad1[3];
    native::gp<LinearHeap> heap;  // +4 LinearHeap of kDefaultLinearHeapSize from pool 3, or 0
    u8 unk8;         // +8 set to 0
    u8 unk9;         // +9 set to 1
};
static_assert(offsetof(MemState, heap) == 4);
constexpr u32 kMemState = 0x01164898;
constexpr u32 kDefaultLinearHeapSize = 0x80000;
constexpr s32 kDefaultLinearHeapPool = 3;

// Native functions (src/memory).
namespace memory {
void* Mem_Alloc(u32 size, s32 pool, u32 align);
void* Mem_New(u32 size);
void Mem_Free(void* p);
u32 Heap_Construct(LinearHeap* heap, u32 size, s32 pool, u8 owns);
void* Mem_AllocPool(u32 align, u32 size, s32 pool, s32 tag);
void Mem_FreePool(void* p, s32 pool, s32 tag);
void* Pool_Alloc(MemPool* pool, u32 align, u32 size);
void Pool_Free(MemPool* pool, void* p);
void Mem_InitSystemStats();
LinearHeap* Mem_GetDefaultStack();
inline MemState& mem_state() { return native::global<MemState>(kMemState); }
inline MemStats& mem_stats() { return native::global<MemStats>(g::mem_stats); }
}  // namespace memory

}  // namespace sly4
