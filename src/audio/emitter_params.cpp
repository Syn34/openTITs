// Emitter_BuildParams: the play-parameter block for an emitter's voice.
#include "audio.h"
#include "snd.h"

namespace sly4::audio {
using namespace opentit::native;

namespace {

constexpr u32 kTuneNoRolloff = 0x010B09E8;  // u8 at +72
constexpr u32 kTuneDoppler = 0x010B0360;    // f32 at +72
constexpr u32 kTuneType30A = 0x010B04F0, kTuneType30B = 0x010B0540;  // f32 at +72
constexpr u32 kTuneType30C = 0x010B0450, kTuneType30D = 0x010B04A0;
constexpr u32 kFloatOneBits = 0x3F800000u;

// Constants before the function (0x00441D58).
constexpr f32 kEps = std::bit_cast<f32>(0x3727C5ACu);
constexpr f32 kZero = 0.0f, k127 = 127.0f;

f32 tune(u32 obj) { return gref<be_f32>(obj + kTuningValue); }
s32 low_word_fctiwz(f64 x) { return static_cast<s32>(static_cast<u32>(rt::fctiw(x, true))); }
f32 one() { return std::bit_cast<f32>(kFloatOneBits); }

// (abs(range) / 90) * 360, with C truncating division (mulhw magic).
u32 Spread_Angle(s32 range) { return static_cast<u32>(snd::Int_Abs(range) / 90) * 360u; }

// The distance-to-127 step: v = clamp(int(dist / range), .., 127); when
// range > 0 and v >= 0 the byte at params+37 is set and flag 0x20000 added.
// dist >= range * 127 gives 127 directly.
void Distance_Step(VoiceParams& p, u32& flags, f32 dist, f32 range) {
    if (range <= kZero) return;
    s32 v = 127;
    if (dist < fmuls(range, k127)) {
        v = low_word_fctiwz(fdivs(dist, range));
        if (v > 127) v = 127;
        if (v < 0) return;
    }
    p.params[1] = static_cast<u8>(v);
    flags |= 0x20000;
}

bool Differs(f32 a, f32 b) { return !(std::fabs(fsubs(a, b)) < kEps); }

}  // namespace

// void Emitter_BuildParams(Params* p, Emitter3D* cur, SoundDef* def, Emitter3D* prev)  (name?)
//
// prev == 0: fills a fresh block from cur (and picks cur's random angle the
// first time). Otherwise sets the changed-field bits in p+4 (and p+40) for
// every value that differs between cur and prev, copying prev's values in.
void Emitter_BuildParams(VoiceParams* pp, Block* cp, SoundDef* dp, Block* prevp) {
    VoiceParams& p = gref(pp);
    Block& cur = gref(cp);
    SoundDef& def = gref(dp);
    const bool no_rolloff = gref<u8>(kTuneNoRolloff + kTuningValue) != 0;

    if (!prevp) {
        const f32 x0 = cur.volume, x20 = cur.vol2;
        p.gain = x0;
        p.angle = cur.pan.get();
        u32 angle = cur.angle;
        if (cur.fixed_angle == 0) {
            angle = Spread_Angle(cur.elevation);
            cur.angle = angle;
        }
        p.spread = angle;
        const f32 x16 = cur.vol1;
        u32 mask = 0x280;
        u32 flags = p.mask;
        p.f144 = x16;
        p.f164 = x16;
        if (!no_rolloff) {
            p.f120 = x20;
            p.w132 = 0u;
            p.f124 = x20;
            p.w136 = 1u;
            p.f128 = x20;
            p.w140 = 2u;
            mask = 0x2E0;
        }
        p.fx_mask = mask | 2;
        p.w60 = 1u;
        p.f68 = cur.pitch.get();
        p.f72 = tune(kTuneDoppler);
        if (def.type == 30) {
            p.f80 = one();
            p.f84 = tune(kTuneType30A);
            const f32 b = tune(kTuneType30B);
            p.w76 = 1u;
            p.f88 = b;
            p.f96 = one();
            const f32 c = tune(kTuneType30C);
            p.fx_mask = mask | 0xE;
            p.w92 = 2u;
            p.f100 = c;
            p.f104 = tune(kTuneType30D);
        }
        flags |= 0x3C;
        Distance_Step(p, flags, cur.distance, def.range);
        p.mask = flags | 1;
        p.param = cur.pitch_code.get();
        return;
    }
    Block& prev = *prevp;

    if (Differs(cur.volume, prev.volume)) {
        const u32 f = p.mask;
        p.gain = prev.volume.get();
        p.mask = f | 4;
    }
    if (Differs(cur.vol1, prev.vol1)) {
        const f32 v = prev.vol1;
        const u32 m = p.fx_mask, f = p.mask;
        p.f144 = v;
        p.f164 = v;
        p.fx_mask = m | 0x280;
        p.mask = f | 0x20;
    }
    if (!no_rolloff && Differs(cur.vol2, prev.vol2)) {
        const f32 v = prev.vol2;
        const u32 m = p.fx_mask, f = p.mask;
        p.f120 = v;
        p.f124 = v;
        p.f128 = v;
        p.fx_mask = m | 0x20;
        p.mask = f | 0x20;
    }
    if (cur.pan != prev.pan) {
        cur.pan = prev.pan.get();
        const u32 f = p.mask;
        p.angle = prev.pan.get();
        p.mask = f | 8;
    }

    const u8 fixed_angle = prev.fixed_angle;
    const u32 angle = cur.angle;
    cur.fixed_angle = fixed_angle;
    u32 wanted = angle;
    if (fixed_angle != 0) {
        wanted = prev.angle;
    } else if (cur.elevation != prev.elevation) {
        cur.elevation = prev.elevation.get();
        wanted = Spread_Angle(prev.elevation);
    }
    if (cur.angle != wanted) {
        cur.angle = wanted;
        const u32 f = p.mask;
        p.spread = wanted;
        p.mask = f | 0x10;
    }

    if (Differs(cur.pitch, prev.pitch)) {
        const f32 v = prev.pitch;
        const u32 f = p.mask, m = p.fx_mask;
        p.f68 = v;
        p.w60 = 1u;
        p.mask = f | 0x20;
        p.fx_mask = m | 2;
        p.f72 = tune(kTuneDoppler);
    }
    const f32 dist = cur.distance;
    if (Differs(dist, prev.distance)) {
        u32 f = p.mask;
        const u32 before = f;
        Distance_Step(p, f, dist, def.range);
        if (f != before) p.mask = f;
    }
    if (cur.pitch_code != prev.pitch_code) {
        const u32 f = p.mask;
        p.param = prev.pitch_code.get();
        p.mask = f | 1;
    }
}

}  // namespace sly4::audio

NATIVE(00441D68, sly4::audio::Emitter_BuildParams)
