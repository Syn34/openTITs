// The renderer range's remaining functions of 19 or more instructions
// (gfx8*.cpp): destructors, list walkers, flag setters and slot wrappers.
#pragma once

#include "gfx7.h"

namespace sly4::render {
namespace gfx8 {

namespace fn {
// Still recompiled (called from gfx8*.cpp).
constexpr u32 FreeList_Destroy = 0x005568FC;  // (FreeList*)
constexpr u32 Holder_FreeData = 0x0055F27C;   // (Holder*)
constexpr u32 PtrVector_Clear = 0x0054EC1C;   // (PtrVector*)
constexpr u32 Nop = 0x0054EC18;               // blr, shared
constexpr u32 Mem_FreeFrom = 0x0054EE6C;      // (p, pool)
constexpr u32 Mem_FreePool = 0x0056C730;      // (p, pool, tag)
constexpr u32 EmitPropsRef_Set = 0x0079CB94;  // (AglEmitProperties** slot, p)
constexpr u32 Iff_ChunkSize = 0x005637D4;     // (IffReader*) -> u32
constexpr u32 FileReader_Read = 0x00563768;   // (FileReader*, void* buf, u32 size) -> u32
}  // namespace fn

}  // namespace gfx8
}  // namespace sly4::render
