// Emitter construction, the emitter key map and growing the emitter pool.
#include "audio.h"
#include "more.h"

namespace sly4::audio {
using namespace opentit::native;

namespace {

constexpr u32 kKeyMap = 0x010AFEF8;   // KeyMap: +0 u16 capacity, +2 u16 count, +4 u16 high water,
                                      // +6 u16 flags (bit 0 = unsorted), +8 entries (12 bytes:
                                      // key, handle, priority), +16 default entry, +24 compare OPD
constexpr u32 kZeroVec = 0x01164B40;  // f32[3] (name?)
constexpr u32 kOne = 0x3F800000u, kPitch = 0x46BB7E00u;  // 1.0f, 23999.0f

struct KeyEntry {
    be_u32 key;       // +0
    be_u32 handle;    // +4
    be_s32 priority;  // +8
};
struct KeyMapFull {
    be_u16 capacity;          // +0
    be_u16 count;             // +2
    be_u16 high;              // +4
    be_u16 flags;             // +6
    be_u32 entries;           // +8 KeyEntry[]
};

// lfs + stfs: the copy goes through double, as the recompiled code does.
void copyf(be_f32& dst, const be_f32& src) { dst = static_cast<f32>(static_cast<f64>(src.get())); }
f32 bits(u32 b) { return std::bit_cast<f32>(b); }

// The 57-byte 3D block's initial value: six 1.0s, three 23999.0s, then 0.
void Block_Init(Block& b) {
    for (be_f32* f : {&b.volume, &b.gain, &b.scale, &b.fade, &b.vol1, &b.vol2}) *f = bits(kOne);
    b.pitch = bits(kPitch);
    b.pitch_target = bits(kPitch);
    b.pitch_cap = bits(kPitch);
    b.pitch_code = 0u;
    b.distance = 0.0f;
    b.pan = 0u;
    b.elevation = 0;
    b.angle = 0u;
    b.fixed_angle = 0;
}

// The value slot of `key`'s entry (or the default entry +16+4... as the
// original: entries + i * 12 + 4, else map + 16).
u32 KeyMap_Slot(s32 i) {
    return i < 0 ? kKeyMap + 16 : global<be_u32>(kKeyMap + 8) + static_cast<u32>(i) * 12 + 4;
}

}  // namespace

// void Emitter_Init(Emitter* e)
// No voice (-1), both copies of the 3D block at their defaults, fade and
// pitch at rest, position and velocity zero, no key, bucket 0.
void Emitter_Init(Emitter* ep) {
    Emitter& e = gref(ep);
    e.voice = 0xFFFFFFFFu;
    e.def = nullptr;
    e.zone = 0u;
    e.bucketed = 0;
    e.is_virtual = 0;
    GuestFrame frame(64);
    Block* block = frame.as<Block>();
    Block_Init(*block);
    call(mfn::memmove, &e.base, block, static_cast<u32>(sizeof(Block)));
    Block_Init(*block);
    call(mfn::memmove, &e.cur, block, static_cast<u32>(sizeof(Block)));
    const be_f32* zero = host<be_f32>(kZeroVec);
    e.fade_cur = bits(kOne);
    e.fade_target = bits(kOne);
    e.pitch_cur = bits(kPitch);
    e.pitch_target = bits(kPitch);
    copyf(e.position[0], zero[0]);
    e.shape = 0u;
    copyf(e.position[1], zero[1]);
    e.key = 0u;
    copyf(e.position[2], zero[2]);
    e.priority = 0;
    copyf(e.velocity[0], zero[0]);
    e.forced = 0;
    copyf(e.velocity[1], zero[1]);
    e.doppler = 0;
    copyf(e.velocity[2], zero[2]);
    e.no_key = 0;
    e.bucket = 0u;
}

// bool KeyMap_Set(u32 handle, u32 key, s32 priority)
// Makes `key` map to the live emitter `handle` unless the emitter the key
// holds now is live and has at least `priority`; that previous emitter is
// stopped first. Returns whether the key was taken.
bool KeyMap_Set(u32 handle, u32 key, s32 priority) {
    if (!Emitter_Of(handle)) return false;

    GuestFrame frame(16);
    be_u32* key1 = frame.as<be_u32>();
    be_u32* key2 = frame.as<be_u32>(4);
    *key1 = key;
    KeyMap* map = host<KeyMap>(kKeyMap);
    const u32 first = KeyMap_Slot(KeyMap_Find(map, key1));
    const u32 k = *key1;
    const u32 previous = gref<be_u32>(first);
    *key2 = k;
    const u32 entry = KeyMap_Slot(KeyMap_Find(map, key2));
    if (Emitter_Of(gref<be_u32>(entry)) && !(priority > gref<be_s32>(entry + 4))) return false;

    Emitter_Stop(previous, 0);
    KeyMap_Insert(map, key1, (static_cast<u64>(handle) << 32) | static_cast<u32>(priority));
    return true;
}

// void KeyMap_Insert(Map* map, const u32* key, u64 value)  (name?)
// value = {handle (high word), priority (low word)}. Grows the map when it
// is full, sorts it if flagged, then insertion-sorts the new entry in by
// unsigned key (ignoring the map's comparator).
void KeyMap_Insert(KeyMap* mp, const be_u32* key, u64 value) {
    KeyMapFull& map = gref(reinterpret_cast<KeyMapFull*>(mp));
    const u32 hi = static_cast<u32>(value >> 32), lo = static_cast<u32>(value);
    u32 count = map.count;
    const u32 cap = map.capacity;
    if (count == cap) {
        Array_Reserve(reinterpret_cast<Array*>(&map), static_cast<u16>((cap << 1) & 0xFFFE));
        count = map.count;
    }
    const u16 flags = map.flags;
    if (flags & 1) {
        map.flags = static_cast<u16>(flags & 0xFFFE);
        if (static_cast<s32>(count) > 0) {
            call(mfn::qsort, map.entries.get(), count, 12u, gref<be_u32>(guest(&map) + 24).get());
            count = map.count;
        }
    }
    const u32 entries = map.entries;
    auto at = [&](s32 i) -> KeyEntry& { return gref<KeyEntry>(entries + static_cast<u32>(i) * 12); };
    s32 pos = static_cast<s32>(count);
    while (pos > 0) {
        KeyEntry& prev = at(pos - 1);
        if (!(gref(key) < prev.key)) break;
        KeyEntry& cur = at(pos);
        cur.key = prev.key.get();
        cur.handle = prev.handle.get();
        cur.priority = prev.priority.get();
        --pos;
    }
    KeyEntry& slot = at(pos);
    slot.key = gref(key).get();
    slot.handle = hi;
    slot.priority = static_cast<s32>(lo);
    const u32 n = (static_cast<u32>(map.count) + 1) & 0xFFFF;
    map.count = static_cast<u16>(n);
    const u32 high = map.high;
    map.high = static_cast<u16>(static_cast<s32>(high) > static_cast<s32>(n) ? high : n);
}

// void HandlePool_Grow(Pool* pool, s32 capacity)  (name?)
// Reallocates the nodes (272 bytes each, constructed in place), copies the
// old ones over and appends the new nodes to the free list. As in the
// original, node [old capacity] gets no free flag or serial reset.
void HandlePool_Grow(EmitterPool* pp, s32 n) {
    EmitterPool& pool = gref(pp);
    if (n == pool.capacity) return;

    const u32 mem = call<u32>(mfn::Mem_Alloc, static_cast<u32>(n) * kNodeSize, 0u, 16u);
    auto node = [](u32 nodes, u32 i) -> EmitterNode& { return gref<EmitterNode>(nodes + i * kNodeSize); };
    if (mem != 0) {
        for (s32 i = 0; i < n; ++i) {
            EmitterNode& nd = node(mem, static_cast<u32>(i));
            Block_Init(gref<Block>(guest(&nd) + 32));
            Block_Init(gref<Block>(guest(&nd) + 92));
            Emitter_Init(&nd.emitter);
        }
    }
    const u32 old_cap = static_cast<u32>(pool.capacity.get());
    call(mfn::memmove, mem, pool.nodes.get(), old_cap * kNodeSize);
    call(mfn::Mem_FreeFrom, pool.nodes.get(), 0u);
    pool.nodes = mem;

    const u32 cap = static_cast<u32>(pool.capacity.get());
    const u32 tail = static_cast<u32>(pool.free_tail.get());
    node(mem, cap).prev = static_cast<s32>(tail);
    u32 nfree = pool.free_count;
    u32 first = static_cast<u32>(pool.capacity.get());
    u32 nodes = pool.nodes;
    if (nfree != 0) node(nodes, tail).next = static_cast<s32>(cap);
    node(nodes, first).next = static_cast<s32>(first + 1);
    if (static_cast<s32>(first + 1) < n) {
        for (u32 i = first + 1; static_cast<s32>(i) < n; ++i) {
            EmitterNode& nd = node(pool.nodes, i);
            nd.prev = static_cast<s32>(i - 1);
            nd.next = static_cast<s32>(i + 1);
            u8& top = gref<u8>(guest(&nd.serial));
            top = top | 0x80;
            nd.serial = nd.serial & ~0x7FFF8000u;
        }
        nfree = pool.free_count;
        first = static_cast<u32>(pool.capacity.get());
        nodes = pool.nodes;
    }
    node(nodes, static_cast<u32>(n) - 1).next = -1;
    if (nfree == 0) pool.free_head = first;
    pool.free_tail = n - 1;
    pool.free_count = nfree + (static_cast<u32>(n) - first);
    pool.capacity = n;
}

}  // namespace sly4::audio

NATIVE(00440B48, sly4::audio::Emitter_Init)
NATIVE(0044385C, sly4::audio::KeyMap_Set)
NATIVE(00784194, sly4::audio::KeyMap_Insert)
NATIVE(007839B8, sly4::audio::HandlePool_Grow)
