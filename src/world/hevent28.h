// Shared names for src/world/hevent28_*.cpp: more of the World event
// handler's remaining callees (geometry level load, camera constructors).
// Guest addresses in the BCES01284 v01.02 EBOOT; "(name?)" marks a guess.
#pragma once

#include <cmath>

#include "hevent27.h"
#include "hevent6.h"

namespace sly4 {
namespace hevent28 {

namespace h28fn {
constexpr u32 GeomInstances_HookEffects = 0x004F23C4;  // (World*) (hevent27_b.cpp)
constexpr u32 Geom_BindClassTextures = 0x004F0000;     // (-, Scene*) (hevent27_b.cpp)
constexpr u32 Geom_LoadScene = 0x004F05A8;             // (World*, Scene*, u32, bool first, bool top) (name?)
constexpr u32 Bank_Open = 0x005904EC;                  // (bank, name, 0)
constexpr u32 Set_Find = 0x004BCE90;                   // (u32 name) -> Set*
constexpr u32 Set_AddXmlPath = 0x004BDD08;             // (Set*, const char* path, u8)
constexpr u32 CameraInstance_New = 0x005A59AC;         // (def) -> camera
constexpr u32 CameraDefMap_Find = 0x0073374C;          // (map, const u32* key) -> value slot
constexpr u32 Mat34_FromRotPos = 0x00553B4C;           // (Mat34* out, const Mat33*, const vec3*)
constexpr u32 String_HashNoCase = 0x0056A710;          // (const char*) -> u32
constexpr u32 WorldObj_MarkDirty = 0x0057D000;         // (obj)
constexpr u32 Obj_InstanceName = 0x000CBA40;           // (Obj*) -> name (Obj_InstanceName0CBA40)
constexpr u32 SpawnQuery_Init = 0x000CC4D4;            // (q, name, prof, a3, m, pos, u8, u8, stack 8 / 10; f1) (Wrapper_0CC4D4)
constexpr u32 SmallAlloc = 0x0055238C;                 // (size) -> block or 0
constexpr u32 SmallFree = 0x00552420;                  // (p, size)
constexpr u32 PtrVector_Construct = 0x005516E0;        // (vec)
constexpr u32 PtrVector_Clear = 0x0054EC1C;            // (vec)
constexpr u32 Nop = 0x0054EC18;                        // (shared empty function)
constexpr u32 String_Copy = 0x00553684;                // (dst, src, size)
constexpr u32 Vector_Add = 0x00733964;                 // (Vector*, const Rec* 192 bytes) (Vector_Add733964)
}  // namespace h28fn

// PPU single-precision forms (computed in double, rounded to float; fmuls
// is hevent::fmuls).
inline f32 fsubs(f64 a, f64 b) { return static_cast<f32>(a - b); }
inline f32 fadds(f64 a, f64 b) { return static_cast<f32>(a + b); }
inline f32 fdivs(f64 a, f64 b) { return static_cast<f32>(a / b); }
inline f32 fsqrts(f64 a) { return static_cast<f32>(std::sqrt(a)); }
inline f32 fmadds(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }   // a * c + b
inline f32 fmsubs(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, -b)); }  // a * c - b

// Map of scene nodes by name hash (hevent6.h map layout at 0x010C2CB8,
// i.e. +12 of the object at 0x010C2CAC); last value found at 0x011DE48C
// (as hevent22_e.cpp).
constexpr u32 kNodeMap = 0x010C2CAC + 12;
constexpr u32 kNodeMapLast = 0x011DE48C;

// The inlined map lookup: the node for key, or 0. A hit stores it as
// current and its value in the global; a miss stores the last node visited
// as current; an empty map clears the global, then current.
inline u32 node_lookup(u32 key) {
    using namespace hevent6;
    const u32 root = load<u32>(load<u32>(kNodeMap));
    if (root == 0) {
        store<u32>(kNodeMapLast, 0);
        store<u32>(kNodeMap + kMapNode, 0);
        return 0;
    }
    const u32 n = map_walk(kNodeMap, root, key);
    if (n == 0) return 0;
    store<u32>(kNodeMap + kMapNode, n);
    store<u32>(kNodeMapLast, load<u32>(n + kNodeValue));
    return n;
}

// Camera definitions: map at (*0x00F8F134) +16, and the
// ApeTListAllocator<ApeCameraDef> slot {+0 allocator, +4 nodes per block}
// at *0x00F8F134 itself.
constexpr u32 kCamDefs = 0x00F8F134;
constexpr u32 kFreeListVtbl = 0x00B140D0;
constexpr u32 kCamDefNodeSize = 192;

// ApeTListAllocator<ApeCameraDef>::GetAllocator() (inlined): the free list
// is created on first use; a failed allocation stores 0 and is returned.
// name: the caller's copy of the "ApeTFreeList<T> &ApeTListAllocator<T>::
// GetAllocator() [with T = ApeCameraDef]" string (one per object file).
inline u32 camdef_allocator(PPUContext& ctx, u32 name) {
    const u32 slot = load<u32>(kCamDefs);
    const u32 alloc = load<u32>(slot);
    if (alloc != 0) return alloc;
    const u32 fl = static_cast<u32>(call(ctx, hevent27::h27fn::Mem_Alloc, {60, 0, 16}));
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
        store<u32>(fl + 0, kFreeListVtbl);
        store<u32>(fl + 56, name);
        call(ctx, hevent27::h27fn::FreeList_Init, {fl, kCamDefNodeSize, per_block, 0, 0});
    }
    store<u32>(slot, fl);
    return fl;
}

}  // namespace hevent28
}  // namespace sly4
