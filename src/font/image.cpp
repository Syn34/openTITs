// Texture file decoding: BMP / TGA images, GTF files, and uploading a
// decoded image into an AglTexture.
#include "image.h"

using namespace sly4;
using namespace sly4::image;

namespace {

constexpr u32 kHeaderTga = 18, kHeaderBmp = 54, kBmpInfoSize = 40;

// Magenta (bytes FF 00 FF) is the colour key: such pixels become 0.
inline bool IsKey(u32 src) { return load<u8>(src) == 0xFF && load<u8>(src + 1) == 0 && load<u8>(src + 2) == 0xFF; }

inline void Zero4(u32 dst) {
    for (u32 k = 0; k < 4; ++k) store<u8>(dst + k, 0);
}

// One row of 32-bit pixels, BGRA -> ARGB; the width is re-read per pixel.
// Returns the source bytes consumed (as the original computes it).
u32 Row32(u32 img, u32 dst, u32 src) {
    u32 w = load<u16>(img + kImgWidth);
    for (u32 col = 0; static_cast<s32>(col) < static_cast<s32>(w);) {
        if (IsKey(src)) {
            Zero4(dst);
        } else {
            store<u8>(dst + 0, load<u8>(src + 3));
            store<u8>(dst + 1, load<u8>(src + 2));
            store<u8>(dst + 2, load<u8>(src + 1));
            store<u8>(dst + 3, load<u8>(src + 0));
        }
        col = (col + 1) & 0xFFFF;
        w = load<u16>(img + kImgWidth);
        dst += 4;
        src += 4;
    }
    return (w << 2) & 0x3FFFC;
}

// One row of 24-bit pixels, BGR -> ARGB with alpha 255.
u32 Row24(u32 img, u32 dst, u32 src) {
    u32 w = load<u16>(img + kImgWidth);
    for (u32 col = 0; static_cast<s32>(col) < static_cast<s32>(w);) {
        if (IsKey(src)) {
            Zero4(dst);
        } else {
            store<u8>(dst + 0, 0xFF);
            store<u8>(dst + 1, load<u8>(src + 2));
            store<u8>(dst + 2, load<u8>(src + 1));
            store<u8>(dst + 3, load<u8>(src + 0));
        }
        col = (col + 1) & 0xFFFF;
        w = load<u16>(img + kImgWidth);
        dst += 4;
        src += 3;
    }
    return w * 3;
}

u32 StackAlloc(PPUContext& ctx, u32 size) {
    const u32 st = static_cast<u32>(call(ctx, ifn::Mem_GetDefaultStack));
    return static_cast<u32>(call(ctx, ifn::MemStack_Alloc, {st, size}));
}

inline u32 Le16(u32 p) { return load<u8>(p) | (static_cast<u32>(load<u8>(p + 1)) << 8); }

}  // namespace

// s32 String_CompareNoCase2(const char* a, const char* b)
// strcasecmp (libc) under another name.
DECOMP(00555D70, String_CompareNoCase2) {
    ret(ctx, call(ctx, ifn::strcasecmp, {arg<u32>(ctx, 0), arg<u32>(ctx, 1)}));
}

// void Texture_Construct(AglTexture* t)
// The base constructor, then the AglTexture vtable. r3 is left holding the
// vtable address, as in the original.
DECOMP(0053A284, Texture_Construct) {
    const u32 t = arg<u32>(ctx, 0);
    call(ctx, ifn::Texture_ConstructBase, {t});
    store<u32>(t, kTexVtbl);
    ret(ctx, kTexVtbl);
}

// bool Texture_FromImage(AglTexture* t, const Image* img, const char* path)
// Both sides must be multiples of 8. Sets up a w x h, 32-bit layout, then
// copies the pixels into it (when the layout got memory) and notes that
// an image was uploaded. 1 on success.
DECOMP(0053A34C, Texture_FromImage) {
    const u32 t = arg<u32>(ctx, 0), img = arg<u32>(ctx, 1);
    const u32 w = load<u16>(img + kImgWidth);
    if ((w & 7) != 0) return ret(ctx, 0u);
    const u32 h = load<u16>(img + kImgHeight);
    if ((h & 7) != 0) return ret(ctx, 0u);
    store<u16>(t + kTexWidth, static_cast<u16>(w));
    store<u16>(t + kTexHeight, static_cast<u16>(h));
    call(ctx, ifn::Texture_InitLayout, {t + kTexData, w, h, 1, 3, 0, 0, 0});
    const u16 f = load<u16>(t + kTexFlags128);
    const u32 data = load<u32>(t + kTexData);
    store<u16>(t + kTex116, 1);
    store<u16>(t + kTex118, 1);
    store<u16>(t + kTexFlags128, static_cast<u16>(f | 1));
    store<u16>(t + kTex120, 0);
    if (data == 0) return ret(ctx, 0u);
    const u32 size = (static_cast<u32>(load<u16>(t + kTexWidth)) * load<u16>(t + kTexHeight)) << 2;
    call(ctx, ifn::Rsx_Copy, {load<u32>(img + kImgPixels), data, size, 1});
    if (load<u8>(kTextureUploaded) == 0) store<u8>(kTextureUploaded, 1);
    ret(ctx, 1u);
}

// bool Texture_FromGtf(AglTexture* t, void* data, u32 size, bool in_place)
// Uses the GTF file where it is (in_place) or copies it into pool-128
// memory first, records its size and RSX offset, then, when the file is
// big enough for its first texture, copies that texture's CellGcmTexture
// into +72 (location from the offset's bit 0, i.e. 0) with the RSX offset
// of its pixels. Width / height come from +72 either way. Always 1.
DECOMP(0053A448, Texture_FromGtf) {
    const u32 t = arg<u32>(ctx, 0), data = arg<u32>(ctx, 1), size = arg<u32>(ctx, 2);
    const u8 in_place = static_cast<u8>(arg<u32>(ctx, 3));
    Stack frame(ctx, 144 - 112);
    const u32 sp = frame.addr() - 112;
    u32 off2;
    const u32 w68 = (load<u32>(t + kTexWord68) & 0x0FFFFFFFu) | 0x10000000u;
    const u32 w64 = (load<u32>(t + kTexWord64) & 0xFu) | (size << 4);
    if (in_place != 0) {
        store<u32>(t + kTexData, data);
        store<u32>(t + kTexWord68, w68);
        store<u32>(t + kTexWord64, w64);
        store<u32>(sp + 116, 0);
        call(ctx, ifn::cellGcmAddressToOffset, {data, sp + 116});
        off2 = load<u32>(sp + 116) << 1;
        const u32 w = load<u32>(t + kTexWord64) | 1;
        store<u32>(t + kTexDataOffset, off2);
        store<u32>(t + kTexWord64, w);
    } else {
        store<u32>(t + kTexWord68, w68);
        store<u32>(t + kTexWord64, w64);
        const u32 copy = static_cast<u32>(call(ctx, ifn::Mem_AllocPool, {128, size & 0x0FFFFFFFu, 4, 1}));
        store<u32>(t + kTexData, copy);
        store<u32>(sp + 112, 0);
        call(ctx, ifn::cellGcmAddressToOffset, {copy, sp + 112});
        store<u32>(t + kTexDataOffset, load<u32>(sp + 112) << 1);
        call(ctx, ifn::Rsx_Copy, {data, load<u32>(t + kTexData), size, 1});
        off2 = load<u32>(t + kTexDataOffset);
    }
    const u32 file = load<u32>(t + kTexData);
    const u32 location = off2 & 1;
    if (file != 0 && size >= 12 && size >= load<u32>(file + 4)) {
        const u32 attr = file + 12;  // CellGtfTextureAttribute: +4 offset to the pixels, +8 their size
        const u32 gcm = t + kTexGcm;
        // (attr / gcm null checks and a location 0 / 1 check in the original can't fail)
        if (attr != 0 && gcm != 0 && (location == 0 || location == 1) &&
            size >= load<u32>(attr + 4) + load<u32>(attr + 8)) {
            for (u32 k = 0; k < 24; k += 4) store<u32>(gcm + k, load<u32>(attr + 12 + k));
            store<u8>(t + kTexGcm + 14, static_cast<u8>(location));
            call(ctx, ifn::cellGcmAddressToOffset, {file + load<u32>(attr + 4), t + kTexGcm + 20});
        }
    }
    store<u16>(t + kTexWidth, load<u16>(t + kTexGcm + 8));
    store<u16>(t + kTexHeight, load<u16>(t + kTexGcm + 10));
    ret(ctx, 1u);
}

// bool Image_FromTga(const u8* data, u32 size, Image* img, const char* path)
// Uncompressed true-colour TGAs only: 24-bit without alpha bits or 32-bit
// with 8. Converts to 32-bit ARGB (magenta keyed to 0), flipping
// bottom-up files. The pixel data isn't checked against `size`.
DECOMP(0050ACAC, Image_FromTga) {
    const u32 data = arg<u32>(ctx, 0), size = arg<u32>(ctx, 1), img = arg<u32>(ctx, 2);
    if (size < kHeaderTga) return ret(ctx, 0u);
    const u32 id_len = load<u8>(data);
    if (load<u8>(data + 1) != 0 || load<u8>(data + 2) != 2) return ret(ctx, 0u);
    const u32 w = Le16(data + 12);
    store<u16>(img + kImgWidth, static_cast<u16>(w));
    const u32 h = Le16(data + 14);
    store<u16>(img + kImgHeight, static_cast<u16>(h));
    const u32 bpp = load<u8>(data + 16);
    store<u16>(img + kImgBpp, static_cast<u16>(bpp));
    if (bpp != 32 && bpp != 24) return ret(ctx, 0u);
    const u32 desc = load<u8>(data + 17);
    const u32 alpha_bits = desc & 0xF;
    if (!(bpp == 24 && alpha_bits == 0) && !(bpp == 32 && alpha_bits == 8)) return ret(ctx, 0u);
    const u32 top_down = desc & 0x20;
    u32 off = id_len + kHeaderTga;
    const u32 pixels = StackAlloc(ctx, (h * w) << 2);
    store<u32>(img + kImgPixels, pixels);
    if (pixels == 0) return ret(ctx, 0u);
    for (u32 row = 0; static_cast<s32>(row) < static_cast<s32>(load<u16>(img + kImgHeight));) {
        const u32 iw = load<u16>(img + kImgWidth);
        const u32 line = top_down != 0 ? row : load<u16>(img + kImgHeight) - row - 1;
        const u32 dst = load<u32>(img + kImgPixels) + ((line * iw) << 2);
        off += bpp == 32 ? Row32(img, dst, off + data) : Row24(img, dst, off + data);
        row = (row + 1) & 0xFFFF;
    }
    ret(ctx, 1u);
}

// bool Image_FromBmp(const u8* data, u32 size, Image* img, const char* path)
// BITMAPINFOHEADER BMPs, one plane: 24 / 32-bit are converted to 32-bit
// ARGB (magenta keyed to 0, rows flipped; 24-bit rows are assumed to need
// no padding), 4 / 8-bit keep their indices (4-bit nibbles swapped, rows
// flipped, source rows 4-byte aligned) with the palette converted
// alongside (alpha 255 unless flag 0x1 is set). `size` isn't checked.
DECOMP(0050A6C8, Image_FromBmp) {
    const u32 data = arg<u32>(ctx, 0), img = arg<u32>(ctx, 2);
    if (load<u8>(data) != 'B' || load<u8>(data + 1) != 'M') return ret(ctx, 0u);
    u32 pix_off = Le16(data + 10) | (static_cast<u32>(load<u8>(data + 12)) << 16);
    const u32 pix_off_hi = load<u8>(data + 13);
    if (static_cast<s16>(Le16(data + 14)) != static_cast<s32>(kBmpInfoSize)) return ret(ctx, 0u);
    const u32 w = Le16(data + 18);
    store<u16>(img + kImgWidth, static_cast<u16>(w));
    const u32 h = Le16(data + 22);
    store<u16>(img + kImgHeight, static_cast<u16>(h));
    if (static_cast<s16>(Le16(data + 26)) != 1) return ret(ctx, 0u);
    const u32 bpp16 = Le16(data + 28);
    const s32 bpp = static_cast<s16>(bpp16);
    u32 colors = Le16(data + 46) | (static_cast<u32>(load<u8>(data + 48)) << 16) |
                 (static_cast<u32>(load<u8>(data + 49)) << 24);
    if (((w * static_cast<u32>(bpp)) >> 3) == 0) return ret(ctx, 0u);
    store<u16>(img + kImgBpp, static_cast<u16>(bpp16));

    if (bpp == 32 || bpp == 24) {
        const u32 pixels = StackAlloc(ctx, (h * w) << 2);
        store<u32>(img + kImgPixels, pixels);
        if (pixels == 0) return ret(ctx, 0u);
        if (bpp == 24) store<u16>(img + kImgBpp, 32);
        pix_off |= pix_off_hi << 24;
        for (u32 row = 0; static_cast<s32>(row) < static_cast<s32>(load<u16>(img + kImgHeight));) {
            const u32 iw = load<u16>(img + kImgWidth);
            const u32 dst = load<u32>(img + kImgPixels) + (((load<u16>(img + kImgHeight) - row) * iw - iw) << 2);
            pix_off += bpp == 24 ? Row24(img, dst, pix_off + data) : Row32(img, dst, pix_off + data);
            row = (row + 1) & 0xFFFF;
        }
        return ret(ctx, 1u);
    }
    if (bpp != 8 && bpp != 4) return ret(ctx, 0u);

    if (colors == 0) colors = 1u << (bpp & 63);
    const u32 entries = 1u << (bpp & 63);
    const u32 pal_size = (entries << 2) & 0x3FFFC;
    store<u16>(img + kImgPalCount, static_cast<u16>(entries));
    const u32 pal = StackAlloc(ctx, pal_size);
    store<u32>(img + kImgPalette, pal);
    if (pal == 0) return ret(ctx, 0u);
    call(ctx, ifn::memset, {pal, 0, pal_size});
    u32 src = data + kHeaderBmp;
    {
        u32 d = load<u32>(img + kImgPalette);
        for (u32 k = 0; k < colors; ++k, d += 4, src += 4) {
            if (IsKey(src)) {
                store<u8>(d + 2, 0);
                store<u8>(d + 1, 0);
                store<u8>(d + 0, 0);
                store<u8>(d + 3, 0);
                continue;
            }
            store<u8>(d + 2, load<u8>(src));
            store<u8>(d + 1, load<u8>(src + 1));
            store<u8>(d + 0, load<u8>(src + 2));
            store<u8>(d + 3, (load<u16>(img + kImgFlags) & 1) != 0 ? load<u8>(src + 3) : 0xFF);
        }
    }
    const u32 total = ((load<u16>(img + kImgWidth) * static_cast<u32>(bpp)) >> 3) * load<u16>(img + kImgHeight);
    const u32 pixels = StackAlloc(ctx, total);
    store<u32>(img + kImgPixels, pixels);
    if (pixels == 0) return ret(ctx, 0u);
    const s32 bits = static_cast<s32>(load<u16>(img + kImgWidth) * static_cast<u32>(load<u16>(img + kImgBpp)));
    const u32 rows = load<u16>(img + kImgHeight);
    const u32 row_bytes = static_cast<u32>(bits / 8);  // srawi / addze: rounds toward zero
    const u32 stride = (row_bytes + 3) & ~3u;
    if (bpp != 4) {
        for (u32 r = 0; r < rows; ++r, src += stride)
            call(ctx, ifn::memmove, {pixels + (rows - r) * row_bytes - row_bytes, src, row_bytes});
        return ret(ctx, 1u);
    }
    for (u32 r = 0; r < rows; ++r, src += stride) {
        const u32 dst = pixels + ((rows - r) * row_bytes - row_bytes);
        for (u32 k = 0; k < row_bytes; ++k) {
            const u32 b = load<u8>(src + k);
            store<u8>(dst + k, static_cast<u8>(((b >> 4) & 0xF) | ((b << 4) & 0xF0)));
        }
    }
    ret(ctx, 1u);
}
