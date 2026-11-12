// PtrVector storage.
#include "container.h"

namespace sly4::container {

Allocator* DefaultAllocator_Get() {
    gp<Allocator>& slot = global<gp<Allocator>>(kDefaultAllocatorPtr);
    if (slot) return slot;
    Allocator* a = host<Allocator>(kDefaultAllocator);
    u8& guard = global<u8>(kDefaultAllocatorGuard);
    if (guard == 0) {
        a->vtable = kDefaultAllocatorVtbl;
        guard = 1;
    }
    a->vtable = kDefaultAllocatorVtbl;  // constructed again in place, as in the original
    slot = a;
    return a;
}

namespace {

u8* Vec_Alloc(PtrVector* vec, u32 size) {
    return vcall<u8*>(vec->allocator.get(), kAllocVfnAlloc, size, 1u, kAllocAlign);
}

void Vec_Free(PtrVector* vec, u8* p, u32 size) { vcall(vec->allocator.get(), kAllocVfnFree, p, size); }

}  // namespace

// Sets the capacity (elements of the size already in +6), the allocator
// (the default one when 0) and allocates the storage when capacity != 0.
void PtrVector_Init(PtrVector* vec, u32 capacity, Allocator* alloc, u32 flag) {
    const u32 bytes = static_cast<u32>(vec->elem_size) * (capacity & 0xFFFF);
    vec->capacity = static_cast<u16>(capacity);
    vec->bits = (vec->bits & 7) | (bytes << 3);
    if (!alloc) alloc = DefaultAllocator_Get();
    vec->allocator = alloc;

    u8* data = nullptr;
    if (capacity != 0) data = vcall<u8*>(alloc, kAllocVfnAlloc, vec->bits >> 3, 1u, kAllocAlign);
    const u32 bits = vec->bits;
    vec->data = data;
    vec->count = 0;
    vec->bits = (bits & ~kVecFlag) | ((flag << 2) & kVecFlag);
}

// Reallocates the storage for `capacity` elements (16 when 0 and there is
// none yet). Growing takes at least 16 more elements' worth; shrinking
// (when neither capacity nor count reach the current capacity) copies only
// the new size.
void PtrVector_Reserve(PtrVector* vec, u32 capacity) {
    if (!vec->data) {
        PtrVector_Init(vec, capacity != 0 ? capacity : 16u, nullptr, 0);
        return;
    }
    const u32 elem = vec->elem_size;
    const u32 new_bytes = capacity * elem;
    const u32 old_bytes = vec->bits >> 3;
    const u32 count = vec->count;
    const u32 wanted = capacity > count ? capacity : count;

    u32 size;
    u8* mem;
    if (wanted < vec->capacity) {
        size = new_bytes;
        mem = Vec_Alloc(vec, size);
        if (vec->data) {
            call(cfn::memmove, mem, vec->data.get(), new_bytes);
            Vec_Free(vec, vec->data, old_bytes);
        }
    } else {
        const u32 grown = old_bytes + ((elem << 4) & 0xFFFF0);
        size = grown < new_bytes ? new_bytes : grown;
        mem = Vec_Alloc(vec, size);
        if (vec->data) {
            call(cfn::memmove, mem, vec->data.get(), old_bytes);
            Vec_Free(vec, vec->data, old_bytes);
        }
    }
    const u32 cap = static_cast<u32>(rt::divwu(size, vec->elem_size));
    const u32 bits = vec->bits;
    vec->data = mem;
    vec->capacity = static_cast<u16>(cap);
    vec->bits = (bits & 7) | (size << 3);
}

}  // namespace sly4::container

NATIVE(00551598, sly4::container::PtrVector_Init)
NATIVE(00551718, sly4::container::PtrVector_Reserve)
