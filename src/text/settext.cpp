// Setting a text element's text and laying out its glyphs.
#include "text.h"

using namespace sly4;
using namespace sly4::text;

namespace {
constexpr u32 kFormatBufSize = 4096;
constexpr u32 kFormatOutCount = 8;
constexpr u32 kMaxLines = 256;      // widths[] in Layout (MeasureLines doesn't check it)
constexpr u32 kMaxLineIndex = 255;
constexpr f32 kHalfUp = -0.5f;      // 0x004DB36C
constexpr f32 kOne = 1.0f;          // 0x004DB370
constexpr f32 kZero = 0.0f;         // 0x004DB374

// Round a buffer size up the way SetText does: (n + 32) & ~31.
u32 TextBuf_Size(u32 n) { return (n + 32) & ~31u; }
}  // namespace

// void UiText_SetText(TextElem* e, const char* text, bool relayout)
// Keeps a copy of `text`, has UiText_Format expand it into a 4 KiB stack
// buffer (reporting the space it needs for the formatted text and for its
// glyph quads), grows those buffers when they are too small (they are
// reallocated when the needed size is 0, too), copies the formatted text
// in, tells the element (vfunc +148) and lays it out again when asked and
// the element lays out automatically.
DECOMP(004DC1B8, UiText_SetText) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 text = arg<u32>(ctx, 1);
    const u8 relayout = arg<u8>(ctx, 2);

    if (field<u32>(e, kTextSource) != 0) {
        call(ctx, tfn::SmallFree, {field<u32>(e, kTextSource), field<u32>(e, kTextSourceCap)});
        field<u32>(e, kTextSource) = 0;
    }
    field<u32>(e, kTextSourceCap) = 0;
    if (text != 0) {
        const u32 n = call32(ctx, tfn::String_Length, {text});
        field<u32>(e, kTextSourceCap) = n;
        if (n != 0) {
            field<u32>(e, kTextSourceCap) = TextBuf_Size(n);
            field<u32>(e, kTextSource) = call32(ctx, tfn::SmallAlloc, {TextBuf_Size(n)});
            call(ctx, tfn::String_Assign, {field<u32>(e, kTextSource), text});
        }
    }

    // The original's frame: eight u32 results at +144 (the first starts as
    // the buffer pointer) and the buffer at +176.
    constexpr u32 kFrameBase = 144;
    Stack frame(ctx, 176 + kFormatBufSize - kFrameBase);
    const u32 outs = frame.addr();
    const u32 buf = outs + 32;
    store<u32>(outs, buf);
    for (u32 k = 1; k < kFormatOutCount; ++k) store<u32>(outs + k * 4, 0);
    call(ctx, tfn::UiText_Format,
         {e, text, buf, kFormatBufSize, outs, outs + 4, outs + 8, outs + 12, outs + 16, outs + 20, outs + 24,
          outs + 28});

    const u32 need_text = load<u32>(outs + 4);
    if (!(field<u32>(e, kTextBufCap) > need_text && need_text != 0)) {
        if (field<u32>(e, kTextBuf) != 0) {
            call(ctx, tfn::SmallFree, {field<u32>(e, kTextBuf), field<u32>(e, kTextBufCap)});
            field<u32>(e, kTextBuf) = 0;
        }
        if (need_text != 0) {
            field<u32>(e, kTextBufCap) = TextBuf_Size(need_text);
            field<u32>(e, kTextBuf) = call32(ctx, tfn::SmallAlloc, {TextBuf_Size(need_text)});
        } else {
            field<u32>(e, kTextBufCap) = 0;
        }
    }

    const u32 need_glyphs = load<u32>(outs + 8);
    if (field<u32>(e, kTextQuadCap) < need_glyphs || need_glyphs == 0) {
        if (field<u32>(e, kTextQuads) != 0) {
            call(ctx, tfn::SmallFree, {field<u32>(e, kTextQuads), field<u32>(e, kTextQuadBytes)});
            field<u32>(e, kTextQuads) = 0;
            field<u32>(e, kTextQuadBytes) = 0;
        }
        if (need_glyphs != 0) {
            const u32 t = need_glyphs + 31;
            field<u32>(e, kTextQuadCap) = t & ~31u;
            const u32 bytes = (t << 5) & 0xFFFFFC00u;  // (t & ~31) * kQuadSize
            field<u32>(e, kTextQuadBytes) = bytes;
            const u32 quads = call32(ctx, tfn::SmallAlloc, {bytes});
            field<u32>(e, kTextQuads) = quads;
            call(ctx, fn::memset, {quads, 0, field<u32>(e, kTextQuadBytes)});
        } else {
            field<u32>(e, kTextQuadCap) = 0;
        }
    }

    if (field<u32>(e, kTextBufCap) != 0) call(ctx, tfn::String_Assign, {field<u32>(e, kTextBuf), buf});
    call_ptr(ctx, vfunc(e, kVfnTextChanged), {e});
    if (relayout != 0 && (field<u32>(e, kTextFlags) & kTextAutoLayout) != 0) call(ctx, tfn::UiText_Layout, {e, 1});
}

// void UiText_Layout(TextElem* e, bool force)
// Builds the glyph quads for the formatted text: lines start at their
// alignment offset (UiText_MeasureLines), the block is offset vertically
// by -0.5 * lines * line height * v-align, spaces and glyphs without a
// quad advance by the space width, and the glyph and line counts end up in
// +216 / +220. Without `force` it only runs for an active element with a
// font set and a text buffer.
DECOMP(004DB378, UiText_Layout) {
    const u32 e = arg<u32>(ctx, 0);
    const u8 force = arg<u8>(ctx, 1);
    if (force == 0) {
        if ((field<u32>(e, kElemFlags) & kElemActive) == 0) return;
        if (field<u32>(e, kTextFontSet) == 0) return;
        if (field<u32>(e, kTextBufCap) == 0) return;
    }

    // The original's frame: scale (sret) at +112, the text cursor at +128,
    // x and y at +132 / +136, widths[256] at +140, up to its saved
    // registers at +1192 (so that more than 256 lines overrun this frame
    // only, rather than the caller's).
    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 1192 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 scale = sp + 112, pp = sp + 128, px = sp + 132, py = sp + 136, widths = sp + 140;

    u32 line = 0, glyphs = 0;
    if (field<u32>(e, kTextQuadCap) != 0) {
        call_ptr(ctx, vfunc(e, kVfnGetScale), {scale, e});
        const f64 space = callf(ctx, tfn::UiText_SpaceWidth, {e, scale}, {});
        ctx.fpr[1] = space;
        const u32 lines = call32(ctx, tfn::UiText_MeasureLines, {e, widths, kMaxLines, 0, scale});

        const u32 set = field<u32>(e, kTextFontSet);
        f64 line_h = lfs(e + kTextLineSpacing);
        const u32 flags = field<u32>(e, kElemFlags);
        const f64 x0 = lfs(widths);
        const u32 quads = field<u32>(e, kTextQuads);
        const u32 quad_bytes = field<u32>(e, kTextQuadBytes);
        f64 h = kZero;
        if (field<u32>(set, kSetFontCount) != 0) {
            const u32 font0 = load<u32>(set + kSetFonts);
            if (font0 != 0)
                h = fmuls(static_cast<f64>(field<u16>(font0, kFontLineHeight).get()), lfs(set + kSetLineScale));
        }
        h = fmuls(h, lfs(scale + 4));
        line_h = fmuls(h, line_h);
        const s32 valign = static_cast<s32>((flags >> 18) & 3);
        stfs(px, x0);
        // fcfid + frsp: the count is rounded to single precision first.
        f64 y = fmuls(fmuls(line_h, kHalfUp), static_cast<f64>(static_cast<f32>(lines)));
        stfs(py, fmuls(y, static_cast<f64>(valign)));
        call(ctx, fn::memset, {quads, 0, quad_bytes});

        store<u32>(pp, field<u32>(e, kTextBuf));
        u32 index = 0, quad_off = 0;
        while (load<u8>(load<u32>(pp)) != 0 && index < field<u32>(e, kTextQuadCap)) {
            const u32 c = call32(ctx, tfn::Utf8_Next, {e, pp, 0, 1});
            bool advance_space = false;
            if (c == '\n' || c == '\r') {
                if (line < kMaxLineIndex) ++line;
                const f64 ny = fadds(lfs(py), line_h);
                stfs(px, lfs(widths + line * 4));
                stfs(py, ny);
                continue;
            }
            if (c == ' ') {
                advance_space = true;
            } else {
                const u32 quad = field<u32>(e, kTextQuads) + quad_off;
                quad_off += kQuadSize;
                ++index;
                ctx.fpr[1] = space;
                // r8 is a leftover the glyph builder doesn't read.
                const u32 ok = call32(ctx, tfn::UiText_BuildGlyph, {e, quad, c, px, py, 0, scale, line});
                if (ok == 0)
                    advance_space = true;
                else
                    ++glyphs;
            }
            if (advance_space)
                stfs(px, fmadds(space, fadds(lfs(e + kTextCharSpacing), kOne), lfs(px)));
        }
    }

    field<u32>(e, kTextFlags) &= ~kTextAutoLayout;
    if (lfs(e + kText224) > static_cast<f64>(kZero)) {
        stfs(e + kTextGlyphCount, lfs(kZeroVec2));
        stfs(e + kTextLineCount, lfs(kZeroVec2 + 4));
    } else {
        stfs(e + kTextGlyphCount, static_cast<f64>(glyphs));
        stfs(e + kTextLineCount, static_cast<f64>(line + 1));
    }
}
