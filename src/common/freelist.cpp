// ApeTFreeList<T>: taking and returning items, destroying a list, and the
// per-type virtual functions of the two lists Game_Load uses.
#include "common.h"

namespace sly4::common {
using namespace opentit::native;
using container::FreeBlock;
using container::FreeItem;

namespace {

// Unlink `item` from a doubly linked list with the given head and tail
// (items: +0 next, +4 prev).
template <typename T>
void List_Unlink(gp<T>& head, gp<T>& tail, T* item) {
    if (T* prev = item->prev) prev->next = item->next.get();
    else head = item->next.get();
    if (T* next = item->next) next->prev = item->prev.get();
    else tail = item->prev.get();
}

// FreeList_Init's default heap, inlined again here.
container::Allocator* DefaultHeap_Get() {
    gp<container::Allocator>& slot = global<gp<container::Allocator>>(kDefaultHeapPtr);
    if (slot) return slot;
    auto* heap = host<container::Allocator>(kDefaultHeap);
    u8& guard = global<u8>(kDefaultHeapGuard);
    if (guard == 0) {
        heap->vtable = kDefaultHeapVtbl;
        guard = 1;
    }
    heap->vtable = kDefaultHeapVtbl;
    slot = heap;
    return heap;
}

// Walks a list from its head, unlinking every item; the count is stored
// after each one, as in the original. An item that isn't the head (prev
// != 0) would be visited again forever; that can't happen walking from the head.
void List_Clear(gp<FreeItem>& head, gp<FreeItem>& tail, be_u32& count_field, FreeItem* first, u32 count) {
    for (FreeItem* it = first; it;) {
        FreeItem* prev = it->prev;
        FreeItem* next;
        if (!prev) {
            next = it->next;
            head = next;
        } else {
            prev->next = it->next.get();
            next = it->next;
        }
        if (!next) tail = it->prev.get();
        else next->prev = it->prev.get();
        count_field = --count;
        if (!prev) it = next;
    }
}

}  // namespace

// Takes the first free item (adding a block when there is none), clears it
// and puts it at the head of the used list. Returns the item's payload.
void* FreeList_Alloc(FreeList* list) {
    FreeItem* item = list->free_head;
    if (!item) {
        container::FreeList_AddBlock(list, 0);
        item = list->free_head;
    }
    List_Unlink(list->free_head, list->free_tail, item);
    list->free_count -= 1;
    const u32 stride = (list->elem_size + 15) & ~15u;
    call(fn::memset, item, 0u, stride + kItemHeader);
    FreeItem* old = list->used_head;
    const u32 n = list->used_count;
    item->next = old;
    item->prev = nullptr;
    FreeItem* tail = list->used_tail;
    list->used_head = item;
    const u32 peak = list->used_peak;
    if (!tail) list->used_tail = item;
    if (old) old->prev = item;
    list->used_count = n + 1;
    list->used_peak = peak > n + 1 ? peak : n + 1;
    return host<void>(guest(item) + kItemHeader);
}

// Moves the item from the used list to the head of the free list.
void FreeList_Free(FreeList* list, void* p) {
    FreeItem* item = host<FreeItem>(guest(p) - kItemHeader);
    const u32 used = list->used_count;
    FreeItem* free_head = list->free_head;
    List_Unlink(list->used_head, list->used_tail, item);
    item->prev = nullptr;
    item->next = free_head;
    list->used_count = used - 1;
    FreeItem* free_tail = list->free_tail;
    list->free_head = item;
    const u32 n = list->free_count;
    if (!free_tail) list->free_tail = item;
    if (FreeItem* next = item->next) next->prev = item;
    list->free_count = n + 1;
}

// Empties the used and free lists, gives every block back to the heap and
// removes the list from the registry (swapping the last entry into its
// place). Does nothing when the list has no heap.
void FreeList_Destroy(FreeList* list) {
    if (!list->heap) return;
    const u32 used = list->used_count;
    FreeItem* free_head = list->free_head;
    const u32 free_n = list->free_count;
    List_Clear(list->used_head, list->used_tail, list->used_count, list->used_head, used);
    List_Clear(list->free_head, list->free_tail, list->free_count, free_head, free_n);
    // Blocks, oldest first, back to the heap.
    for (FreeBlock* b; (b = list->block_tail) != nullptr;) {
        // (+0 newer and +4 older play next and prev here: oldest = +28 "head", newest = +32 "tail")
        if (FreeBlock* prev = b->older) prev->newer = b->newer.get();
        else list->block_tail = b->newer.get();
        if (FreeBlock* next = b->newer) next->older = b->older.get();
        else list->block_head = b->older.get();
        list->block_count -= 1;
        vcall(list->heap.get(), kHeapVfnFree, b, 0u);
    }
    list->heap = nullptr;
    // The registry (made here if no list has been made yet).
    auto* reg = host<container::PtrVector>(kFreeLists);
    u8& guard = global<u8>(kFreeListsGuard);
    if (guard == 0) {
        container::Allocator* heap = DefaultHeap_Get();
        reg->data = nullptr;
        reg->capacity = 0;
        reg->count = 0;
        reg->high_water = 0;
        reg->allocator = nullptr;
        reg->elem_size = 4;
        reg->bits = 0;  // its bit-fields, cleared one by one in the original
        container::PtrVector_Init(reg, 64, heap, 0);  // (the original also passes 4 in r7, unused)
        guard = 1;
    }
    const u32 count = reg->count;
    if (count == 0) return;
    for (u32 i = count - 1;; --i) {
        be_u32* data = reinterpret_cast<be_u32*>(reg->data.get());
        if (gref<be_u32>(guest(data) + i * 4) == guest(list)) {
            const u32 last = count - 1;
            reg->count = static_cast<u16>(last);
            if (i != (last & 0xFFFF)) gref<be_u32>(guest(data) + i * 4) = gref<be_u32>(guest(data) + ((last << 2) & 0x3FFFC)).get();
            return;
        }
        if (i == 0) return;
    }
}

// ---------------------------------------------------------------- per-type virtual functions
// ApeTFreeList<AglRenderTarget> (vtable 0x00B23DC0) and
// ApeTFreeList<ApeInputDevice> (vtable 0x00B243F8):
//   +0 destructor, +4 deleting destructor, +8 (empty), +12 Alloc, +16 Free, +20 Init(count, heap).
void RtFreeList_Destruct(FreeList* list) {
    list->vtable = kRtFreeListVtbl;
    FreeList_Destroy(list);
    list->vtable = kFreeListBaseVtbl;
    FreeList_Destroy(list);
}

void RtFreeList_Delete(FreeList* list) {
    if (!list) return;
    list->vtable = kRtFreeListVtbl;
    FreeList_Destroy(list);
    list->vtable = kFreeListBaseVtbl;
    FreeList_Destroy(list);
    Mem_FreeFrom(list, 0);
}

void RtFreeList_Fn8(FreeList*) {}

void* RtFreeList_Alloc(FreeList* list) { return FreeList_Alloc(list); }

void RtFreeList_Free(FreeList* list, void* p) { FreeList_Free(list, p); }

void RtFreeList_Init(FreeList* list, u32 count, void* heap) { call(cmfn::FreeList_Init, list, kRtSize, count, heap, 0u); }

void InputFreeList_Destruct(FreeList* list) {
    list->vtable = kInputFreeListVtbl;
    FreeList_Destroy(list);
    list->vtable = kFreeListBaseVtbl;
    FreeList_Destroy(list);
}

void InputFreeList_Delete(FreeList* list) {
    if (!list) return;
    list->vtable = kInputFreeListVtbl;
    FreeList_Destroy(list);
    list->vtable = kFreeListBaseVtbl;
    FreeList_Destroy(list);
    memory::Mem_FreePool(list, 0, 0);
}

void InputFreeList_Fn8(FreeList*) {}

void* InputFreeList_Alloc(FreeList* list) { return FreeList_Alloc(list); }

void InputFreeList_Free(FreeList* list, void* p) { FreeList_Free(list, p); }

void InputFreeList_Init(FreeList* list, u32 count, void* heap) {
    call(cmfn::FreeList_Init, list, kInputDeviceSize, count, heap, 0u);
}

}  // namespace sly4::common

NATIVE(00556C04, sly4::common::FreeList_Alloc)
NATIVE(00556D18, sly4::common::FreeList_Free)
NATIVE(005568FC, sly4::common::FreeList_Destroy)
NATIVE(007A2564, sly4::common::RtFreeList_Destruct)
NATIVE(007A25B8, sly4::common::RtFreeList_Delete)
NATIVE(007A2628, sly4::common::RtFreeList_Fn8)
NATIVE(0079EA84, sly4::common::RtFreeList_Alloc)
NATIVE(0079EAA8, sly4::common::RtFreeList_Free)
NATIVE(007A1018, sly4::common::RtFreeList_Init)
NATIVE(007A4E80, sly4::common::InputFreeList_Destruct)
NATIVE(007A4ECC, sly4::common::InputFreeList_Delete)
NATIVE(007A4F38, sly4::common::InputFreeList_Fn8)
NATIVE(007A4A90, sly4::common::InputFreeList_Alloc)
NATIVE(007A4AB0, sly4::common::InputFreeList_Free)
NATIVE(007A4D1C, sly4::common::InputFreeList_Init)
