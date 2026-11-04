// Two instances of the sorted map lookup (identical code but for the
// default comparator they test for).
#include "obj.h"

using namespace sly4;
using namespace sly4::obj;

namespace {

// u32* Map_Find(Map* m, const u32* key)
u32 SortedMap_Find(PPUContext& ctx, u32 m, u32 keyp, u32 default_compare) {
    const u16 flags = load<u16>(m + kMapFlags);
    if ((flags & 1) != 0) {
        store<u16>(m + kMapFlags, static_cast<u16>(flags & 0xFFFE));
        const u32 n = load<u16>(m + kMapCount);
        if (static_cast<s32>(n) > 0)
            call(ctx, ofn::qsort, {load<u32>(m + kMapEntries), n, kMapEntrySize, load<u32>(m + kMapCompare)});
    }
    s32 hi = static_cast<s32>(load<u16>(m + kMapCount)) - 1, lo = 0;
    u32 entries = load<u32>(m + kMapEntries);
    const u32 cmp = load<u32>(m + kMapCompare);
    s32 found = -1;
    if (cmp == default_compare) {
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 key = load<u32>(keyp);
            const u32 k = load<u32>(entries + static_cast<u32>(mid * 8));
            if (k > key) {
                hi = mid - 1;
            } else if (k < key) {
                lo = mid + 1;
            } else {
                found = mid;
                break;
            }
        }
    } else {
        Stack frame(ctx, 16);
        const u32 key = frame.addr();
        store<u32>(key, load<u32>(keyp));
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const s32 r = static_cast<s32>(
                call_ptr(ctx, cmp, {entries + static_cast<u32>(mid * 8), key, cmp, load<u32>(cmp)}));  // r5 / r6 = OPD and entry
            if (r > 0) {
                hi = mid - 1;
            } else if (r < 0) {
                lo = mid + 1;
            } else {
                entries = load<u32>(m + kMapEntries);
                found = mid;
                break;
            }
        }
    }
    if (found < 0) return m + kMapDefault;
    return entries + static_cast<u32>(found * 8) + 4;
}

}  // namespace

// u32* Map_Find(Map* m, const u32* key)  (used by Tex_FindLoaded)
// The value slot for *key (sorting the entries first if they changed),
// or the map's default slot.
DECOMP(0079DD10, Map_Find) { ret(ctx, SortedMap_Find(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), kMapCompareU32A)); }

// u32* HashMap_Find2(Map* m, const u32* key)  (the world class map of UiElem8_Construct)
DECOMP(007399A4, HashMap_Find2) { ret(ctx, SortedMap_Find(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), kMapCompareU32B)); }
