// Game code, medium sweep part 16: parameter blocks and frames.
#include "mid16.h"

using namespace sly4;
using namespace sly4::mid16;

// void Params_Overlay0FE3E8(Owner* s, Params* out)
// (called by 00104F80, 00105F90) Copies the base parameter block at
// s+736 into out (128 bytes: f32 +0..+8, +16..+24, +32..+92, u32 +96,
// f32 +100..+116, u8 +120, u32 +124 = the base's mask word; +12 and +28
// are left alone), then applies each non-null overlay in the list at
// s+540 (count u32 s+536, re-read every pass): overlay bit k of its mask
// word +124 replaces one field - bits 0..2 +0/+4/+8, bit 3 the vector
// +16..+24, bits 4..25 +32 + 4 * (k - 4) (bit 20 = the u32 +96). The
// compiled code re-reads the mask after every copy; reading it before
// every test is the same. f1 is left holding the last f32 loaded.
DECOMP(000FE3E8, Params_Overlay0FE3E8) {
    const u32 s = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    f64 f1 = ctx.fpr[1];
    auto cf = [&](u32 d, u32 a) { const u32 w = load<u32>(a); store<u32>(d, w); f1 = LfsF(w); };
    for (u32 k = 0; k < 3; ++k) cf(o + 4 * k, s + 736 + 4 * k);
    for (u32 k = 0; k < 3; ++k) cf(o + 16 + 4 * k, s + 752 + 4 * k);
    for (u32 k = 0; k < 16; ++k) cf(o + 32 + 4 * k, s + 768 + 4 * k);
    Cw(o + 96, s + 832);
    for (u32 k = 0; k < 5; ++k) cf(o + 100 + 4 * k, s + 836 + 4 * k);
    store<u8>(o + 120, load<u8>(s + 856));
    Cw(o + 124, s + 860);
    if (load<u32>(s + 536) != 0) {
        u32 i = 0;
        do {
            const u32 e = load<u32>(s + 540 + 4 * i);
            if (e != 0) {
                auto m = [&](u32 bit) { return (load<u32>(e + 124) & bit) != 0; };
                for (u32 k = 0; k < 3; ++k)
                    if (m(1u << k)) cf(o + 4 * k, e + 4 * k);
                if (m(0x8u))
                    for (u32 k = 0; k < 3; ++k) cf(o + 16 + 4 * k, e + 16 + 4 * k);
                for (u32 k = 4; k <= 25; ++k) {
                    if (!m(1u << k)) continue;
                    const u32 off = 32 + 4 * (k - 4);
                    if (k == 20) Cw(o + off, e + off);  // the u32 +96
                    else cf(o + off, e + off);
                }
            }
            ++i;
        } while (i < load<u32>(s + 536));
    }
    ctx.fpr[1] = f1;
    ret(ctx, s);
}

// void Frame_FromPoints387B00(Mat34* m, Obj* o, f32 t[2])
// (called by 00388070) Builds m from the three points A/B/C = o->316/320/
// 324 + 128 (o->328 + 128 is read into the frame and never used): m starts
// as a copy of the matrix at 0x01164BD0 (rows 0..3, x/y/z only); t[0] =
// |t[0] - o->572| / |o->580 - o->572|, t[1] = |t[1] - o->576| /
// |o->584 - o->576|; row 0 = normalize(B - A), row 1 = normalize(A - C)
// (with 1 / length, the constant 0x00387AFC), row 2 = row0 x row1, row 1 =
// row2 x row0, then Matrix_NormalizeInPlace(m); row 3 = {A.x + (B.x - A.x)
// * t[0], A.y + (C.y - A.y) * t[1], A.z + (B.z - A.z) * t[0]}. Leaves r3 =
// sp + 136 (the frame copy of B.z) and f1 = m->56.
DECOMP(00387B00, Frame_FromPoints387B00) {
    const u32 m = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), t = arg<u32>(ctx, 2);
    const u32 A = load<u32>(o + 316) + 128, B = load<u32>(o + 320) + 128, C = load<u32>(o + 324) + 128;
    load<u32>(o + 328);  // D = o->328 + 128: copied to the frame, unused
    const f64 ax = Lf(A), ay = Lf(A + 4), az = Lf(A + 8);
    const f64 bx = Lf(B), by = Lf(B + 4), bz = Lf(B + 8);
    const f64 cx = Lf(C), cy = Lf(C + 4), cz = Lf(C + 8);
    constexpr u32 I = 0x01164BD0u;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) Cw(m + 16 * r + 4 * k, I + 16 * r + 4 * k);
    {
        const f64 d0 = std::fabs(S(Lf(t) - Lf(o + 572)));
        const f64 d1 = std::fabs(S(Lf(t + 4) - Lf(o + 576)));
        Sf(t, d0);
        Sf(t + 4, d1);
        Sf(t, Divs(Lf(t), std::fabs(S(Lf(o + 580) - Lf(o + 572)))));
        Sf(t + 4, Divs(Lf(t + 4), std::fabs(S(Lf(o + 584) - Lf(o + 576)))));
    }
    f64 yx = S(ax - cx), yy = S(ay - cy), yz = S(az - cz);
    const f64 dx = S(bx - ax), dy = S(by - ay), dz = S(bz - az);
    const f64 yl = Sqrts(Fmadds(yz, yz, Fmadds(yx, yx, S(yy * yy))));
    const f64 one = Lf(0x00387AFCu);
    const f64 yi = Divs(one, yl);
    Sf(m + 16, S(yx * yi));
    Sf(m + 20, S(yy * yi));
    Sf(m + 24, S(yz * yi));
    Sf(m + 4, dy);
    Sf(m + 0, dx);
    Sf(m + 8, dz);
    {
        const f64 x1 = Lf(m + 4), x0 = Lf(m), x2 = Lf(m + 8);
        const f64 xi = Divs(one, Sqrts(Fmadds(x2, x2, Fmadds(x0, x0, S(x1 * x1)))));
        Sf(m + 8, S(x2 * xi));
        Sf(m + 4, S(x1 * xi));
        Sf(m + 0, S(x0 * xi));
    }
    const u32 X = m, Y = m + 16, Z = m + 32;
    Sf(Z, Fmsubs(Lf(X + 4), Lf(Y + 8), S(Lf(X + 8) * Lf(Y + 4))));
    const f64 z1 = Fmsubs(Lf(X + 8), Lf(Y), S(Lf(X) * Lf(Y + 8)));
    Sf(Z + 4, z1);
    const f64 z2 = Fmsubs(Lf(X), Lf(Y + 4), S(Lf(X + 4) * Lf(Y)));
    Sf(Z + 8, z2);
    Sf(Y, Fmsubs(z1, Lf(X + 8), S(z2 * Lf(X + 4))));
    Sf(Y + 4, Fmsubs(Lf(Z + 8), Lf(X), S(Lf(Z) * Lf(X + 8))));
    Sf(Y + 8, Fmsubs(Lf(Z), Lf(X + 4), S(Lf(Z + 4) * Lf(X))));
    Stack frame(ctx, 240);
    call(ctx, m16fn::Matrix_NormalizeInPlace, {m, Z, Y});
    Sf(m + 48, Fmadds(dx, Lf(t), ax));
    Sf(m + 52, Fmadds(S(cy - ay), Lf(t + 4), ay));
    const f64 r = Fmadds(dz, Lf(t), az);
    Sf(m + 56, r);
    ctx.fpr[1] = r;
    ret(ctx, frame.addr() + 136);
}

// void Obj_Refresh108480(Obj* o)
// (called by Obj_SetRef1088E4, 002194DC) Refreshes o's 37-word block
// +360..+504 (f32 apart from the u32 +444) from its link o->20 when that
// is set and has flag 0x8 in +192 (source link->216 + 1140, 38 words),
// else from the global table at 0x010334D0 (one word every 76 bytes);
// all loads come first. o->12 = 0xFFFFFFFF (or 0x7FFF, then the sign-
// extended -32768), o->16 |= 0x1F, o->208 bit 0x20000000 = source word 37
// bit 0. Leaves f1 = source word 32 and r4 = word 21 (the u32).
DECOMP(00108480, Obj_Refresh108480) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 l = load<u32>(o + 20);
    const u32 v12 = load<u32>(o + 12), v16 = load<u32>(o + 16);
    const bool own = l != 0 && (load<u32>(l + 192) & 0x8u) != 0;
    u32 w[38];
    if (own) {
        const u32 src = load<u32>(l + 216) + 1140;
        for (u32 k = 0; k < 38; ++k) w[k] = load<u32>(src + 4 * k);
    } else {
        for (u32 k = 0; k < 38; ++k) w[k] = load<u32>(0x010334D0u + 76 * k);
    }
    const u32 f208 = (load<u32>(o + 208) & ~0x20000000u) | ((w[37] & 1u) << 29);
    for (u32 k = 0; k < 32; ++k) store<u32>(o + 360 + 4 * k, w[k]);
    store<u32>(o + 12, v12 | 0xFFFFFFFFu);
    for (u32 k = 32; k < 37; ++k) store<u32>(o + 360 + 4 * k, w[k]);
    store<u32>(o + 16, v16 | 0x1Fu);
    store<u32>(o + 208, f208);
    ctx.fpr[1] = LfsF(w[32]);
    ctx.gpr[4] = w[21];
    ret(ctx, o);
}

namespace {

constexpr u32 kIdent = 0x01164BD0u;  // the identity Mat34 (rows at +0/+16/+32/+48)
constexpr u32 Matrix_RotationY = 0x00553E18;   // (Mat34* m, f1 angle) -> m
constexpr u32 WorldObj_MarkDirty = 0x0057D000;  // (WorldObj*)
constexpr u32 WorldObj_UpdateLod = 0x0057EE00;  // (WorldObj*, r4, r5)
constexpr u32 Cam_RotateToward1007D0 = 0x001007D0;  // (Vec3* out, const Vec3* axis, const Vec3* v, f1 angle)
constexpr u32 Cam_Fn0FD040 = 0x000FD040;           // (Vec3* io, Cam* c)

// The 3x3 + translation word k (0..11) of a Mat34: row k / 3, column k % 3.
inline u32 MOff(u32 k) { return 16 * (k / 3) + 4 * (k % 3); }

// Shared body of ThunkSpinY376598 / Obj_SpinY3739D0: t = o->tgt (or o->24
// when null), w = o->24; both set -> the frame matrix (sp+112) = the
// identity, r = Matrix_RotationY(frame, o->ang * 0.0174533 (deg -> rad,
// the f32 at kdeg)); the 12 words at r are copied into the frame, then into
// t->80.. (the world matrix: rows +80/+96/+112/+128; t->164 &= ~0x800,
// stored after row 3's y word or after its z word per `late164`) and
// t->16.. (the local one); WorldObj_MarkDirty(t), WorldObj_UpdateLod(w,
// 0, 0) (its r3 is returned). Neither set -> returns o untouched.
void SpinY(PPUContext& ctx, u32 o, u32 tgt, u32 ang, u32 kdeg, bool late164) {
    u32 t = load<u32>(o + tgt);
    const u32 w = load<u32>(o + 24);
    if (t == 0) t = w;
    if (w == 0 || t == 0) {
        ret(ctx, o);
        return;
    }
    Stack frame(ctx, 192);
    const u32 fm = frame.addr() + 112;
    const f64 a = Lf(o + ang);
    for (u32 k = 0; k < 12; ++k) Cw(fm + MOff(k), kIdent + MOff(k));
    const u32 r = static_cast<u32>(callf(ctx, Matrix_RotationY, {fm}, {S(a * Lf(kdeg))}));
    for (u32 k = 0; k < 12; ++k) Cw(fm + MOff(k), r + MOff(k));
    const u32 f164 = load<u32>(t + 164) & ~0x800u;
    for (u32 k = 0; k < 12; ++k) {
        if (k == 11 && !late164) store<u32>(t + 164, f164);
        Cw(t + 80 + MOff(k), fm + MOff(k));
    }
    if (late164) store<u32>(t + 164, f164);
    for (u32 k = 0; k < 12; ++k) Cw(t + 16 + MOff(k), fm + MOff(k));
    ctx.fpr[1] = Lf(fm + MOff(11));  // left by the copies
    call(ctx, WorldObj_MarkDirty, {t});
    call(ctx, WorldObj_UpdateLod, {w, 0, 0});
}

// The pitch window of a camera: the angles c->652 / c->656 scaled by the
// f32 at K + mul (deg -> rad) are ordered (fsel: lo = a when b - a >= 0)
// and each clamped to [-limit, limit] (the f32 at K + lim, the doubles
// K + 16 / K + 32 and the f32 K + 24 = -limit), keeping the compiled
// fsel / frsp order so NaNs and signed zeros land the same way.
struct Pitch {
    f64 lo, hi;
};
inline Pitch PitchWindow(u32 c, u32 K, u32 mul, u32 lim) {
    using mid15::Fsel15;
    const f64 m = Lf(K + mul);
    const f64 a = S(Lf(c + 652) * m), b = S(Lf(c + 656) * m);
    f64 lo = Fsel15(S(b - a), a, b), hi = Fsel15(S(a - b), a, b);
    const f64 l = Lf(K + lim);
    lo = S(Fsel15(S(l - lo), lo, Ld(K + 16)));
    hi = S(Fsel15(S(l - hi), hi, Ld(K + 16)));
    const f64 nl = Lf(K + 24);
    hi = S(Fsel15(S(hi - nl), hi, Ld(K + 32)));
    lo = S(Fsel15(S(lo - nl), lo, Ld(K + 32)));
    return {lo, hi};
}

}  // namespace

// u32 ThunkSpinY376598(Obj* o)
// (called by ThunkArg1_00376C70) Yaws o's target (o->224, else o->24) to
// o->232 degrees; see SpinY.
DECOMP(00376598, ThunkSpinY376598) {
    SpinY(ctx, arg<u32>(ctx, 0), 224, 232, 0x00376594u, false);
}

// u32 Obj_SpinY3739D0(-, Obj* o)
// (callback) Null o -> returns 0. Yaws o's target (o->192, else o->24) to
// o->352 degrees; see SpinY.
DECOMP(003739D0, Obj_SpinY3739D0) {
    const u32 o = arg<u32>(ctx, 1);
    if (o == 0) {
        ret(ctx, 0);
        return;
    }
    SpinY(ctx, o, 192, 352, 0x003739C8u, true);
}

// void Cam_AimDir106FB8(Cam* c, const Vec3* dir)
// (called by 0003C990, 0014FAD0) A dir shorter than sqrt(1e-5) (|x² + y² +
// z²| < the f32 at 0x00106F48; NaN goes on) does nothing (r3 = dir).
// Otherwise the pitch window (PitchWindow, K = 0x00106F48) rotates dir
// twice - Cam_RotateToward1007D0(sp+128, 0x01164B60, dir, pi/2 - lo), then
// (sp+112, 0x01164B90, sp+128, hi + pi/2) - into c->512..520; c->528 =
// acosf(clamp(dot(c->240, that), -1, 1)) * K108 / max(clamp(g, K88..K72)
// * K104, ...) with g the f32 at 0x01032DF8, the compiled fsel chain kept
// as is. Leaves r3 = 0x01032DB0.
DECOMP(00106FB8, Cam_AimDir106FB8) {
    using mid15::Fsel15;
    const u32 c = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    constexpr u32 K = 0x00106F48u;
    {
        const f64 y = Lf(d + 4), x = Lf(d), z = Lf(d + 8);
        const f64 n = std::fabs(Fmadds(z, z, Fmadds(x, x, S(y * y))));
        if (n < Lf(K)) {
            ctx.fpr[1] = n;
            ret(ctx, d);
            return;
        }
    }
    const Pitch p = PitchWindow(c, K, 4, 8);
    const f64 q = Lf(K + 40);
    Stack frame(ctx, 192);
    const u32 sp = frame.addr();
    callf(ctx, Cam_RotateToward1007D0, {sp + 128, 0x01164B60u, d}, {S(q - p.lo)});
    callf(ctx, Cam_RotateToward1007D0, {sp + 112, 0x01164B90u, sp + 128}, {S(p.hi + q)});
    const f64 v0 = Lf(sp + 112), v1 = Lf(sp + 116), v2 = Lf(sp + 120);
    Sf(c + 512, v0);
    Sf(c + 516, v1);
    Sf(c + 520, v2);
    const u32 e = c + 240;
    f64 x = Fmadds(Lf(e + 8), v2, Fmadds(Lf(e), v0, S(Lf(e + 4) * v1)));
    const f64 one = Lf(K + 44), d48 = Ld(K + 48), d64 = Ld(K + 64);
    x = S(Fsel15(S(one - x), x, d48));
    x = S(Fsel15(S(x - Lf(K + 56)), x, d64));
    x = S(Fsel15(S(x + one), x, d64));
    x = S(Fsel15(S(x - one), d48, x));
    const f64 ac = callf(ctx, mid15::m15fn::Acosf, {}, {x});
    f64 g = Lf(0x01032DB0u + 72);
    g = S(Fsel15(S(Lf(K + 72) - g), g, Ld(K + 80)));
    g = S(Fsel15(S(g - Lf(K + 88)), g, Ld(K + 96)));
    const f64 num = S(ac * Lf(K + 108));
    g = S(g * Lf(K + 104));
    g = S(Fsel15(S(one - g), d48, g));
    const f64 r = Divs(num, g);
    Sf(c + 528, r);
    ctx.fpr[1] = r;
    ret(ctx, 0x01032DB0u);
}

// f32 Cam_LookAt101A60(Cam* c, const Vec3* eye, const Vec3* target, Xf* out)
// (called by 00101CC0, 00104F80, 00105F90) dist = |target - eye|; below
// the f32 at 0x00101A30 (|dist| < 1e-5) returns the f32 at 0x00101A34
// (r3 = 0x00101A30). Otherwise the reversed direction eye - target is
// rotated through the pitch window (PitchWindow, K = 0x00101A30) like
// Cam_AimDir106FB8, negated into sp+144 and passed to Cam_Fn0FD040(sp+144,
// out); out->48.. = target - that * dist; returns dist (r3 = out + 48).
DECOMP(00101A60, Cam_LookAt101A60) {
    const u32 c = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1), t = arg<u32>(ctx, 2), out = arg<u32>(ctx, 3);
    constexpr u32 K = 0x00101A30u;
    const f64 dy = S(Lf(t + 4) - Lf(e + 4)), dx = S(Lf(t) - Lf(e)), dz = S(Lf(t + 8) - Lf(e + 8));
    const f64 dist = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, S(dy * dy))));
    if (std::fabs(dist) < Lf(K)) {
        ctx.fpr[1] = Lf(K + 4);
        ret(ctx, K);
        return;
    }
    Stack frame(ctx, 224);
    const u32 sp = frame.addr();
    Sf(sp + 128, -dx);
    Sf(sp + 132, -dy);
    Sf(sp + 136, -dz);
    const Pitch p = PitchWindow(c, K, 8, 12);
    Sf(sp + 152, dz);  // overwritten below before any read
    Sf(sp + 148, dy);
    Sf(sp + 144, dx);
    const f64 q = Lf(K + 40);
    callf(ctx, Cam_RotateToward1007D0, {sp + 160, 0x01164B60u, sp + 128}, {S(q - p.lo)});
    callf(ctx, Cam_RotateToward1007D0, {sp + 112, 0x01164B90u, sp + 160}, {S(p.hi + q)});
    const f64 v0 = Lf(sp + 112), v1 = Lf(sp + 116), v2 = Lf(sp + 120);
    Sf(sp + 144, -v0);
    Sf(sp + 148, -v1);
    Sf(sp + 152, -v2);
    call(ctx, Cam_Fn0FD040, {sp + 144, out});
    const f64 w0 = Lf(sp + 144), w1 = Lf(sp + 148), w2 = Lf(sp + 152);
    Sf(out + 48, Fnmsubs(w0, dist, Lf(t)));
    Sf(out + 52, Fnmsubs(w1, dist, Lf(t + 4)));
    Sf(out + 56, Fnmsubs(w2, dist, Lf(t + 8)));
    ctx.fpr[1] = dist;
    ret(ctx, out + 48);
}
