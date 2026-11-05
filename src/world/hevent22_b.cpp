// World_HandleEvent callees, part 22b: the track pool's object release
// (named by hevent21_b.cpp TrackPool_Update390724).
#include "hevent22.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent22;

namespace {
constexpr u32 kTrackSubRelease38B174 = 0x0038B174;  // (Sub* rec +32, Base*) (name?)
constexpr u32 kMemFreeFrom = 0x0054EE6C;            // (p, pool) (symbols.txt Mem_FreeFrom)
constexpr u32 kRecKindStop = 9;
constexpr u32 kTypeFnStop = 8, kTypeFnSub = 36;
}  // namespace

// u32 Track_Release38DC0C(TrackObj* t) (name?): with a +4 block: when the
// base (+0 - 152) has flag 0x20, for each of the 8 s16 record indices
// after the base's variable part (+146 record's u16 +0 x, then
// align16(12x + 23) and align16(8y + 21) bytes, y the u16 there, then +160)
// that is not -1, the record's +32 sub-record either gets
// 0x0038B174(sub, base) (sub +16 byte set) or the type (sub +0)'s +36 OPD
// (sub, base) when set, then sub +8 = 0. Next every kind 9 record whose
// type has a +8 OPD gets it (rec +4, base), the base re-read each time.
// Finally Mem_FreeFrom(+4, 0) and +4 = +0 = 0. Records out of range or of
// kind 10+ are read as the null record, as in the original.
DECOMP(0038DC0C, Track_Release38DC0C) {
    const u32 t = arg<u32>(ctx, 0);
    if (load<u32>(t + 4) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 base = load<u32>(t) - kBaseBias;
    if ((load<u16>(base + kBaseFlags) & 0x20u) != 0) {
        const u32 c = load<u16>(base + kBaseExtra) * kRecSize;
        const u32 x = load<u16>(base + c + kBaseBias);
        const u32 a = (12u * x + 23u) & ~15u;
        const u32 y = load<u16>(base + c + a + kBaseBias);
        const u32 b = ((y << 3) + 21u) & ~15u;
        const u32 list = base + c + a + b + 160;
        for (u32 i = 0; i < 8; ++i) {
            const s32 k = static_cast<s16>(load<u16>(list + 2 * i));
            if (k == -1) continue;
            const u32 slot = rec_checked(base, k) + 4;
            const u32 sub = slot + 28;
            if (load<u8>(slot + 16) != 0) {
                call(ctx, kTrackSubRelease38B174, {sub, base});
                continue;
            }
            const u32 fn = load<u32>(type_entry(static_cast<s16>(load<u16>(slot + 28))) + kTypeFnSub);
            if (fn != 0) {
                ctx.gpr[5] = fn;
                ctx.gpr[6] = load<u32>(fn);
                call_ptr(ctx, fn, {sub, base});
            }
            store<u32>(sub + 8, 0);
        }
    }
    for (u32 i = 0;; i = (i + 1) & 0xFFFF) {
        const u32 bs = load<u32>(t) - kBaseBias;
        if (!(static_cast<s32>(i) < static_cast<s32>(load<u16>(bs + kBaseCount)))) break;
        const s32 k = static_cast<s16>(i);
        if (load<u32>(rec_checked(bs, k)) != kRecKindStop) continue;
        const u32 r = rec_checked(bs, k);
        const u32 fn = load<u32>(type_entry(static_cast<s16>(load<u16>(r + 4))) + kTypeFnStop);
        if (fn == 0) continue;
        ctx.gpr[5] = fn;
        ctx.gpr[6] = load<u32>(fn);
        call_ptr(ctx, fn, {rec_checked(bs, k) + 4, bs});
    }
    call(ctx, kMemFreeFrom, {load<u32>(t + 4), 0});
    store<u32>(t + 4, 0);
    store<u32>(t + 0, 0);
}
