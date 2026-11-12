// Game code, medium sweep part 33: two "launch" behaviour message handlers
// (callers f_0027E330 / f_0027FAF0).
//
// Behaviour layout: +8 the owner (vtable: slot 28 SetFlag(o, k, v), slot
// 32 State(o, k), slot 60 Animator(o), slot 64 Body(o); +144 position,
// +20 a node with flags +48), +80 an embedded object (slot 28 Reset(p,
// 0, 0)), +96 a config (+4 "already launched"), +100 the target
// reference (+0 the target object, position +64; u8 +33 a flag).
// Body: +64 / +592 / +608 position vectors, +192 the mass scale,
// +416 flags, +656 launch velocity, +672 the solved velocity.
#include "mid33.h"

using namespace sly4;
using namespace sly4::mid33;

namespace {
using namespace m33fn;
constexpr u32 SolveAccel33 = 0x004A3068u;  // Vec3_SolveAccel_004A3068(out, a, b, c; f1 t)
constexpr u32 SetMode33 = 0x00261328u;     // Body_SetMode(body, mode)
constexpr u32 AnimRestart33 = 0x002775A8u; // Anim_Restart2775A8 (12 arguments + f1..f4)
constexpr u32 kZero33 = 0x01164B40u;       // a zero vector
constexpr u32 kScale33 = 0x01057358u;      // f32 velocity scale
constexpr u32 kGravity33 = 0x0105BB88u;    // the gravity vector (f32 +72 the divisor)

// The owner's Body (owner slot 64).
u32 LaunchBody33(PPUContext& ctx, u32 o) {
    const u32 b = load<u32>(o + 8);
    return static_cast<u32>(VCall(ctx, b, 64, {b}));
}

// Launch: solve the velocity that takes the owner from its position to the
// target's (time sqrt(dx^2 + dz^2 + k0^2) / g), store it at body +672 with
// +416 bit 0x00800000, Body_SetMode(2), set bit 0x40000000 of the body's
// slot 8 object +168, restart animation 0x0022EC2E (r7 / r8 = a7 / a8),
// SetFlag(0, 1) and SetFlag(1, 1), and set node +48 bit 0x40000000 (and
// bit 0x00100000 = the target's u8 +33 bit 0 when withFlag). k = {k0, f1}
// is the f32 pair at kAddr.
void Launch33(PPUContext& ctx, u32 o, u32 b, u32 kAddr, u64 a7, u64 a8, bool withFlag) {
    Stack frame(ctx, 224);
    const u32 sp = frame.addr();
    const u32 from = sp + 144, vel = sp + 160, out = sp + 176;
    for (u32 k = 0; k < 12; k += 4) store<u32>(from + k, load<u32>(b + 144 + k));
    const u32 p = LaunchBody33(ctx, o);
    const f64 sc = Lf(kScale33), m = Lf(p + 192);
    for (u32 k = 0; k < 12; k += 4) store<f32>(vel + k, static_cast<f32>(Fmuls31(Fmuls31(Lf(p + 656 + k), sc), m)));
    const u32 ref = load<u32>(o + 100);
    const u32 tgt = load<u32>(ref) + 64;
    const u32 src = load<u32>(o + 8) + 144;
    const f64 k0 = Lf(kAddr);
    const f64 dx = Sgl28(Lf(tgt) - Lf(src));
    const f64 dz = Sgl28(Lf(tgt + 8) - Lf(src + 8));
    f64 a = Fmadds(dx, dx, Fmuls31(k0, k0));
    a = Fmadds(dz, dz, a);
    const f64 t = Sgl28(Sgl28(std::sqrt(a)) / Lf(kGravity33 + 72));
    callf(ctx, SolveAccel33, {out, from, tgt, vel, kGravity33}, {t});
    const u32 flag = withFlag ? load<u8>(ref + 33) : 0;
    u32 q = LaunchBody33(ctx, o);
    store<u32>(q + 672, load<u32>(out));
    const u32 f416 = load<u32>(q + 416);
    store<u32>(q + 676, load<u32>(out + 4));
    store<u32>(q + 680, load<u32>(out + 8));
    store<u32>(q + 416, f416 | 0x00800000u);
    q = LaunchBody33(ctx, o);
    call(ctx, SetMode33, {q, 2});
    q = LaunchBody33(ctx, o);
    const u32 r = static_cast<u32>(VCall(ctx, q, 8, {q}));
    store<u32>(r + 168, load<u32>(r + 168) | 0x40000000u);
    u32 ow = load<u32>(o + 8);
    const u32 an = static_cast<u32>(VCall(ctx, ow, 60, {ow}));
    // arguments 9 / 10 (r1 +112 / +120) are never written; the callee reads 11 / 12
    callf(ctx, AnimRestart33, {an, 0x0022EC2Eu, 0, 0, a7, a8, 0, 0, 0, 0, 0, ~0ull}, {Lf(kAddr + 4), k0, k0, k0});
    ow = load<u32>(o + 8);
    VCall(ctx, ow, 28, {ow, 0, 1});
    ow = load<u32>(o + 8);
    VCall(ctx, ow, 28, {ow, 1, 1});
    const u32 nd = load<u32>(load<u32>(o + 8) + 20);
    u32 w = load<u32>(nd + 48) | 0x40000000u;
    if (withFlag) w = (w & ~0x00100000u) | ((flag & 1u) << 20);
    store<u32>(nd + 48, w);
}

// The landing reset: body +64 = the zero vector, +608 = +64, +592 = +608,
// then the embedded +80 object's Reset (slot 28 with 0, 0). f1 is left
// holding the last word copied.
void LaunchReset33(PPUContext& ctx, u32 o) {
    const u32 p = LaunchBody33(ctx, o);
    for (u32 k = 0; k < 12; k += 4) store<u32>(p + 64 + k, load<u32>(kZero33 + k));
    for (u32 k = 0; k < 12; k += 4) store<u32>(p + 608 + k, load<u32>(p + 64 + k));
    for (u32 k = 0; k < 12; k += 4) store<u32>(p + 592 + k, load<u32>(p + 608 + k));
    ctx.fpr[1] = Lf(p + 600);
    VCall(ctx, o + 80, 28, {o + 80, 0, 0});
}

// The common entry check: message type 2 and a live target.
bool LaunchMsg33(u32 o, u32 msg) {
    return load<u32>(msg) == 2 && load<u32>(load<u32>(o + 100)) != 0;
}
}  // namespace

// Launch handler (o, msg) -> 1 unhandled / 0: for message 2 with a target:
// launches once (config +4 clear; the target flag goes to node +48), then
// when the owner's State(0) is non-zero, or State(1) is 1, State(0) must
// be 3; in that case the body is reset and 0 returned.
DECOMP(0027D6E8, Launch_OnMsg_0027D6E8) {
    const u32 o = arg<u32>(ctx, 0), msg = arg<u32>(ctx, 1);
    if (!LaunchMsg33(o, msg)) return ret(ctx, 1u);
    u32 b = load<u32>(o + 8);
    if (load<u32>(load<u32>(o + 96) + 4) == 0) {
        Launch33(ctx, o, b, 0x0027D6E0u, 0, ~0ull, true);
        b = load<u32>(o + 8);
    }
    bool check3 = true;
    if (static_cast<s32>(VCall(ctx, b, 32, {b, 0})) == 0) {
        b = load<u32>(o + 8);
        check3 = static_cast<s32>(VCall(ctx, b, 32, {b, 1})) == 1;
    }
    if (check3) {
        b = load<u32>(o + 8);
        if (static_cast<s32>(VCall(ctx, b, 32, {b, 0})) != 3) return ret(ctx, 1u);
    }
    LaunchReset33(ctx, o);
    ret(ctx, 0u);
}

// Launch handler (o, msg) -> 1 unhandled / 0: as 0x0027D6E8 without the
// target flag (and r7 / r8 = -1 / 0 for the animation restart); after the
// launch the body is reset only when State(0) is 0 and State(1) is not 1.
DECOMP(0027EE70, Launch_OnMsg_0027EE70) {
    const u32 o = arg<u32>(ctx, 0), msg = arg<u32>(ctx, 1);
    if (!LaunchMsg33(o, msg)) return ret(ctx, 1u);
    u32 b = load<u32>(o + 8);
    if (load<u32>(load<u32>(o + 96) + 4) == 0) {
        Launch33(ctx, o, b, 0x0027EE64u, ~0ull, 0, false);
        b = load<u32>(o + 8);
    }
    if (static_cast<s32>(VCall(ctx, b, 32, {b, 0})) != 0) return ret(ctx, 1u);
    b = load<u32>(o + 8);
    if (static_cast<s32>(VCall(ctx, b, 32, {b, 1})) == 1) return ret(ctx, 1u);
    LaunchReset33(ctx, o);
    ret(ctx, 0u);
}
