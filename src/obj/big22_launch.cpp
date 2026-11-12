// Game code, large-function sweep part 22: the local player's launch event
// (callee of f_00309AC8 / f_0030C6F8).
#include "big22.h"

namespace sly4::obj {

namespace {
constexpr u32 kLaunchK = 0x00309438u;  // f32 0.0
constexpr u32 kUpVec = 0x01164B60u;    // f32[3] (Constants_Init_0056B93C)
constexpr u32 kLaunchMsgVt = 0x00B0DADCu;  // the 40-byte stack message's vtable
constexpr u32 Obj_ResetPos = 0x0026D9D4u;    // (Obj*)
constexpr u32 Obj_Push2696E0 = 0x002696E0u;  // (Obj*, const Vec3*; f32)
constexpr u32 Body_SetMode = 0x00261328u;    // (Obj*, u32 mode)

// The player: +20 (its +48 flag word), +172 / +192 flags; vfn[80] busy,
// vfn[86] takes a message, vfn[16] the launched object, vfn[7](a, b).
struct LaunchPlayer {
    pad<20> _0;
    be_u32 w20;      // +20
    pad<148> _24;
    be_u32 f172;     // +172
    pad<8> _176;
    be_u64 f184;     // +184 (bit 0 read on the source object)
    be_u32 f192;     // +192
};
static_assert(offsetof(LaunchPlayer, f172) == 172 && offsetof(LaunchPlayer, f184) == 184 && offsetof(LaunchPlayer, f192) == 192);
// The launched object.
struct LaunchObj {
    pad<416> _0;
    be_u32 f416;     // +416
    pad<172> _420;
    Vec3 push;       // +592
    pad<68> _604;
    Vec3 vel;        // +672
};
static_assert(offsetof(LaunchObj, push) == 592 && offsetof(LaunchObj, vel) == 672);
// The 40-byte message.
struct LaunchMsg {
    be_u32 vt;       // +0
    be_u32 a;        // +4
    be_u32 b;        // +8
    be_u16 h12;      // +12
    be_u16 flags;    // +14
    Vec3 dir;        // +16
    pad<4> _28;
    be_f32 x;        // +32
    be_f32 speed;    // +36
};
static_assert(sizeof(LaunchMsg) == 40);
struct LaunchLocals {
    Vec4 push;       // sp+112
    LaunchMsg msg;   // sp+128
};

// The player slot's object when it has bit 3 of +192 set, else 0.
u32 PlayerOf(u32 pl) { return pl != 0 && (gm<LaunchPlayer>(pl).f192 & 8u) != 0 ? pl : 0; }
u32 PlayerRaw() { return gm<be_u32>(global<be_u32>(kWorld) + 4208 + 8); }
u32 Player() { return PlayerOf(PlayerRaw()); }
LaunchPlayer& P(u32 p) { return gm<LaunchPlayer>(p); }
}  // namespace

// void Player_Launch309440(Obj* a, -, u32 b, -, -, const Vec3* dir; f32 x,
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
// No caller reads r3 or f1.
void Player_Launch309440(LaunchPlayer* a, u32, u32 b, u32, u32, Vec3* dir, f64 x, f64 f2, f64 speed) {
    const f64 zero = lf(global<be_f32>(kLaunchK));
    if (x == zero) return;
    GuestFrame frame(56);
    LaunchLocals& L = *frame.as<LaunchLocals>();
    LaunchMsg& msg = L.msg;
    u32 p = Player();
    if ((P(p).f172 & 0x40000u) == 0) return;
    if (vcall<u32>(&P(p), 320) != 0) return;
    f64 vx = lf(dir->x), vy = lf(dir->y), vz = lf(dir->z);
    const u32 hi = (a != nullptr && (a->f184 & 1u) != 0) ? 1u : 0u;
    const u16 fl = static_cast<u16>((msg.flags & ~0xEF00u) | 0x6000u | (hi << 12));
    msg.b = b;
    msg.a = guest(a);
    msg.h12 = 0;
    msg.flags = fl;
    CopyXYZ(msg.dir, *dir);
    msg.vt = kLaunchMsgVt;
    msg.x = static_cast<f32>(x);
    msg.speed = static_cast<f32>(speed);
    p = Player();
    vcall(&P(p), 344, &msg);
    p = Player();
    if ((P(p).f172 & 0x40000u) == 0) return;
    if (vcall<u32>(&P(p), 320) != 0) return;
    p = Player();
    const u32 o = vcall<u32>(&P(p), 64);
    const u32 w = global<be_u32>(kWorld);
    if (o == 0 || !(speed > zero)) return;
    const Vec3& z = global<Vec3>(kZeroVec);
    if (bits(dir->x) == bits(z.x) && bits(dir->y) == bits(z.y) && bits(dir->z) == bits(z.z)) {
        const Vec3& up = global<Vec3>(kUpVec);
        vx = lf(up.x);
        vy = lf(up.y);
        vz = lf(up.z);
    }
    p = PlayerOf(gm<be_u32>(w + 4208 + 8));
    vcall(&P(p), 28, 0u, 0u);
    p = Player();
    vcall(&P(p), 28, 1u, 2u);
    LaunchObj& lo = gm<LaunchObj>(o);
    const u32 ox = bits(lo.push.x), oy = bits(lo.push.y), oz = bits(lo.push.z);
    call(Obj_ResetPos, o);
    set_bits(L.push.x, ox ^ 0x80000000u);  // fneg / stfs
    set_bits(L.push.y, oy ^ 0x80000000u);
    set_bits(L.push.z, oz ^ 0x80000000u);
    call(Obj_Push2696E0, o, &L.push, f2);
    const f64 m1 = S(vy * speed), m2 = S(vz * speed), m0 = S(vx * speed);
    L.push.y = static_cast<f32>(m1);
    L.push.z = static_cast<f32>(m2);
    L.push.x = static_cast<f32>(m0);
    lo.vel.x = static_cast<f32>(m0);
    set_bits(lo.vel.y, bits(L.push.y));
    lo.f416 = lo.f416 | 0x00800000u;
    set_bits(lo.vel.z, bits(L.push.z));
    call(Body_SetMode, o, 2u);
    const u32 r = vcall<u32>(&lo, 8);
    gm<be_u32>(r + 168) = gm<be_u32>(r + 168) | 0x40000000u;
    p = Player();
    vcall(&P(p), 28, 0u, 1u);
    p = Player();
    vcall(&P(p), 28, 1u, 1u);
    be_u32& t = gm<be_u32>(P(Player()).w20 + 48);
    t = t | 0x40000000u;
}

}  // namespace sly4::obj

NATIVE(00309440, sly4::obj::Player_Launch309440)
