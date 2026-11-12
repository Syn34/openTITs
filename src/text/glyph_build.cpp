// UiText_BuildGlyph (0x004DAAA8).
#include "glyph_build.h"

namespace sly4::text {
using namespace opentit::native;

namespace {

constexpr f32 kLineTop = 3.0f;      // 0x004DAA80
constexpr f32 kQuarter = 0.25f;     // 0x004DAA84
constexpr f32 kHalf = 0.5f;         // 0x004DAA88
constexpr f32 kNoFontHeight = 0.0f; // 0x004DAA8C

// fnmsubs d = -(a * c - b), as the recompiler does it.
inline f32 f32_nmsub(f32 a, f32 c, f32 b) { return static_cast<f32>(-std::fma(static_cast<f64>(a), c, -static_cast<f64>(b))); }

inline u64 put_bits(u64 word, u32 shift, u32 width, u64 value) {
    const u64 mask = ((u64{1} << width) - 1) << shift;
    return (word & ~mask) | ((value << shift) & mask);
}

}  // namespace

// Fills the quad for code point `c` at (*x, *y) and advances *x past it
// (by its width, or the space width for a monospaced element, plus the
// spacing and the font's kerning, scaled by the space width). Returns 0,
// touching nothing, when no font has the glyph.
u32 UiText_BuildGlyph(TextElem* e, GlyphQuad* q, u32 c, be_f32* px, be_f32* py, u32, const be_f32* scale, u32 line,
                      f32 space) {
    const FontSet* set = e->font_set;
    if (!set) return 0;
    const u32 count = set->font_count;
    const GlyphRef g = FindGlyph(*set, c);
    if (g.index >= count) return 0;
    const Font* font = set->fonts[g.index];
    if (!font) return 0;

    const f32 set_scale = set->scale;
    const f32 line_scale = set->line_scale;
    f32 top = kNoFontHeight;
    if (count != 0 && set->fonts[0]) top = f32_mul(static_cast<f32>(set->fonts[0]->line_height.get()), line_scale);
    const f32 sx = scale[0], sy = scale[1];
    top = f32_mul(f32_mul(top, sy), kLineTop);

    const u32 field_a = (e->elem.flags >> 22) & 3;
    const Glyph& glyph = gref<Glyph>(font->glyphs.ea + g.glyph * kGlyphSize);
    const u32 g0 = glyph.top, g2 = glyph.advance, g3 = glyph.channel;
    const u32 rows = g0 + glyph.rows + 1;
    const f32 font_scale = set->font_scale[g.index];

    // Bit 19 first (stored on its own in the original), then the rest.
    u64 bits = q->bits;
    bits = put_bits(bits, kBitsFontFlag, 1, (font->flags >> 7) & 1);
    q->page = font->pages[field_a].get();

    const f32 down = f32_mul(f32_mul(static_cast<f32>(g0), font_scale), line_scale);
    const f32 width = f32_mul(f32_mul(f32_mul(static_cast<f32>(g2), font_scale), set_scale), sx);
    const bool mono = (e->text_flags & kTextMonospace) != 0;
    const f32 qx = mono ? f32_madd(f32_sub(space, width), kHalf, *px) : px->get();
    q->x = qx;
    f32 qy = f32_madd(top, kQuarter, *py);
    qy = f32_nmsub(down, sy, qy);
    const f32 height = f32_mul(f32_mul(f32_mul(static_cast<f32>(rows), font_scale), line_scale), sy);
    q->y = qy;
    q->w = width;
    q->h = height;

    // The original converts width and rows back through float and fctidz;
    // both are small integers, so that is the integer itself.
    bits = put_bits(bits, kBitsTexU, 11, glyph.u);
    bits = put_bits(bits, kBitsTexV, 11, glyph.v);
    bits = put_bits(bits, kBitsWidth, 11, g2);
    bits = put_bits(bits, kBitsHeight, 11, rows);
    bits = put_bits(bits, kBitsChannel, 4, g3);
    const s32 mode = static_cast<s32>((e->text_flags >> 22) & 0xF);
    const u64 channel2 = mode < 3 ? 0 : static_cast<u64>(((bits >> kBitsChannel) & 0xF) - 1);
    bits = put_bits(bits, kBitsChannel2, 4, channel2);
    q->_28 = 0u;
    bits = put_bits(bits, kBitsLine, 10, line & 0x3FF);
    q->bits = bits;

    const f32 advance = mono ? space : q->w.get();
    const f32 nx = f32_add(*px, advance);
    *px = nx;
    const f32 gap = f32_add(e->char_spacing, set->font_kerning[g.index]);
    *px = f32_madd(space, gap, nx);
    return 1;
}

}  // namespace sly4::text

NATIVE(004DAAA8, sly4::text::UiText_BuildGlyph)
