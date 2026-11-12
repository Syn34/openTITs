// ApeMemStack::Alloc: allocation from a LinearHeap.
#include "heap.h"

namespace sly4::heap {
using namespace opentit::native;

// Takes size (rounded up to 16) plus a 16-byte header from the stack's
// cursor and records the lowest free space seen in +32. When the stack is
// full an owning stack falls back to the system pool (with a printf), any
// other returns 0. The stack pointer isn't checked.
void* MemStack_Alloc(LinearHeap* stack, u32 size) {
    MemState_EnsureInitNative();

    const u32 aligned = (size + 15) & ~15u;
    const u32 cur = stack->cursor;
    const u32 end = stack->end;
    const u32 next = (cur + aligned + 31) & ~15u;
    if (next <= end) {
        stack->cursor = next;
        u32 remaining = end - next;
        const u32 min_free = stack->min_free;
        if (min_free < remaining) remaining = min_free;
        stack->min_free = remaining;
        *host<be_u32>(cur) = aligned;  // the allocation's header
        return host<void>(cur + kStackHeader);
    }
    if (stack->owns == 0) return nullptr;
    call(libc::printf, kMallocFallbackFormat, size);
    MemState_EnsureInitNative();
    return memory::Mem_AllocPool(kHeapAlign, size, kPoolSystem, 0);
}

}  // namespace sly4::heap

NATIVE(0055260C, sly4::heap::MemStack_Alloc)
