// The hash -> program maps (two instantiations of one sorted-array map
// template, differing only in their default comparator): sorted lazily
// with qsort, searched by binary search, inserted in order.
#include "rsx.h"

namespace sly4::gfx {

namespace {

constexpr u32 qsort = 0x0071EC28;
constexpr u32 Array_Reserve = 0x00558F3C;  // (Array*, u16 capacity)
constexpr u32 kVMapCompare = 0x00F3BF88;   // default comparator OPDs (plain u32 key order)
constexpr u32 kFMapCompare = 0x00F3BF90;

void sort_entries(ProgramMap* map, u32 count) { call(qsort, map->data.get(), count, 8u, map->compare.get()); }

MapEntry* entry(ProgramMap* map, s32 i) { return at<MapEntry>(map->data.get(), static_cast<u32>(i * 8)); }

// Pointer to the value stored under *key, or &map->not_found when there is none.
be_u32* map_find(ProgramMap* map, const be_u32* key, u32 default_compare) {
    const u16 flags = map->flags;
    if (flags & 1) {
        const u16 n = map->count;
        map->flags = static_cast<u16>(flags & 0xFFFE);
        if (n > 0) sort_entries(map, n);
    }
    s32 hi = static_cast<s32>(map->count.get()) - 1, lo = 0;
    MapEntry* base = map->data;
    if (map->compare == default_compare) {
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 k = at<MapEntry>(base, static_cast<u32>(mid * 8))->key;
            const u32 want = *key;
            if (k > want) hi = mid - 1;
            else if (k < want) lo = mid + 1;
            else return mid < 0 ? &map->not_found : &at<MapEntry>(base, static_cast<u32>(mid * 8))->value;
        }
        return &map->not_found;
    }
    GuestFrame f(8);  // a copy of the key for the comparator
    be_u32* copy = f.as<be_u32>();
    *copy = key->get();
    while (hi >= lo) {
        const s32 mid = (hi + lo) >> 1;
        const s32 r = call_ptr<s32>(map->compare, at<MapEntry>(base, static_cast<u32>(mid * 8)), copy);
        if (r > 0) hi = mid - 1;
        else if (r < 0) lo = mid + 1;
        else return mid < 0 ? &map->not_found : &entry(map, mid)->value;
    }
    return &map->not_found;
}

// Inserts {*key, value} in key order (after equal keys), growing the
// array when full.
void map_insert(ProgramMap* map, const be_u32* key, u32 value) {
    u32 count = map->count;
    if (count == map->capacity) {
        call(Array_Reserve, map, static_cast<u32>(map->capacity << 1) & 0xFFFE);
        count = map->count;
    }
    const u16 flags = map->flags;
    if (flags & 1) {
        map->flags = static_cast<u16>(flags & 0xFFFE);
        if (static_cast<s32>(count) > 0) {
            sort_entries(map, count);
            count = map->count;
        }
    }
    MapEntry* base = map->data;
    s32 i = static_cast<s32>(count);
    while (i > 0) {
        const MapEntry* prev = at<MapEntry>(base, static_cast<u32>((i - 1) * 8));
        const u32 kprev = prev->key;
        if (!(*key < kprev)) break;
        MapEntry* to = at<MapEntry>(base, static_cast<u32>(i * 8));
        to->key = kprev;
        to->value = prev->value.get();
        --i;
    }
    MapEntry* slot = at<MapEntry>(base, static_cast<u32>(i * 8));
    slot->key = key->get();
    slot->value = value;
    const u32 n = map->count + 1u;
    const u16 high = map->high;
    map->count = static_cast<u16>(n);
    map->high = static_cast<s32>(high) > static_cast<s32>(n & 0xFFFF) ? high : static_cast<u16>(n);
}

}  // namespace

// void FMap_Insert(ProgramMap* map, const u32* key, FProgram* value)
void FMap_Insert(ProgramMap* map, const be_u32* key, FProgram* value) { map_insert(map, key, guest(value)); }

// FProgram** FMap_Find(ProgramMap* map, const u32* key)
gp<FProgram>* FMap_Find(ProgramMap* map, const be_u32* key) {
    return reinterpret_cast<gp<FProgram>*>(map_find(map, key, kFMapCompare));
}

// void VMap_Insert(ProgramMap* map, const u32* key, VProgram* value)
void VMap_Insert(ProgramMap* map, const be_u32* key, VProgram* value) { map_insert(map, key, guest(value)); }

// VProgram** VMap_Find(ProgramMap* map, const u32* key)
gp<VProgram>* VMap_Find(ProgramMap* map, const be_u32* key) {
    return reinterpret_cast<gp<VProgram>*>(map_find(map, key, kVMapCompare));
}

}  // namespace sly4::gfx

NATIVE(007A2BF4, sly4::gfx::FMap_Insert)
NATIVE(007A2D40, sly4::gfx::FMap_Find)
NATIVE(007A2EC8, sly4::gfx::VMap_Insert)
NATIVE(007A3014, sly4::gfx::VMap_Find)
