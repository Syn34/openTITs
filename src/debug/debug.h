// ApeDebug: the debug font, primitive pools and meshes Debug_Init builds
// (Main_InitSystems stage 8; the retail game still runs it).
#pragma once

#include "../sly4.h"

namespace sly4::debug {

namespace fn {
constexpr u32 Debug_Init = 0x0056A944;  // decompiled: src/debug/debug.cpp
// Still recompiled.
constexpr u32 Texture_LoadFromMemory = 0x0051D91C;  // (data, size, "Bmp", "MEM:...") -> texture (name?)
constexpr u32 DebugFont_Construct = 0x00565410;     // (font*, const metrics*, texture) (name?)
constexpr u32 Shader_Find = 0x004FEBD0;             // (u32 hash) -> shader (name?)
constexpr u32 Material_Create = 0x004FF43C;         // (shader) -> material; stores the shader at +16 (name?)
constexpr u32 Mesh_CreateSphere = 0x0051BC20;       // (material, rings, segments) -> mesh "SphereIdx" (name?)
constexpr u32 Mesh_CreateCube = 0x0051C1A8;         // (material, 1) -> mesh "Cube" (name?)
}  // namespace fn

// The ApeDebug state at 0x01164A88.
struct DebugState {
    u8 enabled;        // +0 the word is cleared, then this byte set to 1
    u8 pad1[3];
    gptr font;         // +4  DebugFont (44 bytes) or 0
    gptr material;     // +8  alpha-blended debug material
    gptr sphere;       // +12 8x8 sphere mesh
    gptr cube;         // +16 cube mesh
};
static_assert(sizeof(DebugState) == 20);
constexpr u32 kDebugState = 0x01164A88;

}  // namespace sly4::debug
