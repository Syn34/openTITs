// Game code, large-function sweep part 21: the spawner child's hop state
// (state 11, the sibling of mid22_a.cpp's Child_Orbit / Child_Arc / Child_Run).
#include "big21.h"

namespace sly4::obj {

namespace {
constexpr u32 kSplineHolder = 0x00F8F158u;     // header*; +16 the spline map
constexpr u32 kCameraDefHolder = 0x00F8F134u;  // header*; +16 the camera def map
constexpr u32 kHopRect = 0x00405F80u;  // f32 1.0, 0.5, 2.0, 0.0, 10.0, 3.1415, -10.0, 20.0, 1/65535, 2.0, 0.25, 0.2, -3.1415
constexpr u32 Anim_Restart = 0x002775A8;        // (an, key, a, b; f, f, c, d; f, f, e, g)
constexpr u32 Effect_FindComponent = 0x004BB4B4;  // (Effect*, u32 hash) -> component or 0
constexpr u32 Mat34_FromRotPos = 0x00553B4C;    // (out, const f32 rows[3][4], const Vec3* pos)
constexpr u32 Quat_FromMatrix = 0x00555478;     // (Quat* out, const Mat34*)
constexpr u32 SplineMap_Find = 0x007335C4;      // (map, const u32* key) -> entry* (entry->0 the spline)
constexpr u32 CameraDefMap_Find = 0x0073374C;   // (map, const u32* key) -> entry* (entry->0 the def)

// The spawner child.
struct HopChild {
    pad<4> _0;
    be_u32 owner;       // +4 (+96 its rows, +144 its position)
    pad<4> _8;
    be_u32 splines[4];  // +12 .. +24
    be_u32 fx;          // +28 (+64 its position)
    pad<12> _32;
    be_u32 camdef;      // +44: the camera def key
    pad<36> _48;
    be_u32 spawner;     // +84
    pad<8> _88;
    Vec3 start;         // +96
    pad<4> _108;
    be_u32 state;       // +112
    be_f32 clock;       // +116
    be_f32 until;       // +120
    be_f32 dur;         // +124
    be_f32 height;      // +128
    pad<4> _132;
    be_f32 pitch;       // +136
    pad<4> _140;
    be_f32 yaw;         // +144
    pad<12> _148;
    Vec4 quat;          // +160
    Vec3 dir;           // +176
    pad<4> _188;
    Vec3 pos;           // +192
    pad<4> _204;
    be_f32 weights[4];  // +208 .. +220
};
static_assert(offsetof(HopChild, fx) == 28 && offsetof(HopChild, spawner) == 84 && offsetof(HopChild, state) == 112);
static_assert(offsetof(HopChild, pitch) == 136 && offsetof(HopChild, quat) == 160 && offsetof(HopChild, weights) == 208);
struct HopOwner {
    pad<96> _0;
    Mtx44 m;            // +96
};
// The spawner: +88 its def (+40 the total), +204 the count, u8 +231.
struct HopSpawner {
    pad<88> _0;
    be_u32 def;         // +88
    pad<112> _92;
    be_s32 count;       // +204
    pad<23> _208;
    u8 third;           // +231
};
static_assert(offsetof(HopSpawner, count) == 204 && offsetof(HopSpawner, third) == 231);
// The original's locals (sp offsets - 144).
struct HopLocals {
    Vec4 p;             // sp+144: the hop point
    Vec4 quat;          // sp+160
    Vec4 rows[3];       // sp+176
    Mtx44 m;            // sp+224
    pad<96> _288;
    be_u32 keys[10];    // sp+384 .. sp+420
};
static_assert(offsetof(HopLocals, keys) == 240);

f64 C(u32 off) { return lf(global<be_f32>(kHopRect + off)); }

// The map entry found for key (stored at *at), its first word.
u32 FindSpline(be_u32& at, u32 key) {
    at = key;
    return gm<be_u32>(call<u32>(SplineMap_Find, gm<be_u32>(global<be_u32>(kSplineHolder) + 16).get(), &at));
}
}  // namespace

// void Child_Hop405FB8(Child* c; f32 dt) (no caller reads r3 or f1): state
// 11. Needs the target effect c->28 and the owner c->4. The clock c->116 += dt, t =
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
void Child_Hop405FB8(HopChild* c, f64 dt) {
    GuestFrame frame(496);
    HopLocals& L = *frame.as<HopLocals>(144);
    auto key = [&](u32 spoff) -> be_u32& { return L.keys[(spoff - 384) / 4]; };
    const u32 fx = c->fx;
    if (fx == 0) return;
    const u32 own = c->owner;
    if (own == 0) return;
    const Vec3& ft = gm<Vec3>(fx + 64);
    const f64 clk = S(lf(c->clock) + dt);
    f64 t = Divs(clk, lf(c->dur));
    c->clock = static_cast<f32>(clk);
    const f64 one = C(0);
    if (t > one) t = one;
    const f64 x0 = lf(c->start.x), y0 = lf(c->start.y), z0 = lf(c->start.z);
    const f64 dx = S(lf(ft.x) - x0);
    const f64 px = Fmadds(dx, t, x0);
    const f64 dy = S(lf(ft.y) - y0);
    const f64 py = Fmadds(dy, t, y0);
    const f64 dz = S(lf(ft.z) - z0);
    L.p.x = static_cast<f32>(px);
    L.p.z = static_cast<f32>(Fmadds(dz, t, z0));
    const f64 fy = lf(ft.y);  // the target height, kept for the rise test
    const f64 u = S(S(t - C(4)) * C(8));
    L.p.y = static_cast<f32>(Fmadds(lf(c->height), Fnmsubs(u, u, one), py));
    HopOwner& ow = gm<HopOwner>(own);
    for (u32 row = 0; row < 3; ++row) CopyXYZ(L.rows[row], ow.m.r[row]);
    call(Mat34_FromRotPos, &L.m, &L.rows[0], &L.p);
    vcall(&ow, 84, &L.m);
    if (!(lf(c->clock) > lf(c->until)) && !(t == one)) return;

    // ---- state 12: pick the orbit
    c->state = 12u;
    {
        be_u32& state = global<be_u32>(kGameRandState);
        const u32 seed = state;
        const u32 mix = GameRandMix(seed);
        state = std::rotl(mix, 16);
        global<be_u64>(kGameRandLast) = static_cast<u64>(mix & 0xFFFFu);
        f64 w[4];
        if (seed % 9 == 0) {
            for (u32 k = 0; k < 4; ++k) c->splines[k] = FindSpline(key(416 - 4 * k), 0xF425FD11u);
            w[0] = one;
            w[1] = w[2] = w[3] = C(12);
        } else {
            for (u32 k = 0; k < 4; ++k) c->splines[k] = FindSpline(key(400 - 4 * k), 0xF425FD12u + k);
            const f64 scale = C(32), ten = C(16), lo = C(44);
            GameRng g;
            g.Load();
            f64 r[4], sum;
            // The first four draws are stored; redraws (sum < 0.2) run in
            // registers and are stored once after the loop.
            auto draw4 = [&] {
                for (u32 k = 0; k < 4; ++k) r[k] = S(S(g.Next() * scale) * ten);
                sum = S(S(S(r[0] + r[1]) + r[2]) + r[3]);
            };
            draw4();
            g.Commit();
            if (sum < lo) {
                do draw4();
                while (sum < lo);
                g.Commit();
            }
            for (u32 k = 0; k < 4; ++k) w[k] = Divs(r[k], sum);
        }
        for (u32 k = 0; k < 4; ++k) c->weights[k] = static_cast<f32>(w[k]);
    }
    const f64 zero = C(12);
    set_bits(c->clock, 0u);
    set_bits(c->dur, 0x40000000u);  // 2.0f
    const Vec3& ft2 = gm<Vec3>(c->fx + 64);
    const f64 ex = S(lf(ft2.x) - lf(c->start.x));
    const f64 ey = S(lf(ft2.y) - lf(c->start.y));
    const f64 ez = S(lf(ft2.z) - lf(c->start.z));
    key(384) = c->camdef.get();
    c->dir.x = static_cast<f32>(ex);
    c->dir.y = static_cast<f32>(ey);
    c->dir.z = static_cast<f32>(ez);
    const u32 def = gm<be_u32>(call<u32>(CameraDefMap_Find, gm<be_u32>(global<be_u32>(kCameraDefHolder) + 16).get(), &key(384)));
    if (def != 0) CopyXYZ(c->dir, gm<Vec3>(def + 112));
    set_bits(c->dir.y, 0u);
    {
        const f64 vy = lf(c->dir.y), vx = lf(c->dir.x), vz = lf(c->dir.z);
        const f64 l = Divs(one, Sqrts(Fmadds(vz, vz, Fmadds(vx, vx, S(vy * vy)))));
        c->dir.x = static_cast<f32>(S(vx * l));
        c->dir.y = static_cast<f32>(S(vy * l));
        c->dir.z = static_cast<f32>(S(vz * l));
    }
    HopOwner& ow2 = gm<HopOwner>(c->owner);
    {
        const f64 ten = C(16);
        const f64 vx = lf(c->dir.x), vy = lf(c->dir.y), vz = lf(c->dir.z);
        c->dir.x = static_cast<f32>(S(vx * ten));
        c->dir.y = static_cast<f32>(S(vy * ten));
        c->dir.z = static_cast<f32>(S(vz * ten));
    }
    CopyXYZ(c->pos, ow2.m.r[3]);
    call(Quat_FromMatrix, &L.quat, &ow2.m);
    CopyXYZ(c->quat, L.quat);
    set_bits(c->quat.w, bits(L.quat.w));
    const f64 rise = S(lf(L.p.y) - fy);
    const f64 pi = C(20), neg10 = C(24), twenty = C(28), scale = C(32);
    {
        GameRng g;
        g.Load();
        const f64 r0 = S(g.Next() * scale);
        const f64 r1 = S(g.Next() * scale);
        f64 pitch, yaw, lift;
        if (rise < one) {
            pitch = S(r0 * pi);
            yaw = Fmadds(r1, twenty, neg10);
            lift = C(36);
        } else {
            pitch = Fmadds(r0, pi, C(48));  // -3.1415
            yaw = zero;
            lift = Fmadds(r1, twenty, neg10);
        }
        g.Commit();
        c->pitch = static_cast<f32>(pitch);
        c->yaw = static_cast<f32>(yaw);
        c->dir.y = static_cast<f32>(lift);
    }
    if (vcall<u32>(&gm<u8>(c->owner), 60) != 0) {
        const u32 an = vcall<u32>(&gm<u8>(c->owner), 60);
        call(Anim_Restart, an, 0xD6A0734Fu, 0u, 0u, one, C(40), 0u, 0u, zero, zero, 0u, -1);
    }
    HopOwner& o5 = gm<HopOwner>(c->owner);
    const u32 e = vcall<u32>(&o5, 272);
    if (e != 0) {
        const u32 p = call<u32>(Effect_FindComponent, e, 0x87208BC1u);
        if (p != 0 && vcall<u32>(&gm<u8>(p), 32) == 4) CopyMtxXYZ(gm<Mtx44>(p + 32), o5.m);
        call(fn::Effect_SetHash, e, 0x80F43146u);
    }
    // The spawner's child count against thirds of its def's total.
    HopSpawner& s = gm<HopSpawner>(c->spawner);
    const s32 was = s.count;
    const s32 now = was + 1;
    s.count = now;
    const s32 total = gm<be_s32>(s.def + 40);
    const s32 third = total / 3;
    if (now >= third && was < third) s.third = 1;
    const s32 two = static_cast<s32>(static_cast<u32>(total) << 1) / 3;
    if (now >= two && was < two) s.third = 1;
}

}  // namespace sly4::obj

NATIVE(00405FB8, sly4::obj::Child_Hop405FB8)
