// World_HandleEvent callees, part 9k: the world object teardown.
#include "hevent9.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent9;

namespace sly4 {
namespace hevent9k {
namespace kfn {
constexpr u32 Mem_FreeFrom = 0x0054EE6C;  // (p, pool)
// Subsystem resets, no arguments (name?).
constexpr u32 Reset20F914 = 0x0020F914, Reset26F8D4 = 0x0026F8D4, Reset28BF1C = 0x0028BF1C;
constexpr u32 Reset274DD4 = 0x00274DD4, Reset2888C4 = 0x002888C4, Reset266CEC = 0x00266CEC;
constexpr u32 Reset293098 = 0x00293098, Reset234558 = 0x00234558, Reset2AD4FC = 0x002AD4FC;
constexpr u32 Reset2AF5F8 = 0x002AF5F8, Reset2C9DA4 = 0x002C9DA4;
}  // namespace kfn

// The world object vector {+0 Object**, +8 u16 count} and the maps {+2
// u16 count, +8 {key, value}[]} emptied here (name?).
constexpr u32 kWorldObjs = 0x01013B34;
constexpr u32 kMapA = 0x01013B04, kMapB = 0x01013B1C, kList3B84 = 0x01013B84;
constexpr u32 kBlocksA = 0x0103214C, kMapC = 0x01032164, kBlocksB = 0x0103217C;  // block maps: values freed
constexpr u32 kMapD = 0x01056F04, kMap6F1C = 0x01056F1C;
constexpr u32 kWord1022080 = 0x01022080;
// Cleared at the end (frame counters, name?); 0x0101DBE0 is Job C's count.
constexpr u32 kCount13BA0 = 0x01013BA0, kCount15BB0 = 0x01015BB0, kJobCCount = 0x0101DBE0;
constexpr u32 kCount17BC0 = 0x01017BC0, kCount19BD0 = 0x01019BD0;
constexpr u32 kFlag_F64730 = 0x00F64730, kValue_F64734 = 0x00F64734;  // u32 1, f32 -1.0 (name?)
// Object: +172 flags (bit 18: notify), +178 s16 use count.
constexpr u32 kObjFlags172 = 172, kObjUses = 178;
constexpr u32 kVRelease = 4, kVNotify = 40, kVUnuse = 76;  // vtable slots
}  // namespace hevent9k
}  // namespace sly4

using namespace sly4::hevent9k;

namespace {
inline u32 map_value(u32 m, u32 i) { return load<u32>(load<u32>(m + 8) + i * 8 + 4); }

// Releases entry 0 until the map is empty (the release takes it out); a
// null entry 0 spins forever, as in the original.
void map_drain(PPUContext& ctx, u32 m) {
    while (load<u16>(m + 2) != 0) {
        const u32 o = map_value(m, 0);
        if (o != 0) vcall(ctx, o, kVRelease, {o});
    }
    store<u16>(m + 2, 0);
}

void map_release(PPUContext& ctx, u32 m, bool is_signed) {
    for (u32 i = 0; is_signed ? static_cast<s32>(i) < static_cast<s32>(load<u16>(m + 2)) : i < load<u16>(m + 2); ++i) {
        const u32 o = map_value(m, i);
        if (o != 0) vcall(ctx, o, kVRelease, {o});
    }
    store<u16>(m + 2, 0);
}

void map_free(PPUContext& ctx, u32 m) {
    for (s32 i = 0; i < static_cast<s32>(load<u16>(m + 2)); ++i)
        call(ctx, kfn::Mem_FreeFrom, {map_value(m, static_cast<u32>(i)), 0});
    store<u16>(m + 2, 0);
}
}  // namespace

// void WorldObjs_Reset0CE110(u8 full) (name?): three subsystem resets, then
// every world object in use with +172 bit 18 is notified (vtable +40, 1),
// and every one is unused (vtable +76 while +178 is set), its count zeroed
// and released (a null entry gets the +178 store, as in the original). With
// full, the object maps are released or freed and eight more resets run;
// without, map A's objects only lose +12. Finally the frame counters are
// cleared, 0x00F64730 = 1 and 0x00F64734 = -1.0. r3 as the original leaves it.
DECOMP(000CE110, WorldObjs_Reset0CE110) {
    const u32 full = arg<u32>(ctx, 0) & 0xFF;
    store<u32>(kWord1022080, 0);
    call(ctx, kfn::Reset20F914);
    call(ctx, kfn::Reset26F8D4);
    call(ctx, kfn::Reset28BF1C);
    for (u32 i = 0; i < load<u16>(kWorldObjs + 8); ++i) {
        const u32 o = load<u32>(load<u32>(kWorldObjs) + i * 4);
        if (static_cast<s16>(load<u16>(o + kObjUses)) > 0 && (load<u32>(o + kObjFlags172) & 0x00040000u) != 0)
            vcall(ctx, o, kVNotify, {o, 1});
    }
    for (u32 i = 0; i < load<u16>(kWorldObjs + 8); ++i) {
        const u32 o = load<u32>(load<u32>(kWorldObjs) + i * 4);
        if (load<u16>(o + kObjUses) != 0) vcall(ctx, o, kVUnuse, {o});
        store<u16>(o + kObjUses, 0);
        if (o != 0) vcall(ctx, o, kVRelease, {o});
    }
    store<u16>(kWorldObjs + 8, 0);
    store<u16>(kMap6F1C + 2, 0);
    if (full != 0) {
        store<u16>(kList3B84 + 8, 0);
        map_drain(ctx, kMapA);
        map_release(ctx, kMapB, false);
        map_free(ctx, kBlocksA);
        map_release(ctx, kMapC, true);
        map_free(ctx, kBlocksB);
        call(ctx, kfn::Reset274DD4);
        call(ctx, kfn::Reset2888C4);
        call(ctx, kfn::Reset266CEC);
        call(ctx, kfn::Reset293098);
        call(ctx, kfn::Reset234558);
        call(ctx, kfn::Reset2AD4FC);
        call(ctx, kfn::Reset2AF5F8);
        store<u16>(kMap6F1C + 2, 0);
        map_drain(ctx, kMapD);
        call(ctx, kfn::Reset2C9DA4);
    } else {
        for (s32 i = 0; i < static_cast<s32>(load<u16>(kMapA + 2)); ++i) store<u32>(map_value(kMapA, static_cast<u32>(i)) + 12, 0);
    }
    store<u32>(kCount13BA0, 0);
    store<u32>(kCount15BB0, 0);
    store<u32>(kJobCCount, 0);
    store<u32>(kCount17BC0, 0);
    store<u32>(kFlag_F64730, 1);
    store<u32>(kCount19BD0, 0);
    store<u32>(kValue_F64734, 0xBF800000u);  // f32 -1.0
    ctx.gpr[3] = 0x01020000;  // r3 as the original leaves it (lis for the stores)
}
