// Game code, large-function sweep part 23: the camera's focus point update
// (callee of f_00101CC0 / f_00104F80 / f_00105F90).
#include "big23.h"

using namespace sly4;
using namespace sly4::big23;

namespace {
constexpr u32 kK = 0x00100B90u;           // .rodata: f32 0.6, f32 0.0, f64 0.6, f32 0.5, f32 1.0, f64 1.0, f64 0.0,
                                          // f32 -1.0, f64 -1.0, f32 pi/4, ... +112 f32 1e-5, f32 0.05, f64 0.05, f64 -0.0
constexpr u32 kHeightRef = 0x01032F48u;   // f32: the height t reaches 1.0 at (at least 0.6)
constexpr u32 kFocusBase = 0x01032EF0u;   // Vec3: the focus offset at t = 0
constexpr u32 kFocusMode = 0x01035410u;   // s32: 2 adds the target's vfn[35] vector to the lead
constexpr u32 kLeadLerpT = 0x01035460u;   // f32: Curve_Lerp3 t for the smoothed velocity
constexpr u32 kZeroVec = 0x01164B40u;     // f32[3] 0
constexpr u32 kUpVec = 0x01164B60u;       // f32[3] (Constants_Init_0056B93C)

// The stfs of the three words at src to dst (lfs / stfs pairs).
void Cw3(u32 dst, u32 src) {
    Cw(dst, src);
    Cw(dst + 4, src + 4);
    Cw(dst + 8, src + 8);
}

// The gate before the focus maths (0x00100CBC..0x00100D24 and the blocks at
// 0x001012F0..0x00101528): decides the "airborne" byte c+192 from the target
// object (c+16) and copies the target position p0 to the anchor c+176 when
// the byte is set or c+336 bit 31 holds. Returns true when the anchor copy is
// forced (the original's r30), in which case c+192 is cleared afterwards.
bool FocusGate(PPUContext& ctx, u32 c, u32 p0) {
    const u32 t = load<u32>(c + 16);
    if (t != 0) {
        if ((load<u32>(t + 192) & 8u) != 0) {
            if (load<u8>(c + 193) == 0) {
                // The rider check: the target's vfn[16] object (kind 1 or 3..6
                // in bits 28..30 of +416 forces the copy), then the target's
                // vfn[8](0) == 1 recomputes the byte from the state flags.
                const u32 rider = static_cast<u32>(VCall(ctx, t, 64, {t}));
                const u32 o = load<u32>(c + 16);
                if (rider != 0) {
                    const u32 kind = (load<u32>(rider + 416) >> 28) & 7u;
                    if (kind == 1 || (kind >= 3 && kind <= 6)) {
                        Cw3(c + 176, p0);
                        return true;
                    }
                }
                if (static_cast<u32>(VCall(ctx, o, 32, {o, 0})) == 1) {
                    const u32 o2 = load<u32>(c + 16);
                    const u32 st = load<u32>(o2 + 20);
                    u32 b = load<u8>(c + 192);
                    b = (b | ((load<u32>(st + 48) >> 10) & 1u)) != 0;
                    store<u8>(c + 192, static_cast<u8>(b));
                    b = (b | ((load<u32>(st + 48) >> 31) == 0 ? 1u : 0u)) != 0;
                    store<u8>(c + 192, static_cast<u8>(b));
                    b = (b | ((load<u32>(st + 48) >> 15) & 1u)) != 0;
                    store<u8>(c + 192, static_cast<u8>(b));
                    const u32 b0 = load<u8>(c + 192);
                    const u32 r = static_cast<u32>(VCall(ctx, o2, 32, {o2, 1}));
                    b = (b0 | (r == 2 ? 1u : 0u)) != 0;
                    store<u8>(c + 192, static_cast<u8>(b));
                    const u32 o3 = load<u32>(c + 16);
                    b = (b | ((load<u32>(o3 + 188) & 0x00100000u) != 0 ? 1u : 0u)) != 0;  // low word of the u64 +184
                    store<u8>(c + 192, static_cast<u8>(b));
                    const bool force = rider != 0 && (load<u32>(rider + 416) & 0x04000000u) != 0;
                    const bool below = Lf(p0 + 4) < Adds(Lf(c + 180), Lf(c + 668));
                    store<u8>(c + 192, (b != 0 || below) ? 1 : 0);
                    if (force) {
                        Cw3(c + 176, p0);
                        return true;
                    }
                }
                goto check;
            }
            store<u8>(c + 192, 1);
        } else {
            const u32 f = load<u32>(t + 200);
            if ((f & 0x800u) != 0 || (f & 0x2000u) != 0) {
                store<u8>(c + 192, 0);
                Cw3(c + 176, p0);
                return true;
            }
        }
    }
check:
    if (load<u8>(c + 192) != 0 || (load<u32>(c + 336) & 0x80000000u) != 0) Cw3(c + 176, p0);
    return false;
}
}  // namespace

// void Cam_UpdateFocus100C18(Cam* c; f32 dt): the target object c+16 gives
// its position p0 (vfn[39](out, o, 0)) and its rows +112 / +128. After the
// gate above, p0.y is replaced by the anchor height c+180. The focus offset
// is kFocusBase lerped to c+624 by t = clamp((c+644 - 0.5) / (max(ref, 0.6)
// - 0.5), 0, 1), expressed in the frame {n, up, n x up} with n = up x row
// (row +128 when acos(clamp(|row1.y|)) < pi/4, else row +112), and added
// up through the VMX chain. The lead velocity c+480 is zeroed (c+336 bit
// 31) or lerped toward (p0 - c+464) / dt (dt >= 1e-5); c+464 = p0. With
// c+704 the lead (or the target's vfn[35] vector in kFocusMode 2) scaled
// by c+660 / c+664 is added; y is clamped to [c+668, c+672]. c+193 snaps
// the spring (c+960 / c+928 = focus, velocities zero), otherwise for dt > 0
// the velocity c+976 = (focus - c+960) / dt, c+960 = focus and the spring
// c+912 steps with 1 / max(c+992, 0.05), c+1000, dt. Finally c+992 =
// max(c+640, 0) (fsel) and c+304 = p0 + c+928.
DECOMP(00100C18, Cam_UpdateFocus100C18) {
    const u32 c = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    Stack frame(ctx, 64);
    const u32 p0 = frame.addr();     // r1+112: the target position
    const u32 vel = p0 + 16;         // r1+160: the lead velocity sample
    const u32 lerped = p0 + 32;      // r1+144: Curve_Lerp3's result
    const u32 extra = p0 + 48;       // r1+176: the vfn[35] vector
    const u32 o = load<u32>(c + 16);
    const f64 r1x = Lf(o + 112), r1y = Lf(o + 116), r1z = Lf(o + 120);
    const f64 r2x = Lf(o + 128), r2y = Lf(o + 132), r2z = Lf(o + 136);
    VCall(ctx, o, 156, {p0, o, 0});
    if (FocusGate(ctx, c, p0)) store<u8>(c + 192, 0);

    Cw(p0 + 4, c + 180);
    const f64 one = Lf(kK + 20), half = Lf(kK + 16), one_d = Lfd(kK + 24), zero_d = Lfd(kK + 32);
    const f64 ref0 = Lf(kHeightRef);
    const f64 ref = S(Fsel(Subs(ref0, Lf(kK)), ref0, Lfd(kK + 8)));
    const f64 ay = std::fabs(r1y);
    const f64 yc = S(Fsel(Subs(one, ay), ay, one_d));
    f64 t = Divs(Subs(Lf(c + 644), half), Subs(ref, half));
    t = S(Fsel(Subs(one, t), t, one_d));
    t = S(Fsel(t, t, zero_d));
    const f64 bx = Lf(kFocusBase), by = Lf(kFocusBase + 4), bz = Lf(kFocusBase + 8);
    const f64 ox = Fmadds(Subs(Lf(c + 624), bx), t, bx);
    const f64 oy = Fmadds(Subs(Lf(c + 628), by), t, by);
    const f64 oz = Fmadds(Subs(Lf(c + 632), bz), t, bz);
    const f64 m1 = Lfd(kK + 48);
    f64 x = S(Fsel(Subs(yc, Lf(kK + 40)), yc, m1));
    x = S(Fsel(Adds(x, one), x, m1));
    x = S(Fsel(Subs(x, one), one_d, x));
    const f64 ang = callf(ctx, b23fn::Acosf, {}, {x});

    const u32 flags = load<u32>(c + 336);
    const f64 l0 = Lf(c + 464), l1 = Lf(c + 468), l2 = Lf(c + 472);
    f64 v0 = Lf(c + 480), v1 = Lf(c + 484), v2 = Lf(c + 488);
    const bool flat = ang < Lf(kK + 56);
    const f64 ax = flat ? r2x : r1x, ayy = flat ? r2y : r1y, az = flat ? r2z : r1z;
    const f64 ux = Lf(kUpVec), uy = Lf(kUpVec + 4), uz = Lf(kUpVec + 8);
    // n = up x row, normalised; d = n x up.
    const f64 cy = Fmsubs(uz, ax, Muls(ux, az));
    const f64 cx = Fmsubs(uy, az, Muls(uz, ayy));
    const f64 cz = Fmsubs(ux, ayy, Muls(uy, ax));
    const f64 inv = Divs(one, Sqrts(Fmadds(cz, cz, Fmadds(cx, cx, Muls(cy, cy)))));
    const f64 nz = Muls(cz, inv), ny = Muls(cy, inv), nx = Muls(cx, inv);
    const f64 dz = Fmsubs(nx, uy, Muls(ny, ux));
    const f64 dy = Fmsubs(nz, ux, Muls(nx, uz));
    const f64 dx = Fmsubs(ny, uz, Muls(nz, uy));
    // vmaddfp chain from a +0 accumulator: d * oz + (up * oy + (n * ox + 0)).
    const auto lane = [&](f64 n, f64 u, f64 d) {
        f32 v = std::fma(static_cast<f32>(n), static_cast<f32>(ox), 0.0f);
        v = std::fma(static_cast<f32>(u), static_cast<f32>(oy), v);
        return static_cast<f64>(std::fma(static_cast<f32>(d), static_cast<f32>(oz), v));
    };
    f64 fx = lane(nx, ux, dx), fy = lane(ny, uy, dy), fz = lane(nz, uz, dz);

    if ((flags & 0x80000000u) != 0) {
        Cw3(c + 480, kZeroVec);
        v0 = Lf(c + 480), v1 = Lf(c + 484), v2 = Lf(c + 488);
    } else if (const f64 eps = Lf(kK + 112); dt > eps || dt == eps) {
        const f64 idt = Divs(one, dt);
        Sf(vel, Muls(Subs(Lf(p0), l0), idt));
        Sf(vel + 4, Muls(Subs(Lf(p0 + 4), l1), idt));
        Sf(vel + 8, Muls(Subs(Lf(p0 + 8), l2), idt));
        callf(ctx, b23fn::Curve_Lerp3, {lerped, c + 480, vel}, {Lf(kLeadLerpT), dt});
        Cw3(c + 480, lerped);
        v0 = Lf(lerped), v1 = Lf(lerped + 4), v2 = Lf(lerped + 8);
    }
    const u32 lead = load<u32>(c + 704);
    Cw3(c + 464, p0);
    if (lead != 0) {
        if (load<s32>(kFocusMode) == 2) {
            const u32 o2 = load<u32>(c + 16);
            VCall(ctx, o2, 140, {extra, o2});
            v0 = Lf(extra), v1 = Lf(extra + 4), v2 = Lf(extra + 8);
        }
        const f64 k0 = Lf(c + 660), k1 = Lf(c + 664);
        fx = Adds(fx, Muls(v0, k0));
        fy = Adds(fy, Muls(v1, k1));
        fz = Adds(fz, Muls(v2, k0));  // z uses the x scale too
    }
    const f64 hi = Lf(c + 672), lo = Lf(c + 668);
    fy = Fsel(Subs(hi, fy), fy, hi);
    fy = Fsel(Subs(fy, lo), fy, lo);
    if (load<u8>(c + 193) != 0) {
        Sf(c + 960, fx);
        Sf(c + 964, fy);
        Sf(c + 968, fz);
        const f64 w = S(Fsel(Lfd(kK + 128), zero_d, zero_d));  // always 0.0
        Sf(c + 992, w);
        Cw3(c + 976, kZeroVec);
        // (w <= 0.0 always holds here)
        Sf(c + 928, fx);
        Sf(c + 932, fy);
        Sf(c + 936, fz);
        Cw3(c + 944, kZeroVec);
    } else if (!(dt <= Lf(kK + 4))) {
        const f64 idt = Divs(one, dt);
        const f64 q0 = Lf(c + 960), q1 = Lf(c + 964), q2 = Lf(c + 968);
        Sf(c + 960, fx);
        Sf(c + 964, fy);
        Sf(c + 968, fz);
        const f64 w = Lf(c + 992);
        Sf(c + 976, Muls(Subs(fx, q0), idt));
        Sf(c + 980, Muls(Subs(fy, q1), idt));
        Sf(c + 984, Muls(Subs(fz, q2), idt));
        const f64 damp = Lf(c + 1000);
        const f64 m = S(Fsel(Subs(Lf(kK + 116), w), Lfd(kK + 120), w));
        callf(ctx, b23fn::Spring_Step3, {c + 912}, {Divs(one, m), damp, dt});
    }
    const f64 r = Lf(c + 640);
    Sf(c + 992, S(Fsel(-r, zero_d, r)));
    Sf(c + 304, Adds(Lf(p0), Lf(c + 928)));
    Sf(c + 308, Adds(Lf(p0 + 4), Lf(c + 932)));
    Sf(c + 312, Adds(Lf(p0 + 8), Lf(c + 936)));
}
