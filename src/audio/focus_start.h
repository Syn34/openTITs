// Audio_StartFocusSound and the helpers it calls. Guest addresses in the
// BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include "audio.h"

namespace sly4 {
namespace audio {
namespace ffn {
constexpr u32 Audio_StartFocusSound = 0x00444610;  // (u32* out, SoundDesc*, PlayDesc*) decompiled: focus_start.cpp
constexpr u32 Emitter_Init = 0x00440B48;           // (Emitter*) (name?)
constexpr u32 KeyMap_Set = 0x0044385C;             // (u32 handle, u32 key, s32 priority) (name?)
constexpr u32 HandlePool_Grow = 0x007839B8;        // (Pool*, s32 capacity) (name?)
constexpr u32 Obj_CollectByType = 0x000DD258;      // (s32 type, PtrVector*) (name?)
constexpr u32 PtrVector_Construct = 0x005516E0;
constexpr u32 PtrVector_Clear = 0x0054EC1C;        // (PtrVector*) (name?)
constexpr u32 PtrVector_Free = 0x0054EC18;         // (PtrVector*) (name?)
constexpr u32 Snd_GetLastError = 0x006D585C;       // () -> s32 load of 0x011D71B4 (name?)
constexpr u32 memset = 0x00726E54;
constexpr u32 KeyMap_Find = 0x00784008;            // decompiled: audio/pool.cpp
}  // namespace ffn

// PlayDesc (the request): +0 position, +16 velocity, +32 f32 gain (flag 4),
// +36 u32, +40 u32 param (params +28), +44 u8[4] (params +36, flags 0x10000 << i),
// +48 key, +52 s32 priority, +56 flags, +60 bank/zone, +64 shape.
constexpr u32 kPlayGain = 32, kPlayKey = 48, kPlayPriority = 52, kPlayFlags = 56;
constexpr u32 kPlayZone = 60, kPlayShape = 64;
constexpr u32 kPlayNoKey = 1 /* (name?) */, kPlay3D = 2, kPlayHasGain = 4, kPlayDoppler = 8;

}  // namespace audio
}  // namespace sly4
