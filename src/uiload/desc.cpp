// Small loader helpers and the text description reader.
#include "uiload.h"

using namespace sly4;
using namespace sly4::uiload;

namespace {

constexpr u32 kCurType = 4;  // XmlCursor: 0 document, 1 element
constexpr u32 kDocPath = 48;

// Element / text fields read from the template element.
constexpr u32 kTextFontSet = 168, kTextLineSpacing = 172, kTextCharSpacing = 176, kTextFlags = 180;
constexpr u32 kTextBufCap = 184, kTextBuf = 192, kTextPeriod = 232;
// Description fields written (see uielem/slots.h kDesc*).
constexpr u32 kDescFontSet = 128, kDescText = 132, kDescLineSpacing = 136, kDescCharSpacing = 140;
constexpr u32 kDescWrap = 144, kDescMonospace = 152, kDescGlyphMode = 156, kDescRevealRate = 160;

}  // namespace

// char* XmlCursor_DocPath(XmlCursor* c)
// For a document cursor (type 0), its document's path (AsDocument() +48);
// 0 for an element cursor. The loaders call it where they report errors.
DECOMP(004F8944, XmlCursor_DocPath) {
    const u32 c = arg<u32>(ctx, 0);
    if (load<u32>(c + kCurType) != 0) return ret(ctx, 0u);
    const u32 opd = load<u32>(load<u32>(c) + 8);
    // r4 / r5 = OPD and entry, as in the original.
    const u32 d = static_cast<u32>(call_ptr(ctx, opd, {c, opd, load<u32>(opd)}));
    ret(ctx, d + kDocPath);
}

// Texture* Tex_GetDefault(s32 n)  (name?)
DECOMP(004FEF30, Tex_GetDefault) {
    const s32 n = static_cast<s32>(static_cast<u32>(ctx.gpr[3]) << 2);  // (s32)(n << 2), as extsw of the shift
    ctx.gpr[4] = kTexDefaults;
    ret(ctx, load<u32>(kTexDefaults + static_cast<u32>(n)));
}

// void UiTextDesc_FromXml(XmlCursor* doc, XmlCursor* node, const TextElem* tmpl, TextDesc* d)
// Reads font=, string=, the spacings, wrap, monospace, reveal rate and
// glyph mode into the description, with the template element's values
// (if any) as defaults. Stops (leaving the rest as it was) when there is
// no font, and before the glyph mode when that attribute isn't there.
DECOMP(004DC720, UiTextDesc_FromXml) {
    const u32 doc = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1), tmpl = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);

    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 208 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 mode_hash = sp + 112;

    const f64 one = static_cast<f64>(load<f32>(kTextDescDefaults));
    u32 font = 0, text = kEmptyString, wrap = 0, mono = 0;
    f64 line = one, chr = static_cast<f64>(load<f32>(kTextDescDefaults + 4));
    f64 rate = static_cast<f64>(load<f32>(kTextDescDefaults + 8));
    if (tmpl != 0) {
        font = load<u32>(tmpl + kTextFontSet);
        if (load<u32>(tmpl + kTextBufCap) != 0) text = load<u32>(tmpl + kTextBuf);
        const u32 tf = load<u32>(tmpl + kTextFlags);
        line = static_cast<f64>(load<f32>(tmpl + kTextLineSpacing));
        chr = static_cast<f64>(load<f32>(tmpl + kTextCharSpacing));
        rate = static_cast<f64>(static_cast<f32>(one / static_cast<f64>(load<f32>(tmpl + kTextPeriod))));
        wrap = (tf >> 6) & 0xFFFF;
        mono = (tf & 0x80000000) != 0 ? 1 : 0;
    }

    const u32 font_attr = static_cast<u32>(call(ctx, ufn::XmlCursor_FindAttrByName, {node, kStrFont}));
    if (font_attr != 0) {
        const u32 hash = static_cast<u32>(call(ctx, ufn::String_HashNoCase, {font_attr}));
        u32 set = 0;
        for (u32 r = load<u32>(kResList); r != 0; r = load<u32>(r + 88))
            if (load<u32>(r + 76) == hash) {
                set = r;
                break;
            }
        store<u32>(d + kDescFontSet, set);
        if (set == 0) call(ctx, ufn::XmlCursor_DocPath, {doc});
    }
    if (load<u32>(d + kDescFontSet) == 0) {
        store<u32>(d + kDescFontSet, font);
        if (font == 0) call(ctx, ufn::XmlCursor_DocPath, {doc});
    }
    if (load<u32>(d + kDescFontSet) == 0) return;

    const u32 str = static_cast<u32>(call(ctx, ufn::XmlCursor_FindAttrByName, {node, kStrString}));
    store<u32>(d + kDescText, str != 0 ? str : text);
    ctx.fpr[1] = line;
    call(ctx, ufn::XmlCursor_ReadF32, {node, kHashLineSpacing, d + kDescLineSpacing});
    ctx.fpr[1] = chr;
    call(ctx, ufn::XmlCursor_ReadF32, {node, kHashCharSpacing, d + kDescCharSpacing});
    call(ctx, ufn::XmlCursor_ReadS32, {node, kHashWrap, d + kDescWrap, wrap});
    call(ctx, ufn::XmlCursor_ReadBool, {node, kHashMonospace, d + kDescMonospace, mono});
    ctx.fpr[1] = rate;
    call(ctx, ufn::XmlCursor_ReadF32, {node, kHashRevealRate, d + kDescRevealRate});
    if (static_cast<u32>(call(ctx, ufn::XmlCursor_ReadHash, {node, kHashGlyphMode, mode_hash, 0})) != 0) return;
    u32 k = 0;
    while (k < kGlyphModeCount && load<u32>(kGlyphModeHashes + k * 4) != load<u32>(mode_hash)) ++k;
    if (k < kGlyphModeCount)
        store<u32>(d + kDescGlyphMode, k);
    else
        call(ctx, ufn::XmlCursor_DocPath, {doc});
}
