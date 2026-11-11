// Game code, VMX sweep part 2 (see vmx2.h): leaf vector / matrix math.
#include "../sly4.h"
#include "vmx2.h"

using namespace sly4;
using namespace sly4::vmx2;

namespace {
constexpr u32 kClampXZPerm = 0x000B37C0;  // three vperm masks: {a0 b0 a0 b3}, {a0 a1 b2 b3}, {a0 a1 b0 b1}
}  // namespace

// void f(Vec4* p, const OrientedBox* b) (called by 000B3A60 / 000BEAF8):
// clamps p in the box's xz plane. b: +0 a 4x4 matrix (rows of 16 bytes,
// translation row +48), +64 min, +80 max. p goes to the box's frame (the
// transposed rotation applied to p - t, where t is {t.x, p.y, t.z}: y keeps
// its own height), x and z are clamped (above max: max, else below min:
// min; y is not), and the result goes back through the matrix (+ t).
// The original builds its rows with vperm from lvlx loads and pads the w
// lanes with four quadwords of its own uninitialised frame (r1+48..+111),
// so p's final w is computed from whatever the stack holds there; the same
// stack words are read here (the frame is reserved at the same place).
// f1 is left as the original leaves it (the pre-clamp z when z was above
// max, else min.z).
DECOMP(000B37F0, OBox_ClampPointXZ) {
    const u32 p = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    Stack frame(ctx, 192);
    const u32 sp = frame.addr();
    const V4 P0 = Lvx(kClampXZPerm), P1 = Lvx(kClampXZPerm + 16), P2 = Lvx(kClampXZPerm + 32);
    const V4 s48 = Lvx(sp + 48), s64 = Lvx(sp + 64), s80 = Lvx(sp + 80), s96 = Lvx(sp + 96);
    const V4 zero = Zero4();
    const V4 m0 = Lvlx(b), m1 = Lvlx(b + 16), m2 = Lvlx(b + 32);
    const V4 m01 = Lvlx(b + 4), m11 = Lvlx(b + 20), m21 = Lvlx(b + 36);
    const V4 m02 = Lvlx(b + 8), m12 = Lvlx(b + 24), m22 = Lvlx(b + 40);
    const V4 m3 = Lvlx(b + 48), m32 = Lvlx(b + 56);
    // t = {t.x, p.y, t.z, pad}
    const V4 t = Perm(Perm(m3, Lvlx(p + 4), P0), Perm(m32, s96, P0), P1);
    const V4 nt = Sub4(zero, t);
    // the transposed rotation's columns
    const V4 c0 = Perm(Mrghw(m0, m1), Mrghw(m2, m11), P2);
    const V4 c1 = Perm(Mrghw(m01, m11), Mrghw(m21, m0), P2);
    const V4 c2 = Perm(Mrghw(m02, m12), Mrghw(m22, m02), P2);
    V4 a = Madd4(c0, SplatW(p), zero);
    V4 o = Madd4(c0, Splat(nt, 0), zero);
    a = Madd4(c1, SplatW(p + 4), a);
    o = Madd4(c1, Splat(nt, 1), o);
    a = Madd4(c2, SplatW(p + 8), a);
    o = Madd4(c2, Splat(nt, 2), o);
    // the matrix rows (w from the stack padding)
    const V4 r0 = Perm(Perm(m0, m01, P0), Perm(m02, s48, P0), P1);
    const V4 r1 = Perm(Perm(m1, m11, P0), Perm(m12, s64, P0), P1);
    const V4 r2 = Perm(Perm(m2, m21, P0), Perm(m22, s80, P0), P1);
    Stvx(p, Add4(o, a));
    // clamp x, then z
    if (F(p) > F(b + 80)) CopyW(p, b + 80);
    else if (F(p) < F(b + 64)) CopyW(p, b + 64);
    const u32 z = load<u32>(p + 8);
    if (F(p + 8) > F(b + 88)) {
        CopyW(p + 8, b + 88);
        SetF1Bits(ctx, z);
    } else {
        if (F(p + 8) < F(b + 72)) CopyW(p + 8, b + 72);
        SetF1Bits(ctx, load<u32>(b + 72));
    }
    V4 w = Madd4(r0, SplatW(p), zero);
    w = Madd4(r1, SplatW(p + 4), w);
    w = Madd4(r2, SplatW(p + 8), w);
    Stvx(p, Add4(t, w));
}

// void f(Node* o; f32 t) (called by 000BE3F0): refreshes a node's world
// matrix (+16 rows, 4x3) and snapshots it. +8 |= 0x80000000. With a parent
// link (+224): its +8 matrix is copied in (raw words) and then multiplied
// into the owner's (+212 -> +160) +80 matrix: each row goes through the
// owner's rows (vmaddfp chain; the translation row also + the owner's
// translation) and is stored back whole (stvx); the owner's matrix is
// not null-checked there (quirk: a missing one is read from address 80).
// Else the def's (+216) +80
// matrix, else the owner's +80 matrix when there is one, is copied in.
// Then +228 = 1, the 4x3 copied to +144 and +236 = t (stfs).
DECOMP(000B4B90, Node_UpdateWorld_000B4B90) {
    const u32 o = arg<u32>(ctx, 0);
    const f32 t = static_cast<f32>(ctx.fpr[1]);
    auto copy43 = [](u32 dst, u32 src) {
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) CopyW(dst + 16 * r + 4 * k, src + 16 * r + 4 * k);
    };
    store<u32>(o + 8, load<u32>(o + 8) | 0x80000000u);
    const u32 link = load<u32>(o + 224);
    if (link != 0) {
        copy43(o + 16, load<u32>(link + 8));
        const Mat4 p = LoadMat(load<u32>(load<u32>(o + 212) + 160) + 80);
        const Mat4 l = LoadMat(o + 16);
        const V4 r0 = XformDir(p.r[0], p.r[1], p.r[2], l.r[0]);
        const V4 r1 = XformDir(p.r[0], p.r[1], p.r[2], l.r[1]);
        const V4 r2 = XformDir(p.r[0], p.r[1], p.r[2], l.r[2]);
        const V4 r3 = XformPoint(p.r[0], p.r[1], p.r[2], p.r[3], l.r[3]);
        Stvx(o + 16, r0);
        Stvx(o + 32, r1);
        Stvx(o + 48, r2);
        Stvx(o + 64, r3);
    } else if (load<u32>(o + 216) != 0) {
        copy43(o + 16, load<u32>(o + 216) + 80);
    } else {
        const u32 p = load<u32>(load<u32>(o + 212) + 160);
        if (p != 0) copy43(o + 16, p + 80);
    }
    store<u32>(o + 228, 1);
    copy43(o + 144, o + 16);
    store<f32>(o + 236, t);
}
