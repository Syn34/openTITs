// Decoded images (BMP / TGA) and the AglTexture fields they are uploaded
// into (Texture_FromImage / Texture_FromGtf).
#pragma once

#include "../sly4.h"

namespace sly4 {
namespace image {

using namespace opentit;

namespace ifn {
constexpr u32 strcasecmp = 0x00723F2C;          // libc (name?)
constexpr u32 Texture_ConstructBase = 0x0050A3E4;  // (AglTexture*) (name?)
constexpr u32 Mem_GetDefaultStack = 0x005525FC;
constexpr u32 MemStack_Alloc = 0x0055260C;      // (stack, size)
constexpr u32 Mem_AllocPool = 0x0056C514;       // (align, size, pool, tag)
constexpr u32 memset = 0x00726E54;
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 Texture_InitLayout = 0x00536E3C;  // (layout, w, h, 1, 3, 0, 0, 0) (name?)
constexpr u32 Rsx_Copy = 0x0056E2EC;            // (src, dst, size, 1) (name?)
constexpr u32 cellGcmAddressToOffset = 0x00ABD5E4;  // import stub
}  // namespace ifn

// Image (20 bytes): what Image_FromBmp / Image_FromTga fill in.
constexpr u32 kImgPixels = 0;     // u8* 32-bit pixels, from the default memory stack
constexpr u32 kImgWidth = 4;      // u16
constexpr u32 kImgHeight = 6;     // u16
constexpr u32 kImgBpp = 8;        // u16: the file's bits per pixel (32 for 24-bit BMPs)
constexpr u32 kImgPalette = 12;   // u8* 4 bytes per entry (palettised BMPs)
constexpr u32 kImgPalCount = 16;  // u16
constexpr u32 kImgFlags = 18;     // u16: 0x1 keep palette alpha, 0x2 buffers not owned

// AglTexture (132 bytes) fields.
constexpr u32 kTexVtbl = 0x00B23EE0;
constexpr u32 kTexWidth = 52, kTexHeight = 54;  // u16
constexpr u32 kTexData = 56;       // void*: pixels / GTF file
constexpr u32 kTexDataOffset = 60; // u32: RSX offset << 1
constexpr u32 kTexWord64 = 64;     // u32: bits 4-31 size, bit 0 in-place (not copied)
constexpr u32 kTexWord68 = 68;     // u32: top nibble = 1
constexpr u32 kTexGcm = 72;        // CellGcmTexture (24 bytes): +8 width, +10 height, +14 location, +20 offset
constexpr u32 kTex116 = 116, kTex118 = 118, kTex120 = 120, kTexFlags128 = 128;  // u16
constexpr u32 kTextureUploaded = 0x011DE4BD;  // u8: set by the first image upload (name?)

}  // namespace image
}  // namespace sly4
