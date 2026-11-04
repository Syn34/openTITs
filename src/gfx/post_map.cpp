// The hash -> program maps (two instantiations of one sorted-array map
// template, differing only in their default comparator): sorted lazily
// with qsort, searched by binary search, inserted in order.
#include "post.h"

using namespace sly4;

namespace {

namespace pfn = sly4::post::fn;
namespace pg = sly4::post::g;

void sort_if_needed(PPUContext& ctx, u32 map, u32 count) {
    call(ctx, pfn::qsort, {load<u32>(map + 8), count, 8, load<u32>(map + 20)});
}

// Pointer to the value stored under *key, or map + 16 when there is none.
u32 map_find(PPUContext& ctx, u32 map, u32 key, u32 default_compare) {
    const u16 flags = load<u16>(map + 6);
    if (flags & 1) {
        const u16 n = load<u16>(map + 2);
        store<u16>(map + 6, static_cast<u16>(flags & 0xFFFE));
        if (n > 0) sort_if_needed(ctx, map, n);
    }
    s32 hi = static_cast<s32>(load<u16>(map + 2)) - 1, lo = 0;
    const u32 base = load<u32>(map + 8);
    if (load<u32>(map + 20) == default_compare) {
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 k = load<u32>(base + static_cast<u32>(mid * 8));
            const u32 want = load<u32>(key);
            if (k > want) hi = mid - 1;
            else if (k < want) lo = mid + 1;
            else return mid < 0 ? map + 16 : base + static_cast<u32>(mid * 8) + 4;
        }
        return map + 16;
    }
    Stack frame(ctx, 8);  // a copy of the key for the comparator
    store<u32>(frame.addr(), load<u32>(key));
    while (hi >= lo) {
        const s32 mid = (hi + lo) >> 1;
        const s32 r = static_cast<s32>(
            call_ptr(ctx, load<u32>(map + 20), {base + static_cast<u32>(mid * 8), frame.addr()}));
        if (r > 0) hi = mid - 1;
        else if (r < 0) lo = mid + 1;
        else return mid < 0 ? map + 16 : load<u32>(map + 8) + static_cast<u32>(mid * 8) + 4;
    }
    return map + 16;
}

// Inserts {*key, value} in key order (after equal keys), growing the
// array when full. Returns the old high-water mark (what r3 holds).
u32 map_insert(PPUContext& ctx, u32 map, u32 key, u32 value) {
    u32 count = load<u16>(map + 2);
    if (count == load<u16>(map)) {
        call(ctx, pfn::Array_Reserve, {map, static_cast<u32>(load<u16>(map) << 1) & 0xFFFE});
        count = load<u16>(map + 2);
    }
    const u16 flags = load<u16>(map + 6);
    if (flags & 1) {
        store<u16>(map + 6, static_cast<u16>(flags & 0xFFFE));
        if (static_cast<s32>(count) > 0) {
            sort_if_needed(ctx, map, count);
            count = load<u16>(map + 2);
        }
    }
    const u32 base = load<u32>(map + 8);
    s32 i = static_cast<s32>(count);
    while (i > 0) {
        const u32 prev = base + static_cast<u32>((i - 1) * 8);
        const u32 kprev = load<u32>(prev);
        if (!(load<u32>(key) < kprev)) break;
        store<u32>(base + static_cast<u32>(i * 8), kprev);
        store<u32>(base + static_cast<u32>(i * 8) + 4, load<u32>(prev + 4));
        --i;
    }
    store<u32>(base + static_cast<u32>(i * 8), load<u32>(key));
    store<u32>(base + static_cast<u32>(i * 8) + 4, value);
    const u32 n = load<u16>(map + 2) + 1u;
    const u16 high = load<u16>(map + 4);
    store<u16>(map + 2, static_cast<u16>(n));
    store<u16>(map + 4, static_cast<s32>(high) > static_cast<s32>(n & 0xFFFF) ? high : static_cast<u16>(n));
    return high;
}

}  // namespace

// void FMap_Insert(ProgramMap* map, const u32* key, FProgram* value)
DECOMP(007A2BF4, FMap_Insert) {
    ctx.gpr[3] = map_insert(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), arg<u32>(ctx, 2));
}

// FProgram** FMap_Find(ProgramMap* map, const u32* key)
DECOMP(007A2D40, FMap_Find) {
    ctx.gpr[3] = map_find(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), pg::fmap_compare);
}

// void VMap_Insert(ProgramMap* map, const u32* key, VProgram* value)
DECOMP(007A2EC8, VMap_Insert) {
    ctx.gpr[3] = map_insert(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), arg<u32>(ctx, 2));
}

// VProgram** VMap_Find(ProgramMap* map, const u32* key)
DECOMP(007A3014, VMap_Find) {
    ctx.gpr[3] = map_find(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), pg::vmap_compare);
}
