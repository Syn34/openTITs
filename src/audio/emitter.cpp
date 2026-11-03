// Emitter_Start and Emitter_Update: per-emitter sound work for Audio_Update.
#include "audio.h"

using namespace sly4;
using namespace sly4::audio;

namespace {

// Emitter fields (an emitter is pool node + 16).
constexpr u32 kEmVoice = 0, kEmDef = 4, kEmBank = 8;
constexpr u32 kEmVirtual = 13, kEmDoppler = 14;
constexpr u32 kEmVolume = 76;      // f32 final volume; also the start of the 3D block (76..132)
constexpr u32 kEmGain = 80, kEmGainScale = 84, kEmFade = 88;
constexpr u32 kEmPitchMax = 104, kEmPitch = 100, kEmPitchOut = 108;
constexpr u32 kEmPitchCode = 112;  // u32
constexpr u32 kEmSpatial = 136;    // u32
constexpr u32 kEmFadeCur = 140, kEmFadeTarget = 144;
constexpr u32 kEmPitchCur = 148, kEmPitchTarget = 152;
constexpr u32 kEmPosition = 176;   // f32[3]
constexpr u32 kEmForced = 192;     // u8
constexpr u32 kEmVelocity = 208;   // f32[3]
constexpr u32 kEmMoving = 224;     // u8
constexpr u32 kParamsSize = 220;
constexpr u32 k3DBlockSize = 57;
constexpr u32 kDefBankObj = 4, kDefSoundId = 84;

constexpr u32 kTuneFadeRate = 0x010B03B0, kTunePitchRate = 0x010B0400, kTuneDoppler = 0x010B0770;
constexpr u32 kForceRefresh = 0x00F65B68;  // u8 (cleared by Audio_Update each frame)
constexpr u32 kOcclusionSource = 0x00F65B64;  // u32 obj (name?)

// Constants just before Emitter_Update (0x00442C00).
constexpr u32 kDefaultFade = 0x00442C00;   // 1.0f
constexpr u32 kDefaultPitch = 0x00442C04;  // 23999.0f
constexpr u32 kZeroWeight = 0x00442C08;    // 0.0f

// Moves *cur towards target by rate * dt, without overshooting. Returns
// the value as the original leaves it in its register.
f32 Approach(u32 cur_ea, f32 target, f32 rate, f64 dt) {
    f32 v = load<f32>(cur_ea);
    if (v < target) {
        v = fmadds(rate, dt, v);
        store<f32>(cur_ea, v);
        if (v >= target) {
            v = target;
            store<f32>(cur_ea, target);
        }
    }
    if (v > target) {
        v = fnmsubs(rate, dt, v);
        store<f32>(cur_ea, v);
        if (v <= target) {
            v = target;
            store<f32>(cur_ea, target);
        }
    }
    return v;
}

}  // namespace

// bool Emitter_Start(u32 handle)  (name?)
// Starts the voice of a virtual emitter again: builds its play
// parameters from the 3D block, plays the sound and, if it plays, clears
// the virtual flag. Returns whether it did.
DECOMP(0044448C, Emitter_Start) {
    const u32 emitter = Emitter_FromHandle(arg<u32>(ctx, 0));
    if (emitter == 0 || ref<u8>(emitter + kEmVirtual) == 0) return ret(ctx, 0u);

    Stack frame(ctx, kParamsSize);
    const u32 params = frame.addr();
    call(ctx, fn::memset, {params, 0, kParamsSize});
    store<u32>(params, kParamsSize);
    const u32 block = emitter + 16;
    call(ctx, afn::Emitter_BuildParams, {params, block, field<u32>(emitter, kEmDef), 0});
    call(ctx, afn::memmove, {block, emitter + kEmVolume, k3DBlockSize});
    const u32 def = field<u32>(emitter, kEmDef);
    const u32 bank = load<u32>(field<u32>(def, kDefBankObj));
    const s64 id = static_cast<s16>(field<u32>(def, kDefSoundId) & 0xFFFF);
    const u64 voice = call(ctx, afn::Voice_Play, {bank, static_cast<u64>(id), params, static_cast<u64>(-2)});
    field<u32>(emitter, kEmVoice) = static_cast<u32>(voice);
    if (static_cast<u32>(call(ctx, afn::Voice_IsPlaying, {voice})) == 0) return ret(ctx, 0u);
    ref<u8>(emitter + kEmVirtual) = 0;
    ret(ctx, 1u);
}

// void Emitter_Update(Emitter* e, Listener* l, -, bool listener_still, f32 dt)  (name?)
//
// Spatialises the emitter, works out a Doppler pitch code when it (or the
// listener) moves, picks fade/pitch targets from the occlusion source,
// then eases fade and pitch towards their targets and computes the final
// volume and pitch.
DECOMP(00442C10, Emitter_Update) {
    const u32 e = arg<u32>(ctx, 0), listener = arg<u32>(ctx, 1);
    const u32 still = arg<u32>(ctx, 3) & 0xFF;
    const f64 dt = farg(ctx, 0);
    const u32 pos = e + kEmPosition;

    ctx.fpr[1] = dt;
    call(ctx, afn::Emitter_Spatialise,
         {e + kEmVolume, listener, pos, e + kEmVelocity, field<u32>(e, kEmDef), field<u32>(e, kEmBank),
          field<u32>(e, kEmSpatial), 0});

    if (ref<u8>(e + kEmDoppler) != 0 &&
        (ref<u8>(e + kEmForced) != 0 || ref<u8>(e + kEmMoving) != 0 || still != 0)) {
        const u32 lp = listener + 48;
        const f32 dy = fsubs(load<f32>(lp + 4), field<f32>(e, kEmPosition + 4));
        const f32 dx = fsubs(load<f32>(lp), field<f32>(e, kEmPosition));
        const f32 dz = fsubs(load<f32>(lp + 8), field<f32>(e, kEmPosition + 8));
        const f32 d2 = fmadds(dz, dz, fmadds(dx, dx, fmuls(dy, dy)));
        const f32 vdot = fmadds(dz, field<f32>(e, kEmVelocity + 8),
                                fmadds(dx, field<f32>(e, kEmVelocity), fmuls(dy, field<f32>(e, kEmVelocity + 4))));
        const f32 dist = fsqrts(d2);
        const f32 v = fmuls(fdivs(vdot, dist), field<f32>(kTuneDoppler, kTuningValue));
        ctx.fpr[1] = v;
        field<u32>(e, kEmPitchCode) = static_cast<u32>(call(ctx, afn::Voice_SetPitch));
    }

    if (ref<u8>(e + kEmForced) != 0 || ref<u8>(kForceRefresh) != 0) {
        f32 fade = load<f32>(kDefaultFade), pitch = load<f32>(kDefaultPitch);
        const u32 source = load<u32>(kOcclusionSource);
        const u32 obj = source != 0 ? load<u32>(source + 112) : 0;
        if (obj != 0) {
            const f32 sf = load<f32>(source + 8), sp = load<f32>(source + 12);
            if (sf < fade || sp < pitch) {
                f32 weight = load<f32>(kZeroWeight);
                const bool zoned = ref<u8>(obj + 612) != 0;
                const u32 mode = zoned ? load<u32>(obj + 428) : 0;
                if (zoned && (mode == 0 || mode == 3)) {
                    const f64 occ = callf(ctx, afn::Obj_GetOcclusion, {obj, pos}, {});
                    weight = fsubs(fade, occ);
                } else if (static_cast<u32>(call_ptr(ctx, vfunc(obj, 316), {obj, pos})) == 0) {
                    weight = fade;
                }
                const f32 df = fsubs(sf, fade), dp = fsubs(sp, pitch);
                fade = fmadds(df, weight, fade);
                pitch = fmadds(dp, weight, pitch);
            }
        }
        field<f32>(e, kEmFadeTarget) = fade;
        field<f32>(e, kEmPitchTarget) = pitch;
    }

    const f32 fade = Approach(e + kEmFadeCur, field<f32>(e, kEmFadeTarget), field<f32>(kTuneFadeRate, kTuningValue), dt);
    const f32 pitch =
        Approach(e + kEmPitchCur, field<f32>(e, kEmPitchTarget), field<f32>(kTunePitchRate, kTuningValue), dt);
    const f32 gain = fmuls(field<f32>(e, kEmGain), field<f32>(e, kEmGainScale));
    const f32 pitch_max = field<f32>(e, kEmPitchMax);
    const f32 over = fsubs(pitch, pitch_max);
    field<f32>(e, kEmFade) = fade;
    field<f32>(e, kEmPitchOut) = pitch;
    field<f32>(e, kEmVolume) = fmuls(gain, fade);
    field<f32>(e, kEmPitch) = static_cast<f32>(fsel(over, pitch_max, pitch));
}
