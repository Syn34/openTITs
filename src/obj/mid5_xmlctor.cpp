// Game code, medium sweep part 5: XML element constructors: the base
// (Obj_InitVt_B21C98: +4 = 1, the base vtable), the fields set to their
// defaults and the class's vtable, then read attribute by attribute
// (each default the field's value). Vectors default to the origin
// (kOrigin, copied as words).
#include "mid5.h"

using namespace sly4;
using namespace sly4::mid5;

namespace {

constexpr u32 Obj_InitVt_B21C98 = 0x004BB3B0;  // (o): +4 = 1, the base vtable
constexpr u32 kOrigin = 0x01164B40;            // f32[3]

namespace xfn2 {
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;  // (c, const char* name) -> const char* or 0
constexpr u32 String_Copy = 0x00553684;               // (dst, src, n)
}  // namespace xfn2

void CopyVec3(u32 dst, u32 src) {
    for (u32 k = 0; k < 3; ++k) store<u32>(dst + 4 * k, load<u32>(src + 4 * k));
}

// The f32[3] at p scaled by s (fmuls), stored back; f1 is left holding
// component f1_k, as the original's register use leaves it.
void ScaleVec3(PPUContext& ctx, u32 p, f32 s, u32 f1_k) {
    f32 v[3];
    for (u32 k = 0; k < 3; ++k) v[k] = std::bit_cast<f32>(load<u32>(p + 4 * k)) * s;
    for (u32 k = 0; k < 3; ++k) store<u32>(p + 4 * k, std::bit_cast<u32>(v[k]));
    ctx.fpr[1] = static_cast<f64>(v[f1_k]);
}

// The shared head of the two curve elements (vtable vt, type hash h).
void CurveCtor(PPUContext& ctx, u32 o, u32 c, u32 vt, u32 h, bool flag) {
    call(ctx, Obj_InitVt_B21C98, {o});
    store<u32>(o + 12, 0);
    store<u32>(o + 16, 0);
    store<u32>(o + 20, 0x3F800000);
    store<u32>(o + 24, 0x3F800000);
    store<u32>(o + 28, 0);
    store<u32>(o + 32, 0x85D98325);
    store<u32>(o + 36, 0);
    store<u32>(o + 40, 0x3F800000);
    store<u32>(o + 44, 0);
    store<u32>(o, vt);
    store<u32>(o + 48, 0x3F800000);
    store<u32>(o + 52, 0x40800000);
    store<u32>(o + 56, 0x40D00000);
    store<u32>(o + 60, 0x40200000);
    store<u32>(o + 64, 0);
    store<u32>(o + 80, load<u32>(kOrigin));
    store<u32>(o + 84, load<u32>(kOrigin + 4));
    store<u32>(o + 96, 0xBF800000);
    store<u32>(o + 88, load<u32>(kOrigin + 8));
    if (flag) store<u8>(o + 112, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, h, o + 12, 0);
    XF32(ctx, c, 0x8FA16092, o + 64);
    XF32(ctx, c, 0xB7EA3D94, o + 20);
    XF32(ctx, c, 0x2DE261F0, o + 24);
    XF32(ctx, c, 0x793E3FEE, o + 28);
    XHash(ctx, c, 0x66D70397, o + 32);
    XF32(ctx, c, 0x65026C9B, o + 36);
    XF32(ctx, c, 0x0489CB20, o + 40);
    XF32(ctx, c, 0x0001564F, o + 44);
    XF32(ctx, c, 0x0026787D, o + 48);
    XF32(ctx, c, 0x6D644E1C, o + 52);
    XF32(ctx, c, 0x21B4E7CD, o + 56);
    XF32(ctx, c, 0x2168595F, o + 60);
}

}  // namespace

// void f(T* o, XmlCursor* c): vtable 0x00B11258; the type hash +12, the
// f32s +16 .. +40 (defaults 0, 1, 10, 20, 10, 40, 1), the s32 +44 (1)
// and the u8 bools +48 / +49 / +50 (0, 1, 0). Returns the last read's
// result.
DECOMP(0009D850, XmlElem_Ctor_0009D850) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    call(ctx, Obj_InitVt_B21C98, {o});
    store<u32>(o + 12, 0);
    store<u32>(o + 16, 0);
    store<u32>(o, 0x00B11258);
    store<u32>(o + 20, 0x3F800000);
    store<u32>(o + 24, 0x41200000);
    store<u32>(o + 28, 0x41A00000);
    store<u32>(o + 32, 0x41200000);
    store<u32>(o + 36, 0x42200000);
    store<u32>(o + 40, 0x3F800000);
    store<u32>(o + 44, 1);
    store<u8>(o + 48, 0);
    store<u8>(o + 49, 1);
    store<u8>(o + 50, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xED2B8734, o + 12, 0);
    XF32(ctx, c, 0x3C5308B5, o + 16);
    XF32(ctx, c, 0x59D5C9EE, o + 20);
    XF32(ctx, c, 0x7089C1B7, o + 24);
    XF32(ctx, c, 0x070ADB70, o + 28);
    XF32(ctx, c, 0xB1D3B545, o + 32);
    XF32(ctx, c, 0x3DB85A7E, o + 36);
    XF32(ctx, c, 0xB7EA3D94, o + 40);
    XS32(ctx, c, 0xB734811C, o + 44);
    XBool(ctx, c, 0x26873C74, o + 48);
    XBool(ctx, c, 0x8488565B, o + 49);
    XBool(ctx, c, 0x2E09C6B8, o + 50);
}

// void f(T* o, XmlCursor* c): a curve element (vtable 0x00B0F298, type
// hash 0x7F247172): f32 fields +20 .. +64, the hash +32 (default
// 0x85D98325), +80 the origin, +96 -1.0. Returns the last read's result.
DECOMP(0006F094, XmlCurve_Ctor_0006F094) {
    CurveCtor(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), 0x00B0F298, 0x7F247172, false);
}

// void f(T* o, XmlCursor* c): the same (vtable 0x00B10FD8, type hash
// 0x7A30D7E3) with the f32 +60 read twice and the u8 bool +112.
DECOMP(0009B3DC, XmlCurve_Ctor_0009B3DC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    CurveCtor(ctx, o, c, 0x00B10FD8, 0x7A30D7E3, true);
    XF32(ctx, c, 0x2168595F, o + 60);
    XBool(ctx, c, 0x8488565B, o + 112);
}

// void f(T* o, XmlCursor* c): vtable 0x00B10F38; the hashes +12 .. +24,
// +48, +80, +84, the vectors +32 / +64 / +96 / +112 (origin defaults;
// +64 and +112 scaled by the f32 just before the function) and the u8
// bools +128 .. +131. Returns the last read's result.
DECOMP(0009A670, XmlElem_Ctor_0009A670) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    const f32 s = std::bit_cast<f32>(load<u32>(0x0009A66C));
    call(ctx, Obj_InitVt_B21C98, {o});
    store<u32>(o + 12, 0);
    store<u32>(o + 16, 0);
    store<u32>(o + 20, 0);
    store<u32>(o + 24, 0);
    store<u32>(o + 32, load<u32>(kOrigin));
    store<u32>(o + 36, load<u32>(kOrigin + 4));
    store<u32>(o + 48, 0);
    store<u32>(o + 40, load<u32>(kOrigin + 8));
    store<u32>(o + 64, load<u32>(kOrigin));
    store<u32>(o + 68, load<u32>(kOrigin + 4));
    store<u32>(o + 80, 0);
    store<u32>(o + 72, load<u32>(kOrigin + 8));
    store<u32>(o + 84, 0);
    store<u32>(o, 0x00B10F38);
    store<u32>(o + 96, load<u32>(kOrigin));
    store<u32>(o + 100, load<u32>(kOrigin + 4));
    store<u32>(o + 104, load<u32>(kOrigin + 8));
    store<u32>(o + 112, load<u32>(kOrigin));
    store<u32>(o + 116, load<u32>(kOrigin + 4));
    store<u8>(o + 128, 0);
    store<u32>(o + 120, load<u32>(kOrigin + 8));
    store<u8>(o + 129, 0);
    store<u8>(o + 130, 0);
    store<u8>(o + 131, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xE7E52D0D, o + 12, 0);
    XHash(ctx, c, 0x523827DF, o + 16);
    XHash(ctx, c, 0x20C00073, o + 20);
    XHash(ctx, c, 0x277591B4, o + 24);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0xEF4DDD13, o + 32, o + 32);
    XHash(ctx, c, 0x1612F114, o + 48);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0xC7400937, o + 64, o + 64);
    XHash(ctx, c, 0xF4F1A270, o + 80);
    ScaleVec3(ctx, o + 64, s, 1);
    XHash(ctx, c, 0xF91B96E2, o + 84);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x1F1D93C2, o + 96, o + 96);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0xA4F068E8, o + 112, o + 112);
    ScaleVec3(ctx, o + 112, s, 0);
    XBool(ctx, c, 0x36E7D729, o + 128);
    XBool(ctx, c, 0x3E3F7E94, o + 129);
    XBool(ctx, c, 0xD1A6825E, o + 130);
    XBool(ctx, c, 0x3E496BEB, o + 131);
}

// u32 f(T* o, XmlCursor* c): the colors +208 / +212, the hashes +204 /
// +228, the f32s +216 / +220 / +260, the s32 +224, the vectors +232 and
// +248 (read through locals defaulting to the origin) and the hash +244
// (default 0); +200 cleared. Returns 1 (f1 left holding the last word
// of +248).
DECOMP(00288C3C, XmlElem_Read_00288C3C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    Stack frame(ctx, 32);
    const u32 v1 = frame.addr() + 16;  // r1+128 in the original
    const u32 v2 = frame.addr();       // r1+112
    store<u32>(v1, load<u32>(kOrigin));
    store<u32>(v1 + 4, load<u32>(kOrigin + 4));
    store<u32>(o + 228, 0);
    store<u32>(v1 + 8, load<u32>(kOrigin + 8));
    XRead(ctx, xfn::XmlCursor_ReadColor, c, 0x8F930B87, o + 208, o + 208);
    XRead(ctx, xfn::XmlCursor_ReadColor, c, 0xB4500507, o + 212, o + 212);
    XHash(ctx, c, 0xE08EDE06, o + 204);
    XF32(ctx, c, 0x4DFD3622, o + 216);
    XF32(ctx, c, 0x726B2021, o + 220);
    XS32(ctx, c, 0x72BD3841, o + 224);
    XHash(ctx, c, 0x39F79104, o + 228);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0xD883FE50, v1, v1);
    CopyVec3(o + 232, v1);
    store<u32>(o + 244, 0);
    store<u32>(o + 260, 0);
    store<u32>(o + 200, 0);
    CopyVec3(v2, kOrigin);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x00255BF3, o + 244, 0);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x313AB0C1, v2, v2);
    XF32(ctx, c, 0x8FA16092, o + 260);
    CopyVec3(o + 248, v2);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(v2 + 8)));
    ret(ctx, 1u);
}

// u32 f(T* o, XmlCursor* c): vtable 0x00B114B0; the type hash +12, the
// hashes +16 / +20 / +28 / +40 / +44, the f32s +32 (1) / +36, the u8
// bools +48 .. +51 (1, 1, 1, 0); then +24 = whether the attribute
// 0x96997C96 is the hash 0x0023BEA4 (left as is without it). Returns 1 / 0
// for the comparison, else the last read's result.
DECOMP(0009FF90, XmlElem_Ctor_0009FF90) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    call(ctx, Obj_InitVt_B21C98, {o});
    store<u32>(o + 12, 0);
    store<u32>(o + 16, 0);
    store<u32>(o, 0x00B114B0);
    store<u32>(o + 20, 0);
    store<u8>(o + 24, 0);
    store<u32>(o + 28, 0);
    store<u32>(o + 32, 0x3F800000);
    store<u32>(o + 36, 0);
    store<u32>(o + 40, 0);
    store<u32>(o + 44, 0);
    store<u8>(o + 48, 1);
    store<u8>(o + 49, 1);
    store<u8>(o + 50, 1);
    store<u8>(o + 51, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x7A30D7E3, o + 12, 0);
    XHash(ctx, c, 0xE8C32B64, o + 16);
    XHash(ctx, c, 0xD017B523, o + 20);
    XHash(ctx, c, 0x044DDD31, o + 28);
    XF32(ctx, c, 0x04B70827, o + 32);
    XF32(ctx, c, 0x03C5A831, o + 36);
    XHash(ctx, c, 0x7AD841CF, o + 40);
    XHash(ctx, c, 0x941382D8, o + 44);
    XBool(ctx, c, 0xC1B28F57, o + 48);
    XBool(ctx, c, 0x8488565B, o + 49);
    XBool(ctx, c, 0x81D8B678, o + 50);
    XBool(ctx, c, 0x0CA4259B, o + 51);
    Stack frame(ctx, 16);
    const u32 h = frame.addr();  // uninitialized: the read stores its default
    if (XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x96997C96, h, 0) != 0) return;
    const u32 is = load<u32>(h) == 0x0023BEA4u ? 1u : 0u;
    store<u8>(o + 24, static_cast<u8>(is));
    ret(ctx, is);
}

// void f(T* o, XmlCursor* c): vtable 0x00B10B20; the hashes +12 .. +48
// (the first defaulting to 0), the vectors +64 / +80 (origin), the f32
// +96 and the name +100 (char[64], copied from "entityName" when given).
// Returns the last call's result.
DECOMP(00095270, XmlElem_Ctor_00095270) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    call(ctx, Obj_InitVt_B21C98, {o});
    for (u32 off = 12; off <= 48; off += 4) store<u32>(o + off, 0);
    store<u32>(o, 0x00B10B20);
    store<u32>(o + 64, load<u32>(kOrigin));
    store<u32>(o + 68, load<u32>(kOrigin + 4));
    store<u32>(o + 72, load<u32>(kOrigin + 8));
    store<u32>(o + 80, load<u32>(kOrigin));
    store<u32>(o + 84, load<u32>(kOrigin + 4));
    store<u32>(o + 96, 0);
    store<u32>(o + 88, load<u32>(kOrigin + 8));
    store<u8>(o + 100, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x7D1591B3, o + 12, 0);
    XHash(ctx, c, 0x8A93973F, o + 16);
    XHash(ctx, c, 0x04B8CC42, o + 20);
    XHash(ctx, c, 0x00010CBB, o + 24);
    XHash(ctx, c, 0x03FCC956, o + 28);
    XHash(ctx, c, 0xBFA06A67, o + 32);
    XHash(ctx, c, 0x230121E0, o + 36);
    XHash(ctx, c, 0x02F0137B, o + 40);
    XHash(ctx, c, 0x273A3735, o + 44);
    XHash(ctx, c, 0xCB0B932E, o + 48);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x40156B55, o + 64, o + 64);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x34FFAD0E, o + 80, o + 80);
    XF32(ctx, c, 0xE97A4F3A, o + 96);
    const u32 s = static_cast<u32>(call(ctx, xfn2::XmlCursor_FindAttrByName, {c, 0x00AC56F4}));  // "entityName"
    if (s != 0) call(ctx, xfn2::String_Copy, {o + 100, s, 64});
}

// T* f(T* o, x, XmlCursor* c): with a cursor, the f32s +56 .. +72, +16 /
// +24 / +32, the hash +0 (through a zeroed local, default 0) and the
// hashes +36 .. +52, +4, +12, +20, +28. Returns o without a cursor, else
// the last read's result.
DECOMP(00229CC4, XmlElem_Read_00229CC4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 2);
    if (c == 0) return;  // r3 still o
    XF32(ctx, c, 0x37D348F8, o + 56);
    XF32(ctx, c, 0x45C73F02, o + 60);
    XF32(ctx, c, 0x85FDED67, o + 64);
    XF32(ctx, c, 0x459F3B32, o + 68);
    XF32(ctx, c, 0x012409E4, o + 72);
    XF32(ctx, c, 0x7F549444, o + 16);
    XF32(ctx, c, 0x52CCA2BC, o + 24);
    XF32(ctx, c, 0x9CD41A9E, o + 32);
    Stack frame(ctx, 16);
    const u32 h = frame.addr();
    store<u32>(h, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x3A15BFAB, h, 0);
    const u32 v = load<u32>(h);
    const u32 d36 = load<u32>(o + 36);
    store<u32>(o, v);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x79930AF1, o + 36, d36);
    XHash(ctx, c, 0x6E695AF2, o + 40);
    XHash(ctx, c, 0xA26189D4, o + 44);
    XHash(ctx, c, 0x2289ADF9, o + 48);
    XHash(ctx, c, 0x228D0181, o + 52);
    XHash(ctx, c, 0x736363FD, o + 4);
    XHash(ctx, c, 0x35453DCF, o + 12);
    XHash(ctx, c, 0x35454047, o + 20);
    XHash(ctx, c, 0x73609267, o + 28);
}

// void f(T* o, XmlCursor* c): vtable 0x00B11968; the vectors +16 / +32
// (origin defaults; +32 scaled by the f32 just before the function when
// read), the hashes +48 .. +64 and the u8 bools +68 / +69 / +70 (0, 1,
// 0). Returns the last read's result.
DECOMP(000A5AA8, XmlElem_Ctor_000A5AA8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    call(ctx, Obj_InitVt_B21C98, {o});
    store<u32>(o + 16, load<u32>(kOrigin));
    store<u32>(o + 20, load<u32>(kOrigin + 4));
    store<u32>(o, 0x00B11968);
    store<u32>(o + 24, load<u32>(kOrigin + 8));
    store<u32>(o + 32, load<u32>(kOrigin));
    store<u32>(o + 36, load<u32>(kOrigin + 4));
    store<u32>(o + 48, 0);
    const u32 z = load<u32>(kOrigin + 8);
    store<u32>(o + 40, z);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(z));  // f1 left by the copies
    store<u32>(o + 52, 0);
    store<u32>(o + 56, 0);
    store<u32>(o + 60, 0);
    store<u32>(o + 64, 0);
    store<u8>(o + 68, 0);
    store<u8>(o + 69, 1);
    store<u8>(o + 70, 0);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0xCD8B05D5, o + 16, o + 16);
    const u32 r = XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x313AB844, o + 32, o + 32);
    const u32 d48 = load<u32>(o + 48);
    if (r == 0) ScaleVec3(ctx, o + 32, std::bit_cast<f32>(load<u32>(0x000A5AA4)), 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x7A30D7E3, o + 48, d48);
    XHash(ctx, c, 0x8D47DA8F, o + 52);
    XHash(ctx, c, 0xA4304B8E, o + 56);
    XHash(ctx, c, 0x4BA42764, o + 60);
    XHash(ctx, c, 0x04DF3157, o + 64);
    XBool(ctx, c, 0x2FA79D7F, o + 68);
    XBool(ctx, c, 0xA18DAB9F, o + 69);
    XBool(ctx, c, 0x3FC9B95A, o + 70);
}

// void f(T* o, XmlCursor* c): vtable 0x00B0F180; the hashes +12 / +16 /
// +20 / +64, the vectors +32 / +48 / +80 / +96 (origin defaults; +48 and
// +96 scaled by the f32 just before the function), the f32 +112 (-1;
// read sets the u8 +120) and the u8 bools +116 .. +119, +121. Returns the
// last read's result.
DECOMP(0006D488, XmlElem_Ctor_0006D488) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    const f32 s = std::bit_cast<f32>(load<u32>(0x0006D480));
    call(ctx, Obj_InitVt_B21C98, {o});
    store<u32>(o + 12, 0);
    store<u32>(o + 16, 0);
    store<u32>(o + 20, 0);
    CopyVec3(o + 32, kOrigin);
    store<u32>(o + 48, load<u32>(kOrigin));
    store<u32>(o + 52, load<u32>(kOrigin + 4));
    store<u32>(o + 64, 0);
    store<u32>(o + 56, load<u32>(kOrigin + 8));
    store<u32>(o, 0x00B0F180);
    CopyVec3(o + 80, kOrigin);
    CopyVec3(o + 96, kOrigin);
    store<u32>(o + 112, 0xBF800000);
    for (u32 k = 116; k <= 121; ++k) store<u8>(o + k, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x8C3DFCAA, o + 12, 0);
    XHash(ctx, c, 0x03D1FCFC, o + 16);
    XHash(ctx, c, 0x20C00073, o + 20);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0xEF4DDD13, o + 32, o + 32);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0xC7400937, o + 48, o + 48);
    ScaleVec3(ctx, o + 48, s, 1);
    XHash(ctx, c, 0xF91B96E2, o + 64);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x1F1D93C2, o + 80, o + 80);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0xA4F068E8, o + 96, o + 96);
    ScaleVec3(ctx, o + 96, s, 2);
    const u32 r = XF32(ctx, c, 0x3B83FE2E, o + 112);
    const u32 d116 = load<u8>(o + 116);
    if (r == 0) store<u8>(o + 120, 1);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x36E7D729, o + 116, d116);
    XBool(ctx, c, 0x3E3F7E94, o + 117);
    XBool(ctx, c, 0xD1A6825E, o + 118);
    XBool(ctx, c, 0xDC623C3E, o + 119);
    XBool(ctx, c, 0x3B757A97, o + 121);
}

// void f(T* o, XmlCursor* c): vtable 0x00B11A08; the hashes +80 .. +100,
// the vectors +112 / +128 (origin defaults; +128 scaled by the f32 just
// before the function), the u8 bools +144 .. +149 and the name +12
// (char[64], copied from "name" when given and not empty). Returns the
// last call's result (f1 left holding the scaled +136).
DECOMP(000A6478, XmlElem_Ctor_000A6478) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    call(ctx, Obj_InitVt_B21C98, {o});
    store<u32>(o + 80, 0);
    store<u32>(o + 84, 0);
    store<u32>(o, 0x00B11A08);
    store<u32>(o + 88, 0);
    store<u32>(o + 92, 0);
    store<u32>(o + 96, 0);
    store<u32>(o + 100, 0);
    CopyVec3(o + 112, kOrigin);
    store<u32>(o + 128, load<u32>(kOrigin));
    store<u32>(o + 132, load<u32>(kOrigin + 4));
    store<u8>(o + 144, 0);
    store<u32>(o + 136, load<u32>(kOrigin + 8));
    for (u32 k = 145; k <= 149; ++k) store<u8>(o + k, 0);
    store<u8>(o + 12, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x79F40122, o + 80, 0);
    XHash(ctx, c, 0xD2111EA5, o + 84);
    XHash(ctx, c, 0xB2E59A9F, o + 88);
    XHash(ctx, c, 0x6D45B808, o + 92);
    XHash(ctx, c, 0xE7E52D0D, o + 96);
    XHash(ctx, c, 0x00255BF3, o + 100);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x313AB0C1, o + 112, o + 112);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x313AB844, o + 128, o + 128);
    ScaleVec3(ctx, o + 128, std::bit_cast<f32>(load<u32>(0x000A6470)), 2);
    XBool(ctx, c, 0x36E7D729, o + 144);
    XBool(ctx, c, 0x3E3F7E94, o + 145);
    XBool(ctx, c, 0x639A3FE5, o + 146);
    XBool(ctx, c, 0x5D4879B3, o + 147);
    XBool(ctx, c, 0x3D1FC26C, o + 148);
    XBool(ctx, c, 0x3E496BEB, o + 149);
    const u32 s = static_cast<u32>(call(ctx, xfn2::XmlCursor_FindAttrByName, {c, 0x00AC5A18}));  // "name"
    if (s != 0 && load<u8>(s) != 0) call(ctx, xfn2::String_Copy, {o + 12, s, 64});
}

// void f(T* o, XmlCursor* c): vtable 0x00B112A8, defaults for +12 ..
// +140; the hashes +16 / +20 / +12 (default 0); only when one of them is
// given: the hash +36, the f32s +40 .. +104 (the hash +52 among them)
// and the u8 bool +140. Returns the last read's result.
DECOMP(0009DEE8, XmlElem_Ctor_0009DEE8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    call(ctx, Obj_InitVt_B21C98, {o});
    store<u32>(o + 12, 0);
    store<u32>(o + 16, 0);
    store<u32>(o, 0x00B112A8);
    store<u32>(o + 20, 0);
    store<u32>(o + 28, 0x3F800000);
    store<u32>(o + 24, 0x489EBD81);
    store<u32>(o + 32, 0);
    store<u32>(o + 36, 0);
    store<u32>(o + 40, 0x3F800000);
    store<u32>(o + 44, 0x3F800000);
    store<u32>(o + 48, 0);
    store<u32>(o + 52, 0x85D98325);
    store<u32>(o + 56, 0);
    store<u32>(o + 60, 0x3F800000);
    store<u32>(o + 64, 0);
    store<u32>(o + 68, 0x3F800000);
    store<u32>(o + 72, 0x40800000);
    store<u32>(o + 76, 0x40D00000);
    store<u32>(o + 80, 0x40200000);
    for (u32 off = 84; off <= 128; off += 4) store<u32>(o + off, 0);
    store<u32>(o + 132, 0xBF800000);
    for (u32 k = 136; k <= 140; ++k) store<u8>(o + k, 0);
    if (XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x7A30D7E3, o + 16, 0) != 0 &&
        XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x5C8A3E05, o + 20, 0) != 0 &&
        XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x8A93973F, o + 12, 0) != 0)
        return;
    XHash(ctx, c, 0x00255BF3, o + 36);
    XF32(ctx, c, 0xB7EA3D94, o + 40);
    XF32(ctx, c, 0x2DE261F0, o + 44);
    XF32(ctx, c, 0x793E3FEE, o + 48);
    XHash(ctx, c, 0x66D70397, o + 52);
    XF32(ctx, c, 0x65026C9B, o + 56);
    XF32(ctx, c, 0x0489CB20, o + 60);
    XF32(ctx, c, 0x0001564F, o + 64);
    XF32(ctx, c, 0x0026787D, o + 68);
    XF32(ctx, c, 0x6D644E1C, o + 72);
    XF32(ctx, c, 0x21B4E7CD, o + 76);
    XF32(ctx, c, 0x2168595F, o + 80);
    XF32(ctx, c, 0xB9EEB6EB, o + 84);
    XF32(ctx, c, 0xEED4F22C, o + 88);
    XF32(ctx, c, 0x23BB2D6D, o + 92);
    XF32(ctx, c, 0xA99476B1, o + 96);
    XF32(ctx, c, 0x8B265490, o + 100);
    XF32(ctx, c, 0x6CB8326F, o + 104);
    XBool(ctx, c, 0x8488565B, o + 140);
}

// u32 f(T* o, XmlCursor* c): vtable 0x00B10D78; the vector +16 (origin),
// the hashes +32 / +36 / +40 / +48, the s32s +44 (one-based in the file:
// a given non-zero value is decremented), +52 / +56 and the u8 bools +60
// .. +62 (1, 0, 0); then +52 = min(+52, +56) (signed). Returns +56.
DECOMP(00098BE4, XmlElem_Ctor_00098BE4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    call(ctx, Obj_InitVt_B21C98, {o});
    store<u32>(o + 16, load<u32>(kOrigin));
    store<u32>(o, 0x00B10D78);
    store<u32>(o + 20, load<u32>(kOrigin + 4));
    store<u32>(o + 32, 0);
    const u32 z = load<u32>(kOrigin + 8);
    store<u32>(o + 24, z);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(z));  // f1 left by the copies
    for (u32 off = 36; off <= 56; off += 4) store<u32>(o + off, 0);
    store<u8>(o + 60, 1);
    store<u8>(o + 61, 0);
    store<u8>(o + 62, 0);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x31B11071, o + 16, o + 16);
    XHash(ctx, c, 0xFE747351, o + 32);
    XHash(ctx, c, 0x37E35A18, o + 36);
    XHash(ctx, c, 0x27805B2B, o + 40);
    const u32 r = XS32(ctx, c, 0x3683141C, o + 44);
    const u32 d48 = load<u32>(o + 48);
    if (r == 0 && load<u32>(o + 44) != 0) store<u32>(o + 44, load<u32>(o + 44) - 1);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x09CFA52D, o + 48, d48);
    XS32(ctx, c, 0x4F479F4E, o + 52);
    XS32(ctx, c, 0xBBEFE7FC, o + 56);
    XBool(ctx, c, 0xC8A764AA, o + 60);
    XBool(ctx, c, 0x6DBD11CC, o + 61);
    XBool(ctx, c, 0x771AFC9F, o + 62);
    const s32 a = static_cast<s32>(load<u32>(o + 52));
    const s32 b = static_cast<s32>(load<u32>(o + 56));
    store<u32>(o + 52, static_cast<u32>(a > b ? b : a));
    ret(ctx, static_cast<u32>(b));
}

// void f(T* o, XmlCursor* c): vtable 0x00B103A0; the color +12 (default
// the u8[4] at 0x01164CA4), the vector +16 (origin), the s32 +32 (default
// -1; one-based in the file: a given positive value is decremented), the
// hashes +36 .. +52 (the last default 0) and the u8 bools +56 .. +59
// (default 0); the bool 0x3B66FD69 (through a local) sets both +60 / +61,
// else they are read (default 0); then the u8 bool +62. Returns the last
// read's result.
DECOMP(000892D4, XmlElem_Ctor_000892D4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    call(ctx, Obj_InitVt_B21C98, {o});
    store<u32>(o, 0x00B103A0);
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 12 + k, load<u8>(0x01164CA4 + k));
    store<u32>(o + 16, load<u32>(kOrigin));
    store<u32>(o + 20, load<u32>(kOrigin + 4));
    store<u32>(o + 32, 0xFFFFFFFFu);
    const u32 z = load<u32>(kOrigin + 8);
    store<u32>(o + 24, z);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(z));  // f1 left by the copies
    for (u32 off = 36; off <= 52; off += 4) store<u32>(o + off, 0);
    for (u32 k = 56; k <= 62; ++k) store<u8>(o + k, 0);
    const u32 r = static_cast<u32>(call(ctx, xfn::XmlCursor_ReadS32, {c, 0x3683141C, o + 32, ~0ull}));
    const u32 d36 = load<u32>(o + 36);
    if (r == 0 && static_cast<s32>(load<u32>(o + 32)) > 0) store<u32>(o + 32, load<u32>(o + 32) - 1);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x8A93973F, o + 36, d36);
    XHash(ctx, c, 0x7A30D7E3, o + 40);
    XHash(ctx, c, 0x98ACCC10, o + 44);
    XHash(ctx, c, 0x884A0CB4, o + 48);
    XRead(ctx, xfn::XmlCursor_ReadColor, c, 0x2ED4500F, o + 12, o + 12);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x8ACA5893, o + 16, o + 16);
    Stack frame(ctx, 16);
    const u32 both = frame.addr();
    store<u8>(both, 0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x3B66FD69, both, 0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x5D28E3FC, o + 56, 0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x9446A097, o + 57, 0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0xD9159FE7, o + 58, 0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x09DA53D5, o + 59, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x6E6EDD6B, o + 52, 0);
    if (load<u8>(both) != 0) {
        store<u8>(o + 60, 1);
        store<u8>(o + 61, 1);
    } else {
        XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x04656B91, o + 60, 0);
        XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x310F51B2, o + 61, 0);
    }
    XBool(ctx, c, 0x5B973429, o + 62);
}

// void f(T* o, XmlCursor* c): vtable 0x00B0F1F8; the hashes +12 / +16
// (default 0x0027469E), +20 / +24 / +28, the u8 bool +36; the value
// 0x001FE7AA into +40 typed by the hash +28 (0x7A30D7E3 / 0x0021B90E: a
// hash, 0x03FE2A3C: an f32, 0x00011BCF: an s32, else not read); then the
// hash +32. Returns the last read's result.
DECOMP(0006E63C, XmlElem_Ctor_0006E63C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    call(ctx, Obj_InitVt_B21C98, {o});
    store<u32>(o, 0x00B0F1F8);
    store<u32>(o + 20, 0);
    store<u32>(o + 12, 0x0027469E);
    store<u32>(o + 16, 0x0027469E);
    store<u32>(o + 24, 0);
    store<u32>(o + 28, 0);
    store<u32>(o + 32, 0);
    store<u8>(o + 36, 0);
    store<u32>(o + 40, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x91930E35, o + 12, 0x0027469E);
    XHash(ctx, c, 0xD6F981F9, o + 16);
    XHash(ctx, c, 0x6A87CEA3, o + 20);
    XHash(ctx, c, 0x03F47A7A, o + 24);
    XHash(ctx, c, 0x995789E4, o + 28);
    XBool(ctx, c, 0x5B973429, o + 36);
    switch (load<u32>(o + 28)) {
    case 0x7A30D7E3:
    case 0x0021B90E: XHash(ctx, c, 0x001FE7AA, o + 40); break;
    case 0x03FE2A3C: XF32(ctx, c, 0x001FE7AA, o + 40); break;
    case 0x00011BCF: XS32(ctx, c, 0x001FE7AA, o + 40); break;
    default: break;
    }
    XHash(ctx, c, 0x80D14B1D, o + 32);
}

// void f(T* o, XmlCursor* c): vtable 0x00B0F838, defaults for +12 ..
// +147; the hashes +16 / +24 / +12 (default 0); only when one of them is
// given: the hashes +20 / +44 / +28 / +60, the f32s +48 .. +140 and the
// u8 bools +144 .. +147. Returns the last read's result.
DECOMP(00078F04, XmlElem_Ctor_00078F04) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    call(ctx, Obj_InitVt_B21C98, {o});
    store<u32>(o + 12, 0);
    store<u32>(o + 16, 0);
    store<u32>(o, 0x00B0F838);
    store<u32>(o + 20, 0);
    store<u32>(o + 24, 0);
    store<u32>(o + 28, 0);
    store<u32>(o + 32, 0x489EBD81);
    store<u32>(o + 36, 0x3F800000);
    store<u32>(o + 40, 0);
    store<u32>(o + 44, 0);
    store<u32>(o + 48, 0x3F800000);
    store<u32>(o + 52, 0x3F800000);
    store<u32>(o + 56, 0);
    store<u32>(o + 60, 0x85D98325);
    store<u32>(o + 64, 0);
    store<u32>(o + 68, 0x3F800000);
    store<u32>(o + 72, 0);
    store<u32>(o + 76, 0x3F800000);
    store<u32>(o + 80, 0x40800000);
    store<u32>(o + 84, 0x40D00000);
    store<u32>(o + 88, 0x40200000);
    for (u32 off = 92; off <= 136; off += 4) store<u32>(o + off, 0);
    store<u32>(o + 140, 0xBF800000);
    for (u32 k = 144; k <= 147; ++k) store<u8>(o + k, 0);
    if (XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x7A30D7E3, o + 16, 0) != 0 &&
        XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x5C8A3E05, o + 24, 0) != 0 &&
        XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x8A93973F, o + 12, 0) != 0)
        return;
    XFields(ctx, c, o,
            {{'H', 0x03D1FCFC, 20}, {'H', 0x00255BF3, 44}, {'F', 0xB7EA3D94, 48}, {'H', 0x63D534F3, 28}, {'F', 0x2DE261F0, 52},
             {'F', 0x793E3FEE, 56}, {'H', 0x66D70397, 60}, {'F', 0x65026C9B, 64}, {'F', 0x0489CB20, 68}, {'F', 0x0001564F, 72},
             {'F', 0x0026787D, 76}, {'F', 0x6D644E1C, 80}, {'F', 0x21B4E7CD, 84}, {'F', 0x2168595F, 88}, {'F', 0xB9EEB6EB, 92},
             {'F', 0xEED4F22C, 96}, {'F', 0x23BB2D6D, 100}, {'F', 0xA99476B1, 104}, {'F', 0x8B265490, 108}, {'F', 0x6CB8326F, 112},
             {'F', 0x9ADB87D2, 116}, {'F', 0x9C906071, 120}, {'F', 0x9E453910, 124}, {'F', 0x2C56B9AA, 128}, {'F', 0x863223EB, 132},
             {'F', 0xE00D8E2C, 136}, {'F', 0x465FBA18, 140}, {'B', 0x7280543D, 144}, {'B', 0x0626FA2C, 145}, {'B', 0xB960A308, 146},
             {'B', 0x3E58C7C1, 147}});
}

// u32 f(T* o, XmlCursor* c): vtable 0x00B0FC48; the vector +16 (origin),
// the hashes +36 / +32 (default 0), the top four bits of the u32 +40
// cleared; only when one of the hashes is given: the vector (default the
// origin) and the bits 0x80000000 .. 0x10000000 of +40 (through a stack
// byte). Returns the last hash read's result, else the new +40.
DECOMP(0007F670, XmlElem_Ctor_0007F670) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    call(ctx, Obj_InitVt_B21C98, {o});
    const u32 f = load<u32>(o + 40) & 0x0FFFFFFFu;
    store<u32>(o + 16, load<u32>(kOrigin));
    store<u32>(o, 0x00B0FC48);
    store<u32>(o + 20, load<u32>(kOrigin + 4));
    store<u32>(o + 32, 0);
    const u32 z = load<u32>(kOrigin + 8);
    store<u32>(o + 24, z);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(z));  // f1 left by the copies
    store<u32>(o + 36, 0);
    store<u32>(o + 40, f);
    if (XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x7A30D7E3, o + 36, 0) != 0 &&
        XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x8A93973F, o + 32, 0) != 0)
        return;
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x9D9B30D5, o + 16, kOrigin);
    Stack frame(ctx, 16);
    const u32 tmp = frame.addr();  // uninitialized: each read stores its default
    XBit<u32>(ctx, c, 0x74C4D4CA, o + 40, 0x80000000u, tmp);
    XBit<u32>(ctx, c, 0xF79F59C2, o + 40, 0x40000000u, tmp);
    XBit<u32>(ctx, c, 0x6BAA379F, o + 40, 0x20000000u, tmp);
    ret(ctx, XBit<u32>(ctx, c, 0x099CBC7C, o + 40, 0x10000000u, tmp));
}

// u32 f(T* o, XmlCursor* c): vtable 0x00B0F810; the hashes +12 (default
// 0), +16, +20, +48 .. +60, +80, +112, +116, the u8 bool +140, the
// vectors +32 / +64 / +96 (origin defaults; +96 scaled by the f32 just
// before the function when read), the f32 +84 (50) and the f32s +120 ..
// +132, each setting its "given" byte +136 .. +139. Returns 1 when +132
// was given, else the last read's result.
DECOMP(000780D0, XmlElem_Ctor_000780D0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    call(ctx, Obj_InitVt_B21C98, {o});
    store<u32>(o + 12, 0);
    store<u32>(o + 16, 0);
    store<u32>(o + 20, 0);
    store<u32>(o + 32, load<u32>(kOrigin));
    store<u32>(o + 36, load<u32>(kOrigin + 4));
    store<u32>(o + 48, 0);
    store<u32>(o + 40, load<u32>(kOrigin + 8));
    store<u32>(o + 52, 0);
    store<u32>(o + 56, 0);
    store<u32>(o + 60, 0);
    store<u32>(o, 0x00B0F810);
    store<u32>(o + 64, load<u32>(kOrigin));
    store<u32>(o + 68, load<u32>(kOrigin + 4));
    store<u32>(o + 80, 0);
    store<u32>(o + 72, load<u32>(kOrigin + 8));
    store<u32>(o + 84, 0x42480000);
    CopyVec3(o + 96, kOrigin);
    for (u32 off = 112; off <= 132; off += 4) store<u32>(o + off, 0);
    for (u32 k = 136; k <= 140; ++k) store<u8>(o + k, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x7F247172, o + 12, 0);
    XHash(ctx, c, 0x7A30D7E3, o + 16);
    XHash(ctx, c, 0xAA44DED3, o + 20);
    XBool(ctx, c, 0x8C63F207, o + 140);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x8ACA5893, o + 32, o + 32);
    XHash(ctx, c, 0x0FDC3B3E, o + 48);
    XHash(ctx, c, 0x5C06E6C0, o + 52);
    XHash(ctx, c, 0x4BA42764, o + 56);
    XHash(ctx, c, 0xD97897B4, o + 60);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x5C3DA814, o + 64, o + 64);
    XHash(ctx, c, 0x92741D5C, o + 80);
    XF32(ctx, c, 0x04B70827, o + 84);
    const u32 r = XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x72B640E0, o + 96, o + 96);
    const u32 d112 = load<u32>(o + 112);
    if (r == 0) ScaleVec3(ctx, o + 96, std::bit_cast<f32>(load<u32>(0x000780C8)), 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xEAC031A0, o + 112, d112);
    XHash(ctx, c, 0x9274464E, o + 116);
    static constexpr u32 kGiven[] = {0x43B894DB, 0x26055815, 0x0291E67F, 0x77FB0D02};
    for (u32 k = 0; k < 4; ++k) {
        if (XF32(ctx, c, kGiven[k], o + 120 + 4 * k) != 0) continue;
        store<u8>(o + 136 + k, 1);
        if (k == 3) ret(ctx, 1u);
    }
}

// void f(T* o, XmlCursor* c): vtable 0x00B10D00; the hashes +12 (default
// 0) / +16 / +20, the f32 +24; three rates read through a zeroed local
// (default k[0]) and, when above k[0], stored as periods k[1] / rate
// (the first into +28 and +32, the second +28, the third +32); the f32
// +36 (default k[0], scaled by k[2] when read) and the u8 bool +40
// (default 0). k is the f32[3] just before the function. Returns the
// last read's result.
DECOMP(00097850, XmlElem_Ctor_00097850) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = arg<u32>(ctx, 1);
    constexpr u32 k = 0x00097844;
    call(ctx, Obj_InitVt_B21C98, {o});
    store<u32>(o + 12, 0);
    store<u32>(o, 0x00B10D00);
    for (u32 off = 16; off <= 36; off += 4) store<u32>(o + off, 0);
    store<u8>(o + 40, 0);
    const f32 k0 = std::bit_cast<f32>(load<u32>(k));
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x7F247172, o + 12, 0);
    XHash(ctx, c, 0x7A30D7E3, o + 16);
    XHash(ctx, c, 0x6EBFEF2B, o + 20);
    XF32(ctx, c, 0x65D8FCF3, o + 24);
    Stack frame(ctx, 16);
    const u32 tmp = frame.addr();
    store<u32>(tmp, 0);
    static constexpr u32 kRates[] = {0x690E3A7D, 0x6EC98295, 0x6EC981A7};
    for (u32 i = 0; i < 3; ++i) {
        callf(ctx, xfn::XmlCursor_ReadF32, {c, kRates[i], tmp}, {static_cast<f64>(k0)});
        if (static_cast<u32>(ctx.gpr[3]) != 0) continue;
        const f32 rate = std::bit_cast<f32>(load<u32>(tmp));
        if (!(rate > k0)) continue;
        const f32 v = std::bit_cast<f32>(load<u32>(k + 4)) / rate;
        store<u32>(tmp, std::bit_cast<u32>(v));
        if (i != 2) store<u32>(o + 28, std::bit_cast<u32>(v));
        if (i != 1) store<u32>(o + 32, std::bit_cast<u32>(v));
    }
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x82946CAC, o + 36}, {static_cast<f64>(k0)});
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        const f32 v = std::bit_cast<f32>(load<u32>(o + 36)) * std::bit_cast<f32>(load<u32>(k + 8));
        store<u32>(o + 36, std::bit_cast<u32>(v));
        ctx.fpr[1] = static_cast<f64>(v);
    }
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x25F79944, o + 40, 0);
}
