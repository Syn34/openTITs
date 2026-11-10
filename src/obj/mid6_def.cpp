// Game code, medium sweep part 6: def factories of the 0x000CBE10 family.
// Each makes a def (SmallAlloc / Mem_Alloc), initializes it through
// Wrapper_0CC4D4 (the spawn-query init) with its name and template, sets
// its vtable and defaults (taken from the template when there is one),
// then reads it from the XML cursor: XmlDef_ReadBase(a0, c, p) and its own
// fields. Quirk: when the allocation fails the reads still run, on p = 0.
#include "mid6.h"

using namespace sly4;
using namespace sly4::mid6;

namespace {

namespace dfn {
constexpr u32 SmallAlloc = 0x0055238C;      // (size) -> p or 0
constexpr u32 Mem_Alloc = 0x0054ED38;       // (size, pool, align)
constexpr u32 SpawnQuery_Init = 0x000CC4D4;  // (q, name, tmpl, a3, m, pos, u8, u8, stack 9th / 11th; f1) (Wrapper_0CC4D4)
}  // namespace dfn

constexpr u32 kIdentity = 0x01164BD0;   // f32[4][4]
constexpr u32 kVec4B30 = 0x01164B30;    // f32[4] passed as the init's position
constexpr u32 kZeroVec = 0x01164B40;    // f32[3] 0
constexpr u32 kNegMax = 0xFF7FFFFFu;    // -FLT_MAX
constexpr u32 kMinusOne = 0xBF800000u;  // -1.0f
constexpr u32 kMax = 0x7F7FFFFFu;       // FLT_MAX

// Wrapper_0CC4D4(p, name, tmpl, 0, identity, kVec4B30, 1, 0, 0, -, 0) with
// f1 = *f1def (the 10th argument's stack slot is not written).
void DefInit(PPUContext& ctx, u32 p, u32 name, u32 tmpl, u32 f1def) {
    callf(ctx, dfn::SpawnQuery_Init, {p, name, tmpl, 0, kIdentity, kVec4B30, 1, 0, 0, 0, 0}, {std::bit_cast<f64>(LfsBits(load<u32>(f1def)))});
}
u32 Small(PPUContext& ctx, u32 size) { return static_cast<u32>(call(ctx, dfn::SmallAlloc, {size})); }
u32 Heap(PPUContext& ctx, u32 size) { return static_cast<u32>(call(ctx, dfn::Mem_Alloc, {size, 0, 16})); }
// The template's word at off, or the word at def without one.
u32 T(u32 tmpl, u32 off, u32 def) { return tmpl != 0 ? load<u32>(tmpl + off) : load<u32>(def); }
// The bool field at p read through the byte tmp (default its value).
void XBoolTmp(PPUContext& ctx, u32 c, u32 hash, u32 p, u32 tmp) {
    XRead(ctx, xfn::XmlCursor_ReadBool, c, hash, tmp, load<u8>(p));
    store<u8>(p, load<u8>(tmp));
}
// The eight f32 fields +268, +240..+264 of the 001AE130 / 001C3040 / 001D4328 defs.
void ReadEightF32(PPUContext& ctx, u32 c, u32 o) {
    XF32(ctx, c, 0x7B2F95A0, o + 268);
    XF32(ctx, c, 0xD5F932F0, o + 240);
    XF32(ctx, c, 0xEABE8E36, o + 244);
    XF32(ctx, c, 0x800ACF3A, o + 248);
    XF32(ctx, c, 0xF9217A56, o + 252);
    XF32(ctx, c, 0x7BA17740, o + 256);
    XF32(ctx, c, 0xA9262FCF, o + 260);
    XF32(ctx, c, 0xA88C3581, o + 264);
}
// +4 |= 4 (read before), +8 |= 0x80000 (read before), +268 / +236 = 0,
// +240..+264 = -FLT_MAX (the 001C3040 / 001D4328 order).
void NegMaxDefaults(u32 p) {
    const u32 f8 = load<u32>(p + 8), f4 = load<u32>(p + 4);
    store<u32>(p + 8, f8 | 0x80000);
    store<u32>(p + 268, 0);
    store<u32>(p + 4, f4 | 4);
    store<u32>(p + 236, 0);
    for (u32 k = 0; k < 7; ++k) store<u32>(p + 240 + 4 * k, kNegMax);
}

}  // namespace

// Def* XmlDef_New_001AA360(a0, XmlCursor* c, name, const Def* tmpl): 272
// bytes, vtable 0x00B152F8: four f32 and a bool from the template.
DECOMP(001AA360, XmlDef_New_001AA360) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 K = 0x001AA354;  // f32 {init f1, default}
    const u32 p = Small(ctx, 272);
    if (p != 0) {
        DefInit(ctx, p, name, tmpl, K);
        const u32 f4 = load<u32>(p + 4);
        store<u32>(p + 236, 0);
        store<u32>(p, 0x00B152F8);
        store<u32>(p + 4, f4 | 4);
        const u32 f12 = load<u32>(p + 12);
        for (u32 k = 0; k < 4; ++k) store<u32>(p + 240 + 4 * k, T(tmpl, 304 + 4 * k, K + 4));
        const u32 b = tmpl != 0 ? load<u8>(tmpl + 320) : 0;
        store<u32>(p + 12, f12 | 1);
        store<u8>(p + 256, b != 0 ? 1 : 0);
    }
    call(ctx, XmlDef_ReadBase, {a0, c, p});
    XF32(ctx, c, 0x4E14723F, p + 240);
    XF32(ctx, c, 0x6A96C7DB, p + 244);
    XF32(ctx, c, 0xEAEDFCE1, p + 248);
    XF32(ctx, c, 0x0A924DA2, p + 252);
    XBool(ctx, c, 0x317F6BB4, p + 256);
    ret(ctx, p);
}

// Def* XmlDef_New_001BDDC0(a0, c, name, tmpl): 272 bytes, vtable
// 0x00B16420: a vec3 and an f32 from the template, +260 = -1, +8 |= 0x200.
DECOMP(001BDDC0, XmlDef_New_001BDDC0) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 K = 0x001BDDB8;  // f32 {init f1, default}
    const u32 p = Small(ctx, 272);
    if (p != 0) {
        DefInit(ctx, p, name, tmpl, K);
        const u32 f8 = load<u32>(p + 8);
        store<u32>(p, 0x00B16420);
        const u32 src = tmpl != 0 ? tmpl + 320 : kZeroVec;
        for (u32 k = 0; k < 3; ++k) store<u32>(p + 240 + 4 * k, load<u32>(src + 4 * k));
        store<u32>(p + 256, T(tmpl, 304, K + 4));
        store<u8>(p + 264, 0);
        store<u32>(p + 260, kMinusOne);
        store<u8>(p + 265, 0);
        store<u8>(p + 266, 0);
        store<u32>(p + 8, f8 | 0x200);
    }
    call(ctx, XmlDef_ReadBase, {a0, c, p});
    XVec3(ctx, c, 0x7DDB2524, p + 240);
    XF32(ctx, c, 0x7DDB32EA, p + 256);
    XF32(ctx, c, 0xE14281CC, p + 260);
    XBool(ctx, c, 0x7D0D427B, p + 264);
    XBool(ctx, c, 0xE019565D, p + 265);
    XBool(ctx, c, 0x846B9DA2, p + 266);
    ret(ctx, p);
}

// The 00177D78 / 001FAB38 defaults: vectors +240 / +256 = 0, +272 = 0,
// +276.. as given; then bit 30 of +132 = bit 31 of the template's word
// at bitoff; +4 |= or4 (read before).
static void VecDefDefaults(u32 p, u32 tmpl, u32 vt, std::initializer_list<std::pair<u32, u32>> rest, u32 bitoff, u32 or4) {
    store<u32>(p, vt);
    for (u32 k = 0; k < 3; ++k) store<u32>(p + 240 + 4 * k, load<u32>(kZeroVec + 4 * k));
    store<u32>(p + 256, load<u32>(kZeroVec));
    store<u32>(p + 260, load<u32>(kZeroVec + 4));
    store<u32>(p + 272, 0);
    store<u32>(p + 264, load<u32>(kZeroVec + 8));
    for (const auto& [off, v] : rest) {
        if (off & 0x80000000u) store<u8>(p + (off & 0xFFFF), static_cast<u8>(v));
        else store<u32>(p + off, v);
    }
    const u32 f4 = load<u32>(p + 4);
    if (tmpl != 0) {
        const u32 bit = (load<u32>(tmpl + bitoff) >> 31) & 1;
        store<u32>(p + 132, (load<u32>(p + 132) & ~0x40000000u) | (bit << 30));
    }
    store<u32>(p + 4, f4 | or4);
}

// Def* XmlDef_New_00177D78(a0, c, name, tmpl): 304 bytes, vtable
// 0x00B14948: two vec3 (+256 in degrees), two f32.
DECOMP(00177D78, XmlDef_New_00177D78) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 K = 0x00177D70;  // f32 {pi/180, init f1}
    const u32 p = Small(ctx, 304);
    if (p != 0) {
        DefInit(ctx, p, name, tmpl, K + 4);
        VecDefDefaults(p, tmpl, 0x00B14948, {{276, 0}, {280, kMinusOne}, {284, kMax}}, 256, 0x800);
    }
    call(ctx, XmlDef_ReadBase, {a0, c, p});
    XVec3(ctx, c, 0xADDC2CDD, p + 240);
    XVec3(ctx, c, 0x0948D097, p + 256);
    XF32(ctx, c, 0x0023FEF4, p + 280);
    XF32(ctx, c, 0xCEDD379A, p + 284);
    ScaleVec3(ctx, p + 256, K);
    ret(ctx, p);
}

// Def* XmlDef_New_001FAB38(a0, c, name, tmpl): 288 bytes (Mem_Alloc),
// vtable 0x00B18B30: two vec3 (+256 in degrees), two f32.
DECOMP(001FAB38, XmlDef_New_001FAB38) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 K = 0x001FAB30;  // f32 {pi/180, init f1}
    const u32 p = Heap(ctx, 288);
    if (p != 0) {
        DefInit(ctx, p, name, tmpl, K + 4);
        VecDefDefaults(p, tmpl, 0x00B18B30, {{276, kMinusOne}, {280, kMax}, {0x80000000u | 284, 0}}, 300, 0x20);
    }
    call(ctx, XmlDef_ReadBase, {a0, c, p});
    XVec3(ctx, c, 0xADDC2CDD, p + 240);
    XVec3(ctx, c, 0x0948D097, p + 256);
    XF32(ctx, c, 0x0023FEF4, p + 276);
    XF32(ctx, c, 0xCEDD379A, p + 280);
    ScaleVec3(ctx, p + 256, K);
    ret(ctx, p);
}

// Def* XmlDef_New_002048A0(a0, c, name, tmpl): 288 bytes, vtable
// 0x00B19178: two vec3 (+256 in degrees), three bools; the template only
// goes to the init.
DECOMP(002048A0, XmlDef_New_002048A0) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 K = 0x00204894;  // f32 {pi/180, init f1}
    Stack frame(ctx, 16);
    const u32 p = Small(ctx, 288);
    if (p != 0) {
        DefInit(ctx, p, name, tmpl, K + 4);
        store<u32>(p, 0x00B19178);
        for (u32 k = 0; k < 3; ++k) store<u32>(p + 240 + 4 * k, load<u32>(kZeroVec + 4 * k));
        store<u32>(p + 256, load<u32>(kZeroVec));
        store<u32>(p + 260, load<u32>(kZeroVec + 4));
        const u32 f4 = load<u32>(p + 4);
        store<u32>(p + 272, 0);
        store<u32>(p + 264, load<u32>(kZeroVec + 8));
        store<u32>(p + 276, 0);
        store<u8>(p + 280, 0);
        store<u8>(p + 281, 0);
        store<u8>(p + 282, 0);
        store<u32>(p + 4, f4 | 0x1000);
    }
    call(ctx, XmlDef_ReadBase, {a0, c, p});
    XVec3(ctx, c, 0xADDC2CDD, p + 240);
    XVec3(ctx, c, 0x0948D097, p + 256);
    ScaleVec3(ctx, p + 256, K);
    XBoolTmp(ctx, c, 0x926425E9, p + 280, frame.addr());
    XBoolTmp(ctx, c, 0x7F6CE437, p + 281, frame.addr());
    XBoolTmp(ctx, c, 0x918F1125, p + 282, frame.addr());
    ret(ctx, p);
}

// Def* XmlDef_New_001AE130(a0, c, name, tmpl): 272 bytes (Mem_Alloc),
// vtable 0x00B159A0: eight f32 (-FLT_MAX / 0 defaults).
DECOMP(001AE130, XmlDef_New_001AE130) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    const u32 p = Heap(ctx, 272);
    if (p != 0) {
        DefInit(ctx, p, name, tmpl, 0x001AE128);
        const u32 f4 = load<u32>(p + 4), f8 = load<u32>(p + 8);
        store<u32>(p + 268, 0);
        store<u32>(p + 240, kNegMax);
        store<u32>(p, 0x00B159A0);
        store<u32>(p + 8, f8 | 0x80000);
        store<u32>(p + 236, 0);
        store<u32>(p + 244, kNegMax);
        store<u32>(p + 4, f4 | 4);
        for (u32 k = 2; k < 7; ++k) store<u32>(p + 240 + 4 * k, kNegMax);
    }
    call(ctx, XmlDef_ReadBase, {a0, c, p});
    ReadEightF32(ctx, c, p);
    ret(ctx, p);
}

// Def* XmlDef_New_001C3040(a0, c, name, tmpl): 288 bytes, vtable
// 0x00B16B30: nine f32 (+272 from the template's +440).
DECOMP(001C3040, XmlDef_New_001C3040) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 K = 0x001C3034;  // f32 {init f1, default}
    const u32 p = Small(ctx, 288);
    if (p != 0) {
        DefInit(ctx, p, name, tmpl, K);
        NegMaxDefaults(p);
        store<u32>(p, 0x00B16B30);
        const u32 f8 = load<u32>(p + 8);
        store<u32>(p + 272, T(tmpl, 440, K + 4));
        store<u32>(p + 8, f8 | 0x1000000);
    }
    call(ctx, XmlDef_ReadBase, {a0, c, p});
    ReadEightF32(ctx, c, p);
    XF32(ctx, c, 0x2DF33CE3, p + 272);
    ret(ctx, p);
}

// Def* XmlDef_New_001D4328(a0, c, name, tmpl): 288 bytes, vtable
// 0x00B17710: eight f32, a hash and an f32 (template +400 / +404).
DECOMP(001D4328, XmlDef_New_001D4328) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 K = 0x001D4320;  // f32 {init f1, default}
    const u32 p = Small(ctx, 288);
    if (p != 0) {
        DefInit(ctx, p, name, tmpl, K);
        NegMaxDefaults(p);
        const u32 f8 = load<u32>(p + 8);
        store<u32>(p, 0x00B17710);
        store<u32>(p + 272, tmpl != 0 ? load<u32>(tmpl + 400) : 0);
        store<u32>(p + 276, T(tmpl, 404, K + 4));
        store<u32>(p + 8, f8 | 0x4000000);
    }
    call(ctx, XmlDef_ReadBase, {a0, c, p});
    ReadEightF32(ctx, c, p);
    XHash(ctx, c, 0x53FFAFC2, p + 272);
    XF32(ctx, c, 0x667AB4F5, p + 276);
    ret(ctx, p);
}

// Def* XmlDef_New_001B2718(a0, c, name, tmpl): 272 bytes, vtable
// 0x00B15E48: five f32; +240..+252 kept >= the minimum, then +240 <= +244.
DECOMP(001B2718, XmlDef_New_001B2718) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 K = 0x001B2708;  // f32 {minimum / +256 default, init f1, default, +244 default}
    const u32 p = Small(ctx, 272);
    if (p != 0) {
        DefInit(ctx, p, name, tmpl, K + 4);
        const u32 f4 = load<u32>(p + 4);
        store<u32>(p + 236, 0);
        store<u32>(p + 4, f4 | 4);
        store<u32>(p, 0x00B15E48);
        const u32 f8 = load<u32>(p + 8);
        store<u32>(p + 240, T(tmpl, 316, K + 8));
        store<u32>(p + 244, T(tmpl, 320, K + 12));
        store<u32>(p + 248, T(tmpl, 324, K + 8));
        store<u32>(p + 252, T(tmpl, 328, K + 8));
        store<u32>(p + 256, T(tmpl, 332, K));
        store<u32>(p + 8, f8 | 0x10000);
    }
    call(ctx, XmlDef_ReadBase, {a0, c, p});
    XF32(ctx, c, 0xCEA8E637, p + 240);
    XF32(ctx, c, 0x06CD2139, p + 244);
    XF32(ctx, c, 0x0DE78502, p + 248);
    XF32(ctx, c, 0x0B0BD5E6, p + 252);
    XF32(ctx, c, 0x285C1380, p + 256);
    const u32 mn = load<u32>(K);
    u32 w[4];
    for (u32 k = 0; k < 4; ++k) w[k] = load<u32>(p + 240 + 4 * k);
    for (u32 k = 0; k < 4; ++k) {
        if (F32(w[k]) < F32(mn)) w[k] = mn;
        store<u32>(p + 240 + 4 * k, w[k]);
    }
    if (F32(w[0]) > F32(w[1])) store<u32>(p + 240, w[1]);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(w[3]));
    ret(ctx, p);
}

// The 001B0398 / 001AB7A0 flag bits: +off bit 31 = the template's bit at
// m31 (1 without a template; the word stored once more with a template),
// bit 30 = its bit at m30 (0 without).
static void TmplBits(u32 p, u32 off, u32 w, u32 tmpl, u32 toff, u32 m31, u32 m30) {
    w = (w & 0x7FFFFFFFu) | ((tmpl != 0 ? (load<u32>(tmpl + toff) & m31) != 0 : 1u) << 31);
    u32 b30 = 0;
    if (tmpl != 0) {
        store<u32>(p + off, w);
        b30 = (load<u32>(tmpl + toff) & m30) != 0;
    }
    store<u32>(p + off, (w & ~0x40000000u) | (b30 << 30));
}

// When the f32 at q was read (r == 0), it is scaled by the f32 at k.
static void ScaleIfRead(u32 r, u32 q, u32 k) {
    if (r == 0) store<u32>(q, W32(F32(load<u32>(q)) * F32(load<u32>(k))));
}

// Def* XmlDef_New_001B0398(a0, c, name, tmpl): 256 bytes, vtable
// 0x00B15C08: two scaled f32, two flag bits of +252, an f32.
DECOMP(001B0398, XmlDef_New_001B0398) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 K = 0x001B0380;  // f32 {scale, init f1, defaults +240 / +244 / +248}
    Stack frame(ctx, 16);
    const u32 p = Small(ctx, 256);
    if (p != 0) {
        DefInit(ctx, p, name, tmpl, K + 4);
        const u32 f4 = load<u32>(p + 4);
        store<u32>(p + 236, 0);
        const u32 w = load<u32>(p + 252);
        store<u32>(p + 4, f4 | 4);
        store<u32>(p, 0x00B15C08);
        const u32 f8 = load<u32>(p + 8);
        store<u32>(p + 240, T(tmpl, 348, K + 8));
        store<u32>(p + 244, T(tmpl, 352, K + 12));
        store<u32>(p + 248, T(tmpl, 360, K + 16));
        TmplBits(p, 252, w, tmpl, 372, 0x00800000u, 0x00400000u);
        store<u32>(p + 8, f8 | 0x40000);
    }
    call(ctx, XmlDef_ReadBase, {a0, c, p});
    ScaleIfRead(XF32(ctx, c, 0x26903F9C, p + 240), p + 240, K);
    ScaleIfRead(XF32(ctx, c, 0xC3017FF2, p + 244), p + 244, K);
    XBit<u32>(ctx, c, 0x4D97C801, p + 252, 0x80000000u, frame.addr());
    XBit<u32>(ctx, c, 0x3AF62E9E, p + 252, 0x40000000u, frame.addr());
    XF32(ctx, c, 0xAA8C2237, p + 248);
    ret(ctx, p);
}

// Def* XmlDef_New_001AB7A0(a0, c, name, tmpl): 272 bytes, vtable
// 0x00B15538: five f32 and two flag bits of +260 from the template.
DECOMP(001AB7A0, XmlDef_New_001AB7A0) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    constexpr u32 K = 0x001AB794;  // f32 {init f1, default}
    Stack frame(ctx, 16);
    const u32 p = Small(ctx, 272);
    if (p != 0) {
        DefInit(ctx, p, name, tmpl, K);
        const u32 f4 = load<u32>(p + 4);
        store<u32>(p + 236, 0);
        store<u32>(p, 0x00B15538);
        store<u32>(p + 4, f4 | 4);
        const u32 w = load<u32>(p + 260), f12 = load<u32>(p + 12);
        for (u32 k = 0; k < 5; ++k) store<u32>(p + 240 + 4 * k, T(tmpl, 304 + 4 * k, K + 4));
        TmplBits(p, 260, w, tmpl, 328, 0x80000000u, 0x40000000u);
        store<u32>(p + 12, f12 | 1);
    }
    call(ctx, XmlDef_ReadBase, {a0, c, p});
    XF32(ctx, c, 0x8722570B, p + 240);
    XF32(ctx, c, 0x1E6104E5, p + 244);
    XF32(ctx, c, 0x08B6A993, p + 248);
    XF32(ctx, c, 0x14EE18CD, p + 252);
    XF32(ctx, c, 0x4E14723F, p + 256);
    XBit<u32>(ctx, c, 0x609A61B6, p + 260, 0x80000000u, frame.addr());
    XBit<u32>(ctx, c, 0x994327AD, p + 260, 0x40000000u, frame.addr());
    ret(ctx, p);
}
