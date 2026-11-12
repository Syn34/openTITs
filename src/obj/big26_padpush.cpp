// Game code, large-function sweep part 26: a pad-driven push / steer tick
// that moves a target object along the left stick, with its animations
// and looping effect.
#include "big26.h"

namespace sly4::obj {

namespace {
constexpr u32 kC411 = 0x0041105C;  // .text f32: 0, 1, 1e-5, 0.1, 5, 3, -3, 0.25, 60, 0.001, 0.5
constexpr u32 kPads = 0x010B19E0;  // pad records of 11968 bytes; +176 the channel block
constexpr u32 kPadIndex = 4209;    // Game +4209: the active pad's index (u8)
constexpr u32 Obj_GetChannel = 0x0045B160;  // Obj_GetChannel_0045B160 (T*, s32 i) -> f1
constexpr u32 Atan2f = 0x005E665C;          // (f1 y, f2 x) -> f1
constexpr u32 Math_Cosf = 0x007344E0;
constexpr u32 Math_Sinf = 0x007345F0;
constexpr u32 Anim_GetLayer = 0x002541C0;   // Anim_GetLayer2541C0 (Animator*, key) -> layer or 0
constexpr u32 Anim_Find = 0x00298874;       // Anim_Find298874 (Animator*, key) -> u8
constexpr u32 Anim_TrackSet36 = 0x0029879C; // Anim_TrackSet36_0029879C (Obj*, s32 id, u8 v) -> bool
constexpr u32 Anim_Restart = 0x002775A8;    // Anim_Restart2775A8 (an, key, a, b; f, f, c, d; f, f, e, g)
constexpr u32 Mat34_FromRotPos = 0x00553B4C;  // (Mat34* out, const Mat33*, const vec3*)
constexpr u32 Effect_Release = 0x004CF3C4;  // (holder thing, u32 hash)
constexpr u32 kAnimPush = 0x437569E0u;      // started when the push begins
constexpr u32 kAnimNear = 0xBAB5810Bu;      // started when the target is within 0.5
constexpr u32 kAnimFar = 0xBA23BD8Cu;       // started when it is farther
constexpr u32 kAnimIdle = 0x0E984C33u;      // started when the push stops (also the layer key tested)
constexpr u32 kFxPushStart = 0x8CCDF8ACu;   // effect released when the push begins
constexpr u32 kFxPushStop = 0x6BCCB3E7u;    // effect released when it stops

// The pushed target: +96 its rotation rows, +144 its position.
struct PushTarget {
    pad<96> _0;
    Mtx44 m;  // +96 (row 3 = the position, +144)
};
struct Pusher {
    pad<92> _0;
    be_u32 target;      // +92: PushTarget* (vfn[60] its animator, vfn[84] place)
    pad<32> _96;
    Vec3 anchor;        // +128
    pad<4> _140;
    Vec3 axis;          // +144
    pad<4> _156;
    be_s32 w160;        // +160
    be_u32 things;      // +164: T*[] (each +84 a state)
    pad<4> _168;
    be_u16 nthings;     // +172
    pad<18> _174;
    be_u32 holder;      // +192 (vfn[272] the effect)
    pad<8> _196;
    u8 pushing;         // +204
    pad<3> _205;
    be_f32 vf;          // +208
    be_f32 vs;          // +212
    pad<12> _216;
    be_u32 last;        // +228
    pad<232> _232;
    u8 b464;            // +464: push animation on
    u8 b465;            // +465: approach started
    u8 b466;            // +466: near animation on
};
static_assert(offsetof(Pusher, anchor) == 128 && offsetof(Pusher, things) == 164 && offsetof(Pusher, holder) == 192);
static_assert(offsetof(Pusher, vf) == 208 && offsetof(Pusher, last) == 228 && offsetof(Pusher, b464) == 464);
f64 C(u32 off) { return lf(global<be_f32>(kC411 + off)); }

// |d| in the compiled order (y first, then fmadds x, fmadds z).
f64 Dist3(f64 dx, f64 dy, f64 dz) { return Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)))); }

// Anim_Restart2775A8(an, key, 0, 0; 1, 0.25, 0, 0; 0, 0, 0, -1).
void Restart(u32 an, u32 key) { call(Anim_Restart, an, key, 0u, 0u, C(4), C(28), 0u, 0u, C(0), C(0), 0u, -1); }

// The 3x3 of t (x/y/z words) to m, then t->vfn[84](t, Mat34_FromRotPos(out, m, pos)).
void PlaceAt(u32 t, Mtx44* m, const Vec4* pos, Mtx44* out) {
    const Mtx44& tm = gm<PushTarget>(t).m;
    for (u32 r = 0; r < 3; ++r) CopyXYZ(m->r[r], tm.r[r]);
    call(Mat34_FromRotPos, out, m, pos);
    vcall(&gm<u8>(t), 84, out);
}

u32 Animator(u32 t) { return vcall<u32>(&gm<u8>(t), 60); }
}  // namespace

// void f(Pusher* o; f32 dt) (called by 00411D30, 00412640). o->+92 the
// target (vfn[60] its animator, vfn[84] place by matrix; +96.. its
// rotation rows, +144 its position), +128 the anchor point, +144 the push
// axis, +164 / +172 (u16) an array of things whose +84 is a state, +192
// the effect holder (vfn[272]), +204 pushing (u8) with the speeds +208
// (forward) / +212 (side), +228 the last side speed, +464 push animation
// on, +465 approach started, +466 near animation on (u8s).
// 1. The stick (channels 0 / 1 of the active pad) is squared off: scaled
//    by the larger of |cos| / |sin| of its angle, its length capped at 1
//    (below 1e-5 it is left unnormalized).
// 2. The approach test: unless +465 is clear and the animator's layer 0
//    is not 0x0E984C33, the target must be within 0.1 of the anchor.
// 3. Pushing goes on with a stick over 0.1, the target at the anchor and
//    either +464 or (a thing in state 3 and forward over 0.1): the speeds
//    grow by dt * stick * 5 (clamped to 3 / [-3, 3]) and the target moves
//    by -axis * forward - row0 * side from the anchor; the first time
//    starts 0x437569E0 and releases effect 0x8CCDF8AC; +465 / +466
//    cleared.
// 4. Otherwise a running push stops (+228 = side speed, animation
//    0x0E984C33, effect 0x6BCCB3E7); else, with no animation playing,
//    0xBAB5810B (within 0.5) or 0xBA23BD8C starts and +465 is set; with
//    the approach on the target is pulled toward the anchor by up to
//    6 dt per tick. Then +160 = -1. No caller reads
// r3 or f1 (r3.sh).
void Pusher_PadTick_00411088(Pusher* o, f32 dt_) {
    const f64 dt = dt_;
    GuestFrame frame(496);
    Vec4* pos = frame.as<Vec4>(144);
    Vec4* pos2 = frame.as<Vec4>(160);
    Mtx44* m2 = frame.as<Mtx44>(176);
    Mtx44* m1 = frame.as<Mtx44>(224);
    Mtx44* out2 = frame.as<Mtx44>(272);
    Mtx44* out1 = frame.as<Mtx44>(336);
    const u32 pad = kPads + static_cast<u32>(gm<u8>(global<be_u32>(kWorld) + kPadIndex)) * 11968u + 176u;

    // 1. The stick.
    const f64 cx = call<f64>(Obj_GetChannel, pad, 0);
    const f64 cy = call<f64>(Obj_GetChannel, pad, 1);
    const f64 zero = C(0);
    const f64 ang = call<f64>(Atan2f, cy, cx);
    const f64 ac = std::fabs(call<f64>(Math_Cosf, ang));
    const f64 as = std::fabs(call<f64>(Math_Sinf, ang));
    const f64 cs = ac > as ? call<f64>(Math_Cosf, ang) : call<f64>(Math_Sinf, ang);
    const f64 k = std::fabs(cs);
    f64 fy = Muls(cy, k), fx = Muls(cx, k);
    const f64 one = C(4);
    const f64 len = Sqrts(Fmadds(Muls(zero, k), Muls(zero, k), Fmadds(fx, fx, Muls(fy, fy))));
    const f64 mag = len < one ? len : one;
    if (mag >= C(8)) {
        const f64 inv = Divs(one, len);
        fx = Muls(fx, inv);
        fy = Muls(fy, inv);
    }

    // 2. The approach test.
    const u32 layer = call<u32>(Anim_GetLayer, Animator(o->target), 0u);
    const u32 t = o->target;
    const f64 px = lf(o->anchor.x), py = lf(o->anchor.y), pz = lf(o->anchor.z);
    const Mtx44& tm = gm<PushTarget>(t).m;
    bool atAnchor = true;
    if (o->b465 != 0 || gm<be_u32>(gm<be_u32>(layer + 4) + 8) == kAnimIdle) {
        const f64 d = Dist3(Subs(px, lf(tm.r[3].x)), Subs(py, lf(tm.r[3].y)), Subs(pz, lf(tm.r[3].z)));
        if (d > C(12)) atAnchor = false;
    }
    const f64 tenth = C(12);
    bool push = false;
    if (mag > tenth && atAnchor) {
        bool state3 = false;
        for (u32 i = 0; i < o->nthings; ++i)
            if (gm<be_s32>(gm<be_u32>(o->things + 4 * i) + 84) == 3) state3 = true;
        push = state3 && fy > tenth;
    }
    const u8 b464 = o->b464;
    if (!push && b464 != 0 && atAnchor && mag > tenth) push = true;

    if (push) {
        // 3. Pushing.
        const f64 fwd = fy <= zero ? zero : fy;
        if (o->pushing == 0) {
            o->pushing = 1;
            o->vf = 0.0f;
            o->vs = 0.0f;
        }
        const f64 five = C(16), three = C(20);
        f64 vf = Fmadds(Muls(dt, fwd), five, lf(o->vf));
        o->vf = static_cast<f32>(vf);
        if (vf > three) {
            set_bits(o->vf, 0x40400000u);
            vf = three;
        }
        f64 vs = Fmadds(Muls(dt, fx), five, lf(o->vs));
        o->vs = static_cast<f32>(vs);
        if (vs > three) {
            set_bits(o->vs, 0x40400000u);
            vs = three;
        }
        if (vs < C(24)) {
            set_bits(o->vs, 0xC0400000u);
            vs = C(24);
        }
        const f64 ax = -lf(o->axis.x), ay = -lf(o->axis.y), az = -lf(o->axis.z);
        const f64 rx = Muls(lf(tm.r[0].x), vs), ry = Muls(lf(tm.r[0].y), vs), rz = Muls(lf(tm.r[0].z), vs);
        pos->x = static_cast<f32>(Adds(Fmsubs(ax, vf, rx), px));
        pos->y = static_cast<f32>(Adds(Fmsubs(ay, vf, ry), py));
        pos->z = static_cast<f32>(Adds(Fmsubs(az, vf, rz), pz));
        PlaceAt(t, m1, pos, out1);
        if (o->b464 == 0) Restart(Animator(o->target), kAnimPush);
        o->b464 = 1;
        call(Effect_Release, vcall<u32>(&gm<u8>(o->holder), 272), kFxPushStart);
        o->b465 = 0;
        o->b466 = 0;
        return;
    }

    // 4. Not pushing.
    auto distTo = [&](u32 q) {
        const Mtx44& qm = gm<PushTarget>(q).m;
        return Dist3(Subs(lf(qm.r[3].x), lf(o->anchor.x)), Subs(lf(qm.r[3].y), lf(o->anchor.y)), Subs(lf(qm.r[3].z), lf(o->anchor.z)));
    };
    if (b464 != 0) {
        o->pushing = 0;
        o->last = bits(o->vs);
        Restart(Animator(t), kAnimIdle);
        o->b464 = 0;
        o->b466 = 0;
        call(Effect_Release, vcall<u32>(&gm<u8>(o->holder), 272), kFxPushStop);
    } else {
        bool far = false;  // 4117C8: start the approach animation
        if (call<u32>(Anim_Find, Animator(t), 0u) == 0 && call<u32>(Anim_TrackSet36, Animator(o->target), 0, 0u) != 0 && o->b465 == 0)
            far = true;
        if (far) {
            const u32 q = o->target;
            const f64 d = distTo(q);
            if (d < C(40)) {
                Restart(Animator(q), kAnimNear);
                o->b466 = 1;
            } else {
                Restart(Animator(q), kAnimFar);
                o->b466 = 0;
            }
            o->b465 = 1;
        } else if (call<u32>(Anim_Find, Animator(o->target), 0u) != 0 && o->b465 != 0) {
            const u32 q = o->target;
            const f64 d = distTo(q);
            if (d < C(40) && o->b466 == 0) {
                Restart(Animator(q), kAnimNear);
                o->b466 = 1;
            }
        }
    }
    if (o->b465 != 0) {
        // The approach: pull the target toward the anchor.
        const u32 q = o->target;
        const Mtx44& qm = gm<PushTarget>(q).m;
        f64 step = Muls(Muls(tenth, dt), C(32));
        const f64 qx = lf(qm.r[3].x), qy = lf(qm.r[3].y), qz = lf(qm.r[3].z);
        const f64 dx = Subs(lf(o->anchor.x), qx), dy = Subs(lf(o->anchor.y), qy), dz = Subs(lf(o->anchor.z), qz);
        const f64 d = Dist3(dx, dy, dz);
        if (d > C(36)) {
            if (d < step) step = d;
            const f64 inv = Divs(one, d);
            pos2->x = static_cast<f32>(Fmadds(Muls(dx, inv), step, qx));
            pos2->y = static_cast<f32>(Fmadds(Muls(dy, inv), step, qy));
            pos2->z = static_cast<f32>(Fmadds(Muls(dz, inv), step, qz));
            PlaceAt(q, m2, pos2, out2);
        }
    }
    o->w160 = -1;
}

}  // namespace sly4::obj

NATIVE(00411088, sly4::obj::Pusher_PadTick_00411088)
