// Callees of the emitter, focus and sound-library code: addresses in the
// BCES01284 v01.02 EBOOT; "?" marks a guess. Decompiled in more_*.cpp.
#pragma once

#include "../sly4.h"

namespace sly4 {
namespace mfn {
// more_emitter.cpp
constexpr u32 Emitter_Init = 0x00440B48;      // (Emitter*)
constexpr u32 KeyMap_Set = 0x0044385C;        // (u32 handle, u32 key, s32 priority) -> bool
constexpr u32 KeyMap_Insert = 0x00784194;     // (Map*, const u32* key, u64 {handle, priority}) (name?)
constexpr u32 HandlePool_Grow = 0x007839B8;   // (Pool*, s32 capacity) (name?)
// more_containers.cpp
constexpr u32 Obj_CollectByType = 0x000DD258;  // (s32 type, PtrVector*) (name?)
constexpr u32 PtrVector_Clear = 0x0054EC1C;    // (PtrVector*) (name?)
constexpr u32 Nop = 0x0054EC18;                // empty function (was "PtrVector_Free")
constexpr u32 Array_Reserve = 0x00558F3C;      // (Array*, u16 capacity) (name?)
// more_snd.cpp
constexpr u32 Snd_GetLastError = 0x006D585C;   // () -> s32 (name?)
constexpr u32 Snd_GetTick = 0x006D4F74;        // () -> u32 (name?)
constexpr u32 Channel_ReleaseVoice = 0x006EFE90;  // (Voice* hw) (name?)
constexpr u32 Channel_Reset = 0x006E7EC0;      // (s32 ch) (name?)
constexpr u32 Effects_Set = 0x006F2814;        // (Effects* dst, const Effects* src) (name?)
// more_math.cpp
constexpr u32 Math_FLog = 0x007062C8;          // (f32 x, s32 base: 0 ln, > 0 log10, < 0 log2) -> f32 (was "Snd_Log2")
constexpr u32 Math_FDunscale = 0x0071F668;     // (s16* exp, f32* x) -> s32 class (libm _FDunscale) (name?)

// Left recompiled.
constexpr u32 Voice_SetStateAtomic = 0x007106BC;  // (obj, u32) ldarx/stdcx. loop (name?)
constexpr u32 Math_FDnorm = 0x0072B094;        // (f32*) -> s16 (libm _FDnorm) (name?)
constexpr u32 Math_Feraise = 0x007060A8;       // (u32 flags) (libm _Feraise) (name?)
constexpr u32 Mem_Alloc = 0x0054ED38;          // (size, pool, align)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;       // (p, pool)
constexpr u32 PtrVector_Reserve = 0x00551718;
constexpr u32 qsort = 0x0071EC28;              // (base, count, size, cmp)
constexpr u32 memmove = 0x0071C3F0;
}  // namespace mfn
}  // namespace sly4
