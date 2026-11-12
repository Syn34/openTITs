// The three allocators that sit on the default LinearHeap ("ApeMemStack"):
// the bucket allocator for small blocks, the default heap's Alloc and the
// stack allocation itself. All three inline the memory manager's lazy
// set-up, which is shared here as MemState_EnsureInit.
// Addresses are guest addresses in the BCES01284 v01.02 EBOOT. A "?" in a
// comment marks a guess from how the code is used.
#pragma once

#include <cstddef>

#include "../container/container.h"
#include "../memory/mem.h"

namespace sly4 {
namespace heap {

namespace hfn {
constexpr u32 SmallAlloc_Impl = 0x00551CC0;    // (u32 size) -> pointer  src/heap/bucket.cpp
constexpr u32 FreeList_Init = 0x00551A58;      // (FreeList*, elem_size, count, heap, bool) src/game/load_util.cpp
constexpr u32 String_Printf = 0x00551540;      // sprintf (src/string/string_misc.cpp)
}  // namespace hfn

// Size classes of the bucket allocator: 10 entries of 80 bytes.
constexpr u32 kBuckets = 0x00F67C74;
constexpr u32 kBucketCount = 10, kBucketStride = 80;
constexpr u32 kBucketLimit = 0;   // u32 largest size served
constexpr u32 kBucketCount4 = 4;  // u32 free-list block size when the config gives none (name?)
constexpr u32 kBucketList = 8;    // FreeList*, made on first use
constexpr u32 kBucketSlack = 12;  // u32 += limit - size per allocation (wasted bytes?)
constexpr u32 kBucketName = 16;   // char[]: "%s:%5d" of the allocator's name and limit
constexpr u32 kBucketSlackTotal = 0x00F67F94;   // u32, all buckets
constexpr u32 kConfigBucketCounts = 0x190;      // g::config s16[10]: per-bucket block size, 0 = default
constexpr u32 kBucketNameFormat = 0x00AF36CC;   // "%s:%5d"
constexpr u32 kBucketFuncName = 0x00AF36A2;     // "void *ApeMem::_BucketAlloc(unsigned int)"
constexpr u32 kMallocFallbackFormat = 0x00AF36D4;  // "ApeMemStack::Alloc using malloc:%d\n"

// ApeTFreeList<T> as the bucket allocator makes it (60 bytes, base vtable).
constexpr u32 kFreeListBaseVtbl = 0x00B24298;
constexpr u32 kFreeListEmptyName = 0x00B0D928;  // "" (name pointer at +56 until the bucket sets it)
constexpr u32 kFreeListSize = 60;
constexpr u32 kFreeListVfnAlloc = 12;

constexpr u32 kHeapAlign = 16;

// LinearHeap fields the stack allocation uses (mem.h names them).
constexpr u32 kStackEnd = 8, kStackCursor = 12, kStackOwns = 28, kStackMinFree = 32;
constexpr u32 kStackHeader = 16;  // each allocation: { u32 size; pad } then the data

// One size class (80 bytes; kBucketCount of them at kBuckets).
struct Bucket {
    be_u32 limit;                          // +0 largest size served
    be_u32 count4;                         // +4 free-list block size when the config gives none (name?)
    native::gp<container::FreeList> list;  // +8 made on first use
    be_u32 slack;                          // +12 += limit - size per allocation (wasted bytes?)
    char name[64];                         // +16 "%s:%5d" of the allocator's name and limit
};
static_assert(sizeof(Bucket) == kBucketStride && offsetof(Bucket, name) == kBucketName);

// The memory manager's lazy set-up, native form of MemState_EnsureInit below.
inline void MemState_EnsureInitNative() {
    MemState& st = memory::mem_state();
    if (st.initialised != 0) return;
    st.initialised = 1;
    st.unk8 = 0;
    st.unk9 = 1;
    memory::Mem_InitSystemStats();
    LinearHeap* obj = nullptr;
    if (LinearHeap* h = static_cast<LinearHeap*>(memory::Mem_New(sizeof(LinearHeap)))) {
        h->base = 0;
        h->end = 0;
        h->vtable = kLinearHeapVtable;
        h->cursor = 0;
        h->size = kDefaultLinearHeapSize;
        h->pool = kDefaultLinearHeapPool;
        h->owns = 1;
        h->min_free = kDefaultLinearHeapSize;
        const u32 base = native::guest(memory::Mem_AllocPool(kHeapAlign, kDefaultLinearHeapSize, kDefaultLinearHeapPool, 0));
        h->base = base;
        h->end = base != 0 ? base + h->size : 0;
        h->cursor = base;
        obj = h;
    }
    st.heap = obj;
}

void* SmallAlloc_Impl(u32 size);
void* DefaultHeap_AllocAligned(void* heap, u32 size, u8 clear, u32 align);
void* MemStack_Alloc(LinearHeap* stack, u32 size);

// The memory manager's lazy set-up, as Mem_New / Mem_Alloc do it (register
// form, still used by src/obj/obj9_spline.cpp; native code uses
// MemState_EnsureInitNative). The
// original inlines its own recursion here several levels deep; with the
// guard set before the nested Mem_New(36), only this outermost level can
// run. The heap's block comes straight from Mem_AllocPool (the inlined
// Mem_Alloc(0x80000, 3, 16)).
inline void MemState_EnsureInit(PPUContext& ctx) {
    MemState& st = ref<MemState>(kMemState);
    if (st.initialised != 0) return;
    st.initialised = 1;
    st.unk8 = 0;
    st.unk9 = 1;
    call(ctx, fn::Mem_InitSystemStats);
    u32 obj = 0;
    const u32 mem = static_cast<u32>(call(ctx, fn::Mem_New, {sizeof(LinearHeap)}));
    if (mem != 0) {
        LinearHeap& h = ref<LinearHeap>(mem);
        h.base = 0;
        h.end = 0;
        h.vtable = kLinearHeapVtable;
        h.cursor = 0;
        h.size = kDefaultLinearHeapSize;
        h.pool = kDefaultLinearHeapPool;
        h.owns = 1;
        h.min_free = kDefaultLinearHeapSize;
        const u32 base = static_cast<u32>(call(ctx, fn::Mem_AllocPool,
                                               {kHeapAlign, kDefaultLinearHeapSize,
                                                static_cast<u64>(static_cast<s64>(kDefaultLinearHeapPool)), 0}));
        h.base = base;
        h.end = base != 0 ? base + h.size : 0;
        h.cursor = base;
        obj = mem;
    }
    st.heap = native::host<LinearHeap>(obj);
}

}  // namespace heap
}  // namespace sly4
