// IFF chunk type registry: four-character codes -> ChunkDesc (handler,
// name, flags), kept in a map and allocated from a free list.
#include "../platform/platform.h"

#include "../container/container.h"
#include "../memory/mem.h"
#include "tags.h"

namespace sly4::platform {
using namespace opentit::native;
namespace pf = sly4::platform;  // its fn/libc/imp/sc sit beside sly4's

namespace {

constexpr u32 kChunkMap = 0x01164E30;  // map<u32 fourcc, ChunkDesc*>

// ChunkDesc (20 bytes, from the free list below).
struct ChunkDesc {
    be_u32 fourcc;   // +0
    be_u32 handler;  // +4  OPD
    be_u32 arg;      // +8  Iff_RegisterChunk's third argument
    gp<const char> name;  // +12
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
container::FreeList* chunk_allocator() {
    gp<container::FreeList>& slot = global<gp<container::FreeList>>(kChunkAllocator);
    if (slot) return slot;
    const u32 count = global<be_u32>(kChunkAllocatorCount);
    container::FreeList* list = nullptr;
    if (auto* obj = static_cast<container::FreeList*>(memory::Mem_New(kFreeListSize))) {
        be_u32* w = reinterpret_cast<be_u32*>(obj);
        for (u32 off = 4; off <= 48; off += 4) w[off / 4] = 0;
        be_u16* h = reinterpret_cast<be_u16*>(obj);
        h[52 / 2] = 0;
        h[54 / 2] &= static_cast<u16>(~0x8000);
        w[0] = kFreeListVtable;
        w[56 / 4] = kStrGetAllocator;
        call(pf::fn::FreeList_Init, obj, static_cast<u32>(sizeof(ChunkDesc)), count, 0u, 0u);
        list = obj;
    }
    slot = list;
    return list;
}

}  // namespace

// Registers (or replaces) the handler for an IFF chunk type. Replacing an
// existing type prints a message unless kIffQuiet is set.
void Iff_RegisterChunk(u32 code, u32 handler, u32 extra, const char* name, u32 flags) {
    tags::SortedMap* map = host<tags::SortedMap>(kChunkMap);
    GuestFrame frame(8);
    be_u32* insert_key = frame.as<be_u32>(0);
    be_u32* find_key = frame.as<be_u32>(4);
    *find_key = code;
    ChunkDesc* existing = host<ChunkDesc>(*tags::SortedMap_Find(map, find_key));
    if (existing) {
        if (!(flags & kIffQuiet))
            call(pf::fn::String_Format, kStrReplacing, code & 0xFF, (code >> 8) & 0xFF, (code >> 16) & 0xFF, code >> 24,
                 existing->name.get(), name);  // the message is formatted, not printed
        tags::SortedMap_Erase(map, &existing->fourcc);
        container::FreeList* list = chunk_allocator();
        vcall(list, kFreeListFreeSlot, existing);
    }
    *insert_key = code;
    container::FreeList* list = chunk_allocator();  // unchecked: 0 if the allocation failed
    ChunkDesc* desc = vcall<ChunkDesc*>(list, kFreeListAllocSlot);
    if (desc) {
        desc->handler = handler;
        desc->fourcc = insert_key->get();
        desc->arg = extra;
        desc->name = name;
        desc->flags = flags & kIffFlag10;
    }
    tags::SortedMap_Insert(map, insert_key, guest(desc));  // even with desc 0
}

}  // namespace sly4::platform

NATIVE(0055902C, sly4::platform::Iff_RegisterChunk)
