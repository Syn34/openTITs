// The default heap's Alloc (vtable 0x00B24278 +0).
#include "heap.h"

namespace sly4::heap {
using namespace opentit::native;

// void* DefaultHeap_AllocAligned(Heap* heap, u32 size, bool clear, u32 align)
// A system-pool block, cleared when asked (and size != 0).
void* DefaultHeap_AllocAligned(void* /*heap*/, u32 size, u8 clear, u32 align) {
    MemState_EnsureInitNative();
    void* p = memory::Mem_AllocPool(align, size, kPoolSystem, 0);
    if (clear != 0 && size != 0) call(fn::memset, p, 0, size);
    return p;
}

}  // namespace sly4::heap

NATIVE(00550F5C, sly4::heap::DefaultHeap_AllocAligned)
