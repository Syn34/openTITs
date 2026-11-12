// Game code, large-function sweep part 32: refit a Havok box body to an
// AABB (new box shape, re-centred position, re-added to its world).
#include "../sly4.h"
#include "big32.h"

using namespace sly4;
using namespace sly4::big32;

namespace {
constexpr u32 kBoxK = 0x0044DE90;      // .text: f32 1e-5, 0.5, 0.0; lane masks y / z / w at +16 / +32 / +48; f32 1.0 at +64
constexpr u32 kZero3 = 0x01164B40;     // f32[3] the zero vector
constexpr u32 kConvexR = 0x00F9203C;   // f32 0.05 (box shape convex radius)
constexpr u32 Hk_BoxShapeCtor899720 = 0x00899720;  // (hkpBoxShape*, const hkVector4* half; f1 radius)
constexpr u32 Body_Remove931F00 = 0x00931F00;      // (world, body)
constexpr u32 HkPhantom_SetPos929FDC = 0x00929FDC; // (body, const hkVector4* pos; f1)
constexpr u32 AnimSkel_Release7D4EA8 = 0x007D4EA8; // (hkReferencedObject*) removeReference
constexpr u32 Body_ShapeChanged85F1F0 = 0x0085F1F0;  // (body + 16)
constexpr u32 Body_Add931D10 = 0x00931D10;         // (world, body)

// |a - b| < 1e-5 for each of three f32 pairs (fsubs, fabs, fcmpu lt).
bool Near3(u32 a, u32 b, f64 eps) {
    for (u32 k = 0; k < 12; k += 4)
        if (!(std::fabs(Subs(Lfs(a + k), Lfs(b + k))) < eps)) return false;
    return true;
}
}  // namespace

// void f(Body* b, const f32 box[2][4] (min +0, max +16), f32 ext[2][4])
// (called by f_00211E40 and f_00214640). With b, its world (+8) and a box
// shape (+16, byte +8 == 3) present: ext = (-half, +half) of the current
// shape (+32 its half extents; stored +16 / +20 / +24 then +0 / +4 / +8).
// Returns when box min or box max is within 1e-5 of kZero3 (r3 = box, f1 =
// 1e-5). Otherwise lo = ext[0] + min, hi = ext[1] + max; a 48-byte block from
// the Havok allocator (u16 +4 = 48 stored even when 0) becomes an
// hkpBoxShape of half extents (hi - lo) * 0.5 (w 0) and radius 0.05; the
// body is removed from its world, moved to its rotation (+160 / +176 /
// +192, read before the allocation) * (lo + hi) * 0.5 + translation (+208)
// (w 1.0), its old shape released, +16 = the new shape (0 when the
// allocation failed), f_0085F1F0(b + 16) called and the body re-added.
// r3 is left at box on the early returns.
DECOMP(0044DED8, Body_RefitBox_0044DED8) {
    const u32 b = arg<u32>(ctx, 0), box = arg<u32>(ctx, 1), ext = arg<u32>(ctx, 2);
    ctx.gpr[3] = box;  // r3 = box from the entry on (the early returns leave it)
    if (b == 0) return;
    const u32 world = load<u32>(b + 8);
    if (world == 0) return;
    const u32 sh = load<u32>(b + 16);
    if (load<u8>(sh + 8) != 3) return;
    const u32 hx = load<u32>(sh + 32), hy = load<u32>(sh + 36), hz = load<u32>(sh + 40);
    store<u32>(ext + 16, hx);
    store<u32>(ext + 20, hy);
    store<u32>(ext + 24, hz);
    store<u32>(ext + 0, hx ^ 0x80000000u);  // fneg
    store<u32>(ext + 4, hy ^ 0x80000000u);
    store<u32>(ext + 8, hz ^ 0x80000000u);
    const f64 eps = Lfs(kBoxK);
    if (Near3(box, kZero3, eps) || Near3(box + 16, kZero3, eps)) {
        ctx.fpr[1] = eps;
        return;
    }
    const f64 lx = Adds(Lfs(ext + 0), Lfs(box + 0)), ly = Adds(Lfs(ext + 4), Lfs(box + 4)), lz = Adds(Lfs(ext + 8), Lfs(box + 8));
    const f64 ux = Adds(Lfs(ext + 16), Lfs(box + 16)), uy = Adds(Lfs(ext + 20), Lfs(box + 20)), uz = Adds(Lfs(ext + 24), Lfs(box + 24));
    f32 rows[4][3];  // rotation rows and translation, copied before the calls
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) rows[r][k] = W32f(b + 160 + 16 * r + 4 * k);
    const f64 half = Lfs(kBoxK + 4);
    Stack frame(ctx, 48);
    const u32 sp = frame.addr() - 112;  // the original's frame (offsets below are its sp offsets)
    const V4 he{{static_cast<f32>(Muls(Subs(ux, lx), half)), static_cast<f32>(Muls(Subs(uy, ly), half)),
                 static_cast<f32>(Muls(Subs(uz, lz), half)), 0.0f}};
    Stvx(sp + 112, he);
    const u32 p = big17::HkAlloc(ctx, 48);
    u32 shape = 0;
    if (p != 0) {
        callf(ctx, Hk_BoxShapeCtor899720, {p, sp + 112}, {Lfs(kConvexR)});
        shape = p;
    }
    const f32 c[3] = {static_cast<f32>(Muls(Adds(ux, lx), half)), static_cast<f32>(Muls(Adds(uy, ly), half)),
                      static_cast<f32>(Muls(Adds(uz, lz), half))};
    V4 pos;
    for (u32 k = 0; k < 3; ++k) {
        f32 a = std::fma(rows[0][k], c[0], 0.0f);
        a = std::fma(rows[1][k], c[1], a);
        a = std::fma(rows[2][k], c[2], a);
        pos.e[k] = rows[3][k] + a;
    }
    pos.e[3] = W32f(kBoxK + 64);
    Stvx(sp + 144, pos);
    call(ctx, Body_Remove931F00, {world, b});
    callf(ctx, HkPhantom_SetPos929FDC, {b, sp + 144}, {Lfs(kBoxK + 8)});
    call(ctx, AnimSkel_Release7D4EA8, {load<u32>(b + 16)});
    store<u32>(b + 16, shape);
    call(ctx, Body_ShapeChanged85F1F0, {b + 16});
    call(ctx, Body_Add931D10, {world, b});
}
