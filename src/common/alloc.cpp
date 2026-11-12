// Heap and allocator virtual functions, the small-block allocator front
// end and the C library's malloc family.
#include "common.h"

#include "../heap/heap.h"

namespace sly4::common {
using namespace opentit::native;

namespace {

struct SizeClass {
    be_u32 limit;                  // +0 largest size served
    pad<4> _4;
    gp<container::FreeList> list;  // +8: vtable +16 = Free(p)
    be_u32 slack;                  // +12 adjusted by limit - size on every free (name?)
    pad<64> _16;
};
static_assert(sizeof(SizeClass) == kSizeClassStride);

// Undo a small allocation of `size` bytes: hand it back to the first size
// class that fits, or to Mem_FreePool when none does. (Inlined twice.)
void SmallFree_Impl(void* p, u32 size) {
    SizeClass* cls = host<SizeClass>(kSizeClasses);
    for (u32 left = kSizeClassCount - 1;; --left, ++cls) {
        const u32 limit = cls->limit;
        if (size <= limit) {
            const u32 slack = limit - size;
            container::FreeList* list = cls->list;
            cls->slack -= slack;
            global<be_u32>(kSmallSlackTotal) -= slack;
            vcall(list, kListVfnFree, p);
            return;
        }
        if (left == 0) break;
    }
    memory::Mem_FreePool(p, 0, 0);
}

void LinearHeap_Release(LinearHeap* h) {
    h->vtable = kLinearHeapVtbl;
    if (h->base != 0) memory::Mem_FreePool(host<void>(h->base), h->pool, 0);
    h->cursor = 0;
    h->end = 0;
    h->base = 0;
}

}  // namespace

void LinearHeap_Destruct(LinearHeap* heap) { LinearHeap_Release(heap); }

// The deleting destructor.
void LinearHeap_Delete(LinearHeap* heap) {
    if (!heap) return;
    LinearHeap_Release(heap);
    memory::Mem_FreePool(heap, 0, 0);
}

// A pointer outside the heap's block goes to Mem_FreePool. One inside it
// only moves the cursor back, when p - 16 lies below the cursor.
void LinearHeap_Free(LinearHeap* h, void* ptr) {
    const u32 p = guest(ptr);
    if (p < h->base || p > h->end) {
        memory::Mem_FreePool(ptr, 0, 0);
        return;
    }
    if (p - 16 < h->cursor) h->cursor = (p - 1) & ~15u;
}

// void* Mem_Stub0(): returns 0
void* Mem_Stub0() { return nullptr; }

// The default heap's +4: always 0 (0x0056C89C is a stub).
void* DefaultHeap_Alloc(void* /*heap*/, u32 /*a*/, u32 /*b*/) { return Mem_Stub0(); }

void DefaultHeap_Free(void* /*heap*/, void* p) { memory::Mem_FreePool(p, 0, 0); }

void Mem_FreeFrom(void* p, s32 pool) { memory::Mem_FreePool(p, pool, 0); }

// The original forwards its registers unchanged to the size-class allocator,
// which reads only the size.
void* SmallAlloc(u32 size) { return heap::SmallAlloc_Impl(size); }

void SmallFree(void* p, u32 size) { SmallFree_Impl(p, size); }

// The allocator PtrVector uses by default (vtable 0x00B24368 +0).
void* DefaultAllocator_Alloc(void* /*a*/, u32 size, u8 clear, u32 /*align*/) {
    void* p = heap::SmallAlloc_Impl(size);
    if (clear != 0 && size != 0) call(fn::memset, p, 0u, size);
    return p;
}

// +4: returns 0
u32 DefaultAllocator_Fn4(void* /*a*/) { return 0; }

// +8
void DefaultAllocator_Free(void* /*a*/, void* p, u32 size) { SmallFree_Impl(p, size); }

void* memalign(u32 align, u32 size) { return call<void*>(cmfn::memalign_r, kReent, align, size); }

void free(void* p) { call(cmfn::free_r, kReent, p); }

void* malloc(u32 size) { return call<void*>(cmfn::malloc_r, kReent, size); }

// vsprintf's output function (OPD 0x00F486A8): appends n bytes, returns the new end.
u32 Sprintf_Output(char* dst, const char* src, u32 n) { return call<u32>(cmfn::memmove, dst, src, n) + n; }

}  // namespace sly4::common

NATIVE(00553140, sly4::common::LinearHeap_Destruct)
NATIVE(005531A4, sly4::common::LinearHeap_Delete)
NATIVE(0055322C, sly4::common::LinearHeap_Free)
NATIVE(005514E0, sly4::common::DefaultHeap_Alloc)
NATIVE(00551510, sly4::common::DefaultHeap_Free)
NATIVE(0056C89C, sly4::common::Mem_Stub0)
NATIVE(0054EE6C, sly4::common::Mem_FreeFrom)
NATIVE(0055238C, sly4::common::SmallAlloc)
NATIVE(00552420, sly4::common::SmallFree)
NATIVE(005523AC, sly4::common::DefaultAllocator_Alloc)
NATIVE(00552418, sly4::common::DefaultAllocator_Fn4)
NATIVE(005524D8, sly4::common::DefaultAllocator_Free)
NATIVE(0072A88C, sly4::common::memalign)
NATIVE(0072A8FC, sly4::common::free)
NATIVE(0072A928, sly4::common::malloc)
NATIVE(007245E4, sly4::common::Sprintf_Output)
