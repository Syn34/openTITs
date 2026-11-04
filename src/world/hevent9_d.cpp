// World_HandleEvent callees, part 9d: the deferred-effect update.
#include "hevent9.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent9;

namespace sly4 {
namespace hevent9d {
namespace dfn {
constexpr u32 PtrVector_Init = 0x00551598;     // (vec, capacity, 0, 0)
constexpr u32 Effect_SetTime = 0x004CF710;     // (Effect*, f32)
constexpr u32 Effect_Destruct = 0x004C8E80;    // (Effect*)
constexpr u32 Effect_Check4CF620 = 0x004CF620; // (Effect*, 0) -> done? (name?)
}  // namespace dfn

// The deferred-effect vector (as hevent4_c.cpp): lazily built PtrVector of
// 140-byte Effect blocks.
constexpr u32 kEffectFreeBlocked = 0x010C2958;  // u8
constexpr u32 kEffectFreeInit = 0x010C2960;     // u8: the vector is built
constexpr u32 kEffectFreeList = 0x010C2968;     // PtrVector {+0 data, +6 u16, +8 u16 count}
constexpr u32 kEffectSize = 140;
// Effect +136 flags: 0x80 time frozen, 0x100 timed, 0x200 release when done.
constexpr u32 kEffectFlags = 136;
constexpr u32 kFlagFrozen = 0x80, kFlagTimed = 0x100, kFlagRelease = 0x200;
}  // namespace hevent9d
}  // namespace sly4

using namespace sly4::hevent9d;

namespace {
// The vector, built on first use (Construct, +6 = 4, Init(16)); every check
// in the original reloads the flag.
void list_ensure(PPUContext& ctx) {
    if (load<u8>(kEffectFreeInit) != 0) return;
    call(ctx, h9fn::PtrVector_Construct, {kEffectFreeList});
    store<u16>(kEffectFreeList + 6, 4);
    call(ctx, dfn::PtrVector_Init, {kEffectFreeList, 16, 0, 0});
    store<u8>(kEffectFreeInit, 1);
}
}  // namespace

// u16 Effects_UpdateDeferred4CFD1C(f32 dt) (name?): unless 0x010C2958 is
// set, walks the deferred-effect vector: timed effects (not frozen) get
// Effect_SetTime(dt); releasing ones that 0x004CF620 reports done are
// swapped with the last entry, destroyed and freed (140 bytes) and popped,
// and the same index is looked at again. The pop is the generic
// swap-remove (with an empty vector it would wrap the count and copy entry
// 0xFFFF over entry -1). r3 as the original leaves it (the flag, or the
// count).
DECOMP(004CFD1C, Effects_UpdateDeferred4CFD1C) {
    const f64 dt = farg(ctx, 0);
    const u32 blocked = load<u8>(kEffectFreeBlocked);
    if (blocked != 0) {
        ret(ctx, blocked);
        return;
    }
    for (u32 i = 0, off = 0;; ++i, off += 4) {
        list_ensure(ctx);
        const u32 n = load<u16>(kEffectFreeList + 8);
        if (!(i < n)) {
            ret(ctx, n);
            return;
        }
        const u32 e = load<u32>(load<u32>(kEffectFreeList) + off);
        u32 flags = load<u32>(e + kEffectFlags);
        if ((flags & kFlagTimed) != 0 && (flags & kFlagFrozen) == 0) {
            callf(ctx, dfn::Effect_SetTime, {e}, {dt});
            flags = load<u32>(e + kEffectFlags);
        }
        if ((flags & kFlagRelease) == 0) continue;
        if (static_cast<u32>(call(ctx, dfn::Effect_Check4CF620, {e, 0})) == 0) continue;
        list_ensure(ctx);
        u32 data = load<u32>(kEffectFreeList);
        const u32 last = load<u32>(data + (load<u16>(kEffectFreeList + 8) - 1) * 4);
        store<u32>(data + off, last);
        list_ensure(ctx);
        store<u32>(load<u32>(kEffectFreeList) + (load<u16>(kEffectFreeList + 8) - 1) * 4, e);
        if (e != 0) {
            call(ctx, dfn::Effect_Destruct, {e});
            call(ctx, h7fn::SmallFree, {e, kEffectSize});
        }
        list_ensure(ctx);
        const u32 c = load<u16>(kEffectFreeList + 8);
        store<u16>(kEffectFreeList + 8, static_cast<u16>(c - 1));
        if (c == 0) {  // never true here: i < count above
            data = load<u32>(kEffectFreeList);
            store<u32>(data - 4, load<u32>(data + 0x3FFFC));
        }
        i -= 1;
        off -= 4;
    }
}
