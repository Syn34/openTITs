// ApeDebug: the debug font, primitive pools and meshes Debug_Init builds
// (Main_InitSystems stage 8; the retail game still runs it).
#pragma once

#include "opentit/native.h"
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

struct DebugFont;  // 44 bytes
struct Mesh;
// Blend state at Material+104.
struct MaterialBlend {
    be_u32 w0;         // +0 bit 31 enable, bits 15-30 source factor
    be_u16 equation;   // +4
    be_u16 dst;        // +6 destination factor
    be_u32 w8;         // +8 bit 31 (meaning unknown)
};
struct Material {
    native::pad<104> _0;
    MaterialBlend blend;  // +104
};

// The ApeDebug state at 0x01164A88.
struct DebugState {
    u8 enabled;                     // +0 the word is cleared, then this byte set to 1
    u8 pad1[3];
    native::gp<DebugFont> font;     // +4  DebugFont (44 bytes) or 0
    native::gp<Material> material;  // +8  alpha-blended debug material
    native::gp<Mesh> sphere;        // +12 8x8 sphere mesh
    native::gp<Mesh> cube;          // +16 cube mesh
};
static_assert(sizeof(DebugState) == 20);
constexpr u32 kDebugState = 0x01164A88;

bool Debug_Init();

}  // namespace sly4::debug
