// Render layer item passes and their helpers: the ZTranslucent depth
// pre-pass (ZLayer_DrawBody), the two WaterResolve / ScreenWarp passes,
// Material_Bind (the material state DrawItem_Draw binds), the 16 slots'
// and the pool's constructors and the pool's small vtable slots.
//
// AglMaterial, as Material_Bind reads it: +16 shader picker (+12 -> a
// table whose [0] is the default Shader), +20 Texture*[n] (n = u8 +44),
// +104 u64 blend word pair (cached in render::g::state_cache_a), +112 u64
// depth / cull word pair (state_cache_b), +128 animation block (+0 u32
// frame stamp, three animated tracks at +8 / +56 / +104).
//   +104 u32: bit 31 blend on, bits 15..30 the factor pair source
//   +108 u16 blend equation, +110 u16 destination factor
//   +112 u32: bit 31 depth write, bits 15..30 depth func
//   +116 u16 cull face (0 = culling off; 0x900 -> front face 0x901)
// Texture: +56 the CellGcmTexture holder bound to a unit (render state
// +52.. caches it; +72 CellGcmTexture, +73 mipmap count), +118 / +120 u16
// filter indexes, +122 / +124 u16 wrap modes.
// Shader: +4 VProgram*, +8 FProgram*, vtable slot 8 Bind(shader, mat).
#pragma once

#include "layers3.h"

namespace sly4::render {
namespace layers4 {

namespace fn {
// Decompiled in layers4*.cpp.
constexpr u32 SlotBase_Construct = 0x005276AC;    // (slot*) (name?)
constexpr u32 Pool_DestructPart = 0x0052EFE0;     // (part*) (name?)
constexpr u32 Pool_Destruct = 0x0079C414;         // pool vtable slot 2 (name?)
constexpr u32 Pool_Delete = 0x007A3BE8;           // pool vtable slot 3 (name?)
constexpr u32 Pool_Nop4 = 0x007A2214;             // pool vtable slot 4: blr (name?)
constexpr u32 Pool_ResetState = 0x0054BD38;       // pool vtable slot 9 (name?)
constexpr u32 Pool_Refresh = 0x00527150;          // pool vtable slot 10 (name?)
constexpr u32 Draw_ApplyView = 0x00527108;        // () (name?)
constexpr u32 Pool_Nop11 = 0x0050F9A0;            // pool vtable slot 11: blr (name?)
constexpr u32 Stats_ResolveTexture = 0x00536D28;  // (mem_stats*) (name?)
constexpr u32 Display_SelectTarget = 0x00537CB8;  // (table*) (name?)
constexpr u32 Draw_CopyRect = 0x00536C3C;         // (Texture* dst, Target* src, sx, sy, dx, dy, w, h) (name?)
constexpr u32 Draw_CopyScreen = 0x00536D4C;       // () (name?)
constexpr u32 ResolveLayer_ItemRect = 0x005145C0;  // (RenderLayer*, DrawItem*, f32* x0, y0, x1, y1) screen rect (name?)
// Still recompiled.
constexpr u32 Draw_Clear = 0x00531F80;           // (u32 mask, u32 colour, f32 depth [f1], u32 stencil, u32 stencil mask) (name?)
constexpr u32 DrawItem_DrawDepth = 0x00538FA0;   // (Material* item+12, DrawItem*) the Z pre-pass draw (name?)
constexpr u32 DrawItem_GetScale = 0x00554484;    // (DrawItem*, f32 out[3]) per-axis scale (name?)
constexpr u32 View_ProjectSphere = 0x00557A90;   // (f32 sphere[4], view matrix, view +1008, f32* x0, y0, x1, y1) 0..1 rect (name?)
constexpr u32 MaterialAnim_Update = 0x00501520;  // (track*, f32 dt [f1]) (name?)
constexpr u32 Mem_FreePool = 0x0056C730;         // (p, pool, tag)
constexpr u32 cellGcmSetTransferImage = 0x005E47AC;
}  // namespace fn

namespace g {
constexpr u32 view_proj = 0x010DD7C0;      // f32[4][4] rows (block_dd780 + 64): item matrix * this -> vertex constants 0..3 (name?)
constexpr u32 anim_stamp = 0x010DDA7C;     // u32: frame stamp Material_Bind animates by (name?)
constexpr u32 anim_rate = 0x010DD5AC;      // f32 (block_dd5a8 + 4), times anim_scale = the animation step (name?)
constexpr u32 anim_scale = 0x010DD5B4;     // f32 (block_dd5a8 + 12) (name?)
constexpr u32 copy_stamp = 0x00F670DC;     // u32: g::config +76 when Draw_CopyScreen last ran (name?)
constexpr u32 copy_texture = 0x00F670E0;   // Texture*: what it returned (name?)
}  // namespace g

// Constants in .text next to the functions that read them.
constexpr u32 kPass1ClearDepth = 0x005141FC;  // f32 1.0
constexpr u32 kItemRectZero = 0x005145BC;    // f32 0.0
constexpr u32 kPass2Consts = 0x00514818;      // f32 0.0 (area start), f64 0.9 at +8 (screen fraction)

constexpr u32 kSlotVtable = 0x00B240F8, kSlotBaseVtable = 0x00B23720;
constexpr u32 kPoolBaseVtable = 0x00B236E8;
constexpr u32 kMaxRects = 96;

// RSX methods (one-word NV4097 commands, count 1 << 18).
constexpr u32 kColorMask = 0x00040324, kDepthTest = 0x00040A74, kDepthFunc = 0x00040A6C, kDepthMask = 0x00040A70;
constexpr u32 kCullEnable = 0x0004183C, kFrontFace = 0x00041834, kAlphaTest = 0x00040304, kBlendEnable = 0x00040310;
constexpr u32 kBlendFunc2 = 0x00080314, kBlendEquation = 0x00040320, kTexAddress = 0x00041A08;
constexpr u32 kVertexConst17 = 0x00441EFC;  // SET_TRANSFORM_CONSTANT_LOAD, 17 words
constexpr u32 kLequal = 0x0203, kCcw = 0x0901, kCw = 0x0900;

}  // namespace layers4
}  // namespace sly4::render
