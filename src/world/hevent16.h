// Shared names for src/world/hevent16_*.cpp: callees named by the
// hevent15_*.cpp functions and more of the World event handler's remaining
// callees. Guest addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include "hevent15.h"

namespace sly4 {
namespace hevent16 {

namespace h16fn {
constexpr u32 Obj_FindComponent = 0x000D1288;  // (Object*, u32 type, 0) -> component or 0 (symbols.txt)
constexpr u32 String_Copy = 0x00553684;        // (dst, src, size): always terminates (sly4.h)
}  // namespace h16fn

// The layer stack (as hevent3_i.cpp / hevent9_f.cpp): *0x00F65858 16-byte
// entries {+0 Layer*, +4 Object*, +8 u32 handle}, u32 count at 0x010AA690.
constexpr u32 kLayerEntries = 0x00F65858, kLayerEntryCount = 0x010AA690, kLayerEntrySize = 16;
constexpr u32 kEntryObject = 4, kEntryHandle = 8;

}  // namespace hevent16
}  // namespace sly4
