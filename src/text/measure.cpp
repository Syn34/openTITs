// UTF-8 decoding and text measurement.
#include "text.h"

namespace sly4::text {
using namespace opentit::native;

namespace {
constexpr f32 kHalf = 0.5f;          // 0x004DB1E0
constexpr f32 kNoFontWidth = 0.0f;   // 0x004DB1E4
constexpr f32 kAlignFactor = -0.5f;  // 0x004DAEE0: offset per alignment step
constexpr u32 kSpace = ' ';
}  // namespace

// Decodes the code point at *p (1 to 4 bytes, by the lead byte's top
// nibble; continuation bytes aren't checked), stores its length in *len
// when len isn't 0 and moves *p past it when `advance` is set. A single
// byte comes back sign-extended to 32 bits, as the original's extsb does.
u32 Utf8_Next(void*, be_u32* pp, be_u32* len, bool advance) {
    const u32 s = *pp;
    const u8* b = &gref<u8>(s);
    const u32 c = b[0];
    u32 n, cp;
    if ((c & 0x80) == 0) {
        if (len) *len = 1u;
        if (advance) *pp = s + 1;
        return static_cast<u32>(static_cast<s32>(static_cast<s8>(c)));
    }
    const u32 hi = c & 0xF0;
    if (hi == 0xE0) {
        n = 3;
        cp = ((c << 12) & 0xF000) + ((static_cast<u32>(b[1]) << 6) & 0xFC0) + (b[2] & 0x3Fu);
    } else if (hi == 0xF0) {
        n = 4;
        cp = ((c << 18) & 0x1C0000) + ((static_cast<u32>(b[1]) << 12) & 0x3F000) + ((static_cast<u32>(b[2]) << 6) & 0xFC0) +
             (b[3] & 0x3Fu);
    } else {
        n = 2;
        cp = ((c << 6) & 0x7C0) | (b[1] & 0x3Fu);
    }
    if (len) *len = n;
    if (advance) *pp = s + n;
    return cp;
}

// Width of a space: its glyph's advance, scaled by the font, the font set
// and *scale. Without a glyph, half the first font's line height (unscaled
// by *scale); 0 without fonts.
f32 UiText_SpaceWidth(TextElem* e, const be_f32* scale) {
    const FontSet* set = e->font_set;
    if (!set) return kNoFontWidth;
    const f32 set_scale = set->scale;
    const u32 count = set->font_count;
    const GlyphRef g = FindGlyph(*set, kSpace);
    if (g.index < count) {
        const Font* font = set->fonts[g.index];
        if (font) {
            const u32 adv = gref<Glyph>(font->glyphs.ea + g.glyph * kGlyphSize).advance;
            f32 w = f32_mul(static_cast<f32>(adv), set->font_scale[g.index]);
            w = f32_mul(w, set_scale);
            return f32_mul(w, *scale);
        }
    }
    if (count == 0) return kNoFontWidth;
    const Font* font0 = set->fonts[0];
    if (!font0) return kNoFontWidth;
    return f32_mul(f32_mul(static_cast<f32>(font0->line_height.get()), set_scale), kHalf);
}

// Fills widths[line] with each line's width (characters advance by their
// glyph or, without one, by `space`; spacing goes between characters),
// then turns each into its alignment offset: width * -0.5 * h-align.
// Returns the number of lines (0 without text). The second argument (the
// size of widths) isn't checked.
u32 UiText_MeasureLines(TextElem* e, be_f32* widths, u32, u32, const be_f32* scale, f32 space) {
    u32 breaks = 0;
    widths[0] = 0.0f;  // stored as the word 0
    const u32 text = e->buf.ea;
    if (text == 0) return 0;

    GuestFrame frame(8);
    be_u32* pp = frame.as<be_u32>();
    *pp = text;
    if (gref<u8>(text) != 0) {
        u32 line = 0;      // index of the current line in widths
        u32 on_line = 0;   // characters on it so far
        do {
            const u32 c = Utf8_Next(e, pp, nullptr, true);
            if (c == '\n' || c == '\r') {
                ++line;
                ++breaks;
                on_line = 0;
                widths[line] = 0.0f;
                continue;
            }
            const FontSet& set = gref(e->font_set);
            bool found = false;
            if ((e->text_flags & kTextMonospace) == 0) {
                const GlyphRef g = FindGlyph(set, c);
                if (g.index < set.font_count) {
                    const Font* font = set.fonts[g.index];
                    if (font) {
                        found = true;
                        if (on_line != 0) {
                            const f32 gap = f32_add(e->char_spacing, set.font_kerning[g.index]);
                            widths[line] = f32_madd(space, gap, widths[line]);
                        }
                        const FontSet& set2 = gref(e->font_set);
                        const u32 adv = gref<Glyph>(font->glyphs.ea + g.glyph * kGlyphSize).advance;
                        f32 w = f32_mul(static_cast<f32>(adv), set2.font_scale[g.index]);
                        w = f32_mul(w, set2.scale);
                        widths[line] = f32_madd(w, *scale, widths[line]);
                    }
                }
            }
            if (!found) {
                if (on_line != 0) widths[line] = f32_madd(space, e->char_spacing, widths[line]);
                widths[line] = f32_add(widths[line], space);
            }
            ++on_line;
        } while (gref<u8>(*pp) != 0);
    }

    const u32 lines = breaks + 1;
    if (lines != 0) {
        u32 left = lines, i = 0;
        do {
            const s32 align = static_cast<s32>((e->elem.flags >> 14) & 3);
            const f32 w = f32_mul(widths[i], kAlignFactor);
            widths[i] = f32_mul(w, static_cast<f32>(align));
            ++i;
            --left;
        } while (static_cast<s32>(left) > 0);
    }
    return lines;
}

}  // namespace sly4::text

NATIVE(004DA380, sly4::text::Utf8_Next)
NATIVE(004DB1E8, sly4::text::UiText_SpaceWidth)
NATIVE(004DAEE8, sly4::text::UiText_MeasureLines)
