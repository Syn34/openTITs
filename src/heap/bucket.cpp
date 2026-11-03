// ApeMem::_BucketAlloc: small blocks come from one free list per size class.
#include "heap.h"

using namespace sly4;
using namespace sly4::heap;

// void* SmallAlloc_Impl(u32 size)  ("void *ApeMem::_BucketAlloc(unsigned int)")
// The first size class whose limit is >= size serves the block from its
// free list (made on first use, with the block size from g::config +0x190
// or the class's default). Bigger blocks go to the system pool like
// operator new.
DECOMP(00551CC0, SmallAlloc_Impl) {
    const u32 size = arg<u32>(ctx, 0);

    u32 bucket = 0, i = 0;
    for (; i < kBucketCount; ++i) {
        bucket = kBuckets + i * kBucketStride;
        if (size <= field<u32>(bucket, kBucketLimit)) break;
    }
    if (i == kBucketCount) {
        MemState_EnsureInit(ctx);
        return ret(ctx, call(ctx, fn::Mem_AllocPool, {kHeapAlign, size, kPoolSystem, 0}));
    }

    u32 list = field<u32>(bucket, kBucketList);
    u32 limit = field<u32>(bucket, kBucketLimit);
    if (list == 0) {
        const u16 configured = load<u16>(g::config + kConfigBucketCounts + i * 2);
        u32 count;
        if (configured != 0) {
            const s32 n = static_cast<s16>(configured);
            count = n > 0 ? static_cast<u32>(n) : 0;
        } else {
            count = field<u32>(bucket, kBucketCount4);
        }
        const u32 mem = static_cast<u32>(call(ctx, fn::Mem_New, {kFreeListSize}));
        u32 made = 0;
        if (mem != 0) {
            field<u32>(mem, 0) = kFreeListBaseVtbl;
            for (u32 off = 4; off <= 48; off += 4) field<u32>(mem, off) = 0;
            field<u16>(mem, 52) = 0;
            field<u16>(mem, 54) &= static_cast<u16>(~0x8000);
            field<u32>(mem, 56) = kFreeListEmptyName;
            call(ctx, hfn::FreeList_Init, {mem, field<u32>(bucket, kBucketLimit), count, 0, 1});
            made = mem;
        }
        field<u32>(bucket, kBucketList) = made;
        call(ctx, hfn::String_Printf,
             {bucket + kBucketName, kBucketNameFormat, kBucketFuncName, field<u32>(bucket, kBucketLimit)});
        list = field<u32>(bucket, kBucketList);
        limit = field<u32>(bucket, kBucketLimit);
        // No null check: a failed Mem_New writes to guest address 56, as in the original.
        field<u32>(list, 56) = bucket + kBucketName;
    }

    const u32 slack = limit - size;
    field<u32>(bucket, kBucketSlack) = field<u32>(bucket, kBucketSlack) + slack;
    store<u32>(kBucketSlackTotal, load<u32>(kBucketSlackTotal) + slack);
    ret(ctx, call_ptr(ctx, vfunc(list, kFreeListVfnAlloc), {list}));
}
