// Names for the audio functions: voice API wrappers, the emitter pool and
// the mix update. Addresses are guest addresses in the BCES01284 v01.02
// EBOOT; "?" marks a guess.
#pragma once

#include <bit>
#include <cmath>
#include <cstddef>

#include "opentit/native.h"
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

// ---------------------------------------------------------------- native structures
// A sorted key map: 12-byte entries, key first.
struct KeyMap {
    native::pad<2> _0;
    native::be_u16 count;      // +2
    native::pad<2> _4;
    native::be_u16 flags;      // +6: bit 0 = needs sorting
    native::be_u32 entries;    // +8
    native::pad<12> _12;
    native::be_u32 compare;    // +24 OPD
};
static_assert(offsetof(KeyMap, compare) == 24);
// A handle pool node (272 bytes): +0 prev, +4 next, +256 serial word (top bit = free).
struct HandleNode {
    native::be_u32 prev;       // +0
    native::be_u32 next;       // +4
    native::pad<248> _8;
    native::be_u32 serial;     // +256
    native::pad<12> _260;
};
static_assert(sizeof(HandleNode) == kNodeSize);
struct HandlePool {
    native::be_u32 w0;          // +0
    native::be_u32 free_head;   // +4 (-1 = empty)
    native::be_u32 first;       // +8
    native::be_u32 last;        // +12
    native::be_u32 used;        // +16
    native::be_u32 free_count;  // +20
    native::be_s32 capacity;    // +24
    native::be_u32 nodes;       // +28
};

// ---------------------------------------------------------------- native functions
// pool.cpp
s32 KeyMap_Find(KeyMap* map, const native::be_u32* key);
void HandlePool_Remove(native::be_u32* out, HandlePool* pool, u32 handle);
// voice.cpp
u32 Voice_IsPlaying(u32 handle);
s32 Voice_GetValue(u32 handle);
u32 Voice_Stop(u32 handle, u32 value);
// lib_math.cpp
void Listener_LocalDirection(native::be_f32* out, const native::be_f32* pos, const native::be_f32* listener);
bool Matrix_NearlyEqual(const native::be_f32* a, const native::be_f32* b, f32 eps);

}  // namespace audio
}  // namespace sly4

// ---------------------------------------------------------------- native emitter structures
namespace sly4::audio {

// The 57-byte "3D block" of an emitter (two copies, +16 and +76) and of a
// sound being started: volumes, pitches and pan computed by Emitter_Spatialise.
struct Block {
    native::be_f32 volume;        // +0 final volume
    native::be_f32 gain;          // +4
    native::be_f32 scale;         // +8 (zone gain)
    native::be_f32 fade;          // +12
    native::be_f32 vol1;          // +16 distance falloff
    native::be_f32 vol2;          // +20 second falloff
    native::be_f32 pitch;         // +24 min(cap, target)
    native::be_f32 pitch_target;  // +28
    native::be_f32 pitch_cap;     // +32
    native::be_u32 pitch_code;    // +36 Doppler pitch / play param
    native::be_f32 distance;      // +40
    native::be_u32 pan;           // +44 degrees
    native::be_s32 elevation;     // +48 (also the random angle range)
    native::be_u32 angle;         // +52
    u8 fixed_angle;               // +56
};
static_assert(sizeof(Block) == 57);

// A sound definition (SoundDesc).
struct SoundDef {
    native::pad<4> _0;
    native::gp<native::be_u32> bank;  // +4 -> bank pointer
    native::be_f32 near_dist;     // +8
    native::be_f32 far_dist;      // +12
    native::be_f32 curve;         // +16
    native::be_f32 range2;        // +20
    native::be_f32 curve2;        // +24
    native::be_f32 pitch_base;    // +28
    native::be_f32 pitch_curve;   // +32
    native::be_f32 range;         // +36
    native::be_f32 radius;        // +40
    native::pad<36> _44;
    u8 zones;                     // +80
    native::pad<3> _81;
    native::be_u32 id;            // +84 (passed as s16)
    native::be_u32 flagged;       // +88
    native::pad<4> _92;
    u8 no_cull;                   // +96
    native::pad<3> _97;
    native::be_s32 type;          // +100 (30 = ...; copied to the emitter's +168)
};
static_assert(offsetof(SoundDef, type) == 100);

// An emitter (a handle pool node's data, node + 16).
struct Emitter {
    native::be_u32 voice;          // +0 voice handle (-1 = none)
    native::gp<SoundDef> def;      // +4
    native::be_u32 zone;           // +8 zone object (was "bank")
    u8 bucketed;                   // +12
    u8 is_virtual;                 // +13
    u8 doppler;                    // +14
    u8 _15;
    Block base;                    // +16
    native::pad<3> _73;
    Block cur;                     // +76
    native::pad<3> _133;
    native::be_u32 shape;          // +136 shape object (was "spatial")
    native::be_f32 fade_cur;       // +140
    native::be_f32 fade_target;    // +144
    native::be_f32 pitch_cur;      // +148
    native::be_f32 pitch_target;   // +152
    native::be_u32 key;            // +156
    native::be_s32 priority;       // +160
    native::be_u32 bucket;         // +164
    native::be_u32 extra;          // +168
    u8 no_key;                     // +172
    native::pad<3> _173;
    native::be_f32 position[3];    // +176
    native::pad<4> _188;
    u8 forced;                     // +192
    native::pad<15> _193;
    native::be_f32 velocity[3];    // +208
    native::pad<4> _220;
    u8 moving;                     // +224
    native::pad<15> _225;
};
static_assert(offsetof(Emitter, cur) == 76 && offsetof(Emitter, shape) == 136 && offsetof(Emitter, position) == 176 &&
              offsetof(Emitter, velocity) == 208 && sizeof(Emitter) == kNodeSerial - kNodeData);

struct EmitterNode {
    native::be_s32 prev;      // +0
    native::be_s32 next;      // +4
    native::pad<8> _8;
    Emitter emitter;          // +16
    native::be_u32 serial;    // +256: bits 15..30 serial, top bit free
    native::pad<12> _260;
};
static_assert(sizeof(EmitterNode) == kNodeSize);

struct EmitterPool {
    native::be_u32 free_head;   // +0
    native::be_s32 free_tail;   // +4
    native::be_u32 first;       // +8
    native::be_u32 last;        // +12
    native::be_s32 used;        // +16
    native::be_u32 free_count;  // +20
    native::be_s32 capacity;    // +24
    native::be_u32 nodes;       // +28
    u8 can_grow;                // +32
};

inline EmitterPool& Pool() { return native::global<EmitterPool>(kEmitters); }
inline EmitterNode& PoolNode(u32 index) { return gref<EmitterNode>(Pool().nodes + index * kNodeSize); }

// The live emitter of `handle`, or nullptr (Emitter_FromHandle, native).
inline Emitter* Emitter_Of(u32 handle) { return native::host<Emitter>(Emitter_FromHandle(handle)); }

// Voice play parameters (220 bytes, "PlayParams"); +40.. the effect block.
struct VoiceParams {
    native::be_u32 size;      // +0 = 220
    native::be_u32 mask;      // +4 changed fields
    native::be_u32 opts;      // +8
    native::pad<4> _12;
    native::be_f32 gain;      // +16 (mask 4)
    native::be_u32 angle;     // +20 (mask 8)
    native::be_u32 spread;    // +24 (mask 0x10)
    native::be_u32 param;     // +28 (mask 1)
    native::pad<4> _32;
    u8 params[4];             // +36 (mask 0x10000 << i)
    native::be_u32 fx_mask;   // +40
    native::pad<16> _44;
    native::be_u32 w60;       // +60
    native::pad<4> _64;
    native::be_f32 f68, f72;  // +68
    native::be_u32 w76;       // +76
    native::be_f32 f80, f84, f88;  // +80
    native::be_u32 w92;       // +92
    native::be_f32 f96, f100, f104;  // +96
    native::pad<12> _108;
    native::be_f32 f120, f124, f128;  // +120
    native::be_u32 w132, w136, w140;  // +132
    native::be_f32 f144;      // +144
    native::pad<16> _148;
    native::be_f32 f164;      // +164
    native::pad<52> _168;
};
static_assert(sizeof(VoiceParams) == 220 && offsetof(VoiceParams, f164) == 164);

// The request Audio_StartFocusSound gets (PlayDesc).
struct PlayDesc {
    native::be_f32 position[3];  // +0
    native::pad<4> _12;
    native::be_f32 velocity[3];  // +16
    native::pad<4> _28;
    native::be_f32 gain;         // +32 (flag 4)
    native::be_u32 w36;          // +36: nonzero = not positioned
    native::be_u32 param;        // +40
    u8 params[4];                // +44
    native::be_u32 key;          // +48
    native::be_s32 priority;     // +52
    native::be_u32 flags;        // +56
    native::be_u32 zone;         // +60
    native::be_u32 shape;        // +64
};
static_assert(offsetof(PlayDesc, shape) == 64);

// ---------------------------------------------------------------- native functions (emitters)
void Emitter_Init(Emitter* e);
bool KeyMap_Set(u32 handle, u32 key, s32 priority);
void KeyMap_Insert(KeyMap* map, const native::be_u32* key, u64 value);
void HandlePool_Grow(EmitterPool* pool, s32 capacity);
void Emitter_BuildParams(VoiceParams* p, Block* cur, SoundDef* def, Block* prev);
bool Emitter_Start(u32 handle);
void Emitter_Update(Emitter* e, const native::be_f32* listener, u32 unused, u8 listener_still, f64 dt);
void Emitter_Stop(u32 handle, u8 fade);
void Audio_FocusPlay();
void Emitter_Spatialise(Block* s, const native::be_f32* l, const native::be_f32* pos, const native::be_f32* vel,
                        SoundDef* def, u32 zone, u32 shape, u8 occlude, u8 doppler);
void Audio_StartFocusSound(native::be_u32* out, SoundDef* desc, PlayDesc* play);
void Audio_UpdateMix(f64 dt);

}  // namespace sly4::audio
