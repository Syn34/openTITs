// Game code, large-function sweep part 21: the spawner child's hop state
// (state 11, the sibling of mid22_a.cpp's Child_Orbit / Child_Arc / Child_Run).
#include "big21.h"

using namespace sly4;
using namespace sly4::big21;

namespace {
constexpr u32 kSplineHolder = 0x00F8F158u;     // header*; +16 the spline map
constexpr u32 kCameraDefHolder = 0x00F8F134u;  // header*; +16 the camera def map
constexpr u32 kHopRect = 0x00405F80u;  // f32 1.0, 0.5, 2.0, 0.0, 10.0, 3.1415, -10.0, 20.0, 1/65535, 2.0, 0.25, 0.2, -3.1415

// The found map entry's first word.
u32 FindSpline(PPUContext& ctx, u32 keyAt, u32 key) {
    store<u32>(keyAt, key);
    const u32 e = static_cast<u32>(call(ctx, b21fn::SplineMap_Find, {load<u32>(load<u32>(kSplineHolder) + 16), keyAt}));
    return load<u32>(e);
}
}  // namespace

// Child_Hop405FB8(Child* c; f32 dt): state 11. Needs the target effect c->28
// and the owner c->4 (r3 = c->28 otherwise). The clock c->116 += dt, t =
// clock / c->124 clamped to 1 (constants K = 0x00405F80); the point c+96 +
// (fx+64 - c+96) * t raised by c->128 * (1 - ((t - 0.5) * 2)^2) poses the
// owner (rows +96/+112/+128, vfn[84](Mat34_FromRotPos)). When the clock has
// passed c->120 or t reaches 1: state 12 (c->112), and one draw of the
// game's generator picks the four orbit splines c->12..24: 1 in 9 the
// spline 0xF425FD11 four times with weights (1, 0, 0, 0), else 0xF425FD12..15
// with four random weights (redrawn until their sum is 0.2 or more,
// normalised) into c->208..220. Clock 0, duration 2.0; the direction c+176 =
// (fx+64 - c+96) or the camera def c->44's +112, flattened (y = 0),
// normalised and scaled by 10; c+192 = the owner's position, c+160 =
// Quat_FromMatrix(owner rows). Two more draws set the orbit speeds c->136 /
// c->144 / c->180: a hop that rose by less than 1 orbits at pitch rate
// r0 * pi, yaw rate -10 + 20 * r1 and lift 2.0; otherwise the pitch rate
// -pi + pi * r0, yaw rate 0 and lift -10 + 20 * r1. The owner's animator
// (vfn[60]) restarts the clip 0xD6A0734F; the owner's effect (vfn[272]) poses
// its component 0x87208BC1 (kind 4) from the owner's rows and cues
// 0x80F43146. The spawner c->84 counts the child (+204) and sets u8 +231 when
// the count crosses a third or two thirds of its def's (+88) total +40.
DECOMP(00405FB8, Child_Hop405FB8) {
    const u32 c = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    Stack frame(ctx, 496);
    const u32 sp = frame.addr();
    constexpr u32 K = kHopRect;
    const u32 fx = load<u32>(c + 28);
    ctx.gpr[3] = fx;
    if (fx == 0) return;
    const u32 own = load<u32>(c + 4);
    if (own == 0) return;
    const f64 clk = S(Lf(c + 116) + dt);
    f64 t = Divs(clk, Lf(c + 124));
    Sf(c + 116, clk);
    const f64 one = Lf(K);
    if (t > one) t = one;
    const f64 x0 = Lf(c + 96), y0 = Lf(c + 100), z0 = Lf(c + 104);
    const f64 dx = S(Lf(fx + 64) - x0);
    const f64 px = Fmadds(dx, t, x0);
    Sf(sp + 144, dx);
    const f64 dy = S(Lf(fx + 68) - y0);
    f64 py = Fmadds(dy, t, y0);
    Sf(sp + 148, dy);
    const f64 dz = S(Lf(fx + 72) - z0);
    Sf(sp + 144, px);
    Sf(sp + 148, py);
    Sf(sp + 152, Fmadds(dz, t, z0));
    const f64 fy = Lf(fx + 68);  // the target height, kept for the rise test
    const f64 u = S(S(t - Lf(K + 4)) * Lf(K + 8));
    Sf(sp + 148, Fmadds(Lf(c + 128), Fnmsubs(u, u, one), py));
    for (u32 row = 0; row < 3; ++row)
        for (u32 k = 0; k < 3; ++k) Cw(sp + 176 + 16 * row + 4 * k, own + 96 + 16 * row + 4 * k);
    ctx.fpr[1] = Lf(own + 136);
    call(ctx, b21fn::Mat34_FromRotPos, {sp + 224, sp + 176, sp + 144});
    VCall(ctx, own, 84, {own, sp + 224});
    ctx.fpr[1] = Lf(c + 116);
    if (!(Lf(c + 116) > Lf(c + 120)) && !(t == one)) return;

    // ---- state 12: pick the orbit
    store<u32>(c + 112, 12);
    {
        const u32 seed = load<u32>(kRand + 8);
        const u32 mix = mid15::RandMix15(seed);
        store<u32>(kRand + 8, std::rotl(mix, 16));
        store<u64>(kRand, mix & 0xFFFFu);
        f64 w0, w1, w2, w3;
        if (seed % 9 == 0) {
            for (u32 k = 0; k < 4; ++k) store<u32>(c + 12 + 4 * k, FindSpline(ctx, sp + 416 - 4 * k, 0xF425FD11u));
            w0 = one;
            w1 = w2 = w3 = Lf(K + 12);
        } else {
            for (u32 k = 0; k < 4; ++k) store<u32>(c + 12 + 4 * k, FindSpline(ctx, sp + 400 - 4 * k, 0xF425FD12u + k));
            const f64 scale = Lf(K + 32), ten = Lf(K + 16), lo = Lf(K + 44);
            Rng g;
            g.Load();
            f64 r[4], sum;
            // The first four draws are stored; redraws (sum < 0.2) run in
            // registers and are stored once after the loop.
            auto draw4 = [&] {
                for (u32 k = 0; k < 4; ++k) r[k] = S(S(g.Next() * scale) * ten);
                sum = S(S(S(r[0] + r[1]) + r[2]) + r[3]);
            };
            draw4();
            store<u32>(kRand + 8, g.seed);
            store<u64>(kRand, g.last);
            if (sum < lo) {
                do draw4();
                while (sum < lo);
                store<u32>(kRand + 8, g.seed);
                store<u64>(kRand, g.last);
            }

            w0 = Divs(r[0], sum);
            w1 = Divs(r[1], sum);
            w2 = Divs(r[2], sum);
            w3 = Divs(r[3], sum);
        }
        Sf(c + 208, w0);
        Sf(c + 212, w1);
        Sf(c + 216, w2);
        Sf(c + 220, w3);
    }
    const f64 zero = Lf(K + 12);
    store<u32>(c + 116, 0);
    store<u32>(c + 124, 0x40000000u);  // 2.0f
    const u32 fx2 = load<u32>(c + 28);
    const f64 ex = S(Lf(fx2 + 64) - Lf(c + 96));
    const f64 ey = S(Lf(fx2 + 68) - Lf(c + 100));
    const f64 ez = S(Lf(fx2 + 72) - Lf(c + 104));
    store<u32>(sp + 384, load<u32>(c + 44));
    Sf(c + 176, ex);
    Sf(c + 180, ey);
    Sf(c + 184, ez);
    ctx.fpr[1] = ex;
    const u32 def = load<u32>(static_cast<u32>(call(ctx, b21fn::CameraDefMap_Find, {load<u32>(load<u32>(kCameraDefHolder) + 16), sp + 384})));
    if (def != 0) {
        Cw(c + 176, def + 112);
        Cw(c + 180, def + 116);
        Cw(c + 184, def + 120);
    }
    store<u32>(c + 180, 0);
    {
        const f64 vy = Lf(c + 180), vx = Lf(c + 176), vz = Lf(c + 184);
        const f64 l = Divs(one, Sqrts(Fmadds(vz, vz, Fmadds(vx, vx, S(vy * vy)))));
        Sf(c + 176, S(vx * l));
        Sf(c + 180, S(vy * l));
        Sf(c + 184, S(vz * l));
    }
    const u32 own2 = load<u32>(c + 4);
    Cw(c + 176, c + 176);
    Cw(c + 180, c + 180);
    {
        const f64 ten = Lf(K + 16);
        const f64 vx = Lf(c + 176), vy = Lf(c + 180), vz = Lf(c + 184);
        Sf(c + 176, S(vx * ten));
        Sf(c + 180, S(vy * ten));
        Sf(c + 184, S(vz * ten));
        ctx.fpr[1] = S(vx * ten);
    }
    Cw(c + 192, own2 + 144);
    Cw(c + 196, own2 + 148);
    Cw(c + 200, own2 + 152);
    call(ctx, b21fn::Quat_FromMatrix, {sp + 160, own2 + 96});
    for (u32 k = 0; k < 4; ++k) Cw(c + 160 + 4 * k, sp + 160 + 4 * k);
    const f64 rise = S(Lf(sp + 148) - fy);
    const f64 pi = Lf(K + 20), neg10 = Lf(K + 24), twenty = Lf(K + 28), scale = Lf(K + 32);
    {
        Rng g;
        g.Load();
        const f64 r0 = S(g.Next() * scale);
        const f64 r1 = S(g.Next() * scale);
        f64 pitch, yaw, lift;
        if (rise < one) {
            pitch = S(r0 * pi);
            yaw = Fmadds(r1, twenty, neg10);
            lift = Lf(K + 36);
        } else {
            pitch = Fmadds(r0, pi, Lf(0x00405FB0u));  // -3.1415
            yaw = zero;
            lift = Fmadds(r1, twenty, neg10);
        }
        store<u64>(kRand, g.last);
        Sf(c + 136, pitch);
        Sf(c + 144, yaw);
        store<u32>(kRand + 8, g.seed);
        Sf(c + 180, lift);
        ctx.fpr[1] = yaw;
    }
    const u32 o3 = load<u32>(c + 4);
    if (static_cast<u32>(VCall(ctx, o3, 60, {o3})) != 0) {
        const u32 o4 = load<u32>(c + 4);
        const u32 an = static_cast<u32>(VCall(ctx, o4, 60, {o4}));
        // arguments 9 / 10 (sp+112 / sp+120) are whatever the frame holds
        callf(ctx, b21fn::Anim_Restart,
              {an, 0xD6A0734Fu, 0, 0, ~0ull, 0, 0, 0, load<u64>(sp + 112), load<u64>(sp + 120), 0, ~0ull},
              {one, Lf(K + 40), zero, zero});
    }
    const u32 o5 = load<u32>(c + 4);
    const u32 e = static_cast<u32>(VCall(ctx, o5, 272, {o5}));
    if (e != 0) {
        const u32 p = static_cast<u32>(call(ctx, b21fn::Effect_FindComponent, {e, 0x87208BC1u}));
        if (p != 0 && static_cast<u32>(VCall(ctx, p, 32, {p})) == 4) {
            for (u32 row = 0; row < 4; ++row)
                for (u32 k = 0; k < 3; ++k) Cw(p + 32 + 16 * row + 4 * k, o5 + 96 + 16 * row + 4 * k);
            ctx.fpr[1] = Lf(o5 + 152);
        }
        call(ctx, b21fn::Effect_SetHash, {e, 0x80F43146u});
    }
    // The spawner's child count against thirds of its def's total.
    const u32 s = load<u32>(c + 84);
    const s32 was = static_cast<s32>(load<u32>(s + 204));
    const s32 now = was + 1;
    store<u32>(s + 204, static_cast<u32>(now));
    const s32 total = static_cast<s32>(load<u32>(load<u32>(s + 88) + 40));
    const s32 third = total / 3;
    if (now >= third && was < third) store<u8>(s + 231, 1);
    const s32 two = static_cast<s32>(static_cast<u32>(total) << 1) / 3;
    ctx.gpr[3] = static_cast<u32>(now);
    if (now < two) return;
    ctx.gpr[3] = static_cast<u32>(two);
    if (was >= two) return;
    store<u8>(s + 231, 1);
    ctx.gpr[3] = 1;
}
