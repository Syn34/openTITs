// Allocator front ends and their accounting.
//
// Every allocation goes through Mem_AllocPool, which picks a backend by pool
// id (4 and 5: the two MemPools, anything else: the C library's memalign)
// and counts the dlmalloc chunk size in g::mem_stats, globally and per tag.
// Mem_Alloc and operator new make sure the memory manager is set up first.
#include "mem.h"

using namespace sly4;

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

MemStats& stats() { return ref<MemStats>(g::mem_stats); }

// A per-tag counter: base offset into MemStats + 4 * tag, wrapping like the
// original's address arithmetic (tags are 0 in practice).
be_u32& tag_counter(u32 offset, s32 tag) {
    return field<u32>(g::mem_stats, offset + static_cast<u32>(tag) * 4);
}
constexpr u32 kTagUsed = 220, kTagSystem = 252, kTagPoolA = 284, kTagPoolB = 316;

// Body of Heap_Construct, also inlined into the lazy set-up. Returns the
// end pointer (what the original leaves in r3).
u32 linear_heap_construct(PPUContext& ctx, u32 obj, u64 size, u64 pool, u8 owns) {
    LinearHeap& h = ref<LinearHeap>(obj);
    h.size = static_cast<u32>(size);
    h.base = 0;
    h.vtable = kLinearHeapVtable;
    h.end = 0;
    h.cursor = 0;
    h.pool = static_cast<s32>(pool);
    h.min_free = static_cast<u32>(size);
    h.owns = owns;
    const u32 base = static_cast<u32>(call(ctx, fn::Mem_Alloc, {size, pool, kHeapAlign}));
    h.base = base;
    const u32 end = base != 0 ? base + h.size : 0;
    h.end = end;
    h.cursor = base;
    return end;
}

// First use of Mem_Alloc / operator new: account for system memory and
// create the default 512 KiB linear heap (whose block is the first real
// allocation; the guard is already set, so the nested calls go straight
// through).
void mem_lazy_init(PPUContext& ctx) {
    MemState& st = ref<MemState>(kMemState);
    st.initialised = 1;
    st.unk8 = 0;
    st.unk9 = 1;
    call(ctx, fn::Mem_InitSystemStats);
    const u32 heap = static_cast<u32>(call(ctx, fn::Mem_New, {sizeof(LinearHeap)}));
    if (heap != 0) linear_heap_construct(ctx, heap, kDefaultLinearHeapSize, kDefaultLinearHeapPool, 1);
    st.heap = heap;
}

}  // namespace

// void* Mem_Alloc(u32 size, s32 pool, u32 align)
// The game's general allocator.
DECOMP(0054ED38, Mem_Alloc) {
    const u64 size = arg<u64>(ctx, 0);
    const u64 pool = arg<u64>(ctx, 1);
    const u32 align = arg<u32>(ctx, 2);
    if (ref<MemState>(kMemState).initialised == 0) mem_lazy_init(ctx);
    ctx.gpr[3] = call(ctx, fn::Mem_AllocPool, {align, size, pool, 0});
}

// void* operator new(u32 size)
// Mem_Alloc(size, 0, 16). The original inlines its own recursion (and
// Heap_Construct) five levels deep; with the guard set first, only the
// outermost level can run, which is what this does.
DECOMP(007A3DD8, Mem_New) {
    const u64 size = arg<u64>(ctx, 0);
    if (ref<MemState>(kMemState).initialised == 0) mem_lazy_init(ctx);
    ctx.gpr[3] = call(ctx, fn::Mem_AllocPool, {kHeapAlign, size, kPoolSystem, 0});
}

// void Mem_Free(void* p)
// Frees a block from the system pool.
DECOMP(005611B0, Mem_Free) {
    ctx.gpr[3] = call(ctx, fn::Mem_FreePool, {ctx.gpr[3], 1, 0});
}

// LinearHeap* Heap_Construct(LinearHeap* this, u32 size, s32 pool, bool owns)
// Builds a LinearHeap over a new block of `size` bytes from `pool`.
// Returns base + size, or 0 if the block couldn't be allocated.
DECOMP(0054ECA0, Heap_Construct) {
    const u32 obj = arg<u32>(ctx, 0);
    const u64 size = arg<u64>(ctx, 1);
    const u64 pool = arg<u64>(ctx, 2);
    const u8 owns = arg<u8>(ctx, 3);
    ret(ctx, linear_heap_construct(ctx, obj, size, pool, owns));
}

// void* Mem_AllocPool(u32 align, u32 size, s32 pool, s32 tag)
DECOMP(0056C514, Mem_AllocPool) {
    const u64 align = arg<u64>(ctx, 0);
    const u64 size = arg<u64>(ctx, 1);
    const s32 pool = arg<s32>(ctx, 2);
    const s32 tag = arg<s32>(ctx, 3);
    MemStats& s = stats();

    u64 result;
    if (pool == kPoolA || pool == kPoolB) {
        const bool a = pool == kPoolA;
        result = call(ctx, fn::Pool_Alloc, {a ? kMemPoolA : kMemPoolB, align, size});
        const u32 p = static_cast<u32>(result);
        if (p != 0) {
            const u32 n = chunk_size(p);
            be_u32& used = a ? s.pool_a_used : s.pool_b_used;
            be_u32& pool_free = a ? s.pool_a_free : s.pool_b_free;
            be_u32& min_free = a ? s.pool_a_min_free : s.pool_b_min_free;
            used += n;
            pool_free -= n;
            const u32 old_min = min_free, now = pool_free;
            min_free = old_min < now ? old_min : now;
            tag_counter(a ? kTagPoolA : kTagPoolB, tag) += n;
        }
    } else {
        result = call(ctx, libc::memalign, {align, size});
        const u32 p = static_cast<u32>(result);
        if (p != 0) {
            const u32 n = chunk_size(p);
            s.system_used += n;
            s.system_free -= n;  // no low-water mark for the system pool
            tag_counter(kTagSystem, tag) += n;
        }
    }
    const u32 p = static_cast<u32>(result);
    if (p != 0) tag_counter(kTagUsed, tag) += chunk_size(p);
    ctx.gpr[3] = result;
}

// void Mem_FreePool(void* p, s32 pool, s32 tag)
// Undoes Mem_AllocPool's accounting and frees `p` in its backend.
// The original reads each per-tag pool counter before it stores the pool
// totals; that order only matters for negative tags (whose counters would
// overlap the totals), and every caller passes tag 0.
DECOMP(0056C730, Mem_FreePool) {
    const u32 p = arg<u32>(ctx, 0);
    const s32 pool = arg<s32>(ctx, 1);
    const s32 tag = arg<s32>(ctx, 2);
    if (p == 0) return;  // r3 is still 0
    MemStats& s = stats();
    const u32 n = chunk_size(p);
    tag_counter(kTagUsed, tag) -= n;

    if (pool == kPoolA || pool == kPoolB) {
        const bool a = pool == kPoolA;
        (a ? s.pool_a_used : s.pool_b_used) -= n;
        (a ? s.pool_a_free : s.pool_b_free) += n;
        tag_counter(a ? kTagPoolA : kTagPoolB, tag) -= n;
        ctx.gpr[3] = call(ctx, fn::Pool_Free, {a ? kMemPoolA : kMemPoolB, ctx.gpr[3]});
    } else {
        s.system_used -= n;
        s.system_free += n;
        tag_counter(kTagSystem, tag) -= n;
        ctx.gpr[3] = call(ctx, libc::free, {ctx.gpr[3]});
    }
}

// void* Pool_Alloc(MemPool* pool, u32 align, u32 size)
DECOMP(00565BD0, Pool_Alloc) {
    const u32 pool_ea = arg<u32>(ctx, 0);
    MemPool& pool = ref<MemPool>(pool_ea);
    const u64 result = call(ctx, fn::Mspace_Memalign, {pool.mspace, ctx.gpr[4], ctx.gpr[5]});
    const u32 p = static_cast<u32>(result);
    if (p != 0) {
        const u32 n = chunk_size(p);
        pool.used += n;
        pool.free -= n;
        const u32 used = pool.used, peak = pool.peak_used;
        pool.peak_used = used > peak ? used : peak;
    }
    ctx.gpr[3] = result;
}

// void Pool_Free(MemPool* pool, void* p)
DECOMP(00565C50, Pool_Free) {
    const u32 pool_ea = arg<u32>(ctx, 0);
    const u32 p = arg<u32>(ctx, 1);
    if (p == 0) return ret(ctx, ctx.gpr[4]);  // r3 = p (0)
    MemPool& pool = ref<MemPool>(pool_ea);
    const u32 n = chunk_size(p);
    pool.used -= n;
    pool.free += n;
    ctx.gpr[3] = call(ctx, fn::Mspace_Free, {pool.mspace, ctx.gpr[4]});
}

// void Mem_InitSystemStats()
// Once: records available user memory as the system pool's free bytes,
// prints the totals (with a warning if under 170 MB are left), then mallocs,
// zeroes and frees all but 6 MB of it.
DECOMP(0056C2E4, Mem_InitSystemStats) {
    const u8 done = ref<u8>(kStatsInitialised);
    if (done != 0) return ret(ctx, static_cast<u32>(done));
    ref<u8>(kStatsInitialised) = 1;
    MemStats& s = stats();
    s.unk60 = 0;
    s.unk64 = 0;

    u32 total, avail;
    {
        Stack frame(ctx, sizeof(MemoryInfo));
        syscall(ctx, sc::sys_memory_get_user_memory_size, {frame.addr()});
        const MemoryInfo& info = ref<MemoryInfo>(frame.addr());
        total = info.total_user_memory;
        avail = info.available_user_memory;
    }
    s.system_free = avail;
    call(ctx, libc::printf, {kStrTotalMb, total >> 20});
    call(ctx, libc::printf, {kStrAvailMb, avail >> 20});
    if ((avail >> 20) < kLowMemoryMb) {
        call(ctx, libc::printf, {kStrNewlines});
        call(ctx, libc::printf, {kStrStars});
        call(ctx, libc::printf, {kStrLowMemory});
        call(ctx, libc::printf, {kStrStars});
        call(ctx, libc::printf, {kStrNewlines});
    }
    const u32 size = avail - kMemoryKeptBack;
    const u32 block = static_cast<u32>(call(ctx, libc::malloc, {size}));
    call(ctx, fn::memset, {block, 0, size});
    ctx.gpr[3] = call(ctx, libc::free, {block});
}
