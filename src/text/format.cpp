// UiText_Format (0x004DB828).
#include "format.h"

namespace sly4::text {
using namespace opentit::native;

namespace {

constexpr f32 kZero = 0.0f;  // 0x004DB820
constexpr f32 kHalf = 0.5f;  // 0x004DB824

bool Ident_Char(u8 ch) {
    const s32 c = static_cast<s8>(ch);
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= 'a' && c <= 'z') return true;
    if (c >= '0' && c <= '9') return true;
    return ch == '_';
}

// Look `c` up in a sorted table: 1 = present, 0 = absent.
bool Table_Contains(u32 table, u32 count, u32 c) {
    const be_u32* t = host<be_u32>(table);
    for (u32 k = 0; k < count; ++k) {
        const u32 v = t[k];
        if (c < v) return false;
        if (c == v) return true;
    }
    return false;
}

}  // namespace

// Copies `text` into the buffer at *out (at most size - 1 bytes, then a
// terminator), measuring as it goes like UiText_MeasureLines:
//   "\n" (backslash, n) and '\n' start a new line; any other backslash is dropped;
//   $name is replaced by the string-table entry, formatted recursively (up to 10 deep);
//   spaces and tabs are break opportunities, '\r' becomes a space (CJK: is dropped);
//   past the wrap width (+180 bits 6..21) the last break becomes '\r' (CJK: a '\r'
//   is inserted there; without one, before an ideograph above U+3000).
// The state lives behind the eight pointers so that the recursion carries on
// where the outer call is (see FormatState). `buf` is only passed on.
void UiText_Format(TextElem* e, const char* text, char* buf, u32 size, be_u32* o_out, be_u32* o_bytes,
                   be_u32* o_glyphs, be_u32* o_on_line, be_f32* o_width, be_u32* o_brk, be_f32* o_brk_width,
                   be_u32* o_last) {
    const bool cjk = gref<be_u32>(g::config + kConfigLanguage) - kLanguageCjk == 0;
    void* strings = host<void>(global<be_u32>(kStrings));
    if (!text || *text == 0) return;

    // The original's frame: the variable name at +160 (the cursor, UTF-8
    // length and path cursor at +144..+152 are plain locals here). A name of
    // 64 characters or more overruns into the original's saved size and
    // buffer arguments; here it overruns this frame.
    GuestFrame frame(kVarNameSize);
    char* name = frame.as<char>();
    be_u32 len_v{}, pp_v{}, path_v{};
    be_u32* len = &len_v;
    be_u32* pp = &pp_v;

    const s32 depth = global<be_s32>(kFormatDepth);
    if (depth >= static_cast<s32>(kMaxDepth)) {
        path_v = call<u32>(tfn::String_Format, kDepthMessage);
        Ui_BuildPath(&e->elem, &path_v);
        return;
    }
    global<be_s32>(kFormatDepth) = depth + 1;

    auto out = [&]() { return o_out->get(); };
    auto put = [&](u8 byte) {
        const u32 w = out();
        *o_out = w + 1;
        gref<u8>(w) = byte;
    };
    auto bump = [](be_u32* p) { *p = p->get() + 1; };
    auto width = [&]() { return o_width->get(); };

    // The space width: as UiText_SpaceWidth, without the element's scale.
    const FontSet& set = gref(e->font_set);
    f32 space = kZero;
    const f32 reset = kZero;
    const f32 set_scale = set.scale;
    {
        const u32 count = set.font_count;
        const GlyphRef g = FindGlyph(set, ' ');
        const u32 font = g.index < count ? set.fonts[g.index].ea.get() : 0u;
        if (g.index < count && font != 0) {
            const u32 adv = gref<Glyph>(gref<Font>(font).glyphs.ea + g.glyph * kGlyphSize).advance;
            space = f32_mul(f32_mul(static_cast<f32>(adv), set.font_scale[g.index]), set_scale);
        } else if (count != 0 && set.fonts[0]) {
            space = f32_mul(f32_mul(static_cast<f32>(set.fonts[0]->line_height.get()), set_scale), kHalf);
        }
    }

    *pp = guest(text);
    for (;;) {
        u32 p = *pp;
        const u8 ch = gref<u8>(p);
        if (ch == 0) break;
        const u32 limit = size - 1;
        if (*o_bytes >= limit) break;
        const s32 sc = static_cast<s8>(ch);

        if (sc == '\\') {
            *pp = ++p;
            if (static_cast<s8>(gref<u8>(p)) != 'n') continue;
            put('\n');
            *o_last = static_cast<u32>('\n');
            bump(o_bytes);
            *o_width = reset;
            *o_brk = 0u;
            *pp = p + 1;
            bump(o_glyphs);
            *o_brk_width = reset;
            *o_on_line = 0u;
            continue;
        }
        if (sc == '$' && strings) {
            *pp = ++p;
            char* w = name;
            while (Ident_Char(gref<u8>(p))) {
                *pp = p + 1;
                *w++ = static_cast<char>(gref<u8>(p));
                ++p;
            }
            *w = 0;
            const char* value = StringTable_Lookup(strings, name, nullptr, false);
            UiText_Format(e, value, buf, size, o_out, o_bytes, o_glyphs, o_on_line, o_width, o_brk, o_brk_width, o_last);
            continue;
        }

        const u32 c = Utf8_Next(e, pp, len, false);
        if (c == ' ' || c == '\t') {
            const u32 src = *pp;
            *o_brk = out();
            *o_brk_width = width();
            *pp = src + 1;
            put(gref<u8>(src));
            bump(o_bytes);
            const f32 nw = f32_add(width(), space);
            bump(o_glyphs);
            bump(o_on_line);
            *o_width = nw;
            *o_last = static_cast<u32>(' ');
            continue;
        }
        if (c == '\r') {
            if (!cjk) {
                *o_brk = out();
                *o_brk_width = width();
                put(' ');
                bump(o_bytes);
                const f32 nw = f32_add(width(), space);
                bump(o_glyphs);
                bump(o_on_line);
                *o_last = static_cast<u32>(' ');
                *o_width = nw;
            }
            *pp = pp->get() + 1;
            continue;
        }
        if (c == '\n') {
            const u32 src = *pp;
            *pp = src + 1;
            put(gref<u8>(src));
            *o_width = reset;
            bump(o_bytes);
            *o_brk = 0u;
            *o_brk_width = reset;
            bump(o_glyphs);
            *o_on_line = 0u;
            *o_last = c;
            continue;
        }

        // Any other character. CJK: a break opportunity before it, unless the
        // line-breaking tables forbid one.
        if (cjk && c > 0x2000 && !Table_Contains(kNoBreakAfter, kNoBreakAfterCount, c) &&
            !Table_Contains(kNoBreakBefore, kNoBreakBeforeCount, c)) {
            *o_brk = out();
            *o_brk_width = width();
        }

        bool found = false;
        if ((e->text_flags & kTextMonospace) == 0) {
            const FontSet& s = gref(e->font_set);
            const GlyphRef g = FindGlyph(s, c);
            if (g.index < s.font_count) {
                const Font* font = s.fonts[g.index];
                if (font && c != ' ') {
                    found = true;
                    if (*o_on_line != 0u) {
                        const f32 gap = f32_add(e->char_spacing, s.font_kerning[g.index]);
                        *o_width = f32_madd(space, gap, width());
                    }
                    const FontSet& s2 = gref(e->font_set);
                    const u32 adv = gref<Glyph>(font->glyphs.ea + g.glyph * kGlyphSize).advance;
                    const f32 w = f32_mul(static_cast<f32>(adv), s2.font_scale[g.index]);
                    *o_width = f32_madd(w, set_scale, width());
                }
            }
        }
        if (!found) {
            if (*o_on_line != 0u) *o_width = f32_madd(space, e->char_spacing, width());
            *o_width = f32_add(width(), space);
        }

        const u32 wrap = (e->text_flags >> kTextWrapShift) & 0xFFFF;
        if (wrap != 0 && width() >= static_cast<f32>(wrap)) {
            const u32 brk = *o_brk;
            if (brk != 0) {
                u32 at = brk;
                if (cjk) {
                    // Make room for a '\r' at the break.
                    u32 w = out();
                    *o_out = w + 1;
                    at = *o_brk;
                    while (w > at) {
                        gref<u8>(w) = gref<u8>(w - 1);
                        --w;
                        at = *o_brk;
                    }
                    bump(o_bytes);
                }
                gref<u8>(at) = '\r';
                const f32 nw = f32_sub(width(), *o_brk_width);
                *o_brk = 0u;
                *o_width = nw;
                *o_brk_width = reset;
            } else if (cjk && c > 0x3000) {
                put('\r');
                *o_width = reset;
                bump(o_bytes);
                *o_brk = 0u;
                *o_brk_width = reset;
                bump(o_glyphs);
                bump(o_on_line);
            }
        }

        // Copy the character's UTF-8 bytes.
        for (;;) {
            const u32 n = *len;
            *len = n - 1;
            if (n == 0) break;
            if (*o_bytes >= limit) break;
            const u32 src = *pp;
            *pp = src + 1;
            put(gref<u8>(src));
            bump(o_bytes);
        }
        *o_last = c;
        bump(o_glyphs);
        bump(o_on_line);
    }

    gref<u8>(out()) = 0;
    global<be_s32>(kFormatDepth) = global<be_s32>(kFormatDepth) - 1;
}

}  // namespace sly4::text

NATIVE(004DB828, sly4::text::UiText_Format)
