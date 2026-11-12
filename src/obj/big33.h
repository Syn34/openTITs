// Game code, large-function sweep part 33: helpers shared by big33_*.cpp.
#pragma once

#include "native/objtypes.h"

namespace sly4::obj {

// Addresses of callees used by several big33 files.
namespace b33fn {
constexpr u32 Nop = 0x0054EC18;                  // empty function (r3 left as passed)
constexpr u32 PtrVector_Clear = 0x0054EC1C;      // (vec)
constexpr u32 PtrVector_Construct = 0x005516E0;  // (vec)
constexpr u32 String_FormatN = 0x00555CF4;       // (buf, size, fmt, ...)
constexpr u32 String_CopyN = 0x00553638;         // (dst, src, n)
constexpr u32 String_HashNoCase = 0x0056A710;    // (const char*) -> u32
}  // namespace b33fn

}  // namespace sly4::obj
