// Game code, medium sweep part 6: small XML elements built on
// Obj_InitVt_B21C98 (+4 refs = 1), constructed and read from a cursor.
#include "mid6.h"

using namespace sly4;
using namespace sly4::mid6;

namespace {

namespace efn6 {
constexpr u32 Mem_Alloc = 0x0054ED38;          // (size, pool, align)
constexpr u32 Obj_InitVt_B21C98 = 0x004BB3B0;  // (o): +4 = 1, vtable 0x00B21C98 (reads r3 only)
}  // namespace efn6

constexpr u32 kWhite = 0x01164CA0;      // u8[4]: the default colour
constexpr u32 kColor4CA8 = 0x01164CA8;  // u8[4]
constexpr u32 kVec4B70 = 0x01164B70;    // f32[3]: a default vector
constexpr u32 kKindTable = 0x00F65BF8;  // u32[33]: the hashes 00085E10 maps to an index
constexpr u32 kOne = 0x3F800000u;

void CopyRgba(u32 dst, u32 src) {
    for (u32 k = 0; k < 4; ++k) store<u8>(dst + k, load<u8>(src + k));
}

}  // namespace

// XmlElem* XmlElem_New_000764F0(a0, XmlCursor* c): 36 bytes, vtable
// 0x00B0F748; hash +12 (when present also hashes +16 / +20, colours +24 /
// +28 and the f32 +32 defaulting to a constant).
DECOMP(000764F0, XmlElem_New_000764F0) {
    const u32 c = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, efn6::Mem_Alloc, {36, 0, 16}));
    if (p != 0) {
        call(ctx, efn6::Obj_InitVt_B21C98, {p});
        store<u32>(p + 12, 0);
        store<u32>(p + 16, 0);
        store<u32>(p + 20, 0);
        store<u32>(p, 0x00B0F748);
        CopyRgba(p + 24, kWhite);
        CopyRgba(p + 28, kColor4CA8);
        const u32 def = load<u32>(0x000764E8);  // f32 just before the function
        store<u32>(p + 32, 0);
        if (XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x8A93973F, p + 12, 0) == 0) {
            XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x40795527, p + 16, 0);
            XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xC4125773, p + 20, 0);
            XRead(ctx, xfn::XmlCursor_ReadColor, c, 0x00210FAA, p + 24, kWhite);
            XRead(ctx, xfn::XmlCursor_ReadColor, c, 2683, p + 28, kColor4CA8);
            callf(ctx, xfn::XmlCursor_ReadF32, {c, 0xB7EA3D94, p + 32}, {std::bit_cast<f64>(LfsBits(def))});
        }
    }
    ret(ctx, p);
}

// XmlElem* XmlElem_New_00085E10(a0, c): 44 bytes, vtable 0x00B100D0;
// seven hashes +12..+36, +40 = the index of the hash attribute 0x0027873A
// in a 33-entry table (unchanged when absent or not listed).
DECOMP(00085E10, XmlElem_New_00085E10) {
    const u32 c = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 p = static_cast<u32>(call(ctx, efn6::Mem_Alloc, {44, 0, 16}));
    if (p != 0) {
        call(ctx, efn6::Obj_InitVt_B21C98, {p});
        store<u32>(p + 12, 0);
        store<u32>(p + 16, 0);
        store<u32>(p + 20, 0);
        store<u32>(p, 0x00B100D0);
        for (u32 off = 24; off <= 40; off += 4) store<u32>(p + off, 0);
        XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x8A93973F, p + 12, 0);
        XHash(ctx, c, 0x7A30D7E3, p + 16);
        XHash(ctx, c, 0x7A98DB59, p + 20);
        XHash(ctx, c, 0xECCE69B4, p + 24);
        XHash(ctx, c, 0xDC6BAA58, p + 28);
        XHash(ctx, c, 0x33D74B44, p + 32);
        XHash(ctx, c, 0x001FE7AA, p + 36);
        const u32 tmp = frame.addr();
        store<u32>(tmp, 0);
        if (XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x0027873A, tmp, 0) == 0) {
            const u32 h = load<u32>(tmp);
            for (u32 i = 0; i < 33; ++i)
                if (load<u32>(kKindTable + 4 * i) == h) {
                    store<u32>(p + 40, i);
                    break;
                }
        }
    }
    ret(ctx, p);
}

// void XmlElem_Ctor_0006F7C8(XmlElem* o, XmlCursor* c): vtable
// 0x00B0F310, +32 = 1; hashes +12 / +16 / +20, then by the kind +20 a
// vec3 +32 (two attribute names), an f32 +48 or a bool +32.
DECOMP(0006F7C8, XmlElem_Ctor_0006F7C8) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    call(ctx, efn6::Obj_InitVt_B21C98, {o});
    store<u32>(o + 12, 0);
    store<u32>(o + 16, 0);
    store<u32>(o, 0x00B0F310);
    store<u32>(o + 20, 0);
    store<u8>(o + 32, 1);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x8A93973F, o + 12, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x7A30D7E3, o + 16, 0);
    XHash(ctx, c, 0x63716B2B, o + 20);
    const u32 kind = load<u32>(o + 20);
    switch (kind) {
    case 0x76A67631: {
        const u32 v = frame.addr();
        for (u32 k = 0; k < 3; ++k) store<u32>(v + 4 * k, load<u32>(kVec4B70 + 4 * k));
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(kVec4B70 + 8)));  // the lfs/stfs pairs' leftover
        u32 r = XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x0001086D, v, kVec4B70);
        if (r != 0) r = XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x6CB8175F, v, kVec4B70);
        if (r == 0) {
            for (u32 k = 0; k < 3; ++k) store<u32>(o + 32 + 4 * k, load<u32>(v + 4 * k));
            ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(v + 8)));
        }
        break;
    }
    case 0x66B98D77:
        XF32Def(ctx, c, 0x03B880B3, o + 48, 0x0006F7C0);  // the f32 just before the function
        break;
    case 0x09AED71D:
        XBool(ctx, c, 0x7A281983, o + 32);
        break;
    case 0xC15C9D78:
        XRead(ctx, xfn::XmlCursor_ReadBool, c, 0xAE8CC010, o + 32, 0);
        break;
    default:
        ctx.gpr[3] = kind;
        break;
    }
}

// u32 XmlElem_Ctor_00075DD4(XmlElem* o, XmlCursor* c): vtable 0x00B0F720
// and defaults; hash +12, bool +44, f32 +32; when +44 is set also f32 +16
// / +20, colours +24 / +28 and two hash attributes mapped to the modes +36
// (0..4) and +40 (0..2). Returns the last read / mode / hash value.
DECOMP(00075DD4, XmlElem_Ctor_00075DD4) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    call(ctx, efn6::Obj_InitVt_B21C98, {o});
    store<u32>(o + 12, 0);
    store<u32>(o, 0x00B0F720);
    store<u32>(o + 16, kOne);
    store<u32>(o + 20, kOne);
    CopyRgba(o + 24, kWhite);
    CopyRgba(o + 28, kWhite);
    store<u32>(o + 32, kOne);
    store<u32>(o + 36, 2);
    store<u32>(o + 40, 0);
    store<u8>(o + 44, 1);
    XHash(ctx, c, 0x89E68B87, o + 12);
    XBool(ctx, c, 0x7A281983, o + 44);
    XF32(ctx, c, 0xB7EA3D94, o + 32);
    const u32 on = load<u8>(o + 44);
    if (on == 0) {
        ctx.gpr[3] = on;
        return;
    }
    XF32(ctx, c, 0xD402B80F, o + 16);
    XF32(ctx, c, 0xFC7D87C8, o + 20);
    XRead(ctx, xfn::XmlCursor_ReadColor, c, 0xF7E02627, o + 24, o + 24);
    XRead(ctx, xfn::XmlCursor_ReadColor, c, 0xDC8F9BE0, o + 28, o + 28);
    const u32 tmp = frame.addr();
    store<u32>(tmp, 0);
    const u32 r1 = XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xDB885954, tmp, 0);
    const u32 h = load<u32>(tmp);
    if (r1 == 0 && h != 0) {
        switch (h) {
        case 0x16914924: store<u32>(o + 36, 4); break;
        case 0x03C5A831: store<u32>(o + 36, 2); break;
        case 0xE589ABCC: store<u32>(o + 36, 3); break;
        case 0x8B54F821: store<u32>(o + 36, 0); break;
        default: break;
        }
    }
    if (XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x7EFAC8CD, tmp, h) != 0) return;
    const u32 v = load<u32>(tmp);
    u32 r = v;
    if (v != 0) {
        switch (v) {
        case 0x658CFC35: r = 1; store<u32>(o + 40, r); break;
        case 0x04B6AE8B: r = 2; store<u32>(o + 40, r); break;
        case 0xAB4F031D: r = 0; store<u32>(o + 40, r); break;
        default: break;
        }
    }
    ctx.gpr[3] = r;
}

// u32 XmlElem_Ctor_000820E0(XmlElem* o, XmlCursor* c): vtable 0x00B0FE50
// and defaults. Without either of the hashes +12 / +16 nothing more is
// read; otherwise hash +20, +24 / +28 from the first of two attribute
// names each, hashes / a bool / three f32 (constant defaults) to +80, two
// bits of +84 and +40 = (hash attribute 0x96997C96 == 0x0023BEA4) when
// present. Quirk: +36 (read above) always ends as 0x86DF6221, returned.
DECOMP(000820E0, XmlElem_Ctor_000820E0) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 K = 0x000820D4;  // f32 {default a, default b} just before the function
    constexpr u32 kDone = 0x86DF6221u;
    Stack frame(ctx, 16);  // +0 / +4 hash temporaries, +8 the bool byte
    const u32 ha = frame.addr(), hb = frame.addr() + 4, bt = frame.addr() + 8;
    const u32 fa = load<u32>(K), fb = load<u32>(K + 4);
    call(ctx, efn6::Obj_InitVt_B21C98, {o});
    const u32 f84 = load<u32>(o + 84);
    store<u32>(o + 12, 0);
    store<u32>(o, 0x00B0FE50);
    for (u32 off : {16u, 20u, 24u, 28u, 36u}) store<u32>(o + off, 0);
    store<u8>(o + 40, 0);
    store<u32>(o + 44, 0);
    store<u32>(o + 48, 0);
    store<u8>(o + 52, 0);
    store<u32>(o + 56, 0);
    store<u32>(o + 60, kOne);
    for (u32 off = 64; off <= 80; off += 4) store<u32>(o + off, 0);
    store<u32>(o + 84, (f84 | 0x80000000u) & ~0x40000000u);
    const auto hash0 = [&](u32 h, u32 out) { return XRead(ctx, xfn::XmlCursor_ReadHash, c, h, out, 0); };
    if (hash0(0x8A93973F, o + 12) == 0 || hash0(0x7A30D7E3, o + 16) == 0) {
        hash0(0xCCEB9AF9, o + 20);
        store<u32>(ha, 0);
        if (hash0(0x001F9B50, ha) == 0 || hash0(0xE8C32B64, ha) == 0) store<u32>(o + 24, load<u32>(ha));
        if (hash0(0x5E1C2DB7, ha) == 0 || hash0(0xD017B523, ha) == 0) store<u32>(o + 28, load<u32>(ha));
        hash0(0x34B3B09B, o + 36);
        hash0(0x044DDD31, o + 44);
        hash0(0xE1209436, o + 48);
        XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x375509AC, o + 52, 0);
        callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x03C5A831, o + 56}, {std::bit_cast<f64>(LfsBits(fa))});
        callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x04B70827, o + 60}, {std::bit_cast<f64>(LfsBits(fb))});
        callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x273C260F, o + 64}, {std::bit_cast<f64>(LfsBits(fa))});
        hash0(0xAF9EFD80, o + 68);
        hash0(0xA4871DCB, o + 72);
        hash0(0xA9D45179, o + 76);
        XHash(ctx, c, 0x7AD841CF, o + 80);
        XBit<u32>(ctx, c, 0xC1B28F57, o + 84, 0x80000000u, bt);
        XBit<u32>(ctx, c, 0xE589ABCC, o + 84, 0x40000000u, bt);
        if (hash0(0x96997C96, hb) == 0) store<u8>(o + 40, load<u32>(hb) == 0x0023BEA4 ? 1 : 0);
    }
    store<u32>(o + 36, kDone);
    ctx.gpr[3] = kDone;
}

// XmlElem_Read_003F0530(a0, XmlCursor* c, a2, Elem* o): hashes +0 / +4 /
// the kind +8; kinds 0xD3002C3D / 0x922D3BD1 (which first reads hash +20)
// read f32 (constant default) / bool / hash / s32 fields +24..+88 (+31 /
// +30 set when +88 / +84 are present); all end with f32 +12 / +16 (+16
// scaled by a constant).
DECOMP(003F0530, XmlElem_Read_003F0530) {
    const u32 c = arg<u32>(ctx, 1), o = arg<u32>(ctx, 3);
    constexpr u32 K = 0x003F0524;  // f32 {default / +12 default, +16 default, +16 scale}
    XHash(ctx, c, 0x048C5D70, o);
    XHash(ctx, c, 0x7A30D7E3, o + 4);
    XHash(ctx, c, 0x87E5B3B6, o + 8);
    const u32 kind = load<u32>(o + 8);
    const auto hash0 = [&](u32 h, u32 out) { return XRead(ctx, xfn::XmlCursor_ReadHash, c, h, out, 0); };
    const auto bool0 = [&](u32 h, u32 out) { return XRead(ctx, xfn::XmlCursor_ReadBool, c, h, out, 0); };
    if (kind == 0xD3002C3D || kind == 0x922D3BD1) {
        if (kind == 0x922D3BD1) hash0(0x922D3BD1, o + 20);
        const u32 d = load<u32>(K);
        const auto f32d = [&](u32 h, u32 out) { callf(ctx, xfn::XmlCursor_ReadF32, {c, h, out}, {std::bit_cast<f64>(LfsBits(d))}); };
        f32d(0xAF9EDCAB, o + 40);
        f32d(0xAF9EDBBD, o + 44);
        f32d(0xF72711F4, o + 48);
        f32d(0xF7271106, o + 52);
        f32d(0x313AA4BF, o + 56);
        f32d(0x313AA3D1, o + 60);
        bool0(0x1F0FC3CA, o + 24);
        f32d(0xBD63705C, o + 64);
        f32d(0x8732C520, o + 68);
        hash0(0x63278722, o + 72);
        hash0(0xD8B20AB6, o + 76);
        bool0(0xD85F5582, o + 25);
        bool0(0x19BBB415, o + 26);
        bool0(0xF520E524, o + 27);
        bool0(0x152D006A, o + 28);
        XRead(ctx, xfn::XmlCursor_ReadS32, c, 0xBA0E969C, o + 32, 0);
        bool0(0x6C345908, o + 29);
        f32d(0x6C95A6E8, o + 36);
        hash0(0x048C5D70, o + 80);
        if (hash0(0x8D14DBDF, o + 88) == 0) store<u8>(o + 31, 1);
        if (hash0(0xDD79D113, o + 84) == 0) store<u8>(o + 30, 1);
    }
    XF32Def(ctx, c, 0x00274ACD, o + 12, K);
    XF32Def(ctx, c, 0xE5F7DE9A, o + 16, K + 4);
    const f32 v = F32(load<u32>(o + 16)) * F32(load<u32>(K + 8));
    store<u32>(o + 16, W32(v));
    ctx.fpr[1] = static_cast<f64>(v);
}

// u32 XmlElem_Ctor_004C30C8(XmlElem* o, XmlCursor* c): +4 = 1, vtable
// 0x00B221A0, flags +60 = 0x80 (+61 bits 7 / 6 cleared); hashes +32 (+61
// bit 6 when present), +12 / +16 / +20 (s32 attributes when absent), +24,
// +28; five bool bits of +60; two vec3 read per component into
// temporaries, +64 / +68 / +72 (+60 bits 2 / 1 / 0 when present), bit 7
// of +61; +36 = the second vector (scaled unless +61 bit 6), +48 = the
// first. Returns +61 (the constants' address when it scaled).
DECOMP(004C30C8, XmlElem_Ctor_004C30C8) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 K = 0x004C30BC;  // f32 {default, scale} just before the function
    Stack frame(ctx, 48);  // +0 / +16 the two vec3, +32 the bool byte
    const u32 va = frame.addr(), vb = frame.addr() + 16, bt = frame.addr() + 32;
    const u32 b60 = load<u8>(o + 60), b61 = load<u8>(o + 61);
    store<u16>(o + 4, 1);
    store<u32>(o + 12, 0);
    store<u32>(o, 0x00B221A0);
    for (u32 off = 16; off <= 32; off += 4) store<u32>(o + off, 0);
    store<u8>(o + 60, static_cast<u8>((b60 | 0x80) & 0x80));
    store<u8>(o + 61, static_cast<u8>(b61 & ~0xC0));
    const auto hash0 = [&](u32 h, u32 out) { return XRead(ctx, xfn::XmlCursor_ReadHash, c, h, out, 0); };
    const auto s320 = [&](u32 h, u32 out) { return XRead(ctx, xfn::XmlCursor_ReadS32, c, h, out, 0); };
    if (hash0(0x87208BC1, o + 32) == 0) store<u8>(o + 61, static_cast<u8>(load<u8>(o + 61) | 0x40));
    if (hash0(0x8A93973F, o + 12) != 0) s320(0x5C8998CD, o + 12);
    if (hash0(0x93116591, o + 16) != 0) s320(0x39F3EE1F, o + 16);
    if (hash0(0x3EF76876, o + 20) != 0) s320(0xABA3F684, o + 20);
    hash0(0x00255BF3, o + 24);
    hash0(0xFB888634, o + 28);
    store<u8>(bt, 0);
    XBit<u8>(ctx, c, 0xE48AEDBD, o + 60, 0x40, bt);
    XBit<u8>(ctx, c, 0xFF40848E, o + 60, 0x20, bt);
    XBit<u8>(ctx, c, 0x96423570, o + 60, 0x10, bt);
    XBit<u8>(ctx, c, 0x2A3FC52F, o + 60, 0x08, bt);
    XBit<u8>(ctx, c, 0x72C27306, o + 60, 0x80, bt);
    const u32 d = load<u32>(K);
    const auto f32d = [&](u32 h, u32 out) {
        callf(ctx, xfn::XmlCursor_ReadF32, {c, h, out}, {std::bit_cast<f64>(LfsBits(d))});
        return static_cast<u32>(ctx.gpr[3]);
    };
    for (u32 k = 0; k < 3; ++k) store<u32>(va + 4 * k, 0);
    f32d(0x002590A4, va);
    f32d(0x002590A5, va + 4);
    f32d(0x002590A6, va + 8);
    for (u32 k = 0; k < 3; ++k) store<u32>(vb + 4 * k, 0);
    f32d(0x00267981, vb);
    f32d(0x00267982, vb + 4);
    f32d(0x00267983, vb + 8);
    if (f32d(0x98F54B26, o + 64) == 0) store<u8>(o + 60, static_cast<u8>(load<u8>(o + 60) | 4));
    if (f32d(0x98F54B27, o + 68) == 0) store<u8>(o + 60, static_cast<u8>(load<u8>(o + 60) | 2));
    if (f32d(0x98F54B28, o + 72) == 0) store<u8>(o + 60, static_cast<u8>(load<u8>(o + 60) | 1));
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x7631EAF5, bt, 0);
    const u32 n61 = (load<u8>(o + 61) & ~0x80u) | ((load<u8>(bt) & 1u) << 7);
    store<u8>(o + 61, static_cast<u8>(n61));
    u32 b[3], a[3];
    for (u32 k = 0; k < 3; ++k) b[k] = load<u32>(vb + 4 * k), a[k] = load<u32>(va + 4 * k);
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 36 + 4 * k, b[k]);
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 48 + 4 * k, a[k]);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(b[2]));
    if ((n61 & 0x40) == 0) {
        const f32 s = F32(load<u32>(K + 4));
        f32 v[3];
        for (u32 k = 0; k < 3; ++k) v[k] = F32(b[k]) * s;
        for (u32 k = 0; k < 3; ++k) store<u32>(o + 36 + 4 * k, W32(v[k]));
        ctx.fpr[1] = static_cast<f64>(v[2]);
        ctx.gpr[3] = K;  // leftover: the constant's address
        return;
    }
    ctx.gpr[3] = n61;
}
