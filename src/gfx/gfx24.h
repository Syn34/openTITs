// The renderer range's last large recompiled functions (gfx24*.cpp): the
// direct (non-job) slot culling loops (gfx24_slots.cpp), the light volume
// draws (gfx24_light.cpp), the geometry batch draw (gfx24_batch.cpp) and
// the view frame driver (gfx24_frame.cpp).
#pragma once

#include "gfx23.h"

namespace sly4::render {
namespace gfx24 {

namespace fn {
// Still recompiled or decompiled elsewhere (called from gfx24*.cpp).
constexpr u32 Sphere_FrustumFine = 0x007A6EFC;  // (const f32 planes[8 SoA] (+96), f32 c[3], f32 e[3], f32 m[3][4]) -> bool (obj/obj17.cpp)
constexpr u32 Slots_PrepareB = 0x0058B128;      // () fills the B parameter block (obj/obj16_small.cpp)
}  // namespace fn

// Slots_UpdateDirect_00589BC8 / Slots_UpdateDirectB_0058B4B8: the
// non-job versions of the slot culling job. Both walk 128-byte slot
// records and append a 16-byte draw entry for each visible one.
namespace slots {
// The slot record (128 bytes).
constexpr u32 kRecSize = 128;
constexpr u32 kRecMatrix = 0;      // f32 3 rows of 4 (stride 16): the box orientation; +12 nonzero = entry flag 0x10
constexpr u32 kRecHasExtra = 12;
constexpr u32 kRecPos = 48;        // f32[3]: the LOD / distance point
constexpr u32 kRecSphere = 64;     // f32[4]: centre, radius
constexpr u32 kRecRadius = 76;
constexpr u32 kRecExtents = 80;    // f32[3]: the box half extents
constexpr u32 kRecMask = 92;       // u32: visibility mask (0 = always) against the block's +2996
constexpr u32 kRecDrawWord = 96;   // u32: copied to the entry's +0
constexpr u32 kRecMaxDistSq = 100; // f32: (distance * scale)^2 limit
constexpr u32 kRecFadeNear = 104;  // f32 (name?)
constexpr u32 kRecFadeA = 108;     // u16 (name?)
constexpr u32 kRecFadeB = 110;     // u16 (name?)
constexpr u32 kRecLodDist = 112;   // f32[]: LOD distance thresholds
constexpr u32 kRecFlags = 124;     // u16: 1|2 both needed, 4 skip, 8 no extra lists / entry 8, 0x20 box occlusion test (B)
constexpr u32 kRecAlpha = 126;     // u8: scaled by the fade
constexpr u32 kRecLodCount = 127;  // u8
constexpr u32 kFlagNeed = 3, kFlagSkip = 4, kFlagLit = 2, kFlagNoExtra = 8, kFlagBoxOcclude = 0x20;
// The draw entry (16 bytes).
constexpr u32 kEntrySize = 16;
constexpr u32 kEntryWord = 0, kEntryRec = 4, kEntryLod = 8, kEntryAlpha = 9, kEntryFlags = 10;
// The parameter block (the job's; B's is Slots_PrepareB's 0x0118B180).
constexpr u32 kPlanes = 96;        // 8 planes SoA: x +96/+160, y +112/+176, z +128/+192, d +144/+208
constexpr u32 kExtraSetStride = 224;  // extra plane set k at +224 * (k + 1)
constexpr u32 kViewPos = 2960;     // f32[3]
constexpr u32 kMaskBits = 2996;    // u32
constexpr u32 kDistScale = 3000;   // f32
constexpr u32 kOutCursor = 3100;   // u32*: receives the final entry pointer (atomically)
constexpr u32 kExtraCursors = 3156; // u32*[]: per extra set, a cursor word (pointer array of entry pointers)
constexpr u32 kExtraCount = 3208;  // u32
constexpr u32 kBlockA = 0x0117E280, kEntriesA = 0x0116E110;
constexpr u32 kBlockB = 0x0118B180, kEntriesB = 0x01183100;
// The slot table: +0 records, u16 +4 count (A), u16 +6 first - 1 / +8 end (B); 0xFFFF = none.
constexpr u32 kSlotTable = 0x01193004;
// The occluders: +0 SoA planes (groups of 4 occluders x 5 planes: x, y, z,
// d vectors per 4 planes, 320 bytes), u16 +4 occluder count.
constexpr u32 kOccluders = 0x0117F010;
constexpr u32 kOccluderSize = 80, kOccluderGroup = 320;
// The function-local constants: 5 vperm masks regrouping 20 plane values
// into the 4 occluders' 5, then f32 100, 65535, 1e-5, 0, 1/255, 1.
constexpr u32 kConstsA = 0x00589B60, kConstsB = 0x0058B450;
constexpr u32 kC100 = 80, kC65535 = 84, kCEps = 88, kCZero = 92, kC255th = 96, kCOne = 100;
}  // namespace slots

// LightVolume_DrawStencil_00573600 / LightVolume_DrawOutside_00574370:
// the light volume (LightVolumes_Draw's 144-byte light, 336-byte
// matrices) drawn as its frustum box (Matrix_PackShaderRows of the 8
// corners: 14 vertices, a strip) in stencil passes, then lit where the
// stencil marks it.
namespace lightvol {
// Still recompiled or decompiled elsewhere.
constexpr u32 View_GetCurrent = 0x004FDBA8;
constexpr u32 Mat4_Invert = 0x00554C28;             // (out, tmp, in) (view/camera_mat.cpp)
constexpr u32 Matrix_MulAffine = 0x0079DE98;        // (out, a, b) (gfx/gfx9.cpp)
constexpr u32 Matrix_MultiplyTo = 0x0055499C;       // (out, a, b)
constexpr u32 Matrix_PackShaderRows = 0x00573410;   // (const f32 m[42], f32 out[42]) (obj/obj15_small.cpp)
constexpr u32 Shader_LoadGroup = 0x0050F0F0;        // (index) -> group (gfx/shadergroup.cpp)
constexpr u32 Vec_TransformByMatrix = 0x00553A2C;   // (out, in xyz, m) (view/vector.cpp)
constexpr u32 FProgram_SetConstant = 0x0052F270;    // (index, const f32 v[4]) (gfx/shader3.cpp)
constexpr u32 Display_GetDepthTexture = 0x00537BB8; // (display) -> Target* (gfx/shader3.cpp)
constexpr u32 Draw_InlineArray = 0x007A31CC;        // (mode, count, data, VertexFormat*) (gfx/pool_draw.cpp)
// The light (LightVolumes_Draw's layout, obj/obj16_draw.cpp).
constexpr u32 kLtTexture = 96;    // u32: +56 is the light's texture (Target-like)
constexpr u32 kLtColour = 100;    // u8[4] rgba
constexpr u32 kLtNear = 112, kLtFar = 116;  // f32: the lit depth range (both ~0: the other shader)
// Statics.
constexpr u32 kBasis = 0x0116BE80;       // f32[4][4]: times the inverted view (Matrix_MulAffine order)
constexpr u32 kLightBias = 0x0116BEC0;   // f32[4][4]: the light view-projection's right factor
constexpr u32 kVertexFormat = 0x0116BE00;
constexpr u32 kLightShaders = 0x00F88968;  // u32: +12 -> two program pairs (+0 normal, +4 zero depth range)
constexpr u32 kDisplay = 0x010F0E20;
// The function-local constants: f32 1/255, 1e-5, 1, -, f64 1, f32 0.001, -, f64 0.001.
constexpr u32 kConstsStencil = 0x005735D8, kConstsOutside = 0x00574348;
constexpr u32 kC255th = 0, kCEps = 4, kCOne = 8, kCOneD = 16, kCMin = 24, kCMinD = 32;
// RSX state values.
constexpr u32 kFrontFace = 0x00041834;   // NV4097_SET_FRONT_FACE
constexpr u32 kTexAddress1 = 0x00041A28; // TEXTURE_ADDRESS for unit 1
constexpr u32 kStencilOp = 0x000C033C, kStencilMask = 0x0004032C, kStencilFunc = 0x000C0330;
constexpr u32 kCW = 0x0900, kCCW = 0x0901;
constexpr u32 kKeep = 0x1E00, kReplace = 0x1E01;
constexpr u32 kLess = 0x0201, kEqual = 0x0202, kNotEqual = 0x0205, kAlways = 0x0207;
constexpr u32 kTexAddr0 = 0x00030303, kTexAddr1 = 0x00030404;
}  // namespace lightvol

// GeomBatches_Draw_00575D18: the static geometry batches (gfx22.h
// batch:: layout, GeomBatches_Build's) drawn instance by instance.
namespace batchdraw {
constexpr u32 View_GetCurrent = 0x004FDBA8;
constexpr u32 FProgram_SetConstant = 0x0052F270;
constexpr u32 Draw_InvalidateState = 0x0051279C;
constexpr u32 kBatches = 0x0116BF00, kBatchSize = 272, kBatchCount = 0x0116E100;
// The batch.
constexpr u32 kFadeFar = 0, kFadeScale = 4, kRadiusSq = 8, kFadeNear = 12;
constexpr u32 kInstCount = 68, kInsts = 72;   // 80-byte instances: rows +0/+16/+32, position +48, sphere +64
constexpr u32 kDepthFunc = 76, kDepthWrite = 80, kCull = 84, kOpaque = 88, kSrcFactor = 92, kDstFactor = 96,
              kBlendEq = 100, kAlphaRef = 104, kColourB = 108, kColourA = 112;
constexpr u32 kVConst6 = 128, kVConst6W = 140, kVConst7 = 144, kVConst8 = 160;
constexpr u32 kTexture = 176, kMinFilter = 184, kMagFilter = 188, kWrapS = 192, kWrapT = 196;
constexpr u32 kVProgram = 200, kFProgram = 204, kLodCount = 208, kLodDistSq = 224;
constexpr u32 kLodBuffers = 232, kLodIndexBuffers = 248, kLodIndexCounts = 256, kVertexFormat = 264, kMode = 268;
constexpr u32 kInstSize = 80;
// Globals.
constexpr u32 kColourTable = 0x010DD9A0;  // vertex constant 32 = +16 * B + 32 * A
constexpr u32 kFConstTable = 0x010DD980;  // fragment constant 32 = +16 * A
constexpr u32 kTime = 0x010DD5B0;         // f32 copied to the batch's +140
// View: +880 f32[16] (to the instance matrices), +272 planes (+96: 8 SoA), +816 camera rows (+48 position).
constexpr u32 kViewMatrix = 880, kViewPlanes = 272, kViewCamera = 816;
constexpr u32 kConsts = 0x00575D00;       // f32 1, 0, f64 1, f32 1/255
// RSX.
constexpr u32 kAlphaFunc = 0x00080308, kAlphaTestEnable = 0x00040304, kFrontFace = 0x00041834;
constexpr u32 kTexAddress0 = 0x00041A08, kGequal = 0x0206;
}  // namespace batchdraw

// Render_DrawFrame_00518E78: the view's frame (Call_0051A6A0 without the
// owner's u8 +48), the layer draws between GPU markers, with the deferred
// command segments (gfx22.h defer::) recorded or their bodies run inline.
namespace frame {
constexpr u32 GetGlobal = 0x00533778;            // () -> u32 (name?)
constexpr u32 Gcm_Marker = 0x005E2650;           // (context, u32 id, u32 value) (f_005E2650; name?)
constexpr u32 CmdBuf_ReserveCall = 0x00537DD4;   // (u32 out[3]) (name?)
constexpr u32 Draw_InvalidateState = 0x0051279C;
constexpr u32 ShapeList_Draw = 0x00518B64;       // (list, bool) (name?)
constexpr u32 Fn_576BB4 = 0x00576BB4;            // -> GeomBatches_Draw (obj/obj13_misc.cpp)
constexpr u32 DrawQueue_Begin = 0x00518610;      // () (name?)
constexpr u32 SortEntries = 0x0056A81C;          // (entries, count, size)
constexpr u32 Display_Prepare = 0x005358A8;      // (u32) (name?)
constexpr u32 Display_BindScreenTargets = 0x00535C84;
constexpr u32 Draw_SetViewport = 0x00532450;     // (surface, viewport rect, scissor rect) (gfx11.cpp)
constexpr u32 Draw_Pass = 0x005159E8;            // (Stream*, const Pass*) (gfx15.cpp)
constexpr u32 Call_00548B88 = 0x00548B88;
constexpr u32 DeferredLists_Flush = 0x00517880;  // (gfx22.cpp)
constexpr u32 Render_Pass575C34 = 0x00575C34;
constexpr u32 Draw_Lists = 0x00517488;           // (u32 list) -> u32
constexpr u32 GpuTimer_Update = 0x0053CA20;      // (timer)
constexpr u32 Gcm_SetReferenceCommand = 0x005E24F8;  // (context, u32)
constexpr u32 RenderState_Reset = 0x00517FB8;
constexpr u32 ShadowPass_CopyDepth = 0x005181E0;
constexpr u32 Draw_SetTarget = 0x005373A8;       // (Target*, u32)
constexpr u32 Bloom_Run = 0x0053DF38;            // (const BloomArgs*)
constexpr u32 Draw_Clear = 0x00531F80;           // (mask, colour, -, stencil, ...; f1 depth)
// Globals.
constexpr u32 kGcmContext = 0x010F10A0;   // the render state; +156 the state stamp
constexpr u32 kPending = 0x00F66F94;      // u8: record the segments (defer::kPending)
constexpr u32 kLayers = 0x010DDA90;       // RenderLayer*[] (render.h layers + 4); [0] is stream 0
constexpr u32 kDisplay = 0x00F66FBC;
constexpr u32 kShapes = 0x010DF8B0;       // the stencil shadow pool (pool.h)
constexpr u32 kFlagSegOverlayEarly = 0x010DDAD4;  // u8: the overlay segment before (set) or after the view setup
constexpr u32 kFlagLayerOrder = 0x010DDAD5;       // u8: the layer 3..6 order
constexpr u32 kFlagTimer = 0x010DDAD6;            // u8: run the GPU timer pass
constexpr u32 kCurrentSurface = 0x010DDA74;       // +672 the current View
constexpr u32 kStateCacheA = 0x010DD598, kStateCacheB = 0x010DD5A0;  // u64, -1 = stale
constexpr u32 kPassStream = 0x010DDBB4;   // Stream* for Draw_Pass (render.h state_cache)
constexpr u32 kPassMatrix = 0x01164C10;   // f32[16]
constexpr u32 kNoBloomTimer = 0x010F0A41; // u8: the lists 3 segment instead of the bloom / timer block
constexpr u32 kTimer = 0x010DDB88;        // the GPU timer block; +32 u8
constexpr u32 kTimerOn = 0x010DDBB0;      // u8
constexpr u32 kBloomTex = 0x011640D0, kBloomTarget = 0x011640D4;
constexpr u32 kConsts = 0x00518E68;       // f32 0 (the polygon offset), 65536, 2^24, 1
constexpr u32 kSorted = 0x010EF63C, kSortedCount = 0x010F0A3C, kSortedWait = 0x010DDC00, kSortedBase = 0x00F66FD8;
constexpr u32 kOverlayOn = 0x00F66FD4, kOverlayObj = 0x00F66FD0;
constexpr u32 kSysTimerUsleep = 141, kSleepUs = 300;
// The segment jump words (defer::kSegments).
constexpr u32 kSegStream0 = 0x010DD480, kSegSorted = 0x010DD4A0, kSegLists1 = 0x010DD4B0, kSegLists2 = 0x010DD4C0,
              kSegLists3 = 0x010DD4D0, kSegOverlay = 0x010DD4E0, kSegPass = 0x010DD4F0;
constexpr u32 kPolygonOffsetFill = 0x00040A68;
constexpr u32 kRefTimerStart = 2042, kRefTimerEnd = 2043;
constexpr u32 kFinishA = 0xFFFFFFC3u, kFinishB = 60;  // the owner's vtable +28 arguments
// The owner: +256 flags (0x10 the geometry batches), +512 surface, +656 -> +8 the scene (vfunc +12; +32 / +48 to Bloom_Run).
constexpr u32 kOwnerFlags = 256, kOwnerSurface = 512, kOwnerScene = 656;
}  // namespace frame

}  // namespace gfx24
}  // namespace sly4::render
