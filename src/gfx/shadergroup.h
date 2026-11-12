// Shader groups: the six built-in materials made from shaders looked up by
// name hash, the shader registry they come from, and AglMaterial
// allocation from its free list.
#pragma once

#include "../sly4.h"

namespace sly4::shgroup {

namespace fn {
constexpr u32 Shader_LoadGroup = 0x0050F0F0;   // native: src/gfx/shadergroup.cpp
constexpr u32 Shader_FindByHash = 0x004FE990;  // native: src/gfx/shadergroup.cpp
constexpr u32 Material_Alloc = 0x004FF2E8;     // native: src/gfx/shadergroup.cpp
// Still recompiled.
constexpr u32 Material_Construct = 0x004FF0D8;  // (AglMaterial*) (name?)
constexpr u32 PtrVector_Construct = 0x005516E0;
constexpr u32 PtrVector_Init = 0x00551598;      // (vector, capacity, 0, 0) as used here
constexpr u32 FreeList_Init = 0x00551A58;       // (list, elem_size, count, heap, bool)
}  // namespace fn

namespace g {
constexpr u32 groups = 0x010F0C60;           // AglMaterial*[6]
constexpr u32 registry_made = 0x010F0BA0;    // u8
constexpr u32 registry = 0x010F0BA8;         // PtrVector* of shaders: +0 data, +8 u16 count; shader +8 = name hash
constexpr u32 group_textures = 0x010DDADC;   // two texture pointers for groups 4 and 5 (name?)
constexpr u32 material_list = 0x00F8F114;    // AglMaterial free-list header: +0 allocator, +4 capacity
constexpr u32 vt_material_list = 0x00B10ED0;  // ApeTFreeList<AglMaterial>
constexpr u32 vt_material = 0x00B233D0;
constexpr u32 str_material_list = 0x00AF1193;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglMaterial]"
}  // namespace g

}  // namespace sly4::shgroup
