// Allocating render surfaces (AglRenderSurface, from a free list) and the
// default and depth render targets.
#pragma once

#include "../sly4.h"

namespace sly4::surf {

namespace imp {
constexpr u32 cellGcmAddressToOffset = 0x00ABD5E4;
}  // namespace imp

namespace fn {
constexpr u32 Target_Construct = 0x0053044C;         // decompiled: src/gfx/surface_alloc.cpp
constexpr u32 Target_InitDepth = 0x00530A9C;         // decompiled: src/gfx/surface_alloc.cpp
constexpr u32 RenderSurface_Alloc = 0x0051AF28;      // decompiled: src/gfx/surface_alloc.cpp
constexpr u32 RenderSurface_Construct = 0x0051AB74;  // decompiled: src/gfx/surface_alloc.cpp
// Still recompiled.
constexpr u32 Surface_GetSize = 0x00530318;   // decompiled: src/gfx/init_surface.cpp
constexpr u32 Target_BindTile = 0x0052F32C;   // decompiled: src/gfx/tile.cpp
constexpr u32 Target_BindZcull = 0x00530998;  // decompiled: src/gfx/tile.cpp
constexpr u32 Mem_AllocPool = 0x0056C514;     // (align, size, pool, flags)
constexpr u32 FreeList_Init = 0x00551A58;     // (list, elem_size, count, heap, bool)
}  // namespace fn

namespace g {
constexpr u32 format_table = 0x00F67254;  // 24-byte entries: +8 bits per pixel, +20 u32
constexpr u32 tile_slots = 0x00F67244;    // u16: bit i set when tile slot i (0..13) is taken
constexpr u32 tag_cursor = 0x00F67248;    // u32: next compression tag, in 64 KiB units
constexpr u32 zcull_count = 0x00F6724C;   // u32: zcull regions handed out
constexpr u32 zcull_cursor = 0x00F67250;  // u32: next free zcull memory (limit 0x300000)
constexpr u32 surface_list = 0x00F8F110;  // AglRenderSurface free-list header: +0 allocator, +4 capacity
constexpr u32 surface_serial = 0x01164CD0;  // u32: last serial given to a render surface
// Vtables.
constexpr u32 vt_surface_list = 0x00B23D80;   // ApeTFreeList<AglRenderSurface>
constexpr u32 vt_surface_base0 = 0x00B242B8;  // written first by RenderSurface_Construct
constexpr u32 vt_surface_base = 0x00B234B0;
constexpr u32 vt_render_surface = 0x00B23F20;
constexpr u32 str_surface_list = 0x00AF1141;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglRenderSurface]"
}  // namespace g

}  // namespace sly4::surf
