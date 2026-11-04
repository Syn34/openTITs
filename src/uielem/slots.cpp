// The app-level text element's slots (vtable 0x00B1C9D0) and the text
// clone (text vfunc +8).
#include "slots.h"

using namespace sly4;
using namespace sly4::uielem;
using namespace sly4::text;

namespace {

constexpr f32 kOne = 1.0f;              // 0x004E183C
constexpr u32 kRevealRate3 = 0x40400000;  // 3.0f
constexpr u32 kFormatBufSize = 4096;

inline u8 Bit(u32 flags, u32 bit) { return (flags & bit) != 0 ? 1 : 0; }

// The fields both clones copy from the element into the description.
void Desc_Fill(u32 d, u32 e, u32 name, u32 parent, u32 arg) {
    const u32 flags = field<u32>(e, kElemFlags), tflags = field<u32>(e, kTextFlags);
    store<u32>(d + kDescName, name);
    for (u32 row = 0; row < 5; ++row)
        for (u32 k = 0; k < 3; ++k)
            store<u32>(d + kDescMatrix + row * 16 + k * 4, load<u32>(e + 16 + row * 16 + k * 4));
    for (u32 k = 0; k < 4; ++k) store<u8>(d + kDescColor + k, load<u8>(e + 132 + k));
    store<u32>(d + kDescFieldA, (flags >> 22) & 3);
    store<u32>(d + kDescParent, parent);
    store<u32>(d + kDescArg, arg);
    store<u32>(d + kDescFieldB, (flags >> 14) & 0xFF);
    store<u32>(d + kDescFieldC, (flags >> 6) & 0xFF);
    static constexpr u32 kBits[7] = {0x80000000, 0x40000000, 0x20000000, 0x10000000,
                                     0x08000000, 0x04000000, 0x00000020};
    for (u32 k = 0; k < 7; ++k) store<u8>(d + kDescFlags + k, Bit(flags, kBits[k]));
    store<u32>(d + kDescFontSet, field<u32>(e, kTextFontSet));
    store<u32>(d + kDescText, field<u32>(e, kTextBuf));
    store<u32>(d + kDescLineSpacing, load<u32>(e + kTextLineSpacing));
    store<u32>(d + kDescCharSpacing, load<u32>(e + kTextCharSpacing));
    store<u32>(d + kDescWrap, (tflags >> 6) & 0xFFFF);
    store<u32>(d + kDescField26, (tflags >> 26) & 0xF);
    store<u8>(d + kDescMonospace, Bit(tflags, 0x80000000));
}

void Text2_Release(PPUContext& ctx, u32 e) {
    field<u32>(e, 0) = kText2Vtbl;
    if (field<u32>(e, kText2Source) != 0) {
        call(ctx, tsfn::SmallFree, {field<u32>(e, kText2Source), field<u32>(e, kText2SourceCap)});
        field<u32>(e, kText2Source) = 0;
    }
    call(ctx, tsfn::UiText_DestructThunk, {e});
}

}  // namespace

// void UiText2_Destruct(TextElem2* e)  [+0]
DECOMP(0024F2C8, UiText2_Destruct) { Text2_Release(ctx, arg<u32>(ctx, 0)); }

// void UiText2_Delete(TextElem2* e)  [+4]
DECOMP(0024F328, UiText2_Delete) {
    const u32 e = arg<u32>(ctx, 0);
    if (e == 0) return;
    Text2_Release(ctx, e);
    call(ctx, tsfn::Mem_FreeFrom, {e, 0});
}

// Element* UiText_Clone(TextElem* e, const char* name, Element* parent, u32 arg, bool deep)  [text +8]
// A new text element (240 bytes) built from a description of this one,
// and, when deep, clones of the children under it.
DECOMP(004E1840, UiText_Clone) {
    const u32 e = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), parent = arg<u32>(ctx, 2);
    const u64 a6 = ctx.gpr[6], a7 = ctx.gpr[7];

    // The original's frame: the description at +112 (with the spilled
    // arguments at +288 / +304 / +320 and the result at +296 inside it).
    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 328 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 d = sp + 112, result = sp + 296;

    Desc_Fill(d, e, name, parent, static_cast<u32>(a6));
    store<u32>(d + kDescGlyphMode, (field<u32>(e, kTextFlags) >> 22) & 0xF);
    store<f32>(d + kDescRevealRate, static_cast<f32>(kOne / static_cast<f64>(load<f32>(e + kText232))));
    store<u8>(d + kDescByte164, 0);
    store<u32>(result, 0);
    store<u64>(sp + 288, e);
    store<u64>(sp + 304, a6);
    store<u64>(sp + 320, a7);

    const u32 mem = static_cast<u32>(call(ctx, sfn2::Mem_Alloc, {kTextSize, 0, 16}));
    store<u64>(sp + 312, mem);
    if (mem != 0) {
        call(ctx, sfn2::UiText_Construct, {mem, d});
        store<u32>(result, mem);
    }
    if ((a7 & 0xFF) != 0)
        for (u32 c = field<u32>(e, kElemFirstChild); c != 0; c = field<u32>(c, kElemNext))
            call_ptr(ctx, vfunc(c, kVfnClone), {c, c + kElemName, load<u32>(result), a6, 1});
    ret(ctx, load<u32>(result));
}

// Element* UiText2_Clone(TextElem2* e, const char* name, Element* parent, u32 arg, bool deep)  [+8]
// The same for the app-level class (288 bytes): the copy also formats
// its own copy of the source string.
DECOMP(0024F3A0, UiText2_Clone) {
    const u32 e = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), parent = arg<u32>(ctx, 2);
    const u64 a6 = ctx.gpr[6], a7 = ctx.gpr[7];

    // The original's frame: the format buffer at +144, its eight results
    // at +4416, the description at +4240, the spills at +4448..4480.
    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 4488 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 buf = sp + 144, d = sp + 4240, outs = sp + 4416, result = sp + 4464;

    Desc_Fill(d, e, name, parent, static_cast<u32>(a6));
    store<u32>(d + kDescGlyphMode, 0);
    store<u32>(d + kDescRevealRate, kRevealRate3);
    store<u8>(d + kDescByte164, 0);
    store<u32>(d + kDescSource, field<u32>(e, kText2Source));
    store<u64>(sp + 4456, e);
    store<u64>(sp + 4472, a6);
    store<u64>(sp + 4480, a7);

    const u32 mem = static_cast<u32>(call(ctx, sfn2::Mem_Alloc, {kText2Size, 0, 16}));
    store<u64>(sp + 4448, mem);
    store<u32>(result, 0);
    if (mem != 0) {
        call(ctx, sfn2::UiText2_Construct, {mem, d});
        for (u32 off = 236; off <= 272; off += 4) field<u32>(mem, off) = 0;
        field<u32>(mem, 0) = kText2Vtbl;
        const u32 source = load<u32>(d + kDescSource);
        if (source != 0) {
            store<u32>(outs, buf);
            for (u32 k = 1; k < 8; ++k) store<u32>(outs + k * 4, 0);
            call(ctx, sfn2::UiText_Format,
                 {mem, source, buf, kFormatBufSize, outs, outs + 4, outs + 8, outs + 12, outs + 16, outs + 20,
                  outs + 24, outs + 28});
            const u32 bytes = load<u32>(outs + 4);
            if (bytes != 0) {
                const u32 cap = (bytes + 32) & ~31u;
                field<u32>(mem, kText2SourceCap) = cap;
                field<u32>(mem, kText2Source) = static_cast<u32>(call(ctx, sfn2::SmallAlloc, {cap}));
                call(ctx, sfn2::String_Assign, {field<u32>(mem, kText2Source), buf});
                field<u32>(mem, kText2Glyphs) = load<u32>(outs + 8);
            }
        }
        store<u32>(result, mem);
    }
    const u32 out = load<u32>(result);
    if ((a7 & 0xFF) != 0) call(ctx, sfn2::UiElem_CloneChildren, {e, out, a6});
    ret(ctx, out);
}

// void UiText2_SetText(TextElem2* e, const char* text)  [+192]
// Formats `text`, reserves the text buffer and the quads (plus the extra
// glyphs at +264 when there are any), copies the text in and calls +148.
DECOMP(0024FA84, UiText2_SetText) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 text = arg<u32>(ctx, 1);
    constexpr u32 kFrameBase = 144;
    Stack frame(ctx, 176 + kFormatBufSize - kFrameBase);
    const u32 outs = frame.addr(), buf = outs + 32;
    store<u32>(outs, buf);
    for (u32 k = 1; k < 8; ++k) store<u32>(outs + k * 4, 0);
    call(ctx, sfn2::UiText_Format,
         {e, text, buf, kFormatBufSize, outs, outs + 4, outs + 8, outs + 12, outs + 16, outs + 20, outs + 24,
          outs + 28});
    call(ctx, sfn2::UiText_ReserveText, {e, load<u32>(outs + 4)});
    const u32 glyphs = load<u32>(outs + 8);
    call(ctx, sfn2::UiText_ReserveQuads, {e, glyphs != 0 ? glyphs + field<u32>(e, kText2Glyphs) : 0u});
    if (field<u32>(e, kTextBufCap) != 0) call(ctx, sfn2::String_Assign, {field<u32>(e, kTextBuf), buf});
    call_ptr(ctx, vfunc(e, kVfnTextChanged), {e});
}
