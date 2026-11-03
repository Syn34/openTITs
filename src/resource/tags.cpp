// The tag chunk types and the tag table.
#include "tags.h"

using namespace sly4;
namespace tg = sly4::tags;

namespace {

constexpr u32 kIffAtag = 0x00F3C110, kIffTag2 = 0x00F3C118, kIffTagg = 0x00F3C120;  // handler OPDs
constexpr u32 kTagTable = 0x01164DE8;   // SortedMap* or 0
constexpr u32 kTagCompare = 0x00F3C220;  // the tag table's comparator (OPD)
constexpr u16 kTagCapacity = 32;

constexpr u32 code(char a, char b, char c, char d) {
    return static_cast<u32>(static_cast<u8>(a)) | static_cast<u32>(static_cast<u8>(b)) << 8 |
           static_cast<u32>(static_cast<u8>(c)) << 16 | static_cast<u32>(static_cast<u8>(d)) << 24;
}

// The default allocator, built on first use (a function-local static).
u32 default_allocator() {
    u32 a = load<u32>(tg::kDefaultAllocatorPtr);
    if (a != 0) return a;
    if (ref<u8>(tg::kDefaultAllocatorGuard) == 0) {
        store<u32>(tg::kDefaultAllocator, tg::kDefaultAllocatorVtable);
        ref<u8>(tg::kDefaultAllocatorGuard) = 1;
    }
    // (The original then null-tests the static's address and stores the vtable again.)
    store<u32>(tg::kDefaultAllocator, tg::kDefaultAllocatorVtable);
    store<u32>(tg::kDefaultAllocatorPtr, tg::kDefaultAllocator);
    return tg::kDefaultAllocator;
}

}  // namespace

// bool Iff_InitTags()
// Registers the ATAG / TAG2 / TAGG chunk types and creates the tag table
// (a SortedMap of 32 entries with its own comparator). Always true.
DECOMP(00559310, Iff_InitTags) {
    call(ctx, tg::fn::Iff_RegisterChunk, {code('A', 'T', 'A', 'G'), kIffAtag, 0, 0, 0});
    call(ctx, tg::fn::Iff_RegisterChunk, {code('T', 'A', 'G', '2'), kIffTag2, 0, 0, 0});
    call(ctx, tg::fn::Iff_RegisterChunk, {code('T', 'A', 'G', 'G'), kIffTagg, 0, 0, 0});

    u32 table = 0;
    const u32 obj = static_cast<u32>(call(ctx, sly4::fn::Mem_New, {sizeof(tg::SortedMap)}));
    if (obj != 0) {
        tg::SortedMap& map = ref<tg::SortedMap>(obj);
        map.flags = static_cast<u16>(tg::kEntrySize << 1);
        map.entries = 0;
        map.capacity = kTagCapacity;
        map.count = 0;
        map.high_water = 0;
        const u32 alloc = default_allocator();
        map.allocator = alloc;
        if (map.capacity != 0) {
            const u32 bytes = map.capacity * ((static_cast<u32>(map.flags) >> 1) & 0x7FFF);
            map.entries = static_cast<u32>(call_ptr(ctx, load<u32>(load<u32>(alloc)), {alloc, bytes, 0, 16}));
        }
        map.null_value = 0;
        map.compare = kTagCompare;
        table = obj;
    }
    store<u32>(kTagTable, table);
    ret(ctx, 1u);
}
