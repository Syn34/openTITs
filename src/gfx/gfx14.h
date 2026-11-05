// The renderer range's next recompiled functions (gfx14*.cpp): the
// callees gfx13*.cpp left (the post-effect chain's constructor, the
// effect slot submit, ...) and the next smallest functions.
#pragma once

#include <cmath>

#include "gfx13.h"
#include "shader2.h"
#include "target.h"

namespace sly4::render {
namespace gfx14 {

namespace fn {
// Still recompiled or decompiled elsewhere (called from gfx14*.cpp).
constexpr u32 Trail_Construct_0051EF28 = 0x0051EF28;     // (Trail*) (gfx11.cpp)
constexpr u32 ScreenWarp_Construct_0051F1C0 = 0x0051F1C0; // (ScreenWarp*) (gfx13.cpp)
constexpr u32 Construct_0051F708 = 0x0051F708;           // (o) (gfx10.cpp)
constexpr u32 RenderTarget_BindTexture_005229B0 = 0x005229B0;  // (rt, Texture*) (gfx9.cpp)
constexpr u32 Material_SetMatrixParams_005AA718 = 0x005AA718;  // (Material*, v, const f32[16]) (gfx13.cpp)
constexpr u32 EffectSlot_Texture_005AB5F0 = 0x005AB5F0;  // (slot + 28, slot->+40, owner, view) -> Texture* (name?)
constexpr u32 EffectSlot_Draw_005AA998 = 0x005AA998;     // (view, rt, const f32 plane[4], owner, slot->+4; f1 fade) (name?)
constexpr u32 Emitter_Check_00506050 = 0x00506050;  // (Emitter*, item, a, b, c) -> 0 skips the draw (name?)
constexpr u32 Draw_Layer_00517488 = 0x00517488;      // (u32 3) (name?)
constexpr u32 Draw_Pass_005159E8 = 0x005159E8;       // (render::g::state_cache object, const Pass* {+0 0, +4 0, +16 f32[16]}) (name?)
constexpr u32 Stencil_Pass_0054C268 = 0x0054C268;    // (o, u32 pass; f1 scale) (name?)
constexpr u32 Topology_Construct = 0x00508690;  // (AglTopology*) (obj/obj6.h)
constexpr u32 Array_Reserve = 0x00558F3C;       // (Array*, u16 capacity) (post.h)
constexpr u32 FileReader_Read32 = 0x00569C10;   // (IffReader*, char out[32], name) (name?)
constexpr u32 Map_Find = 0x0079DD10;            // (map, const u32* key) -> value slot (uiload/more.h)
constexpr u32 Texture_LoadFromHeader_0053A94C = 0x0053A94C;  // (Texture*, const TextureHeader*) (name?)
constexpr u32 EmitProps_SetDesc_004FF4C0 = 0x004FF4C0;  // (AglEmitProperties*, const EmitDesc*) (name?)
constexpr u32 Gcm_Transfer_005E104C = 0x005E104C;  // (context, mode, dst offset, dst pitch, dst x, y, src offset, src pitch, src x, y, w, h, bytes per pixel) cellGcmSetTransferImage variant (libgcm; name?)
constexpr u32 Gcm_WriteLabel_005E0688 = 0x005E0688;  // (context, u32 label index, u32 value) (libgcm; name?)
constexpr u32 String_Format = 0x005534D0;        // (fmt, ...) the debug print (gcm.h)
constexpr u32 FileReader_ReadVec4Swapped = 0x00569AE8;  // (FileReader*, f32 out[4], name)
constexpr u32 cellSpursEventFlagClear = 0x00ABE004;  // (CellSpursEventFlag*, u16 bits) import
constexpr u32 EnsureTextures_00542804 = 0x00542804;  // () (name?)
constexpr u32 Texture_InitDefaults_0051DA1C = 0x0051DA1C;  // () (name?)
}  // namespace fn

namespace g {
constexpr u32 white = 0x01164CA0;  // u8[4] (gfx13::g::white)
constexpr u32 draw_list = 0x010EF63C;  // {u32 key, Item*}[640], u32 count +5120 (DrawList_Run_00517754's)
constexpr u32 draw_list_a = 0x010EE238;  // the same layout: Draw_Lists_00517488's bit-0 list (name?)
constexpr u32 draw_list_b = 0x010ECE34;  // its bit-1 list (name?)
constexpr u32 draw_busy = 0x010DDC00;  // u32: waited on before the draw list runs (layers3::g::atomic_ddc00)
constexpr u32 draw_slots = 0x00F66FD8; // u32: a table of 32-byte slots, indexed by the item's +76 (name?)
constexpr u32 vt_topology_list = 0x00B23E00;   // ApeTFreeList<AglTopology> (obj/obj6.h kTopologyListVtbl)
constexpr u32 str_topology_list = 0x00AF12C9;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglTopology]"
// The frame read-back (Display_Readback_0053BC14; name?).
constexpr u32 readback_on = 0x01163F81;     // u8: enabled
constexpr u32 readback_index = 0x01163FB4;  // u32: the buffer being read (the other one is written)
constexpr u32 readback_label = 0x01163FBC;  // u32: the RSX label value; written as +1
constexpr u32 readback_pending = 0x01163FC4;  // u8[2]: per buffer, a copy is in flight
constexpr u32 readback_busy = 0x01163FC6;   // u8: set once per frame (the function runs once)
constexpr u32 readback_offsets = 0x01163FE0;  // u32[2]: the buffers' RSX offsets
constexpr u32 readback_out_w = 0x01163FE8, readback_out_h = 0x01163FEC;  // u32: the second copy's size
constexpr u32 readback_pitch = 0x01163FF0;  // u32
constexpr u32 readback_out_bpp = 0x01163FF4;  // u32: the second copy's bytes per pixel
constexpr u32 default_tex_ready = 0x011972A0;  // u8: default_tex fetched (SelectShader_005D47A0's function-local static)
constexpr u32 default_tex = 0x011972A8;        // Texture*: Tex_GetDefault(5)
constexpr u32 default_tex_ready2 = 0x011972B0;  // u8 / Texture*: SelectShader_005D62E8's
constexpr u32 default_tex2 = 0x011972B8;
constexpr u32 view_dir = 0x010DD8A0;           // f32[3]: the camera's view direction (next to render::g::camera_pos) (name?)
constexpr u32 identity_164C10 = 0x01164C10;  // f32[16]: copied into the pass Draw_Pass_005159E8 gets (name?)
}  // namespace g

// PostChain_Construct_0051F850's .text constants.
constexpr u32 kPostChainConsts = 0x0051F838;  // f32 {255, 0.9, 0.5, 0.4, 0.2}
constexpr u32 kVec3_164B40 = 0x01164B40;      // f32[3] copied into the effects' +48 (name?)
constexpr u32 kVec4_164BC0 = 0x01164BC0;      // f32[4] copied into GodRays +64 (gfx12::g::frustum_pad)

// EffectSlot_Submit_005AB908's constants: f32 {0, -1, 1, 0.01, 0.4, 0},
// f64 {1, 0} +24 / +32, f32 0.8 +40.
constexpr u32 kSlotSubmitConsts = 0x005AB8D8;
constexpr u32 kDefaultTexKind = 5;  // Tex_GetDefault's kind for a slot facing away

constexpr u32 kDegToRad_5D20D8 = 0x005D20D8;  // f32 pi / 180: LoadParams_005D20E0 turns d+120 / +124 to radians
constexpr u32 kDegToRad_5D7DE4 = 0x005D7DE4;  // f32 pi / 180: LoadParams_005D7DE8's

constexpr u32 kClearDepth_51A914 = 0x0051A914;  // f32 1.0
constexpr u32 kSysTimerUsleep = 141;
constexpr u32 kDrawWaitUsec = 300;

constexpr u32 kStencilPassConsts = 0x0054CB4C;  // f32 {1e-5, 1.0, 0.5}
constexpr u32 kStencilEqual = 0x0202;

constexpr u32 kTopologySize = 112;  // an AglTopology (its free list's element size)

// The texture chunk's fields (Iff_HandleTexture_0051D4B4's header).
constexpr u32 str_ver = 0x00B23763, str_name_hash = 0x00B23767, str_width = 0x00B23770, str_height = 0x00B23776,
              str_depth = 0x00B2377D, str_bpp = 0x00B23783, str_type = 0x00B23787, str_name = 0x00B2378C,
              str_pack_buf_ofst = 0x00B23791, str_pack_buf_size = 0x00B2379D;
constexpr u32 kIffShared = 4;  // IffReader u8 +12 bit 2: the textures are shared (look up only)

constexpr u32 kLightConsts = 0x00500D34;  // f32 {0, 1, 255}: Particle_LightColour_00500D40's clamp and scale

constexpr u32 kDepthStateConsts = 0x005495CC;  // f32 {0, 65536, 2^24}: DepthState_Apply_005495D8's bias scales
constexpr u32 kFrontFace = 0x00041834, kCcw = 0x0901, kLequal = 0x0203;

constexpr u32 kRatioOne = 0x00537110;  // f32 2^20: Surface_Copy_00537118's 12.20 scale ratio
constexpr u32 kTransferFormatA8R8G8B8 = 10;  // CELL_GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8 (name?)

constexpr u32 kReadbackLabel = 251;  // the RSX label index the read-back signals
constexpr u32 kTransferLocalToMain = 2, kTransferMainToLocal = 1;  // as Surface_Copy_00537118 numbers them

// The deflector (Iff_HandleDeflector_0052A7C0): 112 bytes {vtable, u16 +4
// refs, +8 plane count, +12 f32 bounce, +16 f32[6][4] planes}.
constexpr u32 vt_deflector = 0x00B232A0;
constexpr u32 str_deflector_ignored = 0x00AF1C48;  // "%s - Deflector will be ignored as particles not emit in world space\n"
constexpr u32 str_empty_b232c8 = 0x00B232C8;       // "" (the %s argument)
constexpr u32 str_bounce = 0x00B23BA5;
constexpr u32 str_planes0 = 0x00B23BAC;            // "planes[0]".."planes[5]", 10 bytes apart
constexpr u32 kNoPlane = 0x0052A7BC;               // f32 FLT_MAX: a plane w that marks it unused

constexpr u32 kParamFlag_5C67D8 = 0x005C67D8;  // f32 {1, 0}: LoadParams_005C67E0's param 3
constexpr u32 kZero_50B6D0 = 0x0050B6D0;  // f32 0
// The 16-slot job table SpursQueue_Push_007A61F4 fills (name?).
constexpr u32 kJobQueueOn = 11992;     // u8: accepting jobs
constexpr u32 kJobQueueMask = 11264;   // u32: bit i set while slot i is taken (0xFFFF: full)
constexpr u32 kJobQueueSlots = 11268;  // 44-byte slots {40-byte job copy, u32 +40 busy}
constexpr u32 kJobQueueFlag = 10624;   // CellSpursEventFlag: slot i's bit cleared when it's taken
constexpr u32 kJobQueueSlotSize = 44;
// The texture animation (Iff_HandleTexAnim_0052A528): 48 bytes {vtable,
// u16 +4 refs, f32 +8 speed, +12 1 / frames, +16 frames, u8 +20 mode,
// f32 +24 / +28 / +32 per mode}.
constexpr u32 vt_tex_anim = 0x00B23280;
constexpr u32 str_mode = 0x00B23C80, str_num_frame_x = 0x00B23C85, str_speed = 0x00B23C8F, str_variance2 = 0x00B23C95;
constexpr u32 kTexAnimConsts = 0x0052A51C;  // f32 {1, 2}
// Texture_SystemInit_0051DF3C's IFF chunk handlers (OPDs) and tags.
constexpr u32 opd_iff_texture_frxt = 0x00F3B4D8, opd_iff_texture_rxet = 0x00F3B4E0, opd_iff_texture_hrxt = 0x00F3B4E8,
              opd_iff_texture_cnit = 0x00F3B4F0, opd_iff_texture_data = 0x00F3B4F8;
constexpr u32 opd_map_compare_u32 = 0x00F3BB80;  // the unsigned-key map compare (obj/obj.h kMapCompareU32A)
constexpr u32 kTexturesDefault = 1024;  // texture capacity when config u16 +350 is 0
constexpr u32 kSelectConsts_5D4798 = 0x005D4798;  // f32 {0, 1e-5}
constexpr u32 kSelectConsts_5D62DC = 0x005D62DC;  // f32 {0, 1e-5}

// The post-effect chain (6848 bytes; RenderTarget_Setup_005203A8 then
// writes gfx13::vt_post_chain): +4..+40 the 10 effects in update order,
// then the effects themselves.
namespace post_chain {
constexpr u32 kEffects = 4;     // PostEffect*[10]
constexpr u32 kNumEffects = 10;
constexpr u32 kBloom = 48;
constexpr u32 kDoF = 112;
constexpr u32 kDesaturation = 192;
constexpr u32 kBlur = 256;
constexpr u32 kRadialBlur = 284;
constexpr u32 kScreenWarp = 316;
constexpr u32 kGodRays = 6176;
constexpr u32 kTrail = 6336;
constexpr u32 kBloomNoise = 6560;
constexpr u32 kLast = 6736;     // Construct_0051F708's object
constexpr u32 kSize = 6848;
}  // namespace post_chain

// Vtables PostChain_Construct_0051F850 writes: each effect gets the
// common post-effect base first, then its own.
constexpr u32 vt_post_chain_base = 0x00B23690;  // the chain's own base (then gfx13::vt_post_chain)
constexpr u32 vt_post_effect = 0x00B234E8;      // the post-effect base (Bloom's)
constexpr u32 vt_bloom = 0x00B23F38;
constexpr u32 vt_dof_base = 0x00B2356C;
constexpr u32 vt_dof = 0x00B23F90;
constexpr u32 vt_desaturation_base = 0x00B2353C;
constexpr u32 vt_desaturation = 0x00B240B0;
constexpr u32 vt_blur_base = 0x00B23514;
constexpr u32 vt_blur = 0x00B23F60;
constexpr u32 vt_radial_blur_base = 0x00B23590;
constexpr u32 vt_radial_blur = 0x00B235BC;
constexpr u32 vt_screen_warp2 = 0x00B24070;     // ScreenWarp's derived vtable (after gfx13::vt_screen_warp)
constexpr u32 vt_god_rays_base = 0x00B23610;
constexpr u32 vt_god_rays = 0x00B23FF0;
constexpr u32 vt_trail = 0x00B24038;
constexpr u32 vt_bloom_noise_base = 0x00B235DC;
constexpr u32 vt_bloom_noise = 0x00B23FC0;
constexpr u32 vt_last = 0x00B24090;             // Construct_0051F708's object's derived vtable
// Effect names (the rest are annotated by funcinfo).
constexpr u32 str_bloom = 0x00B23504;
constexpr u32 str_dof = 0x00B23584;           // "DoF"
constexpr u32 str_desaturation = 0x00B23554;
constexpr u32 str_blur = 0x00B2352C;
constexpr u32 str_radial_blur = 0x00B235A8;
constexpr u32 str_god_rays = 0x00AF1B58;
constexpr u32 str_bloom_noise = 0x00B235F8;

}  // namespace gfx14
}  // namespace sly4::render
