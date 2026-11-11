// Game code, large-function sweep part 10: stick-direction anim selection
// (00402970).
#include "big10.h"

using namespace sly4;
using namespace sly4::big10;

namespace {
constexpr u32 kGetChannel = 0x0045B160u;    // Obj_GetChannel_0045B160 (pad channels, u32 axis) -> f32
constexpr u32 kAtan2f = 0x005E665Cu;        // atan2f (y, x)
constexpr u32 kCosf = 0x007344E0u;          // Math_Cosf
constexpr u32 kSinf = 0x007345F0u;          // Math_Sinf
constexpr u32 kTrackSet = 0x0029879Cu;      // Anim_TrackSet36_0029879C (anim, 0, 0) -> u32
constexpr u32 kAnimFind = 0x00298874u;      // Anim_Find298874 (anim, 0) -> u32
constexpr u32 kRestart = 0x002775A8u;       // Anim_Restart2775A8 (anim, hash, ...; f1..f4)
constexpr u32 kAnimStop = 0x00254814u;      // Anim_Stop254814 (anim, hash)

constexpr u32 kDirK = 0x0040295Cu;          // .text f32: +0 0, +4 max length (1), +8, +12 dead zone, +16 blend
constexpr u32 kPads = 0x010B19E0u;          // pad records (11968 bytes; +176 the channels)
constexpr u32 kStopHash = 0xB36A8124u;
// The four direction anims (state at +216 / +224): 0 down, 1 right, 2 up, 3 left (name?).
constexpr u32 kDirHash[4] = {0x84CBA01Au, 0xDAFB8629u, 0xDAF80AC4u, 0x0EE1193Du};

u32 Anim(PPUContext& ctx, u32 o) {
    const u32 a = load<u32>(o + 92);
    return static_cast<u32>(VCall(ctx, a, 60, {a}));
}

// TrackSet != 0 with no anim found, or an anim found (one Find call each).
bool AnimIdle(PPUContext& ctx, u32 o) {
    if (call(ctx, kTrackSet, {Anim(ctx, o), 0, 0}) != 0 && static_cast<u32>(call(ctx, kAnimFind, {Anim(ctx, o), 0})) == 0) return true;
    return static_cast<u32>(call(ctx, kAnimFind, {Anim(ctx, o), 0})) != 0;
}
}  // namespace

// void Obj_StickDirAnim(Obj* o)
// The controlling pad's stick (channels 0 / 1) picks one of four direction
// anims by its larger component once past the dead zone; the anim restarts
// (f1 the length cap, f2 the blend) and the stop anim is cleared. With the
// stick released the state words are reset to -1 when the anim is idle.
DECOMP(00402970, Obj_StickDirAnim402970) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 r = static_cast<u32>(VCall(ctx, o, 76, {o}));
    const u32 ch = kPads + static_cast<u32>(load<u8>(r + 4209)) * 11968u + 176;
    const f64 x = callf(ctx, kGetChannel, {ch, 0}, {});
    const f64 y = callf(ctx, kGetChannel, {ch, 1}, {});
    const f64 zero = F(kDirK);
    const f64 ang = callf(ctx, kAtan2f, {}, {y, x});
    const f64 c = std::fabs(callf(ctx, kCosf, {}, {ang}));
    const f64 s = std::fabs(callf(ctx, kSinf, {}, {ang}));
    const f64 k = std::fabs(c > s ? callf(ctx, kCosf, {}, {ang}) : callf(ctx, kSinf, {}, {ang}));
    // Scaled to the unit square's edge.
    f32 dy = static_cast<f32>(y * k);
    f32 dx = static_cast<f32>(x * k);
    const f32 dz = static_cast<f32>(zero * k);
    const f32 cap = F(kDirK + 4);
    f32 len = dy * dy;
    len = Fmadds(dx, dx, len);
    len = Fmadds(dz, dz, len);
    len = static_cast<f32>(std::sqrt(static_cast<f64>(len)));
    const f32 m = len < cap ? len : cap;
    ctx.fpr[1] = m;
    if (m >= F(kDirK + 8)) {
        const f32 sc = cap / len;
        dx = dx * sc;
        dy = dy * sc;
    }
    if (!(m > F(kDirK + 12))) {
        if (AnimIdle(ctx, o)) {
            store<u32>(o + 224, 0xFFFFFFFFu);
            store<u32>(o + 216, 0xFFFFFFFFu);
            ctx.gpr[3] = 0xFFFFFFFFu;   // r3 as the original leaves it
        }
        return;
    }
    if (!AnimIdle(ctx, o)) return;
    const f32 ay = std::fabs(dy), ax = std::fabs(dx);
    const f32 blend = F(kDirK + 16);
    ctx.fpr[1] = ay;   // left in f1 when the state is already set
    u32 st;
    if (ay > ax) st = dy > zero ? 2u : 3u;
    else st = dx > zero ? 1u : 0u;
    if (load<u32>(o + 216) == st || load<u32>(o + 224) == st) {
        ctx.gpr[3] = st;   // r3: the state word just compared
        return;
    }
    const u32 a = Anim(ctx, o);
    // r8 (and r7 for state 0) are not set by the original (the vcall's
    // leftovers); stack words 9 / 10 are the caller frame's, 11 = 0, 12 = -1.
    const u32 r7 = st == 0 ? static_cast<u32>(ctx.gpr[7]) : 0u;
    callf(ctx, kRestart, {a, kDirHash[st], 0, 0, r7, static_cast<u32>(ctx.gpr[8]), 0, 0, 0, 0, 0, ~0ull},
          {static_cast<f64>(cap), static_cast<f64>(blend), zero, zero});
    call(ctx, kAnimStop, {Anim(ctx, o), kStopHash});
    if (st == 0) {
        store<u32>(o + 216, 0);
        store<u32>(o + 220, 0xFFFFFFFFu);
        store<u32>(o + 224, 0);
    } else {
        store<u32>(o + 220, 0xFFFFFFFFu);
        store<u32>(o + 216, st);
        store<u32>(o + 224, st);
        ctx.gpr[3] = st;   // r3: the value stored
    }
}
