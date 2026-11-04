// UiText_BuildGlyph (0x004DAAA8).
#include "glyph_build.h"

using namespace sly4;
using namespace sly4::text;

namespace {

constexpr f32 kLineTop = 3.0f;      // 0x004DAA80
constexpr f32 kQuarter = 0.25f;     // 0x004DAA84
constexpr f32 kHalf = 0.5f;         // 0x004DAA88
constexpr f32 kNoFontHeight = 0.0f; // 0x004DAA8C

inline f64 fsubs(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a - b)); }
// fnmsubs d = -(a * c - b), as the recompiler does it.
inline f64 fnmsubs(f64 a, f64 c, f64 b) { return static_cast<f64>(static_cast<f32>(-std::fma(a, c, -b))); }
// fcfid + frsp of a small unsigned integer.
inline f64 to_f32(u32 v) { return static_cast<f64>(static_cast<f32>(static_cast<f64>(v))); }

inline u64 put_bits(u64 word, u32 shift, u32 width, u64 value) {
    const u64 mask = ((u64{1} << width) - 1) << shift;
    return (word & ~mask) | ((value << shift) & mask);
}

}  // namespace

// bool UiText_BuildGlyph(TextElem* e, Quad* q, u32 c, f32* x, f32* y, -, const f32* scale, u32 line; f1 = space)
// Fills the quad for code point `c` at (*x, *y) and advances *x past it
// (by its width, or the space width for a monospaced element, plus the
// spacing and the font's kerning, scaled by the space width). Returns 0,
// touching nothing, when no font has the glyph.
DECOMP(004DAAA8, UiText_BuildGlyph) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 q = arg<u32>(ctx, 1);
    const u32 c = arg<u32>(ctx, 2);
    const u32 px = arg<u32>(ctx, 3);
    const u32 py = arg<u32>(ctx, 4);
    const u32 scale = arg<u32>(ctx, 6);
    const u32 line = arg<u32>(ctx, 7);
    const f64 space = farg(ctx, 0);

    const u32 set = field<u32>(e, kTextFontSet);
    if (set == 0) return ret(ctx, 0u);
    const u32 count = field<u32>(set, kSetFontCount);
    const GlyphRef g = FontSet_FindGlyph(set, c);
    if (g.index >= count) return ret(ctx, 0u);
    const u32 font = load<u32>(set + kSetFonts + g.index * 4);
    if (font == 0) return ret(ctx, 0u);

    const f64 set_scale = lfs(set + kSetScale);
    const f64 line_scale = lfs(set + kSetLineScale);
    f64 top = kNoFontHeight;
    if (count != 0 && load<u32>(set + kSetFonts) != 0)
        top = fmuls(static_cast<f64>(field<u16>(load<u32>(set + kSetFonts), kFontLineHeight).get()), line_scale);
    const f64 sx = lfs(scale), sy = lfs(scale + 4);
    top = fmuls(fmuls(top, sy), kLineTop);

    const u32 field_a = (field<u32>(e, kElemFlags) >> 22) & 3;
    const u32 glyph = field<u32>(font, kFontGlyphs) + g.glyph * kGlyphSize;
    const u32 g0 = load<u8>(glyph + 0), g1 = load<u8>(glyph + 1), g2 = load<u8>(glyph + 2), g3 = load<u8>(glyph + 3);
    const u32 rows = g0 + g1 + 1;
    const f64 font_scale = lfs(set + g.index * 4 + kSetFontScale);

    // Bit 19 first (stored on its own in the original), then the rest.
    u64 bits = load<u64>(q + kQuadBits);
    bits = put_bits(bits, kBitsFontFlag, 1, (ref<u8>(font + kFontFlags) >> 7) & 1);
    store<u32>(q + kQuadPage, load<u32>(font + kFontPages + field_a * 4));

    const f64 down = fmuls(fmuls(to_f32(g0), font_scale), line_scale);
    const f64 width = fmuls(fmuls(fmuls(to_f32(g2), font_scale), set_scale), sx);
    const bool mono = (field<u32>(e, kTextFlags) & kTextMonospace) != 0;
    const f64 qx = mono ? fmadds(fsubs(space, width), kHalf, lfs(px)) : lfs(px);
    stfs(q + kQuadX, qx);
    f64 qy = fmadds(top, kQuarter, lfs(py));
    qy = fnmsubs(down, sy, qy);
    const f64 height = fmuls(fmuls(fmuls(to_f32(rows), font_scale), line_scale), sy);
    stfs(q + kQuadY, qy);
    stfs(q + kQuadW, width);
    stfs(q + kQuadH, height);

    // The original converts width and rows back through float and fctidz;
    // both are small integers, so that is the integer itself.
    bits = put_bits(bits, kBitsTexU, 11, load<u16>(glyph + 4));
    bits = put_bits(bits, kBitsTexV, 11, load<u16>(glyph + 6));
    bits = put_bits(bits, kBitsWidth, 11, g2);
    bits = put_bits(bits, kBitsHeight, 11, rows);
    bits = put_bits(bits, kBitsChannel, 4, g3);
    const s32 mode = static_cast<s32>((field<u32>(e, kTextFlags) >> 22) & 0xF);
    const u64 channel2 = mode < 3 ? 0 : static_cast<u64>(((bits >> kBitsChannel) & 0xF) - 1);
    bits = put_bits(bits, kBitsChannel2, 4, channel2);
    store<u32>(q + kQuad28, 0);
    bits = put_bits(bits, kBitsLine, 10, line & 0x3FF);
    store<u64>(q + kQuadBits, bits);

    const f64 advance = mono ? space : lfs(q + kQuadW);
    const f64 nx = fadds(lfs(px), advance);
    stfs(px, nx);
    const f64 gap = fadds(lfs(e + kTextCharSpacing), lfs(set + g.index * 4 + kSetFontKerning));
    stfs(px, fmadds(space, gap, nx));
    ret(ctx, 1u);
}
