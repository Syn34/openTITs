// Game code, medium sweep part 7: the two spline-follow steps 0x0027DB68
// / 0x0027F2C0 (identical code, each with its own constant block {f32 0,
// 1, -1, -, f64 0, 1}). For state 2 with a spline instance, the object's
// speed (vfn[35] of the body at +8) advances the parameter of the nearest
// spline point, the horizontal direction to the point there becomes +48
// (y = 0, normalized) and +64 = the follower's f32 +8; at an open end it
// calls vfn[7](o + 80, 0, 0) instead and returns 0.
#include "mid7.h"

using namespace sly4;
using namespace sly4::mid7;

namespace {

namespace sfn {
constexpr u32 SplineInstance_Nearest = 0x005927FC;    // (inst, p[3], u32* seg, f32* dist) -> f1
constexpr u32 WorldObj_SplinePointAt = 0x00592748;    // (inst, -, f32 out[4], bool; f1 t)
constexpr u32 SplineInstance_TangentAt = 0x005A8034;  // (inst, f1 t, f32 out[4], bool)
}  // namespace sfn

f32 Mul(f64 a, f64 b) { return static_cast<f32>(a * b); }
f32 Madd(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }
f32 Sqrt(f64 x) { return static_cast<f32>(std::sqrt(x)); }
// fsel: a >= 0 ? c : b.
f64 Fsel(f64 a, f64 c, f64 b) { return a >= 0.0 ? c : b; }
f64 Ld(u32 ea) { return std::bit_cast<f64>(load<u64>(ea)); }

// u32 (Follower* o, const State* s): K = the constant block.
void SplineStep(PPUContext& ctx, u32 K) {
    const u32 o = arg<u32>(ctx, 0), st = arg<u32>(ctx, 1);
    const u32 sp = load<u32>(o + 100);  // {+0 SplineInstance*, +4 f32 t, +8 f32, +12 flags (bit 31: closed)}
    u32 body = load<u32>(o + 8);
    if (load<u32>(st) != 2 || load<u32>(sp) == 0) return ret(ctx, 1);
    // r1+112 velocity / direction, +128 point, +144 / +160 start point / tangent, +176 segment, +180 distance.
    Stack frame(ctx, 80);
    const u32 S = frame.addr() - 112;
    const f64 k1 = F32(load<u32>(K + 4));
    if (load<u32>(load<u32>(o + 96) + 4) == 0) {
        const f64 t0 = F32(load<u32>(sp + 4));
        const f32 c0 = static_cast<f32>(Fsel(static_cast<f32>(k1 - t0), t0, Ld(K + 16)));
        const f32 t = static_cast<f32>(Fsel(c0, c0, Ld(K + 24)));  // t clamped to [0, 1]
        callf(ctx, sfn::WorldObj_SplinePointAt, {load<u32>(sp), K, S + 144, 0}, {t});
        callf(ctx, sfn::SplineInstance_TangentAt, {load<u32>(sp), ctx.gpr[4], S + 160, 0}, {t});  // r4: left by the previous call (unread)
        for (u32 k = 0; k < 3; ++k) {
            const u32 w = load<u32>(S + 160 + 4 * k);
            store<u32>(S + 160 + 4 * k, (load<u32>(sp + 12) & 0x80000000u) != 0 ? w : w ^ 0x80000000u);  // fneg when open
        }
    }
    VCall(ctx, body, 140, {S + 112, body});
    const f64 vx = F32(load<u32>(S + 112)), vy = F32(load<u32>(S + 116)), vz = F32(load<u32>(S + 120));
    const f32 speed = Sqrt(Madd(vz, vz, Madd(vx, vx, Mul(vy, vy))));
    const f32 s3 = Sqrt(Madd(speed, speed, Madd(speed, speed, Mul(speed, speed))));
    body += 144;  // the body's position
    const f64 t = callf(ctx, sfn::SplineInstance_Nearest, {load<u32>(sp), body, S + 176, S + 180}, {});
    const f64 k0 = F32(load<u32>(K));
    const bool closed = (load<u32>(sp + 12) & 0x80000000u) != 0;
    if ((t == k0 && !closed) || (t == k1 && closed)) {
        ctx.fpr[1] = k0;
        VCall(ctx, o + 80, 28, {o + 80, 0, 0});
        return ret(ctx, 0);
    }
    const f64 scale = closed ? k1 : static_cast<f64>(F32(load<u32>(K + 8)));
    const f32 adv = Mul(Mul(s3, F32(load<u32>(st + 4))), scale);
    const f64 len = F32(load<u32>(load<u32>(load<u32>(sp) + 208) + 120));  // the spline's length
    const f32 a = static_cast<f32>(static_cast<f64>(static_cast<f32>(static_cast<f64>(adv) / len)) + t);
    const f32 c1 = static_cast<f32>(Fsel(static_cast<f32>(k1 - a), a, Ld(K + 16)));
    const f32 nt = static_cast<f32>(Fsel(c1, c1, Ld(K + 24)));  // clamped to [0, 1]
    callf(ctx, sfn::WorldObj_SplinePointAt, {load<u32>(sp), K, S + 128, 0}, {nt});
    const f32 dy = static_cast<f32>(static_cast<f64>(F32(load<u32>(S + 132))) - F32(load<u32>(body + 4)));
    const f32 dx = static_cast<f32>(static_cast<f64>(F32(load<u32>(S + 128))) - F32(load<u32>(body)));
    const f32 dz = static_cast<f32>(static_cast<f64>(F32(load<u32>(S + 136))) - F32(load<u32>(body + 8)));
    store<u32>(o + 52, W32(dy));
    store<u32>(o + 48, W32(dx));
    store<u32>(S + 112, W32(dx));
    store<u32>(S + 116, W32(dy));
    store<u32>(S + 120, W32(dz));
    store<u32>(o + 56, W32(dz));
    store<u32>(o + 52, 0);  // the direction is made horizontal
    const f64 x = F32(load<u32>(o + 48)), y = F32(load<u32>(o + 52)), z = F32(load<u32>(o + 56));
    const f32 inv = static_cast<f32>(k1 / static_cast<f64>(Sqrt(Madd(z, z, Madd(x, x, Mul(y, y))))));
    store<u32>(o + 48, W32(Mul(x, inv)));
    store<u32>(o + 52, W32(Mul(y, inv)));
    store<u32>(o + 56, W32(Mul(z, inv)));
    const u32 w = load<u32>(sp + 8);
    store<u32>(o + 64, w);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(w));
    ret(ctx, 1);
}

}  // namespace

// u32 SplineFollow_Step_0027DB68(Follower* o, const State* s): constants
// at 0x0027DB48.
DECOMP(0027DB68, SplineFollow_Step_0027DB68) { SplineStep(ctx, 0x0027DB48); }

// u32 SplineFollow_Step_0027F2C0(Follower* o, const State* s): constants
// at 0x0027F2A0.
DECOMP(0027F2C0, SplineFollow_Step_0027F2C0) { SplineStep(ctx, 0x0027F2A0); }
