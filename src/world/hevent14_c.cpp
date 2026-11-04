// World_HandleEvent callees, part 14c: Session_Start2E3AEC, starting an
// entry (level change, spawning, travelling to its point and the stay at the
// user's own point).
#include "hevent14.h"

using namespace sly4;
using namespace sly4::hevent13;
using namespace sly4::hevent14;

namespace {
// Entry: vt +76 point hash for a user index, vt +80 level, vt +92 busy; +20
// flags (bit 5: spawn users), +2404 target point hash, +2416 spawns (u32[9]),
// +2500 / +2512 per-user bit sets (release / stay).
constexpr u32 kEnVtPoint = 76, kEnVtLevel = 80, kEnVtBusy = 92;
constexpr u32 kEnFlags = 20, kEnFlagSpawn = 0x20;
constexpr u32 kEnTarget = 2404, kEnReleaseBits = 2500, kEnStayBits = 2512;
// World +0x128114: the current level.
constexpr u32 kWLevel = 0x128114;
// UserBlock: +2 u8 user index, +8 player, +16 the 3x4 transform (rows of 16).
constexpr u32 kUbIndex = 2, kUbPlayer = 8, kUbMatrix = 16;
}  // namespace

// u32 Session_Start2E3AEC(Session*, Entry* e, u32 user) (name?): starts e for
// user block `user`. A different level is started; otherwise, unless the
// player is already at the entry's target point, the users are spawned (e
// +20 bit 5, not busy) or the player travels there (from the named point of
// its spawn, else its own transform). Then, when the entry's stay bit is set
// for the player's index, the player stays at that index's point and, with
// the release bit, is released.
DECOMP(002E3AEC, Session_Start2E3AEC) {
    const u32 e = arg<u32>(ctx, 1), user = arg<u32>(ctx, 2);
    const u32 ub = load<u32>(hevent::kCurrentWorld) + user * hevent::kUserBlockSize + hevent::kUserBlocks;
    Stack frame(ctx, 192);  // the original's r1+128 / +192 matrices, +256 / +260 keys
    const u32 r1 = frame.addr();
    const u32 travel_mat = r1 + 16, stay_mat = r1 + 80, key0 = r1 + 144, key1 = r1 + 148;

    bool new_level = false;
    if (static_cast<u32>(hevent::vcall(ctx, e, kEnVtLevel, {e})) != 0) {
        const u32 lv = static_cast<u32>(hevent::vcall(ctx, e, kEnVtLevel, {e}));
        const u32 w = load<u32>(hevent::kCurrentWorld);
        if (lv != load<u32>(w + kWLevel)) {
            const u32 lv2 = static_cast<u32>(hevent::vcall(ctx, e, kEnVtLevel, {e}));
            call(ctx, h14fn::World_StartLevel, {w, lv2, 0});
            new_level = true;
        }
    }
    if (!new_level) {
        const u32 target = load<u32>(e + kEnTarget);
        bool there = true;
        if (target != 0) {
            const u32 p = load<u32>(ub + kUbPlayer);
            if (p != 0) {
                const u32 owner = load<u32>(p + kObjOwner);
                const u32 h = owner != 0 ? load<u32>(owner + kOwnerHash) : 0;
                if (h != target) {
                    there = false;
                    if (target == kAnyPoint) {
                        if ((load<u32>(p + kObjFlags188) & kObjFlag188Bit5) != 0) there = true;
                        // Quirk: without an owner this reads guest address 192.
                        else if ((load<u32>(p + kObjFlags192) & kObjFlag192Bit3) != 0 &&
                                 load<u32>(owner + kOwnerAltHash) == target)
                            there = true;
                    }
                }
            }
        }
        if (there) {
            if (load<u32>(e + kEntrySpawns) != 0 && (load<u32>(e + kEnFlags) & kEnFlagSpawn) != 0 &&
                static_cast<u32>(hevent::vcall(ctx, e, kEnVtBusy, {e})) == 0)
                call(ctx, h14fn::World_SpawnUsers487670, {load<u32>(hevent::kCurrentWorld), e});
        } else {
            copy_mat(ctx, travel_mat, ub + kUbMatrix);
            s32 idx = static_cast<s32>(call(ctx, h14fn::Entry_Index325EBC, {e, load<u32>(ub + kUbPlayer)}));
            const s32 idx0 = idx;
            const u32 a = idx0 == -1 ? load<u8>(ub + kUbIndex) : static_cast<u32>(idx0);
            if (static_cast<u32>(hevent::vcall(ctx, e, kEnVtPoint, {e, a})) != 0) {
                if (idx == -1) idx = load<u8>(ub + kUbIndex);
                const u32 h = static_cast<u32>(hevent::vcall(ctx, e, kEnVtPoint, {e, static_cast<u32>(idx)}));
                const u32 pt = find_point(ctx, key1, h);
                if (pt != 0) copy_mat(ctx, travel_mat, pt + kPointMatrix);
            }
            if (static_cast<u32>(hevent::vcall(ctx, e, kEnVtBusy, {e})) == 0) {
                call(ctx, h14fn::User_Travel484BE0,
                     {load<u32>(hevent::kCurrentWorld), ub, load<u32>(e + kEnTarget), travel_mat, 0, 0, 0, 0, 1});
            }
        }
    }

    s32 idx = static_cast<s32>(call(ctx, h14fn::Entry_Index325EBC, {e, load<u32>(ub + kUbPlayer)}));
    const u32 stay = load<u32>(e + kEnStayBits);
    ctx.gpr[3] = load<u32>(e);  // r3 left with the vtable
    if (idx == -1) idx = load<u8>(ub + kUbIndex);
    const u32 byte = static_cast<u32>(idx) >> 3, bit = 1u << (static_cast<u32>(idx) & 7);
    if ((load<u8>(stay + byte) & bit) == 0) return;
    if (static_cast<u32>(hevent::vcall(ctx, e, kEnVtPoint, {e, static_cast<u32>(idx)})) == 0) return;
    const u32 h = static_cast<u32>(hevent::vcall(ctx, e, kEnVtPoint, {e, static_cast<u32>(idx)}));
    store<u32>(key0, h);
    const u32 slot = static_cast<u32>(call(ctx, kMapFind73343C, {load<u32>(load<u32>(kMapHolderPoint) + kHolderMap), key0}));
    const u32 pt = load<u32>(slot);
    if (pt == 0) return;  // r3 = slot
    copy_mat(ctx, stay_mat, pt + kPointMatrix);
    call(ctx, h14fn::User_Stay488178, {load<u32>(hevent::kCurrentWorld), ub, stay_mat});
    const u32 rel = load<u32>(e + kEnReleaseBits);
    const u32 p = load<u32>(ub + kUbPlayer);
    if ((load<u8>(rel + byte) & bit) == 0) return ret(ctx, 0u);
    call(ctx, h14fn::Obj_Release12BF70, {p});
}
