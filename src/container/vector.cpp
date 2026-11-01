// PtrVector storage.
#include "container.h"

using namespace sly4;
using namespace sly4::container;

namespace {

u32 DefaultAllocator_Get() {
    u32 a = load<u32>(kDefaultAllocatorPtr);
    if (a != 0) return a;
    if (ref<u8>(kDefaultAllocatorGuard) == 0) {
        store<u32>(kDefaultAllocator, kDefaultAllocatorVtbl);
        ref<u8>(kDefaultAllocatorGuard) = 1;
    }
    store<u32>(kDefaultAllocator, kDefaultAllocatorVtbl);  // constructed again in place, as in the original
    store<u32>(kDefaultAllocatorPtr, kDefaultAllocator);
    return kDefaultAllocator;
}

u32 Vec_Alloc(PPUContext& ctx, u32 vec, u32 size) {
    const u32 a = field<u32>(vec, kVecAllocator);
    return static_cast<u32>(call_ptr(ctx, vfunc(a, kAllocVfnAlloc), {a, size, 1, kAllocAlign}));
}

void Vec_Free(PPUContext& ctx, u32 vec, u32 p, u32 size) {
    const u32 a = field<u32>(vec, kVecAllocator);
    call_ptr(ctx, vfunc(a, kAllocVfnFree), {a, p, size});
}

}  // namespace

// void PtrVector_Init(PtrVector* vec, u32 capacity, Allocator* alloc, bool flag)
// Sets the capacity (elements of the size already in +6), the allocator
// (the default one when 0) and allocates the storage when capacity != 0.
DECOMP(00551598, PtrVector_Init) {
    const u32 vec = arg<u32>(ctx, 0);
    const u32 capacity = arg<u32>(ctx, 1);
    u32 alloc = arg<u32>(ctx, 2);
    const u32 flag = arg<u32>(ctx, 3);

    const u32 bytes = static_cast<u32>(field<u16>(vec, kVecElemSize)) * (capacity & 0xFFFF);
    field<u16>(vec, kVecCapacity) = static_cast<u16>(capacity);
    field<u32>(vec, kVecBits) = (field<u32>(vec, kVecBits) & 7) | (bytes << 3);
    if (alloc == 0) alloc = DefaultAllocator_Get();
    field<u32>(vec, kVecAllocator) = alloc;

    u32 data = 0;
    if (capacity != 0)
        data = static_cast<u32>(call_ptr(ctx, vfunc(alloc, kAllocVfnAlloc),
                                         {alloc, field<u32>(vec, kVecBits) >> 3, 1, kAllocAlign}));
    const u32 bits = field<u32>(vec, kVecBits);
    field<u32>(vec, kVecData) = data;
    field<u16>(vec, kVecCount) = 0;
    field<u32>(vec, kVecBits) = (bits & ~kVecFlag) | ((flag << 2) & kVecFlag);
}

// void PtrVector_Reserve(PtrVector* vec, u32 capacity)
// Reallocates the storage for `capacity` elements (16 when 0 and there is
// none yet). Growing takes at least 16 more elements' worth; shrinking
// (when neither capacity nor count reach the current capacity) copies only
// the new size.
DECOMP(00551718, PtrVector_Reserve) {
    const u32 vec = arg<u32>(ctx, 0);
    const u32 capacity = arg<u32>(ctx, 1);

    if (field<u32>(vec, kVecData) == 0) {
        call(ctx, cfn::PtrVector_Init, {vec, capacity != 0 ? capacity : 16u, 0, 0});
        return;
    }
    const u32 elem = field<u16>(vec, kVecElemSize);
    const u32 new_bytes = capacity * elem;
    const u32 old_bytes = field<u32>(vec, kVecBits) >> 3;
    const u32 count = field<u16>(vec, kVecCount);
    const u32 wanted = capacity > count ? capacity : count;

    u32 size, mem;
    if (wanted < field<u16>(vec, kVecCapacity)) {
        size = new_bytes;
        mem = Vec_Alloc(ctx, vec, size);
        if (field<u32>(vec, kVecData) != 0) {
            call(ctx, cfn::memmove, {mem, field<u32>(vec, kVecData), new_bytes});
            Vec_Free(ctx, vec, field<u32>(vec, kVecData), old_bytes);
        }
    } else {
        const u32 grown = old_bytes + ((elem << 4) & 0xFFFF0);
        size = grown < new_bytes ? new_bytes : grown;
        mem = Vec_Alloc(ctx, vec, size);
        if (field<u32>(vec, kVecData) != 0) {
            call(ctx, cfn::memmove, {mem, field<u32>(vec, kVecData), old_bytes});
            Vec_Free(ctx, vec, field<u32>(vec, kVecData), old_bytes);
        }
    }
    const u32 cap = static_cast<u32>(rt::divwu(size, field<u16>(vec, kVecElemSize)));
    const u32 bits = field<u32>(vec, kVecBits);
    field<u32>(vec, kVecData) = mem;
    field<u16>(vec, kVecCapacity) = static_cast<u16>(cap);
    field<u32>(vec, kVecBits) = (bits & 7) | (size << 3);
}
