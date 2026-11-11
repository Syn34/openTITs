// Game code, medium sweep part 7: component template readers (the
// 0x002879AC family). Each reads a template from an XML element: no "name"
// attribute -> (optionally XmlCursor_DocPath(a0) for the error) 0;
// otherwise SmallAlloc(size) and, when that succeeded, the registered
// template base (vtable 0x00B0DAAC, +4 the name's String_HashNoCase, +8 0,
// +12 the kind, +16 0, {hash, tmpl} appended to the template registry
// 0x01056F04), then its own vtable and defaults; then +16 = the u8 flag
// argument and its fields are read from the cursor. Returns the template.
// Quirk: when SmallAlloc fails, the flag is written to address 16 and the
// fields are read on p = 0.
#include "mid7.h"

using namespace sly4;
using namespace sly4::mid7;

namespace {

namespace tfn {
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;  // (c, const char*) -> const char* or 0
constexpr u32 XmlCursor_DocPath = 0x004F8944;         // (x) (reads r3 only)
constexpr u32 String_HashNoCase = 0x0056A710;         // (const char*) -> u32
constexpr u32 Array_Reserve = 0x00558F3C;             // (Array*, u16 capacity)
}  // namespace tfn

constexpr u32 kStrName = 0x00AD4C0C;       // "name"
constexpr u32 kTmplRegistry = 0x01056F04;  // {+0 u16 cap, +2 u16 count, +4 u16 high-water, +6 u16 flags, +8 {u32 hash, Tmpl*}[]}
constexpr u32 kTmplVtBuild = 0x00B0DAAC;   // the template base vtable

// The shared prologue. Returns false when there is no name attribute (the
// caller returns 0); otherwise p = the template (0 when SmallAlloc failed).
// order: the store order of the base fields: 0 = +8, +16, +4, +12; 1 =
// +16, +8, +4, +12 with the count stored before the registry entry's
// template (002CBB40 / 002D2F3C); 2 = +8, +4, +12, +16; 3 = as 1 with the
// registry's +6 stored before +4 (002BA548); 4 = the base fields as 0 and
// the registry as 1 (002666C0).
bool TmplNew(PPUContext& ctx, u32 a0, u32 c, u32 size, u32 kind, bool docpath, u32& p, int order = 0) {
    const bool alt = order == 1 || order == 3;
    const bool altReg = alt || order == 4;
    p = 0;
    const u32 name = static_cast<u32>(call(ctx, tfn::XmlCursor_FindAttrByName, {c, kStrName}));
    if (name == 0) {
        if (docpath) call(ctx, tfn::XmlCursor_DocPath, {a0});
        return false;
    }
    p = static_cast<u32>(call(ctx, m7fn::SmallAlloc, {size}));
    if (p == 0) return true;
    store<u32>(p, kTmplVtBuild);
    const u32 h = static_cast<u32>(call(ctx, tfn::String_HashNoCase, {name}));
    if (alt) store<u8>(p + 16, 0);  // orders 1 / 3
    store<u32>(p + 8, 0);
    if (order == 0 || order == 4) store<u8>(p + 16, 0);
    store<u32>(p + 4, h);
    store<u32>(p + 12, kind);
    if (order == 2) store<u8>(p + 16, 0);
    const u32 R = kTmplRegistry;
    const u32 cap = load<u16>(R);
    if (load<u16>(R + 2) == cap) call(ctx, tfn::Array_Reserve, {R, (cap << 1) & 0xFFFEu});
    const u32 n = load<u16>(R + 2), data = load<u32>(R + 8);
    store<u32>(data + ((n << 3) & 0x7FFF8u), load<u32>(p + 4));
    const u32 n1 = n + 1, hw = load<u16>(R + 4), fl = load<u16>(R + 6);
    if (!altReg) store<u32>(data + (n << 3) + 4, p);
    store<u16>(R + 2, static_cast<u16>(n1));
    if (altReg) store<u32>(data + (n << 3) + 4, p);
    if (order == 3) store<u16>(R + 6, static_cast<u16>(fl | 1));
    store<u16>(R + 4, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFF) ? hw : n1));
    if (order != 3) store<u16>(R + 6, static_cast<u16>(fl | 1));
    return true;
}

}  // namespace

// Tmpl* TmplRead_002879AC(a0, XmlCursor* c, u8 flag): kind 14, 36 bytes,
// vtable 0x00B1C618: four hashes +20..+32 (default 0).
DECOMP(002879AC, TmplRead_002879AC) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    u32 p;
    if (!TmplNew(ctx, a0, c, 36, 14, true, p)) return ret(ctx, 0);
    if (p != 0) {
        store<u32>(p, 0x00B1C618);
        for (u32 k = 0; k < 4; ++k) store<u32>(p + 20 + 4 * k, 0);
    }
    store<u8>(p + 16, flag);
    XHash(ctx, c, 0x0001293C, p + 20);
    XHash(ctx, c, 0x00276B9E, p + 24);
    XHash(ctx, c, 0xF690C76B, p + 28);
    XHash(ctx, c, 0x00014042, p + 32);
    ret(ctx, p);
}

// Tmpl* TmplRead_002BB9FC(a0, c, flag): kind 31, 36 bytes, vtable
// 0x00B1CF70: f32 +20 (0.5), +24 (1), +28 (0), bool +32 (true).
DECOMP(002BB9FC, TmplRead_002BB9FC) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    u32 p;
    if (!TmplNew(ctx, a0, c, 36, 31, true, p)) return ret(ctx, 0);
    if (p != 0) {
        store<u32>(p, 0x00B1CF70);
        store<u32>(p + 20, 0x3F000000);
        store<u32>(p + 24, 0x3F800000);
        store<u32>(p + 28, 0);
        store<u8>(p + 32, 1);
    }
    store<u8>(p + 16, flag);
    XF32(ctx, c, 0x8FA16092, p + 20);
    XF32(ctx, c, 0x7ECB8527, p + 24);
    XF32(ctx, c, 0x0023FEF4, p + 28);
    XBool(ctx, c, 0xBEF25C9F, p + 32);
    ret(ctx, p);
}

// Tmpl* TmplRead_0028F080(a0, c, flag): kind 22, 32 bytes, vtable
// 0x00B1C720: hash +20, f32 +24 (an angle: scaled by the f32 at
// 0x0028F078 when present), bools +28 / +29.
DECOMP(0028F080, TmplRead_0028F080) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    u32 p;
    if (!TmplNew(ctx, a0, c, 32, 22, true, p)) return ret(ctx, 0);
    if (p != 0) {
        store<u32>(p, 0x00B1C720);
        store<u32>(p + 20, 0);
        store<u32>(p + 24, 0);
        store<u8>(p + 28, 0);
        store<u8>(p + 29, 0);
    }
    store<u8>(p + 16, flag);
    XHash(ctx, c, 0x00255BF3, p + 20);
    const u32 r = XF32(ctx, c, 0x2D3AC4D0, p + 24);
    const u8 b28 = load<u8>(p + 28);
    if (r == 0) {
        const f32 v = F32(load<u32>(p + 24)) * F32(load<u32>(0x0028F078));
        store<u32>(p + 24, W32(v));
        ctx.fpr[1] = v;
    }
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x0453E88E, p + 28, b28);
    XBool(ctx, c, 0x05AB7B55, p + 29);
    ret(ctx, p);
}

// Tmpl* TmplRead_002CBB40(a0, c, flag): kind 38, 44 bytes, vtable
// 0x00B1D0F0: hash +20, f32 +24..+36 (0, 1, 0, 1), bool +40 (true); no
// DocPath without a name.
DECOMP(002CBB40, TmplRead_002CBB40) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    u32 p;
    if (!TmplNew(ctx, a0, c, 44, 38, false, p, 1)) return ret(ctx, 0);
    if (p != 0) {
        store<u32>(p, 0x00B1D0F0);
        store<u32>(p + 20, 0);
        store<u32>(p + 24, 0);
        store<u32>(p + 28, 0x3F800000);
        store<u32>(p + 32, 0);
        store<u32>(p + 36, 0x3F800000);
        store<u8>(p + 40, 1);
    }
    store<u8>(p + 16, flag);
    XHash(ctx, c, 0x9178C4CB, p + 20);
    XF32(ctx, c, 0xDE1DA62B, p + 24);
    XF32(ctx, c, 0x47FC4259, p + 28);
    XF32(ctx, c, 0xDD33E5C1, p + 32);
    XF32(ctx, c, 0x471281EF, p + 36);
    XBool(ctx, c, 0xBEF25C9F, p + 40);
    ret(ctx, p);
}

// Tmpl* TmplRead_002D2F3C(a0, c, flag): kind 47, 44 bytes, vtable
// 0x00B1D258: hash +20, s32 +24 (-1), f32 +28..+36 (1), bools +40 (true)
// / +41; no DocPath without a name.
DECOMP(002D2F3C, TmplRead_002D2F3C) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    u32 p;
    if (!TmplNew(ctx, a0, c, 44, 47, false, p, 1)) return ret(ctx, 0);
    if (p != 0) {
        store<u32>(p, 0x00B1D258);
        store<u32>(p + 20, 0);
        store<u32>(p + 24, 0xFFFFFFFFu);
        for (u32 k = 0; k < 3; ++k) store<u32>(p + 28 + 4 * k, 0x3F800000);
        store<u8>(p + 40, 1);
        store<u8>(p + 41, 0);
    }
    store<u8>(p + 16, flag);
    XHash(ctx, c, 0x9178C4CB, p + 20);
    XS32(ctx, c, 0xBF260CF3, p + 24);
    XF32(ctx, c, 0x3EB49FDD, p + 28);
    XF32(ctx, c, 0x2B6FE96E, p + 32);
    XF32(ctx, c, 0x4FA1D749, p + 36);
    XBool(ctx, c, 0xBEF25C9F, p + 40);
    XBool(ctx, c, 0x173A6338, p + 41);
    ret(ctx, p);
}

// Tmpl* TmplRead_002D08C0(a0, c, flag): kind 45, 48 bytes, vtable
// 0x00B1D1D0: hashes +20 / +24 (default 0), the f32 +28 read twice (two
// attribute names, default the f32 at 0x002D08BC each time), bool +36
// and hashes +40 / +44 (default 0).
DECOMP(002D08C0, TmplRead_002D08C0) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    u32 p;
    if (!TmplNew(ctx, a0, c, 48, 45, true, p)) return ret(ctx, 0);
    if (p != 0) {
        store<u32>(p + 20, 0);
        store<u32>(p, 0x00B1D1D0);
        store<u32>(p + 24, 0);
        store<u32>(p + 28, 0x3F0CCCCD);
        store<u32>(p + 32, 0x3F000000);
        store<u8>(p + 36, 0);
        store<u32>(p + 40, 0);
        store<u32>(p + 44, 0);
    }
    store<u8>(p + 16, flag);
    constexpr u32 K = 0x002D08BC;
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xA424A34F, p + 20, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xC490A0EE, p + 24, 0);
    XF32Def(ctx, c, 0x8FA16092, p + 28, K);
    XF32Def(ctx, c, 0x84323AC5, p + 28, K);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x00218413, p + 36, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x5D1A6CBA, p + 40, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x2BCD00F1, p + 44, 0);
    ret(ctx, p);
}

// Tmpl* TmplRead_0026EFD4(a0, c, flag): kind 0, 160 bytes, vtable
// 0x00B1BFC8: +20 = the word at *0x00F65CA0 + 0x1281D4 (also written
// without a template), a 3x4 identity +32, f32 +100 (100), vec3 +112
// (0.65, 1, 0.5) and +128 (zero), bools +144 (true) / +145.
DECOMP(0026EFD4, TmplRead_0026EFD4) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    constexpr u32 kIdentity = 0x01164BD0, kZeroVec = 0x01164B40;
    u32 p;
    if (!TmplNew(ctx, a0, c, 160, 0, true, p, 2)) return ret(ctx, 0);
    if (p != 0) {
        store<u32>(p, 0x00B1BFC8);
        store<u32>(p + 20, 0);
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) {
                if (r == 3 && k == 2) store<u32>(p + 96, 0);
                store<u32>(p + 32 + 16 * r + 4 * k, load<u32>(kIdentity + 16 * r + 4 * k));
            }
        store<u32>(p + 100, 0x42C80000);
        store<u32>(p + 112, 0x3F266666);
        store<u32>(p + 116, 0x3F800000);
        store<u32>(p + 120, 0x3F000000);
        for (u32 k = 0; k < 3; ++k) store<u32>(p + 128 + 4 * k, load<u32>(kZeroVec + 4 * k));
        store<u8>(p + 144, 1);
        store<u8>(p + 145, 0);
    }
    store<u8>(p + 16, flag);
    store<u32>(p + 20, load<u32>(load<u32>(0x00F65CA0) + 0x001281D4));
    XF32(ctx, c, 0x0023FEF4, p + 100);
    XVec3(ctx, c, 0x0026D801, p + 112);
    XVec3(ctx, c, 0x8ACA5893, p + 128);
    XBool(ctx, c, 0xCADB1721, p + 144);
    XBool(ctx, c, 0x8BB0F765, p + 145);
    ret(ctx, p);
}

// Tmpl* TmplRead_002BA548(a0, c, flag): kind 30, 44 bytes, vtable
// 0x00B1CF18: hashes +20 / +24, f32 +28 (-1; squared when positive after
// the read), colours +32 / +36 (default 0x01164CA0), flags +40: bit 31 =
// bool 0xBEF25C9F (default true), bit 30 / 29 = the colour found, bit 28
// = bool 0xF6DB9816. No DocPath without a name.
DECOMP(002BA548, TmplRead_002BA548) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    constexpr u32 kColor = 0x01164CA0;
    u32 p;
    if (!TmplNew(ctx, a0, c, 44, 30, false, p, 3)) return ret(ctx, 0);
    if (p != 0) {
        store<u32>(p, 0x00B1CF18);
        store<u32>(p + 20, 0);
        store<u32>(p + 24, 0);
        store<u32>(p + 28, 0xBF800000);
        for (u32 k = 0; k < 4; ++k) store<u8>(p + 32 + k, load<u8>(kColor + k));
        for (u32 k = 0; k < 4; ++k) store<u8>(p + 36 + k, load<u8>(kColor + k));
        store<u32>(p + 40, (load<u32>(p + 40) | 0x80000000u) & ~0x70000000u);
    }
    store<u8>(p + 16, flag);
    Stack frame(ctx, 16);
    const u32 tmp = frame.addr();
    store<u8>(tmp, 1);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0xBEF25C9F, tmp, 1);
    store<u32>(p + 40, (load<u32>(p + 40) & 0x7FFFFFFFu) | ((load<u8>(tmp) & 1u) << 31));
    XHash(ctx, c, 0x457D8076, p + 20);
    XHash(ctx, c, 0x4EA1C7E6, p + 24);
    XF32(ctx, c, 0x4852D5AF, p + 28);
    u32 r = XRead(ctx, xfn::XmlCursor_ReadColor, c, 0x7087CB18, p + 32, p + 32);
    store<u32>(p + 40, (load<u32>(p + 40) & ~0x40000000u) | (r == 0 ? 0x40000000u : 0));
    r = XRead(ctx, xfn::XmlCursor_ReadColor, c, 0x83D09DA8, p + 36, p + 36);
    store<u32>(p + 40, (load<u32>(p + 40) & ~0x20000000u) | (r == 0 ? 0x20000000u : 0));
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0xF6DB9816, tmp + 1, 0);
    store<u32>(p + 40, (load<u32>(p + 40) & ~0x10000000u) | ((load<u8>(tmp + 1) & 1u) << 28));
    const f32 v = F32(load<u32>(p + 28));
    ctx.fpr[1] = v;
    if (v > F32(load<u32>(0x002BA544))) {
        const f32 sq = v * v;
        store<u32>(p + 28, W32(sq));
        ctx.fpr[1] = sq;
    }
    ret(ctx, p);
}

// Tmpl* TmplRead_002CC470(a0, c, flag): kind 44, 112 bytes, vtable
// 0x00B1D148: f32 fields +20..+96; +20 / +28 / +32 are scaled by the f32
// at 0x002CC468 after they are read.
DECOMP(002CC470, TmplRead_002CC470) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    u32 p;
    if (!TmplNew(ctx, a0, c, 112, 44, true, p)) return ret(ctx, 0);
    if (p != 0) {
        static constexpr u32 kDef[][2] = {{0, 0x00B1D148}, {20, 0x42700000}, {24, 0x41000000}, {28, 0x41700000}, {32, 0x41F00000}, {36, 0x3F000000},
                                          {40, 0x3E99999A}, {64, 0}, {48, 0x3F4CCCCD}, {52, 0x3FB33333}, {56, 0x3F4CCCCD}, {68, 0},
                                          {72, 0xBEE66666}, {80, 0}, {84, 0x41700000}, {88, 0x411CCCCD}, {92, 0x3ECCCCCD}, {96, 0x3F000000}};
        for (const auto& d : kDef) store<u32>(p + d[0], d[1]);
    }
    store<u8>(p + 16, flag);
    XF32(ctx, c, 0xB5292F41, p + 20);
    XF32(ctx, c, 0x9B3DBC47, p + 28);
    XF32(ctx, c, 0x9B3DBC39, p + 32);
    const f32 k = F32(load<u32>(0x002CC468));
    const f32 a = F32(load<u32>(p + 20)) * k, b = F32(load<u32>(p + 28)) * k, d = F32(load<u32>(p + 32)) * k;
    store<u32>(p + 20, W32(a));
    store<u32>(p + 28, W32(b));
    store<u32>(p + 32, W32(d));
    XF32(ctx, c, 0x53A7F241, p + 24);
    XF32(ctx, c, 0x06CD2139, p + 36);
    XF32(ctx, c, 0x82E789DB, p + 40);
    XF32(ctx, c, 0x977C8AB2, p + 80);
    XF32(ctx, c, 0xB081C3B4, p + 84);
    XF32(ctx, c, 0x01B279B3, p + 88);
    XF32(ctx, c, 0x009DAD78, p + 96);
    ret(ctx, p);
}

// Tmpl* TmplRead_002ACBF8(a0, c, flag): kind 32, 100 bytes, vtable
// 0x00B1CAA0: f32 fields +20..+96 and the hash +92.
DECOMP(002ACBF8, TmplRead_002ACBF8) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    u32 p;
    if (!TmplNew(ctx, a0, c, 100, 32, true, p)) return ret(ctx, 0);
    if (p != 0) {
        static constexpr u32 kDef[][2] = {{0, 0x00B1CAA0}, {20, 0x40800000}, {24, 0x40000000}, {36, 0x3F000000}, {28, 0x3F800000}, {32, 0x3ECCCCCD},
                                          {40, 0x3F19999A}, {72, 0x3F800000}, {60, 0x40400000}, {64, 0x3E4CCCCD}, {68, 0x40400000}, {76, 0x3F800000},
                                          {80, 0x41F00000}, {84, 0}, {88, 0x41F00000}, {92, 0}, {96, 0x3F800000}};
        for (const auto& d : kDef) store<u32>(p + d[0], d[1]);
    }
    store<u8>(p + 16, flag);
    static constexpr u32 kF[][2] = {{0x4738D3DF, 20}, {0xE165E36B, 24}, {0x543E5574, 28}, {0xFFF7B927, 32}, {0xCD0D2AF5, 64},
                                    {0x4801705E, 60}, {0x7F154347, 36}, {0x708A4E60, 68}, {0x827EF485, 40}, {0x735FF3D6, 80},
                                    {0x735FF3D7, 84}, {0x735FF3D8, 88}, {0x8E11EB8A, 72}, {0xC4F41CF6, 76}};
    for (const auto& f : kF) XF32(ctx, c, f[0], p + f[1]);
    XHash(ctx, c, 0x143774CB, p + 92);
    XF32(ctx, c, 0x1E7884C5, p + 96);
    ret(ctx, p);
}

// Tmpl* TmplRead_002B1D18(a0, c, flag): kind 40, 64 bytes, vtable
// 0x00B1CC70 (no other defaults): f32 +20 (default its value), vec3 +32
// (default zero, from the stack), f32 +48 / +52 / +56 (defaults the f32[3]
// at 0x002B1D08).
DECOMP(002B1D18, TmplRead_002B1D18) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    constexpr u32 K = 0x002B1D08;
    u32 p;
    if (!TmplNew(ctx, a0, c, 64, 40, true, p)) return ret(ctx, 0);
    if (p != 0) store<u32>(p, 0x00B1CC70);
    store<u8>(p + 16, flag);
    XF32(ctx, c, 0x04B14672, p + 20);
    Stack frame(ctx, 16);
    const u32 zero = frame.addr();
    for (u32 k = 0; k < 3; ++k) store<u32>(zero + 4 * k, 0);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x7A29AB99, p + 32, zero);
    XF32Def(ctx, c, 0x7D22FFFB, p + 48, K);
    XF32Def(ctx, c, 0x001FE6E0, p + 52, K + 4);
    XF32Def(ctx, c, 0x04B70827, p + 56, K + 8);
    ret(ctx, p);
}

// Tmpl* TmplRead_0027B648(a0, c, flag): kind 4, 112 bytes, vtable
// 0x00B1C0BC: the euler angles 0x898B7952 as the quaternion +32 (default
// identity; constants 0x0027B63C), vec3 +48 / +64, hashes +80 / +84, f32
// +88..+100 (-1) and +104, +108 = (+104 present), bools +109 / +110
// (true). No DocPath without a name. (The original interleaves the
// registry's +4 store with the defaults.)
DECOMP(0027B648, TmplRead_0027B648) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    constexpr u32 kZeroVec = 0x01164B40, kVec4C90 = 0x01164C90;
    u32 p;
    if (!TmplNew(ctx, a0, c, 112, 4, false, p, 3)) return ret(ctx, 0);
    if (p != 0) {
        for (u32 k = 0; k < 4; ++k) store<u32>(p + 32 + 4 * k, load<u32>(kVec4C90 + 4 * k));
        store<u32>(p + 48, load<u32>(kZeroVec));
        store<u32>(p, 0x00B1C0BC);
        store<u32>(p + 52, load<u32>(kZeroVec + 4));
        store<u32>(p + 56, load<u32>(kZeroVec + 8));
        store<u32>(p + 64, load<u32>(kZeroVec));
        store<u32>(p + 80, 0);
        store<u32>(p + 68, load<u32>(kZeroVec + 4));
        store<u32>(p + 84, 0);
        store<u32>(p + 72, load<u32>(kZeroVec + 8));
        for (u32 k = 0; k < 4; ++k) store<u32>(p + 88 + 4 * k, 0xBF800000);
        store<u32>(p + 104, 0);
        store<u8>(p + 108, 0);
        store<u8>(p + 109, 0);
        store<u8>(p + 110, 1);
    }
    store<u8>(p + 16, flag);
    Stack frame(ctx, 16);
    const u32 s = frame.addr();
    for (u32 k = 0; k < 3; ++k) store<u32>(s + 4 * k, load<u32>(kZeroVec + 4 * k));
    ctx.fpr[1] = F32(load<u32>(kZeroVec + 8));
    if (XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x898B7952, s, s) == 0) {
        const EulerQuat q = EulerToQuat(ctx, s, 0x0027B63C);
        store<u32>(p + 44, W32(q.w));
        store<u32>(p + 32, W32(q.x));
        store<u32>(p + 40, W32(q.z));
        store<u32>(p + 36, W32(q.y));
    }
    XVec3(ctx, c, 0x3818B318, p + 48);
    XVec3(ctx, c, 0xE8B802BD, p + 64);
    XHash(ctx, c, 0xA7E293B8, p + 80);
    XHash(ctx, c, 0x26161B29, p + 84);
    XF32(ctx, c, 0xF97D90C8, p + 88);
    XF32(ctx, c, 0xF97D8FDA, p + 92);
    XF32(ctx, c, 0x0F8DA662, p + 96);
    XF32(ctx, c, 0x0F8DA574, p + 100);
    const u32 r = XF32(ctx, c, 0xFDF98B14, p + 104);
    store<u8>(p + 108, r == 0 ? 1 : 0);
    XBool(ctx, c, 0xBC175CCC, p + 109);
    XBool(ctx, c, 0xBEF25C9F, p + 110);
    ret(ctx, p);
}

// Tmpl* TmplRead_002666C0(a0, c, flag): kind 18, 44 bytes, vtable
// 0x00B1BEF0, the PtrVector +20 (capacity 4) of the entries made from the
// 0x01698F69 children (0x00265D60(a0, node, flag) -> 0x00266570(p,
// entry)); bit 31 of +40 = bool 0xB0A96CB9 (default true).
DECOMP(002666C0, TmplRead_002666C0) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u32 flag = static_cast<u8>(ctx.gpr[5]);
    constexpr u32 kEntryRead = 0x00265D60;  // (a0, node, flag) -> entry or 0 (name?)
    constexpr u32 kEntryAdd = 0x00266570;   // (tmpl, entry) (name?)
    u32 p;
    if (!TmplNew(ctx, a0, c, 44, 18, true, p, 4)) return ret(ctx, 0);
    if (p != 0) {
        store<u32>(p, 0x00B1BEF0);
        call(ctx, 0x005516E0, {p + 20});  // PtrVector_Construct
        store<u16>(p + 26, 4);
        call(ctx, 0x00551598, {p + 20, 4, 0, 0});  // PtrVector_Init
        store<u32>(p + 40, load<u32>(p + 40) | 0x80000000u);
    }
    store<u8>(p + 16, static_cast<u8>(flag));
    for (u32 n = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c})); n != 0;
         n = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {n}))) {
        if ((load<u8>(n + 24) & 0x80) == 0 || load<u32>(n + 20) != 0x01698F69u) continue;
        const u32 e = static_cast<u32>(call(ctx, kEntryRead, {a0, n, flag}));
        if (e != 0) call(ctx, kEntryAdd, {p, e});
    }
    Stack frame(ctx, 16);
    const u32 tmp = frame.addr();
    store<u8>(tmp, 1);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0xB0A96CB9, tmp, (load<u32>(p + 40) & 0x80000000u) != 0 ? 1 : 0);
    store<u32>(p + 40, (load<u32>(p + 40) & 0x7FFFFFFFu) | ((load<u8>(tmp) & 1u) << 31));
    ret(ctx, p);
}
