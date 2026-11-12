// Small utilities reached from Game_Load: String_Format (printf into a ring
// of static buffers), the thread checks it uses, vsprintf, and
// FreeList_Init, which every ApeTFreeList<T> allocator goes through.
#include "load.h"

namespace sly4::gameload {
using namespace opentit::native;
using container::Allocator;
using container::FreeList;
using container::PtrVector;

namespace {

constexpr u32 kFormatBufSize = 1024;
constexpr u32 kFormatBufMask = 7 * kFormatBufSize;  // 8 buffers per ring

// The thread-id static ({ u64 main; u64 second; }), built by whoever
// reaches it first (as in Main_InitSystems).
be_u64* ThreadIds_Get() {
    be_u64* ids = host<be_u64>(lg::kThreadIds);
    if (global<u8>(lg::kThreadIdsGuard) == 0) {
        ids[0] = 0u;
        ids[1] = 0u;
        global<u8>(lg::kThreadIdsGuard) = 1;
    }
    return ids;
}

// True when no thread is recorded in slot `which` or it is the calling thread.
bool Thread_Is(u32 which) {
    const u64 recorded = ThreadIds_Get()[which];
    GuestFrame frame(8);
    be_u64* id = frame.as<be_u64>();
    *id = 0u;
    call(limp::sys_ppu_thread_get_id, id);
    return recorded == 0 || *id == recorded;
}

// A function-local static u32 counter, zeroed on first use, bumped atomically
// (lwarx/stwcx. in the original). Returns the value before the increment.
u32 AtomicCounter_Next(u32 counter, u32 guard) {
    if (global<u8>(guard) == 0) {
        global<be_u32>(counter) = 0u;
        global<u8>(guard) = 1;
    }
    for (;;) {
        const u32 old = mem::r32(counter);
        if (mem::cas32(counter, old, old + 1)) return old;
    }
}

// The default heap free lists use when given none: a static object at
// lg::kDefaultHeap, cached in lg::kDefaultHeapPtr.
Allocator* DefaultHeap_Get() {
    gp<Allocator>& slot = global<gp<Allocator>>(lg::kDefaultHeapPtr);
    if (slot) return slot;
    Allocator* heap = host<Allocator>(lg::kDefaultHeap);
    if (global<u8>(lg::kDefaultHeapGuard) == 0) {
        global<u8>(lg::kDefaultHeapGuard) = 1;
        heap->vtable = lg::kDefaultHeapVtbl;
    }
    heap->vtable = lg::kDefaultHeapVtbl;  // constructed again in place, as in the original
    slot = heap;
    return heap;
}

}  // namespace

// True on the main thread, or when it hasn't been recorded yet.
bool Thread_IsMain() { return Thread_Is(0); }

// Same for the second thread id in the static (+8).  (name?)
bool Thread_IsSecond() { return Thread_Is(1); }

// vsprintf into the next of 8 static 1 KiB buffers and return it. The main
// thread, the second thread and any other thread each have a ring of their
// own; only the main thread's counter is bumped without an atomic.
// Variadic: the arguments are spilled to the caller's parameter save area
// (fmt at +52, the rest as doublewords from +56), as the original does, and
// that area is the va_list; arguments past the eighth are already there.
// So it needs the guest caller's r1: native callers use
// call<char*>(0x005534D0, ...).
char* String_Format(const char* fmt, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6, u64 a7) {
    const u32 sp = static_cast<u32>(ctx().gpr[1]);  // the caller's stack pointer
    *host<be_u32>(sp + 52) = guest(fmt);
    be_u64* va = host<be_u64>(sp + 56);
    const u64 rest[7] = {a1, a2, a3, a4, a5, a6, a7};
    for (u32 k = 0; k < 7; ++k) va[k] = rest[k];

    u32 n, ring;
    if (Thread_IsMain()) {
        n = global<be_u32>(lg::kFormatCountMain) + 1;
        global<be_u32>(lg::kFormatCountMain) = n;
        ring = lg::kFormatBufMain;
    } else if (Thread_IsSecond()) {
        n = AtomicCounter_Next(lg::kFormatCountSecond, lg::kFormatGuardSecond);
        ring = lg::kFormatBufSecond;
    } else {
        n = AtomicCounter_Next(lg::kFormatCountOther, lg::kFormatGuardOther);
        ring = lg::kFormatBufOther;
    }
    char* buf = host<char>(ring + ((n << 10) & kFormatBufMask));
    Libc_Vsprintf(buf, host<const char>(*host<be_u32>(sp + 52)), guest(va));
    return buf;
}

// The C library's: formats through the shared printf core with an output
// function that appends to dst, then terminates dst when the count is >= 0.
s32 Libc_Vsprintf(char* dst, const char* fmt, u32 va) {
    const s32 n = call<s32>(lfn::vprintf_core, lg::kSprintfOutput, dst, fmt, va);
    if (n >= 0) dst[n] = 0;
    return n;
}

// Second half of ApeTFreeList<T> construction: records the element size
// and heap (the default heap when 0), works out the block count, and adds
// the list to the global registry of free lists.
void FreeList_Init(FreeList* list, u32 elem_size, u32 count, Allocator* heap, u8 flag) {
    list->elem_size = elem_size;
    if (!heap) heap = DefaultHeap_Get();
    list->heap = heap;
    list->used_peak = 0u;
    list->items_per_block = static_cast<u16>(count > 1 ? count : 1);
    if (count != 0) {
        container::FreeList_AddBlock(list, flag != 0 ? 1u : 0u);
        u32 blocks = count;
        if (count > 16) {
            blocks = count >> 2;
            if (!(blocks > 16)) blocks = 16;
        }
        list->items_per_block = static_cast<u16>(blocks);
    }

    // The registry: a PtrVector of every free list, made on first use.
    PtrVector* reg = host<PtrVector>(lg::kFreeLists);
    if (global<u8>(lg::kFreeListsGuard) == 0) {
        Allocator* reg_heap = DefaultHeap_Get();
        reg->data = nullptr;
        reg->capacity = 0;
        reg->count = 0;
        reg->high_water = 0;
        reg->allocator = nullptr;
        reg->elem_size = 4;
        reg->bits = 0u;  // the original clears its bit-fields one by one, which leaves 0
        container::PtrVector_Init(reg, 64, reg_heap, 0);
        global<u8>(lg::kFreeListsGuard) = 1;
    }
    u32 n = reg->count;
    const u32 capacity = reg->capacity;
    if (n == capacity) {
        container::PtrVector_Reserve(reg, (capacity << 1) & 0x1FFFE);
        n = reg->count;
    }
    gref<be_u32>(reg->data.ea + ((n << 2) & 0x3FFFC)) = guest(list);
    const u32 next = reg->count + 1u;
    const u32 high = reg->high_water;
    reg->count = static_cast<u16>(next);
    reg->high_water = static_cast<u16>(static_cast<s32>(high) > static_cast<s32>(next & 0xFFFF) ? high : next);
}

}  // namespace sly4::gameload

NATIVE(005DB0A8, sly4::gameload::Thread_IsMain)
NATIVE(005DB13C, sly4::gameload::Thread_IsSecond)
NATIVE(005534D0, sly4::gameload::String_Format)
NATIVE(00724588, sly4::gameload::Libc_Vsprintf)
NATIVE(00551A58, sly4::gameload::FreeList_Init)
