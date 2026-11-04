// Emitter_BuildParams: the play-parameter block for an emitter's voice.
#include "audio.h"

using namespace sly4;
using namespace sly4::audio;

namespace {

constexpr u32 kRandomize = 0x0071B054;  // (s32 range) -> s32 (name?)
constexpr u32 kTuneNoRolloff = 0x010B09E8;  // u8 at +72
constexpr u32 kTuneDoppler = 0x010B0360;    // f32 at +72
constexpr u32 kTuneType30A = 0x010B04F0, kTuneType30B = 0x010B0540;  // f32 at +72
constexpr u32 kTuneType30C = 0x010B0450, kTuneType30D = 0x010B04A0;
constexpr u32 kFloatOneBits = 0x3F800000u;
constexpr u32 kDefType = 100;   // s32 SoundDef +100: 30 = ... (name?)
constexpr u32 kDefRange = 36;   // f32 SoundDef +36

// Constants before the function (0x00441D58).
constexpr f32 kEps = std::bit_cast<f32>(0x3727C5ACu);
constexpr f32 kZero = 0.0f, k127 = 127.0f;

f32 tune(u32 obj) { return field<f32>(obj, kTuningValue).get(); }
s32 low_word_fctiwz(f64 x) { return static_cast<s32>(static_cast<u32>(rt::fctiw(x, true))); }

// (rand(range) / 90) * 360, with C truncating division (mulhw magic).
u32 Random_Angle(PPUContext& ctx, s32 range) {
    const s32 r = static_cast<s32>(call(ctx, kRandomize, {static_cast<u64>(static_cast<s64>(range))}));
    return static_cast<u32>(r / 90) * 360u;
}

// The distance-to-127 step: v = clamp(int(dist / range), .., 127); when
// range > 0 and v >= 0 the byte at params+37 is set and flag 0x20000 added.
// dist >= range * 127 gives 127 directly.
void Distance_Step(u32 params, u32& flags, f32 dist, f32 range) {
    if (range <= kZero) return;
    s32 v = 127;
    if (dist < fmuls(range, k127)) {
        v = low_word_fctiwz(fdivs(dist, range));
        if (v > 127) v = 127;
        if (v < 0) return;
    }
    ref<u8>(params + 37) = static_cast<u8>(v);
    flags |= 0x20000;
}

bool Differs(f32 a, f32 b) { return !(std::fabs(fsubs(a, b)) < kEps); }

}  // namespace

// void Emitter_BuildParams(Params* p, Emitter3D* cur, SoundDef* def, Emitter3D* prev)  (name?)
//
// prev == 0: fills a fresh block from cur (and picks cur's random angle the
// first time). Otherwise sets the changed-field bits in p+4 (and p+40) for
// every value that differs between cur and prev, copying prev's values in.
DECOMP(00441D68, Emitter_BuildParams) {
    const u32 p = arg<u32>(ctx, 0), cur = arg<u32>(ctx, 1), def = arg<u32>(ctx, 2), prev = arg<u32>(ctx, 3);

    if (prev == 0) {
        const f32 x0 = field<f32>(cur, 0), x20 = field<f32>(cur, 20);
        field<f32>(p, 16) = x0;
        field<u32>(p, 20) = field<u32>(cur, 44);
        u32 angle = field<u32>(cur, 52);
        if (ref<u8>(cur + 56) == 0) {
            angle = Random_Angle(ctx, field<s32>(cur, 48));
            field<u32>(cur, 52) = angle;
        }
        field<u32>(p, 24) = angle;
        const f32 x16 = field<f32>(cur, 16);
        u32 mask = 0x280;
        u32 flags = field<u32>(p, 4);
        field<f32>(p, 144) = x16;
        field<f32>(p, 164) = x16;
        if (field<u8>(kTuneNoRolloff, kTuningValue) == 0) {
            field<f32>(p, 120) = x20;
            field<u32>(p, 132) = 0;
            field<f32>(p, 124) = x20;
            field<u32>(p, 136) = 1;
            field<f32>(p, 128) = x20;
            field<u32>(p, 140) = 2;
            mask = 0x2E0;
        }
        field<u32>(p, 40) = mask | 2;
        field<u32>(p, 60) = 1;
        field<f32>(p, 68) = field<f32>(cur, 24);
        field<f32>(p, 72) = tune(kTuneDoppler);
        if (field<s32>(def, kDefType) == 30) {
            field<u32>(p, 80) = kFloatOneBits;
            field<f32>(p, 84) = tune(kTuneType30A);
            const f32 b = tune(kTuneType30B);
            field<u32>(p, 76) = 1;
            field<f32>(p, 88) = b;
            field<u32>(p, 96) = kFloatOneBits;
            const f32 c = tune(kTuneType30C);
            field<u32>(p, 40) = mask | 0xE;
            field<u32>(p, 92) = 2;
            field<f32>(p, 100) = c;
            field<f32>(p, 104) = tune(kTuneType30D);
        }
        flags |= 0x3C;
        Distance_Step(p, flags, field<f32>(cur, 40), field<f32>(def, kDefRange));
        field<u32>(p, 4) = flags | 1;
        field<u32>(p, 28) = field<u32>(cur, 36);
        return;
    }

    if (Differs(field<f32>(cur, 0), field<f32>(prev, 0))) {
        const u32 f = field<u32>(p, 4);
        field<f32>(p, 16) = field<f32>(prev, 0);
        field<u32>(p, 4) = f | 4;
    }
    if (Differs(field<f32>(cur, 16), field<f32>(prev, 16))) {
        const f32 v = field<f32>(prev, 16);
        const u32 m = field<u32>(p, 40), f = field<u32>(p, 4);
        field<f32>(p, 144) = v;
        field<f32>(p, 164) = v;
        field<u32>(p, 40) = m | 0x280;
        field<u32>(p, 4) = f | 0x20;
    }
    if (field<u8>(kTuneNoRolloff, kTuningValue) == 0 && Differs(field<f32>(cur, 20), field<f32>(prev, 20))) {
        const f32 v = field<f32>(prev, 20);
        const u32 m = field<u32>(p, 40), f = field<u32>(p, 4);
        field<f32>(p, 120) = v;
        field<f32>(p, 124) = v;
        field<f32>(p, 128) = v;
        field<u32>(p, 40) = m | 0x20;
        field<u32>(p, 4) = f | 0x20;
    }
    if (field<u32>(cur, 44) != field<u32>(prev, 44)) {
        field<u32>(cur, 44) = field<u32>(prev, 44);
        const u32 f = field<u32>(p, 4);
        field<u32>(p, 20) = field<u32>(prev, 44);
        field<u32>(p, 4) = f | 8;
    }

    const u8 fixed_angle = ref<u8>(prev + 56);
    const u32 angle = field<u32>(cur, 52);
    ref<u8>(cur + 56) = fixed_angle;
    u32 wanted = angle;
    if (fixed_angle != 0) {
        wanted = field<u32>(prev, 52);
    } else if (field<s32>(cur, 48) != field<s32>(prev, 48)) {
        field<s32>(cur, 48) = field<s32>(prev, 48);
        wanted = Random_Angle(ctx, field<s32>(prev, 48));
    }
    const u32 now = field<u32>(cur, 52);
    if (now != wanted) {
        field<u32>(cur, 52) = wanted;
        const u32 f = field<u32>(p, 4);
        field<u32>(p, 24) = wanted;
        field<u32>(p, 4) = f | 0x10;
    }

    if (Differs(field<f32>(cur, 24), field<f32>(prev, 24))) {
        const f32 v = field<f32>(prev, 24);
        const u32 f = field<u32>(p, 4), m = field<u32>(p, 40);
        field<f32>(p, 68) = v;
        field<u32>(p, 60) = 1;
        field<u32>(p, 4) = f | 0x20;
        field<u32>(p, 40) = m | 2;
        field<f32>(p, 72) = tune(kTuneDoppler);
    }
    const f32 dist = field<f32>(cur, 40);
    if (Differs(dist, field<f32>(prev, 40))) {
        u32 f = field<u32>(p, 4);
        const u32 before = f;
        Distance_Step(p, f, dist, field<f32>(def, kDefRange));
        if (f != before) field<u32>(p, 4) = f;
    }
    if (field<u32>(cur, 36) != field<u32>(prev, 36)) {
        const u32 f = field<u32>(p, 4);
        field<u32>(p, 28) = field<u32>(prev, 36);
        field<u32>(p, 4) = f | 1;
    }
}
