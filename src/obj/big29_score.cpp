// Game code, large-function sweep part 29: the score popup spawn (a
// "Scoring%u" object placed between the camera and the scoring object,
// with its drift and its "%d" animation).
#include "big29.h"
#include "native/vec.h"

namespace sly4::obj {

namespace {
constexpr u32 kScoreSerial = 0x00F64C1C;  // u32: the next popup's number
constexpr u32 kFmtScoring = 0x00AD537C;  // "Scoring%u"
constexpr u32 kFmtD = 0x00AD5124;        // "%d"
constexpr u32 kScoreTable = 0xC07487EC;  // Map_LookupThen_00367900 table hash
constexpr u32 kPoolScore = 0x002B2190;        // .text pool: +0 0.0, +48 0.3, +52 0.1, +56 1.0, +60 -1.0, +64 the random scale
constexpr u32 kIdentity = 0x01164BD0;    // the global 4x4 identity
constexpr u32 kAnimSet = 0x01164CA4;     // the default anim set Obj_SetAnim callers pass
constexpr u32 Map_LookupThen = 0x00367900;   // (u32 table, u32 key) -> T*
constexpr u32 String_HashNoCase = 0x0056A710;  // (const char*) -> u32
constexpr u32 WorldObj_MarkDirty = 0x0057D000;  // (T*)
constexpr u32 Obj_SetAnim = 0x00363A3C;      // (o, name, now, -, b, c, d; f32)

// The scoring object: +8 its source (whose +160 points at a block with
// the point at +128), +32..+40 the world position (raw), +48 the drift
// speed, +52 / +56.
struct Scorer {
    pad<8> _0;
    be_u32 src;      // +8
    pad<20> _12;
    be_u32 pos[3];   // +32
    pad<4> _44;
    be_f32 speed;    // +48
    be_u32 w52;      // +52
    be_u32 w56;      // +56
};
static_assert(offsetof(Scorer, pos) == 32 && offsetof(Scorer, w56) == 56);
struct ScoreSrc {
    pad<160> _0;
    be_u32 block;  // +160: the point at +128
};
// The popup's transform object: +16 the rotation rows, +64 the translation.
struct ScoreXform {
    pad<16> _0;
    Mtx44 m;  // +16
};
struct ScorePopup {
    pad<24> _0;
    gp<ScoreXform> xf;  // +24
    pad<324> _28;
    be_u32 pos[3];      // +352
    pad<4> _364;
    be_f32 dir[2];      // +368
    be_u32 w376;        // +376
    pad<4> _380;
    be_u32 w384;        // +384
    be_u32 w388;        // +388
    pad<4> _392;
    be_u32 flags;       // +396
};
static_assert(offsetof(ScorePopup, pos) == 352 && offsetof(ScorePopup, flags) == 396);

// Row r (x, y, z) of m as VMX lanes (w 0, the words raw).
V4 Row(const Mtx44& m, u32 r) { return FromWords(bits(m.r[r].x), bits(m.r[r].y), bits(m.r[r].z), 0u); }
// m * {x, y, z, 1}: the vmaddfp chain over rows 0..2 then the vaddfp of row 3.
V4 XformPt(const Mtx44& m, f32 x, f32 y, f32 z) { return Add4(Row(m, 3), Row3(Row(m, 0), Row(m, 1), Row(m, 2), x, y, z)); }
f32 Fw(const be_f32& v) { return std::bit_cast<f32>(bits(v)); }
}  // namespace

// void f(Scorer* o, s32 n, u32 mult) (called by f_001693D0,
// Msg_AddScore_0016A358, f_00175758, f_0018E0C0). Only with the game, its
// +0x128164 record with +5572 bit 0x00200000 and o->+8: the popup p =
// Map_LookupThen(kScoreTable, String_HashNoCase(String_Format("Scoring%u",
// ++kScoreSerial))). With o->+48 > 0 a random unit direction (three steps
// of the game's inlined generator, components 2 * s * r - s, the x / y
// pair normalised by the length of all three). Without p: returns. Else the point o->+8->+160 +80 (+48..+56) through the
// game's +0x128168 matrix (+752) then its +0x12816C matrix (+816) gives
// q; p->+24's transform = the identity rows with the translation c + 0.3
// (q - c) (c that matrix's +48..+56); WorldObj_MarkDirty(p->+24); p->+396
// |= 0x200000; then the drift: when the x / y distance from o->+32 / +36
// exceeds 0.1, the direction gains the perpendicular (-ey, ex) / |e| * o->+48;
// p->+352.. = o->+32/+36/+40, +368/+372 the direction, +376 0, +384 =
// o->+56, +388 = o->+52, +396 |= 0x300000 (on the value before); then
// Obj_SetAnim(p, String_Format("%d", mult > 1 ? n * mult : n), 1, -, 1,
// 0, kAnimSet; f1 = -1.0). No caller reads r3 or f1 (r3.sh).
void Score_SpawnPopup_002B21D8(Scorer* o, u32 n, u32 mult) {
    {
        const u32 g = global<be_u32>(kWorld);
        u32 g1 = 0;
        if (g == 0 || (g1 = gm<be_u32>(g + 0x128164)) == 0 || ((gm<be_u32>(g1 + 5572) >> 21) & 1) == 0 || o->src == 0u) return;
    }
    be_u32& serial_g = global<be_u32>(kScoreSerial);
    const u32 serial = serial_g + 1;
    serial_g = serial;
    const u32 s = call<u32>(fn::String_Format, kFmtScoring, serial);
    const u32 h = call<u32>(String_HashNoCase, s);
    const u32 p = call<u32>(Map_LookupThen, kScoreTable, h);
    const f64 k0 = lf(global<be_f32>(kPoolScore));
    f64 dx = k0, dy = k0;
    f64 f1 = lf(o->speed);
    const u32 src = o->src;
    const u32 g = global<be_u32>(kWorld);
    if (f1 > dy) {
        // Three steps of the inlined generator: the values are the u64 at
        // kGameRandLast, then the low 16 bits of each mix.
        const f64 neg = -f1, k = lf(global<be_f32>(kPoolScore + 64));
        be_u64& last = global<be_u64>(kGameRandLast);
        be_u32& state = global<be_u32>(kGameRandState);
        const u64 v0 = last;
        const u32 s0 = state;
        const u32 s1 = GameRandMix(s0);
        const u32 s2 = GameRandMix(std::rotl(s1, 16));
        const u32 s3 = GameRandMix(std::rotl(s2, 16));
        const f64 w = Subs(f1, neg);
        state = std::rotl(s3, 16);
        last = static_cast<u64>(s3 & 0xFFFF);
        dx = Fmadds(w, Muls(GameRandF(v0), k), neg);
        dy = Fmadds(w, Muls(GameRandF(s1 & 0xFFFF), k), neg);
        const f64 dz = Fmadds(w, Muls(GameRandF(s2 & 0xFFFF), k), neg);
        f1 = Divs(lf(global<be_f32>(kPoolScore + 56)), Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)))));
        dx = Muls(dx, f1);
        dy = Muls(dy, f1);
    }
    if (p == 0) return;
    ScorePopup& pp = gm<ScorePopup>(p);
    ScoreXform& t = gm<ScoreXform>(pp.xf.ea);
    const Vec3& pt = gm<Vec3>(gm<ScoreSrc>(src).block + 128);
    const V4 q1 = XformPt(gm<Mtx44>(gm<be_u32>(g + 0x128168) + 752), Fw(pt.x), Fw(pt.y), Fw(pt.z));
    const Mtx44& cam = gm<Mtx44>(gm<be_u32>(g + 0x12816C) + 816);
    const V4 q = XformPt(cam, q1.e[0], q1.e[1], q1.e[2]);
    const f64 c0 = lf(cam.r[3].x), c1 = lf(cam.r[3].y), c2 = lf(cam.r[3].z), k3 = lf(global<be_f32>(kPoolScore + 48));
    const Mtx44& idm = global<Mtx44>(kIdentity);
    u32 id[9];
    for (u32 r = 0; r < 3; ++r) id[3 * r] = bits(idm.r[r].x), id[3 * r + 1] = bits(idm.r[r].y), id[3 * r + 2] = bits(idm.r[r].z);
    const f64 px = Fmadds(Subs(q.e[0], c0), k3, c0);
    const f64 py = Fmadds(Subs(q.e[1], c1), k3, c1);
    const f64 pz = Fmadds(Subs(q.e[2], c2), k3, c2);
    for (u32 r = 0; r < 3; ++r) {
        set_bits(t.m.r[r].x, id[3 * r]);
        set_bits(t.m.r[r].y, id[3 * r + 1]);
        set_bits(t.m.r[r].z, id[3 * r + 2]);
    }
    t.m.r[3].x = static_cast<f32>(px);
    t.m.r[3].y = static_cast<f32>(py);
    t.m.r[3].z = static_cast<f32>(pz);
    call(WorldObj_MarkDirty, &t);
    const u32 f396 = pp.flags;
    pp.flags = f396 | 0x00200000u;
    // The drift.
    const u32 wx = o->pos[0], wy = o->pos[1];
    const f64 ex = Subs(LfsW(wx), px), ey = Subs(LfsW(wy), py);
    const f64 d2 = Fmadds(k0, k0, Fmadds(ex, ex, Muls(ey, ey)));
    const u32 wz = o->pos[2], w56 = o->w56, w52 = o->w52;
    const f64 d = Sqrts(d2);
    if (d > lf(global<be_f32>(kPoolScore + 52))) {
        const f64 inv = Divs(lf(global<be_f32>(kPoolScore + 56)), d), sp = lf(o->speed);
        dy = Fmadds(Muls(ex, inv), sp, dy);
        dx = Adds(dx, -Muls(Muls(ey, inv), sp));
    }
    pp.pos[0] = wx;
    pp.pos[1] = wy;
    pp.pos[2] = wz;
    pp.dir[0] = static_cast<f32>(dx);
    pp.dir[1] = static_cast<f32>(dy);
    pp.w376 = 0u;
    pp.w384 = w56;
    pp.w388 = w52;
    pp.flags = f396 | 0x00300000u;
    const u32 name = mult > 1 ? call<u32>(fn::String_Format, kFmtD, n * mult)  // the product as mullw leaves it
                              : call<u32>(fn::String_Format, kFmtD, static_cast<s32>(n));
    call(Obj_SetAnim, &pp, name, 1u, 0u, 1u, 0u, kAnimSet, lf(global<be_f32>(kPoolScore + 60)));
}

}  // namespace sly4::obj

NATIVE(002B21D8, sly4::obj::Score_SpawnPopup_002B21D8)
