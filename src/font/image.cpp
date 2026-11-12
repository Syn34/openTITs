// Texture file decoding: BMP / TGA images, GTF files, and uploading a
// decoded image into an AglTexture.
#include "image.h"

#include "../heap/heap.h"
#include "../memory/mem.h"

namespace sly4::image {
using namespace opentit::native;

namespace {

constexpr u32 kHeaderTga = 18, kHeaderBmp = 54, kBmpInfoSize = 40;

// Magenta (bytes FF 00 FF) is the colour key: such pixels become 0.
inline bool IsKey(u32 src) { return gref<u8>(src) == 0xFF && gref<u8>(src + 1) == 0 && gref<u8>(src + 2) == 0xFF; }

inline void Zero4(u32 dst) {
    for (u32 k = 0; k < 4; ++k) gref<u8>(dst + k) = 0;
}

// One row of 32-bit pixels, BGRA -> ARGB; the width is re-read per pixel.
// Returns the source bytes consumed (as the original computes it).
u32 Row32(const Image* img, u32 dst, u32 src) {
    u32 w = img->width;
    for (u32 col = 0; static_cast<s32>(col) < static_cast<s32>(w);) {
        if (IsKey(src)) {
            Zero4(dst);
        } else {
            gref<u8>(dst + 0) = gref<u8>(src + 3);
            gref<u8>(dst + 1) = gref<u8>(src + 2);
            gref<u8>(dst + 2) = gref<u8>(src + 1);
            gref<u8>(dst + 3) = gref<u8>(src + 0);
        }
        col = (col + 1) & 0xFFFF;
        w = img->width;
        dst += 4;
        src += 4;
    }
    return (w << 2) & 0x3FFFC;
}

// One row of 24-bit pixels, BGR -> ARGB with alpha 255.
u32 Row24(const Image* img, u32 dst, u32 src) {
    u32 w = img->width;
    for (u32 col = 0; static_cast<s32>(col) < static_cast<s32>(w);) {
        if (IsKey(src)) {
            Zero4(dst);
        } else {
            gref<u8>(dst + 0) = 0xFF;
            gref<u8>(dst + 1) = gref<u8>(src + 2);
            gref<u8>(dst + 2) = gref<u8>(src + 1);
            gref<u8>(dst + 3) = gref<u8>(src + 0);
        }
        col = (col + 1) & 0xFFFF;
        w = img->width;
        dst += 4;
        src += 3;
    }
    return w * 3;
}

u32 StackAlloc(u32 size) { return guest(heap::MemStack_Alloc(memory::Mem_GetDefaultStack(), size)); }

inline u32 Le16(u32 p) { return gref<u8>(p) | (static_cast<u32>(gref<u8>(p + 1)) << 8); }

}  // namespace

// strcasecmp (libc) under another name.
s32 String_CompareNoCase2(const char* a, const char* b) { return call<s32>(ifn::strcasecmp, a, b); }

// The base constructor, then the AglTexture vtable.
void Texture_Construct(AglTexture* t) {
    image2::Texture_ConstructBase(t);
    t->vtable = kTexVtbl;
}

// Both sides must be multiples of 8. Sets up a w x h, 32-bit layout, then
// copies the pixels into it (when the layout got memory) and notes that
// an image was uploaded. 1 on success.
u32 Texture_FromImage(AglTexture* t, const Image* img, const char*) {
    const u32 w = img->width;
    if ((w & 7) != 0) return 0;
    const u32 h = img->height;
    if ((h & 7) != 0) return 0;
    t->width = static_cast<u16>(w);
    t->height = static_cast<u16>(h);
    image2::Texture_InitLayout(&t->layout, w, h, 1, 3, 0, nullptr, 0);
    const u16 f = t->flags128;
    const u32 data = t->layout.data;
    t->f116 = 1;
    t->f118 = 1;
    t->flags128 = static_cast<u16>(f | 1);
    t->f120 = 0;
    if (data == 0) return 0;
    const u32 size = (static_cast<u32>(t->width) * t->height) << 2;
    call(ifn::Rsx_Copy, img->pixels.ea.get(), data, size, 1u);
    if (global<u8>(kTextureUploaded) == 0) global<u8>(kTextureUploaded) = 1;
    return 1;
}

// Uses the GTF file where it is (in_place, low byte) or copies it into
// pool-128 memory first, records its size and RSX offset, then, when the
// file is big enough for its first texture, copies that texture's
// CellGcmTexture into +72 (location from the offset's bit 0, i.e. 0) with
// the RSX offset of its pixels. Width / height come from +72 either way.
// Always 1.
u32 Texture_FromGtf(AglTexture* t, void* data_p, u32 size, u32 in_place) {
    const u32 data = guest(data_p);
    GuestFrame frame(144 - 112);
    be_u32* off_copy = frame.as<be_u32>(0);   // sp+112
    be_u32* off_place = frame.as<be_u32>(4);  // sp+116
    image2::TexLayout& l = t->layout;
    u32 off2;
    const u32 w68 = (l.word12 & 0x0FFFFFFFu) | 0x10000000u;
    const u32 w64 = (l.size & 0xFu) | (size << 4);
    if (static_cast<u8>(in_place) != 0) {
        l.data = data;
        l.word12 = w68;
        l.size = w64;
        *off_place = 0u;
        call(ifn::cellGcmAddressToOffset, data, off_place);
        off2 = off_place->get() << 1;
        const u32 w = l.size | 1;
        l.offset = off2;
        l.size = w;
    } else {
        l.word12 = w68;
        l.size = w64;
        const u32 copy = guest(memory::Mem_AllocPool(128, size & 0x0FFFFFFFu, 4, 1));
        l.data = copy;
        *off_copy = 0u;
        call(ifn::cellGcmAddressToOffset, copy, off_copy);
        l.offset = off_copy->get() << 1;
        call(ifn::Rsx_Copy, data, l.data.get(), size, 1u);
        off2 = l.offset;
    }
    const u32 file = l.data;
    const u32 location = off2 & 1;
    if (file != 0 && size >= 12 && size >= gref<be_u32>(file + 4)) {
        const u32 attr = file + 12;  // CellGtfTextureAttribute: +4 offset to the pixels, +8 their size
        // (attr / gcm null checks and a location 0 / 1 check in the original can't fail)
        if ((location == 0 || location == 1) && size >= gref<be_u32>(attr + 4) + gref<be_u32>(attr + 8)) {
            be_u32* gcm = reinterpret_cast<be_u32*>(&l.format);
            for (u32 k = 0; k < 6; ++k) gcm[k] = gref<be_u32>(attr + 12 + k * 4).get();
            l.location = static_cast<u8>(location);
            call(ifn::cellGcmAddressToOffset, file + gref<be_u32>(attr + 4), &l.gcm_offset);
        }
    }
    t->width = l.width.get();
    t->height = l.height.get();
    return 1;
}

// Uncompressed true-colour TGAs only: 24-bit without alpha bits or 32-bit
// with 8. Converts to 32-bit ARGB (magenta keyed to 0), flipping
// bottom-up files. The pixel data isn't checked against `size`.
u32 Image_FromTga(const u8* data_p, u32 size, Image* img, const char*) {
    const u32 data = guest(data_p);
    if (size < kHeaderTga) return 0;
    const u32 id_len = gref<u8>(data);
    if (gref<u8>(data + 1) != 0 || gref<u8>(data + 2) != 2) return 0;
    const u32 w = Le16(data + 12);
    img->width = static_cast<u16>(w);
    const u32 h = Le16(data + 14);
    img->height = static_cast<u16>(h);
    const u32 bpp = gref<u8>(data + 16);
    img->bpp = static_cast<u16>(bpp);
    if (bpp != 32 && bpp != 24) return 0;
    const u32 desc = gref<u8>(data + 17);
    const u32 alpha_bits = desc & 0xF;
    if (!(bpp == 24 && alpha_bits == 0) && !(bpp == 32 && alpha_bits == 8)) return 0;
    const u32 top_down = desc & 0x20;
    u32 off = id_len + kHeaderTga;
    const u32 pixels = StackAlloc((h * w) << 2);
    img->pixels.ea = pixels;
    if (pixels == 0) return 0;
    for (u32 row = 0; static_cast<s32>(row) < static_cast<s32>(img->height.get());) {
        const u32 iw = img->width;
        const u32 line = top_down != 0 ? row : img->height - row - 1;
        const u32 dst = img->pixels.ea + ((line * iw) << 2);
        off += bpp == 32 ? Row32(img, dst, off + data) : Row24(img, dst, off + data);
        row = (row + 1) & 0xFFFF;
    }
    return 1;
}

// BITMAPINFOHEADER BMPs, one plane: 24 / 32-bit are converted to 32-bit
// ARGB (magenta keyed to 0, rows flipped; 24-bit rows are assumed to need
// no padding), 4 / 8-bit keep their indices (4-bit nibbles swapped, rows
// flipped, source rows 4-byte aligned) with the palette converted
// alongside (alpha 255 unless flag 0x1 is set). `size` isn't checked.
u32 Image_FromBmp(const u8* data_p, u32, Image* img, const char*) {
    const u32 data = guest(data_p);
    if (gref<u8>(data) != 'B' || gref<u8>(data + 1) != 'M') return 0;
    u32 pix_off = Le16(data + 10) | (static_cast<u32>(gref<u8>(data + 12)) << 16);
    const u32 pix_off_hi = gref<u8>(data + 13);
    if (static_cast<s16>(Le16(data + 14)) != static_cast<s32>(kBmpInfoSize)) return 0;
    const u32 w = Le16(data + 18);
    img->width = static_cast<u16>(w);
    const u32 h = Le16(data + 22);
    img->height = static_cast<u16>(h);
    if (static_cast<s16>(Le16(data + 26)) != 1) return 0;
    const u32 bpp16 = Le16(data + 28);
    const s32 bpp = static_cast<s16>(bpp16);
    u32 colors = Le16(data + 46) | (static_cast<u32>(gref<u8>(data + 48)) << 16) |
                 (static_cast<u32>(gref<u8>(data + 49)) << 24);
    if (((w * static_cast<u32>(bpp)) >> 3) == 0) return 0;
    img->bpp = static_cast<u16>(bpp16);

    if (bpp == 32 || bpp == 24) {
        const u32 pixels = StackAlloc((h * w) << 2);
        img->pixels.ea = pixels;
        if (pixels == 0) return 0;
        if (bpp == 24) img->bpp = 32;
        pix_off |= pix_off_hi << 24;
        for (u32 row = 0; static_cast<s32>(row) < static_cast<s32>(img->height.get());) {
            const u32 iw = img->width;
            const u32 dst = img->pixels.ea + (((img->height - row) * iw - iw) << 2);
            pix_off += bpp == 24 ? Row24(img, dst, pix_off + data) : Row32(img, dst, pix_off + data);
            row = (row + 1) & 0xFFFF;
        }
        return 1;
    }
    if (bpp != 8 && bpp != 4) return 0;

    if (colors == 0) colors = 1u << (bpp & 63);
    const u32 entries = 1u << (bpp & 63);
    const u32 pal_size = (entries << 2) & 0x3FFFC;
    img->pal_count = static_cast<u16>(entries);
    const u32 pal = StackAlloc(pal_size);
    img->palette.ea = pal;
    if (pal == 0) return 0;
    call(ifn::memset, pal, 0u, pal_size);
    u32 src = data + kHeaderBmp;
    {
        u32 d = img->palette.ea;
        for (u32 k = 0; k < colors; ++k, d += 4, src += 4) {
            if (IsKey(src)) {
                gref<u8>(d + 2) = 0;
                gref<u8>(d + 1) = 0;
                gref<u8>(d + 0) = 0;
                gref<u8>(d + 3) = 0;
                continue;
            }
            gref<u8>(d + 2) = gref<u8>(src);
            gref<u8>(d + 1) = gref<u8>(src + 1);
            gref<u8>(d + 0) = gref<u8>(src + 2);
            gref<u8>(d + 3) = (img->flags & 1) != 0 ? gref<u8>(src + 3) : u8{0xFF};
        }
    }
    const u32 total = ((img->width * static_cast<u32>(bpp)) >> 3) * img->height;
    const u32 pixels = StackAlloc(total);
    img->pixels.ea = pixels;
    if (pixels == 0) return 0;
    const s32 bits = static_cast<s32>(img->width * static_cast<u32>(img->bpp));
    const u32 rows = img->height;
    const u32 row_bytes = static_cast<u32>(bits / 8);  // srawi / addze: rounds toward zero
    const u32 stride = (row_bytes + 3) & ~3u;
    if (bpp != 4) {
        for (u32 r = 0; r < rows; ++r, src += stride)
            call(ifn::memmove, pixels + (rows - r) * row_bytes - row_bytes, src, row_bytes);
        return 1;
    }
    for (u32 r = 0; r < rows; ++r, src += stride) {
        const u32 dst = pixels + ((rows - r) * row_bytes - row_bytes);
        for (u32 k = 0; k < row_bytes; ++k) {
            const u32 b = gref<u8>(src + k);
            gref<u8>(dst + k) = static_cast<u8>(((b >> 4) & 0xF) | ((b << 4) & 0xF0));
        }
    }
    return 1;
}

}  // namespace sly4::image

NATIVE(00555D70, sly4::image::String_CompareNoCase2)
NATIVE(0053A284, sly4::image::Texture_Construct)
NATIVE(0053A34C, sly4::image::Texture_FromImage)
NATIVE(0053A448, sly4::image::Texture_FromGtf)
NATIVE(0050ACAC, sly4::image::Image_FromTga)
NATIVE(0050A6C8, sly4::image::Image_FromBmp)
