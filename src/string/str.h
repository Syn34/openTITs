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

}  // namespace sly4
