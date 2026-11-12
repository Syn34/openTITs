// Game code, large-function sweep part 32: a sensor phantom's AABB set to
// the 30-degree wedge in front of its owner, then a scan of what it
// overlaps for a grab-flagged shape.
#include "big32.h"

namespace sly4::obj {

struct WedgeOwner {
    pad<128> _0;
    be_f32 fwd_x;   // +128: the forward axis (matrix +96, row 2) x
    pad<4> _132;
    be_f32 fwd_z;   // +136
    pad<4> _140;
    Vec3 pos;       // +144
};
static_assert(offsetof(WedgeOwner, fwd_z) == 136 && offsetof(WedgeOwner, pos) == 144);
struct WedgeCollidable {
    pad<16> _0;
    s8 props_off;  // +16: the property block, relative
    pad<7> _17;
    s8 kind;       // +24: 1 has properties
};
static_assert(offsetof(WedgeCollidable, kind) == 24);
struct WedgeBody {
    pad<8> _0;
    be_u32 w8;                         // +8
    pad<180> _12;
    gp<gp<WedgeCollidable>> overlaps;  // +192
    be_s32 count;                      // +196
};
static_assert(offsetof(WedgeBody, overlaps) == 192);
struct WedgeSensor {
    pad<8> _0;
    gp<WedgeOwner> owner;  // +8
    pad<108> _12;
    gp<WedgeBody> body;    // +120: the phantom
};
static_assert(offsetof(WedgeSensor, body) == 120);

namespace {
// .text: f32 -0 (sign mask), 0.0, 1e-5, 1.0, 0.5, pi/6, 8.0, -8.0 at +0..+28
constexpr u32 kWedgeK = 0x002D0220;
constexpr u32 kFltMax4 = 0x00B2C3A0;  // f32[4] 0x7F7FFFEE (the AABB min seed; max seeds with its negation)
constexpr u32 kUp = 0x01164B60;       // Vec3: world up
constexpr u32 kPropGrab = 0x418F096F;  // shape property: grab flag (as Ledge_Probe2CCFF0)
constexpr u32 Math_Cosf7344E0 = 0x007344E0;   // f1 -> f1
constexpr u32 Math_Sinf7345F0 = 0x007345F0;   // f1 -> f1
constexpr u32 Hk_SetAabb924B90 = 0x00924B90;  // (body, const hkAabb*)

f64 KF(u32 off) { return lf(gm<be_f32>(kWedgeK + off)); }

// One lane of p transformed by the rows c (lane k) / up / d and the
// translation t: vmaddfp chain from +0, then vaddfp.
f32 XLane(f32 c, f32 u, f32 d, f32 t, f32 px, f32 py, f32 pz) {
    f32 a = std::fma(c, px, 0.0f);
    a = std::fma(u, py, a);
    a = std::fma(d, pz, a);
    return a + t;
}
}  // namespace

// bool f(Sensor* s) (called by f_00131798 and f_00132AC0). With the phantom
// body, the owner and the body's +8 all non-zero: the owner's forward axis
// (y taken as 0) is normalized (returns 0 when its squared length is below
// 1e-5); the frame d = that axis, row 1 = kUp, row 0 = kUp x d, origin =
// owner pos + kUp * 0.5 transforms the five points (0, 0, 8), (8 sin, 0, 8
// cos), (-8 sin, 0, 8 cos) (30 degrees), (0, 0, 0), (0, 1, 0); their min /
// max (w lanes 0; seeded with +-0x7F7FFFEE) go to an hkAabb on the stack
// and Hk_SetAabb924B90(body, aabb) is called. Then for each overlapped
// collidable of the body (the count read once): a shape whose kind is 1
// with a non-zero property block whose kPropGrab value (low word) is
// non-zero gives 1. Otherwise 0.
bool Sensor_WedgeHasGrab_002D02E0(WedgeSensor* s) {
    if (!s->body) return false;
    const WedgeOwner* own = s->owner;
    if (own == nullptr) return false;
    if (s->body->w8 == 0) return false;
    const f64 zero = KF(4);
    const f64 x = lf(own->fwd_x), z = lf(own->fwd_z);
    f64 len2 = Fmadds(x, x, Muls(zero, zero));
    len2 = Fmadds(z, z, len2);
    if (std::fabs(len2) < KF(8)) return false;
    const f64 len = Sqrts(len2);
    const f64 half = KF(16), ang = KF(20);
    const Vec3& up = global<Vec3>(kUp);
    const f64 ux = lf(up.x), uy = lf(up.y), uz = lf(up.z);
    const f64 tx = Fmadds(ux, half, lf(own->pos.x)), ty = Fmadds(uy, half, lf(own->pos.y)), tz = Fmadds(uz, half, lf(own->pos.z));
    const f64 inv = Divs(KF(12), len);
    const f64 dy = Muls(zero, inv), dx = Muls(x, inv), dz = Muls(z, inv);
    const f64 cx = Fmsubs(uy, dz, Muls(uz, dy));
    const f64 cz = Fmsubs(ux, dy, Muls(uy, dx));
    const f64 cy = Fmsubs(uz, dx, Muls(ux, dz));
    const f64 cs = call<f64>(Math_Cosf7344E0, ang);
    const f64 sn = call<f64>(Math_Sinf7345F0, ang);
    const f64 eight = KF(24);
    const f32 sn8 = static_cast<f32>(Muls(sn, eight)), c8 = static_cast<f32>(Muls(cs, eight));
    const f64 ms8 = Muls(sn, KF(28));
    const f32 pts[5][3] = {{0.0f, 0.0f, 8.0f}, {sn8, 0.0f, c8}, {static_cast<f32>(ms8), 0.0f, c8}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    const f32 c[3] = {static_cast<f32>(cx), static_cast<f32>(cy), static_cast<f32>(cz)};
    const f32 u[3] = {up.x.get(), up.y.get(), up.z.get()};
    const f32 d[3] = {static_cast<f32>(dx), static_cast<f32>(dy), static_cast<f32>(dz)};
    const f32 t[3] = {static_cast<f32>(tx), static_cast<f32>(ty), static_cast<f32>(tz)};
    WedgeBody* body = s->body;
    GuestFrame frame(48);
    u8* aabb = frame.as<u8>(0);  // hkAabb {min, max}
    V4 mn = Lvx(host<u8>(kFltMax4)), mx = Xor4(mn, V4{{-0.0f, -0.0f, -0.0f, -0.0f}});
    for (u32 i = 0; i < 5; ++i) {
        V4 p{{0.0f, 0.0f, 0.0f, 0.0f}};  // the w lane cleared by vandc
        for (u32 k = 0; k < 3; ++k) p.e[k] = XLane(c[k], u[k], d[k], t[k], pts[i][0], pts[i][1], pts[i][2]);
        mn = Min4(mn, p);
        mx = Max4(mx, p);
    }
    Stvx(aabb + 16, mx);
    Stvx(aabb, mn);
    call(Hk_SetAabb924B90, body, aabb);
    const WedgeBody& b = gm<WedgeBody>(s->body.ea);  // re-read after the call (unchecked)
    const s32 n = b.count;
    for (s32 i = 0; i < n; ++i) {
        const u32 sh = gm<be_u32>(b.overlaps.ea + 4 * static_cast<u32>(i));
        const WedgeCollidable& col = gm<WedgeCollidable>(sh);
        if (col.kind != 1) continue;
        const u32 props = sh + static_cast<u32>(static_cast<s32>(col.props_off));
        if (props == 0) continue;
        const HkProps& pr = gm<HkProps>(props);
        u32 v = 0;
        for (s32 j = 0; j < pr.count; ++j) {
            if (gm<HkPropEntry>(pr.table.ea + 16 * static_cast<u32>(j)).key == kPropGrab) {
                v = gm<HkPropEntry>(pr.table.ea + (static_cast<u32>(j) << 4)).lo;
                break;
            }
        }
        if (v != 0) return true;
    }
    return false;
}

}  // namespace sly4::obj

NATIVE(002D02E0, sly4::obj::Sensor_WedgeHasGrab_002D02E0)
