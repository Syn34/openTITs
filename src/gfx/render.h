// Renderer-side helpers: display buffer selection, the depth target, the
// render-state cache reset and the BloomNoise post effect.
//
// The display (draw::g::display, 0x00F66FBC): +16.. / +64.. / +100.. /
// +108.. Target* per eye (index ((+52 >> 28) & 3)), +24 / +72 / +88 /
// +120 / +124 depth targets, +52 u32 flags (0x02000000: per-eye buffers),
// +84 alternate colour target, +148 u8 bit 7: alternate buffer set.
// g::mem_stats (0x010F0E20) +92.. / +112.. / +120.. / +128 hold the
// matching scene textures (fx::scene_colour), +184 u32 buffer selector.
#pragma once

#include "fx.h"

namespace sly4::render {

namespace fn {
constexpr u32 Display_GetTargets = 0x005356F4;    // decompiled: src/gfx/render_state.cpp
constexpr u32 Display_SelectBuffer = 0x00537C18;  // decompiled: src/gfx/render_state.cpp
constexpr u32 Draw_SetDepthTarget = 0x0053755C;   // decompiled: src/gfx/render_state.cpp
constexpr u32 Draw_InvalidateState = 0x0051279C;  // decompiled: src/gfx/render_state.cpp
constexpr u32 VertexFormat_Get = 0x00515750;      // decompiled: src/gfx/render_state.cpp
constexpr u32 BloomNoise_Draw = 0x00540DD8;       // decompiled: src/gfx/bloomnoise.cpp
constexpr u32 Job_Swap = 0x00737278;              // decompiled: src/gfx/render_job.cpp
constexpr u32 Job_EndFrame = 0x0078896C;          // decompiled: src/gfx/render_job.cpp
constexpr u32 World_RunRenderJobs = 0x00458BAC;   // decompiled: src/gfx/render_job.cpp
}  // namespace fn

// RenderLayer (vtable 0x00B23410; base class 0x00B233E8): a static draw
// list. +0 vtable, +4 Entry* base, +8 Entry* cursor (Entry = {u32 key,
// DrawItem* item}), +12 u8 flags (bit 7: skip), +13 u8 stencil ref,
// +16 const char* name, +20 Callback[4] pre-draw (+84 u8 count), +88
// Callback[4] post-draw (+152 u8 count), +156 u32 capacity, +160 u32
// high-water count, +164 u32 capacity * 8, +168 u8[4].
// Slots: 0 Reset, 1 AddKeyed, 2 Add, 3 Sort, 4 Draw, 5 ~, 6 delete.
namespace layer {
constexpr u32 kBase = 4, kCursor = 8;
constexpr u32 kVtable = 0x00B23410, kBaseVtable = 0x00B233E8;
constexpr u32 kAddConsts = 0x007A10F8;  // f64 0.5, f64 0x1FF8000000000000 as a value, f32 1e-5, f64 0.0, f32 2.0
constexpr u32 RenderLayer_Reset = 0x007A1080;         // decompiled: src/gfx/render_layer.cpp
constexpr u32 RenderLayer_AddKeyed = 0x0051384C;      // decompiled: src/gfx/render_layer.cpp
constexpr u32 RenderLayerBase_Add = 0x00513868;       // decompiled: src/gfx/render_layer.cpp
constexpr u32 RenderLayer_Add = 0x007A1120;           // decompiled: src/gfx/render_layer.cpp
constexpr u32 RenderLayer_Sort = 0x00513894;          // decompiled: src/gfx/render_layer.cpp
constexpr u32 RenderLayer_Draw = 0x00513984;          // decompiled: src/gfx/render_layer.cpp
constexpr u32 RenderLayer_Destruct = 0x0079C260;      // decompiled: src/gfx/render_layer.cpp
constexpr u32 RenderLayerBase_Destruct = 0x0079ECC4;  // decompiled: src/gfx/render_layer.cpp
constexpr u32 RenderLayer_Delete = 0x007A1210;        // decompiled: src/gfx/render_layer.cpp
constexpr u32 RenderLayerBase_Delete = 0x007A108C;    // decompiled: src/gfx/render_layer.cpp
constexpr u32 Draw_SetStencilRef = 0x00512EF4;        // decompiled: src/gfx/render_layer.cpp
// Still recompiled.
constexpr u32 RenderLayer_RadixSort = 0x00513394;  // (RenderLayer*) sort for > 512 entries (name?)
constexpr u32 SortEntries = 0x0056A81C;            // (Entry*, u32 count, u32 size 8) (name?)
constexpr u32 DrawItem_Draw = 0x00512C30;          // (DrawItem*) (name?)
constexpr u32 RenderLayer_End = 0x00512DA0;        // () (name?)
constexpr u32 RenderLayers_Register = 0x0050D744;  // () builds g::layers (name?)
constexpr u32 RenderLayers_Construct = 0x0052E164; // static constructor of the 17 layers (name?)
constexpr u32 RenderLayers_Destroy = 0x0052DD00;   // static destructor (name?)
}  // namespace layer

namespace g {
constexpr u32 layers = 0x010DDA8C;          // {u32 count; RenderLayer* layer[]}: [15] (0x010DDACC) is MotionTrail
constexpr u32 camera_pos = 0x010DD8B0;      // f32[3]: eye position RenderLayer_Add measures from (name?)
constexpr u32 stencil_bytes = 0x010DDA60;   // u8[2]: Draw_SetStencilRef writes {0, ref} (name?)
constexpr u32 vertex_formats = 0x010DC740;  // two 128-byte vertex formats (VertexFormat_Get)
constexpr u32 state_cache = 0x010DDBB4;     // u32: object whose cached state Draw_InvalidateState resets (name?)
constexpr u32 state_cache_a = 0x010DD598;   // u64, reset to -1 (name?)
constexpr u32 state_cache_b = 0x010DD5A0;   // u64, reset to -1 (name?)
}  // namespace g

// Draw_SetDepthTarget, as BloomNoise_Draw also inlines it: nothing when d
// is already the depth target; otherwise fx::set_depth_target.
// Returns the r3 the original leaves: the render context on the early
// out, else the depth-mask word (d != 0).
inline u64 set_depth_target(PPUContext& ctx, u32 d) {
    const u32 st = fx::g::state;
    const u32 context = load<u32>(st);
    if (load<u32>(st + 84) == d) return context;
    fx::set_depth_target(ctx, d);
    return d != 0 ? 1u : 0u;
}

}  // namespace sly4::render
