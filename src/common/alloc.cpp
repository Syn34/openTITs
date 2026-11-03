// Heap and allocator virtual functions, the small-block allocator front
// end and the C library's malloc family.
#include "common.h"

using namespace sly4;
using namespace sly4::common;

namespace {

constexpr u32 kDefaultHeapAlloc = 0x0056C89C;  // () -> 0

// Undo a small allocation of `size` bytes: hand it back to the first size
// class that fits, or to Mem_FreePool when none does. (Inlined twice.)
void SmallFree_Impl(PPUContext& ctx, u32 p, u32 size) {
    u32 cls = kSizeClasses;
    for (u32 left = kSizeClassCount - 1;; --left) {
        const u32 limit = field<u32>(cls, kClassLimit);
        if (size <= limit) {
            const u32 slack = limit - size;
            const u32 list = field<u32>(cls, kClassList);
            field<u32>(cls, kClassSlack) = field<u32>(cls, kClassSlack) - slack;
            store<u32>(kSmallSlackTotal, load<u32>(kSmallSlackTotal) - slack);
            call_ptr(ctx, vfunc(list, kListVfnFree), {list, p});
            return;
        }
        cls += kSizeClassStride;
        if (left == 0) break;
    }
    call(ctx, fn::Mem_FreePool, {p, 0, 0});
}

void LinearHeap_Release(PPUContext& ctx, u32 heap) {
    auto& h = ref<LinearHeap>(heap);
    h.vtable = kLinearHeapVtbl;
    if (h.base != 0)
        call(ctx, fn::Mem_FreePool, {h.base.get(), static_cast<u64>(static_cast<s64>(h.pool.get())), 0});
    h.cursor = 0;
    h.end = 0;
    h.base = 0;
}

}  // namespace

// void LinearHeap_Destruct(LinearHeap* heap)
DECOMP(00553140, LinearHeap_Destruct) {
    LinearHeap_Release(ctx, arg<u32>(ctx, 0));
    ret(ctx, 0u);
}

// void LinearHeap_Delete(LinearHeap* heap)  (deleting destructor)
DECOMP(005531A4, LinearHeap_Delete) {
    const u32 heap = arg<u32>(ctx, 0);
    if (heap == 0) return ret(ctx, 0u);
    LinearHeap_Release(ctx, heap);
    call(ctx, fn::Mem_FreePool, {heap, 0, 0});
}

// void LinearHeap_Free(LinearHeap* heap, void* p)
// A pointer outside the heap's block goes to Mem_FreePool. One inside it
// only moves the cursor back, when p - 16 lies below the cursor.
DECOMP(0055322C, LinearHeap_Free) {
    const u32 heap = arg<u32>(ctx, 0);
    const u32 p = arg<u32>(ctx, 1);
    auto& h = ref<LinearHeap>(heap);
    if (p < h.base || p > h.end) {
        call(ctx, fn::Mem_FreePool, {p, 0, 0});
        return;
    }
    if (p - 16 < h.cursor) h.cursor = (p - 1) & ~15u;
}

// void* DefaultHeap_Alloc(Heap* heap, u32 a, u32 b)  (name?)
// The default heap's +4: always 0 (0x0056C89C is a stub).
DECOMP(005514E0, DefaultHeap_Alloc) {
    ret(ctx, call(ctx, kDefaultHeapAlloc, {arg<u32>(ctx, 1), arg<u32>(ctx, 2), 0}));
}

// void DefaultHeap_Free(Heap* heap, void* p)
DECOMP(00551510, DefaultHeap_Free) {
    ret(ctx, call(ctx, fn::Mem_FreePool, {arg<u32>(ctx, 1), 0, 0}));
}

// void* Mem_Stub0()  (name?): returns 0
DECOMP(0056C89C, Mem_Stub0) { ret(ctx, 0u); }

// void Mem_FreeFrom(void* p, s32 pool)
DECOMP(0054EE6C, Mem_FreeFrom) {
    ret(ctx, call(ctx, fn::Mem_FreePool, {ctx.gpr[3], ctx.gpr[4], 0}));
}

// void* SmallAlloc(u32 size)  (name?)
// Forwards its registers unchanged to the size-class allocator.
DECOMP(0055238C, SmallAlloc) {
    ret(ctx, call(ctx, cmfn::SmallAlloc_Impl,
                  {ctx.gpr[3], ctx.gpr[4], ctx.gpr[5], ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], ctx.gpr[9], ctx.gpr[10]}));
}

// void SmallFree(void* p, u32 size)
DECOMP(00552420, SmallFree) { SmallFree_Impl(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1)); }

// void* DefaultAllocator_Alloc(Allocator* a, u32 size, bool clear, u32 align)
// The allocator PtrVector uses by default (vtable 0x00B24368 +0).
DECOMP(005523AC, DefaultAllocator_Alloc) {
    const u32 size = arg<u32>(ctx, 1);
    const u8 clear = arg<u8>(ctx, 2);
    // r4 still holds the size and r5 the flag when SmallAlloc_Impl runs.
    const u32 p = call32(ctx, cmfn::SmallAlloc_Impl, {size, size, ctx.gpr[5], ctx.gpr[6]});
    if (clear != 0 && size != 0) call(ctx, fn::memset, {p, 0, size});
    ret(ctx, p);
}

// u32 DefaultAllocator_Fn4(Allocator* a)  (+4): returns 0
DECOMP(00552418, DefaultAllocator_Fn4) { ret(ctx, 0u); }

// void DefaultAllocator_Free(Allocator* a, void* p, u32 size)  (+8)
DECOMP(005524D8, DefaultAllocator_Free) { SmallFree_Impl(ctx, arg<u32>(ctx, 1), arg<u32>(ctx, 2)); }

// void* memalign(u32 align, u32 size)
DECOMP(0072A88C, memalign) {
    ret(ctx, call32(ctx, cmfn::memalign_r, {kReent, ctx.gpr[3], ctx.gpr[4]}));
}

// void free(void* p)
DECOMP(0072A8FC, free) { ret(ctx, call(ctx, cmfn::free_r, {kReent, ctx.gpr[3]})); }

// void* malloc(u32 size)
DECOMP(0072A928, malloc) { ret(ctx, call32(ctx, cmfn::malloc_r, {kReent, ctx.gpr[3]})); }

// char* Sprintf_Output(char* dst, const char* src, u32 n)
// vsprintf's output function (OPD 0x00F486A8): appends n bytes, returns the new end.
DECOMP(007245E4, Sprintf_Output) {
    const u64 n = ctx.gpr[5];
    const u64 r = call(ctx, cmfn::memmove, {ctx.gpr[3], ctx.gpr[4], static_cast<u32>(n)});
    ret(ctx, static_cast<u32>(r + n));
}
