// More of the sound library: handler bookkeeping, logging, the bank and the
// helpers it uses. Builds on voice_layer.h. "?" marks a guess.
#pragma once

#include <bit>
#include <cmath>

#include "voice_layer.h"

namespace sly4 {
namespace snd {

namespace sfn2 {
constexpr u32 Vsnprintf = 0x0071F36C;        // (buf, size, fmt, va_list) (name?)
constexpr u32 Handler_OnPause = 0x006E9B40;  // (Handler*) (name?)
constexpr u32 Handler_OnResume = 0x006E9E0C; // (Handler*) (name?)
constexpr u32 Matrix_NearlyEqual = 0x005545B4;  // (const f32[16], const f32[16], f32 eps) -> bool (name?)
constexpr u32 Qsort = 0x0071EC28;
}  // namespace sfn2

// Logging.
constexpr u32 kLogDisabled = 0x011D73C0;  // u32: nonzero = Snd_Log prints nothing
constexpr u32 kLogBuffer = 0x011D71C0;    // char[512]
constexpr u32 kLogCallback = 0x00F8E53C;  // u32 OPD called with the formatted text

// Active handler list and the group pause flags.
constexpr u32 kFirstActive = 0x00F8E560;  // u32 Handler*
constexpr u32 kGroupFlags = 0x011D7DE0;   // u32[32]: bit 1 = paused
constexpr u32 kFmtInvalidValloc = 0x00AFFD34, kFileValloc = 0x00AFFD54;  // "src/common/valloc.cpp"
constexpr u32 kFmtInvalidBlock = 0x00AFE294, kFileBlocksnd = 0x00AFE2B4; // "src/common/blocksnd.cpp"

// Sub-records ("tracks"?) of a handler: 4 of 48 bytes from +92, on a list
// at kTrackList (linked by +44) with a count at kTrackCount.
constexpr u32 kTrackList = 0x00F8E30C, kTrackCount = 0x011D71B0;
constexpr u32 kTrackBase = 92, kTrackSize = 48, kTrackCountPerHandler = 4, kTrackNext = 44;

constexpr u32 kHTypeInfoPause = 36;  // TypeInfo +36: (Handler*, bool pause, opd) OPD
constexpr u32 kTypeCountPausable = 8;

// Single-precision PPU ops.
inline f32 fadds(f64 a, f64 b) { return static_cast<f32>(a + b); }
inline f32 fsubs(f64 a, f64 b) { return static_cast<f32>(a - b); }
inline f32 fmuls(f64 a, f64 c) { return static_cast<f32>(a * c); }
inline f32 fdivs(f64 a, f64 b) { return static_cast<f32>(a / b); }

}  // namespace snd
}  // namespace sly4
