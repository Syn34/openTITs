// Small helpers of Gcm_Init: system module loading with memory accounting,
// the RSX local / mapped main memory pools, the render command buffer and
// a bump region. (Pool_Create is the memory manager's, but only Gcm_Init's
// helpers call it.)
#include "rsx.h"

#include <atomic>

namespace sly4::gfx {

namespace {

constexpr u32 kMemPoolA = 0x0116B340, kMemPoolB = 0x0116B328;  // pools 4 and 5 (src/memory/mem.h)
constexpr u32 cellSysmoduleLoadModule = 0x00ABD944;
constexpr u32 cellGcmMapMainMemory = 0x00ABD764;
constexpr u32 sys_memory_get_user_memory_size = 352;
constexpr u32 init_user_mstate = 0x005504F4;  // dlmalloc (name?)
constexpr u32 Mem_AllocPool = 0x0056C514;     // (align, size, pool, flags)
constexpr u32 kMagic = 0x58585858;

// dlmalloc's mparams (0x011648A8) and the global mstate (0x011648C0).
struct MParams {
    be_u32 magic;           // +0
    be_u32 page_size;       // +4
    be_u32 granularity;     // +8
    be_u32 mmap_threshold;  // +12
    be_u32 trim_threshold;  // +16
    be_u32 default_mflags;  // +20
};
constexpr u32 kMparams = 0x011648A8;
constexpr u32 kGlobalMstate = 0x011648C0;  // +436 mflags

struct MemPool {
    be_u32 base;    // +0
    be_u32 size;    // +4
    be_u32 mspace;  // +8
    be_u32 used;    // +12
    be_u32 free;    // +16
};

struct Region {
    be_u32 base;    // +0
    be_u32 cursor;  // +4
    be_u32 size;    // +8
};

// Two RSX command buffers and their get / put words.
struct CommandBuffer {
    pad<256> _0;
    gp<be_u32> get;      // +256: points at +0
    gp<be_u32> put;      // +260: points at +128
    be_u32 size;         // +264
    be_u32 buffer[2];    // +268
    be_u32 offset[2];    // +276
    be_u32 w284;         // +284
};
static_assert(offsetof(CommandBuffer, w284) == 284);

be_u32& stats_u32(u32 off) { return *at<be_u32>(&mem_stats(), off); }

void atomic_store(be_u32* p, u32 v) {  // lwarx / stwcx. loop in the original
    std::atomic_ref<u32>(*reinterpret_cast<u32*>(p)).store(bswap32(v));
}

}  // namespace

void Pool_Create(MemPool* pool, u32 base, u32 size, bool reset);

// bool Sysmodule_Require(u16 id)
// cellSysmoduleLoadModule, adding the user memory it took to the stats.
bool Sysmodule_Require(u32 id) {
    id &= 0xFFFF;
    GuestFrame f(16);
    MemoryInfo* before = f.as<MemoryInfo>(0);
    MemoryInfo* after = f.as<MemoryInfo>(8);
    // `sc` has no native form: the LV2 call goes through the PPU context.
    decomp::syscall(ctx(), sys_memory_get_user_memory_size, {guest(before)});
    const s32 rc = call<s32>(cellSysmoduleLoadModule, id);
    decomp::syscall(ctx(), sys_memory_get_user_memory_size, {guest(after)});
    be_u32& module_memory = stats_u32(360);
    module_memory = module_memory + (before->available_user_memory - after->available_user_memory);
    return rc >= 0;
}

// void RsxLocal_Init(u32 address, u32 size)
// Records RSX local memory and makes it pool 4 (16 bytes kept back).
void RsxLocal_Init(u32 address, u32 size) {
    stats_u32(60) = address;
    stats_u32(64) = size;
    Pool_Create(host<MemPool>(kMemPoolA), address, size - 16, false);
    stats_u32(212) = size;  // pool_a_free
    stats_u32(216) = size;  // pool_a_min_free
    stats_u32(208) = 0u;    // pool_a_used
}

// void RsxMain_Init(u32 address, u32 size)
// Maps main memory for the RSX and makes it pool 5.
void RsxMain_Init(u32 address, u32 size) {
    {
        GuestFrame f(8);
        call(cellGcmMapMainMemory, address, size, f.as<be_u32>());
    }
    Pool_Create(host<MemPool>(kMemPoolB), address, size, false);
    stats_u32(196) = 0u;    // pool_b_used
    stats_u32(200) = size;  // pool_b_free
    stats_u32(204) = size;  // pool_b_min_free
}

// void Heap_InitRegion(Region* r, u32 base, u32 size): { base, cursor = base, size }.
void Heap_InitRegion(Region* r, u32 base, u32 size) {
    r->base = base;
    r->size = size;
    r->cursor = base;
}

// void CommandBuffer_Construct(CommandBuffer* cb, u32 size)
// Two RSX command buffers of size (rounded to 4) + 4 KiB from pool 5,
// with their RSX offsets, cleared; then the get/put words.
void CommandBuffer_Construct(CommandBuffer* cb, u32 size) {
    size = (size + 3) & ~3u;
    const u32 alloc = size + 4096;
    cb->get = host<be_u32>(guest(cb));
    cb->put = host<be_u32>(guest(cb) + 128);
    cb->w284 = 0u;
    cb->size = size;
    for (u32 i = 0; i < 2; ++i) {
        cb->buffer[i] = call<u32>(Mem_AllocPool, 128u, alloc, 5u, 7u);
        cb->offset[i] = 0u;
        call(fn::cellGcmAddressToOffset, cb->buffer[i].get(), &cb->offset[i]);
        call(sly4::fn::memset, cb->buffer[i].get(), 0u, alloc);
    }
    atomic_store(cb->get, 0);
    const u32 words = cb->size >> 2;
    atomic_store(cb->put, words);
}

// void Pool_Create(MemPool* pool, void* base, u32 size, bool reset)
// dlmalloc create_mspace_with_base (init_mparams inlined) for a MemPool:
// { base, size, mspace, used = 0, free = size }. No mspace (0) if the
// region is too small or too large.
void Pool_Create(MemPool* pool, u32 base, u32 size, bool reset) {
    if (reset && pool->mspace != 0) pool->mspace = 0u;
    MParams& mp = global<MParams>(kMparams);
    u32 page = mp.page_size;
    if (page == 0) {
        mp.mmap_threshold = 0xFFFFFFFFu;
        mp.trim_threshold = 0x200000u;
        mp.default_mflags = 4u;
        if (mp.magic == 0) {
            mp.magic = kMagic;
            global<be_u32>(kGlobalMstate + 436) = 4u;
        }
        page = 4096;
        mp.page_size = page;
        mp.granularity = 0x10000u;
    }
    u32 mspace = 0;
    if (size > 0x208 && size < static_cast<u32>(-static_cast<s32>(page + 520))) {
        mspace = call<u32>(init_user_mstate, base, size);
        global<be_u32>(mspace + 452) = 8u;
    }
    pool->mspace = mspace;
    pool->base = base;
    pool->size = size;
    pool->used = 0u;
    pool->free = size;
}

}  // namespace sly4::gfx

NATIVE(0056C020, sly4::gfx::Sysmodule_Require)
NATIVE(0056C428, sly4::gfx::RsxLocal_Init)
NATIVE(0056C49C, sly4::gfx::RsxMain_Init)
NATIVE(0056DF80, sly4::gfx::Heap_InitRegion)
NATIVE(0052F670, sly4::gfx::CommandBuffer_Construct)
NATIVE(00565AA8, sly4::gfx::Pool_Create)
