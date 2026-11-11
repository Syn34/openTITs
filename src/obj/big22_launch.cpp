// Game code, large-function sweep part 22: the local player's launch event
// (callee of f_00309AC8 / f_0030C6F8).
#include "big22.h"

using namespace sly4;
using namespace sly4::big22;

namespace {
constexpr u32 kWorld = 0x00F65CA0u;    // world state; +4208 the player slot, +8 its object
constexpr u32 kLaunchK = 0x00309438u;  // f32 0.0
constexpr u32 kZeroVec = 0x01164B40u;  // f32[3] 0
constexpr u32 kUpVec = 0x01164B60u;    // f32[3] (Constants_Init_0056B93C)
constexpr u32 kLaunchMsgVt = 0x00B0DADCu;  // the 40-byte stack message's vtable

namespace lfn {
constexpr u32 Obj_ResetPos = 0x0026D9D4u;  // (Obj*)
constexpr u32 Obj_Push2696E0 = 0x002696E0u;  // (Obj*, const Vec3*; f32)
constexpr u32 Body_SetMode = 0x00261328u;    // (Obj*, u32 mode)
}  // namespace lfn

// The player slot's object when it has bit 3 of +192 set, else 0.
u32 PlayerOf(u32 pl) { return pl != 0 && (load<u32>(pl + 192) & 8u) != 0 ? pl : 0; }
u32 PlayerRaw() { return load<u32>(load<u32>(kWorld) + 4208 + 8); }
u32 Player() { return PlayerOf(PlayerRaw()); }
}  // namespace

// u32 Player_Launch309440(Obj* a, -, u32 b, -, -, const Vec3* dir; f32 x,
// f32 f2, f32 speed): nothing when x == 0.0, when the local player (world
// +4208 +8, used only with bit 3 of +192; a null object is read through as
// the original does) lacks bit 18 of +172 or its vfn[80] says busy. A
// 40-byte message {vtable 0x00B0DADC, a, b, u16 0, u16 flags (the stack's
// old low byte kept; 0x6000, bit 12 = bit 0 of a's u64 +184), dir, -, x,
// speed} goes to the player's vfn[86]. Then (same checks again) the
// player's vfn[16] gives the object o; with o and speed > 0.0: the player's
// vfn[7](0, 0) and (1, 2); o reset, pushed by -o+592 with f2, its +672..680
// = dir * speed (dir = kUpVec when it is the zero vector), u32 +416 |=
// 0x00800000, mode 2; o's vfn[2] object's +168 and the player's +20 ->+48
// get 0x40000000; the player's vfn[7](0, 1) and (1, 1) between them.
// r3 as left by each exit (see the comments).
DECOMP(00309440, Player_Launch309440) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 b = arg<u32>(ctx, 2);
    const u32 dir = arg<u32>(ctx, 5);
    const f64 x = farg(ctx, 0), f2 = farg(ctx, 1), speed = farg(ctx, 2);
    const f64 zero = Lf(kLaunchK);
    if (x == zero) {
        ret(ctx, a);  // r3 untouched
        return;
    }
    Stack frame(ctx, 56);
    const u32 sp = frame.addr();  // +0: the push vector (r1+112), +16: the message (r1+128)
    const u32 msg = sp + 16;
    u32 p = Player();
    if ((load<u32>(p + 172) & 0x40000u) == 0) {
        ret(ctx, 0u);
        return;
    }
    if (const u32 r = static_cast<u32>(VCall(ctx, p, 320, {p})); r != 0) {
        ret(ctx, r);
        return;
    }
    f64 vx = Lf(dir), vy = Lf(dir + 4), vz = Lf(dir + 8);
    const u32 hi = (a != 0 && (load<u64>(a + 184) & 1u) != 0) ? 1u : 0u;
    const u16 fl = static_cast<u16>((load<u16>(msg + 14) & ~0xEF00u) | 0x6000u | (hi << 12));
    store<u32>(msg + 8, b);
    store<u32>(msg + 4, a);
    store<u16>(msg + 12, 0);
    Cw(msg + 16, dir);
    store<u16>(msg + 14, fl);
    Cw(msg + 20, dir + 4);
    store<u32>(msg + 0, kLaunchMsgVt);
    Cw(msg + 24, dir + 8);
    Sf(msg + 32, x);
    Sf(msg + 36, speed);
    p = Player();
    VCall(ctx, p, 344, {p, msg});
    const u32 pl = PlayerRaw();
    p = PlayerOf(pl);
    if ((load<u32>(p + 172) & 0x40000u) == 0) {
        ret(ctx, pl);  // r3 = the raw slot object
        return;
    }
    if (const u32 r = static_cast<u32>(VCall(ctx, p, 320, {p})); r != 0) {
        ret(ctx, r);
        return;
    }
    p = Player();
    const u32 o = static_cast<u32>(VCall(ctx, p, 64, {p}));
    const u32 w = load<u32>(kWorld);
    if (o == 0 || !(speed > zero)) {
        ret(ctx, w);  // r3 = the world pointer
        return;
    }
    if (load<u64>(dir) == load<u64>(kZeroVec) && load<u32>(dir + 8) == load<u32>(kZeroVec + 8)) {
        vx = Lf(kUpVec);
        vy = Lf(kUpVec + 4);
        vz = Lf(kUpVec + 8);
    }
    p = PlayerOf(load<u32>(w + 4208 + 8));
    VCall(ctx, p, 28, {p, 0, 0});
    p = Player();
    VCall(ctx, p, 28, {p, 1, 2});
    const u32 ox = load<u32>(o + 592), oy = load<u32>(o + 596), oz = load<u32>(o + 600);
    call(ctx, lfn::Obj_ResetPos, {o});
    store<u32>(sp + 0, ox ^ 0x80000000u);  // fneg / stfs
    store<u32>(sp + 4, oy ^ 0x80000000u);
    store<u32>(sp + 8, oz ^ 0x80000000u);
    callf(ctx, lfn::Obj_Push2696E0, {o, sp}, {f2});
    const f64 m1 = S(vy * speed), m2 = S(vz * speed), m0 = S(vx * speed);
    Sf(sp + 4, m1);
    Sf(sp + 8, m2);
    Sf(sp + 0, m0);
    Sf(o + 672, m0);
    const u32 f416 = load<u32>(o + 416);
    Cw(o + 676, sp + 4);
    store<u32>(o + 416, f416 | 0x00800000u);
    Cw(o + 680, sp + 8);
    call(ctx, lfn::Body_SetMode, {o, 2});
    const u32 r = static_cast<u32>(VCall(ctx, o, 8, {o}));
    store<u32>(r + 168, load<u32>(r + 168) | 0x40000000u);
    p = Player();
    VCall(ctx, p, 28, {p, 0, 1});
    p = Player();
    VCall(ctx, p, 28, {p, 1, 1});
    const u32 t = load<u32>(Player() + 20) + 48;
    store<u32>(t, load<u32>(t) | 0x40000000u);
    ret(ctx, t);
}
