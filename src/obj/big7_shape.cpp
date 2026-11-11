// Game code, large-function sweep part 7: a collision shape record from a
// Havok rigid body (00011410).
#include "big7.h"

using namespace sly4;
using namespace sly4::big7;
using namespace sly4::big7::b7fn;

namespace {
// hkpRigidBody: +16 its shape, +240 its transform (4 rows of 4 f32).
constexpr u32 kBodyShape = 16;
constexpr u32 kBodyXform = 240;
// hkpShape: +8 u8 type; vfunc +20 getAabb(shape, const hkTransform*, f1 tolerance, hkAabb* out).
constexpr u32 kShapeType = 8;
constexpr u32 kVfnGetAabb = 20;
constexpr s32 kTypeConvexTranslate = 10;  // +32 translation
constexpr s32 kTypeConvexTransform = 11;  // +48 transform
constexpr s32 kTypeTransform = 14;        // +48 transform

constexpr u32 kConst = 0x000113D0u;     // .text: f32 0.0 (tolerance), f32 2.0; lane masks y / z / w at +16 / +32 / +48
constexpr u32 kIdentity = 0x00B2C360u;  // the identity hkTransform
constexpr u32 kHalf = 0x00B2C2C0u;      // f32 0.5 x 4
constexpr u32 kTransformMul = 0x007CF51Cu;    // hkTransform::setMul(out, a, b) (Havok)
constexpr u32 kTransformMulEq = 0x007CF188u;  // hkTransform::setMulEq(this, a) (Havok)

// shape->getAabb(xf, tolerance, out) (the float takes r5's slot, which holds the OPD).
void GetAabb(PPUContext& ctx, u32 shape, u32 xf, f64 tol, u32 out) {
    const u32 opd = load<u32>(load<u32>(shape) + kVfnGetAabb);
    ctx.fpr[1] = tol;
    call_ptr(ctx, opd, {shape, xf, opd, out, load<u32>(opd)});
}
}  // namespace

// u32 Shape_InitBody(Shape* out, System* sys, hkpRigidBody* body, u32 unused, u32 a4, u16 a5)
// Builds the shape record from the body: its transform (combined with a
// translate / transform shape's own), half the extents of the shape's
// local AABB times 2.0, and for other shapes the world AABB's centre as the
// translation; then Shape_Init110EC(out, sys, matrix, extents, a4, a5).
DECOMP(00011410, Shape_InitBody) {
    const u32 out = arg<u32>(ctx, 0);
    const u32 sys = arg<u32>(ctx, 1);
    const u32 body = arg<u32>(ctx, 2);
    const u32 a4 = arg<u32>(ctx, 4);
    const u32 a5 = arg<u32>(ctx, 5);
    Stack frame(ctx, 336);  // the original's sp+112..+447
    const u32 sp = frame.addr() - 112;
    const u32 m = sp + 256;  // the body's transform (whole rows)
    for (u32 r = 0; r < 4; ++r) Stvx(m + 16 * r, Lvx(body + kBodyXform + 16 * r));
    bool local = false;  // the shape carries its own transform
    const u32 shape = load<u32>(body + kBodyShape);
    if (shape != 0) {
        const s32 type = load<u8>(shape + kShapeType);
        if (type > 11) {
            if (type == kTypeTransform) {
                call(ctx, kTransformMulEq, {m, shape + 48});
                local = true;
            }
        } else if (type == kTypeConvexTransform) {
            call(ctx, kTransformMul, {m, shape + 48, m});
            local = true;
        } else if (type == kTypeConvexTranslate) {
            // m's x/y/z words to sp+384 (w lanes left), the translation
            // through it into m's last row with w = 0.
            const u32 t = sp + 384;
            for (u32 r = 0; r < 4; ++r)
                for (u32 c = 0; c < 3; ++c) CopyW(t + 16 * r + 4 * c, m + 16 * r + 4 * c);
            const f32 s0 = std::bit_cast<f32>(LaneW(Lvlx(shape + 32), 0));
            const f32 s1 = std::bit_cast<f32>(LaneW(Lvlx(shape + 36), 0));
            const f32 s2 = std::bit_cast<f32>(LaneW(Lvlx(shape + 40), 0));
            const V4 p = Add4(Lvx(t + 48), Row3(Lvx(t), Lvx(t + 16), Lvx(t + 32), s0, s1, s2));
            Stvx(sp + 128, p);
            const V4 v = Andc(Sel(Sel(Splat(p, 0), Splat(p, 1), Lvx(kConst + 16)), Splat(p, 2), Lvx(kConst + 32)), Lvx(kConst + 48));
            Stvx(m + 48, v);
            local = true;
        }
    }
    // The x/y/z words of m to the matrix passed on (sp+320).
    const u32 mat = sp + 320;
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 3; ++c) CopyW(mat + 16 * r + 4 * c, m + 16 * r + 4 * c);
    const f64 tol = F(kConst);
    const f32 scale = F(kConst + 4);
    const V4 half = Lvx(kHalf);
    f32 hx, hy, hz;
    if (local) {
        GetAabb(ctx, load<u32>(body + kBodyShape), kIdentity, tol, sp + 192);
        const V4 h = Mul0(half, Sub4(Lvx(sp + 208), Lvx(sp + 192)));
        Stvx(sp + 144, h);
        hx = h.e[0], hy = h.e[1], hz = h.e[2];
    } else {
        GetAabb(ctx, load<u32>(body + kBodyShape), kIdentity, tol, sp + 224);
        const V4 h = Mul0(half, Sub4(Lvx(sp + 240), Lvx(sp + 224)));
        Stvx(sp + 160, h);
        hx = h.e[0], hy = h.e[1], hz = h.e[2];
        GetAabb(ctx, load<u32>(body + kBodyShape), m, tol, sp + 224);
        const V4 c = Mul0(half, Add4(Lvx(sp + 224), Lvx(sp + 240)));
        Stvx(sp + 176, c);
        store<u32>(mat + 48, LaneW(c, 0));
        store<u32>(mat + 52, LaneW(c, 1));
        store<u32>(mat + 56, LaneW(c, 2));
    }
    const u32 ext = sp + 112;
    store<f32>(ext, hx * scale);
    store<f32>(ext + 4, hy * scale);
    store<f32>(ext + 8, hz * scale);
    ctx.fpr[1] = hy * scale;
    call(ctx, kShapeInit, {out, sys, mat, ext, a4, a5});
}
