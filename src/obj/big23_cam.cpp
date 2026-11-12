// Game code, large-function sweep part 23: the camera's focus point update
// (callee of f_00101CC0 / f_00104F80 / f_00105F90).
#include "big23.h"

namespace sly4::obj {

namespace {
constexpr u32 kK = 0x00100B90u;           // .rodata: f32 0.6, f32 0.0, f64 0.6, f32 0.5, f32 1.0, f64 1.0, f64 0.0,
                                          // f32 -1.0, f64 -1.0, f32 pi/4, ... +112 f32 1e-5, f32 0.05, f64 0.05, f64 -0.0
constexpr u32 kHeightRef = 0x01032F48u;   // f32: the height t reaches 1.0 at (at least 0.6)
constexpr u32 kFocusBase = 0x01032EF0u;   // Vec3: the focus offset at t = 0
constexpr u32 kFocusMode = 0x01035410u;   // s32: 2 adds the target's vfn[35] vector to the lead
constexpr u32 kLeadLerpT = 0x01035460u;   // f32: Curve_Lerp3 t for the smoothed velocity
constexpr u32 kUpVec = 0x01164B60u;       // f32[3] (Constants_Init_0056B93C)
constexpr u32 Acosf = 0x005E649Cu;          // acosf: f1 -> f1
constexpr u32 Curve_Lerp3 = 0x004A8598u;    // (Vec3* out, const Vec3* a, const Vec3* b; f32 t, f32 u)
constexpr u32 Spring_Step3 = 0x000F8C70u;   // (Spring3* s; f32 k, f32 damping, f32 dt) (reads r3 f1 f2 f3)

// The camera's target object: +20 its state (+48 flags), +112 / +128 two
// rows, +188 the low word of the u64 +184, +192 / +200 flags; vfn[16] its
// rider (+416 kind bits), vfn[8](n) states, vfn[39](out, o, 0) its
// position, vfn[35](out, o) the lead vector.
struct CamTarget {
    pad<20> _0;
    be_u32 state;     // +20
    pad<88> _24;
    Vec3 row1;        // +112
    pad<4> _124;
    Vec3 row2;        // +128
    pad<48> _140;
    be_u32 w188;      // +188
    be_u32 flags;     // +192
    pad<4> _196;
    be_u32 f200;      // +200
};
static_assert(offsetof(CamTarget, row1) == 112 && offsetof(CamTarget, w188) == 188 && offsetof(CamTarget, f200) == 200);
struct CamFocus {
    pad<16> _0;
    be_u32 target;    // +16: CamTarget*
    pad<156> _20;
    Vec3 anchor;      // +176
    pad<4> _188;
    u8 air;           // +192
    u8 snap;          // +193
    pad<110> _194;
    Vec3 out;         // +304
    pad<20> _316;
    be_u32 flags;     // +336
    pad<124> _340;
    Vec3 last;        // +464
    pad<4> _476;
    Vec3 lead;        // +480
    pad<132> _492;
    Vec3 focus;       // +624
    pad<4> _636;
    be_f32 f640;      // +640
    be_f32 height;    // +644
    pad<12> _648;
    be_f32 k0;        // +660
    be_f32 k1;        // +664
    be_f32 lo;        // +668
    be_f32 hi;        // +672
    pad<28> _676;
    be_u32 lead_on;   // +704
    pad<204> _708;
    pad<16> spring;   // +912: the spring (Spring_Step3)
    Vec3 sp_pos;      // +928
    pad<4> _940;
    Vec3 sp_vel;      // +944
    pad<4> _956;
    Vec3 q;           // +960
    pad<4> _972;
    Vec3 qv;          // +976
    pad<4> _988;
    be_f32 w992;      // +992
    pad<4> _996;
    be_f32 damp;      // +1000
};
static_assert(offsetof(CamFocus, anchor) == 176 && offsetof(CamFocus, out) == 304 && offsetof(CamFocus, flags) == 336);
static_assert(offsetof(CamFocus, last) == 464 && offsetof(CamFocus, focus) == 624 && offsetof(CamFocus, k0) == 660);
static_assert(offsetof(CamFocus, lead_on) == 704 && offsetof(CamFocus, spring) == 912 && offsetof(CamFocus, q) == 960);
static_assert(offsetof(CamFocus, w992) == 992 && offsetof(CamFocus, damp) == 1000);
// The locals (the DECOMP's frame layout).
struct CamLocals {
    Vec4 p0;          // +0: the target position
    Vec4 vel;         // +16: the lead velocity sample
    Vec4 lerped;      // +32: Curve_Lerp3's result
    Vec4 extra;       // +48: the vfn[35] vector
};

f64 Kf(u32 off) { return lf(global<be_f32>(kK + off)); }
f64 Kd(u32 off) { return ld(global<be_f64>(kK + off)); }
CamTarget& Tgt(const CamFocus* c) { return gm<CamTarget>(c->target); }

// The gate before the focus maths (0x00100CBC..0x00100D24 and the blocks at
// 0x001012F0..0x00101528): decides the "airborne" byte c+192 from the target
// object (c+16) and copies the target position p0 to the anchor c+176 when
// the byte is set or c+336 bit 31 holds. Returns true when the anchor copy is
// forced (the original's r30), in which case c+192 is cleared afterwards.
bool FocusGate(CamFocus* c, const Vec4& p0) {
    if (c->target != 0u) {
        CamTarget& t = Tgt(c);
        if ((t.flags & 8u) != 0) {
            if (c->snap == 0) {
                // The rider check: the target's vfn[16] object (kind 1 or 3..6
                // in bits 28..30 of +416 forces the copy), then the target's
                // vfn[8](0) == 1 recomputes the byte from the state flags.
                const u32 rider = vcall<u32>(&t, 64);
                CamTarget& o = Tgt(c);
                if (rider != 0) {
                    const u32 kind = (gm<be_u32>(rider + 416) >> 28) & 7u;
                    if (kind == 1 || (kind >= 3 && kind <= 6)) {
                        CopyXYZ(c->anchor, p0);
                        return true;
                    }
                }
                if (vcall<u32>(&o, 32, 0u) == 1) {
                    CamTarget& o2 = Tgt(c);
                    const u32 st = o2.state;
                    u32 b = c->air;
                    b = (b | ((gm<be_u32>(st + 48) >> 10) & 1u)) != 0;
                    c->air = static_cast<u8>(b);
                    b = (b | ((gm<be_u32>(st + 48) >> 31) == 0 ? 1u : 0u)) != 0;
                    c->air = static_cast<u8>(b);
                    b = (b | ((gm<be_u32>(st + 48) >> 15) & 1u)) != 0;
                    c->air = static_cast<u8>(b);
                    const u32 b0 = c->air;
                    const u32 r = vcall<u32>(&o2, 32, 1u);
                    b = (b0 | (r == 2 ? 1u : 0u)) != 0;
                    c->air = static_cast<u8>(b);
                    b = (b | ((Tgt(c).w188 & 0x00100000u) != 0 ? 1u : 0u)) != 0;  // low word of the u64 +184
                    c->air = static_cast<u8>(b);
                    const bool force = rider != 0 && (gm<be_u32>(rider + 416) & 0x04000000u) != 0;
                    const bool below = lf(p0.y) < Adds(lf(c->anchor.y), lf(c->lo));
                    c->air = (b != 0 || below) ? 1 : 0;
                    if (force) {
                        CopyXYZ(c->anchor, p0);
                        return true;
                    }
                }
                goto check;
            }
            c->air = 1;
        } else {
            const u32 f = t.f200;
            if ((f & 0x800u) != 0 || (f & 0x2000u) != 0) {
                c->air = 0;
                CopyXYZ(c->anchor, p0);
                return true;
            }
        }
    }
check:
    if (c->air != 0 || (c->flags & 0x80000000u) != 0) CopyXYZ(c->anchor, p0);
    return false;
}
}  // namespace

// void Cam_UpdateFocus100C18(Cam* c; f32 dt) (no caller reads r3 or f1):
// the target object c+16 gives its position p0 (vfn[39](out, o, 0)) and
// its rows +112 / +128. After the gate above, p0.y is replaced by the
// anchor height c+180. The focus offset is kFocusBase lerped to c+624 by t
// = clamp((c+644 - 0.5) / (max(ref, 0.6) - 0.5), 0, 1), expressed in the
// frame {n, up, n x up} with n = up x row (row +128 when acos(clamp(|row1.y|))
// < pi/4, else row +112), and added up through the VMX chain. The lead
// velocity c+480 is zeroed (c+336 bit 31) or lerped toward (p0 - c+464) /
// dt (dt >= 1e-5); c+464 = p0. With c+704 the lead (or the target's
// vfn[35] vector in kFocusMode 2) scaled by c+660 / c+664 is added; y is
// clamped to [c+668, c+672]. c+193 snaps the spring (c+960 / c+928 =
// focus, velocities zero), otherwise for dt > 0 the velocity c+976 =
// (focus - c+960) / dt, c+960 = focus and the spring c+912 steps with 1 /
// max(c+992, 0.05), c+1000, dt. Finally c+992 = max(c+640, 0) (fsel) and
// c+304 = p0 + c+928.
void Cam_UpdateFocus100C18(CamFocus* c, f64 dt) {
    GuestFrame frame(64);
    CamLocals& L = *frame.as<CamLocals>();
    Vec4& p0 = L.p0;
    CamTarget& o = Tgt(c);
    const f64 r1x = lf(o.row1.x), r1y = lf(o.row1.y), r1z = lf(o.row1.z);
    const f64 r2x = lf(o.row2.x), r2y = lf(o.row2.y), r2z = lf(o.row2.z);
    call_ptr(vfn(&o, 156), &p0, &o, 0u);
    if (FocusGate(c, p0)) c->air = 0;

    set_bits(p0.y, bits(c->anchor.y));
    const f64 one = Kf(20), half = Kf(16), one_d = Kd(24), zero_d = Kd(32);
    const f64 ref0 = lf(global<be_f32>(kHeightRef));
    const f64 ref = S(Fsel(Subs(ref0, Kf(0)), ref0, Kd(8)));
    const f64 ay = std::fabs(r1y);
    const f64 yc = S(Fsel(Subs(one, ay), ay, one_d));
    f64 t = Divs(Subs(lf(c->height), half), Subs(ref, half));
    t = S(Fsel(Subs(one, t), t, one_d));
    t = S(Fsel(t, t, zero_d));
    const Vec3& base = global<Vec3>(kFocusBase);
    const f64 bx = lf(base.x), by = lf(base.y), bz = lf(base.z);
    const f64 ox = Fmadds(Subs(lf(c->focus.x), bx), t, bx);
    const f64 oy = Fmadds(Subs(lf(c->focus.y), by), t, by);
    const f64 oz = Fmadds(Subs(lf(c->focus.z), bz), t, bz);
    const f64 m1 = Kd(48);
    f64 x = S(Fsel(Subs(yc, Kf(40)), yc, m1));
    x = S(Fsel(Adds(x, one), x, m1));
    x = S(Fsel(Subs(x, one), one_d, x));
    const f64 ang = call<f64>(Acosf, x);

    const u32 flags = c->flags;
    const f64 l0 = lf(c->last.x), l1 = lf(c->last.y), l2 = lf(c->last.z);
    f64 v0 = lf(c->lead.x), v1 = lf(c->lead.y), v2 = lf(c->lead.z);
    const bool flat = ang < Kf(56);
    const f64 ax = flat ? r2x : r1x, ayy = flat ? r2y : r1y, az = flat ? r2z : r1z;
    const Vec3& up = global<Vec3>(kUpVec);
    const f64 ux = lf(up.x), uy = lf(up.y), uz = lf(up.z);
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
        CopyXYZ(c->lead, global<Vec3>(kZeroVec));
        v0 = lf(c->lead.x), v1 = lf(c->lead.y), v2 = lf(c->lead.z);
    } else if (const f64 eps = Kf(112); dt > eps || dt == eps) {
        const f64 idt = Divs(one, dt);
        L.vel.x = static_cast<f32>(Muls(Subs(lf(p0.x), l0), idt));
        L.vel.y = static_cast<f32>(Muls(Subs(lf(p0.y), l1), idt));
        L.vel.z = static_cast<f32>(Muls(Subs(lf(p0.z), l2), idt));
        call(Curve_Lerp3, &L.lerped, &c->lead, &L.vel, lf(global<be_f32>(kLeadLerpT)), dt);
        CopyXYZ(c->lead, L.lerped);
        v0 = lf(L.lerped.x), v1 = lf(L.lerped.y), v2 = lf(L.lerped.z);
    }
    const u32 lead = c->lead_on;
    CopyXYZ(c->last, p0);
    if (lead != 0) {
        if (global<be_s32>(kFocusMode) == 2) {
            CamTarget& o2 = Tgt(c);
            call_ptr(vfn(&o2, 140), &L.extra, &o2);
            v0 = lf(L.extra.x), v1 = lf(L.extra.y), v2 = lf(L.extra.z);
        }
        const f64 k0 = lf(c->k0), k1 = lf(c->k1);
        fx = Adds(fx, Muls(v0, k0));
        fy = Adds(fy, Muls(v1, k1));
        fz = Adds(fz, Muls(v2, k0));  // z uses the x scale too
    }
    const f64 hi = lf(c->hi), lo = lf(c->lo);
    fy = Fsel(Subs(hi, fy), fy, hi);
    fy = Fsel(Subs(fy, lo), fy, lo);
    auto setv = [](Vec3& d, f64 a, f64 b, f64 e) {
        d.x = static_cast<f32>(a);
        d.y = static_cast<f32>(b);
        d.z = static_cast<f32>(e);
    };
    if (c->snap != 0) {
        setv(c->q, fx, fy, fz);
        const f64 w = S(Fsel(Kd(128), zero_d, zero_d));  // always 0.0
        c->w992 = static_cast<f32>(w);
        CopyXYZ(c->qv, global<Vec3>(kZeroVec));
        // (w <= 0.0 always holds here)
        setv(c->sp_pos, fx, fy, fz);
        CopyXYZ(c->sp_vel, global<Vec3>(kZeroVec));
    } else if (!(dt <= Kf(4))) {
        const f64 idt = Divs(one, dt);
        const f64 q0 = lf(c->q.x), q1 = lf(c->q.y), q2 = lf(c->q.z);
        setv(c->q, fx, fy, fz);
        const f64 w = lf(c->w992);
        setv(c->qv, Muls(Subs(fx, q0), idt), Muls(Subs(fy, q1), idt), Muls(Subs(fz, q2), idt));
        const f64 damp = lf(c->damp);
        const f64 m = S(Fsel(Subs(Kf(116), w), Kd(120), w));
        call(Spring_Step3, &c->spring, Divs(one, m), damp, dt);
    }
    const f64 r = lf(c->f640);
    c->w992 = static_cast<f32>(S(Fsel(-r, zero_d, r)));
    setv(c->out, Adds(lf(p0.x), lf(c->sp_pos.x)), Adds(lf(p0.y), lf(c->sp_pos.y)), Adds(lf(p0.z), lf(c->sp_pos.z)));
}

}  // namespace sly4::obj

NATIVE(00100C18, sly4::obj::Cam_UpdateFocus100C18)
