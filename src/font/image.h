// Decoded images (BMP / TGA) and the AglTexture fields they are uploaded
// into (Texture_FromImage / Texture_FromGtf).
#pragma once

#include <cstddef>

#include "opentit/native.h"
#include "../sly4.h"
#include "image2.h"

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

// ---------------------------------------------------------------- native structures
struct Image {
    native::gp<u8> pixels;    // +0 32-bit pixels, from the default memory stack
    be_u16 width;             // +4
    be_u16 height;            // +6
    be_u16 bpp;               // +8 the file's bits per pixel (32 for 24-bit BMPs)
    native::pad<2> _10;
    native::gp<u8> palette;   // +12 4 bytes per entry (palettised BMPs)
    be_u16 pal_count;         // +16
    be_u16 flags;             // +18 0x1 keep palette alpha, 0x2 buffers not owned
};
static_assert(sizeof(Image) == 20);

struct AglTexture {
    be_u32 vtable;                 // +0
    native::pad<48> _4;
    be_u16 width;                  // +52
    be_u16 height;                 // +54
    image2::TexLayout layout;      // +56 (CellGcmTexture at +72)
    native::pad<12> _104;
    be_u16 f116, f118, f120;       // +116
    native::pad<6> _122;
    be_u16 flags128;               // +128
    native::pad<2> _130;
};
static_assert(sizeof(AglTexture) == 132 && offsetof(AglTexture, layout) == kTexData && offsetof(AglTexture, flags128) == kTexFlags128);

// Native functions (src/font/image.cpp).
s32 String_CompareNoCase2(const char* a, const char* b);
void Texture_Construct(AglTexture* t);
u32 Texture_FromImage(AglTexture* t, const Image* img, const char* path);
u32 Texture_FromGtf(AglTexture* t, void* data, u32 size, u32 in_place);
u32 Image_FromTga(const u8* data, u32 size, Image* img, const char* path);
u32 Image_FromBmp(const u8* data, u32 size, Image* img, const char* path);
AglTexture* Texture_Alloc();
u32 Texture_Init(AglTexture* t, const u8* data, u32 size, const char* ext, const char* path);

}  // namespace image
}  // namespace sly4
