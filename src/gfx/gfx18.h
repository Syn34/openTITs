// The renderer range's next recompiled functions (gfx18*.cpp): the
// emitter draws Emitter_Draw_00507B18 picks, ...
#pragma once

#include "gfx17.h"

namespace sly4::render {
namespace gfx18 {

namespace fn {
// Still recompiled or decompiled elsewhere (called from gfx18*.cpp).
constexpr u32 PtrVector_Init = 0x00551598;  // (PtrVector*, u32 capacity, 0, 0) (shader3.h)
constexpr u32 Gcm_EnterSubContext_0052F780 = 0x0052F780;  // () (gfx10.cpp)
constexpr u32 RenderState_Reset_0053170C = 0x0053170C;    // () (gfx15.cpp)
constexpr u32 cellVideoOutSetGamma = 0x00ABE3A4;          // import (cellSysutilAvconfExt): (u32 video out; f1 gamma)
constexpr u32 Mem_NewAligned = 0x00731648;                // (u32 size, u32 align) -> pointer or 0 (platform.h)
constexpr u32 Mem_New = 0x007A3DD8;                       // (size) -> pointer (memory/alloc.cpp)
constexpr u32 strlen = 0x0071ED08;
constexpr u32 Debug_DrawLine = 0x00569CDC;                // (const Prim*) (obj/obj16_small.cpp)
constexpr u32 Debug_DrawRect = 0x00569EC8;                // (const Prim*) (obj/obj17.cpp)
constexpr u32 DebugFont_Draw_00565550 = 0x00565550;       // (DebugFont*, FrameBuffer*, const f32 pos[3], const char*, colour*; f1 size) (name?)
constexpr u32 Draw_SetTexture = 0x00523310;               // (FrameBuffer*, Texture*) (world/draw.cpp)
constexpr u32 Stream_PutByteCmd_005234DC = 0x005234DC;    // (FrameBuffer*, u8) (gfx8.cpp)
constexpr u32 Matrix_LookAt = 0x0056A0D0;                 // (f32 out[4][4], const f32 eye[3], const f32 dir[3], const f32 up[3]) (world/render.cpp)
constexpr u32 Obj23D50_FitBox_0052BC9C = 0x0052BC9C;      // (o, const f32 box[8] {min, -, max, -}, const f32 m[4][4]) (name?)
constexpr u32 MeshFormat_Get_0051B5C0 = 0x0051B5C0;       // (u32 kind) -> VertexFormat* (name?)
constexpr u32 Math_Sinf = 0x007345F0;                     // (f1 x) -> f1 (name?)
constexpr u32 Math_Cosf = 0x007344E0;                     // (f1 x) -> f1 (obj/obj10.h)
constexpr u32 Mem_NewAligned_00731620 = 0x00731620;       // (u32 size, u32 align) -> pointer (Mem_NewAligned's neighbour) (name?)
}  // namespace fn

namespace g {
// The per-particle arrays the emitter draws fill (gfx13.h has the rest).
constexpr u32 particle_uv = 0x010CAD30;   // f32[][4]: per particle index the texture rect {u0, u1, v1, v0} the sprite draws use (name?)
constexpr u32 particle_pos2 = 0x010D0D30;  // f32[][4]: the positions EmitterDraw_SpritesDefault_005019B8 draws from (name?)
constexpr u32 particle_dir = 0x010CED30;  // f32[][4]: per particle index the direction (xyz) EmitterDraw_Oriented_00505740 aligns to (name?)
// ApeTListAllocator<ApeDebug::Prim>*: {+0 ApeTFreeList*, +4 heap}; the
// list's nodes carry a Prim 16 bytes in, +4 the next node (name?).
constexpr u32 debug_prims = 0x01164E5C;
constexpr u32 vt_debug_prim_list = 0x00B24418;   // ApeTFreeList<ApeDebug::Prim>
constexpr u32 str_debug_prim_list = 0x00AF35A3;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = ApeDebug::Prim]"
constexpr u32 debug_draw_state = 0x01164BD0;     // the draw state the debug text binds (disp::g::identity's address) (name?)
// Frame_Begin_005325D8's blocks (names?).
constexpr u32 video = 0x010F0E20;         // +56 f32 gamma last set, +184 u32 target mode (1 or 3)
constexpr u32 patch_buffer = 0x010F1338;  // the shader patch buffer: +0 size, +4 u32 side (flips), +16 s32 free, +20 s32 lowest free seen
}  // namespace g

// EmitterDraw_Oriented_00505740's constants (.text 0x00505734): 1 and
// the |dot| above which the up vector switches to axis_b60.
constexpr f32 kOne = 1.0f;
constexpr f32 kAlongLimit = 0.95f;  // 0x3F733333
// EmitterDraw_Facing_005043B0's other up vector (uielem/vtbl.h's kRotationAxis).
constexpr u32 kAxisB70 = 0x01164B70;  // f32[3]

// The sprite draws' record (Emitter_Check_00506050 fills it; 144 bytes
// from the frame stack, Emitter_Draw_00507B18):
//   +0 f32[4][4] the emitter's matrix, +64 / +68 f32 half width / height,
//   +72 / +76 f32 pivot (x, y; in half sizes), +108 u32 positions are
//   offsets from each particle matrix's translation, +124 u32 particle
//   count, +128 u8 draw mode.
constexpr u32 kRecHalfW = 64, kRecHalfH = 68, kRecPivotX = 72, kRecPivotY = 76, kRecRelative = 108, kRecCount = 124;
// The stream buffers' cursors (g16::g::stream_buffers; gfx10.cpp's
// StreamBuffers_Flip_0050FE10): +216 u32 vertex stride, +232 vertex
// base, +236 index base, +240 vertex cursor (0: no buffer), +244 u16*
// index cursor; the batch just written: +248 first index, +252 count.
constexpr u32 kSbStride = 216, kSbVertexBase = 232, kSbIndexBase = 236, kSbVertexCursor = 240, kSbIndexCursor = 244,
              kSbFirst = 248, kSbCount = 252;
constexpr u32 kSpriteVertexSize = 24;

// Frame_Begin_005325D8's constants (.text 0x005325C0): f32 {0.8, 0.5,
// -0.5, 1, 0, 1.2}: the gamma range, the viewport scale and depth range.
constexpr u32 kFrameConsts = 0x005325C0;
constexpr u32 kStrPatchBufferFree = 0x00AF32E0;  // "INFO: Shader patch buffer: %3dk free\n"
constexpr u32 kPatchReserve = 512;
constexpr u32 kClearDepthStencil = 0xFFFFFF00u;
constexpr u32 kClearAll = 0xF3;  // CELL_GCM_CLEAR_Z | S | R | G | B | A
// Outline_Init_0050D2E0: the chunk handlers it registers ({tag, handler
// OPD}; the tags read 'OUTL', 'OUTB', ... in file order), the outline
// shader's name hash, the vertex format template (12-byte attributes
// up to +4 == 0) and the stream buffer's sizes.
struct ChunkReg {
    u32 tag, opd;
};
constexpr ChunkReg kOutlineChunks[8] = {
    {0x4C54554F, 0x00F3B580}, {0x4254554F, 0x00F3B590}, {0x4154554F, 0x00F3B598}, {0x5654554F, 0x00F3B5A0},
    {0x4554554F, 0x00F3B5A8}, {0x4F54554F, 0x00F3B5B0}, {0x5754554F, 0x00F3B5B8}, {0x484C544F, 0x00F3B588},
};
constexpr u32 kOutlineShaderHash = 0xF23CD1AEu;
constexpr u32 kOutlineFormat = 0x00F668F8;
constexpr u32 kCfgOutlineCount = 492;  // g::config u32: the outline buffers' entry count (0: 1) (name?)
constexpr u32 kOutlineBufferSize = 512, kOutlineBufferAlign = 128;
// Debug_Draw_0056AEFC: ApeDebug::Prim {+0 s32 kind, +4 colour, +12 / +16
// f32 x / y, +24 f32 size, +28 char text[]} (124 bytes) and its kinds.
constexpr u32 kPrimText = 3, kPrimLine = 7, kPrimRect = 8, kPrimDone = 9;
constexpr u32 kPrimSize = 124, kPrimNode = 16;
// Obj23D50_FitBoxes_0052C6B8's constants (.text 0x0052C680): f32 {1,
// 1e-5}, the two vperm masks of its transpose (+16 / +32) and {FLT_MAX,
// -FLT_MAX} (+48).
constexpr u32 kFitConsts = 0x0052C680;
constexpr u32 kZeroVec = 0x01164B40;  // f32[3]: the look-at's eye (0) (name?)
// Mesh_CreateSphere_0051B788: f32 {pi, 2 pi, 1} (.text 0x0051B778) and
// the mesh's name.
constexpr u32 kSphereConsts = 0x0051B778;
constexpr u32 kStrSphere = 0x00AF1A54;  // "Sphere"
constexpr u32 kPrimTriangleStrip = 6;   // CELL_GCM_PRIMITIVE_TRIANGLE_STRIP
constexpr u32 kScissorHorizontal2 = 0x000808C0u;  // SET_SCISSOR_HORIZONTAL, 2 words

// RenderState_Construct_0054E7D8: the render state's (fx::g::state)
// initial GCM enums, the program maps' Array header {u16 capacity 256,
// u16 count, u16, u16 element 16, +8 data, +12 allocator, +16, +20
// compare OPD}, the job buffer part (gfx10.cpp's k10JobPart) and the
// PostProc_ThermalObject ShaderType static.
constexpr u32 kGcmAlways = 0x207, kGcmKeep = 0x1E00;  // CELL_GCM_ALWAYS, CELL_GCM_KEEP
constexpr u32 kRenderStateSize = 236;
constexpr u32 kProgramMapCapacity = 256, kProgramMapElement = 16;
constexpr u32 kJobPart = 0x01163E80;               // {+0 u32, +4 bit 0 = 1, +8 bits 1..3 = 1, +12 top nibble cleared}
constexpr u32 kThermalName = 0x00B241C8;          // "PostProc_ThermalObject"
constexpr u32 kThermalTypeVtable = 0x00B241B0;    // its ShaderType vtable  // {f32 x, y, z; u8 colour[4]; f32 u, v}

}  // namespace gfx18
}  // namespace sly4::render
