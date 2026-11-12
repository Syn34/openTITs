// Game code, large-function sweep part 26: a pad-driven push / steer tick
// that moves a target object along the left stick, with its animations
// and looping effect.
#include "../sly4.h"
#include "big26.h"

using namespace sly4;
using namespace sly4::big26;

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
constexpr u32 Anim_Restart = 0x002775A8;    // Anim_Restart2775A8 (12 arguments; f1..f4)
constexpr u32 Mat34_FromRotPos = 0x00553B4C;  // (Mat34* out, const Mat33*, const vec3*)
constexpr u32 Effect_Release = 0x004CF3C4;  // (holder thing, u32 hash)
constexpr u32 kAnimPush = 0x437569E0u;      // started when the push begins
constexpr u32 kAnimNear = 0xBAB5810Bu;      // started when the target is within 0.5
constexpr u32 kAnimFar = 0xBA23BD8Cu;       // started when it is farther
constexpr u32 kAnimIdle = 0x0E984C33u;      // started when the push stops (also the layer key tested)
constexpr u32 kFxPushStart = 0x8CCDF8ACu;   // effect released when the push begins
constexpr u32 kFxPushStop = 0x6BCCB3E7u;    // effect released when it stops

f64 C(u32 off) { return Lf(kC411 + off); }

// |a - b| for the vec3s at a / b, in the compiled order (y first, then
// fmadds x, fmadds z; single rounding at each step).
f64 Dist3(f64 dx, f64 dy, f64 dz) { return Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)))); }

// Anim_Restart2775A8(an, key, 0, 0, r7, r8, 0, 0, -, -, 0, -1; 1, 0.25,
// 0, 0). Arguments 9 / 10 (r1 +112 / +120) are never written by the
// original; the callee does not read them.
void Restart(PPUContext& ctx, u32 an, u32 key, u64 r7, u64 r8) {
    callf(ctx, Anim_Restart, {an, key, 0, 0, r7, r8, 0, 0, 0, 0, 0, ~0ull}, {C(4), C(28), C(0), C(0)});
}

// The 3x3 of t (+96 / +112 / +128 rows, x/y/z words) to m (rows 16 apart),
// the position pos, then t->vfn[84](t, Mat34_FromRotPos(out, m, pos)).
// f1 is left as the last word copied (t+136), as the original has it.
void PlaceAt(PPUContext& ctx, u32 t, u32 m, u32 pos, u32 out) {
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k) store<u32>(m + 16 * r + 4 * k, load<u32>(t + 96 + 16 * r + 4 * k));
    ctx.fpr[1] = Lf(t + 136);
    call(ctx, Mat34_FromRotPos, {out, m, pos});
    VCall(ctx, t, 84, {t, out});
}

u32 Animator(PPUContext& ctx, u32 t) { return static_cast<u32>(VCall(ctx, t, 60, {t})); }
}  // namespace

// T* f(Pusher* o; f32 dt) (called by 00411D30, 00412640). o->+92 the
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
//    cleared. Returns 0.
// 4. Otherwise a running push stops (+228 = side speed, animation
//    0x0E984C33, effect 0x6BCCB3E7); else, with no animation playing,
//    0xBAB5810B (within 0.5) or 0xBA23BD8C starts and +465 is set; with
//    the approach on the target is pulled toward the anchor by up to
//    6 dt per tick. Then +160 = -1 and -1 is returned.
DECOMP(00411088, Pusher_PadTick_00411088) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    Stack frame(ctx, 496);
    const u32 sp = frame.addr();
    const u32 pad = kPads + static_cast<u32>(load<u8>(load<u32>(b26fn::Game) + kPadIndex)) * 11968u + 176u;

    // 1. The stick.
    call(ctx, Obj_GetChannel, {pad, 0});
    const f64 cx = ctx.fpr[1];
    call(ctx, Obj_GetChannel, {pad, 1});
    const f64 cy = ctx.fpr[1];
    const f64 zero = C(0);
    const f64 ang = callf(ctx, Atan2f, {}, {cy, cx});
    const f64 ac = std::fabs(callf(ctx, Math_Cosf, {}, {ang}));
    const f64 as = std::fabs(callf(ctx, Math_Sinf, {}, {ang}));
    const f64 cs = ac > as ? callf(ctx, Math_Cosf, {}, {ang}) : callf(ctx, Math_Sinf, {}, {ang});
    const f64 k = std::fabs(cs);
    f64 fy = Muls(cy, k), fx = Muls(cx, k);
    const f64 one = C(4);
    const f64 len = Sqrts(Fmadds(Muls(zero, k), Muls(zero, k), Fmadds(fx, fx, Muls(fy, fy))));
    const f64 mag = len < one ? len : one;
    if (mag >= C(8)) {
        const f64 inv = Divs(one, len);
        fx = Muls(fx, inv);
        fy = Muls(fy, inv);
        ctx.fpr[1] = inv;
    } else {
        ctx.fpr[1] = len;
    }

    // 2. The approach test.
    const u32 tgt0 = load<u32>(o + 92);
    const u32 layer = static_cast<u32>(call(ctx, Anim_GetLayer, {Animator(ctx, tgt0), 0}));
    const u32 t = load<u32>(o + 92);
    const f64 px = Lf(o + 128), py = Lf(o + 132), pz = Lf(o + 136);
    ctx.fpr[1] = px;
    bool atAnchor = true;
    if (load<u8>(o + 465) != 0 || load<u32>(load<u32>(layer + 4) + 8) == kAnimIdle) {
        const f64 d = Dist3(Subs(px, Lf(t + 144)), Subs(py, Lf(t + 148)), Subs(pz, Lf(t + 152)));
        if (d > C(12)) atAnchor = false;
    }
    const f64 tenth = C(12);
    bool push = false;
    if (mag > tenth && atAnchor) {
        bool state3 = false;
        for (u32 i = 0; i < load<u16>(o + 172); ++i)
            if (load<s32>(load<u32>(load<u32>(o + 164) + 4 * i) + 84) == 3) state3 = true;
        push = state3 && fy > tenth;
    }
    const u8 b464 = load<u8>(o + 464);
    if (!push && b464 != 0 && atAnchor && mag > tenth) push = true;

    if (push) {
        // 3. Pushing.
        const f64 fwd = fy <= zero ? zero : fy;
        if (load<u8>(o + 204) == 0) {
            store<u8>(o + 204, 1);
            store<u32>(o + 208, 0);
            store<u32>(o + 212, 0);
        }
        const f64 five = C(16), three = C(20);
        f64 vf = Fmadds(Muls(dt, fwd), five, Lf(o + 208));
        Sf(o + 208, vf);
        if (vf > three) {
            store<u32>(o + 208, 0x40400000u);
            vf = three;
        }
        f64 vs = Fmadds(Muls(dt, fx), five, Lf(o + 212));
        Sf(o + 212, vs);
        if (vs > three) {
            store<u32>(o + 212, 0x40400000u);
            vs = three;
        }
        if (vs < C(24)) {
            store<u32>(o + 212, 0xC0400000u);
            vs = C(24);
        }
        const f64 ax = -Lf(o + 144), ay = -Lf(o + 148), az = -Lf(o + 152);
        const f64 rx = Muls(Lf(t + 96), vs), ry = Muls(Lf(t + 100), vs), rz = Muls(Lf(t + 104), vs);
        Sf(sp + 144, Adds(Fmsubs(ax, vf, rx), px));
        Sf(sp + 148, Adds(Fmsubs(ay, vf, ry), py));
        Sf(sp + 152, Adds(Fmsubs(az, vf, rz), pz));
        PlaceAt(ctx, t, sp + 224, sp + 144, sp + 336);
        if (load<u8>(o + 464) == 0) Restart(ctx, Animator(ctx, load<u32>(o + 92)), kAnimPush, ~0ull, 0);
        store<u8>(o + 464, 1);
        const u32 h = load<u32>(o + 192);
        const u32 fx = static_cast<u32>(VCall(ctx, h, 272, {h}));
        call(ctx, Effect_Release, {fx, kFxPushStart});
        store<u8>(o + 465, 0);
        store<u8>(o + 466, 0);
        ctx.gpr[3] = 0;
        return;
    }

    // 4. Not pushing.
    if (b464 != 0) {
        store<u8>(o + 204, 0);
        store<u32>(o + 228, load<u32>(o + 212));
        ctx.fpr[1] = Lf(o + 212);
        const u32 an = Animator(ctx, t);
        Restart(ctx, an, kAnimIdle, ~0ull, ctx.gpr[8]);  // r8: what the vcall left (never set)
        store<u8>(o + 464, 0);
        store<u8>(o + 466, 0);
        const u32 h = load<u32>(o + 192);
        const u32 fx = static_cast<u32>(VCall(ctx, h, 272, {h}));
        call(ctx, Effect_Release, {fx, kFxPushStop});
    } else {
        bool far = false;  // 4117C8: start the approach animation
        if (static_cast<u32>(call(ctx, Anim_Find, {Animator(ctx, t), 0})) == 0 &&
            static_cast<u32>(call(ctx, Anim_TrackSet36, {Animator(ctx, load<u32>(o + 92)), 0, 0})) != 0 && load<u8>(o + 465) == 0) {
            far = true;
        }
        if (far) {
            const u32 q = load<u32>(o + 92);
            const f64 d = Dist3(Subs(Lf(q + 144), Lf(o + 128)), Subs(Lf(q + 148), Lf(o + 132)), Subs(Lf(q + 152), Lf(o + 136)));
            ctx.fpr[1] = d;
            if (d < C(40)) {
                Restart(ctx, Animator(ctx, q), kAnimNear, 0, ~0ull);
                store<u8>(o + 466, 1);
            } else {
                const u32 an = Animator(ctx, q);
                Restart(ctx, an, kAnimFar, ~0ull, ctx.gpr[8]);  // r8: what the vcall left (never set)
                store<u8>(o + 466, 0);
            }
            store<u8>(o + 465, 1);
        } else if (static_cast<u32>(call(ctx, Anim_Find, {Animator(ctx, load<u32>(o + 92)), 0})) != 0 && load<u8>(o + 465) != 0) {
            const u32 q = load<u32>(o + 92);
            const f64 d = Dist3(Subs(Lf(q + 144), Lf(o + 128)), Subs(Lf(q + 148), Lf(o + 132)), Subs(Lf(q + 152), Lf(o + 136)));
            ctx.fpr[1] = d;
            if (d < C(40) && load<u8>(o + 466) == 0) {
                Restart(ctx, Animator(ctx, q), kAnimNear, 0, ~0ull);
                store<u8>(o + 466, 1);
            }
        }
    }
    if (load<u8>(o + 465) != 0) {
        // The approach: pull the target toward the anchor.
        const u32 q = load<u32>(o + 92);
        f64 step = Muls(Muls(tenth, dt), C(32));
        const f64 qx = Lf(q + 144), qy = Lf(q + 148), qz = Lf(q + 152);
        const f64 dx = Subs(Lf(o + 128), qx), dy = Subs(Lf(o + 132), qy), dz = Subs(Lf(o + 136), qz);
        ctx.fpr[1] = qx;
        const f64 d = Dist3(dx, dy, dz);
        if (d > C(36)) {
            if (d < step) step = d;
            const f64 inv = Divs(one, d);
            Sf(sp + 160, Fmadds(Muls(dx, inv), step, qx));
            Sf(sp + 164, Fmadds(Muls(dy, inv), step, qy));
            Sf(sp + 168, Fmadds(Muls(dz, inv), step, qz));
            PlaceAt(ctx, q, sp + 176, sp + 160, sp + 272);
        }
    }
    store<s32>(o + 160, -1);
    ctx.gpr[3] = ~0ull;
}
