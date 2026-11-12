// Game code, large-function sweep part 29: the follow camera's update
// (blend tracks, the eye from the target's offset, a ray cast pulling the
// eye in front of blocking geometry, then the look-at matrix).
#include "big29.h"
#include "big31.h"

namespace sly4::obj {

namespace {
constexpr u32 kPoolCam = 0x00304F90;   // .text: +0 1.0, +16 / +32 / +48 the y / z / w lane masks
constexpr u32 kMinEyeY = 0x0106A268;   // f32: the eye's lowest y
constexpr u32 kIdentity = 0x01164BD0;  // the global 4x4 identity
constexpr u32 kUp = 0x01164B60;        // the global up vector
constexpr u32 kPropNoCamBlock = 0x91C7CF5B;  // shape property: set -> the hit does not block the camera
constexpr u32 Obj_Blend = 0x004A0E60;        // (Track*; f1 dt)
constexpr u32 Matrix_NormalizeInPlace = 0x0056D374;  // (f32 m[4][4])

// The follow target: +128 a direction, +144 a base point, +432 the position.
struct FollowTarget {
    pad<128> _0;
    Vec3 dir;     // +128
    pad<4> _140;
    Vec3 base;    // +144
    pad<276> _156;
    Vec3 pos;     // +432
};
static_assert(offsetof(FollowTarget, base) == 144 && offsetof(FollowTarget, pos) == 432);
// The camera object: +16 its transform (4x4 rows, w lanes untouched).
struct FollowCamObj {
    pad<16> _0;
    Mtx44 m;  // +16
};
// The follow camera: +0 the camera object, +4 the target, four blend
// tracks, the eye offsets.
struct FollowCam {
    gp<FollowCamObj> cam;     // +0
    gp<FollowTarget> target;  // +4
    pad<40> _8;
    pad<28> track0;           // +48
    be_f32 back;              // +76
    pad<4> _80;
    pad<28> track1;           // +84
    be_f32 yoff;              // +112
    pad<4> _116;
    pad<28> track2;           // +120
    be_f32 eye_y;             // +148
    pad<4> _152;
    pad<28> track3;           // +156
    be_f32 dist;              // +184
};
static_assert(offsetof(FollowCam, back) == 76 && offsetof(FollowCam, yoff) == 112);
static_assert(offsetof(FollowCam, eye_y) == 148 && offsetof(FollowCam, dist) == 184);
}  // namespace

// void f(Cam* o; f32 dt) (called by Obj_AttachCam_003055E8, f_0030F798,
// f_003179B8). Nothing without o->+0 (the camera object). The tracks
// +48 / +84 / +120 / +156 are blended (Obj_Blend, dt). With the target t
// = o->+4 (+432 its position p, +128 a direction a, +144 a base point b):
// the eye e = b + o->+184 * a / |a| (y + o->+148); the ray runs from {p.x,
// p.y + o->+112, p.z} to that start minus o->+76 along the unit vector to
// e (RayInput at sp + 112: from +0, to +16, +32 = 1, +36 = 16, +40 = 0;
// collector at sp + 224 with eight inline 80-byte hits at sp + 256). The
// first hit whose shape (hit +68) has type 1 (+28 & 0x1F) and, for a
// shape with byte +24 == 1, no non-zero kPropNoCamBlock property in the
// list at shape + s8 +16 (+124 the 16-byte entries, +128 the count), puts
// the start at p + (start - p) * its fraction (+16) when that is not 1.
// The start's y is raised to kMinEyeY; the camera's 4x3 transform +16..
// = the look-at matrix from the start towards e (rows from the up vector
// 0x01164B60, Matrix_NormalizeInPlace) with the start as translation. A
// heap hit array is freed through kHkHeap->vfn[20]. No caller reads r3
// or f1 (r3.sh).
void Cam_UpdateFollow_00304FD0(FollowCam* o, f32 dt_) {
    const f64 dt = dt_;
    if (o->cam.ea == 0u) return;
    GuestFrame frame(1008);  // the original's frame (offsets below are its sp offsets)
    RayInput* in = frame.as<RayInput>(112);
    Mtx44* m = frame.as<Mtx44>(160);
    RayCollector* col = frame.as<RayCollector>(224);
    const u32 pea = o->target.ea + 432;
    const Vec3& p = gm<Vec3>(pea);
    for (const u32 k : {48u, 84u, 120u, 156u}) call(Obj_Blend, guest(o) + k, dt);
    const FollowTarget& t = gm<FollowTarget>(o->target.ea);
    const Vec3 &a = t.dir, &b = t.base;
    const f64 ay = lf(a.y), ax = lf(a.x), az = lf(a.z);
    const f64 one = lf(global<be_f32>(kPoolCam));
    const f64 alen = Sqrts(Fmadds(az, az, Fmadds(ax, ax, Muls(ay, ay))));
    f64 sy = Adds(lf(p.y), lf(o->yoff));
    const f64 dist = lf(o->dist), back = lf(o->back);
    const Masks3 mk = LoadMasks3(kPoolCam + 16);
    // RayInput: from = {p.x, p.y + o->+112, p.z, 0}.
    Stvx(&in->from, RowSel(SplatW(&p.x), Fill4(static_cast<f32>(sy)), SplatW(&p.z), mk));
    const f64 inv = Divs(one, alen);
    in->b32 = 1;
    in->w36 = 16u;
    in->w40 = 0u;
    const f64 ey = Adds(Fmadds(dist, Muls(inv, ay), lf(b.y)), lf(o->eye_y));
    const f64 ex = Fmadds(dist, Muls(inv, ax), lf(b.x));
    const f64 ez = Fmadds(dist, Muls(inv, az), lf(b.z));
    f64 sx = lf(p.x), sz = lf(p.z);
    {
        f64 dx = Subs(ex, sx), dz = Subs(ez, sz), dy = Subs(ey, sy);
        const f64 il = Divs(one, Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)))));
        dx = Muls(dx, il), dy = Muls(dy, il), dz = Muls(dz, il);
        sx = Fnmsubs(dx, back, sx);
        sy = Fnmsubs(dy, back, sy);
        sz = Fnmsubs(dz, back, sz);
    }
    Stvx(&in->to, RowSel(Fill4(static_cast<f32>(sx)), Fill4(static_cast<f32>(sy)), Fill4(static_cast<f32>(sz)), mk));
    RayCollector_Init(col);
    call(b31fn::HkWorld_CastRay92E510, PhysWorld(), in, col);
    // The first blocking hit's fraction (1 without one).
    f64 frac = one;
    {
        u32 hit = col->hits;
        const s32 n = col->count;
        for (s32 i = 0; i < n; ++i, hit += 80) {
            const RayHit& h = gm<RayHit>(hit);
            const HkCollidable& sh = gm<HkCollidable>(h.shape);
            const bool type1 = (sh.type & 0x1F) == 1;
            if (sh.kind == 1) {
                const u32 props = h.shape + static_cast<u32>(static_cast<s32>(sh.owner_off));
                if (props != 0 && HkPropLo(props, kPropNoCamBlock) != 0) continue;  // the low word is tested
            }
            if (!type1) continue;
            frac = lf(h.fraction);
            break;
        }
    }
    if (frac != one) {
        const f64 px = lf(p.x), py = lf(p.y), pz = lf(p.z);
        sx = Fmadds(Subs(sx, px), frac, px);
        sy = Fmadds(Subs(sy, py), frac, py);
        sz = Fmadds(Subs(sz, pz), frac, pz);
    }
    if (const f64 lo = lf(global<be_f32>(kMinEyeY)); sy < lo) sy = lo;
    // The look-at matrix (rows 0..2, translation row 3; w lanes untouched).
    f64 dy = Subs(ey, sy), dx = Subs(ex, sx), dz = Subs(ez, sz);
    const f64 len = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy))));
    const Mtx44& id = global<Mtx44>(kIdentity);
    for (u32 r = 0; r < 4; ++r) {
        set_bits(m->r[r].x, bits(id.r[r].x));
        set_bits(m->r[r].y, bits(id.r[r].y));
        set_bits(m->r[r].z, bits(id.r[r].z));
    }
    const f64 il = Divs(one, len);
    dx = Muls(dx, il), dy = Muls(dy, il), dz = Muls(dz, il);
    m->r[2].x = static_cast<f32>(dx), m->r[2].y = static_cast<f32>(dy), m->r[2].z = static_cast<f32>(dz);
    const Vec3& up = global<Vec3>(kUp);
    const f64 r0x = Fmsubs(lf(up.y), dz, Muls(lf(up.z), dy));
    const f64 r0y = Fmsubs(lf(up.z), dx, Muls(lf(up.x), dz));
    const f64 r0z = Fmsubs(lf(up.x), dy, Muls(lf(up.y), dx));
    const f64 r1x = Fmsubs(dy, r0z, Muls(dz, r0y));
    const f64 r1y = Fmsubs(dz, r0x, Muls(dx, r0z));
    const f64 r1z = Fmsubs(dx, r0y, Muls(dy, r0x));
    m->r[0].x = static_cast<f32>(r0x), m->r[0].y = static_cast<f32>(r0y), m->r[0].z = static_cast<f32>(r0z);
    m->r[1].x = static_cast<f32>(r1x), m->r[1].y = static_cast<f32>(r1y), m->r[1].z = static_cast<f32>(r1z);
    call(Matrix_NormalizeInPlace, m);
    m->r[3].x = static_cast<f32>(sx), m->r[3].y = static_cast<f32>(sy), m->r[3].z = static_cast<f32>(sz);
    Mtx44& cm = gm<FollowCamObj>(o->cam.ea).m;
    for (u32 r = 0; r < 4; ++r) {
        set_bits(cm.r[r].x, bits(m->r[r].x));
        set_bits(cm.r[r].y, bits(m->r[r].y));
        set_bits(cm.r[r].z, bits(m->r[r].z));
    }
    const u32 cap = col->capflags, hits = col->hits;
    RayCollector_Free(col, hits, cap);
}

}  // namespace sly4::obj

NATIVE(00304FD0, sly4::obj::Cam_UpdateFollow_00304FD0)
