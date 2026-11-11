// Game code, large-function sweep part 7: an object's attachment matrix
// with an aim offset turned towards the camera (0049FFE0).
#include "big7.h"

using namespace sly4;
using namespace sly4::big7;
using namespace sly4::big7::b7fn;

namespace {
constexpr u32 kObjMat = 96;           // f32 [4][4]
constexpr u32 kObjFlags172 = 172;     // 0x40000: has attachments
constexpr u32 kAttBone = 68;          // AttachMat (a Matrix): +68 s16 bone

constexpr u32 kConst = 0x0049FFA0u;   // .text: f32 0.01, f32 1.0; vperm masks {a0 b0 a0 a3} / {a0 a1 b0 a3} / {a0 a1 a2 b3} at +16 / +32 / +48
constexpr u32 kZero3 = 0x01164B40u;   // the zero vector
constexpr u32 kUp = 0x01164B60u;      // the up vector
constexpr u32 kIdentity = 0x01164BD0u;  // the identity matrix
constexpr u32 kWorldPtr = 0x00F65CA0u;  // World*: +0x1281E0 -> camera (+144 its position)
constexpr u32 kWorldCamera = 0x001281E0u;

V4 Col(f32 a, f32 b, f32 c, const V4& w, const V4& m1, const V4& m2, const V4& m3) {
    return Perm(Perm(Perm(Splat1(a), Splat1(b), m1), Splat1(c), m2), w, m3);
}
}  // namespace

// void Obj_AimMatrix(Obj* o, f32 out[4][4], const AttachMat* att, s32 bone, WorldObj* wo, const f32 pos[3], const f32 aim[3], const f32 quat[4])
// out starts as the identity. For an object with attachments (+172
// 0x40000): the rotation quat at pos, times the base matrix (the object's
// +96, times the bone matrix of att's bone and att itself; else bone's
// world matrix; else wo's +80). A non-zero aim offset (any component
// 0.01 or more from zero) is then turned into the frame looking from the
// result's position to the camera (up = the up vector) and added to the
// translation.
DECOMP(0049FFE0, Obj_AimMatrix) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 out = arg<u32>(ctx, 1);
    const u32 att = arg<u32>(ctx, 2);
    const u64 bone = ctx.gpr[6];
    const u32 wo = arg<u32>(ctx, 4);
    const u32 pos = arg<u32>(ctx, 5);
    const u32 aim = arg<u32>(ctx, 6);
    const u32 quat = arg<u32>(ctx, 7);
    const u32 last = CopyMat43Last(out, kIdentity);
    if (o == 0 || (load<u32>(o + kObjFlags172) & 0x40000u) == 0) {
        SetF1Bits(ctx, last);
        ret(ctx, quat);  // r3 still holds it at the original's exit
        return;
    }
    Stack frame(ctx, 352);  // the original's sp+112..+463
    const u32 sp = frame.addr() - 112;
    const u32 a = sp + 128;  // quat's rotation, pos as its last row
    const u32 b = sp + 192;  // the base matrix
    call(ctx, kQuatToMatrix, {a, quat, kZero3});
    CopyW(a + 48, pos);
    CopyW(a + 52, pos + 4);
    CopyW(a + 56, pos + 8);
    CopyMat43W(b, o + kObjMat);
    if (att != 0) {
        const u32 c = sp + 256;
        call(ctx, kObjBoneMatrix, {c, o, static_cast<u64>(static_cast<s64>(static_cast<s16>(load<u16>(att + kAttBone)))), 0, 1});
        call(ctx, kMatrixMultiply, {c, att});
        const V4 c0 = Lvx(c), c1 = Lvx(c + 16), c2 = Lvx(c + 32), c3 = Lvx(c + 48);
        const V4 b0 = Lvx(b), b1 = Lvx(b + 16), b2 = Lvx(b + 32), b3 = Lvx(b + 48);
        Stvx(b, Row3(c0, c1, c2, b0.e[0], b0.e[1], b0.e[2]));
        Stvx(b + 16, Row3(c0, c1, c2, b1.e[0], b1.e[1], b1.e[2]));
        Stvx(b + 32, Row3(c0, c1, c2, b2.e[0], b2.e[1], b2.e[2]));
        Stvx(b + 48, Add4(c3, Row3(c0, c1, c2, b3.e[0], b3.e[1], b3.e[2])));
    } else if (static_cast<s32>(bone) != -1) {
        call(ctx, kObjBoneMatrix, {sp + 320, o, bone, 1, 1});
        CopyMat43W(b, sp + 320);
    } else if (wo != 0) {
        CopyMat43W(b, wo + 80);
    }
    {
        const V4 b0 = Lvx(b), b1 = Lvx(b + 16), b2 = Lvx(b + 32), b3 = Lvx(b + 48);
        const V4 a0 = Lvx(a), a1 = Lvx(a + 16), a2 = Lvx(a + 32), a3 = Lvx(a + 48);
        Stvx(out, Row3(b0, b1, b2, a0.e[0], a0.e[1], a0.e[2]));
        Stvx(out + 16, Row3(b0, b1, b2, a1.e[0], a1.e[1], a1.e[2]));
        Stvx(out + 32, Row3(b0, b1, b2, a2.e[0], a2.e[1], a2.e[2]));
        Stvx(out + 48, Add4(b3, Row3(b0, b1, b2, a3.e[0], a3.e[1], a3.e[2])));
    }
    ret(ctx, kConst);  // r3 at the original's exit
    const f32 eps = F(kConst);
    const f32 px = F(out + 48), py = F(out + 52);
    const u32 pzw = load<u32>(out + 56);
    const f32 pz = std::bit_cast<f32>(pzw);
    if (std::fabs(F(aim) - F(kZero3)) < eps && std::fabs(F(aim + 4) - F(kZero3 + 4)) < eps && std::fabs(F(aim + 8) - F(kZero3 + 8)) < eps) {
        SetF1Bits(ctx, pzw);
        return;
    }
    // d: unit vector to the camera; s = up x d; t = d x s.
    const u32 cam = load<u32>(load<u32>(kWorldPtr) + kWorldCamera) + 144;
    f32 dy = F(cam + 4) - py;
    f32 dx = F(cam) - px;
    f32 dz = F(cam + 8) - pz;
    f32 l2 = dy * dy;
    l2 = Fmadds(dx, dx, l2);
    l2 = Fmadds(dz, dz, l2);
    const f32 len = static_cast<f32>(std::sqrt(static_cast<f64>(l2)));
    const f32 inv = static_cast<f32>(static_cast<f64>(F(kConst + 4)) / len);
    dy = dy * inv;
    dx = dx * inv;
    dz = dz * inv;
    const f32 ux = F(kUp), uy = F(kUp + 4), uz = F(kUp + 8);
    const f32 sx = Fmsubs(uy, dz, uz * dy);
    const f32 sz = Fmsubs(ux, dy, uy * dx);
    const f32 sy = Fmsubs(uz, dx, ux * dz);
    const f32 tz = Fmsubs(dx, sy, dy * sx);
    const f32 ty = Fmsubs(dz, sx, dx * sz);
    const f32 tx = Fmsubs(dy, sz, dz * sy);
    const V4 m1 = Lvx(kConst + 16), m2 = Lvx(kConst + 32), m3 = Lvx(kConst + 48);
    // The w lanes come from stack words the original never writes (sp+396 /
    // +412 / +428); only x/y/z are used.
    const V4 c0 = Col(sz, sy, sx, Lvx(sp + 384), m1, m2, m3);
    const V4 c1 = Col(tz, ty, tx, Lvx(sp + 400), m1, m2, m3);
    const V4 c2 = Col(dz, dy, dx, Lvx(sp + 416), m1, m2, m3);
    const f32 a0 = std::bit_cast<f32>(LaneW(Lvlx(aim), 0));
    const f32 a1 = std::bit_cast<f32>(LaneW(Lvlx(aim + 4), 0));
    const f32 a2 = std::bit_cast<f32>(LaneW(Lvlx(aim + 8), 0));
    const V4 v = Row3(c0, c1, c2, a0, a1, a2);
    const f32 nz = pz + v.e[2];
    store<f32>(out + 48, px + v.e[0]);
    store<f32>(out + 52, py + v.e[1]);
    store<f32>(out + 56, nz);
    ctx.fpr[1] = nz;
}
