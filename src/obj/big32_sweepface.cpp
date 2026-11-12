// Game code, large-function sweep part 32: move an object's matrix to a
// target point with a Havok sweep, then turn its forward axis toward the
// distance actually moved.
#include "big32.h"

namespace sly4::obj {

struct Mat44 {
    Vec4 r[4];  // +0, +16, +32, +48 (rows 0..2, translation)
};
struct SweepDef {
    pad<372> _0;
    be_u32 flags;  // +372: 0x400 notify on a hit, 0x400000 turn toward the move
};
struct SweepHolder {
    pad<8> _0;
    be_u32 w8;  // +8
};
struct SweepObj {
    be_u32 vtable;           // +0 (vt 352: the hit notification (o, f32, 0, 0))
    pad<212> _4;
    gp<SweepDef> def;        // +216
    pad<124> _220;
    be_f32 hit_arg;          // +344
    pad<40> _348;
    gp<SweepHolder> hk;      // +388
};
static_assert(offsetof(SweepObj, def) == 216 && offsetof(SweepObj, hit_arg) == 344 && offsetof(SweepObj, hk) == 388);

namespace {
constexpr u32 kSweepK = 0x000EB400;  // .text: f32 1e-5, 1.0, 1000.0; f64 1000.0 at +16, f64 1.0 at +24
constexpr u32 kTurnDiv = 0x01021CD0;  // f32: the turn divisor (clamped to [1, 1000])
constexpr u32 kAxisX = 0x01164B50;    // Vec3: the fallback axes
constexpr u32 kAxisY = 0x01164B60;
constexpr u32 kAxisZ = 0x01164B70;
constexpr u32 Hk_SweepToPoint_004A41C0 = 0x004A41C0;  // bool (Holder*, const Mat44* m, const Vec3* p, Mat44* out)
constexpr u32 Matrix_NormalizeInPlace56D374 = 0x0056D374;  // (Mat34*)

f64 KF(u32 off) { return lf(gm<be_f32>(kSweepK + off)); }
f64 KD(u32 off) { return gm<be_f64>(kSweepK + off).get(); }

// v's x / y / z to d as raw words (lfs / stfs pairs).
void Copy3(Vec4& d, const Vec4& s) {
    set_bits(d.x, bits(s.x));
    set_bits(d.y, bits(s.y));
    set_bits(d.z, bits(s.z));
}
void Copy3(Vec4& d, const Vec3& s) {
    set_bits(d.x, bits(s.x));
    set_bits(d.y, bits(s.y));
    set_bits(d.z, bits(s.z));
}

// Each of (x, y, z) within eps of the zero vector (fsubs, fabs, fcmpu lt;
// the zero vector re-read each time).
bool NearZero(f64 x, f64 y, f64 z, f64 eps) {
    const Vec3& zv = global<Vec3>(kZeroVec);
    return std::fabs(Subs(x, lf(zv.x))) < eps && std::fabs(Subs(y, lf(zv.y))) < eps && std::fabs(Subs(z, lf(zv.z))) < eps;
}
}  // namespace

// void f(Obj* o, const Mat44* m, const Vec3* p, Mat44* out, u8 turn)
// (called by f_000F45E0 and f_000F5518). out = m (x / y / z of the four
// rows) with translation p. With the Havok holder (and its +8) set:
// Hk_SweepToPoint_004A41C0(hk, m, p, out); on a hit with flag 0x400 of
// o->def, o->vfn[352](o, o->hit_arg, 0, 0). Then, with flag 0x400000 and
// turn set and the translation moved (out.t - m.t not within 1e-5 of m.t
// nor of zero): f = normalize(m.row2 + normalize(d) / clamp(kTurnDiv, 1,
// 1000)) (out.row2 when that is ~zero), row0 = out.row1 x f (or, when
// ~zero, the least-aligned of kAxisX / Y / Z x f), row1 = f x row0; the
// matrix on the stack (translation out.t) goes through
// Matrix_NormalizeInPlace and back to out.
void Obj_SweepTurnToMove_000EB420(SweepObj* o, const Mat44* m, const Vec3* p, Mat44* out, u8 turn) {
    for (u32 r = 0; r < 4; ++r) Copy3(out->r[r], m->r[r]);
    Copy3(out->r[3], *p);
    SweepHolder* hk = o->hk;
    const u32 def = o->def.ea;
    if (hk == nullptr || hk->w8 == 0) return;
    if (call<u32>(Hk_SweepToPoint_004A41C0, hk, m, p, out) != 0 && (gm<SweepDef>(def).flags & 0x400u) != 0)
        call_ptr(vfn(o, 352), o, lf(o->hit_arg), 0u, 0u);
    if ((gm<SweepDef>(def).flags & 0x400000u) == 0 || turn == 0) return;
    const f64 eps = KF(0);
    const Vec4& mt = m->r[3];
    const Vec4& ot = out->r[3];
    const f64 ox = lf(ot.x), mx = lf(mt.x);
    if (std::fabs(Subs(mx, ox)) < eps && std::fabs(Subs(lf(mt.y), lf(ot.y))) < eps && std::fabs(Subs(lf(mt.z), lf(ot.z))) < eps)
        return;
    const f64 dx = Subs(ox, mx), dy = Subs(lf(ot.y), lf(mt.y)), dz = Subs(lf(ot.z), lf(mt.z));
    if (NearZero(dx, dy, dz, eps)) return;
    const f64 len = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy))));
    const f64 g = lf(global<be_f32>(kTurnDiv)), one = KF(4);
    f64 div = S(Fsel(Subs(KF(8), g), g, KD(16)));
    div = S(Fsel(Subs(div, one), div, KD(24)));
    const f64 idiv = Divs(one, div), il = Divs(one, len);
    f64 fy = Fmadds(Muls(dy, il), idiv, lf(m->r[2].y));
    f64 fx = Fmadds(Muls(dx, il), idiv, lf(m->r[2].x));
    f64 fz = Fmadds(Muls(dz, il), idiv, lf(m->r[2].z));
    GuestFrame frame(64);
    Mat44& t = *frame.as<Mat44>();  // the original's sp+112
    Copy3(t.r[0], out->r[0]);
    Copy3(t.r[1], out->r[1]);
    Copy3(t.r[3], out->r[3]);
    const f64 r1x = lf(out->r[1].x), r1y = lf(out->r[1].y), r1z = lf(out->r[1].z);
    const f64 il2 = Divs(one, Sqrts(Fmadds(fz, fz, Fmadds(fx, fx, Muls(fy, fy)))));
    fx = Muls(fx, il2);
    fy = Muls(fy, il2);
    fz = Muls(fz, il2);
    t.r[2].x = static_cast<f32>(fx);
    t.r[2].y = static_cast<f32>(fy);
    t.r[2].z = static_cast<f32>(fz);
    if (NearZero(fx, fy, fz, eps)) {
        Copy3(t.r[2], out->r[2]);
        fx = lf(t.r[2].x);
        fy = lf(t.r[2].y);
        fz = lf(t.r[2].z);
    }
    f64 c0 = Fmsubs(r1y, fz, Muls(r1z, fy)), c1 = Fmsubs(r1z, fx, Muls(r1x, fz)), c2 = Fmsubs(r1x, fy, Muls(r1y, fx));
    if (NearZero(c0, c1, c2, eps)) {
        u32 ax = kAxisX;
        const f64 az = std::fabs(fz), ay = std::fabs(fy);
        if (az < ay && az < std::fabs(fx)) ax = kAxisZ;
        else if (ay < std::fabs(fx)) ax = kAxisY;
        const Vec3& v = gm<Vec3>(ax);
        const f64 vx = lf(v.x), vy = lf(v.y), vz = lf(v.z);
        c0 = Fmsubs(vy, fz, Muls(vz, fy));
        c1 = Fmsubs(vz, fx, Muls(vx, fz));
        c2 = Fmsubs(vx, fy, Muls(vy, fx));
    }
    t.r[0].x = static_cast<f32>(c0);
    t.r[0].y = static_cast<f32>(c1);
    t.r[0].z = static_cast<f32>(c2);
    t.r[1].x = static_cast<f32>(Fmsubs(fy, c2, Muls(fz, c1)));
    t.r[1].y = static_cast<f32>(Fmsubs(fz, c0, Muls(fx, c2)));
    t.r[1].z = static_cast<f32>(Fmsubs(fx, c1, Muls(fy, c0)));
    call(Matrix_NormalizeInPlace56D374, &t);
    for (u32 r = 0; r < 4; ++r) Copy3(out->r[r], t.r[r]);
}

}  // namespace sly4::obj

NATIVE(000EB420, sly4::obj::Obj_SweepTurnToMove_000EB420)
