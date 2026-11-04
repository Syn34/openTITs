// UI resources: the reference-counted objects the UI files declare (fonts,
// font sets, textures, included files). All of them are on one list
// (0x00F66864, linked by +88) that the loaders search by name hash.
#pragma once

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

// Inlined by every UiRes destructor: take r off the resource list.
// Leaves r4 = the previous entry (0 when r was first), as the original.
void Res_Unlink(PPUContext& ctx, u32 r);

}  // namespace font
}  // namespace sly4
