// World_HandleEvent callees, part 4: PushStack_Blend (the audio push stack's
// per-frame blend, see PushStack_Changed in hevent2_a.cpp).
#include <cmath>

#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent4b {
namespace bfn {
constexpr u32 Snd_Lock = 0x006EFD10;
constexpr u32 Snd_Unlock = 0x006EFD34;
}  // namespace bfn

// The push stack's blocks (as in hevent2_a.cpp): {u32; {s32 op; f32 value}[31]}.
constexpr u32 kPushCur = 0x010AFB94;      // the values in use
constexpr u32 kPushTarget = 0x010AFC90;   // where the blend goes
constexpr u32 kPushFrom = 0x010AFD8C;     // where it started
constexpr u32 kPushTime = 0x010AFE88;     // f32 blend time
constexpr u32 kPushElapsed = 0x010AFE8C;  // f32 blend fraction 0..1
constexpr u32 kPushValues = 31;

// Constants in front of the function.
constexpr u32 kEps = 0x00447848;          // f32 1e-5
constexpr u32 kOneF = 0x0044784C;         // f32 1.0
constexpr u32 kOneD = 0x00447850;         // f64 1.0
constexpr u32 kZeroD = 0x00447858;        // f64 0.0

// Per-group output: every 1 << *kGroupStrideShift'th value from the audio
// frame counter on is scaled and clamped into kGroupVolumes (name?).
constexpr u32 kGroupStrideShift = 0x010B0B20;  // u32 (0x010B0AD8 + 72)
constexpr u32 kAudioFrame = 0x010B00DC;        // u32 frame counter (game/frame.h)
constexpr u32 kGroupDirty = 0x011D86D0;        // u32 bit per value; -1 = all (name?)
constexpr u32 kGroupVolumes = 0x00F8E5F8;      // f32[31] (name?)
constexpr u32 kMasterVolume = 0x00F8E5F4;      // f32 (name?)
constexpr u32 kMasterLast = 0x00F65B8C;        // f32: the master value last applied (name?)
// Scales: kScaleA (0x00F65B40..4C) times tuning values (+72 of the objects).
constexpr u32 kScaleSfx = 0x00F65B44, kScaleMusic = 0x00F65B48;   // f32 (name?)
constexpr u32 kScaleVoice = 0x00F65B40, kScaleVoice2 = 0x00F65B4C;  // f32 (name?)
constexpr u32 kTuneSfx = 0x010AEDDC, kTuneMusic = 0x010AEE28;     // f32 (0x010AED94 / 0x010AEDE0 + 72) (name?)
constexpr u32 kTuneVoice = 0x010AED90;                             // f32 (0x010AED48 + 72) (name?)
constexpr u32 kTuneMaster = 0x010AECF8;                            // f32 (0x010AECB0 + 72) (name?)
// Which scale applies to value i (bit 1 << i).
constexpr u32 kMaskSfx = 0x9FFFFFF9u, kMaskMusic = 0x60000004u, kMaskVoice = 0x2u;

// slw: shifts of 32..63 give 0 (only the low 6 bits of the amount count).
inline u32 slw(u32 v, u32 n) {
    n &= 63;
    return n >= 32 ? 0u : v << n;
}
// fsel(a, b, c) then frsp.
inline f32 fsel_rsp(f64 a, f64 b, f64 c) { return static_cast<f32>(a >= 0.0 ? b : c); }
}  // namespace hevent4b
}  // namespace sly4

using namespace sly4::hevent4b;

// void PushStack_Blend(f32 t): unless the blend time is under 1e-5 (|time|),
// the fraction +t/time is added up to 1.0 (reaching it ends the blend: fraction
// 1.0, time 0) and every value becomes from + (target - from) * fraction. Then,
// under the sound lock, the values i = frame & (stride - 1), +stride, ... below
// 31 (stride = 1 << *0x010B0B20) are scaled by their group's scale and tuning,
// clamped to 0..1 into the group volumes and marked dirty; the master tuning
// value clamped to 0..1, when it moved by 1e-5 or more, is stored as the master
// volume (all groups dirty).
DECOMP(00447860, PushStack_Blend) {
    const f64 t = farg(ctx, 0);
    const f64 eps = static_cast<f64>(load<f32>(kEps));
    const f32 time = load<f32>(kPushTime);
    if (!(std::fabs(static_cast<f64>(time)) < eps)) {  // NaN blends
        const f32 step = static_cast<f32>(t / static_cast<f64>(time));  // fdivs
        const f32 el = static_cast<f32>(static_cast<f64>(load<f32>(kPushElapsed)) + static_cast<f64>(step));
        store<f32>(kPushElapsed, el);
        const f32 one = load<f32>(kOneF);
        f64 f = el;
        if (el > one || el == one) {
            store<u32>(kPushElapsed, 0x3F800000u);  // 1.0
            store<u32>(kPushTime, 0);
            f = one;
        }
        for (u32 i = 0; i < kPushValues; ++i) {
            const f32 from = load<f32>(kPushFrom + 8 + i * 8);
            const f32 d = static_cast<f32>(static_cast<f64>(load<f32>(kPushTarget + 8 + i * 8)) - from);
            store<f32>(kPushCur + 8 + i * 8, static_cast<f32>(std::fma(static_cast<f64>(d), f, static_cast<f64>(from))));
        }
    }

    call(ctx, bfn::Snd_Lock);
    const f64 oneF = static_cast<f64>(load<f32>(kOneF));
    const f64 oneD = load<f64>(kOneD), zeroD = load<f64>(kZeroD);
    const u32 stride = slw(1, load<u32>(kGroupStrideShift));
    u32 i = load<u32>(kAudioFrame) & (stride - 1);
    if (static_cast<s32>(i) < 31) {
        const f64 sSfx = load<f32>(kScaleSfx), tSfx = load<f32>(kTuneSfx);
        const f64 sMus = load<f32>(kScaleMusic), tMus = load<f32>(kTuneMusic);
        const f64 sVo = load<f32>(kScaleVoice), tVo = load<f32>(kTuneVoice), sVo2 = load<f32>(kScaleVoice2);
        u32 dirty = load<u32>(kGroupDirty);
        u32 bit = slw(1, i);
        do {
            f64 v = load<f32>(kPushCur + 8 + i * 8);
            if ((bit & kMaskSfx) != 0) v = fmuls(fmuls(sSfx, v), tSfx);
            if ((bit & kMaskMusic) != 0) v = fmuls(fmuls(sMus, v), tMus);
            if ((bit & kMaskVoice) != 0) v = fmuls(sVo2, fmuls(fmuls(sVo, v), tVo));
            const f32 room = static_cast<f32>(oneF - v);
            dirty |= bit;
            store<u32>(kGroupDirty, dirty);
            f32 c = fsel_rsp(room, v, oneD);  // > 1 -> 1 (NaN -> 1)
            c = fsel_rsp(c, c, zeroD);        // < 0 -> 0
            store<f32>(kGroupVolumes + i * 4, c);
            i += stride;
            bit = slw(bit, stride);
        } while (static_cast<s32>(i) < 31);
    }

    const f32 tune = load<f32>(kTuneMaster);
    f32 m = fsel_rsp(static_cast<f32>(oneF - tune), tune, oneD);
    m = fsel_rsp(m, m, zeroD);
    const f32 diff = static_cast<f32>(static_cast<f64>(load<f32>(kMasterLast)) - m);
    if (!(std::fabs(static_cast<f64>(diff)) < eps)) {
        store<f32>(kMasterVolume, m);
        store<u32>(kGroupDirty, 0xFFFFFFFFu);
        store<f32>(kMasterLast, m);
    }
    ctx.fpr[1] = m;  // f1 as the original leaves it for Snd_Unlock
    call(ctx, bfn::Snd_Unlock);
}
