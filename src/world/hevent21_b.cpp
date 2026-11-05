// World_HandleEvent callees, part 21b: the level set restart walk, the track
// pool rebuild and the coach object refresh.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kTrackRelease38DC0C = 0x0038DC0C;  // (Object*) (name?)
constexpr u32 kCoachModeHash = 0x8B3BD7A1;       // hash given to the group objects' vt +128 (name?)
}  // namespace

// u32 Set_Restart4BE420(Set* s) (name?): walks the set's object map (+16,
// iterator path +20, depth +100, current +104; the engine's inlined tree
// iterator, as hevent6.h) in order and calls each node's object (+8) vt +12.
// Returns 0.
DECOMP(004BE420, Set_Restart4BE420) {
    const u32 m = arg<u32>(ctx, 0) + hevent6::kSetDefs;
    u32 cur = hevent6::map_first(m);
    while (cur != 0) {
        const u32 obj = load<u32>(cur + hevent6::kNodeValue);
        vcall(ctx, obj, 12, {obj});
        cur = load<u32>(m + hevent6::kMapNode);
        if (cur == 0) break;
        cur = hevent6::map_next(m, cur);
    }
    ret(ctx, 0);
}

// s32 TrackPool_Update390724(Pool* p) (name?): the pool of 16-byte entries
// (+28: +0 prev, +4 next, +8 Object*, +12 bit 31 free / bits 15..30
// serial; +24 s32 capacity) releases its live objects with 0x0038DC0C,
// following the live list from the head (+8, tail +12) for +16 (u16)
// steps; past the tail the handle is 0, so entry 0 is released again, as in
// the original. Then every entry is relinked into the free list (prev k-1,
// next k+1, freed, serial cleared), the ends get -1 (the last-entry store
// lands 12 bytes before the table when the capacity is 0), head / tail
// = -1, +0 = 0, +4 = capacity - 1 (-1 for capacity <= 1), +16 = 0, +20 =
// capacity, u16 +38 = 0. Returns the capacity.
DECOMP(00390724, TrackPool_Update390724) {
    const u32 p = arg<u32>(ctx, 0);
    u32 live = load<u32>(p + 16);
    Stack frame(ctx, 16);
    const u32 cur = frame.addr();     // u32 handle being released (112)
    const u32 first = cur + 4;        // {u16 index, u16 serial} of the head (116)
    if (live != 0) {
        const u32 idx = load<u32>(p + 8);
        const u32 table = load<u32>(p + 28);
        store<u32>(first, 0);
        store<u16>(first, static_cast<u16>(idx));
        store<u16>(first + 2, static_cast<u16>(load<u32>(table + (idx << 4) + 12) >> 15));
    } else {
        store<u32>(first, 0);
        store<u16>(first + 2, 0);
    }
    live &= 0xFFFF;
    store<u32>(cur, load<u32>(first));
    if (static_cast<s32>(live) > 0) {
        for (u32 k = 0;;) {
            const u32 idx = load<u16>(cur);
            call(ctx, kTrackRelease38DC0C, {load<u32>(load<u32>(p + 28) + (idx << 4) + 8)});
            u32 h = 0;
            if (static_cast<s32>(idx) != static_cast<s32>(load<u32>(p + 12))) {
                const u32 table = load<u32>(p + 28);
                const u32 next = load<u32>(table + ((idx << 4) & 0x000FFFF0u) + 4);
                (void)load<u32>(table + (idx << 4) + 4);  // the same word is read twice
                h = (next << 16) | ((load<u32>(table + (next << 4) + 12) >> 15) & 0xFFFF);
            }
            k = (k + 1) & 0xFFFF;
            store<u32>(cur, h);
            if (!(static_cast<s32>(k) < static_cast<s32>(live))) break;
        }
    }
    u32 count = load<u32>(p + 24);
    if (static_cast<s32>(count) > 0) {
        u32 k = 0;
        do {
            const u32 e = load<u32>(p + 28) + 16 * k;
            store<u32>(e, k - 1);
            store<u32>(e + 4, k + 1);
            store<u8>(e + 12, load<u8>(e + 12) | 0x80);
            const u32 w = load<u32>(p + 28) + 16 * k + 12;
            store<u32>(w, load<u32>(w) & ~0x7FFF8000u);
            ++k;
            count = load<u32>(p + 24);
        } while (static_cast<s32>(k) < static_cast<s32>(count));
    }
    const u32 table = load<u32>(p + 28);
    store<u32>(table, 0xFFFFFFFFu);
    store<u32>(table + (count << 4) - 12, 0xFFFFFFFFu);
    store<u32>(p + 8, 0xFFFFFFFFu);
    store<u32>(p + 12, 0xFFFFFFFFu);
    store<u32>(p + 0, 0);
    store<u32>(p + 4, static_cast<s32>(count) > 1 ? count - 1 : 0xFFFFFFFFu);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, count);
    store<u16>(p + 38, 0);
    ret(ctx, count);
}

// u32 Coach_Update25A690(Coach* c) (name?): the effect +2268 gets vt +40
// (e, 0, 1) and the animator +2176 vt +88 (a, 0, 1, 1, 1). Then for each
// group of the +64 table (u16 count +72): its object +52 gets vt +88 (x,
// 0, 0, 1, 1), vt +284 (x, 0, 0) and vt +128 (x, 0x8B3BD7A1, 1), and each
// member of its 8-byte table (+0, u16 count +8) vt +40 (m, 0, 1). Returns
// the group count.
DECOMP(0025A690, Coach_Update25A690) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 e = load<u32>(c + 2268);
    if (e != 0) vcall(ctx, e, 40, {e, 0, 1});
    const u32 a = load<u32>(c + 2176);
    if (a != 0) vcall(ctx, a, 88, {a, 0, 1, 1, 1});
    u32 i = 0, n;
    while (i < (n = load<u16>(c + 72))) {
        const u32 g = load<u32>(load<u32>(c + 64) + 4 * i);
        if (g != 0) {
            u32 x = load<u32>(g + 52);
            if (x != 0) {
                vcall(ctx, x, 88, {x, 0, 0, 1, 1});
                x = load<u32>(g + 52);
                vcall(ctx, x, 284, {x, 0, 0});
                x = load<u32>(g + 52);
                vcall(ctx, x, 128, {x, kCoachModeHash, 1});
            }
            for (u32 j = 0; j < load<u16>(g + 8); ++j) {
                const u32 m = load<u32>(load<u32>(g) + 8 * j);
                if (m != 0) vcall(ctx, m, 40, {m, 0, 1});
            }
        }
        ++i;
    }
    ret(ctx, n);
}
