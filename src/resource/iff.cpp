// IFF chunk type registry: four-character codes -> ChunkDesc (handler,
// name, flags), kept in a map and allocated from a free list.
#include "../platform/platform.h"

using namespace sly4;
using namespace sly4::platform;
namespace pf = sly4::platform;  // its fn/libc/imp/sc sit beside sly4's

namespace {

constexpr u32 kChunkMap = 0x01164E30;  // map<u32 fourcc, ChunkDesc*>

// ChunkDesc (20 bytes, from the free list below).
struct ChunkDesc {
    be_u32 fourcc;   // +0
    gptr handler;    // +4  OPD
    be_u32 arg;      // +8  Iff_RegisterChunk's third argument
    gptr name;       // +12 const char*
    be_u32 flags;    // +16 only kIffFlag10 is kept
};
static_assert(sizeof(ChunkDesc) == 20);

// ApeTListAllocator<IFF::ChunkDesc>::GetAllocator(): a lazily built free list.
constexpr u32 kChunkAllocator = 0x01164E28;       // FreeList*
constexpr u32 kChunkAllocatorCount = 0x01164E2C;  // u32 element count for FreeList_Init
constexpr u32 kFreeListSize = 60;
constexpr u32 kFreeListVtable = 0x00B24438;
constexpr u32 kStrGetAllocator = 0x00AF363A;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = IFF::ChunkDesc]"
constexpr u32 kFreeListAllocSlot = 12;  // vtable offsets
constexpr u32 kFreeListFreeSlot = 16;

constexpr u32 kStrReplacing = 0x00AF3754;  // "Replacing %c%c%c%c (%s) with %s\n"

// Build the 60-byte free list header the way the original inlines it.
u32 chunk_allocator(PPUContext& ctx) {
    u32 list = load<u32>(kChunkAllocator);
    if (list != 0) return list;
    const u32 count = load<u32>(kChunkAllocatorCount);
    const u32 obj = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {kFreeListSize}));
    if (obj != 0) {
        for (u32 off = 4; off <= 48; off += 4) field<u32>(obj, off) = 0;
        field<u16>(obj, 52) = 0;
        field<u16>(obj, 54) &= static_cast<u16>(~0x8000);
        field<u32>(obj, 0) = kFreeListVtable;
        field<u32>(obj, 56) = kStrGetAllocator;
        call(ctx, pf::fn::FreeList_Init, {obj, sizeof(ChunkDesc), count, 0, 0});
        list = obj;
    }
    store<u32>(kChunkAllocator, list);
    return list;
}

u32 vcall(PPUContext& ctx, u32 obj, u32 slot, std::initializer_list<u64> args) {
    return static_cast<u32>(call_ptr(ctx, load<u32>(load<u32>(obj) + slot), args));
}

}  // namespace

// void Iff_RegisterChunk(u32 fourcc, handler opd, u32 arg, const char* name, u32 flags)
// Registers (or replaces) the handler for an IFF chunk type. Replacing an
// existing type prints a message unless kIffQuiet is set.
DECOMP(0055902C, Iff_RegisterChunk) {
    const u32 code = arg<u32>(ctx, 0);
    const u32 handler = arg<u32>(ctx, 1);
    const u32 extra = arg<u32>(ctx, 2);
    const u32 name = arg<u32>(ctx, 3);
    const u32 flags = arg<u32>(ctx, 4);

    Stack frame(ctx, 8);
    const u32 insert_key = frame.addr(), find_key = frame.addr() + 4;
    store<u32>(find_key, code);
    const u32 slot = static_cast<u32>(call(ctx, pf::fn::IffMap_Find, {kChunkMap, find_key}));
    const u32 existing = load<u32>(slot);
    if (existing != 0) {
        if (!(flags & kIffQuiet))
            call(ctx, pf::fn::String_Format,
                 {kStrReplacing, code & 0xFF, (code >> 8) & 0xFF, (code >> 16) & 0xFF, code >> 24,
                  ref<ChunkDesc>(existing).name.get(), name});  // the message is formatted, not printed
        call(ctx, pf::fn::IffMap_Erase, {kChunkMap, existing});
        const u32 list = chunk_allocator(ctx);
        vcall(ctx, list, kFreeListFreeSlot, {list, existing});
    }

    store<u32>(insert_key, code);
    const u32 list = chunk_allocator(ctx);  // unchecked: 0 if the allocation failed
    const u32 desc = vcall(ctx, list, kFreeListAllocSlot, {list});
    if (desc != 0) {
        ChunkDesc& d = ref<ChunkDesc>(desc);
        d.handler = handler;
        d.fourcc = load<u32>(insert_key);
        d.arg = extra;
        d.name = name;
        d.flags = flags & kIffFlag10;
    }
    ctx.gpr[3] = call(ctx, pf::fn::IffMap_Insert, {kChunkMap, insert_key, desc});  // even with desc 0
}
