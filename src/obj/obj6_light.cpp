// World objects, part 6: registering a light instance's animation record
// (the flicker / pulse / random intensity animation of 0x01194EC0).
#include <cmath>

#include "obj6.h"

using namespace sly4;
using namespace sly4::obj4;
using namespace sly4::obj6;

namespace {

// The game's random generator step, inlined in the original: returns the
// previous kRandomLast (64-bit, as s64 -> double -> float) and steps the
// state (state = rotl16(mix), last = mix & 0xFFFF).
f32 RandomPrev() {
    const auto rotl = [](u32 v, int n) { return (v << n) | (v >> (32 - n)); };
    const u64 last = load<u64>(kRandomLast);
    const u32 s = load<u32>(kRandomState);
    const u32 mix = (rotl(s, 4) + rotl(s, 16)) ^ kRandomMix;
    store<u32>(kRandomState, rotl(mix, 16));
    store<u64>(kRandomLast, mix & 0xFFFFu);
    return static_cast<f32>(static_cast<f64>(static_cast<s64>(last)));
}

inline f32 C(u32 off) { return load<f32>(kLaConsts + off); }
inline f32 Fmadds(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(c), static_cast<f64>(b))); }
inline f32 Fmsubs(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(c), -static_cast<f64>(b))); }
inline f32 Fnmsubs(f32 a, f32 c, f32 b) { return static_cast<f32>(-std::fma(static_cast<f64>(a), static_cast<f64>(c), -static_cast<f64>(b))); }

}  // namespace

// void LightInstance_Register(LightInstance* l)
// When 0x00591948(l, 0) accepts the light: marks its root dirty (the
// inlined WorldObj_MarkDirty), updates the root's LOD (0x0057EE00), and
// appends an 80-byte animation record (the next one of *0x01194EC0; the
// light keeps its index at +382): defaults 25 / 625 / 400, two random
// values at +52 / +56, then 0x00591820(record, l) fills it in. The record
// is then advanced once by its type (+40): 3 clamps the phase (+32) to the
// period (+36), 0 wraps it (sawtooth), 1 bounces it (direction at +44), 2
// wraps it and draws a new random target when it passes the period. The
// intensity = sqrt(max(0, base (+60) + amplitude (+64) * shape(phase) +
// noise)) (the shape: linear, a parabolic sine, or the random values
// blended; the noise (rand^2 - 0.5) * 2 * +48); only base when the
// period is not positive. A changed intensity goes to the owner's (+68)
// +376 and its Changed vfunc (+36); non-spot owners (+368 != 2) first copy
// config +96 to +108.
DECOMP(005A2528, LightInstance_Register) {
    const u32 l = arg<u32>(ctx, 0);
    if (static_cast<u32>(call(ctx, o6fn::Light_Accepts, {l, 0})) == 0) return;
    MarkDirtyInline(ctx, load<u32>(l + kObjRoot));
    call(ctx, o4fn::WorldObj_UpdateLod, {load<u32>(l + kObjRoot), 0, 0});
    const u32 recs = load<u32>(load<u32>(cls::kLightRecs));
    u32 count = load<u16>(recs + kLrCount);
    u32 base = load<u32>(recs);
    store<u16>(l + cls::kLightRec, static_cast<u16>(count));
    const u32 rec0 = base + count * cls::kLightRecSize;
    if (rec0 != 0) {
        const u8 b12 = load<u8>(rec0 + kLaFlags);
        store<u32>(rec0 + kLaType, 0);
        store<u32>(rec0 + 8, kLa400);
        store<u32>(rec0 + 0, kLa25);
        store<u8>(rec0 + kLaFlags, static_cast<u8>(b12 & 0x7F));
        store<u32>(rec0 + 4, kLa625);
        store<u32>(rec0 + kLaPhase, 0);
        store<u32>(rec0 + kLaPeriod, 0);
        store<u8>(rec0 + kLaDir, 0);
        store<u32>(rec0 + kLaNoise, 0);
        const f32 r1 = RandomPrev(), r2 = RandomPrev();
        store<u32>(rec0 + kLaBase, 0);
        store<u32>(rec0 + kLaAmp, 0);
        store<f32>(rec0 + kLaRandA, r1 * C(kLcInv65535));
        store<u32>(rec0 + kLaOwner, 0);
        store<f32>(rec0 + kLaRandB, r2 * C(kLcInv65535));
        call(ctx, o6fn::LightAnim_Init, {rec0, l});
        count = load<u16>(recs + kLrCount);
        base = load<u32>(recs);
    }
    const u32 n = count + 1;
    store<u16>(recs + kLrCount, static_cast<u16>(n));
    const u32 rec = base + (n & 0xFFFF) * cls::kLightRecSize - cls::kLightRecSize;
    const s32 type = static_cast<s32>(load<u32>(rec + kLaType));
    const f32 zero = C(kLcZero);
    if (type == 3) {
        const f32 period = load<f32>(rec + kLaPeriod);
        if (!(period <= zero) && load<f32>(rec + kLaPhase) > period) store<f32>(rec + kLaPhase, period);
    } else if (type == 0) {
        const f32 period = load<f32>(rec + kLaPeriod);
        if (!(period <= zero)) {
            f32 p = load<f32>(rec + kLaPhase);
            while (p > period) {
                p = p - period;
                store<f32>(rec + kLaPhase, p);
            }
        }
    } else if (type == 1) {
        const f32 period = load<f32>(rec + kLaPeriod);
        if (!(period <= zero)) {
            if (load<u8>(rec + kLaDir) != 0) {
                const f32 p = load<f32>(rec + kLaPhase);
                if (p < zero) {
                    store<u8>(rec + kLaDir, 0);
                    store<f32>(rec + kLaPhase, -p);
                }
            } else {
                f32 p = load<f32>(rec + kLaPhase);
                if (p > period) {
                    do {
                        p = p - period;
                        store<f32>(rec + kLaPhase, p);
                    } while (p > period);
                    store<u8>(rec + kLaDir, 1);
                    store<f32>(rec + kLaPhase, period - p);
                }
            }
        }
    } else if (type == 2) {
        const f32 period = load<f32>(rec + kLaPeriod);
        if (!(period <= zero)) {
            f32 p = load<f32>(rec + kLaPhase);
            if (p > period) {
                store<u32>(rec + kLaRandA, load<u32>(rec + kLaRandB));
                store<f32>(rec + kLaRandB, RandomPrev() * C(kLcInv65535));
            }
            while (p > period) {
                p = p - period;
                store<f32>(rec + kLaPhase, p);
            }
        }
    }
    // The intensity.
    const f32 rnd = RandomPrev();
    f32 phase = load<f32>(rec + kLaPhase);
    f32 v = load<f32>(rec + kLaBase);
    const f32 half = C(kLcHalf);
    f32 q = rnd * C(kLcInv65535);
    q = Fmsubs(q, q, half);
    f32 noise = q * C(kLcTwo);
    noise = noise * load<f32>(rec + kLaNoise);
    if (phase < zero) phase = zero;
    const f32 period = load<f32>(rec + kLaPeriod);
    if (phase > period) phase = period;
    if (period > zero) {
        if (type == 1) {
            const f32 twopi = C(kLcTwoPi);
            const f32 x = (phase * twopi) / period;
            const f32 pi = static_cast<f32>(x >= 0.0f ? load<f64>(kLaConsts + kLcPiD) : load<f64>(kLaConsts + kLcNegPiD));
            f32 a = pi + x;
            const f32 k = static_cast<f32>(rt::fcfid(rt::fctid(static_cast<f64>(a / twopi), true)));
            a = Fnmsubs(k, twopi, a);
            a = a - pi;
            a = a + C(kLcHalfPi);
            if (a > C(kLcPi)) a = a - twopi;
            // The parabolic sine of a, refined once.
            f32 s = (a * C(kLcNeg4OverPi2)) * std::fabs(a);
            s = Fmadds(a, C(kLc4OverPi), s);
            const f32 t = Fmsubs(s, std::fabs(s), s);
            s = Fmadds(t, C(kLcSinP), s);
            s = s + C(kLcOne);
            v = Fmadds(load<f32>(rec + kLaAmp) * s, half, v);
        } else if (type == 2) {
            const f32 x = phase / period;
            const f32 w = (C(kLcOne) - x) * load<f32>(rec + kLaRandA);
            noise = Fmadds(x, load<f32>(rec + kLaRandB), w) * load<f32>(rec + kLaAmp);
        } else {
            v = v + (load<f32>(rec + kLaAmp) * phase) / period;
        }
        v = v + noise;
        if (v < zero) v = zero;
    }
    v = static_cast<f32>(std::sqrt(static_cast<f64>(v)));
    const u32 owner = load<u32>(rec + kLaOwner);
    if (load<f32>(owner + cls::kLightRangeSq) == v) return;
    const s32 otype = static_cast<s32>(load<u32>(owner + cls::kLightType));
    store<f32>(owner + cls::kLightRangeSq, v);
    if (otype != 2) store<u32>(cls::kConfigLights + 108, load<u32>(cls::kConfigLights + 96));
    Vcall0(ctx, owner, kVfnChanged);
}
