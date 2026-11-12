// Small helpers used by the sound bank code.
#include "snd.h"

namespace sly4::snd {
using namespace opentit::native;

namespace {
constexpr u32 kCurveConsts = 0x006DBA10;  // f32 { 0, 16129 (127^2), 127, 1 }
constexpr u32 kMapDefaultCompare = 0x00F39AD8;
constexpr u32 kBankSoundsOffset = 28;

// Map of 8-byte { key, value } entries.
struct Map {
    pad<2> _0;
    be_u16 count;     // +2
    pad<2> _4;
    be_u16 flags;     // +6: bit 0 = unsorted
    be_u32 entries;   // +8
    pad<4> _12;
    be_u32 fallback;  // +16 default value
    be_u32 compare;   // +20 OPD
};
}  // namespace

// SoundEntry* Bank_GetSounds(Bank* bank)
u32 Bank_GetSounds(u32 bank) {
    if (bank == 0 && Snd_AssertFailed(kFmtInvalidBlock, kFileBlocksnd, 572, 0x006D8110)) return 0;
    return bank + gref<be_u32>(bank + kBankSoundsOffset);
}

// f32 Snd_VolumeCurve(s8 volume, s8 curve)  (name?; was Snd_DbToGain)
// v = clamp(volume, 0, 127) as float; curve 2: v^2 / 127^2; curve 1:
// 1 - (127 - v)^2 / 127^2; otherwise v / 127.
f32 Snd_VolumeCurve(s8 volume, s8 curve) {
    f32 v = static_cast<f32>(static_cast<f64>(volume));  // fcfid + frsp
    const s32 as_int = static_cast<s32>(static_cast<u32>(rt::fctiw(v, true)));
    const be_f32* k = host<be_f32>(kCurveConsts);
    const f32 zero = k[0], sq = k[1], max = k[2], one = k[3];
    if (as_int < 0)
        v = zero;
    else if (v > max)
        v = max;
    if (curve == 2) return fdivs(fmuls(v, v), sq);
    if (curve == 1) {
        const f32 d = fsubs(max, v);
        return fsubs(one, fdivs(fmuls(d, d), sq));
    }
    return fdivs(v, max);
}

// u32 Angle_Wrap360(s16 a)  (name?; was Snd_CentsToPitch)
// a mod 360 in [0, 360), as an s16 zero-extended from 32 bits.
u32 Angle_Wrap360(s16 a) {
    s32 r = static_cast<s16>(static_cast<u32>(a) - static_cast<u32>((a / 360) * 360));
    if (r < 0) r = static_cast<s16>(r + 360);
    return static_cast<u32>(r);
}

// int abs(int)  (was Rand_Range)
s32 Int_Abs(s32 x) {
    const s32 m = x >> 31;
    return static_cast<s32>((static_cast<u32>(x) ^ static_cast<u32>(m)) - static_cast<u32>(m));
}

// u32* Map_Lookup(Map* map, const u32* key)  (name?)
// Map of 8-byte { key, value } entries. Returns the value slot of the key,
// or the default slot (+16).
u32 Map_Lookup(u32 mp, const be_u32* key) {
    Map& map = gref<Map>(mp);
    const u16 flags = map.flags;
    if (flags & 1) {
        map.flags = static_cast<u16>(flags & 0xFFFE);
        const u16 count = map.count;
        if (static_cast<s32>(count) > 0) call(sfn2::Qsort, map.entries.get(), static_cast<u32>(count), 8u, map.compare.get());
    }
    s32 hi = static_cast<s32>(map.count.get()) - 1, lo = 0;
    u32 entries = map.entries;
    const u32 compare = map.compare;
    auto mid_of = [&] { return static_cast<s32>(static_cast<u32>(hi) + static_cast<u32>(lo)) >> 1; };
    auto slot = [&](s32 mid) { return entries + static_cast<u32>(mid) * 8 + 4; };

    if (compare == kMapDefaultCompare) {
        while (lo <= hi) {
            const s32 mid = mid_of();
            const u32 k = gref(key);
            const u32 e = gref<be_u32>(entries + static_cast<u32>(mid) * 8);
            if (e > k)
                hi = mid - 1;
            else if (e < k)
                lo = mid + 1;
            else
                return slot(mid);
        }
        return mp + 16;
    }
    GuestFrame frame(8);
    be_u32* probe = frame.as<be_u32>();
    *probe = gref(key).get();
    while (lo <= hi) {
        const s32 mid = mid_of();
        const s32 r = call_ptr<s32>(compare, entries + static_cast<u32>(mid) * 8, probe);
        if (r > 0) {
            hi = mid - 1;
        } else if (r < 0) {
            lo = mid + 1;
        } else {
            entries = map.entries;
            return slot(mid);
        }
    }
    return mp + 16;
}

}  // namespace sly4::snd

NATIVE(006D8098, sly4::snd::Bank_GetSounds)
NATIVE(006DBA20, sly4::snd::Snd_VolumeCurve)
NATIVE(006DBAC8, sly4::snd::Angle_Wrap360)
NATIVE(0071B054, sly4::snd::Int_Abs)
NATIVE(007848EC, sly4::snd::Map_Lookup)
