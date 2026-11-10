// Game code, medium sweep part 5: the high-score table insertion of the
// minigame result screens (three instances of one template, differing
// only in the result object's field offsets).
//
// The table lives in the current world's save slot: World +4208 u8 the
// slot (4056 bytes each, from World +0); per profile (the u8 at
// World->+0x128164->+5580, 44 bytes each) +688 u32 scores[5] and +708
// char initials[5][4], best first. The result object o holds the new
// score, its rank, the entered initials and the screen's widget state.
#include "mid5.h"

using namespace sly4;
using namespace sly4::mid5;

namespace {

namespace sfn {
constexpr u32 Handle_ResolveBit1 = 0x003568E0;  // (hash) -> Object* or 0
constexpr u32 Obj_SetText = 0x00367530;         // (Object*, const char*, u32 slot)
constexpr u32 Obj_GetNodePos = 0x0036336C;     // (Object*, Vec3* out, u32 i) (obj28.cpp)
constexpr u32 Score_Submit_002E6A88 = 0x002E6A88;  // (x, u32 profile, u32 rank) (name?)
}  // namespace sfn

constexpr u32 kCurrentWorld = 0x00F65CA0;   // World*
constexpr u32 kWorldProfile = 0x128164;     // World +: Profile*
constexpr u32 kProfileFlags = 5572;         // Profile +: u32 (0x00200000: tables enabled)
constexpr u32 kProfileIndex = 5580;         // Profile +: u8
constexpr u32 kWorldSlot = 4208;            // World +: u8
constexpr u32 kHashScoreScreen = 0x2508CAD1;  // the result screen's handle
constexpr u32 kFmtInt = 0x00ADF3D8;         // "%d"
constexpr u32 kSubmitArg = 0x00F64C20;      // u32 (name?)
constexpr u32 kResetVec = 0x01164B40;       // f32[3]
constexpr u32 kResetF = 0x3F19999A;         // 0.6f

// The result object's fields.
struct ScoreFields {
    u32 score;    // u32 the new score
    u32 rank;     // s32 its rank (the last row it beats)
    u32 w;        // u32, cleared on insertion
    u32 init;     // char[4] the entered initials, then +4 u8 (cleared: the terminator)
    u32 flag;     // u8: the score made the table
    u32 f;        // f32 = 0.6 at the end
    u32 v;        // f32[3] = kResetVec at the end
    u32 p;        // Vec3: the new row's node position (Obj_GetNodePos_0036336C)
};

// The profile's table (World->+0x128164 re-read at each use, as the original).
u32 Table(u32 slot) {
    const u32 w = load<u32>(kCurrentWorld);
    return slot + static_cast<u32>(load<u8>(load<u32>(w + kWorldProfile) + kProfileIndex)) * 44;
}

// u8 f(Result* o): insert o's score into the table. Returns 0 when the
// world, its profile or the profile's table flag is missing; else o's
// flag (f1 left holding kResetVec[2]). Each row the score beats is moved
// one down (shown on the screen widget's text slots 3 * row + 5 / + 6 when
// the widget exists); then for a new entry 0x002E6A88(*kSubmitArg,
// profile, rank), o's initials reset to four blanks and the new row
// written ('.' shown for a blank first initial).
void ScoreInsert(PPUContext& ctx, const ScoreFields& F) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(kCurrentWorld);
    store<u8>(o + F.flag, 0);
    if (w == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 prof = load<u32>(w + kWorldProfile);
    if (prof == 0 || ((load<u32>(prof + kProfileFlags) >> 21) & 1u) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 slot = w + static_cast<u32>(load<u8>(w + kWorldSlot)) * 4056;
    store<u32>(o + F.rank, 0);
    const u32 scr = static_cast<u32>(call(ctx, sfn::Handle_ResolveBit1, {kHashScoreScreen}));
    Stack frame(ctx, 16);
    const u32 tmp = frame.addr() + 8;  // char[5]: the row being moved
    const u32 txt = frame.addr();      // char[5]: the new row's initials
    for (u32 n = 5; n != 0; --n) {
        const u32 k = n - 1;
        const u32 at = 4 * k;
        const u32 sc = load<u32>(Table(slot) + 688 + at);
        for (u32 j = 0; j < 4; ++j) store<u8>(tmp + j, load<u8>(Table(slot) + 708 + j + at));
        store<u8>(tmp + 4, 0);
        if (!(load<u32>(o + F.score) > sc)) continue;
        store<u32>(o + F.rank, k);
        store<u8>(o + F.flag, 1);
        if (!(n < 5)) continue;
        if (scr != 0) {
            const u32 s = static_cast<u32>(call(ctx, m5fn::String_Format, {kFmtInt, sc}));
            call(ctx, sfn::Obj_SetText, {scr, s, 3 * n + 5});
            call(ctx, sfn::Obj_SetText, {scr, tmp, 3 * n + 6});
        }
        for (u32 j = 0; j < 4; ++j) store<u8>(Table(slot) + 708 + j + at + 4, load<u8>(Table(slot) + 708 + j + at));
        store<u32>(Table(slot) + 688 + at + 4, load<u32>(Table(slot) + 688 + at));
    }
    if (load<u8>(o + F.flag) != 0) {
        call(ctx, sfn::Score_Submit_002E6A88, {load<u32>(kSubmitArg), load<u8>(load<u32>(load<u32>(kCurrentWorld) + kWorldProfile) + kProfileIndex), load<u32>(o + F.rank)});
        store<u8>(o + F.init + 4, 0);
        store<u32>(o + F.w, 0);
        for (u32 j = 0; j < 4; ++j) store<u8>(o + F.init + j, ' ');
        store<u8>(Table(slot + 80) + (load<u32>(o + F.rank) << 2) + 628, ' ');
        for (u32 j = 1; j < 4; ++j) store<u8>(Table(slot + 80) + (load<u32>(o + F.rank) << 2) + 628 + j, load<u8>(o + F.init + j));
        store<u32>(Table(slot + 80) + (load<u32>(o + F.rank) << 2) + 608, load<u32>(o + F.score));
        for (u32 j = 0; j < 5; ++j) store<u8>(txt + j, load<u8>(o + F.init + j));
        const u32 sc = load<u32>(o + F.score);
        if (load<u8>(o + F.init) == ' ') store<u8>(txt, '.');
        if (scr != 0) {
            const u32 s = static_cast<u32>(call(ctx, m5fn::String_Format, {kFmtInt, sc}));
            call(ctx, sfn::Obj_SetText, {scr, s, load<u32>(o + F.rank) * 3 + 5});
            call(ctx, sfn::Obj_SetText, {scr, txt, load<u32>(o + F.rank) * 3 + 6});
            call(ctx, sfn::Obj_GetNodePos, {scr, o + F.p, load<u32>(o + F.rank) * 3 + 5});
        }
    }
    store<u32>(o + F.f, kResetF);
    store<u32>(o + F.v, load<u32>(kResetVec));
    store<u32>(o + F.v + 4, load<u32>(kResetVec + 4));
    const u32 z = load<u32>(kResetVec + 8);
    store<u32>(o + F.v + 8, z);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(z));
    ret(ctx, static_cast<u32>(load<u8>(o + F.flag)));
}

}  // namespace

// u8 f(Result* o): the high-score insertion of the result screen with the
// fields at +100 (0x003DE1C0 / 0x003E1020's object).
DECOMP(003DDBB8, Score_Insert_003DDBB8) { ScoreInsert(ctx, {100, 104, 108, 112, 118, 144, 272, 256}); }

// u8 f(Result* o): the same with the fields at +2308.
DECOMP(003E4884, Score_Insert_003E4884) { ScoreInsert(ctx, {2308, 2312, 2316, 2320, 2326, 2332, 2400, 2384}); }

// u8 f(Result* o): the same with the fields at +164.
DECOMP(003F123C, Score_Insert_003F123C) { ScoreInsert(ctx, {164, 168, 172, 176, 182, 188, 208, 192}); }
