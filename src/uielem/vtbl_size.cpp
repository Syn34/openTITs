// UiElem_GetSize (vfunc +140, 0x004E1B58): a text element's measured size.
#include "vtbl.h"

using namespace sly4;
using namespace sly4::uielem;

namespace {
constexpr f32 kZero = 0.0f;  // 0x004E1B38
constexpr f32 kHalf = 0.5f;  // 0x004E1B50
constexpr u32 kSizeDirty = 0x02000000;  // +136: re-measure on the next call
constexpr u32 kCachedSize = 140;        // f32[2] at +140 / +144
constexpr u32 kRevealTime = 224;        // f32: seconds per character, > 0 to reveal gradually (name?)
constexpr u32 kRevealElapsed = 228;     // f32 (name?)
inline f64 to_f32(u32 v) { return static_cast<f64>(static_cast<f32>(static_cast<f64>(v))); }
}  // namespace

// Vec2 UiElem_GetSize(Element* e)  [sret in r3, e in r4]
// The cached size at +140, re-measured when flag 0x02000000 is set: the
// widest line and the total height of the text (only the characters
// revealed so far when +224 > 0), or (0, 0) without fonts or text.
DECOMP(004E1B58, UiElem_GetSize) {
    const u32 out = arg<u32>(ctx, 0);
    const u32 e = arg<u32>(ctx, 1);

    if ((field<u32>(e, kElemFlags) & kSizeDirty) == 0) {
        stfs(out, lfs(e + kCachedSize));
        stfs(out + 4, lfs(e + kCachedSize + 4));
        return;
    }

    f64 max_w = lfs(kZeroVec2), height = lfs(kZeroVec2 + 4);
    if (field<u32>(e, kTextFontSet) != 0 && field<u32>(e, kTextBufCap) != 0) {
        constexpr u32 kFrameBase = 112;
        Stack frame(ctx, 144 - kFrameBase);
        const u32 sp = frame.addr() - kFrameBase;
        const u32 scale = sp + 112, pp = sp + 128;

        const f64 set_scale = lfs(field<u32>(e, kTextFontSet) + kSetScale);
        call_ptr(ctx, vfunc(e, kVfnGetScale), {scale, e});
        const u32 set = field<u32>(e, kTextFontSet);
        const u32 count = field<u32>(set, kSetFontCount);

        // The space width (as UiText_SpaceWidth).
        f64 space = kZero;
        const GlyphRef sg = FontSet_FindGlyph(set, ' ');
        const u32 sfont = sg.index < count ? load<u32>(set + kSetFonts + sg.index * 4) : 0;
        if (sg.index < count && sfont != 0) {
            const u32 adv = load<u8>(field<u32>(sfont, kFontGlyphs) + sg.glyph * kGlyphSize + kGlyphAdvance);
            space = fmuls(fmuls(fmuls(to_f32(adv), lfs(set + sg.index * 4 + kSetFontScale)), set_scale), lfs(scale));
        } else if (count != 0 && load<u32>(set + kSetFonts) != 0) {
            const u32 lh = field<u16>(load<u32>(set + kSetFonts), kFontLineHeight);
            space = fmuls(fmuls(to_f32(lh), set_scale), kHalf);
        }

        f64 line_h = kZero;
        if (count != 0 && load<u32>(set + kSetFonts) != 0) {
            const u32 lh = field<u16>(load<u32>(set + kSetFonts), kFontLineHeight);
            line_h = fmuls(to_f32(lh), lfs(set + kSetLineScale));
        }
        const f64 reveal = lfs(e + kRevealTime);
        line_h = fmuls(line_h, lfs(scale + 4));
        u32 p = field<u32>(e, kTextBuf);
        store<u32>(pp, p);
        u32 line_start = p;
        height = line_h;

        u32 limit = field<u32>(e, kTextQuadCap);
        if (reveal > static_cast<f64>(kZero)) {
            const u32 n = static_cast<u32>(rt::fctid(static_cast<f64>(static_cast<f32>(lfs(e + kRevealElapsed) / reveal)), true));
            if (n < limit) limit = n;
        }
        max_w = kZero;
        f64 line_w = kZero;
        u32 glyphs = 0;
        while (load<u8>(p) != 0 && glyphs < limit) {
            const u32 c = static_cast<u32>(call(ctx, tfn::Utf8_Next, {e, pp, 0, 1}));
            p = load<u32>(pp);
            if (c == '\n' || c == '\r') {
                const f64 h = fmadds(line_h, lfs(e + kTextLineSpacing), height);
                line_w = kZero;
                height = h < static_cast<f64>(kZero) ? static_cast<f64>(kZero) : h;
                height = fadds(height, line_h);
                line_start = p;
                continue;
            }
            f64 w;
            bool found = false;
            if ((field<u32>(e, kTextFlags) & kTextMonospace) == 0) {
                const u32 s = field<u32>(e, kTextFontSet);
                const GlyphRef g = FontSet_FindGlyph(s, c);
                if (g.index < field<u32>(s, kSetFontCount)) {
                    const u32 font = load<u32>(s + kSetFonts + g.index * 4);
                    if (font != 0 && c != ' ') {
                        found = true;
                        if (p > line_start) {
                            const f64 gap = fadds(lfs(e + kTextCharSpacing), lfs(s + g.index * 4 + kSetFontKerning));
                            line_w = fmadds(space, gap, line_w);
                        }
                        const u32 adv = load<u8>(field<u32>(font, kFontGlyphs) + g.glyph * kGlyphSize + kGlyphAdvance);
                        ++glyphs;
                        w = fmuls(fmuls(fmuls(to_f32(adv), lfs(s + g.index * 4 + kSetFontScale)), set_scale), lfs(scale));
                    }
                }
            }
            if (!found) {
                if (p > line_start) line_w = fmadds(space, lfs(e + kTextCharSpacing), line_w);
                w = space;
            }
            line_w = fadds(w, line_w);
            if (max_w < line_w) max_w = line_w;
        }
    }

    field<u32>(e, kElemFlags) &= ~kSizeDirty;
    stfs(e + kCachedSize, max_w);
    stfs(e + kCachedSize + 4, height);
    stfs(out, max_w);
    stfs(out + 4, lfs(e + kCachedSize + 4));
}
