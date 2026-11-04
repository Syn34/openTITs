// UiQuad_Update, UiText_UpdateQuads, UiElem_UpdateChildren, UiText2_Update.
#include "update.h"

using namespace sly4;
using namespace sly4::uielem;
using namespace sly4::text;

namespace {

constexpr f32 kZero = 0.0f;       // 0x004E239C / 0x0024FB9C
constexpr f32 kHalfUp = -0.5f;    // 0x0024FBA0
constexpr f32 kOne = 1.0f;        // 0x0024FBA4

inline u64 Get(u64 b, u32 shift, u32 width) { return (b >> shift) & ((u64{1} << width) - 1); }
inline u64 Put(u64 b, u32 shift, u32 width, u64 v) {
    const u64 m = ((u64{1} << width) - 1) << shift;
    return (b & ~m) | ((v << shift) & m);
}
inline f64 fsubs(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a - b)); }
inline f64 to_f32(u64 v) { return static_cast<f64>(static_cast<f32>(static_cast<f64>(v))); }

}  // namespace

// void UiQuad_Update(Quad* q; f1 = dt, f2 = period, r5 = mode)
// Steps a multi-cell glyph's cell once per `period` seconds (catching up
// when dt spans several), as set by the mode.
DECOMP(004E1454, UiQuad_Update) {
    const u32 q = arg<u32>(ctx, 0);
    const s32 mode = arg<s32>(ctx, 2);
    const f64 dt = farg(ctx, 0), period = farg(ctx, 1);

    u64 bits = load<u64>(q + kQuadBits);
    if (Get(bits, kQuadCells, 4) <= 1) return;
    f64 t = fadds(lfs(q + kQuadTimer), dt);
    stfs(q + kQuadTimer, t);
    while (t >= period) {
        const u64 cell = Get(bits, kQuadCell, 4), cells = Get(bits, kQuadCells, 4);
        if (mode == kCellPingPong) {
            const u64 dir = Get(bits, kQuadPingPong, 1);
            bits = Put(bits, kQuadCell, 4, dir == 0 ? cell + 1 : cell - 1);
            const u64 c2 = Get(bits, kQuadCell, 4);
            const u64 at_end = (c2 == Get(bits, kQuadCells, 4) - 1 || c2 == 0) ? 1 : 0;
            bits = Put(bits, kQuadPingPong, 1, Get(bits, kQuadPingPong, 1) ^ at_end);
        } else if (mode == kCellOnce) {
            const u64 next = cell + 1, last = cells - 1;
            bits = Put(bits, kQuadCell, 4, next < last ? next : last);
        } else if (mode == kCellLoopBack) {
            bits = Put(bits, kQuadCell, 4, (cell != 0 ? cell : cells) - 1);
        } else if (mode == kCellOnceBack) {
            const s32 prev = static_cast<s32>(cell) - 1;
            bits = Put(bits, kQuadCell, 4, prev >= 0 ? static_cast<u64>(static_cast<s64>(prev)) : 0);
        } else {  // mode <= 0 or > 4
            bits = Put(bits, kQuadCell, 4, (cell + 1) % cells);
        }
        store<u64>(q + kQuadBits, bits);
        t = fsubs(t, period);
        stfs(q + kQuadTimer, t);
    }
}

// void UiText_UpdateQuads(TextElem* e; f1 = dt)
// UiText_Update without the layout and the children: advances the reveal
// clock and steps every quad's cells.
DECOMP(004E23A0, UiText_UpdateQuads) {
    const u32 e = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const u32 flags = field<u32>(e, kElemFlags);
    if ((flags & kElemActive) == 0) return;
    stfs(e + kTextRevealElapsed, fadds(lfs(e + kTextRevealElapsed), dt));
    field<u32>(e, kElemFlags) = flags | (lfs(e + kTextReveal) > static_cast<f64>(kZero) ? kElemSizeDirty : 0);
    for (u32 k = 0; k < field<u32>(e, kTextQuadCap); ++k) {
        ctx.fpr[1] = dt;
        ctx.fpr[2] = lfs(e + kText232);
        call(ctx, ufn2::UiQuad_Update,
             {field<u32>(e, kTextQuads) + k * kQuadSize, 0, (field<u32>(e, kTextFlags) >> 22) & 0xF});
    }
}

// void UiElem_UpdateChildren(Element* e; f1 = dt)
DECOMP(004D89CC, UiElem_UpdateChildren) {
    const u32 e = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    if ((field<u32>(e, kElemFlags) & kElemActive) == 0) return;
    for (u32 c = field<u32>(e, kElemFirstChild); c != 0; c = field<u32>(c, kElemNext)) {
        ctx.fpr[1] = dt;
        const u32 opd = vfunc(c, kVfnUpdate);
        call_ptr(ctx, opd, {c, opd, load<u32>(opd)});  // r4 / r5 = OPD and entry, as in the original
    }
}

// void UiText2_Update(TextElem2* e; f1 = dt)  [vtable 0x00B1C9D0 +152]
// The typewriter: after a (re)layout the quads are cleared and the pen
// starts at the aligned origin; each time the timer drops below zero the
// next character is placed (the timer then gets +224 more seconds). After
// that, the glyphs of +260 are previewed from the pen position, and the
// +264 quads after the cursor keep their cells animating.
DECOMP(0024FBA8, UiText2_Update) {
    const u32 e = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    if (field<u32>(e, kTwPaused) != 0) return;

    ctx.fpr[1] = dt;
    call(ctx, ufn2::UiText_UpdateQuads, {e});
    if ((field<u32>(e, kElemFlags) & kElemActive) == 0) {
        ctx.fpr[1] = dt;
        call(ctx, ufn2::UiElem_UpdateChildren, {e});
        return;
    }

    // The original's frame: two scale results at +112 / +128, the pen at
    // +144 / +148, the preview cursor at +152, widths at +156 and +1180.
    constexpr u32 kFrameBase = 112;
    // The whole original frame (2304 bytes) is reserved: the preview loop has
    // no 255 cap on the line, so it can read widths past +1180's 256 entries.
    Stack frame(ctx, 2304 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 scale_a = sp + 112, scale_b = sp + 128, px = sp + 144, py = sp + 148, pp = sp + 152;
    const u32 widths_a = sp + 156, widths_b = sp + 1180;

    u32 cursor = field<u32>(e, kTwCursor);
    f64 timer = lfs(e + kTwTimer);
    if (field<u32>(e, kTextFlags) & kTextLayoutPending) {
        {
            const u32 opd = vfunc(e, kVfnGetScale);
            call_ptr(ctx, opd, {scale_a, e, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry, as in the original
        }
        timer = kZero;
        store<u32>(widths_a, 0);
        const f64 space = callf(ctx, ufn2::UiText_SpaceWidth, {e, scale_a}, {});
        ctx.fpr[1] = space;
        // r6 is not set here in the original (SpaceWidth's leftover); MeasureLines reads neither r5 nor r6.
        const u32 lines = static_cast<u32>(call(ctx, ufn2::UiText_MeasureLines, {e, widths_a, 256, 0, scale_a}));
        const u32 set = field<u32>(e, kTextFontSet);
        const u32 flags = field<u32>(e, kElemFlags);
        f64 h = timer;  // 0 without a first font
        if (field<u32>(set, kSetFontCount) != 0 && load<u32>(set + kSetFonts) != 0)
            h = fmuls(to_f32(field<u16>(load<u32>(set + kSetFonts), kFontLineHeight).get()), lfs(set + kSetLineScale));
        const u32 quads = field<u32>(e, kTextQuads), bytes = (field<u32>(e, kTextQuadCap) << 5) & ~31u;
        h = fmuls(fmuls(h, lfs(scale_a + 4)), lfs(e + kTextLineSpacing));
        f64 y = fmuls(fmuls(h, kHalfUp), to_f32(lines));
        field<u32>(e, kTwQuad) = 0;
        field<u32>(e, kTwLine) = 0;
        store<u32>(e + kTwX, load<u32>(widths_a));
        y = fmuls(y, to_f32(static_cast<u64>(static_cast<s64>(static_cast<s32>((flags >> 18) & 3)))));
        stfs(e + kTwY, y);
        call(ctx, ufn2::memset, {quads, 0, bytes});
        const f64 glyphs = to_f32(field<u32>(e, kTextGlyphCountCached));
        const f64 reveal = lfs(e + kTextReveal);
        cursor = field<u32>(e, kTextBuf);
        field<u32>(e, kTextFlags) &= ~kTextLayoutPending;
        field<u32>(e, kTwTimer) = 0;
        field<u32>(e, kTwCursor) = cursor;
        stfs(e + kTextRevealElapsed, fmuls(reveal, glyphs));
    }

    bool emit = false;
    if (cursor != 0 && load<u8>(cursor) != 0) {
        timer = fsubs(timer, dt);
        stfs(e + kTwTimer, timer);
        emit = timer <= static_cast<f64>(kZero);
    }

    if (emit) {
        store<u32>(px, load<u32>(e + kTwX));
        store<u32>(py, load<u32>(e + kTwY));
        u32 line = field<u32>(e, kTwLine);  // read before the scale call, as in the original
        {
            const u32 opd = vfunc(e, kVfnGetScale);
            call_ptr(ctx, opd, {scale_b, e, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry, as in the original
        }
        const u32 set = field<u32>(e, kTextFontSet);
        const f64 spacing = lfs(e + kTextLineSpacing);
        f64 h = kZero;
        if (field<u32>(set, kSetFontCount) != 0 && load<u32>(set + kSetFonts) != 0)
            h = fmuls(to_f32(field<u16>(load<u32>(set + kSetFonts), kFontLineHeight).get()), lfs(set + kSetLineScale));
        const f64 line_h = fmuls(h, lfs(scale_b + 4));
        const f64 space = callf(ctx, ufn2::UiText_SpaceWidth, {e, scale_b}, {});
        store<u32>(widths_b, 0);
        ctx.fpr[1] = space;
        call(ctx, ufn2::UiText_MeasureLines, {e, widths_b, 256, 0, scale_b});


        auto space_advance = [&] {
            stfs(px, fmadds(space, fadds(lfs(e + kTextCharSpacing), kOne), lfs(px)));
        };
        while (lfs(e + kTwTimer) < static_cast<f64>(kZero) && load<u8>(field<u32>(e, kTwCursor)) != 0) {
            const u32 c = static_cast<u32>(call(ctx, tfn::Utf8_Next, {e, e + kTwCursor, 0, 1}));
            if (c == '\n' || c == '\r') {
                if (line < 255) ++line;
                const f64 ny = fmadds(line_h, spacing, lfs(py));
                store<u32>(px, load<u32>(widths_b + line * 4));
                stfs(py, ny);
                continue;
            }
            if (c == ' ') {
                space_advance();
                continue;
            }
            const u32 quad = field<u32>(e, kTextQuads) + (field<u32>(e, kTwQuad) << 5);
            ctx.fpr[1] = space;
            // r8 still holds +208 in the original; the glyph builder doesn't read it.
            const u32 ok = static_cast<u32>(call(ctx, ufn2::UiText_BuildGlyph,
                                                 {e, quad, c, px, py, field<u32>(e, kTextQuads), scale_b, line}));
            if (ok == 0) {
                space_advance();
                continue;
            }
            const f64 nt = fadds(lfs(e + kTwTimer), lfs(e + kTextReveal));
            field<u32>(e, kTwQuad) = field<u32>(e, kTwQuad) + 1;
            stfs(e + kTwTimer, nt);
        }
        store<u32>(e + kTwX, load<u32>(px));
        store<u32>(e + kTwY, load<u32>(py));
        field<u32>(e, kTwLine) = line;

        // Preview the +260 text from the pen position, into the quads after the cursor.
        const u32 preview = field<u32>(e, kTwPreview);
        if (preview != 0) {
            store<u32>(pp, preview);
            if (load<u8>(preview) != 0) {
                u32 quad_off = field<u32>(e, kTwQuad) << 5;
                do {
                    const u32 c = static_cast<u32>(call(ctx, tfn::Utf8_Next, {e, pp, 0, 1}));
                    if (c == '\n' || c == '\r') {
                        ++line;  // no 255 cap here, as in the original
                        const f64 ny = fmadds(line_h, spacing, lfs(py));
                        stfs(py, ny);
                        store<u32>(px, load<u32>(widths_b + line * 4));
                    } else if (c == ' ') {
                        space_advance();
                    } else {
                        ctx.fpr[1] = space;
                        // r8 is whatever Utf8_Next left in the original; the glyph builder doesn't read it.
                        const u32 ok = static_cast<u32>(
                            call(ctx, ufn2::UiText_BuildGlyph,
                                 {e, field<u32>(e, kTextQuads) + quad_off, c, px, py, 0, scale_b, line}));
                        if (ok == 0)
                            space_advance();
                        else
                            quad_off += kQuadSize;
                    }
                } while (load<u8>(load<u32>(pp)) != 0);
            }
        }
    }

    if (field<u32>(e, kTextQuads) != 0) {
        const s32 n = static_cast<s32>(field<u32>(e, kTwPreviewQuads).get());
        if (n > 0) {
            const u32 first = field<u32>(e, kTwQuad) << 5;
            for (s32 k = 0; k < static_cast<s32>(field<u32>(e, kTwPreviewQuads).get()); ++k) {
                ctx.fpr[1] = dt;
                ctx.fpr[2] = lfs(e + kText232);
                // r4 is not set in the original; UiQuad_Update overwrites it before reading it.
                call(ctx, ufn2::UiQuad_Update,
                     {field<u32>(e, kTextQuads) + first + static_cast<u32>(k) * kQuadSize, 0,
                      (field<u32>(e, kTextFlags) >> 22) & 0xF});
            }
        }
    }
    ctx.fpr[1] = dt;
    call(ctx, ufn2::UiElem_UpdateChildren, {e});
}
