// Game code, large-function sweep part 28: a game object's ring of spline
// effects (spawned, aged and placed along five splines, faded, triggering a
// camera event near the camera) and its camera target along spline 2.
#include "big28.h"

namespace sly4::obj {

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
constexpr u32 GameLayers = 0x001281E0;                // Game +0x1281E0: the camera layer object
constexpr u32 UpVec = 0x01164B60;                     // the global up vector
constexpr u32 kVtCamTarget = 0x00B13B38;              // 112-byte camera target
constexpr u32 kVtCamEvent = 0x00B0DADC;               // camera event on the stack
constexpr u32 kVtCamEventEnd = 0x00B0DABC;            // stored over it after the post
constexpr u32 kHashSpawn = 0x76F894FCu;               // effect hash: spawned
constexpr u32 kHashFade = 0x879F2BDAu;                // effect hash: fading
constexpr u32 kHashNear = 0x00011773u;                // effect hash: passed near the camera
constexpr u32 kEventNear = 0x2DA0A51Du;               // the event's hash
f64 Tw(u32 base) { return lf(global<be_f32>(base + 72)); }
// fsel / frsp / fsel: x clamped to [kLo, kHi] by 1 - x >= 0 then x >= 0 (NaN: kHi, then kept).
f64 Clamp01(f64 one, f64 x) {
    const f64 a = S(Subs(one, x) >= 0.0 ? x : kHi);
    return a >= 0.0 ? a : kLo;
}

// A fade controller (an object's +128 block +56): +0, +32 bits 27..20 the
// mode, +36 the fade value.
struct FxCtl {
    be_u32 w0;     // +0
    pad<28> _4;
    be_u32 mode;   // +32
    be_f32 fade;   // +36
};
static_assert(offsetof(FxCtl, fade) == 36);
// The 112-byte camera target: +4 its layer object, the look frame (+16
// right, +32 up, +48 direction, +64 eye), +80 the field of view.
struct CamTarget112 {
    be_u32 vtable;   // +0
    be_u32 layer;    // +4
    u8 b8;           // +8
    pad<7> _9;
    Vec4 right;      // +16
    Vec4 up;         // +32
    Vec4 dir;        // +48
    Vec4 eye;        // +64
    be_u32 fov;      // +80 (raw)
    be_u32 w84;      // +84
    be_u32 w88;      // +88: bits 30..23 = 1, bit 31 cleared
    be_u32 w92;      // +92
    be_u32 w96;      // +96
    be_u32 w100;     // +100
    pad<8> _104;
};
static_assert(sizeof(CamTarget112) == 112 && offsetof(CamTarget112, fov) == 80);
// The camera event posted near the camera (on the stack).
struct CamEvent {
    be_u32 vtable;   // +0
    be_u32 obj;      // +4
    be_u32 hash;     // +8
    be_u16 h12;      // +12
    be_u16 flags;    // +14
    Vec3 up;         // +16
    pad<4> _28;
    be_u32 range;    // +32 (raw)
    be_u32 w36;      // +36
    pad<8> _40;
};
static_assert(sizeof(CamEvent) == 48);
struct SplineFxObj {
    pad<36> _0;
    be_f32 cur;          // +36
    pad<20> _40;
    be_u32 o60;          // +60
    pad<244> _64;
    be_s32 mode;         // +308
    pad<1424> _312;
    be_f32 timer;        // +1736
    u8 head;             // +1740
    u8 tail;             // +1741
    u8 on;               // +1742
    pad<9> _1743;
    be_u32 groups;       // +1752: its +160 -> +208 the group block
    be_u32 objs[256];    // +1756
    be_f32 age[256];     // +2780
    be_u32 idx[256];     // +3804
    u8 b[256];           // +4828
    pad<4> _5084;
    be_u32 life;         // +5088: its +120 the spline length
    pad<12> _5092;
    be_u32 splines[5];   // +5104
    be_u32 target;       // +5124: CamTarget112*
    be_u32 path;         // +5128
};
static_assert(offsetof(SplineFxObj, mode) == 308 && offsetof(SplineFxObj, timer) == 1736 && offsetof(SplineFxObj, objs) == 1756);
static_assert(offsetof(SplineFxObj, b) == 4828 && offsetof(SplineFxObj, life) == 5088 && offsetof(SplineFxObj, splines) == 5104);
static_assert(offsetof(SplineFxObj, path) == 5128);
// The original's locals (sp offsets - 112).
struct FxLocals {
    Vec4 campos;     // sp+112
    Vec4 pt;         // sp+128
    CamEvent ev;     // sp+144
    Mtx44 frame;     // sp+192: right, up, tangent, point
    be_f32 t;        // sp+256
};
static_assert(offsetof(FxLocals, frame) == 80 && offsetof(FxLocals, t) == 144);

void Look(Vec4& r, f64 x, f64 y, f64 z) {
    r.x = static_cast<f32>(x), r.y = static_cast<f32>(y), r.z = static_cast<f32>(z);
}
}  // namespace

// void f(GameObj* o; f32 dt) (called by f_00056468, f_0005A6A0). Effects
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
// No caller reads r3 or f1 (r3.sh).
void Obj_UpdateSplineFx_00054880(SplineFxObj* o, f32 dt_) {
    f64 dt = dt_;  // f18
    GuestFrame frame(512);
    FxLocals& L = *frame.as<FxLocals>(112);
    const f64 one = kOne;  // f19
    f64 scale = kOne, limit = kLimit0;
    f64 span = Tw(kTwSpan), rate = Tw(kTwRate);  // f15, f31
    f64 cur = lf(o->cur);                       // f3
    if (o->mode == 8) {
        span = Tw(kTwSpan8);
        rate = Tw(kTwRate8);
        scale = Tw(kTwSpeed8);
        limit = kLimit8;
    }

    // 1. The controllers.
    const bool fading = cur < limit && o->on != 0;
    if (fading) dt = Muls(dt, scale);
    {
        const u32 grp = gm<be_u32>(o->groups + 160);
        for (u32 i = 0;; ++i) {
            u32 g = gm<be_u32>(grp + 208);
            if (!(i < gm<u8>(g + 112))) break;
            for (u32 j = 0;; ++j) {
                if (!(j < gm<be_u16>(gm<be_u32>(g + 8 * i + 132) + 52))) break;
                const u32 list = gm<be_u32>(gm<be_u32>(g + 8 * i + 132) + 48);
                const u32 m = gm<be_u32>(gm<be_u32>(list + 8 * j + 4) + 128);
                if (m == 0 || m + 56 == 0 || !(cur < limit)) continue;
                FxCtl& c = gm<FxCtl>(m + 56);
                const u32 w = (c.mode & ~0x0FF00000u) | (2u << 20);
                c.w0 = 0u;
                if (fading) {
                    c.mode = w;
                    cur = lf(o->cur);
                    c.fade = static_cast<f32>(Muls(Divs(cur, limit), kFade));
                } else {
                    set_bits(c.fade, 0xBF000000u);  // -0.5
                    c.mode = w;
                    cur = lf(o->cur);
                }
                g = gm<be_u32>(grp + 208);
            }
        }
    }
    for (u32 k = 0; k < 5; ++k) vcall(host<u8>(o->splines[k]), 40, fading ? 0u : 1u, 0u);

    // 2. Spawning.
    call(Game_BuildEffectSplines, o);
    const f64 acc = Adds(lf(o->timer), dt);
    o->timer = static_cast<f32>(acc);
    if (acc > Divs(kSpawnK, rate)) {
        const u32 head = o->head, tail = o->tail;
        o->timer = 0.0f;
        if (((head + 1) & 0xFF) != tail) {
            call(Obj_SetEffectHash, o->objs[head].get(), kHashSpawn);
            const u32 h1 = o->head;
            be_u32& state = global<be_u32>(kGameRandState);
            const u32 st = state;
            const u32 mix = GameRandMix(st);
            state = std::rotl(mix, 16);
            global<be_u64>(kGameRandLast) = static_cast<u64>(mix & 0xFFFFu);
            o->age[h1] = 0.0f;
            const u32 h2 = o->head;
            o->head = static_cast<u8>(h2 + 1);
            o->b[h2] = 0;
            o->idx[h2] = st % 5;
        }
    }

    // 3. The camera and its target.
    const auto camera = [](u32 need) -> u32 {
        const u32 g5 = gm<be_u32>(global<be_u32>(kWorld) + 4208 + 8);
        return g5 != 0 && (gm<be_u32>(g5 + 192) & need) == need ? g5 : 0u;
    };
    const u32 cam = camera(10u);  // r26: bits 2 and 8
    call_ptr(vfn(&gm<u8>(cam), 80), &L.campos, cam);  // the camera position (a null camera reads the vtable at 0)
    const Vec3& upv = global<Vec3>(UpVec);
    set_bits(L.frame.r[1].x, bits(upv.x)), set_bits(L.frame.r[1].y, bits(upv.y)), set_bits(L.frame.r[1].z, bits(upv.z));
    for (u32 k = 0; k < 5; ++k) call(SplineInstance_Nearest, o->splines[k].get(), &L.campos, 0u, 0u);
    const u32 lay = gm<be_u32>(global<be_u32>(kWorld) + GameLayers);  // r24
    if (lay != 0 && global<u8>(kTwCamOn + 72) != 0) {
        bool follow = vcall<u32>(&gm<u8>(cam), 64) == 0;
        if (!follow) follow = ((gm<be_u32>(vcall<u32>(&gm<u8>(cam), 64) + 416) >> 28) & 7) != 1;
        if (!follow) {
            if (const u32 t = o->target; t != 0) {
                call(Obj_PopLayer, lay, t, Tw(kTwBlend));
                o->target = 0u;
                o->path = 0u;
            }
        } else {
            const u32 pose = call<u32>(Obj_GetPathPose, cam, &L.t, 0u, 0u);
            u32 path = o->path;
            const u32 sp2 = o->splines[2];
            if (pose != 0) {
                path = pose;
                o->path = pose;
            }
            if (path != 0) {
                o->path = sp2;
                const f64 near = call<f64>(SplineInstance_Nearest, sp2, cam + 144, 0u, 0u);
                L.t = static_cast<f32>(near);
                const u32 s = o->path;
                const f64 lead = Divs(Tw(kTwLead), lf(gm<be_f32>(gm<be_u32>(s + 208) + 120)));
                const Vec3& cp = gm<Vec3>(cam + 144);
                f64 ex = lf(cp.x), ey = lf(cp.y), ez = lf(cp.z);  // f23 / f24 / f25
                const f64 u = S(Clamp01(one, Adds(near, lead)));
                call(WorldObj_SplinePointAt, s, 0u, &L.pt, 0u, u);
                const f64 px = lf(L.pt.x), pz = lf(L.pt.z);
                f64 py = lf(L.pt.y);
                const u32 tgt = o->target;
                if (global<u8>(kTwCamY + 72) != 0) py = lf(cp.y);
                py = Adds(py, Tw(kTwYOff));
                f64 dx = Subs(px, ex), dz = Subs(pz, ez), dy = Subs(py, ey);
                L.pt.y = static_cast<f32>(py);
                const f64 ux = lf(upv.x), uz = lf(upv.z), uy = lf(upv.y);
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
                const auto frame12 = [&](CamTarget112& p) {
                    Look(p.up, vx, vy, vz);
                    Look(p.dir, dx, dy, dz);
                    Look(p.eye, ex, ey, ez);
                };
                if (tgt != 0) {
                    CamTarget112& p = gm<CamTarget112>(tgt);
                    Look(p.right, rx, ry, rz);
                    frame12(p);
                } else {
                    const u32 pa = call<u32>(fn::SmallAlloc, 112u);
                    if (pa != 0) {
                        CamTarget112& p = gm<CamTarget112>(pa);
                        p.layer = lay;
                        p.b8 = 0;
                        p.vtable = kVtCamTarget;
                        Look(p.right, rx, ry, rz);
                        const u32 w88 = p.w88;
                        frame12(p);
                        p.fov = 0x425C0000u;  // 55.0
                        p.w88 = ((w88 & 0x7FFFFFFFu) & ~0x7F800000u) | (1u << 23);  // bit 31 cleared, bits 23..30 = 1
                        p.w92 = 0u;
                        p.w96 = 0u;
                        p.w100 = 0u;
                        p.w84 = 0u;
                    }
                    o->target = pa;
                    call(Camera_PushTarget, lay, pa, 0u, 0u, Tw(kTwBlend));
                }
            }
        }
    }

    // 4. The ring.
    const f64 life = lf(gm<be_f32>(o->life + 120));
    const f64 fadeAt0 = Divs(Tw(kTwFadeAt), life);
    span = Divs(life, span);
    u32 i = o->tail;
    u32 head = o->head;
    const f64 fa = S(Subs(one, fadeAt0) >= 0.0 ? fadeAt0 : kHi);
    const f64 fadeAt = S(fa >= 0.0 ? fa : kLo);  // f31
    if (i == head) return;
    const f64 tan2 = Muls(kTanY, kTanY);  // f27
    Mtx44& fr = L.frame;
    for (;;) {
        const f64 age = Adds(lf(o->age[i]), dt);
        const f64 q = Divs(age, span);
        o->age[i] = static_cast<f32>(age);
        const u32 si = o->idx[i];
        const f64 c1 = Clamp01(one, q);
        const f64 t = Subs(one, S(c1));  // f30
        if (t == kTanY) {
            o->tail = static_cast<u8>(o->tail + 1);
        } else {
            call(WorldObj_SplinePointTangentAt, o->splines[si].get(), 0u, &fr.r[3], &fr.r[2], 0u, t);
            const f64 tx = lf(fr.r[2].x), tz = lf(fr.r[2].z);
            const f64 uy = lf(fr.r[1].y), uz = lf(fr.r[1].z), ux = lf(fr.r[1].x);
            const f64 il = Divs(one, Sqrts(Fmadds(tz, tz, Fmadds(tx, tx, tan2))));
            const f64 nx = Muls(tx, il), ny = Muls(kTanY, il), nz = Muls(tz, il);
            Look(fr.r[2], nx, ny, nz);
            const f64 bz = Fmsubs(ux, ny, Muls(uy, nx));
            const f64 bx = Fmsubs(uy, nz, Muls(uz, ny));
            const f64 by = Fmsubs(uz, nx, Muls(ux, nz));
            Look(fr.r[0], bx, by, bz);
            vcall(host<u8>(o->objs[i]), 84, &fr);  // the frame: rows sp+192 / +208 / +224 / +240
            if (fadeAt > t) {
                if (vcall<u32>(host<u8>(o->objs[i]), 132) != kHashFade) call(Obj_SetEffectHash, o->objs[i].get(), kHashFade);
            } else {
                const f64 dy = Subs(lf(L.campos.y), lf(fr.r[3].y)), dx = Subs(lf(L.campos.x), lf(fr.r[3].x));
                const f64 dz = Subs(lf(L.campos.z), lf(fr.r[3].z));
                const f64 d2 = Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)));
                if (d2 < kNearSq) {
                    CamEvent& ev = L.ev;
                    const u32 fl = (ev.flags & ~0xEF00u) | 0x6000u;  // over what the stack held
                    const u32 o60 = o->o60;
                    ev.hash = kEventNear;
                    ev.obj = o60;
                    ev.h12 = 0;
                    u32 bit = 0;
                    if (o60 != 0) {
                        ev.flags = static_cast<u16>(fl);
                        if ((gm<be_u32>(o60 + 188) & 1) != 0) bit = 1;  // bit 0 of the u64 at +184
                    }
                    ev.flags = static_cast<u16>((fl & ~0x1000u) | (bit << 12));
                    ev.vtable = kVtCamEvent;
                    set_bits(ev.up.x, bits(upv.x)), set_bits(ev.up.y, bits(upv.y)), set_bits(ev.up.z, bits(upv.z));
                    ev.range = 0x447A0000u;  // 1000.0
                    ev.w36 = 0u;
                    const u32 dst = camera(2u);
                    vcall(&gm<u8>(dst), 344, &ev);
                    call(Obj_SetEffectHash, o->objs[i].get(), kHashNear);
                    ev.vtable = kVtCamEventEnd;
                }
            }
        }
        i = (i + 1) & 0xFF;
        head = o->head;
        if (i == head) break;
    }
}

}  // namespace sly4::obj

NATIVE(00054880, sly4::obj::Obj_UpdateSplineFx_00054880)
