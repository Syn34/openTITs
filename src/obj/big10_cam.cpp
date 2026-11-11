// Game code, large-function sweep part 10: a scripted camera rig's update
// (004168B0): anchor the two attached objects to their points, then step
// the rig's mode.
#include "big10.h"

using namespace sly4;
using namespace sly4::big10;

namespace {
constexpr u32 kPointFind = 0x0073343Cu;     // PointMap_Find (map, const u32* key) -> node** (node +64 position)
constexpr u32 kCamDefFind = 0x0073374Cu;    // CameraDefMap_Find (map, const u32* key) -> def** (def +112 direction)
constexpr u32 kFromRotPos = 0x00553B4Cu;    // Mat34_FromRotPos553B4C (Mat34* out, const Mat33* rot, const vec3* pos)
constexpr u32 kTrackSet = 0x0029879Cu;      // Anim_TrackSet36_0029879C (anim, 0, 0)
constexpr u32 kPointMaps = 0x00F8F154u;     // -> (+16 the point map)
constexpr u32 kCamDefMaps = 0x00F8F134u;    // -> (+16 the camera def map)
constexpr u32 kWorldPtr = 0x00F65CA0u;
constexpr u32 kWorldCamera = 0x001281E0u;   // World: camera (0: no offset along the view)
constexpr u32 kAxisX = 0x01164BD0u;         // identity row 0: the default direction
constexpr u32 kRigK = 0x004168A4u;          // .text f32: +0 1 (the blend cap), +4 1 / 65536-ish (random scale)
constexpr u32 kRand = 0x01164888u;          // u64 previous random value, +8 u32 state

// Mode handlers (o; f1 dt) (name?).
constexpr u32 kMode2 = 0x00416360u, kMode3 = 0x00415F10u, kMode4 = 0x00415DA0u, kMode5 = 0x00415830u;
constexpr u32 kMode6 = 0x00415680u, kMode7 = 0x00415088u, kMode8 = 0x00414CB0u, kMode9 = 0x004149D0u;
constexpr u32 kMode10 = 0x004142F0u, kMode11 = 0x00414058u, kMode12 = 0x00413DF8u, kMode14 = 0x00413970u;
constexpr u32 kMode15 = 0x00413804u, kMode13Done = 0x00413BACu;

// Anchor: obj's 3x3 rotation (+96 rows) and pos (offset along the camera
// def's direction, negated, or the X axis when there is no def, by o +104)
// make a matrix handed to obj's vfunc +84.
void Anchor(PPUContext& ctx, u32 o, u32 sp, u32 vpos, u32 key, u32 rot, u32 mat, u32 objsel) {
    u32 obj = load<u32>(o + objsel);
    if (load<u32>(load<u32>(kWorldPtr) + kWorldCamera) != 0) {
        store<u32>(key, load<u32>(o + 28));
        const u32 cd = load<u32>(static_cast<u32>(call(ctx, kCamDefFind, {load<u32>(load<u32>(kCamDefMaps) + 16), key})));
        const f32 k = F(o + 104);
        f32 d0 = F(kAxisX), d1 = F(kAxisX + 4), d2 = F(kAxisX + 8);
        const f32 p0 = F(vpos), p1 = F(vpos + 4), p2 = F(vpos + 8);
        obj = load<u32>(o + objsel);
        if (cd != 0) { d0 = -F(cd + 112); d1 = -F(cd + 116); d2 = -F(cd + 120); }
        store<f32>(vpos + 0, Fmadds(d0, k, p0));
        store<f32>(vpos + 4, Fmadds(d1, k, p1));
        store<f32>(vpos + 8, Fmadds(d2, k, p2));
        if (objsel == 76) {
            obj = load<u32>(obj + 92);
            if (obj == 0) return;
        }
    } else if (objsel == 76) {
        return;
    }
    (void)sp;
    for (u32 r = 0; r < 3; ++r)
        for (u32 c = 0; c < 3; ++c) CopyW(rot + 16 * r + 4 * c, obj + 96 + 16 * r + 4 * c);
    call(ctx, kFromRotPos, {mat, rot, vpos});
    VCall(ctx, obj, 84, {obj, mat});
}

// The blend tail of modes 2 / 4 / 9: +136 = +132 * t + (1 - t) * +136,
// +128 = 0, +132 = rate.
void Blend(u32 o, f32 one, f32 t, u32 rate) {
    const f32 b = F(o + 136), a = F(o + 132);
    store<u32>(o + 128, 0);
    store<u32>(o + 132, rate);
    store<f32>(o + 136, Fmadds(a, t, (one - t) * b));
}
}  // namespace

// void CamRig_Update(CamRig* o; f32 dt)
// Anchors the rig's object (+4) at point +12 and the target's +92 object at
// point +8 (each pushed along the camera def's view direction when a camera
// exists), advances the blend timer (+128, capped at 1), runs the mode
// handler (+80: 0 random delay, 1 wait, 2..15 handlers), then switches to
// mode 14 / 12 when +104 - +108 leaves +-(+20).
DECOMP(004168B0, CamRig_Update4168B0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    Stack frame(ctx, 288);
    const u32 sp = frame.addr() - 112;   // offsets below are the original frame's
    store<u32>(sp + 388, load<u32>(o + 12));
    const u32 pt = load<u32>(static_cast<u32>(call(ctx, kPointFind, {load<u32>(load<u32>(kPointMaps) + 16), sp + 388})));
    if (pt != 0 && load<u32>(o + 4) != 0) {
        CopyW(sp + 112, pt + 64);
        CopyW(sp + 116, pt + 68);
        CopyW(sp + 120, pt + 72);
        Anchor(ctx, o, sp, sp + 112, sp + 384, sp + 192, sp + 304, 4);
    }
    store<u32>(sp + 380, load<u32>(o + 8));
    const u32 p2 = load<u32>(static_cast<u32>(call(ctx, kPointFind, {load<u32>(load<u32>(kPointMaps) + 16), sp + 380})));
    if (p2 != 0) {
        CopyW(sp + 128, p2 + 64);
        CopyW(sp + 132, p2 + 68);
        CopyW(sp + 136, p2 + 72);
        Anchor(ctx, o, sp, sp + 128, sp + 376, sp + 144, sp + 240, 76);
    }

    f32 t = static_cast<f32>(static_cast<f64>(F(o + 128)) + dt);
    store<f32>(o + 128, t);
    const f32 one = F(kRigK);
    if (t > one) {
        store<u32>(o + 128, 0x3F800000u);
        t = one;
    }
    const u32 mode = load<u32>(o + 80);
    u32 r3 = mode;   // r3 as the original leaves it on each path
    auto H = [&](u32 fn) { callf(ctx, fn, {o}, {dt}); r3 = static_cast<u32>(ctx.gpr[3]); };
    switch (static_cast<s32>(mode)) {
    case 0: {
        const u64 prev = load<u64>(kRand);
        store<u32>(o + 80, 1);
        const f32 lo = F(o + 56);
        const f32 span = F(o + 52) - lo;
        const u32 st = load<u32>(kRand + 8);
        const u32 x = (std::rotl(st, 4) + std::rotl(st, 16)) ^ 0x87654321u;
        const f32 r = static_cast<f32>(static_cast<f64>(static_cast<s64>(prev)));
        store<u32>(kRand + 8, std::rotl(x, 16));
        store<u64>(kRand, x & 0xFFFFu);
        store<u32>(o + 96, 0);
        store<f32>(o + 100, Fmadds(span, r * F(kRigK + 4), lo));
        r3 = 0x01160000u;
        break;
    }
    case 1: {
        const f32 w = static_cast<f32>(static_cast<f64>(F(o + 96)) + dt);
        const f32 lim = F(o + 100);
        store<f32>(o + 96, w);
        if (w > lim) {
            store<u32>(o + 100, 0x3F800000u);
            store<u32>(o + 80, 2);
            r3 = 0x3F800000u;
        }
        break;
    }
    case 2: {
        const f32 b = F(o + 136), a = F(o + 132);
        store<u32>(o + 128, 0);
        store<u32>(o + 132, 0x3F4CCCCDu);
        store<f32>(o + 136, Fmadds(a, t, (one - t) * b));
        H(kMode2);
        break;
    }
    case 3: H(kMode3); break;
    case 4:
        H(kMode4);
        Blend(o, one, F(o + 128), 0x3F800000u);
        r3 = 0;
        break;
    case 5: H(kMode5); break;
    case 6: H(kMode6); break;
    case 7: H(kMode7); break;
    case 8: H(kMode8); break;
    case 9:
        H(kMode9);
        Blend(o, one, F(o + 128), 0x3F666666u);
        r3 = 0x3F666666u;
        break;
    case 10: H(kMode10); break;
    case 11: H(kMode11); break;
    case 12: H(kMode12); break;
    case 13: {
        const u32 an = load<u32>(load<u32>(o + 76) + 92);
        const u32 a = static_cast<u32>(VCall(ctx, an, 60, {an}));
        r3 = static_cast<u32>(call(ctx, kTrackSet, {a, 0, 0}));
        if (r3 != 0) r3 = static_cast<u32>(call(ctx, kMode13Done, {load<u32>(o + 76)}));
        break;
    }
    case 14: H(kMode14); break;
    case 15: H(kMode15); break;
    default: break;
    }

    const f32 d = F(o + 104) - F(o + 108);
    const u32 lw = load<u32>(o + 20);
    const f32 lim = std::bit_cast<f32>(lw);
    if (d > lim) {
        r3 = load<u32>(o + 80);
        if (r3 != 15 && r3 != 14) { r3 = 14; store<u32>(o + 80, 14); }
    }
    if (d < -lim) {
        r3 = load<u32>(o + 80);
        if (r3 != 13 && r3 != 12) { r3 = 12; store<u32>(o + 80, 12); }
    }
    // Left in r3 / f1 by the original (f1 = -limit).
    ctx.gpr[3] = r3;
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(lw) ^ (1ull << 63));
}
