// World_HandleEvent callees, part 12i: Loader_Begin2E0008 (called by
// Loader_Finish2E0498), which marks the loader's scene groups in the current
// user's save slot.
#include "hevent12.h"

using namespace sly4;
using namespace sly4::hevent12;

namespace {
// Loader: +20 Group[] (860 bytes each), +24 u32 group count.
constexpr u32 kLdGroups = 20, kLdGroupCount = 24, kGroupSize = 860;
// Group: +716 Scene[] (536 bytes each), +720 u8 scene count, +812 u32 value,
// +822 u8 save flag index, +828 u8 open.
constexpr u32 kGrScenes = 716, kGrSceneCount = 720, kGrValue = 812, kGrFlagIndex = 822, kGrOpen = 828;
constexpr u32 kSceneSize = 536;
// Scene: +468 u32 handle, +500 u8 group, +501 u8, +502 u8 save byte, +503 u8
// (0: marks the group).
constexpr u32 kScHandle = 468, kScGroup = 500, kSc501 = 501, kScSaveByte = 502, kScNoMark = 503;
// Save slot (World +80 + 4056 * user byte at World +4208): +160 u32, +464[]
// u8 scene states (2: seen), +496 u16 group flags (4 bytes each; +498 u8).
constexpr u32 kSvValue = 160, kSvSceneState = 464, kSvGroupFlags = 496, kSvGroupByte = 498;
constexpr u8 kSceneSeen = 2;
constexpr u32 kGfStateMask = 0x18, kGfState1 = 0x08, kGfState2 = 0x10, kGfMarked = 0x20, kGfRevisit = 0x80;
constexpr u32 kSceneOpen49E038 = 0x0049E038;  // (u32 handle) (name?)
constexpr u32 kGroupStart2DB70C = 0x002DB70C; // (Group*) (name?)

// The current user's save slot (re-read like the original at each use).
u32 save_slot() {
    const u32 w = load<u32>(hevent::kCurrentWorld);
    return w + load<u8>(w + hevent::kUserBlocks) * hevent::kSaveSlotSize + hevent::kSaveData;
}
u32 group_flags(u32 g) { return save_slot() - hevent::kSaveData + (static_cast<u32>(load<u8>(g + kGrFlagIndex)) << 2) + kSvGroupFlags; }
void or_flags(u32 g, u32 bits) {
    const u32 a = group_flags(g);
    store<u16>(a, static_cast<u16>(load<u16>(a) | bits));
}
void set_state(u32 g, u32 state) {
    const u32 a = group_flags(g);
    store<u16>(a, static_cast<u16>((load<u16>(a) & ~kGfStateMask) | state));
}
}  // namespace

// void Loader_Begin2E0008(Loader* l, Scene* s) (name?): with s: for every
// group, scenes already seen in the save slot (all of them in the open groups
// before s's) are marked seen again and opened (0x0049E038); the group flags get
// state 2 (open groups before s's), 0x80 (s is a later scene of the group)
// and 0x20 plus +160 = group +812 (a seen scene marks it). Then s's group
// gets its +498 byte (s +501, unless s was seen), state 1 if not 2, the
// marked bit when s marks it (and +160), and 0x002DB70C. r3 is left as the
// original leaves it.
DECOMP(002E0008, Loader_Begin2E0008) {
    const u32 l = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    if (s == 0) return;  // r3 left as l
    const u32 n = load<u8>(s + kScGroup);
    const u32 open = load<u8>(load<u32>(l + kLdGroups) + n * kGroupSize + kGrOpen);
    for (u32 i = 0; i < load<u32>(l + kLdGroupCount); ++i) {
        const u32 g = load<u32>(l + kLdGroups) + i * kGroupSize;
        const bool all = i < n && open != 0;
        bool mark = false, later = false;
        for (u32 j = 0; j < load<u8>(g + kGrSceneCount); ++j) {
            const u32 sc = load<u32>(g + kGrScenes) + j * kSceneSize;
            if (all || load<u8>(save_slot() + kSvSceneState + load<u8>(sc + kScSaveByte)) == kSceneSeen) {
                store<u8>(save_slot() + kSvSceneState + load<u8>(sc + kScSaveByte), kSceneSeen);
                call(ctx, kSceneOpen49E038, {load<u32>(sc + kScHandle)});
                if (load<u8>(sc + kScNoMark) == 0) mark = true;
            }
            if (s == sc && j != 0) later = true;
        }
        if (all) set_state(g, kGfState2);
        if (later) or_flags(g, kGfRevisit);
        if (mark) {
            or_flags(g, kGfMarked);
            store<u32>(save_slot() + kSvValue, load<u32>(g + kGrValue));
        }
    }
    const u32 base = load<u32>(l + kLdGroups);
    const u32 g = base + load<u8>(s + kScGroup) * kGroupSize;
    if (g == 0) return ret(ctx, load<u8>(s + kScGroup) * kGroupSize);
    if (load<u8>(save_slot() + kSvSceneState + load<u8>(s + kScSaveByte)) != kSceneSeen)
        store<u8>(group_flags(g) - kSvGroupFlags + kSvGroupByte, load<u8>(s + kSc501));
    if (((load<u16>(group_flags(g)) >> 3) & 3) != 2) set_state(g, kGfState1);
    const u32 a = group_flags(g);
    const u32 v = load<u16>(a);
    if ((v & kGfMarked) == 0) store<u16>(a, static_cast<u16>(load<u8>(s + kScNoMark) == 0 ? v | kGfMarked : v));
    if ((load<u16>(group_flags(g)) & kGfMarked) != 0) store<u32>(save_slot() + kSvValue, load<u32>(g + kGrValue));
    call(ctx, kGroupStart2DB70C, {g});
}
