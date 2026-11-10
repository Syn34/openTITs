// Game code, medium sweep part 5: XML element readers (XmlCursor_Read*
// write the value or the default into the field, so each default is the
// field's current value unless noted; bool attributes kept as bits go
// through a stack byte).
#include "mid5.h"

using namespace sly4;
using namespace sly4::mid5;

namespace {

// The trigger-volume flags shared by the trigger readers: bits 0x20 /
// 0x10 / 0x08 / 0x02 of the u8 +184, through the byte tmp.
void ReadTriggerFlags(PPUContext& ctx, u32 c, u32 o, u32 tmp) {
    XBit<u8>(ctx, c, 0xBEF25C9F, o + 184, 0x20, tmp);
    XBit<u8>(ctx, c, 0x1EF4F9F5, o + 184, 0x10, tmp);
    XBit<u8>(ctx, c, 0x2D407057, o + 184, 0x08, tmp);
    XBit<u8>(ctx, c, 0x778A1C92, o + 184, 0x02, tmp);
}

constexpr u32 kJumpKinds = 0x00AD3FE4;  // u32[7]: the hashes of the kind names

}  // namespace

// u32 f(x, XmlCursor* c, Trigger* o): the trigger flags; the kind (the
// index of a hash attribute in kJumpKinds into the s32 +192, kept when
// not listed), the hash +188, the f32 +196 and the bits 0x80000000 ..
// 0x08000000 of the u32 +200. Returns the new +200.
DECOMP(0021C074, Trigger_ReadXml_0021C074) {
    const u32 c = arg<u32>(ctx, 1);
    const u32 o = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);
    const u32 kind = frame.addr();         // u32
    const u32 tmp2 = frame.addr() + 4;     // u8 (uninitialized, as the original)
    const u32 tmp = frame.addr() + 5;      // u8
    store<u8>(tmp, 0);
    ReadTriggerFlags(ctx, c, o, tmp);
    store<u32>(kind, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x51609AAA, kind, 0);
    const u32 old188 = load<u32>(o + 188);
    for (u32 i = 0; i < 7; ++i) {
        if (load<u32>(kind) == load<u32>(kJumpKinds + 4 * i)) {
            store<u32>(o + 192, i);
            break;
        }
    }
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x63C8D78E, o + 188, old188);
    XF32(ctx, c, 0x7D873487, o + 196);
    XBit<u32>(ctx, c, 0x25320E69, o + 200, 0x80000000u, tmp2);
    XBit<u32>(ctx, c, 0x98FD6F46, o + 200, 0x40000000u, tmp2);
    XBit<u32>(ctx, c, 0x945416B0, o + 200, 0x20000000u, tmp2);
    XBit<u32>(ctx, c, 0xF1D47360, o + 200, 0x10000000u, tmp2);
    ret(ctx, XBit<u32>(ctx, c, 0xF403C89E, o + 200, 0x08000000u, tmp2));
}

// u32 f(x, XmlCursor* c, Trigger* o): the trigger flags; the vec3 +192
// (scaled by the f32 just before the function when read), the hash +208,
// the f32s +212 / +216 / +220 and the u8 bools +224 .. +227. Returns the
// last read's result.
DECOMP(0021AA68, Trigger_ReadXml_0021AA68) {
    const u32 c = arg<u32>(ctx, 1);
    const u32 o = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);
    const u32 tmp = frame.addr();
    store<u8>(tmp, 0);
    ReadTriggerFlags(ctx, c, o, tmp);
    const u32 r = XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x3C6AE50B, o + 192, o + 192);
    const u32 old208 = load<u32>(o + 208);
    if (r == 0) {
        const f32 s = std::bit_cast<f32>(load<u32>(0x0021AA60));
        const f32 x = std::bit_cast<f32>(load<u32>(o + 192)) * s;
        const f32 y = std::bit_cast<f32>(load<u32>(o + 196)) * s;
        const f32 z = std::bit_cast<f32>(load<u32>(o + 200)) * s;
        store<u32>(o + 192, std::bit_cast<u32>(x));
        store<u32>(o + 196, std::bit_cast<u32>(y));
        store<u32>(o + 200, std::bit_cast<u32>(z));
    }
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x632DA119, o + 208, old208);
    XF32(ctx, c, 0x1486FF40, o + 212);
    XF32(ctx, c, 0x96B07E43, o + 216);
    XF32(ctx, c, 0x1B75F0DA, o + 220);
    XBool(ctx, c, 0xC53B9DE4, o + 224);
    XBool(ctx, c, 0xF7C51457, o + 225);
    XBool(ctx, c, 0x013BDD92, o + 226);
    XBool(ctx, c, 0x23872185, o + 227);
}

namespace {

// The f32 field at p, defaulting to the f32 at def.
void XF32From(PPUContext& ctx, u32 c, u32 hash, u32 p, u32 def) {
    callf(ctx, xfn::XmlCursor_ReadF32, {c, hash, p}, {std::bit_cast<f64>(LfsBits(load<u32>(def)))});
}

constexpr u32 kClamp = 0x0022A798;  // f32 1.0, then f64 1.0, f64 0.0: the clamp of +8

}  // namespace

// u32 f(T* o, x, XmlCursor* c): with a cursor, the f32s +0 .. +48 (+16 /
// +20 defaulting to +12, as the original), the u8 bools +52 .. +55, the
// f32s +56 .. +64 and the u8 bool +68; then +8 clamped to [0, 1] (two
// fsel). Returns o without a cursor, else the last read's result (f1 =
// the clamped value).
DECOMP(0022A7B0, XmlElem_Read_0022A7B0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 2);
    if (c == 0) return;  // r3 still o
    XF32(ctx, c, 0x073FDA26, o);
    XF32(ctx, c, 0x828A472C, o + 4);
    XF32(ctx, c, 0x830581A9, o + 8);
    XF32(ctx, c, 0x656C2768, o + 12);
    XF32From(ctx, c, 0x8CAF155E, o + 16, o + 12);
    XF32From(ctx, c, 0xB12704BB, o + 20, o + 12);
    XF32(ctx, c, 0x20541870, o + 24);
    XF32(ctx, c, 0x800468F9, o + 28);
    XF32(ctx, c, 0xE71A7685, o + 32);
    XF32(ctx, c, 0xC5998708, o + 36);
    XF32(ctx, c, 0x6F1FE90F, o + 40);
    XF32(ctx, c, 0x78B2E37F, o + 44);
    XF32(ctx, c, 0xE627F803, o + 48);
    XBool(ctx, c, 0x11DF5AF3, o + 52);
    XBool(ctx, c, 0xAF71B899, o + 53);
    XBool(ctx, c, 0xE36D9B92, o + 54);
    XBool(ctx, c, 0xCBEB973F, o + 55);
    XF32(ctx, c, 0x2F0F9838, o + 56);
    XF32(ctx, c, 0x1C738971, o + 60);
    XF32(ctx, c, 0x5D88B981, o + 64);
    XBool(ctx, c, 0x4587F608, o + 68);
    const f32 v = std::bit_cast<f32>(load<u32>(o + 8));
    const f32 d = std::bit_cast<f32>(load<u32>(kClamp)) - v;
    f64 x = d >= 0.0f ? static_cast<f64>(v) : std::bit_cast<f64>(load<u64>(kClamp + 8));
    x = static_cast<f64>(static_cast<f32>(x));
    x = x >= 0.0 ? x : std::bit_cast<f64>(load<u64>(kClamp + 16));
    const f32 y = static_cast<f32>(x);
    store<u32>(o + 8, std::bit_cast<u32>(y));
    ctx.fpr[1] = static_cast<f64>(y);
}

namespace {

// The f32 at p minus the f32 at q (fsubs), stored back.
void SubF32(u32 p, u32 q) {
    store<u32>(p, std::bit_cast<u32>(std::bit_cast<f32>(load<u32>(p)) - std::bit_cast<f32>(load<u32>(q))));
}

}  // namespace

// u32 f(T* o, x, XmlCursor* c): with a cursor, the f32s +0 .. +84 (+16 /
// +24, when read, made relative to +12), the u8 bool +88 and the f32s
// +92 .. +100. Returns o without a cursor, else the last read's result.
DECOMP(0022CDE4, XmlElem_Read_0022CDE4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 2);
    if (c == 0) return;  // r3 still o
    XF32(ctx, c, 0x878E8E5D, o);
    XF32(ctx, c, 0x6AB3212D, o + 4);
    XF32(ctx, c, 0x87225E0B, o + 8);
    XF32(ctx, c, 0xBBBCFC4E, o + 12);
    if (XF32(ctx, c, 0xB9929A14, o + 16) == 0) SubF32(o + 16, o + 12);
    XF32(ctx, c, 0xEB520DD7, o + 20);
    if (XF32(ctx, c, 0xFE6B4E76, o + 24) == 0) SubF32(o + 24, o + 12);
    static constexpr u32 kHashes[] = {0x67090F29, 0x669CDED7, 0x94A37F18, 0x0E1B3C68, 0x94374EC6, 0xFECDC4C2, 0xC1430B1E,
                                      0x199012B5, 0x3C62C907, 0x5497B0D4, 0xA2F8435A, 0x41DDD71A, 0xE8255C41, 0xDB0B8253, 0x4B4910D7};
    for (u32 k = 0; k < 15; ++k) XF32(ctx, c, kHashes[k], o + 28 + 4 * k);
    XBool(ctx, c, 0x32376944, o + 88);
    XF32(ctx, c, 0x87B7924C, o + 92);
    XF32(ctx, c, 0xDD3EA7BA, o + 96);
    XF32(ctx, c, 0xCA98C11E, o + 100);
}
