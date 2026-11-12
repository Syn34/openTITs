// Game code, large-function sweep part 34: a driven object's per-frame
// input step (steering, throttle and brake smoothed from the pad state).
#include "big34.h"

namespace sly4::obj {

// The drive tuning block (the tuning owner's +872).
struct DriveTuning {
    be_f32 rate;         // +0: steering approach rate (and the integration rate)
    be_f32 rate_out;     // +4: steering rate while the target exceeds the current value
    be_f32 dead;         // +8: stick dead zone
    be_f32 accel_idle;   // +12
    be_f32 accel_thr;    // +16
    be_f32 accel_brk;    // +20
    pad<28> _24;
    u8 horn;             // +52: the horn button is read
    u8 analog;           // +53: throttle / brake kept as levels, not signs
    u8 no_block;         // +54: the block button does not cancel them
};
static_assert(offsetof(DriveTuning, horn) == 52);
struct DriveTuningOwner {
    pad<872> _0;
    DriveTuning tuning;  // +872
};
// The input state (o->src's vt slot 12).
struct DriveInput {
    be_f32 x;         // +0
    pad<4> _4;
    be_f32 y;         // +8
    pad<24> _12;
    be_u32 buttons;   // +36
};
static_assert(offsetof(DriveInput, buttons) == 36);
// The output block.
struct DriveOut {
    pad<8> _0;
    be_f32 steer;  // +8
    be_f32 level;  // +12
    u8 horn;       // +16
    u8 brake;      // +17
};
struct DriveOutHolder {
    pad<156> _0;
    be_u32 out;  // +156: a DriveOut
};
struct DriveObj {
    pad<384> _0;
    be_u32 flags;                    // +384: bit 31 input enabled
    pad<8> _388;
    gp<DriveTuningOwner> tuning;     // +396
    pad<212> _400;
    gp<DriveOutHolder> out;          // +612
    pad<316> _616;
    be_f32 steer;                    // +932
    be_f32 level;                    // +936
    be_f32 int_x;                    // +940
    be_f32 int_y;                    // +944
    pad<4> _948;
    be_u32 src;                      // +952: the input source (vt slot 12)
    pad<16> _956;
    be_f32 thr;                      // +972
    be_f32 t_thr;                    // +976
    be_f32 brk;                      // +980
    be_f32 t_brk;                    // +984
    be_f32 t_idle;                   // +988
};
static_assert(offsetof(DriveObj, tuning) == 396 && offsetof(DriveObj, out) == 612);
static_assert(offsetof(DriveObj, steer) == 932 && offsetof(DriveObj, src) == 952 && offsetof(DriveObj, t_idle) == 988);

namespace {
constexpr f64 kDead = 1e-5f;          // 0x00236328 + 4 (-kDead at +8)
constexpr f64 kThrottleLow = 0.01f;   // +28: the throttle level below which the brake button counts
constexpr u32 kBtnThrottle = 0x4, kBtnBrake = 0x10, kBtnHorn = 0x40, kBtnBlock = 0x4000;

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
// o->out and o->tuning (P = its tuning block). The input state comes from
// o->src's vt slot 12; o->flags bit 31 enables it. Steering: the stick x
// past the dead zone (rescaled to 0..1, signed) is approached at rate
// P.rate_out while its magnitude exceeds the current o->steer, else P.rate;
// o->int_x / int_y integrate x / -y at P.rate * dt; all clamped to [-1, 1].
// Throttle (bit 0x4) and brake (0x10 when not throttling), as signs unless
// P.analog, both zeroed by bit 0x4000 unless P.no_block; o->level follows
// throttle - brake at rate accel_thr / accel_brk (accel_idle when idle).
// Writes the output's steer / level / brake / horn (bit 0x40 when P.horn),
// o->thr / brk and the time spent throttling / braking / idle (reset when
// not).
void Drive_UpdateInput_00236348(DriveObj* o, f32 dt_) {
    const f64 dt = dt_;
    if (!o->out || !o->tuning) return;
    const u32 in_ea = vcall<u32>(host<u8>(o->src), 12);
    const DriveInput& in = gm<DriveInput>(in_ea);  // read only when enabled, except the block bit (unchecked)
    const DriveTuning& P = o->tuning->tuning;
    DriveOut& out = gm<DriveOut>(o->out->out);  // unchecked
    const bool on = (o->flags & 0x80000000u) != 0;
    const f64 v932 = lf(o->steer), v940 = lf(o->int_x), v944 = lf(o->int_y), v936 = lf(o->level);
    const u8 noBlock = P.no_block;

    // steering
    const f64 stick = on ? lf(in.x) : 0.0;
    const f64 dz = lf(P.dead);
    const f64 q = Divs(Subs(std::fabs(stick), dz), Subs(1.0, dz));
    const f64 target = Muls(Sign(stick), S(-q >= 0.0 ? 0.0 : q));
    const f64 rate = std::fabs(target) > std::fabs(v932) ? lf(P.rate_out) : lf(P.rate);
    const f64 step = Muls(lf(P.rate), dt);
    const f64 dSteer = Fmadds(Subs(target, v932), Muls(rate, dt), 0.0);
    o->int_x = static_cast<f32>(Clamp1(Fmadds(v940, step, stick)));
    const f64 pitch = on ? -lf(in.y) : 0.0;
    o->int_y = static_cast<f32>(Clamp1(Fmadds(v944, step, pitch)));

    // throttle / brake
    f64 thr = 0.0, brk = 0.0;
    if (on) {
        const u32 b = in.buttons;
        thr = (b & kBtnThrottle) != 0 ? 1.0 : 0.0;
        brk = thr < kThrottleLow && (b & kBtnBrake) != 0 ? 1.0 : 0.0;
        if (P.analog == 0) {
            thr = Sign(thr);
            brk = Sign(brk);
        }
    }
    if (noBlock == 0 && (in.buttons & kBtnBlock) != 0) thr = brk = 0.0;
    const f64 accel = thr == 0.0 && brk == 0.0 ? lf(P.accel_idle) : (thr > 0.0 ? lf(P.accel_thr) : lf(P.accel_brk));
    const f64 d = Subs(Adds(v936, thr), brk);
    const f64 steer = Clamp1(Adds(v932, dSteer));
    const f64 dThr = S(-std::fma(Muls(d, accel), dt, -0.0));
    o->steer = static_cast<f32>(steer);
    const f64 level = Clamp1(Adds(v936, dThr));
    o->level = static_cast<f32>(level);
    u8 horn = 0;
    if (on && P.horn != 0) horn = (in.buttons & kBtnHorn) != 0;
    out.steer = static_cast<f32>(steer);
    out.level = static_cast<f32>(level);
    out.brake = brk != 0.0 ? 1 : 0;
    out.horn = horn;
    o->thr = static_cast<f32>(thr);
    o->brk = static_cast<f32>(brk);
    o->t_thr = static_cast<f32>(thr != 0.0 ? Adds(lf(o->t_thr), dt) : 0.0);
    o->t_brk = static_cast<f32>(brk != 0.0 ? Adds(lf(o->t_brk), dt) : 0.0);
    o->t_idle = static_cast<f32>(thr == 0.0 && brk == 0.0 ? Adds(lf(o->t_idle), dt) : 0.0);
}

}  // namespace sly4::obj

NATIVE(00236348, sly4::obj::Drive_UpdateInput_00236348)
