// Base UI element virtual functions with real bodies (vtable 0x00B1C9D0).
#pragma once

#include "../text/text.h"

namespace sly4 {
namespace uielem {

using namespace sly4::text;

constexpr u32 kVfnSize = 140;         // sret f32[2]
constexpr u32 kVfnWorldMatrix = 164;  // sret f32[16]
constexpr u32 kElemMatrix = 16;       // 3x3 at +16 (rows 16 bytes apart)
constexpr u32 kElemPosition = 64;     // f32[3]
constexpr u32 kElemInheritScale = 0x10000000;
constexpr u32 kScreenMatrix = 0x01164BD0;  // f32[16] used for elements without a parent

}  // namespace uielem
}  // namespace sly4

namespace sly4 {
namespace uielem {

namespace vfn {
constexpr u32 UiElem_Invalidate = 0x004E22DC;          // vfunc +148
constexpr u32 UiElem_InvalidateLocal = 0x004E2178;     // (Element*) 89 instructions, still recompiled (name?)
constexpr u32 Matrix_FromAxisAngle = 0x0056A410;       // (f32 out[12], const f32 axis[3]; f1 = radians) (name?)
constexpr u32 HashMap_Find = 0x007948E4;               // (Map*, const u32* key) -> value slot (name?)
constexpr u32 UiText_Destruct = 0x004E1604;            // (TextElem*) 116 instructions (name?)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;               // (void*, s32 pool)
}  // namespace vfn

constexpr u32 kVfnColor = 172;          // sret u8[4]
constexpr u32 kVfnResourceType = 8;     // resource () -> s32 type
constexpr u32 kElemColor = 132;         // u8[4] RGBA
constexpr u32 kElemInheritColor = 0x08000000;  // +136: multiply rgb by the parent's
constexpr u32 kElemInheritAlpha = 0x04000000;  // +136: multiply alpha by the parent's
constexpr u32 kElemLayoutPending = 0x40000000; // +180 (text): set by Invalidate, cleared by UiText_Layout
constexpr u32 kElemSizeDirty = 0x02000000;     // +136: UiElem_GetSize measures again
constexpr u32 kElemFirstChild = 152, kElemNext = 156;
constexpr u32 kRotationAxis = 0x01164B70;      // f32[3]
constexpr u32 kResourcesA = 0x010C2B08;        // HashMap name hash -> resource, searched first (name?)
constexpr u32 kResourcesB = 0x010C2AF0;        // HashMap, searched second (name?)

}  // namespace uielem
}  // namespace sly4
