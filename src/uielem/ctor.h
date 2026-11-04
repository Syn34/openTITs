// Element and text element construction from an element description,
// and the text element's buffer reservation.
#pragma once

#include "slots.h"

namespace sly4 {
namespace uielem {

namespace cfn {
constexpr u32 UiElem_Construct = 0x004D76D0;   // (Element*, const ElemDesc*)
constexpr u32 String_CopyN = 0x00553638;       // (char* dst, const char* src, u32 n) -> dst, or 0 when n == 0
constexpr u32 String_HashNoCase = 0x0056A710;  // (const char*) -> u32
constexpr u32 strncpy = 0x00723C88;
constexpr u32 SmallAlloc = 0x0055238C;         // (u32 size)
constexpr u32 SmallFree = 0x00552420;          // (void*, u32 size)
constexpr u32 memset = 0x00726E54;
}  // namespace cfn

constexpr u32 kElemZeroVec2 = 0x01164AFC;  // f32[2] of zeros (text::kZeroVec2)
constexpr u32 kTextCtorConsts = 0x004DC424;  // f32 { 1e-5, FLT_MAX, 1.0 }

// Element fields the base constructor sets (see uielem.h for +136).
constexpr u32 kElemNameHash = 128;   // u32 String_HashNoCase(desc name)
constexpr u32 kElem140 = 140;        // f32[2], zeroed (name?)
constexpr u32 kElemArg = 160;        // u32 = desc +108 (name?)
constexpr u32 kElem164 = 164;        // u32, zeroed (name?)
constexpr u32 kElemNameLen = 32;     // char[32] at +96, copied with n = 31

// FontSet reference count / load state.
constexpr u32 kSetRefs = 4;          // u16
constexpr u32 kSetUsers = 80;        // u16
constexpr u32 kSetLoaded = 82;       // u16: vfunc +12's result
constexpr u32 kVfnSetLoad = 12;      // FontSet: () -> u16

}  // namespace uielem
}  // namespace sly4
