// Small renderer helpers next to the stencil shadow code: the surface
// clear, an item's per-axis scale, the screen rect of a view-space sphere
// and the material animation tracks Material_Bind steps.
#include "pool.h"

using namespace sly4;

namespace {

namespace l4 = sly4::render::layers4;
namespace sp = sly4::render::pool;

using sp::fmadds;
using sp::fnmsubs;
using sp::fmsubs;
using sp::lfs;
using sp::sgl;
using sp::stfs;

constexpr u32 kClearConsts = 0x00531F60;    // f32 1.0 (the far plane), f32 16777215.0 (24-bit depth scale)
constexpr u32 kSphereConsts = 0x00557A68;   // f32 -1.0, 1.0, 0.0; f64 -1.0, 1.0; f32 0.5, -0.5 at +32
constexpr u32 kAnimConsts = 0x005014F8;     // f32 0.0, 2pi; f64 pi, -pi; f32 4/pi, -4/pi^2, 0.225, 1e-5
constexpr u32 kRandomScale = 0x00501408;    // f32 1/65535

// MaterialAnim track: +0 f32 rate, +8 f32 period, +12 / +16 f32 delay range,
// +20 f32 hold time, +32 u32 flags (bits 12..19 state: 0 delay, 1 run, 2
// stopped; bits 20..27 mode: 0 loop, 1 oscillate, 3 frozen; bit 28 sine
// oscillation), +36 f32 value, +40 f32 countdown, +44 f32 phase.
constexpr u32 kStateMask = 0x000FF000;

u32 rotl32(u32 v, u32 n) { return (v << n) | (v >> (32 - n)); }

}  // namespace

// void Draw_Clear(u32 mask, u32 colour, f32 depth, u32 unused, u32 stencil, u32 stencil_mask)   (name?)
// Clears the surface: clear colour, depth (24 bits, 0xFFFFFF for 1.0)
// and stencil, the stencil write mask for the clear, then back to the
// render state's mask (+168).
DECOMP(00531F80, Draw_Clear) {
    const u64 mask = ctx.gpr[3], colour = ctx.gpr[4], stencil = ctx.gpr[6], stencil_mask = ctx.gpr[7];
    const f64 depth = farg(ctx, 0);
    const u32 st = fx::g::state;
    call(ctx, fx::fn::cellGcmSetClearColor, {load<u32>(st), colour});
    u32 z = 0x00FFFFFF;
    if (!(depth == lfs(kClearConsts))) z = static_cast<u32>(rt::fctid(sgl(depth * lfs(kClearConsts + 4)), true));
    call(ctx, fx::fn::cellGcmSetClearDepthStencil, {load<u32>(st), static_cast<u32>((z << 8) | stencil)});
    call(ctx, fx::fn::cellGcmSetStencilMask, {load<u32>(st), stencil_mask});
    call(ctx, fx::fn::cellGcmSetClearSurface, {load<u32>(st), static_cast<u32>(mask)});
    ctx.gpr[3] = call(ctx, fx::fn::cellGcmSetStencilMask, {load<u32>(st), load<u32>(st + 168)});
}

// f32* DrawItem_GetScale(const f32 m[3][4], f32 out[3])   (name?)
// The lengths of the matrix's first three rows. Returns out (f1 the last
// length; r5 / r6 left at out + 4 / + 8).
DECOMP(00554484, DrawItem_GetScale) {
    const u32 m = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    f64 x = 0, z = 0, len = 0;
    for (u32 i = 0; i < 3; ++i) {
        const u32 row = m + 16 * i;
        const f64 y = lfs(row + 4);
        x = lfs(row);
        z = lfs(row + 8);
        len = sgl(std::sqrt(fmadds(z, z, fmadds(x, x, sgl(y * y)))));
        stfs(out + 4 * i, len);
    }
    ctx.gpr[3] = out;
    ctx.gpr[5] = out + 4;
    ctx.gpr[6] = out + 8;
    ctx.fpr[1] = len;
    ctx.fpr[2] = x;
    ctx.fpr[3] = z;
}

// void View_ProjectSphere(const f32 sphere[4], const f32 m[4][4], const f32 proj[], f32* x0, f32* y0, f32* x1, f32* y1)   (name?)
// The sphere's centre goes into view space (rows of m, w column ignored,
// in vmaddfp order); for x and y the tangent planes through the eye give
// the bounds in clip units (start -1 / 1, clamped there), with proj[0] /
// proj[5] as the scales. The rect is returned in 0..1 screen units (y
// flipped). Leaves f1 = *y1.
DECOMP(00557A90, View_ProjectSphere) {
    const u32 sphere = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1) & ~15u, proj = arg<u32>(ctx, 2);
    const f32 sx = load<f32>(sphere), sy = load<f32>(sphere + 4), sz = load<f32>(sphere + 8);
    f32 p[4];
    for (u32 k = 0; k < 4; ++k) {
        f32 v = std::fmaf(load<f32>(m + 4 * k), sx, 0.0f);
        v = std::fmaf(load<f32>(m + 16 + 4 * k), sy, v);
        v = std::fmaf(load<f32>(m + 32 + 4 * k), sz, v);
        p[k] = load<f32>(m + 48 + 4 * k) + v;
    }
    const f64 scale[2] = {lfs(proj), lfs(proj + 20)};
    const u32 c = kSphereConsts;
    f64 lo[2] = {-1.0, -1.0}, hi[2] = {1.0, 1.0};  // the stack slots, stored as -1.0f / 1.0f
    const f64 r = lfs(sphere + 12);
    const f64 nz = -static_cast<f64>(p[2]);
    const f64 z2 = sgl(nz * nz);
    for (u32 i = 0; i < 2; ++i) {
        const f64 pi = p[i];
        const f64 d2 = fmadds(pi, pi, z2);
        const f64 t2 = fnmsubs(r, r, d2);  // d^2 - r^2
        if (!(t2 > lfs(c + 8))) continue;
        f64 t = sgl(std::sqrt(t2));
        const f64 dz = sgl(d2 * nz);
        f64 tp = sgl(t * pi);
        t = sgl(t * nz);
        f64 n0 = fnmsubs(r, nz, tp);
        const f64 d0 = fmadds(r, pi, t);
        if (dz > -sgl(r * n0)) {
            n0 = sgl(n0 * scale[i]);
            const f64 q = sgl(n0 / d0);
            lo[i] = sgl(sp::sel_ge(sgl(lfs(c) - q), load<f64>(c + 16), q));
        }
        tp = fmadds(r, nz, tp);
        const f64 d1 = fnmsubs(r, pi, t);
        if (dz > sgl(r * tp)) {
            const f64 q = sgl(sgl(tp * scale[i]) / d1);
            hi[i] = sgl(sp::sel_ge(sgl(q - lfs(c + 4)), load<f64>(c + 24), q));
        }
    }
    const f64 one = lfs(c + 4), half = lfs(c + 32), nhalf = lfs(c + 36);
    const f64 x0 = sgl(sgl(lo[0] + one) * half);
    const f64 y0 = sgl(sgl(hi[1] - one) * nhalf);
    const f64 x1 = sgl(sgl(hi[0] + one) * half);
    const f64 y1 = sgl(sgl(lo[1] - one) * nhalf);
    stfs(arg<u32>(ctx, 3), x0);
    stfs(arg<u32>(ctx, 4), y0);
    stfs(arg<u32>(ctx, 5), x1);
    stfs(arg<u32>(ctx, 6), y1);
    ctx.fpr[1] = y1;
}

// void MaterialAnim_SetState(Track* t, s32 state)   (name?)
// 0: a random delay in [+12, +16) (the previous random value times
// 1/65535; the generator steps); 1: the hold time (+20) as the countdown;
// 2: stopped. Other states only clear the phase (+44), which every state
// clears.
DECOMP(00501410, MaterialAnim_SetState) {
    const u32 t = arg<u32>(ctx, 0);
    const s32 state = arg<s32>(ctx, 1);
    const u32 rnd = sp::g::anim_random;
    if (state == 0) {
        const u64 prev = load<u64>(rnd);
        const f64 lo = lfs(t + 12);
        const f64 range = sgl(lfs(t + 16) - lo);
        const u32 seed = load<u32>(rnd + 8);
        const u32 x = (rotl32(seed, 4) + rotl32(seed, 16)) ^ 0x87654321u;
        const f64 f = sgl(rt::fcfid(prev));
        store<u32>(rnd + 8, rotl32(x, 16));
        store<u64>(rnd, x & 0xFFFF);
        const u32 w = load<u32>(t + 32) & ~kStateMask;  // rlwimi 0,12,19 with 0
        const f64 scaled = sgl(f * lfs(kRandomScale));
        store<u32>(t + 32, w);
        stfs(t + 40, fmadds(range, scaled, lo));
    } else if (state == 1) {
        const u32 w = load<u32>(t + 32);
        sp::copy_word(t + 40, t + 20);
        store<u32>(t + 32, (w & ~kStateMask) | (1u << 12));
    } else if (state == 2) {
        store<u32>(t + 32, (load<u32>(t + 32) & ~kStateMask) | (2u << 12));
    }
    store<u32>(t + 44, 0);
    ctx.gpr[4] = 0;
}

// void MaterialAnim_Update(Track* t, f32 dt)   (name?)
// Mode 3 is frozen. State 0 counts the delay down and then runs; state 1
// counts the hold time down (when there is one) and then re-enters the
// delay with the overshoot added to dt, and animates the value: mode 0
// loops it over [0, period) at +0 per second; mode 1 either oscillates it
// as period * sin(2pi * phase / (period / rate)) (bit 28; a parabolic sine
// approximation) or ping-pongs it between 0 and period - 1e-5 by flipping
// the rate.
DECOMP(00501520, MaterialAnim_Update) {
    const u32 t = arg<u32>(ctx, 0);
    f64 dt = farg(ctx, 0);
    const u32 c = kAnimConsts;
    u32 w = load<u32>(t + 32);
    if (((w >> 20) & 0xFF) == 3) return;  // rlwinm 12,24,31
    const u32 state = (w >> 12) & 0xFF;   // rlwinm 20,24,31
    if (state == 0) {
        const f64 v = sgl(lfs(t + 40) - dt);
        stfs(t + 40, v);
        if (v <= lfs(c)) call(ctx, sp::fn::MaterialAnim_SetState, {t, 1});
        return;
    }
    if (state != 1) return;
    const f64 zero = lfs(c);
    if (lfs(t + 20) > zero) {
        const f64 v = sgl(lfs(t + 40) - dt);
        stfs(t + 40, v);
        if (v <= zero) {
            dt = sgl(dt + v);
            call(ctx, sp::fn::MaterialAnim_SetState, {t, 0});
            w = load<u32>(t + 32);
        }
    }
    const u32 mode = (w >> 20) & 0xFF;
    if (mode == 0) {
        f64 v = fmadds(lfs(t), dt, lfs(t + 36));
        stfs(t + 36, v);
        if (v < zero) {
            v = sgl(v + lfs(t + 8));
            stfs(t + 36, v);
        }
        const f64 period = lfs(t + 8);
        if (v >= period) stfs(t + 36, sgl(v - period));
        return;
    }
    if (mode != 1) return;
    if (((w >> 28) & 1) != 0) {  // rlwinm 4,31,31
        const f64 period = lfs(t + 8);
        const f64 cycle = sgl(period / lfs(t));
        f64 phase = sgl(lfs(t + 44) + dt);
        stfs(t + 44, phase);
        if (phase > cycle) {
            phase = sgl(phase - cycle);
            stfs(t + 44, phase);
        }
        const f64 two_pi = lfs(c + 4);
        const f64 a = sgl(sgl(phase / cycle) * two_pi);
        const f64 half = sgl(sp::sel_ge(a, load<f64>(c + 8), load<f64>(c + 16)));  // +-pi
        f64 x = sgl(half + a);
        const f64 turns = sgl(rt::fcfid(rt::fctid(sgl(x / two_pi), true)));
        x = fnmsubs(turns, two_pi, x);
        x = sgl(x - half);  // wrapped to [-pi, pi]
        f64 y = sgl(sgl(x * lfs(c + 28)) * std::fabs(x));
        y = fmadds(x, lfs(c + 24), y);
        y = fmadds(fmsubs(y, std::fabs(y), y), lfs(c + 32), y);
        stfs(t + 36, sgl(period * y));
        return;
    }
    const f64 rate = lfs(t);
    f64 neg = rate;
    f64 v = fmadds(rate, dt, lfs(t + 36));
    stfs(t + 36, v);
    if (v < zero) {
        neg = -neg;
        v = zero;
        store<u32>(t + 36, 0);
        stfs(t, neg);
    }
    const f64 period = lfs(t + 8);
    if (v >= period) {
        stfs(t, -neg);
        stfs(t + 36, sgl(period - lfs(c + 36)));
    }
}
