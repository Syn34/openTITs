// The renderer range's next recompiled functions (gfx16*.cpp): the frame
// start, the debug shape list, ... and the next smallest functions.
#pragma once

#include <cmath>

#include "gfx15.h"

namespace sly4::render {
namespace gfx16 {

namespace fn {
// Still recompiled or decompiled elsewhere (called from gfx16*.cpp).
constexpr u32 Call_0053BBE0 = 0x0053BBE0;                    // () (name?)
constexpr u32 FrameBuffer_Begin = 0x0050FF64;                // (FrameBuffer*) (game/update.h)
constexpr u32 StreamBuffers_Flip_0050FE10 = 0x0050FE10;      // () (gfx10.cpp)
constexpr u32 PtrVector_Construct = 0x005516E0;              // (PtrVector*)
constexpr u32 memset = 0x00726E54;
constexpr u32 Vec3_RotateTo = 0x005533EC;                    // (f32 out[3], const f32 v[3], const f32 m[3][4]) (shader4.cpp)
constexpr u32 Skin_TransformPositions_00518748 = 0x00518748; // (SkinMesh*, pose) (gfx15.cpp)
constexpr u32 Lines_AddCircle_00518A20 = 0x00518A20;         // (LineBuf*, Batch*, const f32 c[3]; f1 r) (gfx10_math.cpp)
constexpr u32 Matrix_ScaleDiag = 0x00553BC4;                 // (f32 m[4][4], const f32 s[3]) (obj/obj14_small.cpp)
constexpr u32 Call_00548508 = 0x00548508;                    // (surface) (gfx7_misc.cpp)
constexpr u32 Sync_00548550 = 0x00548550;                    // (surface, x) (gfx8.cpp)
constexpr u32 Bounds_Draw_005ABB58 = 0x005ABB58;             // (const Box*, const f32 m[4][4]) (gfx16.cpp)
constexpr u32 PtrVector_Reserve = 0x00551718;                // (PtrVector*, u32 capacity)
constexpr u32 Shader_Construct = 0x0050AFDC;                 // (Shader*)
constexpr u32 Iff_ReadU8 = 0x005643C4;                       // (IffReader*, u8* out, name)
constexpr u32 FileReader_ReadF32Swapped = 0x005698F4;        // (FileReader*, f32* out, name)
constexpr u32 FreeList_Init = 0x00551A58;                    // (list, elem_size, count, heap, bool)
constexpr u32 FogRanges_Set_0050B6D8 = 0x0050B6D8;           // (AglMaterialAnim*, const AnimRecord*) the record into track u8 +33 (gfx14.cpp)
constexpr u32 TopoShared_Release = 0x005084CC;               // (TopoShared*) (obj/obj7.cpp)
constexpr u32 TopoRecord_Set = 0x0079C668;                   // (record, value) (obj/obj7.cpp)
constexpr u32 TopoRecordRef_Release_0079C47C = 0x0079C47C;   // (record)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;                     // (p, pool)
constexpr u32 ApeRoot_Destruct = 0x00558CE8;                 // (ApeRoot*)
constexpr u32 _cellSpursTasksetAttributeInitialize = 0x00ABDF64;  // import
constexpr u32 cellSpursTasksetAttributeSetName = 0x00ABE064;       // import
constexpr u32 cellSpursCreateTasksetWithAttribute = 0x00ABE1E4;    // import
constexpr u32 _cellSpursEventFlagInitialize = 0x00ABE044;          // import (system/subsystems.h)
constexpr u32 cellSpursEventFlagAttachLv2EventQueue = 0x00ABE0C4;  // import (system/subsystems.h)
constexpr u32 Buffer_Init_005DF960 = 0x005DF960;            // (CellGcmContextData*, buffer, size, callback OPD) (libgcm-side; gfx10.cpp)
constexpr u32 cellGcmSetVertexProgramConstants = 0x005E5CB0; // (context, first, count, f32*) (draw_state.h)
constexpr u32 cellGcmSetDrawIndexArray = 0x005E4F64;         // (context, mode, count, type, location, offset) (pool.h)
constexpr u32 cellGcmSetDrawArrays = 0x005E4D98;             // (context, mode, first, count) (pool.h)
constexpr u32 MemStack_Alloc = 0x0055260C;                   // (ApeMemStack*, u32 size) -> pointer
// Emitter_Draw_00507B18's draws (names?).
constexpr u32 EmitterDraw_005077B4 = 0x005077B4;             // (Emitter*, Item*, Slot*): kind 0, batched
constexpr u32 EmitterDraw_005073F8 = 0x005073F8;             // (Emitter*, Slot*): kind 1, batched
constexpr u32 EmitterDraw_00506598 = 0x00506598;             // (Emitter*, record, const f32 view[4][4], x, u16 y)
constexpr u32 EmitterDraw_00505740 = 0x00505740;             // (Emitter*, record, const f32 view_proj[4][4]): kind 1, mode 5
constexpr u32 EmitterDraw_Matrices_00505518 = 0x00505518;    // (Emitter*, record, view_proj) (gfx13.cpp)
constexpr u32 Stencil_SetEqual32_005011A0 = 0x005011A0;      // (Emitter*)
constexpr u32 Sprite_Bind_00501820 = 0x00501820;             // (Emitter*, Item*) (gfx11.cpp)
constexpr u32 True_0050129C = 0x0050129C;                    // (Emitter*, s32 count) -> 1 (gfx7_stubs.cpp)
constexpr u32 EmitterDraw_005043B0 = 0x005043B0;             // (Emitter*, record): kind 0 by record mode 5
constexpr u32 EmitterDraw_00503BC0 = 0x00503BC0;             // mode 4
constexpr u32 EmitterDraw_00503340 = 0x00503340;             // mode 3
constexpr u32 EmitterDraw_00502AC0 = 0x00502AC0;             // mode 2
constexpr u32 EmitterDraw_00502240 = 0x00502240;             // mode 1
constexpr u32 EmitterDraw_005019B8 = 0x005019B8;             // any other mode
constexpr u32 Matrix_FromAxisAngle = 0x0056A410;             // (f32 m[4][4], const f32 axis[3]; f1 angle) (view/matrix.cpp)
constexpr u32 Matrix_ScaleRows = 0x00554394;                 // (f32 rows[3][4], const f32 scale[3]) (view/view2.cpp)
constexpr u32 CmdBuf_ReserveCall_00537DD4 = 0x00537DD4;       // (CallSlot* out {at, after, after's offset}) (gfx10.cpp)
constexpr u32 SmallAlloc = 0x0055238C;                       // (size) -> pointer
constexpr u32 ChunkReader_Remaining = 0x0056424C;            // (ChunkReader*) -> s32 bytes left in the chunk
constexpr u32 FileReader_ReadU32 = 0x00569C74;               // (FileReader*, u32* out, name)
constexpr u32 Stats_ResetCounters_00510554 = 0x00510554;     // () (name?)
constexpr u32 RenderBuffer_FlipStats_0050CC10 = 0x0050CC10;  // () (gfx10.cpp)
constexpr u32 Gcm_Fn5E3F18 = 0x005E3F18;                     // (context, unit, 0, 0, 0, 0) libgcm (name?)
constexpr u32 Display_SelectBuffer = 0x00537C18;             // (mem_stats) -> Surface* (render_state.cpp)
constexpr u32 Gcm_FlipContext_0053BE94 = 0x0053BE94;         // ()
constexpr u32 Surface_Copy_00537118 = 0x00537118;            // (Surface* from, Surface* to) (gfx14.cpp)
constexpr u32 EmitterJob_Run_0056F050 = 0x0056F050;         // (const JobHeader*, records, u32 count, u32** busy) the SPU job (name?)
constexpr u32 Shader_Load = 0x0053A9D8;                      // (Shader*, vertex program name, fragment program name or 0, vparams, fparams)
}  // namespace fn

namespace g {
// The frame clock Render_BeginFrame_0051008C advances (names?).
constexpr u32 frame_time = 0x010DD9E0;    // f32 (layers3::g::block_dd780 +608): += the system state's +4 each frame
constexpr u32 frame_dt = 0x010DD5A8;      // {f32 +0 this frame's step, +4 the last one, +8 their sum, +12 1.0} (layers3::g::block_dd5a8)
constexpr u32 config_frame_flag = 0x010C5A20 + 80;  // u8 of g::config: set every frame (name?)
// The draw-record list (DrawRecordList_Init's, inlined): u8 +0 built, +4
// PtrVector, u16 +10 capacity, +28 the current block {+160 base, +176
// u32 index, +180 / +184 the block's start, +256 atomic end}, +32 u32,
// +128 / +256 atomics; its guard is u8 0x011DE480.
constexpr u32 record_list = 0x011DE300;
constexpr u32 record_list_ready = 0x011DE480;
constexpr u32 kRecordListCapacity = 160;
// The frame arena (shader3::kArena, AglRenderBuffer): u32 +0 the buffer
// index, +8 size, +12 [index] base, +20 cursor, +24 low, +28 high, +256
// u32* atomic end; the second half +260 size, +264 [index] base, +272
// aligned start, +276 end.
constexpr u32 arena = 0x010DD600;
constexpr u32 clear_stencil = 0x010DF8A2;  // u8 (pool.h g::clear_stencil)
constexpr u32 default_frame_buffer = 0x010DDBB4;  // FrameBuffer* (render.h state_cache)
// The 16 query slots (176 bytes, u8 +4 in use; layers3::g::slots) and
// the flag set when one is in use (gfx9.cpp k9QueryActive).
constexpr u32 query_slots = 0x010DFAD0;
constexpr u32 query_active = 0x010E05D0;
// The debug shape list ShapeList_Draw_00518B64 draws: 112-byte entries
// {f32[3][4] +0 matrix, f32[3] +64 position, f32 +80 radius, Shape* +84,
// u8 +93 mode, u32 +96 owner key}, u16 count (cleared every frame by
// View_BeginFrame) (names?).
constexpr u32 shapes = 0x010DDCA0;
constexpr u32 shape_count = 0x010DF8A0;
constexpr u32 state_cache_a = 0x010DD598;  // u64 (render.h)
constexpr u32 state_cache_b = 0x010DD5A0;  // u64 (render.h)
// static ApeTFreeList<AglMaterialAnim>& ApeTMemClassFL<AglMaterialAnim, 256>::GetAllocator()'s
// list (world/hevent2_b.cpp kAnimFreeList).
constexpr u32 anim_free_list = 0x011DE4B8;
// ApeTFreeList<AglTopologyMeshMtlTable>& ApeTListAllocator<T>::GetAllocator()'s
// holder: +0 the list (made on first use), +4 its capacity.
constexpr u32 mesh_mtl_list = 0x00F66EE8;
// EmitProps_AddMesh_004FEBF0's function-local static: the emitter mesh
// shader (hash 0x59D9B0B7) and its guard (names?).
constexpr u32 mesh_shader_ready = 0x010F0BB0;
constexpr u32 mesh_shader = 0x010F0BB8;
// EmitProps_SetMaterial_004FEDD0's: the emitter shader (hash
// gfx15::kEmitShaderHash) and its guard (names?).
constexpr u32 emit_shader_ready = 0x010F0BC0;
constexpr u32 emit_shader = 0x010F0BC8;
constexpr u32 vconst_164BC0 = 0x01164BC0;  // f32[4][4]: vertex constants 4..7 (gfx14::kVec4_164BC0's block; name?)
constexpr u32 emitters_batched = 0x010F0A40;  // u8: emitters without props +136 draw batched (shader4_frame.cpp kClearedByte, cleared every frame; name?)
constexpr u32 axis_z = 0x01164B70;  // f32[3] {0, 0, 1}: the UV matrices' rotation axis (name?)
constexpr u32 stream_buffers = 0x010DDBBC;  // the double-buffered sprite stream (StreamBuffers_Flip_0050FE10's): a VertexFormat at +0 (name?)
constexpr u32 up = 0x01164B60;  // f32[3]: the box spawn's base direction (name?)
constexpr u32 pending_copy = 0x010C5A08;    // {Surface* +0, u8 +4 set}: a copy of the display Render_EndFrame_005105E0 makes (name?)
constexpr u32 object_list_on = 0x00F66FD4;  // u8: the object list (gfx13::g::object_list_f66fd0) is live (name?)
// EmitterJobs_Submit_00528130's: the items (80 bytes; +64 renderer, +68
// / +72 its arguments, +76 slot index), their count (one of the u32s
// View_BeginFrame clears), and the busy counter's pointer (made once).
constexpr u32 emit_items = 0x010E0630;
constexpr u32 emit_item_count = 0x010ECE30;
constexpr u32 busy_ptr = 0x010F0DE0;
constexpr u32 busy_ptr_ready = 0x010F0DD8;
constexpr u32 bounds_cube = 0x00F8895C;      // the bounds cube mesh (Resources_InitBoundsCube's; obj/obj14_small.cpp kBoundsCube)
constexpr u32 bounds_material = 0x00F88960;  // its material (kBoundsMaterial)
}  // namespace g

constexpr u32 kQuerySlotSize = 176, kQuerySlots = 16;
constexpr u32 kShapeSize = 112;
// The line buffer ShapeList_Draw_00518B64 allocates on first use: +4
// 48 KiB of line records, +304 1 KiB (names?).
constexpr u32 kLineBufSize = 0xC000, kLineBufAuxSize = 1024;
// Bounds_Draw_005ABB58: f32 {0.5, 0.01} (the centre scale and the
// scale's padding), the 80-byte draw item and the shader slot it calls.
constexpr u32 kBoundsConsts = 0x005ABB50;
constexpr u32 kBoundsItemSize = 80;
constexpr u32 kShaderDrawItem = 12;   // Shader vtable +12: (Shader*, Item*) (name?)
constexpr u32 kObjDrawBounded = 56;   // the flip entry object's vtable +56: (obj) (name?)
constexpr u32 opd_flip_sync = 0x00F3CA38;  // the second argument FlipEntry_DrawBounds_005ABE5C gives Sync_00548550 (name?)
// The stencil cache in the gcm state block (fx::g::state, 0x010F10A0).
namespace stencil_cache {
constexpr u32 kFunc = 160, kRef = 164, kWriteMask = 168, kMask = 172, kFail = 176, kZFail = 180, kZPass = 184;
}  // namespace stencil_cache
constexpr u32 kColorMask = 0x00040324, kStencilTestEnable = 0x00040328, kStencilWriteMask = 0x0004032C,
              kStencilFunc = 0x000C0330, kStencilOp = 0x000C033C;
constexpr u32 kKeep = 0x1E00, kIncr = 0x1E02, kAlways = 0x0207, kNotEqual = 0x0205;
// The SHO types' Create: an 8-entry name table (12-byte entries {vertex
// program, its UVX twin, fragment program}), the shaders' vtable and
// parameter tables.
struct ShoType {
    u32 table, vtable, vparams, fparams;
};
constexpr ShoType sho_default = {0x00F8AFA0, 0x00B260F8, 0x00F8AF80, 0x00F8AF68};      // "SHO_Default"
constexpr ShoType sho_night_blend = {0x00F8B088, 0x00B26198, 0x00F8B078, 0x00F8B058};  // "SHO_NightBlend"
constexpr u32 kShoTableCount = 8;
constexpr u32 kShaderSize = 44;
// Iff_HandleMaterialAnim_0051CD48: the record's field names ("speed",
// "steps", "limit", "delayMin", "delayMax", "playTime", "uOffset",
// "vOffset", "mode", "channel", "flags"), the AglMaterialAnim (152 bytes,
// at AglMaterial +128) and its free list (world/hevent2_b.cpp).
constexpr u32 str_anim_fields[11] = {0x00B2388B, 0x00B23891, 0x00B23897, 0x00B2389D, 0x00B238A6, 0x00B238AF,
                                     0x00B238B8, 0x00B238C0, 0x00B238C8, 0x00B238CD, 0x00B238D5};
constexpr u32 kMatAnim = 128;
constexpr u32 kAnimSize = 152, kAnimPerBlock = 256;
constexpr u32 kFreeListObjSize = 60;
constexpr u32 vt_free_list = 0x00B0EF88;        // ApeTFreeList<AglMaterialAnim>
constexpr u32 str_anim_free_list = 0x00AF10D9;  // the GetAllocator() signature
constexpr u32 kFreeListAlloc = 12;              // ApeTFreeList vtable +12: () -> element
constexpr u32 kSlyCharacterParams = 0x005BC4E0;  // f32 15, f64 0 +8: SlyCharacter_LoadParams' param 4 default and floor
constexpr u32 kDecalDefaultTexKind = 4;        // Tex_GetDefault kind SlyDecal_LoadParams substitutes (gfx15 default_textures: normal)
constexpr u32 kSlyDecal4Consts = 0x005C52F0;    // f32 {1.0, 0, pi / 180}: SlyDecal4_LoadParams' defaults and degrees to radians
constexpr u32 kSlyFurConsts = 0x005C0F38;  // f32 15, f64 0 +8, f32 1.0 +16: SlyCharacterFur_LoadParams' defaults
constexpr u32 kFurLayersMax = 14;          // its param 19 limit (name?)
// The AglTopology (obj/obj6.h): vtable, draw table {obj, obj}[u16 +52]
// at +48 (+16 the inline one, 4 entries), +88 shared block, +92 record.
constexpr u32 vt_topology = 0x00B23368;
constexpr u32 kTopoDraws = 48, kTopoDrawCount = 52, kTopoShared = 88, kTopoRecord = 92;
constexpr u32 kRelease = 8;           // the draw entries' objects' vtable +8: release
constexpr u32 kFreeListFree = 16;     // ApeTFreeList vtable +16: (list, element)
constexpr u32 vt_mesh_mtl_list = 0x00B23E20;   // ApeTFreeList<AglTopologyMeshMtlTable>
constexpr u32 str_mesh_mtl_list = 0x00AF1316;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglTopologyMeshMtlTable]"
constexpr u32 kMeshMtlTableSize = 320;
// The Edge zlib queue wrapper EdgeDecompressor_Construct sets up (the
// job table fields are gfx14.h's kJobQueue*; names?).
constexpr u32 vt_edge_queue = 0x00B245A4;
constexpr u32 str_edge_taskset = 0x00B24584;  // "edgeDecompressorTaskset"
namespace edge_queue {
constexpr u32 kTagZlib = 0x7A6C6962;  // +4 'zlib'
constexpr u32 kFlags = 20;            // u32: low 10 bits kept, 0xA800 set
constexpr u32 kTasksetObj = 4224;     // CellSpursTaskset (made when there is a SPURS instance)
constexpr u32 kSpurs = 11972, kTaskset = 11976, kAnswer32 = 11980, kCounter = 11984, kAnswer36 = 11988;
constexpr u32 kSlotA = 24, kSlotB = 28, kSlotC = 32, kSlotD = 36;  // its vtable slots 6..9 (names?)
constexpr u32 kLimitA = 0x1000, kLimitB = 0x200;
constexpr u32 kSlotTableSize = 704;   // 16 * 44 bytes at kJobQueueSlots
constexpr u32 kTasksetAttrSize = 512; // CellSpursTasksetAttribute
constexpr u32 kSdkVersion = 0x00330000;
}  // namespace edge_queue
// EmitProps_AddMesh_004FEBF0's scratch command context.
constexpr u32 kScratchCmdSize = 256;
constexpr u32 opd_scratch_callback = 0x00F3BCF0;  // the callback Gcm_EnterSubContext_0052F780 also gives Buffer_Init_005DF960
constexpr u32 kEmitMeshShaderHash = 0x59D9B0B7;
constexpr u32 kEmitMaterialZero = 0x004FEDCC;  // f32 0: EmitProps_SetMaterial_004FEDD0's default params
constexpr u32 kEmitRecordSize = 144;  // Emitter_Draw_00507B18's frame-stack draw record (Emitter_Check_00506050 fills it)
constexpr u32 str_render_target_primary = 0x00AF1B7C;  // "RenderTarget_Primary"
constexpr u32 kUvAnimZero = 0x005D70DC;  // f32 0: UvAnim_Matrices_005D70E0's default for missing params
// The stream buffers: u32 +228 the current buffer, +128 [i] 16-byte
// vertex data records (+4 location / offset word), +160 [i] 20-byte index
// buffers (+4 location / offset word, +16 type).
constexpr u32 kSbIndex = 228, kSbVertexData = 128, kSbIndexBuffers = 160;
constexpr u32 kPrimTriangles = 5;  // CELL_GCM_PRIMITIVE_TRIANGLES
constexpr u32 kVertexF32 = 2;      // CELL_GCM_VERTEX_F
constexpr u32 kGcmJump = 0x20000000;  // a command word that jumps to the offset in its low bits (old-style JUMP)
// Iff_HandleEmitCurve_005291F8: the emitter colour curve (84 bytes;
// obj/obj6.h kEpCurve, at AglEmitProperties +112) and its constants
// f32 {1e-5, 1e8}.
constexpr u32 kEmitCurveConsts = 0x005291EC;
constexpr u32 vt_emit_curve = 0x00B23200;
constexpr u32 kEmitCurveSize = 84;
constexpr u32 str_alpha_fade_in = 0x00B23C20, str_alpha_fade_out = 0x00B23C2C, str_rgba = 0x00AF1C00;
// BoxShape_Spawn_0079FFB0's constants: f32 {1 / 65535, 2, -1, 0}, f64
// {pi, -pi} +16, f32 {2 pi, pi / 2, pi, 4 / pi, -4 / pi^2, 0.225} +32.
constexpr u32 kBoxSpawnConsts = 0x0079FF78;
// SphereShape_Spawn_0079EEF8's: f32 {1 / 65535, 2, -1, 1, 0}, then the
// same spin block as the box's 8 bytes on (f64 {pi, -pi} +24, ...).
constexpr u32 kSphereSpawnConsts = 0x0079EEB8;
constexpr u32 kSphereVolumeSpawnConsts = 0x0079F3A8;  // SphereVolumeShape_Spawn_0079F3E8's: the same layout
constexpr u32 kBoxSurfaceSpawnConsts = 0x0079F960;    // BoxSurfaceShape_Spawn_0079F9A0's: the same layout
constexpr u32 kEmitItemSize = 80, kEmitJobRecordSize = 144, kEmitJobHeaderSize = 160, kEmitJobBatch = 16;
// The line buffer's vtable slots (byte offsets; names?).
namespace lines_vt {
constexpr u32 kBegin = 20, kShape = 24, kSkin = 28, kFlush = 32, kEnd = 36;
}  // namespace lines_vt

}  // namespace gfx16
}  // namespace sly4::render
