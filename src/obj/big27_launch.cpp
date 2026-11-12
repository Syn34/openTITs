// Game code, large-function sweep part 27: an object launching the piece it
// holds (detached, aimed, given linear and random angular velocity).
#include "big27.h"
#include "native/vec.h"

namespace sly4::obj {

namespace {
constexpr u32 kPool3A9 = 0x003A9EB0;  // .text pool: f32 1.0; masks {y} (+16), {z} (+32), {w} (+48); -1.0 (+64), 1/65535 (+68)
constexpr u32 kFxHash = 0xC01710E7u;  // effect hash set on the piece's effect
constexpr u32 kPropIndex = 0x0026E1FEu;  // body property: o->+400 (sign-extended)
constexpr u32 kPropHeldFlag = 0x516CC502u;   // body property: o->+384 bit 0x08000000
constexpr u32 Obj_Detach = 0x000D72C0;              // (holder, Object*)
constexpr u32 Matrix_FromDir = 0x004A54F8;          // (const vec3* dir, Matrix* m)
constexpr u32 Hk_Activate = 0x0091A4C8;             // (body)
constexpr u32 HkBody_AddListener_0091A82C = 0x0091A82C;  // (body, listener)
constexpr u32 HkBody_AddListener_0091A5F8 = 0x0091A5F8;  // (body, listener)
constexpr u32 Array_Reserve = 0x00558F3C;           // (array, u32 n)
constexpr u32 Hk_AddProperty = 0x0093FE00;          // (entity, u32 key, u64 value)

// The launch definition: +512 the origin offset, +528 the launch point,
// +544 / +560 the angular velocity range, +584 the speed.
struct LaunchDef {
    pad<512> _0;
    Vec4 origin;   // +512
    Vec4 point;    // +528
    Vec4 lo;       // +544
    Vec4 hi;       // +560
    pad<8> _576;
    be_f32 speed;  // +584
};
static_assert(offsetof(LaunchDef, point) == 528 && offsetof(LaunchDef, speed) == 584);
// An 80-byte entry of o->+140: +4 the object whose +80 matrix places the
// launch point, +76 the launch count.
struct LaunchEnt {
    pad<4> _0;
    be_u32 obj;    // +4
    pad<68> _8;
    be_u32 count;  // +76
};
static_assert(sizeof(LaunchEnt) == 80);
struct LaunchObj {
    pad<56> _0;
    be_u32 l56;          // +56: a body listener
    be_u32 l60;          // +60: another
    pad<8> _64;
    be_u16 cap;          // +72: the {body, piece} array
    be_u16 size;         // +74
    be_u16 mark;         // +76
    be_u16 aflags;       // +78
    be_u32 data;         // +80
    pad<56> _84;
    be_u32 ents;         // +140: LaunchEnt[]
    pad<68> _144;
    be_u32 def;          // +212: LaunchDef*
    pad<152> _216;
    be_u32 held;         // +368
    be_u32 holder;       // +372
    pad<8> _376;
    be_u32 f384;         // +384
    pad<12> _388;
    be_u32 idx;          // +400
};
static_assert(offsetof(LaunchObj, cap) == 72 && offsetof(LaunchObj, ents) == 140 && offsetof(LaunchObj, def) == 212);
static_assert(offsetof(LaunchObj, held) == 368 && offsetof(LaunchObj, idx) == 400);
// The original's locals (sp offsets - 128).
struct LaunchLocals {
    Vec4 dir;   // sp+128
    Vec4 q;     // sp+144
    Vec4 v;     // sp+160
    Mtx44 m;    // sp+176
};

f32 Fw(const be_f32& v) { return std::bit_cast<f32>(bits(v)); }
}  // namespace

// void f(T* o) (called by Obj_OnEvent_003AA9F0, Obj_OnEventAdj64_003AAA64).
// Without the held piece x (o->+368) or its holder h (o->+372) returns.
// x leaves h (+368 = 0, Obj_Detach). The launch point is the def's
// (o->+212) +528.. taken through the matrix (+80..) of the object at
// o->+140[o->+400] (80-byte entries) +4 when set; x gets h's matrix
// (+96..) re-aimed along the unit direction from h's origin to that point
// (Matrix_FromDir), with def+512.. through it as origin (x->vfn[84]); its
// effect (x->vfn[272]) gets kFxHash. With x's body b (x->vfn[144], its +8
// set): linear velocity = the aim row * def+584, angular velocity = one
// random sign times lerp(def+544.., def+560.., 16-bit randoms / 65535) per
// axis (the global generator advanced four times), both after
// Hk_Activate(b) through b+224's vfn[68] / vfn[72]; o+56 / o+60 listen on
// b; {b, x} is pushed on o's array (+72 u16 capacity, +74 size, +76 high
// mark, +78 |= 1, +80 data); properties kPropIndex / kPropHeldFlag; the entry's
// +76 count is incremented. No caller reads r3 or f1 (r3.sh).
void Obj_LaunchHeld_003A9EF8(LaunchObj* o) {
    const u32 x = o->held;
    if (x == 0 || o->holder == 0u) return;
    o->held = 0u;
    call(Obj_Detach, o->holder.get(), x);
    GuestFrame frame(416);
    LaunchLocals& L = *frame.as<LaunchLocals>(128);
    Mtx44& M = L.m;
    const Mtx44& hm = gm<Mtx44>(o->holder + 96);
    LaunchEnt& ent = gm<LaunchEnt>(o->ents + o->idx * 80);
    const LaunchDef& def = gm<LaunchDef>(o->def);
    Mtx44 m;  // h's rows (x / y / z words; read before any store)
    CopyMtxXYZ(m, hm);
    u32 px = bits(def.point.x), py = bits(def.point.y), pz = bits(def.point.z);
    if (const u32 t = ent.obj; t != 0) {
        const Mtx44& tm = gm<Mtx44>(t + 80);
        const V4 r0 = Lvx(&tm.r[0]), r1 = Lvx(&tm.r[1]), r2 = Lvx(&tm.r[2]), r3 = Lvx(&tm.r[3]);
        const V4 p = Add4N(r3, Row3N(r0, r1, r2, std::bit_cast<f32>(px), std::bit_cast<f32>(py), std::bit_cast<f32>(pz)));
        px = LaneW(p, 0), py = LaneW(p, 1), pz = LaneW(p, 2);
    }
    const f64 dy = Subs(LfsW(py), lf(m.r[3].y)), dx = Subs(LfsW(px), lf(m.r[3].x)), dz = Subs(LfsW(pz), lf(m.r[3].z));
    const f64 len = Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy))));
    const f64 one = lf(global<be_f32>(kPool3A9));
    CopyMtxXYZ(M, m);
    const f64 inv = Divs(one, len);
    L.dir.x = static_cast<f32>(Muls(dx, inv));
    L.dir.y = static_cast<f32>(Muls(dy, inv));
    L.dir.z = static_cast<f32>(Muls(dz, inv));
    call(Matrix_FromDir, &L.dir, &M);
    {
        const Vec4& q = gm<LaunchDef>(o->def).origin;
        CopyXYZ(L.q, q);
        const V4 m0 = Lvx(&M.r[0]), m1 = Lvx(&M.r[1]), m2 = Lvx(&M.r[2]), m3 = Lvx(&M.r[3]);
        Stvx(&L.q, Add4N(m3, Row3N(m0, m1, m2, Fw(q.x), Fw(q.y), Fw(q.z))));
        CopyXYZ(M.r[3], L.q);
    }
    vcall(host<u8>(x), 84, &M);
    if (const u32 e = vcall<u32>(host<u8>(x), 272); e != 0) call(fn::Effect_SetHash, e, kFxHash);
    const u32 b = vcall<u32>(host<u8>(x), 144);
    if (b == 0 || gm<be_u32>(b + 8) == 0u) return;
    const Masks3 mk = LoadMasks3(kPool3A9 + 16);
    auto row = [&](f64 a, f64 c, f64 d) {
        return RowSel(Fill4(static_cast<f32>(a)), Fill4(static_cast<f32>(c)), Fill4(static_cast<f32>(d)), mk);
    };
    {
        const f64 sp = lf(gm<LaunchDef>(o->def).speed);
        Stvx(&L.v, row(Muls(lf(M.r[2].x), sp), Muls(lf(M.r[2].y), sp), Muls(lf(M.r[2].z), sp)));
    }
    call(Hk_Activate, b);
    u8* bi = &gm<u8>(b + 224);
    vcall(bi, 68, &L.v);

    // Angular velocity from four steps of the global generator.
    be_u32& state = global<be_u32>(kGameRandState);
    const u32 s0 = state;
    const f64 sign = (s0 & 1) == 1 ? one : lf(global<be_f32>(kPool3A9 + 64));
    const LaunchDef& d2 = gm<LaunchDef>(o->def);
    const u32 x1 = GameRandMix(s0);
    const u32 x2 = GameRandMix(std::rotl(x1, 16));
    const u32 x3 = GameRandMix(std::rotl(x2, 16));
    const u32 x4 = GameRandMix(std::rotl(x3, 16));
    const f64 sc = lf(global<be_f32>(kPool3A9 + 68));
    auto rnd = [](u32 v) { return S(static_cast<f64>(v & 0xFFFF)); };
    const f64 lo0 = lf(d2.lo.x), lo1 = lf(d2.lo.y), lo2 = lf(d2.lo.z);
    const f64 w0 = Muls(Fmadds(Subs(lf(d2.hi.x), lo0), Muls(rnd(x1), sc), lo0), sign);
    const f64 w1 = Muls(Fmadds(Subs(lf(d2.hi.y), lo1), Muls(rnd(x2), sc), lo1), sign);
    state = std::rotl(x4, 16);
    global<be_u64>(kGameRandLast) = static_cast<u64>(x4 & 0xFFFF);
    const f64 w2 = Muls(Fmadds(Subs(lf(d2.hi.z), lo2), Muls(rnd(x3), sc), lo2), sign);
    Stvx(&L.v, row(w0, w1, w2));
    call(Hk_Activate, b);
    vcall(bi, 72, &L.v);
    call(HkBody_AddListener_0091A82C, b, &o->l56);
    call(HkBody_AddListener_0091A5F8, b, &o->l60);

    // Push {b, x}.
    u32 n = o->size;
    if (const u32 cap = o->cap; n == cap) {
        call(Array_Reserve, &o->cap, (cap << 1) & 0xFFFE);
        n = o->size;
    }
    const u32 data = o->data;
    gm<be_u32>(data + ((n << 3) & 0x7FFF8u)) = b;
    gm<be_u32>(data + ((static_cast<u32>(o->size) << 3) & 0xFFFFFFF8u) + 4) = x;
    const u32 n1 = o->size + 1u;
    u32 hi = o->mark;
    const u32 fl = o->aflags;
    o->size = static_cast<u16>(n1);
    const u32 idx = o->idx, w384 = o->f384;
    if (!(static_cast<s32>(hi) > static_cast<s32>(n1 & 0xFFFF))) hi = n1;
    o->mark = static_cast<u16>(hi);
    o->aflags = static_cast<u16>(fl | 1);
    call(Hk_AddProperty, b, kPropIndex, static_cast<s64>(static_cast<s32>(idx)));
    call(Hk_AddProperty, b, kPropHeldFlag, u64{(w384 >> 27) & 1});
    ent.count = ent.count + 1u;
}

}  // namespace sly4::obj

NATIVE(003A9EF8, sly4::obj::Obj_LaunchHeld_003A9EF8)
