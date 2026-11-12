// Game code, large-function sweep part 25: a lever / sliding switch prop's tick
// (sibling of the sway prop tick 003FA110 in big24_sway.cpp).
#include "big25.h"
#include "native/vec.h"

namespace sly4::obj {

namespace {
constexpr u32 kCLever = 0x003FAF34;  // .text f32 table (0.5, -0.6, 0.6, -0.2, 0.2, 1, 0, -1, -pi, 180, pi, -pi/4, pi/4)
constexpr u32 kHashFast = 0x2874B3ADu;   // effect while |speed| > 0.6
constexpr u32 kHashMove = 0x287AC552u;   // effect while |speed| > 0.2
constexpr u32 kHashEnd = 0xEA31D667u;    // effect when the travel first reaches an end
constexpr u32 kPointKey = 0xD906B0C2u;   // PointMap key of the reference frame object
constexpr u32 kChildHash = 0x435538CBu;  // the child that gets vfn[88](0, 1, 1, 1)
constexpr u32 Math_Sinf = 0x007345F0;          // f1 -> f1
constexpr u32 Math_Cosf = 0x007344E0;          // f1 -> f1
constexpr u32 Quat_ToMatrix = 0x00553A90;      // (Mat44* out, const Quat*, const Vec3* pos)
constexpr u32 Matrix_MulRot = 0x00553FF0;      // (out, a, b)
constexpr u32 PointMap_Find = 0x0073343C;      // (map, const u32* key) -> value slot*
constexpr u32 Obj_SetEffectHash = 0x000DAEC8;  // Obj_SetEffectHash_000DAEC8 (Obj*, u32 hash)
constexpr u32 Obj_FindByHash = 0x000DB290;     // Obj_FindByHash_000DB290 (Obj*, u32 hash) -> child or 0
constexpr u32 MapHolderPoint = 0x00F8F154;     // header*; +16 the map PointMap_Find searches
constexpr u32 AxisVec = 0x01164B70;            // the global vector after UpVec (lever hinge axis)

struct LeverParams {
    pad<24> _0;
    be_f32 gain;   // +24
    be_f32 damp;   // +28
};
struct Lever {
    pad<88> _0;
    be_u32 params;     // +88: LeverParams*
    pad<12> _92;
    be_f32 x;          // +104: the travel in [-1, 1]
    be_f32 v;          // +108: the speed
    be_f32 mass;       // +112
    pad<8> _116;
    be_f32 angle;      // +124
    pad<96> _128;
    Vec3 point;        // +224
    pad<20> _236;
    be_u32 body;       // +256: its +96 matrix
    pad<32> _260;
    be_u32 holder;     // +292: the effect holder (vfn[272])
    pad<4> _296;
    be_u32 model;      // +300
    be_f32 push;       // +304
    pad<28> _308;
    Vec3 vec;          // +336
    pad<4> _348;
    u8 latched;        // +352
    u8 nolatch;        // +353
};
static_assert(offsetof(Lever, x) == 104 && offsetof(Lever, angle) == 124 && offsetof(Lever, point) == 224);
static_assert(offsetof(Lever, holder) == 292 && offsetof(Lever, push) == 304 && offsetof(Lever, vec) == 336);
static_assert(offsetof(Lever, nolatch) == 353);
// The original's locals (sp offsets - 112).
struct LeverLocals {
    Vec4 quat;     // sp+112
    Vec4 pt;       // sp+128
    Mtx44 qm;      // sp+144
    Mtx44 m;       // sp+208
    be_u32 key;    // sp+272
};
static_assert(offsetof(LeverLocals, key) == 160);

f64 C(u32 off) { return lf(global<be_f32>(kCLever + off)); }
f32 Fw(const be_f32& v) { return std::bit_cast<f32>(bits(v)); }

// holder->vfn[272]() gives a thing whose +4 is the current effect (its +8
// the hash); unless that is already `hash`, Obj_SetEffectHash(o->+292,
// hash). Returns whether the hash was already current.
bool EnsureEffect(Lever* o, u32 holder, u32 hash) {
    const u32 r = vcall<u32>(&gm<u8>(holder), 272);
    const u32 e = gm<be_u32>(r + 4);
    const u32 obj = o->holder;
    const u32 cur = e != 0 ? gm<be_u32>(e + 8).get() : 0u;
    if (cur == hash) return true;
    call(Obj_SetEffectHash, obj, hash);
    return false;
}

// The end-of-travel latch: unless +353, (when +352 is still clear)
// Effect_SetHash4CF988(holder->vfn[272](), 0xEA31D667); then +352 = 1 and
// the vector +336 updated by upd. Returns the travel value the original
// carries on with (x re-read after the effect call).
template <typename Upd>
f64 Latch(Lever* o, f64 f1, Upd upd) {
    if (o->nolatch != 0) return f1;
    if (o->latched == 0) {
        call(fn::Effect_SetHash, vcall<u32>(&gm<u8>(o->holder), 272), kHashEnd);
        f1 = lf(o->x);
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
//    vfn[88](0, 1, 1, 1). No caller reads r3 or f1 (r3.sh).
void Prop_LeverTick_003FAF68(Lever* o, f32 dt_) {
    const f64 dt = dt_;
    GuestFrame frame(336);
    LeverLocals& L = *frame.as<LeverLocals>(112);
    const u32 holder = o->holder;

    // 1. Travel.
    if (o->latched == 0) {
        const LeverParams& p = gm<LeverParams>(o->params);
        const f64 v0 = lf(o->v);
        const f64 a = Divs(Fmsubs(lf(o->push), lf(p.gain), S(v0 * lf(p.damp))), lf(o->mass));
        const f64 xa = Fmadds(v0, dt, lf(o->x));
        const f64 v = Fmadds(a, dt, v0);
        o->v = static_cast<f32>(v);
        o->x = static_cast<f32>(Fmadds(S(S(a * C(0)) * dt), dt, xa));
        bool mid = true;
        f64 vc = v;
        if (v < C(4) || v > C(8)) {
            mid = EnsureEffect(o, holder, kHashFast);
            vc = lf(o->v);  // the speed as reloaded before the test (the original's f1)
        }
        if (mid && (vc < C(12) || vc > C(16))) EnsureEffect(o, o->holder, kHashMove);

        f64 f1 = lf(o->x);
        const f64 one = C(20);
        if (f1 > one) {
            f1 = one;
            set_bits(o->x, 0x3F800000u);
            const f64 zero = C(24);
            if (lf(o->v) > zero) o->v = 0.0f;
            f1 = Latch(o, f1, [&] {
                const f64 nx = NegAdds(lf(o->vec.x), zero), ny = NegAdds(lf(o->vec.y), one), nz = NegAdds(lf(o->vec.z), zero);
                o->latched = 1;
                o->vec.x = static_cast<f32>(nx);
                o->vec.y = static_cast<f32>(ny);
                o->vec.z = static_cast<f32>(nz);
            });
        }
        if (f1 < C(28)) {
            set_bits(o->x, 0xBF800000u);
            const f64 zero = C(24);
            if (lf(o->v) < zero) o->v = 0.0f;
            Latch(o, f1, [&] {
                const f64 nx = Adds(lf(o->vec.x), zero), ny = Adds(lf(o->vec.y), one), nz = Adds(lf(o->vec.z), zero);
                o->latched = 1;
                o->vec.x = static_cast<f32>(nx);
                o->vec.y = static_cast<f32>(ny);
                o->vec.z = static_cast<f32>(nz);
            });
        }
    }

    // 2. Angle.
    const f64 t = lf(o->angle);
    f64 ang = Divs(S(t * C(32)), C(36));
    const f64 pos = Divs(S(t * C(40)), C(36));
    if (Adds(ang, pos) < C(44)) ang = NegSubs(pos, C(48));
    if (Adds(ang, pos) > C(48)) ang = Subs(C(48), pos);
    if (gm<be_u32>(global<be_u32>(kWorld) + 0x1281E0u) == 0u) return;

    // 3. Hinge rotation.
    const Vec3& axis = global<Vec3>(AxisVec);
    const f64 ax = lf(axis.x), ay = lf(axis.y), az = lf(axis.z);
    const f64 len = Sqrts(Fmadds(az, az, Fmadds(ax, ax, S(ay * ay))));
    const f64 half = NegMuls(ang, C(0));
    const f64 inv = Divs(C(20), len);
    const f64 nx = S(ax * inv), ny = S(ay * inv), nz = S(az * inv);
    const f64 s = call<f64>(Math_Sinf, half);
    const f64 qx = S(s * nx), qy = S(s * ny), qz = S(s * nz);
    const f64 c = call<f64>(Math_Cosf, half);
    L.quat.x = static_cast<f32>(qx);
    L.quat.y = static_cast<f32>(qy);
    L.quat.z = static_cast<f32>(qz);
    L.quat.w = static_cast<f32>(c);
    call(Quat_ToMatrix, &L.qm, &L.quat, kZeroVec);
    const u32 map = gm<be_u32>(global<be_u32>(MapHolderPoint) + 16);
    L.key = kPointKey;
    const u32 slot = call<u32>(PointMap_Find, map, &L.key);
    const u32 frm = gm<be_u32>(slot);
    if (frm == 0) return;
    vcall(o, 76);
    call(Matrix_MulRot, &L.m, &L.qm, frm + 16);
    const Mtx44& rows = gm<Mtx44>(o->body + 96);
    const V4 p = Add4(Lvx(&rows.r[3]), Row3(Lvx(&rows.r[0]), Lvx(&rows.r[1]), Lvx(&rows.r[2]), Fw(o->point.x), Fw(o->point.y), Fw(o->point.z)));
    Stvx(&L.pt, p);
    CopyXYZ(L.m.r[3], L.pt);
    vcall(&gm<u8>(o->model), 84, &L.m);
    vcall(&gm<u8>(o->model), 88, 1u, 1u, 1u, 1u);
    if (call<u32>(Obj_FindByHash, o->model.get(), kChildHash) == 0) return;
    vcall(&gm<u8>(call<u32>(Obj_FindByHash, o->model.get(), kChildHash)), 88, 0u, 1u, 1u, 1u);
}

}  // namespace sly4::obj

NATIVE(003FAF68, sly4::obj::Prop_LeverTick_003FAF68)
