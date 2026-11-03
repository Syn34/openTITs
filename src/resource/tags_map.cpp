// SortedMap: lookup, erase and insert.
#include "tags.h"

using namespace sly4;
namespace tg = sly4::tags;

namespace {

tg::SortedMap& map_at(u32 ea) { return ref<tg::SortedMap>(ea); }

// Sort the entries if an insert left them unsorted.
void sort_if_needed(PPUContext& ctx, u32 m) {
    tg::SortedMap& map = map_at(m);
    const u16 flags = map.flags;
    if (!(flags & tg::kUnsorted)) return;
    map.flags = static_cast<u16>(flags & 0xFFFE);
    const u16 count = map.count;
    if (static_cast<s32>(count) > 0) call(ctx, tg::fn::qsort, {map.entries.get(), count, tg::kEntrySize, map.compare.get()});
}

struct Search {
    s32 index;  // -1 if not found
    u64 r3;     // what the original leaves in r3 when the key is missing (Erase returns it)
};

// Binary search over [lo, hi] = [0, count - 1], as the original does it:
// direct unsigned key compares with the default comparator, otherwise the
// map's comparator called with (entry, &key copy). `key` is re-read on
// every step of the direct search.
Search search(PPUContext& ctx, u32 m, u32 key_ptr, u64 r3_direct) {
    tg::SortedMap& map = map_at(m);
    s32 hi = static_cast<s32>(map.count) - 1, lo = 0;
    const u32 entries = map.entries;
    if (map.compare != tg::kDefaultCompare) {
        Stack frame(ctx, 8);
        store<u32>(frame.addr(), load<u32>(key_ptr));
        u64 r3 = load<u32>(key_ptr);
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const s32 c = static_cast<s32>(
                call_ptr(ctx, map_at(m).compare, {entries + static_cast<u32>(mid) * tg::kEntrySize, frame.addr()}));
            r3 = ctx.gpr[3];
            if (c > 0) hi = mid - 1;
            else if (c < 0) lo = mid + 1;
            else return {mid, r3};
        }
        return {-1, r3};
    }
    while (hi >= lo) {
        const s32 mid = (hi + lo) >> 1;
        const u32 key = load<u32>(key_ptr);
        const u32 e = load<u32>(entries + static_cast<u32>(mid) * tg::kEntrySize);
        if (e > key) hi = mid - 1;
        else if (e < key) lo = mid + 1;
        else return {mid, r3_direct};
    }
    return {-1, r3_direct};
}

}  // namespace

// u32* SortedMap_Find(SortedMap* map, const u32* key)
// Returns the value slot for `key`, or &map->null_value if it's missing.
DECOMP(007A46EC, SortedMap_Find) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 key = arg<u32>(ctx, 1);
    sort_if_needed(ctx, m);
    const Search s = search(ctx, m, key, 0);
    if (s.index < 0) return ret(ctx, m + 16);
    ret(ctx, map_at(m).entries + static_cast<u32>(s.index) * tg::kEntrySize + 4);
}

// void SortedMap_Erase(SortedMap* map, const u32* key)
// Removes `key` if present, shifting the later entries down. (r3 on return
// is the new count when found; otherwise whatever the search left.)
DECOMP(007A44F8, SortedMap_Erase) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 key = arg<u32>(ctx, 1);
    sort_if_needed(ctx, m);
    tg::SortedMap& map = map_at(m);
    const Search s = search(ctx, m, key, map.count);
    if (s.index < 0) {
        ctx.gpr[3] = s.r3;
        return;
    }
    const s32 count = map.count;
    const u32 entries = map.entries;
    if (count >= 2 && s.index < count - 1) {
        for (s32 k = s.index; k < static_cast<s32>(map.count) - 1; ++k) {
            const u32 e = entries + static_cast<u32>(k) * tg::kEntrySize;
            store<u32>(e, load<u32>(e + 8));
            store<u32>(e + 4, load<u32>(e + 12));
        }
    }
    const s32 now = static_cast<s32>(map.count) - 1;
    map.count = static_cast<u16>(now);
    ret(ctx, now);
}

// void SortedMap_Insert(SortedMap* map, const u32* key, u32 value)
// Appends (growing to max(2 * capacity, capacity + 16) through the map's
// allocator when full) and marks the map unsorted. No duplicate check.
DECOMP(007A4390, SortedMap_Insert) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 key = arg<u32>(ctx, 1);
    const u32 value = arg<u32>(ctx, 2);
    tg::SortedMap& map = map_at(m);

    const u16 capacity = map.capacity;
    u32 entries = map.entries;
    if (map.count == capacity) {
        const u32 size = (map.flags >> 1) & 0x7FFF;
        const u32 old_bytes = capacity * size;
        const s32 doubled = static_cast<s32>((static_cast<u32>(capacity) << 1) & 0xFFFE);
        const s32 stepped = static_cast<s32>((capacity + tg::kGrowStep) & 0xFFFF);
        const u32 new_capacity = static_cast<u32>(doubled > stepped ? doubled : stepped);
        const u32 alloc = map.allocator;
        entries = static_cast<u32>(
            call_ptr(ctx, load<u32>(load<u32>(alloc)), {alloc, new_capacity * size, 0, 16}));
        const u32 old = map.entries;
        if (old != 0) {
            call(ctx, tg::fn::memmove, {entries, old, old_bytes});
            const u32 a = map.allocator;
            call_ptr(ctx, load<u32>(load<u32>(a) + 8), {a, map.entries.get(), old_bytes});
        }
        map.entries = entries;
        map.capacity = static_cast<u16>(new_capacity);
    }

    const u16 index = map.count;
    const u32 count = static_cast<u32>(index) + 1;
    const u16 high = map.high_water;
    map.count = static_cast<u16>(count);
    store<u32>(entries + static_cast<u32>(index) * tg::kEntrySize, load<u32>(key));
    store<u32>(entries + static_cast<u32>(index) * tg::kEntrySize + 4, value);
    const u16 flags = static_cast<u16>(map.flags | tg::kUnsorted);
    map.high_water = static_cast<u16>(static_cast<s32>(high) > static_cast<s32>(count & 0xFFFF) ? high : count);
    map.flags = flags;
    ret(ctx, static_cast<u32>(flags));
}
