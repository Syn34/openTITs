// World objects, part 3: the remaining WorldObj vtable slots and the
// helpers the constructor / destructor / release use.
#pragma once

#include "obj2.h"

namespace sly4 {
namespace obj3 {

using namespace opentit;

namespace o3fn {
constexpr u32 Texture_ReleaseRef = 0x0050CAE8;   // (obj +288) (name?)
constexpr u32 WorldObj_FreeExtra300 = 0x005807D8; // (obj +300) (name?)
constexpr u32 WorldObj_DetachAll = 0x0057DA68;   // (obj) (name?)
constexpr u32 WorldObj_SetParent = 0x0057D8AC;   // (obj, parent) (name?)
constexpr u32 WorldObj_DestructRoot = 0x00558CE8; // (obj) (name?)
constexpr u32 WorldObj_RefreshSelf = 0x00580094; // (obj, s32 depth, flags) (name?)
constexpr u32 WorldObj_RefreshTree = 0x00584160; // (obj, s32 depth) (name?)
}  // namespace o3fn

constexpr u32 kVtblBase1 = 0x00B24610, kVtblBase2 = 0x00B24660;
constexpr u32 kSlotTable = 0x01193004;        // SlotEntry* (128 bytes each): +96 the object
constexpr u32 kSlotPeers = 0x01194598;        // **: 160-byte records indexed by object +164 (u16); +144 = slot entry (name?)
constexpr u32 kKindList = 0x011944C4;         // PtrVector of kind-8 / kind-10 objects: +0 data, +8 u16 count
constexpr u32 kObjFlags164 = 164;             // u32: 0x8000 hidden (name?), 0x400 queued
constexpr u32 kObjByte321 = 321;              // u8: 0x02 = in the kind list
constexpr u32 kObjExtra288 = 288, kObjExtra300 = 300;
constexpr u32 kObjColor = 316;                // u8[4]
constexpr u32 kObjU16_328 = 328;

}  // namespace obj3
}  // namespace sly4
