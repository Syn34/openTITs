// The reference-counted base and the UiRes base class (vtables 0x00B225E0,
// 0x00B225F8).
#include "res.h"

#include "../common/common.h"
#include "../string/str.h"

namespace sly4::font {
using namespace opentit::native;

namespace {
inline bool IsNameChar(u8 c) {
    const s8 s = static_cast<s8>(c);
    return (s >= 'A' && s <= 'Z') || (s >= 'a' && s <= 'z') || (s >= '0' && s <= '9') || c == '_';
}
}  // namespace

// [0x00B225E0 +0]
void Ref_Destruct(UiRes* r) { r->vtable = kRefVtbl; }

// [+4]
void Ref_Delete(UiRes* r) {
    if (!r) return;
    r->vtable = kRefVtbl;
    common::Mem_FreeFrom(r, 0);
}

// [+8] Deletes it when the count drops to 0.
void Ref_Release(UiRes* r) {
    UiRes& o = gref<UiRes>(guest(r));
    const u16 n = static_cast<u16>(o.refs - 1);
    o.refs = n;
    if (static_cast<s16>(n) > 0 || !r) return;
    vcall(r, kVfnDelete);
}

// Copies the path, expanding "$<var>" (only the variable whose name hashes
// to 0x243084BD; others are dropped), hashes the name and the file name,
// and puts the resource at the head of the resource list.
void UiRes_Construct(UiRes* r, const char* name, const char* path_in, u32, u32 a84) {
    GuestFrame frame(256 - 112);
    char* var = frame.as<char>(0);    // sp+112
    char* file = frame.as<char>(32);  // sp+144

    r->file_hash = 0u;
    r->refs = 1;
    r->vtable = kResVtbl;
    r->hash = str::String_HashNoCase(name);
    r->users = 0;
    r->loaded = 0;
    r->arg = a84;
    r->next = nullptr;

    u32 path = guest(path_in);
    u32 out = guest(r->path);
    u32 slash = out - 1;  // where the last '/' or '\' went (one before the start when there is none)
    u32 count = 0;
    while (path != 0 && gref<u8>(path) != 0) {
        const u8 c = gref<u8>(path);
        if (c == '/' || c == '\\') {
            slash = out;
        } else if (c == '$') {
            ++path;
            char* v = var;
            while (IsNameChar(gref<u8>(path))) *v++ = static_cast<char>(gref<u8>(path++));
            *v = 0;
            if (str::String_HashNoCase(var) != kResPathVarHash) continue;
            for (u32 s = global<be_u32>(kResPathVar); gref<u8>(s) != 0 && count < kResPathVarMax; ++s) {
                gref<u8>(out++) = gref<u8>(s);
                ++count;
            }
            continue;
        }
        gref<u8>(out++) = c;
        ++path;
        ++count;
    }
    gref<u8>(out) = 0;
    // The file name starts after the last slash (or at the start).
    common::String_Assign(file, host<const char>(slash + 1));
    r->file_hash = str::String_HashNoCase(file);
    r->next.ea = global<be_u32>(kResList).get();
    global<be_u32>(kResList) = guest(r);
}

// Thunk: the same with the fourth argument cut to a byte.
void UiRes_ConstructThunk(UiRes* r, const char* name, const char* path, u32 b, u32 a) {
    UiRes_Construct(r, name, path, b & 0xFF, a);
}

// [0x00B225F8 +0]
void UiRes_Destruct(UiRes* r) {
    r->vtable = kResVtbl;
    Res_Unlink(r);
    r->vtable = kRefVtbl;
}

// [+4]
void UiRes_Delete(UiRes* r) {
    if (!r) return;
    r->vtable = kResVtbl;
    Res_Unlink(r);
    r->vtable = kRefVtbl;
    common::Mem_FreeFrom(r, 0);
}

// [+8] One user fewer: unloads it when the last one goes, then releases
// the reference.
void UiRes_Unuse(UiRes* r) {
    UiRes& o = gref<UiRes>(guest(r));
    const u16 users = static_cast<u16>(o.users - 1);
    o.users = users;
    if (static_cast<s16>(users) <= 0 && o.loaded > 1) {
        vcall(r, kVfnUnload);
        o.loaded = 0;
    }
    const u16 refs = static_cast<u16>(o.refs - 1);
    o.refs = refs;
    if (static_cast<s16>(refs) > 0 || !r) return;
    vcall(r, kVfnDelete);
}

}  // namespace sly4::font

NATIVE(00794F20, sly4::font::Ref_Destruct)
NATIVE(00794F30, sly4::font::Ref_Delete)
NATIVE(00794F74, sly4::font::Ref_Release)
NATIVE(004D5274, sly4::font::UiRes_Construct)
NATIVE(004D54D4, sly4::font::UiRes_ConstructThunk)
NATIVE(004D5878, sly4::font::UiRes_Destruct)
NATIVE(004D58E4, sly4::font::UiRes_Delete)
NATIVE(004D5990, sly4::font::UiRes_Unuse)
