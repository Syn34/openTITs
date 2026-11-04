// UiText_Format (0x004DB828).
#include "format.h"

using namespace sly4;
using namespace sly4::text;

namespace {

constexpr f32 kZero = 0.0f;  // 0x004DB820
constexpr f32 kHalf = 0.5f;  // 0x004DB824

inline f64 fsubs(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a - b)); }

bool Ident_Char(u8 ch) {
    const s32 c = static_cast<s8>(ch);
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= 'a' && c <= 'z') return true;
    if (c >= '0' && c <= '9') return true;
    return ch == '_';
}

// Look `c` up in a sorted table: 1 = present, 0 = absent.
bool Table_Contains(u32 table, u32 count, u32 c) {
    for (u32 k = 0; k < count; ++k) {
        const u32 t = load<u32>(table + k * 4);
        if (c < t) return false;
        if (c == t) return true;
    }
    return false;
}

struct Format {
    u32 e, buf_arg, size_arg;
    u32 o[8];  // the eight state pointers (FormatState fields)
    f64 space, set_scale;
    bool cjk;

    u32 out() const { return load<u32>(o[0]); }
    void put(u8 byte) {
        const u32 w = out();
        store<u32>(o[0], w + 1);
        store<u8>(w, byte);
    }
    void bump(int k) { store<u32>(o[k], load<u32>(o[k]) + 1); }
    f64 width() const { return lfs(o[4]); }
};

}  // namespace

// void UiText_Format(TextElem* e, const char* text, char* buf, u32 size,
//                    char** out, u32* bytes, u32* glyphs, u32* on_line,
//                    f32* width, char** brk, f32* brk_width, u32* last)
// Copies `text` into the buffer at *out (at most size - 1 bytes, then a
// terminator), measuring as it goes like UiText_MeasureLines:
//   "\n" (backslash, n) and '\n' start a new line; any other backslash is dropped;
//   $name is replaced by the string-table entry, formatted recursively (up to 10 deep);
//   spaces and tabs are break opportunities, '\r' becomes a space (CJK: is dropped);
//   past the wrap width (+180 bits 6..21) the last break becomes '\r' (CJK: a '\r'
//   is inserted there; without one, before an ideograph above U+3000).
DECOMP(004DB828, UiText_Format) {
    const u32 entry_sp = static_cast<u32>(ctx.gpr[1]);
    Format f{arg<u32>(ctx, 0), arg<u32>(ctx, 2), arg<u32>(ctx, 3), {}, 0, 0, false};
    const u32 text = arg<u32>(ctx, 1);
    for (int k = 0; k < 4; ++k) f.o[k] = arg<u32>(ctx, 4 + k);
    for (int k = 0; k < 4; ++k) f.o[4 + k] = load<u32>(entry_sp + 116 + 8 * k);  // arguments 9-12

    f.cjk = field<u32>(g::config, kConfigLanguage) - kLanguageCjk == 0;
    const u32 strings = load<u32>(kStrings);
    if (text == 0 || load<u8>(text) == 0) return;

    // The original's frame: the variable name at +160, the text cursor at
    // +148, the UTF-8 length at +144, the path cursor at +152. (A name of
    // 64 characters or more overruns into the original's saved size and
    // buffer arguments; here it overruns this frame.)
    constexpr u32 kFrameBase = 144;
    Stack frame(ctx, 160 + kVarNameSize - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 len = sp + 144, pp = sp + 148, path = sp + 152, name = sp + 160;

    const s32 depth = load<s32>(kFormatDepth);
    if (depth >= static_cast<s32>(kMaxDepth)) {
        store<u32>(path, call32(ctx, tfn::String_Format, {kDepthMessage}));
        call(ctx, tfn::Ui_BuildPath, {f.e, path});
        return;
    }
    store<s32>(kFormatDepth, depth + 1);

    // The space width: as UiText_SpaceWidth, without the element's scale.
    const u32 set = field<u32>(f.e, kTextFontSet);
    f.space = kZero;
    const f64 reset = kZero;
    f.set_scale = lfs(set + kSetScale);
    {
        const u32 count = field<u32>(set, kSetFontCount);
        const GlyphRef g = FontSet_FindGlyph(set, ' ');
        const u32 font = g.index < count ? load<u32>(set + kSetFonts + g.index * 4) : 0;
        if (g.index < count && font != 0) {
            const u32 adv = load<u8>(field<u32>(font, kFontGlyphs) + g.glyph * kGlyphSize + kGlyphAdvance);
            f.space = fmuls(fmuls(static_cast<f64>(adv), lfs(set + g.index * 4 + kSetFontScale)), f.set_scale);
        } else if (count != 0 && load<u32>(set + kSetFonts) != 0) {
            const u32 font0 = load<u32>(set + kSetFonts);
            f.space = fmuls(fmuls(static_cast<f64>(field<u16>(font0, kFontLineHeight).get()), f.set_scale), kHalf);
        }
    }

    store<u32>(pp, text);
    for (;;) {
        u32 p = load<u32>(pp);
        const u8 ch = load<u8>(p);
        if (ch == 0) break;
        const u32 limit = f.size_arg - 1;
        if (load<u32>(f.o[1]) >= limit) break;
        const s32 sc = static_cast<s8>(ch);

        if (sc == '\\') {
            store<u32>(pp, ++p);
            if (static_cast<s8>(load<u8>(p)) != 'n') continue;
            f.put('\n');
            store<u32>(f.o[7], '\n');
            f.bump(1);
            stfs(f.o[4], reset);
            store<u32>(f.o[5], 0);
            store<u32>(pp, p + 1);
            f.bump(2);
            stfs(f.o[6], reset);
            store<u32>(f.o[3], 0);
            continue;
        }
        if (sc == '$' && strings != 0) {
            store<u32>(pp, ++p);
            u32 w = name;
            while (Ident_Char(load<u8>(p))) {
                store<u32>(pp, p + 1);
                store<u8>(w++, load<u8>(p));
                ++p;
            }
            store<u8>(w, 0);
            const u32 value = call32(ctx, tfn::StringTable_Lookup, {strings, name, 0, 0});
            call(ctx, tfn::UiText_Format,
                 {f.e, value, f.buf_arg, f.size_arg, f.o[0], f.o[1], f.o[2], f.o[3], f.o[4], f.o[5], f.o[6], f.o[7]});
            continue;
        }

        const u32 c = call32(ctx, tfn::Utf8_Next, {f.e, pp, len, 0});
        if (c == ' ' || c == '\t') {
            const u32 src = load<u32>(pp);
            store<u32>(f.o[5], f.out());
            stfs(f.o[6], f.width());
            store<u32>(pp, src + 1);
            f.put(load<u8>(src));
            f.bump(1);
            const f64 nw = fadds(f.width(), f.space);
            f.bump(2);
            f.bump(3);
            stfs(f.o[4], nw);
            store<u32>(f.o[7], ' ');
            continue;
        }
        if (c == '\r') {
            if (!f.cjk) {
                store<u32>(f.o[5], f.out());
                stfs(f.o[6], f.width());
                f.put(' ');
                f.bump(1);
                const f64 nw = fadds(f.width(), f.space);
                f.bump(2);
                f.bump(3);
                store<u32>(f.o[7], ' ');
                stfs(f.o[4], nw);
            }
            store<u32>(pp, load<u32>(pp) + 1);
            continue;
        }
        if (c == '\n') {
            const u32 src = load<u32>(pp);
            store<u32>(pp, src + 1);
            f.put(load<u8>(src));
            stfs(f.o[4], reset);
            f.bump(1);
            store<u32>(f.o[5], 0);
            stfs(f.o[6], reset);
            f.bump(2);
            store<u32>(f.o[3], 0);
            store<u32>(f.o[7], c);
            continue;
        }

        // Any other character. CJK: a break opportunity before it, unless the
        // line-breaking tables forbid one.
        if (f.cjk && c > 0x2000 && !Table_Contains(kNoBreakAfter, kNoBreakAfterCount, c) &&
            !Table_Contains(kNoBreakBefore, kNoBreakBeforeCount, c)) {
            store<u32>(f.o[5], f.out());
            stfs(f.o[6], f.width());
        }

        bool found = false;
        if ((field<u32>(f.e, kTextFlags) & kTextMonospace) == 0) {
            const u32 s = field<u32>(f.e, kTextFontSet);
            const GlyphRef g = FontSet_FindGlyph(s, c);
            if (g.index < field<u32>(s, kSetFontCount)) {
                const u32 font = load<u32>(s + kSetFonts + g.index * 4);
                if (font != 0 && c != ' ') {
                    found = true;
                    if (load<u32>(f.o[3]) != 0) {
                        const f64 gap = fadds(lfs(f.e + kTextCharSpacing), lfs(s + g.index * 4 + kSetFontKerning));
                        stfs(f.o[4], fmadds(f.space, gap, f.width()));
                    }
                    const u32 s2 = field<u32>(f.e, kTextFontSet);
                    const u32 adv = load<u8>(field<u32>(font, kFontGlyphs) + g.glyph * kGlyphSize + kGlyphAdvance);
                    const f64 w = fmuls(static_cast<f64>(adv), lfs(s2 + g.index * 4 + kSetFontScale));
                    stfs(f.o[4], fmadds(w, f.set_scale, f.width()));
                }
            }
        }
        if (!found) {
            if (load<u32>(f.o[3]) != 0) stfs(f.o[4], fmadds(f.space, lfs(f.e + kTextCharSpacing), f.width()));
            stfs(f.o[4], fadds(f.width(), f.space));
        }

        const u32 wrap = (field<u32>(f.e, kTextFlags) >> kTextWrapShift) & 0xFFFF;
        if (wrap != 0 && f.width() >= static_cast<f64>(static_cast<f32>(wrap))) {
            const u32 brk = load<u32>(f.o[5]);
            if (brk != 0) {
                u32 at = brk;
                if (f.cjk) {
                    // Make room for a '\r' at the break.
                    u32 w = f.out();
                    store<u32>(f.o[0], w + 1);
                    at = load<u32>(f.o[5]);
                    while (w > at) {
                        store<u8>(w, load<u8>(w - 1));
                        --w;
                        at = load<u32>(f.o[5]);
                    }
                    f.bump(1);
                }
                store<u8>(at, '\r');
                const f64 nw = fsubs(f.width(), lfs(f.o[6]));
                store<u32>(f.o[5], 0);
                stfs(f.o[4], nw);
                stfs(f.o[6], reset);
            } else if (f.cjk && c > 0x3000) {
                f.put('\r');
                stfs(f.o[4], reset);
                f.bump(1);
                store<u32>(f.o[5], 0);
                stfs(f.o[6], reset);
                f.bump(2);
                f.bump(3);
            }
        }

        // Copy the character's UTF-8 bytes.
        for (;;) {
            const u32 n = load<u32>(len);
            store<u32>(len, n - 1);
            if (n == 0) break;
            if (load<u32>(f.o[1]) >= limit) break;
            const u32 src = load<u32>(pp);
            store<u32>(pp, src + 1);
            f.put(load<u8>(src));
            f.bump(1);
        }
        store<u32>(f.o[7], c);
        f.bump(2);
        f.bump(3);
    }

    store<u8>(f.out(), 0);
    store<s32>(kFormatDepth, load<s32>(kFormatDepth) - 1);
}
