// World_HandleEvent callees, part 12g: Loader_Start2DEA50, which switches the
// loader to a user entry (called by Loader_Reset2DEDB4 / Loader_Finish2E0498).
#include "hevent12.h"

using namespace sly4;
using namespace sly4::hevent12;

namespace {
// Loader (as hevent12_c.cpp): +72 current entry, +100 flags.
constexpr u32 kLdCurrent = 72, kLdFlags = 100;
constexpr u32 kLdStarting = 0x40000000u;  // set while Loader_Start2DEA50 runs
// User block (World +4208): +8 player object, +16, +80 Entry*[], +88 u16
// count, +0x20074 u32 per-entry flags (by entry +2452 index).
constexpr u32 kUbPlayer = 8, kUb16 = 16, kUbEntries = 80, kUbCount = 88, kUbEntryFlags = 0x20074;
constexpr u32 kUbFlagsClear = 0x0C000000u;  // bits cleared for the entries passed over
// Entry: +20 u32 (bit 5: keep level, bits 0..2 dropped for vt +40), +2404
// level id, +2416 u32, +2452 s32 flags index; vtable +24 (e, 1, s32), +40
// (e, u32), +80 (e) -> level id.
constexpr u32 kEnValue = 20, kEnLevel = 2404, kEn2416 = 2416, kEnIndex = 2452;
constexpr u32 kEnKeepLevel = 0x20;
constexpr u32 kEnVtSet = 24, kEnVtSelect = 40, kEnVtLevel = 80;
constexpr u32 kWorldLevel = 0x128114;   // World +0x128114: current level id
constexpr u32 kGameState = 0x00F64C20;  // u32: +40 -> +492 default level id (name?)
constexpr u32 kGsSub = 40, kGsDefaultLevel = 492;
// Player object: +184 u64 (low word bit 5), +192 u32 (bit 3), +216 sub (+52
// level id, +192 u32).
constexpr u32 kPlFlagsLo = 188, kPlFlags192 = 192, kPlSub = 216, kSubLevel = 52, kSub192 = 192;
constexpr u32 kPlBusy = 0x20, kPlTravel = 0x8;
constexpr u32 kLevelHub = 0x00014120;  // level id with the extra checks (name?)
constexpr u32 kEntryNotify487670 = 0x00487670;  // (World*, Entry*) (name?)
constexpr u32 kLoaderLeave = 0x002DE794;        // (Loader*, Entry*, UserBlock*) (name?)
constexpr u32 kLoaderEnter = 0x002DE41C;        // (Loader*, UserBlock*, Entry*) (name?)
constexpr u32 kUserStay = 0x00488178;           // (World*, UserBlock*, UserBlock* +16) (name?)
constexpr u32 kUserTravel = 0x00484BE0;         // (World*, UserBlock*, level, +16, 0, 0, 0, 0, 1) (name?)
constexpr u32 kWorldStartLevel = 0x004674C4;    // World_StartLevel (World*, u32 level, u8)
}  // namespace

// u32 Loader_Start2DEA50(Loader* l, Entry* e, u8 keep, u8 start) (name?): with
// e: (unless keep) notifies / leaves the old entry; resets every user entry
// before e (vt +24 (x, 1, 5), clearing its flags), selects e (vt +40, +72,
// vt +24 (e, 1, -1)) and (unless keep) enters it and moves the player to its
// level (0x00488178 when already there or busy, else 0x00484BE0). Finally,
// with start set, starts the current entry's level (vt +80) when it differs
// from the world's. r3 is left as the original leaves it.
DECOMP(002DEA50, Loader_Start2DEA50) {
    const u32 l = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const u32 keep = arg<u32>(ctx, 2) & 0xFF, start = arg<u32>(ctx, 3);
    if (e == 0) return;  // r3 left as l
    store<u32>(l + kLdFlags, load<u32>(l + kLdFlags) | kLdStarting);
    const u32 world = load<u32>(hevent::kCurrentWorld);
    const u32 ub = world + hevent::kUserBlocks;
    if (keep == 0) {
        if (load<u32>(e + kEn2416) != 0 && (load<u32>(e + kEnValue) & kEnKeepLevel) != 0) call(ctx, kEntryNotify487670, {world, e});
        call(ctx, kLoaderLeave, {l, e, ub});
    }
    for (u32 i = 0; i < load<u16>(ub + kUbCount); ++i) {
        const u32 x = load<u32>(load<u32>(ub + kUbEntries) + i * 4);
        if (x != e) {
            hevent::vcall(ctx, x, kEnVtSet, {x, 1, 5});
            const u32 fa = ub + load<u32>(x + kEnIndex) * 4 + kUbEntryFlags;
            store<u32>(fa, load<u32>(fa) & ~kUbFlagsClear);
            continue;
        }
        hevent::vcall(ctx, x, kEnVtSelect, {x, load<u32>(x + kEnValue) & ~7u});
        store<u32>(l + kLdCurrent, x);
        hevent::vcall(ctx, x, kEnVtSet, {x, 1, 0xFFFFFFFFu});
        if (keep != 0) break;
        call(ctx, kLoaderEnter, {l, ub, x});
        const bool keep_level = (load<u32>(x + kEnValue) & kEnKeepLevel) != 0;
        const u32 w = load<u32>(hevent::kCurrentWorld);
        bool stay = true;
        u32 level = 0;
        if (!keep_level) {
            level = load<u32>(x + kEnLevel);
            if (level == 0) level = load<u32>(load<u32>(load<u32>(kGameState) + kGsSub) + kGsDefaultLevel);
            const u32 pl = level != 0 ? load<u32>(ub + kUbPlayer) : 0u;
            if (pl != 0) {
                const u32 sub = load<u32>(pl + kPlSub);
                const u32 cur = sub != 0 ? load<u32>(sub + kSubLevel) : 0u;
                if (cur != level) {
                    stay = false;
                    if (level == kLevelHub) {
                        if ((load<u32>(pl + kPlFlagsLo) & kPlBusy) != 0) {
                            stay = true;
                        } else if ((load<u32>(pl + kPlFlags192) & kPlTravel) != 0 && load<u32>(sub + kSub192) == level) {
                            stay = true;  // sub read unchecked
                        }
                    }
                }
            }
        }
        if (stay) call(ctx, kUserStay, {w, ub, ub + kUb16});
        else call(ctx, kUserTravel, {w, ub, level, ub + kUb16, 0, 0, 0, 0, 1});
        break;
    }
    store<u32>(l + kLdFlags, load<u32>(l + kLdFlags) & ~kLdStarting);
    const u32 cur = load<u32>(l + kLdCurrent);
    u32 level = 0;
    if (cur != 0) level = static_cast<u32>(hevent::vcall(ctx, cur, kEnVtLevel, {cur}));
    if ((start & 0xFF) == 0 || level == 0) return ret(ctx, start & 0xFF);
    const u32 w = load<u32>(hevent::kCurrentWorld);
    if (load<u32>(w + kWorldLevel) == level) return ret(ctx, w);
    call(ctx, kWorldStartLevel, {w, level, 0});
}
