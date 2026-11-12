// KeyMap_Find and HandlePool_Remove: containers the audio code uses.
#include "audio.h"

namespace sly4::audio {
using namespace opentit::native;

namespace {
constexpr u32 kKeyEntrySize = 12;
}  // namespace

// Sorts first if needed, then binary-searches; returns the index or -1.
// The default comparator is replaced by an unsigned compare of the keys;
// any other is called with (entry, {key, 0, 0}).  (name?)
s32 KeyMap_Find(KeyMap* map, const be_u32* key) {
    const u16 flags = map->flags;
    if (flags & 1) {
        map->flags = static_cast<u16>(flags & 0xFFFE);
        const u16 count = map->count;
        if (static_cast<s32>(count) > 0) call(afn::SortCompare, map->entries.get(), static_cast<u32>(count), kKeyEntrySize, map->compare.get());
    }
    s32 hi = static_cast<s32>(map->count.get()) - 1;
    s32 lo = 0;
    const u32 entries = map->entries;
    const u32 compare = map->compare;
    auto mid_of = [&] { return static_cast<s32>(static_cast<u32>(hi) + static_cast<u32>(lo)) >> 1; };

    if (compare == kMapDefaultCompare) {
        while (lo <= hi) {
            const s32 mid = mid_of();
            const u32 e = gref<be_u32>(entries + static_cast<u32>(mid) * kKeyEntrySize);
            const u32 k = *key;
            if (e > k)
                hi = mid - 1;
            else if (e < k)
                lo = mid + 1;
            else
                return mid;
        }
        return -1;
    }

    struct Probe {
        be_u32 key;
        be_u32 w4;
        be_u32 w8;
    };
    GuestFrame frame(sizeof(Probe));
    Probe* probe = frame.as<Probe>();
    probe->w4 = 0u;
    probe->w8 = 0u;
    probe->key = key->get();
    while (lo <= hi) {
        const s32 mid = mid_of();
        const s32 r = call_ptr<s32>(compare, entries + static_cast<u32>(mid) * kKeyEntrySize, probe);
        if (r > 0)
            hi = mid - 1;
        else if (r < 0)
            lo = mid + 1;
        else
            return mid;
    }
    return -1;
}

// Unlinks the node of a live handle from the used list, pushes it on the
// free list and writes the handle of the node that followed it (0xFFFF0000
// if it was the last) to *out; *out = 0 for a dead handle.  (name?)
void HandlePool_Remove(be_u32* out, HandlePool* pool, u32 handle) {
    const u32 used = pool->used;
    const u32 free_head = pool->free_head;
    const u32 last = pool->last;
    if (used == 0) {
        *out = 0u;
        return;
    }
    const u32 serial = handle & 0xFFFF;
    const u32 index = handle >> 16;
    const s32 capacity = pool->capacity;
    if (serial == 0 || !(static_cast<s32>(index) < capacity)) {
        *out = 0u;
        return;
    }
    const u32 nodes = pool->nodes;
    auto node_at = [&](u32 i) -> HandleNode& { return gref<HandleNode>(nodes + i * kNodeSize); };
    HandleNode& node = node_at(index);
    if (((node.serial >> 15) & 0xFFFF) != serial || (gref<u8>(guest(&node.serial)) & kNodeFree)) {
        *out = 0u;
        return;
    }

    if (static_cast<s32>(index) == static_cast<s32>(pool->first.get()))
        pool->first = node.next.get();
    else
        node_at(node.prev).next = node.next.get();

    u32 next_handle;
    if (static_cast<s32>(index) == static_cast<s32>(last)) {
        pool->last = node.prev.get();
        next_handle = 0xFFFF0000u;
    } else {
        node_at(node.next).prev = node.prev.get();
        const u32 n = node.next;
        next_handle = (n << 16) | ((node_at(n & 0xFFFF).serial >> 15) & 0xFFFF);
    }

    // Mark the node free: top bit set, the serial (bits 15..30) cleared.
    auto mark_free = [&] {
        u8& first_byte = gref<u8>(guest(&node.serial));
        const u8 top = static_cast<u8>(first_byte | 0x80);
        const u32 word = node.serial;
        first_byte = top;
        node.serial = ((word & 0x00FFFFFFu) | (static_cast<u32>(top) << 24)) & ~0x7FFF8000u;
    };
    if (static_cast<s32>(free_head) != -1) {
        node_at(free_head).next = index;
        node.prev = free_head;
        node.next = 0xFFFFFFFFu;
        mark_free();
        pool->free_head = index;
        if (capacity == 1) pool->w0 = index;
    } else {
        node.prev = 0xFFFFFFFFu;
        node.next = 0xFFFFFFFFu;
        mark_free();
        pool->free_head = index;
        pool->w0 = index;
    }

    const u32 left = used - 1;
    pool->used = left;
    pool->free_count = pool->free_count + 1;
    if (left == 0) {
        pool->last = 0xFFFFFFFFu;
        pool->first = 0xFFFFFFFFu;
    }
    *out = next_handle;
}

}  // namespace sly4::audio

NATIVE(00784008, sly4::audio::KeyMap_Find)
NATIVE(0078378C, sly4::audio::HandlePool_Remove)
