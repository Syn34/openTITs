// Small loader helpers and the text description reader.
#include "uiload.h"

#include "../string/str.h"

namespace sly4::uiload {
using namespace opentit::native;

namespace {
constexpr u32 kTextPeriod = 232;  // TextElem +232 f32: 1 / reveal rate
}  // namespace

// For a document cursor (type 0), its document's path (AsDocument() +48);
// 0 for an element cursor. The loaders call it where they report errors.
const char* XmlCursor_DocPath(XmlCursor* c) {
    if (c->type != 0u) return nullptr;
    xml::XmlDoc* d = vcall<xml::XmlDoc*>(c, xml::kDocVfnAsDocument);
    return host<const char>(guest(d) + xml::kDocPath);
}

// (name?) The default texture n: kTexDefaults[n] (the index is (s32)(n << 2)
// bytes, as the original's extsw of the shift).
u32 Tex_GetDefault(s32 n) {
    const s32 off = static_cast<s32>(static_cast<u32>(n) << 2);
    return gref<be_u32>(kTexDefaults + static_cast<u32>(off));
}

// Reads font=, string=, the spacings, wrap, monospace, reveal rate and
// glyph mode into the description, with the template element's values
// (if any) as defaults. Stops (leaving the rest as it was) when there is
// no font, and before the glyph mode when that attribute isn't there.
void UiTextDesc_FromXml(XmlCursor* doc, XmlCursor* node, const text::TextElem* tmpl, TextDesc* d) {
    GuestFrame frame(16);
    be_u32* mode_hash = frame.as<be_u32>();

    const be_f32* defaults = host<be_f32>(kTextDescDefaults);
    const f32 one = defaults[0];
    u32 font = 0, wrap = 0;
    u8 mono = 0;
    const char* text = host<const char>(kEmptyString);
    f32 line = one, chr = defaults[1], rate = defaults[2];
    if (tmpl) {
        font = tmpl->font_set.ea;
        if (tmpl->buf_cap != 0u) text = tmpl->buf;
        const u32 tf = tmpl->text_flags;
        line = tmpl->line_spacing;
        chr = tmpl->char_spacing;
        rate = text::f32_div(one, gref<be_f32>(guest(tmpl) + kTextPeriod));
        wrap = (tf >> 6) & 0xFFFF;
        mono = (tf & 0x80000000) != 0 ? 1 : 0;
    }

    if (const char* font_attr = xml::XmlCursor_FindAttrByName(node, host<const char>(kStrFont))) {
        const u32 hash = str::String_HashNoCase(font_attr);
        UiRes* set = nullptr;
        for (UiRes* r = host<UiRes>(global<be_u32>(kResList)); r; r = r->next)
            if (r->hash == hash) {
                set = r;
                break;
            }
        d->font_set.ea = guest(set);
        if (!set) XmlCursor_DocPath(doc);
    }
    if (!d->font_set) {
        d->font_set.ea = font;
        if (font == 0) XmlCursor_DocPath(doc);
    }
    if (!d->font_set) return;

    const char* str = xml::XmlCursor_FindAttrByName(node, host<const char>(kStrString));
    d->text = str ? str : text;
    xml::XmlCursor_ReadF32(node, kHashLineSpacing, &d->line_spacing, line);
    xml::XmlCursor_ReadF32(node, kHashCharSpacing, &d->char_spacing, chr);
    xml::XmlCursor_ReadS32(node, kHashWrap, &d->wrap, wrap);
    xml::XmlCursor_ReadBool(node, kHashMonospace, &d->monospace, mono);
    xml::XmlCursor_ReadF32(node, kHashRevealRate, &d->reveal_rate, rate);
    if (xml::XmlCursor_ReadHash(node, kHashGlyphMode, mode_hash, 0) != 0) return;
    const be_u32* modes = host<be_u32>(kGlyphModeHashes);
    u32 k = 0;
    while (k < kGlyphModeCount && modes[k] != *mode_hash) ++k;
    if (k < kGlyphModeCount)
        d->glyph_mode = k;
    else
        XmlCursor_DocPath(doc);
}

}  // namespace sly4::uiload

NATIVE(004F8944, sly4::uiload::XmlCursor_DocPath)
NATIVE(004FEF30, sly4::uiload::Tex_GetDefault)
NATIVE(004DC720, sly4::uiload::UiTextDesc_FromXml)
