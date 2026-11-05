// The renderer range's next recompiled functions (gfx12*.cpp): the
// callees gfx11*.cpp left (the shadow projection's frustum corners and
// bounds, the trail's screen-space fade, the frustum planes, ...) and the
// next smallest functions.
#pragma once

#include <cmath>

#include "gfx11.h"

namespace sly4::render {
namespace gfx12 {

namespace fn {
// Still recompiled (called from gfx12*.cpp).
constexpr u32 Vec4_Transform = 0x005539E4;  // (f32 v[4] in / out, const Matrix*) (view/camera.h)
constexpr u32 TopoRecord_Set = 0x0079C668;  // (TopoRecordRef*, TopoRecord*) -> ref (obj/obj7.cpp)
constexpr u32 MemStack_Alloc = 0x0055260C;  // (ApeMemStack*, u32 size) -> pointer
constexpr u32 DrawQueue_Submit_0051669C = 0x0051669C;  // (Batch*) the batch's records to a job (CmdBuf_ReserveCall, WorkQueue_Submit) (name?)
constexpr u32 RenderTarget_Setup_005203A8 = 0x005203A8;  // (RenderTarget*) after RenderTarget_Init_00521110 binds the surface (name?)
constexpr u32 String_Copy = 0x00553684;     // (dst, src, size): always terminates
constexpr u32 memset = 0x00726E54;
constexpr u32 memmove = 0x0071C3F0;          // (dst, src, n)
constexpr u32 Particle_PositionAt_00505D50 = 0x00505D50;  // (Point*, const f32 p[8], f32 out[3]; f1 t, f2 s) (gfx10_math.cpp)
constexpr u32 Particle_Integrate_00505C30 = 0x00505C30;   // (Point*, Particle*, const f32 c[3], const f32 n[4]; f1 s) (gfx10_math.cpp)
constexpr u32 Topology_AddDraw = 0x00508B94;     // (topo, mesh, material)
constexpr u32 SubMesh_Construct5386DC = 0x005386DC;  // (SubMesh*)
constexpr u32 Iff_ReadChunks = 0x00569388;       // (IffReader*, obj)
constexpr u32 FreeList_Init = 0x00551A58;        // (list, elem_size, heap, 0, 0)
constexpr u32 PtrVector_Construct = 0x005516E0;  // (PtrVector*)
constexpr u32 Display_Prepare_005358A8 = 0x005358A8;   // (u32 3) before Display_BindScreenTargets (name?)
constexpr u32 Display_BindScreenTargets = 0x00535C84;  // ()
constexpr u32 Surface_Bind_0053B668 = 0x0053B668;      // (RenderSurface*, 1) (name?)
constexpr u32 Obj_GetField4 = 0x00556BF0;              // (p) -> p->+4
constexpr u32 View_Update = 0x005115F0;                // (view, 0)
constexpr u32 RenderTarget_Draw_005109B0 = 0x005109B0; // (RenderTarget*) after View_Update (name?)
constexpr u32 Material_ParamSlot = 0x0050C018;         // (Material*, u32 index) -> u32* (shader5.h)
constexpr u32 Mem_GetDefaultStack = 0x005525FC;        // () -> ApeMemStack*
constexpr u32 Iff_ChunkSize = 0x005637D4;              // (IffReader*) -> u32
constexpr u32 FileReader_Read = 0x00563768;            // (FileReader*, buf, size) -> u32
constexpr u32 Mesh_SetFlag_004FCD78 = 0x004FCD78;      // (Mesh*, 3) after the skin weights (name?)
constexpr u32 Draw_BindScreenTargets = 0x00513038;     // () (draw_state.h)
constexpr u32 Gcm_SetVertexProgramAt = 0x005E3C00;     // (context, ucode offset) libgcm? (effect_draw.h)
constexpr u32 Buffer_Alloc52F198 = 0x0052F198;         // (Buffer*, bytes, align, where, flags) -> RSX offset
constexpr u32 cellGcmSetTextureFilter = 0x005E3E88;   // (context, index, bias, min, mag, conv) (draw_state.h)
constexpr u32 cellGcmSetTextureControl = 0x005E3F18;  // (context, index, enable, minlod, maxlod, aniso) (draw_state.h)
constexpr u32 cellGcmSetTextureAddress = 0x005E3EC8;  // (context, index, wraps, wrapt, wrapr, unsigned remap, zfunc, gamma) libgcm (name?)
constexpr u32 cellGcmSetTexture = 0x005E4514;         // (context, index, const CellGcmTexture*) (draw_state.h)
constexpr u32 Bloom_Chain = 0x0053D1B0;               // (const BloomArgs*) (fx_bloom.cpp)
constexpr u32 Shader_Construct = 0x0050AFDC;          // (Shader*)
constexpr u32 Shader_Load = 0x0053A9D8;               // (Shader*, vertex program name, fragment program name or 0, vparams, fparams)
constexpr u32 PtrVector_Reserve = 0x00551718;         // (PtrVector*, u32 capacity)
constexpr u32 Tex_GetDefault = 0x004FEF30;            // (kind) -> Texture*
constexpr u32 Shader_IsReady_005AF298 = 0x005AF298;   // (0x01196B20, Material*, Desc*) -> 0 skips the LoadParams (name?)
constexpr u32 cellGcmSetTransferScaleMode = 0x005E4B24;     // (context, mode, surface) libgcm (name?)
constexpr u32 cellGcmSetTransferScaleSurface = 0x005E4C2C;  // (context, const CellGcmTransferScale*, const CellGcmTransferSurface*) libgcm (name?)
constexpr u32 Emitter_Advance_00500908 = 0x00500908;  // (Emitter*; f1 dt) (name?)
constexpr u32 Emitter_Emit_0079D280 = 0x0079D280;     // (Emitter*; f1 time offset) one particle (name?)
constexpr u32 Obj_DestructBase_004FD198 = 0x004FD198; // (obj) the base part of the destructors (name?)
constexpr u32 Mem_DefaultAllocator = 0x00550EE8;      // () -> Allocator*
constexpr u32 Array_Init = 0x00558F38;                // (Array*)
constexpr u32 Iff_RegisterChunk = 0x0055902C;         // (u32 tag, handler OPD, 0, 0, 0)
constexpr u32 RegisterKLBM = 0x00548CD4;              // ()
constexpr u32 SmallAlloc = 0x0055238C;                // (size) -> pointer
constexpr u32 Emitter_Construct_00500798 = 0x00500798;  // (Emitter*, AglEmitProperties*) (gfx10.cpp)
constexpr u32 Debug_FreePrimPools = 0x0056AC78;     // () -> bool
constexpr u32 Bank_FreeAll = 0x00588C3C;            // () -> bool
constexpr u32 Render_Shutdown_0050FB5C = 0x0050FB5C;  // () -> bool (name?)
constexpr u32 Platform_Shutdown = 0x0055E7C0;       // () -> bool
constexpr u32 Spurs_Shutdown = 0x0056DCEC;          // () -> bool
// Imports.
constexpr u32 cellSpursEventFlagWait = 0x00ABDFE4;  // (CellSpursEventFlag*, u16* bits, mode)
}  // namespace fn

namespace g {
constexpr u32 frustum_pad = 0x01164BC0;  // f32[4]: the padding plane Frustum_FromMatrix repeats twice (name?)
constexpr u32 frame_stack = 0x010DD718;  // ApeMemStack*: layers3::g::block_dd600 +280, the per-frame allocator (name?)
constexpr u32 obj_serial = 0x01164CD0;   // u32: bumped by every object construction (obj/obj2.h kObjSerial)
constexpr u32 obj_init_bytes = 0x01164CA4;  // u8[4] the 2 attribute records' +2..+5 start from (name?)
constexpr u32 current_target = 0x00F66FC4;  // RenderTarget*: cleared when re-initialised (name?)
constexpr u32 str_render_target_surface = 0x00AF1B94;  // "RenderTarget_Surface"
constexpr u32 mesh_formats_ready = 0x00F67144;  // u8: mesh_formats built (name?)
constexpr u32 mesh_formats = 0x010F0CC0;   // 2 x 128-byte vertex formats {12-byte attributes, +120 u16 stride, +124 u16 flags, +126 u16 register mask} (name?)
constexpr u32 mesh_format_tpl0 = 0x00F670E4;  // the attribute templates (up to the one with +4 == 0) (name?)
constexpr u32 mesh_format_tpl1 = 0x00F67114;
constexpr u32 submesh_list = 0x00F8F120;   // ApeTListAllocator<AglSubMesh>: {+0 ApeTFreeList*, +4 heap}* (name?)
constexpr u32 vt_submesh_list = 0x00B23DE0;   // ApeTFreeList<AglSubMesh>
constexpr u32 str_submesh_list = 0x00AF127D;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglSubMesh]"
constexpr u32 light_src = 0x011950B0;      // {Light* +0, f32 +4}[]: the lights LightList_Snapshot_005ACDE4 copies (name?)
constexpr u32 light_counts = 0x01195110;   // u16[3] (name?)
constexpr u32 light_copies = 0x01195120;   // 144-byte copies (name?)
constexpr u32 light_counts_copy = 0x011957E0;  // u16[3] (name?)
constexpr u32 draw_records = 0x011DE300;   // the draw-record list (obj/obj12.h kTrList) (name?)
constexpr u32 draw_records_ready = 0x011DE480;  // u8: its guard (obj/obj12.h kTrListGuard)
constexpr u32 draw_records_guard2 = 0x011DE4B0; // u8: a function-local static's guard, only set (name?)
constexpr u32 screen = 0x010F0E20;          // the screen (+160 size, +164 eye separation, +184 u32) (name?)
constexpr u32 frame_cache_dd598 = 0x010DD598;  // u64[2] set to -1 when the display rebinds (name?)
constexpr u32 render_targets = 0x00F8F11C;  // ApeTListAllocator<AglRenderTarget>: {+0 ApeTFreeList*, +4 heap}* (name?)
constexpr u32 vt_render_target_list = 0x00B23DC0;  // ApeTFreeList<AglRenderTarget>
constexpr u32 str_render_target_list = 0x00AF122C; // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglRenderTarget]"
constexpr u32 reset_texture = 0x011640D0;  // Texture*: bound to unit 0 by RenderState_Reset_00517FB8 (name?)
constexpr u32 bloom_used = 0x00F67948;     // u8: set once the bloom chain ran (name?)
constexpr u32 sho_texture_warp_type = 0x01196B20;  // the object Shader_IsReady_005AF298 checks (name?)
constexpr u32 view_matrix = 0x010DD840;    // f32[4][4] world -> view (layers3::g::view_matrix)
constexpr u32 fog_start = 0x010DD9B0;      // f32: the distance SelectShader_005D8C6C compares (name?)
constexpr u32 topology_list = 0x00F8F124;  // ApeTListAllocator<AglTopology> holder {+0 list, +4 capacity, +8} (name?)
constexpr u32 topology_map = 0x010DDC80;   // Array* (gfx11::g::topology_map)
constexpr u32 topology_cmp = 0x00F3BB88;   // OPD: the topology map's compare (name?)
constexpr u32 ring_list = 0x011DE4B4;      // ApeTFreeList*: ApeTMemClassFL<ring, 24>'s allocator (name?)
constexpr u32 vt_ring_list = 0x00B0EF68;   // its ApeTFreeList vtable (name?)
constexpr u32 str_ring_list = 0x00AF1066;  // "static ApeTFreeList<T> &ApeTMemClassFL<T, size>::GetAllocator() [with T = Ape..."
constexpr u32 static_targets = 0x011642C0; // 5 x 52-byte entries {part +0, target +40, u8 +48 not owned} (name?)
constexpr u32 tile_slots = 0x00F67244;     // u16: bit i set while tile slot i is taken (surface_alloc.h)
constexpr u32 shutdown_holder = 0x011DE094;  // a function-local static {+20 / +24 u32, +28 Obj*, u8 +32 flags, +92 400 bytes} (name?)
constexpr u32 shutdown_holder_ready = 0x011DE280;  // u8: its guard (name?)
constexpr u32 shutdown_holder_ptr = 0x011972E8;    // u32: = shutdown_holder once built (name?)
constexpr u32 screen_bias = 0x01164BB0;  // f32[4]: added to the projected point before the 0.5 scale (gfx11::g::vec_bb0) (name?)
}  // namespace g

// Vtables RenderTarget_Init_00521110 writes.
constexpr u32 vt_obj_base0 = 0x00B242B8;   // written first (obj/obj2.h kVtblObjBase0)
constexpr u32 vt_obj_base = 0x00B236C0;    // the common base both objects get
constexpr u32 vt_render_target = 0x00B240D0;  // AglRenderTarget (game/load.h kRenderTargetVtbl)

constexpr u32 kTopoRecordDefault = 500;  // u16 config +500: the number of entry slots a new TopoRecord gets (0: 1) (name?)

}  // namespace gfx12
}  // namespace sly4::render
