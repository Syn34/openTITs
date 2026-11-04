// Font_Load and the BMP pixel reader it scans with (see bmp.h).
#include "bmp.h"

#include "../uiload/more.h"

using namespace sly4;
using namespace sly4::bmpfont;

namespace {

inline u32 vcall1(PPUContext& ctx, u32 obj, u32 off) {
    const u32 opd = load<u32>(load<u32>(obj) + off);
    return static_cast<u32>(call_ptr(ctx, opd, {obj, opd, load<u32>(opd)}));  // r4 / r5 = OPD and entry
}

inline u32 le32(u32 p) {
    return load<u8>(p) | (load<u8>(p + 1) << 8) | (load<u8>(p + 2) << 16) | (static_cast<u32>(load<u8>(p + 3)) << 24);
}

inline s32 s16x(s32 v) { return static_cast<s16>(v); }  // extsh after each step, as the original

}  // namespace

// void Bmp_GetPixel(u8 out[4], const u8* pixels, const u8* palette, s32 x, s32 y,
//                   s32 stride, s32 height, s32 bpp, bool top_down)
// Pixel (x, y) of a BMP's pixel data as R, G, B, 255. `stride` is the row
// length in pixels; rows are stored bottom-up unless `top_down`. 1, 4 and
// 8 bits per pixel go through the BGRA palette (1-bit pixels are taken
// from the low bit of each byte up); other depths than 24 give black.
DECOMP(004D65EC, Bmp_GetPixel) {
    const u32 out = arg<u32>(ctx, 0), pixels = arg<u32>(ctx, 1), palette = arg<u32>(ctx, 2);
    const u32 x = arg<u32>(ctx, 3), y = arg<u32>(ctx, 4), stride = arg<u32>(ctx, 5), height = arg<u32>(ctx, 6);
    const s32 bpp = arg<s32>(ctx, 7);
    // The 9th argument, in the caller's parameter save area (low byte of the word at 116).
    const u8 top_down = static_cast<u8>(load<u32>(static_cast<u32>(ctx.gpr[1]) + 116));
    const u32 row = top_down != 0 ? y : height - y - 1;
    const u32 line = row * stride;

    u32 src;  // the B, G, R bytes
    switch (bpp) {
        case 24: src = pixels + (line + x) * 3; break;
        case 8: src = palette + load<u8>(pixels + line + x) * 4u; break;
        case 4: {
            const u32 b = load<u8>(pixels + (line >> 1) + (x >> 1));
            src = palette + ((b >> ((x & 1) == 0 ? 4 : 0)) & 0xF) * 4;
            break;
        }
        case 1: {
            const u32 b = load<u8>(pixels + (line >> 3) + (x >> 3));
            src = palette + ((b & (1u << (x & 7))) != 0 ? 4 : 0);
            break;
        }
        default:
            for (u32 k = 0; k < 3; ++k) store<u8>(out + k, 0);
            store<u8>(out + 3, 0xFF);
            return;
    }
    const u8 r = load<u8>(src + 2), g = load<u8>(src + 1), b = load<u8>(src);
    store<u8>(out + 0, r);
    store<u8>(out + 1, g);
    store<u8>(out + 2, b);
    store<u8>(out + 3, 0xFF);
}

// u16 Font_Load(Font* f)  [vtable 0x00B22638 +12]
// Uses the font's texture resource (loading it if needed) and, when it is
// loaded, makes a page material per blend index. Then reads the glyph
// cells from the BMP at the font's path (see bmp.h) into a new glyph
// table, the line height being the smallest distance between baseline
// rows (at most the picture's height). 2 if the font has glyphs, else 3.
DECOMP(004D6768, Font_Load) {
    using namespace sly4::uimore;
    const u32 f = arg<u32>(ctx, 0);

    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 2432 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 records = sp + 140;  // Glyph[256]
    const u32 px0 = sp + 2188, px1 = sp + 2192, px2 = sp + 2196, px3 = sp + 2200, px4 = sp + 2204,
              px5 = sp + 2208, px6 = sp + 2212;  // one pixel buffer per call site

    {
        const u32 tex = load<u32>(f + kFontTexture);
        const u16 users = static_cast<u16>(load<u16>(tex + kResUsers) + 1);
        store<u16>(tex + kResUsers, users);
        if (static_cast<s16>(users) > 0 && load<u16>(tex + kResLoaded) < 1)
            store<u16>(tex + kResLoaded, static_cast<u16>(vcall1(ctx, tex, kResVfnLoad)));
    }
    if (load<u16>(load<u32>(f + kFontTexture) + kResLoaded) == 2) {
        static constexpr s32 kModes[3] = {2, 3, 4};  // also stored at sp+128 in the original
        for (u32 k = 0; k < 3; ++k) {
            const u32 tex = load<u32>(f + kFontTexture);
            const u32 src = load<u16>(tex + kResLoaded) == 2 ? load<u32>(tex + kTexHandle)
                                                              : static_cast<u32>(call(ctx, bfn::Tex_GetDefault, {0}));
            const u32 m = static_cast<u32>(call(ctx, bfn::Tex_Acquire, {src}));
            const u32 slot = f + kFontPages + k * 4;
            store<u32>(slot, m);
            Blend_SetMode(m + kMatBlend, kModes[k]);
            Blend_SetDepth(load<u32>(slot) + kMatBlend, (load<u8>(f + kFontFlags) & 0x40) != 0 ? kGlLequal : kGlAlways);
            const u32 test = (load<u8>(f + kFontFlags) & 0x20) != 0 ? 1 : 0;
            const u32 b = load<u32>(slot) + kMatBlend;
            store<u32>(b + 8, (load<u32>(b + 8) & 0x7FFFFFFF) | (test << 31));
        }
    }

    const u32 buf = static_cast<u32>(call(ctx, bfn::File_LoadWhole, {f + kFontPath, 0}));
    if (buf != 0) {
        if (load<u8>(buf) == 'B' && load<u8>(buf + 1) == 'M') {
            const u32 pixels = buf + le32(buf + kBmpDataOffset);
            const u32 palette = buf + le32(buf + kBmpHeaderSize) + 14;
            const s32 width = static_cast<s32>(le32(buf + kBmpWidth));
            s32 height = static_cast<s32>(le32(buf + kBmpHeight));
            const s32 bpp = static_cast<s16>(load<u8>(buf + kBmpBpp) | (load<u8>(buf + kBmpBpp + 1) << 8));
            const u32 top_down = height < 0 ? 1 : 0;  // a negative height: rows stored top-down
            if (top_down) height = -height;

            auto pixel = [&](u32 out, s32 x, s32 y) -> u32 {
                call(ctx, bfn::Bmp_GetPixel,
                     {out, pixels, palette, static_cast<u32>(x), static_cast<u32>(y), static_cast<u32>(width),
                      static_cast<u32>(height), static_cast<u32>(bpp), top_down});
                return out;
            };
            auto red = [](u32 p) { return load<u8>(p + 0); };
            auto green = [](u32 p) { return load<u8>(p + 1); };
            auto blue = [](u32 p) { return load<u8>(p + 2); };

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
                store<u8>(rec + kGlyphWidth, static_cast<u8>(right - left + 1));
                store<u8>(rec + kGlyphAbove, static_cast<u8>(base - top));
                store<u8>(rec + kGlyphBelow, static_cast<u8>(bottom - base));
                store<u8>(rec + kGlyphCells, 1);
                store<u16>(rec + kGlyphX, static_cast<u16>(left));
                store<u16>(rec + kGlyphY, static_cast<u16>(top));
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
                const u32 last = rec - kGlyphSize;
                if (x != static_cast<s32>(load<u16>(last + kGlyphX) + load<u8>(last + kGlyphWidth))) {
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
                const s32 w = load<u8>(last + kGlyphWidth);
                const s32 extra = static_cast<s32>(rt::divw(static_cast<u32>(r28 - start + w), static_cast<u32>(w)));
                store<u8>(last + kGlyphCells, static_cast<u8>(load<u8>(last + kGlyphCells) + extra));
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
            store<u8>(f + kFontGlyphCount, static_cast<u8>(r14));
            if (static_cast<u8>(r14) != 0) {
                const u32 size = (r14 << 3) & ~7u;
                const u32 table = static_cast<u32>(call(ctx, bfn::Mem_Alloc, {size, 0, 16}));
                store<u32>(f + kFontGlyphs, table);
                if (table != 0) {
                    store<u16>(f + kFontLineHeight, line_height);
                    call(ctx, bfn::memmove, {table, records, size});
                }
            }
        }
        call(ctx, bfn::Mem_Free, {buf});
    }
    ret(ctx, load<u8>(f + kFontGlyphCount) != 0 ? 2u : 3u);
}
