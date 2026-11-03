// String helpers used by the text code: element paths and the localised
// string table.
#include "text.h"

using namespace sly4;
using namespace sly4::text;

namespace {
constexpr u32 kPathSegmentFormat = 0x00AEF49C;  // ".%s"
constexpr u32 kNotFoundFormat = 0x00AF0E38;     // "String id '%s' not found"
constexpr u32 kEmptyString = 0x00AF0E58;        // ""

// String table (Strings +4).
constexpr u32 kStringsTable = 4;
constexpr u32 kTableCount = 20;    // u32 number of ids
constexpr u32 kTableIds = 24;      // { u32 hash; s32 index; }[] sorted by hash
constexpr u32 kTableOffsets = 28;  // u32[] offset of each string
constexpr u32 kTableText = 32;     // char* base of the strings
}  // namespace

// void Ui_BuildPath(Element* e, char** cursor)
// Writes ".parent.….name" at *cursor (outermost first) and moves *cursor
// to the end of each segment's terminator.
DECOMP(004D3700, Ui_BuildPath) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 cursor = arg<u32>(ctx, 1);
    if (field<u32>(e, kElemParent) != 0) call(ctx, tfn::Ui_BuildPath, {field<u32>(e, kElemParent), cursor});
    const u32 name = e + kElemName;
    call(ctx, tfn::String_Printf, {load<u32>(cursor), kPathSegmentFormat, name});
    const u32 at = load<u32>(cursor);
    const s32 len = static_cast<s32>(call32(ctx, tfn::String_Length, {name}));
    store<u32>(cursor, at + static_cast<u32>(len + 1));
}

// const char* StringTable_Lookup(Strings* s, const char* id, const char* fallback, bool quiet)
// Binary search of the string table by String_HashNoCase(id). When it isn't
// there (or its index is negative) the "not found" message is formatted
// unless `quiet` (and dropped: a stripped log) and `fallback`, or "", is
// returned.
DECOMP(004F87C4, StringTable_Lookup) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 id = arg<u32>(ctx, 1);
    const u32 fallback = arg<u32>(ctx, 2);
    const u8 quiet = arg<u8>(ctx, 3);

    const u32 table = field<u32>(s, kStringsTable);
    if (table != 0) {
        const u32 hash = call32(ctx, fn::String_HashNoCase, {id});
        s32 hi = static_cast<s32>(field<u32>(table, kTableCount).get()) - 1, lo = 0;
        if (hi >= 0) {
            const u32 ids = field<u32>(table, kTableIds);
            const u32 t = field<u32>(s, kStringsTable);
            do {
                const s32 mid = (hi + lo) >> 1;
                const u32 entry = ids + static_cast<u32>(mid) * 8;
                const u32 h = load<u32>(entry);
                if (h > hash) {
                    hi = mid - 1;
                } else if (h < hash) {
                    lo = mid + 1;
                } else {
                    const s32 index = static_cast<s32>(load<u32>(entry + 4));
                    if (index < 0) break;
                    const u32 off = load<u32>(field<u32>(t, kTableOffsets) + (static_cast<u32>(index) << 2));
                    return ret(ctx, field<u32>(t, kTableText) + off);
                }
            } while (hi >= lo);
        }
    }
    if (quiet == 0) call(ctx, tfn::String_Format, {kNotFoundFormat, id});
    ret(ctx, fallback != 0 ? fallback : kEmptyString);
}
