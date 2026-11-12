// Game code, large-function sweep part 34: a driven object's per-frame
// input step (steering, throttle and brake smoothed from the pad state).
#include "../sly4.h"
#include "big34.h"

using namespace sly4;
using namespace sly4::big34;

namespace {
constexpr f64 kDead = 1e-5f;     // 0x00236328 + 4 (-kDead at +8)
constexpr f64 kThrottleLow = 0.01f;   // +28: the throttle level below which the brake button counts
constexpr u32 kBtnThrottle = 0x4, kBtnBrake = 0x10, kBtnHorn = 0x40, kBtnBlock = 0x4000;  // in->36

// The sign of x with a dead zone, as a float (-1, 0 or 1).
f64 Sign(f64 x) { return static_cast<f64>(static_cast<f32>((x > kDead ? 1 : 0) - (x < -kDead ? 1 : 0))); }
// x limited to [-1, 1] as the original's compare chain does (NaN gives 1).
f64 Clamp1(f64 x) {
    const f64 m = x < 1.0 ? x : 1.0;
    if (!(m > -1.0)) return -1.0;
    return x < 1.0 ? x : 1.0;
}
}  // namespace

// void T::UpdateDriveInput(T* o, f32 dt) (called by f_00238F78). Needs
// o->612 (whose +156 is the output block) and o->396 (+872: the tuning P).
// The input state comes from o->952's vt slot 12; o->384 bit 31 enables
// it. Steering: the stick in->0 past the dead zone P+8 (rescaled to 0..1,
// signed) is approached at rate P+4 while its magnitude exceeds the current
// o->932, else P+0; o->940 / o->944 integrate in->0 / -in->8 at P+0 * dt;
// all clamped to [-1, 1]. Throttle (in->36 bit 0x4) and brake (0x10 when not
// throttling), as signs unless P+53 is set, both zeroed by bit 0x4000 unless
// P+54; o->936 follows throttle - brake at rate P+16 / P+20 (P+12 when
// idle). Writes out+8 / +12 (steer, throttle level), out+17 (braking),
// out+16 (bit 0x40 when P+52), o->972 / 980 (throttle, brake) and the time
// spent throttling / braking / idle in o->976 / 984 / 988 (reset when not).
DECOMP(00236348, Drive_UpdateInput_00236348) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    if (load<u32>(o + 612) == 0 || load<u32>(o + 396) == 0) return ret(ctx, 0u);
    const u32 src = load<u32>(o + 952);
    const u32 in = static_cast<u32>(VCall(ctx, src, 12, {src}));
    const u32 P = load<u32>(o + 396) + 872;
    const u32 out = load<u32>(load<u32>(o + 612) + 156);
    const bool on = (load<u32>(o + 384) & 0x80000000u) != 0;
    const f64 v932 = Lfs(o + 932), v940 = Lfs(o + 940), v944 = Lfs(o + 944), v936 = Lfs(o + 936);
    const u8 noBlock = load<u8>(P + 54);

    // steering
    const f64 stick = on ? Lfs(in) : 0.0;
    const f64 dz = Lfs(P + 8);
    const f64 q = Divs(Subs(std::fabs(stick), dz), Subs(1.0, dz));
    const f64 target = Muls(Sign(stick), S(-q >= 0.0 ? 0.0 : q));
    const f64 rate = std::fabs(target) > std::fabs(v932) ? Lfs(P + 4) : Lfs(P + 0);
    const f64 step = Muls(Lfs(P + 0), dt);
    const f64 dSteer = Fmadds(Subs(target, v932), Muls(rate, dt), 0.0);
    store<u32>(o + 940, big29::Fb(Clamp1(Fmadds(v940, step, stick))));
    const f64 pitch = on ? -Lfs(in + 8) : 0.0;
    store<u32>(o + 944, big29::Fb(Clamp1(Fmadds(v944, step, pitch))));

    // throttle / brake
    f64 thr = 0.0, brk = 0.0;
    if (on) {
        const u32 b = load<u32>(in + 36);
        thr = (b & kBtnThrottle) != 0 ? 1.0 : 0.0;
        brk = thr < kThrottleLow && (b & kBtnBrake) != 0 ? 1.0 : 0.0;
        if (load<u8>(P + 53) == 0) {
            thr = Sign(thr);
            brk = Sign(brk);
        }
    }
    if (noBlock == 0 && (load<u32>(in + 36) & kBtnBlock) != 0) thr = brk = 0.0;
    const f64 accel = thr == 0.0 && brk == 0.0 ? Lfs(P + 12) : (thr > 0.0 ? Lfs(P + 16) : Lfs(P + 20));
    const f64 d = Subs(Adds(v936, thr), brk);
    const f64 steer = Clamp1(Adds(v932, dSteer));
    const f64 dThr = S(-std::fma(Muls(d, accel), dt, -0.0));
    store<u32>(o + 932, big29::Fb(steer));
    const f64 level = Clamp1(Adds(v936, dThr));
    store<u32>(o + 936, big29::Fb(level));
    u8 horn = 0;
    if (on && load<u8>(P + 52) != 0) {
        const u32 bits = load<u32>(in + 36) & kBtnHorn;
        horn = bits != 0;
        ctx.gpr[3] = static_cast<u32>(std::countl_zero(bits));  // r3 as the original leaves it
    } else {
        ctx.gpr[3] = in;
    }
    store<u32>(out + 8, big29::Fb(steer));
    store<u32>(out + 12, big29::Fb(level));
    store<u8>(out + 17, brk != 0.0 ? 1 : 0);
    store<u8>(out + 16, horn);
    store<u32>(o + 972, big29::Fb(thr));
    store<u32>(o + 980, big29::Fb(brk));
    store<u32>(o + 976, big29::Fb(thr != 0.0 ? Adds(Lfs(o + 976), dt) : 0.0));
    const f64 tb = brk != 0.0 ? Adds(Lfs(o + 984), dt) : 0.0;
    store<u32>(o + 984, big29::Fb(tb));
    store<u32>(o + 988, big29::Fb(thr == 0.0 && brk == 0.0 ? Adds(Lfs(o + 988), dt) : 0.0));
    ctx.fpr[1] = tb;  // f1 as the original leaves it
}
