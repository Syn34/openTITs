// Font_Load and the BMP pixel reader it scans with (see bmp.h).
#include "bmp.h"

#include "../memory/mem.h"
#include "../text/text.h"
#include "../uiload/more.h"
#include "../uiload/uiload.h"

namespace sly4::bmpfont {
using namespace opentit::native;

namespace {

inline u32 le32(u32 p) {
    return gref<u8>(p) | (gref<u8>(p + 1) << 8) | (gref<u8>(p + 2) << 16) | (static_cast<u32>(gref<u8>(p + 3)) << 24);
}

inline s32 s16x(s32 v) { return static_cast<s16>(v); }  // extsh after each step, as the original

}  // namespace

// Pixel (x, y) of a BMP's pixel data as R, G, B, 255. `stride` is the row
// length in pixels; rows are stored bottom-up unless `top_down` (the 9th
// argument, on the stack). 1, 4 and 8 bits per pixel go through the BGRA
// palette (1-bit pixels are taken from the low bit of each byte up); other
// depths than 24 give black.
void Bmp_GetPixel(u8* out, const u8* pixels_p, const u8* palette_p, u32 x, u32 y, u32 stride, u32 height, s32 bpp,
                  bool top_down) {
    const u32 pixels = guest(pixels_p), palette = guest(palette_p);
    const u32 row = top_down ? y : height - y - 1;
    const u32 line = row * stride;

    u32 src;  // the B, G, R bytes
    switch (bpp) {
        case 24: src = pixels + (line + x) * 3; break;
        case 8: src = palette + gref<u8>(pixels + line + x) * 4u; break;
        case 4: {
            const u32 b = gref<u8>(pixels + (line >> 1) + (x >> 1));
            src = palette + ((b >> ((x & 1) == 0 ? 4 : 0)) & 0xF) * 4;
            break;
        }
        case 1: {
            const u32 b = gref<u8>(pixels + (line >> 3) + (x >> 3));
            src = palette + ((b & (1u << (x & 7))) != 0 ? 4 : 0);
            break;
        }
        default:
            for (u32 k = 0; k < 3; ++k) out[k] = 0;
            out[3] = 0xFF;
            return;
    }
    const u8 r = gref<u8>(src + 2), g = gref<u8>(src + 1), b = gref<u8>(src);
    out[0] = r;
    out[1] = g;
    out[2] = b;
    out[3] = 0xFF;
}

// [vtable 0x00B22638 +12]
// Uses the font's texture resource (loading it if needed) and, when it is
// loaded, makes a page material per blend index. Then reads the glyph
// cells from the BMP at the font's path (see bmp.h) into a new glyph
// table, the line height being the smallest distance between baseline
// rows (at most the picture's height). 2 if the font has glyphs, else 3.
u32 Font_Load(text::Font* font) {
    using namespace sly4::uimore;

    // The original's frame: the glyph records at +140 (copied out with memmove).
    GuestFrame frame(2432 - 112);
    const u32 records = guest(frame.as<u8>(140 - 112));  // Glyph[256]
    u8 px0[4], px1[4], px2[4], px3[4], px4[4], px5[4], px6[4];  // one pixel buffer per call site

    {
        font::UiTexture& tex = gref(font->texture);
        const u16 users = static_cast<u16>(tex.res.users + 1);
        tex.res.users = users;
        if (static_cast<s16>(users) > 0 && tex.res.loaded < 1)
            tex.res.loaded = static_cast<u16>(vcall<u32>(font->texture.get(), kResVfnLoad));
    }
    if (gref(font->texture).res.loaded == 2) {
        static constexpr s32 kModes[3] = {2, 3, 4};  // also stored at sp+128 in the original
        for (u32 k = 0; k < 3; ++k) {
            const font::UiTexture& tex = gref(font->texture);
            const u32 src = tex.res.loaded == 2 ? tex.handle.get() : uiload::Tex_GetDefault(0);
            Material* m = Tex_Acquire(host<void>(src));
            font->pages[k] = guest(m);
            Blend_SetMode(gref<Material>(guest(m)).blend, kModes[k]);
            Blend_SetDepth(gref<Material>(font->pages[k]).blend, (font->flags & 0x40) != 0 ? kGlLequal : kGlAlways);
            const u32 test = (font->flags & 0x20) != 0 ? 1 : 0;
            be_u32* b = gref<Material>(font->pages[k]).blend;
            b[2] = (b[2] & 0x7FFFFFFFu) | (test << 31);
        }
    }

    const u32 buf = call<u32>(bfn::File_LoadWhole, font->res.path, 0u);
    if (buf != 0) {
        if (gref<u8>(buf) == 'B' && gref<u8>(buf + 1) == 'M') {
            const u32 pixels = buf + le32(buf + kBmpDataOffset);
            const u32 palette = buf + le32(buf + kBmpHeaderSize) + 14;
            const s32 width = static_cast<s32>(le32(buf + kBmpWidth));
            s32 height = static_cast<s32>(le32(buf + kBmpHeight));
            const s32 bpp = static_cast<s16>(gref<u8>(buf + kBmpBpp) | (gref<u8>(buf + kBmpBpp + 1) << 8));
            const u32 top_down = height < 0 ? 1 : 0;  // a negative height: rows stored top-down
            if (top_down) height = -height;

            auto pixel = [&](u8* out, s32 x, s32 y) -> const u8* {
                Bmp_GetPixel(out, host<u8>(pixels), host<u8>(palette), static_cast<u32>(x), static_cast<u32>(y),
                             static_cast<u32>(width), static_cast<u32>(height), bpp, top_down != 0);
                return out;
            };
            auto red = [](const u8* p) { return p[0]; };
            auto green = [](const u8* p) { return p[1]; };
            auto blue = [](const u8* p) { return p[2]; };

            // The scan, block for block as the original (its loops step
            // their s16 counters and re-test the bounds where it does).
            u8 done = 0;
            u32 count = 0, r14 = 0, rowcount = 0;
            u16 line_height = static_cast<u16>(height);
            s32 base = s16x(-height);  // the last baseline row
            s32 y = 0, x = 0;
            u32 rec = records;         // next record
            s32 top = 0, right = 0, bottom = 0, left = 0, col = 0, r31 = 0, r28 = 0;

        row:  // 6C34
            if (!(y < height) || !(r14 < kMaxGlyphs)) goto finish;
            count = r14;
            rowcount = 0;
            x = 0;
            r14 = 0;  // becomes 1 on a baseline row
            goto scan;
        next_row:  // 6C58
            y = s16x(y + 1);
            r14 = count;
            if (done == 0) goto row;
            goto finish;

        scan:  // 6C74: look for a baseline marker in row y
            if (!(x < width) || !(count < kMaxGlyphs)) goto row_end;
            if (blue(pixel(px0, x, y)) < kMarker) goto scan_next;
            {
                const s32 d = y - base;
                line_height = static_cast<u16>(static_cast<s32>(line_height) < d ? static_cast<s32>(line_height) : d);
            }
            x = 0;
            r14 = 1;
            rec = records + count * kGlyphSize;
            base = y;

        cell:  // 6D38: walk the baseline row
            if (!(x < width)) goto scan_next;
            col = x;
            if (green(pixel(px1, x, y)) < kMarker) goto widen;
            // A cell: up the left side, along the top, down the right side.
            bottom = y;
            right = x;
            r28 = y;
            left = x;
            if (r28 >= 0) {
                for (s32 yy = y;;) {
                    if (green(pixel(px2, col, yy)) < kMarker) break;
                    r28 = yy;
                    yy = s16x(r28 - 1);
                    if (yy < 0) break;
                }
            }
            top = r28;
            r31 = top;
            if (x < width) {
                for (;;) {
                    if (green(pixel(px3, x, r31)) < kMarker) break;
                    right = x;
                    x = s16x(right + 1);
                    if (!(x < width)) break;
                }
            }
            if (r31 < height) {
                for (;;) {
                    if (green(pixel(px4, right, r31)) < kMarker) break;
                    bottom = r31;
                    r31 = s16x(bottom + 1);
                    if (!(r31 < height)) break;
                }
            }
            if (count < kMaxGlyphs) {
                text::Glyph& g = gref<text::Glyph>(rec);
                g.advance = static_cast<u8>(right - left + 1);
                g.top = static_cast<u8>(base - top);
                g.rows = static_cast<u8>(bottom - base);
                g.channel = 1;
                g.u = static_cast<u16>(left);
                g.v = static_cast<u16>(top);
                ++rowcount;
                ++count;
                rec += kGlyphSize;
            } else {
                done = 1;
            }
            x = right;
            y = base;
        cell_next:  // 6DC4
            x = s16x(x + 1);
            if (done == 0) goto cell;
            goto scan_next;

        widen:  // 6E24: a red marker right after the last glyph widens it
            if (red(px1) < kMarker) goto cell_next;
            if (count < 1) {
                done = 1;
                goto cell_next;
            }
            {
                text::Glyph& last = gref<text::Glyph>(rec - kGlyphSize);
                if (x != static_cast<s32>(last.u + last.advance)) {
                    done = 1;
                    goto cell_next;
                }
                r31 = y;
                r28 = x;
                const s32 start = x;
                if (r31 >= 0) {
                    for (s32 yy = y;;) {
                        if (red(pixel(px5, col, yy)) < kMarker) break;
                        r31 = yy;
                        yy = s16x(r31 - 1);
                        if (yy < 0) break;
                    }
                }
                if (x < width) {
                    for (;;) {
                        if (red(pixel(px6, x, r31)) < kMarker) break;
                        r28 = x;
                        x = s16x(r28 + 1);
                        if (!(x < width)) break;
                    }
                }
                x = r28;
                const s32 w = last.advance;
                const s32 extra = static_cast<s32>(rt::divw(static_cast<u32>(r28 - start + w), static_cast<u32>(w)));
                last.channel = static_cast<u8>(last.channel + extra);
                y = base;
            }
            goto cell_next;

        scan_next:  // 6D20
            x = s16x(x + 1);
            if (done == 0) goto scan;
        row_end:  // 703C: a baseline row without glyphs ends the scan
            if (r14 != 0 && rowcount == 0) done = 1;
            goto next_row;

        finish:  // 705C
            font->range = static_cast<u8>(r14);
            if (static_cast<u8>(r14) != 0) {
                const u32 size = (r14 << 3) & ~7u;
                text::Glyph* table = static_cast<text::Glyph*>(memory::Mem_Alloc(size, 0, 16));
                font->glyphs = table;
                if (table) {
                    font->line_height = line_height;
                    call(bfn::memmove, table, records, size);
                }
            }
        }
        memory::Mem_Free(host<void>(buf));
    }
    return font->range != 0 ? 2u : 3u;
}

}  // namespace sly4::bmpfont

NATIVE(004D65EC, sly4::bmpfont::Bmp_GetPixel)
NATIVE(004D6768, sly4::bmpfont::Font_Load)

