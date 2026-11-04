// Shared names for src/world/hevent7_*.cpp: callees of the World event
// handler's out-of-line helpers (hevent6_*.cpp) and more of the handler's
// remaining callees. Guest addresses in the BCES01284 v01.02 EBOOT; "?"
// marks a guess.
#pragma once

#include <bit>

#include "hevent6.h"

namespace sly4 {
namespace hevent7 {

namespace h7fn {
constexpr u32 SubMesh_ConstructBase = 0x004FCFE8;  // (SubMesh*) -> 0x004FCE88 (name?)
constexpr u32 Target_Init = 0x00530DB4;            // (Target** out, w, h, format, aa, where)
constexpr u32 Surface_SetBuffer = 0x00532D5C;      // (Surface*, Target*)
constexpr u32 Mem_AllocPool = 0x0056C514;          // (align, size, pool, flags)
constexpr u32 cellGcmAddressToOffset = 0x00ABD5E4; // import stub
constexpr u32 RenderSurface_Alloc = 0x0051AF28;    // () -> RenderSurface*
constexpr u32 SmallFree = 0x00552420;              // (void* p, u32 size)
constexpr u32 Obj_Offset362830 = 0x00362830;       // (vec3* out, s32 kind, const vec3* pos) (name?)
constexpr u32 Motion_Stop4A0C08 = 0x004A0C08;      // (Motion*): +4, +8 = 0 (name?)
constexpr u32 Motion_Set4A0C20 = 0x004A0C20;       // (Motion*; f32) (name?)
constexpr u32 WorldObj_MarkDirty = 0x0057D000;     // (WorldObj*)
constexpr u32 Voice_GetValue = 0x006D64AC;         // (voice) -> s32 sub-voice count (name?)
constexpr u32 Voice_GetSub = 0x006D6514;           // (voice, s16 i) -> sub or 0 (name?)
constexpr u32 Sub_GetInfo = 0x006FA384;            // (sub, s32* a, s32* b, 0, 0) (name?)
constexpr u32 SmallAlloc = 0x0055238C;             // (size) -> block or 0
constexpr u32 Effect_Construct = 0x004C7A68;       // (Effect*, def)
constexpr u32 Effect_Start = 0x004C753C;           // (Effect*)
constexpr u32 Effect_FindComponent = 0x004BB4B4;   // (Effect*, u32 type hash) -> component or 0
constexpr u32 qsort = 0x0071EC28;                  // libc (base, n, size, compare OPD)
constexpr u32 Sub_GetTime6F9A18 = 0x006F9A18;      // (sub, f32* out, 0) (name?)
constexpr u32 fios_ReadPath5FF4F4 = 0x005FF4F4;    // (scheduler, attr*, path, buf, size) -> fios_op* (name?)
}  // namespace h7fn

// Call through an OPD; like the original sequence, the OPD and its entry
// address are left in the two registers after the arguments.
inline u64 opdcall(PPUContext& ctx, u32 opd, std::initializer_list<u64> args) {
    ctx.gpr[3 + args.size()] = opd;
    ctx.gpr[4 + args.size()] = load<u32>(opd);
    return call_ptr(ctx, opd, args);
}

// The f64 that lfs makes of the single at ea (NaNs keep their payload,
// signalling ones included, as on the PPU).
inline f64 lfs_f64(u32 ea) {
    const u32 w = load<u32>(ea);
    if ((w & 0x7F800000u) == 0x7F800000u && (w & 0x007FFFFFu) != 0)
        return std::bit_cast<f64>((static_cast<u64>(w & 0x80000000u) << 32) | 0x7FF0000000000000ull |
                                  (static_cast<u64>(w & 0x007FFFFFu) << 29));
    return static_cast<f64>(std::bit_cast<f32>(w));
}

constexpr u32 kSubMeshVtable = 0x00B23EC0;
constexpr u32 kZeroVec = 0x01164B40;        // vec3 {0, 0, 0} (as hevent6_d.cpp)
constexpr u32 kObjLists = 0x0106C2D4;       // per-kind object lists {+0 Object**, +8 u16 count}, 20 bytes each
constexpr u32 kObjListStride = 20;
constexpr u32 kKindNone = 5;

// Emitter pool (as hevent6_i.cpp): +24 s32 capacity, +28 272-byte nodes
// (+16 voice, +256 u32: bit 31 free, bits 15..30 serial).
constexpr u32 kEmitters = 0x010B0200;
constexpr u32 kPoolCapacity = 24, kPoolNodes = 28, kNodeSize = 272, kNodeVoice = 16, kNodeSerial = 256;
constexpr u32 kMusicPushStack = 0x010AEC60;  // u8: push stack active (as hevent6_i.cpp)
// Sound: +0 sample (+0 data, +88 s32 pending), +4 u32 handle (u16 emitter
// index, u16 serial), +97 u8 playing.
constexpr u32 kSndSample = 0, kSndEmitter = 4, kSndPlaying = 97;
constexpr u32 kSampleData = 0, kDataPending = 88;

}  // namespace hevent7
}  // namespace sly4
