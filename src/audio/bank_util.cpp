// Small helpers used by the sound bank code.
#include "snd.h"

using namespace sly4;
using namespace sly4::snd;

namespace {
constexpr u32 kCurveConsts = 0x006DBA10;  // f32 { 0, 16129 (127^2), 127, 1 }
constexpr u32 kMapDefaultCompare = 0x00F39AD8;
constexpr u32 kBankSoundsOffset = 28;
}  // namespace

// SoundEntry* Bank_GetSounds(Bank* bank)
DECOMP(006D8098, Bank_GetSounds) {
    const u32 bank = arg<u32>(ctx, 0);
    if (bank == 0 && Snd_AssertFailed(ctx, kFmtInvalidBlock, kFileBlocksnd, 572, 0x006D8110)) return ret(ctx, 0u);
    ret(ctx, bank + load<u32>(bank + kBankSoundsOffset));
}

// f32 Snd_VolumeCurve(s8 volume, s8 curve)  (name?; was Snd_DbToGain)
// v = clamp(volume, 0, 127) as float; curve 2: v^2 / 127^2; curve 1:
// 1 - (127 - v)^2 / 127^2; otherwise v / 127.
DECOMP(006DBA20, Snd_VolumeCurve) {
    const s32 volume = static_cast<s8>(arg<u32>(ctx, 0) & 0xFF);
    const s32 curve = static_cast<s8>(arg<u32>(ctx, 1) & 0xFF);
    f32 v = static_cast<f32>(static_cast<f64>(volume));  // fcfid + frsp
    const s32 as_int = static_cast<s32>(static_cast<u32>(rt::fctiw(v, true)));
    const f32 zero = load<f32>(kCurveConsts), sq = load<f32>(kCurveConsts + 4);
    const f32 max = load<f32>(kCurveConsts + 8), one = load<f32>(kCurveConsts + 12);
    if (as_int < 0)
        v = zero;
    else if (v > max)
        v = max;
    if (curve == 2) {
        ctx.fpr[1] = fdivs(fmuls(v, v), sq);
    } else if (curve == 1) {
        const f32 d = fsubs(max, v);
        ctx.fpr[1] = fsubs(one, fdivs(fmuls(d, d), sq));
    } else {
        ctx.fpr[1] = fdivs(v, max);
    }
}

// u32 Angle_Wrap360(s16 a)  (name?; was Snd_CentsToPitch)
// a mod 360 in [0, 360), as an s16 zero-extended from 32 bits.
DECOMP(006DBAC8, Angle_Wrap360) {
    const s32 a = static_cast<s16>(arg<u32>(ctx, 0) & 0xFFFF);
    s32 r = static_cast<s16>(static_cast<u32>(a) - static_cast<u32>((a / 360) * 360));
    if (r < 0) r = static_cast<s16>(r + 360);
    ret(ctx, static_cast<u32>(r));
}

// int abs(int)  (was Rand_Range)
DECOMP(0071B054, abs) {
    const s32 x = arg<s32>(ctx, 0);
    const s32 m = x >> 31;
    ret(ctx, static_cast<s32>((static_cast<u32>(x) ^ static_cast<u32>(m)) - static_cast<u32>(m)));
}

// u32* Map_Lookup(Map* map, const u32* key)  (name?)
// Map of 8-byte { key, value } entries (+2 u16 count, +6 flags bit 0 =
// unsorted, +8 entries, +16 default value, +20 comparator OPD). Returns the
// value slot of the key, or the default slot.
DECOMP(007848EC, Map_Lookup) {
    const u32 map = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u16 flags = load<u16>(map + 6);
    if (flags & 1) {
        store<u16>(map + 6, flags & 0xFFFE);
        const u16 count = load<u16>(map + 2);
        if (static_cast<s32>(count) > 0) call(ctx, sfn2::Qsort, {load<u32>(map + 8), count, 8, load<u32>(map + 20)});
    }
    s32 hi = static_cast<s32>(load<u16>(map + 2)) - 1, lo = 0;
    u32 entries = load<u32>(map + 8);
    const u32 compare = load<u32>(map + 20);
    auto mid_of = [&] { return static_cast<s32>(static_cast<u32>(hi) + static_cast<u32>(lo)) >> 1; };
    auto found = [&](s32 mid) { ret(ctx, entries + static_cast<u32>(mid) * 8 + 4); };

    if (compare == kMapDefaultCompare) {
        while (lo <= hi) {
            const s32 mid = mid_of();
            const u32 k = load<u32>(key);
            const u32 e = load<u32>(entries + static_cast<u32>(mid) * 8);
            if (e > k)
                hi = mid - 1;
            else if (e < k)
                lo = mid + 1;
            else
                return found(mid);
        }
        return ret(ctx, map + 16);
    }
    Stack frame(ctx, 8);
    store<u32>(frame.addr(), load<u32>(key));
    while (lo <= hi) {
        const s32 mid = mid_of();
        const s32 r = static_cast<s32>(call_ptr(ctx, compare, {entries + static_cast<u32>(mid) * 8, frame.addr()}));
        if (r > 0) {
            hi = mid - 1;
        } else if (r < 0) {
            lo = mid + 1;
        } else {
            entries = load<u32>(map + 8);
            return found(mid);
        }
    }
    ret(ctx, map + 16);
}
