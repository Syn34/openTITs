// The renderer range's next recompiled functions (gfx15*.cpp): the
// callees gfx14*.cpp left (the effect slot texture pick, the emitter
// check, ...) and the next smallest functions.
#pragma once

#include <cmath>

#include "gfx14.h"

namespace sly4::render {
namespace gfx15 {

namespace fn {
// Still recompiled or decompiled elsewhere (called from gfx15*.cpp).
constexpr u32 Vec_TransformByMatrix_553A2C = 0x00553A2C;  // (f32 out[4], const f32 in[3], const Matrix*) (view/vector.cpp)
constexpr u32 Particle_CollidePlanes_00505E90 = 0x00505E90;  // (Emitter*) (gfx12.cpp)
constexpr u32 Particle_LightColour_00500D40 = 0x00500D40;    // (Emitter*, u8 colour[4], a, b) (gfx14.cpp)
constexpr u32 EmitProps_AddMesh_004FEBF0 = 0x004FEBF0;      // (AglEmitProperties*, Mesh*) (name?)
constexpr u32 EmitProps_SetMaterial_004FEDD0 = 0x004FEDD0;  // (AglEmitProperties*, AglMaterial*): +72, the material's refs (name?)
constexpr u32 MeshMap_Find = 0x0079C8C4;                    // (map, const u32* key) -> value slot (obj/obj16_map.cpp)
constexpr u32 Obj_GetField4 = 0x00556BF0;                   // (p) -> p->+4
constexpr u32 Shader_FindByHash = 0x004FE990;               // (hash) -> Shader* (shadergroup.h)
constexpr u32 Material_Alloc = 0x004FF2E8;                  // () -> AglMaterial* (shadergroup.h)
constexpr u32 Mem_Alloc = 0x0054ED38;                       // (size, heap, align)
constexpr u32 FreeList_Init = 0x00551A58;                   // (list, elem_size, count, heap, bool) (shadergroup.h)
// Draw_Pass_005159E8's command handlers (they read their own record).
constexpr u32 Stream_ReadTwo_00514CC8 = 0x00514CC8;        // (Stream*, u32* a, u32* b) (gfx8.cpp)
constexpr u32 Stream_SetDepthBias_00514D68 = 0x00514D68;   // (Stream*) (gfx15.cpp)
constexpr u32 Stream_SetLineWidth_00515018 = 0x00515018;   // (Stream*) (gfx8.cpp)
constexpr u32 Stream_SetAlphaTest_005150A8 = 0x005150A8;   // (Stream*)
constexpr u32 Stream_Read4x16_00515168 = 0x00515168;       // (Stream*, u32* a, u32* b, u32* c, bool* d) (gfx8.cpp)
constexpr u32 Record_Stencil_005151E8 = 0x005151E8;        // (Stream*)
constexpr u32 Cmd_SetStencil_00515350 = 0x00515350;        // (Stream*) (gfx12.cpp)
constexpr u32 Stream_ReadPair_00515544 = 0x00515544;       // (Stream*, u32* second) -> first (gfx8.cpp)
constexpr u32 Stream_SetFpConstant_005155C4 = 0x005155C4;  // (Stream*) (gfx9.cpp)
constexpr u32 Stream_SetVpConstant_00515688 = 0x00515688;  // (Stream*)
constexpr u32 VpConst_SetMvp_00515784 = 0x00515784;        // (x, const f32 a[16], const f32 b[16])
constexpr u32 VpConst_SetMvp_005158B4 = 0x005158B4;        // (x, const f32 a[16], const f32 b[16])
constexpr u32 Draw_Pass_005159E8 = 0x005159E8;             // (Stream*, Pass*) (gfx15.cpp; itself, for a sub-stream)
constexpr u32 Matrix_MulAffine_0079DE98 = 0x0079DE98;      // (out, a, b)
constexpr u32 Texture_Bind_0079DF78 = 0x0079DF78;          // (Texture*, s32 unit)
constexpr u32 Draw_BindVertexArrays_0079E0C0 = 0x0079E0C0; // (Buffer*, VertexFormat*, offset, stream) (gfx12.cpp)
constexpr u32 Draw_SetDepthBounds_0079E2B8 = 0x0079E2B8;   // (f1 zmin, f2 zmax)
constexpr u32 Material_Bind = 0x0079CD0C;                  // (AglMaterial*, Shader*)
constexpr u32 FProgram_Upload = 0x0052F4F0;                // (FProgram** current) (fx.h)
constexpr u32 cellGcmSetFragmentProgram = 0x005E42A0;      // (context, program, 1) (draw_state.h)
constexpr u32 cellGcmSetDrawArrays = 0x005E4D98;           // (context, mode, first, count) (pool.h)
constexpr u32 cellGcmSetVertexDataArrayUnsafe = 0x005E4394;  // (context, index, frequency, stride, size, type, location, offset) (draw3.h)
constexpr u32 View_Update = 0x005115F0;                     // (View*, u32)
constexpr u32 Draw_SetState = 0x005218D8;                   // (FrameBuffer*, const State*) (world/draw.cpp)
constexpr u32 FrameBuffer_SetScissor_0052173C = 0x0052173C; // (FrameBuffer*, const u16 rect[4])
constexpr u32 Mem_FreePool = 0x0056C730;                    // (p, pool, u32 index)
constexpr u32 Mem_AllocPool = 0x0056C514;                   // (align, size, pool, flags) -> pointer (gfx11.h)
constexpr u32 Iff_ChunkSize = 0x005637D4;                   // (IffReader*) -> u32
constexpr u32 FileReader_Read = 0x00563768;                 // (FileReader*, buf, size) -> u32
constexpr u32 Texture_CopyDefaultFormat_005398CC = 0x005398CC;  // (Texture*) -> bool (gfx9.cpp)
constexpr u32 cellGcmAddressToOffset = 0x00ABD5E4;          // (address, u32* offset) import
constexpr u32 Mat4_Invert = 0x00554C28;                     // (f32 tmp[16], f32 out[16], const f32 in[16]) (view/camera.h)
// libgcm one-method setters (context, value), by the method they write (names?).
constexpr u32 cellGcmSetColorMask_5E3CFC = 0x005E3CFC;      // 0x0324
constexpr u32 cellGcmSetCullFace_5E3D20 = 0x005E3D20;       // 0x1830
constexpr u32 cellGcmSetDepthFunc_5E3D44 = 0x005E3D44;      // 0x0A6C
constexpr u32 cellGcmSetFrontFace_5E3D68 = 0x005E3D68;      // 0x1834
constexpr u32 cellGcmSetAlphaTestEnable_5E3DB0 = 0x005E3DB0;  // 0x0304
constexpr u32 cellGcmSetBlendEnable_5E3DD4 = 0x005E3DD4;    // 0x0310
constexpr u32 cellGcmSetCullFaceEnable_5E3DF8 = 0x005E3DF8; // 0x183C
constexpr u32 cellGcmSetDepthTestEnable_5E3E1C = 0x005E3E1C;  // 0x0A74
constexpr u32 cellGcmSetRestartIndexEnable_5E3E40 = 0x005E3E40;  // 0x1DAC
constexpr u32 cellGcmSetRestartIndex_5E3F5C = 0x005E3F5C;   // 0x1DB0
constexpr u32 Gcm_5E3FA4 = 0x005E3FA4;                      // (context, a, b): methods 0x1E98 / 0x0100 (name?)
constexpr u32 Trail_UpdateFade_0051ED00 = 0x0051ED00;       // (PostEffect*)
constexpr u32 Countdown_Tick = 0x0051F160;                  // (o; f1 dt)
constexpr u32 View_SaveRestore_00543AAC = 0x00543AAC;       // (o)
constexpr u32 View_ProjectPoint = 0x00522750;               // (View*, f32 out[4], const f32 in[3])
constexpr u32 Draw_BindScreenTargets = 0x00513038;          // () (draw_state.cpp)
constexpr u32 PtrVector_Clear = 0x0054EC1C;                 // (PtrVector*)
constexpr u32 Nop = 0x0054EC18;                             // the shared empty function
constexpr u32 Mem_FreeFrom = 0x0054EE6C;                    // (p, pool)
constexpr u32 Part_Release_0052F06C = 0x0052F06C;           // (Handle*)
constexpr u32 Pool_DestructPart = 0x0052EFE0;               // (part*)
constexpr u32 Tex_FindLoaded = 0x0050C398;                  // (name) -> Texture* or 0 (gfx11.h)
constexpr u32 Tex_GetDefault = 0x004FEF30;                  // (kind) -> Texture* (gfx11.h)
constexpr u32 Material_AddTexture_004FF008 = 0x004FF008;    // (Material*, Texture*) (gfx12.cpp)
constexpr u32 Material_ParamSlot = 0x0050C018;              // (Material*, u32 index) -> u32* (gfx12.h)
constexpr u32 View_GetCurrent = 0x004FDBA8;                 // () -> View*
constexpr u32 Draw_ResetState = 0x005355D8;                 // (u32, u32) (draw_state.h)
constexpr u32 Draw_SetTexture0 = 0x00536AC0;                // (Target*, u32 unit, u32) (draw_state.h)
constexpr u32 Draw_PrepareQuad = 0x0053539C;                // () (draw_state.h)
constexpr u32 Draw_RestoreDepth = 0x00535294;               // () (draw_state.h)
constexpr u32 Stencil_Variant_0052DAEC = 0x0052DAEC;        // (o, u32 pass, s32 i) -> fragment program index (gfx8.cpp Calc_0052DAEC)
constexpr u32 Gcm_SetDepthBoundsTest_005E2BD4 = 0x005E2BD4;  // (context, u32 enable) libgcm (name?)
constexpr u32 Gcm_SetDepthBounds_005E341C = 0x005E341C;      // (context; f1 zmin, f2 zmax) libgcm (name?)
}  // namespace fn

namespace g {
constexpr u32 current_surface = 0x010DDA74;  // u32: object whose +672 is the current View (view/view2.h)
constexpr u32 white = 0x01164CA0;            // u8[4] (gfx14::g::white)
constexpr u32 display = 0x00F66FBC;          // AglRenderSurface*: the display (draw::g::display)
constexpr u32 mesh_map = 0x010DDC80;         // the mesh map (gfx11::g::topology_map; Mesh_FindByHash's)
constexpr u32 draw_pass_arg = 0x00F66FB8;   // u32: the third argument of the ShaderType select calls (draw3.h block_f66fb8)
constexpr u32 vertex_format_1 = 0x010DC7C0; // the second 128-byte vertex format (render.h vertex_formats + 128)
constexpr u32 blend_cache = 0x010DD598;     // {u32 bit 31 blend on, bits 15..30 sfactor; u16 +4 equation, u16 +6 dfactor} (render.h state_cache_a)
constexpr u32 depth_cache = 0x010DD5A0;     // {u32 bit 31 depth write, bits 15..30 func; u16 +4 front face} (render.h state_cache_b)
constexpr u32 identity = 0x01164BD0;         // f32[4][4] identity (gfx11::g::identity): the layer's first state
constexpr u32 uv_to_clip = 0x010F0A50;       // f32[4][4] (layers3::g::uv_to_clip)
constexpr u32 state_reset_once = 0x011647A4;  // u8: set by RenderState_Reset_0053170C (name?)
// What Render_FreeLists_005AD15C frees (names?).
constexpr u32 tr_list_live = 0x011DE480;   // u64: the transform list is built (obj/obj12.h kTrListGuard)
constexpr u32 tr_list = 0x011DE304;        // PtrVector (obj/obj12.h kTrList + 4)
constexpr u32 bank_globals = 0x01193060;   // +40 / +48 / +64 blocks, u16 +68 / +5200 counts, +5220 kind list (obj/obj15.h kBankGlobals)
constexpr u32 bank_count_193000 = 0x01193000;  // u16 cleared with them
constexpr u32 pools_last = 0x0116DFF0;     // the last of 32 272-byte pools {+72 block, s32 +208 count, +216 Obj*[]} (going down)
constexpr u32 callback_a = 0x011950A0;     // {+0 arg, +4 {+4 arg, +12 Obj*}}: slot 2 called with both args
constexpr u32 parts_last = 0x01194F40;     // the last of 3 52-byte parts at +44 (going down)
constexpr u32 callback_b = 0x01194580;     // the same shape (obj/obj5.h kK13List)
constexpr u32 block_16bd08 = 0x0116BD08;   // a block freed last
constexpr u32 stencil_vconst_ready = 0x01164860;  // u8: stencil_vconst zeroed (name?)
constexpr u32 stencil_vconst = 0x01164870;        // f32[4] zeros: vertex constant 1 of the stencil pass (name?)
constexpr u32 vp_stencil = 0x010F0ADC;            // VProgram*: the stencil pass's (name?)
constexpr u32 fp_stencil = 0x010F0AE0;            // FProgram*[]: per Stencil_Variant_0052DAEC index (name?)
constexpr u32 default_textures = 0x010DDADC;  // Texture*[6]: white, white, grey, falloff, normal, black (shgroup::g::group_textures; name?)
constexpr u32 group_texture = 0x010DDADC;    // Texture*: shgroup::g::group_textures[0], the emitter material's texture
constexpr u32 material_list = 0x00F8F114;    // AglMaterial free-list header: +0 allocator, +4 capacity (shgroup::g::material_list)
constexpr u32 vt_material_list = 0x00B10ED0;   // ApeTFreeList<AglMaterial>
constexpr u32 str_material_list = 0x00AF1193;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglMaterial]"
}  // namespace g

// EffectSlot_Texture_005AB5F0's .text constants: f32 {FLT_MAX, -FLT_MAX, 1}.
constexpr u32 kSlotTextureConsts = 0x005AB5C8;
// Texture u16 +52: the level-of-detail size EffectSlot_Texture_005AB5F0 compares (name?).
constexpr u32 kTexLodSize = 52;

// Emitter_Check_00506050's .text constants: f32 1, f64 1 +8, f64 0 +16,
// f32 255 +24 (the depth fade's clamp and scale).
constexpr u32 kEmitterCheckConsts = 0x00506030;
constexpr u32 kViewMatrix = 752;  // View +752: the f32[4][4] the depth fade transforms by (name?)

// EmitProps_SetDesc_004FF4C0's .text constants: f32 {1, 0.75, 0}.
constexpr u32 kEmitDescConsts = 0x004FF4B0;
constexpr u32 kEmitShaderHash = 0x90873B1B;  // the shader a new emitter material gets
// The new material's blend word +104: bit 31 set, bits 15..30 = 770
// (GL_SRC_ALPHA: own blending), u16 +108 = 0x8006 (GL_FUNC_ADD).
constexpr u32 kEmitBlendSrc = 770, kEmitBlendEq = 0x8006;
constexpr u32 kMaterialSize = 144;

// The built-in textures Texture_InitDefaults_0051DA1C makes from BMPs in
// .data ({u32 size, bytes}), named "MEM:<name>.Bmp" while loading and
// "APE_<NAME>" once loaded, into gfx15::g::default_textures.
struct BuiltinTexture {
    u32 data;       // u32 size, then the BMP
    u32 mem_name;   // "MEM:White.Bmp", ...
    u32 ape_name;   // "APE_WHITE", ...
    u32 slot;       // byte offset in default_textures
};
constexpr BuiltinTexture kBuiltinTextures[] = {
    {0x00F66910, 0x00AF1A94, 0x00AF1AA4, 0},   // White (also slot 4)
    {0x00F66A0C, 0x00AF1AB4, 0x00AF1AC4, 8},   // Grey
    {0x00F66B08, 0x00AF1AD4, 0x00AF1AE4, 12},  // Falloff
    {0x00F66CC4, 0x00AF1AF4, 0x00AF1B04, 16},  // Normal
    {0x00F66DC0, 0x00AF1B14, 0x00AF1B24, 20},  // Black
};
constexpr u32 str_bmp = 0x00AF1A8C;  // "Bmp": Texture_Init's kind
constexpr u32 kTexNameSize = 32;     // Texture +16 char[32] (+47 forced 0), +48 its hash

// Stencil_Pass_0054C268: the depth texture by display mode (mem_stats
// offsets), the stencil light record's fields and the fragment parameter
// records it fills.
constexpr u32 kStencilPassOne = 0x0054C264;  // f32 1
constexpr u32 kStatsDepthAlt = 132, kStatsDepthMono = 88, kStatsDepthEye0 = 104, kStatsDepthEye1 = 108;
constexpr u32 kViewInvProj = 1008;  // View +1008: the matrix the light's depth range goes through (name?)
constexpr u32 kTexAddressUnit1 = 0x00041A28, kTexAddressClampBorder = 0x10030303;  // TEXTURE_ADDRESS(1) (name?)

// Draw_Pass_005159E8's stream commands (the u8 at the 12-byte header).
namespace pass_cmd {
constexpr u32 kSetMatrix = 0, kMulMatrix = 1, kMvp2 = 2, kMvp = 3, kBindMaterial = 4, kBindShader = 5,
              kVpConstant = 6, kFpConstant = 7, kTexture = 8, kScissor = 9, kDepth = 10, kBlend = 11,
              kCull = 12, kStencil = 13, kStencilRecord = 14, kAlphaTest = 15, kLineWidth = 16, kDepthBias = 17,
              kSubStream = 18, kDrawArrays = 19, kEnd = 22, kReturn = 23;
}  // namespace pass_cmd
// ShaderType / Shader vtable slots the pass calls.
constexpr u32 kShaderTypeSelect = 16;  // (ShaderType*, AglMaterial*, x) -> Shader*
constexpr u32 kShaderBegin = 8;        // (Shader*, AglMaterial*)
constexpr u32 kShaderEnd = 16;         // (Shader*, AglMaterial*)
constexpr u32 kScissorMethod = 0x000808C0;  // SET_SCISSOR_HORIZONTAL, 2 words
constexpr u32 kDepthFuncMethod = 0x00040A6C, kFrontFaceMethod = 0x00041834, kCw = 0x0900, kCcw = 0x0901;
constexpr u32 kDepthBiasConsts = 0x00514D54;  // f32 {-1e-4, 0, 65536, 2^24}: Stream_SetDepthBias_00514D68's bias and scales
constexpr u32 kDrawPassZero = 0x005159E0;  // f32 0: the depth bounds the pass resets to

// The frame buffer's command stream writer: +0 start, +4 end, +16 the
// write cursor; FrameBuffer_BeginLayer resets its state cache +80..+200.
constexpr u32 kFarDefault = 0x47C35000;  // f32 100000: +96
constexpr u32 kMinusOne = 0xBF800000;    // f32 -1: +196

// Iff_HandleTextureData_005399CC: the texture's pixel data (+56 pointer,
// +60 RSX offset << 1 | location, +64 size << 4 | low bits, +68 top
// nibble the pool index) and its header copy +72 (24 bytes: u8 +72
// format, u8 +86 location, u32 +92 offset, u16 +80 / +82 size).
constexpr u32 format_table = 0x00F67254;  // 24-byte entries {u32 +0 value, u32 +4 format id} (fx copy_colour_to's kFormatTable)
constexpr u32 kFormatCount = 19;
constexpr u32 kPoolTexture = 4, kPoolTextureAlt = 5;  // Mem_FreePool pools by +60 bit 0
constexpr u32 kTexDataAlign = 128;
constexpr u32 kHeaderMin = 12;

// The light group's per-light vtable slots (LightGroup_Draw_0052CD5C).
constexpr u32 kLightPrepare = 12;  // (Light*)
constexpr u32 kLightDraw = 24;     // (Light*, LightGroup*, const LightView*, x)

// The post effects' vtable slots GodRays_Update_005437F0 calls (byte
// offsets; names?).
namespace post_vt {
constexpr u32 kIsActive = 12, kPrepare = 24, kBegin = 32, kTrailStep = 40, kBeginPass = 36, kFirstFrame = 44, kPass = 48,
              kFirstFrameEnd = 52, kSetIntensity = 60;
}  // namespace post_vt
constexpr u32 kGodRaysZero = 0x005437EC;  // f32 0
constexpr u32 kTrailOne = 0x0054486C;     // f32 1: Trail_Update_00544870's lerp
constexpr u32 kPostOn = 0x80;      // PostEffect u8 +20 bit 7: enabled
constexpr u32 kGodRaysStarted = 0x40, kGodRaysPrimed = 0x80;  // u8 +120

constexpr u32 kLoadParams5C3518 = 0x005C3518;  // f32 15, f64 0 +8: LoadParams_005C3528's param 4 default and floor

}  // namespace gfx15
}  // namespace sly4::render
