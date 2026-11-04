// World_HandleEvent callees, part 9j: the timed treasure update.
#include <bit>
#include <cmath>

#include "hevent9.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent9;

namespace sly4 {
namespace hevent9j {
namespace jfn {
constexpr u32 Spawner_Find0D155C = 0x000D155C;    // (a, b) -> spawn point or 0 (name?)
constexpr u32 Treasure_Place425F18 = 0x00425F18;  // (Treasure*, vec3* pos, quat* rot) (name?)
constexpr u32 Obj_Signal4CF988 = 0x004CF988;      // (Object*, u32 hash) (as hevent.h sub_004CF988)
constexpr u32 Obj_MoveTo0D7904 = 0x000D7904;      // (Object*, const vec3*, u8) (name?)
constexpr u32 Math_Cosf = 0x007344E0;
constexpr u32 String_Format = 0x005534D0;         // (fmt, ...)
constexpr u32 Obj_SetText367530 = 0x00367530;     // (Object*, const char*, u32 slot)
constexpr u32 Obj_SetAnim = 0x00363A3C;           // (o, name, now, -, b, c, d; f32)
constexpr u32 Treasure_Release425BA8 = 0x00425BA8;  // (Treasure*, u8 lost)
}  // namespace jfn

// Treasure: +96 / +112 vec3 start and end of the drop, +132 / +136
// spawner args (+136 the moving object), +140 f32 (timer on), +144 f32
// time left, +148 f32 phase time, +152 Object* timer display, +156
// Object* signalled after the wait, +160 s32 phase (0 wait, 1 drop, 2
// done), +164 flags (bit 31 active, bit 30 held).
constexpr u32 kTStart = 96, kTEnd = 112, kTSpawnA = 132, kTMover = 136, kTTimerOn = 140, kTLeft = 144;
constexpr u32 kTPhaseTime = 148, kTDisplay = 152, kTSignal = 156, kTPhase = 160, kTFlags = 164;
constexpr u32 kConsts = 0x00426364;   // f32 0.0, 1e-5, 1.0, pi, 0.5, -1.0
constexpr u32 kTuneWait = 0x01089FB8; // f32 (0x01089F70 + 72) (name?)
constexpr u32 kTuneDrop = 0x0108A008; // f32 (0x01089FC0 + 72) (name?)
constexpr u32 kTuneTick = 0x0108A0F8; // f32 (0x0108A0B0 + 72): tick anim below this (name?)
constexpr u32 kSignalHash = 0xC4E55EE8;
constexpr u32 kFmtTime = 0x00AE2034;  // "%2.2f"
constexpr u32 kAnimArg = 0x01164CA4;
// Display: +24 parent (+164 bit 15), +396 bit 30 (text display).
constexpr u32 kDispParent = 24, kDispFlags = 396, kParentFlags = 164;

// floorf as the original inlines it: x when trunc(x) == x (by the bits of
// the single difference, so -0.0 counts as different) or |x| >= 2^24,
// else trunc(x), less 1 (through the 32-bit truncation) for negatives.
f64 floor_inl(u32 raw, f64 x) {
    const u32 ti = static_cast<u32>(rt::fctiw(x, true));
    const f64 tr = static_cast<f32>(rt::fcfid(rt::fctid(x, true)));
    if (std::bit_cast<u32>(static_cast<f32>(tr - x)) == 0) return x;
    if (static_cast<s32>(raw & 0x7F800000u) >= 0x4B800000) return x;
    if (raw & 0x80000000u) return static_cast<f32>(rt::fcfid(static_cast<u64>(static_cast<s64>(static_cast<s32>(ti - 1)))));
    return tr;
}
}  // namespace hevent9j
}  // namespace sly4

using namespace sly4::hevent9j;

// void Treasure_Tick426380(Treasure* t, f32 dt) (name?): for an active,
// not held treasure: phase 0 waits (placed at the spawner's point when
// 0x000D155C finds one) until the wait tuning, then signals +156 and drops;
// phase 1 eases (cosine) from +96 to +112 over the drop tuning, moving +136
// with 0x000D7904. With the timer on (+140 > 0) the time left (+144) runs
// down (clamped at 0) and is shown on +152 ("%2.2f": as text, or with the
// tick anim when the whole second changed under the tick tuning); at 0 the
// treasure is lost (Treasure_Release425BA8(t, 1)). r3 / f1 as the original
// leaves them.
DECOMP(00426380, Treasure_Tick426380) {
    const u32 t = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const u32 flags = load<u32>(t + kTFlags);
    ctx.gpr[3] = flags;
    if ((flags & 0x80000000u) == 0) return;
    if ((flags & 0x40000000u) != 0) {
        ctx.gpr[3] = 1;
        return;
    }
    const s32 phase = static_cast<s32>(load<u32>(t + kTPhase));
    const f64 c0 = lfs_f64(kConsts);
    if (phase == 0) {
        store<f32>(t + kTPhaseTime, static_cast<f32>(lfs_f64(t + kTPhaseTime) + dt));
        const u32 a = load<u32>(t + kTSpawnA);
        if (a != 0) {
            const u32 sp = static_cast<u32>(call(ctx, jfn::Spawner_Find0D155C, {a, load<u32>(t + kTMover)}));
            if (sp != 0) call(ctx, jfn::Treasure_Place425F18, {t, sp + 144, sp + 128});
        }
        if (lfs_f64(t + kTPhaseTime) >= lfs_f64(kTuneWait)) {
            const u32 sig = load<u32>(t + kTSignal);
            if (sig != 0) call(ctx, jfn::Obj_Signal4CF988, {sig, kSignalHash});
            store<u32>(t + kTPhaseTime, 0);
            store<u32>(t + kTPhase, 1);
        }
    } else if (phase == 1) {
        f64 pt = static_cast<f32>(lfs_f64(t + kTPhaseTime) + dt);
        store<f32>(t + kTPhaseTime, static_cast<f32>(pt));
        const u32 mover = load<u32>(t + kTMover);
        if (mover != 0) {
            const f64 one = lfs_f64(kConsts + 8), drop = lfs_f64(kTuneDrop);
            const f64 f = drop > c0 ? static_cast<f64>(static_cast<f32>(pt / drop)) : one;
            const f64 lo = f < one ? f : one;
            const f64 u = lo > c0 ? (f < one ? f : one) : c0;  // clamp(f, 0, 1)
            const f64 pi = lfs_f64(kConsts + 12);
            const f64 a = static_cast<f32>(-std::fma(u, pi, -pi));  // fnmsubs: pi - u * pi
            const f64 c = callf(ctx, jfn::Math_Cosf, {}, {a});
            const f64 k = fmuls(static_cast<f32>(c + one), lfs_f64(kConsts + 16));
            Stack frame(ctx, 16);  // the position at r1+112
            for (u32 i = 0; i < 3; ++i) {
                const f64 s = lfs_f64(t + kTStart + 4 * i), e = lfs_f64(t + kTEnd + 4 * i);
                store<f32>(frame.addr() + 4 * i, static_cast<f32>(std::fma(static_cast<f32>(e - s), k, s)));
            }
            call(ctx, jfn::Obj_MoveTo0D7904, {load<u32>(t + kTMover), frame.addr(), 1});
            pt = lfs_f64(t + kTPhaseTime);
        }
        if (pt >= lfs_f64(kTuneDrop)) {
            store<u32>(t + kTPhaseTime, 0);
            store<u32>(t + kTPhase, 2);
        }
    }

    const f64 on = lfs_f64(t + kTTimerOn);
    if (!(on > c0)) {
        ctx.gpr[3] = kConsts;
        ctx.fpr[1] = on;
        return;
    }
    const f64 x = lfs_f64(t + kTLeft);
    const f64 whole = floor_inl(load<u32>(t + kTLeft), x);
    const f64 dx = static_cast<f32>(x - dt);
    const f64 left = dx > c0 ? dx : c0;
    store<f32>(t + kTLeft, static_cast<f32>(left));
    const u32 disp = load<u32>(t + kTDisplay);
    ctx.gpr[3] = disp;
    if (disp != 0) {
        // Variadic: the double goes in f1 and in r4.
        callf(ctx, jfn::String_Format, {kFmtTime, std::bit_cast<u64>(left)}, {left});
        const u32 str = static_cast<u32>(ctx.gpr[3]);
        const u32 d = load<u32>(t + kTDisplay);
        const f64 v = lfs_f64(t + kTLeft);
        const u32 vraw = load<u32>(t + kTLeft);
        bool text = true;
        const u32 par = load<u32>(d + kDispParent);
        if ((load<u32>(d + kDispFlags) & 0x40000000u) == 0 || par == 0 || (load<u32>(par + kParentFlags) & 0x8000u) == 0) {
            text = false;
        } else if (whole <= lfs_f64(kTuneTick)) {
            if (!(std::fabs(static_cast<f32>(whole - floor_inl(vraw, v))) < lfs_f64(kConsts + 4))) text = false;
        }
        if (text) {
            call(ctx, jfn::Obj_SetText367530, {d, str, 0});
        } else {
            ctx.fpr[1] = lfs_f64(kConsts + 20);
            call(ctx, jfn::Obj_SetAnim, {d, str, 1, kConsts, 0, 0, kAnimArg});
        }
    }
    const f64 now = lfs_f64(t + kTLeft);
    ctx.fpr[1] = now;
    if (now <= c0) call(ctx, jfn::Treasure_Release425BA8, {t, 1});
}
