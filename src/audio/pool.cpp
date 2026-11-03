// KeyMap_Find and HandlePool_Remove: containers the audio code uses.
#include "audio.h"

using namespace sly4;
using namespace sly4::audio;

// s32 KeyMap_Find(Map* map, const u32* key)  (name?)
//
// Map: +2 u16 count, +6 u16 flags (bit 0 = needs sorting), +8 entries of
// 12 bytes (key first), +24 comparator OPD. Sorts first if needed, then
// binary-searches; returns the index or -1. The default comparator is
// replaced by an unsigned compare of the keys; any other is called with
// (entry, {key, 0, 0}).
DECOMP(00784008, KeyMap_Find) {
    const u32 map = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u16 flags = load<u16>(map + 6);
    if (flags & 1) {
        store<u16>(map + 6, flags & 0xFFFE);
        const u16 count = load<u16>(map + 2);
        if (static_cast<s32>(count) > 0)
            call(ctx, afn::SortCompare, {load<u32>(map + 8), count, 12, load<u32>(map + 24)});
    }
    s32 hi = static_cast<s32>(load<u16>(map + 2)) - 1;
    s32 lo = 0;
    const u32 entries = load<u32>(map + 8);
    const u32 compare = load<u32>(map + 24);
    auto mid_of = [&] { return static_cast<s32>(static_cast<u32>(hi) + static_cast<u32>(lo)) >> 1; };

    if (compare == kMapDefaultCompare) {
        while (lo <= hi) {
            const s32 mid = mid_of();
            const u32 e = load<u32>(entries + static_cast<u32>(mid) * 12);
            const u32 k = load<u32>(key);
            if (e > k)
                hi = mid - 1;
            else if (e < k)
                lo = mid + 1;
            else
                return ret(ctx, mid);
        }
        return ret(ctx, -1);
    }

    Stack frame(ctx, 12);
    const u32 probe = frame.addr();
    store<u32>(probe + 4, 0);
    store<u16>(probe + 6, 0);
    store<u32>(probe + 8, 0);
    store<u32>(probe, load<u32>(key));
    while (lo <= hi) {
        const s32 mid = mid_of();
        const s32 r = static_cast<s32>(call_ptr(ctx, compare, {entries + static_cast<u32>(mid) * 12, probe}));
        if (r > 0)
            hi = mid - 1;
        else if (r < 0)
            lo = mid + 1;
        else
            return ret(ctx, mid);
    }
    ret(ctx, -1);
}

// void HandlePool_Remove(u32* out, Pool* pool, u32 handle)  (name?)
//
// Pool: +0 ?, +4 free list head (-1 = empty), +8 first used, +12 last
// used, +16 used count, +20 free count, +24 capacity, +28 nodes (272
// bytes: +0 prev, +4 next, +256 serial word, top bit = free). Unlinks the
// node of a live handle from the used list, pushes it on the free list
// and writes the handle of the node that followed it (0xFFFF0000 if it
// was the last) to *out; *out = 0 for a dead handle.
DECOMP(0078378C, HandlePool_Remove) {
    const u32 out = arg<u32>(ctx, 0), pool = arg<u32>(ctx, 1), handle = arg<u32>(ctx, 2);
    const u32 used = load<u32>(pool + 16);
    const u32 free_head = load<u32>(pool + 4);
    const u32 last = load<u32>(pool + 12);
    auto done = [&](u32 v) { store<u32>(out, v); };
    if (used == 0) return done(0);
    const u32 serial = handle & 0xFFFF;
    if (serial == 0) return done(0);
    const u32 index = handle >> 16;
    const s32 capacity = load<s32>(pool + 24);
    if (!(static_cast<s32>(index) < capacity)) return done(0);
    const u32 nodes = load<u32>(pool + 28);
    const u32 node = nodes + index * kNodeSize;
    if (((load<u32>(node + kNodeSerial) >> 15) & 0xFFFF) != serial) return done(0);
    if (ref<u8>(node + kNodeSerial) & kNodeFree) return done(0);

    if (static_cast<s32>(index) == load<s32>(pool + 8))
        store<u32>(pool + 8, load<u32>(node + 4));
    else
        store<u32>(nodes + load<u32>(node) * kNodeSize + 4, load<u32>(node + 4));

    u32 next_handle;
    if (static_cast<s32>(index) == static_cast<s32>(last)) {
        store<u32>(pool + 12, load<u32>(node));
        next_handle = 0xFFFF0000u;
    } else {
        const u32 next = load<u32>(node + 4);
        store<u32>(nodes + next * kNodeSize, load<u32>(node));
        const u32 n = load<u32>(node + 4);
        next_handle = (n << 16) | ((load<u32>(nodes + (n & 0xFFFF) * kNodeSize + kNodeSerial) >> 15) & 0xFFFF);
    }

    // Mark the node free: top bit set, the serial (bits 15..30) cleared.
    auto mark_free = [&] {
        const u32 word_ea = node + kNodeSerial;
        const u8 top = static_cast<u8>(ref<u8>(word_ea) | 0x80);
        const u32 word = load<u32>(word_ea);
        ref<u8>(word_ea) = top;
        store<u32>(word_ea, ((word & 0x00FFFFFFu) | (static_cast<u32>(top) << 24)) & ~0x7FFF8000u);
    };
    if (static_cast<s32>(free_head) != -1) {
        store<u32>(nodes + free_head * kNodeSize + 4, index);
        store<u32>(node, free_head);
        store<s32>(node + 4, -1);
        mark_free();
        store<u32>(pool + 4, index);
        if (capacity == 1) store<u32>(pool + 0, index);
    } else {
        store<s32>(node, -1);
        store<s32>(node + 4, -1);
        mark_free();
        store<u32>(pool + 4, index);
        store<u32>(pool + 0, index);
    }

    const u32 left = used - 1;
    store<u32>(pool + 16, left);
    store<u32>(pool + 20, load<u32>(pool + 20) + 1);
    if (left == 0) {
        store<s32>(pool + 12, -1);
        store<s32>(pool + 8, -1);
    }
    done(next_handle);
}
