// Game code, medium sweep part 9: the callees part 8 named (a 144-byte
// record push, the 002741F4 def constructor, the shared base of the
// 0020BFE8-family def readers, the 2624-byte object of DefNew_0033322C).
#include "mid9.h"

using namespace sly4;
using namespace sly4::mid9;

// u32 Vec144_Push75FF20(PtrVector* v, const Rec144* r): pushes a 144-byte
// record onto the vector {+0 data, +4 u16 capacity, +8 u16 count, +10 u16
// high-water} (PtrVector_Reserve to twice the capacity when full). The
// record is copied field by field (the words +0..+12, f32 +16..+24,
// +32..+40, +48, the byte +52, f32 +64..+72, +80..+100, +112..+120, +128;
// the padding is left alone). Returns the new count - 1; f1 = the f32 +128.
DECOMP(0075FF20, Vec144_Push75FF20) {
    const u32 v = arg<u32>(ctx, 0), r = arg<u32>(ctx, 1);
    u32 n = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (n == cap) {
        call(ctx, m9fn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
        n = load<u16>(v + 8);
    }
    const u32 d = load<u32>(v) + n * 144;
    for (u32 off : {0u, 4u, 8u, 12u, 16u, 20u, 24u, 32u, 36u, 40u, 48u}) CopyW(d + off, r + off);
    store<u8>(d + 52, load<u8>(r + 52));
    for (u32 off : {64u, 68u, 72u, 80u, 84u, 88u, 92u, 96u, 100u, 112u, 116u, 120u, 128u}) CopyW(d + off, r + off);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(r + 128)));
    const u32 c1 = load<u16>(v + 8) + 1u, hw = load<u16>(v + 10), m = c1 & 0xFFFF;
    store<u16>(v + 8, static_cast<u16>(c1));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(m) ? hw : c1));
    ret(ctx, m - 1);
}

// u32 Def_Construct273E90(Def* d, const Record* r): builds the 128-byte def
// of DefNew_002741F4 from its stack record: vtable 0x00B1C030; +72 r->4;
// the flags word +96 bits 31 / 30 / 29 = the bools r->8 / 9 / 10; f32 +84
// r->48, +80 0, +88 -r->44, +100 r->52; +108..+120 r->60..72; +36 the
// String_HashNoCase of the name r->0, copied (31 chars) to +4; hashes +40
// r->12, +44 r->16 and +48 r->20 (each 0 -> r->12), +52 r->24, f32 +56 /
// +60 / +64 / +68 r->32 / 28 / 36 / 40. When the default effect set
// exists, +104 = the def of the key r->56 in it (0 when missing; the
// lookup's bookkeeping as EffectDefLookup). Then {+36, d} is appended to
// the def map 0x01057544. Returns the map's flags.
DECOMP(00273E90, Def_Construct273E90) {
    const u32 d = arg<u32>(ctx, 0), r = arg<u32>(ctx, 1);
    constexpr u32 kVt = 0x00B1C030, kMap = 0x01057544;
    u32 fl = load<u32>(d + 96) & 0x7FFFFFFFu;
    store<u32>(d + 72, load<u32>(r + 4));
    fl |= (load<u8>(r + 8) & 1u) << 31;
    store<u32>(d + 96, fl);
    store<u32>(d, kVt);
    fl = (fl & ~0x40000000u) | ((load<u8>(r + 9) & 1u) << 30);
    CopyW(d + 84, r + 48);
    store<u32>(d + 96, fl);
    store<u32>(d + 80, 0);
    fl = (fl & ~0x20000000u) | ((load<u8>(r + 10) & 1u) << 29);
    store<u32>(d + 88, load<u32>(r + 44) ^ 0x80000000u);  // fneg
    store<u32>(d + 96, fl);
    store<u32>(d + 108, load<u32>(r + 60));
    CopyW(d + 100, r + 52);
    store<u32>(d + 112, load<u32>(r + 64));
    store<u32>(d + 116, load<u32>(r + 68));
    store<u32>(d + 120, load<u32>(r + 72));
    store<u32>(d + 36, static_cast<u32>(call(ctx, m8fn::String_HashNoCase, {load<u32>(r)})));
    call(ctx, m8fn::String_CopyN, {d + 4, load<u32>(r), 31});
    const u32 h12 = load<u32>(r + 12);
    store<u32>(d + 40, h12);
    const u32 h16 = load<u32>(r + 16);
    store<u32>(d + 44, h16);
    const u32 h20 = load<u32>(r + 20), h24 = load<u32>(r + 24);
    const u32 f28 = load<u32>(r + 28), f32w = load<u32>(r + 32), f36 = load<u32>(r + 36), f40 = load<u32>(r + 40);
    if (h16 == 0) store<u32>(d + 44, h12);
    store<u32>(d + 48, h20);
    if (h20 == 0) store<u32>(d + 48, h12);
    store<u32>(d + 52, h24);
    store<u32>(d + 56, f32w);
    store<u32>(d + 60, f28);
    store<u32>(d + 64, f36);
    store<u32>(d + 68, f40);
    // f1..f4 still hold r->32 / 28 / 36 / 40 (lfs) across Set_Find, which leaves them alone.
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(f32w));
    ctx.fpr[2] = std::bit_cast<f64>(LfsBits(f28));
    ctx.fpr[3] = std::bit_cast<f64>(LfsBits(f36));
    ctx.fpr[4] = std::bit_cast<f64>(LfsBits(f40));
    const u32 s = static_cast<u32>(call(ctx, m9fn::Set_Find, {0}));
    if (s != 0) store<u32>(d + 104, EffectDefLookup(s, load<u32>(r + 56)));
    ret(ctx, RegPushKV2(ctx, kMap, load<u32>(d + 36), d));
}

// u32 Def_ReadBase20BFE8(a0, XmlCursor* c, x, const Def* tmpl, Def* o): the
// shared base of the 0020BFE8-family def readers: Rec_CopyFields_000C387C
// (a0, c, x, tmpl, o); the defaults come from tmpl when it has the flag
// 0x00400000 in +20 (the string +140, bools +172 / +173 / +174), else (a
// template without it gets XmlCursor_DocPath(a0)) the empty string at
// 0x00AD3670 and 1, 0, 0. +76 the attribute 0x00AD3688 (or the default
// string), bools +80 0xBEF25C9F, +81 0x1EF4F9F5, +82 0x2D407057, +83
// 0x778A1C92 (default its value). Returns the last ReadBool result.
DECOMP(0020BFE8, Def_ReadBase20BFE8) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3), o = arg<u32>(ctx, 4);
    call(ctx, m9fn::Rec_CopyFields_000C387C, {a0, c, x, tmpl, o});
    u32 b0 = 1, b1 = 0, b2 = 0, str = 0x00AD3670;
    if (tmpl != 0) {
        if ((load<u32>(tmpl + 20) & 0x00400000u) != 0) {
            b0 = load<u8>(tmpl + 172);
            str = tmpl + 140;
            b1 = load<u8>(tmpl + 173);
            b2 = load<u8>(tmpl + 174);
        } else {
            call(ctx, m8fn::XmlCursor_DocPath, {a0});
        }
    }
    const u32 s = static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, 0x00AD3688}));
    store<u32>(o + 76, s != 0 ? s : str);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0xBEF25C9F, o + 80, b0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x1EF4F9F5, o + 81, b1);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x2D407057, o + 82, b2);
    ret(ctx, XBool(ctx, c, 0x778A1C92, o + 83));
}

// u32 Obj_Construct332E28(Obj* p, x, u32 hash, u8 b6, const f32 vec[3], u8
// b0, u8 b1, u8 b2, u8 b3, u8 b4, u8 b5, u32 hash2) (12 arguments, the last
// four on the stack): the 2624-byte object of DefNew_0033322C:
// Obj_Construct_00769BD8(p, x), vtable 0x00B1EAF8, +2572 hash, +2576 hash2,
// vec3 +2592, +2612 0, the byte +2556 0, the flags word +2608 bits 31..25
// = b0..b6 (bit 0 of each). Returns the flags word; f1 = vec[2].
DECOMP(00332E28, Obj_Construct332E28) {
    const u32 p = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), hash = arg<u32>(ctx, 2), b6 = arg<u32>(ctx, 3), vec = arg<u32>(ctx, 4);
    const u32 b0 = arg<u32>(ctx, 5), b1 = arg<u32>(ctx, 6), b2 = arg<u32>(ctx, 7);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);  // stack arguments 8..11 (low words)
    const u32 b3 = load<u32>(sp + 116), b4 = load<u32>(sp + 124), b5 = load<u32>(sp + 132), hash2 = load<u32>(sp + 140);
    call(ctx, m9fn::Obj_Construct_00769BD8, {p, x});
    u32 fl = load<u32>(p + 2608) & 0x7FFFFFFFu;
    CopyW(p + 2592, vec);
    fl |= (b0 & 1u) << 31;
    store<u32>(p, 0x00B1EAF8);
    fl = (fl & ~0x40000000u) | ((b1 & 1u) << 30);
    fl = (fl & ~0x20000000u) | ((b2 & 1u) << 29);
    store<u32>(p + 2572, hash);
    CopyW(p + 2596, vec + 4);
    fl = (fl & ~0x10000000u) | ((b3 & 1u) << 28);
    store<u32>(p + 2576, hash2);
    CopyW(p + 2600, vec + 8);
    fl = (fl & ~0x08000000u) | ((b4 & 1u) << 27);
    fl = (fl & ~0x04000000u) | ((b5 & 1u) << 26);
    store<u32>(p + 2612, 0);
    fl = (fl & ~0x02000000u) | ((b6 & 1u) << 25);
    store<u8>(p + 2556, 0);
    store<u32>(p + 2608, fl);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(vec + 8)));
    ret(ctx, fl);
}
