// Game code, VMX sweep part 4 (see vmx4.h): vector / matrix math on objects.
#include "../sly4.h"
#include "vmx4.h"

using namespace sly4;
using namespace sly4::vmx4;

// void f(f32 out[3], Obj* o) (called through a pointer): a world point of o.
// Without o->+216 (p): the zero vector. With a bone (o->+744 != -1): the
// translation row of Obj_BoneMatrix(&m, o, bone, 1, 1). With an attached
// matrix o->+748: its words +128..+136. Else p's offset (p+416..+424),
// through o's matrix (+96 rows) when o->+388 is null or its u16 +8 is 0;
// otherwise the offset plus (*q ->+28)+368.. through the rows at that
// block's +240 (vector multiply-adds from +0, then + row 3). f1 / r3 are
// left as the original leaves them.
DECOMP(001CE92C, Obj_WorldPoint_001CE92C) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 p = load<u32>(o + 216);
    if (p == 0) {
        for (u32 k = 0; k < 3; ++k) CopyW(out + 4 * k, kV3Zero3 + 4 * k);
        SetF1Bits(ctx, load<u32>(out + 8));
        ctx.gpr[3] = 0x01160000u;
        return;
    }
    const s32 bone = static_cast<s32>(load<u32>(o + 744));
    if (bone != -1) {
        Stack frame(ctx, 288);
        const u32 m = static_cast<u32>(ctx.gpr[1]) + 128;
        call(ctx, v4fn::Obj_BoneMatrix, {m, o, static_cast<u64>(static_cast<s64>(bone)), 1, 1});
        for (u32 k = 0; k < 3; ++k) CopyW(out + 4 * k, m + 48 + 4 * k);
        SetF1Bits(ctx, load<u32>(out + 8));
        return;
    }
    const u32 att = load<u32>(o + 748);
    if (att != 0) {
        for (u32 k = 0; k < 3; ++k) CopyW(out + 4 * k, att + 128 + 4 * k);
        SetF1Bits(ctx, load<u32>(out + 8));
        ctx.gpr[3] = att + 128;
        return;
    }
    f32 x = F(p + 416), y = F(p + 420), z = F(p + 424);
    const u32 q = load<u32>(o + 388);
    bool own = true;
    if (q != 0) own = load<u16>(q + 8) == 0;
    V4 r;
    if (own) {
        r = XformPoint(Lvx(o + 96), Lvx(o + 112), Lvx(o + 128), Lvx(o + 144), W3(p + 416));
        ctx.gpr[3] = o + 144;
    } else {
        const u32 blk = load<u32>(load<u32>(q) + 28);
        x = Adds(x, F(blk + 368));
        y = Adds(y, F(blk + 372));
        z = Adds(z, F(blk + 376));
        // Rows copied word by word to the stack (lanes 0..2; lane 3 unused).
        r = XformPoint(W3(blk + 240), W3(blk + 256), W3(blk + 272), W3(blk + 288), V4{{x, y, z, 0.0f}});
        ctx.gpr[3] = 112;
    }
    St3(out, r);
    SetF1Bits(ctx, LaneW(r, 2));
}

namespace {
constexpr u32 kPerm1D0640 = 0x001D0620;  // two vperm masks (the transpose)
}  // namespace

// void f(-, Obj* o, const f32 m[4][4]) (called by Obj_ApplyPose_001D0E58):
// with o, o's local matrix (+80 rows, 4x3 words) = m and flag 0x800 of
// +164 cleared; o's world rows (+16..+72, 4x3 words) = m, taken into the
// parent's frame first (the rigid inverse of the parent o->+260 rows +80)
// when there is one; then WorldObj_MarkDirty(o). f1 / r3 are left as the
// original leaves them.
DECOMP(001D0640, Obj_SetLocalMatrix_001D0640) {
    const u32 o = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    ctx.gpr[3] = o;
    if (o == 0) return;
    u32 w[12];
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) w[3 * r + k] = load<u32>(m + 16 * r + 4 * k);
    for (u32 r = 0; r < 2; ++r)
        for (u32 k = 0; k < 3; ++k) store<u32>(o + 80 + 16 * r + 4 * k, w[3 * r + k]);
    const u32 fl = load<u32>(o + 164) & ~0x800u;
    for (u32 r = 2; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) store<u32>(o + 80 + 16 * r + 4 * k, w[3 * r + k]);
    store<u32>(o + 164, fl);
    const u32 par = load<u32>(o + 260);
    if (par != 0) {
        const InvRows4 inv = InvRigid4(par + 80, Lvx(kPerm1D0640), Lvx(kPerm1D0640 + 16));
        V4 q[4];
        for (u32 r = 0; r < 4; ++r) {
            const V4 v{{std::bit_cast<f32>(w[3 * r]), std::bit_cast<f32>(w[3 * r + 1]), std::bit_cast<f32>(w[3 * r + 2]), 0.0f}};
            q[r] = XformDir(inv.c[0], inv.c[1], inv.c[2], v);
        }
        q[3] = Add4(inv.t, q[3]);
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) w[3 * r + k] = LaneW(q[r], k);
    }
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) store<u32>(o + 16 + 16 * r + 4 * k, w[3 * r + k]);
    SetF1Bits(ctx, w[0]);
    call(ctx, v4fn::WorldObj_MarkDirty, {o});
}

// void Slot_BuildMatrix(f32 out[4][4], Obj* o, s32 bone, Mat* m, const f32
// q[4], const f32 t[3]) (called by 000CEDD8, 000DB348, 002B8A80,
// Slot_FreePos_002B92F8): out = (q's rotation with translation t) through
// the frame F, F = o's bone matrix (Obj_BoneMatrix(.., o, bone, 1, 1)) when
// bone != -1, else m's rows +80 when m, else o's rows +96 (out first gets
// o's rows +96, then the identity at 0x01164BD0, as the original writes it;
// F's rows are copied 3 words each into the stack, so the fourth lanes of
// the vector product read whatever the stack held there, as the original
// does). f1 / r3 are left as the original leaves them.
DECOMP(000CEA48, Slot_BuildMatrix_000CEA48) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), q = arg<u32>(ctx, 4), t = arg<u32>(ctx, 5);
    const s32 bone = static_cast<s32>(arg<u32>(ctx, 2));
    const u32 m = arg<u32>(ctx, 3);
    auto copy43 = [](u32 d, u32 s) {
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) CopyW(d + 16 * r + 4 * k, s + 16 * r + 4 * k);
    };
    Stack frame(ctx, 352);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 rot = sp + 112, fr = sp + 176, bm = sp + 240;
    copy43(out, o + 96);
    call(ctx, v4fn::Quat_ToMatrix, {rot, q, kV3Zero3});
    for (u32 k = 0; k < 3; ++k) CopyW(rot + 48 + 4 * k, t + 4 * k);
    copy43(fr, o + 96);
    if (bone != -1) {
        call(ctx, v4fn::Obj_BoneMatrix, {bm, o, static_cast<u64>(static_cast<s64>(bone)), 1, 1});
        copy43(fr, bm);
    } else if (m != 0) {
        copy43(fr, m + 80);
    }
    copy43(out, 0x01164BD0u);
    const V4 f0 = Lvx(fr), f1 = Lvx(fr + 16), f2 = Lvx(fr + 32), f3 = Lvx(fr + 48);
    const V4 r0 = Lvx(rot), r1 = Lvx(rot + 16), r2 = Lvx(rot + 32), tr = Lvx(rot + 48);
    Stvx(out, XformDir(f0, f1, f2, r0));
    Stvx(out + 16, XformDir(f0, f1, f2, r1));
    Stvx(out + 32, XformDir(f0, f1, f2, r2));
    Stvx(out + 48, XformPoint(f0, f1, f2, f3, tr));
    SetF1Bits(ctx, load<u32>(0x01164BD0u + 56));
    ctx.gpr[3] = out + 16;
}

namespace {
// The 4x3 words of the rows at s copied to the rows at d (lfs / stfs).
inline void Copy43w(u32 d, u32 s) {
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW(d + 16 * r + 4 * k, s + 16 * r + 4 * k);
}
}  // namespace

// void f(T* o) (called by 001CB000): refreshes o's two cached matrices,
// A (+560 rows) and B (+624 rows), 4x3 words each. With o->+836 or
// o->+840: B = the +836 object's rows +80 or the body matrix
// (Field_Get160Body), then A = the +840 object's rows +80 or the body
// matrix. Else A = the body matrix taken through the +848 object's matrix
// (+8 ->: copied in, then each row through the body rows, stvx all four
// lanes), or the +832 object's rows +80, or the body matrix. f1 / r3 are
// left as the original leaves them.
DECOMP(001CAAEC, Obj_RefreshMatrices_001CAAEC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a836 = load<u32>(o + 836);
    auto body = [&]() { return static_cast<u32>(call(ctx, v4fn::Field_Get160Body, {o})); };
    u32 src;
    if (a836 != 0 || load<u32>(o + 840) != 0) {
        src = a836 != 0 ? a836 + 80 : body();
        Copy43w(o + 624, src);
        const u32 a840 = load<u32>(o + 840);
        src = a840 != 0 ? a840 + 80 : body();
        Copy43w(o + 560, src);
        SetF1Bits(ctx, load<u32>(src + 56));
        ctx.gpr[3] = src;
        return;
    }
    const u32 a848 = load<u32>(o + 848);
    if (a848 != 0) {
        const u32 m = load<u32>(a848 + 8);
        Copy43w(o + 560, m);
        SetF1Bits(ctx, load<u32>(m + 56));
        const u32 g = body();
        const V4 g0 = Lvx(g), g1 = Lvx(g + 16), g2 = Lvx(g + 32), g3 = Lvx(g + 48);
        const V4 a0 = Lvx(o + 560), a1 = Lvx(o + 576), a2 = Lvx(o + 592), a3 = Lvx(o + 608);
        const V4 n3 = XformPoint(g0, g1, g2, g3, a3);
        Stvx(o + 560, XformDir(g0, g1, g2, a0));
        Stvx(o + 576, XformDir(g0, g1, g2, a1));
        Stvx(o + 592, XformDir(g0, g1, g2, a2));
        Stvx(o + 608, n3);
        ctx.gpr[3] = g + 48;
        return;
    }
    const u32 a832 = load<u32>(o + 832);
    src = a832 != 0 ? a832 + 80 : body();
    Copy43w(o + 560, src);
    SetF1Bits(ctx, load<u32>(src + 56));
    ctx.gpr[3] = src;
}

namespace {
constexpr u32 kKF6610 = 0x000F6610;  // f32 1/65535, then (+16) a vperm mask {a0 a1 b0 a3}
// The u64 random value as f32 times 1/65535 (fcfid, frsp, fmuls).
inline f32 RandU(u64 v, f32 k) { return Muls(static_cast<f32>(static_cast<f64>(static_cast<s64>(v))), k); }
// lo + (hi - lo) * u style spread: base + (2a * u - a) (fsubs, fmadds, fadds).
inline f32 Spread(f32 base, f32 a, f32 u) { return Adds(base, Fmadds(Subs(a, -a), u, -a)); }
}  // namespace

// void f(f32 out[4], const Params* p, const f32 d[3]) (called through a
// pointer): a random emission vector. Three rotations built with
// Matrix_FromAxisAngle (axes (-d.z, 0, d.x), 0x01164B60 and d; angles p->
// +128.. + a random spread of +/- p->+144..), each completed with the zero
// translation (0x01164B40; copied 3 words / built with the vperm mask, so
// the fourth lanes read what the stack held, as the original does), are
// applied in turn to d (vector multiply-adds, stvx into out each time),
// then out.xyz is scaled by a random value in [p->+160, p->+164). The random
// state (0x01164888) advances four steps. f1 / r3 are left as the original
// leaves them.
DECOMP(000F6630, Particle_RandomDir_000F6630) {
    const u32 out = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    Stack frame(ctx, 432);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 ax = sp + 112, m1 = sp + 128, m2 = sp + 192, m3 = sp + 256;
    const f32 K = F(kKF6610);
    store<f32>(ax + 8, F(d));
    store<f32>(ax, -F(d + 8));
    store<u32>(ax + 4, 0);
    const u32 zero = kV3Zero3, yaxis = 0x01164B60u;
    {
        const f32 u = RandU(RandStep28(), K);
        callf(ctx, v4fn::Matrix_FromAxisAngle, {m1, ax}, {Spread(F(p + 128), F(p + 144), u)});
    }
    for (u32 k = 0; k < 3; ++k) CopyW(m1 + 48 + 4 * k, zero + 4 * k);
    {
        const f32 u = RandU(RandStep28(), K);
        callf(ctx, v4fn::Matrix_FromAxisAngle, {m2, yaxis}, {Spread(F(p + 132), F(p + 148), u)});
    }
    for (u32 k = 0; k < 3; ++k) CopyW(m2 + 48 + 4 * k, zero + 4 * k);
    {
        const f32 u = RandU(RandStep28(), K);
        callf(ctx, v4fn::Matrix_FromAxisAngle, {m3, d}, {Spread(F(p + 136), F(p + 152), u)});
    }
    CopyW(m3 + 48, zero);
    CopyW(m3 + 52, zero + 4);
    const V4 t3 = Perm(Lvx(m3 + 48), Lvlx(zero + 8), Lvx(kKF6610 + 16));
    for (u32 k = 0; k < 3; ++k) CopyW(out + 4 * k, d + 4 * k);
    Stvx(out, XformPoint(Lvx(m1), Lvx(m1 + 16), Lvx(m1 + 32), Lvx(m1 + 48), W3(d)));
    Stvx(out, XformPoint(Lvx(m2), Lvx(m2 + 16), Lvx(m2 + 32), Lvx(m2 + 48), W3(out)));
    Stvx(out, XformPoint(Lvx(m3), Lvx(m3 + 16), Lvx(m3 + 32), t3, W3(out)));
    const f32 lo = F(p + 160), span = Subs(F(p + 164), lo);
    const f32 s = Fmadds(span, RandU(RandStep28(), K), lo);
    const f32 x = Muls(F(out), s), y = Muls(F(out + 4), s), z = Muls(F(out + 8), s);
    store<f32>(out, x);
    store<f32>(out + 4, y);
    store<f32>(out + 8, z);
    ctx.fpr[1] = z;
    ctx.gpr[3] = load<u32>(0x01164890);
}

namespace {
constexpr u32 kK2A6080 = 0x002A6080;  // f32 1.0, 0.0; f64 1.0; f32 -1.0; f64 -1.0
}  // namespace

// void f(-, f32 m[4][4], f32* out, const T* a, const f32 r[3][4], bool
// follow) (called by 002A6498): re-aims the matrix m. The angle between
// a's +32 axis and m's row 2 (dot product clamped to [-1, 1] by the fsel
// chain, acosf; negated when -(a's +0 vector . m's row 2) > 0) gives the
// rotation Matrix_FromAxisAngle(rot, 0x01164B70, angle) that becomes m's
// rows 0..2 (3 words each). m's position: with follow, d = (m's position -
// the player's (world +0x1280F0 ->+144)) in xz taken through r (vector
// multiply-adds, the y lane times 0), normalised in xz when |d| exceeds the
// threshold 0x0105A1C8, and pos = base - U * d.x + V * d.z (base / U / V
// the vectors at 0x0105A0B0 / 0x0105A110 / 0x0105A170), *out = the height
// blend (|dy| clamped to [lo, hi] = [0x0105B638, 0x0105B688], mapped onto
// [0x0105B6D8, 0x0105B728]); else pos = base. f1 / r3 are left as the
// original leaves them.
DECOMP(002A60A0, Cam_ReAim_002A60A0) {
    const u32 m = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2), a = arg<u32>(ctx, 3), r = arg<u32>(ctx, 4);
    const u32 follow = arg<u32>(ctx, 5);
    const f32 one = F(kK2A6080), zero = F(kK2A6080 + 4), m1 = F(kK2A6080 + 16);
    const f64 oneD = std::bit_cast<f64>(load<u64>(kK2A6080 + 8)), m1D = std::bit_cast<f64>(load<u64>(kK2A6080 + 24));
    const f32 bx = F(m + 32), by = F(m + 36), bz = F(m + 40);
    const f32 dot = Fmadds(F(a + 40), bz, Fmadds(F(a + 32), bx, Muls(F(a + 36), by)));
    f32 c = Fsels(Subs(one, dot), dot, oneD);
    c = Fsels(Subs(c, m1), c, m1D);
    c = Fsels(Adds(c, one), c, m1D);
    c = Fsels(Subs(c, one), oneD, c);
    const f32 side = Fmadds(-F(a + 8), bz, Fmadds(-F(a), bx, Muls(-F(a + 4), by)));
    const f32 tx = F(m + 48), ty = F(m + 52), tz = F(m + 56);
    f64 ang = callf(ctx, v4fn::acosf, {}, {c});
    if (side > zero) ang = -ang;
    Stack frame(ctx, 304);
    const u32 rot = static_cast<u32>(ctx.gpr[1]) + 128;
    callf(ctx, v4fn::Matrix_FromAxisAngle, {rot, 0x01164B70u}, {ang});
    constexpr u32 kBase = 0x0105A0B0u, kU = 0x0105A110u, kV = 0x0105A170u;
    f32 px, py, pz;
    if (follow == 0) {
        px = F(kBase);
        py = F(kBase + 4);
        pz = F(kBase + 8);
        ctx.gpr[3] = 0x0105A060u;
    } else {
        const u32 P = load<u32>(load<u32>(kV3WorldPtr) + 0x001280F0u) + 144;
        const f32 dx = Subs(tx, F(P)), dz = Subs(tz, F(P + 8));
        const f32 d = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(zero, zero))));
        const f32 thr = F(0x0105A1C8u);
        const V4 loc = Madd4(Lvx(r + 32), Fill4(dz), Madd4(Lvx(r + 16), Zero4(), Madd4(Lvx(r), Fill4(dx), Zero4())));
        const f32 lx = loc.e[0], lz = loc.e[2];
        const f32 len = d > thr ? Len3s(lx, loc.e[1], lz) : thr;
        const f32 inv = Divs(one, len);
        const f32 nx = Muls(lx, inv), nz = Muls(lz, inv);
        px = Fmadds(F(kV), nz, Fnmsubs(F(kU), nx, F(kBase)));
        py = Fmadds(F(kV + 4), nz, Fnmsubs(F(kU + 4), nx, F(kBase + 4)));
        pz = Fmadds(F(kV + 8), nz, Fnmsubs(F(kU + 8), nx, F(kBase + 8)));
        const f32 hi = F(0x0105B688u), lo = F(0x0105B638u);
        const f32 span = Subs(hi, lo);
        f32 h = static_cast<f32>(std::fabs(static_cast<f64>(Subs(ty, F(P + 4)))));
        h = Fsels(Subs(hi, h), h, hi);
        h = Fsels(Subs(h, lo), h, lo);
        const f32 b0 = F(0x0105B6D8u);
        store<f32>(out, Fmadds(Divs(Subs(hi, h), span), Subs(F(0x0105B728u), b0), b0));
        ctx.gpr[3] = P;
    }
    for (u32 k = 0; k < 3; ++k) CopyW(m + 4 * k, rot + 4 * k);
    for (u32 k = 0; k < 3; ++k) CopyW(m + 16 + 4 * k, rot + 16 + 4 * k);
    CopyW(m + 32, rot + 32);
    CopyW(m + 36, rot + 36);
    store<f32>(m + 48, px);
    CopyW(m + 40, rot + 40);
    store<f32>(m + 52, py);
    store<f32>(m + 56, pz);
    ctx.fpr[1] = py;
}

namespace {
constexpr u32 kPerm1B3400 = 0x001B3400;  // two vperm masks (the transpose)
// The 4x4 rows at d replaced by themselves through inv (stvx row 0, 1, 2, 3).
inline void ApplyInv4(u32 d, const InvRows4& inv) {
    const V4 q0 = Lvx(d), q1 = Lvx(d + 16), q2 = Lvx(d + 32), q3 = Lvx(d + 48);
    const V4 n3 = Add4(inv.t, XformDir(inv.c[0], inv.c[1], inv.c[2], q3));
    Stvx(d, XformDir(inv.c[0], inv.c[1], inv.c[2], q0));
    Stvx(d + 16, XformDir(inv.c[0], inv.c[1], inv.c[2], q1));
    Stvx(d + 32, XformDir(inv.c[0], inv.c[1], inv.c[2], q2));
    Stvx(d + 48, n3);
}
}  // namespace

// void f(T* o) (called by ThunkArg1_001B37C0): with the attachment a =
// o->+536, a's local matrix (+8 ->) = o's 4x3 rows +432 taken into the
// frame of o's world object (the rigid inverse of o->+160 rows +80), copied
// (4x3 words) to a's second matrix (+4 ->), which is further taken into the
// frame of o->+532 's matrix (+8 ->); then the world object's skeleton
// (vt[7], twice: the first result's byte +31 gets 0x80, the second goes to
// Skeleton_Update), WorldObj_MarkDirty and WorldObj_UpdateLod(.., 0, 0).
// f1 / r3 are left as the original leaves them.
DECOMP(001B3420, Obj_SyncAttachedPose_001B3420) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 536);
    if (a == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 l = load<u32>(a + 8);
    Copy43w(l, o + 432);
    const V4 k0 = Lvx(kPerm1B3400), k1 = Lvx(kPerm1B3400 + 16);
    ApplyInv4(l, InvRigid4(load<u32>(o + 160) + 80, k0, k1));
    const u32 a2 = load<u32>(o + 536);
    const u32 d = load<u32>(a2 + 4);
    Copy43w(d, l);
    SetF1Bits(ctx, load<u32>(l + 56));
    if (a2 != 0) ApplyInv4(d, InvRigid4(load<u32>(load<u32>(o + 532) + 8), k0, k1));
    const u32 w = load<u32>(o + 160);
    const u32 sk = static_cast<u32>(VCall(ctx, w, 28, {w}));
    store<u8>(sk + 31, static_cast<u8>(load<u8>(sk + 31) | 0x80u));
    const u32 w2 = load<u32>(o + 160);
    const u32 sk2 = static_cast<u32>(VCall(ctx, w2, 28, {w2}));
    call(ctx, v4fn::Skeleton_Update, {sk2});
    call(ctx, v4fn::WorldObj_MarkDirty, {load<u32>(o + 160)});
    call(ctx, v4fn::WorldObj_UpdateLod, {load<u32>(o + 160), 0, 0});
}

namespace {
constexpr u32 kK381EB0 = 0x00381EB0;  // f32 1.0, 0.0; f64 1.0; f32 -1.0; f64 -1.0 (as kK2A6080)
}  // namespace

// void f(T* o, f32 m[4][4], f32* out, const T* a, const f32 r[3][4], bool
// follow) (called by 00382308): the sibling of Cam_ReAim_002A60A0 for o's
// body. The aim rotation rot = Matrix_FromAxisAngle(.., 0x01164B70, angle)
// (angle as there; rot's row 3 the zero vector) right-multiplies a stack
// copy of o's body rows (o->+208 +80, 4x3 words); with follow, the copy's
// position first becomes body - (h * d.x, -h * d.z, 0) (h = o->+8 ->+192,
// d the player offset through r as there, normalised in xz beyond the
// threshold o->+8 ->+196; the 0.0 pool word multiplies the other terms)
// and *out = the height blend ([0x0106CB90, 0x0106CBE0] onto [0x0106CC30,
// 0x0106CC80]). m's 4x3 words = Matrix_Multiply(copy, rot). f1 / r3 are
// left as the original leaves them.
DECOMP(00381ED0, Body_ReAim_00381ED0) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2), a = arg<u32>(ctx, 3), r = arg<u32>(ctx, 4);
    const u32 follow = arg<u32>(ctx, 5);
    const f32 one = F(kK381EB0), zero = F(kK381EB0 + 4), m1 = F(kK381EB0 + 16);
    const f64 oneD = std::bit_cast<f64>(load<u64>(kK381EB0 + 8)), m1D = std::bit_cast<f64>(load<u64>(kK381EB0 + 24));
    const f32 bx = F(m + 32), by = F(m + 36), bz = F(m + 40);
    const f32 dot = Fmadds(F(a + 40), bz, Fmadds(F(a + 32), bx, Muls(F(a + 36), by)));
    f32 c = Fsels(Subs(one, dot), dot, oneD);
    c = Fsels(Subs(c, m1), c, m1D);
    c = Fsels(Adds(c, one), c, m1D);
    c = Fsels(Subs(c, one), oneD, c);
    const f32 side = Fmadds(-F(a + 8), bz, Fmadds(-F(a), bx, Muls(-F(a + 4), by)));
    const u32 prm = load<u32>(o + 8);
    const f32 tx = F(m + 48), ty = F(m + 52), tz = F(m + 56);
    f64 ang = callf(ctx, v4fn::acosf, {}, {c});
    if (side > zero) ang = -ang;
    Stack frame(ctx, 384);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 rot = sp + 128, cp = sp + 192;
    callf(ctx, v4fn::Matrix_FromAxisAngle, {rot, 0x01164B70u}, {ang});
    const u32 body = load<u32>(o + 208) + 80;
    for (u32 k = 0; k < 3; ++k) CopyW(rot + 48 + 4 * k, kV3Zero3 + 4 * k);
    Copy43w(cp, body);
    if (follow != 0) {
        const u32 w = load<u32>(kV3WorldPtr);
        const u32 P = load<u32>(w + 0x001280F0u) + 144;
        const f32 dx = Subs(tx, F(P)), dz = Subs(tz, F(P + 8));
        const f32 d = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(zero, zero))));
        const V4 loc = Madd4(Lvx(r + 32), Fill4(dz), Madd4(Lvx(r + 16), Zero4(), Madd4(Lvx(r), Fill4(dx), Zero4())));
        const f32 thr = F(prm + 196);
        const f32 lx = loc.e[0], lz = loc.e[2];
        const f32 len = d > thr ? Len3s(lx, loc.e[1], lz) : thr;
        const f32 inv = Divs(one, len);
        const f32 h = F(prm + 192);
        const f32 nz = Muls(lz, inv), nx = Muls(lx, inv);
        const f32 px = Fnmsubs(h, nx, Fmadds(zero, nz, F(body + 48)));
        const f32 py = Fnmsubs(zero, nx, Fmadds(h, nz, F(body + 52)));
        const f32 pz = Fnmsubs(zero, nx, Fmadds(zero, nz, F(body + 56)));
        store<f32>(cp + 48, px);
        store<f32>(cp + 52, py);
        store<f32>(cp + 56, pz);
        const f32 hi = F(0x0106CBE0u), lo = F(0x0106CB90u);
        const f32 span = Subs(hi, lo);
        f32 hh = static_cast<f32>(std::fabs(static_cast<f64>(Subs(ty, F(P + 4)))));
        hh = Fsels(Subs(hi, hh), hh, hi);
        hh = Fsels(Subs(hh, lo), hh, lo);
        const f32 b0 = F(0x0106CC30u);
        store<f32>(out, Fmadds(Divs(Subs(hi, hh), span), Subs(F(0x0106CC80u), b0), b0));
    }
    call(ctx, v4fn::Matrix_Multiply, {cp, rot});
    Copy43w(m, cp);
    SetF1Bits(ctx, load<u32>(cp + 56));
}

namespace {
constexpr u32 kK106740 = 0x00106740;  // f32 {1e-5, 1.0, pi, 0.0}, f64 {pi, 0.0}, then (+32) a vperm mask {a0 a1 b0 a3}
}  // namespace

// void f(f32 out[3], const f32 a[3], const f32 b[3]; f32 max) (called by
// 000EB978, 001978F0, 002C5140): turns the unit direction of a toward b's
// by at most max (clamped to [0, pi]) about the vertical, keeping b's
// height. Degenerate inputs (|.|^2 < 1e-5): a and b -> the vector at
// 0x01164B70; a -> b normalised; b -> a normalised; a's or b's horizontal
// part -> b normalised. With |max| < 1e-5: a's horizontal direction scaled
// to b's horizontal length, with b's y. When cos(max) <= ha . hb or max >=
// pi: b normalised. Else that vector rotated by max about the axis ha x hb
// (Matrix_FromAxisAngle; translation the zero vector, built with vperm).
// f1 / r3 are left as the original leaves them.
DECOMP(00106770, Dir_TurnToward_00106770) {
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const f64 maxA = farg(ctx, 0);
    const f32 eps = F(kK106740), one = F(kK106740 + 4), pi = F(kK106740 + 8), z0 = F(kK106740 + 12);
    const f64 piD = std::bit_cast<f64>(load<u64>(kK106740 + 16)), zD = std::bit_cast<f64>(load<u64>(kK106740 + 24));
    auto tiny = [&](f32 v) { return std::fabs(static_cast<f64>(v)) < static_cast<f64>(eps); };
    auto put = [&](f32 x, f32 y, f32 z) {
        store<f32>(out, x);
        store<f32>(out + 4, y);
        store<f32>(out + 8, z);
    };
    const f32 ax = F(a), ay = F(a + 4), az = F(a + 8);
    const f32 la2 = Fmadds(az, az, Fmadds(ax, ax, Muls(ay, ay)));
    const f32 bx = F(b), by = F(b + 4), bz = F(b + 8);
    const f32 lb2 = Fmadds(bz, bz, Fmadds(bx, bx, Muls(by, by)));
    if (tiny(la2)) {
        if (tiny(lb2)) {
            for (u32 k = 0; k < 3; ++k) CopyW(out + 4 * k, 0x01164B70u + 4 * k);
            SetF1Bits(ctx, load<u32>(0x01164B78u));
            ctx.gpr[3] = 0x01160000u;
            return;
        }
        const f32 s = Divs(one, Sqrts(lb2));
        const f32 z = Muls(s, bz);
        put(Muls(s, bx), Muls(s, by), z);
        ctx.fpr[1] = z;
        ctx.gpr[3] = kK106740;
        return;
    }
    ctx.gpr[3] = kK106740;
    const f32 ia = Divs(one, Sqrts(la2));
    const f32 anx = Muls(ia, ax), any = Muls(ia, ay), anz = Muls(ia, az);
    ctx.fpr[1] = eps;
    if (tiny(lb2)) {
        put(anx, any, anz);
        return;
    }
    const f32 c1 = Fsels(static_cast<f64>(Subs(pi, maxA)), maxA, piD);
    const f32 ang = Fsels(c1, c1, zD);
    const f32 ib = Divs(one, Sqrts(lb2));
    const f32 bnx = Muls(bx, ib), bny = Muls(by, ib), bnz = Muls(bz, ib);
    const f32 k2 = Muls(z0, z0);
    const f32 ha2 = Fmadds(anz, anz, Fmadds(anx, anx, k2));
    if (tiny(ha2)) {
        put(bnx, bny, bnz);
        return;
    }
    const f32 iha = Divs(one, Sqrts(ha2));
    const f32 hax = Muls(anx, iha), hay = Muls(z0, iha), haz = Muls(anz, iha);
    const f32 hb2 = Fmadds(bnz, bnz, Fmadds(bnx, bnx, k2));
    if (tiny(hb2)) {
        put(bnx, bny, bnz);
        return;
    }
    const f32 ihb = Divs(one, Sqrts(hb2));
    const f32 hbx = Muls(bnx, ihb), hby = Muls(z0, ihb), hbz = Muls(bnz, ihb);
    if (tiny(ang)) {
        const f32 hl = Sqrts(Fmadds(bnx, bnx, Muls(bnz, bnz)));
        store<f32>(out + 4, bny);
        const f32 z = Muls(haz, hl);
        store<f32>(out, Muls(hax, hl));
        store<f32>(out + 8, z);
        ctx.fpr[1] = z;
        return;
    }
    const f64 c = callf(ctx, v4fn::Math_Cosf, {}, {ang});
    const f32 d = Fmadds(haz, hbz, Fmadds(hax, hbx, Muls(hay, hby)));
    if (c <= static_cast<f64>(d) || ang >= pi) {
        put(bnx, bny, bnz);
        return;
    }
    const f32 cx = Fmsubs(hay, hbz, Muls(haz, hby));
    const f32 cy = Fmsubs(haz, hbx, Muls(hax, hbz));
    const f32 cz = Fmsubs(hax, hby, Muls(hay, hbx));
    const f32 il = Divs(one, Sqrts(Fmadds(cz, cz, Fmadds(cx, cx, Muls(cy, cy)))));
    Stack frame(ctx, 336);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 axis = sp + 112, rot = sp + 144;
    store<f32>(axis, Muls(cx, il));
    store<f32>(axis + 4, Muls(cy, il));
    store<f32>(axis + 8, Muls(cz, il));
    callf(ctx, v4fn::Matrix_FromAxisAngle, {rot, axis}, {ang});
    const f32 hl = Sqrts(Fmadds(bnx, bnx, Muls(bnz, bnz)));
    CopyW(rot + 48, kV3Zero3);
    CopyW(rot + 52, kV3Zero3 + 4);
    const V4 t3 = Perm(Lvx(rot + 48), Lvlx(kV3Zero3 + 8), Lvx(kK106740 + 32));
    const V4 v{{Muls(hax, hl), bny, Muls(haz, hl), 0.0f}};
    const V4 r = Add4(t3, XformDir(Lvx(rot), Lvx(rot + 16), Lvx(rot + 32), v));
    St3(out, r);
    SetF1Bits(ctx, LaneW(r, 2));
    ctx.gpr[3] = 144;
}
