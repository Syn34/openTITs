// The renderer range's remaining functions of 45 or more instructions
// (gfx9*.cpp): destructor bodies, a handle release, the IFF include
// reader, a worker thread loop and the query-slot flush.
#pragma once

#include "gfx8.h"

namespace sly4::render {
namespace gfx9 {

namespace fn {
// Decompiled in gfx9.cpp.
constexpr u32 Material_DestructBody = 0x0050C118;      // (Material*)
constexpr u32 Texture_DestructBody = 0x0050A5E4;       // (Texture*)
constexpr u32 RenderTarget_DestructBody = 0x0052163C;  // (RenderTarget*)
constexpr u32 Handle_Release = 0x00537684;             // (Handle** slot)
constexpr u32 Iff_ReadInclude = 0x00569638;            // (IffReader*, char* path_out or 0) -> bool

// Still recompiled (called from gfx9*.cpp).
constexpr u32 ApeRoot_Destruct = 0x00558CE8;          // (obj)
constexpr u32 Pool_DestructPart = 0x0052EFE0;         // (part*)
constexpr u32 Release_0052F06C = 0x0052F06C;          // (x) before Mem_FreeFrom(x, 0) (name?)
constexpr u32 SmallFree = 0x00552420;                 // (p, u32 size)
constexpr u32 FreeList_Init = 0x00551A58;             // (list, elem_size, count, heap, bool)
constexpr u32 RenderTarget_SetViewport = 0x005200AC;  // (rt, ViewRect*)
constexpr u32 Draw_InvalidateState = 0x0051279C;
constexpr u32 Query_Begin_005488AC = 0x005488AC;      // Call_005488AC (name?)
constexpr u32 Query_End_00548900 = 0x00548900;        // Call_00548900 (name?)
constexpr u32 Iff_ReadChunks = 0x00569388;            // (IffReader*, obj)
constexpr u32 LFQueue_Detach_0060D988 = 0x0060D988;   // Edge zlib: cellSpursLFQueueDetachLv2EventQueue + assert
constexpr u32 Libc_strlen = 0x0071ED08;
constexpr u32 Libc_strncpy = 0x00723C88;
constexpr u32 Libc_strcpy = 0x007239D8;
constexpr u32 Libc_strcat = 0x0072354C;               // Libc_72354C (strcat?)
// Imports.
constexpr u32 cellGcmUnbindTile = 0x00ABD884;
constexpr u32 cellGcmUnbindZcull = 0x00ABD7C4;
constexpr u32 cellSpursEventFlagDetachLv2EventQueue = 0x00ABDF84;
constexpr u32 cellSpursShutdownTaskset = 0x00ABE144;
constexpr u32 cellSpursJoinTaskset = 0x00ABE104;
}  // namespace fn

}  // namespace gfx9
}  // namespace sly4::render
