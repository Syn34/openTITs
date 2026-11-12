// Game code, large-function sweep part 28: a game object's ring of spline
// effects (spawned, aged and placed along five splines, faded, triggering a
// camera event near the camera) and its camera target along spline 2.
#include "../sly4.h"
#include "big28.h"

using namespace sly4;
using namespace sly4::big28;

namespace {
constexpr u32 kPool548 = 0x00054850;  // .text pool: the f32 / f64 constants below
constexpr f32 kOne = 1.0f;            // pool +0 (f19)
constexpr f32 kLimit0 = 7.6f;         // pool +4: the fade limit
constexpr f32 kLimit8 = 3.0f;         // pool +8: the fade limit in mode 8
constexpr f32 kTanY = 0.0f;           // pool +12: the tangent's y as used
constexpr f64 kHi = 1.0;              // pool +16 (f64)
constexpr f64 kLo = 0.0;              // pool +24 (f64)
constexpr f32 kFade = -0.5f;          // pool +32
constexpr f32 kSpawnK = 60.0f;        // pool +36
constexpr f32 kNearSq = 2.25f;        // pool +40
// Tweak variables (f32 / u8 value at +72 of each).
constexpr u32 kTwSpan = 0x00F9A70C;     // f15: the life divisor
constexpr u32 kTwRate = 0x00F9A758;     // f31: spawns per 60
constexpr u32 kTwSpan8 = 0x00F9A7A4;    // mode 8 kTwSpan
constexpr u32 kTwRate8 = 0x00F9A7F0;    // mode 8 kTwRate
constexpr u32 kTwSpeed8 = 0x00F9A888;   // mode 8 time scale
constexpr u32 kTwFadeAt = 0x00F9A83C;   // the fade point (over o->5088's +120)
constexpr u32 kTwCamOn = 0x00F9A4F8;    // u8: camera target enabled
constexpr u32 kTwBlend = 0x00F9A6C0;    // camera blend time
constexpr u32 kTwLead = 0x00F9A674;     // the target's lead along the spline (over its +208's +120)
constexpr u32 kTwCamY = 0x00F9A544;     // u8: the target's y from the camera's
constexpr u32 kTwYOff = 0x00F9A628;     // target y offset
constexpr u32 kTwBack = 0x00F9A590;     // eye offset back along the direction
constexpr u32 kTwUp = 0x00F9A5DC;       // eye offset along UpVec
// Code and data.
constexpr u32 Game_BuildEffectSplines = 0x000533A0;  // (GameObj*)
constexpr u32 Obj_SetEffectHash = 0x000DAEC8;         // (Obj*, u32 hash)
constexpr u32 SplineInstance_Nearest = 0x005927FC;    // (inst, const f32 p[3], u32*, f32*) -> f1
constexpr u32 WorldObj_SplinePointAt = 0x00592748;    // (o, -, f32 out[4], u8; f1 t)
constexpr u32 WorldObj_SplinePointTangentAt = 0x005A80D8;  // (o, -, f32 pos[4], f32 tan[4], u8; f1 t)
constexpr u32 Obj_GetPathPose = 0x0012919C;           // (Obj*, f32* t, Mat34*, u32*) -> u32
constexpr u32 Obj_PopLayer = 0x000FBB88;              // (Object*, Animator*; f1 t)
constexpr u32 Camera_PushTarget = 0x000FC040;         // (Camera*, Target*, -, u8 priority; f1 blend)
constexpr u32 SmallAlloc = 0x0055238C;                // (u32 size) -> p or 0
constexpr u32 Game = 0x00F65CA0;                      // Game*
constexpr u32 GameLayers = 0x001281E0;                // Game +0x1281E0: the camera layer object
constexpr u32 Rng = 0x01164888;                       // u64 +0 the last output (16 bits); u32 +8 the state
constexpr u32 UpVec = 0x01164B60;                     // the global up vector
constexpr u32 kVtCamTarget = 0x00B13B38;              // 112-byte camera target
constexpr u32 kVtCamEvent = 0x00B0DADC;               // camera event on the stack
constexpr u32 kVtCamEventEnd = 0x00B0DABC;            // stored over it after the post
constexpr u32 kHashSpawn = 0x76F894FCu;               // effect hash: spawned
constexpr u32 kHashFade = 0x879F2BDAu;                // effect hash: fading
constexpr u32 kHashNear = 0x00011773u;                // effect hash: passed near the camera
constexpr u32 kEventNear = 0x2DA0A51Du;               // the event's hash
inline f64 Tw(u32 base) { return Lf(base + 72); }
// fsel / frsp / fsel: x clamped to [kLo, kHi] by 1 - x >= 0 then x >= 0 (NaN: kHi, then kept).
inline f64 Clamp01(f64 one, f64 x) {
    const f64 a = S(Subs(one, x) >= 0.0 ? x : kHi);
    return a >= 0.0 ? a : kLo;
}
}  // namespace

// u32 f(GameObj* o; f32 dt) (called by f_00056468, f_0005A6A0). Effects
// live in a 256-entry ring (u8 head o->+1740, tail +1741): object +1756[i],
// age +2780[i], spline index 0..4 +3804[i], u8 +4828[i]; splines +5104[0..4].
// 1. With o->+36 under the fade limit (7.6, 3.0 in mode 8 = o->+308) and
//    u8 +1742 set, dt is scaled (by 1, or a tweak in mode 8) and every
//    entry j of every group i of (o->+1752)->+160's +208 (u8 count +112,
//    groups +132 + 8i: list +48 (8-byte entries, object +4), u16 count +52)
//    whose object's +128 controller c (+56) exists gets c+0 = 0, c+32 bits
//    0x0FF00000 = 2 and c+36 = o->+36 / limit * -0.5 while o->+36 stays
//    under the limit; then each spline's vfn[40](s, 0, 0). Else the same
//    walk sets c+36 = -0.5 and vfn[40](s, 1, 0).
// 2. Game_BuildEffectSplines(o); the spawn timer +1736 += dt; past 60 /
//    rate it restarts and, with room in the ring, the head object gets
//    kHashSpawn, age 0, a random spline (generator state % 5), u8 0, head++.
// 3. The camera (Game->+4208's +8, when its +192 has bits 2 and 8; else 0)
//    vfn[80] gives its position; every spline's SplineInstance_Nearest to
//    it. With the layer object (Game +0x1281E0) and kTwCamOn: if the
//    camera's vfn[64] object has mode (+416 >> 28 & 7) 1, a camera target
//    +5124 is popped (Obj_PopLayer) and +5124 / +5128 cleared; otherwise
//    from the camera's path (Obj_GetPathPose, else +5128 kept) spline 2
//    (+5112, stored to +5128) gives a point a lead past the camera's
//    nearest; a look frame from the camera's +144.. (eye moved back / up by
//    tweaks) toward it fills the target (+16.. right, +32.. up, +48.. dir,
//    +64.. eye), a new 112-byte one (SmallAlloc) being pushed on the layer
//    object (Camera_PushTarget).
// 4. The fade point (tweak / o->+5088's +120, clamped to [0, 1]) and from
//    tail to head: age += dt, t = 1 - clamp(age / (span / tweak)); t == 0
//    retires the tail (tail++); else the object is placed at the spline
//    point at t with the frame {UpVec x tangent, UpVec, tangent (y = 0,
//    normalised), point} (vfn[84]); while t is under the fade point it
//    gets kHashFade (unless vfn[132] already gives it); otherwise within
//    1.5 of the camera position a camera event (kEventNear, o->+60, flags
//    from the stack's +158 with 0x1000 from o->+60's +184 bit 0) is posted
//    to the camera (vfn[344], camera with bit 2) and it gets kHashNear.
// Returns the head (r3) with f1 left as the original leaves it.
DECOMP(00054880, Obj_UpdateSplineFx_00054880) {
    const u32 o = arg<u32>(ctx, 0);
    f64 dt = ctx.fpr[1];  // f18
    Stack frame(ctx, 512);
    const u32 sp = frame.addr();
    const f64 one = kOne;  // f19
    f64 scale = kOne, limit = kLimit0;
    f64 span = Tw(kTwSpan), rate = Tw(kTwRate);  // f15, f31
    f64 cur = Lf(o + 36);                       // f3
    if (load<s32>(o + 308) == 8) {
        span = Tw(kTwSpan8);
        rate = Tw(kTwRate8);
        scale = Tw(kTwSpeed8);
        limit = kLimit8;
    }
    // (a five-iteration empty bdnz loop)

    // 1. The controllers.
    const bool fading = cur < limit && load<u8>(o + 1742) != 0;
    if (fading) dt = Muls(dt, scale);
    {
        const u32 grp = load<u32>(load<u32>(o + 1752) + 160);
        for (u32 i = 0;; ++i) {
            u32 g = load<u32>(grp + 208);
            if (!(i < load<u8>(g + 112))) break;
            for (u32 j = 0;; ++j) {
                if (!(j < load<u16>(load<u32>(g + 8 * i + 132) + 52))) break;
                const u32 m = load<u32>(load<u32>(load<u32>(load<u32>(g + 8 * i + 132) + 48) + 8 * j + 4) + 128);
                if (m == 0 || m + 56 == 0 || !(cur < limit)) continue;
                const u32 c = m + 56;
                const u32 w = (load<u32>(c + 32) & ~0x0FF00000u) | (2u << 20);
                store<u32>(c + 0, 0);
                if (fading) {
                    store<u32>(c + 32, w);
                    cur = Lf(o + 36);
                    Sf(c + 36, Muls(Divs(cur, limit), kFade));
                } else {
                    store<u32>(c + 36, 0xBF000000u);  // -0.5
                    store<u32>(c + 32, w);
                    cur = Lf(o + 36);
                }
                g = load<u32>(grp + 208);
            }
        }
    }
    for (u32 k = 0; k < 5; ++k) {
        const u32 s = load<u32>(o + 5104 + 4 * k);
        VCall(ctx, s, 40, {s, fading ? 0u : 1u, 0});
    }

    // 2. Spawning.
    call(ctx, Game_BuildEffectSplines, {o});
    const f64 acc = Adds(Lf(o + 1736), dt);
    Sf(o + 1736, acc);
    if (acc > Divs(kSpawnK, rate)) {
        const u32 head = load<u8>(o + 1740), tail = load<u8>(o + 1741);
        store<u32>(o + 1736, 0);
        if (((head + 1) & 0xFF) != tail) {
            call(ctx, Obj_SetEffectHash, {load<u32>(o + 1756 + 4 * head), kHashSpawn});
            const u32 h1 = load<u8>(o + 1740);
            const u32 st = load<u32>(Rng + 8);
            const u32 mix = big27::RngMix27(st);
            store<u32>(Rng + 8, std::rotl(mix, 16));
            store<u64>(Rng, mix & 0xFFFFu);
            store<u32>(o + 2780 + 4 * h1, 0);
            const u32 h2 = load<u8>(o + 1740);
            store<u8>(o + 1740, static_cast<u8>(h2 + 1));
            store<u8>(o + 4828 + h2, 0);
            store<u32>(o + 3804 + 4 * h2, st % 5);
        }
    }

    // 3. The camera and its target.
    u32 cam = 0;  // r26
    {
        const u32 g5 = load<u32>(load<u32>(Game) + 4208 + 8);
        if (g5 != 0 && (load<u32>(g5 + 192) & 2) != 0 && (load<u32>(g5 + 192) & 8) != 0) cam = g5;
    }
    VCall(ctx, cam, 80, {sp + 112, cam});  // the camera position to sp+112 (a null camera reads the vtable at 0)
    for (u32 k = 0; k < 3; ++k) store<u32>(sp + 208 + 4 * k, load<u32>(UpVec + 4 * k));
    for (u32 k = 0; k < 5; ++k) call(ctx, SplineInstance_Nearest, {load<u32>(o + 5104 + 4 * k), sp + 112, 0, 0});
    const u32 lay = load<u32>(load<u32>(Game) + GameLayers);  // r24
    if (lay != 0 && load<u8>(kTwCamOn + 72) != 0) {
        bool follow = static_cast<u32>(VCall(ctx, cam, 64, {cam})) == 0;
        if (!follow) follow = ((load<u32>(static_cast<u32>(VCall(ctx, cam, 64, {cam})) + 416) >> 28) & 7) != 1;
        if (!follow) {
            if (const u32 t = load<u32>(o + 5124); t != 0) {
                callf(ctx, Obj_PopLayer, {lay, t}, {Tw(kTwBlend)});
                store<u32>(o + 5124, 0);
                store<u32>(o + 5128, 0);
            }
        } else {
            const u32 pose = static_cast<u32>(call(ctx, Obj_GetPathPose, {cam, sp + 256, 0, 0}));
            u32 path = load<u32>(o + 5128);
            const u32 sp2 = load<u32>(o + 5112);
            if (pose != 0) {
                path = pose;
                store<u32>(o + 5128, pose);
            }
            if (path != 0) {
                store<u32>(o + 5128, sp2);
                call(ctx, SplineInstance_Nearest, {sp2, cam + 144, 0, 0});
                const f64 near = ctx.fpr[1];
                Sf(sp + 256, near);
                const u32 s = load<u32>(o + 5128);
                const f64 lead = Divs(Tw(kTwLead), Lf(load<u32>(s + 208) + 120));
                f64 ex = Lf(cam + 144), ey = Lf(cam + 148), ez = Lf(cam + 152);  // f23 / f24 / f25
                const f64 u = S(Clamp01(one, Adds(near, lead)));
                callf(ctx, WorldObj_SplinePointAt, {s, s, sp + 128, 0}, {u});  // r4 unread (left = r3)
                const f64 px = Lf(sp + 128), pz = Lf(sp + 136);
                f64 py = Lf(sp + 132);
                const u32 tgt = load<u32>(o + 5124);
                if (load<u8>(kTwCamY + 72) != 0) py = Lf(cam + 148);
                py = Adds(py, Tw(kTwYOff));
                f64 dx = Subs(px, ex), dz = Subs(pz, ez), dy = Subs(py, ey);
                Sf(sp + 132, py);
                const f64 ux = Lf(UpVec), uz = Lf(UpVec + 8), uy = Lf(UpVec + 4);
                const f64 back = Tw(kTwBack), up = Tw(kTwUp);
                const f64 il = Divs(one, Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)))));
                dz = Muls(dz, il), dx = Muls(dx, il), dy = Muls(dy, il);
                f64 rx = Fmsubs(uy, dz, Muls(uz, dy));
                f64 ry = Fmsubs(uz, dx, Muls(ux, dz));
                f64 rz = Fmsubs(ux, dy, Muls(uy, dx));
                ex = Adds(ex, Fmadds(ux, up, -Muls(dx, back)));
                ey = Adds(ey, Fmadds(uy, up, -Muls(dy, back)));
                ez = Adds(ez, Fmadds(uz, up, -Muls(dz, back)));
                const f64 ir = Divs(one, Sqrts(Fmadds(rz, rz, Fmadds(rx, rx, Muls(ry, ry)))));
                rx = Muls(rx, ir), ry = Muls(ry, ir), rz = Muls(rz, ir);
                const f64 vz = Fmsubs(dx, ry, Muls(dy, rx));
                const f64 vx = Fmsubs(dy, rz, Muls(dz, ry));
                const f64 vy = Fmsubs(dz, rx, Muls(dx, rz));
                const auto frame12 = [&](u32 p) {
                    Sf(p + 32, vx), Sf(p + 36, vy), Sf(p + 40, vz);
                    Sf(p + 48, dx), Sf(p + 52, dy), Sf(p + 56, dz);
                    Sf(p + 64, ex), Sf(p + 68, ey), Sf(p + 72, ez);
                };
                ctx.fpr[1] = ir;
                if (tgt != 0) {
                    Sf(tgt + 16, rx), Sf(tgt + 20, ry), Sf(tgt + 24, rz);
                    frame12(tgt);
                } else {
                    const u32 p = static_cast<u32>(call(ctx, SmallAlloc, {112}));
                    if (p != 0) {
                        store<u32>(p + 4, lay);
                        store<u8>(p + 8, 0);
                        store<u32>(p + 0, kVtCamTarget);
                        Sf(p + 16, rx), Sf(p + 20, ry), Sf(p + 24, rz);
                        const u32 w88 = load<u32>(p + 88);
                        frame12(p);
                        store<u32>(p + 80, 0x425C0000u);  // 55.0
                        store<u32>(p + 88, ((w88 & 0x7FFFFFFFu) & ~0x7F800000u) | (1u << 23));  // bit 31 cleared, bits 23..30 = 1
                        store<u32>(p + 92, 0);
                        store<u32>(p + 96, 0);
                        store<u32>(p + 100, 0);
                        store<u32>(p + 84, 0);
                    }
                    store<u32>(o + 5124, p);
                    callf(ctx, Camera_PushTarget, {lay, p, kTwBlend, 0}, {Tw(kTwBlend)});  // r5 unread (left: the tweak's address)
                }
            }
        }
    }

    // 4. The ring.
    const f64 life = Lf(load<u32>(o + 5088) + 120);
    const f64 fadeAt0 = Divs(Tw(kTwFadeAt), life);
    span = Divs(life, span);
    u32 i = load<u8>(o + 1741);
    u32 head = load<u8>(o + 1740);
    const f64 fa = S(Subs(one, fadeAt0) >= 0.0 ? fadeAt0 : kHi);
    const f64 fadeAt = S(fa >= 0.0 ? fa : kLo);  // f31
    ctx.fpr[1] = fa;
    if (i == head) {
        ret(ctx, head);
        return;
    }
    const f64 tan2 = Muls(kTanY, kTanY);  // f27
    for (;;) {
        const u32 ageA = o + 2780 + 4 * i, idxA = o + 3804 + 4 * i, objA = o + 1756 + 4 * i;
        const f64 age = Adds(Lf(ageA), dt);
        const f64 q = Divs(age, span);
        Sf(ageA, age);
        const u32 si = load<u32>(idxA);
        const f64 c1 = Clamp01(one, q);
        const f64 t = Subs(one, S(c1));  // f30
        if (t == kTanY) {
            store<u8>(o + 1741, static_cast<u8>(load<u8>(o + 1741) + 1));
            ctx.fpr[1] = c1;
        } else {
            callf(ctx, WorldObj_SplinePointTangentAt, {load<u32>(o + 5104 + 4 * si), idxA, sp + 240, sp + 224, 0}, {t});  // r4 unread (left: &spline index)
            const f64 tx = Lf(sp + 224), tz = Lf(sp + 232);
            const f64 uy = Lf(sp + 212), uz = Lf(sp + 216), ux = Lf(sp + 208);
            const f64 il = Divs(one, Sqrts(Fmadds(tz, tz, Fmadds(tx, tx, tan2))));
            const f64 nx = Muls(tx, il), ny = Muls(kTanY, il), nz = Muls(tz, il);
            Sf(sp + 224, nx), Sf(sp + 228, ny), Sf(sp + 232, nz);
            const f64 bz = Fmsubs(ux, ny, Muls(uy, nx));
            const f64 bx = Fmsubs(uy, nz, Muls(uz, ny));
            const f64 by = Fmsubs(uz, nx, Muls(ux, nz));
            Sf(sp + 200, bz), Sf(sp + 192, bx), Sf(sp + 196, by);
            const u32 ob = load<u32>(objA);
            ctx.fpr[1] = by;
            VCall(ctx, ob, 84, {ob, sp + 192});  // the frame: rows sp+192 / +208 / +224 / +240
            if (fadeAt > t) {
                const u32 ob2 = load<u32>(objA);
                if (static_cast<u32>(VCall(ctx, ob2, 132, {ob2})) != kHashFade) call(ctx, Obj_SetEffectHash, {load<u32>(objA), kHashFade});
            } else {
                const f64 dy = Subs(Lf(sp + 116), Lf(sp + 244)), dx = Subs(Lf(sp + 112), Lf(sp + 240)), dz = Subs(Lf(sp + 120), Lf(sp + 248));
                const f64 d2 = Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)));
                ctx.fpr[1] = d2;
                if (d2 < kNearSq) {
                    const u32 fl = (load<u16>(sp + 158) & ~0xEF00u) | 0x6000u;
                    const u32 o60 = load<u32>(o + 60);
                    store<u32>(sp + 152, kEventNear);
                    store<u32>(sp + 148, o60);
                    store<u16>(sp + 156, 0);
                    u32 bit = 0;
                    if (o60 != 0) {
                        store<u16>(sp + 158, static_cast<u16>(fl));
                        if ((load<u64>(o60 + 184) & 1) != 0) bit = 1;
                    }
                    store<u16>(sp + 158, static_cast<u16>((fl & ~0x1000u) | (bit << 12)));
                    store<u32>(sp + 144, kVtCamEvent);
                    for (u32 k = 0; k < 3; ++k) store<u32>(sp + 160 + 4 * k, load<u32>(UpVec + 4 * k));
                    store<u32>(sp + 176, 0x447A0000u);  // 1000.0
                    store<u32>(sp + 180, 0);
                    const u32 g5 = load<u32>(load<u32>(Game) + 4208 + 8);
                    const u32 dst = g5 != 0 && (load<u32>(g5 + 192) & 2) != 0 ? g5 : 0;
                    SetF1(ctx, UpVec + 8);
                    VCall(ctx, dst, 344, {dst, sp + 144});
                    call(ctx, Obj_SetEffectHash, {load<u32>(objA), kHashNear});
                    store<u32>(sp + 144, kVtCamEventEnd);
                }
            }
        }
        i = (i + 1) & 0xFF;
        head = load<u8>(o + 1740);
        if (i == head) break;
    }
    ret(ctx, head);
}
