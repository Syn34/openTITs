// Small renderer helpers next to the stencil shadow code: the surface
// clear, an item's per-axis scale, the screen rect of a view-space sphere
// and the material animation tracks Material_Bind steps.
#include "pool.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

// Constant pools next to the functions (read-only data in the EBOOT).
struct ClearConsts {
    be_f32 far_plane;    // +0: 1.0
    be_f32 depth_scale;  // +4: 16777215.0 (24-bit depth)
};
struct SphereConsts {
    be_f32 neg_one;   // +0: -1.0
    be_f32 one;       // +4: 1.0
    be_f32 zero;      // +8: 0.0
    pad<4> _12;
    be_f64 neg_one_d;  // +16: -1.0
    be_f64 one_d;     // +24: 1.0
    be_f32 half;      // +32: 0.5
    be_f32 neg_half;  // +36: -0.5
};
struct AnimConsts {
    be_f32 zero;      // +0: 0.0
    be_f32 two_pi;    // +4
    be_f64 pi;        // +8
    be_f64 neg_pi;    // +16
    be_f32 b;         // +24: 4/pi
    be_f32 c;         // +28: -4/pi^2
    be_f32 p;         // +32: 0.225
    be_f32 eps;       // +36: 1e-5
};
static_assert(offsetof(SphereConsts, half) == 32 && offsetof(AnimConsts, eps) == 36);
const ClearConsts& clear_consts() { return global<ClearConsts>(0x00531F60); }
const SphereConsts& sphere_consts() { return global<SphereConsts>(0x00557A68); }
const AnimConsts& anim_consts() { return global<AnimConsts>(0x005014F8); }
constexpr u32 kRandomScale = 0x00501408;  // f32 1/65535

// The random generator MaterialAnim_SetState steps.
struct AnimRandom {
    be_u64 last;  // +0: 0..0xFFFF
    be_u32 seed;  // +8
};

constexpr u32 kStateMask = 0x000FF000;

u32 rotl32(u32 v, u32 n) { return (v << n) | (v >> (32 - n)); }

}  // namespace

// void Draw_Clear(u32 mask, u32 colour, f32 depth, u32 stencil, u32 stencil_mask)   (name?)
// Clears the surface: clear colour, depth (24 bits, 0xFFFFFF for 1.0)
// and stencil, the stencil write mask for the clear, then back to the
// render state's mask (+168).
void Draw_Clear(u32 mask, u32 colour, f32 depth, u32 stencil, u32 stencil_mask) {
    const ClearConsts& c = clear_consts();
    call(fn::cellGcmSetClearColor, gcm(), colour);
    u32 z = 0x00FFFFFF;
    if (!(depth == c.far_plane)) z = static_cast<u32>(fctidz(depth * c.depth_scale));
    call(fn::cellGcmSetClearDepthStencil, gcm(), (z << 8) | stencil);
    call(fn::cellGcmSetStencilMask, gcm(), stencil_mask);
    call(fn::cellGcmSetClearSurface, gcm(), mask);
    call(fn::cellGcmSetStencilMask, gcm(), rs().stencil_mask.get());
}

// void DrawItem_GetScale(const f32 m[3][4], f32 out[3])   (name?)
// The lengths of the matrix's first three rows.
void DrawItem_GetScale(const be_f32* m, be_f32* out) {
    for (u32 i = 0; i < 3; ++i) {
        const be_f32* row = m + 4 * i;
        const f32 y = row[1], x = row[0], z = row[2];
        out[i] = std::sqrt(fmadds(z, z, fmadds(x, x, y * y)));
    }
}

// void View_ProjectSphere(const f32 sphere[4], const f32 m[4][4], const f32 proj[], f32* x0, f32* y0, f32* x1, f32* y1)   (name?)
// The sphere's centre goes into view space (rows of m, w column ignored,
// in vmaddfp order); for x and y the tangent planes through the eye give
// the bounds in clip units (start -1 / 1, clamped there), with proj[0] /
// proj[5] as the scales. The rect is returned in 0..1 screen units (y
// flipped).
void View_ProjectSphere(const be_f32* sphere, const be_f32* matrix, const be_f32* proj, be_f32* out_x0, be_f32* out_y0,
                        be_f32* out_x1, be_f32* out_y1) {
    const be_f32* m = aligned16(matrix);  // lvx
    const f32 sx = sphere[0], sy = sphere[1], sz = sphere[2];
    f32 p[4];
    for (u32 k = 0; k < 4; ++k) {
        f32 v = std::fmaf(m[k], sx, 0.0f);
        v = std::fmaf(m[4 + k], sy, v);
        v = std::fmaf(m[8 + k], sz, v);
        p[k] = m[12 + k] + v;
    }
    const SphereConsts& c = sphere_consts();
    const f32 scale[2] = {proj[0], proj[5]};
    f32 lo[2] = {-1.0f, -1.0f}, hi[2] = {1.0f, 1.0f};  // stack slots, stored as -1.0f / 1.0f
    const f32 r = sphere[3];
    const f32 nz = -p[2];
    const f32 z2 = nz * nz;
    for (u32 i = 0; i < 2; ++i) {
        const f32 pi = p[i];
        const f32 d2 = fmadds(pi, pi, z2);
        const f32 t2 = fnmsubs(r, r, d2);  // d^2 - r^2
        if (!(t2 > c.zero)) continue;
        f32 t = std::sqrt(t2);
        const f32 dz = d2 * nz;
        f32 tp = t * pi;
        t = t * nz;
        f32 n0 = fnmsubs(r, nz, tp);
        const f32 d0 = fmadds(r, pi, t);
        if (dz > -(r * n0)) {
            n0 = n0 * scale[i];
            const f32 q = n0 / d0;
            lo[i] = static_cast<f32>(fsel(c.neg_one - q, c.neg_one_d, q));
        }
        tp = fmadds(r, nz, tp);
        const f32 d1 = fnmsubs(r, pi, t);
        if (dz > r * tp) {
            const f32 q = (tp * scale[i]) / d1;
            hi[i] = static_cast<f32>(fsel(q - c.one, c.one_d, q));
        }
    }
    const f32 one = c.one, half = c.half, nhalf = c.neg_half;
    *out_x0 = (lo[0] + one) * half;
    *out_y0 = (hi[1] - one) * nhalf;
    *out_x1 = (hi[0] + one) * half;
    *out_y1 = (lo[1] - one) * nhalf;
}

// void MaterialAnim_SetState(AnimTrack* t, s32 state)   (name?)
// 0: a random delay in [+12, +16) (the previous random value times
// 1/65535; the generator steps); 1: the hold time (+20) as the countdown;
// 2: stopped. Other states only clear the phase (+44), which every state
// clears.
void MaterialAnim_SetState(AnimTrack* t, s32 state) {
    AnimRandom& rnd = global<AnimRandom>(sly4::render::pool::g::anim_random);
    if (state == 0) {
        const u64 prev = rnd.last;
        const f32 lo = t->delay_lo;
        const f32 range = t->delay_hi - lo;
        const u32 seed = rnd.seed;
        const u32 x = (rotl32(seed, 4) + rotl32(seed, 16)) ^ 0x87654321u;
        const f32 f = static_cast<f32>(static_cast<s64>(prev));  // fcfid + frsp
        rnd.seed = rotl32(x, 16);
        rnd.last = x & 0xFFFF;
        const u32 w = t->flags & ~kStateMask;
        const f32 scaled = f * global<be_f32>(kRandomScale);
        t->flags = w;
        t->countdown = fmadds(range, scaled, lo);
    } else if (state == 1) {
        const u32 w = t->flags;
        *at<be_u32>(t, 40) = at<be_u32>(t, 20)->get();  // countdown = hold (raw word)
        t->flags = (w & ~kStateMask) | (1u << 12);
    } else if (state == 2) {
        t->flags = (t->flags & ~kStateMask) | (2u << 12);
    }
    *at<be_u32>(t, 44) = 0u;  // phase
}

// void MaterialAnim_Update(AnimTrack* t, f32 dt)   (name?)
// Mode 3 is frozen. State 0 counts the delay down and then runs; state 1
// counts the hold time down (when there is one) and then re-enters the
// delay with the overshoot added to dt, and animates the value: mode 0
// loops it over [0, period) at +0 per second; mode 1 either oscillates it
// as period * sin(2pi * phase / (period / rate)) (bit 28; a parabolic sine
// approximation) or ping-pongs it between 0 and period - 1e-5 by flipping
// the rate.
void MaterialAnim_Update(AnimTrack* t, f32 dt) {
    const AnimConsts& c = anim_consts();
    u32 w = t->flags;
    if (((w >> 20) & 0xFF) == 3) return;  // rlwinm 12,24,31
    const u32 state = (w >> 12) & 0xFF;   // rlwinm 20,24,31
    if (state == 0) {
        const f32 v = t->countdown - dt;
        t->countdown = v;
        if (v <= c.zero) MaterialAnim_SetState(t, 1);
        return;
    }
    if (state != 1) return;
    const f32 zero = c.zero;
    if (t->hold > zero) {
        const f32 v = t->countdown - dt;
        t->countdown = v;
        if (v <= zero) {
            dt = dt + v;
            MaterialAnim_SetState(t, 0);
            w = t->flags;
        }
    }
    const u32 mode = (w >> 20) & 0xFF;
    if (mode == 0) {
        f32 v = fmadds(t->rate, dt, t->value);
        t->value = v;
        if (v < zero) {
            v = v + t->period;
            t->value = v;
        }
        const f32 period = t->period;
        if (v >= period) t->value = v - period;
        return;
    }
    if (mode != 1) return;
    if (((w >> 28) & 1) != 0) {  // rlwinm 4,31,31
        const f32 period = t->period;
        const f32 cycle = period / t->rate;
        f32 phase = t->phase + dt;
        t->phase = phase;
        if (phase > cycle) {
            phase = phase - cycle;
            t->phase = phase;
        }
        const f32 two_pi = c.two_pi;
        const f32 a = (phase / cycle) * two_pi;
        const f32 half = static_cast<f32>(fsel(a, c.pi, c.neg_pi));  // +-pi
        f32 x = half + a;
        const f32 turns = static_cast<f32>(static_cast<f64>(fctidz(x / two_pi)));  // fctidz, fcfid, frsp
        x = fnmsubs(turns, two_pi, x);
        x = x - half;  // wrapped to [-pi, pi]
        f32 y = (x * c.c) * std::fabs(x);
        y = fmadds(x, c.b, y);
        y = fmadds(fmsubs(y, std::fabs(y), y), c.p, y);
        t->value = period * y;
        return;
    }
    const f32 rate = t->rate;
    f32 neg = rate;
    f32 v = fmadds(rate, dt, t->value);
    t->value = v;
    if (v < zero) {
        neg = -neg;
        v = zero;
        *at<be_u32>(t, 36) = 0u;
        t->rate = neg;
    }
    const f32 period = t->period;
    if (v >= period) {
        t->rate = -neg;
        t->value = period - c.eps;
    }
}

}  // namespace sly4::gfx

NATIVE(00531F80, sly4::gfx::Draw_Clear)
NATIVE(00554484, sly4::gfx::DrawItem_GetScale)
NATIVE(00557A90, sly4::gfx::View_ProjectSphere)
NATIVE(00501410, sly4::gfx::MaterialAnim_SetState)
NATIVE(00501520, sly4::gfx::MaterialAnim_Update)
