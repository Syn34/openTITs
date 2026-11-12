// Game code, large-function sweep part 24: the four-ray ground-normal probe.
#include "big24.h"
#include "native/vec.h"

namespace sly4::obj {

namespace {
constexpr u32 kQuatToMatrix = 0x00553A90;     // (Mat44* out, const Quat* q, const Vec3* pos)
constexpr u32 kCosf = 0x007344E0;             // f1 -> f1
constexpr u32 kSinf = 0x007345F0;             // f1 -> f1
constexpr u32 kHkFilterTest = 0x00884DC8;     // (hkBool* out, filter, u32 info, u32 info2) (Havok) (name?)
constexpr u32 kPhantomCastRay = 0x009250F0;   // (Phantom*, const RayIn*, RayOut*) (Havok) (name?)
constexpr u32 kWorldCastRay = 0x0092E2E8;     // HkWorld_CastRay92E2E8 (hkpWorld*, const RayIn*, RayOut*)
constexpr u32 kConsts = 0x0026DA40;           // f32 1, 2pi, 0.25, 0.5; masks {0,~0,0,0} {0,0,~0,0} {0,0,0,~0}; f32 pi/180
constexpr u32 kDefaultUp = 0x01164B60;        // Vec3: the normal when the object has no ground normal
constexpr u32 kProbeOn = 0x0105C7E8;          // u8: probing enabled
constexpr u32 kRayHeight = 0x0105C838;        // f32: ray length (from +h/2 above, down by h)
constexpr u32 kUsePhantom = 0x0105C2A0;       // word: cast through o->+48 when set
constexpr u32 kMaxSlopeDeg = 0x01057144;      // f32: steepest accepted slope (degrees)

// hkpWorldRayCastInput (48 bytes): from, to, u8 +32, +36 filter info, +40.
struct GrRayIn {
    Vec4 from;     // +0
    Vec4 to;       // +16
    u8 b32;        // +32
    pad<3> _33;
    be_u32 info;   // +36
    be_u32 w40;    // +40
    pad<4> _44;
};
static_assert(sizeof(GrRayIn) == 48);
// hkpWorldRayCastOutput (80 bytes): +0 the normal, +16 the hit fraction,
// +20 / +32 keys, +68 the collidable (its +28 the filter info).
struct GrRayOut {
    Vec3 normal;      // +0
    pad<4> _12;
    be_u32 fraction;  // +16
    be_u32 w20;       // +20
    pad<8> _24;
    be_u32 w32;       // +32
    pad<28> _36;
    be_u32 w64;       // +64
    be_u32 coll;      // +68
    pad<8> _72;
};
static_assert(sizeof(GrRayOut) == 80 && offsetof(GrRayOut, coll) == 68);
// The original's locals (sp offsets).
struct GrLocals {
    Vec4 tri[3];        // sp+128: the three hit points
    Mtx44 mtx;          // sp+176
    Vec4 pts[4];        // sp+240: hit points
    Vec4 nrms[4];       // sp+304: hit normals
    GrRayIn rin[4];     // sp+368
    GrRayOut rout[4];   // sp+560
    u8 hkb;             // sp+880: hkBool
};
static_assert(offsetof(GrLocals, mtx) == 48 && offsetof(GrLocals, rin) == 240 && offsetof(GrLocals, rout) == 432);
static_assert(offsetof(GrLocals, hkb) == 752);
// The probing object: +48 its phantom, +272 bit 0x80000000 has a ground
// normal (+304), +428 the filter info, +480 the position, +496 the
// rotation quaternion.
struct GrObj {
    pad<48> _0;
    be_u32 phantom;     // +48
    pad<220> _52;
    be_u32 flags;       // +272
    pad<28> _276;
    Vec3 normal;        // +304
    pad<112> _316;
    be_u32 info;        // +428
    pad<48> _432;
    Vec4 pos;           // +480
    Vec4 quat;          // +496
};
static_assert(offsetof(GrObj, flags) == 272 && offsetof(GrObj, normal) == 304 && offsetof(GrObj, info) == 428);
static_assert(offsetof(GrObj, pos) == 480 && offsetof(GrObj, quat) == 496);

f64 C(u32 off) { return lf(global<be_f32>(kConsts + off)); }
// Normalize (x, y, z) as the original does (sum y*y, x*x, z*z; 1 / fsqrts)
// and store it to out.
void StoreUnit(Vec3* out, f64 x, f64 y, f64 z) {
    f64 l = S(y * y);
    l = Fmadds(x, x, l);
    l = Fmadds(z, z, l);
    const f64 inv = S(C(0) / S(std::sqrt(l)));
    out->x = static_cast<f32>(S(x * inv));
    out->y = static_cast<f32>(S(y * inv));
    out->z = static_cast<f32>(S(z * inv));
}

// The normal of the triangle (p0, p1, p2): (p2 - p1) x (p1 - p0), components
// rounded as fmuls + fmsubs.
void StoreTriNormal(Vec3* out, const Vec4& p0, const Vec4& p1, const Vec4& p2) {
    const f64 bx = S(lf(p2.x) - lf(p1.x)), by = S(lf(p2.y) - lf(p1.y)), bz = S(lf(p2.z) - lf(p1.z));
    const f64 ax = S(lf(p1.x) - lf(p0.x)), ay = S(lf(p1.y) - lf(p0.y)), az = S(lf(p1.z) - lf(p0.z));
    const f64 ny = Fmsubs(bz, ax, S(bx * az));
    const f64 nx = Fmsubs(by, az, S(bz * ay));
    const f64 nz = Fmsubs(bx, ay, S(by * ax));
    StoreUnit(out, nx, ny, nz);
}
}  // namespace

// u32 f(Obj* o, Vec3* n; f32 radius, taken as the double f1 holds) (called by 001233C0, 00131798,
// 00148130; the first two read r3): the ground normal under o from four
// downward rays.
// n = o->+304 when bit 0x80000000 of o->+272 is set, else kDefaultUp; with
// probing off (u8 kProbeOn) that's all (r3 0). Otherwise o's matrix comes
// from Quat_ToMatrix(o->+496 quat, o->+480 pos); ray k (0..3) starts at
// M * (r cos(k pi/2), h/2, r sin(k pi/2)) and ends h down along M's row 1
// (h = kRayHeight); the Havok inputs (48 bytes at sp+368, filter info
// o->+428) and outputs (80 bytes at sp+560, hit fraction 1, keys -1) are
// cast with the phantom o->+48 (0x009250F0) when it and kUsePhantom are
// set, else with the game's hkpWorld. A ray counts when it hit a collidable
// that passes the filter test (0x00884DC8 on world->+116, o->+428,
// collidable->+28) and its normal's y >= cos(kMaxSlopeDeg); its hit point
// and normal are kept and bit k set. Two hits (masks 3/5/6/9/10/12): n =
// the normalized sum of their normals onto kZeroVec (n.x / n.y first get
// kZeroVec's, a quirk); three hits: the normal of the triangle of the hit
// points in ray order; four: (p1 - p3) x (p0 - p2). Returns the hit mask.
u32 Ground_ProbeNormal_0026DA88(GrObj* o, Vec3* n, f64 radius) {
    GuestFrame frame(1168);
    GrLocals& L = *frame.as<GrLocals>(128);
    if ((o->flags & 0x80000000u) != 0) CopyXYZ(*n, o->normal);
    else CopyXYZ(*n, global<Vec3>(kDefaultUp));
    if (global<u8>(kProbeOn) == 0) return 0;
    call(kQuatToMatrix, &L.mtx, &o->quat, &o->pos);
    const u32 info = o->info;
    const u32 world = gm<be_u32>(gm<be_u32>(global<be_u32>(kWorld) + 0x1281D4u) + 4);
    const u32 filter = gm<be_u32>(world + 116);
    for (u32 k = 0; k < 4; ++k) {
        GrRayIn& ri = L.rin[k];
        GrRayOut& ro = L.rout[k];
        ri.b32 = 0;
        ri.info = 0u;
        ri.w40 = 0u;
        ro.fraction = 0x3F800000u;
        ro.w20 = 0xFFFFFFFFu;
        ro.w64 = 0u;
        ro.w32 = 0xFFFFFFFFu;
        ro.coll = 0u;
    }
    const f64 neg_h = -lf(global<be_f32>(kRayHeight));
    const f64 dx = S(lf(L.mtx.r[1].x) * neg_h), dy = S(lf(L.mtx.r[1].y) * neg_h), dz = S(lf(L.mtx.r[1].z) * neg_h);
    const V4 r0 = Lvx(&L.mtx.r[0]), r1 = Lvx(&L.mtx.r[1]), r2 = Lvx(&L.mtx.r[2]), r3 = Lvx(&L.mtx.r[3]);
    for (u32 k = 0; k < 4; ++k) {
        const f64 ang = S(S(S(static_cast<f64>(static_cast<s32>(k))) * C(4)) * C(8));
        const f64 cx = S(call<f64>(kCosf, ang) * radius);
        const f64 cy = S(lf(global<be_f32>(kRayHeight)) * C(12));
        const f64 cz = S(call<f64>(kSinf, ang) * radius);
        const V4 p = Add4(r3, Row3(r0, r1, r2, static_cast<f32>(cx), static_cast<f32>(cy), static_cast<f32>(cz)));
        GrRayIn& ri = L.rin[k];
        ri.info = info;
        set_bits(ri.from.x, LaneW(p, 0));
        set_bits(ri.from.y, LaneW(p, 1));
        set_bits(ri.from.z, LaneW(p, 2));
        set_bits(ri.from.w, 0u);
        ri.to.x = static_cast<f32>(S(static_cast<f64>(p.e[0]) + dx));
        ri.to.y = static_cast<f32>(S(static_cast<f64>(p.e[1]) + dy));
        ri.to.z = static_cast<f32>(S(static_cast<f64>(p.e[2]) + dz));
        set_bits(ri.to.w, 0u);
    }
    if (o->phantom != 0u && global<be_u32>(kUsePhantom) != 0u) {
        for (u32 k = 0; k < 4; ++k) call(kPhantomCastRay, o->phantom.get(), &L.rin[k], &L.rout[k]);
    } else {
        for (u32 k = 0; k < 4; ++k) call(kWorldCastRay, world, &L.rin[k], &L.rout[k]);
    }
    const f64 min_ny = call<f64>(kCosf, S(lf(global<be_f32>(kMaxSlopeDeg)) * C(64)));
    u32 mask = 0;
    for (u32 k = 0; k < 4; ++k) {
        const GrRayIn& ri = L.rin[k];
        const GrRayOut& ro = L.rout[k];
        const f64 fx = lf(ri.from.x), fy = lf(ri.from.y), fz = lf(ri.from.z);
        const u32 coll = ro.coll;
        if (coll == 0) continue;
        call(kHkFilterTest, &L.hkb, filter, info, gm<be_u32>(coll + 28).get());
        if (L.hkb == 0) continue;
        if (!(lf(ro.normal.y) >= min_ny)) continue;
        const f64 t = LfsW(ro.fraction);
        L.pts[k].x = static_cast<f32>(Fmadds(t, dx, fx));
        L.pts[k].y = static_cast<f32>(Fmadds(t, dy, fy));
        L.pts[k].z = static_cast<f32>(Fmadds(t, dz, fz));
        mask |= 1u << k;
        CopyXYZ(L.nrms[k], ro.normal);
    }
    switch (mask) {
    case 3: case 5: case 6: case 9: case 10: case 12: {
        const Vec3& z0 = global<Vec3>(kZeroVec);
        set_bits(n->x, bits(z0.x));  // quirk: x and y written before the sum
        set_bits(n->y, bits(z0.y));
        f64 x = lf(z0.x), y = lf(z0.y), z = lf(z0.z);
        for (u32 k = 0; k < 4; ++k) {
            if ((mask & (1u << k)) == 0) continue;
            x = Adds(x, lf(L.nrms[k].x));
            y = Adds(y, lf(L.nrms[k].y));
            z = Adds(z, lf(L.nrms[k].z));
        }
        StoreUnit(n, x, y, z);
        break;
    }
    case 7: case 11: case 13: case 14: {
        u32 d = 0;
        for (u32 k = 0; k < 4; ++k) {
            if ((mask & (1u << k)) == 0) continue;
            CopyXYZ(L.tri[d], L.pts[k]);
            ++d;
        }
        StoreTriNormal(n, L.tri[0], L.tri[1], L.tri[2]);
        break;
    }
    case 15: {
        const Vec4* p = L.pts;
        const f64 bx = S(lf(p[1].x) - lf(p[3].x)), by = S(lf(p[1].y) - lf(p[3].y)), bz = S(lf(p[1].z) - lf(p[3].z));
        const f64 ax = S(lf(p[0].x) - lf(p[2].x)), ay = S(lf(p[0].y) - lf(p[2].y)), az = S(lf(p[0].z) - lf(p[2].z));
        const f64 ny = Fmsubs(bz, ax, S(bx * az));
        const f64 nx = Fmsubs(by, az, S(bz * ay));
        const f64 nz = Fmsubs(bx, ay, S(by * ax));
        StoreUnit(n, nx, ny, nz);
        break;
    }
    default:
        break;
    }
    return mask;
}

}  // namespace sly4::obj

NATIVE(0026DA88, sly4::obj::Ground_ProbeNormal_0026DA88)
