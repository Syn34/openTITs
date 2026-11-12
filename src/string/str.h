// String helpers and the C library string functions they sit on.
#pragma once

#include "../sly4.h"

namespace sly4 {

// C library string functions (still recompiled).
namespace libc {
constexpr u32 strlen = 0x0071ED08;   // (const char*) -> u32
constexpr u32 strstr = 0x0071EEF8;   // (const char* hay, const char* needle)
constexpr u32 strchr = 0x00723678;   // (const char*, int c)
constexpr u32 strncpy = 0x00723C88;  // (char* dst, const char* src, u32 n): pads with zeros
}  // namespace libc

// Native functions (src/string).
namespace str {
char* String_Copy(char* dst, const char* src, u32 size);
u32 String_HashNoCase(const char* s);
s32 String_Compare(const char* a, const char* b);
s32 String_Length(const char* s);
s32 Mem_Compare(const void* a, const void* b, u32 n);
// String_Printf (0x00551540) is variadic and reads its caller's frame: call it with call<>.
}  // namespace str

}  // namespace sly4
