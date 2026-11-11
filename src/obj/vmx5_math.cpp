// Game code, VMX sweep part 5 (see vmx5.h): vector / matrix math on objects.
#include "../sly4.h"
#include "vmx5.h"

using namespace sly4;
using namespace sly4::vmx5;

namespace {
constexpr u32 kPerm1E1B70 = 0x001E1B50;  // two vperm masks {a0 b0 a0 b3}, {a0 a1 b2 b3}
}  // namespace

// void f(f32 out[4][4], Obj* o) (called by 001E2088): the world matrix of
// o's offset p = o->+216: out = Quat_ToMatrix(p+400 quaternion, zero
// translation) times the frame M (rows 4x3 words on the stack: o's world
// rows +96, else those of o->+616 +80 when set, else, with a bone
// (o->+612 != -1), Obj_BoneMatrix(&m, o, bone, 0, 1)), its translation the
// point p+384 through M; then Matrix_NormalizeInPlace(out). The w lanes of
// M are whatever the stack holds (the original reads them with lvx). f1 is
// left as the original leaves it.
DECOMP(001E1B70, Obj_OffsetMatrix_001E1B70) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Stack frame(ctx, 336);
    const u32 sp = static_cast<u32>(ctx.gpr[1]), M = sp + 128;
    const u32 p = load<u32>(o + 216);
    CopyMat43W(M, o + 96);
    const s32 bone = static_cast<s32>(load<u32>(o + 612));
    if (bone != -1) {
        call(ctx, v4fn::Obj_BoneMatrix, {sp + 192, o, static_cast<u64>(static_cast<s64>(bone)), 0, 1});
        CopyMat43W(M, sp + 192);
    } else {
        const u32 att = load<u32>(o + 616);
        if (att != 0) CopyMat43W(M, att + 80);
    }
    const V4 px = Lvlx(p + 384), py = Lvlx(p + 388), pz = Lvlx(p + 392);
    call(ctx, v4fn::Quat_ToMatrix, {out, p + 400, kV3Zero3});
    const V4 m0 = Lvx(M), m1 = Lvx(M + 16), m2 = Lvx(M + 32), m3 = Lvx(M + 48);
    const V4 pt = Add4(m3, Row3(m0, m1, m2, px.e[0], py.e[0], pz.e[0]));
    Stvx(sp + 112, pt);
    const V4 r0 = Lvx(out), r1 = Lvx(out + 16), r2 = Lvx(out + 32), r3 = Lvx(out + 48);
    const V4 k0 = Lvx(kPerm1E1B70), k1 = Lvx(kPerm1E1B70 + 16);
    // {0, 0, 0, M3.w} from the zero vector's words and M's row 3.
    const V4 t = Perm(Perm(Lvlx(kV3Zero3), Lvlx(kV3Zero3 + 4), k0), Perm(Lvlx(kV3Zero3 + 8), m3, k0), k1);
    Stvx(out, XformDir(m0, m1, m2, r0));
    Stvx(out + 16, XformDir(m0, m1, m2, r1));
    Stvx(out + 32, XformDir(m0, m1, m2, r2));
    Stvx(out + 48, Add4(t, XformDir(m0, m1, m2, r3)));
    for (u32 k = 0; k < 3; ++k) store<u32>(out + 48 + 4 * k, LaneW(pt, k));
    SetF1Bits(ctx, LaneW(pt, 2));
    call(ctx, v5fn::Matrix_NormalizeInPlace, {out});
}

namespace {
constexpr u32 kGuard112870a = 0x01036A28, kGuard112870b = 0x01036A30;  // static-local guard bytes
constexpr u32 kRate112870 = 0x01034F00;  // f32[2] stick-to-rate scale (x, y)
constexpr u32 kEase112870 = 0x01034F60;  // f32[2] Curve_Ease limits (x, y)
constexpr u32 kMax112870 = 0x01034FC0;   // f32[2] clamp magnitudes (x, y)
constexpr u32 kZeroF112870 = 0x0011286C;  // f32 0.0
}  // namespace

// void f(T* o; f32 dt) (called through a pointer): free-look camera update.
// With bit 0 of the owner's (o->+12) doubleword +184 set: channels 3 and 2
// of the input slot (World->+4208 + 0x20C80 * (s8)owner->+172, its byte +1
// selecting the 11968-byte pad record at 0x010B19E0, +176) square-scaled
// into the pitch / yaw speeds o->+176 / +180 (each eased with
// Curve_Ease(v, 0, limit, dt) and clamped to a magnitude); o's rows +112..
// +172 turned by Matrix_RotationY(yaw * dt) (rows through the rotation,
// translation row added), then Matrix_Multiply(o+112, RotationX(pitch *
// dt)) with the translation words +160..+168 kept. Returns 0 when the bit is
// clear (the masked doubleword); else r3 / f1 as the original leaves them.
DECOMP(00112870, Cam_FreeLookUpdate_00112870) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    const u32 own = load<u32>(o + 12);
    if (load<u8>(kGuard112870a) == 0) store<u8>(kGuard112870a, 1);
    if (load<u8>(kGuard112870b) == 0) store<u8>(kGuard112870b, 1);
    if ((load<u32>(own + 188) & 1u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    Stack frame(ctx, 240);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const s32 slot = static_cast<s8>(load<u8>(own + 172));
    const u32 rec = load<u32>(kV3WorldPtr) + static_cast<u32>(slot) * 0x20C80u + 4208;
    const u32 pad = 0x010B19E0u + load<u8>(rec + 1) * 11968u + 176;
    const f64 sx = callf(ctx, v4fn::Obj_GetChannel, {pad, 3}, {});
    const f64 sy = callf(ctx, v4fn::Obj_GetChannel, {pad, 2}, {});
    const f64 zero = Lfs4(kZeroF112870);
    // pitch speed (+176)
    f32 v = Fmadds(Muls(Lfs4(kRate112870), Muls(sx, std::fabs(sx))), dt, Lfs4(o + 176));
    store<f32>(o + 176, v);
    v = static_cast<f32>(callf(ctx, v5fn::Curve_Ease, {}, {static_cast<f64>(v), zero, Lfs4(kEase112870), dt}));
    store<f32>(o + 176, v);
    {
        const f64 a = std::fabs(static_cast<f64>(v)), lim = Lfs4(kMax112870);
        if (a > lim) {
            v = Muls(v, Divs(lim, a));
            store<f32>(o + 176, v);
        }
    }
    // yaw speed (+180)
    f32 w = Fnmsubs(Muls(Lfs4(kRate112870 + 4), Muls(sy, std::fabs(sy))), dt, Lfs4(o + 180));
    store<f32>(o + 180, w);
    w = static_cast<f32>(callf(ctx, v5fn::Curve_Ease, {}, {static_cast<f64>(w), zero, Lfs4(kEase112870 + 4), dt}));
    store<f32>(o + 180, w);
    {
        const f64 a = std::fabs(static_cast<f64>(w)), lim = Lfs4(kMax112870 + 4);
        if (a > lim) {
            w = Muls(w, Divs(lim, a));
            store<f32>(o + 180, w);
        }
    }
    const u32 t0 = load<u32>(o + 160), t1 = load<u32>(o + 164), t2 = load<u32>(o + 168);
    const u32 R = sp + 112;
    callf(ctx, v5fn::Matrix_RotationY, {R}, {static_cast<f64>(Muls(w, dt))});
    {
        const V4 r0 = Lvx(R), r1 = Lvx(R + 16), r2 = Lvx(R + 32), r3 = Lvx(R + 48);
        const V4 a0 = Lvx(o + 112), a1 = Lvx(o + 128), a2 = Lvx(o + 144), a3 = Lvx(o + 160);
        Stvx(o + 112, XformDir(r0, r1, r2, a0));
        Stvx(o + 128, XformDir(r0, r1, r2, a1));
        Stvx(o + 144, XformDir(r0, r1, r2, a2));
        Stvx(o + 160, Add4(r3, XformDir(r0, r1, r2, a3)));
    }
    callf(ctx, v5fn::Matrix_RotationX, {R}, {static_cast<f64>(Muls(Lfs4(o + 176), dt))});
    call(ctx, v4fn::Matrix_Multiply, {o + 112, R});
    store<u32>(o + 160, t0);
    store<u32>(o + 164, t1);
    store<u32>(o + 168, t2);
}

namespace {
// f32 1.0, 1e-5, pi, 0; f64 pi, 0.0 (+16, +24); f32 0.001 (+32), -0.999
// (+36); a vperm mask {a0 a1 b0 a3} (+48).
constexpr u32 kK1007D0 = 0x00100790;
constexpr u32 kAxis1007D0a = 0x01164B50, kAxis1007D0b = 0x01164B60;  // f32[3] fallback axes
}  // namespace

// void f(f32 out[3], const f32 from[3], const f32 to[3]; f32 maxAngle)
// (called by 000EB978, Obj_SetRange_00101958, Cam_LookAt101A60, 00101CC0,
// 001026D0, 00102BE8, Cam_AimDir106FB8, 00148130, 0024C860,
// Obj_SetFacing2C5140, 002C5E30): turns the direction from toward to by at
// most maxAngle (clamped to [0, pi] with fsel). out = from normalized when
// |to|^2 < 1e-5 or the angle is below 0.001; to normalized when the angle
// between them is already within reach (dot >= cos) or the angle is pi;
// when nearly opposite (dot <= -0.999) from rotated by the angle about
// from x axis A (axis B when that cross is degenerate) via
// Matrix_FromAxisAngle; else cos * from + sin * (the part of to
// perpendicular to from, normalized). r3 / f1 are left as the original
// leaves them.
DECOMP(001007D0, Vec_RotateToward_001007D0) {
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const f64 ang = ctx.fpr[1];
    Stack frame(ctx, 320);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const f32 ay = F(a + 4), ax = F(a), az = F(a + 8);
    const f32 one = F(kK1007D0), eps = F(kK1007D0 + 4);
    const f32 by = F(b + 4);
    const f32 la = Sqrts(Fmadds(az, az, Fmadds(ax, ax, Muls(ay, ay))));
    const f32 bx = F(b), bz = F(b + 8);
    const f32 lb2 = Fmadds(bz, bz, Fmadds(bx, bx, Muls(by, by)));
    const f32 ia = Divs(one, la);
    const f32 nax = Muls(ia, ax), nay = Muls(ia, ay), naz = Muls(ia, az);
    auto put = [&](f32 x, f32 y, f32 z) {
        store<f32>(out, x);
        store<f32>(out + 4, y);
        store<f32>(out + 8, z);
    };
    if (std::fabs(lb2) < eps) {
        put(nax, nay, naz);
        ctx.fpr[1] = bx;
        ctx.gpr[3] = out;
        return;
    }
    const f32 lb = Sqrts(lb2);
    const f32 pi = F(kK1007D0 + 8);
    const f64 c3 = Frsp(Fsel(Subs(pi, ang), ang, Lfd(kK1007D0 + 16)));
    const f32 ang2 = static_cast<f32>(Fsel(c3, c3, Lfd(kK1007D0 + 24)));
    const f32 ib = Divs(one, lb);
    const f32 nbx = Muls(ib, bx), nby = Muls(ib, by), nbz = Muls(ib, bz);
    if (std::fabs(ang2) < F(kK1007D0 + 32)) {
        put(nax, nay, naz);
        ctx.fpr[1] = bx;
        ctx.gpr[3] = kK1007D0;
        return;
    }
    const f32 c = static_cast<f32>(callf(ctx, v5fn::Math_Cosf, {}, {static_cast<f64>(ang2)}));
    const f32 dot = Fmadds(nbz, naz, Fmadds(nbx, nax, Muls(nby, nay)));
    ctx.fpr[1] = dot;
    if (dot >= c || ang2 >= pi) {
        put(nbx, nby, nbz);  // r3: Math_Cosf's
        return;
    }
    if (dot <= F(kK1007D0 + 36)) {
        auto cross = [&](u32 z, f32& cx, f32& cy, f32& cz) {
            const f32 z0 = F(z), z1 = F(z + 4), z2 = F(z + 8);
            cy = Fmsubs(naz, z0, Muls(nax, z2));
            cx = Fmsubs(nay, z2, Muls(naz, z1));
            cz = Fmsubs(nax, z1, Muls(nay, z0));
        };
        f32 cx, cy, cz;
        cross(kAxis1007D0a, cx, cy, cz);
        if (std::fabs(Fmadds(cz, cz, Fmadds(cx, cx, Muls(cy, cy)))) < eps) cross(kAxis1007D0b, cx, cy, cz);
        const f32 il = Divs(one, Sqrts(Fmadds(cz, cz, Fmadds(cx, cx, Muls(cy, cy)))));
        store<f32>(sp + 112, Muls(cx, il));
        store<f32>(sp + 116, Muls(cy, il));
        store<f32>(sp + 120, Muls(cz, il));
        callf(ctx, v4fn::Matrix_FromAxisAngle, {sp + 144, sp + 112}, {static_cast<f64>(ang2)});
        const V4 r0 = Lvx(sp + 144), r1 = Lvx(sp + 160), r2 = Lvx(sp + 176);
        store<u32>(sp + 192, load<u32>(kV3Zero3));
        store<u32>(sp + 196, load<u32>(kV3Zero3 + 4));
        const V4 t = Perm(Lvx(sp + 192), Lvlx(kV3Zero3 + 8), Lvx(kK1007D0 + 48));  // the zero vector (lane 3 stack)
        const V4 v = Add4(t, Row3(r0, r1, r2, nax, nay, naz));
        Stvx(sp + 128, v);
        for (u32 k = 0; k < 3; ++k) store<u32>(out + 4 * k, LaneW(v, k));
        SetF1Bits(ctx, LaneW(v, 2));
        ctx.gpr[3] = 128;
        return;
    }
    const f32 ty = Fnmsubs(dot, nay, nby), tx = Fnmsubs(dot, nax, nbx), tz = Fnmsubs(dot, naz, nbz);
    const f32 lt = Sqrts(Fmadds(tz, tz, Fmadds(tx, tx, Muls(ty, ty))));
    const f32 s = Divs(callf(ctx, v5fn::Math_Sinf, {}, {static_cast<f64>(ang2)}), lt);
    const f32 ox = Fmadds(c, nax, Muls(s, tx)), oy = Fmadds(c, nay, Muls(s, ty)), oz = Fmadds(c, naz, Muls(s, tz));
    put(ox, oy, oz);
    ctx.fpr[1] = oz;  // r3: Math_Sinf's
}

namespace {
// f32 pi / 180; vperm masks {a0 b0 a0 b3} (+16), {a0 a1 b2 b3} (+32); f32 1.0 (+48).
constexpr u32 kK1F02A8 = 0x001F0270;
constexpr u32 kIdent1F02A8 = 0x01164BD0;  // f32[4][4] the identity
constexpr u32 kUp1F02A8 = 0x01164B60;     // f32[3] the up axis
constexpr u32 kTilt1F02A8 = 0x01053BF8;   // f32 tilt (degrees)
constexpr u32 kOfs1F02A8 = 0x010534C0;    // f32[3] offset x, height, z
}  // namespace

// void f(f32 out[4][4], Obj* o) (called by 001F08C0): an aim frame for o.
// M = the world rows of o->+856 (+96, 4x3 words on the stack) times
// Matrix_RotationX(-(tilt * pi/180 + Libm_Unary5E6578(o->+772))); out =
// identity words, then row 2 = the words +48..+56 of o->+500, row 1 = the
// up axis, row 0 = up x row 2 normalized, row 3 = (offset x, 0, offset z)
// through M plus M's original translation with the height added to y (the
// w lane from the multiplied row 3). r3 / f1 are left as the original
// leaves them.
DECOMP(001F02A8, Obj_AimFrame_001F02A8) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    Stack frame(ctx, 400);
    const u32 sp = static_cast<u32>(ctx.gpr[1]), M = sp + 128, R = sp + 192;
    CopyMat43W(M, load<u32>(o + 856) + 96);
    const f64 a = callf(ctx, v5fn::Libm_Unary5E6578, {o + 768}, {Lfs4(o + 772)});
    CopyMat43W(R, kIdent1F02A8);
    const f32 ang = static_cast<f32>(-std::fma(Lfs4(kTilt1F02A8), Lfs4(kK1F02A8), a));
    callf(ctx, v5fn::Matrix_RotationX, {R}, {static_cast<f64>(ang)});
    const u32 ox = load<u32>(M + 48), oy = load<u32>(M + 52), oz = load<u32>(M + 56);
    const V4 w31 = Lvlx(M + 48), w30 = Lvlx(M + 56);
    const f32 hy = F(kOfs1F02A8 + 4);
    call(ctx, v4fn::Matrix_Multiply, {M, R});
    const f32 y = Adds(std::bit_cast<f32>(oy), hy);
    const V4 r3 = Lvx(M + 48);
    store<u32>(M + 48, ox);
    store<u32>(M + 56, oz);
    store<f32>(M + 52, y);
    for (u32 k = 0; k < 4; ++k) store<f32>(sp + 256 + 4 * k, y);
    const V4 k0 = Lvx(kK1F02A8 + 16), k1 = Lvx(kK1F02A8 + 32);
    const V4 t = Perm(Perm(w31, Lvx(sp + 256), k0), Perm(w30, r3, k0), k1);
    const f32 px = Splat(Lvlx(kOfs1F02A8), 0).e[0], pz = Splat(Lvlx(kOfs1F02A8 + 8), 0).e[0];
    const V4 pt = Add4(t, Row3(Lvx(M), Lvx(M + 16), Lvx(M + 32), px, 0.0f, pz));
    Stvx(sp + 112, pt);
    const u32 fw = load<u32>(o + 500) + 16;
    const u32 f0 = load<u32>(fw + 32), f1 = load<u32>(fw + 36), f2 = load<u32>(fw + 40);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) store<u32>(out + 16 * r + 4 * k, load<u32>(kIdent1F02A8 + 16 * r + 4 * k));
    store<u32>(out + 40, f2);
    store<u32>(out + 32, f0);
    store<u32>(out + 36, f1);
    const u32 u0 = load<u32>(kUp1F02A8), u1 = load<u32>(kUp1F02A8 + 4), u2 = load<u32>(kUp1F02A8 + 8);
    store<u32>(out + 16, u0);
    store<u32>(out + 20, u1);
    store<u32>(out + 24, u2);
    {
        const f32 a0 = std::bit_cast<f32>(u0), a1 = std::bit_cast<f32>(u1), a2 = std::bit_cast<f32>(u2);
        const f32 z = F(out + 40), y2 = F(out + 36), x = F(out + 32);
        const f32 cy = Fmsubs(a2, x, Muls(a0, z)), cx = Fmsubs(a1, z, Muls(a2, y2)), cz = Fmsubs(a0, y2, Muls(a1, x));
        store<f32>(out + 4, cy);
        store<f32>(out, cx);
        store<f32>(out + 8, cz);
    }
    {
        const f32 y2 = F(out + 4), x = F(out), z = F(out + 8);
        const f32 s = Divs(F(kK1F02A8 + 48), Sqrts(Fmadds(z, z, Fmadds(x, x, Muls(y2, y2)))));
        store<f32>(out, Muls(x, s));
        store<f32>(out + 4, Muls(y2, s));
        store<f32>(out + 8, Muls(z, s));
    }
    for (u32 k = 0; k < 3; ++k) store<u32>(out + 48 + 4 * k, LaneW(pt, k));
    SetF1Bits(ctx, LaneW(pt, 2));
    ctx.gpr[3] = out + 48;
}

namespace {
// f32 fov divisor, 1.0 cap, yaw rate (+8), ease limit (+12), pitch rate
// (+16), angle scale (+20).
constexpr u32 kK10AD50 = 0x0010AD34;
}  // namespace

// void f(Cam* o, -, f32 m[4][4]; f32 dt) (called through a pointer): a
// player-driven look camera. Only with bits 0x40000000 and 0x10000000 of
// o->+176. Without an owner (o->+12), or one whose +172 lacks bit 0x40000,
// (after the owner's vt[40](owner, 0)) the owner and the o->+16 link are
// dropped (o->+16's vt[40](link, 0) first; both words cleared). With the
// owner's input bit (doubleword +184 bit 0): its pad record (as
// Cam_FreeLookUpdate_00112870); a pressed button 0x2000 (pad +8 & +12)
// toggles bit 0x80000000 of o->+176; while that bit is clear the speeds
// o->+164 / +160 ease (Curve_Ease, limit, dt) toward channel 2 / -channel
// 3 times the rates times min(m->+64 / fov, 1). Then m is turned by
// RotationY(-yaw * scale) (rows through it, translation added) and
// Matrix_Multiply(m, RotationX(-pitch * scale)), its translation words
// +48..+56 kept. r3 / f1 are left as the original leaves them.
DECOMP(0010AD50, Cam_LookUpdate_0010AD50) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 2);
    const f64 dt = ctx.fpr[1];
    Stack frame(ctx, 256);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 w = load<u32>(o + 176);
    ctx.gpr[3] = w;
    if (((w >> 30) & 1u) == 0 || ((w >> 28) & 1u) == 0) return;
    const u32 own = load<u32>(o + 12);
    if (own == 0 || ((load<u32>(own + 172) >> 18) & 1u) == 0) {
        if (own != 0) VCall(ctx, own, 40, {own, 0});
        const u32 link = load<u32>(o + 16);
        store<u32>(o + 12, 0);
        ctx.gpr[3] = link;
        if (link == 0) return;
        VCall(ctx, link, 40, {link, 0});
        store<u32>(o + 16, 0);
        ctx.gpr[3] = 0;
        return;
    }
    if ((load<u32>(own + 188) & 1u) == 0) return;  // r3: o->+176
    const s32 slot = static_cast<s8>(load<u8>(own + 172));
    const u32 rec = load<u32>(kV3WorldPtr) + static_cast<u32>(slot) * 0x20C80u + 4208;
    const u32 pad = 0x010B19E0u + load<u8>(rec + 1) * 11968u + 176;
    u32 w2 = w;
    if (((load<u32>(pad + 8) & load<u32>(pad + 12)) & 0x2000u) != 0) {
        w2 = (w & 0x7FFFFFFFu) | ((((w >> 31) & 1u) ^ 1u) << 31);
        store<u32>(o + 176, w2);
    }
    const u32 K = kK10AD50;
    if (((w2 >> 31) & 1u) == 0) {
        const f32 r = Divs(F(m + 64), F(K));
        f32 s = F(K + 4);
        if (!(r > s)) s = r;
        const f64 c2 = callf(ctx, v4fn::Obj_GetChannel, {pad, 2}, {});
        const f32 lim = F(K + 12);
        const f32 a = static_cast<f32>(callf(ctx, v5fn::Curve_Ease, {}, {Lfs4(o + 164), static_cast<f64>(Muls(Muls(s, F(K + 8)), c2)), static_cast<f64>(lim), dt}));
        store<f32>(o + 164, a);
        const f64 c3 = callf(ctx, v4fn::Obj_GetChannel, {pad, 3}, {});
        const f32 b = static_cast<f32>(callf(ctx, v5fn::Curve_Ease, {}, {Lfs4(o + 160), static_cast<f64>(Muls(Muls(s, F(K + 16)), -c3)), static_cast<f64>(lim), dt}));
        store<f32>(o + 160, b);
    }
    const f32 k = F(K + 20);
    const u32 R = sp + 112, t = m + 48;
    const u32 t0 = load<u32>(t), t1 = load<u32>(t + 4), t2 = load<u32>(t + 8);
    callf(ctx, v5fn::Matrix_RotationY, {R}, {static_cast<f64>(-Muls(F(o + 164), k))});
    {
        const V4 r0 = Lvx(R), r1 = Lvx(R + 16), r2 = Lvx(R + 32), r3 = Lvx(R + 48);
        const V4 a0 = Lvx(m), a1 = Lvx(m + 16), a2 = Lvx(m + 32), a3 = Lvx(m + 48);
        Stvx(m, XformDir(r0, r1, r2, a0));
        Stvx(m + 16, XformDir(r0, r1, r2, a1));
        Stvx(m + 32, XformDir(r0, r1, r2, a2));
        Stvx(m + 48, Add4(r3, XformDir(r0, r1, r2, a3)));
    }
    callf(ctx, v5fn::Matrix_RotationX, {R}, {static_cast<f64>(-Muls(F(o + 160), k))});
    call(ctx, v4fn::Matrix_Multiply, {m, R});
    store<u32>(t, t0);
    store<u32>(t + 4, t1);
    store<u32>(t + 8, t2);
}
