// World_HandleEvent callees, part 28e: the handle table's release
// (HandleTable_Release79A328, used by GeomScene_Reset4F41D8).
#include "hevent28.h"

using namespace sly4;

namespace {
// HandleTable: +0 u32, +4 s32 free-list tail, +8 s32 first live, +12 s32
// last live, +16 s32 live count, +20 u32 release counter, +24 s32
// capacity, +28 Entry*. Entry (16 bytes): +0 s32 prev, +4 s32 next, +8
// data, +12 flags (bit 31 free, bits 15..30 serial). A handle is idx << 16
// | serial (serial 0 is never valid).
constexpr u32 kTabFree0 = 0, kTabFreeTail = 4, kTabFirst = 8, kTabLast = 12, kTabCount = 16, kTabReleased = 20, kTabCap = 24, kTabEntries = 28;
constexpr u32 kEntPrev = 0, kEntNext = 4, kEntFlags = 12;
constexpr u32 kNone = 0xFFFFFFFFu;

u32 entry_serial(u32 flags) { return (flags >> 15) & 0xFFFFu; }

// Marks the entry free (bit 31) and clears its serial; the flag byte is
// written first, then the whole word.
void entry_free(u32 flags_ea) {
    const u8 b = static_cast<u8>(load<u8>(flags_ea) | 0x80);
    const u32 w = ((load<u32>(flags_ea) & 0x00FFFFFFu) | (static_cast<u32>(b) << 24)) & ~0x7FFF8000u;
    store<u8>(flags_ea, b);
    store<u32>(flags_ea, w);
}
}  // namespace

// u32* HandleTable_Release79A328(u32* out, HandleTable* t, u32 handle)
// (name?): for a live handle, unlinks its entry from the live list, appends
// it to the free list, and stores the handle of the next live entry in
// *out (0xFFFF0000 when it was the last). An invalid handle (empty table,
// serial 0 or stale, index out of range, entry already free) stores 0.
// Returns out.
DECOMP(0079A328, HandleTable_Release79A328) {
    const u32 out = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1), h = arg<u32>(ctx, 2);
    const u32 count = load<u32>(t + kTabCount);
    const s32 tail = static_cast<s32>(load<u32>(t + kTabFreeTail));
    const s32 last = static_cast<s32>(load<u32>(t + kTabLast));
    u32 res = 0;
    do {
        if (count == 0) break;
        const u32 serial = h & 0xFFFFu;
        if (serial == 0) break;
        const s32 idx = static_cast<s32>((h >> 16) & 0xFFFFu);
        const s32 cap = static_cast<s32>(load<u32>(t + kTabCap));
        if (!(idx < cap)) break;
        const u32 off = (h >> 12) & 0xFFFF0u;  // idx * 16
        const u32 ents = load<u32>(t + kTabEntries);
        const u32 e = ents + off;
        if (entry_serial(load<u32>(e + kEntFlags)) != serial) break;
        if ((load<u8>(e + kEntFlags) & 0x80) != 0) break;
        // Unlink from the live list.
        if (idx != static_cast<s32>(load<u32>(t + kTabFirst))) {
            const u32 prev = load<u32>(ents + off + kEntPrev);
            store<u32>(ents + (prev << 4) + kEntNext, load<u32>(e + kEntNext));
        } else {
            store<u32>(t + kTabFirst, load<u32>(e + kEntNext));
        }
        if (idx != last) {
            const u32 next = load<u32>(e + kEntNext);
            store<u32>(ents + (next << 4) + kEntPrev, load<u32>(ents + off + kEntPrev));
            const u32 n2 = load<u32>(e + kEntNext);
            res = (n2 << 16) | entry_serial(load<u32>(ents + ((n2 << 4) & 0xFFFF0u) + kEntFlags));
        } else {
            store<u32>(t + kTabLast, load<u32>(ents + off + kEntPrev));
            res = 0xFFFF0000u;
        }
        // Append to the free list.
        if (tail != -1) {
            store<u32>(ents + (static_cast<u32>(tail) << 4) + kEntNext, static_cast<u32>(idx));
            store<u32>(ents + off + kEntPrev, static_cast<u32>(tail));
            const u32 e2 = load<u32>(t + kTabEntries) + off;
            store<u32>(e2 + kEntNext, kNone);
            entry_free(e2 + kEntFlags);
            store<u32>(t + kTabFreeTail, static_cast<u32>(idx));
            if (cap == 1) store<u32>(t + kTabFree0, static_cast<u32>(idx));
        } else {
            store<u32>(ents + off + kEntPrev, kNone);
            store<u32>(e + kEntNext, kNone);
            entry_free(load<u32>(t + kTabEntries) + off + kEntFlags);
            store<u32>(t + kTabFreeTail, static_cast<u32>(idx));
            store<u32>(t + kTabFree0, static_cast<u32>(idx));
        }
        const u32 released = load<u32>(t + kTabReleased);
        store<u32>(t + kTabCount, count - 1);
        store<u32>(t + kTabReleased, released + 1);
        if (count - 1 == 0) {
            store<u32>(t + kTabLast, kNone);
            store<u32>(t + kTabFirst, kNone);
        }
    } while (false);
    store<u32>(out, res);
    ctx.gpr[3] = out;
}
