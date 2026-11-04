// World_HandleEvent callees, part 10d: layer entry release, the handle
// list erase and the per-player bit count.
#include "hevent10.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent10;

namespace sly4 {
namespace hevent10d {
namespace dfn10 {
constexpr u32 Map_Find76642C = 0x0076642C;  // (map, const u32* key) -> value* (name?)
}  // namespace dfn10

// Layer stack entry (16 bytes, as hevent9_f.cpp): +4 Object*, +12 flags:
//   bit 31 restart, bit 30 show argument (cleared here), bit 29 skip hide,
//   bit 28 hide with 0 (else with the show bit), bit 27 / 25 -> owner +40 bits
//   12 / 10, bit 26 argument of vtable +96.
// Object: +164 owner (+40 u16 flags); vtable +176 (obj, u8), +180 (obj, 0),
// +96 (obj, u8), +40 (obj, u8).
constexpr u32 kEntObj = 4, kEntFlags = 12;
constexpr u32 kEfRestart = 0x80000000u, kEfShow = 0x40000000u, kEfNoHide = 0x20000000u;
constexpr u32 kEfHide0 = 0x10000000u, kEfOwner12 = 0x08000000u, kEfArg96 = 0x04000000u;
constexpr u32 kEfOwner10 = 0x02000000u;
constexpr u32 kObjOwner = 164, kOwnerFlags = 40;
constexpr u32 kV176 = 176, kV180 = 180, kV96 = 96, kV40 = 40;

// Handle list (as Seq_Update2EED60): +0 free head, +4 free tail, +8 used
// head, +12 used tail, +16 u32 used count, +20 u32 free count, +24 s32
// capacity, +28 16-byte nodes {+0 prev, +4 next, +12 u32: bit 31 free,
// bits 15..30 serial}. Handles: index << 16 | serial.
constexpr u32 kHlFreeHead = 0, kHlFreeTail = 4, kHlUsedHead = 8, kHlUsedTail = 12;
constexpr u32 kHlUsed = 16, kHlFree = 20, kHlCap = 24, kHlNodes = 28;
constexpr u32 kNodePrev = 0, kNodeNext = 4, kNodeWord = 12, kNodeSize = 16;

// Player (860 bytes in the session's array): +724 u32 id, +824 u8 index
// (255: none). Save slot +100: u16 bit set per player index.
constexpr u32 kSessionPtr = 0x00F64C20;  // u32 Session*: +20 Player[] (+24 u32 count)
constexpr u32 kSesPlayers = 20, kSesPlayerCount = 24, kPlayerSize = 860, kPlayerId = 724, kPlayerIndex = 824;
constexpr u32 kSlotBits = 100;
// Record map: +2 u16 count, +8 8-byte entries {+4 Rec*}. Rec: +0 u8 owner
// player index, +4 key, +8 u32, +12 u32 player id. Map value: +1 u8 bit.
constexpr u32 kRecMap = 0x0106819C;
}  // namespace hevent10d
}  // namespace sly4

using namespace sly4::hevent10d;

// void Entry_Release42AEE8(Entry16* e) (name?): hides the entry's object
// (vtable +176 with the show bit 30 when bit 28 is clear, else with 0;
// nothing with bit 29), clears bit 30, copies bits 27 / 25 into the owner's
// +40 bits 12 / 10 (when it has one), then vtable +180 (0), +96 (bit 26),
// +40 (0) and, with bit 31, +40 (1); the entry's object is cleared.
// Returns 0.
DECOMP(0042AEE8, Entry_Release42AEE8) {
    const u32 e = arg<u32>(ctx, 0);
    u32 o = load<u32>(e + kEntObj);
    if (o == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 f0 = load<u32>(e + kEntFlags);
    if ((f0 & kEfNoHide) == 0) {
        if ((f0 & kEfHide0) != 0) vcall(ctx, o, kV176, {o, 0u});
        else vcall(ctx, o, kV176, {o, (f0 & kEfShow) != 0 ? 1u : 0u});
    }
    const u32 f = load<u32>(e + kEntFlags) & ~kEfShow;
    o = load<u32>(e + kEntObj);
    store<u32>(e + kEntFlags, f);
    u32 owner = load<u32>(o + kObjOwner);
    if (owner != 0) {
        const u16 h = load<u16>(owner + kOwnerFlags);
        store<u16>(owner + kOwnerFlags, static_cast<u16>((h & ~0x1000) | ((f & kEfOwner12) != 0 ? 0x1000 : 0)));
        const u32 f2 = load<u32>(e + kEntFlags);
        owner = load<u32>(load<u32>(e + kEntObj) + kObjOwner);
        const u16 h2 = load<u16>(owner + kOwnerFlags);
        store<u16>(owner + kOwnerFlags, static_cast<u16>((h2 & ~0x400) | ((f2 & kEfOwner10) != 0 ? 0x400 : 0)));
        o = load<u32>(e + kEntObj);
    }
    vcall(ctx, o, kV180, {o, 0u});
    o = load<u32>(e + kEntObj);
    vcall(ctx, o, kV96, {o, (load<u32>(e + kEntFlags) & kEfArg96) != 0 ? 1u : 0u});
    o = load<u32>(e + kEntObj);
    vcall(ctx, o, kV40, {o, 0u});
    if ((load<u32>(e + kEntFlags) & kEfRestart) != 0) {
        o = load<u32>(e + kEntObj);
        vcall(ctx, o, kV40, {o, 1u});
    }
    store<u32>(e + kEntObj, 0);
    ret(ctx, 0u);
}

namespace {
// Marks a node free: the byte store of the old top byte | 0x80, then the
// word with the serial bits cleared.
void node_free(u32 a) {
    const u8 b = static_cast<u8>(load<u8>(a) | 0x80);
    const u32 w = (((load<u32>(a) & 0x00FFFFFFu) | (static_cast<u32>(b) << 24)) & ~0x7FFF8000u);
    store<u8>(a, b);
    store<u32>(a, w);
}
}  // namespace

// void Seq_Next767618(u32* out, HandleList* l, u32 handle) (name?): erases
// a live handle (serial matches, not free, index below capacity) from the
// used list and appends its node to the free list; *out = the handle of the
// next used node (0xFFFF0000 after the tail), or 0 when nothing was erased.
// An emptied used list gets head = tail = -1.
DECOMP(00767618, Seq_Next767618) {
    const u32 out = arg<u32>(ctx, 0);
    const u32 l = arg<u32>(ctx, 1);
    const u32 h = arg<u32>(ctx, 2);
    const u32 used = load<u32>(l + kHlUsed);
    const u32 free_tail = load<u32>(l + kHlFreeTail);
    const u32 used_tail = load<u32>(l + kHlUsedTail);
    u32 res = 0;
    const u32 serial = h & 0xFFFF, idx = h >> 16;
    if (used != 0 && serial != 0) {
        const s32 cap = static_cast<s32>(load<u32>(l + kHlCap));
        if (static_cast<s32>(idx) < cap) {
            const u32 base = load<u32>(l + kHlNodes);
            const u32 off = idx * kNodeSize;
            const u32 node = base + off;
            if (((load<u32>(node + kNodeWord) >> 15) & 0xFFFF) == serial && (load<u8>(node + kNodeWord) & 0x80) == 0) {
                if (idx == load<u32>(l + kHlUsedHead))
                    store<u32>(l + kHlUsedHead, load<u32>(node + kNodeNext));
                else
                    store<u32>(base + (load<u32>(node + kNodePrev) << 4) + kNodeNext, load<u32>(node + kNodeNext));
                if (idx == used_tail) {
                    store<u32>(l + kHlUsedTail, load<u32>(node + kNodePrev));
                    res = 0xFFFF0000u;
                } else {
                    store<u32>(base + (load<u32>(node + kNodeNext) << 4) + kNodePrev, load<u32>(node + kNodePrev));
                    const u32 nx = load<u32>(node + kNodeNext);
                    const u32 w = load<u32>(base + ((nx << 4) & 0x000FFFF0u) + kNodeWord);
                    res = ((nx << 16) & 0xFFFF0000u) | ((w >> 15) & 0xFFFF);
                }
                if (free_tail != 0xFFFFFFFFu) {
                    store<u32>(base + (free_tail << 4) + kNodeNext, idx);
                    store<u32>(base + off + kNodePrev, free_tail);
                    const u32 n2 = load<u32>(l + kHlNodes) + off;
                    store<u32>(n2 + kNodeNext, 0xFFFFFFFFu);
                    node_free(n2 + kNodeWord);
                    store<u32>(l + kHlFreeTail, idx);
                    if (cap == 1) store<u32>(l + kHlFreeHead, idx);
                } else {
                    store<u32>(base + off + kNodePrev, 0xFFFFFFFFu);
                    store<u32>(node + kNodeNext, 0xFFFFFFFFu);
                    node_free(load<u32>(l + kHlNodes) + off + kNodeWord);
                    store<u32>(l + kHlFreeTail, idx);
                    store<u32>(l + kHlFreeHead, idx);
                }
                const u32 left = used - 1;
                const u32 nfree = load<u32>(l + kHlFree);
                store<u32>(l + kHlUsed, left);
                store<u32>(l + kHlFree, nfree + 1);
                if (left == 0) {
                    store<u32>(l + kHlUsedTail, 0xFFFFFFFFu);
                    store<u32>(l + kHlUsedHead, 0xFFFFFFFFu);
                }
            }
        }
    }
    store<u32>(out, res);
    ctx.gpr[3] = out;
}

namespace {
// Index of the session player with id, or -1.
s32 player_index(u32 id) {
    const u32 s = load<u32>(kSessionPtr);
    const u32 n = load<u32>(s + kSesPlayerCount);
    for (u32 i = 0; i < n; ++i)
        if (load<u32>(load<u32>(s + kSesPlayers) + i * kPlayerSize + kPlayerId) == id) return static_cast<s32>(i);
    return -1;
}
}  // namespace

// u32 Player_Count2DAC24(Player* p, SaveSlot* slot) (name?): the number of
// bits set in the slot's word for p's index, plus one for every record p
// owns (+0 == p's session index; +8 and +12 non-zero) whose target player
// (+12) has the record's bit (map value +1) set in its own word. 0 for a
// player without an index.
DECOMP(002DAC24, Player_Count2DAC24) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 slot = arg<u32>(ctx, 1);
    const u32 k = load<u8>(p + kPlayerIndex);
    if (k == 0xFF) {
        ret(ctx, 0u);
        return;
    }
    u32 count = 0;
    for (u32 b = load<u16>(slot + kSlotBits + k * 2); b != 0; b &= b - 1) ++count;
    const s32 me = player_index(load<u32>(p + kPlayerId));
    for (s32 i = 0; i < static_cast<s32>(load<u16>(kRecMap + 2)); ++i) {
        const u32 rec = load<u32>(load<u32>(kRecMap + 8) + static_cast<u32>(i) * 8 + 4);
        if (static_cast<s32>(load<u8>(rec)) != me || load<u32>(rec + 8) == 0) continue;
        const u32 id = load<u32>(rec + 12);
        if (id == 0) continue;
        const u32 s = load<u32>(kSessionPtr);
        const u32 n = load<u32>(s + kSesPlayerCount);
        u32 target = 0;
        for (u32 j = 0; j < n; ++j) {
            const u32 players = load<u32>(s + kSesPlayers);
            if (load<u32>(players + j * kPlayerSize + kPlayerId) == id) {
                target = players + j * kPlayerSize;
                break;
            }
        }
        if (target == 0) continue;
        const u32 v = load<u32>(static_cast<u32>(call(ctx, dfn10::Map_Find76642C, {kRecMap, rec + 4})));
        if (v == 0) continue;
        const u32 sh = load<u8>(v + 1);
        const u32 bit = (sh & 0x20) != 0 ? 0 : 1u << (sh & 0x1F);
        if ((load<u16>(slot + kSlotBits + load<u8>(target + kPlayerIndex) * 2) & bit) != 0) ++count;
    }
    ret(ctx, count);
}
