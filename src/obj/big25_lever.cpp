// Game code, large-function sweep part 25: a lever / sliding switch prop's tick
// (sibling of the sway prop tick 003FA110 in big24_sway.cpp).
#include "../sly4.h"
#include "big25.h"

using namespace sly4;
using namespace sly4::big25;

namespace {
constexpr u32 kCLever = 0x003FAF34;  // .text f32 table (0.5, -0.6, 0.6, -0.2, 0.2, 1, 0, -1, -pi, 180, pi, -pi/4, pi/4)
constexpr u32 kHashFast = 0x2874B3ADu;   // effect while |speed| > 0.6
constexpr u32 kHashMove = 0x287AC552u;   // effect while |speed| > 0.2
constexpr u32 kHashEnd = 0xEA31D667u;    // effect when the travel first reaches an end
constexpr u32 kPointKey = 0xD906B0C2u;   // PointMap key of the reference frame object
constexpr u32 kChildHash = 0x435538CBu;  // the child that gets vfn[88](0, 1, 1, 1)

f64 C(u32 off) { return Lf(kCLever + off); }

// holder->vfn[272]() gives a thing whose +4 is the current effect (its +8
// the hash); unless that is already `hash`, Obj_SetEffectHash(o->+292,
// hash). Returns whether the hash was already current.
bool EnsureEffect(PPUContext& ctx, u32 o, u32 holder, u32 hash, bool reload_f1) {
    const u32 r = static_cast<u32>(VCall(ctx, holder, 272, {holder}));
    const u32 e = load<u32>(r + 4);
    const u32 obj = load<u32>(o + 292);
    if (reload_f1) ctx.fpr[1] = Lf(o + 108);  // as the original leaves f1 for the call
    const u32 cur = e != 0 ? load<u32>(e + 8) : 0;
    if (cur == hash) return true;
    call(ctx, b25fn::Obj_SetEffectHash, {obj, hash});
    return false;
}

// The end-of-travel latch: unless +353, (when +352 is still clear)
// Effect_SetHash4CF988(holder->vfn[272](), 0xEA31D667); then +352 = 1 and
// the vector +336 updated by upd. Returns the f1 the original carries on.
template <typename Upd>
f64 Latch(PPUContext& ctx, u32 o, f64 f1, Upd upd) {
    if (load<u8>(o + 353) != 0) return f1;
    if (load<u8>(o + 352) == 0) {
        const u32 h = load<u32>(o + 292);
        const u32 r = static_cast<u32>(VCall(ctx, h, 272, {h}));
        call(ctx, b25fn::Effect_SetHash, {r, kHashEnd});
        f1 = Lf(o + 104);
    }
    upd();
    return f1;
}
}  // namespace

// void f(Lever* o; f32 dt) (called by 003FC468, 003FD8D0). o->+88 params
// (+24 push gain, +28 damping), +104 travel x in [-1, 1], +108 speed v,
// +112 mass, +304 push, +292 the effect holder, +256 the body, +300 the
// model, +352 latched (u8), +353 latch disabled (u8).
// 1. Unless latched: a = (push * gain - v * damping) / mass; x += v dt +
//    a/2 dt^2, v += a dt (fmadds order kept). |v| > 0.6 ensures effect
//    0x2874B3AD (and, when it already was, falls through to the next
//    test); |v| > 0.2 ensures 0x287AC552. x past +1 / -1 is clamped there,
//    the outward speed zeroed, and (unless +353) the first time fires
//    0xEA31D667 and latches +352; the vector +336 is negated around
//    (0, 1, 0) at +1 (-v + (0, 1, 0)) and offset by (0, 1, 0) at -1.
// 2. The angle +124: a = -angle pi / 180 (kept so that a + angle pi / 180
//    stays within +-pi/4). Without the game's reference object (0x00F65CA0
//    -> +0x1281E0) that's all.
// 3. Otherwise the quaternion about the normalized 0x01164B70 by -a (half
//    angle -a/2) through Quat_ToMatrix, times the frame of the PointMap
//    object 0xD906B0C2 (+16; none: done) after o->vfn[76](); the body's
//    matrix +96 transforms the point +224 into the translation; the model
//    gets vfn[84](m), vfn[88](1, 1, 1, 1), and its child 0x435538CB
//    vfn[88](0, 1, 1, 1).
DECOMP(003FAF68, Prop_LeverTick_003FAF68) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    Stack frame(ctx, 336);
    const u32 sp = frame.addr();
    const u32 quat = sp + 112, pt = sp + 128, qm = sp + 144, m = sp + 208, key = sp + 272;
    const u32 holder = load<u32>(o + 292);

    // 1. Travel.
    if (load<u8>(o + 352) == 0) {
        const u32 p = load<u32>(o + 88);
        const f64 v0 = Lf(o + 108);
        const f64 a = Divs(Fmsubs(Lf(o + 304), Lf(p + 24), S(v0 * Lf(p + 28))), Lf(o + 112));
        const f64 xa = Fmadds(v0, dt, Lf(o + 104));
        const f64 v = Fmadds(a, dt, v0);
        Sf(o + 108, v);
        Sf(o + 104, Fmadds(S(S(a * C(0)) * dt), dt, xa));
        bool mid = true;
        f64 vc = v;
        if (v < C(4) || v > C(8)) {
            mid = EnsureEffect(ctx, o, holder, kHashFast, true);
            vc = ctx.fpr[1];
        }
        if (mid && (vc < C(12) || vc > C(16))) EnsureEffect(ctx, o, load<u32>(o + 292), kHashMove, false);

        f64 f1 = Lf(o + 104);
        const f64 one = C(20);
        if (f1 > one) {
            f1 = one;
            store<u32>(o + 104, 0x3F800000u);
            const f64 zero = C(24);
            if (Lf(o + 108) > zero) store<u32>(o + 108, 0);
            f1 = Latch(ctx, o, f1, [&] {
                const f64 nx = NegAdds(Lf(o + 336), zero), ny = NegAdds(Lf(o + 340), one), nz = NegAdds(Lf(o + 344), zero);
                store<u8>(o + 352, 1);
                Sf(o + 336, nx);
                Sf(o + 340, ny);
                Sf(o + 344, nz);
            });
        }
        if (f1 < C(28)) {
            store<u32>(o + 104, 0xBF800000u);
            const f64 zero = C(24);
            if (Lf(o + 108) < zero) store<u32>(o + 108, 0);
            Latch(ctx, o, f1, [&] {
                const f64 nx = Adds(Lf(o + 336), zero), ny = Adds(Lf(o + 340), one), nz = Adds(Lf(o + 344), zero);
                store<u8>(o + 352, 1);
                Sf(o + 336, nx);
                Sf(o + 340, ny);
                Sf(o + 344, nz);
            });
        }
    }

    // 2. Angle.
    const f64 t = Lf(o + 124);
    f64 ang = Divs(S(t * C(32)), C(36));
    const f64 pos = Divs(S(t * C(40)), C(36));
    if (Adds(ang, pos) < C(44)) ang = NegSubs(pos, C(48));
    if (Adds(ang, pos) > C(48)) ang = Subs(C(48), pos);
    const u32 ref = load<u32>(load<u32>(b25fn::Game) + 0x1281E0u);
    if (ref == 0) {
        ctx.gpr[3] = 0;
        ctx.fpr[1] = C(48);  // as the original leaves f1
        return;
    }

    // 3. Hinge rotation.
    const f64 ax = Lf(b25fn::AxisVec), ay = Lf(b25fn::AxisVec + 4), az = Lf(b25fn::AxisVec + 8);
    const f64 len = Sqrts(Fmadds(az, az, Fmadds(ax, ax, S(ay * ay))));
    const f64 half = NegMuls(ang, C(0));
    const f64 inv = Divs(C(20), len);
    const f64 nx = S(ax * inv), ny = S(ay * inv), nz = S(az * inv);
    const f64 s = callf(ctx, b25fn::Math_Sinf, {}, {half});
    const f64 qx = S(s * nx), qy = S(s * ny), qz = S(s * nz);
    const f64 c = callf(ctx, b25fn::Math_Cosf, {}, {half});
    Sf(quat, qx);
    Sf(quat + 4, qy);
    Sf(quat + 8, qz);
    Sf(quat + 12, c);
    call(ctx, b25fn::Quat_ToMatrix, {qm, quat, b25fn::ZeroVec});
    const u32 map = load<u32>(load<u32>(b25fn::MapHolderPoint) + 16);
    store<u32>(key, kPointKey);
    const u32 slot = static_cast<u32>(call(ctx, b25fn::PointMap_Find, {map, key}));
    const u32 frm = load<u32>(slot);
    if (frm == 0) return;  // r3 = the slot
    VCall(ctx, o, 76, {o});
    call(ctx, b25fn::Matrix_MulRot, {m, qm, frm + 16});
    const u32 rows = load<u32>(o + 256) + 96;
    const V4 p = Add4(Lvx(rows + 48), Row3(Lvx(rows), Lvx(rows + 16), Lvx(rows + 32), SplatLvlx(o + 224), SplatLvlx(o + 228), SplatLvlx(o + 232)));
    Stvx(pt, p);
    Cw(m + 48, pt);
    Cw(m + 52, pt + 4);
    Cw(m + 56, pt + 8);
    ctx.fpr[2] = Lf(pt + 4);  // the original copies through f1 / f2
    ctx.fpr[1] = Lf(pt + 8);
    const u32 mdl = load<u32>(o + 300);
    VCall(ctx, mdl, 84, {mdl, m});
    const u32 mdl2 = load<u32>(o + 300);
    VCall(ctx, mdl2, 88, {mdl2, 1, 1, 1, 1});
    if (call(ctx, b25fn::Obj_FindByHash, {load<u32>(o + 300), kChildHash}) == 0) return;
    const u32 ch = static_cast<u32>(call(ctx, b25fn::Obj_FindByHash, {load<u32>(o + 300), kChildHash}));
    VCall(ctx, ch, 88, {ch, 0, 1, 1, 1});
}
