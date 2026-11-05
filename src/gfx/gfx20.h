// The renderer range's next recompiled functions (gfx20*.cpp): the light
// culling pass, the particle emitter builder and its callees, the effect
// slot draw and the next undone renderer functions.
#pragma once

#include "gfx19.h"

namespace sly4::render {
namespace gfx20 {

namespace fn {
// Still recompiled or decompiled elsewhere (called from gfx20*.cpp).
constexpr u32 Matrix_LookAt = 0x0056A0D0;       // (f32 out[4][4], const f32 eye[3], const f32 dir[3], const f32 up[3]) (world/render.cpp)
constexpr u32 Sphere_FrustumFine = 0x007A6EFC;  // (const f32 planes[6][4], f32 c[3], f32 e[3], f32 m[3][4]) -> bool (obj/obj17.cpp)
}  // namespace fn

namespace g {
// The light table (gfx10.cpp's TableEntry_Acquire_005ACF90): +4 -> {u16
// capacity, u16 count}, +8 back-pointer array, +12 the 128-byte lights;
// gfx12.h's light_src (+16) and light_counts follow it.
constexpr u32 lights_on = 0x0119509C;  // u8
constexpr u32 lights = 0x011950A0;
constexpr u32 light_list = 0x011950B0;    // {Light*, f32 fade}[12] (gfx12.h light_src)
constexpr u32 light_counts = 0x01195110;  // u16 {total, in light_list, flagged ones appended after them}
}  // namespace g

// A light (128 bytes, g::lights +12): +0 u8 flags (bit 0 on, bit 1 goes
// after the others, bit 2 not listed this frame), +4 u32 orthographic,
// +16 eye, +32 direction, +48 up (f32[3] each, Matrix_LookAt), +80 / +84
// near / far, +88 / +92 half width / height (at the near plane for a
// perspective light), +103 u8 listed when non-zero, +104 / +108 the view
// depth its fade starts at and the fade's length.
constexpr u32 kLightSize = 128;
constexpr u32 kLightOrtho = 4, kLightEye = 16, kLightDir = 32, kLightUp = 48;
constexpr u32 kLightNear = 80, kLightFar = 84, kLightHalfW = 88, kLightHalfH = 92;
constexpr u32 kLightListed = 103, kLightFadeStart = 104, kLightFadeLength = 108;
constexpr u8 kLightOn = 1, kLightAfter = 2, kLightNotListed = 4;
constexpr u32 kMaxListed = 12;
// Lights_Cull_005AC840's constants (.text 0x005AC800): the two vperm
// masks of its 3x3 transpose, then f32 0.5, 0, 1 and f64 1, 0.
constexpr u32 kCullConsts = 0x005AC800;
// View fields it reads: +256 flags (bit 1: lights wanted), +272 the six
// frustum planes, +752 the world-to-view matrix.
constexpr u32 kViewFlags = 256, kViewPlanes = 272, kViewMatrix = 752;

namespace fn {
constexpr u32 Matrix_RotationX = 0x00553D18;      // (f32 m[4][4]; f1 angle) (obj/obj14_small.cpp)
constexpr u32 Matrix_RotationY = 0x00553E18;      // (f32 m[4][4]; f1 angle) (obj/obj14_small.cpp)
constexpr u32 Matrix_RotationZ = 0x00553F18;      // (f32 m[4][4]; f1 angle) (gfx7_math.cpp)
constexpr u32 Matrix_FromAxisAngle = 0x0056A410;  // (f32 m[4][4], const f32 axis[3]; f1 angle) (view/matrix.cpp)
constexpr u32 Spline_PointAt = 0x005602BC;        // (SplineData*, -, f32 out[3], bool; f1 t) (obj/obj8_math.cpp)
constexpr u32 Spline_TangentAt = 0x00560690;      // (SplineData*, -, f32 out[4], bool; f1 t) (obj/obj12_small.cpp)
constexpr u32 Mat4_Invert = 0x00554C28;             // (f32 tmp[16], f32 out[16], const f32 in[16]) (view/camera_mat.cpp)
constexpr u32 Vec4_TransformByMatrix = 0x00553910;  // (f32 out[4], const f32 v[4], const f32 m[16]) (obj/obj14_small.cpp)
constexpr u32 Records_AddToViews = 0x0058C9B0;     // (?, Rec*, ViewSet*) (obj/obj16_small.cpp)
constexpr u32 Iff_ChunkSize = 0x005637D4;          // (IffReader*) -> size (obj/obj12_small.cpp)
constexpr u32 FileReader_Read = 0x00563768;        // (FileReader*, void* buf, u32 size) -> bytes
constexpr u32 FileReader_ReadVec3Swapped = 0x005699F0;  // (FileReader*, f32 out[3], 0) -> bytes
constexpr u32 String_Length = 0x00555FA4;          // (const char*) -> length (string/string_misc.cpp)
constexpr u32 String_CopyN = 0x00553638;           // (dst, src, n) (uielem/ctor.cpp)
constexpr u32 PtrVector_Reserve = 0x00551718;      // (PtrVector*, u32 capacity) (container/vector.cpp)
constexpr u32 Tree_Collect_0058E990 = 0x0058E990;  // itself (gfx20.cpp)
constexpr u32 GeomDef_New = 0x007A71A4;            // () -> GeomDef* (obj/class_geomdef.cpp)
constexpr u32 Iff_ReadChunks = 0x00569388;         // (IffReader*, obj) (obj/obj6_iff.cpp)
constexpr u32 Array_Reserve = 0x00558F3C;          // (Array*, u16 capacity) (audio/more_containers.cpp)
constexpr u32 SetFlags54_0050A214 = 0x0050A214;    // (o, bool, bool, bool)
constexpr u32 Plane_Intersect3 = 0x0055EB58;       // (f32 out[3], const f32 a[4], b[4], c[4]) -> bool (obj/obj15_small.cpp)
constexpr u32 EmitterDef_Copy = 0x0058E3E4;        // (def, bool) -> copy (obj/obj5_new.cpp)
constexpr u32 Node_SyncTree_0058E680 = 0x0058E680; // itself (gfx20.cpp)
constexpr u32 strlen = 0x0071ED08;
constexpr u32 strchr = 0x00723678;
constexpr u32 Draw_BeginPrims = 0x005236A0;  // (Prims* out, FrameBuffer*, u8 type, u16 count; r7 extra) (world/draw.cpp)
constexpr u32 Draw_EndPrims = 0x00523748;    // (FrameBuffer*) (world/draw.cpp)
constexpr u32 Shader_LoadGroup = 0x0050F0F0;      // (u32) -> Material* (shadergroup.cpp)
constexpr u32 Gcm_SetVertexProgramAt = 0x005E3C00;  // (context, ucode offset) (gfx12.h)
constexpr u32 cellGcmSetDrawInlineArray = 0x005E51AC;  // (context, mode, words, data) (draw_state.h)
}  // namespace fn

// Emitter_BuildParticles_00506598: the emitter's particle ring (Emitter
// +72): +0 first slot, +4 end of the slots, +8 one past the last live
// particle, +12 the oldest, +20 live count; 36-byte particles: +0 f32[3]
// position, +12 f32 birth time, +16 f32[3] velocity, +28 u8 life scale
// (/127), +29 u8 size scale (/255), +30 / +31 s8 spin rate / phase
// (/127), +32 u32 random (low byte: colour darkening, layer, UV frame).
constexpr u32 kPoolFirst = 0, kPoolEnd = 4, kPoolLast = 8, kPoolOldest = 12, kPoolCount = 20;
constexpr u32 kParticleSize = 36;
constexpr u32 kPartBirth = 12, kPartVelocity = 16, kPartLife = 28, kPartSize = 29, kPartSpinRate = 30,
              kPartSpinPhase = 31, kPartRandom = 32;
// The draw record (gfx18.h): +0 f32[4][4] the emitter's matrix, +80 f32
// rate (age scale), +84 / +88 the view depth range kept, +92 colour
// curve, +96 motion, +100 size curve, +104 UV animation, +108 spin (all
// pointers, 0: none), +116 u8[4] tint, +124 particle count out, +129 u8
// 1: per-particle layer byte.
constexpr u32 kRecRate = 80, kRecFar = 84, kRecNear = 88, kRecColour = 92, kRecMotion = 96, kRecSize = 100,
              kRecUv = 104, kRecSpin = 108, kRecTint = 116, kRecCount = 124, kRecLayered = 129;
// Colour curve: +6 u8 key count, +8 u16 darkening, +12 / +16 f32 fade
// in / out rates, +20 u8[][4] keys. Motion: +16 f32[3] acceleration, +32
// f32 delay. Size curve: +8 u32 key count, +16 f32[] keys. UV
// animation: +12 f32 frame width, +16 / +24 / +28 / +32 per-mode
// factors, +20 u8 mode (1 by age, 2 looping, 3 ping-pong, 4 random
// frame). Spin: +16 f32[3] axis, +36 u32 key count, +40 f32[] angle
// keys, +172 f32 phase spread, +176 u8 axis (0 x, 1 y, 2 z, 3 +16).
constexpr u32 kMaxParticleIndex = 0x200;  // the build stops once more than this many were written
// Its constants (.text 0x00506560): f32 1/127, 1, (f64 +8, f32 +16
// unused here), f32 0 (+20), f64 0 (+24), f32 256, 0.5, 2 pi, 1/255, 1e-5, 2.
constexpr u32 kParticleConsts = 0x00506560;
// Obj23D50_FitBox_0052BC9C: the stencil box. Its 14-vertex triangle
// strip (corner bits: x, y, z; 1 = max) and the vertex format it draws
// with (pool.h's layout; +120 stride, +126 mask).
constexpr u8 kBoxStrip[14] = {0b110, 0b010, 0b111, 0b011, 0b001, 0b010, 0b000,
                              0b110, 0b100, 0b111, 0b101, 0b001, 0b100, 0b000};
constexpr u32 kBoxFormat = 0x010DD500;
constexpr u32 kBoxShaderGroup = 2;   // Shader_LoadGroup's argument
constexpr u32 kViewViewProj = 880;   // View +880: f32[4][4] (layers4::g::view_proj's source)
constexpr u32 kPrimTriangleStrip = 6;
constexpr u32 kAttribOff = 2;        // the format word of a disabled attribute
constexpr u32 kGcmIncrWrap = 0x8507, kGcmDecrWrap = 0x8508, kGcmKeep = 0x1E00, kGcmAlways = 0x0207;
constexpr u32 kGcmLess = 0x0201, kGcmLequal = 0x0202, kGcmCw = 0x0900;
// DebugFont_Draw_00565550: the font (+4 Texture* (+52 / +54 u16 width /
// height), +12 f32 line height, +28 u16 last code, +32 u16* glyph index
// per code, +40 Glyph[16 bytes]: s16 u0, v0, u1, v1 (texels), +10 s16
// width, +12 s16 advance); the vertices are 24 bytes {f32 x, y, z = -1;
// u8 colour[4]; f32 u, v}, 6 per glyph; tabs advance x to the next
// multiple of 30 (.text 0x00565548: f32 1, 30).
constexpr u32 kFontConsts = 0x00565548;
constexpr u32 kFontTexture = 4, kFontLineHeight = 12, kFontLastCode = 28, kFontIndex = 32, kFontGlyphs = 40;
constexpr u32 kGlyphSize = 16;
constexpr u32 kPrimQuadsAsTriangles = 5;  // Draw_BeginPrims type
constexpr u32 kDebugVertexSize = 24;
// LoadProgress_Update_004FB5E0's state: f32 the progress so far (stage
// weights summed) and the last value shown, the loading screen object
// (+94 u8: an unknown stage was reported); the stage weights (.text
// 0x004FB5C0: f32 0 (+0), 80 (+24), 10 (+28)).
namespace g {
constexpr u32 load_screen = 0x00F668E8;     // Obj* (name?)
constexpr u32 load_shown = 0x00F668EC;      // f32 (name?)
constexpr u32 load_progress = 0x00F668F0;   // f32 (name?)
}  // namespace g
constexpr u32 kProgressConsts = 0x004FB5C0;
constexpr u32 kLoadScreenUnknownStage = 94;
// The bank globals' quads (obj/obj15.h kBankGlobals +64: 112-byte
// entries, +68 u16 count): +0 f32[4][4] the corners (xyz each), +70
// char name[32].
constexpr u32 kBankQuads = 0x01193060 + 64, kBankQuadCount = 0x01193060 + 68;
constexpr u32 kQuadSize = 112, kQuadName = 70, kQuadNameMax = 31, kQuadNameRecord = 64;
// Records_AddList_0058CD44: the record set (u16 +12 its id) and the
// resource handle table (+4 base of 128-byte entries; resources.cpp
// kHandleTable).
constexpr u32 kRecordSet = 0x0118E080;
constexpr u32 kHandleTable = 0x01193004;
constexpr u32 kNoHandle = 0xFFFF;
// Tree_Collect_0058E990: the node (+4 u16 visits, +8 u32 flags (bits 6..9
// the collector's +56 level), +268 first child, +272 next sibling) and
// the collector (+36 PtrVector {data, u16 +40 capacity, u16 +44 count},
// u16 +46 most collected, u8 +56 level).
constexpr u32 kNodeVisits = 4, kNodeFlags = 8, kNodeChild = 268, kNodeNext = 272;
constexpr u32 kCollVector = 36, kCollCapacity = 40, kCollCount = 44, kCollPeak = 46, kCollLevel = 56;
// GeomDef_ReadChunks_0059B788: the geometry class (*0x00F8F12C; world
// hevent27_b.cpp kClassRegistry): +8 OPD called around loading (geom,
// +12 its argument, 0 before / 1 after), +16 the named map (Array of
// {u32 name, GeomDef*}: u16 +0 capacity, +2 count, +4 peak, +6 flags,
// +8 data). GeomDef: +8 u32 flags (0x400 registered), +96 name hash,
// +112 u8 LOD count, +113 u8 LODs given, +115 u8 last LOD, +132 the LOD
// list (8-byte entries, +0 the object SetFlags54_0050A214 marks).
constexpr u32 kGeomClass = 0x00F8F12C;
constexpr u32 kGeomRegistered = 0x400;
// Quads_ClipSetup_00578F64: the eight frustum corners as plane triples
// ({u32 a, b, c} indices into the six planes; .data 0x00F88994).
constexpr u32 kCornerPlanes = 0x00F88994;
constexpr u32 kFrustumPad = 0x01164BC0;  // gfx12::g::frustum_pad
// Node_SyncTree_0058E680: the scene node (beyond Tree_Collect's): +4
// bits 10..15 kind (10: emitter), +16 / +32 / +48 f32[3] rotation rows,
// +64 f32[3] position, +164 u32 flags (bits 6..7 follow the parent, bit
// 10 queued, bit 11 dirty), +208 its emitter def (+112 -> u8 +42 bit 7:
// copy on sync), +256 parent, +260 attached transform (+80 f32[3][4]).
constexpr u32 kNodeKindEmitter = 10;
constexpr u32 kNodeRot = 16, kNodePos = 64, kNodeState = 164, kNodeDef = 208, kNodeParent = 256, kNodeAttach = 260;
constexpr u32 kStateQueued = 0x400, kStateDirty = 0x800;
constexpr u32 kDirtyList = 0x0116BD00;      // PtrVector* {data, u16 +4 capacity, +8 count, +10 peak} (obj/obj.h)
constexpr u32 kDirtyListOff = 0x0116BD04;   // u8 (obj/obj15.h)
// An occluder (80 bytes, the caller's array): +0 f32[3] half extents,
// +16 f32[4][4] the transform into its box.
constexpr u32 kOccluderSize = 80;

}  // namespace gfx20
}  // namespace sly4::render
