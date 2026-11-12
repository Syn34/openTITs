// AglTexture construction and its surface layout (Texture_InitLayout).
#pragma once

#include <cstddef>

#include "opentit/native.h"
#include "../sly4.h"

namespace sly4 {
namespace image2 {

using namespace opentit;

namespace i2fn {
constexpr u32 Texture_ConstructRoot = 0x0050A2F0;  // (AglTexture*) (name?)
constexpr u32 Target_Init = 0x00530DB4;
constexpr u32 Surface_SetBuffer = 0x00532D5C;
constexpr u32 Mem_AllocPool = 0x0056C514;          // (align, size, pool, tag)
constexpr u32 cellGcmAddressToOffset = 0x00ABD5E4;
}  // namespace i2fn

// Texture layout (AglTexture +56).
constexpr u32 kLayData = 0, kLayOffset = 4;  // +4: RSX offset << 1 | bit 0 (pool 5)
constexpr u32 kLaySize = 8;                  // u32: bits 4-31 size
constexpr u32 kLayWord12 = 12;               // u32: top nibble = 1
constexpr u32 kLayFormat = 16, kLayMips = 17, kLayDim = 18, kLayCube = 19;  // u8 (CellGcmTexture-like)
constexpr u32 kLayRemap = 20, kLayWidth = 24, kLayHeight = 26, kLayDepth = 28;
constexpr u32 kLayLocation = 30, kLayPitch = 32, kLayGcmOffset = 36, kLayFormatIndex = 44;
constexpr u32 kFormatTable = 0x00F67254;     // 24-byte entries: +4 gcm format (0x20 = linear), +8 bits per pixel, +16 remap

// A texture's surface layout (AglTexture +56, 48 bytes).
struct TexLayout {
    be_u32 data;         // +0 memory (pixels / GTF file)
    be_u32 offset;       // +4 RSX offset << 1 | bit 0 (pool 5 / in place)
    be_u32 size;         // +8 bits 4-31 size
    be_u32 word12;       // +12 top nibble = 1
    u8 format;           // +16 (CellGcmTexture from here, 24 bytes)
    u8 mips;             // +17
    u8 dim;              // +18
    u8 cube;             // +19
    be_u32 remap;        // +20
    be_u16 width;        // +24
    be_u16 height;       // +26
    be_u16 depth;        // +28
    u8 location;         // +30
    u8 b31;              // +31
    be_u32 pitch;        // +32
    be_u32 gcm_offset;   // +36
    native::pad<4> _40;
    be_u32 format_index; // +44
};
static_assert(sizeof(TexLayout) == 48 && offsetof(TexLayout, gcm_offset) == kLayGcmOffset &&
              offsetof(TexLayout, format_index) == kLayFormatIndex);

// Native functions (src/font/image2.cpp).
u32 Texture_ConstructBase(void* t);
u32 Texture_InitLayout(TexLayout* l, u32 w, u32 h, u32 mips, u32 fmt, u32 target, be_u32* out, u32 pool5);

}  // namespace image2
}  // namespace sly4
