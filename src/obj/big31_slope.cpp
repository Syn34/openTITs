// Game code, large-function sweep part 31: the steepest ground slope under
// a point, from five Havok ray casts (the point and four neighbours).
#include "big31.h"

namespace sly4::obj {

// A PtrVector-managed array of 16-byte points (+6 the element size).
struct PointVec {
    gp<Vec4> data;   // +0
    be_u16 cap;      // +4
    be_u16 elem;     // +6
    be_u16 count;    // +8
    be_u16 mark;     // +10
};

namespace {
// .text: f32 2.0 (offset), 5.0 (ray length); lane masks y / z / w at +16 /
// +32 / +48; f32 1.0 at +64, f64 1.0 at +72, f32 -1.0 at +80, f64 -1.0 at
// +88, f32 57.29578 at +96
constexpr u32 kSlopeK = 0x000F0230;
constexpr u32 kOffsReady = 0x01032200;  // u8: kOffs built
constexpr u32 kOffs = 0x01032210;       // Vec4[5]: the ray offsets (copies of the global vectors below)
constexpr u32 kDirReady = 0x01032260;   // u8: kDir built
constexpr u32 kDir = 0x01032270;        // Vec3: the ray direction (a copy of 0x01164B90)
constexpr u32 kOffsSrc[5] = {0x01164B40, 0x01164B50, 0x01164B70, 0x01164B80, 0x01164BA0};
constexpr u32 kDirSrc = 0x01164B90;
constexpr u32 kVtRayColBase = 0x00B0EF40;  // the collector's base vtable (its state after use)
constexpr u32 PtrVector_Construct = 0x005516E0;    // (vec)
constexpr u32 PtrVector_Init = 0x00551598;         // (vec, u32 n, 0, 0)
constexpr u32 PtrVector_Clear = 0x0054EC1C;        // (vec)
constexpr u32 Nop = 0x0054EC18;                    // (vec)
constexpr u32 Acosf = 0x005E649C;                  // f1 -> f1

f64 KF(u32 off) { return lf(gm<be_f32>(kSlopeK + off)); }
f64 KD(u32 off) { return gm<be_f64>(kSlopeK + off).get(); }

// Three words from s to d (lfs / stfs pairs).
template <typename D, typename Src>
void Copy3(D& d, const Src& s) {
    set_bits(d.x, bits(s.x));
    set_bits(d.y, bits(s.y));
    set_bits(d.z, bits(s.z));
}

// The four fsel / frsp steps clamping x to [-1, 1] (as the original
// repeats them, NaN -> 1.0).
f64 Clamp1(f64 x, f64 one, f64 d_one, f64 m_one, f64 d_m_one) {
    x = S(Fsel(Subs(one, x), x, d_one));
    x = S(Fsel(Subs(x, m_one), x, d_m_one));
    x = S(Fsel(Adds(x, one), x, d_m_one));
    return S(Fsel(Subs(x, one), d_one, x));
}
}  // namespace

// void f(-, f32* out, const f32 p[3], const f32 up[3]) (called by
// Obj_SyncComps_000F0B48; r3 is not read). For each of the five offsets
// (built once: 0x01164B40 / B50 / B70 / B80 / BA0) a ray from
// p + 2 * offset along kDir (built once from 0x01164B90) * 5 is cast
// (an all-hits collector with eight inline hits) and the hits sorted.
// The first hit whose shape (+68) type (+28 & 0x1F) is 1..4 or 28 and that
// is not excluded is kept: a shape with byte +24 == 2 (an owned shape) is
// excluded, as is one with +24 == 1 whose kPropKind is kKindOwned and whose
// kPropOwner is non-zero. Its point (from + (to - from) * fraction) is
// pushed into a 16-byte PtrVector at sp + 832. A heap hit array is freed
// through kHkHeap->vfn[20] (entry in r6, OPD in r7). With all five points,
// for the four triangles (centre, q[j - 1], q[j]) (j = 1..4, q[0] taken
// as q[4]) the dot of the unit normal (-(a x b)) with up is computed and
// clamped to [-1, 1] (acosf called on each, the result unused); *out =
// acosf(the clamped minimum) * 57.29578 (degrees). Otherwise *out is left
// alone. The vector is then cleared (PtrVector_Clear, Nop).
void Obj_GroundSlope_000F0298(u32, be_f32* out, const Vec3* p, const Vec3* up) {
    GuestFrame frame(768);  // the original's sp+112..
    RayInput* ray = frame.as<RayInput>(0);
    RayCollector* coll = frame.as<RayCollector>(48);
    PointVec* vec = frame.as<PointVec>(720);
    ray->b32 = 0;
    ray->w36 = 0u;
    ray->w40 = 0u;
    Vec4* offs = host<Vec4>(kOffs);
    if (global<u8>(kOffsReady) == 0) {
        for (u32 k = 0; k < 5; ++k) Copy3(offs[k], gm<Vec3>(kOffsSrc[k]));
        global<u8>(kOffsReady) = 1;
    }
    call(PtrVector_Construct, vec);
    vec->elem = 16;
    call(PtrVector_Init, vec, 5u, 0u, 0u);
    Vec3& dir = global<Vec3>(kDir);
    u32 found = 0;
    for (u32 k = 0; k < 5; ++k) {
        const f64 two = KF(0);
        const Vec4& o = offs[k];
        f64 x = Fmadds(lf(o.x), two, lf(p->x)), y = Fmadds(lf(o.y), two, lf(p->y)), z = Fmadds(lf(o.z), two, lf(p->z));
        if (global<u8>(kDirReady) == 0) {
            Copy3(dir, gm<Vec3>(kDirSrc));
            global<u8>(kDirReady) = 1;
        }
        const f64 len = KF(4);
        const f64 ex = Fmadds(len, lf(dir.x), x), ey = Fmadds(len, lf(dir.y), y), ez = Fmadds(len, lf(dir.z), z);
        const Masks3 mk = LoadMasks3(kSlopeK + 16);
        Stvx(&ray->from, RowSel(Fill4(static_cast<f32>(x)), Fill4(static_cast<f32>(y)), Fill4(static_cast<f32>(z)), mk));
        Stvx(&ray->to, RowSel(Fill4(static_cast<f32>(ex)), Fill4(static_cast<f32>(ey)), Fill4(static_cast<f32>(ez)), mk));
        ray->w36 = 0u;
        const u32 world = PhysWorld();
        RayCollector_Init(coll);
        call(b31fn::HkWorld_CastRay92E510, world, ray, coll);
        call(b31fn::HkAllHits_Sort887E28, coll);
        const u32 hp = coll->hits;
        u32 cnt = vec->count;
        const s32 n = coll->count;
        const u32 cap = vec->cap;
        for (s32 i = 0; i < n; ++i) {
            const RayHit& hit = gm<RayHit>(hp + 80 * static_cast<u32>(i));
            const u32 sh = hit.shape;
            const HkCollidable& col = gm<HkCollidable>(sh);
            const u32 ty = col.type & 0x1F;
            if (!((ty >= 1 && ty <= 4) || ty == 28)) continue;
            const s8 kind = col.kind;
            const u32 rel = sh + static_cast<u32>(static_cast<s32>(col.owner_off));
            const u32 props = kind == 1 ? rel : 0;
            const u32 owned = kind == 2 ? rel : 0;
            if (props != 0 && HkPropLo(props, kPropKind) == kKindOwned) {
                const u32 owner = gm<HkProps>(props).count > 0 ? HkPropLo(props, kPropOwner) : 0;
                if (owner != 0) continue;
            }
            if (owned != 0) continue;
            const f64 t = lf(hit.fraction);
            x = Fmadds(Subs(ex, x), t, x);
            y = Fmadds(Subs(ey, y), t, y);
            z = Fmadds(Subs(ez, z), t, z);
            if (cnt == cap) {
                call(fn::PtrVector_Reserve, vec, (cap << 1) & 0x1FFFEu);
                cnt = vec->count;
            }
            Vec4& e = gm<Vec4>(vec->data.ea + ((cnt << 4) & 0xFFFF0u));
            e.x = static_cast<f32>(x);
            e.y = static_cast<f32>(y);
            e.z = static_cast<f32>(z);
            const u32 c1 = vec->count + 1u;
            const u32 mx = vec->mark;
            vec->count = static_cast<u16>(c1);
            vec->mark = static_cast<u16>(static_cast<s32>(mx) > static_cast<s32>(c1 & 0xFFFF) ? mx : c1);
            ++found;
            break;
        }
        const u32 cw = coll->capflags;
        coll->count = 0;
        if ((cw & 0x80000000u) == 0) {
            const u32 hpn = coll->hits;
            coll->vtable = kVtRayCol;
            vcall(host<u8>(kHkHeap), 20, hpn, static_cast<s32>(80u * (cw & 0x3FFFFFFFu)));
        }
        coll->hits = 0u;
        coll->capflags = 0x80000000u;
        coll->vtable = kVtRayColBase;
    }
    if (found == 5) {
        const f64 one = KF(64), d_one = KD(72), m_one = KF(80), d_m_one = KD(88);
        f64 mn = one;
        u32 prev = 4;
        for (u32 i = 1; i < 5; ++i) {
            const Vec4* pts = &gm<Vec4>(vec->data.ea);
            const Vec4 &d = pts[0], &a = pts[prev], &b = pts[i];
            f64 ay = Subs(lf(a.y), lf(d.y)), by = Subs(lf(b.y), lf(d.y));
            f64 ax = Subs(lf(a.x), lf(d.x)), bx = Subs(lf(b.x), lf(d.x));
            f64 az = Subs(lf(a.z), lf(d.z)), bz = Subs(lf(b.z), lf(d.z));
            f64 la = Muls(ay, ay), lb = Muls(by, by);
            la = Fmadds(ax, ax, la);
            lb = Fmadds(bx, bx, lb);
            la = Fmadds(az, az, la);
            lb = Fmadds(bz, bz, lb);
            const f64 ia = Divs(one, Sqrts(la)), ib = Divs(one, Sqrts(lb));
            az = Muls(az, ia);
            ax = Muls(ax, ia);
            ay = Muls(ay, ia);
            bx = Muls(bx, ib);
            bz = Muls(bz, ib);
            by = Muls(by, ib);
            const f64 t7 = Muls(az, bx), t8 = Muls(ay, bz), t12 = Muls(ax, by);
            const f64 ny = Fmsubs(ax, bz, t7), nx = Fmsubs(az, by, t8), nz = Fmsubs(ay, bx, t12);
            f64 dot = Muls(ny, lf(up->y));
            dot = Fmadds(nx, lf(up->x), dot);
            dot = Fmadds(nz, lf(up->z), dot);
            f64 c = S(Fsel(Subs(one, dot), dot, d_one));
            c = S(Fsel(Subs(c, m_one), c, d_m_one));
            mn = Fsel(Subs(dot, mn), mn, dot);
            c = S(Fsel(Adds(c, one), c, d_m_one));
            c = S(Fsel(Subs(c, one), d_one, c));
            call<f64>(Acosf, c);  // the result is not used
            prev = i;
        }
        const f64 r = call<f64>(Acosf, Clamp1(mn, one, d_one, m_one, d_m_one));
        *out = static_cast<f32>(Muls(r, KF(96)));
    }
    call(PtrVector_Clear, vec);
    call(Nop, vec);
}

}  // namespace sly4::obj

NATIVE(000F0298, sly4::obj::Obj_GroundSlope_000F0298)
