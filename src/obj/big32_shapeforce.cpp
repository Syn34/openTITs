// Game code, large-function sweep part 32: the per-shape-type dispatch of a
// body force (buoyancy / drag): leaf shapes go to their handlers, container
// and transform shapes recurse over their children.
#include "big32.h"

namespace sly4::obj {

// An hkpShape as this dispatch reads it (by type): +8 the type; +16 an
// embedded container (types 6 / 8 / 18 / 13 / 27); +20 the child of type
// 14 with its transform at +48; +24 the child of types 10 / 11 / 30 with
// its vector / hkQsTransform at +32.
struct ForceShape {
    pad<8> _0;
    u8 type;                 // +8
    pad<7> _9;
    pad<4> container;        // +16
    gp<ForceShape> child20;  // +20
    gp<ForceShape> child24;  // +24
    pad<4> _28;
    Vec4 v32;                // +32
    Vec4 v48;                // +48
};
static_assert(offsetof(ForceShape, child24) == 24 && offsetof(ForceShape, v32) == 32 && offsetof(ForceShape, v48) == 48);
struct ForceObj {
    pad<56> _0;
    be_f32 level;  // +56: type 5 picks its handler by level > 0
};
// hkTransform: three rotation rows and the translation (64 bytes).
struct HkXf {
    Vec4 r[4];
};

namespace {
constexpr u32 kZeroF = 0x004ABC24;  // .text: f32 0.0
constexpr u32 HkBody_SphereDrag_004AB940 = 0x004AB940;       // (o, shape, xf): type 0
constexpr u32 HkBody_CylinderImpulse_004AB690 = 0x004AB690;  // (o, shape, xf): type 1
constexpr u32 Floater_TriangleForce_004AB2B0 = 0x004AB2B0;   // (o, shape, xf): type 2
constexpr u32 ShapeForce_Type3_4AA010 = 0x004AA010;          // (o, shape, xf): type 3
constexpr u32 ShapeForce_Type4_4AAC60 = 0x004AAC60;          // (o, shape, xf): type 4
constexpr u32 ShapeForce_Type5A_4AA4D0 = 0x004AA4D0;         // (o, shape, xf): type 5 when o->level > 0
constexpr u32 ShapeForce_Type5B_4A9800 = 0x004A9800;         // (o, shape, xf): type 5 otherwise
constexpr u32 Mat_RotateVec7D16D0 = 0x007D16D0;              // (f32 out[4], const hkTransform*, const f32 v[4]): rows 0..2 * v
constexpr u32 HkQs_ToTransform7CF654 = 0x007CF654;           // (const hkQsTransform*, hkTransform* out)
constexpr u32 HkTransform_SetMul7CF51C = 0x007CF51C;         // (hkTransform* out, const hkTransform* a, const hkTransform* b)
}  // namespace

void Shape_ApplyForce_004ABC28(ForceObj* o, ForceShape* s, const HkXf* xf);

namespace {
// The children of container c: key = vfn[first](c); while key != -1 (low
// word): child = vfn[get](c, key, buf); recurse; key = vfn[next](c, key).
void Kids(ForceObj* o, u8* c, u32 first, u32 get, u32 next, u8* buf, const HkXf* xf) {
    u64 key = vcall<u64>(c, first);
    while (static_cast<s32>(key) != -1) {
        ForceShape* kid = vcall<ForceShape*>(c, get, key, buf);
        Shape_ApplyForce_004ABC28(o, kid, xf);
        key = vcall<u64>(c, next, key);
    }
}
}  // namespace

// void f(Obj* o, const hkpShape* s, const hkTransform* xf) (called by
// Force_Apply_004AC210 and itself). By the shape type:
//  0 / 1 / 2 / 3 / 4: the leaf handlers 0x004AB940 / 0x004AB690 /
//    0x004AB2B0 / 0x004AA010 / 0x004AAC60 (o, s, xf);
//  5: 0x004AA4D0 when o->level > 0.0, else 0x004A9800;
//  6 / 8 / 18 and 13 / 27: the embedded container (vfn 12 first key, 24
//    child into a 256-byte stack buffer, 16 next key);
//  7 / 9 / 22: the container s->vfn[60]();
//  26: s itself (vfn 80 / 92 / 84);
//  10: the child +24 under xf with translation R * v32 + xf.t
//    (0x007D16D0, all four lanes);
//  11: the child +24 under xf * (v32 as hkQsTransform);
//  14: the child +20 under xf * (the transform at +48);
//  30: the child +24 under xf.
// Other types do nothing.
void Shape_ApplyForce_004ABC28(ForceObj* o, ForceShape* s_, const HkXf* xf) {
    ForceShape& s = gm<ForceShape>(guest(s_));  // unchecked
    GuestFrame frame(1296);  // the original's frame from sp+112 (offsets below: sp offset - 112)
    auto at = [&](u32 sp_off) { return frame.as<u8>(sp_off - 112); };
    switch (s.type) {
    case 0: call(HkBody_SphereDrag_004AB940, o, &s, xf); break;
    case 1: call(HkBody_CylinderImpulse_004AB690, o, &s, xf); break;
    case 2: call(Floater_TriangleForce_004AB2B0, o, &s, xf); break;
    case 3: call(ShapeForce_Type3_4AA010, o, &s, xf); break;
    case 4: call(ShapeForce_Type4_4AAC60, o, &s, xf); break;
    case 5: {
        const f64 v = lf(gm<ForceObj>(guest(o)).level);
        call(v > lf(gm<be_f32>(kZeroF)) ? ShapeForce_Type5A_4AA4D0 : ShapeForce_Type5B_4A9800, o, &s, xf);
        break;
    }
    case 6: case 8: case 18: Kids(o, s.container.raw, 12, 24, 16, at(384), xf); break;
    case 13: case 27: Kids(o, s.container.raw, 12, 24, 16, at(640), xf); break;
    case 7: case 9: case 22: {
        u8* c = vcall<u8*>(&s, 60);
        Kids(o, c, 12, 24, 16, at(1152), xf);
        break;
    }
    case 26: Kids(o, reinterpret_cast<u8*>(&s), 80, 92, 84, at(896), xf); break;
    case 10: {
        ForceShape* kid = s.child24;
        u8* v = at(112);
        call(Mat_RotateVec7D16D0, v, xf, &s.v32);
        const V4 t = Add4(Lvx(v), Lvx(&xf->r[3]));
        Stvx(v, t);
        HkXf* nx = frame.as<HkXf>(320 - 112);
        Stvx(&nx->r[0], Lvx(&xf->r[0]));
        Stvx(&nx->r[1], Lvx(&xf->r[1]));
        const V4 r2 = Lvx(&xf->r[2]);
        Stvx(&nx->r[3], t);
        Stvx(&nx->r[2], r2);
        Shape_ApplyForce_004ABC28(o, kid, nx);
        break;
    }
    case 11: {
        ForceShape* kid = s.child24;
        u8* q = at(192);
        HkXf* nx = frame.as<HkXf>(256 - 112);
        call(HkQs_ToTransform7CF654, &s.v32, q);
        call(HkTransform_SetMul7CF51C, nx, xf, q);
        Shape_ApplyForce_004ABC28(o, kid, nx);
        break;
    }
    case 14: {
        ForceShape* kid = s.child20;
        HkXf* nx = frame.as<HkXf>(128 - 112);
        call(HkTransform_SetMul7CF51C, nx, xf, &s.v48);
        Shape_ApplyForce_004ABC28(o, kid, nx);
        break;
    }
    case 30: Shape_ApplyForce_004ABC28(o, s.child24, xf); break;
    default: break;
    }
}

}  // namespace sly4::obj

NATIVE(004ABC28, sly4::obj::Shape_ApplyForce_004ABC28)
