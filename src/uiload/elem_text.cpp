// Text and type-5 elements from a UI file.
#include "uiload.h"

using namespace sly4;
using namespace sly4::uiload;

namespace {

// The base description (name, matrix rows, colour, the u32 fields) that
// both builders copy from the loader's.
void CopyBase(u32 d, u32 base) {
    store<u32>(d, load<u32>(base));
    for (u32 row = 16; row <= 80; row += 16)
        for (u32 k = 0; k < 12; k += 4) store<u32>(d + row + k, load<u32>(base + row + k));
    for (u32 k = 0; k < 4; ++k) store<u8>(d + 96 + k, load<u8>(base + 96 + k));
    for (u32 off = 100; off <= 116; off += 4) store<u32>(d + off, load<u32>(base + off));
}

// The text defaults both builders start from.
void TextDefaults(u32 d) {
    store<u32>(d + 128, 0);
    store<u32>(d + 132, kEmptyString);
    store<u32>(d + 136, 0x3F800000);  // line spacing 1.0
    store<u32>(d + 140, 0);
    store<u32>(d + 144, 0);
    store<u32>(d + 148, 0);
    store<u8>(d + 152, 0);
    store<u32>(d + 156, 0);
    store<u32>(d + 160, 0x40400000);  // reveal rate 3.0
    store<u8>(d + 164, 0);
}

}  // namespace

// TextElem* UiText_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const TextElem* tmpl)
DECOMP(004DCE7C, UiText_FromXml) {
    const u32 doc = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1), base = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 384 - kFrameBase);
    const u32 d = frame.addr() - kFrameBase + 112;

    CopyBase(d, base);
    for (u32 k = 0; k < 7; ++k) store<u8>(d + 120 + k, load<u8>(base + 120 + k));
    TextDefaults(d);
    call(ctx, efn::UiTextDesc_FromXml, {doc, node, tmpl, d});
    if (load<u32>(d + 128) == 0) return ret(ctx, 0u);
    const u32 e = static_cast<u32>(call(ctx, efn::Mem_Alloc, {240, 0, 16}));
    if (e == 0) return ret(ctx, 0u);
    call(ctx, efn::UiText_Construct, {e, d});
    ret(ctx, e);
}

// void UiText5Desc_FromXml(XmlCursor* doc, XmlCursor* node, const Element* tmpl, Text5Desc* d)
// The text attributes, then the type-5 ones (defaults from the template
// when it is a type-5 element; +180 always defaults to 0). +168, +172 and
// +176 are raised to at least 0.
DECOMP(004DCA08, UiText5Desc_FromXml) {
    const u32 doc = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1), tmpl = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    call(ctx, efn::UiTextDesc_FromXml, {doc, node, tmpl, d});
    if (load<u32>(d + 128) == 0) return;

    const f64 zero = static_cast<f64>(load<f32>(kText5DescConsts));
    f64 a = static_cast<f64>(load<f32>(kText5DescConsts + 4)), w = zero, c = zero;
    u32 flag = 0;
    if (tmpl != 0) {
        const u32 opd = load<u32>(load<u32>(tmpl) + kVfnType);
        const s32 type = static_cast<s32>(call_ptr(ctx, opd, {tmpl, opd, load<u32>(opd)}));  // r4 / r5 = OPD and entry
        if (type == 5) {
            flag = (load<u8>(tmpl + 256) & 0x40) != 0 ? 1 : 0;
            a = static_cast<f64>(load<f32>(tmpl + 236));
            w = static_cast<f64>(load<f32>(tmpl + 240));
            c = static_cast<f64>(load<f32>(tmpl + 244));
        } else {
            call(ctx, ufn::XmlCursor_DocPath, {doc});
        }
    }
    ctx.fpr[1] = a;
    call(ctx, ufn::XmlCursor_ReadF32, {node, kHashT5_168, d + 168});
    ctx.fpr[1] = w;
    call(ctx, ufn::XmlCursor_ReadF32, {node, kHashT5Width, d + 172});
    ctx.fpr[1] = c;
    call(ctx, ufn::XmlCursor_ReadF32, {node, kHashT5_176, d + 176});
    ctx.fpr[1] = zero;
    call(ctx, ufn::XmlCursor_ReadF32, {node, kHashT5Start, d + 180});
    call(ctx, ufn::XmlCursor_ReadBool, {node, kHashT5Flag, d + 184, flag});
    for (u32 off : {168u, 172u, 176u}) {
        const f64 v = static_cast<f64>(load<f32>(d + off));
        store<f32>(d + off, static_cast<f32>(v < zero ? zero : v));
    }
    ctx.fpr[1] = static_cast<f64>(load<f32>(d + 176));  // f1 as the original leaves it
}

// Text5* UiText5_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const Element* tmpl)
// The description's flag bytes are taken shifted, as in the type-5 clone:
// +120..+124 = base +122..+126, +125 = 1, +126 = 0, +156 = base +120 and
// +160 = (float)base +121.
DECOMP(004DCC04, UiText5_FromXml) {
    const u32 doc = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1), base = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 464 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 d = sp + 112;

    CopyBase(d, base);
    for (u32 k = 0; k < 5; ++k) store<u8>(d + 120 + k, load<u8>(base + 122 + k));
    store<u8>(d + 125, 1);
    store<u8>(d + 126, 0);
    TextDefaults(d);
    store<u32>(d + 156, load<u8>(base + 120));
    store<u64>(sp + 304, load<u8>(base + 121));  // the fcfid scratch
    store<f32>(d + 160, static_cast<f32>(static_cast<f64>(load<u8>(base + 121))));
    store<u32>(d + 168, 0x3F800000);
    store<u32>(d + 172, 0);
    store<u32>(d + 176, 0);
    store<u32>(d + 180, 0);
    store<u8>(d + 184, 0);
    call(ctx, efn::UiText5Desc_FromXml, {doc, node, tmpl, d});
    if (load<u32>(d + 128) == 0) return ret(ctx, 0u);
    const u32 e = static_cast<u32>(call(ctx, efn::Mem_Alloc, {272, 0, 16}));
    if (e == 0) return ret(ctx, 0u);
    call(ctx, efn::UiText5_Construct, {e, d});
    ret(ctx, e);
}
