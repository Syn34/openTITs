// Emitter_Spatialise: distance attenuation, occlusion, panning and Doppler
// for an emitter's 3D block. Every float step mirrors the recompiler's
// semantics (single ops round to float; fsub/fmul/fmadd stay double).
#include <bit>
#include <cmath>

#include "audio.h"
#include "snd.h"
#include "../math/math.h"

namespace sly4::audio {
using namespace opentit::native;

namespace {

// Constants before the function (0x00442420).
constexpr u32 K = 0x00442420;
f32 kf(u32 off) { return global<be_f32>(K + off); }
f64 kd(u32 off) { return std::bit_cast<f64>(global<be_u64>(K + off).get()); }
constexpr u32 kZero = 0, kOne = 4, kEps = 8, kOneD = 16, kOneBitsD = 24, kMaxPitch = 32;
constexpr u32 kRadToDeg = 36, k360 = 40, k3599 = 44, k3599D = 48, kZeroD = 56, k90 = 80;

constexpr u32 kTuneOcclusionTarget = 0x010B0310;  // f32 at +72
constexpr u32 kTuneDoppler = 0x010B0770;          // f32 at +72
constexpr u32 kOcclusionSource = 0x00F65B64;
constexpr u32 kVfnDistance = 320;                 // shape: (shape, pos, listener pos) -> f32 t
constexpr u32 kVfnContains = 316;                 // zone: (obj, pos) -> bool
constexpr u32 kZoneGain = 484;                    // f32

struct OcclusionSource {
    pad<8> _0;
    be_f32 fade;   // +8
    be_f32 pitch;  // +12
    pad<96> _16;
    be_u32 obj;    // +112
};

inline f32 frsp(f64 x) { return static_cast<f32>(x); }
f32 tune(u32 obj) { return gref<be_f32>(obj + kTuningValue); }

// The bit-trick pow the compiler inlines: x^y via the double's bits
// (fcfid on the bits, fmadd with (1 - y) * bits(1.0), fctid, frsp).
f32 Fast_Pow(f64 x, f64 y) {
    const f64 xi = rt::fcfid(std::bit_cast<u64>(x));
    const f64 c = (kd(kOneD) - y) * kd(kOneBitsD);
    const f64 r = std::fma(xi, y, c);
    return frsp(std::bit_cast<f64>(rt::fctid(r, false)));
}

// 1 - t^curve for t in (0, 1), with the original's shortcuts.
f32 Falloff(f64 t, f64 curve) {
    const f32 one = kf(kOne);
    if (std::fabs(fsubs(t, one)) < kf(kEps)) return fsubs(one, t);
    if (curve == kf(kZero)) return fsubs(one, one);
    if (t == kf(kZero)) return fsubs(one, kf(kZero));
    return fsubs(one, Fast_Pow(t, curve));
}

f32 min1(f32 v) { return v < kf(kOne) ? v : kf(kOne); }

}  // namespace

// void Emitter_Spatialise(Block* s, Listener* l, const f32 pos[3], const f32 vel[3],
//                         SoundDef* def, Zone* zone, Obj* shape, bool occlude, bool doppler (9th arg))  (name?)
void Emitter_Spatialise(Block* sp, const be_f32* lp_, const be_f32* p, const be_f32* v, SoundDef* dp, u32 zone,
                        u32 shape, u8 occlude, u8 doppler) {
    Block& s = gref(sp);
    SoundDef& def = gref(dp);
    const be_f32* lp = &gref(lp_) + 12;
    const be_f32* pos = &gref(p);
    const be_f32* vel = &gref(v);

    GuestFrame frame(48);
    be_f32* copy = frame.as<be_f32>();     // listener position (sp+112 in the original)
    be_f32* dir = frame.as<be_f32>(16);    // sp+128
    copy[0] = lp[0].get();
    const f32 dy = fsubs(pos[1], lp[1]);
    const f32 dx = fsubs(pos[0], lp[0]);
    copy[1] = lp[1].get();
    const f32 dz = fsubs(pos[2], lp[2]);
    copy[2] = lp[2].get();
    const f32 dist = fsqrts(fmadds(dz, dz, fmadds(dx, dx, fmuls(dy, dy))));
    s.distance = dist;

    // t: normalised distance, from the shape or the def's near/far.
    f64 t;
    if (shape != 0) {
        t = call_ptr<f64>(vfunc(shape, kVfnDistance), shape, p, copy);
    } else {
        const f32 nearv = def.near_dist, farv = def.far_dist;
        t = kf(kZero);
        if (!(dist <= nearv)) t = fdivs(fsubs(dist, nearv), fsubs(farv, nearv));
    }

    // Volume falloff (def +16 curve) and the second curve (def +20 range, +24 curve).
    f32 vol;
    if (t <= kf(kZero))
        vol = kf(kOne);
    else if (t >= kf(kOne))
        vol = kf(kZero);
    else
        vol = Falloff(t, def.curve);
    f32 vol2;
    const f32 range2 = def.range2;
    if (t <= kf(kZero))
        vol2 = kf(kOne);
    else if (t >= range2)
        vol2 = kf(kZero);
    else
        vol2 = Falloff(fdivs(t, range2), def.curve2);

    if (zone != 0 && dp && def.zones != 0) {
        const f32 b = gref<be_f32>(zone + kZoneGain);
        const f32 g = fmuls(s.gain, b);
        s.scale = b;
        s.volume = fmuls(g, s.fade);
    }

    s.pitch_code = 0u;
    if (fsel(fsubs(vol, vol2), vol, vol2) > kf(kZero)) {
        // Pitch target from the def's +28 towards the tuning value, by 1 - t^(def +32).
        f32 w;
        const f32 curve = def.pitch_curve;
        if (t <= kf(kZero))
            w = kf(kOne);
        else if (t >= kf(kOne))
            w = kf(kZero);
        else
            w = Falloff(t, curve);
        const f32 base = def.pitch_base;
        const f32 target = fmadds(w, fsubs(tune(kTuneOcclusionTarget), base), base);
        const f32 cap = s.pitch_cap;
        s.pitch_target = target;
        s.pitch = static_cast<f32>(fsel(fsubs(cap, target), target, cap));

        if (occlude != 0) {
            f32 fade = kf(kOne), pitch = kf(kMaxPitch);
            const u32 source = global<be_u32>(kOcclusionSource);
            const OcclusionSource& src = gref<OcclusionSource>(source);
            const u32 obj = source != 0 ? src.obj.get() : 0;
            if (obj != 0) {
                const f32 sf = src.fade, spt = src.pitch;
                if (sf < fade || spt < pitch) {
                    f32 weight = kf(kZero);
                    const bool zoned = gref<u8>(obj + 612) != 0;
                    const u32 mode = zoned ? gref<be_u32>(obj + 428).get() : 0;
                    if (zoned && (mode == 0 || mode == 3)) {
                        weight = fsubs(fade, call<f32>(afn::Obj_GetOcclusion, obj, p));
                    } else if (call_ptr<u32>(vfunc(obj, kVfnContains), obj, p) == 0) {
                        weight = fade;
                    }
                    const f32 df = fsubs(sf, fade), dpt = fsubs(spt, pitch);
                    fade = fmadds(df, weight, fade);
                    pitch = fmadds(dpt, weight, pitch);
                }
            }
            const f32 g = fmuls(s.gain, s.scale);
            const f32 cur = s.pitch_target;
            s.fade = fade;
            s.pitch_cap = pitch;
            s.volume = fmuls(g, fade);
            s.pitch = static_cast<f32>(fsel(fsubs(pitch, cur), cur, pitch));
        }

        if (shape == 0) {
            // Pan angle (degrees, 0..359.9) and elevation (degrees from the normalised y).
            Listener_LocalDirection(dir, p, lp_);
            f64 a = math::Atan2f(dir[0], static_cast<f32>(-static_cast<f64>(dir[2].get())));
            const f32 x = dir[0], y = dir[1], z = dir[2];
            a = fmuls(a, kf(kRadToDeg));
            if (a < kf(kZero)) a = fadds(a, kf(k360));
            if (a >= kf(k360)) a = fsubs(a, kf(k360));
            const f32 len = fsqrts(fmadds(z, z, fmadds(x, x, fmuls(y, y))));
            f64 c = frsp(fsel(fsubs(kf(k3599), a), a, kd(k3599D)));
            c = frsp(fsel(c, c, kd(kZeroD)));
            s.pan = static_cast<u32>(rt::fctid(c, true));
            const f32 inv = fdivs(kf(kOne), len);
            const f32 nx = fmuls(x, inv), ny = fmuls(y, inv), nz = fmuls(z, inv);
            dir[0] = nx;
            dir[1] = ny;
            dir[2] = nz;
            s.elevation = static_cast<s32>(static_cast<u32>(rt::fctiw(fmuls(ny, kf(k90)), true)));
        }
        if (doppler != 0) {
            const f32 ey = fsubs(copy[1], pos[1]);
            const f32 ex = fsubs(copy[0], pos[0]);
            const f32 ez = fsubs(copy[2], pos[2]);
            const f32 d2 = fmadds(ez, ez, fmadds(ex, ex, fmuls(ey, ey)));
            const f32 dot = fmadds(ez, vel[2], fmadds(ex, vel[0], fmuls(ey, vel[1])));
            const f32 speed = fmuls(fdivs(dot, fsqrts(d2)), tune(kTuneDoppler));
            s.pitch_code = static_cast<u32>(snd::Snd_PitchFromSpeed(speed));
        }
    } else {
        s.pan = 0u;
        s.elevation = 0;
    }

    // Final volumes clamped to [0, 1].
    f32 out1 = min1(vol);
    if (!(out1 > kf(kZero))) out1 = kf(kZero);
    f32 out2 = min1(vol2);
    if (!(out2 > kf(kZero))) out2 = kf(kZero);
    s.vol2 = out2;
    s.vol1 = out1;
}

}  // namespace sly4::audio

NATIVE(00442478, sly4::audio::Emitter_Spatialise)
