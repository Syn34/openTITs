// Emitter_Spatialise: distance attenuation, occlusion, panning and Doppler
// for an emitter's 3D block. Every float step mirrors the recompiler's
// semantics (single ops round to float; fsub/fmul/fmadd stay double).
#include <bit>
#include <cmath>

#include "audio.h"

using namespace sly4;
using namespace sly4::audio;

namespace {

// Constants before the function (0x00442420).
constexpr u32 K = 0x00442420;
f32 kf(u32 off) { return load<f32>(K + off); }
f64 kd(u32 off) { return std::bit_cast<f64>(load<u64>(K + off)); }
constexpr u32 kZero = 0, kOne = 4, kEps = 8, kOneD = 16, kOneBitsD = 24, kMaxPitch = 32;
constexpr u32 kRadToDeg = 36, k360 = 40, k3599 = 44, k3599D = 48, kZeroD = 56, k90 = 80;

constexpr u32 kTuneOcclusionTarget = 0x010B0310;  // f32 at +72
constexpr u32 kTuneDoppler = 0x010B0770;          // f32 at +72
constexpr u32 kOcclusionSource = 0x00F65B64;
constexpr u32 kLocalDirection = 0x005532E4;       // (f32 out[3], pos, Listener*) (name?)
constexpr u32 kAtan2f = 0x005E665C;
constexpr u32 kPitchFromSpeed = 0x006D7508;       // (f32) -> u32 (name?)
constexpr u32 kOcclusion = 0x00213870;            // (obj, pos) -> f32

inline f32 frsp(f64 x) { return static_cast<f32>(x); }

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
//                         SoundDef* def, Bank* bank, Obj* shape, bool occlude, bool doppler (9th arg))  (name?)
DECOMP(00442478, Emitter_Spatialise) {
    const u32 s = arg<u32>(ctx, 0), l = arg<u32>(ctx, 1), p = arg<u32>(ctx, 2), v = arg<u32>(ctx, 3);
    const u32 def = arg<u32>(ctx, 4), bank = arg<u32>(ctx, 5), shape = arg<u32>(ctx, 6);
    const u32 occlude = arg<u32>(ctx, 7) & 0xFF;
    const u32 doppler = load<u32>(static_cast<u32>(ctx.gpr[1]) + 116) & 0xFF;  // 9th argument
    const u32 lp = l + 48;

    Stack frame(ctx, 48);
    const u32 copy = frame.addr();      // listener position (sp+112 in the original)
    const u32 dir = copy + 16;          // sp+128
    store<f32>(copy, load<f32>(lp));
    const f32 dy = fsubs(load<f32>(p + 4), load<f32>(lp + 4));
    const f32 dx = fsubs(load<f32>(p), load<f32>(lp));
    store<f32>(copy + 4, load<f32>(lp + 4));
    const f32 dz = fsubs(load<f32>(p + 8), load<f32>(lp + 8));
    store<f32>(copy + 8, load<f32>(lp + 8));
    const f32 dist = fsqrts(fmadds(dz, dz, fmadds(dx, dx, fmuls(dy, dy))));
    store<f32>(s + 40, dist);

    // t: normalised distance, from the shape or the def's near/far.
    f64 t;
    if (shape != 0) {
        call_ptr(ctx, load<u32>(load<u32>(shape) + 320), {shape, p, copy});
        t = ctx.fpr[1];
    } else {
        const f32 nearv = load<f32>(def + 8), farv = load<f32>(def + 12);
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
        vol = Falloff(t, load<f32>(def + 16));
    f32 vol2;
    const f32 range2 = load<f32>(def + 20);
    if (t <= kf(kZero))
        vol2 = kf(kOne);
    else if (t >= range2)
        vol2 = kf(kZero);
    else
        vol2 = Falloff(fdivs(t, range2), load<f32>(def + 24));

    if (bank != 0 && def != 0 && ref<u8>(def + 80) != 0) {
        const f32 b = load<f32>(bank + 484);
        const f32 g = fmuls(load<f32>(s + 4), b);
        store<f32>(s + 8, b);
        store<f32>(s, fmuls(g, load<f32>(s + 12)));
    }

    store<u32>(s + 36, 0);
    if (fsel(fsubs(vol, vol2), vol, vol2) > kf(kZero)) {
        // Pitch target from the def's +28 towards the tuning value, by 1 - t^(def +32).
        f32 w;
        const f32 curve = load<f32>(def + 32);
        if (t <= kf(kZero))
            w = kf(kOne);
        else if (t >= kf(kOne))
            w = kf(kZero);
        else
            w = Falloff(t, curve);
        const f32 base = load<f32>(def + 28);
        const f32 target = fmadds(w, fsubs(field<f32>(kTuneOcclusionTarget, kTuningValue), base), base);
        const f32 cap = load<f32>(s + 32);
        store<f32>(s + 28, target);
        store<f32>(s + 24, static_cast<f32>(fsel(fsubs(cap, target), target, cap)));

        if (occlude != 0) {
            f32 fade = kf(kOne), pitch = kf(kMaxPitch);
            const u32 source = load<u32>(kOcclusionSource);
            const u32 obj = source != 0 ? load<u32>(source + 112) : 0;
            if (obj != 0) {
                const f32 sf = load<f32>(source + 8), sp = load<f32>(source + 12);
                if (sf < fade || sp < pitch) {
                    f32 weight = kf(kZero);
                    const bool zoned = ref<u8>(obj + 612) != 0;
                    const u32 mode = zoned ? load<u32>(obj + 428) : 0;
                    if (zoned && (mode == 0 || mode == 3)) {
                        weight = fsubs(fade, callf(ctx, kOcclusion, {obj, p}, {}));
                    } else if (static_cast<u32>(call_ptr(ctx, load<u32>(load<u32>(obj) + 316), {obj, p})) == 0) {
                        weight = fade;
                    }
                    const f32 df = fsubs(sf, fade), dp = fsubs(sp, pitch);
                    fade = fmadds(df, weight, fade);
                    pitch = fmadds(dp, weight, pitch);
                }
            }
            const f32 g = fmuls(load<f32>(s + 4), load<f32>(s + 8));
            const f32 cur = load<f32>(s + 28);
            store<f32>(s + 12, fade);
            store<f32>(s + 32, pitch);
            store<f32>(s, fmuls(g, fade));
            store<f32>(s + 24, static_cast<f32>(fsel(fsubs(pitch, cur), cur, pitch)));
        }

        if (shape == 0) {
            // Pan angle (degrees, 0..359.9) and elevation (degrees from the normalised y).
            call(ctx, kLocalDirection, {dir, p, l});
            f64 a = callf(ctx, kAtan2f, {}, {load<f32>(dir), -static_cast<f64>(load<f32>(dir + 8))});
            const f32 x = load<f32>(dir), y = load<f32>(dir + 4), z = load<f32>(dir + 8);
            a = fmuls(a, kf(kRadToDeg));
            if (a < kf(kZero)) a = fadds(a, kf(k360));
            if (a >= kf(k360)) a = fsubs(a, kf(k360));
            const f32 len = fsqrts(fmadds(z, z, fmadds(x, x, fmuls(y, y))));
            f64 c = frsp(fsel(fsubs(kf(k3599), a), a, kd(k3599D)));
            c = frsp(fsel(c, c, kd(kZeroD)));
            store<u32>(s + 44, static_cast<u32>(rt::fctid(c, true)));
            const f32 inv = fdivs(kf(kOne), len);
            const f32 nx = fmuls(x, inv), ny = fmuls(y, inv), nz = fmuls(z, inv);
            store<f32>(dir, nx);
            store<f32>(dir + 4, ny);
            store<f32>(dir + 8, nz);
            store<u32>(s + 48, static_cast<u32>(rt::fctiw(fmuls(ny, kf(k90)), true)));
        }
        if (doppler != 0) {
            const f32 ey = fsubs(load<f32>(copy + 4), load<f32>(p + 4));
            const f32 ex = fsubs(load<f32>(copy), load<f32>(p));
            const f32 ez = fsubs(load<f32>(copy + 8), load<f32>(p + 8));
            const f32 d2 = fmadds(ez, ez, fmadds(ex, ex, fmuls(ey, ey)));
            const f32 dot = fmadds(ez, load<f32>(v + 8), fmadds(ex, load<f32>(v), fmuls(ey, load<f32>(v + 4))));
            const f32 speed = fmuls(fdivs(dot, fsqrts(d2)), field<f32>(kTuneDoppler, kTuningValue));
            ctx.fpr[1] = speed;
            store<u32>(s + 36, static_cast<u32>(call(ctx, kPitchFromSpeed)));
        }
    } else {
        store<u32>(s + 44, 0);
        store<u32>(s + 48, 0);
    }

    // Final volumes clamped to [0, 1].
    f32 out1 = min1(vol);
    if (!(out1 > kf(kZero))) out1 = kf(kZero);
    f32 out2 = min1(vol2);
    if (!(out2 > kf(kZero))) out2 = kf(kZero);
    store<f32>(s + 20, out2);
    store<f32>(s + 16, out1);
}
