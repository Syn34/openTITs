// Emitter construction, the emitter key map and growing the emitter pool.
#include "audio.h"
#include "more.h"

using namespace sly4;
using namespace sly4::audio;

namespace {

constexpr u32 kKeyMap = 0x010AFEF8;   // Map: +0 u16 capacity, +2 u16 count, +4 u16 high water,
                                      // +6 u16 flags (bit 0 = unsorted), +8 entries (12 bytes:
                                      // key, handle, priority), +16 default entry, +24 compare OPD
constexpr u32 kKeyMapFind = 0x00784008;
constexpr u32 kZeroVec = 0x01164B40;  // f32[3] (name?)
constexpr u32 kOne = 0x3F800000u, kPitch = 0x46BB7E00u;  // 1.0f, 23999.0f
constexpr u32 kBlockSize = 57;

f64 lf(u32 ea) { return static_cast<f64>(load<f32>(ea)); }
// lfs + stfs: the copy goes through double, as the recompiled code does.
void copyf(u32 dst, u32 src) { store<f32>(dst, static_cast<f32>(lf(src))); }

// The 57-byte 3D block's initial value: six 1.0s, three 23999.0s, then 0.
void Block_Init(u32 b) {
    for (u32 o = 0; o < 24; o += 4) store<u32>(b + o, kOne);
    for (u32 o = 24; o < 36; o += 4) store<u32>(b + o, kPitch);
    for (u32 o = 36; o < 56; o += 4) store<u32>(b + o, 0);
    ref<u8>(b + 56) = 0;
}

u32 Node(u32 nodes, u32 i) { return nodes + i * kNodeSize; }

}  // namespace

// void Emitter_Init(Emitter* e)
// No voice (-1), both copies of the 3D block at their defaults, fade and
// pitch at rest, position and velocity zero, no key, bucket 0.
DECOMP(00440B48, Emitter_Init) {
    const u32 e = arg<u32>(ctx, 0);
    store<s32>(e, -1);
    store<u32>(e + 4, 0);
    store<u32>(e + 8, 0);
    ref<u8>(e + 12) = 0;
    ref<u8>(e + 13) = 0;
    Stack frame(ctx, 64);
    const u32 block = frame.addr();
    Block_Init(block);
    call(ctx, mfn::memmove, {e + 16, block, kBlockSize});
    Block_Init(block);
    call(ctx, mfn::memmove, {e + 76, block, kBlockSize});
    store<u32>(e + 140, kOne);
    store<u32>(e + 144, kOne);
    store<u32>(e + 148, kPitch);
    store<u32>(e + 152, kPitch);
    copyf(e + 176, kZeroVec);
    store<u32>(e + 136, 0);
    copyf(e + 180, kZeroVec + 4);
    store<u32>(e + 156, 0);
    copyf(e + 184, kZeroVec + 8);
    store<u32>(e + 160, 0);
    copyf(e + 208, kZeroVec);
    ref<u8>(e + 192) = 0;
    copyf(e + 212, kZeroVec + 4);
    ref<u8>(e + 14) = 0;
    copyf(e + 216, kZeroVec + 8);
    ref<u8>(e + 172) = 0;
    store<u32>(e + 164, 0);
}

// bool KeyMap_Set(u32 handle, u32 key, s32 priority)
// Makes `key` map to the live emitter `handle` unless the emitter the key
// holds now is live and has at least `priority`; that previous emitter is
// stopped first. Returns whether the key was taken.
DECOMP(0044385C, KeyMap_Set) {
    const u32 handle = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 priority = arg<u32>(ctx, 2);
    if (Emitter_FromHandle(handle) == 0) return ret(ctx, 0u);

    Stack frame(ctx, 16);
    const u32 key1 = frame.addr(), key2 = key1 + 4, pair = key1 + 8;
    store<u32>(key1, key);
    const s32 i = static_cast<s32>(call(ctx, kKeyMapFind, {kKeyMap, key1}));
    const u32 first = i < 0 ? kKeyMap + 16 : load<u32>(kKeyMap + 8) + static_cast<u32>(i) * 12 + 4;
    const u32 k = load<u32>(key1);
    const u32 previous = load<u32>(first);
    store<u32>(key2, k);
    const s32 j = static_cast<s32>(call(ctx, kKeyMapFind, {kKeyMap, key2}));
    const u32 entry = j < 0 ? kKeyMap + 16 : load<u32>(kKeyMap + 8) + static_cast<u32>(j) * 12 + 4;
    if (Emitter_FromHandle(load<u32>(entry)) != 0 &&
        !(static_cast<s32>(priority) > load<s32>(entry + 4)))
        return ret(ctx, 0u);

    call(ctx, afn::Emitter_Stop, {previous, 0});
    store<u32>(pair, handle);
    store<u32>(pair + 4, priority);
    call(ctx, mfn::KeyMap_Insert, {kKeyMap, key1, load<u64>(pair)});  // {handle, priority} in one register
    ret(ctx, 1u);
}

// void KeyMap_Insert(Map* map, const u32* key, u64 value)  (name?)
// value = {handle (high word), priority (low word)}. Grows the map when it
// is full, sorts it if flagged, then insertion-sorts the new entry in by
// unsigned key (ignoring the map's comparator).
DECOMP(00784194, KeyMap_Insert) {
    const u32 map = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u64 value = ctx.gpr[5];
    const u32 hi = static_cast<u32>(value >> 32), lo = static_cast<u32>(value);
    u32 count = load<u16>(map + 2);
    const u32 cap = load<u16>(map);
    if (count == cap) {
        call(ctx, mfn::Array_Reserve, {map, (cap << 1) & 0xFFFE});
        count = load<u16>(map + 2);
    }
    const u16 flags = load<u16>(map + 6);
    if (flags & 1) {
        store<u16>(map + 6, flags & 0xFFFE);
        if (static_cast<s32>(count) > 0) {
            call(ctx, mfn::qsort, {load<u32>(map + 8), count, 12, load<u32>(map + 24)});
            count = load<u16>(map + 2);
        }
    }
    const u32 entries = load<u32>(map + 8);
    s32 pos = static_cast<s32>(count);
    while (pos > 0) {
        const u32 prev = entries + static_cast<u32>(pos - 1) * 12;
        if (!(load<u32>(key) < load<u32>(prev))) break;
        const u32 cur = entries + static_cast<u32>(pos) * 12;
        store<u32>(cur, load<u32>(prev));
        store<u32>(cur + 4, load<u32>(prev + 4));
        store<u32>(cur + 8, load<u32>(prev + 8));
        --pos;
    }
    const u32 slot = entries + static_cast<u32>(pos) * 12;
    store<u32>(slot, load<u32>(key));
    store<u32>(slot + 4, hi);
    store<u32>(slot + 8, lo);
    const u32 n = (static_cast<u32>(load<u16>(map + 2)) + 1) & 0xFFFF;
    store<u16>(map + 2, static_cast<u16>(n));
    const u32 high = load<u16>(map + 4);
    store<u16>(map + 4, static_cast<u16>(static_cast<s32>(high) > static_cast<s32>(n) ? high : n));
}

// void HandlePool_Grow(Pool* pool, s32 capacity)  (name?)
// Reallocates the nodes (272 bytes each, constructed in place), copies the
// old ones over and appends the new nodes to the free list. As in the
// original, node [old capacity] gets no free flag or serial reset.
DECOMP(007839B8, HandlePool_Grow) {
    const u32 pool = arg<u32>(ctx, 0);
    const s32 n = arg<s32>(ctx, 1);
    if (n == load<s32>(pool + 24)) return;

    const u32 mem = static_cast<u32>(call(ctx, mfn::Mem_Alloc, {static_cast<u32>(n) * kNodeSize, 0, 16}));
    if (mem != 0) {
        for (s32 i = 0; i < n; ++i) {
            const u32 node = Node(mem, static_cast<u32>(i));
            Block_Init(node + 32);
            Block_Init(node + 92);
            call(ctx, mfn::Emitter_Init, {node + kNodeData});
        }
    }
    const u32 old_cap = load<u32>(pool + 24);
    call(ctx, mfn::memmove, {mem, load<u32>(pool + 28), old_cap * kNodeSize});
    call(ctx, mfn::Mem_FreeFrom, {load<u32>(pool + 28), 0});
    store<u32>(pool + 28, mem);

    const u32 cap = load<u32>(pool + 24);
    const u32 tail = load<u32>(pool + 4);
    store<u32>(Node(mem, cap), tail);
    u32 nfree = load<u32>(pool + 20);
    u32 first = load<u32>(pool + 24);
    u32 nodes = load<u32>(pool + 28);
    if (nfree != 0) store<u32>(Node(nodes, tail) + 4, cap);
    store<u32>(Node(nodes, first) + 4, first + 1);
    if (static_cast<s32>(first + 1) < n) {
        for (u32 i = first + 1; static_cast<s32>(i) < n; ++i) {
            const u32 node = Node(load<u32>(pool + 28), i);
            store<u32>(node, i - 1);
            store<u32>(node + 4, i + 1);
            ref<u8>(node + kNodeSerial) = ref<u8>(node + kNodeSerial) | 0x80;
            const u32 w = Node(load<u32>(pool + 28), i) + kNodeSerial;
            store<u32>(w, load<u32>(w) & ~0x7FFF8000u);
        }
        nfree = load<u32>(pool + 20);
        first = load<u32>(pool + 24);
        nodes = load<u32>(pool + 28);
    }
    store<s32>(Node(nodes, static_cast<u32>(n) - 1) + 4, -1);
    if (nfree == 0) store<u32>(pool, first);
    store<u32>(pool + 4, static_cast<u32>(n) - 1);
    store<u32>(pool + 20, nfree + (static_cast<u32>(n) - first));
    store<s32>(pool + 24, n);
}
