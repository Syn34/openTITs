// Game code, large-function sweep part 32: the per-shape-type dispatch of a
// body force (buoyancy / drag): leaf shapes go to their handlers, container
// and transform shapes recurse over their children.
#include "../sly4.h"
#include "big32.h"

using namespace sly4;
using namespace sly4::big32;

namespace {
constexpr u32 kZeroF = 0x004ABC24;  // .text: f32 0.0
constexpr u32 Self = 0x004ABC28;
constexpr u32 HkBody_SphereDrag_004AB940 = 0x004AB940;       // (o, shape, xf): type 0
constexpr u32 HkBody_CylinderImpulse_004AB690 = 0x004AB690;  // (o, shape, xf): type 1
constexpr u32 Floater_TriangleForce_004AB2B0 = 0x004AB2B0;   // (o, shape, xf): type 2
constexpr u32 ShapeForce_Type3_4AA010 = 0x004AA010;          // (o, shape, xf): type 3
constexpr u32 ShapeForce_Type4_4AAC60 = 0x004AAC60;          // (o, shape, xf): type 4
constexpr u32 ShapeForce_Type5A_4AA4D0 = 0x004AA4D0;         // (o, shape, xf): type 5 when o +56 > 0
constexpr u32 ShapeForce_Type5B_4A9800 = 0x004A9800;         // (o, shape, xf): type 5 otherwise
constexpr u32 Mat_RotateVec7D16D0 = 0x007D16D0;              // (f32 out[4], const hkTransform*, const f32 v[4]): rows 0..2 * v
constexpr u32 HkQs_ToTransform7CF654 = 0x007CF654;           // (const hkQsTransform*, hkTransform* out)
constexpr u32 HkTransform_SetMul7CF51C = 0x007CF51C;         // (hkTransform* out, const hkTransform* a, const hkTransform* b)

// The children of container c: key = vfn[first](c); while key != -1 (low
// word): child = vfn[get](c, key, buf); recurse; key = vfn[next](c, key).
void Kids(PPUContext& ctx, u32 o, u32 c, u32 first, u32 get, u32 next, u32 buf, u32 xf) {
    u64 key = VCall(ctx, c, first, {c});
    while (static_cast<s32>(key) != -1) {
        const u64 kid = VCall(ctx, c, get, {c, key, buf});
        call(ctx, Self, {o, kid, xf});
        key = VCall(ctx, c, next, {c, key});
    }
}
}  // namespace

// void f(Obj* o, const hkpShape* s, const hkTransform* xf) (called by
// Force_Apply_004AC210 and itself). By the shape type (u8 +8):
//  0 / 1 / 2 / 3 / 4: the leaf handlers 0x004AB940 / 0x004AB690 /
//    0x004AB2B0 / 0x004AA010 / 0x004AAC60 (o, s, xf);
//  5: 0x004AA4D0 when o +56 > 0.0, else 0x004A9800 (f1 left at o +56);
//  6 / 8 / 18 and 13 / 27: the container at s + 16 (vfn 12 first key, 24
//    child into a 256-byte buffer at sp + 384 resp. sp + 640, 16 next key);
//  7 / 9 / 22: the container s->vfn[60]() (buffer sp + 1152);
//  26: s itself (vfn 80 / 92 / 84, buffer sp + 896);
//  10: the child +24 under xf with translation R * (s + 32) + xf.t
//    (0x007D16D0, all four lanes), the transform built at sp + 320;
//  11: the child +24 under xf * (s + 32 as hkQsTransform);
//  14: the child +20 under xf * (s + 48);
//  30: the child +24 under xf.
// Other types do nothing (r3 left at s).
DECOMP(004ABC28, Shape_ApplyForce_004ABC28) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1), xf = arg<u32>(ctx, 2);
    Stack frame(ctx, 1296);
    const u32 sp = frame.addr() - 112;  // the original's frame (offsets below are its sp offsets)
    ctx.gpr[3] = s;
    switch (load<u8>(s + 8)) {
    case 0: call(ctx, HkBody_SphereDrag_004AB940, {o, s, xf}); break;
    case 1: call(ctx, HkBody_CylinderImpulse_004AB690, {o, s, xf}); break;
    case 2: call(ctx, Floater_TriangleForce_004AB2B0, {o, s, xf}); break;
    case 3: call(ctx, ShapeForce_Type3_4AA010, {o, s, xf}); break;
    case 4: call(ctx, ShapeForce_Type4_4AAC60, {o, s, xf}); break;
    case 5: {
        const f64 v = Lfs(o + 56);
        ctx.fpr[1] = v;  // left in f1 by the original's compare (no callee reads it)
        call(ctx, v > Lfs(kZeroF) ? ShapeForce_Type5A_4AA4D0 : ShapeForce_Type5B_4A9800, {o, s, xf});
        break;
    }
    case 6: case 8: case 18: Kids(ctx, o, s + 16, 12, 24, 16, sp + 384, xf); break;
    case 13: case 27: Kids(ctx, o, s + 16, 12, 24, 16, sp + 640, xf); break;
    case 7: case 9: case 22: {
        const u32 c = static_cast<u32>(VCall(ctx, s, 60, {s}));
        Kids(ctx, o, c, 12, 24, 16, sp + 1152, xf);
        break;
    }
    case 26: Kids(ctx, o, s, 80, 92, 84, sp + 896, xf); break;
    case 10: {
        const u32 kid = load<u32>(s + 24);
        call(ctx, Mat_RotateVec7D16D0, {sp + 112, xf, s + 32});
        const V4 t = Add4(Lvx(sp + 112), Lvx(xf + 48));
        Stvx(sp + 112, t);
        Stvx(sp + 320, Lvx(xf));
        Stvx(sp + 336, Lvx(xf + 16));
        const V4 r2 = Lvx(xf + 32);
        Stvx(sp + 368, t);
        Stvx(sp + 352, r2);
        call(ctx, Self, {o, kid, sp + 320});
        break;
    }
    case 11: {
        const u32 kid = load<u32>(s + 24);
        call(ctx, HkQs_ToTransform7CF654, {s + 32, sp + 192});
        call(ctx, HkTransform_SetMul7CF51C, {sp + 256, xf, sp + 192});
        call(ctx, Self, {o, kid, sp + 256});
        break;
    }
    case 14: {
        const u32 kid = load<u32>(s + 20);
        call(ctx, HkTransform_SetMul7CF51C, {sp + 128, xf, s + 48});
        call(ctx, Self, {o, kid, sp + 128});
        break;
    }
    case 30: call(ctx, Self, {o, load<u32>(s + 24), xf}); break;
    default: break;
    }
}
