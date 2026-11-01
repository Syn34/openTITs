// Small utilities reached from Game_Load: String_Format (printf into a ring
// of static buffers), the thread checks it uses, vsprintf, and
// FreeList_Init, which every ApeTFreeList<T> allocator goes through.
#include "load.h"

using namespace sly4;
using namespace sly4::gameload;

namespace {

constexpr u32 kFormatBufSize = 1024;
constexpr u32 kFormatBufMask = 7 * kFormatBufSize;  // 8 buffers per ring

// The thread-id static, built by whoever reaches it first (as in Main_InitSystems).
void ThreadIds_Get() {
    if (ref<u8>(lg::kThreadIdsGuard) == 0) {
        store<u64>(lg::kThreadIds, 0);
        store<u64>(lg::kThreadIds + 8, 0);
        ref<u8>(lg::kThreadIdsGuard) = 1;
    }
}

// True when no thread is recorded in slot `which` or it is the calling thread.
bool Thread_Is(PPUContext& ctx, u32 which) {
    ThreadIds_Get();
    const u64 recorded = load<u64>(lg::kThreadIds + which * 8);
    Stack frame(ctx, 8);
    store<u64>(frame.addr(), 0);
    call(ctx, limp::sys_ppu_thread_get_id, {frame.addr()});
    return recorded == 0 || load<u64>(frame.addr()) == recorded;
}

// A function-local static u32 counter, zeroed on first use, bumped atomically
// (lwarx/stwcx. in the original). Returns the value before the increment.
u32 AtomicCounter_Next(u32 counter, u32 guard) {
    if (ref<u8>(guard) == 0) {
        store<u32>(counter, 0);
        ref<u8>(guard) = 1;
    }
    for (;;) {
        const u32 old = mem::r32(counter);
        if (mem::cas32(counter, old, old + 1)) return old;
    }
}

// The default heap free lists use when given none: a static object at
// lg::kDefaultHeap, cached in lg::kDefaultHeapPtr.
u32 DefaultHeap_Get() {
    u32 heap = load<u32>(lg::kDefaultHeapPtr);
    if (heap != 0) return heap;
    if (ref<u8>(lg::kDefaultHeapGuard) == 0) {
        ref<u8>(lg::kDefaultHeapGuard) = 1;
        store<u32>(lg::kDefaultHeap, lg::kDefaultHeapVtbl);
    }
    store<u32>(lg::kDefaultHeap, lg::kDefaultHeapVtbl);  // constructed again in place, as in the original
    store<u32>(lg::kDefaultHeapPtr, lg::kDefaultHeap);
    return lg::kDefaultHeap;
}

}  // namespace

// bool Thread_IsMain()
// True on the main thread, or when it hasn't been recorded yet.
DECOMP(005DB0A8, Thread_IsMain) { ret(ctx, Thread_Is(ctx, 0) ? 1u : 0u); }

// bool Thread_IsSecond()  (name?)
// Same for the second thread id in the static (+8).
DECOMP(005DB13C, Thread_IsSecond) { ret(ctx, Thread_Is(ctx, 1) ? 1u : 0u); }

// char* String_Format(const char* fmt, ...)
// vsprintf into the next of 8 static 1 KiB buffers and return it. The main
// thread, the second thread and any other thread each have a ring of their
// own; only the main thread's counter is bumped without an atomic.
DECOMP(005534D0, String_Format) {
    // Spill the arguments to the caller's parameter save area, as the
    // variadic original does: fmt at +52, the rest as doublewords from +56.
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    store<u32>(sp + 52, static_cast<u32>(ctx.gpr[3]));
    for (u32 k = 0; k < 7; ++k) store<u64>(sp + 56 + k * 8, ctx.gpr[4 + k]);
    const u32 va = sp + 56;

    u32 n, ring;
    if (call32(ctx, lfn::Thread_IsMain) != 0) {
        n = load<u32>(lg::kFormatCountMain) + 1;
        store<u32>(lg::kFormatCountMain, n);
        ring = lg::kFormatBufMain;
    } else if (call32(ctx, lfn::Thread_IsSecond) != 0) {
        n = AtomicCounter_Next(lg::kFormatCountSecond, lg::kFormatGuardSecond);
        ring = lg::kFormatBufSecond;
    } else {
        n = AtomicCounter_Next(lg::kFormatCountOther, lg::kFormatGuardOther);
        ring = lg::kFormatBufOther;
    }
    const u32 buf = ring + ((n << 10) & kFormatBufMask);
    call(ctx, lfn::vsprintf, {buf, load<u32>(sp + 52), va});
    ret(ctx, buf);
}

// int vsprintf(char* dst, const char* fmt, va_list va)
// The C library's: formats through the shared printf core with an output
// function that appends to dst, then terminates dst when the count is >= 0.
DECOMP(00724588, vsprintf) {
    const u32 dst = arg<u32>(ctx, 0);
    const u32 fmt = arg<u32>(ctx, 1);
    const u32 va = arg<u32>(ctx, 2);
    const s32 n = static_cast<s32>(call32(ctx, lfn::vprintf_core, {lg::kSprintfOutput, dst, fmt, va}));
    if (n >= 0) store<u8>(dst + static_cast<u32>(n), 0);
    ret(ctx, n);
}

// void FreeList_Init(FreeList* list, u32 elem_size, u32 count, Heap* heap, bool flag)
// Second half of ApeTFreeList<T> construction: records the element size
// and heap (the default heap when 0), works out the block count, and adds
// the list to the global registry of free lists.
DECOMP(00551A58, FreeList_Init) {
    const u32 list = arg<u32>(ctx, 0);
    const u32 elem_size = arg<u32>(ctx, 1);
    const u32 count = arg<u32>(ctx, 2);
    u32 heap = arg<u32>(ctx, 3);
    const u32 flag = arg<u32>(ctx, 4);

    field<u32>(list, kFreeListElemSize) = elem_size;
    if (heap == 0) heap = DefaultHeap_Get();
    field<u32>(list, kFreeListHeap) = heap;
    field<u32>(list, kFreeList48) = 0;
    field<u16>(list, kFreeListBlockCount) = static_cast<u16>(count > 1 ? count : 1);
    if (count != 0) {
        call(ctx, lfn::FreeList_AddBlock, {list, (flag & 0xFF) != 0 ? 1u : 0u});
        u32 blocks = count;
        if (count > 16) {
            blocks = count >> 2;
            if (!(blocks > 16)) blocks = 16;
        }
        field<u16>(list, kFreeListBlockCount) = static_cast<u16>(blocks);
    }

    // The registry: a PtrVector of every free list, made on first use.
    const u32 reg = lg::kFreeLists;
    if (ref<u8>(lg::kFreeListsGuard) == 0) {
        const u32 reg_heap = DefaultHeap_Get();
        field<u32>(reg, 0) = 0;
        field<u16>(reg, 4) = 0;
        field<u16>(reg, 8) = 0;
        field<u16>(reg, 10) = 0;
        field<u32>(reg, 16) = 0;
        field<u16>(reg, 6) = 4;
        field<u32>(reg, 12) = 0;  // the original clears its bit-fields one by one, which leaves 0
        call(ctx, lfn::PtrVector_Init, {reg, 64, reg_heap, 0, 0});
        ref<u8>(lg::kFreeListsGuard) = 1;
    }
    u32 n = field<u16>(reg, 8);
    const u32 capacity = field<u16>(reg, 4);
    if (n == capacity) {
        call(ctx, lfn::PtrVector_Reserve, {reg, (capacity << 1) & 0x1FFFE});
        n = field<u16>(reg, 8);
    }
    store<u32>(load<u32>(reg) + ((n << 2) & 0x3FFFC), list);
    const u32 next = field<u16>(reg, 8) + 1u;
    const u32 high = field<u16>(reg, 10);
    field<u16>(reg, 8) = static_cast<u16>(next);
    field<u16>(reg, 10) = static_cast<u16>(static_cast<s32>(high) > static_cast<s32>(next & 0xFFFF) ? high : next);
}
