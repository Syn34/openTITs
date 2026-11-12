// Game code, large-function sweep part 27: dropping a ground marker (pushed
// clear of the existing ones, snapped to the ground by a ray cast).
#include "../sly4.h"
#include "big27.h"

using namespace sly4;
using namespace sly4::big27;

namespace {
// .text pool: f32 5.0, 1e-5 (+4), 1/65535 (+8), 2.0 (+12), -1.0 (+16),
// 1.0 (+20), 25.0 (+24), 0.0 (+28), -50.0 (+32); masks {y} (+48), {z}
// (+64), {w} (+80).
constexpr u32 kPool30A = 0x0030A8A0;
constexpr u32 kSepRef = 0x01164AFC;     // f32[2]: the offset treated as "on top of" a marker (x, z)
constexpr u32 kMarkerId = 0x0106A3FC;   // u32 running marker id
constexpr u32 kMarkerTab = 0x01069F00;  // +72 f32 passed as the marker's y
}  // namespace

// u32 f(T* o, const Src* src, const f32 pos[3], u8 separate, u8 alt)
// (called by 0030AFF0, 0030B6D8). (x, z) = pos's; with separate, up to
// five passes push it out of every marker on o's list (+52 data, u16 +60
// count; marker +32 / +40 its x / z, +48 its radius) closer than that
// radius + src->+32 (a direction exactly on top of one is replaced by a
// random unit one from the global generator). A ray from (x, o->+20 + 25,
// z) 50 down (world Game->+0x1281D4->+4, filter 13): no hit returns 0; a
// hit whose height is beyond +-1 returns the hit collidable. Else a
// 144-byte marker (SmallAlloc): +0 o->+0, +16 the up vector * 5 (+ the
// zero vector), +32 / +40 the hit x / z, +36 0.5, +48.. src+32..+52, +80
// o->+176 (+180 with alt), +84 o->+184, +112.. the zero vector, +136 alt,
// +140 the next marker id; Marker_Init_003087C0(m, {x, kMarkerTab+72,
// z}), Marker_SetState_003081C8(m, 3). Then (even without the marker,
// quirk: the writes go to 0 + offset) +112.. = pos, u8 +128 = 1, +132 =
// the camera's (Game->+4216 when its +192 bit 3, else 0, quirk) vfn[100]()
// value. The marker is pushed on o's list (u16 +56 capacity, +62 high
// mark), whose high mark is returned (r3 the vfn[100] value without the
// marker).
DECOMP(0030A900, Obj_DropMarker_0030A900) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1), pos = arg<u32>(ctx, 2);
    const u32 sep = arg<u32>(ctx, 3) & 0xFF, alt = arg<u32>(ctx, 4) & 0xFF;
    const u32 K = kPool30A;
    const f64 k0 = Lf(K);
    const f64 f31 = Fmadds(Lf(b27fn::UpVec), k0, Lf(b27fn::ZeroVec));
    const f64 f30 = Fmadds(Lf(b27fn::UpVec + 4), k0, Lf(b27fn::ZeroVec + 4));
    const f64 f29 = Fmadds(Lf(b27fn::UpVec + 8), k0, Lf(b27fn::ZeroVec + 8));
    f64 x = Lf(pos), z = Lf(pos + 8);
    const u32 px = load<u32>(pos), py = load<u32>(pos + 4), pz = load<u32>(pos + 8);
    Stack frame(ctx, 448);
    const u32 sp = frame.addr();
    auto len2 = [](f64 a, f64 b) { return Sqrts(Fmadds(a, a, Muls(b, b))); };

    // Separation from the existing markers.
    if (sep != 0) {
        const u32 n = load<u16>(o + 60), data = load<u32>(o + 52);
        for (u32 it = 0; it < 5; ++it) {
            const f64 x0 = x, z0 = z;
            for (u32 j = 0; j < n; ++j) {
                const u32 e = load<u32>(data + 4 * j);
                const f64 c0 = Lf(kSepRef), eps = Lf(K + 4);
                f64 dx = Subs(x, Lf(e + 32));
                const f64 er = Lf(e + 48);
                f64 dz = Subs(z, Lf(e + 40));
                f64 d;
                if (std::fabs(Subs(dx, c0)) < eps && std::fabs(Subs(dz, Lf(kSepRef + 4))) < eps) {
                    const f64 c1 = Lf(kSepRef + 4);
                    const f64 sc = Lf(K + 8), k2 = Lf(K + 12), k3 = Lf(K + 16), k1 = Lf(K + 20);
                    dx = c0, dz = c1;
                    while (std::fabs(Subs(dx, c0)) < eps && std::fabs(Subs(dz, c1)) < eps) {
                        // A random direction: the last output (u64) and one new 16-bit value.
                        const u64 last = load<u64>(b27fn::Rng);
                        const u32 x1 = RngMix27(load<u32>(b27fn::Rng + 8));
                        f64 a = S(static_cast<f64>(static_cast<s64>(last))), b = S(static_cast<f64>(x1 & 0xFFFF));
                        const u32 x2 = RngMix27(std::rotl(x1, 16));
                        a = Muls(a, sc), b = Muls(b, sc);
                        store<u32>(b27fn::Rng + 8, std::rotl(x2, 16));
                        a = Fmadds(a, k2, k3), b = Fmadds(b, k2, k3);
                        store<u64>(b27fn::Rng, x2 & 0xFFFF);
                        const f64 t = Divs(k1, len2(a, b));
                        dx = Muls(a, t), dz = Muls(b, t);
                    }
                    const f64 t = Divs(k1, len2(dx, dz));
                    dz = Muls(dz, t), dx = Muls(dx, t);
                    d = len2(dx, dz);
                } else {
                    d = len2(dx, dz);
                    const f64 t = Divs(Lf(K + 20), d);
                    dx = Muls(dx, t), dz = Muls(dz, t);
                }
                const f64 r = Adds(Lf(src + 32), er);
                if (d < r) {
                    const f64 pen = Subs(r, d);
                    x = Fmadds(pen, dx, x);
                    z = Fmadds(pen, dz, z);
                }
            }
            if (std::fabs(Subs(x, x0)) < Lf(K + 4) && std::fabs(Subs(z, z0)) < Lf(K + 4)) break;
        }
    }

    // The ray cast.
    const f64 y0 = Adds(Lf(o + 20), Lf(K + 24));
    const f64 f24 = Lf(K + 28), f25 = Lf(K + 32), lim = Lf(K + 20);
    const u32 in = sp + 128, out = sp + 176;
    store<u8>(in + 32, 0);
    store<u32>(in + 36, 0);
    store<u32>(in + 40, 0);
    store<u32>(out + 16, 0x3F800000u);  // hit fraction 1.0
    store<u32>(out + 20, 0xFFFFFFFFu);
    store<u32>(out + 64, 0);
    store<u32>(out + 32, 0xFFFFFFFFu);
    store<u32>(out + 68, 0);
    const u32 world = load<u32>(load<u32>(load<u32>(b27fn::Game) + b27fn::GamePhys) + 4);
    const Masks3 mk = LoadMasks3(K + 48);
    auto row = [&](f64 a, f64 b, f64 c) {
        return Andc(Sel(Sel(Fill4(static_cast<f32>(a)), Fill4(static_cast<f32>(b)), mk.m[0]), Fill4(static_cast<f32>(c)), mk.m[1]), mk.m[2]);
    };
    const f64 tx = Adds(x, f24), ty = Adds(y0, f25), tz = Adds(z, f24);
    Stvx(in, row(x, y0, z));
    store<u32>(in + 36, 13);  // collision filter info
    Stvx(in + 16, row(tx, ty, tz));
    ctx.fpr[1] = tx;  // leftover f1
    call(ctx, b27fn::HkWorld_CastRay, {world, in, out});
    const u32 hit = load<u32>(out + 68);
    if (hit == 0) {
        ret(ctx, 0u);
        return;
    }
    const f64 frac = Lf(out + 16);
    const f64 hy = Fmadds(frac, f25, y0), hx = Fmadds(frac, f24, x), hz = Fmadds(frac, f24, z);
    ctx.fpr[1] = std::fabs(hy);
    if (std::fabs(hy) > lim) {
        ret(ctx, hit);
        return;
    }

    // The marker.
    u32 sw[6];
    for (u32 k = 0; k < 6; ++k) sw[k] = load<u32>(src + 32 + 4 * k);
    const u32 rec = load<u32>(o + (alt != 0 ? 180 : 176));
    const u32 p = static_cast<u32>(call(ctx, b27fn::SmallAlloc, {144}));
    u32 m = 0;
    if (p != 0) {
        const u32 w0 = load<u32>(o + 0), w184 = load<u32>(o + 184);
        store<u32>(p + 0, w0);
        Sf(p + 16, f31);
        Sf(p + 20, f30);
        Sf(p + 24, f29);
        store<u32>(p + 36, 0x3F000000u);  // 0.5
        Sf(p + 32, hx);
        Sf(p + 40, hz);
        for (u32 k = 0; k < 6; ++k) store<u32>(p + 48 + 4 * k, sw[k]);
        store<u32>(p + 80, rec);
        store<u32>(p + 84, w184);
        for (u32 k = 88; k <= 100; k += 4) store<u32>(p + k, 0);
        store<u32>(p + 112, load<u32>(b27fn::ZeroVec));
        store<u32>(p + 116, load<u32>(b27fn::ZeroVec + 4));
        store<u8>(p + 128, 0);
        store<u32>(p + 120, load<u32>(b27fn::ZeroVec + 8));
        store<u32>(p + 132, 0);
        store<u8>(p + 136, static_cast<u8>(alt));
        const u32 id = load<u32>(kMarkerId);
        store<u32>(kMarkerId, id + 1);
        store<u32>(p + 140, id);
        Sf(sp + 112, hx);
        store<u32>(sp + 116, load<u32>(p + 36));
        store<u32>(sp + 120, load<u32>(p + 40));
        store<u32>(sp + 116, load<u32>(kMarkerTab + 72));
        SetF1(ctx, p + 40);  // leftover f1
        call(ctx, b27fn::Marker_Init_003087C0, {p, sp + 112});
        call(ctx, b27fn::Marker_SetState_003081C8, {p, 3});
        m = p;
    }
    // Written even when the allocation failed (m = 0; quirk).
    store<u32>(m + 112, px);
    store<u32>(m + 116, py);
    store<u32>(m + 120, pz);
    store<u8>(m + 128, 1);
    const u32 g5 = load<u32>(load<u32>(b27fn::Game) + 4216);
    const u32 cam = (g5 != 0 && (load<u32>(g5 + 192) & 8) != 0) ? g5 : 0;
    const u32 v = static_cast<u32>(VCall(ctx, cam, 100, {cam}));  // through 0's "vtable" without one (quirk)
    store<u32>(m + 132, v);
    if (m == 0) {
        ret(ctx, v);
        return;
    }
    u32 n = load<u16>(o + 60);
    if (const u32 cap = load<u16>(o + 56); n == cap) {
        call(ctx, b27fn::PtrVector_Reserve, {o + 52, (cap << 1) & 0x1FFFEu});
        n = load<u16>(o + 60);
    }
    store<u32>(load<u32>(o + 52) + ((n << 2) & 0x3FFFCu), m);
    u32 n1 = load<u16>(o + 60) + 1;
    const u32 hi = load<u16>(o + 62);
    store<u16>(o + 60, static_cast<u16>(n1));
    if (static_cast<s32>(hi) > static_cast<s32>(n1 & 0xFFFF)) n1 = hi;
    store<u16>(o + 62, static_cast<u16>(n1));
    ret(ctx, n1);
}
