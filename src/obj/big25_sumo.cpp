// Game code, large-function sweep part 25: the sumo minigame's target
// spawner tick (state machine; state 0 picks a random spline and spawns the
// "Target" effect object along it).
#include "big25.h"

namespace sly4::obj {

namespace {
constexpr u32 kCSumo = 0x0040E9EC;              // .text f32 table: 1/65535, 0.7, 0.8, 0, 1, 0.25
constexpr u32 kSplineHolder = 0x00F8F158;       // header*; +16 the map SplineMap_Find searches
constexpr u32 kSplineDefault = 0xE720C39Cu;     // the spline used when the game is not in progress
constexpr u32 kAnimKey = 0x8919BDE9u;           // the animation restarted on the animator
constexpr u32 kProfileKey = 0xC5877D2Du;        // Save_Profile0C9FC8 key (the target effect def)
constexpr u32 kPoolMap = 0x01013B1C;            // SortedMap_Find732F38 map of pooled spawn descs
constexpr u32 kDescVt = 0x00B12260;             // the stack spawn desc's vtable
constexpr u32 kTargetStr = 0x00ADFC08;          // "Target"
constexpr u32 kVec_01164B30 = 0x01164B30;       // f32[3] copied into the desc (+96)
constexpr u32 kUpVec = 0x01164B60;              // the global up vector
constexpr u32 kDoneFlag = 0x0106B62C;           // u8 set when a round ends
constexpr u32 kSaveProfile = 0x000C9FC8;        // Save_Profile0C9FC8 (key) -> def or 0
constexpr u32 kAnimRestart = 0x002775A8;        // Anim_Restart2775A8 (an, key, a, b; f, f, c, d; f, f, e, g)
constexpr u32 kSet180 = 0x0036EA80;             // Obj_Set180_0036EA80 (Obj*, a)
constexpr u32 kRoundEnd = 0x0040E220;           // (Game*)
constexpr u32 kState3 = 0x0040E400;             // (Spawner*; f32 dt)
constexpr u32 kState1 = 0x0040E6E0;             // (Spawner*; f32 dt)
constexpr u32 kNop = 0x0054EC18;
constexpr u32 kPtrVectorClear = 0x0054EC1C;
constexpr u32 kPtrVectorConstruct = 0x005516E0;
constexpr u32 kStringCopy = 0x00553684;         // (dst, src, n)
constexpr u32 kSplineTangent = 0x005A80D8;      // WorldObj_SplinePointTangentAt (spline, -, f32 pos[4], f32 tan[4], bool back; f1 t)
constexpr u32 kSortedMapFind = 0x00732F38;      // (map, const u32* key) -> slot*
constexpr u32 kSplineMapFind = 0x007335C4;      // (map, const u32* key) -> slot*
constexpr u32 kVectorAdd = 0x00733964;          // Vector_Add733964 (Vector*, const Rec* (192 bytes))

// The minigame: +53 / +97 in progress (u8s), +88 its def (+64 the round
// count), +468 / +484 counters, +480 the Obj_Set180 target.
struct SumoGame {
    pad<53> _0;
    u8 run53;           // +53
    pad<34> _54;
    be_u32 def;         // +88
    pad<5> _92;
    u8 run97;           // +97
    pad<370> _98;
    be_u32 w468;        // +468
    pad<8> _472;
    be_u32 w480;        // +480
    be_u32 w484;        // +484
};
static_assert(offsetof(SumoGame, def) == 88 && offsetof(SumoGame, run97) == 97 && offsetof(SumoGame, w484) == 484);
struct Spawner {
    pad<24> _0;
    be_u32 hashes[3];   // +24 / +28 / +32: the spline hashes
    pad<36> _36;
    be_u32 game;        // +72: SumoGame*
    be_u32 anim;        // +76: the animator owner (vfn[60])
    be_u32 target;      // +80
    be_u32 spline;      // +84
    be_u32 state;       // +88
    be_u32 t92;         // +92
    be_u32 t96;         // +96
    pad<12> _100;
    Vec4 pos;           // +112
    pad<32> _128;
    Vec4 vel;           // +160
    Vec4 tan;           // +176
    be_f32 speed;       // +192
    u8 rev;             // +196
};
static_assert(offsetof(Spawner, game) == 72 && offsetof(Spawner, t96) == 96 && offsetof(Spawner, pos) == 112);
static_assert(offsetof(Spawner, vel) == 160 && offsetof(Spawner, speed) == 192 && offsetof(Spawner, rev) == 196);
// The target effect def: u8 +32 (bit 2), +36 / +40 copied into the desc,
// +136 a vector of 192-byte records.
struct SumoDef {
    pad<32> _0;
    u8 b32;             // +32
    pad<3> _33;
    be_u32 w36;         // +36
    be_u32 w40;         // +40
    pad<92> _44;
    be_u32 recs;        // +136: {+0 data, +8 (u16) count}
};
static_assert(offsetof(SumoDef, recs) == 136);
struct SumoRecs {
    be_u32 data;        // +0
    pad<4> _4;
    be_u16 count;       // +8
};
// The spawn desc (200 bytes; on the stack, or the pooled one).
struct SpawnDesc {
    be_u32 vt;          // +0
    be_u32 w4, w8, w12; // +4 .. +12
    be_u32 vec;         // +16: the record vector (SmallAlloc'ed, 20 bytes)
    pad<12> _20;
    Mtx44 m;            // +32: back / up / right, +80 the position
    Vec4 v96;           // +96: the f32[3] at 0x01164B30
    be_u32 w112;        // +112
    be_u32 w116;        // +116
    be_u32 def;         // +120
    be_u32 w124;        // +124
    be_u32 w128;        // +128
    be_u32 flags;       // +132
    be_u32 w136;        // +136
    u8 name[60];        // +140
};
static_assert(sizeof(SpawnDesc) == 200 && offsetof(SpawnDesc, m) == 32 && offsetof(SpawnDesc, flags) == 132);
// The record vector: +6 (u16) the element size.
struct SumoVec {
    pad<6> _0;
    be_u16 esize;       // +6
};
// The original's locals (sp offsets).
struct SumoLocals {
    Vec4 back;          // sp+144
    Vec4 up;            // sp+160
    Vec4 right;         // sp+176
    Vec4 pos;           // sp+192
    SpawnDesc desc;     // sp+208
    be_u32 key3;        // sp+408
    be_u32 key2;        // sp+412
    be_u32 key1;        // sp+416
    be_u32 hashes[3];   // sp+420
};
static_assert(offsetof(SumoLocals, desc) == 64 && offsetof(SumoLocals, key3) == 264 && offsetof(SumoLocals, hashes) == 276);

f64 C(u32 off) { return lf(global<be_f32>(kCSumo + off)); }
void SetV(Vec4& d, f64 x, f64 y, f64 z) {
    d.x = static_cast<f32>(x);
    d.y = static_cast<f32>(y);
    d.z = static_cast<f32>(z);
}
}  // namespace

// void f(Spawner* o; f32 dt) (called by 00411D30, 00412640; no caller
// reads r3 or f1, r3.sh). o: +24/+28/+32 three spline hashes, +72 the game
// (u8 +53 / +97: in progress), +76 the animator owner, +80 the spawned
// target, +84 the spline, +88 the state, +92/+96 timer (0, 5.0), +112
// position, +160 velocity, +176 tangent, +192 speed, +196 reversed (u8).
// State 3 / 1: 0x0040E400 / 0x0040E6E0 (o; dt).
// State 5: state 0; if the game is in progress, its +468 and +484 count up
// (+484 reaching its def's +64 ends the round, 0x0040E220), Obj_Set180(+480,
// +484), and the byte 0x0106B62C = 1.
// State 0: speed = 0.8 + 0.7 * (last random output) / 65535; a previous
// target (+80) has bit 0x1000 of its +176 set and is dropped (the random
// generator's stored state advances once; without one the advanced state
// is still used below); then reversed = (state & 3) == 1 and the spline
// is one of +24/+28/+32 picked by the next output (mod 3) (the default
// spline 0xE720C39C, not reversed, when the game is not in progress); the
// state becomes 1, velocity 0; the animator (+76 vfn[60]) restarts
// 0x8919BDE9; with the target def (Save_Profile0C9FC8(0xC5877D2D)): the
// spline position / tangent at t 0 (reversed: t 1, tangent negated) gives
// a frame (back = -tangent, up' = right x tangent normalized, right = up x
// tangent normalized; reversed: back and right negated) and the position
// (y - 1) for a spawn desc (on the stack, or the pooled one for
// 0xC5877D2D) named "Target" with the def's +136 records (192 bytes each)
// added to a PtrVector; def->vfn[28](desc) is the target (+80), which gets
// vfn[88](1, 1, 1, 1) and vfn[84](back vector).
void Sumo_SpawnerTick_0040EA08(Spawner* o, f64 dt) {
    const u32 st = o->state;
    if (st == 5) {
        o->state = 0u;
        SumoGame& g = gm<SumoGame>(o->game);
        if (g.run53 == 0 && g.run97 == 0) return;
        const u32 a = g.w468, n = g.w484 + 1;
        const u32 def = g.def;
        g.w468 = a + 1;
        g.w484 = n;
        u32 cur = n;
        if (n == gm<be_u32>(def + 64)) {
            call(kRoundEnd, &g);
            cur = g.w484;
        }
        call(kSet180, g.w480.get(), cur);
        global<u8>(kDoneFlag) = 1;
        return;
    }
    if (st == 3) {
        call(kState3, o, dt);
        return;
    }
    if (st == 1) {
        call(kState1, o, dt);
        return;
    }
    if (st != 0) return;

    GuestFrame frame(512);
    SumoLocals& L = *frame.as<SumoLocals>(144);
    SpawnDesc& D = L.desc;

    // State 0: pick a spline.
    be_u64& last = global<be_u64>(kGameRandLast);
    be_u32& state = global<be_u32>(kGameRandState);
    u32 s = state;
    {
        const u32 m = GameRandMix(s);
        o->speed = static_cast<f32>(Fmadds(S(GameRandF(last) * C(0)), C(4), C(8)));
        // The advanced state is used either way, but only stored with a
        // previous target.
        s = std::rotl(m, 16);
        if (const u32 prev = o->target) {
            state = s;
            last = static_cast<u64>(m & 0xFFFF);
            gm<be_u16>(prev + 176) = static_cast<u16>(gm<be_u16>(prev + 176) | 0x1000);
            o->target = 0u;
            s = state;
        }
    }
    const u32 y = GameRandMix(s);
    const u32 s1 = std::rotl(y, 16);
    const u32 z = GameRandMix(s1);
    o->rev = (s & 3) == 1 ? 1 : 0;
    const f64 zero = C(12);
    L.hashes[0] = o->hashes[0].get();
    o->t92 = 0u;
    L.hashes[1] = o->hashes[1].get();
    o->t96 = 0x40A00000u;  // 5.0
    L.hashes[2] = o->hashes[2].get();
    state = std::rotl(z, 16);
    last = static_cast<u64>(z & 0xFFFF);
    const u32 map = gm<be_u32>(global<be_u32>(kSplineHolder) + 16);
    L.key1 = L.hashes[s1 % 3].get();
    o->spline = gm<be_u32>(call<u32>(kSplineMapFind, map, &L.key1)).get();
    {
        const SumoGame& g = gm<SumoGame>(o->game);
        if (g.run53 == 0 && g.run97 == 0) {
            const u32 map2 = gm<be_u32>(global<be_u32>(kSplineHolder) + 16);
            L.key2 = kSplineDefault;
            const u32 sp2 = gm<be_u32>(call<u32>(kSplineMapFind, map2, &L.key2));
            o->rev = 0;
            o->spline = sp2;
        }
    }
    o->state = 1u;
    CopyXYZ(o->vel, global<Vec3>(kZeroVec));
    {
        const u32 an = vcall<u32>(&gm<u8>(o->anim), 60);
        call(kAnimRestart, an, kAnimKey, 0u, 0u, C(16), C(20), 0u, 0u, zero, zero, 0u, -1);
    }
    const u32 def = call<u32>(kSaveProfile, kProfileKey);
    if (def == 0) return;
    SumoDef& df = gm<SumoDef>(def);

    // The spawn frame.
    f64 tx, ty, tz;
    if (o->rev == 0) {
        call(kSplineTangent, o->spline.get(), &o->tan, &o->pos, &o->tan, 0u, zero);
        tz = lf(o->tan.z);
        ty = lf(o->tan.y);
        tx = lf(o->tan.x);
    } else {
        call(kSplineTangent, o->spline.get(), &o->tan, &o->pos, &o->tan, 0u, C(16));
        tx = -lf(o->tan.x);
        ty = -lf(o->tan.y);
        tz = -lf(o->tan.z);
        SetV(o->tan, tx, ty, tz);
    }
    const f64 one = C(16);
    const Vec3& up = global<Vec3>(kUpVec);
    const f64 ux = lf(up.x), uy = lf(up.y), uz = lf(up.z);
    f64 rx = Fmsubs(uy, tz, S(uz * ty));
    f64 ry = Fmsubs(uz, tx, S(ux * tz));
    f64 rz = Fmsubs(ux, ty, S(uy * tx));
    const f64 lr = Sqrts(Fmadds(rz, rz, Fmadds(rx, rx, S(ry * ry))));
    f64 bx = -tx, by = -ty, bz = -tz;
    const f64 py = lf(o->pos.y);
    const u32 wpx = bits(o->pos.x), wpz = bits(o->pos.z);
    set_bits(L.pos.x, wpx);
    set_bits(L.pos.z, wpz);
    SetV(L.back, bx, by, bz);
    const f64 ir = Divs(one, lr);
    rz = MulsA(ir, rz);
    ry = MulsA(ir, ry);
    rx = MulsA(ir, rx);
    const f64 vy = Fmsubs(tz, rx, S(tx * rz));
    const f64 vx = Fmsubs(ty, rz, S(tz * ry));
    const f64 vz = Fmsubs(tx, ry, S(ty * rx));
    SetV(L.right, rx, ry, rz);
    const f64 lv = Sqrts(Fmadds(vz, vz, Fmadds(vx, vx, S(vy * vy))));
    const f64 iv = Divs(one, lv);
    const f64 pyy = Subs(py, one);
    L.pos.y = static_cast<f32>(pyy);
    const f64 ox = MulsA(iv, vx), oy = MulsA(iv, vy), oz = MulsA(iv, vz);
    SetV(L.up, ox, oy, oz);
    if (o->rev != 0) {
        bx = -bx, by = -by, bz = -bz, rx = -rx, ry = -ry, rz = -rz;
        SetV(L.back, bx, by, bz);
        SetV(L.right, rx, ry, rz);
    }

    // The spawn desc.
    D.vec = 0u;
    D.vt = kDescVt;
    SetV(D.m.r[0], bx, by, bz);
    SetV(D.m.r[1], ox, oy, oz);
    SetV(D.m.r[2], rx, ry, rz);
    set_bits(D.m.r[3].x, wpx);
    D.m.r[3].y = static_cast<f32>(pyy);
    set_bits(D.m.r[3].z, wpz);
    CopyXYZ(D.v96, global<Vec3>(kVec_01164B30));
    D.w112 = 0u;
    D.w116 = 0u;
    D.def = def;
    {
        // +132: the top bit set, bit 0x40000000 clear, 0x20000000 = bit 2 of
        // the def's +32, 0x1FE00000 set; the low 21 bits stay as the stack had them.
        u32 w = D.flags | 0x80000000u;
        D.w124 = df.w36.get();
        w &= ~0x40000000u;
        D.w128 = df.w40.get();
        D.flags = w;
        const u32 f = (df.b32 >> 2) & 1;
        D.w136 = 0u;
        D.w4 = 0u;
        w = (w & ~0x20000000u) | (f << 29);
        D.w8 = 0u;
        w |= 0x1FE00000u;
        D.w12 = 0u;
        D.name[0] = 0;
        D.flags = w;
    }
    call(kStringCopy, &D.name[0], kTargetStr, 40u);
    {
        u32 recs = df.recs;
        if (recs != 0 && gm<SumoRecs>(recs).count != 0) {
            for (u32 i = 0, off = 0;; ++i, off += 192) {
                const u32 cnt = recs != 0 ? gm<SumoRecs>(recs).count.get() : 0u;
                if (!(i < cnt)) break;
                const u32 rec = gm<SumoRecs>(recs).data + off;
                u32 v = D.vec;
                if (v == 0) {
                    v = call<u32>(fn::SmallAlloc, 20u);
                    D.vec = v;
                    if (v != 0) {
                        call(kPtrVectorConstruct, v);
                        gm<SumoVec>(v).esize = 192;
                        v = D.vec;
                    }
                }
                call(kVectorAdd, v, rec);  // v may be 0 after a failed allocation (quirk kept)
                recs = df.recs;
            }
        }
    }
    L.key3 = kProfileKey;
    SpawnDesc* d = &D;
    if (const u32 pooled = gm<be_u32>(call<u32>(kSortedMapFind, kPoolMap, &L.key3))) {
        SpawnDesc& P = gm<SpawnDesc>(pooled);
        P.w112 = D.w112.get();
        CopyMtxXYZ(P.m, D.m);
        CopyXYZ(P.v96, D.v96);
        P.def = def;
        d = &P;
    }
    const u32 tgt = vcall<u32>(&df, 28, d);
    o->target = tgt;
    if (tgt != 0) {
        vcall(&gm<u8>(tgt), 88, 1u, 1u, 1u, 1u);
        vcall(&gm<u8>(o->target), 84, &L.back);
    }
    const u32 v = D.vec;
    if (v == 0) return;
    D.vt = kDescVt;
    call(kPtrVectorClear, v);
    call(kNop, v);
    call(fn::SmallFree, D.vec.get(), 20u);
}

}  // namespace sly4::obj

NATIVE(0040EA08, sly4::obj::Sumo_SpawnerTick_0040EA08)
