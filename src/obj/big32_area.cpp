// Game code, large-function sweep part 32: the point of a sound area
// (sphere or box) nearest to a listener, with its weight and spread.
#include "../sly4.h"
#include "big32.h"

using namespace sly4;
using namespace sly4::big32;

namespace {
constexpr u32 kAreaK = 0x00210D30;  // .text: f32 1.0, f32 360.0, f64 0.0 at +8 (its high word f32 0.0), vperm masks at
                                    // +16 / +32 / +48, f64 1.0 at +64, f32 4.0 at +72, f32 57.29578 at +76
constexpr u32 Box_ContainsPoint5573D0 = 0x005573D0;  // bool (const Box* b, const f32 p[3]) (lanes x / y / z)
constexpr u32 Atanf5E67D4 = 0x005E67D4;              // atanf: f1 -> f1

// One lane of the local point: R^T * p + R^T * (0 - t) as the two vmaddfp
// chains the original builds from the transposed rows (+96 / +112 / +128,
// translation +144), summed with vaddfp.
f32 LocalLane(u32 o, u32 k, f32 px, f32 py, f32 pz) {
    const f32 c0 = W32f(o + 96 + 16 * k), c1 = W32f(o + 100 + 16 * k), c2 = W32f(o + 104 + 16 * k);
    const f32 nx = 0.0f - W32f(o + 144), ny = 0.0f - W32f(o + 148), nz = 0.0f - W32f(o + 152);
    f32 a = std::fma(c0, px, 0.0f);
    a = std::fma(c1, py, a);
    a = std::fma(c2, pz, a);
    f32 b = std::fma(c0, nx, 0.0f);
    b = std::fma(c1, ny, b);
    b = std::fma(c2, nz, b);
    return b + a;
}

// The local point (x, y, z) back to world space: rows +96 / +112 / +128
// (lvx) scaled by the lanes, plus the translation row +144.
V4 ToWorld(u32 o, f32 x, f32 y, f32 z) { return Add4(Lvx(o + 144), Row3(Lvx(o + 96), Lvx(o + 112), Lvx(o + 128), x, y, z)); }

// fsel / fsubs clamp of x (f64 of an f32) to [mn, mx]: x <= mx ? x : mx, then
// that >= mn ? it : mn.
f64 ClampBox(f64 x, f64 mn, f64 mx) {
    x = Fsel31(Subs(mx, x), x, mx);
    return Fsel31(Subs(x, mn), x, mn);
}
}  // namespace

// f32 f(Area* a, f32 p[3] (in / out), f32* weight, f32* spread) (called by
// f_002112C8 and Area_TouchSound_002114FC). The area's 4x4 matrix is at
// +96..+159; p is taken to the area's local space (R^T * (p - t)) and
// *weight = 1.0, *spread = 360.0 are stored. By the shape at +428:
//  - 3 (box: min +496, max +512): when Box_ContainsPoint(a + 496, local),
//    +608 = 0 and p = the local point back in world space; otherwise the
//    falloff f = max over x / y / z of (local - max) / +560[k] (when
//    +560[k] > 0 and local > max) or (local - min) / +576[k] (when +576[k]
//    < 0 and local < min), 0 at least; p = the local point clamped to the
//    box, in world space; *spread = (1 - f)^3 * 360; +608 = f.
//  - 0 (sphere: radius +488, outer radius +492): when |local| > +488,
//    *spread = atanf(r / d) * 4 * 57.29578 and +608 = (d - r) / (+492 - r)
//    (else +608 = 0); p = the centre (+144).
//  - other: p is left alone.
// *weight is then re-read, clamped to [0, 1] (NaN -> 1) and stored; f1
// returns it. r3 is left at kAreaK (no caller reads it; set to match).
DECOMP(00210D80, Area_EmitPoint_00210D80) {
    const u32 o = arg<u32>(ctx, 0), pos = arg<u32>(ctx, 1), wt = arg<u32>(ctx, 2), spr = arg<u32>(ctx, 3);
    Stack frame(ctx, 144);
    const u32 sp = frame.addr() - 112;  // the original's frame (offsets below are its sp offsets)
    const u32 loc = sp + 112;           // the local point (lane w, built from an unset stack word, is never read)
    const f32 px = W32f(pos), py = W32f(pos + 4), pz = W32f(pos + 8);
    for (u32 k = 0; k < 3; ++k) store<f32>(loc + 4 * k, LocalLane(o, k, px, py, pz));
    store<u32>(wt, 0x3F800000u);   // 1.0
    store<u32>(spr, 0x43B40000u);  // 360.0
    const f64 one = Lfs(kAreaK), deg = Lfs(kAreaK + 4);
    const s32 shape = load<s32>(o + 428);
    if (shape == 3) {
        if (static_cast<u32>(call(ctx, Box_ContainsPoint5573D0, {o + 496, loc})) != 0) {
            store<u32>(o + 608, 0);
            Store3W(pos, ToWorld(o, W32f(loc), W32f(loc + 4), W32f(loc + 8)));
        } else {
            const f64 zero = Lfs(kAreaK + 8);
            f64 f = zero;
            for (u32 k = 0; k < 3; ++k) {
                f64 r = zero;
                bool hit = false;
                const f64 up = Lfs(o + 560 + 4 * k);
                if (up > r) {
                    const f64 v = Lfs(loc + 4 * k), mx = Lfs(o + 512 + 4 * k);
                    if (v > mx) {
                        r = Divs(Subs(v, mx), up);
                        hit = true;
                    }
                }
                if (!hit) {
                    const f64 dn = Lfs(o + 576 + 4 * k);
                    if (dn < zero) {
                        const f64 v = Lfs(loc + 4 * k), mn = Lfs(o + 496 + 4 * k);
                        if (v < mn) r = Divs(Subs(v, mn), dn);
                    }
                }
                if (r > f) f = r;
            }
            const f64 x = ClampBox(Lfs(loc), Lfs(o + 496), Lfs(o + 512));
            const f64 y = ClampBox(Lfs(loc + 4), Lfs(o + 500), Lfs(o + 516));
            const f64 z = ClampBox(Lfs(loc + 8), Lfs(o + 504), Lfs(o + 520));
            store<u32>(loc, Fb(x));
            store<u32>(loc + 4, Fb(y));
            store<u32>(loc + 8, Fb(z));
            Store3W(pos, ToWorld(o, static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z)));
            const f64 g = Subs(one, f);
            f64 s = Muls(g, g);
            s = Muls(s, g);
            s = Muls(s, g);
            store<u32>(spr, Fb(Muls(s, deg)));
            store<u32>(o + 608, Fb(f));
        }
    } else if (shape == 0) {
        const f64 lx = Lfs(loc), ly = Lfs(loc + 4), lz = Lfs(loc + 8);
        f64 d2 = Muls(ly, ly);
        d2 = Fmadds(lx, lx, d2);
        d2 = Fmadds(lz, lz, d2);
        const f64 r = Lfs(o + 488);
        f64 t = Lfs(kAreaK + 8);
        const f64 d = Sqrts(d2);
        if (!(d < r || d == r)) {
            const f64 q = Divs(r, d);
            t = Divs(Subs(d, r), Subs(Lfs(o + 492), r));
            f64 a = callf(ctx, Atanf5E67D4, {}, {q});
            a = Muls(a, Lfs(kAreaK + 72));
            store<u32>(spr, Fb(Muls(a, Lfs(kAreaK + 76))));
        }
        Copy3W(pos, o + 144);
        store<u32>(o + 608, Fb(t));
    }
    const f64 w0 = Lfs(wt);
    f64 w = S(Fsel31(Subs(one, w0), w0, Lfd31(kAreaK + 64)));
    w = S(Fsel31(w, w, Lfd31(kAreaK + 8)));
    store<u32>(wt, Fb(w));
    ctx.fpr[1] = w;
    ctx.gpr[3] = kAreaK;
}
