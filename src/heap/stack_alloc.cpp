// ApeMemStack::Alloc: allocation from a LinearHeap.
#include "heap.h"

using namespace sly4;
using namespace sly4::heap;

// void* MemStack_Alloc(LinearHeap* stack, u32 size)
// Takes size (rounded up to 16) plus a 16-byte header from the stack's
// cursor and records the lowest free space seen in +32. When the stack is
// full an owning stack falls back to the system pool (with a printf), any
// other returns 0. The stack pointer isn't checked.
DECOMP(0055260C, MemStack_Alloc) {
    const u32 stack = arg<u32>(ctx, 0);
    const u32 size = arg<u32>(ctx, 1);
    MemState_EnsureInit(ctx);

    const u32 aligned = (size + 15) & ~15u;
    const u32 cur = field<u32>(stack, kStackCursor);
    const u32 end = field<u32>(stack, kStackEnd);
    const u32 next = (cur + aligned + 31) & ~15u;
    if (next <= end) {
        field<u32>(stack, kStackCursor) = next;
        u32 remaining = end - next;
        const u32 min_free = field<u32>(stack, kStackMinFree);
        if (min_free < remaining) remaining = min_free;
        field<u32>(stack, kStackMinFree) = remaining;
        store<u32>(cur, aligned);
        return ret(ctx, cur + kStackHeader);
    }
    if (ref<u8>(stack + kStackOwns) == 0) return ret(ctx, 0u);
    call(ctx, libc::printf, {kMallocFallbackFormat, size});
    MemState_EnsureInit(ctx);
    ret(ctx, call(ctx, fn::Mem_AllocPool, {kHeapAlign, size, kPoolSystem, 0}));
}
