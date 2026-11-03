// The engine's sorted map (u32 key -> u32 value, 8-byte entries), used for
// the IFF chunk registry and the tag table; and Iff_InitTags.
#pragma once

#include "../sly4.h"

namespace sly4::tags {

namespace fn {
// Decompiled in src/resource/tags.cpp and tags_map.cpp.
constexpr u32 Iff_InitTags = 0x00559310;
constexpr u32 SortedMap_Find = 0x007A46EC;    // (map, const u32* key) -> u32* value slot (or &map->null_value)
constexpr u32 SortedMap_Erase = 0x007A44F8;   // (map, const u32* key)
constexpr u32 SortedMap_Insert = 0x007A4390;  // (map, const u32* key, u32 value): appends, marks unsorted
// Still recompiled.
constexpr u32 qsort = 0x0071EC28;      // (base, count, size, compare opd)
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 Iff_RegisterChunk = 0x0055902C;
}  // namespace fn

// SortedMap (24 bytes). The entries are sorted lazily: Insert appends and
// sets kUnsorted; Find and Erase sort first.
struct SortedMap {
    be_u16 capacity;    // +0
    be_u16 count;       // +2
    be_u16 high_water;  // +4
    be_u16 flags;       // +6: bit 0 unsorted, the rest is entry size << 1
    gptr entries;       // +8: { u32 key; u32 value; } []
    gptr allocator;     // +12: vtable slot 0 alloc(this, size, 0, align), slot 2 free(this, p, size)
    be_u32 null_value;  // +16: Find returns &null_value when the key is missing
    gptr compare;       // +20: opd int(const Entry*, const u32* key); also the qsort comparator
};
static_assert(sizeof(SortedMap) == 24);
constexpr u16 kUnsorted = 1;
constexpr u32 kEntrySize = 8;
constexpr u32 kDefaultCompare = 0x00F3C250;  // with this comparator the search compares keys directly (unsigned)
constexpr u32 kGrowStep = 16;

// The default heap-backed allocator (a function-local static).
constexpr u32 kDefaultAllocatorPtr = 0x00F88704;  // u32
constexpr u32 kDefaultAllocator = 0x01165018;      // the object (just a vtable)
constexpr u32 kDefaultAllocatorGuard = 0x01165010;
constexpr u32 kDefaultAllocatorVtable = 0x00B24368;

}  // namespace sly4::tags
