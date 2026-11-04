// World objects, part 2: construction, dirty marking, class copies, the
// light free list, and the four class maps' lookups.
#pragma once

#include "obj.h"

namespace sly4 {
namespace obj2 {

using namespace opentit;

namespace o2fn {
constexpr u32 Mem_Alloc = 0x0054ED38;
constexpr u32 Mem_FreeFrom = 0x0054EE6C;      // (block, 0)
constexpr u32 FreeList_Init = 0x00551A58;     // (list, elem_size, count, heap, bool)
constexpr u32 String_Assign = 0x00555F80;     // (String* dst, const String* src)
constexpr u32 WorldObj_InitTransform = 0x0057D688;  // (obj) (name?)
constexpr u32 WorldObj_InitLinks = 0x00580658;      // (obj) (name?)
constexpr u32 WorldClass_Alloc = 0x007A71A4;        // () -> a new class (name?)
constexpr u32 Topology_Copy = 0x005095C0;           // (AglTopology*, CopyTable*) -> copy (name?)
constexpr u32 WorldRes_Release = 0x00584DFC;        // (res) drops a reference (name?)
}  // namespace o2fn

// ---------------------------------------------------------------- base object (WorldObj_Construct)
constexpr u32 kObjWord4 = 4;        // u32: high half = 1 (ref count?), bits 10-15 = kind
constexpr u32 kObjWord8 = 8;        // u32: bits 11-31 serial, 0x400 / 0x3C0 cleared
constexpr u32 kObjFlags322 = 322;   // u8: 0x20 set on construction
constexpr u32 kObjWord284 = 284;    // u16: 0xFFFF on construction
constexpr u32 kObjSerial = 0x01164CD0;  // u32: bumped by every construction
constexpr u32 kVtblObjBase0 = 0x00B242B8, kVtblObjBase1 = 0x00B24610, kVtblObjBase2 = 0x00B24660;

// ---------------------------------------------------------------- world class (WorldClass_Copy)
constexpr u32 kClassName = 64;          // String
constexpr u32 kClassShared = 108;       // {u16 refs, u16, void* data}*: freed with the last reference
constexpr u32 kClassTopoCount = 112;    // u8
constexpr u32 kClassTopoIndex = 115;    // u8: re-copied entry (name?)
constexpr u32 kClassRes = 164;          // {u16 refs, ...}*: released through WorldRes_Release
constexpr u32 kClassTopos = 132;        // {AglTopology*, f32}[kClassTopoCount]
constexpr u32 kCopyTableCount = 1280;   // u32 in the 1344-byte table Topology_Copy fills

// ---------------------------------------------------------------- lights (Light_Alloc)
constexpr u32 kLightList = 0x010DDC84;  // AglLight free-list header*: +0 allocator, +4 capacity
constexpr u32 kLightListVtbl = 0x00B23E60;
constexpr u32 kStrLightList = 0x00AF13BC;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglLight]"
constexpr u32 kLightSize = 240;
constexpr u32 kLightSerial = 0x011DE4BC;   // u8
constexpr u32 kConfigLightSlots = 0x010C5A20 + 153;  // u8 in g::config (name?)

// ---------------------------------------------------------------- class maps
constexpr u32 kMapCompareA = 0x00F3CC48, kMapCompareB = 0x00F3CBB8;
constexpr u32 kMapCompareC = 0x00F3A9B8, kMapCompareD = 0x00F3A9C0;

}  // namespace obj2
}  // namespace sly4
