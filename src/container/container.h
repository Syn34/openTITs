// PtrVector (a growable array with its own allocator) and the block
// allocation of ApeTFreeList<T>. Addresses are guest addresses in the
// BCES01284 v01.02 EBOOT. A "?" in a comment marks a guess.
#pragma once

#include <cstddef>

#include "opentit/native.h"
#include "../sly4.h"

namespace sly4::container {
using namespace opentit::native;

namespace cfn {
constexpr u32 PtrVector_Init = 0x00551598;     // (PtrVector*, u32 capacity, Allocator* or 0, bool flag)
constexpr u32 PtrVector_Reserve = 0x00551718;  // (PtrVector*, u32 capacity)
constexpr u32 FreeList_AddBlock = 0x005518FC;  // (FreeList*, u32 flag)
constexpr u32 memmove = 0x0071C3F0;            // (dst, src, n)
}  // namespace cfn

// An allocator object. Virtual functions (byte offsets into the vtable):
// +0 alloc(u32 size, u32 ?, u32 align) -> pointer, +8 free(void* p, u32 size).
struct Allocator {
    be_u32 vtable;  // +0
};
constexpr u32 kAllocVfnAlloc = 0;
constexpr u32 kAllocVfnFree = 8;
constexpr u32 kAllocAlign = 16;

struct PtrVector {
    gp<u8> data;               // +0
    be_u16 capacity;           // +4
    be_u16 elem_size;          // +6
    be_u16 count;              // +8
    be_u16 high_water;         // +10 largest count so far
    be_u32 bits;               // +12: bytes allocated << 3 | flags (bit 2 = Init's flag)
    gp<Allocator> allocator;   // +16
};
static_assert(offsetof(PtrVector, bits) == 12 && offsetof(PtrVector, allocator) == 16);
constexpr u32 kVecFlag = 0x4;

// The allocator a PtrVector gets when given none: a static object.
constexpr u32 kDefaultAllocatorPtr = 0x00F88704;    // Allocator*: &kDefaultAllocator once made
constexpr u32 kDefaultAllocator = 0x01165018;
constexpr u32 kDefaultAllocatorGuard = 0x01165010;  // u8
constexpr u32 kDefaultAllocatorVtbl = 0x00B24368;

// ApeTFreeList<T>. Items and blocks are linked both ways.
struct FreeItem {
    gp<FreeItem> next;  // +0
    gp<FreeItem> prev;  // +4
};
// A block: this header, then the items (16-byte header plus the element
// size rounded up to 16 each).
struct FreeBlock {
    gp<FreeBlock> newer;  // +0
    gp<FreeBlock> older;  // +4
    be_u32 bytes;         // +8: bytes after this header
    be_u32 flag;          // +12
};
struct FreeList {
    be_u32 vtable;              // +0
    gp<FreeItem> used_head;     // +4
    gp<FreeItem> used_tail;     // +8
    be_u32 used_count;          // +12
    gp<FreeItem> free_head;     // +16
    gp<FreeItem> free_tail;     // +20
    be_u32 free_count;          // +24
    gp<FreeBlock> block_tail;   // +28
    gp<FreeBlock> block_head;   // +32
    be_u32 block_count;         // +36
    gp<Allocator> heap;         // +40
    be_u32 elem_size;           // +44
    be_u32 used_peak;           // +48
    be_u16 items_per_block;     // +52
    be_u16 flags;               // +54: bit 15 cleared on construction
    be_u32 name;                // +56 const char*
};
static_assert(sizeof(FreeList) == 60);
static_assert(offsetof(FreeList, heap) == 40 && offsetof(FreeList, items_per_block) == 52);
constexpr u32 kBlockHeader = 16, kItemHeader = 16;

Allocator* DefaultAllocator_Get();  // the static default allocator, built on first use
void FreeList_AddBlock(FreeList* list, u32 flag);
void PtrVector_Init(PtrVector* vec, u32 capacity, Allocator* alloc, u32 flag);
void PtrVector_Reserve(PtrVector* vec, u32 capacity);

}  // namespace sly4::container
