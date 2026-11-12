// Game code, large-function sweep part 32: refit a Havok box body to an
// AABB (new box shape, re-centred position, re-added to its world).
#include "big32.h"

namespace sly4::obj {

struct HkBoxShape {
    pad<8> _0;
    u8 type;      // +8: 3 a box
    pad<23> _9;
    Vec4 half;    // +32: half extents
};
static_assert(offsetof(HkBoxShape, half) == 32);
struct RefitBody {
    pad<8> _0;
    be_u32 world;            // +8
    pad<4> _12;
    gp<HkBoxShape> shape;    // +16
    pad<140> _20;
    Vec4 rot[3];             // +160: rotation rows
    Vec4 t;                  // +208: translation
};
static_assert(offsetof(RefitBody, shape) == 16 && offsetof(RefitBody, rot) == 160 && offsetof(RefitBody, t) == 208);
// An AABB as two quadwords.
struct Aabb8 {
    Vec4 min;  // +0
    Vec4 max;  // +16
};

namespace {
constexpr u32 kBoxK = 0x0044DE90;      // .text: f32 1e-5, 0.5, 0.0; f32 1.0 at +64
constexpr u32 kConvexR = 0x00F9203C;   // f32 0.05 (box shape convex radius)
constexpr u32 Hk_BoxShapeCtor899720 = 0x00899720;  // (hkpBoxShape*, const hkVector4* half; f1 radius)
constexpr u32 Body_Remove931F00 = 0x00931F00;      // (world, body)
constexpr u32 HkPhantom_SetPos929FDC = 0x00929FDC; // (body, const hkVector4* pos; f1)
constexpr u32 AnimSkel_Release7D4EA8 = 0x007D4EA8; // (hkReferencedObject*) removeReference
constexpr u32 Body_ShapeChanged85F1F0 = 0x0085F1F0;  // (body + 16)
constexpr u32 Body_Add931D10 = 0x00931D10;         // (world, body)

const be_f32& KF(u32 off) { return gm<be_f32>(kBoxK + off); }

// |a - b| < eps for each of three f32 pairs (fsubs, fabs, fcmpu lt).
bool Near3(const Vec4& a, const Vec3& b, f64 eps) {
    return std::fabs(Subs(lf(a.x), lf(b.x))) < eps && std::fabs(Subs(lf(a.y), lf(b.y))) < eps &&
           std::fabs(Subs(lf(a.z), lf(b.z))) < eps;
}
}  // namespace

// void f(Body* b, const Aabb* box, Aabb* ext) (called by f_00211E40 and
// f_00214640). With b, its world and a box shape present: ext = (-half,
// +half) of the current shape. Returns when box min or box max is within
// 1e-5 of the zero vector. Otherwise lo = ext.min + min, hi = ext.max +
// max; a 48-byte block from the Havok allocator (HkAlloc) becomes an
// hkpBoxShape of half extents (hi - lo) * 0.5 (w 0) and radius 0.05; the
// body is removed from its world, moved to its rotation (read before the
// allocation) * (lo + hi) * 0.5 + translation (w 1.0), its old shape
// released, the new shape set (0 when the allocation failed),
// Body_ShapeChanged85F1F0(&b->shape) called and the body re-added.
void Body_RefitBox_0044DED8(RefitBody* b, const Aabb8* box, Aabb8* ext) {
    if (b == nullptr) return;
    const u32 world = b->world;
    if (world == 0) return;
    const HkBoxShape& sh = gm<HkBoxShape>(b->shape.ea);  // unchecked
    if (sh.type != 3) return;
    const u32 hx = bits(sh.half.x), hy = bits(sh.half.y), hz = bits(sh.half.z);  // raw words
    set_bits(ext->max.x, hx);
    set_bits(ext->max.y, hy);
    set_bits(ext->max.z, hz);
    set_bits(ext->min.x, hx ^ 0x80000000u);  // fneg
    set_bits(ext->min.y, hy ^ 0x80000000u);
    set_bits(ext->min.z, hz ^ 0x80000000u);
    const f64 eps = lf(KF(0));
    const Vec3& zero = global<Vec3>(kZeroVec);
    if (Near3(box->min, zero, eps) || Near3(box->max, zero, eps)) return;
    const f64 lx = Adds(lf(ext->min.x), lf(box->min.x)), ly = Adds(lf(ext->min.y), lf(box->min.y)),
              lz = Adds(lf(ext->min.z), lf(box->min.z));
    const f64 ux = Adds(lf(ext->max.x), lf(box->max.x)), uy = Adds(lf(ext->max.y), lf(box->max.y)),
              uz = Adds(lf(ext->max.z), lf(box->max.z));
    f32 rows[4][3];  // rotation rows and translation, copied before the calls
    for (u32 r = 0; r < 4; ++r) {
        const Vec4& v = r < 3 ? b->rot[r] : b->t;
        rows[r][0] = v.x.get();
        rows[r][1] = v.y.get();
        rows[r][2] = v.z.get();
    }
    const f64 half = lf(KF(4));
    GuestFrame frame(48);
    u8* he_p = frame.as<u8>(0);    // the half extents (hkVector4)
    u8* pos_p = frame.as<u8>(32);  // the new position (hkVector4)
    const V4 he{{static_cast<f32>(Muls(Subs(ux, lx), half)), static_cast<f32>(Muls(Subs(uy, ly), half)),
                 static_cast<f32>(Muls(Subs(uz, lz), half)), 0.0f}};
    Stvx(he_p, he);
    const u32 p = HkAlloc(48);
    u32 shape = 0;
    if (p != 0) {
        call(Hk_BoxShapeCtor899720, p, he_p, lf(global<be_f32>(kConvexR)));
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
    pos.e[3] = KF(64).get();
    Stvx(pos_p, pos);
    call(Body_Remove931F00, world, b);
    call(HkPhantom_SetPos929FDC, b, pos_p, lf(KF(8)));
    call(AnimSkel_Release7D4EA8, b->shape.ea.get());
    b->shape.ea = shape;
    call(Body_ShapeChanged85F1F0, &b->shape);
    call(Body_Add931D10, world, b);
}

}  // namespace sly4::obj

NATIVE(0044DED8, sly4::obj::Body_RefitBox_0044DED8)
