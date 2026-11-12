// Setting a text element's text and laying out its glyphs.
#include "format.h"

#include "../common/common.h"
#include "../string/str.h"

namespace sly4::text {
using namespace opentit::native;

namespace {
constexpr u32 kFormatBufSize = 4096;
constexpr u32 kMaxLines = 256;      // widths[] in Layout (MeasureLines doesn't check it)
constexpr u32 kMaxLineIndex = 255;
constexpr f32 kHalfUp = -0.5f;      // 0x004DB36C
constexpr f32 kOne = 1.0f;          // 0x004DB370
constexpr f32 kZero = 0.0f;         // 0x004DB374

// Round a buffer size up the way SetText does: (n + 32) & ~31.
u32 TextBuf_Size(u32 n) { return (n + 32) & ~31u; }
}  // namespace

// Keeps a copy of `text`, has UiText_Format expand it into a 4 KiB stack
// buffer (reporting the space it needs for the formatted text and for its
// glyph quads), grows those buffers when they are too small (they are
// reallocated when the needed size is 0, too), copies the formatted text
// in, tells the element (vfunc +148) and lays it out again when asked and
// the element lays out automatically.
void UiText_SetText(TextElem* e, const char* text, bool relayout) {
    if (e->source) {
        common::SmallFree(e->source, e->source_cap);
        e->source = nullptr;
    }
    e->source_cap = 0u;
    if (text) {
        const u32 n = static_cast<u32>(str::String_Length(text));
        e->source_cap = n;
        if (n != 0) {
            e->source_cap = TextBuf_Size(n);
            e->source = static_cast<char*>(common::SmallAlloc(TextBuf_Size(n)));
            common::String_Assign(e->source, text);
        }
    }

    // The original's frame: the eight state words at +144 (the first starts
    // as the buffer pointer) and the buffer at +176.
    GuestFrame frame(sizeof(FormatState) + kFormatBufSize);
    FormatState* st = frame.as<FormatState>();
    char* buf = frame.as<char>(sizeof(FormatState));
    st->out = buf;
    st->bytes = 0u;
    st->glyphs = 0u;
    st->on_line = 0u;
    reinterpret_cast<be_u32&>(st->width) = 0u;
    st->brk = nullptr;
    reinterpret_cast<be_u32&>(st->brk_width) = 0u;
    st->last = 0u;
    UiText_Format(e, text, buf, kFormatBufSize, &st->out.ea, &st->bytes, &st->glyphs, &st->on_line, &st->width,
                  &st->brk.ea, &st->brk_width, &st->last);

    const u32 need_text = st->bytes;
    if (!(e->buf_cap > need_text && need_text != 0)) {
        if (e->buf) {
            common::SmallFree(e->buf, e->buf_cap);
            e->buf = nullptr;
        }
        if (need_text != 0) {
            e->buf_cap = TextBuf_Size(need_text);
            e->buf = static_cast<char*>(common::SmallAlloc(TextBuf_Size(need_text)));
        } else {
            e->buf_cap = 0u;
        }
    }

    const u32 need_glyphs = st->glyphs;
    if (e->quad_cap < need_glyphs || need_glyphs == 0) {
        if (e->quads) {
            common::SmallFree(e->quads, e->quad_bytes);
            e->quads = nullptr;
            e->quad_bytes = 0u;
        }
        if (need_glyphs != 0) {
            const u32 t = need_glyphs + 31;
            e->quad_cap = t & ~31u;
            const u32 bytes = (t << 5) & 0xFFFFFC00u;  // (t & ~31) * kQuadSize
            e->quad_bytes = bytes;
            GlyphQuad* quads = static_cast<GlyphQuad*>(common::SmallAlloc(bytes));
            e->quads = quads;
            call(fn::memset, quads, 0u, e->quad_bytes);
        } else {
            e->quad_cap = 0u;
        }
    }

    if (e->buf_cap != 0u) common::String_Assign(e->buf, buf);
    vcall(e, kVfnTextChanged);
    if (relayout && (e->text_flags & kTextAutoLayout) != 0) UiText_Layout(e, true);
}

// Builds the glyph quads for the formatted text: lines start at their
// alignment offset (UiText_MeasureLines), the block is offset vertically
// by -0.5 * lines * line height * v-align, spaces and glyphs without a
// quad advance by the space width, and the glyph and line counts end up in
// +216 / +220. Without `force` it only runs for an active element with a
// font set and a text buffer.
void UiText_Layout(TextElem* e, bool force) {
    if (!force) {
        if ((e->elem.flags & kElemActive) == 0) return;
        if (!e->font_set) return;
        if (e->buf_cap == 0u) return;
    }

    // The original's frame: scale (sret) at +112, widths[256] at +140, up
    // to its saved registers at +1192 (so that more than 256 lines overrun
    // this frame only, rather than the caller's).
    constexpr u32 kFrameBase = 112;
    GuestFrame frame(1192 - kFrameBase);
    Vec3f* scale = frame.as<Vec3f>(112 - kFrameBase);
    be_f32* widths = frame.as<be_f32>(140 - kFrameBase);
    be_u32 pp{};
    be_f32 px{}, py{};

    u32 line = 0, glyphs = 0;
    if (e->quad_cap != 0u) {
        call_ptr(vslot(e, kVfnGetScale), scale, e);
        const be_f32* sc = &scale->x;
        const f32 space = UiText_SpaceWidth(e, sc);
        const u32 lines = UiText_MeasureLines(e, widths, kMaxLines, 0, sc, space);

        const FontSet& set = gref(e->font_set);
        f32 line_h = e->line_spacing;
        const u32 flags = e->elem.flags;
        const f32 x0 = widths[0];
        GlyphQuad* quads = e->quads;
        const u32 quad_bytes = e->quad_bytes;
        f32 h = kZero;
        if (set.font_count != 0u) {
            const Font* font0 = set.fonts[0];
            if (font0) h = f32_mul(static_cast<f32>(font0->line_height.get()), set.line_scale);
        }
        h = f32_mul(h, scale->y);
        line_h = f32_mul(h, line_h);
        const s32 valign = static_cast<s32>((flags >> 18) & 3);
        px = x0;
        // fcfid + frsp: the count is rounded to single precision first.
        const f32 y = f32_mul(f32_mul(line_h, kHalfUp), static_cast<f32>(lines));
        py = f32_mul(y, static_cast<f32>(valign));
        call(fn::memset, quads, 0u, quad_bytes);

        pp = e->buf.ea;
        u32 index = 0, quad_off = 0;
        while (gref<u8>(pp) != 0 && index < e->quad_cap) {
            const u32 c = Utf8_Next(e, &pp, nullptr, true);
            bool advance_space = false;
            if (c == '\n' || c == '\r') {
                if (line < kMaxLineIndex) ++line;
                const f32 ny = f32_add(py, line_h);
                px = widths[line].get();
                py = ny;
                continue;
            }
            if (c == ' ') {
                advance_space = true;
            } else {
                GlyphQuad* quad = host<GlyphQuad>(e->quads.ea + quad_off);
                quad_off += kQuadSize;
                ++index;
                const u32 ok = UiText_BuildGlyph(e, quad, c, &px, &py, 0, sc, line, space);
                if (ok == 0)
                    advance_space = true;
                else
                    ++glyphs;
            }
            if (advance_space) px = f32_madd(space, f32_add(e->char_spacing, kOne), px);
        }
    }

    e->text_flags &= ~kTextAutoLayout;
    if (e->f224 > kZero) {
        reinterpret_cast<be_u32&>(e->glyph_count) = gref<be_u32>(kZeroVec2).get();
        reinterpret_cast<be_u32&>(e->line_count) = gref<be_u32>(kZeroVec2 + 4).get();
    } else {
        e->glyph_count = static_cast<f32>(glyphs);
        e->line_count = static_cast<f32>(line + 1);
    }
}

}  // namespace sly4::text

NATIVE(004DC1B8, sly4::text::UiText_SetText)
NATIVE(004DB378, sly4::text::UiText_Layout)
