// World_HandleEvent callees, part 9g: the session refresh.
#include "hevent9.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent9;

namespace sly4 {
namespace hevent9g {
namespace gfn {
constexpr u32 UserBlock_Find465F30 = 0x00465F30;  // (UserBlock*, u32 id) -> Entry* (name?)
constexpr u32 Session_Release2D9A14 = 0x002D9A14; // (u32) (as hevent3_i.cpp sub_002D9A14)
constexpr u32 Session_Clear2DDAB0 = 0x002DDAB0;   // (Session*) (as hevent3_i.cpp sub_002DDAB0)
constexpr u32 Player_Count2DAC24 = 0x002DAC24;    // (Player*, SaveSlot*) -> u32 (name?)
constexpr u32 World_StartLevel = 0x004674C4;      // (World*, u32 level, u8 flag)
constexpr u32 String_Format = 0x005534D0;         // (fmt, ...)
}  // namespace gfn

// Session: +0..+16 cleared on reset (+8 handle, +16 flags: bit 29 reset),
// +20 Player[] (860 bytes each), +24 u32 count, +36 Player* main, +40 u32,
// +56 / +60 u32 counts, +64..+92 cleared, +72 Entry*, +96 u32 entry id,
// +100 flags (bits 28, 29 cleared).
constexpr u32 kSesFlags = 16, kSesPlayers = 20, kSesCount = 24, kSesMain = 36, kSes40 = 40;
constexpr u32 kSesEpisodeBits = 56, kSesPlayerSum = 60, kSesEntry = 72, kSesEntryId = 96, kSesFlags2 = 100;
constexpr u32 kSesReset = 0x20000000u;
// Player: +748 u32 session ids[] (count u8 +820), +823 u8 episode (255:
// none), +826 u8 (skip), +828 u8 (counted).
constexpr u32 kPlayerSize = 860, kPlayerIds = 748, kPlayerIdCount = 820, kPlayerEpisode = 823;
constexpr u32 kPlayerSkip = 826, kPlayerCounted = 828;
constexpr u32 kEpisodes = 12;
constexpr u32 kSlotEpisodeBits = 132;  // SaveSlot u32[12]: per-episode bits
constexpr u32 kWorldSessionId = 0x128114;
constexpr u32 kEntryKind = 18, kEntryKindSkip = 5;  // user entry u8 kind
constexpr u32 kVGetId = 80;  // Entry vtable slot
constexpr u32 kFmtEpisode = 0x00B1D308;  // "Episode Index is out of bounds '%i'! ..."
}  // namespace hevent9g
}  // namespace sly4

using namespace sly4::hevent9g;

namespace {
// The current world's save slot (World +80 + slot * 4056, slot = u8 +4208).
inline u32 save_slot(u32 w) { return w + static_cast<u32>(load<u8>(w + kUserBlocks)) * kSaveSlotSize; }
}  // namespace

// void Session_Refresh2DDDCC(Session* s) (name?): the main player (+36, or
// the first non-skipped player listing the world's session id) decides
// +56 (the popcount of its episode's bits in the save slot; 0 without an
// episode, out of range: logged) and +60 (the sum of 0x002DAC24 over the
// counted players); without one both are 0. Then the entry (by id +96 from
// user block 0, or with +40 the first user entry not of kind 5) goes to
// +72, the state words are cleared, a pending reset (+16 bit 29) is done,
// and a ready entry whose id differs from the world's session id starts
// that level. r3 as the original leaves it.
DECOMP(002DDDCC, Session_Refresh2DDDCC) {
    const u32 s = arg<u32>(ctx, 0);
    u32 m = load<u32>(s + kSesMain);
    if (m == 0) {
        // The first player not skipped (+826) whose id list has the
        // world's session id (a match at a null player counts as none).
        const u32 id = load<u32>(load<u32>(kCurrentWorld) + kWorldSessionId);
        const u32 n = load<u32>(s + kSesCount);
        u32 p = load<u32>(s + kSesPlayers);
        bool found = false;
        for (u32 i = 0; i < n && !found; ++i, p += kPlayerSize) {
            if (load<u8>(p + kPlayerSkip) != 0) continue;
            const u32 k = load<u8>(p + kPlayerIdCount);
            for (u32 j = 0; j < k && !found; ++j) found = load<u32>(p + kPlayerIds + 4 * j) == id;
            if (found) m = p;
        }
    }
    if (m != 0) {
        u32 bits = 0;
        const u32 w = load<u32>(kCurrentWorld);
        const u32 ep = load<u8>(m + kPlayerEpisode);
        const u32 slot = save_slot(w);
        if (ep != 0xFF) {
            if (ep < kEpisodes) {
                for (u32 v = load<u32>(slot + ep * 4 + kSlotEpisodeBits); v != 0; v &= v - 1) ++bits;
            } else {
                call(ctx, gfn::String_Format, {kFmtEpisode, ep, kEpisodes});
            }
        }
        store<u32>(s + kSesEpisodeBits, bits);
        u32 sum = 0;
        for (u32 j = 0, off = 0; j < load<u32>(s + kSesCount); ++j, off += kPlayerSize) {
            const u32 p = load<u32>(s + kSesPlayers) + off;
            if (load<u8>(p + kPlayerCounted) == 0) continue;
            const u32 w2 = load<u32>(kCurrentWorld);
            sum += static_cast<u32>(call(ctx, gfn::Player_Count2DAC24, {p, save_slot(w2) + kSaveData}));
        }
        store<u32>(s + kSesPlayerSum, sum);
    } else {
        store<u32>(s + kSesEpisodeBits, 0);
        store<u32>(s + kSesPlayerSum, 0);
    }

    u32 e = 0;
    const u32 eid = load<u32>(s + kSesEntryId);
    if (eid != 0) {
        e = static_cast<u32>(call(ctx, gfn::UserBlock_Find465F30, {load<u32>(kCurrentWorld) + kUserBlocks, eid}));
    } else if (load<u32>(s + kSes40) != 0) {
        const u32 blk = load<u32>(kCurrentWorld) + kUserBlocks;
        for (u32 i = 0; i < load<u16>(blk + 88); ++i) {
            const u32 x = load<u32>(load<u32>(blk + 80) + i * 4);
            if (x != 0 && load<u8>(x + kEntryKind) != kEntryKindSkip) {
                e = x;
                break;
            }
        }
    }
    u32 f2 = load<u32>(s + kSesFlags2);
    store<u32>(s + kSesEntry, e);
    f2 &= ~0x30000000u;
    store<u32>(s + 76, 0);
    store<u32>(s + 80, 0);
    store<u32>(s + 64, 0);
    store<u32>(s + 68, 0);
    const u32 fl = load<u32>(s + kSesFlags);
    store<u32>(s + 88, 0);
    store<u32>(s + 92, 0);
    store<u32>(s + 84, 0);
    store<u32>(s + kSesFlags2, f2);
    ctx.gpr[3] = 0;
    if ((fl & kSesReset) != 0) {
        const u32 h = load<u32>(s + 8);
        if (h != 0) call(ctx, gfn::Session_Release2D9A14, {h});
        for (u32 k = 0; k < 20; k += 4) store<u32>(s + k, 0);
        call(ctx, gfn::Session_Clear2DDAB0, {s});
        e = load<u32>(s + kSesEntry);
    }
    if (e == 0) return;
    if (static_cast<u32>(vcall(ctx, e, kVGetId, {e})) == 0) return;
    const u32 x = load<u32>(s + kSesEntry);
    const u32 id = static_cast<u32>(vcall(ctx, x, kVGetId, {x}));
    const u32 w = load<u32>(kCurrentWorld);
    if (id == load<u32>(w + kWorldSessionId)) return;
    const u32 x2 = load<u32>(s + kSesEntry);
    const u32 level = static_cast<u32>(vcall(ctx, x2, kVGetId, {x2}));
    call(ctx, gfn::World_StartLevel, {w, level, 0});
}
