// Game code, large-function sweep part 32: the point of a sound area
// (sphere or box) nearest to a listener, with its weight and spread.
#include "big32.h"

namespace sly4::obj {

struct SoundArea {
    pad<96> _0;
    Vec4 rows[3];       // +96: the rotation rows
    Vec4 t;             // +144: the translation row
    pad<268> _160;
    be_s32 shape;       // +428: 0 sphere, 3 box
    pad<56> _432;
    be_f32 radius;      // +488
    be_f32 outer;       // +492
    Vec4 bmin;          // +496: the box (Box_ContainsPoint's argument)
    Vec4 bmax;          // +512
    pad<32> _528;
    Vec4 up;            // +560: per-axis falloff above the box (> 0 to use)
    Vec4 dn;            // +576: per-axis falloff below the box (< 0 to use)
    pad<16> _592;
    be_f32 falloff;     // +608
};
static_assert(offsetof(SoundArea, shape) == 428 && offsetof(SoundArea, radius) == 488 && offsetof(SoundArea, bmin) == 496);
static_assert(offsetof(SoundArea, up) == 560 && offsetof(SoundArea, falloff) == 608);

namespace {
// .text constants: f32 1.0, f32 360.0, f64 0.0 at +8 (its high word f32
// 0.0), f64 1.0 at +64, f32 4.0 at +72, f32 57.29578 at +76.
constexpr u32 kAreaK = 0x00210D30;
constexpr u32 Box_ContainsPoint5573D0 = 0x005573D0;  // bool (const Box* b, const f32 p[3]) (lanes x / y / z)
constexpr u32 Atanf5E67D4 = 0x005E67D4;              // atanf: f1 -> f1

const be_f32& KF(u32 off) { return gm<be_f32>(kAreaK + off); }
f64 KD(u32 off) { return gm<be_f64>(kAreaK + off).get(); }

const be_f32& Lane(const Vec4& v, u32 k) { return (&v.x)[k]; }

// One lane of the local point: R^T * p + R^T * (0 - t) as the two vmaddfp
// chains the original builds from the transposed rows, summed with vaddfp.
f32 LocalLane(const SoundArea* o, u32 k, f32 px, f32 py, f32 pz) {
    const Vec4& r = o->rows[k];
    const f32 c0 = r.x.get(), c1 = r.y.get(), c2 = r.z.get();
    const f32 nx = 0.0f - o->t.x.get(), ny = 0.0f - o->t.y.get(), nz = 0.0f - o->t.z.get();
    f32 a = std::fma(c0, px, 0.0f);
    a = std::fma(c1, py, a);
    a = std::fma(c2, pz, a);
    f32 b = std::fma(c0, nx, 0.0f);
    b = std::fma(c1, ny, b);
    b = std::fma(c2, nz, b);
    return b + a;
}

// The local point (x, y, z) back to world space: the rows scaled by the
// lanes, plus the translation row.
V4 ToWorld(const SoundArea* o, f32 x, f32 y, f32 z) {
    return Add4(Lvx(&o->t), Row3(Lvx(&o->rows[0]), Lvx(&o->rows[1]), Lvx(&o->rows[2]), x, y, z));
}
// The first three lanes to v as raw words.
void Store3(Vec3* v, const V4& w) {
    set_bits(v->x, LaneW(w, 0));
    set_bits(v->y, LaneW(w, 1));
    set_bits(v->z, LaneW(w, 2));
}

// fsel / fsubs clamp of x (f64 of an f32) to [mn, mx]: x <= mx ? x : mx, then
// that >= mn ? it : mn.
f64 ClampBox(f64 x, f64 mn, f64 mx) {
    x = Fsel(Subs(mx, x), x, mx);
    return Fsel(Subs(x, mn), x, mn);
}
}  // namespace

// f32 f(Area* a, f32 p[3] (in / out), f32* weight, f32* spread) (called by
// f_002112C8 and Area_TouchSound_002114FC). p is taken to the area's local
// space (R^T * (p - t)) and *weight = 1.0, *spread = 360.0 are stored. By
// the shape:
//  - 3 (box): when Box_ContainsPoint(&a->bmin, local), falloff = 0 and p =
//    the local point back in world space; otherwise the falloff f = max
//    over x / y / z of (local - max) / up[k] (when up[k] > 0 and local >
//    max) or (local - min) / dn[k] (when dn[k] < 0 and local < min), 0 at
//    least; p = the local point clamped to the box, in world space;
//    *spread = (1 - f)^3 * 360; falloff = f.
//  - 0 (sphere): when |local| > radius, *spread = atanf(r / d) * 4 *
//    57.29578 and falloff = (d - r) / (outer - r) (else 0); p = the centre.
//  - other: p is left alone.
// *weight is then re-read, clamped to [0, 1] (NaN -> 1), stored and
// returned.
f32 Area_EmitPoint_00210D80(SoundArea* o, Vec3* pos, be_f32* wt, be_f32* spr) {
    GuestFrame frame(144);
    Vec3* loc = frame.as<Vec3>();  // the local point (lane w, built from an unset stack word, is never read)
    const f32 px = pos->x.get(), py = pos->y.get(), pz = pos->z.get();
    loc->x = LocalLane(o, 0, px, py, pz);
    loc->y = LocalLane(o, 1, px, py, pz);
    loc->z = LocalLane(o, 2, px, py, pz);
    set_bits(*wt, 0x3F800000u);   // 1.0
    set_bits(*spr, 0x43B40000u);  // 360.0
    const f64 one = lf(KF(0)), deg = lf(KF(4));
    const s32 shape = o->shape;
    if (shape == 3) {
        if (call<u32>(Box_ContainsPoint5573D0, &o->bmin, loc) != 0) {
            o->falloff = 0.0f;
            Store3(pos, ToWorld(o, loc->x.get(), loc->y.get(), loc->z.get()));
        } else {
            const f64 zero = lf(KF(8));
            const be_f32* l = &loc->x;
            f64 f = zero;
            for (u32 k = 0; k < 3; ++k) {
                f64 r = zero;
                bool hit = false;
                const f64 up = lf(Lane(o->up, k));
                if (up > r) {
                    const f64 v = lf(l[k]), mx = lf(Lane(o->bmax, k));
                    if (v > mx) {
                        r = Divs(Subs(v, mx), up);
                        hit = true;
                    }
                }
                if (!hit) {
                    const f64 dn = lf(Lane(o->dn, k));
                    if (dn < zero) {
                        const f64 v = lf(l[k]), mn = lf(Lane(o->bmin, k));
                        if (v < mn) r = Divs(Subs(v, mn), dn);
                    }
                }
                if (r > f) f = r;
            }
            const f64 x = ClampBox(lf(loc->x), lf(o->bmin.x), lf(o->bmax.x));
            const f64 y = ClampBox(lf(loc->y), lf(o->bmin.y), lf(o->bmax.y));
            const f64 z = ClampBox(lf(loc->z), lf(o->bmin.z), lf(o->bmax.z));
            loc->x = static_cast<f32>(x);
            loc->y = static_cast<f32>(y);
            loc->z = static_cast<f32>(z);
            Store3(pos, ToWorld(o, static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z)));
            const f64 g = Subs(one, f);
            f64 s = Muls(g, g);
            s = Muls(s, g);
            s = Muls(s, g);
            *spr = static_cast<f32>(Muls(s, deg));
            o->falloff = static_cast<f32>(f);
        }
    } else if (shape == 0) {
        const f64 lx = lf(loc->x), ly = lf(loc->y), lz = lf(loc->z);
        f64 d2 = Muls(ly, ly);
        d2 = Fmadds(lx, lx, d2);
        d2 = Fmadds(lz, lz, d2);
        const f64 r = lf(o->radius);
        f64 t = lf(KF(8));
        const f64 d = Sqrts(d2);
        if (!(d < r || d == r)) {
            const f64 q = Divs(r, d);
            t = Divs(Subs(d, r), Subs(lf(o->outer), r));
            f64 a = call<f64>(Atanf5E67D4, q);
            a = Muls(a, lf(KF(72)));
            *spr = static_cast<f32>(Muls(a, lf(KF(76))));
        }
        set_bits(pos->x, bits(o->t.x));  // lfs / stfs: raw words
        set_bits(pos->y, bits(o->t.y));
        set_bits(pos->z, bits(o->t.z));
        o->falloff = static_cast<f32>(t);
    }
    const f64 w0 = lf(*wt);
    f64 w = S(Fsel(Subs(one, w0), w0, KD(64)));
    w = S(Fsel(w, w, KD(8)));
    *wt = static_cast<f32>(w);
    return static_cast<f32>(w);
}

}  // namespace sly4::obj

NATIVE(00210D80, sly4::obj::Area_EmitPoint_00210D80)
