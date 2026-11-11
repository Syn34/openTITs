// Game code, VMX sweep part 7 (see vmx7.h): view-dependent set-up.
#include "../sly4.h"
#include "vmx7.h"

using namespace sly4;
using namespace sly4::vmx7;

namespace {
constexpr u32 kVec4_Transform = 0x005539E4;  // (f32 v[4] in / out, const Matrix*)
constexpr u32 kMat4_Invert = 0x00554C28;  // (f32 tmp[16], f32 out[16], const f32 in[16])
constexpr u32 kMatrix_Perspective = 0x0056A200;  // (f32 out[16]; f1 fov, f2 aspect, f3 near, f4 far)
constexpr u32 kGridView = 0x010AA1B0;  // +4 u32 use the fixed projection; +96.. the frustum block F
constexpr u32 kGridViewK = 0x004294E0;  // f32 {0.5, 1, fov 1.0472, near 0.1, far 50}
constexpr u32 kGridAspect = 0x010C5A20;  // +16 f32 the aspect ratio
// The four NDC corners {x, y, 0.5, 1} written to F+16.. (in store order).
constexpr u32 kGridCorners[16] = {0xBF800000, 0xBF800000, 0x3F000000, 0x3F800000, 0x3F800000, 0xBF800000, 0x3F000000, 0x3F800000,
                                  0x3F800000, 0x3F800000, 0x3F000000, 0x3F800000, 0xBF800000, 0x3F800000, 0x3F000000, 0x3F800000};
}  // namespace

// u32 f(Camera* o) (from 0x00429950): builds the frustum block F =
// kGridView + 96: F+176 = o's view-projection (+880, 16 words); the
// inverse goes to F+112 by Mat4_Invert of o's matrix +880 (o's +944 copied
// to F+240 first) or, when kGridView->+4 is set, of a fixed perspective
// (fov 60 degrees, the global aspect, 0.1 .. 50) whose transpose is kept at
// F+240. The four near-plane NDC corners at F+16.. are unprojected
// (Vec4_Transform, then divided by w), F+80 = the midpoint of corners 0
// and 2, F+0.. = the plane {n, -n.p0} through corners 0 / 1 / 2 (n unit),
// F+96 = o's position +864. Returns o + 864; f1 is o->+872.
DECOMP(004294F8, Grid_BuildViewCorners_004294F8) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 384);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 F = kGridView + 96, K = kGridViewK;
    for (u32 k = 0; k < 16; ++k) store<u32>(F + 16 + 4 * k, kGridCorners[k]);
    const f64 half = Lfs4(K), one = Lfs4(K + 4);
    CopyW6(F + 176, o + 880, 16);
    const u32 inv = F + 112;
    if (load<u32>(kGridView + 4) == 0) {
        CopyW6(F + 240, o + 944, 16);
        call(ctx, kMat4_Invert, {sp + 112, inv, o + 880});
    } else {
        callf(ctx, kMatrix_Perspective, {sp + 240}, {Lfs4(K + 8), Lfs4(kGridAspect + 16), Lfs4(K + 12), Lfs4(K + 16)});
        const V4 r0 = Lvx(sp + 240), r1 = Lvx(sp + 256), r2 = Lvx(sp + 272), r3 = Lvx(sp + 288);
        const V4 a = Mrghw(r0, r2), b = Mrghw(r1, r3), c = Mrglw(r0, r2), d = Mrglw(r1, r3);
        Stvx(F + 240, Mrghw(a, b));
        Stvx(F + 256, Mrglw(a, b));
        Stvx(F + 272, Mrghw(c, d));
        Stvx(F + 288, Mrglw(c, d));
        call(ctx, kMat4_Invert, {sp + 176, inv, sp + 240});
    }
    for (u32 i = 0; i < 4; ++i) {
        const u32 p = F + 16 + 16 * i;
        call(ctx, kVec4_Transform, {p, inv});
        const f32 s = Divs(one, Lfs4(p + 12));
        const f32 x = Muls(Lfs4(p), s), y = Muls(Lfs4(p + 4), s), z = Muls(Lfs4(p + 8), s), w = Muls(Lfs4(p + 12), s);
        store<f32>(p, x);
        store<f32>(p + 4, y);
        store<f32>(p + 8, z);
        store<f32>(p + 12, w);
    }
    const f32 mx = Muls(Adds(Lfs4(F + 16), Lfs4(F + 48)), half);
    const f32 my = Muls(Adds(Lfs4(F + 20), Lfs4(F + 52)), half);
    const f32 mz = Muls(Adds(Lfs4(F + 24), Lfs4(F + 56)), half);
    store<f32>(F + 80, mx);
    store<f32>(F + 84, my);
    store<f32>(F + 88, mz);
    const u32 p0 = F + 16, p1 = F + 32, p2 = F + 48;
    const f32 ax = Subs(Lfs4(p1), Lfs4(p0)), ay = Subs(Lfs4(p1 + 4), Lfs4(p0 + 4)), az = Subs(Lfs4(p1 + 8), Lfs4(p0 + 8));
    const f32 bx = Subs(Lfs4(p2), Lfs4(p0)), by = Subs(Lfs4(p2 + 4), Lfs4(p0 + 4)), bz = Subs(Lfs4(p2 + 8), Lfs4(p0 + 8));
    f32 ny = Fmsubs(az, bx, Muls(ax, bz));
    f32 nx = Fmsubs(ay, bz, Muls(az, by));
    f32 nz = Fmsubs(ax, by, Muls(ay, bx));
    store<u32>(F + 12, 0x3F800000);
    const f32 rl = Divs(one, Sqrts(Fmadds(nz, nz, Fmadds(nx, nx, Muls(ny, ny)))));
    nx = Muls(nx, rl);
    ny = Muls(ny, rl);
    nz = Muls(nz, rl);
    store<f32>(F, nx);
    store<f32>(F + 4, ny);
    store<f32>(F + 8, nz);
    const f32 dd = Fmadds(nx, Lfs4(p0), Muls(ny, Lfs4(p0 + 4)));
    store<f32>(F + 12, static_cast<f32>(-std::fma(static_cast<f64>(nz), Lfs4(p0 + 8), static_cast<f64>(dd))));
    CopyW6(F + 96, o + 864, 3);
    ctx.fpr[1] = Lfs4(o + 872);
    ctx.gpr[3] = o + 864;
}


// u32 f(f32 out[4][4], const Attach* a) (from several callers): an
// attachment's world matrix (x / y / z words of the four rows written).
// - a->+24 bit 0x10000000 with a camera (World +0x1281E0): the camera's
//   rows (+96); returns the camera + 96, f1 its +152.
// - a->+80 set: that matrix; returns a->+8, f1 its +56.
// - else the base B = (a->+32) + 80 (when a->+32 is set) or the bone
//   a->+28 of the object a->+8 (Obj_BoneMatrixCE688(.., 1, 1)): the
//   position is B's rows times a's local offset (+48, vmaddfp) plus B's
//   translation; the rotation is B's (or, with a->+24 bit 0x20000000, the
//   object's +96 rows) times Quat_ToMatrix(a's quaternion +64)
//   (Matrix_Multiply); r3 is what Matrix_Multiply leaves, f1 the +40 word.
DECOMP(00271080, Attach_WorldMatrix_00271080) {
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 obj = load<u32>(a + 8);
    if ((load<u32>(a + 24) & 0x10000000u) != 0) {
        const u32 cam = load<u32>(load<u32>(kV3WorldPtr) + kV3WorldCam);
        if (cam != 0) {
            CopyRowsXyz7(out, cam + 96, 4);
            ctx.fpr[1] = Lfs4(cam + 96 + 56);
            ctx.gpr[3] = cam + 96;
            return;
        }
    }
    const u32 m = load<u32>(a + 80);
    if (m != 0) {
        CopyRowsXyz7(out, m, 4);
        ctx.fpr[1] = Lfs4(m + 56);
        ctx.gpr[3] = obj;
        return;
    }
    Stack frame(ctx, 384);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 S = sp + 128;
    CopyRowsXyz7(S, kAttachIdentity, 4);
    u32 tz;  // B's translation z (f29)
    const u32 b = load<u32>(a + 32);
    if (b != 0) {
        CopyRowsXyz7(S, b + 80, 3);
        CopyW6(S + 48, b + 80 + 48, 2);
        tz = load<u32>(b + 80 + 56);
    } else {
        call(ctx, v4fn::Obj_BoneMatrix, {sp + 256, obj, static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(a + 28)))), 1, 1});
        CopyRowsXyz7(S, sp + 256, 4);
        tz = load<u32>(sp + 256 + 56);
    }
    const V4 r0 = Lvx(S), r1 = Lvx(S + 16), r2 = Lvx(S + 32), r3 = Lvx(S + 48);
    const V4 t = {{r3.e[0], r3.e[1], std::bit_cast<f32>(tz), r3.e[3]}};  // vperm 0x00271070
    const V4 p = Add4(t, Madd4(r2, big7::Splat1(F(a + 56)), Madd4(r1, big7::Splat1(F(a + 52)), Mul0(r0, big7::Splat1(F(a + 48))))));
    store<u32>(S + 56, tz);
    call(ctx, v4fn::Quat_ToMatrix, {sp + 192, a + 64, kV3Zero3});
    if ((load<u32>(a + 24) & 0x20000000u) != 0) CopyRowsXyz7(S, load<u32>(a + 8) + 96, 3);
    CopyW6(S + 48, kV3Zero3, 3);
    call(ctx, v4fn::Matrix_Multiply, {S, sp + 192});
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k)
            if (r != 2 || k != 2) store<u32>(out + 16 * r + 4 * k, load<u32>(S + 16 * r + 4 * k));
    store<f32>(out + 48, p.e[0]);
    store<u32>(out + 40, load<u32>(S + 40));
    store<f32>(out + 52, p.e[1]);
    store<f32>(out + 56, p.e[2]);
    ctx.fpr[1] = Lfs4(S + 40);
}

namespace {
constexpr u32 kLookK115F90 = 0x00115F90;  // f32 {0, 1e-5, 1, pi / 180}, then a vperm mask (+16)
constexpr u32 kLookFallbackFwd = 0x01164B70;  // f32[3] the forward axis used when the direction is vertical
}  // namespace

// u32 f(Cam* o, -, f32 out[4][4]) (called through a pointer): a camera
// looking from object A = o->+12 to B = o->+16. Without both (or without
// their +172 bit 0x40000) out's rows are o's +32.. rows (returns A, f1
// the +88 word). Else, with d = B's position - A's (rows at +96): the
// forward axis F is d's horizontal part (out's +32 / +40 when that is
// shorter than 1e-5, then the global axis when still so), normalised; the
// rows are {U x F, U, F, (A.x, o->+144, A.z)} with U the global up row
// (0x01164BD0 row 1), times Matrix_RotationX(o->+132 degrees); then
// A->vfn[39](&d, A, rows) gives a point (d's stack slot), and out's
// translation = that point + rows x o's offset (+112, vmaddfp) - out's
// row 2 * o->+128; out->+64 = o->+104. Returns 128 (r3 as left), f1 the
// +104 word.
DECOMP(00115FB0, Cam_LookBetween_00115FB0) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 2);
    const u32 A = load<u32>(o + 12);
    bool ok = A != 0 && (load<u32>(A + 172) & 0x40000u) != 0;
    if (ok) {
        const u32 B = load<u32>(o + 16);
        ok = B != 0 && (load<u32>(B + 172) & 0x40000u) != 0;
    }
    if (!ok) {
        CopyRowsXyz7(out, o + 32, 4);
        ctx.fpr[1] = Lfs4(o + 88);
        ctx.gpr[3] = A;
        return;
    }
    const u32 B = load<u32>(o + 16);
    Stack frame(ctx, 304);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 pa = A + 96, pb = B + 96, I = kAttachIdentity, K = kLookK115F90, S = sp + 144;
    const u32 wax = load<u32>(pa + 48), waz = load<u32>(pa + 56);
    const f64 ax = Lfs4(pa + 48), az = Lfs4(pa + 56);
    const f32 dx = Subs(Lfs4(pb + 48), ax), dy = Subs(Lfs4(pb + 52), Lfs4(pa + 52)), dz = Subs(Lfs4(pb + 56), az);
    CopyRowsXyz7(S, I, 4);
    const u32 wux = load<u32>(I + 16), wuy = load<u32>(I + 20), wuz = load<u32>(I + 24);
    const f64 ux = Lfs4(I + 16), uy = Lfs4(I + 20), uz = Lfs4(I + 24);
    store<f32>(sp + 112, dx);
    store<f32>(sp + 116, dy);
    store<f32>(sp + 120, dz);
    store<f32>(S + 32, dx);
    store<f32>(S + 40, dz);
    const f64 k0 = Lfs4(K);
    const f32 k0sq = Muls(k0, k0);
    store<u32>(S + 36, 0);
    const f64 eps = Lfs4(K + 4);
    f64 fx = dx, fz = dz;
    if (std::fabs(Fmadds(fz, fz, Fmadds(fx, fx, k0sq))) < eps) {
        fx = Lfs4(out + 32);
        store<u32>(S + 32, load<u32>(out + 32));
        fz = Lfs4(out + 40);
        store<u32>(S + 36, 0);
        store<u32>(S + 40, load<u32>(out + 40));
    }
    f64 Fx = fx, Fy = k0, Fz = fz;
    if (std::fabs(Fmadds(fz, fz, Fmadds(fx, fx, k0sq))) < eps) {
        Fx = Lfs4(kLookFallbackFwd);
        store<u32>(S + 32, load<u32>(kLookFallbackFwd));
        Fy = Lfs4(kLookFallbackFwd + 4);
        store<u32>(S + 36, load<u32>(kLookFallbackFwd + 4));
        Fz = Lfs4(kLookFallbackFwd + 8);
    }
    const f32 s = Divs(Lfs4(K + 8), Sqrts(Fmadds(Fz, Fz, Fmadds(Fx, Fx, Muls(Fy, Fy)))));
    const f32 nx = Muls(Fx, s), ny = Muls(Fy, s), nz = Muls(Fz, s);
    store<u32>(S + 48, wax);
    store<u32>(S + 56, waz);
    const f32 rz = Fmsubs(ux, ny, Muls(uy, nx));
    const f32 rx = Fmsubs(uy, nz, Muls(uz, ny));
    const f32 ry = Fmsubs(uz, nx, Muls(ux, nz));
    store<f32>(S + 32, nx);
    store<f32>(S + 36, ny);
    store<f32>(S + 40, nz);
    store<f32>(S + 8, rz);
    store<f32>(S, rx);
    store<f32>(S + 4, ry);
    const u32 w144 = load<u32>(o + 144);
    store<u32>(S + 52, w144);
    store<f32>(out, rx);
    store<f32>(out + 4, ry);
    store<f32>(out + 8, rz);
    store<u32>(out + 16, wux);
    store<u32>(out + 20, wuy);
    store<u32>(out + 24, wuz);
    store<f32>(out + 32, nx);
    store<f32>(out + 36, ny);
    store<f32>(out + 40, nz);
    store<u32>(out + 48, wax);
    store<u32>(out + 52, w144);
    store<u32>(out + 56, waz);
    CopyRowsXyz7(sp + 208, I, 4);
    callf(ctx, v5fn::Matrix_RotationX, {sp + 208}, {Muls(Lfs4(o + 132), Lfs4(K + 12))});
    call(ctx, v4fn::Matrix_Multiply, {out, sp + 208});
    const u32 A2 = load<u32>(o + 12);
    VCall(ctx, A2, 156, {sp + 112, A2, S});
    const f64 x1 = Lfs4(sp + 112);
    store<u32>(out + 48, load<u32>(sp + 112));
    const f64 y1 = Lfs4(sp + 116);
    store<u32>(out + 52, load<u32>(sp + 116));
    const f64 z1 = Lfs4(sp + 120);
    store<u32>(out + 56, load<u32>(sp + 120));
    const V4 s0 = Lvx(S), s1 = Lvx(S + 16), s2 = Lvx(S + 32), s3 = Lvx(S + 48);
    const V4 t = {{0.0f, 0.0f, 0.0f, s3.e[3]}};  // vperm 0x00115FA0: zero, zero, zero, row 3's w
    const V4 v = Add4(t, Madd4(s2, big7::Splat1(F(o + 120)), Madd4(s1, big7::Splat1(F(o + 116)), Mul0(s0, big7::Splat1(F(o + 112))))));
    const f32 x2 = Adds(x1, v.e[0]), y2 = Adds(y1, v.e[1]), z2 = Adds(z1, v.e[2]);
    store<f32>(out + 48, x2);
    store<f32>(out + 52, y2);
    store<f32>(out + 56, z2);
    const f64 back = Lfs4(o + 128);
    const f64 bx = Lfs4(out + 32), by = Lfs4(out + 36), bz = Lfs4(out + 40);
    store<f32>(out + 48, Fnmsubs(bx, back, x2));
    store<f32>(out + 52, Fnmsubs(by, back, y2));
    store<f32>(out + 56, Fnmsubs(bz, back, z2));
    store<u32>(out + 64, load<u32>(o + 104));
    ctx.fpr[1] = Lfs4(o + 104);
    ctx.gpr[3] = 128;
}
