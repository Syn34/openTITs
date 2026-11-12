// Game code, large-function sweep part 33: the four strings of a save game
// entry (title, location subtitle, details with the completion figures,
// save text) built from the string table.
#include <bit>

#include "../sly4.h"
#include "big33.h"

using namespace sly4;
using namespace sly4::big33;

namespace {
using opentit::rt::fcfid;
using opentit::rt::fctid;
using opentit::rt::fctiw;
using opentit::rt::to_single;

constexpr u32 kStrTable = 0x00F6687C;        // u32: the string table (0 before it loads)
constexpr u32 kSession = 0x00F64C20;         // Session*
constexpr u32 kMissingTitle = 0x00AC5EBC;    // "[Missing Title String]"
constexpr u32 kMissingSubtitle = 0x00AC5ED4;  // "[Missing Subtitle String]"
constexpr u32 kMissingDetails = 0x00AC5EF4;  // "[Missing Details String]"
constexpr u32 kMissingSave = 0x00AC5F14;     // "[Missing save text]"
constexpr u32 kSysGameName = 0x00AC5F2C;     // "SYSTEM_GAMENAME"
constexpr u32 kFmtTwoStrings = 0x00AC5F3C;   // "%s - %s"
constexpr u32 kUnknownTitle = 0x00AC5F44;    // "UnknownTitleName"
constexpr u32 kMissingLocation = 0x00AC5F5C;  // "[Missing Location String]"
constexpr u32 kStrSaveDetails = 0x00AC5F7C;  // "STR_SAVE_DETAILS"
constexpr u32 kSysCreateSave = 0x00AC5F94;   // "SYSTEM_CREATE_SAVE"

constexpr u32 Session_FindLevel2DABCC = 0x002DABCC;  // (session, key) -> level record or 0
constexpr u32 Name_Resolve_002E1220 = 0x002E1220;    // (session, hash) -> named record or 0
constexpr u32 Session_Level2E23AC = 0x002E23AC;      // (session) -> level record or 0
constexpr u32 Game_CompletePercent2E3188 = 0x002E3188;  // (session, src) -> f1
constexpr u32 Game_Stat2E32D8 = 0x002E32D8;          // (session, src) -> f1
constexpr u32 Game_Stat2E34A8 = 0x002E34A8;          // (session, src) -> f1
constexpr u32 Game_Stat2E3698 = 0x002E3698;          // (session, src) -> f1
constexpr u32 Game_Stat2E3898 = 0x002E3898;          // (session, src) -> f1
constexpr u32 StringTable_Lookup = 0x004F87C4;       // (strings, id, fallback, quiet) -> const char*

// The inlined floorf before (int): the value left in f1 for the final
// fctidz (x itself when already integral, NaN / huge, else trunc or
// trunc - 1 for negatives), as compiled.
f64 FloorF(f64 x) {
    const f64 t = to_single(fcfid(fctid(x, true)));
    const u32 w = static_cast<u32>(fctiw(x, true));
    if (std::bit_cast<u32>(static_cast<f32>(t - x)) == 0) return x;
    const u32 xb = std::bit_cast<u32>(static_cast<f32>(x));
    if (static_cast<s32>(xb & 0x7F800000u) >= 0x4B800000) return x;
    if (xb & 0x80000000u) return to_single(fcfid(static_cast<u64>(static_cast<s64>(static_cast<s32>(w - 1)))));
    return t;
}

// stat(session, src) floored to an int (the low word of fctidz); f1 is left
// holding the fctidz result.
u32 StatInt(PPUContext& ctx, u32 fn, u32 src) {
    call(ctx, fn, {load<u32>(kSession), src});
    const u64 d = fctid(FloorF(ctx.fpr[1]), true);
    ctx.fpr[1] = std::bit_cast<f64>(d);
    return static_cast<u32>(d);
}

u32 Lookup(PPUContext& ctx, u32 tbl, u32 id, u32 fallback, u32 quiet) {
    return static_cast<u32>(call(ctx, StringTable_Lookup, {tbl, id, fallback, quiet}));
}
}  // namespace

// void SaveParams_Set(const SaveSrc* src, char* title, u32 titleN, char* sub,
// u32 subN, char* det, u32 detN, char* save, s32 saveN) (called by
// Save_SaveGameToList and Save_AutoSaveGame; saveN is the 9th argument, on
// the stack). Without a string table all four get their "[Missing ...]"
// texts. Otherwise: title = SYSTEM_GAMENAME; sub = the level record's name
// (Session_FindLevel2DABCC(session, src+36), else Session_Level2E23AC, else
// session+20; record +1 id, u8 +827 quiet), "%s - %s" with the src+44 named
// record's name (+1, u8 +504) when that resolves, or UnknownTitleName
// without a level; det = String_FormatN(STR_SAVE_DETAILS, completion, s32
// src+32, and the floored stats 2E32D8, 2E3698, 2E3898, 2E34A8 (9th
// argument)); save = SYSTEM_CREATE_SAVE. r3 is left as the last copy's
// result and f1 as the last fctidz.
DECOMP(000AE78C, SaveParams_Set) {
    const u32 src = arg<u32>(ctx, 0), title = arg<u32>(ctx, 1), titleN = arg<u32>(ctx, 2);
    const u32 sub = arg<u32>(ctx, 3), subN = arg<u32>(ctx, 4), det = arg<u32>(ctx, 5);
    const u32 detN = arg<u32>(ctx, 6), save = arg<u32>(ctx, 7);
    const u64 saveN = static_cast<u64>(static_cast<s64>(static_cast<s32>(StackArg(static_cast<u32>(ctx.gpr[1]), 8))));
    const u32 tbl = load<u32>(kStrTable);
    if (tbl == 0) {
        call(ctx, b33fn::String_CopyN, {title, kMissingTitle, titleN});
        call(ctx, b33fn::String_CopyN, {sub, kMissingSubtitle, subN});
        call(ctx, b33fn::String_CopyN, {det, kMissingDetails, detN});
        call(ctx, b33fn::String_CopyN, {save, kMissingSave, saveN});
        return;
    }
    call(ctx, b33fn::String_CopyN, {title, Lookup(ctx, tbl, kSysGameName, kMissingTitle, 0), titleN});

    u32 lvl = load<u32>(src + 36);
    if (lvl != 0) lvl = static_cast<u32>(call(ctx, Session_FindLevel2DABCC, {load<u32>(kSession), lvl}));
    if (lvl == 0) lvl = static_cast<u32>(call(ctx, Session_Level2E23AC, {load<u32>(kSession)}));
    if (lvl == 0) lvl = load<u32>(load<u32>(kSession) + 20);
    if (lvl == 0) {
        call(ctx, b33fn::String_CopyN, {sub, Lookup(ctx, tbl, kUnknownTitle, kMissingLocation, 0), subN});
    } else {
        const u32 name = Lookup(ctx, tbl, lvl + 1, lvl, load<u8>(lvl + 827));
        const u32 h = load<u32>(src + 44);
        const u32 n = h != 0 ? static_cast<u32>(call(ctx, Name_Resolve_002E1220, {load<u32>(kSession), h})) : 0;
        if (n != 0) {
            const u32 s2 = Lookup(ctx, tbl, n + 1, n, load<u8>(n + 504));
            call(ctx, b33fn::String_FormatN, {sub, subN, kFmtTwoStrings, name, s2});
        } else {
            call(ctx, b33fn::String_CopyN, {sub, name, subN});
        }
    }

    const u32 fmt = Lookup(ctx, tbl, kStrSaveDetails, kMissingDetails, 0);
    const u32 pct = StatInt(ctx, Game_CompletePercent2E3188, src);
    const u64 v32 = static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(src + 32))));
    const u32 a = StatInt(ctx, Game_Stat2E32D8, src);
    const u32 b = StatInt(ctx, Game_Stat2E3698, src);
    const u32 c = StatInt(ctx, Game_Stat2E3898, src);
    const u32 d = StatInt(ctx, Game_Stat2E34A8, src);
    call(ctx, b33fn::String_FormatN, {det, detN, fmt, pct, v32, a, b, c, d});
    call(ctx, b33fn::String_CopyN, {save, Lookup(ctx, tbl, kSysCreateSave, kMissingSave, 0), saveN});
}
