// Game code, large-function sweep part 33: the four strings of a save game
// entry (title, location subtitle, details with the completion figures,
// save text) built from the string table.
#include <bit>

#include "big33.h"

namespace sly4::obj {

struct SaveSrc {
    pad<32> _0;
    be_s32 v32;     // +32
    be_u32 level;   // +36: level key (0: none)
    pad<4> _40;
    be_u32 name;    // +44: hash of a named record (0: none)
};
static_assert(offsetof(SaveSrc, level) == 36 && offsetof(SaveSrc, name) == 44);
// A level record: +1 its string id, u8 +827 quiet.
struct SaveLevel {
    pad<1> _0;
    pad<826> id;  // +1
    u8 quiet;     // +827
};
static_assert(offsetof(SaveLevel, quiet) == 827);
// A named record: +1 its string id, u8 +504 quiet.
struct SaveNamed {
    pad<1> _0;
    pad<503> id;  // +1
    u8 quiet;     // +504
};
static_assert(offsetof(SaveNamed, quiet) == 504);
struct SaveSession {
    pad<20> _0;
    gp<SaveLevel> level;  // +20
};

namespace {
using opentit::rt::fcfid;
using opentit::rt::fctid;
using opentit::rt::fctiw;
using opentit::rt::to_single;

constexpr u32 kStrTable = 0x00F6687C;        // u32: the string table (0 before it loads)
constexpr u32 kSession = 0x00F64C20;         // SaveSession*
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

u32 Session() { return global<be_u32>(kSession); }

// The inlined floorf before (int): the value given to the final fctidz (x
// itself when already integral, NaN / huge, else trunc or trunc - 1 for
// negatives), as compiled.
f64 FloorF(f64 x) {
    const f64 t = to_single(fcfid(fctid(x, true)));
    const u32 w = static_cast<u32>(fctiw(x, true));
    if (std::bit_cast<u32>(static_cast<f32>(t - x)) == 0) return x;
    const u32 xb = std::bit_cast<u32>(static_cast<f32>(x));
    if (static_cast<s32>(xb & 0x7F800000u) >= 0x4B800000) return x;
    if (xb & 0x80000000u) return to_single(fcfid(static_cast<u64>(static_cast<s64>(static_cast<s32>(w - 1)))));
    return t;
}

// stat(session, src) floored to an int (the low word of fctidz).
u32 StatInt(u32 fn, const SaveSrc* src) {
    const f64 v = call<f64>(fn, Session(), src);
    return static_cast<u32>(fctid(FloorF(v), true));
}

u32 Lookup(u32 tbl, u32 id, u32 fallback, u32 quiet) { return call<u32>(StringTable_Lookup, tbl, id, fallback, quiet); }
}  // namespace

// void SaveParams_Set(const SaveSrc* src, char* title, u32 titleN, char* sub,
// u32 subN, char* det, u32 detN, char* save, s32 saveN) (called by
// Save_SaveGameToList and Save_AutoSaveGame; saveN is the 9th argument, on
// the stack). Without a string table all four get their "[Missing ...]"
// texts. Otherwise: title = SYSTEM_GAMENAME; sub = the level record's name
// (Session_FindLevel2DABCC(session, src->level), else Session_Level2E23AC,
// else session->level), "%s - %s" with the src->name named record's name
// when that resolves, or UnknownTitleName without a level; det =
// String_FormatN(STR_SAVE_DETAILS, completion, src->v32, and the floored
// stats 2E32D8, 2E3698, 2E3898, 2E34A8 (9th argument)); save =
// SYSTEM_CREATE_SAVE.
void SaveParams_Set(const SaveSrc* src, char* title, u32 titleN, char* sub, u32 subN, char* det, u32 detN, char* save,
                    s32 saveN) {
    const u32 tbl = global<be_u32>(kStrTable);
    if (tbl == 0) {
        call(b33fn::String_CopyN, title, kMissingTitle, titleN);
        call(b33fn::String_CopyN, sub, kMissingSubtitle, subN);
        call(b33fn::String_CopyN, det, kMissingDetails, detN);
        call(b33fn::String_CopyN, save, kMissingSave, saveN);
        return;
    }
    call(b33fn::String_CopyN, title, Lookup(tbl, kSysGameName, kMissingTitle, 0), titleN);

    u32 lvl = src->level;
    if (lvl != 0) lvl = call<u32>(Session_FindLevel2DABCC, Session(), lvl);
    if (lvl == 0) lvl = call<u32>(Session_Level2E23AC, Session());
    if (lvl == 0) lvl = gm<SaveSession>(Session()).level.ea;  // unchecked
    if (lvl == 0) {
        call(b33fn::String_CopyN, sub, Lookup(tbl, kUnknownTitle, kMissingLocation, 0), subN);
    } else {
        const SaveLevel& L = gm<SaveLevel>(lvl);
        const u32 name = Lookup(tbl, guest(&L.id), lvl, L.quiet);
        const u32 h = src->name;
        const u32 n = h != 0 ? call<u32>(Name_Resolve_002E1220, Session(), h) : 0u;
        if (n != 0) {
            const SaveNamed& N = gm<SaveNamed>(n);
            const u32 s2 = Lookup(tbl, guest(&N.id), n, N.quiet);
            call(b33fn::String_FormatN, sub, subN, kFmtTwoStrings, name, s2);
        } else {
            call(b33fn::String_CopyN, sub, name, subN);
        }
    }

    const u32 fmt = Lookup(tbl, kStrSaveDetails, kMissingDetails, 0);
    const u32 pct = StatInt(Game_CompletePercent2E3188, src);
    const s32 v32 = src->v32;
    const u32 a = StatInt(Game_Stat2E32D8, src);
    const u32 b = StatInt(Game_Stat2E3698, src);
    const u32 c = StatInt(Game_Stat2E3898, src);
    const u32 d = StatInt(Game_Stat2E34A8, src);
    call(b33fn::String_FormatN, det, detN, fmt, pct, v32, a, b, c, d);
    call(b33fn::String_CopyN, save, Lookup(tbl, kSysCreateSave, kMissingSave, 0), saveN);
}

}  // namespace sly4::obj

NATIVE(000AE78C, sly4::obj::SaveParams_Set)
