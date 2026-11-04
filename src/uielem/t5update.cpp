// UiText5_Update (see t5update.h).
#include "t5update.h"

using namespace sly4;
using namespace sly4::text;
using namespace sly4::t5upd;

namespace {

inline f64 fsubs(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a - b)); }
inline f64 fdivs(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a / b)); }
inline f64 fmsubs(f64 a, f64 c, f64 b) { return static_cast<f64>(static_cast<f32>(std::fma(a, c, -b))); }
inline f64 fnmsubs(f64 a, f64 c, f64 b) { return static_cast<f64>(static_cast<f32>(-std::fma(a, c, -b))); }
inline f64 to_f32(s64 v) { return static_cast<f64>(static_cast<f32>(static_cast<f64>(v))); }  // fcfid + frsp

inline u64 Put(u64 b, u32 shift, u32 width, u64 v) {
    const u64 m = ((u64{1} << width) - 1) << shift;
    return (b & ~m) | ((v << shift) & m);
}

// The original's inline truncation of a float toward zero, on its bits
// (|x| < 1 gives +0.0; infinities and NaNs are kept).
f32 TruncBits(f32 x) {
    u32 bits;
    std::memcpy(&bits, &x, 4);
    const s32 r4 = 127 - static_cast<s32>((bits >> 23) & 0xFF);
    const u32 whole = (r4 - 1) < 0 ? 0xFFFFFFFFu : 0;  // |x| >= 1
    const u32 n = static_cast<u32>(-r4) & 63;           // sraw shift amount
    const u32 frac = n > 31 ? 0 : (0x007FFFFFu >> n);
    const u32 mask = (r4 > -31 ? frac : 0) | ~whole;
    bits &= ~mask;
    std::memcpy(&x, &bits, 4);
    return x;
}

// A virtual call with a result buffer: r5 / r6 = OPD and entry, as in the original.
inline void VcallSret(PPUContext& ctx, u32 e, u32 slot, u32 out) {
    const u32 opd = vfunc(e, slot);
    call_ptr(ctx, opd, {out, e, opd, load<u32>(opd)});
}

void UpdateChildren(PPUContext& ctx, u32 e, f64 dt) {
    if ((field<u32>(e, kElemFlags) & 0x80000000) == 0) return;
    for (u32 c = field<u32>(e, kElemFirstChild); c != 0; c = field<u32>(c, kElemNext)) {
        ctx.fpr[1] = dt;
        const u32 opd = vfunc(c, kVfnUpdate);
        call_ptr(ctx, opd, {c, opd, load<u32>(opd)});  // r4 / r5 = OPD and entry
    }
}

}  // namespace

// void UiText5_Update(Text5* e; f1 = dt)  [vtable 0x00B22B78 +152]
// Layout (only while kTextLayoutPending is set): the text on one line,
// starting at x = -h_align * width / 2 - scroll, glyphs left of 0 being
// drawn again one text width (the measured size) further right, and none
// past +240. With a reveal period (+224 > 0) at most elapsed / period
// glyphs are placed. Then the scroll advances and, while scrolling, the
// layout stays pending. The children are updated last.
DECOMP(004E70D0, UiText5_Update) {
    const u32 e = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 320 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 scale = sp + 112, cursor = sp + 128, size = sp + 136, color = sp + 144;

    const u32 set0 = field<u32>(e, kTextFontSet);
    if ((field<u32>(e, kElemFlags) & 0x80000000) == 0 || set0 == 0 || field<u32>(e, kTextBufCap) == 0 ||
        (field<u32>(e, kTextFlags) & kTextLayoutPending) == 0) {
        UpdateChildren(ctx, e, dt);
        return;
    }

    const f64 half = lfs(kConsts), zero = lfs(kConsts + 4), minus_half = lfs(kConsts + 8);
    const f64 three = lfs(kConsts + 12), quarter = lfs(kConsts + 16);
    const f64 set_scale = lfs(set0 + kSetScale), line_scale = lfs(set0 + kSetLineScale);  // read before the calls
    VcallSret(ctx, e, kVfnGetScale, scale);
    VcallSret(ctx, e, kVfnColor, color);  // the colour isn't used
    VcallSret(ctx, e, kVfnGetSize, size);

    // The space width: the space glyph's advance, else half the first
    // font's line height, else 0.
    u32 set = field<u32>(e, kTextFontSet);
    f64 space;
    {
        const u32 count = field<u32>(set, kSetFontCount);
        const GlyphRef g = count != 0 ? FontSet_FindGlyph(set, ' ') : GlyphRef{0xFFFFFFFFu, 0xFFFFFFFFu};
        if (g.index < count && load<u32>(set + kSetFonts + g.index * 4) != 0) {
            const u32 font = load<u32>(set + kSetFonts + g.index * 4);
            const f64 adv = to_f32(load<u8>(field<u32>(font, kFontGlyphs) + g.glyph * kGlyphSize + kGlyphAdvance));
            space = fmuls(fmuls(fmuls(adv, lfs(set + kSetFontScale + g.index * 4)), set_scale), lfs(scale));
        } else if (count != 0 && load<u32>(set + kSetFonts) != 0) {
            space = fmuls(fmuls(to_f32(field<u16>(load<u32>(set + kSetFonts), kFontLineHeight).get()), set_scale), half);
        } else {
            space = zero;
        }
    }

    const f64 halign = to_f32(static_cast<s32>((field<u32>(e, kElemFlags) >> 14) & 3));
    const f64 char_spacing = lfs(e + kTextCharSpacing);
    f64 x = fmuls(lfs(size), minus_half);
    {
        const u32 opd = vfunc(e, kVfnFieldB);
        call_ptr(ctx, opd, {e, opd, load<u32>(opd)});  // r4 / r5 = OPD and entry; the result isn't used
    }
    const f64 width = lfs(e + kText5Width);
    x = fmsubs(x, halign, lfs(e + kText5Scroll));
    f64 x2 = fadds(x, lfs(size));  // the second copy

    set = field<u32>(e, kTextFontSet);
    f64 line_h = zero;
    if (field<u32>(set, kSetFontCount) != 0 && load<u32>(set + kSetFonts) != 0)
        line_h = fmuls(to_f32(field<u16>(load<u32>(set + kSetFonts), kFontLineHeight).get()), lfs(set + kSetLineScale));
    const f64 valign = to_f32(static_cast<s32>((field<u32>(e, kElemFlags) >> 18) & 3));
    line_h = fmuls(line_h, lfs(scale + 4));
    const f64 reveal = lfs(e + kTextReveal);
    f64 yoff = fmuls(line_h, lfs(e + kTextLineSpacing));
    line_h = fmuls(line_h, three);
    yoff = fmuls(fmuls(yoff, minus_half), valign);

    u32 limit;
    if (reveal > zero) {
        const f64 n = fdivs(lfs(e + kTextRevealElapsed), reveal);
        const f64 cap = to_f32(static_cast<s64>(field<u32>(e, kTextQuadCap).get()));
        f64 v = fsubs(cap, n) >= 0.0 ? n : cap;
        v = v >= 0.0 ? v : load<f64>(kConsts + 24);  // 4294967295.0 (which becomes 2^32 as a single: 0)
        limit = static_cast<u32>(rt::fctid(static_cast<f64>(static_cast<f32>(v)), true));
    } else {
        limit = field<u32>(e, kTextQuadCap);
    }

    u32 n = 0, quad_off = 0;
    store<u32>(cursor, field<u32>(e, kTextBuf));
    while (load<u8>(load<u32>(cursor)) != 0 && n < limit) {
        const u32 c = static_cast<u32>(call(ctx, ufn::Utf8_Next, {e, cursor, 0, 1}));
        if (c == '\n' || c == '\r') continue;

        u32 font = 0;
        GlyphRef g{0xFFFFFFFFu, 0xFFFFFFFFu};
        if (c != ' ') {
            set = field<u32>(e, kTextFontSet);
            const u32 count = field<u32>(set, kSetFontCount);
            if (count != 0) g = FontSet_FindGlyph(set, c);
            if (g.index < count) font = load<u32>(set + kSetFonts + g.index * 4);
        }
        if (font == 0) {  // a space, or no glyph: advance only
            const f64 adv = fmadds(space, char_spacing, space);
            x = fadds(adv, x);
            if (x < zero) x2 = fadds(adv, x2);
            continue;
        }

        const u32 glyph = field<u32>(font, kFontGlyphs) + g.glyph * kGlyphSize;
        const u32 adv_texels = load<u8>(glyph + kGlyphAdvance);
        const f64 adv_f = to_f32(adv_texels);
        const bool mono = (field<u32>(e, kTextFlags) & kTextMonospace) != 0;
        f64 gx = x;
        if (mono) gx = fmadds(fsubs(space, adv_f), half, x);  // centred in the space width
        const f64 font_scale = lfs(set + kSetFontScale + g.index * 4);
        const f64 w = fmuls(fmuls(fmuls(adv_f, font_scale), set_scale), lfs(scale));
        const f64 step = fmadds(space, char_spacing, mono ? space : w);
        x = fadds(step, x);
        if (x < zero) {
            gx = x2;
            x2 = fadds(step, gx);
            if (x2 < zero || gx > width) break;
        } else if (gx > width) {
            continue;
        }

        // The quad.
        const u32 q = field<u32>(e, kTextQuads) + quad_off;
        const u32 page = load<u32>(font + kFontPages + ((field<u32>(e, kElemFlags) >> 22) & 3) * 4);
        u64 bits = load<u64>(q + 16);
        store<u32>(q + 24, page);
        bits = Put(bits, 19, 1, (load<u8>(font + kFontFlags) >> 7) & 1);
        store<u64>(q + 16, bits);
        const f64 base_y = fmadds(line_h, quarter, yoff);
        const u32 above = load<u8>(glyph + 0);
        const f64 fs = lfs(load<u32>(e + kTextFontSet) + kSetFontScale + g.index * 4);
        const f64 sy = lfs(scale + 4);
        stfs(q + 0, gx);
        quad_off += kQuadSize;
        ++n;
        stfs(q + 4, fnmsubs(fmuls(fmuls(to_f32(above), fs), line_scale), sy, base_y));
        const u32 rows = static_cast<u32>(static_cast<s32>(above + load<u8>(glyph + 1) + 1));
        bits = Put(load<u64>(q + 16), 53, 11, load<u16>(glyph + 4));
        stfs(q + 8, w);
        store<u64>(q + 16, bits);
        stfs(q + 12, fmuls(fmuls(fmuls(to_f32(static_cast<s32>(rows)), fs), line_scale), sy));
        // The width and height fields go through the float values in the
        // original (exact for these small integers).
        bits = Put(bits, 42, 11, load<u16>(glyph + 6));
        bits = Put(bits, 31, 11, adv_texels);
        bits = Put(bits, 20, 11, rows);
        store<u64>(q + 16, bits);
    }

    // Unused quads lose their page.
    const u32 cap = field<u32>(e, kTextQuadCap);
    if (n < cap) {
        u32 off = n << 5;
        for (s32 left = static_cast<s32>(cap - n); left > 0; --left, off += kQuadSize)
            store<u32>(field<u32>(e, kTextQuads) + off + 24, 0);
    }
    const u32 text_flags = field<u32>(e, kTextFlags);
    field<u32>(e, kTextFlags) = text_flags & ~kTextLayoutPending;
    store<u32>(e + kTextLineCount, 0x3F800000);  // 1.0
    stfs(e + kTextGlyphCount, to_f32(static_cast<s64>(n)));
    if (lfs(e + kTextReveal) > zero) {
        const f64 t = fadds(lfs(e + kTextRevealElapsed), dt);
        field<u32>(e, kElemFlags) |= kElemSizeDirty;
        stfs(e + kTextRevealElapsed, t);
    }

    if ((load<u8>(e + kText5Flags) & 0x80) != 0) {
        const f64 speed = lfs(e + kText5Speed);
        if (speed > zero) {
            f64 s = fmadds(dt, speed, lfs(e + kText5Scroll));
            stfs(e + kText5Scroll, s);
            const f64 total = lfs(e + kElemSize);
            if (s > total || s == total) {
                const f64 a = std::fabs(total);
                s = fdivs(s, a);
                s = fmuls(a, fsubs(s, static_cast<f64>(TruncBits(static_cast<f32>(s)))));
                stfs(e + kText5Scroll, s);
            }
            field<u32>(e, kTextFlags) = text_flags | kTextLayoutPending;  // the flags as read above
        }
    }
    UpdateChildren(ctx, e, dt);
}
