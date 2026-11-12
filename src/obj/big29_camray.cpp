// Game code, large-function sweep part 29: the follow camera's update
// (blend tracks, the eye from the target's offset, a ray cast pulling the
// eye in front of blocking geometry, then the look-at matrix).
#include "../sly4.h"
#include "big29.h"

using namespace sly4;
using namespace sly4::big29;

namespace {
constexpr u32 kPoolCam = 0x00304F90;   // .text: +0 1.0, +16 / +32 / +48 the y / z / w lane masks
constexpr u32 kGame = 0x00F65CA0;      // Game* (+0x1281D4 the physics holder, +4 its world)
constexpr u32 kVtRayCol = 0x00F1DE80;  // ray hit collector vtable (80-byte hits, 8 inline)
constexpr u32 kMinEyeY = 0x0106A268;   // f32: the eye's lowest y
constexpr u32 kIdentity = 0x01164BD0;  // the global 4x4 identity
constexpr u32 kUp = 0x01164B60;        // the global up vector
constexpr u32 kHkHeap = 0x011DF31C;    // Havok heap allocator (vfn[20] (p, bytes) frees)
constexpr u32 kPropNoCamBlock = 0x91C7CF5B;  // shape property: set -> the hit does not block the camera
constexpr u32 Obj_Blend = 0x004A0E60;        // (Track*; f1 dt)
constexpr u32 Matrix_NormalizeInPlace = 0x0056D374;  // (f32 m[4][4])
constexpr u32 HkWorld_CastRay_92E510 = 0x0092E510;  // (hkpWorld*, const RayInput*, Collector*)
}  // namespace

// u32 f(Cam* o; f32 dt) (called by Obj_AttachCam_003055E8, f_0030F798,
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
// heap hit array is freed through kHkHeap->vfn[20] (entry in r6, OPD r7).
DECOMP(00304FD0, Cam_UpdateFollow_00304FD0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    if (load<u32>(o) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    Stack frame(ctx, 1008);
    const u32 sp = frame.addr();  // the original's frame (offsets below are its sp offsets)
    const u32 p = load<u32>(o + 4) + 432;
    for (const u32 k : {48u, 84u, 120u, 156u}) callf(ctx, Obj_Blend, {o + k}, {dt});
    const u32 t = load<u32>(o + 4);
    const u32 a = t + 128, b = t + 144;
    const f64 ay = Lf(a + 4), ax = Lf(a + 0), az = Lf(a + 8);
    const f64 one = Lf(kPoolCam);
    const f64 alen = Sqrts(Fmadds(az, az, Fmadds(ax, ax, Muls(ay, ay))));
    f64 sy = Adds(Lf(p + 4), Lf(o + 112));
    const f64 dist = Lf(o + 184), back = Lf(o + 76);
    const Masks3 mk = LoadMasks3(kPoolCam + 16);
    // RayInput: from = {p.x, p.y + o->+112, p.z, 0}.
    Stvx(sp + 112, RowSel(SplatW(p), Fill4D(sy), SplatW(p + 8), mk));
    const f64 inv = Divs(one, alen);
    store<u8>(sp + 144, 1);
    store<u32>(sp + 148, 16);
    store<u32>(sp + 152, 0);
    store<u32>(sp + 228, 0x3F800000u);
    store<u32>(sp + 224, kVtRayCol);
    store<u32>(sp + 240, sp + 256);
    store<u32>(sp + 244, 0);
    store<u32>(sp + 248, 0x80000008u);
    const f64 ey = Adds(Fmadds(dist, Muls(inv, ay), Lf(b + 4)), Lf(o + 148));
    const f64 ex = Fmadds(dist, Muls(inv, ax), Lf(b + 0));
    const f64 ez = Fmadds(dist, Muls(inv, az), Lf(b + 8));
    f64 sx = Lf(p + 0), sz = Lf(p + 8);
    {
        f64 dx = Subs(ex, sx), dz = Subs(ez, sz), dy = Subs(ey, sy);
        const f64 il = Divs(one, Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)))));
        dx = Muls(dx, il), dy = Muls(dy, il), dz = Muls(dz, il);
        sx = mid14::Fnmsubs(dx, back, sx);
        sy = mid14::Fnmsubs(dy, back, sy);
        sz = mid14::Fnmsubs(dz, back, sz);
    }
    Stvx(sp + 128, RowSel(Fill4D(sx), Fill4D(sy), Fill4D(sz), mk));
    for (u32 k = 0; k < 8; ++k) {
        const u32 h = sp + 256 + 80 * k;
        store<u32>(h + 16, 0x3F800000u);
        store<u32>(h + 20, 0xFFFFFFFFu);
        store<u32>(h + 64, 0);
        store<u32>(h + 32, 0xFFFFFFFFu);
        store<u32>(h + 68, 0);
    }
    store<u32>(sp + 244, 0);
    store<u32>(sp + 228, 0x3F800000u);
    call(ctx, HkWorld_CastRay_92E510, {load<u32>(load<u32>(load<u32>(kGame) + 0x1281D4) + 4), sp + 112, sp + 224});
    // The first blocking hit's fraction (1 without one).
    f64 frac = one;
    {
        u32 hit = load<u32>(sp + 240);
        const s32 n = load<s32>(sp + 244);
        for (s32 i = 0; i < n; ++i, hit += 80) {
            const u32 sh = load<u32>(hit + 68);
            const bool type1 = (load<u32>(sh + 28) & 0x1F) == 1;
            if (static_cast<s8>(load<u8>(sh + 24)) == 1) {
                const u32 props = sh + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(sh + 16))));
                if (props != 0) {
                    u32 v = 0;
                    for (s32 j = 0; j < load<s32>(props + 128); ++j) {
                        const u32 e = load<u32>(props + 124) + 16 * static_cast<u32>(j);
                        if (load<u32>(e) == kPropNoCamBlock) {
                            v = static_cast<u32>(load<u64>(e + 8));  // the low word is tested
                            break;
                        }
                    }
                    if (v != 0) continue;
                }
            }
            if (!type1) continue;
            frac = Lf(hit + 16);
            break;
        }
    }
    if (frac != one) {
        const f64 px = Lf(p + 0), py = Lf(p + 4), pz = Lf(p + 8);
        sx = Fmadds(Subs(sx, px), frac, px);
        sy = Fmadds(Subs(sy, py), frac, py);
        sz = Fmadds(Subs(sz, pz), frac, pz);
    }
    if (const f64 lo = Lf(kMinEyeY); sy < lo) sy = lo;
    // The look-at matrix at sp + 160 (rows +0 / +16 / +32, translation +48).
    const u32 m = sp + 160;
    f64 dy = Subs(ey, sy), dx = Subs(ex, sx), dz = Subs(ez, sz);
    const f64 len = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy))));
    for (const u32 k : {32u, 36u, 40u, 0u, 4u, 8u, 16u, 20u, 24u, 48u, 52u, 56u}) store<u32>(m + k, load<u32>(kIdentity + k));
    const f64 il = Divs(one, len);
    dx = Muls(dx, il), dy = Muls(dy, il), dz = Muls(dz, il);
    Sf(m + 32, dx), Sf(m + 36, dy), Sf(m + 40, dz);
    const f64 r0x = Fmsubs(Lf(kUp + 4), dz, Muls(Lf(kUp + 8), dy));
    Sf(m + 0, r0x);
    const f64 r0y = Fmsubs(Lf(kUp + 8), dx, Muls(Lf(kUp + 0), dz));
    Sf(m + 4, r0y);
    const f64 r1z = Fmsubs(dx, r0y, Muls(dy, r0x));
    const f64 r0z = Fmsubs(Lf(kUp + 0), dy, Muls(Lf(kUp + 4), dx));
    Sf(m + 24, r1z);
    const f64 r1x = Fmsubs(dy, r0z, Muls(dz, r0y));
    Sf(m + 8, r0z);
    const f64 r1y = Fmsubs(dz, r0x, Muls(dx, r0z));
    Sf(m + 16, r1x);
    Sf(m + 20, r1y);
    call(ctx, Matrix_NormalizeInPlace, {m});
    Sf(m + 48, sx), Sf(m + 52, sy), Sf(m + 56, sz);
    const u32 cam = load<u32>(o);
    for (const u32 k : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u, 48u, 52u, 56u}) store<u32>(cam + 16 + k, load<u32>(m + k));
    SetF1(ctx, m + 56);  // f1 as the last lfs left it
    const u32 cap = load<u32>(sp + 248), hits = load<u32>(sp + 240);
    if ((cap & 0x80000000u) != 0) {
        ctx.gpr[3] = cap;
        return;
    }
    store<u32>(sp + 244, 0);
    store<u32>(sp + 224, kVtRayCol);
    const u32 opd = load<u32>(load<u32>(kHkHeap) + 20);
    const u64 bytes = static_cast<u64>(static_cast<s64>(static_cast<s32>(80 * (cap & 0x3FFFFFFFu))));
    call_ptr(ctx, opd, {kHkHeap, hits, bytes, load<u32>(opd), opd});
}
