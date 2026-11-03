// The default heap's Alloc (vtable 0x00B24278 +0).
#include "heap.h"

using namespace sly4;
using namespace sly4::heap;

// void* DefaultHeap_AllocAligned(Heap* heap, u32 size, bool clear, u32 align)
// A system-pool block, cleared when asked (and size != 0).
DECOMP(00550F5C, DefaultHeap_AllocAligned) {
    const u32 size = arg<u32>(ctx, 1);
    const u8 clear = arg<u8>(ctx, 2);
    const u32 align = arg<u32>(ctx, 3);
    MemState_EnsureInit(ctx);
    const u32 p = static_cast<u32>(call(ctx, fn::Mem_AllocPool, {align, size, kPoolSystem, 0}));
    if (clear != 0 && size != 0) call(ctx, fn::memset, {p, 0, size});
    ret(ctx, p);
}
