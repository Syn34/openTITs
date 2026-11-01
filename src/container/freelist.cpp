// ApeTFreeList<T> block allocation.
#include "container.h"

using namespace sly4;
using namespace sly4::container;

// void FreeList_AddBlock(FreeList* list, u32 flag)
// Allocates a block of items-per-block items (16-byte header plus the
// element size rounded up to 16 each), links it at the head of the list's
// blocks and pushes every item onto the free list.
DECOMP(005518FC, FreeList_AddBlock) {
    const u32 list = arg<u32>(ctx, 0);
    const u32 flag = arg<u32>(ctx, 1);

    const u32 stride = (field<u32>(list, kFlElemSize) + 15) & ~15u;
    const u32 n = field<u16>(list, kFlItemsPerBlock);
    const u32 bytes = stride * n + (n << 4);
    const u32 heap = field<u32>(list, kFlHeap);
    const u32 block = static_cast<u32>(
        call_ptr(ctx, vfunc(heap, 0), {heap, bytes + kBlockHeader + kItemHeader, 0, 16}));

    field<u32>(block, 8) = bytes + kBlockHeader;
    field<u32>(block, 12) = flag;
    field<u32>(block, 0) = 0;
    field<u32>(block, 4) = field<u32>(list, kFlBlockHead);
    const u32 tail = field<u32>(list, kFlBlockTail);
    field<u32>(list, kFlBlockHead) = block;
    const u32 blocks = field<u32>(list, kFlBlockCount);
    if (tail == 0) field<u32>(list, kFlBlockTail) = block;
    const u32 older = field<u32>(block, 4);
    if (older != 0) field<u32>(older, 0) = block;
    field<u32>(list, kFlBlockCount) = blocks + 1;

    u32 head = field<u32>(list, kFlFreeHead);
    u32 item = block + kBlockHeader;
    for (u32 i = 0; i < field<u16>(list, kFlItemsPerBlock); ++i) {
        field<u32>(item, 4) = 0;
        field<u32>(item, 0) = head;
        const u32 free_tail = field<u32>(list, kFlFreeTail);
        field<u32>(list, kFlFreeHead) = item;
        head = item;
        const u32 free_count = field<u32>(list, kFlFreeCount);
        if (free_tail == 0) field<u32>(list, kFlFreeTail) = item;
        const u32 next = field<u32>(item, 0);
        if (next != 0) field<u32>(next, 4) = item;
        field<u32>(list, kFlFreeCount) = free_count + 1;
        item += stride + kItemHeader;
    }
}
