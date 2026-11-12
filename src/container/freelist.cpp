// ApeTFreeList<T> block allocation.
#include "container.h"

namespace sly4::container {

// Allocates a block of items-per-block items, links it at the head of the
// list's blocks and pushes every item onto the free list.
void FreeList_AddBlock(FreeList* list, u32 flag) {
    const u32 stride = (list->elem_size + 15) & ~15u;
    const u32 n = list->items_per_block;
    const u32 bytes = stride * n + (n << 4);
    FreeBlock* block = vcall<FreeBlock*>(list->heap.get(), kAllocVfnAlloc, bytes + kBlockHeader + kItemHeader, 0u, 16u);

    block->bytes = bytes + kBlockHeader;
    block->flag = flag;
    block->newer = nullptr;
    block->older = list->block_head;
    FreeBlock* tail = list->block_tail;
    list->block_head = block;
    const u32 blocks = list->block_count;
    if (!tail) list->block_tail = block;
    if (FreeBlock* older = block->older) older->newer = block;
    list->block_count = blocks + 1;

    FreeItem* head = list->free_head;
    u32 item = guest(block) + kBlockHeader;
    for (u32 i = 0; i < list->items_per_block; ++i) {
        FreeItem* it = host<FreeItem>(item);
        it->prev = nullptr;
        it->next = head;
        FreeItem* free_tail = list->free_tail;
        list->free_head = it;
        head = it;
        const u32 free_count = list->free_count;
        if (!free_tail) list->free_tail = it;
        if (FreeItem* next = it->next) next->prev = it;
        list->free_count = free_count + 1;
        item += stride + kItemHeader;
    }
}

}  // namespace sly4::container

NATIVE(005518FC, sly4::container::FreeList_AddBlock)
