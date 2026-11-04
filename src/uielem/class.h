// The element classes: vtables, the base class's own slots, and the
// shared helpers their constructors / destructors use.
#pragma once

#include "ctor.h"

namespace sly4 {
namespace uielem {

// Element vtables, 200 bytes apart (50 slots, the last three zero).
constexpr u32 kVtblBase = 0x00B22710;
constexpr u32 kVtblText = 0x00B227D8;
constexpr u32 kVtbl920 = 0x00B22920;   // clone 004E4BF8 (name?)
constexpr u32 kVtbl9E8 = 0x00B229E8;   // clone 004E53F0 (name?)
constexpr u32 kVtblAB0 = 0x00B22AB0;   // clone 004E6414 (name?)
constexpr u32 kVtblText5 = 0x00B22B78; // type 5, derived from UiText
constexpr u32 kVtblC40 = 0x00B22C40;   // clone 004E7D60 (name?)
constexpr u32 kVtblD08 = 0x00B22D08;   // clone 004E8460 (name?)
constexpr u32 kVtblDD0 = 0x00B22DD0;   // clone 004E9144 (name?)
constexpr u32 kVtblE98 = 0x00B22E98;   // clone 004E9424 (name?)

namespace clfn {
constexpr u32 UiElem_Destruct = 0x004DFF1C;
constexpr u32 PureVirtual_Abort = 0x00732D44;  // calls a handler if one is set, then aborts (name?)
constexpr u32 HashMap_Find = 0x007948E4;
constexpr u32 Mem_FreeFrom = 0x0054EE6C;
}  // namespace clfn

constexpr u32 kVfnGetSize = 140;     // (sret f32[2])
constexpr u32 kVfnInvalidate = 148;  // ()
constexpr u32 kVfnDraw = 156;        // ()
constexpr u32 kVfnResType = 8;       // resource: () -> s32
constexpr u32 kUiResources = 0x010C2AF0;  // HashMap of the base FindResource (vtbl.h kResourcesB)
constexpr u32 kUiFocusSlots = 0x010C2AD8, kUiFocusSlotCount = 4;
constexpr u32 kUiRoots0 = 0x00F66868;

// The base destructor's body (also inlined by every deleting destructor):
// deletes the children, unlinks the element unless detached, and clears
// it from the focus slots. Leaves r3..r8 as the original does.
void ElemDestructBody(PPUContext& ctx, u32 e);

}  // namespace uielem
}  // namespace sly4
