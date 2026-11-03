// Shader start-up (called from Graphics_Init): the material chunk types and
// the shader archive (System\Shaders.Zip or a preloaded copy).
#pragma once

#include "../sly4.h"

namespace sly4::shaders {

namespace fn {
constexpr u32 Shaders_Init = 0x0050F308;  // decompiled: src/gfx/shaders.cpp
// Still recompiled.
constexpr u32 Iff_RegisterChunk = 0x0055902C;
constexpr u32 Archive_MountMemory = 0x00567F2C;  // (const void* data, const char* prefix, u32 buffer) -> archive index or 0 (name?)
constexpr u32 Archive_MountFile = 0x0055D684;    // (const char* path, const char* prefix, u32 buffer) -> archive index or 0 (name?)
constexpr u32 Archive_Unmount = 0x0055E350;      // (u16 archive index)
constexpr u32 Shaders_Init50B138 = 0x0050B138;   // () (unknown)
constexpr u32 PostProc_Init = 0x00534310;        // () "PostProc_Resolve2xMSAA" (name?)
constexpr u32 PostProc_InitCopy = 0x00546F6C;    // () "PostProc_Copy", 1312 instructions (name?)
constexpr u32 StencilShadow_Init = 0x0054BF48;   // () "StencilShadow" (name?)
constexpr u32 Shaders_Init54CDB0 = 0x0054CDB0;   // () once (unknown)
constexpr u32 Shader_LoadGroup = 0x0050F0F0;     // (s32 group 0..5) (name?)
}  // namespace fn

constexpr u32 kShaderPool = 0x00F8F114;      // 16 bytes { 0, count (config +348, 256), 0 }
constexpr u32 kConfigShaderCount = 348;      // s16 in g::config
constexpr u32 kConfigShaderData = 84;        // const void*: preloaded Shaders.Zip, or 0
constexpr u32 kConfigShaderPath = 88;        // const char*: shader archive path, or 0
constexpr u32 kShaderGroups = 6;

}  // namespace sly4::shaders
