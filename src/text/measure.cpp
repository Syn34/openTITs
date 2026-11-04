// UTF-8 decoding and text measurement.
#include "text.h"

using namespace sly4;
using namespace sly4::text;

namespace {
constexpr f32 kHalf = 0.5f;          // 0x004DB1E0
constexpr f32 kNoFontWidth = 0.0f;   // 0x004DB1E4
constexpr f32 kAlignFactor = -0.5f;  // 0x004DAEE0: offset per alignment step
constexpr u32 kSpace = ' ';
}  // namespace

// u32 Utf8_Next(void* unused, const char** p, u32* len, bool advance)
// Decodes the code point at *p (1 to 4 bytes, by the lead byte's top
// nibble; continuation bytes aren't checked), stores its length in *len
// when len isn't 0 and moves *p past it when `advance` is set. A single
// byte comes back sign-extended to 32 bits, as the original's extsb does.
DECOMP(004DA380, Utf8_Next) {
    const u32 pp = arg<u32>(ctx, 1);
    const u32 len = arg<u32>(ctx, 2);
    const bool advance = arg<u8>(ctx, 3) != 0;
    const u32 s = load<u32>(pp);
    const u32 c = load<u8>(s);

    u32 n, cp;
    if ((c & 0x80) == 0) {
        if (len != 0) store<u32>(len, 1);
        if (advance) store<u32>(pp, s + 1);
        return ret(ctx, static_cast<u32>(static_cast<s32>(static_cast<s8>(c))));
    }
    const u32 hi = c & 0xF0;
    if (hi == 0xE0) {
        n = 3;
        cp = ((c << 12) & 0xF000) + ((load<u8>(s + 1) << 6) & 0xFC0) + (load<u8>(s + 2) & 0x3F);
    } else if (hi == 0xF0) {
        n = 4;
        cp = ((c << 18) & 0x1C0000) + ((load<u8>(s + 1) << 12) & 0x3F000) + ((load<u8>(s + 2) << 6) & 0xFC0) +
             (load<u8>(s + 3) & 0x3F);
    } else {
        n = 2;
        cp = ((c << 6) & 0x7C0) | (load<u8>(s + 1) & 0x3F);
    }
    if (len != 0) store<u32>(len, n);
    if (advance) store<u32>(pp, s + n);
    ret(ctx, cp);
}

// f32 UiText_SpaceWidth(TextElem* e, const f32* scale)
// Width of a space: its glyph's advance, scaled by the font, the font set
// and *scale. Without a glyph, half the first font's line height (unscaled
// by *scale); 0 without fonts.
DECOMP(004DB1E8, UiText_SpaceWidth) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 scale = arg<u32>(ctx, 1);
    const u32 set = field<u32>(e, kTextFontSet);
    if (set == 0) return ret(ctx, kNoFontWidth);

    const f64 set_scale = lfs(set + kSetScale);
    const u32 count = field<u32>(set, kSetFontCount);
    const GlyphRef g = FontSet_FindGlyph(set, kSpace);
    if (g.index < count) {
        const u32 font = load<u32>(set + kSetFonts + g.index * 4);
        if (font != 0) {
            const u32 adv = load<u8>(field<u32>(font, kFontGlyphs) + g.glyph * kGlyphSize + kGlyphAdvance);
            f64 w = fmuls(static_cast<f64>(adv), lfs(set + kSetFontScale + g.index * 4));
            w = fmuls(w, set_scale);
            return ret(ctx, static_cast<f32>(fmuls(w, lfs(scale))));
        }
    }
    if (count == 0) return ret(ctx, kNoFontWidth);
    const u32 font0 = load<u32>(set + kSetFonts);
    if (font0 == 0) return ret(ctx, kNoFontWidth);
    const f64 lh = static_cast<f64>(field<u16>(font0, kFontLineHeight).get());
    ret(ctx, static_cast<f32>(fmuls(fmuls(lh, set_scale), kHalf)));
}

// u32 UiText_MeasureLines(TextElem* e, f32* widths, u32 max, u32, const f32* scale; f1 = space)
// Fills widths[line] with each line's width (characters advance by their
// glyph or, without one, by `space`; spacing goes between characters),
// then turns each into its alignment offset: width * -0.5 * h-align.
// Returns the number of lines (0 without text). `max` isn't checked.
DECOMP(004DAEE8, UiText_MeasureLines) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 widths = arg<u32>(ctx, 1);
    const u32 scale = arg<u32>(ctx, 4);
    const f64 space = farg(ctx, 0);

    u32 breaks = 0;
    store<u32>(widths, 0);
    const u32 text = field<u32>(e, kTextBuf);
    if (text == 0) return ret(ctx, 0u);

    Stack frame(ctx, 8);
    const u32 pp = frame.addr();
    store<u32>(pp, text);
    if (load<u8>(text) != 0) {
        u32 line = 0;      // byte offset of widths[current line]
        u32 on_line = 0;   // characters on it so far
        do {
            const u32 c = call32(ctx, tfn::Utf8_Next, {e, pp, 0, 1});
            if (c == '\n' || c == '\r') {
                line += 4;
                ++breaks;
                on_line = 0;
                store<u32>(widths + line, 0);
                continue;
            }
            const u32 set = field<u32>(e, kTextFontSet);
            bool found = false;
            if ((field<u32>(e, kTextFlags) & kTextMonospace) == 0) {
                const GlyphRef g = FontSet_FindGlyph(set, c);
                if (g.index < field<u32>(set, kSetFontCount)) {
                    const u32 font = load<u32>(set + kSetFonts + g.index * 4);
                    if (font != 0) {
                        found = true;
                        if (on_line != 0) {
                            const f64 gap = fadds(lfs(e + kTextCharSpacing), lfs(set + g.index * 4 + kSetFontKerning));
                            stfs(widths + line, fmadds(space, gap, lfs(widths + line)));
                        }
                        const u32 set2 = field<u32>(e, kTextFontSet);
                        const u32 adv = load<u8>(field<u32>(font, kFontGlyphs) + g.glyph * kGlyphSize + kGlyphAdvance);
                        f64 w = fmuls(static_cast<f64>(adv), lfs(set2 + g.index * 4 + kSetFontScale));
                        w = fmuls(w, lfs(set2 + kSetScale));
                        stfs(widths + line, fmadds(w, lfs(scale), lfs(widths + line)));
                    }
                }
            }
            if (!found) {
                if (on_line != 0) stfs(widths + line, fmadds(space, lfs(e + kTextCharSpacing), lfs(widths + line)));
                stfs(widths + line, fadds(lfs(widths + line), space));
            }
            ++on_line;
        } while (load<u8>(load<u32>(pp)) != 0);
    }

    const u32 lines = breaks + 1;
    if (lines != 0) {
        u32 left = lines, off = 0;
        do {
            const s32 align = static_cast<s32>((field<u32>(e, kElemFlags) >> 14) & 3);
            f64 w = fmuls(lfs(widths + off), kAlignFactor);
            stfs(widths + off, fmuls(w, static_cast<f64>(align)));
            off += 4;
            --left;
        } while (static_cast<s32>(left) > 0);
    }
    ret(ctx, lines);
}
