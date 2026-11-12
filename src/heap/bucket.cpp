// ApeMem::_BucketAlloc: small blocks come from one free list per size class.
#include "heap.h"

namespace sly4::heap {
using namespace opentit::native;
using container::FreeList;

// "void *ApeMem::_BucketAlloc(unsigned int)"
// The first size class whose limit is >= size serves the block from its
// free list (made on first use, with the block size from g::config +0x190
// or the class's default). Bigger blocks go to the system pool like
// operator new.
void* SmallAlloc_Impl(u32 size) {
    Bucket* buckets = host<Bucket>(kBuckets);
    Bucket* bucket = nullptr;
    u32 i = 0;
    for (; i < kBucketCount; ++i) {
        bucket = &buckets[i];
        if (size <= bucket->limit) break;
    }
    if (i == kBucketCount) {
        MemState_EnsureInitNative();
        return memory::Mem_AllocPool(kHeapAlign, size, kPoolSystem, 0);
    }

    FreeList* list = bucket->list;
    u32 limit = bucket->limit;
    if (!list) {
        const u16 configured = host<be_u16>(g::config + kConfigBucketCounts)[i];
        u32 count;
        if (configured != 0) {
            const s32 n = static_cast<s16>(configured);
            count = n > 0 ? static_cast<u32>(n) : 0;
        } else {
            count = bucket->count4;
        }
        FreeList* made = nullptr;
        if (FreeList* fl = static_cast<FreeList*>(memory::Mem_New(kFreeListSize))) {
            be_u32* w = reinterpret_cast<be_u32*>(fl);
            w[0] = kFreeListBaseVtbl;
            for (u32 off = 4; off <= 48; off += 4) w[off / 4] = 0;
            be_u16* h = reinterpret_cast<be_u16*>(fl);
            h[52 / 2] = 0;
            h[54 / 2] &= static_cast<u16>(~0x8000);
            w[56 / 4] = kFreeListEmptyName;
            call(hfn::FreeList_Init, fl, bucket->limit.get(), count, 0u, 1u);
            made = fl;
        }
        bucket->list = made;
        // String_Printf is variadic and needs a guest caller (see string_misc.cpp).
        call(hfn::String_Printf, bucket->name, kBucketNameFormat, kBucketFuncName, bucket->limit.get());
        list = bucket->list;
        limit = bucket->limit;
        // No null check: a failed Mem_New writes to guest address 56, as in the original.
        *host<be_u32>(guest(list) + 56) = guest(bucket->name);
    }

    const u32 slack = limit - size;
    bucket->slack += slack;
    global<be_u32>(kBucketSlackTotal) += slack;
    return vcall<void*>(list, kFreeListVfnAlloc);
}

}  // namespace sly4::heap

NATIVE(00551CC0, sly4::heap::SmallAlloc_Impl)
