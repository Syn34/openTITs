// The renderer range's next recompiled functions (gfx17*.cpp).
#pragma once

#include "gfx16.h"

namespace sly4::render {
namespace gfx17 {

namespace fn {
// Still recompiled or decompiled elsewhere (called from gfx17*.cpp).
constexpr u32 Tex_FindLoaded = 0x0050C398;                // (name) -> Texture* or 0 (gfx11.h)
constexpr u32 Material_AddTexture_004FF008 = 0x004FF008;  // (Material*, Texture*) (gfx12.cpp)
constexpr u32 Material_ParamSlot = 0x0050C018;            // (Material*, u32 index) -> u32* (gfx12.h)
constexpr u32 Debug_RetirePrims = 0x0056AD18;             // () (obj/obj16_small.cpp)
constexpr u32 Debug_Draw_0056AEFC = 0x0056AEFC;           // () the debug prims; leaves r4 that View_EndDraw_00522140 reads (name?)
constexpr u32 Surface_Bind_0053B668 = 0x0053B668;         // (RenderSurface*, bool clear) (gfx13.cpp)
constexpr u32 Draw_SetViewport_00532450 = 0x00532450;     // (surface, viewport rect, scissor rect) (gfx11.cpp)
constexpr u32 Display_BindScreenTargets = 0x00535C84;     // () (gfx8.cpp)
constexpr u32 PtrVector_Construct = 0x005516E0;           // (PtrVector*)
constexpr u32 DrawQueue_Run_00516CD0 = 0x00516CD0;        // (DrawQueue*) (gfx11.cpp)
constexpr u32 Draw_Pass_005159E8 = 0x005159E8;            // (Stream*, Pass*) (gfx15.cpp)
constexpr u32 WorldObj_Construct = 0x00580708;             // (obj, kind) (obj/obj2.cpp)
constexpr u32 Kind13_Construct = 0x005A74A8;               // (obj) (obj/obj5_new.cpp)
constexpr u32 Iff_ReadChunks = 0x00569388;                 // (IffReader*, obj) (obj/obj6_iff.cpp)
constexpr u32 Array_Reserve = 0x00558F3C;                  // (Array*, u32 capacity)
constexpr u32 Iff_ReadU32 = 0x005642AC;                    // (IffReader*, u32* out, name)
constexpr u32 FileReader_ReadVec3Swapped = 0x005699F0;     // (FileReader*, f32 out[3], name)
constexpr u32 SplineArray_Reset = 0x0055F244;              // (SplineArray*) (obj/obj9_spline.cpp)
constexpr u32 SplineArray_Init = 0x0055F2BC;               // (SplineArray*, u32 count, bool closed, bool flag30) (obj/obj9_spline.cpp)
constexpr u32 SplineArray_Finish = 0x0055FB88;             // (SplineArray*) (obj/obj12_spline.cpp)
constexpr u32 printf = 0x007240F4;                         // libc
constexpr u32 Tex_GetDefault = 0x004FEF30;                 // (kind) -> Texture* (gfx11.h)
constexpr u32 FProgram_SetConstant = 0x0052F270;      // (u32 index, const f32 v[4]) (draw3_shader.h)
constexpr u32 UvAnim_Matrices_005D70E0 = 0x005D70E0;  // (Material*, f32 a[16], f32 b[16], f32 out[4]) (gfx16.cpp)
constexpr u32 Obj_GetField4 = 0x00556BF0;             // (p) -> p->+4 (obj/obj13_misc.cpp)
constexpr u32 FreeList_Init = 0x00551A58;             // (list, elem_size, count, heap, bool)
constexpr u32 WorkQueue_Fn56EEF0 = 0x0056EEF0;        // (QueueRef* {queue, handle}, header, records, count, busy) (obj/obj15_job.cpp)
}  // namespace fn

// The SPURS job queue (obj/obj15_job.cpp's kJobMutex / kJobQueue): its
// lwmutex, the queue (vtable +16 acquire a descriptor, +20 submit it).
constexpr u32 job_mutex = 0x0116B580;
constexpr u32 job_queue = 0x0116B600;
constexpr u32 kJobLockTimeout = 100000;  // usec
constexpr u32 kQueueAcquire = 16, kQueueSubmit = 20;

// The deferred destruction state (DeferredDestroy's, obj/obj3.cpp): +0
// a header {+0 the ApeTFreeList<AglDeferredDestructionObj> (made on first
// use), +4 its capacity}, u32 +4 the frame counter, u8 +8 deferring. Each
// entry (12 bytes) is {obj, due frame, destroy OPD}; the list's nodes
// carry it 16 bytes in, +0 the next node.
constexpr u32 defer_state = 0x010DDC88;
constexpr u32 vt_defer_list = 0x00B23EA0;   // ApeTFreeList<AglDeferredDestructionObj>
constexpr u32 str_defer_list = 0x00AF145A;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglDeferredDestructionObj]"
constexpr u32 kDeferEntrySize = 12, kFreeListSize = 60, kFreeListNode = 16;
constexpr u32 kFreeListFree = 16;  // ApeTFreeList vtable +16: (list, element)

// The PostProc_ThermalObject shader (vtable 0x00B24190): its params'
// default (f32 0) and the vec4 whose z / w it pads constants 2 / 3 with.
constexpr u32 kThermalZero = 0x0054CE78;
constexpr u32 vec4_164BC0 = 0x01164BC0;  // gfx16::g::vconst_164BC0's first row
constexpr u32 kMatParams = 124;          // Material +124: f32 params or 0

// SlyTerrainSnow_LoadParams: f32 {0.0001, 1.0, 0.3, 0.5, 30, 15}, the
// defaults of params 5..9 / 11..14, and the colours' defaults.
constexpr u32 kSnowConsts = 0x005CF598;
constexpr u32 kSnowColourA = 0xFFFFFFFFu, kSnowColourB = 0xFF661919u;

// View_EndDraw_00522140: the view's callback lists (16-byte entries
// {OPD, arg, u32 +8 view mode}, u8 +64 count) at View +520 / +588, its
// 3x4 matrix +816 saved to +1200, its end callback OPD +660; the stream
// record kind it appends (the top byte) and the layer flag that skips
// layer 0.
constexpr u32 kViewCallbacksA = 520, kViewCallbacksB = 588, kViewCallbackCount = 64;
constexpr u32 kViewMatrix = 816, kViewSavedMatrix = 1200, kViewEndCallback = 660;
constexpr u32 kViewMode = 48;     // u8 of the current View compared with an entry's +8
constexpr u32 kStreamEndViewCmd = 22;
constexpr u32 skip_layer0 = 0x00F66F94;  // u8 (name?)
constexpr u32 kLayerReset = 8;           // RenderLayer +8: the write cursor of 8-byte records {-1, 0} (name?)
constexpr u32 kLayerEnd = 12;            // RenderLayer vtable +12: () (name?)
constexpr u32 view_counter = 0x010DDA78; // u32 after the current surface (gfx15::g::current_surface): ++ per view drawn (name?)

// Render_DrawLayers_005170B8: the draw queue (gfx16::g::record_list and
// its guard, as DrawQueue_Begin_00518610 builds it), the identity-ish
// matrix its pass state carries (+16) and the frame offset of that state.
constexpr u32 kSysTimerUsleep = 141, kQueueWaitUsec = 300;
constexpr u32 pass_matrix = 0x01164C10;
constexpr u32 kPassStateFrame = 112;  // 224-byte frame, state at +112: entry r1 - 112
constexpr u32 kLayerDraw = 16;        // RenderLayer vtable +16: () draw (name?)
constexpr u32 kOwnerFinish = 28;      // the owner's vtable +28: (owner, u32 -1) (name?)

// Iff_HandleEmitCurve2_005295A0's constants: f32 {1e-5, 1e8, 256}.
constexpr u32 kEmitCurve2Consts = 0x00529578;
constexpr u32 str_alpha_fade_in2 = 0x00B23BF9;   // "alphaFadeIn" (the copy this handler names)
constexpr u32 str_alpha_fade_out2 = 0x00B23C05;  // "alphaFadeOut"
constexpr u32 str_shade_variance = 0x00B23C12;   // "shadeVariance"

// The instance IFF readers (obj/obj18.cpp CameraInstance_ReadChunks'
// pattern): the instance list headers (+0 the free list, +4 its
// capacity, +8 the callback OPD, +12 its argument, +16 the name map).
constexpr u32 emitter_insts = 0x00F8F14C;  // ApeEmitterInstance (obj18::kEmitterInsts)
constexpr u32 spline_insts = 0x00F8F158;   // ApeSplineInstance (obj8::kSplineInstList)
constexpr u32 kHdrCallback = 8, kHdrCallbackArg = 12, kHdrMap = 16;
constexpr u32 kKindEmitter = 10, kKindLight = 9, kKindGeom = 8;
constexpr u32 geom_insts = 0x00F8F144;   // ApeGeomInstance (obj18::kGeomInsts)
constexpr u32 light_insts = 0x00F8F148;  // ApeLightInstance (obj18::kLightInsts)
constexpr u32 vt_geom_instance = 0x00B24858, vt_light_instance = 0x00B248A8;
constexpr u32 vt_geom_inst_list = 0x00B0F008, str_geom_inst_list = 0x00AF4B7E;  // "... [with T = ApeGeomInstance]"
constexpr u32 kGeomInstanceSize = 352;
// A geometry instance's u32 +320: u8 +322 bit 0x20 cleared after reading;
// only those whose bits 9..12 are then 3 enter the name map (name?).
constexpr u32 kGeomFlags = 320, kGeomFlagByte = 322, kGeomMapped = 3;
// A new light instance's defaults: +352 f32[3] from 0x01164B30, +368 3,
// +372 1, +376 / +388 1.0, u8 +380 127, u16 +382 0xFFFF, +384 0 (names?).
constexpr u32 light_default_vec = 0x01164B30;
constexpr u32 kIrFlags = 12;          // IffReader u8: 0x04 = reuse an existing definition (obj18.cpp)
constexpr u32 kVfnAddChild = 20;      // the parent's vtable +20: (parent, child, s32 tag)
constexpr u32 kObjInMap = 0x400;      // +8: entered into the name map

// SlyHeightFog_LoadParams: f32 {1.0, pi / 180, 0, 0.5, 5.0, 0.2}.
constexpr u32 kHeightFogConsts = 0x005C7DCC;

// Iff_HandleEmitSpline_0052996C: the 32-byte spline shape (vtable
// 0x00B232C0, u16 refs +4, SplineArray +16 {u32 +0 bit 31 closed / low
// 15 bits the point count, +4 64-byte points: +0 position, +16 / +32 the
// two controls}) at AglEmitProperties +136, and its strings.
constexpr u32 vt_emit_spline = 0x00B232C0;
constexpr u32 kEmitSplineSize = 32, kEmitSpline = 136, kSplineArray = 16, kSplinePointSize = 64;
constexpr u32 str_is_loop = 0x00B23BE8, str_num_points = 0x00B23BEF;  // "isLoop", "numPoints"
constexpr u32 str_position = 0x00AF1C08, str_control1 = 0x00AF1C18, str_control2 = 0x00AF1C28;
constexpr u32 vec3_zero_164B40 = 0x01164B40;  // f32[3]: the end points' outer controls (name?)

// Gcm_DumpLocks_0052F91C's strings and the state it prints: the ring
// buffer context (*0x011972EC {begin, end, current}), the linear
// double-buffer context 0x010F0E0C, the command buffer object
// (gfx13::g::command_buffer: +268 / +272 addresses, +276 / +280 offsets),
// the frame flip label pointer and the 21 layer label pointers.
constexpr u32 str_locks_title = 0x00AF2764;   // "=== RSX Potential Locks ===..."
constexpr u32 str_cur_ctx = 0x00AF27A4, str_ring_ctx = 0x00AF27C4, str_begin = 0x00AF27E4, str_end = 0x00AF27F4,
              str_curr = 0x00AF2804, str_linear_ctx = 0x00AF2814, str_linear0 = 0x00AF2708, str_linear1 = 0x00AF2738,
              str_swap_label = 0x00AF2834, str_flip_label = 0x00AF284C, str_layer_labels = 0x00AF287C,
              str_locks_end = 0x00AF2C6C;
constexpr u32 kLayerLabelStrStride = 48, kLayerLabelCount = 21;
constexpr u32 ring_ctx = 0x011972EC;     // CellGcmContextData* (name?)
constexpr u32 linear_ctx = 0x010F0E0C;   // CellGcmContextData (name?)
constexpr u32 command_buffer = 0x010F0E08;
constexpr u32 flip_label = 0x00F671D8;   // volatile u32*: RSX label AGL_PS3_GCM_LABEL_INDEX_FRAME_FLIP (name?)
constexpr u32 layer_labels = 0x010F0FA0; // volatile u32*[21]: the layers' RSX labels (name?)

// SlyCharacter2_LoadParams: f32 {1.0, 9.75, 0, pi / 180}; its fixed
// params' words (-0.1, 0.1, 2.0, 0.5).
constexpr u32 kSlyCharacter2Consts = 0x005BF5F8;
constexpr u32 kMinusTenth = 0xBDCCCCCDu, kTenth = 0x3DCCCCCDu, kTwo = 0x40000000u, kHalf = 0x3F000000u;

// UvAnim_SetConstants_005D7438's frame (336 bytes, as the original's, so
// the rows' unwritten w words read the same stack bytes): out +112, the
// 3x3 rows +128 (16-byte stride, w left as found), a +176, b +240.
constexpr u32 kUvAnimFrame = 336;
constexpr u32 kUvAnimOut = 112, kUvAnimRows = 128, kUvAnimA = 176, kUvAnimB = 240;

}  // namespace gfx17
}  // namespace sly4::render
