// PtrVector (a growable array with its own allocator) and the block
// allocation of ApeTFreeList<T>. Addresses are guest addresses in the
// BCES01284 v01.02 EBOOT. A "?" in a comment marks a guess.
#pragma once

#include "../sly4.h"

namespace sly4 {
namespace container {

namespace cfn {
constexpr u32 PtrVector_Init = 0x00551598;     // (PtrVector*, u32 capacity, Allocator* or 0, bool flag)
constexpr u32 PtrVector_Reserve = 0x00551718;  // (PtrVector*, u32 capacity)
constexpr u32 FreeList_AddBlock = 0x005518FC;  // (FreeList*, u32 flag)
constexpr u32 memmove = 0x0071C3F0;            // (dst, src, n)
}  // namespace cfn

// PtrVector
constexpr u32 kVecData = 0;       // u32 pointer
constexpr u32 kVecCapacity = 4;   // u16
constexpr u32 kVecElemSize = 6;   // u16
constexpr u32 kVecCount = 8;      // u16
constexpr u32 kVecHigh = 10;      // u16 high-water mark
constexpr u32 kVecBits = 12;      // u32: bytes allocated << 3 | flags (bit 2 = Init's flag)
constexpr u32 kVecAllocator = 16; // u32 Allocator*
constexpr u32 kVecFlag = 0x4;

// Allocator virtual functions (byte offsets into the vtable).
constexpr u32 kAllocVfnAlloc = 0;  // (u32 size, u32 ?, u32 align) -> pointer
constexpr u32 kAllocVfnFree = 8;   // (void* p, u32 size)
constexpr u32 kAllocAlign = 16;

// The allocator a PtrVector gets when given none: a static object.
constexpr u32 kDefaultAllocatorPtr = 0x00F88704;    // u32: &kDefaultAllocator once made
constexpr u32 kDefaultAllocator = 0x01165018;
constexpr u32 kDefaultAllocatorGuard = 0x01165010;  // u8
constexpr u32 kDefaultAllocatorVtbl = 0x00B24368;

// ApeTFreeList<T> fields used by FreeList_AddBlock.
constexpr u32 kFlFreeHead = 16, kFlFreeTail = 20, kFlFreeCount = 24;  // items: { next, prev, ... }
constexpr u32 kFlBlockTail = 28, kFlBlockHead = 32, kFlBlockCount = 36;
constexpr u32 kFlHeap = 40, kFlElemSize = 44, kFlItemsPerBlock = 52;  // u16
// A block: { newer, older, u32 bytes after the header, u32 flag } then the items.
constexpr u32 kBlockHeader = 16, kItemHeader = 16;

inline u32 vfunc(u32 obj, u32 byte_offset) { return load<u32>(load<u32>(obj) + byte_offset); }

}  // namespace container
}  // namespace sly4
