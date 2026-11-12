// Text and type-5 elements from a UI file.
#include "uiload.h"

#include "../memory/mem.h"

namespace sly4::uiload {
using namespace opentit::native;

namespace {

// The base description (name, matrix rows, scale, colour, the u32 fields)
// that both builders copy from the loader's (raw words).
void CopyBase(ElemDesc* d, const ElemDesc* base) {
    be_u32* dw = reinterpret_cast<be_u32*>(d);
    const be_u32* bw = reinterpret_cast<const be_u32*>(base);
    dw[0] = bw[0].get();
    for (u32 row = 16; row <= 80; row += 16)
        for (u32 k = 0; k < 12; k += 4) dw[(row + k) / 4] = bw[(row + k) / 4].get();
    for (u32 k = 0; k < 4; ++k) d->color[k] = base->color[k];
    for (u32 off = 100; off <= 116; off += 4) dw[off / 4] = bw[off / 4].get();
}

// The text defaults both builders start from.
void TextDefaults(TextDesc* d) {
    d->font_set = nullptr;
    d->text = host<const char>(kEmptyString);
    d->line_spacing = 1.0f;
    reinterpret_cast<be_u32&>(d->char_spacing) = 0u;
    d->wrap = 0u;
    d->f148 = 0u;
    d->monospace = 0;
    d->glyph_mode = 0u;
    d->reveal_rate = 3.0f;
    d->b164 = 0;
}

}  // namespace

text::TextElem* UiText_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const text::TextElem* tmpl) {
    GuestFrame frame(384 - 112);  // the original's frame from the description up
    TextDesc* d = frame.as<TextDesc>();
    CopyBase(&d->base, base);
    for (u32 k = 0; k < 7; ++k) d->base.flags[k] = base->flags[k];
    TextDefaults(d);
    UiTextDesc_FromXml(doc, node, tmpl, d);
    if (!d->font_set) return nullptr;
    text::TextElem* e = static_cast<text::TextElem*>(memory::Mem_Alloc(240, 0, 16));
    if (!e) return nullptr;
    call(efn::UiText_Construct, e, d);
    return e;
}

// The text attributes, then the type-5 ones (defaults from the template
// when it is a type-5 element; +180 always defaults to 0). +168, +172 and
// +176 are raised to at least 0.
void UiText5Desc_FromXml(XmlCursor* doc, XmlCursor* node, const text::UiElem* tmpl, Text5Desc* d) {
    UiTextDesc_FromXml(doc, node, reinterpret_cast<const text::TextElem*>(tmpl), &d->text);
    if (!d->text.font_set) return;

    const be_f32* consts = host<be_f32>(kText5DescConsts);
    const f32 zero = consts[0];
    f32 a = consts[1], w = zero, c = zero;
    u8 flag = 0;
    if (tmpl) {
        const s32 type = vcall<s32>(tmpl, kVfnType);
        if (type == 5) {
            const u32 t = guest(tmpl);
            flag = (gref<u8>(t + 256) & 0x40) != 0 ? 1 : 0;
            a = gref<be_f32>(t + 236);
            w = gref<be_f32>(t + 240);
            c = gref<be_f32>(t + 244);
        } else {
            XmlCursor_DocPath(doc);
        }
    }
    xml::XmlCursor_ReadF32(node, kHashT5_168, &d->f168, a);
    xml::XmlCursor_ReadF32(node, kHashT5Width, &d->width, w);
    xml::XmlCursor_ReadF32(node, kHashT5_176, &d->f176, c);
    xml::XmlCursor_ReadF32(node, kHashT5Start, &d->start, zero);
    xml::XmlCursor_ReadBool(node, kHashT5Flag, &d->flag, flag);
    for (be_f32* f : {&d->f168, &d->width, &d->f176}) {
        const f32 v = *f;
        *f = v < zero ? zero : v;
    }
}

// The description's flag bytes are taken shifted, as in the type-5 clone:
// +120..+124 = base +122..+126, +125 = 1, +126 = 0, +156 = base +120 and
// +160 = (float)base +121.
text::UiElem* UiText5_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const text::UiElem* tmpl) {
    // The original's frame from the description up (its fcfid scratch at +192 of it).
    GuestFrame frame(464 - 112);
    Text5Desc* d = frame.as<Text5Desc>();
    CopyBase(&d->text.base, base);
    for (u32 k = 0; k < 5; ++k) d->text.base.flags[k] = base->flags[2 + k];
    d->text.base.flags[5] = 1;
    d->text.base.flags[6] = 0;
    TextDefaults(&d->text);
    d->text.glyph_mode = base->flags[0];
    *frame.as<be_u64>(304 - 112) = base->flags[1];  // the fcfid scratch
    d->text.reveal_rate = static_cast<f32>(static_cast<f64>(base->flags[1]));
    d->f168 = 1.0f;
    reinterpret_cast<be_u32&>(d->width) = 0u;
    reinterpret_cast<be_u32&>(d->f176) = 0u;
    reinterpret_cast<be_u32&>(d->start) = 0u;
    d->flag = 0;
    UiText5Desc_FromXml(doc, node, tmpl, d);
    if (!d->text.font_set) return nullptr;
    text::UiElem* e = static_cast<text::UiElem*>(memory::Mem_Alloc(272, 0, 16));
    if (!e) return nullptr;
    call(efn::UiText5_Construct, e, d);
    return e;
}

}  // namespace sly4::uiload

NATIVE(004DCE7C, sly4::uiload::UiText_FromXml)
NATIVE(004DCA08, sly4::uiload::UiText5Desc_FromXml)
NATIVE(004DCC04, sly4::uiload::UiText5_FromXml)
