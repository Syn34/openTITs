// Game code, large-function sweep part 22: the 448-byte spawned physics
// object built by SpawnerC0FE8 / Obj_SlotApply_000B6208 / f_000B3A60.
#include "big22.h"

namespace sly4::obj {

namespace {
constexpr u32 kSpawnK = 0x000B2DD0u;   // f32 1.0, 1e-5; masks +16 / +32 / +48; f32 1/65535 +64, -1.0 +68
constexpr u32 kSpawnVt = 0x00B12080u;  // the object's vtable
constexpr u32 kListenerVt = 0x00B12204u;  // the embedded listener's (+380) vtable
constexpr u32 kUnknownName = 0x00B0DA98u;  // "<unknown>"
constexpr u32 kUpVec = 0x01164B60u;    // f32[3] (Constants_Init_0056B93C)
constexpr u32 Thunk_000E02D8 = 0x000E02D8;         // (o, QueryEx*): the base object constructor (0x000E01C0)
constexpr u32 StateMachine_SetState = 0x00464D1C;  // (o, level, id)
constexpr u32 HkBody_SetName = 0x007CE950;         // (body + 120, const char*)
constexpr u32 HkBody_AddListener = 0x0091A82C;     // (body, listener)
constexpr u32 Hk_SetTransform = 0x0091EAB8;        // (body, const f32 m[4][4])
constexpr u32 Hk_AddProperty = 0x0093FE00;         // (entity, u32 key, u64 value)

}  // namespace

// The construction query: +208 / +224 the box corners, +240 / +252
// copied, +244 / +248 the anchor weights.
struct SpawnQuery {
    pad<208> _0;
    Vec4 lo;         // +208
    Vec4 hi;         // +224
    be_u32 w240;     // +240
    be_f32 up;       // +244
    be_f32 len;      // +248
    be_u32 w252;     // +252
};
static_assert(offsetof(SpawnQuery, w240) == 240 && offsetof(SpawnQuery, w252) == 252);
// The source def (p->216): +208 / +224 the random range, +240 the body factory.
struct SpawnSrc {
    pad<208> _0;
    Vec4 lo;         // +208
    Vec4 hi;         // +224
    be_u32 factory;  // +240
};
struct SpawnedObj {
    be_u32 vt;       // +0
    pad<16> _4;
    be_u32 out;      // +20: the random vector's destination
    pad<72> _24;
    Mtx44 m;         // +96 (+144 the position)
    be_u32 name;     // +160 (vfn[3] its string)
    pad<52> _164;
    be_u32 src;      // +216: SpawnSrc*
    pad<160> _220;
    be_u32 listener; // +380: the listener's vtable
    Vec4 box[3];     // +384: min / anchor / max
    be_u32 body;     // +432
    be_u32 w436;     // +436
    be_u32 w440;     // +440
};
static_assert(offsetof(SpawnedObj, name) == 160 && offsetof(SpawnedObj, src) == 216 && offsetof(SpawnedObj, listener) == 380);
static_assert(offsetof(SpawnedObj, box) == 384 && offsetof(SpawnedObj, body) == 432);

// void Spawned_Construct_000B2E18(Obj* p, QueryEx* q): the base object
// (Thunk_000E02D8), vtable 0x00B12080, body +432 = 0, +436 / +440 = q->240 /
// q->252, listener +380 (vtable 0x00B12204). The box +384..+424 (16-byte
// rows zeroed first) gets the corners q+208 / q+224; the anchor +400 = min +
// dir * (|max - min| * q->248) + kUpVec * q->244, dir the normalised box
// diagonal (left unnormalised, with the length taken as 1.0, when its square
// is below 1e-5 or NaN); +144..152 = the min corner. With a body factory
// p->216->240, its vfn[8] makes the body (+432), named by p->160's vfn[3] (or
// "<unknown>"), posed from the rows +96 / +112 / +128 / +144 (w = 0), given
// the properties 0xA734004D = 0x7A30D7E3 and 0x91C7CF5B = p and the listener.
// Then state (0, 0), and p->20 gets a random vector: per axis lerp(p->216's
// +208, +224, r) with r a draw of the game's generator, times -1 or (odd
// seed bit) 1. No caller reads r3 or f1.
void Spawned_Construct_000B2E18(SpawnedObj* p, SpawnQuery* q) {
    GuestFrame frame(64);
    Vec4* rows = frame.as<Vec4>();
    call(Thunk_000E02D8, p, q);
    p->body = 0u;
    p->vt = kSpawnVt;
    p->w436 = q->w240.get();
    p->listener = kListenerVt;
    p->w440 = q->w252.get();
    SpawnSrc& src = gm<SpawnSrc>(p->src);
    Quad* zb = &gm<Quad>(guest(&p->box[0]) & ~15u);  // stvx x3: aligned 16-byte rows
    for (u32 k = 0; k < 3; ++k) zb[k] = Quad{};
    const f64 a0 = lf(q->lo.x), a1 = lf(q->lo.y), a2 = lf(q->lo.z);
    CopyXYZ(p->box[0], q->lo);
    const f64 b0 = lf(q->hi.x), b1 = lf(q->hi.y), b2 = lf(q->hi.z);
    CopyXYZ(p->box[2], q->hi);
    f64 d0 = S(b0 - a0), d1 = S(b1 - a1), d2 = S(b2 - a2);
    const f64 one = lf(global<be_f32>(kSpawnK));
    const f64 lenSel = lf(q->len), upSel = lf(q->up);
    const u32 factory = src.factory;
    const f64 sq = Fmadds(d2, d2, Fmadds(d0, d0, S(d1 * d1)));
    f64 len = one;
    const f64 eps = lf(global<be_f32>(kSpawnK + 4));
    if (sq > eps || sq == eps) {
        len = Sqrts(sq);
        const f64 inv = Divs(one, len);
        d0 = S(inv * d0);
        d1 = S(inv * d1);
        d2 = S(inv * d2);
    }
    const f64 sc = S(len * lenSel);
    f64 x = Fmadds(sc, d0, a0), y = Fmadds(sc, d1, a1), z = Fmadds(sc, d2, a2);
    const Vec3& up = global<Vec3>(kUpVec);
    x = Fmadds(lf(up.x), upSel, x);
    y = Fmadds(lf(up.y), upSel, y);
    z = Fmadds(lf(up.z), upSel, z);
    p->box[1].x = static_cast<f32>(x);
    p->box[1].y = static_cast<f32>(y);
    p->box[1].z = static_cast<f32>(z);
    CopyXYZ(p->m.r[3], q->lo);  // stfs of the loaded q->208 (+148 / +152 from +388 / +392, the same words)
    if (factory != 0) {
        const u32 body = vcall<u32>(&gm<u8>(factory), 32);
        p->body = body;
        const u32 nm = p->name;
        const u32 name = nm != 0 ? vcall<u32>(&gm<u8>(nm), 12) : kUnknownName;
        call(HkBody_SetName, body + 120, name);
        const u32 b = p->body;
        for (u32 r = 0; r < 4; ++r) {
            CopyXYZ(rows[r], p->m.r[r]);
            set_bits(rows[r].w, 0u);
        }
        call(Hk_SetTransform, b, rows);
        call(Hk_AddProperty, p->body.get(), 0xA734004Du, 0x7A30D7E3u);
        call(Hk_AddProperty, p->body.get(), 0x91C7CF5Bu, p);
        call(HkBody_AddListener, p->body.get(), &p->listener);
    }
    call(StateMachine_SetState, p, 0u, 0u);
    GameRng rng;
    rng.Load();
    const f64 rk = lf(global<be_f32>(kSpawnK + 64)), neg = lf(global<be_f32>(kSpawnK + 68));
    const u32 out = p->out;
    const be_f32* lo = &src.lo.x;
    const be_f32* hi = &src.hi.x;
    f64 v[3];
    for (u32 k = 0; k < 3; ++k) {
        const f64 l = lf(lo[k]), h = lf(hi[k]);
        const f64 r = S(rng.Next() * rk);
        const f64 t = Fmadds(S(h - l), r, l);
        const u32 bit = rng.seed & 1;
        rng.Step();
        v[k] = S(t * (bit != 0 ? one : neg));
    }
    rng.Commit();
    Vec3& o = gm<Vec3>(out);
    o.x = static_cast<f32>(v[0]);
    o.y = static_cast<f32>(v[1]);
    o.z = static_cast<f32>(v[2]);
}

}  // namespace sly4::obj

NATIVE(000B2E18, sly4::obj::Spawned_Construct_000B2E18)
