// UI resources: the reference-counted objects the UI files declare (fonts,
// font sets, textures, included files). All of them are on one list
// (0x00F66864, linked by +88) that the loaders search by name hash.
#pragma once

#include <cstddef>

#include "opentit/native.h"
#include "../sly4.h"

namespace sly4 {
namespace font {

using namespace opentit;

// Vtables.
constexpr u32 kRefVtbl = 0x00B225E0;      // reference-counted base: +0 dtor, +4 delete, +8 Release
constexpr u32 kResVtbl = 0x00B225F8;      // UiRes: +8 Unuse, +12 Load (pure), +16 Unload (pure)
constexpr u32 kTextureVtbl = 0x00B22618;  // made by 0x004D5A4C (name?)
constexpr u32 kFontVtbl = 0x00B22638;     // made by 0x004D601C
constexpr u32 kFontSetVtbl = 0x00B22658;  // made by 0x004D71D0
constexpr u32 kIncludeVtbl = 0x00B22678;  // made by ?, Load parses another UI file (name?)

// UiRes fields.
constexpr u32 kRefCount = 4;      // u16, 1 at construction
constexpr u32 kResPath = 6;       // char[]: the path, "$var" expanded
constexpr u32 kResFileHash = 72;  // u32 String_HashNoCase(file name after the last slash)
constexpr u32 kResHash = 76;      // u32 String_HashNoCase(name)
constexpr u32 kResUsers = 80;     // u16 (signed in the tests)
constexpr u32 kResLoaded = 82;    // u16: 0 not loaded, 2 loaded (Load's result)
constexpr u32 kResArg = 84;       // u32 (name?)
constexpr u32 kResNext = 88;      // UiRes*
constexpr u32 kResList = 0x00F66864;
constexpr u32 kResPathVar = 0x00F668A4;  // const char*: what "$<var>" expands to
constexpr u32 kResPathVarHash = 0x243084BD;
constexpr u32 kResPathVarMax = 63;       // the expansion stops at 63 characters in all

constexpr u32 kVfnDelete = 4;
constexpr u32 kVfnUnuse = 8;
constexpr u32 kVfnLoad = 12;    // () -> u16 state
constexpr u32 kVfnUnload = 16;  // ()

// FontSet fields.
constexpr u32 kSetScale = 92;     // f32
constexpr u32 kSetLineScale = 96; // f32
constexpr u32 kSetCount = 100;    // u32
constexpr u32 kSetFonts = 104;    // Font*[16]
constexpr u32 kSetFontScale = 168;// f32[16]
constexpr u32 kSetKerning = 232;  // f32[16]
constexpr u32 kFontLineHeight = 102;  // u16
constexpr u32 kFontSetConsts = 0x007954C8;  // f32 1.0

namespace rfn {
constexpr u32 UiRes_Construct = 0x004D5274;
constexpr u32 String_HashNoCase = 0x0056A710;
constexpr u32 String_Assign = 0x00555F80;  // strcpy
constexpr u32 Mem_FreeFrom = 0x0054EE6C;
}  // namespace rfn

// ---------------------------------------------------------------- native structures
// UiRes (92 bytes): the base of every UI resource.
struct UiRes {
    be_u32 vtable;                // +0
    be_u16 refs;                  // +4, 1 at construction
    char path[66];                // +6 the path, "$var" expanded
    be_u32 file_hash;             // +72 String_HashNoCase(file name after the last slash)
    be_u32 hash;                  // +76 String_HashNoCase(name)
    be_u16 users;                 // +80 (signed in the tests)
    be_u16 loaded;                // +82 0 not loaded, 2 loaded (Load's result)
    be_u32 arg;                   // +84 (name?) the Include* that declared it
    native::gp<UiRes> next;       // +88
};
static_assert(sizeof(UiRes) == 92 && offsetof(UiRes, file_hash) == kResFileHash && offsetof(UiRes, next) == kResNext);

// The texture resource (vtable 0x00B22618, 112 bytes).
struct UiTexture {
    UiRes res;          // +0
    be_u32 flags;       // +92 0x80000000 depth LEQUAL, 0x40000000 depth test (from two bools)
    be_u32 handle;      // +96 Texture* (a Ref)
    be_u32 pages[3];    // +100 Material* per blend index
};
static_assert(sizeof(UiTexture) == 112);

// Native functions (src/font/res.cpp).
void Ref_Destruct(UiRes* r);
void Ref_Delete(UiRes* r);
void Ref_Release(UiRes* r);
void UiRes_Construct(UiRes* r, const char* name, const char* path, u32, u32 arg);
void UiRes_ConstructThunk(UiRes* r, const char* name, const char* path, u32 b, u32 arg);
void UiRes_Destruct(UiRes* r);
void UiRes_Delete(UiRes* r);
void UiRes_Unuse(UiRes* r);

// The resource list's head.
inline UiRes* Res_First() { return native::host<UiRes>(native::global<be_u32>(kResList)); }
// The resource on the resource list whose name hashes to `hash`, or 0.
inline UiRes* Res_Find(u32 hash) {
    for (UiRes* r = Res_First(); r; r = r->next)
        if (r->hash == hash) return r;
    return nullptr;
}
// Take r off the resource list (inlined by every UiRes destructor).
inline void Res_Unlink(UiRes* r) {
    UiRes* prev = nullptr;
    for (UiRes* it = Res_First(); it; it = it->next) {
        if (it == r) {
            if (prev)
                prev->next.ea = r->next.ea.get();
            else
                native::global<be_u32>(kResList) = r->next.ea.get();
            break;
        }
        prev = it;
    }
}

}  // namespace font
}  // namespace sly4
