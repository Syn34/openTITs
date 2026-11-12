// Game code, large-function sweep part 27: dropping a ground marker (pushed
// clear of the existing ones, snapped to the ground by a ray cast).
#include "big27.h"
#include "big31.h"

namespace sly4::obj {

namespace {
// .text pool: f32 5.0, 1e-5 (+4), 1/65535 (+8), 2.0 (+12), -1.0 (+16),
// 1.0 (+20), 25.0 (+24), 0.0 (+28), -50.0 (+32); masks {y} (+48), {z}
// (+64), {w} (+80).
constexpr u32 kPool30A = 0x0030A8A0;
constexpr u32 kSepRef = 0x01164AFC;     // f32[2]: the offset treated as "on top of" a marker (x, z)
constexpr u32 kMarkerId = 0x0106A3FC;   // u32 running marker id
constexpr u32 kMarkerTab = 0x01069F00;  // +72 f32 passed as the marker's y
constexpr u32 UpVec = 0x01164B60;
constexpr u32 HkWorld_CastRay = 0x0092E2E8;         // (hkpWorld*, const hkpWorldRayCastInput*, hkpWorldRayCastOutput*)
constexpr u32 Marker_Init_003087C0 = 0x003087C0;    // (Marker*, const f32 pos[3]) (name?)
constexpr u32 Marker_SetState_003081C8 = 0x003081C8;  // (Marker*, u32 state) (name?)

// A 144-byte ground marker.
struct Marker {
    be_u32 w0;        // +0
    pad<12> _4;
    Vec3 up;          // +16
    pad<4> _28;
    be_f32 x;         // +32
    be_u32 half;      // +36 (raw 0.5)
    be_f32 z;         // +40
    pad<4> _44;
    be_u32 src[6];    // +48: the source's +32.. (+48 the radius)
    pad<8> _72;
    be_u32 rec;       // +80
    be_u32 w84;       // +84
    be_u32 w88[4];    // +88 .. +100
    pad<8> _104;
    be_u32 pos[3];    // +112
    pad<4> _124;
    u8 placed;        // +128
    pad<3> _129;
    be_u32 cam;       // +132
    u8 alt;           // +136
    pad<3> _137;
    be_u32 id;        // +140
};
static_assert(sizeof(Marker) == 144 && offsetof(Marker, rec) == 80 && offsetof(Marker, pos) == 112 && offsetof(Marker, id) == 140);
// The marker source: +32 the radius and five more words.
struct MarkerSrc {
    pad<32> _0;
    be_u32 w[6];  // +32
};
struct MarkerOwner {
    be_u32 w0;              // +0
    pad<16> _4;
    be_f32 y;               // +20
    pad<28> _24;
    PtrVec<Marker> list;    // +52
    pad<112> _64;
    be_u32 rec[2];          // +176 / +180
    be_u32 w184;            // +184
};
static_assert(offsetof(MarkerOwner, list) == 52 && offsetof(MarkerOwner, rec) == 176);
}  // namespace

// void f(T* o, const Src* src, const f32 pos[3], u8 separate, u8 alt)
// (called by 0030AFF0, 0030B6D8). (x, z) = pos's; with separate, up to
// five passes push it out of every marker on o's list (+52 data, u16 +60
// count; marker +32 / +40 its x / z, +48 its radius) closer than that
// radius + src->+32 (a direction exactly on top of one is replaced by a
// random unit one from the global generator). A ray from (x, o->+20 + 25,
// z) 50 down (world Game->+0x1281D4->+4, filter 13): nothing more without
// a hit or for a hit whose height is beyond +-1. Else a
// 144-byte marker (SmallAlloc): +0 o->+0, +16 the up vector * 5 (+ the
// zero vector), +32 / +40 the hit x / z, +36 0.5, +48.. src+32..+52, +80
// o->+176 (+180 with alt), +84 o->+184, +112.. the zero vector, +136 alt,
// +140 the next marker id; Marker_Init_003087C0(m, {x, kMarkerTab+72,
// z}), Marker_SetState_003081C8(m, 3). Then (even without the marker,
// quirk: the writes go to 0 + offset) +112.. = pos, u8 +128 = 1, +132 =
// the camera's (Game->+4216 when its +192 bit 3, else 0, quirk) vfn[100]()
// value. The marker is pushed on o's list (u16 +56 capacity, +62 high
// mark). No caller reads r3 or f1 (r3.sh).
void Obj_DropMarker_0030A900(MarkerOwner* o, const MarkerSrc* src, const Vec3* pos, u32 sep_, u32 alt_) {
    const u32 sep = sep_ & 0xFF, alt = alt_ & 0xFF;
    const u32 K = kPool30A;
    auto Kf = [](u32 ea) { return lf(global<be_f32>(ea)); };
    const Vec3 &up = global<Vec3>(UpVec), &zv = global<Vec3>(kZeroVec);
    const f64 k0 = Kf(K);
    const f64 f31 = Fmadds(lf(up.x), k0, lf(zv.x));
    const f64 f30 = Fmadds(lf(up.y), k0, lf(zv.y));
    const f64 f29 = Fmadds(lf(up.z), k0, lf(zv.z));
    f64 x = lf(pos->x), z = lf(pos->z);
    const u32 px = bits(pos->x), py = bits(pos->y), pz = bits(pos->z);
    GuestFrame frame(448);
    auto len2 = [](f64 a, f64 b) { return Sqrts(Fmadds(a, a, Muls(b, b))); };

    // Separation from the existing markers.
    if (sep != 0) {
        const u32 n = o->list.count, data = o->list.data.ea;
        for (u32 it = 0; it < 5; ++it) {
            const f64 x0 = x, z0 = z;
            for (u32 j = 0; j < n; ++j) {
                const Marker& e = gm<Marker>(gm<be_u32>(data + 4 * j));
                const f64 c0 = Kf(kSepRef), eps = Kf(K + 4);
                f64 dx = Subs(x, lf(e.x));
                const f64 er = LfsW(e.src[0]);
                f64 dz = Subs(z, lf(e.z));
                f64 d;
                if (std::fabs(Subs(dx, c0)) < eps && std::fabs(Subs(dz, Kf(kSepRef + 4))) < eps) {
                    const f64 c1 = Kf(kSepRef + 4);
                    const f64 sc = Kf(K + 8), k2 = Kf(K + 12), k3 = Kf(K + 16), k1 = Kf(K + 20);
                    dx = c0, dz = c1;
                    while (std::fabs(Subs(dx, c0)) < eps && std::fabs(Subs(dz, c1)) < eps) {
                        // A random direction: the last output (u64) and one new 16-bit value.
                        be_u32& state = global<be_u32>(kGameRandState);
                        be_u64& lastv = global<be_u64>(kGameRandLast);
                        const u64 last = lastv;
                        const u32 x1 = GameRandMix(state);
                        f64 a = GameRandF(last), b = GameRandF(x1 & 0xFFFF);
                        const u32 x2 = GameRandMix(std::rotl(x1, 16));
                        a = Muls(a, sc), b = Muls(b, sc);
                        state = std::rotl(x2, 16);
                        a = Fmadds(a, k2, k3), b = Fmadds(b, k2, k3);
                        lastv = static_cast<u64>(x2 & 0xFFFF);
                        const f64 t = Divs(k1, len2(a, b));
                        dx = Muls(a, t), dz = Muls(b, t);
                    }
                    const f64 t = Divs(k1, len2(dx, dz));
                    dz = Muls(dz, t), dx = Muls(dx, t);
                    d = len2(dx, dz);
                } else {
                    d = len2(dx, dz);
                    const f64 t = Divs(Kf(K + 20), d);
                    dx = Muls(dx, t), dz = Muls(dz, t);
                }
                const f64 r = Adds(LfsW(src->w[0]), er);
                if (d < r) {
                    const f64 pen = Subs(r, d);
                    x = Fmadds(pen, dx, x);
                    z = Fmadds(pen, dz, z);
                }
            }
            if (std::fabs(Subs(x, x0)) < Kf(K + 4) && std::fabs(Subs(z, z0)) < Kf(K + 4)) break;
        }
    }

    // The ray cast.
    const f64 y0 = Adds(lf(o->y), Kf(K + 24));
    const f64 f24 = Kf(K + 28), f25 = Kf(K + 32), lim = Kf(K + 20);
    RayInput& in = *frame.as<RayInput>(128);
    RayHit& out = *frame.as<RayHit>(176);
    in.b32 = 0;
    in.w40 = 0u;
    set_bits(out.fraction, 0x3F800000u);  // hit fraction 1.0
    out.w20 = 0xFFFFFFFFu;
    out.w64 = 0u;
    out.w32 = 0xFFFFFFFFu;
    out.shape = 0u;
    const u32 world = PhysWorld();
    const Masks3 mk = LoadMasks3(K + 48);
    auto row = [&](f64 a, f64 b, f64 c) { return RowSel(Fill4(static_cast<f32>(a)), Fill4(static_cast<f32>(b)), Fill4(static_cast<f32>(c)), mk); };
    const f64 tx = Adds(x, f24), ty = Adds(y0, f25), tz = Adds(z, f24);
    Stvx(&in.from, row(x, y0, z));
    in.w36 = 13u;  // collision filter info
    Stvx(&in.to, row(tx, ty, tz));
    call(HkWorld_CastRay, world, &in, &out);
    if (out.shape == 0u) return;
    const f64 frac = lf(out.fraction);
    const f64 hy = Fmadds(frac, f25, y0), hx = Fmadds(frac, f24, x), hz = Fmadds(frac, f24, z);
    if (std::fabs(hy) > lim) return;

    // The marker.
    u32 sw[6];
    for (u32 k = 0; k < 6; ++k) sw[k] = src->w[k];
    const u32 rec = o->rec[alt != 0 ? 1 : 0];
    const u32 pa = call<u32>(fn::SmallAlloc, 144u);
    u32 m = 0;
    if (pa != 0) {
        Marker& p = gm<Marker>(pa);
        const u32 w0 = o->w0, w184 = o->w184;
        p.w0 = w0;
        p.up.x = static_cast<f32>(f31);
        p.up.y = static_cast<f32>(f30);
        p.up.z = static_cast<f32>(f29);
        p.half = 0x3F000000u;  // 0.5
        p.x = static_cast<f32>(hx);
        p.z = static_cast<f32>(hz);
        for (u32 k = 0; k < 6; ++k) p.src[k] = sw[k];
        p.rec = rec;
        p.w84 = w184;
        for (be_u32& w : p.w88) w = 0u;
        p.pos[0] = bits(zv.x);
        p.pos[1] = bits(zv.y);
        p.placed = 0;
        p.pos[2] = bits(zv.z);
        p.cam = 0u;
        p.alt = static_cast<u8>(alt);
        be_u32& id = global<be_u32>(kMarkerId);
        const u32 idv = id;
        id = idv + 1;
        p.id = idv;
        Vec3* at = frame.as<Vec3>(112);
        at->x = static_cast<f32>(hx);
        set_bits(at->y, bits(global<be_f32>(kMarkerTab + 72)));
        set_bits(at->z, bits(p.z));
        call(Marker_Init_003087C0, pa, at);
        call(Marker_SetState_003081C8, pa, 3u);
        m = pa;
    }
    // Written even when the allocation failed (m = 0; quirk).
    Marker& mm = gm<Marker>(m);
    mm.pos[0] = px;
    mm.pos[1] = py;
    mm.pos[2] = pz;
    mm.placed = 1;
    const u32 g5 = gm<be_u32>(global<be_u32>(kWorld) + 4216);
    const u32 cam = (g5 != 0 && (gm<be_u32>(g5 + 192) & 8) != 0) ? g5 : 0u;
    const u32 v = vcall<u32>(&gm<u8>(cam), 100);  // through 0's "vtable" without one (quirk)
    mm.cam = v;
    if (m == 0) return;
    u32 n = o->list.count;
    if (const u32 cap = o->list.cap; n == cap) {
        call(fn::PtrVector_Reserve, &o->list, (cap << 1) & 0x1FFFEu);
        n = o->list.count;
    }
    gm<be_u32>(o->list.data.ea + ((n << 2) & 0x3FFFCu)) = m;
    u32 n1 = o->list.count + 1u;
    const u32 hi = o->list.mark;
    o->list.count = static_cast<u16>(n1);
    if (static_cast<s32>(hi) > static_cast<s32>(n1 & 0xFFFF)) n1 = hi;
    o->list.mark = static_cast<u16>(n1);
}

}  // namespace sly4::obj

NATIVE(0030A900, sly4::obj::Obj_DropMarker_0030A900)
