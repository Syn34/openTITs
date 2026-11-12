// Game code, large-function sweep part 29: the score popup spawn (a
// "Scoring%u" object placed between the camera and the scoring object,
// with its drift and its "%d" animation).
#include "../sly4.h"
#include "big29.h"

using namespace sly4;
using namespace sly4::big29;

namespace {
constexpr u32 kGame = 0x00F65CA0;        // Game*
constexpr u32 kScoreSerial = 0x00F64C1C;  // u32: the next popup's number
constexpr u32 kFmtScoring = 0x00AD537C;  // "Scoring%u"
constexpr u32 kFmtD = 0x00AD5124;        // "%d"
constexpr u32 kScoreTable = 0xC07487EC;  // Map_LookupThen_00367900 table hash
constexpr u32 kPoolScore = 0x002B2190;        // .text pool: +0 0.0, +48 0.3, +52 0.1, +56 1.0, +60 -1.0, +64 the random scale
constexpr u32 kIdentity = 0x01164BD0;    // the global 4x4 identity (rows of 16 bytes)
constexpr u32 kAnimSet = 0x01164CA4;     // the default anim set Obj_SetAnim callers pass
constexpr u32 Map_LookupThen = 0x00367900;   // (u32 table, u32 key) -> T*
constexpr u32 String_Format = 0x005534D0;    // (fmt, ...) -> rotating static buffer
constexpr u32 String_HashNoCase = 0x0056A710;  // (const char*) -> u32
constexpr u32 WorldObj_MarkDirty = 0x0057D000;  // (T*)
constexpr u32 Obj_SetAnim = 0x00363A3C;      // (o, name, now, -, b, c, d; f32)

// Row r (x, y, z) of the 4x4 f32 matrix at m as VMX lanes (w 0).
big5::V4 Row(u32 m, u32 r) { return big5::V4{{Fw27(m + 16 * r), Fw27(m + 16 * r + 4), Fw27(m + 16 * r + 8), 0.0f}}; }
// m * {x, y, z, 1}: the vmaddfp chain over rows 0..2 then the vaddfp of row 3.
big5::V4 XformPt(u32 m, f32 x, f32 y, f32 z) { return big5::Add4(Row(m, 3), big5::Row3(Row(m, 0), Row(m, 1), Row(m, 2), x, y, z)); }
}  // namespace

// u32 f(Scorer* o, s32 n, u32 mult) (called by f_001693D0,
// Msg_AddScore_0016A358, f_00175758, f_0018E0C0). Only with the game, its
// +0x128164 record with +5572 bit 0x00200000 and o->+8: the popup p =
// Map_LookupThen(kScoreTable, String_HashNoCase(String_Format("Scoring%u",
// ++kScoreSerial))). With o->+48 > 0 a random unit direction (three steps
// of the game's inlined generator, components 2 * s * r - s, the x / y
// pair normalised by the length of all three). Without p: returns (r3 the
// game pointer). Else the point o->+8->+160 +80 (+48..+56) through the
// game's +0x128168 matrix (+752) then its +0x12816C matrix (+816) gives
// q; p->+24's transform = the identity rows with the translation c + 0.3
// (q - c) (c that matrix's +48..+56); WorldObj_MarkDirty(p->+24); p->+396
// |= 0x200000; then the drift: when the x / y distance from o->+32 / +36
// exceeds 0.1, the direction gains the perpendicular (-ey, ex) / |e| * o->+48;
// p->+352.. = o->+32/+36/+40, +368/+372 the direction, +376 0, +384 =
// o->+56, +388 = o->+52, +396 |= 0x300000 (on the value before); then
// Obj_SetAnim(p, String_Format("%d", mult > 1 ? n * mult : n), 1, -, 1,
// 0, kAnimSet; f1 = -1.0) (its r3).
DECOMP(002B21D8, Score_SpawnPopup_002B21D8) {
    const u32 o = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1), mult = arg<u32>(ctx, 2);
    {
        const u32 g = load<u32>(kGame);
        u32 g1 = 0;
        if (g == 0 || (g1 = load<u32>(g + 0x128164)) == 0 || ((load<u32>(g1 + 5572) >> 21) & 1) == 0 || load<u32>(o + 8) == 0) {
            ctx.gpr[3] = 0;  // each test's value
            return;
        }
    }
    const u32 serial = load<u32>(kScoreSerial) + 1;
    store<u32>(kScoreSerial, serial);
    const u32 s = static_cast<u32>(call(ctx, String_Format, {kFmtScoring, serial}));
    const u32 h = static_cast<u32>(call(ctx, String_HashNoCase, {s}));
    const u32 p = static_cast<u32>(call(ctx, Map_LookupThen, {kScoreTable, h}));
    const f64 k0 = Lf(kPoolScore);
    f64 dx = k0, dy = k0;
    f64 f1 = Lf(o + 48);
    const u32 src = load<u32>(o + 8);
    const u32 g = load<u32>(kGame);
    if (f1 > dy) {
        // Three steps of the inlined generator (mid15::RandMix15): the
        // values are the u64 at +0, then the low 16 bits of each mix.
        const f64 neg = -f1, k = Lf(kPoolScore + 64);
        const u64 v0 = load<u64>(mid15::kRand15);
        const u32 s0 = load<u32>(mid15::kRand15 + 8);
        const u32 s1 = mid15::RandMix15(s0);
        const u32 s2 = mid15::RandMix15(std::rotl(s1, 16));
        const u32 s3 = mid15::RandMix15(std::rotl(s2, 16));
        const f64 w = Subs(f1, neg);
        store<u32>(mid15::kRand15 + 8, std::rotl(s3, 16));
        store<u64>(mid15::kRand15, s3 & 0xFFFF);
        dx = Fmadds(w, Muls(mid15::RandF15(v0), k), neg);
        dy = Fmadds(w, Muls(mid15::RandF15(s1 & 0xFFFF), k), neg);
        const f64 dz = Fmadds(w, Muls(mid15::RandF15(s2 & 0xFFFF), k), neg);
        f1 = Divs(Lf(kPoolScore + 56), Sqrts(Fmadds(dz, dz, Fmadds(dx, dx, Muls(dy, dy)))));
        dx = Muls(dx, f1);
        dy = Muls(dy, f1);
    }
    if (p == 0) {
        ctx.gpr[3] = g;  // the game pointer the matrix fetch reloaded
        ctx.fpr[1] = f1;
        return;
    }
    const u32 t = load<u32>(p + 24);
    const u32 pt = load<u32>(src + 160) + 80;
    const big5::V4 q1 = XformPt(load<u32>(g + 0x128168) + 752, Fw27(pt + 48), Fw27(pt + 52), Fw27(pt + 56));
    const u32 cam = load<u32>(g + 0x12816C) + 816;
    const big5::V4 q = XformPt(cam, q1.e[0], q1.e[1], q1.e[2]);
    const f64 c0 = Lf(cam + 48), c1 = Lf(cam + 52), c2 = Lf(cam + 56), k3 = Lf(kPoolScore + 48);
    u32 id[9];
    for (u32 r = 0; r < 3; ++r)
        for (u32 j = 0; j < 3; ++j) id[3 * r + j] = load<u32>(kIdentity + 16 * r + 4 * j);
    const f64 px = Fmadds(Subs(q.e[0], c0), k3, c0);
    const f64 py = Fmadds(Subs(q.e[1], c1), k3, c1);
    const f64 pz = Fmadds(Subs(q.e[2], c2), k3, c2);
    for (u32 r = 0; r < 3; ++r)
        for (u32 j = 0; j < 3; ++j) store<u32>(t + 16 + 16 * r + 4 * j, id[3 * r + j]);
    Sf(t + 64, px);
    Sf(t + 68, py);
    Sf(t + 72, pz);
    call(ctx, WorldObj_MarkDirty, {t});
    const u32 f396 = load<u32>(p + 396);
    store<u32>(p + 396, f396 | 0x00200000u);
    // The drift.
    const u32 wx = load<u32>(o + 32), wy = load<u32>(o + 36);
    const f64 ex = Subs(LfsF(wx), px), ey = Subs(LfsF(wy), py);
    const f64 d2 = Fmadds(k0, k0, Fmadds(ex, ex, Muls(ey, ey)));
    const u32 wz = load<u32>(o + 40), w56 = load<u32>(o + 56), w52 = load<u32>(o + 52);
    const f64 d = Sqrts(d2);
    if (d > Lf(kPoolScore + 52)) {
        const f64 inv = Divs(Lf(kPoolScore + 56), d), sp = Lf(o + 48);
        dy = Fmadds(Muls(ex, inv), sp, dy);
        dx = Adds(dx, -Muls(Muls(ey, inv), sp));
    }
    store<u32>(p + 352, wx);
    store<u32>(p + 356, wy);
    store<u32>(p + 360, wz);
    Sf(p + 368, dx);
    Sf(p + 372, dy);
    store<u32>(p + 376, 0);
    store<u32>(p + 384, w56);
    store<u32>(p + 388, w52);
    store<u32>(p + 396, f396 | 0x00300000u);
    u32 name;
    if (mult > 1)
        name = static_cast<u32>(call(ctx, String_Format, {kFmtD, n * mult, static_cast<u64>(static_cast<s64>(static_cast<s32>(mult)))}));
    else
        name = static_cast<u32>(call(ctx, String_Format, {kFmtD, static_cast<u64>(static_cast<s64>(static_cast<s32>(n)))}));
    // r6 is not read by Obj_SetAnim (left as String_Format leaves it).
    callf(ctx, Obj_SetAnim, {p, name, 1, ctx.gpr[6], 1, 0, kAnimSet}, {Lf(kPoolScore + 60)});
}
