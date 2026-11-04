// The type-5 element class's own slots (vtable 0x00B22B78).
#include "class_text5.h"

using namespace sly4;
using namespace sly4::uielem;
using namespace sly4::text;

namespace {

inline u8 Bit(u32 v, u32 bit) { return (v & bit) != 0 ? 1 : 0; }
inline f64 to_f32(u64 v) { return static_cast<f64>(static_cast<f32>(static_cast<f64>(v))); }

constexpr u32 kText5SizeConsts = 0x004DA4A0;  // f32 { 0.5, 0.0 }, f64 0.0
constexpr u32 kZeroVec = 0x01164AFC;
constexpr u32 kConfig = 0x010C5A20;           // +4 / +6 u16: the screen size used without a parent
constexpr u32 kVfnScale = 168;
constexpr u32 kText5Margin = 244;             // added to the measured width (kText5_244)

}  // namespace

// void UiText5_Destruct(Text5* e)  [+0]
DECOMP(007964A8, UiText5_Destruct) {
    const u32 e = arg<u32>(ctx, 0);
    store<u32>(e, kVtblText5);
    call(ctx, t5fn::UiText_Destruct, {e, kVtblText5});  // r4 = the vtable, as in the original
}

// void UiText5_Delete(Text5* e)  [+4]
DECOMP(007964D4, UiText5_Delete) {
    const u32 e = arg<u32>(ctx, 0);
    if (e == 0) return;
    store<u32>(e, kVtblText5);
    call(ctx, t5fn::UiText_Destruct, {e, kVtblText5});
    call(ctx, clfn::Mem_FreeFrom, {e, 0});
}

// void UiText5_Construct(Text5* e, const Text5Desc* d)
DECOMP(004DC5E0, UiText5_Construct) {
    const u32 e = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    // r5 = 0x004E0000 in the original; UiText_Construct doesn't read it.
    call(ctx, sfn2::UiText_Construct, {e, d, 0x004E0000});

    store<u32>(e + kText5_236, load<u32>(d + kDesc5_168));
    store<u32>(e + kText5Width, load<u32>(d + kDesc5Width));
    store<u32>(e, kVtblText5);
    store<u32>(e + kText5_244, load<u32>(d + kDesc5_176));
    store<u32>(e + kText5Scroll, 0);
    store<u32>(e + kText5Start, load<u32>(d + kDesc5Start));
    u8 f = load<u8>(e + kText5Flags);
    f = static_cast<u8>((f & ~0x80) | ((load<u8>(d + kDesc5Flag) << 7) & 0x80));
    store<u8>(e + kText5Flags, f);
    f = static_cast<u8>((f & ~0x40) | ((load<u8>(d + kDesc5Flag) << 6) & 0x40));
    store<u8>(e + kText5Flags, f);

    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 160 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const f64 width = static_cast<f64>(load<f32>(d + kDesc5Width));
    const u32 parent = load<u32>(e + 148);
    if (width == static_cast<f64>(load<f32>(kText5Consts)) && (load<u32>(e + 136) & 0x20000000) && parent != 0) {
        const u32 opd = vfunc(parent, kVfnGetSize);
        call_ptr(ctx, opd, {sp + 120, parent, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry
        store<u32>(e + kText5Width, load<u32>(sp + 120));
    }
    call(ctx, t5fn::UiText5_GetSize, {sp + 112, e});
    const f64 text_w = static_cast<f64>(load<f32>(sp + 112));
    if (text_w < static_cast<f64>(load<f32>(e + kText5Width))) store<f32>(e + kText5Width, static_cast<f32>(text_w));
}

// void UiText5_Invalidate(Text5* e)  [+148]
// Restarts the reveal and the scroll, recounts the glyphs, and
// invalidates the children.
DECOMP(004E7A58, UiText5_Invalidate) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 tflags = load<u32>(e + kTextFlags);
    store<u32>(e + kTextRevealElapsed, 0);
    store<u32>(e + kText5Scroll, load<u32>(e + kText5Start));
    store<u32>(e + kTextFlags, tflags | kTextLayoutPending);
    // r4 / r5 as in the original; CountGlyphs reads only r3.
    call(ctx, t5fn::UiText_CountGlyphs, {e, tflags | kTextLayoutPending, tflags});
    store<u32>(e + 136, load<u32>(e + 136) | 0x02000000);
    for (u32 c = load<u32>(e + kElemFirstChild); c != 0; c = load<u32>(c + kElemNext)) {
        const u32 opd = vfunc(c, kVfnInvalidate);
        call_ptr(ctx, opd, {c, opd, load<u32>(opd)});
    }
}

// Element* UiText5_Clone(Text5* e, const char* name, Element* parent, u32 arg, bool deep)  [+8]
// Builds a description from the element and constructs a new one from it.
// The description is not the one the constructor reads back field for
// field: desc +120..+126 get flags 0x20000000, 0x10000000, 0x08000000,
// 0x04000000, 0x20, 1, 0; +156 gets the active bit and +160 the focusable
// bit as a float (the reveal rate). Kept as in the original.
DECOMP(004E6D3C, UiText5_Clone) {
    const u32 e = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), parent = arg<u32>(ctx, 2);
    const u64 a6 = ctx.gpr[6], a7 = ctx.gpr[7];

    // The original's frame: the description at +112, the result at +312,
    // spills at +304 (e), +320 (arg), +328 (allocation), +336 (deep).
    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 480 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 d = sp + 112, result = sp + 312;

    const u32 flags = load<u32>(e + 136), tflags = load<u32>(e + kTextFlags);
    store<u32>(d + kDescName, name);
    for (u32 row = 0; row < 5; ++row)
        for (u32 k = 0; k < 3; ++k)
            store<u32>(d + kDescMatrix + row * 16 + k * 4, load<u32>(e + 16 + row * 16 + k * 4));
    for (u32 k = 0; k < 4; ++k) store<u8>(d + kDescColor + k, load<u8>(e + 132 + k));
    store<u32>(d + kDescFieldA, (flags >> 22) & 3);
    store<u32>(d + kDescParent, parent);
    store<u32>(d + kDescArg, static_cast<u32>(a6));
    store<u32>(d + kDescFieldB, (flags >> 14) & 0xFF);
    store<u32>(d + kDescFieldC, (flags >> 6) & 0xFF);
    store<u8>(d + kDescFlags + 0, Bit(flags, 0x20000000));
    store<u8>(d + kDescFlags + 1, Bit(flags, 0x10000000));
    store<u8>(d + kDescFlags + 2, Bit(flags, 0x08000000));
    store<u8>(d + kDescFlags + 3, Bit(flags, 0x04000000));
    store<u8>(d + kDescFlags + 4, Bit(flags, 0x00000020));
    store<u8>(d + kDescFlags + 5, 1);
    store<u8>(d + kDescFlags + 6, 0);
    store<u32>(d + kDescFontSet, load<u32>(e + kTextFontSet));
    store<u32>(d + kDescText, load<u32>(e + kTextBuf));
    store<u32>(d + kDescLineSpacing, load<u32>(e + kTextLineSpacing));
    store<u32>(d + kDescCharSpacing, load<u32>(e + kTextCharSpacing));
    store<u32>(d + kDescWrap, (tflags >> 6) & 0xFFFF);
    store<u32>(d + kDescField26, (tflags >> 26) & 0xF);
    store<u8>(d + kDescMonospace, Bit(tflags, 0x80000000));
    store<u32>(d + kDescGlyphMode, Bit(flags, 0x80000000));
    store<f32>(d + kDescRevealRate, static_cast<f32>(Bit(flags, 0x40000000)));
    store<u8>(d + kDescByte164, 0);
    store<u32>(d + kDesc5_168, load<u32>(e + kText5_236));
    store<u32>(d + kDesc5Width, load<u32>(e + kText5Width));
    store<u32>(d + kDesc5_176, load<u32>(e + kText5_244));
    store<u32>(d + kDesc5Start, load<u32>(e + kText5Start));
    store<u8>(d + kDesc5Flag, Bit(load<u8>(e + kText5Flags), 0x40));
    store<u32>(result, 0);
    store<u64>(sp + 304, e);
    store<u64>(sp + 320, a6);
    store<u64>(sp + 336, a7);
    store<u64>(sp + 344, Bit(flags, 0x40000000));  // the fcfid scratch

    const u32 mem = static_cast<u32>(call(ctx, t5fn::Mem_Alloc, {kText5Size, 0, 16}));
    store<u64>(sp + 328, mem);
    if (mem != 0) {
        call(ctx, t5fn::UiText5_Construct, {mem, d});
        store<u32>(result, mem);
    }
    if ((a7 & 0xFF) != 0) {
        for (u32 c = load<u32>(e + kElemFirstChild); c != 0; c = load<u32>(c + kElemNext)) {
            const u32 opd = vfunc(c, kVfnClone);
            // r8 / r9 = OPD and entry, as in the original.
            call_ptr(ctx, opd, {c, c + 96, load<u32>(result), a6, 1, opd, load<u32>(opd)});
        }
    }
    ret(ctx, load<u32>(result));
}

// Vec2 UiText5_GetSize(Text5* e)  [+140, sret in r3, e in r4]
// When the size is dirty: the width of the text revealed so far on one
// line (newlines are skipped), or, with flag 0x20000000, at least the
// parent's width (the screen's without a parent) and the parent's height;
// plus +244. Also sets +240 to it and marks the layout pending.
DECOMP(004DA4C0, UiText5_GetSize) {
    const u32 out = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    f64 w = static_cast<f64>(load<f32>(kZeroVec));
    f64 h = static_cast<f64>(load<f32>(kZeroVec + 4));
    u32 flags = load<u32>(e + 136);
    if ((flags & 0x02000000) == 0) {
        store<u32>(out, load<u32>(e + 140));
        store<u32>(out + 4, load<u32>(e + 144));
        ctx.fpr[1] = static_cast<f64>(load<f32>(e + 144));
        ctx.fpr[2] = static_cast<f64>(load<f32>(e + 140));
        return;
    }

    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 256 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 scale = sp + 112, pp = sp + 128, psize = sp + 132;

    if (load<u32>(e + kTextFontSet) != 0 && load<u32>(e + kTextBufCap) != 0) {
        const f64 set_scale = static_cast<f64>(load<f32>(load<u32>(e + kTextFontSet) + kSetScale));
        {
            const u32 opd = vfunc(e, kVfnScale);
            call_ptr(ctx, opd, {scale, e, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry
        }
        const u32 set = load<u32>(e + kTextFontSet);
        const u32 count = load<u32>(set + kSetFontCount);
        const f64 half = static_cast<f64>(load<f32>(kText5SizeConsts));
        const f64 zero = static_cast<f64>(load<f32>(kText5SizeConsts + 4));

        f64 space = zero;
        const GlyphRef sg = FontSet_FindGlyph(set, ' ');
        const u32 sfont = sg.index < count ? load<u32>(set + kSetFonts + sg.index * 4) : 0;
        if (sfont != 0) {
            const u32 adv = load<u8>(load<u32>(sfont + kFontGlyphs) + sg.glyph * 8 + 2);
            space = fmuls(fmuls(fmuls(to_f32(adv), lfs(set + kSetFontScale + sg.index * 4)), set_scale), lfs(scale));
        } else if (count != 0 && load<u32>(set + kSetFonts) != 0) {
            space = fmuls(fmuls(to_f32(load<u16>(load<u32>(set + kSetFonts) + kFontLineHeight)), set_scale), half);
        }
        h = zero;
        if (count != 0 && load<u32>(set + kSetFonts) != 0)
            h = fmuls(to_f32(load<u16>(load<u32>(set + kSetFonts) + kFontLineHeight)), lfs(set + kSetLineScale));

        store<u32>(pp, load<u32>(e + kTextBuf));
        const f64 reveal = lfs(e + kTextReveal);
        const f64 scale_y = lfs(scale + 4);
        u32 limit;
        if (reveal > zero) {
            const f64 n = static_cast<f64>(static_cast<f32>(lfs(e + kTextRevealElapsed) / reveal));
            const f64 cap = to_f32(load<u32>(e + kTextQuadCap));
            const f64 diff = static_cast<f64>(static_cast<f32>(cap - n));
            f64 v = rt::fsel(diff, cap, n);  // min(n, cap); a NaN n gives cap
            v = rt::fsel(v, load<f64>(kText5SizeConsts + 8), v);
            limit = static_cast<u32>(rt::fctid(static_cast<f64>(static_cast<f32>(v)), true));
        } else {
            limit = load<u32>(e + kTextQuadCap);
        }

        u32 glyphs = 0;
        f64 max_w = zero, x = zero;
        while (load<u8>(load<u32>(pp)) != 0 && glyphs < limit) {
            const u32 c = static_cast<u32>(call(ctx, tfn::Utf8_Next, {e, pp, 0, 1}));
            if (c == '\n' || c == '\r') continue;
            bool found = false;
            if ((load<u32>(e + kTextFlags) & kTextMonospace) == 0) {
                const u32 s = load<u32>(e + kTextFontSet);
                const GlyphRef g = FontSet_FindGlyph(s, c);
                if (g.index < load<u32>(s + kSetFontCount)) {
                    const u32 font = load<u32>(s + kSetFonts + g.index * 4);
                    if (font != 0 && c != ' ') {
                        found = true;
                        const u32 adv = load<u8>(load<u32>(font + kFontGlyphs) + g.glyph * 8 + 2);
                        ++glyphs;
                        const f64 a = fmuls(fmuls(to_f32(adv), lfs(s + kSetFontScale + g.index * 4)), set_scale);
                        const f64 t = fmadds(a, lfs(scale), x);
                        x = fadds(t, fmuls(space, lfs(e + kTextCharSpacing)));
                    }
                }
            }
            if (!found) x = fadds(space, x);
            if (max_w < x) max_w = x;
        }

        flags = load<u32>(e + 136);
        w = max_w;
        h = fmuls(h, scale_y);
        if (flags & 0x20000000) {
            const u32 parent = load<u32>(e + 148);
            f64 pw;
            if (parent != 0) {
                const u32 opd = vfunc(parent, kVfnGetSize);
                call_ptr(ctx, opd, {psize, parent, opd, load<u32>(opd)});
                pw = lfs(psize);
                h = lfs(psize + 4);
                flags = load<u32>(e + 136);
            } else {
                store<u64>(sp + 144, load<u16>(kConfig + 4));
                pw = to_f32(load<u16>(kConfig + 4));
                store<u64>(sp + 144, load<u16>(kConfig + 6));
                h = to_f32(load<u16>(kConfig + 6));
            }
            if (pw > w) w = pw;
        }
    }

    const f64 total = fadds(w, lfs(e + kText5Margin));
    stfs(e + 144, h);
    store<u32>(e + 136, flags & ~0x02000000u);
    store<u32>(e + kTextFlags, load<u32>(e + kTextFlags) | kTextLayoutPending);
    stfs(e + 140, total);
    store<u32>(e + kText5Width, load<u32>(e + 140));
    stfs(out, total);
    store<u32>(out + 4, load<u32>(e + 144));
    ctx.fpr[1] = static_cast<f64>(load<f32>(e + 144));  // f1 / f2 as the original leaves them
    ctx.fpr[2] = total;
}
