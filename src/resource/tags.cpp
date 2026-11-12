// The tag chunk types and the tag table.
#include "tags.h"

#include "../container/container.h"
#include "../memory/mem.h"
#include "../platform/platform.h"

namespace sly4::tags {
using namespace opentit::native;

namespace {

constexpr u32 kIffAtag = 0x00F3C110, kIffTag2 = 0x00F3C118, kIffTagg = 0x00F3C120;  // handler OPDs
constexpr u32 kTagTable = 0x01164DE8;   // SortedMap* or 0
constexpr u32 kTagCompare = 0x00F3C220;  // the tag table's comparator (OPD)
constexpr u16 kTagCapacity = 32;

constexpr u32 code(char a, char b, char c, char d) {
    return static_cast<u32>(static_cast<u8>(a)) | static_cast<u32>(static_cast<u8>(b)) << 8 |
           static_cast<u32>(static_cast<u8>(c)) << 16 | static_cast<u32>(static_cast<u8>(d)) << 24;
}

}  // namespace

// Registers the ATAG / TAG2 / TAGG chunk types and creates the tag table
// (a SortedMap of 32 entries with its own comparator). Always true.
bool Iff_InitTags() {
    platform::Iff_RegisterChunk(code('A', 'T', 'A', 'G'), kIffAtag, 0, nullptr, 0);
    platform::Iff_RegisterChunk(code('T', 'A', 'G', '2'), kIffTag2, 0, nullptr, 0);
    platform::Iff_RegisterChunk(code('T', 'A', 'G', 'G'), kIffTagg, 0, nullptr, 0);
    SortedMap* table = nullptr;
    if (SortedMap* map = static_cast<SortedMap*>(memory::Mem_New(sizeof(SortedMap)))) {
        map->flags = static_cast<u16>(kEntrySize << 1);
        map->entries = 0;
        map->capacity = kTagCapacity;
        map->count = 0;
        map->high_water = 0;
        // The default allocator, built on first use (the original then
        // null-tests the static's address and stores the vtable again).
        container::Allocator* alloc = container::DefaultAllocator_Get();
        map->allocator = guest(alloc);
        if (map->capacity != 0) {
            const u32 bytes = map->capacity * ((static_cast<u32>(map->flags) >> 1) & 0x7FFF);
            map->entries = vcall<u32>(alloc, 0, bytes, 0u, 16u);
        }
        map->null_value = 0;
        map->compare = kTagCompare;
        table = map;
    }
    global<be_u32>(kTagTable) = guest(table);
    return true;
}

}  // namespace sly4::tags

NATIVE(00559310, sly4::tags::Iff_InitTags)
