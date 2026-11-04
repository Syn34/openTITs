// World_HandleEvent callees, part 3: 83 instructions.
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent3g {
namespace gfn {
constexpr u32 sub_000B27B4 = 0x000B27B4;     // (u32 id): called once all 35 levels are marked (name?)
}  // namespace gfn

constexpr u32 kCamRig = 0x00F66FD0;          // u32 object reset by CamRig_Reset (name?)
constexpr u32 kCamRigDefaults = 0x010AA620;  // f32[8] defaults (name?)
// CamRig_Reset's constants (f32 bits).
constexpr u32 kF20 = 0x41A00000, kF0_1 = 0x3DCCCCCD, kF0_4 = 0x3ECCCCCD;
constexpr u32 kF0_25 = 0x3E800000, kF1 = 0x3F800000, kFm50 = 0xC2480000, kF150 = 0x43160000;

constexpr u32 kLevelHashes = 0x00AE91E8;     // u32[35]
constexpr u32 kLevelCount = 35;
constexpr u32 kWorldLevelHash = 0x128114;    // u32 (name?)
constexpr u32 kSlotLevelsSeen = 312;         // SaveSlot + : u64 bit per kLevelHashes entry (name?)
constexpr u32 kAllLevelsId = 0x0023FE97;     // given to sub_000B27B4 (name?)
}  // namespace hevent3g
}  // namespace sly4

using namespace sly4::hevent3g;

// void CamRig_Reset(): for the object at 0x00F66FD0: +32/+36/+40 and +64/+68
// from the defaults at 0x010AA620, vfunc +24(o; f1 = default +24), +88 =
// default +28, +96 |= 0x20; then by mode (+92): 0 -> +256 = 20, +264 = 0.1,
// +268 = 0.4 and its first part (+4) gets +20 = -50, +24 = 150; 1 -> both
// parts (+4, +8) get +4/+8/+1536 = {0 or 0.25, 0.25 or 1, 1} and +20/+24 =
// -50/150. r3 is left as the original leaves it on each path (name?)
DECOMP(0042A740, CamRig_Reset) {
    const u32 o = load<u32>(kCamRig);
    if (o == 0) return;  // r3 untouched
    const u32 d = kCamRigDefaults;
    store<u32>(o + 32, load<u32>(d + 0));
    store<u32>(o + 36, load<u32>(d + 4));
    store<u32>(o + 40, load<u32>(d + 8));
    store<u32>(o + 64, load<u32>(d + 16));
    store<u32>(o + 68, load<u32>(d + 20));
    ctx.fpr[1] = static_cast<f64>(load<f32>(d + 24));
    vcall(ctx, o, 24, {o});
    const u32 b96 = load<u8>(o + 96) | 0x20u;
    const s32 mode = load<s32>(o + 92);
    store<u32>(o + 88, load<u32>(d + 28));
    ctx.fpr[1] = static_cast<f64>(load<f32>(d + 28));
    store<u8>(o + 96, static_cast<u8>(b96));
    ctx.gpr[3] = o + 4;
    if (mode == 0) {
        store<u32>(o + 256, kF20);
        const u32 a = load<u32>(o + 4);
        store<u32>(o + 264, kF0_1);
        store<u32>(o + 268, kF0_4);
        store<u32>(a + 20, kFm50);
        store<u32>(a + 24, kF150);
        ctx.gpr[3] = a;
    } else if (mode == 1) {
        const u32 a = load<u32>(o + 4), b = load<u32>(o + 8);
        store<u32>(a + 4, 0);
        store<u32>(a + 8, kF0_25);
        store<u32>(a + 1536, kF1);
        store<u32>(b + 4, kF0_25);
        store<u32>(b + 8, kF1);
        store<u32>(b + 1536, kF1);
        store<u32>(a + 20, kFm50);
        store<u32>(a + 24, kF150);
        store<u32>(b + 20, kFm50);
        store<u32>(b + 24, kF150);
        ctx.gpr[3] = b;
    }
}

// u64 World_MarkLevelSeen(World* w): sets the bit of w's level hash
// (+0x128114) among the 35 at 0x00AE91E8 in the current save slot's (user 0)
// u64 at +312; when that adds a bit and all 35 are set, sub_000B27B4(0x0023FE97).
// r3 is computed as the original leaves it (name?)
DECOMP(0048BF28, World_MarkLevelSeen) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 cw = load<u32>(kCurrentWorld);
    const u32 slot = cw + load<u8>(cw + kUserBlocks) * kSaveSlotSize + kSaveData;
    const u64 old = load<u64>(slot + kSlotLevelsSeen);
    u64 r3 = 0;
    for (u32 i = 0; i < kLevelCount; ++i) {
        r3 = load<u32>(kLevelHashes + i * 4);
        if (static_cast<u32>(r3) == load<u32>(w + kWorldLevelHash)) {
            r3 = load<u64>(slot + kSlotLevelsSeen) | (1ull << i);
            store<u64>(slot + kSlotLevelsSeen, r3);
            break;
        }
    }
    u64 bits = load<u64>(slot + kSlotLevelsSeen);
    if (bits == old || bits == 0) {
        ctx.gpr[3] = r3;
        return;
    }
    // Popcount, two bits per round; r3 ends at the count before the last round's step.
    u32 count = 0, before = 0;
    while (true) {
        before = count;
        bits &= bits - 1;
        count = before + 1;
        if (bits == 0) break;
        bits &= bits - 1;
        count = before + 2;
        if (bits == 0) break;
    }
    ctx.gpr[3] = before;
    if (count >= kLevelCount) call(ctx, gfn::sub_000B27B4, {kAllLevelsId});
}
