// Game code, large-function sweep part 24: a swaying / tilting prop's tick.
#include "big24.h"

namespace sly4::obj {

namespace {
constexpr u32 kSinf = 0x007345F0;           // f1 -> f1
constexpr u32 kCosf = 0x007344E0;           // f1 -> f1
constexpr u32 kMat34FromRotPos = 0x00553B4C;  // (Mat34* out, const Mat33*, const Vec3*)
constexpr u32 kVec3ToItemSpace = 0x00553468;  // (f32 v[3] in/out, const f32 m[3][4])
constexpr u32 kQuatToMatrix = 0x00553A90;   // (Mat44* out, const Quat*, const Vec3* pos)
constexpr u32 kMatrixMulRot = 0x00553FF0;   // (out, a, b)
constexpr u32 kSetEffectHash = 0x000DAEC8;  // Obj_SetEffectHash_000DAEC8 (Obj*, hash)
constexpr u32 kC = 0x003FA0D0;              // .text f32 table (2pi, 8.6, 1.2, 0.2, 11.7, 2.2, 7, 3.7, 5, 0.5, 0, -pi, 180, 1, 0.001, 1e-4)
constexpr u32 kUp = 0x01164B60;
constexpr u32 kHashTiltUp = 0x603D2AEEu;    // effect when the tilt speed turns positive
constexpr u32 kHashTiltDown = 0xA7BE93F5u;  // ... negative

struct SwayParams {
    pad<32> _0;
    be_f32 amp;    // +32
    pad<8> _36;
    be_f32 limit;  // +44
};
// The body: +96 its rows (+144 the position); vfn[84] places it, vfn[272]
// its effect holder.
struct SwayBody {
    pad<96> _0;
    Mtx44 m;       // +96
};
struct SwayProp {
    pad<88> _0;
    be_u32 params;      // +88: SwayParams*
    pad<32> _92;
    be_f32 angle;       // +124
    pad<4> _128;
    be_f32 speed;       // +132
    be_f32 base_y;      // +136
    be_f32 input;       // +140
    be_f32 phase[3];    // +144 / +148 / +152
    pad<4> _156;
    pad<48> frame;      // +160: the frame (Vec3_ToItemSpace / Matrix_MulRot)
    Vec3 pos;           // +208
    pad<36> _220;
    be_u32 body;        // +256: SwayBody*
    pad<108> _260;
    be_s32 dir;         // +368: the tilt direction (1 / -1)
};
static_assert(offsetof(SwayProp, angle) == 124 && offsetof(SwayProp, phase) == 144 && offsetof(SwayProp, frame) == 160);
static_assert(offsetof(SwayProp, pos) == 208 && offsetof(SwayProp, body) == 256 && offsetof(SwayProp, dir) == 368);
// The original's locals (sp offsets).
struct SwayLocals {
    Vec4 pos;      // sp+112
    Vec4 va;       // sp+128
    Vec4 vb;       // sp+144
    Vec4 q;        // sp+160
    Vec4 rot[3];   // sp+176: a Mat33
    Mtx44 qm;      // sp+224
    Mtx44 m2;      // sp+288
    Mtx44 m1;      // sp+352
};
static_assert(offsetof(SwayLocals, qm) == 112 && offsetof(SwayLocals, m1) == 240);

f64 C(u32 off) { return lf(global<be_f32>(kC + off)); }
SwayBody* Body(const SwayProp* o) { return host<SwayBody>(o->body); }
}  // namespace

// void f(Prop* o; f32 dt) (called by 003FC468, 003FD8D0; no caller reads
// r3 or f1, r3.sh). o->+88 = params (+32 bob amplitude, +44 tilt limit),
// o->+256 the body.
// 1. Bob: three phases +144/+148/+152 (periods 8.6, 11.7, 7; each phase
//    advances by dt and wraps by subtracting 8.6, 3.7 (sic) and 7 when
//    past its period) give y = sum of amp * sin(2pi * p / T + phase)
//    weighted 1, 0.2, 0.2 (as fmadds) plus +136; the body (vfn[84]) gets
//    Mat34_FromRotPos(its rows +96/+112/+128, o->+208 with y added).
// 2. Tilt: +124 angle, +132 speed: target clamp(+140 * limit, +-limit)
//    pulls the speed with gain 5 (acc = (target - angle) * 5 - speed),
//    angle += speed dt + acc dt^2 / 2 (fmadds order kept); clamped to the
//    limit, the speed zeroed when pushing outwards. A sign change of the
//    speed (u32 +368 state 1 / -1) fires the effect 0x603D2AEE /
//    0xA7BE93F5 on the body when its vfn[272] is set. +136 / +140 are
//    cleared.
// 3. With the game's reference object r (0x00F65CA0 -> +0x1281E0): a =
//    r->+96 * cos(-pi angle / 180) + up * sin(...), b = r->+96, both into
//    o's frame +160 (Vec3_ToItemSpace); the rotation from b to a
//    (normalized; half-vector quaternion, or a perpendicular axis when
//    they're opposite) goes through Quat_ToMatrix / Matrix_MulRot(+160)
//    to the body's vfn[84] with the body's position +144.
void Prop_SwayTick_003FA110(SwayProp* o, f64 dt) {
    GuestFrame frame(496);
    SwayLocals& L = *frame.as<SwayLocals>(112);
    auto params = [&]() -> const SwayParams& { return gm<SwayParams>(o->params); };

    // 1. Bob.
    const f64 s1 = call<f64>(kSinf, Adds(S(S(lf(o->phase[0]) * C(0)) / C(4)), C(8)));
    const f64 w = C(12);
    f64 acc = S(s1 * lf(params().amp));
    const f64 s2 = call<f64>(kSinf, Adds(S(S(lf(o->phase[1]) * C(0)) / C(16)), C(20)));
    const f64 t2 = S(S(s2 * lf(params().amp)) * w);
    const f64 a3 = S(S(lf(o->phase[2]) * C(0)) / C(24));
    acc = Fmadds(acc, w, t2);
    const f64 s3 = call<f64>(kSinf, a3);
    const f64 p0 = Adds(lf(o->phase[0]), dt), p1 = Adds(lf(o->phase[1]), dt), p2 = Adds(lf(o->phase[2]), dt);
    const f64 amp = lf(params().amp);
    const f64 base_y = lf(o->base_y);
    SwayBody* body = Body(o);
    o->phase[0] = static_cast<f32>(p0);
    o->phase[1] = static_cast<f32>(p1);
    o->phase[2] = static_cast<f32>(p2);
    const f64 y = Adds(Fmadds(S(s3 * amp), w, acc), base_y);
    if (p0 > C(4)) o->phase[0] = static_cast<f32>(Subs(p0, C(4)));
    if (p1 > C(16)) o->phase[1] = static_cast<f32>(Subs(p1, C(28)));  // quirk: wraps by 3.7, not by the 11.7 period
    if (p2 > C(24)) o->phase[2] = static_cast<f32>(Subs(p2, C(24)));
    set_bits(L.pos.x, bits(o->pos.x));
    L.pos.y = static_cast<f32>(Adds(lf(o->pos.y), y));
    set_bits(L.pos.z, bits(o->pos.z));
    for (u32 r = 0; r < 3; ++r) CopyXYZ(L.rot[r], body->m.r[r]);
    call(kMat34FromRotPos, &L.m1, &L.rot[0], &L.pos);
    vcall(body, 84, &L.m1);

    // 2. Tilt.
    const f64 lim = lf(params().limit);
    f64 tgt = S(lf(o->input) * lim);
    const f64 ang0 = lf(o->angle), spd0 = lf(o->speed);
    SwayBody* body2 = Body(o);
    if (tgt > lim) tgt = lim;
    const f64 nlim = -lim;
    if (tgt < nlim) tgt = nlim;
    f64 ang = Fmadds(spd0, dt, ang0);
    const f64 a = Fmsubs(S(tgt - ang0), C(32), spd0);
    ang = Fmadds(S(S(a * C(36)) * dt), dt, ang);
    f64 spd = Fmadds(a, dt, spd0);
    o->angle = static_cast<f32>(ang);
    o->speed = static_cast<f32>(spd);
    if (ang > lim) {
        ang = lim;
        o->angle = static_cast<f32>(lim);
        if (spd > C(40)) {
            spd = C(40);
            o->speed = 0.0f;
        }
    }
    if (ang < nlim) {
        o->angle = static_cast<f32>(nlim);
        if (spd < C(40)) {
            spd = C(40);
            o->speed = 0.0f;
        }
    }
    if (spd > C(40)) {
        if (o->dir < 0 && vcall<u32>(body2, 272) != 0) call(kSetEffectHash, o->body.get(), kHashTiltUp);
        o->dir = 1;
    } else if (spd < C(40)) {
        if (o->dir > 0 && vcall<u32>(body2, 272) != 0) call(kSetEffectHash, o->body.get(), kHashTiltDown);
        o->dir = -1;
    }
    set_bits(o->base_y, 0u);
    set_bits(o->input, 0u);
    const u32 ref = gm<be_u32>(global<be_u32>(kWorld) + 0x1281E0u);
    if (ref == 0) return;

    // 3. Lean towards the reference object.
    const f64 k_pi = C(44), k_180 = C(48);
    const Vec3& rv = gm<Vec3>(ref + 96);
    const f64 rx = lf(rv.x), ry = lf(rv.y), rz = lf(rv.z);
    const f64 c = call<f64>(kCosf, S(S(lf(o->angle) * k_pi) / k_180));
    const f64 s = call<f64>(kSinf, S(S(lf(o->angle) * k_pi) / k_180));
    void* frm = &o->frame;
    CopyXYZ(L.vb, rv);
    const Vec3& up = global<Vec3>(kUp);
    L.va.x = static_cast<f32>(Fmadds(rx, c, S(lf(up.x) * s)));
    L.va.y = static_cast<f32>(Fmadds(ry, c, S(lf(up.y) * s)));
    L.va.z = static_cast<f32>(Fmadds(rz, c, S(lf(up.z) * s)));
    call(kVec3ToItemSpace, &L.va, frm);
    call(kVec3ToItemSpace, &L.vb, frm);
    const f64 bx0 = lf(L.vb.x), by0 = lf(L.vb.y), bz0 = lf(L.vb.z);
    const f64 ax0 = lf(L.va.x), ay0 = lf(L.va.y), az0 = lf(L.va.z);
    const f64 lb = Fmadds(bz0, bz0, Fmadds(bx0, bx0, S(by0 * by0)));
    const f64 la = Fmadds(az0, az0, Fmadds(ax0, ax0, S(ay0 * ay0)));
    const f64 one = C(52);
    const f64 ib = S(one / S(std::sqrt(lb))), ia = S(one / S(std::sqrt(la)));
    const f64 by = S(ib * by0), bz = S(ib * bz0), bx = S(ib * bx0);
    const f64 ay = S(ia * ay0), ax = S(ia * ax0), az = S(ia * az0);
    const f64 hy = Adds(by, ay), hx = Adds(bx, ax), hz = Adds(bz, az);
    const f64 d = Fmadds(hz, bz, Fmadds(hx, bx, S(hy * by)));
    Vec4& q = L.q;
    if (std::fabs(d) > C(56)) {
        const f64 qy = Fmsubs(bz, ax, S(bx * az));
        const f64 qx = Fmsubs(by, az, S(bz * ay));
        const f64 qz = Fmsubs(bx, ay, S(by * ax));
        const f64 l = Fmadds(d, d, Fmadds(qz, qz, Fmadds(qx, qx, S(qy * qy))));
        const f64 inv = S(one / S(std::sqrt(l)));
        q.x = static_cast<f32>(S(qx * inv));
        q.y = static_cast<f32>(S(qy * inv));
        q.z = static_cast<f32>(S(qz * inv));
        q.w = static_cast<f32>(S(d * inv));
    } else {
        // b and a opposite: a quarter turn about an axis perpendicular to b.
        const bool xbig = bx > by;
        const f64 l = S(std::sqrt(Fmadds(xbig ? bx : by, xbig ? bx : by, S(bz * bz))));
        if (l < C(60)) {
            set_bits(q.x, 0u);
            set_bits(q.y, 0u);
            set_bits(q.z, 0u);
            set_bits(q.w, 0x3F800000u);
        } else if (xbig) {
            const f64 inv = S(one / l);
            set_bits(q.y, 0u);
            set_bits(q.w, 0u);
            q.z = static_cast<f32>(S(inv * bx));
            q.x = static_cast<f32>(-S(inv * bz));
        } else {
            const f64 inv = S(one / l);
            set_bits(q.x, 0u);
            set_bits(q.w, 0u);
            q.y = static_cast<f32>(S(inv * bz));
            q.z = static_cast<f32>(-S(inv * by));
        }
    }
    call(kQuatToMatrix, &L.qm, &q, kZeroVec);
    call(kMatrixMulRot, &L.m2, &L.qm, frm);
    SwayBody* body3 = Body(o);
    CopyXYZ(L.m2.r[3], body3->m.r[3]);
    vcall(body3, 84, &L.m2);
}

}  // namespace sly4::obj

NATIVE(003FA110, sly4::obj::Prop_SwayTick_003FA110)
