// Render layer table set-up: the static constructor of the 17 render
// layers (and the render module statics it also initialises) and the
// registration that allocates their entry buffers.
//
// Layer layout: see render.h (namespace layer). The two 192-byte layers
// (WaterResolve, ScreenWarp) add +176 / +180 / +184 u32 (zeroed) and +188
// f32 (1.0) after +172.
#pragma once

#include "layer2.h"

namespace sly4::render {
namespace layers3 {

constexpr u32 kCount = 17;

// The layer statics, in g::layers order.
namespace at {
constexpr u32 Opaque = 0x010DC84C, Panorama = 0x010DC8F8, PreShadow = 0x010DC9A4,
              Translucent0 = 0x010DCA50, Translucent1 = 0x010DCAFC, Translucent2 = 0x010DCBA8,
              ZTranslucent = 0x010DCC54, WaterResolve = 0x010DCD00, ScreenWarp = 0x010DCDC0,
              Effects1 = 0x010DCE80, PostShadow1 = 0x010DCF2C, PostShadow2 = 0x010DCFD8,
              UI = 0x010DD084, PostPrim = 0x010DD130, Occluder = 0x010DD1DC,
              MotionTrail = 0x010DD288, ThermalVision = 0x010DD334;
}  // namespace at

// Layer names (const char*, .rodata).
namespace name {
constexpr u32 Opaque = 0x00AF1C90, Panorama = 0x00AF1C98, PreShadow = 0x00AF1CA8,
              Translucent0 = 0x00AF1CB8, Translucent1 = 0x00AF1CC8, Translucent2 = 0x00AF1CD8,
              ZTranslucent = 0x00AF1CE8, WaterResolve = 0x00AF1CF8, ScreenWarp = 0x00AF1B60,
              Effects1 = 0x00AF1D08, PostShadow1 = 0x00AF1D18, PostShadow2 = 0x00AF1D28,
              UI = 0x00AF1D38, PostPrim = 0x00AF1D40, Occluder = 0x00AF1D50,
              MotionTrail = 0x00AF1D60, ThermalVision = 0x00AF1D70;
}  // namespace name

// Derived vtables (slots as in render.h; all share slots 0, 1, 3 with the
// base class 0x00B233E8):
//   layer::kVtable 0x00B23410  Opaque, PostShadow1/2, PostPrim, Occluder,
//                              MotionTrail, ThermalVision (Add 0x007A1120)
//   kVtableDepth    0x00B23434 Panorama, PreShadow, Translucent0..2,
//                              Effects1 (Add 0x007A1290: view-depth key, far first)
//   kVtableZ        0x00B23458 ZTranslucent (Add 0x007A1400, Draw 0x00514074)
//   kVtableUI       0x00B23480 UI (Add 0x007A1290)
//   kVtableResolve  0x00B23CA8 WaterResolve, ScreenWarp (192 bytes; Draw
//                              0x00514B0C)
constexpr u32 kVtableDepth = 0x00B23434;     // (name?)
constexpr u32 kVtableZ = 0x00B23458;         // (name?)
constexpr u32 kVtableUI = 0x00B23480;        // (name?)
constexpr u32 kVtableResolve = 0x00B23CA8;   // (name?)

// Layer fields beyond render.h's kBase / kCursor.
constexpr u32 kFlags = 12, kStencilRef = 13, kName = 16, kPreDraw = 20, kPostDraw = 88;
constexpr u32 kCapacity = 156, kHighWater = 160, kBytes = 164, kColour = 168;
constexpr u32 kCallbacks = 4, kCallbackSize = 16;
constexpr u8 kFlagsKeep = 0x3F;  // the constructor clears bits 7 (skip) and 6

namespace fn {
constexpr u32 PtrVector_Construct = 0x005516E0;  // (PtrVector*) (name?)
constexpr u32 PtrVector_Init = 0x00551598;       // (PtrVector*, capacity, heap, flags) (name?)
constexpr u32 Slot_Construct = 0x005485B4;  // native (rsx.h) (slot*) one of the 16 176-byte slots (name?)
constexpr u32 Pool_Construct = 0x00526D18;  // native (rsx.h) (pool*) (name?)
}  // namespace fn

namespace g {
// RenderLayers_Register
constexpr u32 max_capacity = 0x010DDAD8;  // u32: largest layer capacity registered (name?)
// RGBA u8[4] colour statics copied into the layers' +168.
constexpr u32 colour_ca0 = 0x01164CA0, colour_cac = 0x01164CAC, colour_cb0 = 0x01164CB0,
              colour_cb4 = 0x01164CB4, colour_cb8 = 0x01164CB8, colour_cc8 = 0x01164CC8;

// RenderLayers_Construct
constexpr u32 byte_da70 = 0x010DDA70;     // u8, set to 0xFF (name?)
constexpr u32 block_e061c = 0x010E061C;   // {u32 0, u32 32} (name?)
constexpr u32 block_dd5a8 = 0x010DD5A8;   // +4 / +8 u32 0, +12..+20 f32 1.0 (name?)
constexpr u32 atomic_ddc00 = 0x010DDC00;  // u32, atomically cleared (name?)
constexpr u32 block_ddb88 = 0x010DDB88;   // +24 u64 0, +32 u8 0 (name?)
constexpr u32 block_dd600 = 0x010DD600;   // 384 bytes; +256 points at +128 (name?)
constexpr u32 atomic_dd400 = 0x010DD400;  // u32, atomically cleared (name?)
constexpr u32 block_dd780 = 0x010DD780;   // +608 u32 0; +736 is render::g::stencil_bytes (name?)
constexpr u32 slots = 0x010DFAD0;         // 16 objects of 176 bytes (layer2.cpp kSlots)
constexpr u32 pool = 0x010DF8B0;          // +352 u32, +356 f32, +360 u32 (layer2.cpp kPool)
constexpr u32 pool_vector = 0x010DFA24;   // PtrVector of 4-byte elements (layer2.cpp kVecB)
constexpr u32 uv_to_clip = 0x010F0A50;    // f32[4][4]: (2u - 1, 1 - 2v, z); w column not written (name?)
constexpr u32 clip_to_uv = 0x010F0A90;    // f32[4][4]: (0.5x + 0.5, 0.5 - 0.5y, z, 1) (name?)
}  // namespace g

constexpr u32 kSlotCount = 16, kSlotSize = 176;
constexpr u32 kPoolVtable = 0x00B241E8;
constexpr u32 kConfigSize = 544, kBlockDd600Size = 384;

}  // namespace layers3
}  // namespace sly4::render

namespace sly4::render {
namespace layers3 {

// Derived-layer slot functions (decompiled in layers3.cpp) and their callees.
namespace fn {
constexpr u32 Mem_FreeFrom = 0x0054EE6C;            // (p, pool)
constexpr u32 ZLayer_DrawBody = 0x00513B78;  // native (rsx.h) (RenderLayer*) ZTranslucent's item pass (name?)
constexpr u32 ResolveLayer_Pass1 = 0x00514200;  // native (rsx.h) (RenderLayer*) WaterResolve / ScreenWarp (name?)
constexpr u32 ResolveLayer_Pass2 = 0x00514838;  // native (rsx.h) (RenderLayer*) (name?)
}  // namespace fn

namespace g {
constexpr u32 view_matrix = 0x010DD840;  // f32[4][4] rows: world -> view, read by the depth-key Adds (name?)
}  // namespace g

// {f32 bias scale 0.5, f32 near 50.0, f32 scale -1000.0} before each Add.
constexpr u32 kDepthAddConsts = 0x007A1280, kZAddConsts = 0x007A13F0;
constexpr u32 kDepthKeyMax = 0x00FFFFFF;
constexpr u32 kViewFlags = 256, kViewResolve = 0x8;  // View +256 bit 3: resolve layers draw

}  // namespace layers3
}  // namespace sly4::render
