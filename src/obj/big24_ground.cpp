// Game code, large-function sweep part 24: the four-ray ground-normal probe.
#include "../sly4.h"
#include "big24.h"
#include "vmx2.h"

using namespace sly4;
using namespace sly4::big24;

namespace {
using big5::V4;

constexpr u32 kQuatToMatrix = 0x00553A90;     // (Mat44* out, const Quat* q, const Vec3* pos)
constexpr u32 kCosf = 0x007344E0;             // f1 -> f1
constexpr u32 kSinf = 0x007345F0;             // f1 -> f1
constexpr u32 kHkFilterTest = 0x00884DC8;     // (hkBool* out, filter, u32 info, u32 info2) (Havok) (name?)
constexpr u32 kPhantomCastRay = 0x009250F0;   // (Phantom*, const RayIn*, RayOut*) (Havok) (name?)
constexpr u32 kWorldCastRay = 0x0092E2E8;     // HkWorld_CastRay92E2E8 (hkpWorld*, const RayIn*, RayOut*)
constexpr u32 kConsts = 0x0026DA40;           // f32 1, 2pi, 0.25, 0.5; masks {0,~0,0,0} {0,0,~0,0} {0,0,0,~0}; f32 pi/180
constexpr u32 kDefaultUp = 0x01164B60;        // Vec3: the normal when the object has no ground normal
constexpr u32 kZeroVec = 0x01164B40;          // Vec3 the hit normals are summed onto
constexpr u32 kProbeOn = 0x0105C7E8;          // u8: probing enabled
constexpr u32 kRayHeight = 0x0105C838;        // f32: ray length (from +h/2 above, down by h)
constexpr u32 kUsePhantom = 0x0105C2A0;       // word: cast through o->+48 when set
constexpr u32 kMaxSlopeDeg = 0x01057144;      // f32: steepest accepted slope (degrees)
constexpr u32 kGame = 0x00F65CA0;             // Game* ; +0x1281D4 -> {+4 hkpWorld*}

// Normalize (x, y, z) as the original does (sum y*y, x*x, z*z; 1 / fsqrts)
// and store it to out.
void StoreUnit(u32 out, f64 x, f64 y, f64 z) {
    f64 l = S(y * y);
    l = Fmadds(x, x, l);
    l = Fmadds(z, z, l);
    const f64 inv = S(Lf(kConsts) / S(std::sqrt(l)));
    Sf(out, S(x * inv));
    Sf(out + 4, S(y * inv));
    Sf(out + 8, S(z * inv));
}

// The normal of the triangle (p0, p1, p2): (p2 - p1) x (p1 - p0), components
// rounded as fmuls + fmsubs.
void StoreTriNormal(u32 out, u32 p0, u32 p1, u32 p2) {
    const f64 bx = S(Lf(p2) - Lf(p1)), by = S(Lf(p2 + 4) - Lf(p1 + 4)), bz = S(Lf(p2 + 8) - Lf(p1 + 8));
    const f64 ax = S(Lf(p1) - Lf(p0)), ay = S(Lf(p1 + 4) - Lf(p0 + 4)), az = S(Lf(p1 + 8) - Lf(p0 + 8));
    const f64 ny = Fmsubs(bz, ax, S(bx * az));
    const f64 nx = Fmsubs(by, az, S(bz * ay));
    const f64 nz = Fmsubs(bx, ay, S(by * ax));
    StoreUnit(out, nx, ny, nz);
}
}  // namespace

// u32 f(Obj* o, Vec3* n; f32 radius) (called by 001233C0, 00131798,
// 00148130): the ground normal under o from four downward rays.
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
// points in ray order; four: (p1 - p3) x (p0 - p2). r3: the hit mask.
// f1 is left unmatched (callers don't read it).
DECOMP(0026DA88, Ground_ProbeNormal_0026DA88) {
    const u32 o = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1);
    const f64 radius = ctx.fpr[1];
    Stack frame(ctx, 1168);
    const u32 sp = frame.addr();
    const u32 tri = sp + 128;    // Vec4[3] the three hit points
    const u32 mtx = sp + 176;    // Mat44
    const u32 pts = sp + 240;    // Vec4[4] hit points
    const u32 nrms = sp + 304;   // Vec4[4] hit normals
    const u32 rin = sp + 368;    // RayIn[4], 48 bytes: from, to, u8 +32, +36 filter info, +40
    const u32 rout = sp + 560;   // RayOut[4], 80 bytes
    const u32 hkb = sp + 880;    // hkBool
    if ((load<u32>(o + 272) & 0x80000000u) != 0) {
        Cw(n, o + 304);
        Cw(n + 4, o + 308);
        Cw(n + 8, o + 312);
    } else {
        Cw(n, kDefaultUp);
        Cw(n + 4, kDefaultUp + 4);
        Cw(n + 8, kDefaultUp + 8);
    }
    if (load<u8>(kProbeOn) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, kQuatToMatrix, {mtx, o + 496, o + 480});
    const u32 info = load<u32>(o + 428);
    const u32 world = load<u32>(load<u32>(load<u32>(kGame) + 0x1281D4u) + 4);
    const u32 filter = load<u32>(world + 116);
    for (u32 k = 0; k < 4; ++k) {
        const u32 ri = rin + 48 * k, ro = rout + 80 * k;
        store<u8>(ri + 32, 0);
        store<u32>(ri + 36, 0);
        store<u32>(ri + 40, 0);
        store<u32>(ro + 16, 0x3F800000u);
        store<u32>(ro + 20, 0xFFFFFFFFu);
        store<u32>(ro + 64, 0);
        store<u32>(ro + 32, 0xFFFFFFFFu);
        store<u32>(ro + 68, 0);
    }
    const f64 neg_h = -Lf(kRayHeight);
    const f64 dx = S(Lf(mtx + 16) * neg_h), dy = S(Lf(mtx + 20) * neg_h), dz = S(Lf(mtx + 24) * neg_h);
    const V4 r0 = big5::Lvx(mtx), r1 = big5::Lvx(mtx + 16), r2 = big5::Lvx(mtx + 32), r3 = big5::Lvx(mtx + 48);
    for (u32 k = 0; k < 4; ++k) {
        const f64 ang = S(S(S(static_cast<f64>(static_cast<s32>(k))) * Lf(kConsts + 4)) * Lf(kConsts + 8));
        const f64 cx = S(callf(ctx, kCosf, {}, {ang}) * radius);
        const f64 cy = S(Lf(kRayHeight) * Lf(kConsts + 12));
        const f64 cz = S(callf(ctx, kSinf, {}, {ang}) * radius);
        const V4 p = big5::Add4(r3, big5::Row3(r0, r1, r2, static_cast<f32>(cx), static_cast<f32>(cy), static_cast<f32>(cz)));
        const u32 ri = rin + 48 * k;
        store<u32>(ri + 36, info);
        store<f32>(ri, p.e[0]);
        store<f32>(ri + 4, p.e[1]);
        store<f32>(ri + 8, p.e[2]);
        store<u32>(ri + 12, 0);
        Sf(ri + 16, S(static_cast<f64>(p.e[0]) + dx));
        Sf(ri + 20, S(static_cast<f64>(p.e[1]) + dy));
        Sf(ri + 24, S(static_cast<f64>(p.e[2]) + dz));
        store<u32>(ri + 28, 0);
    }
    if (load<u32>(o + 48) != 0 && load<u32>(kUsePhantom) != 0) {
        for (u32 k = 0; k < 4; ++k) call(ctx, kPhantomCastRay, {load<u32>(o + 48), rin + 48 * k, rout + 80 * k});
    } else {
        for (u32 k = 0; k < 4; ++k) call(ctx, kWorldCastRay, {world, rin + 48 * k, rout + 80 * k});
    }
    const f64 min_ny = callf(ctx, kCosf, {}, {S(Lf(kMaxSlopeDeg) * Lf(kConsts + 64))});
    u32 mask = 0;
    for (u32 k = 0; k < 4; ++k) {
        const u32 ri = rin + 48 * k, ro = rout + 80 * k;
        const f64 fx = Lf(ri), fy = Lf(ri + 4), fz = Lf(ri + 8);
        const u32 coll = load<u32>(ro + 68);
        if (coll == 0) continue;
        call(ctx, kHkFilterTest, {hkb, filter, info, load<u32>(coll + 28)});
        if (load<u8>(hkb) == 0) continue;
        if (!(Lf(ro + 4) >= min_ny)) continue;
        const f64 t = Lf(ro + 16);
        Sf(pts + 16 * k, Fmadds(t, dx, fx));
        Sf(pts + 16 * k + 4, Fmadds(t, dy, fy));
        Sf(pts + 16 * k + 8, Fmadds(t, dz, fz));
        mask |= 1u << k;
        Cw(nrms + 16 * k, ro);
        Cw(nrms + 16 * k + 4, ro + 4);
        Cw(nrms + 16 * k + 8, ro + 8);
    }
    switch (mask) {
    case 3: case 5: case 6: case 9: case 10: case 12: {
        Cw(n, kZeroVec);  // quirk: x and y written before the sum
        Cw(n + 4, kZeroVec + 4);
        f64 x = Lf(kZeroVec), y = Lf(kZeroVec + 4), z = Lf(kZeroVec + 8);
        for (u32 k = 0; k < 4; ++k) {
            if ((mask & (1u << k)) == 0) continue;
            x = Adds(x, Lf(nrms + 16 * k));
            y = Adds(y, Lf(nrms + 16 * k + 4));
            z = Adds(z, Lf(nrms + 16 * k + 8));
        }
        StoreUnit(n, x, y, z);
        break;
    }
    case 7: case 11: case 13: case 14: {
        u32 d = tri;
        for (u32 k = 0; k < 4; ++k) {
            if ((mask & (1u << k)) == 0) continue;
            Cw(d, pts + 16 * k);
            Cw(d + 4, pts + 16 * k + 4);
            Cw(d + 8, pts + 16 * k + 8);
            d += 16;
        }
        StoreTriNormal(n, tri, tri + 16, tri + 32);
        break;
    }
    case 15: {
        const f64 bx = S(Lf(pts + 16) - Lf(pts + 48)), by = S(Lf(pts + 20) - Lf(pts + 52)), bz = S(Lf(pts + 24) - Lf(pts + 56));
        const f64 ax = S(Lf(pts) - Lf(pts + 32)), ay = S(Lf(pts + 4) - Lf(pts + 36)), az = S(Lf(pts + 8) - Lf(pts + 40));
        const f64 ny = Fmsubs(bz, ax, S(bx * az));
        const f64 nx = Fmsubs(by, az, S(bz * ay));
        const f64 nz = Fmsubs(bx, ay, S(by * ax));
        StoreUnit(n, nx, ny, nz);
        break;
    }
    default:
        break;
    }
    ctx.gpr[3] = mask;
}
