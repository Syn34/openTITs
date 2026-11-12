// String helpers used by the text code: element paths and the localised
// string table.
#include "text.h"

#include "../string/str.h"

namespace sly4::text {
using namespace opentit::native;

namespace {
constexpr u32 kPathSegmentFormat = 0x00AEF49C;  // ".%s"
constexpr u32 kNotFoundFormat = 0x00AF0E38;     // "String id '%s' not found"
constexpr u32 kEmptyString = 0x00AF0E58;        // ""

struct StringId {
    be_u32 hash;   // +0
    be_s32 index;  // +4
};
struct StringTableData {
    pad<20> _0;
    be_u32 count;          // +20 number of ids
    gp<StringId> ids;      // +24 sorted by hash
    gp<be_u32> offsets;    // +28 offset of each string
    be_u32 text;           // +32 base of the strings
};
static_assert(offsetof(StringTableData, count) == 20 && offsetof(StringTableData, text) == 32);
struct Strings {
    pad<4> _0;
    gp<StringTableData> table;  // +4
};
}  // namespace

// Writes ".parent.….name" at *cursor (outermost first) and moves *cursor
// to the end of each segment's terminator.
void Ui_BuildPath(UiElem* e, be_u32* cursor) {
    if (e->parent) Ui_BuildPath(e->parent, cursor);
    call<s32>(tfn::String_Printf, cursor->get(), kPathSegmentFormat, e->name);
    const u32 at = *cursor;
    const s32 len = str::String_Length(e->name);
    *cursor = at + static_cast<u32>(len + 1);
}

// Binary search of the string table by String_HashNoCase(id). When it isn't
// there (or its index is negative) the "not found" message is formatted
// unless `quiet` (and dropped: a stripped log) and `fallback`, or "", is
// returned.
const char* StringTable_Lookup(void* strings, const char* id, const char* fallback, bool quiet) {
    Strings* s = static_cast<Strings*>(strings);
    if (s->table) {
        const u32 hash = str::String_HashNoCase(id);
        s32 hi = static_cast<s32>(s->table->count.get()) - 1, lo = 0;
        if (hi >= 0) {
            const StringId* ids = s->table->ids;
            const StringTableData* t = s->table;
            do {
                const s32 mid = (hi + lo) >> 1;
                const StringId& entry = ids[mid];
                const u32 h = entry.hash;
                if (h > hash) {
                    hi = mid - 1;
                } else if (h < hash) {
                    lo = mid + 1;
                } else {
                    const s32 index = entry.index;
                    if (index < 0) break;
                    const u32 off = gref<be_u32>(t->offsets.ea + (static_cast<u32>(index) << 2));
                    return host<const char>(t->text + off);
                }
            } while (hi >= lo);
        }
    }
    if (!quiet) call(tfn::String_Format, kNotFoundFormat, id);
    return fallback ? fallback : host<const char>(kEmptyString);
}

}  // namespace sly4::text

NATIVE(004D3700, sly4::text::Ui_BuildPath)
NATIVE(004F87C4, sly4::text::StringTable_Lookup)
