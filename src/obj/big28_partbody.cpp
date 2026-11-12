// Game code, large-function sweep part 28: a part's rigid body moved toward a
// target point (optionally stopped short by a ray cast) and turned to face
// along its displacement.
#include "big28.h"
#include "native/vec.h"

namespace sly4::obj {

namespace {
constexpr u32 kPool103 = 0x00103A70;  // .text pool: masks {y} / {z} / {w}, then the f32 constants below
constexpr f32 kZero = 0.0f;           // pool +0 (lane 0 of the {y} mask)
constexpr f32 kMinMove = 0.01f;       // pool +48
constexpr f32 kOne = 1.0f;            // pool +52
constexpr f32 kMinDot = 0.001f;       // pool +56
constexpr f32 kMinLen = 1e-4f;        // pool +60
constexpr f32 kMinHitSq = 1e-5f;      // pool +64
constexpr f32 kMinGap = 0.1f;         // pool +68 (and as a double at +72)
constexpr u32 AxisZ = 0x01164B70;     // the global vector after UpVec ({0, 0, 1})

// A 48-byte ray hit: the point first.
struct HitPt48 {
    Vec3 p;  // +0
    pad<36> _12;
};
static_assert(sizeof(HitPt48) == 48);
// The ray hit collector the world's vfn[64] fills (eight inline hits).
struct HitCol48 {
    be_u32 vtable;     // +0
    be_u32 early_out;  // +4 (raw)
    pad<8> _8;
    be_u32 data;       // +16: HitPt48*
    be_s32 size;       // +20
    be_u32 capflags;   // +24: capacity | 0x80000000 (inline storage)
    pad<4> _28;
    HitPt48 inline_hits[8];  // +32
};
static_assert(sizeof(HitCol48) == 416);
// The body owner: +288 its position.
struct PartOwner {
    pad<288> _0;
    Vec3 pos;  // +288
};
// The rigid body: +8 its world, +224 its motion (vfn[68] / vfn[72] set the
// linear / angular velocity).
struct PartBody {
    pad<8> _0;
    be_u32 world;  // +8
    pad<212> _12;
    pad<4> motion; // +224
};
static_assert(offsetof(PartBody, motion) == 224);
struct PartObj {
    pad<16> _0;
    be_u32 w16;          // +16
    gp<PartOwner> own;   // +20
    be_u32 body;         // +24: PartBody*
    pad<12> _28;
    be_f32 maxd;         // +40
    pad<228> _44;
    be_u32 world;        // +272: the collision world (vfn[64] casts)
    pad<124> _276;
    Vec3 dir;            // +400: the facing when barely moving
    pad<288> _412;
    be_f32 gap;          // +700
};
static_assert(offsetof(PartObj, world) == 272 && offsetof(PartObj, dir) == 400 && offsetof(PartObj, gap) == 700);
// The original's locals (sp offsets - 112).
struct PartLocals {
    Vec4 from;     // sp+112
    Vec4 vec;      // sp+128
    Vec4 rot;      // sp+144
    Vec4 to;       // sp+160
    Vec4 zero;     // sp+176
    HitCol48 col;  // sp+192
};
static_assert(offsetof(PartLocals, col) == 80);
}  // namespace

// void f(T* o, const f32 target[3], u8 cast) (called by f_00104508,
// Part_SyncBody_00105360). Needs o->+16, the body owner o->+20 and the body
// b = o->+24. p is
// the owner's +288.. position; the target is pulled back to at most o->+40
// from p. With cast: a ray from p to the target through the world o->+272
// (vfn[64](world, &from, &to, collector, 0); collector on the stack with
// vtable VtHitCol, early-out 0x7F7FFFEE and an inline array of 8 48-byte
// hits {+16 data, +20 size, +24 capacity 0x80000008}); with hits they are
// sorted and the target becomes p + dir * max(dist - o->+700, 0.1) toward
// the first (dir normalised only when the squared distance is above 1e-5);
// a heap array (flag 0x80000000 clear) is freed through HkHeap->vfn[20].
// Then d = p - target (o->+400.. when |d| < 0.01) and the rotation taking
// AxisZ to d (shortest arc; a perpendicular half-turn axis, or identity,
// when the two are opposite). Without b->+8 returns.
// Else HkBody_SetPosition_0091DFF0(b, {0, 0, |d|, 0}), HkBody_SetPosRot(b,
// target, rotation) and, for b re-read each time, Hk_Activate(b) and b+224's
// vfn[68] / vfn[72] with the zero vector (linear / angular velocity). No caller reads r3 or f1 (r3.sh).
void Part_MoveBodyToward_00103AC0(PartObj* o, const Vec3* tp, u32 cast_) {
    const bool cast = (cast_ & 0xFF) != 0;
    if (o->w16 == 0u || o->own.ea == 0u || o->body == 0u) return;
    GuestFrame frame(800);
    PartLocals& L = *frame.as<PartLocals>(112);
    const Masks3 mk = LoadMasks3(kPool103);
    const Vec3& P = o->own->pos;
    const f64 px = lf(P.x), py = lf(P.y), pz = lf(P.z);
    f64 tx = lf(tp->x), ty = lf(tp->y), tz = lf(tp->z);
    {
        const f64 dx = Subs(tx, px), dy = Subs(ty, py), dz = Subs(tz, pz);
        const f64 len = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy))));
        const f64 maxd = lf(o->maxd);
        if (len > maxd) {
            const f64 s = Divs(kOne, len);
            tx = Fmadds(maxd, Muls(s, dx), px);
            ty = Fmadds(maxd, Muls(s, dy), py);
            tz = Fmadds(maxd, Muls(s, dz), pz);
        }
    }

    // The ray cast.
    if (cast) {
        const u32 world = o->world;
        HitCol48& col = L.col;
        col.vtable = b28fn::VtHitCol;
        col.data = guest(&col.inline_hits[0]);
        col.capflags = 0x80000008u;
        col.size = 0;
        col.early_out = 0x7F7FFFEEu;
        Stvx(&L.zero, Zero4());
        Stvx(&L.from, RowSelW(&P.x, &P.y, &P.z, mk));
        Stvx(&L.to, RowSel(Fill4(static_cast<f32>(tx)), Fill4(static_cast<f32>(ty)), Fill4(static_cast<f32>(tz)), mk));
        vcall(host<u8>(world), 64, &L.from, &L.to, &col, 0u);
        if (col.size > 0) {
            call(b28fn::HkHits_Sort, &col);
            const HitPt48& h = gm<HitPt48>(col.data);
            f64 hx = Subs(lf(h.p.x), px), hy = Subs(lf(h.p.y), py), hz = Subs(lf(h.p.z), pz);
            const f64 gap = lf(o->gap);
            const f64 sq = Fmadds(hz, hz, Fmadds(hx, hx, Muls(hy, hy)));
            const f64 dist = Sqrts(sq);
            if (sq > kMinHitSq) {
                const f64 s = Divs(kOne, dist);
                hx = Muls(s, hx);
                hy = Muls(s, hy);
                hz = Muls(s, hz);
            }
            const f64 d = Subs(dist, gap);
            const f64 k = S(Subs(d, kMinGap) >= 0.0 ? d : static_cast<f64>(kMinGap));  // fsel, frsp
            tx = Fmadds(k, hx, px);
            ty = Fmadds(k, hy, py);
            tz = Fmadds(k, hz, pz);
        }
        col.size = 0;
        const u32 cap = col.capflags;
        if ((cap & 0x80000000u) == 0) {
            col.vtable = b28fn::VtHitCol;
            const u32 bytes = (cap & 0x3FFFFFFFu) * 48u;
            vcall(host<u8>(b28fn::HkHeap), 20, col.data.get(), static_cast<s32>(bytes));
        }
        col.data = 0u;
        col.capflags = 0x80000000u;
        col.vtable = b28fn::VtHitColEnd;
    }

    // The facing rotation.
    f64 dx = Subs(px, tx), dy = Subs(py, ty), dz = Subs(pz, tz);
    const f64 len = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy))));
    if (len < kMinMove) dx = lf(o->dir.x), dy = lf(o->dir.y), dz = lf(o->dir.z);
    const Vec3& az3 = global<Vec3>(AxisZ);
    const f64 ax0 = lf(az3.x), ay0 = lf(az3.y), az0 = lf(az3.z);
    const f64 ia = Divs(kOne, Sqrts(Fmadds(az0, az0, Fmadds(ax0, ax0, Muls(ay0, ay0)))));
    const f64 id = Divs(kOne, Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)))));
    const f64 ay = Muls(ia, ay0), ax = Muls(ia, ax0), az = Muls(ia, az0);
    const f64 ny = Muls(id, dy), nx = Muls(id, dx), nz = Muls(id, dz);
    const f64 hy = Adds(ay, ny), hx = Adds(ax, nx), hz = Adds(az, nz);
    const f64 dot = Fmadds(hz, az, Fmadds(hx, ax, Muls(hy, ay)));
    f64 qx, qy, qz, qw;
    if (std::fabs(dot) > kMinDot) {
        qy = Fmsubs(az, nx, Muls(ax, nz));
        qx = Fmsubs(ay, nz, Muls(az, ny));
        qz = Fmsubs(ax, ny, Muls(ay, nx));
        const f64 r = Divs(kOne, Sqrts(Fmadds(dot, dot, Fmadds(qz, qz, Fmadds(qx, qx, Muls(qy, qy))))));
        qx = Muls(qx, r);
        qy = Muls(qy, r);
        qz = Muls(qz, r);
        qw = Muls(dot, r);
    } else if (ax > ay) {
        const f64 n = Sqrts(Fmadds(ax, ax, Muls(az, az)));
        if (n < kMinLen) {
            qx = qy = qz = kZero, qw = kOne;
        } else {
            const f64 s = Divs(kOne, n);
            qx = -Muls(s, az), qy = kZero, qz = Muls(s, ax), qw = kZero;
        }
    } else {
        const f64 n = Sqrts(Fmadds(ay, ay, Muls(az, az)));
        if (n < kMinLen) {
            qx = qy = qz = kZero, qw = kOne;
        } else {
            const f64 s = Divs(kOne, n);
            qx = kZero, qy = Muls(s, az), qz = -Muls(s, ay), qw = kZero;
        }
    }

    const u32 b = o->body;
    if (gm<PartBody>(b).world == 0u) return;
    auto F = [](f64 x) { return Fill4(static_cast<f32>(x)); };
    Stvx(&L.vec, Andc(Sel(Zero4(), F(len), mk.m[1]), mk.m[2]));
    call(b28fn::HkBody_SetPosition_0091DFF0, b, &L.vec);
    Stvx(&L.vec, RowSel(F(tx), F(ty), F(tz), mk));
    Stvx(&L.rot, RowSel4(F(qx), F(qy), F(qz), F(qw), mk));
    call(b28fn::HkBody_SetPosRot, o->body.get(), &L.vec, &L.rot);
    const Vec3& zv = global<Vec3>(kZeroVec);
    for (u32 pass = 0; pass < 2; ++pass) {
        const u32 bb = o->body;
        Stvx(&L.vec, RowSelW(&zv.x, &zv.y, &zv.z, mk));
        call(b28fn::Hk_Activate, bb);
        vcall(&gm<PartBody>(bb).motion, pass == 0 ? 68 : 72, &L.vec);  // linear / angular velocity
    }
}

}  // namespace sly4::obj

NATIVE(00103AC0, sly4::obj::Part_MoveBodyToward_00103AC0)
