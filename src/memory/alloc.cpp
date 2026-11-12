// Allocator front ends and their accounting.
//
// Every allocation goes through Mem_AllocPool, which picks a backend by pool
// id (4 and 5: the two MemPools, anything else: the C library's memalign)
// and counts the dlmalloc chunk size in g::mem_stats, globally and per tag.
// Mem_Alloc and operator new make sure the memory manager is set up first.
#include "mem.h"

namespace sly4::memory {
using namespace opentit::native;

namespace {

constexpr u32 kStatsInitialised = 0x00F88778;  // u8, Mem_InitSystemStats' once flag
constexpr u32 kLowMemoryMb = 170;               // below this, warn about GCM debug
constexpr u32 kMemoryKeptBack = 0x600000;       // not touched by Mem_InitSystemStats
constexpr u32 kHeapAlign = 16;

// Strings (in the EBOOT).
constexpr u32 kStrNewlines = 0x00AF3A30;   // "\n\n\n\n"
constexpr u32 kStrStars = 0x00AF3BBC;      // "*****..."
constexpr u32 kStrTotalMb = 0x00AF3B30;    // "  totalUserMemory:       %3dMb\n"
constexpr u32 kStrAvailMb = 0x00AF3B50;    // "  available_user_memory: %3dMb\n"
constexpr u32 kStrLowMemory = 0x00AF3C14;  // "* Low available memory, do you have GCM Debug enabled? \n"

// A per-tag counter: counters[tag], wrapping like the original's address
// arithmetic (tags are 0 in practice).
be_u32& tag_counter(be_u32 (&counters)[8], s32 tag) {
    return *host<be_u32>(guest(counters) + static_cast<u32>(tag) * 4);
}

MemPool* pool_of(bool a) { return host<MemPool>(a ? kMemPoolA : kMemPoolB); }

// Body of Heap_Construct, also inlined into the lazy set-up. Returns the
// end pointer (what the original leaves in r3).
u32 linear_heap_construct(LinearHeap* h, u32 size, s32 pool, u8 owns) {
    h->size = size;
    h->base = 0;
    h->vtable = kLinearHeapVtable;
    h->end = 0;
    h->cursor = 0;
    h->pool = pool;
    h->min_free = size;
    h->owns = owns;
    const u32 base = guest(Mem_Alloc(size, pool, kHeapAlign));
    h->base = base;
    const u32 end = base != 0 ? base + h->size : 0;
    h->end = end;
    h->cursor = base;
    return end;
}

// First use of Mem_Alloc / operator new: account for system memory and
// create the default 512 KiB linear heap (whose block is the first real
// allocation; the guard is already set, so the nested calls go straight
// through).
void mem_lazy_init() {
    MemState& st = mem_state();
    st.initialised = 1;
    st.unk8 = 0;
    st.unk9 = 1;
    Mem_InitSystemStats();
    LinearHeap* heap = static_cast<LinearHeap*>(Mem_New(sizeof(LinearHeap)));
    if (heap) linear_heap_construct(heap, kDefaultLinearHeapSize, kDefaultLinearHeapPool, 1);
    st.heap = heap;
}

}  // namespace

// The game's general allocator.
void* Mem_Alloc(u32 size, s32 pool, u32 align) {
    if (mem_state().initialised == 0) mem_lazy_init();
    return Mem_AllocPool(align, size, pool, 0);
}

// operator new(size): Mem_Alloc(size, 0, 16). The original inlines its own
// recursion (and Heap_Construct) five levels deep; with the guard set
// first, only the outermost level can run, which is what this does.
void* Mem_New(u32 size) {
    if (mem_state().initialised == 0) mem_lazy_init();
    return Mem_AllocPool(kHeapAlign, size, kPoolSystem, 0);
}

// Frees a block from the system pool.
void Mem_Free(void* p) { Mem_FreePool(p, 1, 0); }

// Builds a LinearHeap over a new block of `size` bytes from `pool`.
// Returns base + size, or 0 if the block couldn't be allocated.
u32 Heap_Construct(LinearHeap* heap, u32 size, s32 pool, u8 owns) {
    return linear_heap_construct(heap, size, pool, owns);
}

void* Mem_AllocPool(u32 align, u32 size, s32 pool, s32 tag) {
    MemStats& s = mem_stats();
    void* p;
    if (pool == kPoolA || pool == kPoolB) {
        const bool a = pool == kPoolA;
        p = Pool_Alloc(pool_of(a), align, size);
        if (p) {
            const u32 n = chunk_size(p);
            be_u32& used = a ? s.pool_a_used : s.pool_b_used;
            be_u32& pool_free = a ? s.pool_a_free : s.pool_b_free;
            be_u32& min_free = a ? s.pool_a_min_free : s.pool_b_min_free;
            used += n;
            pool_free -= n;
            const u32 old_min = min_free, now = pool_free;
            min_free = old_min < now ? old_min : now;
            tag_counter(a ? s.tag_pool_a : s.tag_pool_b, tag) += n;
        }
    } else {
        p = call<void*>(libc::memalign, align, size);
        if (p) {
            const u32 n = chunk_size(p);
            s.system_used += n;
            s.system_free -= n;  // no low-water mark for the system pool
            tag_counter(s.tag_system, tag) += n;
        }
    }
    if (p) tag_counter(s.tag_used, tag) += chunk_size(p);
    return p;
}

// Undoes Mem_AllocPool's accounting and frees `p` in its backend.
// The original reads each per-tag pool counter before it stores the pool
// totals; that order only matters for negative tags (whose counters would
// overlap the totals), and every caller passes tag 0.
void Mem_FreePool(void* p, s32 pool, s32 tag) {
    if (!p) return;
    MemStats& s = mem_stats();
    const u32 n = chunk_size(p);
    tag_counter(s.tag_used, tag) -= n;

    if (pool == kPoolA || pool == kPoolB) {
        const bool a = pool == kPoolA;
        (a ? s.pool_a_used : s.pool_b_used) -= n;
        (a ? s.pool_a_free : s.pool_b_free) += n;
        tag_counter(a ? s.tag_pool_a : s.tag_pool_b, tag) -= n;
        Pool_Free(pool_of(a), p);
    } else {
        s.system_used -= n;
        s.system_free += n;
        tag_counter(s.tag_system, tag) -= n;
        call(libc::free, p);
    }
}

void* Pool_Alloc(MemPool* pool, u32 align, u32 size) {
    void* p = call<void*>(fn::Mspace_Memalign, pool->mspace.get(), align, size);
    if (p) {
        const u32 n = chunk_size(p);
        pool->used += n;
        pool->free -= n;
        const u32 used = pool->used, peak = pool->peak_used;
        pool->peak_used = used > peak ? used : peak;
    }
    return p;
}

void Pool_Free(MemPool* pool, void* p) {
    if (!p) return;
    const u32 n = chunk_size(p);
    pool->used -= n;
    pool->free += n;
    call(fn::Mspace_Free, pool->mspace.get(), p);
}

// Once: records available user memory as the system pool's free bytes,
// prints the totals (with a warning if under 170 MB are left), then mallocs,
// zeroes and frees all but 6 MB of it.
void Mem_InitSystemStats() {
    u8& done = global<u8>(kStatsInitialised);
    if (done != 0) return;
    done = 1;
    MemStats& s = mem_stats();
    s.unk60 = 0;
    s.unk64 = 0;

    u32 total, avail;
    {
        GuestFrame frame(sizeof(MemoryInfo));
        MemoryInfo* info = frame.as<MemoryInfo>();
        lv2::syscall(sc::sys_memory_get_user_memory_size, info);
        total = info->total_user_memory;
        avail = info->available_user_memory;
    }
    s.system_free = avail;
    call(libc::printf, kStrTotalMb, total >> 20);
    call(libc::printf, kStrAvailMb, avail >> 20);
    if ((avail >> 20) < kLowMemoryMb) {
        call(libc::printf, kStrNewlines);
        call(libc::printf, kStrStars);
        call(libc::printf, kStrLowMemory);
        call(libc::printf, kStrStars);
        call(libc::printf, kStrNewlines);
    }
    const u32 size = avail - kMemoryKeptBack;
    void* block = call<void*>(libc::malloc, size);
    call(fn::memset, block, 0, size);
    call(libc::free, block);
}

}  // namespace sly4::memory

NATIVE(0054ED38, sly4::memory::Mem_Alloc)
NATIVE(007A3DD8, sly4::memory::Mem_New)
NATIVE(005611B0, sly4::memory::Mem_Free)
NATIVE(0054ECA0, sly4::memory::Heap_Construct)
NATIVE(0056C514, sly4::memory::Mem_AllocPool)
NATIVE(0056C730, sly4::memory::Mem_FreePool)
NATIVE(00565BD0, sly4::memory::Pool_Alloc)
NATIVE(00565C50, sly4::memory::Pool_Free)
NATIVE(0056C2E4, sly4::memory::Mem_InitSystemStats)
