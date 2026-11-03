// ApeTFreeList<T>: taking and returning items, destroying a list, and the
// per-type virtual functions of the two lists Game_Load uses.
#include "common.h"

using namespace sly4;
using namespace sly4::common;

namespace {

// Unlink `item` from the doubly linked list whose head and tail are at
// list + head / list + tail (items: +0 next, +4 prev).
void List_Unlink(u32 list, u32 item, u32 head, u32 tail) {
    const u32 prev = field<u32>(item, 4);
    if (prev == 0)
        field<u32>(list, head) = field<u32>(item, 0);
    else
        field<u32>(prev, 0) = field<u32>(item, 0);
    const u32 next = field<u32>(item, 0);
    if (next == 0)
        field<u32>(list, tail) = field<u32>(item, 4);
    else
        field<u32>(next, 4) = field<u32>(item, 4);
}

// FreeList_Init's default heap, inlined again here.
u32 DefaultHeap_Get() {
    u32 heap = load<u32>(kDefaultHeapPtr);
    if (heap != 0) return heap;
    if (ref<u8>(kDefaultHeapGuard) == 0) {
        store<u32>(kDefaultHeap, kDefaultHeapVtbl);
        ref<u8>(kDefaultHeapGuard) = 1;
    }
    store<u32>(kDefaultHeap, kDefaultHeapVtbl);
    store<u32>(kDefaultHeapPtr, kDefaultHeap);
    return kDefaultHeap;
}

}  // namespace

// void* FreeList_Alloc(FreeList* list)
// Takes the first free item (adding a block when there is none), clears it
// and puts it at the head of the used list. Returns the item's payload.
DECOMP(00556C04, FreeList_Alloc) {
    const u32 list = arg<u32>(ctx, 0);
    u32 item = field<u32>(list, kFlFreeHead);
    if (item == 0) {
        call(ctx, cmfn::FreeList_AddBlock, {list, 0});
        item = field<u32>(list, kFlFreeHead);
    }
    List_Unlink(list, item, kFlFreeHead, kFlFreeTail);
    field<u32>(list, kFlFreeCount) = field<u32>(list, kFlFreeCount) - 1;
    const u32 stride = (field<u32>(list, kFlElemSize) + 15) & ~15u;
    call(ctx, fn::memset, {item, 0, stride + kItemHeader});

    const u32 old = field<u32>(list, kFlUsedHead);
    const u32 n = field<u32>(list, kFlUsedCount);
    field<u32>(item, 0) = old;
    field<u32>(item, 4) = 0;
    const u32 tail = field<u32>(list, kFlUsedTail);
    field<u32>(list, kFlUsedHead) = item;
    const u32 peak = field<u32>(list, kFlUsedPeak);
    if (tail == 0) field<u32>(list, kFlUsedTail) = item;
    if (old != 0) field<u32>(old, 4) = item;
    field<u32>(list, kFlUsedCount) = n + 1;
    field<u32>(list, kFlUsedPeak) = peak > n + 1 ? peak : n + 1;
    ret(ctx, item + kItemHeader);
}

// void FreeList_Free(FreeList* list, void* p)
// Moves the item from the used list to the head of the free list.
DECOMP(00556D18, FreeList_Free) {
    const u32 list = arg<u32>(ctx, 0);
    const u32 item = arg<u32>(ctx, 1) - kItemHeader;
    const u32 used = field<u32>(list, kFlUsedCount);
    const u32 free_head = field<u32>(list, kFlFreeHead);
    List_Unlink(list, item, kFlUsedHead, kFlUsedTail);
    field<u32>(item, 4) = 0;
    field<u32>(item, 0) = free_head;
    field<u32>(list, kFlUsedCount) = used - 1;
    const u32 free_tail = field<u32>(list, kFlFreeTail);
    field<u32>(list, kFlFreeHead) = item;
    const u32 n = field<u32>(list, kFlFreeCount);
    if (free_tail == 0) field<u32>(list, kFlFreeTail) = item;
    const u32 next = field<u32>(item, 0);
    if (next != 0) field<u32>(next, 4) = item;
    field<u32>(list, kFlFreeCount) = n + 1;
}

// void FreeList_Destroy(FreeList* list)
// Empties the used and free lists, gives every block back to the heap and
// removes the list from the registry (swapping the last entry into its
// place). Does nothing when the list has no heap.
DECOMP(005568FC, FreeList_Destroy) {
    const u32 list = arg<u32>(ctx, 0);
    if (field<u32>(list, kFlHeap) == 0) return;

    // The used and free lists. Counts are kept in registers and stored after
    // each item, as in the original. An item that isn't the head (prev != 0)
    // would be visited again forever; that can't happen when walking from the head.
    u32 used = field<u32>(list, kFlUsedCount);
    u32 free_head = field<u32>(list, kFlFreeHead);
    u32 free_n = field<u32>(list, kFlFreeCount);
    for (u32 it = field<u32>(list, kFlUsedHead); it != 0;) {
        const u32 prev = field<u32>(it, 4);
        u32 next;
        if (prev == 0) {
            next = field<u32>(it, 0);
            field<u32>(list, kFlUsedHead) = next;
        } else {
            field<u32>(prev, 0) = field<u32>(it, 0);
            next = field<u32>(it, 0);
        }
        if (next == 0)
            field<u32>(list, kFlUsedTail) = field<u32>(it, 4);
        else
            field<u32>(next, 4) = field<u32>(it, 4);
        field<u32>(list, kFlUsedCount) = --used;
        if (prev == 0) it = next;
    }
    for (u32 it = free_head; it != 0;) {
        const u32 prev = field<u32>(it, 4);
        u32 next;
        if (prev == 0) {
            next = field<u32>(it, 0);
            field<u32>(list, kFlFreeHead) = next;
        } else {
            field<u32>(prev, 0) = field<u32>(it, 0);
            next = field<u32>(it, 0);
        }
        if (next == 0)
            field<u32>(list, kFlFreeTail) = field<u32>(it, 4);
        else
            field<u32>(next, 4) = field<u32>(it, 4);
        field<u32>(list, kFlFreeCount) = --free_n;
        if (prev == 0) it = next;
    }
    // Blocks, oldest first, back to the heap.
    for (u32 b; (b = field<u32>(list, kFlBlockOldest)) != 0;) {
        List_Unlink(list, b, kFlBlockOldest, kFlBlockNewest);
        field<u32>(list, kFlBlockCount) = field<u32>(list, kFlBlockCount) - 1;
        const u32 heap = field<u32>(list, kFlHeap);
        call_ptr(ctx, vfunc(heap, kHeapVfnFree), {heap, b, 0});
    }
    field<u32>(list, kFlHeap) = 0;

    // The registry (made here if no list has been made yet).
    const u32 reg = kFreeLists;
    if (ref<u8>(kFreeListsGuard) == 0) {
        const u32 heap = DefaultHeap_Get();
        field<u32>(reg, 0) = 0;
        field<u16>(reg, 4) = 0;
        field<u16>(reg, 8) = 0;
        field<u16>(reg, 10) = 0;
        field<u32>(reg, 16) = 0;
        field<u16>(reg, 6) = 4;
        field<u32>(reg, 12) = 0;  // its bit-fields, cleared one by one in the original
        call(ctx, cmfn::PtrVector_Init, {reg, 64, heap, 0, 4});
        ref<u8>(kFreeListsGuard) = 1;
    }
    const u32 count = field<u16>(reg, 8);
    if (count == 0) return;
    for (u32 i = count - 1;; --i) {
        const u32 data = field<u32>(reg, 0);
        if (load<u32>(data + i * 4) == list) {
            const u32 last = count - 1;
            field<u16>(reg, 8) = static_cast<u16>(last);
            if (i != (last & 0xFFFF)) store<u32>(data + i * 4, load<u32>(data + ((last << 2) & 0x3FFFC)));
            return;
        }
        if (i == 0) return;
    }
}

// ---------------------------------------------------------------- per-type virtual functions
// ApeTFreeList<AglRenderTarget> (vtable 0x00B23DC0) and
// ApeTFreeList<ApeInputDevice> (vtable 0x00B243F8):
//   +0 destructor, +4 deleting destructor, +8 (empty), +12 Alloc, +16 Free, +20 Init(count, heap).

// void RtFreeList_Destruct(FreeList*)
DECOMP(007A2564, RtFreeList_Destruct) {
    const u32 list = arg<u32>(ctx, 0);
    field<u32>(list, 0) = kRtFreeListVtbl;
    call(ctx, cmfn::FreeList_Destroy, {list});
    field<u32>(list, 0) = kFreeListBaseVtbl;
    call(ctx, cmfn::FreeList_Destroy, {list});
}

// void RtFreeList_Delete(FreeList*)
DECOMP(007A25B8, RtFreeList_Delete) {
    const u32 list = arg<u32>(ctx, 0);
    if (list == 0) return ret(ctx, 0u);
    field<u32>(list, 0) = kRtFreeListVtbl;
    call(ctx, cmfn::FreeList_Destroy, {list});
    field<u32>(list, 0) = kFreeListBaseVtbl;
    call(ctx, cmfn::FreeList_Destroy, {list});
    call(ctx, cmfn::Mem_FreeFrom, {list, 0});
}

// void RtFreeList_Fn8(FreeList*): empty
DECOMP(007A2628, RtFreeList_Fn8) {}

// AglRenderTarget* RtFreeList_Alloc(FreeList*)
DECOMP(0079EA84, RtFreeList_Alloc) { ret(ctx, call(ctx, cmfn::FreeList_Alloc, {ctx.gpr[3]})); }

// void RtFreeList_Free(FreeList*, AglRenderTarget*)
DECOMP(0079EAA8, RtFreeList_Free) { ret(ctx, call(ctx, cmfn::FreeList_Free, {ctx.gpr[3], ctx.gpr[4]})); }

// void RtFreeList_Init(FreeList*, u32 count, Heap* heap)
DECOMP(007A1018, RtFreeList_Init) {
    ret(ctx, call(ctx, cmfn::FreeList_Init, {ctx.gpr[3], kRtSize, ctx.gpr[4], ctx.gpr[5], 0}));
}

// void InputFreeList_Destruct(FreeList*)
DECOMP(007A4E80, InputFreeList_Destruct) {
    const u32 list = arg<u32>(ctx, 0);
    field<u32>(list, 0) = kInputFreeListVtbl;
    call(ctx, cmfn::FreeList_Destroy, {list});
    field<u32>(list, 0) = kFreeListBaseVtbl;
    call(ctx, cmfn::FreeList_Destroy, {list});
}

// void InputFreeList_Delete(FreeList*)
DECOMP(007A4ECC, InputFreeList_Delete) {
    const u32 list = arg<u32>(ctx, 0);
    if (list == 0) return ret(ctx, 0u);
    field<u32>(list, 0) = kInputFreeListVtbl;
    call(ctx, cmfn::FreeList_Destroy, {list});
    field<u32>(list, 0) = kFreeListBaseVtbl;
    call(ctx, cmfn::FreeList_Destroy, {list});
    call(ctx, fn::Mem_FreePool, {list, 0, 0});
}

// void InputFreeList_Fn8(FreeList*): empty
DECOMP(007A4F38, InputFreeList_Fn8) {}

// ApeInputDevice* InputFreeList_Alloc(FreeList*)
DECOMP(007A4A90, InputFreeList_Alloc) { ret(ctx, call(ctx, cmfn::FreeList_Alloc, {ctx.gpr[3]})); }

// void InputFreeList_Free(FreeList*, ApeInputDevice*)
DECOMP(007A4AB0, InputFreeList_Free) { ret(ctx, call(ctx, cmfn::FreeList_Free, {ctx.gpr[3], ctx.gpr[4]})); }

// void InputFreeList_Init(FreeList*, u32 count, Heap* heap)
DECOMP(007A4D1C, InputFreeList_Init) {
    ret(ctx, call(ctx, cmfn::FreeList_Init, {ctx.gpr[3], kInputDeviceSize, ctx.gpr[4], ctx.gpr[5], 0}));
}
