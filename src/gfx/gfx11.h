// The renderer range's remaining recompiled functions of ~80 to ~450
// instructions (gfx11*.cpp): the callees gfx10*.cpp left (GPU timer,
// stencil shadow quad, ...) and the next smallest functions.
#pragma once

#include <bit>
#include <cmath>

#include "gfx10.h"

namespace sly4::render {
namespace gfx11 {

namespace fn {
// Still recompiled (called from gfx11*.cpp).
constexpr u32 Buffer_Alloc52F198 = 0x0052F198;  // (Buffer*, bytes, align, where, flags) -> RSX offset
constexpr u32 Mem_AllocPool = 0x0056C514;       // (align, size, pool, flags) -> pointer
constexpr u32 Thunk_PrepareQuad_0053C850 = 0x0053C850;  // Draw_PrepareQuad tail wrapper (Thunk_0053C850)
constexpr u32 Thunk_RestoreDepth_0053C830 = 0x0053C830;  // Draw_RestoreDepth tail wrapper (Thunk_0053C830)
constexpr u32 DrawQueue_Build_00516970 = 0x00516970;     // (Batch* on the stack: +0 list, +4 0x24000, +8..+24 0) (name?)
constexpr u32 Tex_FindLoaded = 0x0050C398;    // (name) -> Texture* or 0
constexpr u32 Tex_GetDefault = 0x004FEF30;    // (kind) -> Texture*
constexpr u32 Material_AddTexture_004FF008 = 0x004FF008;  // (Material*, Texture*) (name?)
constexpr u32 View_GetCurrent = 0x004FDBA8;   // () -> View*
constexpr u32 Mat4_Ortho = 0x0056A358;          // (f32 out[16]; f1 l, f2 r, f3 b, f4 t, f5 n, f6 f)
constexpr u32 Sphere_Transform = 0x00557900;    // (f32 sphere[4], const Matrix*, bool scale_radius)
constexpr u32 Frustum_SliceSphere_0052B210 = 0x0052B210;  // (f32 out[4], const f32 c[16]; f1 near, f2 far) (name?)
constexpr u32 Shadow_FrustumCorners_0052B4B8 = 0x0052B4B8;  // (cam, -, -, f32 out[32]; f1 near, f2 far) reads r3 / r6 (name?)
constexpr u32 Shadow_Bounds_0052B2EC = 0x0052B2EC;  // (f32 lo[3], f32 hi[3], corners, Matrix* cam+128, Matrix* view) (name?)
constexpr u32 Shadow_SnapBounds_0052AF78 = 0x0052AF78;  // (f32 lo[3], f32 hi[3], u16 w, u16 h) (name?)
constexpr u32 Shadow_DepthRange_0052AA98 = 0x0052AA98;  // (f32* near, f32* far, bounds[6], view, x, range[8]) (name?)
constexpr u32 Matrix_Copy_004FDEA0 = 0x004FDEA0;  // (dst, src) (name?)
constexpr u32 Has56_0053A9C4 = 0x0053A9C4;    // bool (Texture*): +56 != 0
constexpr u32 Owner_Create_004FD8D4 = 0x004FD8D4;  // (o) -> the record list holder kept at o+92 (name?)
constexpr u32 SortedMap_Remove79E7B0 = 0x0079E7B0;  // (map, const u32* key)
constexpr u32 Obj_SetSlot20 = 0x004FEF48;           // (obj, u32, u32)
constexpr u32 Obj23650_DestructInline = 0x0051F4B0; // (Warp*) the destructor body
constexpr u32 SortedMap_Remove79E4C8 = 0x0079E4C8;  // (map, const u32* key)
constexpr u32 Call_00548938 = 0x00548938;          // (obj) (name?)
constexpr u32 Fn_00548A10 = 0x00548A10;            // () (name?)
constexpr u32 Fn_00511F8C = 0x00511F8C;            // () (name?)
constexpr u32 Fn_00512148 = 0x00512148;            // () (name?)
constexpr u32 Debug_SwapPrimPools = 0x0056B4E8;
constexpr u32 Debug_ReleasePrimPool = 0x0056B2B8;
constexpr u32 JobQueue_Get = 0x0056E038;           // () -> 0x0116B600
constexpr u32 JobChain_Kick = 0x0056E1F0;          // (JobChainOwner*)
constexpr u32 Surface_DefaultRects_0051AE24 = 0x0051AE24;  // (Surface*, ViewRect** a, ViewRect** b)
constexpr u32 Gcm_SetViewport = 0x005E4068;  // (ctx, x, y, w, h, -, -, f32 scale[4], f32 offset[4]; f1 min, f2 max) libgcm
constexpr u32 String_CompareNoCase = 0x00555DE8;  // (a, b) -> 0 if equal
constexpr u32 memmove = 0x0071C3F0;               // (dst, src, n)
constexpr u32 Target_Bind51B4C4 = 0x0051B4C4;   // (colour, depth, Texture*) -> RenderSurface*
constexpr u32 RenderTarget_Init_00521110 = 0x00521110;  // (RenderTarget*, arg) (name?)
constexpr u32 Iff_ReadU16 = 0x00564340;             // (IffReader*, u16* out, name)
constexpr u32 RenderTarget_ColourTexture_0051E514 = 0x0051E514;  // decompiled: gfx11.cpp
constexpr u32 Random_Pair_0051F660 = 0x0051F660;  // (o)
constexpr u32 ThermalVision_Draw = 0x00545B60;    // (o)
constexpr u32 Trail_Fn_0051E8E0 = 0x0051E8E0;  // (o, o+48, o+64, o+80) reads r4..r6 (name?)
constexpr u32 Trail_Fn_0051EB20 = 0x0051EB20;  // (o) -> f1 the target value (name?)
constexpr u32 Material_Bind = 0x0079CD0C;        // (AglMaterial*, Shader*)
constexpr u32 Matrix_MulRot = 0x00553FF0;        // (out, a, b): rows 0..2 = a x b, translation 0
constexpr u32 VpConst_SetModelView_005012A4 = 0x005012A4;  // decompiled: gfx11.cpp
constexpr u32 Stencil_SetEqual32_005011A0 = 0x005011A0;
constexpr u32 VpConst_Set_004FCC34 = 0x004FCC34;  // (const f32 m[16]) (name?)
// Imports.
constexpr u32 cellGcmGetTimeStamp = 0x00ABD664;  // (report index) -> u64 ns
constexpr u32 cellGcmAddressToOffset = 0x00ABD5E4;  // (address, u32* offset)
}  // namespace fn

namespace g {
constexpr u32 frame_counter = 0x010F0F9C;  // u32 counter GpuTimer_Update_0053CA20 copies to +36 (name?)
constexpr u32 shadow_depth = 0x011640D8;    // Target*: the depth target ShadowPass_CopyDepth_005181E0 fills (name?)
constexpr u32 zero_c90_ready = 0x010F0C80;  // u8: 0x010F0C90 zeroed (name?)
constexpr u32 zero_c90 = 0x010F0C90;        // f32[4] zeros, vertex constant 1 of the depth copy (name?)
constexpr u32 origin = 0x01164B40;   // f32[3] {0, 0, 0}
constexpr u32 axis_b50 = 0x01164B50; // f32[3]: the up vector when the direction is along axis_b60 (name?)
constexpr u32 axis_b60 = 0x01164B60; // f32[3]: the default up vector (name?)
constexpr u32 texture_alloc = 0x00F8F118;  // u32: ApeTListAllocator<AglTexture> state {+0 free list, +4 heap, +8 OPD on load, +12 its argument} (name?)
constexpr u32 texture_list = 0x010DDAF4;   // Array* {u16 cap, u16 count, u16 peak, u16 flags, {u32 hash, Texture*}* +8} (name?)
constexpr u32 skip_flag_66fc0 = 0x00F66FC0; // u8: 0 skips RenderTarget_Run_00522D18's work (wupdate kFlag66FC0)
constexpr u32 identity = 0x01164BD0;       // f32[4][4] identity
constexpr u32 topology_map = 0x010DDC80;   // u32: map AglTopology_Delete_005261A0 removes +80 from (name?)
constexpr u32 effect_slots = 0x01194ED8;   // 200-byte entries, count u32 0x01194FA0 (name?)
constexpr u32 effect_count = 0x01194FA0;
constexpr u32 effect_pair = 0x01195090;    // u32[2] cleared with the count (name?)
constexpr u32 pending_list = 0x010E05D4;   // u32[]: objects Render_Phase_005124F4 flushes (Call_00548938), count u32 0x010E0614 (name?)
constexpr u32 pending_count = 0x010E0614;
constexpr u32 depth_bounds_warned = 0x011DE2C4;  // u8: set once for a depth format without depth bounds (name?)
constexpr u32 vec_bb0 = 0x01164BB0;  // f32[4] default copied by Trail_Construct_0051EF28 (name?)
constexpr u32 queries_on = 0x00F66FD5;     // u8: Query_PollResults_00548A10 runs (name?)
constexpr u32 shadow_verts_peak = 0x010DD590;  // u16: largest shadow mesh vertex count (name?)
constexpr u32 thermal_defaults_set = 0x00F67A02;  // u8: ThermalVision_Update_0054664C filled +20..+28 once (name?)
constexpr u32 thermal_tex_alt = 0x00F67500;   // Texture*: over mem_stats +84 (name?)
constexpr u32 thermal_tex_eye = 0x00F67504;   // Texture*: over mem_stats +100, config +34 set (name?)
constexpr u32 billboard_rot = 0x010DD940;  // f32[4][4]: the rotation billboards multiply by (name?)
}  // namespace g

// The f32 bits of a double rounded to single (frsp).
inline u32 f32_bits(f64 v) { return std::bit_cast<u32>(static_cast<f32>(v)); }
// f32 / f64 constants as the FPR holds them (non-NaN .text constants).
inline f64 lfs(u32 a) { return gfx10::lfs_value(load<u32>(a)); }
inline f64 lfd(u32 a) { return std::bit_cast<f64>(load<u64>(a)); }
// stfs of an FPR: NaNs keep their payload bits (not quieted).
inline u32 stfs_bits(f64 v) {
    const u64 d = std::bit_cast<u64>(v);
    if (std::isnan(v)) return static_cast<u32>(((d >> 63) << 31) | (0xFFu << 23) | ((d >> 29) & 0x7FFFFF));
    return std::bit_cast<u32>(static_cast<f32>(v));
}
// Single-precision arithmetic as the FPU does it: the s-forms round the
// double result; the fused ones round once.
inline f64 sgl(f64 x) { return static_cast<f64>(static_cast<f32>(x)); }
inline f64 madds(f64 a, f64 c, f64 b) { return sgl(std::fma(a, c, b)); }    // fmadds
inline f64 msubs(f64 a, f64 c, f64 b) { return sgl(std::fma(a, c, -b)); }   // fmsubs
inline f64 nmsubs(f64 a, f64 c, f64 b) { return sgl(-std::fma(a, c, -b)); } // fnmsubs
// fctidz then fcfid: truncation to an integer-valued double (saturating,
// NaN -> -2^63).
inline f64 trunc_d(f64 x) {
    if (std::isnan(x)) return -9223372036854775808.0;
    const f64 r = std::trunc(x);
    if (r >= 9223372036854775807.0) return 9223372036854775807.0;
    if (r <= -9223372036854775808.0) return -9223372036854775808.0;
    return static_cast<f64>(static_cast<s64>(r));
}

}  // namespace gfx11
}  // namespace sly4::render
