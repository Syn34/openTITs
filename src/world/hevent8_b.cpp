// World_HandleEvent callees, part 8b: AVL rotations, the sorted pair
// table lookup, the user-entry check and the event group filter.
#include "hevent8.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent8;

namespace sly4 {
namespace hevent8b {
// AVL node: +0 s16 balance (right height - left height), +2 s16 height,
// +4 key, +8 value, +12 left, +16 right.
constexpr u32 kAvlBalance = 0, kAvlHeight = 2, kAvlLeft = 12, kAvlRight = 16;

// Sorted pair table: +2 u16 count, +6 u16 flags (bit 0: unsorted), +8
// {key, value}[] entries, +16 "not found" value, +20 compare OPD.
constexpr u32 kTabCount = 2, kTabFlags = 6, kTabEntries = 8, kTabMiss = 16, kTabCompare = 20;
constexpr u32 kTabUnsorted = 1;
constexpr u32 kCompareU32b = 0x00F35FA8;  // default compare: plain unsigned keys

// User entries (World +4208): +80 Entry*[], +88 u16 count.
constexpr u32 kUserEntries = 80, kUserCount = 88;
constexpr u32 kObjUserIndex = 2456;       // s32, -1 for none
constexpr u32 kWorldSessionId = 0x00128114;  // u32 (as Ev_RefreshGroups2DFC2C)
constexpr u32 kEntryGetId = 80;           // vtable slot

constexpr u32 kGroupDef = 60, kDefSpawnerKey = 204;
constexpr u32 kTreasureInfoMap = 0x01068184;  // map of treasure spawner info by key (name?)
constexpr u32 kEvPlayers = 20, kEvMain = 36;
constexpr u32 kPlayerSize = 860, kPlayerEpisode = 825;
constexpr u32 kEpisodes = 12;
constexpr u32 kGroupApply = 24, kDefName = 12;  // vtable slots
constexpr u32 kFmtEpisode = 0x00B1D308;   // "Episode Index is out of bounds '%i'! ..."
constexpr u32 kFmtSpawner = 0x00AD8C08;   // "Couldn't find treasure spawner '%s' ..."
namespace bfn {
constexpr u32 String_Format = 0x005534D0;
constexpr u32 Map_Find7662A4 = 0x007662A4;  // (Map*, const u32* key) -> value* (name?)
}  // namespace bfn
}  // namespace hevent8b
}  // namespace sly4

using namespace sly4::hevent8b;

namespace {
// Height and balance of x from its children's heights.
void avl_update(u32 x) {
    const u32 l = load<u32>(x + kAvlLeft);
    const s32 hl = l != 0 ? static_cast<s16>(load<u16>(l + kAvlHeight)) : 0;
    const u32 r = load<u32>(x + kAvlRight);
    const s32 hr = r != 0 ? static_cast<s16>(load<u16>(r + kAvlHeight)) : 0;
    store<u16>(x + kAvlHeight, static_cast<u16>((hl > hr ? hl : hr) + 1));
    store<u16>(x + kAvlBalance, static_cast<u16>(hr - hl));
}

// The rotation shared by both directions: child = the node's `in` child
// becomes the subtree root, the node its `out` child; then the heights of
// the new root's children and the new root are redone (each reloaded from
// the slot). r3 is the new root (or the old one when it had no child).
void avl_rotate(PPUContext& ctx, u32 in, u32 out) {
    const u32 slot = arg<u32>(ctx, 1);
    const u32 n = load<u32>(slot);
    const u32 c = load<u32>(n + in);
    store<u32>(slot, c);
    if (c == 0) {
        ctx.gpr[3] = n;
        return;
    }
    store<u32>(n + in, load<u32>(c + out));
    store<u32>(load<u32>(slot) + out, n);
    const u32 a = load<u32>(load<u32>(slot) + kAvlLeft);
    if (a != 0) avl_update(a);
    const u32 b = load<u32>(load<u32>(slot) + kAvlRight);
    if (b != 0) avl_update(b);
    const u32 root = load<u32>(slot);
    if (root != 0) avl_update(root);
    ctx.gpr[3] = root;
}
}  // namespace

// Node* Avl_RotateRight785D58(AVL*, Node** slot) (name?): right rotation
// at *slot (the left child moves up).
DECOMP(00785D58, Avl_RotateRight785D58) { avl_rotate(ctx, kAvlLeft, kAvlRight); }

// Node* Avl_RotateLeft785EA0(AVL*, Node** slot) (name?): left rotation at
// *slot (the right child moves up).
DECOMP(00785EA0, Avl_RotateLeft785EA0) { avl_rotate(ctx, kAvlRight, kAvlLeft); }

// void* Map_Find7665B4(Table* t, const u32* key) (name?): as
// Table_Find74493C with the default compare 0x00F35FA8: sorts first when
// flagged, then a binary search (unsigned keys, or the compare OPD on
// (entry, &key copy)). Returns &entry.value, or &t->miss (+16).
DECOMP(007665B4, Map_Find7665B4) {
    const u32 t = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 flags = load<u16>(t + kTabFlags);
    if (flags & kTabUnsorted) {
        store<u16>(t + kTabFlags, static_cast<u16>(flags & 0xFFFE));
        const u32 n = load<u16>(t + kTabCount);
        if (n != 0) call(ctx, h7fn::qsort, {load<u32>(t + kTabEntries), n, 8, load<u32>(t + kTabCompare)});
    }
    s32 hi = static_cast<s32>(load<u16>(t + kTabCount)) - 1, lo = 0;
    u32 base = load<u32>(t + kTabEntries);
    s32 found = -1;
    if (load<u32>(t + kTabCompare) == kCompareU32b) {
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 want = load<u32>(key), k = load<u32>(base + static_cast<u32>(mid) * 8);
            if (k > want) hi = mid - 1;
            else if (k < want) lo = mid + 1;
            else { found = mid; break; }
        }
    } else {
        Stack frame(ctx, 16);  // the key copy at r1+112
        store<u32>(frame.addr(), load<u32>(key));
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const s32 c = static_cast<s32>(opdcall(ctx, load<u32>(t + kTabCompare), {base + static_cast<u32>(mid) * 8, frame.addr()}));
            if (c > 0) hi = mid - 1;
            else if (c < 0) lo = mid + 1;
            else {
                base = load<u32>(t + kTabEntries);
                found = mid;
                break;
            }
        }
    }
    ret(ctx, found >= 0 ? base + static_cast<u32>(found) * 8 + 4 : t + kTabMiss);
}

// bool Obj_CanShow2E2104(Session*, Object* o) (name?): o's user entry (s32
// index at +2456, not -1, below the world's user count, unsigned) is set,
// its id (vtable +80) is non-zero and equals the world's session id at
// +0x128114 (non-zero); the id is asked twice, as in the original.
DECOMP(002E2104, Obj_CanShow2E2104) {
    const u32 o = arg<u32>(ctx, 1);
    ret(ctx, 0u);
    if (o == 0) return;
    const u32 idx = load<u32>(o + kObjUserIndex);
    if (idx == 0xFFFFFFFFu) return;
    const u32 blk = load<u32>(kCurrentWorld) + kUserBlocks;
    if (!(idx < load<u16>(blk + kUserCount))) return;
    const u32 e = load<u32>(load<u32>(blk + kUserEntries) + (idx << 2));
    if (e == 0) return;
    if (static_cast<u32>(hevent::vcall(ctx, e, kEntryGetId, {e})) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 id = load<u32>(load<u32>(kCurrentWorld) + kWorldSessionId);
    if (id == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 got = static_cast<u32>(hevent::vcall(ctx, e, kEntryGetId, {e}));
    ret(ctx, got == id ? 1u : 0u);
}

// void Ev_ApplyGroup2DFAA0(Event* ev, Group* g, const u16* masks, u8 main)
// (name?): finds the group's treasure spawner (def +60, key +204) in the
// treasure info map at 0x01068184 ({u8 player, u8 bit}) and tests its bit
// in the player's episode mask (masks[player +825 episode]). The group is
// applied (vtable +24) when the bit is clear; for main, instead when the bit
// is set or the event has no +36 object. Logs a missing spawner (with the def's name, vtable
// +12) or an episode index of 12 or more; a player slot of 0 or an episode
// of 255 does nothing.
DECOMP(002DFAA0, Ev_ApplyGroup2DFAA0) {
    const u32 ev = arg<u32>(ctx, 0), g = arg<u32>(ctx, 1), masks = arg<u32>(ctx, 2);
    const u32 main = arg<u32>(ctx, 3) & 0xFF;
    if (g == 0 || masks == 0) return;  // r3 stays ev
    u32 def = load<u32>(g + kGroupDef);
    const u32 key = load<u32>(def + kDefSpawnerKey);
    u32 info = 0;
    if (key != 0) {
        Stack frame(ctx, 16);  // the key at r1+112
        store<u32>(frame.addr(), key);
        const u32 p = static_cast<u32>(call(ctx, bfn::Map_Find7662A4, {kTreasureInfoMap, frame.addr()}));
        info = load<u32>(p);
        def = load<u32>(g + kGroupDef);
    }
    if (info == 0) {
        const u32 name = static_cast<u32>(hevent::vcall(ctx, def, kDefName, {def}));
        call(ctx, bfn::String_Format, {kFmtSpawner, name});
        return;
    }
    ctx.gpr[3] = info;
    const u32 p = load<u32>(ev + kEvPlayers) + load<u8>(info) * kPlayerSize;
    if (p == 0) return;
    const u32 ep = load<u8>(p + kPlayerEpisode);
    if (ep == 0xFF) return;
    if (!(ep < kEpisodes)) {
        call(ctx, bfn::String_Format, {kFmtEpisode, ep, kEpisodes});
        return;
    }
    const u32 sh = load<u8>(info + 1) & 63;
    const u32 bit = sh >= 32 ? 0 : 1u << sh;  // slw
    const u32 set = load<u16>(masks + ((ep << 1) & 0x1FE)) & bit;
    ctx.gpr[3] = set;
    bool apply;
    if (main == 0) {
        apply = set == 0;
    } else {
        const u32 obj = load<u32>(ev + kEvMain);
        ctx.gpr[3] = obj;
        apply = obj == 0 || set != 0;
    }
    if (apply) hevent::vcall(ctx, g, kGroupApply, {g});
}
