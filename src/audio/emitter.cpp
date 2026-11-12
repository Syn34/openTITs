// Emitter_Start and Emitter_Update: per-emitter sound work for Audio_Update.
#include "audio.h"
#include "snd.h"

namespace sly4::audio {
using namespace opentit::native;

namespace {

constexpr u32 kTuneFadeRate = 0x010B03B0, kTunePitchRate = 0x010B0400, kTuneDoppler = 0x010B0770;
constexpr u32 kForceRefresh = 0x00F65B68;     // u8 (cleared by Audio_Update each frame)
constexpr u32 kOcclusionSource = 0x00F65B64;  // u32 obj (name?)
constexpr u32 kVfnContains = 316;             // obj vtable: (obj, const f32 pos[3]) -> bool

// Constants just before Emitter_Update (0x00442C00).
constexpr u32 kDefaultFade = 0x00442C00;   // 1.0f
constexpr u32 kDefaultPitch = 0x00442C04;  // 23999.0f
constexpr u32 kZeroWeight = 0x00442C08;    // 0.0f

// The occlusion source: +8 fade, +12 pitch, +112 the zone object.
struct OcclusionSource {
    pad<8> _0;
    be_f32 fade;   // +8
    be_f32 pitch;  // +12
    pad<96> _16;
    be_u32 obj;    // +112
};
f32 tune(u32 obj) { return gref<be_f32>(obj + kTuningValue); }

// Moves *cur towards target by rate * dt, without overshooting.
f32 Approach(be_f32& cur, f32 target, f32 rate, f64 dt) {
    f32 v = cur;
    if (v < target) {
        v = fmadds(rate, dt, v);
        cur = v;
        if (v >= target) {
            v = target;
            cur = target;
        }
    }
    if (v > target) {
        v = fnmsubs(rate, dt, v);
        cur = v;
        if (v <= target) {
            v = target;
            cur = target;
        }
    }
    return v;
}

}  // namespace

// bool Emitter_Start(u32 handle)  (name?)
// Starts the voice of a virtual emitter again: builds its play
// parameters from the 3D block, plays the sound and, if it plays, clears
// the virtual flag. Returns whether it did.
bool Emitter_Start(u32 handle) {
    Emitter* e = Emitter_Of(handle);
    if (!e || e->is_virtual == 0) return false;

    GuestFrame frame(sizeof(VoiceParams));
    VoiceParams* params = frame.as<VoiceParams>();
    call(fn::memset, params, 0u, static_cast<u32>(sizeof(VoiceParams)));
    params->size = static_cast<u32>(sizeof(VoiceParams));
    Emitter_BuildParams(params, &e->base, e->def, nullptr);
    call(afn::memmove, &e->base, &e->cur, static_cast<u32>(sizeof(Block)));
    SoundDef& def = gref(e->def);
    const u32 bank = gref(def.bank);
    const u64 voice = snd::Voice_Play(bank, static_cast<s16>(def.id & 0xFFFF), guest(params), static_cast<u64>(-2));
    e->voice = static_cast<u32>(voice);
    if (Voice_IsPlaying(static_cast<u32>(voice)) == 0) return false;
    e->is_virtual = 0;
    return true;
}

// void Emitter_Update(Emitter* e, Listener* l, -, bool listener_still, f32 dt)  (name?)
//
// Spatialises the emitter, works out a Doppler pitch code when it (or the
// listener) moves, picks fade/pitch targets from the occlusion source,
// then eases fade and pitch towards their targets and computes the final
// volume and pitch.
void Emitter_Update(Emitter* ep, const be_f32* listener, u32, u8 still, f64 dt) {
    Emitter& e = gref(ep);
    Emitter_Spatialise(&e.cur, listener, e.position, e.velocity, e.def, e.zone, e.shape, 0, 0);

    if (e.doppler != 0 && (e.forced != 0 || e.moving != 0 || still != 0)) {
        const be_f32* lp = &gref(listener) + 12;
        const f32 dy = fsubs(lp[1], e.position[1]);
        const f32 dx = fsubs(lp[0], e.position[0]);
        const f32 dz = fsubs(lp[2], e.position[2]);
        const f32 d2 = fmadds(dz, dz, fmadds(dx, dx, fmuls(dy, dy)));
        const f32 vdot = fmadds(dz, e.velocity[2], fmadds(dx, e.velocity[0], fmuls(dy, e.velocity[1])));
        const f32 dist = fsqrts(d2);
        const f32 v = fmuls(fdivs(vdot, dist), tune(kTuneDoppler));
        e.cur.pitch_code = static_cast<u32>(snd::Snd_PitchFromSpeed(v));
    }

    if (e.forced != 0 || global<u8>(kForceRefresh) != 0) {
        f32 fade = global<be_f32>(kDefaultFade), pitch = global<be_f32>(kDefaultPitch);
        const u32 source = global<be_u32>(kOcclusionSource);
        const OcclusionSource& src = gref<OcclusionSource>(source);
        const u32 obj = source != 0 ? src.obj.get() : 0;
        if (obj != 0) {
            const f32 sf = src.fade, sp = src.pitch;
            if (sf < fade || sp < pitch) {
                f32 weight = global<be_f32>(kZeroWeight);
                const bool zoned = gref<u8>(obj + 612) != 0;
                const u32 mode = zoned ? gref<be_u32>(obj + 428).get() : 0;
                if (zoned && (mode == 0 || mode == 3)) {
                    const f32 occ = call<f32>(afn::Obj_GetOcclusion, obj, e.position);
                    weight = fsubs(fade, occ);
                } else if (call_ptr<u32>(vfunc(obj, kVfnContains), obj, e.position) == 0) {
                    weight = fade;
                }
                const f32 df = fsubs(sf, fade), dp = fsubs(sp, pitch);
                fade = fmadds(df, weight, fade);
                pitch = fmadds(dp, weight, pitch);
            }
        }
        e.fade_target = fade;
        e.pitch_target = pitch;
    }

    const f32 fade = Approach(e.fade_cur, e.fade_target, tune(kTuneFadeRate), dt);
    const f32 pitch = Approach(e.pitch_cur, e.pitch_target, tune(kTunePitchRate), dt);
    const f32 gain = fmuls(e.cur.gain, e.cur.scale);
    const f32 pitch_max = e.cur.pitch_target;
    const f32 over = fsubs(pitch, pitch_max);
    e.cur.fade = fade;
    e.cur.pitch_cap = pitch;
    e.cur.volume = fmuls(gain, fade);
    e.cur.pitch = static_cast<f32>(fsel(over, pitch_max, pitch));
}

}  // namespace sly4::audio

NATIVE(0044448C, sly4::audio::Emitter_Start)
NATIVE(00442C10, sly4::audio::Emitter_Update)
