// Game code, large-function sweep part 28: a part's rigid body moved toward a
// target point (optionally stopped short by a ray cast) and turned to face
// along its displacement.
#include "../sly4.h"
#include "big28.h"

using namespace sly4;
using namespace sly4::big28;

namespace {
constexpr u32 kPool103 = 0x00103A70;  // .text pool: masks {y} / {z} / {w}, then the f32 constants below
constexpr f32 kZero = 0.0f;           // pool +0 (lane 0 of the {y} mask)
constexpr f32 kMinMove = 0.01f;       // pool +48
constexpr f32 kOne = 1.0f;            // pool +52
constexpr f32 kMinDot = 0.001f;       // pool +56
constexpr f32 kMinLen = 1e-4f;        // pool +60
constexpr f32 kMinHitSq = 1e-5f;      // pool +64
constexpr f32 kMinGap = 0.1f;         // pool +68 (and as a double at +72)
}  // namespace

// void f(T* o, const f32 target[3], u8 cast) (called by f_00104508,
// Part_SyncBody_00105360). Needs o->+16, the body owner o->+20 and the body
// b = o->+24 (returns with r3 = the first null one, o->+20 for b). p is
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
// when the two are opposite). Without b->+8 returns (r3 = b, f1 = |d|).
// Else HkBody_SetPosition_0091DFF0(b, {0, 0, |d|, 0}), HkBody_SetPosRot(b,
// target, rotation) and, for b re-read each time, Hk_Activate(b) and b+224's
// vfn[68] / vfn[72] with the zero vector (linear / angular velocity).
DECOMP(00103AC0, Part_MoveBodyToward_00103AC0) {
    const u32 o = arg<u32>(ctx, 0), tp = arg<u32>(ctx, 1);
    const bool cast = (arg<u32>(ctx, 2) & 0xFF) != 0;
    if (load<u32>(o + 16) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 own = load<u32>(o + 20);
    if (own == 0) {
        ret(ctx, 0u);
        return;
    }
    if (load<u32>(o + 24) == 0) {
        ret(ctx, own);
        return;
    }
    Stack frame(ctx, 800);
    const u32 sp = frame.addr();
    const u32 VEC = sp + 128, ROT = sp + 144, COL = sp + 192;
    const Masks3 mk = LoadMasks3(kPool103);
    const u32 P = own + 288;
    const f64 px = Lf(P), py = Lf(P + 4), pz = Lf(P + 8);
    f64 tx = Lf(tp), ty = Lf(tp + 4), tz = Lf(tp + 8);
    {
        const f64 dx = Subs(tx, px), dy = Subs(ty, py), dz = Subs(tz, pz);
        const f64 len = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy))));
        const f64 maxd = Lf(o + 40);
        if (len > maxd) {
            const f64 s = Divs(kOne, len);
            tx = Fmadds(maxd, Muls(s, dx), px);
            ty = Fmadds(maxd, Muls(s, dy), py);
            tz = Fmadds(maxd, Muls(s, dz), pz);
        }
    }

    // The ray cast.
    if (cast) {
        const u32 world = load<u32>(o + 272);
        store<u32>(sp + 176, 0x34000000u);
        store<u32>(sp + 180, 0x34000000u);
        store<u32>(COL + 0, b28fn::VtHitCol);
        store<u32>(COL + 16, sp + 224);  // the inline array
        store<u32>(COL + 24, 0x80000008u);
        store<u32>(COL + 20, 0);
        store<u32>(COL + 4, 0x7F7FFFEEu);
        Stvx(sp + 176, Zero4());
        for (u32 k = 0; k < 4; ++k) store<u32>(sp + 608 + 4 * k, Fb(tz));  // the last splat it builds there
        Stvx(sp + 112, RowSelW(P, P + 4, P + 8, mk));
        Stvx(sp + 160, RowSel(Fill4D(tx), Fill4D(ty), Fill4D(tz), mk));
        VCall(ctx, world, 64, {world, sp + 112, sp + 160, COL, 0});
        if (load<s32>(COL + 20) > 0) {
            call(ctx, b28fn::HkHits_Sort, {COL});
            const u32 h = load<u32>(COL + 16);
            f64 hx = Subs(Lf(h), px), hy = Subs(Lf(h + 4), py), hz = Subs(Lf(h + 8), pz);
            const f64 gap = Lf(o + 700);
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
        store<u32>(COL + 20, 0);
        const u32 cap = load<u32>(COL + 24);
        if ((cap & 0x80000000u) == 0) {
            store<u32>(COL + 0, b28fn::VtHitCol);
            const u32 bytes = (cap & 0x3FFFFFFFu) * 48u;
            VCallEO27(ctx, b28fn::HkHeap, 20, {b28fn::HkHeap, load<u32>(COL + 16), static_cast<u64>(static_cast<s64>(static_cast<s32>(bytes)))});
        }
        store<u32>(COL + 16, 0);
        store<u32>(COL + 24, 0x80000000u);
        store<u32>(COL + 0, b28fn::VtHitColEnd);
    }

    // The facing rotation.
    f64 dx = Subs(px, tx), dy = Subs(py, ty), dz = Subs(pz, tz);
    const f64 len = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy))));
    if (len < kMinMove) dx = Lf(o + 400), dy = Lf(o + 404), dz = Lf(o + 408);
    const f64 ax0 = Lf(b28fn::AxisZ), ay0 = Lf(b28fn::AxisZ + 4), az0 = Lf(b28fn::AxisZ + 8);
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

    const u32 b = load<u32>(o + 24);
    ctx.fpr[1] = len;
    if (load<u32>(b + 8) == 0) {
        ret(ctx, b);
        return;
    }
    for (u32 k = 0; k < 4; ++k) store<u32>(sp + 608 + 4 * k, Fb(len));
    Stvx(VEC, Andc(Sel(Zero4(), Fill4D(len), mk.m[1]), mk.m[2]));
    call(ctx, b28fn::HkBody_SetPosition_0091DFF0, {b, VEC});
    for (u32 k = 0; k < 4; ++k) store<u32>(sp + 608 + 4 * k, Fb(qw));
    Stvx(VEC, RowSel(Fill4D(tx), Fill4D(ty), Fill4D(tz), mk));
    Stvx(ROT, RowSel4(Fill4D(qx), Fill4D(qy), Fill4D(qz), Fill4D(qw), mk));
    call(ctx, b28fn::HkBody_SetPosRot, {load<u32>(o + 24), VEC, ROT});
    for (u32 pass = 0; pass < 2; ++pass) {
        const u32 bb = load<u32>(o + 24);
        Stvx(VEC, RowSelW(b28fn::ZeroVec, b28fn::ZeroVec + 4, b28fn::ZeroVec + 8, mk));
        call(ctx, b28fn::Hk_Activate, {bb});
        VCall(ctx, bb + 224, pass == 0 ? 68 : 72, {bb + 224, VEC});  // linear / angular velocity
    }
}
