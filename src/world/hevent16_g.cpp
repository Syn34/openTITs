// World_HandleEvent callees, part 16g: the item update (called by
// Set_Update3A0470): the on / off blink timers and the two attachment slots
// (object-driven box or a spline-driven pose).
#include <bit>
#include <cmath>

#include "hevent16.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent16;
using hevent7::lfs_f64;

namespace {
namespace gfn {
constexpr u32 Emitter_Stop = 0x00443424;         // (handle, bool fade)
constexpr u32 Ptr_DerefNonZero = 0x00446960;     // (symbols.txt)
constexpr u32 Sound_Play44C474 = 0x0044C474;     // 11 arguments (symbols.txt)
constexpr u32 Box_AddPoint = 0x00556FE8;         // (f32 box[8], const f32 p[3]) (symbols.txt)
constexpr u32 Sphere_FromBox = 0x005577E0;       // (f32 sphere[4], const f32 box[8]) (symbols.txt)
constexpr u32 WorldObj_MarkDirty = 0x0057D000;   // (WorldObj*) (symbols.txt)
constexpr u32 WorldObj_SplineEval = 0x005A81CC;  // (WorldObj*, -, f32 out[4], u32 seg, bool back; f1 t) -> seg (symbols.txt)
}  // namespace gfn

// Item: +276 s32 kind (1: blinks), +316 / +320 / +324 f32 timers (delay,
// lifetime, toggle), +328 flags (bit 31 0x80000000 start on, bit 28
// 0x10000000 on, bit 29 0x20000000 forced, bit 27 0x08000000 always
// update), +288 Widget* (vt +40 show(w, u8, 1)), +284 Bounds* (+16 box, +48
// sphere), +304 SoundDesc*, +308 sound handle, +240 position.
constexpr u32 kItKind = 276, kItDelay = 316, kItLife = 320, kItToggle = 324, kItFlags = 328;
constexpr u32 kItWidget = 288, kItBounds = 284, kItSound = 304, kItHandle = 308, kItPos = 240;
constexpr u32 kOnStart = 0x80000000u, kOn = 0x10000000u, kForced = 0x20000000u, kAlways = 0x08000000u;
// Two 96-byte slots at +0: +0 Mat (3x4, rows 16 apart), +64 WorldObj*
// spline, +68 u32 segment, +72 Object* driver (+80.. its matrix, +128
// position), +76 / +80 attached Holder* (+28 WorldObj*), +84 f32 speed, +88
// f32 spline t, +92 flags (bits 30..31 mode: 1 wrap, 2 ping-pong; bit 29
// backwards; bit 28 always).
constexpr u32 kSlots = 2, kSlotSize = 96, kSlSpline = 64, kSlSeg = 68, kSlDriver = 72, kSlAttach = 76, kSlHolder = 80;
constexpr u32 kSlSpeed = 84, kSlT = 88, kSlFlags = 92, kSlBack = 0x20000000u, kSlAlways = 0x10000000u;
constexpr u32 kHolderObj = 28, kDriverMat = 80, kDriverPos = 128;

constexpr u32 kConst = 0x0039C1B4;          // f32 0.0, 1/65535, 1.0, -1.0
constexpr u32 kForceUpdate = 0x0106D084;    // u8 (name?)
constexpr u32 kToggleMin = 0x0106D664, kToggleMax = 0x0106D6B0;  // f32 (name?)
constexpr u32 kRandState = 0x01164888;      // u64 seed (low 16 bits used), +8 u32 mixer (name?)

constexpr u32 kFrame = 160, kSpSound = 152;

u32 rotl(u32 v, u32 n) { return (v << n) | (v >> (32 - n)); }
void copy32(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }
}  // namespace

// u32 Item_Update39C1C8(Item* it, f32 dt) (name?): returns whether the item
// is active. Blinking items (+276 == 1) count their delay and lifetime down;
// at the end of the delay the toggle timer is rearmed (random in
// [0x0106D664, 0x0106D6B0]) and the on bit flips, at the end of the
// lifetime the timers clear, the item returns to its start state and
// restarts its sound. Active ones then drive their two slots: a driver
// object grows the bounds, a spline advances (wrapping or ping-ponging) and
// the pose is pushed to the attached object.
DECOMP(0039C1C8, Item_Update39C1C8) {
    const u32 it = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    f64 f1 = dt;  // the original's f1, left as it ends
    u32 active = 0;
    const f64 zero = lfs_f64(kConst), one = lfs_f64(kConst + 8);

    auto notify = [&](u32 flag) {
        for (u32 k = 0; k < kSlots; ++k) {
            const u32 s = it + kSlotSize * k;
            if (load<u32>(s + kSlAttach) == 0) continue;
            const u32 h = load<u32>(s + kSlHolder);
            if (h == 0) continue;
            const u32 o = load<u32>(h + kHolderObj);
            if (o != 0) vcall(ctx, o, 40, {o, flag, 1});
        }
    };

    if (load<s32>(it + kItKind) == 1) {
        f1 = lfs_f64(it + kItLife);
        if (f1 > zero) {
            const f64 d = static_cast<f32>(lfs_f64(it + kItDelay) - dt);
            store<f32>(it + kItDelay, static_cast<f32>(d));
            if (d <= zero) {
                const f64 life = static_cast<f32>(f1 - dt);
                f1 = static_cast<f32>(lfs_f64(it + kItToggle) - dt);
                store<u32>(it + kItDelay, 0);
                store<f32>(it + kItLife, static_cast<f32>(life));
                store<f32>(it + kItToggle, static_cast<f32>(f1));
                if (life <= zero) {
                    // Lifetime over: back to the start state.
                    u32 f = load<u32>(it + kItFlags);
                    store<u32>(it + kItDelay, 0);
                    store<u32>(it + kItLife, 0);
                    store<u32>(it + kItToggle, 0);
                    const u32 start_on = f >> 31;
                    f = (f & ~kOn) | (start_on == 0 ? kOn : 0u);
                    store<u32>(it + kItFlags, f);
                    if (const u32 w = load<u32>(it + kItWidget); w != 0) vcall(ctx, w, 40, {w, start_on == 0 ? 1u : 0u, 1});
                    notify((load<u32>(it + kItFlags) & kForced) != 0 || start_on == 0 ? 1u : 0u);
                    if (static_cast<u32>(call(ctx, gfn::Ptr_DerefNonZero, {load<u32>(it + kItSound)})) != 0) {
                        call(ctx, gfn::Emitter_Stop, {load<u32>(it + kItHandle), 1});
                        store<u32>(it + kItHandle, 0);
                        if (start_on == 0) {
                            Stack frame(ctx, kFrame);
                            const u32 sp = frame.addr();
                            call(ctx, gfn::Sound_Play44C474, {sp + kSpSound, load<u32>(it + kItSound), it + kItPos, 0, 0, 0, 0, 0, 0, 0, 0});
                            store<u32>(it + kItHandle, load<u32>(sp + kSpSound));
                        }
                    }
                } else if (f1 <= zero) {
                    // Toggle: rearm with a random time and flip the on bit.
                    const u64 seed = load<u64>(kRandState);
                    const u32 mix = load<u32>(kRandState + 8);
                    const f64 lo = lfs_f64(kToggleMin), hi = lfs_f64(kToggleMax);
                    const u32 x = (rotl(mix, 4) + rotl(mix, 16)) ^ 0x87654321u;
                    store<u32>(kRandState + 8, rotl(x, 16));
                    store<u64>(kRandState, x & 0xFFFF);
                    const f64 r = static_cast<f32>(rt::fcfid(seed));
                    u32 f = load<u32>(it + kItFlags);
                    const u32 was_on = (f >> 28) & 1;
                    f = (f & ~kOn) | (was_on == 0 ? kOn : 0u);
                    store<u32>(it + kItFlags, f);
                    f1 = fmuls(r, lfs_f64(kConst + 4));
                    f1 = static_cast<f32>(std::fma(static_cast<f64>(static_cast<f32>(hi - lo)), f1, lo));
                    store<f32>(it + kItToggle, static_cast<f32>(f1));
                    if (const u32 w = load<u32>(it + kItWidget); w != 0) vcall(ctx, w, 40, {w, was_on == 0 ? 1u : 0u, 1});
                    notify((load<u32>(it + kItFlags) & kForced) != 0 || was_on == 0 ? 1u : 0u);
                }
            }
            active = 1;
        }
    }

    if (load<u8>(kForceUpdate) != 0 || (load<u32>(it + kItFlags) & kAlways) != 0 || active != 0) {
        for (u32 k = 0; k < kSlots; ++k) {
            const u32 s = it + kSlotSize * k;
            u32 drv = load<u32>(s + kSlDriver);
            if (drv == 0) {
                if (load<u32>(s + kSlSpline) == 0) continue;
                const u32 fl = load<u32>(s + kSlFlags);
                if ((fl & kSlAlways) == 0) {
                    const u32 m = fl >> 30;
                    if (m != 1 && m != 2) continue;
                    f1 = lfs_f64(s + kSlSpeed);
                    if (!(f1 > zero)) continue;
                }
                // Advance the spline parameter.
                const u32 fl2 = load<u32>(s + kSlFlags);
                f1 = fmuls(dt, lfs_f64(s + kSlSpeed));
                const f64 t0 = lfs_f64(s + kSlT);
                const f64 dir = (fl2 & kSlBack) != 0 ? lfs_f64(kConst + 12) : one;
                f1 = static_cast<f32>(std::fma(f1, dir, t0));
                store<f32>(s + kSlT, static_cast<f32>(f1));
                const u32 m = load<u32>(s + kSlFlags) >> 30;
                const f64 whole = static_cast<f32>(rt::fcfid(rt::fctid(f1, true)));
                if (m == 1) {
                    if (f1 > one) {
                        f1 = static_cast<f32>(f1 - whole);
                        store<f32>(s + kSlT, static_cast<f32>(f1));
                    } else if (f1 < zero) {
                        f1 = static_cast<f32>(f1 - whole);
                        f1 = static_cast<f32>(f1 + one);
                        store<f32>(s + kSlT, static_cast<f32>(f1));
                    }
                } else if (f1 > one) {
                    f1 = static_cast<f32>(f1 - whole);
                    f1 = static_cast<f32>(one - f1);
                    store<f32>(s + kSlT, static_cast<f32>(f1));
                    store<u32>(s + kSlFlags, load<u32>(s + kSlFlags) | kSlBack);
                } else if (f1 < zero) {
                    f1 = static_cast<f32>(whole - f1);
                    store<f32>(s + kSlT, static_cast<f32>(f1));
                    store<u32>(s + kSlFlags, load<u32>(s + kSlFlags) & ~kSlBack);
                }
            } else {
                const u32 b = load<u32>(it + kItBounds);
                call(ctx, gfn::Box_AddPoint, {b + 16, drv + kDriverPos});
                call(ctx, gfn::Sphere_FromBox, {b + 48, b + 16});
            }
            // loc_0039C604
            if ((load<u32>(it + kItFlags) & kOn) == 0 && active == 0) continue;
            drv = load<u32>(s + kSlDriver);
            active = 1;
            if (drv != 0) {
                for (u32 r = 0; r < 64; r += 16)
                    for (u32 c = 0; c < 12; c += 4) copy32(s + r + c, drv + kDriverMat + r + c);
                f1 = lfs_f64(drv + kDriverMat + 56);
            } else if (const u32 sp = load<u32>(s + kSlSpline); sp != 0) {
                const u32 back = (load<u32>(s + kSlFlags) >> 29) & 1;
                f1 = lfs_f64(s + kSlT);
                // r4 is cntlzw of the backwards bit (32 / 31); not read.
                callf(ctx, gfn::WorldObj_SplineEval, {sp, back != 0 ? 31u : 32u, s + 48, load<u32>(s + kSlSeg), back}, {f1});
                store<u32>(s + kSlSeg, static_cast<u32>(ctx.gpr[3]));
            }
            if (load<u32>(s + kSlAttach) == 0) continue;
            const u32 h = load<u32>(s + kSlHolder);
            if (h == 0) continue;
            const u32 o = load<u32>(h + kHolderObj);
            if (o == 0) continue;
            for (u32 r = 0; r < 64; r += 16)
                for (u32 c = 0; c < 12; c += 4) copy32(o + 16 + r + c, s + r + c);
            f1 = lfs_f64(s + 56);
            call(ctx, gfn::WorldObj_MarkDirty, {o});
        }
    }
    ctx.fpr[1] = f1;
    ret(ctx, active);
}
