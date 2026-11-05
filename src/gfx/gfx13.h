// The renderer range's next recompiled functions (gfx13*.cpp): the
// callees gfx12*.cpp left (the draw queue's job submit, ...) and the
// next smallest functions.
#pragma once

#include <cmath>

#include "gfx12.h"
#include "layers4.h"
#include "pool.h"
#include "render.h"

namespace sly4::render {
namespace gfx13 {

namespace fn {
// Still recompiled (called from gfx13*.cpp).
constexpr u32 CmdBuf_ReserveCall_00537DD4 = 0x00537DD4;  // (CallSlot* out {at, after, offset}) (gfx10.cpp)
constexpr u32 WorkQueue_Submit56E710 = 0x0056E854;       // (a, u32* pb, u32* pc, Obj* d) (obj/obj15_job.cpp)
constexpr u32 ThreadId_005DB070 = 0x005DB070;            // () -> u64 (gfx7_misc.cpp)
constexpr u32 RenderTarget_SetMatrix = 0x005201B0;       // (rt, const f32[16]) (gfx10_math.cpp)
constexpr u32 Matrix_NearlyEqual = 0x005545B4;           // (const f32[16], const f32[16]; f1 eps) -> bool (audio/snd.h)
constexpr u32 cellGcmSetAntiAliasingControl = 0x005E3C38;  // (context, enable, alpha to coverage, alpha to one, sample mask) method 0x1D7C libgcm (name?)
constexpr u32 cellGcmSetAntiAliasingControl_005E2590 = 0x005E2590;  // the same with the space check (callback) libgcm (name?)
constexpr u32 Display_ResolveTo_00534BAC = 0x00534BAC;     // (display, u32 unit) the resolve target as texture (gfx13.cpp Display_BindResolve_00534BAC)
constexpr u32 Display_BindAsTexture_00534DB4 = 0x00534DB4; // (display, u32 unit) (gfx12.cpp)
constexpr u32 PostChain_Construct_0051F850 = 0x0051F850; // (PostChain*) "Bloom", "Desaturation", "Blur", ... (name?)
constexpr u32 Call_005489CC = 0x005489CC;                // () (gfx7_misc.cpp)
constexpr u32 DrawRecordList_Reset_0050F9A4 = 0x0050F9A4; // () (gfx12.cpp)
constexpr u32 TopoShared_Release = 0x005084CC;           // (TopoShared*) (obj/obj7.cpp)
constexpr u32 Call520028_00546F3C = 0x00546F3C;          // () (gfx7_thunks.cpp)
constexpr u32 RenderJobs_Shutdown_0053B558 = 0x0053B558; // () the Edge job context / SPURS job chain shut down (name?)
constexpr u32 RefBlock_Release = 0x00557C80;             // (RefBlock*) (obj/obj14_small.cpp)
constexpr u32 Sphere_FrustumFine = 0x007A6EFC;           // (view +272, f32 c[3], f32 e[3], f32 m[3][4]) -> bool (obj/obj17.cpp)
constexpr u32 EffectSlot_Submit_005AB908 = 0x005AB908;   // (view, EffectSlot*) (name?)
constexpr u32 Stream_NextSlot_004FCE70 = 0x004FCE70;     // (o) -> u8 o+109, post-incremented (gfx13.cpp)
constexpr u32 EmitProps_Construct = 0x004FFFF0;          // (AglEmitProperties*) (obj/obj6.h)
constexpr u32 Thread_Create = 0x0056F5BC;                // (Thread*, entry OPD, ThreadParams*) (platform/platform.h)
constexpr u32 EmitterDraw_BindLayer_00504D40 = 0x00504D40;  // (o, u32 layer, 0, 0): layer (o+4)->+72[layer]'s vertex format etc. (name?)
constexpr u32 EmitterDraw_Particle_00504F68 = 0x00504F68;   // (o, list, u32 layer, u32 index, x) (name?)
constexpr u32 Matrix_ScaleAxes = 0x005540F8;             // (f32 m[4][4], const f32 s[3]): rows' xyz scaled
constexpr u32 EmitterDraw_Layered_005052FC = 0x005052FC; // (o, list, x) (gfx13.cpp)
constexpr u32 DrawRing_Init_0050EB08 = 0x0050EB08;       // (ring, size, count, u8 buffers, u8 b) (gfx12.cpp)
constexpr u32 ChunkReader_Remaining = 0x0056424C;        // (ChunkReader*) -> s32 bytes left in the chunk
}  // namespace fn

namespace g {
constexpr u32 command_buffer = 0x010F0E08;  // the double-buffered command buffer object (gcm.cpp kCommandBuffer)
constexpr u32 str_render_target = 0x00AF1B6C;     // "RenderTarget"
constexpr u32 opd_post_chain_draw = 0x00F3B440;   // OPD of Call_0051A6A0: the render target's +660 callback
constexpr u32 object_list_f66fd0 = 0x00F66FD0;  // {+4 Obj*[u8 +20]}*: released by Render_Shutdown_0050FB5C (name?)
constexpr u32 block_f66fb8 = 0x00F66FB8;  // u32: an 80-byte block freed at shutdown (draw3.h block_f66fb8)
constexpr u32 alloc_ddc88 = 0x010DDC88;   // u32: a block freed at shutdown (name?)
constexpr u32 random_last = 0x01164888;   // u64: the low 16 bits of the last random number (game/update.h kRandomLast)
constexpr u32 random_state = 0x01164890;  // u32: the generator's state (game/update.h kRandomState)
constexpr u32 main_thread = 0x011DE290;        // u64[2]: the main thread id static (game/load.h kThreadIds)
constexpr u32 main_thread_guard = 0x011DE2A0;  // u8: its guard
constexpr u32 effect_index = 0x01195098;      // u32: which effect_pair slot is current (name?)
constexpr u32 effects_on = 0x00F88958;        // u8: the effect slots are culled (wupdate.h kFlag88958) (name?)
constexpr u32 effect_global_seen = 0x01196981; // u8: set when a visible slot has a +0 owner (name?)
constexpr u32 bank_state = 0x01164DEC;  // {u32, Buffer* +4 (+8 base), u32} (obj/obj13_misc.cpp kBankState)
constexpr u32 emit_props_list = 0x00F8F128;      // ApeTListAllocator<AglEmitProperties>: {+0 ApeTFreeList*, +4 heap, +8 OPD on load, +12 its argument} (obj/obj6.h kEmitPropsList)
constexpr u32 vt_emit_props_list = 0x00B23E80;   // ApeTFreeList<AglEmitProperties>
constexpr u32 str_emit_props_list = 0x00AF1407;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglEmitProperties]"
constexpr u32 white = 0x01164CA0;          // u8[4] {255, 255, 255, 255}? (uielem/build6.h kWhite)
constexpr u32 str_screen_warp = 0x00AF1B60;  // "ScreenWarp"
constexpr u32 render_thread = 0x010C5A10;       // RenderThread*: the one RenderThread_Create_005129BC makes (name?)
constexpr u32 opd_render_thread_main = 0x00F3B650;  // OPD: the render thread's entry (name?)
constexpr u32 str_agl_render_thread = 0x00B23730;   // "AglRenderThread"
constexpr u32 str_wait_render_start = 0x00B23740;   // "WaitRenderStart"
constexpr u32 str_wait_render_complete = 0x00B23750;  // "WaitRenderComplete"
constexpr u32 bucket_counts = 0x010D3D30;  // u32[4]: per-layer particle counts EmitterDraw_Layered_005052FC fills (name?)
constexpr u32 particle_layer = 0x010D3D40; // u8[]: per particle index the layer weight (* layer count >> 8) (name?)
constexpr u32 bucket_rows = 0x010D1D30;    // u32[][512]: row (layer + 1) holds that layer's particle indices (name?)
constexpr u32 particle_pos = 0x010CCD30;     // f32[][4]: per particle index the position (xyz) (name?)
constexpr u32 particle_scale = 0x010CA530;   // f32[]: per particle index the size (name?)
constexpr u32 particle_mtx = 0x010D4740;     // f32[][4][4]: per particle index the matrix EmitterDraw_Matrices_00505518 builds (name?)
constexpr u32 particle_colour = 0x010D3F40;  // u8[][4]: per particle index the colour (name?)
constexpr u32 vertex_format_tpl0 = 0x00F66F64;  // the first vertex format's 12-byte attribute templates (up to +4 == 0) (name?)
constexpr u32 vertex_format_tpl1 = 0x00F66F34;  // the second's (name?)
constexpr u32 str_variance = 0x00B23C39;  // "variance"
constexpr u32 str_scale = 0x00AF1C34;     // "Scale"
}  // namespace g

constexpr u32 vt_post_chain = 0x00B236A8;     // the post-effect chain's vtable (name?)
constexpr u32 kSurfaceBindConsts = 0x0053B658;  // f32 {0.5, -0.5, 1, 0}
constexpr u32 kDegToRad = 0x005AF294;  // f32 pi / 180: Shader_IsReady_005AF298 turns d+120 / +124 to radians
constexpr u32 kRandomMix = 0x87654321u;       // the generator's xor
constexpr u32 kEmitterConsts = 0x005008F8;    // f32 {0, 1 / 65535, 1}
constexpr u32 kEmitConsts = 0x0079D260;       // f32 {1 / 65535, 1, 0, 127, 255, 32768, -1}
constexpr u32 kParticleSize = 36;             // a particle pool entry
constexpr u32 kHalf5AC278 = 0x005AC278;       // f32 0.5
constexpr u32 vt_screen_warp = 0x00B23650;    // the ScreenWarp post effect's vtable (name?)
constexpr u32 kParam3Default = 0x005CD0FC;    // f32: LoadParams_005CD100's param 3 before desc version 4
constexpr u32 kEmitterUpdateConsts = 0x00506330;  // f32 {0.01, 0, 1} (as Emitter_Update_007A0C10's 0x007A0C00)
constexpr u32 kSysEventFlagCreate = 82;  // sys_event_flag_create(u32* id, const sys_event_flag_attribute_t*, u64 init)
constexpr u32 kInv255_504F60 = 0x00504F60;    // f32 1 / 255
constexpr u32 kParam100Default = 0x005CBE88;  // f32: LoadParams_005CBE90's d+100 for an upgraded desc
constexpr u32 kClipConsts = 0x005A9B3C;       // f32 {0, 1e-5}: Poly_ClipPlane_005A9B48's side test and parallel limit
constexpr u32 kParams11Default = 0x005B6520;  // f32: LoadParams_005B6528's params 11..20 for older descs
constexpr u32 vt_scale_curve = 0x00B23220;    // the 80-byte scale curve {u16 +4 refs, +8 count, +12 variance, +16 f32[] keys} (name?)
constexpr u32 kUnitRange = 0x00529D74;        // f32 {1, 0}: the variance's clamp
constexpr u32 kRtSetupConsts = 0x00520394;    // f32 {1, 0.1, 1 / 255, 1e-5}

constexpr u32 kDrawCopySize = 352;  // a DrawQueue copy's size (the stride of a single-record batch)

}  // namespace gfx13
}  // namespace sly4::render
