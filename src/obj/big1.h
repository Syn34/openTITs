// Game code, large-function sweep part 1: helpers shared by big1_*.cpp
// (still recompiled game functions of 400+ instructions: the UI slot-def
// reader and factory, ...).
#pragma once

#include <bit>
#include <cmath>

#include "mid2.h"

namespace sly4 {
namespace big1 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using obj19::VCall;
using obj25::Sx;

namespace b1fn {
using namespace mid2::m2fn;
constexpr u32 XmlCursor_FindAttr = 0x004F9534;         // (cursor, hash) -> const char* or 0
constexpr u32 XmlCursor_FirstChildAny = 0x004F9140;    // (cursor) -> the first child element, or 0
constexpr u32 XmlCursor_NextSiblingAny = 0x004F8D74;   // (cursor) -> the next sibling element, or 0
constexpr u32 Obj_GetField4 = 0x00556BF0;            // (p) -> p->+4
constexpr u32 FreeList_Init = 0x00551A58;            // (list, elem_size, count, heap, bool)
constexpr u32 Tex_Acquire = 0x00523B7C;              // (Texture*) -> new AglMaterial on it
constexpr u32 String_Format = 0x005534D0;            // (fmt, ...) -> rotating static buffer
constexpr u32 String_HashNoCase = 0x0056A710;        // (s) -> u32
constexpr u32 WorldObj_FindByHash = 0x005801DC;      // (hash, kind) -> WorldObj* or 0
constexpr u32 WorldObj_FindById4A5AE8 = 0x004A5AE8;  // (WorldObj*, s32 id) -> WorldObj* (name?)
constexpr u32 WorldObj_SetName = 0x0057D368;         // (o, name, u8)
constexpr u32 Body_SetMode4A5C20 = 0x004A5C20;       // (WorldObj*, mode) (name?)
constexpr u32 Obj_CollectLodGroup = 0x00590FEC;      // (o, u8 recurse)
}  // namespace b1fn

// ---------------------------------------------------------------- materials

// The AglMaterial free list (an ApeTFreeList<AglMaterial> made on first
// use, its header at *kMatListSlot: +0 the allocator, +4 the per-block
// count) and its live nodes: the node at n - 16 {+0 next, +20 s16 use
// count, +36 the material's texture object, +56 the name hash}, walked by
// the n + 16 pointers the original uses.
constexpr u32 kMatListSlot = 0x00F8F114;
constexpr u32 kVtMatFreeList = 0x00B10ED0;
constexpr u32 kMatNodeSize = 144, kMatNodeBias = 16;

// The inlined ApeTListAllocator<AglMaterial>::GetAllocator(): a new 60-byte
// free list (vtable kVtMatFreeList, FreeList_Init(144, +4)) when there is
// none; stored back (0 when the allocation fails).
inline u32 MatAllocator(PPUContext& ctx, u32 name) {
    const u32 slot = load<u32>(kMatListSlot);
    const u32 al = load<u32>(slot);
    if (al != 0) return al;
    const u32 fl = static_cast<u32>(call(ctx, b1fn::Mem_Alloc, {60, 0, 16}));
    if (fl != 0) {
        const u32 per_block = load<u32>(slot + 4);
        store<u32>(fl + 8, 0);
        store<u32>(fl + 4, 0);
        store<u32>(fl + 12, 0);
        store<u32>(fl + 20, 0);
        store<u32>(fl + 16, 0);
        const u16 flags = load<u16>(fl + 54);
        store<u32>(fl + 24, 0);
        store<u32>(fl + 32, 0);
        store<u32>(fl + 28, 0);
        for (u32 k = 36; k <= 48; k += 4) store<u32>(fl + k, 0);
        store<u16>(fl + 52, 0);
        store<u16>(fl + 54, static_cast<u16>(flags & 0x7FFF));
        store<u32>(fl + 0, kVtMatFreeList);
        store<u32>(fl + 56, name);
        call(ctx, b1fn::FreeList_Init, {fl, kMatNodeSize, per_block, 0, 0});
    }
    store<u32>(slot, fl);
    return fl;
}
inline u32 MatFirst(PPUContext& ctx, u32 al) {
    const u32 n = static_cast<u32>(call(ctx, b1fn::Obj_GetField4, {al}));
    return n != 0 ? n + kMatNodeBias : 0;
}
inline u32 MatNext(u32 m) {
    const u32 n = load<u32>(m - kMatNodeBias);
    return n != 0 ? n + kMatNodeBias : 0;
}

// XmlCursor_Read* helpers whose default is the field's current value (the
// readers' usual pattern).
inline void ReadHash(PPUContext& ctx, u32 cur, u32 hash, u32 p) {
    call(ctx, b1fn::XmlCursor_ReadHash, {cur, hash, p, load<u32>(p)});
}
inline void ReadBool(PPUContext& ctx, u32 cur, u32 hash, u32 p) {
    call(ctx, b1fn::XmlCursor_ReadBool, {cur, hash, p, load<u8>(p)});
}
inline void ReadF32(PPUContext& ctx, u32 cur, u32 hash, u32 p) {
    callf(ctx, b1fn::XmlCursor_ReadF32, {cur, hash, p}, {std::bit_cast<f64>(LfsBits(load<u32>(p)))});
}
inline void ReadColor(PPUContext& ctx, u32 cur, u32 hash, u32 p) {
    call(ctx, b1fn::XmlCursor_ReadColor, {cur, hash, p, p});
}
// The string attribute `hash` copied into the 32-char field at p when present.
inline void ReadName(PPUContext& ctx, u32 cur, u32 hash, u32 p) {
    const u32 a = static_cast<u32>(call(ctx, b1fn::XmlCursor_FindAttr, {cur, hash}));
    if (a != 0) call(ctx, b1fn::String_Copy, {p, a, 32});
}

}  // namespace big1
}  // namespace sly4
