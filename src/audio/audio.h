// Names for the audio functions: voice API wrappers, the emitter pool and
// the mix update. Addresses are guest addresses in the BCES01284 v01.02
// EBOOT; "?" marks a guess.
#pragma once

#include <bit>
#include <cmath>

#include "../sly4.h"

namespace sly4 {
namespace audio {

// ---------------------------------------------------------------- PPU float ops
inline f32 fadds(f64 a, f64 b) { return static_cast<f32>(a + b); }
inline f32 fsubs(f64 a, f64 b) { return static_cast<f32>(a - b); }
inline f32 fmuls(f64 a, f64 c) { return static_cast<f32>(a * c); }
inline f32 fdivs(f64 a, f64 b) { return static_cast<f32>(a / b); }
inline f32 fsqrts(f64 a) { return static_cast<f32>(std::sqrt(a)); }
inline f32 fmadds(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }     // a * c + b
inline f32 fnmsubs(f64 a, f64 c, f64 b) { return static_cast<f32>(-std::fma(a, c, -b)); }  // b - a * c
inline f64 fsel(f64 a, f64 c, f64 b) { return a >= 0.0 ? c : b; }                           // a >= 0 ? c : b
// fctiwz, then the low byte sign-extended (lwz + extsb in the original).
inline s32 fctiwz_s8(f64 x) { return static_cast<s8>(static_cast<u8>(rt::fctiw(x, true))); }

inline u32 vfunc(u32 obj, u32 offset) { return load<u32>(load<u32>(obj) + offset); }

// ---------------------------------------------------------------- functions
namespace afn {
constexpr u32 Voice_Lock = 0x006EFD10, Voice_Unlock = 0x006EFD34;  // () (name?)
constexpr u32 Voice_Lookup = 0x006E5B6C;    // (handle) -> Voice* or 0 (name?)
constexpr u32 Voice_GetCount = 0x006E6718;  // (Voice*) -> s32 (name?)
constexpr u32 Voice_DoStop = 0x006E620C;    // (Voice*, 1, u32, 0) (name?)
constexpr u32 Voice_IsPlaying = 0x006D5E10; // decompiled: audio/voice.cpp
constexpr u32 Voice_GetValue = 0x006D64AC;
constexpr u32 Voice_Stop = 0x006D60AC;
constexpr u32 Voice_SetParam = 0x006D50F0;  // (voice, u32 which, s32 value) (name?)
constexpr u32 Voice_GetSub = 0x006D6514;    // (voice, s16 i) -> sub or 0 (name?)
constexpr u32 Sub_GetInfo = 0x006FA384;     // (sub, s32* a, s32* b, 0, 0) (name?)
constexpr u32 Voice_Play = 0x006D5EE0;      // (bank, s16 id, Params*, -2) -> voice (name?)
constexpr u32 Emitter_BuildParams = 0x00441D68;  // (Params*, Emitter3D*, SoundDef*, 0) (name?)
constexpr u32 Emitter_Spatialise = 0x00442478;   // (8 args, f1 = dt) (name?)
constexpr u32 Voice_SetPitch = 0x006D7508;  // (f32) -> u32 (name?)
constexpr u32 Obj_GetOcclusion = 0x00213870;  // (obj, const f32 pos[3]) -> f32 (name?)
constexpr u32 SortCompare = 0x0071EC28;     // qsort(base, count, size, cmp) (name?)
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 Pool_Free = 0x00552420;       // (ptr, size)
constexpr u32 Audio_FocusPlay = 0x0044A3D4;  // () (name?)
constexpr u32 Emitter_Stop = 0x00443424;   // (u32 handle, u8) (name?)
}  // namespace afn

// ---------------------------------------------------------------- globals
constexpr u32 kAudioReady = 0x010AEC60;  // u8
constexpr u32 kTuningValue = 72;         // tuning objects keep their value at +72

// Emitter pool (a handle pool): see HandlePool_Remove.
constexpr u32 kEmitters = 0x010B0200;
constexpr u32 kNodeSize = 272, kNodeData = 16, kNodeSerial = 256;
constexpr u8 kNodeFree = 0x80;

// Pointer to the emitter of `handle` if the handle is live, else 0 (the
// check the original inlines everywhere).
inline u32 Emitter_FromHandle(u32 handle) {
    if (ref<u8>(kAudioReady) == 0) return 0;
    const u32 serial = handle & 0xFFFF;
    if (serial == 0) return 0;
    const u32 index = handle >> 16;
    if (!(static_cast<s32>(index) < load<s32>(kEmitters + 24))) return 0;
    const u32 node = load<u32>(kEmitters + 28) + index * kNodeSize;
    if (((load<u32>(node + kNodeSerial) >> 15) & 0xFFFF) != serial) return 0;
    if (ref<u8>(node + kNodeSerial) & kNodeFree) return 0;
    return node + kNodeData;
}

// Default comparator of a sorted map (OPD); others go through call_ptr.
constexpr u32 kMapDefaultCompare = 0x00F39AB0;

}  // namespace audio
}  // namespace sly4
