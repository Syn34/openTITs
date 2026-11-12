// SortedMap: lookup, erase and insert.
#include "tags.h"

namespace sly4::tags {
using namespace opentit::native;

namespace {

MapEntry* entries_of(const SortedMap* map) { return &gref<MapEntry>(map->entries); }

// Sort the entries if an insert left them unsorted.
void sort_if_needed(SortedMap* map) {
    const u16 flags = map->flags;
    if (!(flags & kUnsorted)) return;
    map->flags = static_cast<u16>(flags & 0xFFFE);
    const u16 count = map->count;
    if (static_cast<s32>(count) > 0) call(fn::qsort, map->entries.get(), static_cast<u32>(count), kEntrySize, map->compare.get());
}

// Binary search over [lo, hi] = [0, count - 1], as the original does it:
// direct unsigned key compares with the default comparator, otherwise the
// map's comparator called with (entry, &key copy). `key` is re-read on
// every step of the direct search. -1 if not found.
s32 search(SortedMap* map, const be_u32* key) {
    s32 hi = static_cast<s32>(map->count) - 1, lo = 0;
    const u32 entries = map->entries;
    if (map->compare != kDefaultCompare) {
        GuestFrame frame(8);
        be_u32* copy = frame.as<be_u32>();
        *copy = key->get();
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const s32 c = call_ptr<s32>(map->compare, entries + static_cast<u32>(mid) * kEntrySize, copy);
            if (c > 0) hi = mid - 1;
            else if (c < 0) lo = mid + 1;
            else return mid;
        }
        return -1;
    }
    while (hi >= lo) {
        const s32 mid = (hi + lo) >> 1;
        const u32 k = *key;
        const u32 e = gref<MapEntry>(entries + static_cast<u32>(mid) * kEntrySize).key;
        if (e > k) hi = mid - 1;
        else if (e < k) lo = mid + 1;
        else return mid;
    }
    return -1;
}

}  // namespace

// Returns the value slot for `key`, or &map->null_value if it's missing.
be_u32* SortedMap_Find(SortedMap* map, const be_u32* key) {
    sort_if_needed(map);
    const s32 i = search(map, key);
    if (i < 0) return &map->null_value;
    return &gref<MapEntry>(map->entries + static_cast<u32>(i) * kEntrySize).value;
}

// Removes `key` if present, shifting the later entries down.
void SortedMap_Erase(SortedMap* map, const be_u32* key) {
    sort_if_needed(map);
    const s32 i = search(map, key);
    if (i < 0) return;
    const s32 count = map->count;
    MapEntry* entries = entries_of(map);
    if (count >= 2 && i < count - 1)
        for (s32 k = i; k < static_cast<s32>(map->count) - 1; ++k) entries[k] = entries[k + 1];
    map->count = static_cast<u16>(static_cast<s32>(map->count) - 1);
}

// Appends (growing to max(2 * capacity, capacity + 16) through the map's
// allocator when full) and marks the map unsorted. No duplicate check.
void SortedMap_Insert(SortedMap* map, const be_u32* key, u32 value) {
    const u16 capacity = map->capacity;
    u32 entries = map->entries;
    if (map->count == capacity) {
        const u32 size = (map->flags >> 1) & 0x7FFF;
        const u32 old_bytes = capacity * size;
        const s32 doubled = static_cast<s32>((static_cast<u32>(capacity) << 1) & 0xFFFE);
        const s32 stepped = static_cast<s32>((capacity + kGrowStep) & 0xFFFF);
        const u32 new_capacity = static_cast<u32>(doubled > stepped ? doubled : stepped);
        u8* alloc = host<u8>(map->allocator);
        entries = vcall<u32>(alloc, 0, new_capacity * size, 0u, 16u);
        if (const u32 old = map->entries) {
            call(fn::memmove, entries, old, old_bytes);
            vcall(host<u8>(map->allocator), 8, map->entries.get(), old_bytes);
        }
        map->entries = entries;
        map->capacity = static_cast<u16>(new_capacity);
    }
    const u16 index = map->count;
    const u32 count = static_cast<u32>(index) + 1;
    const u16 high = map->high_water;
    map->count = static_cast<u16>(count);
    MapEntry* e = &gref<MapEntry>(entries + static_cast<u32>(index) * kEntrySize);  // unchecked
    e->key = key->get();
    e->value = value;
    const u16 flags = static_cast<u16>(map->flags | kUnsorted);
    map->high_water = static_cast<u16>(static_cast<s32>(high) > static_cast<s32>(count & 0xFFFF) ? high : count);
    map->flags = flags;
}

}  // namespace sly4::tags

NATIVE(007A46EC, sly4::tags::SortedMap_Find)
NATIVE(007A44F8, sly4::tags::SortedMap_Erase)
NATIVE(007A4390, sly4::tags::SortedMap_Insert)
