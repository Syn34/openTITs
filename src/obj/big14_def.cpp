// Game code, large-function sweep part 14: the leaf definitions' shared
// XML base reader.
#include "big14.h"

namespace sly4::obj {

namespace {
inline be_u32& W(u32 base, u32 off) { return gm<be_u32>(base + off); }
inline f64 F(u32 ea) { return LfsW(gm<be_u32>(ea)); }
inline void St(u32 ea, f64 v) { gm<be_u32>(ea) = StfsBits(v); }
inline void CopyW(u32 dst, u32 src) { gm<be_u32>(dst) = gm<be_u32>(src).get(); }
}  // namespace

namespace {
namespace hc40 {
constexpr u32 Damage_Read = 0x000C3910;         // Damage_Read0C3910(a0, c, x, tmpl, o)
constexpr u32 XmlCursor_DocPath = 0x004F8944;
constexpr u32 XmlCursor_NextSiblingAny = 0x004F8D74;
constexpr u32 XmlCursor_FirstChildAny = 0x004F9140;
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;
constexpr u32 XmlCursor_ReadBool = 0x004F986C;  // (c, hash, u8* out, u8 default) -> 0 ok / 1 missing / 2 wrong type
constexpr u32 XmlCursor_ReadF32 = 0x004F9B20;   // (c, hash, f32* out; f32 default) -> status
constexpr u32 XmlCursor_ReadVec2 = 0x004F9C9C;  // (c, hash, f32 out[2], const f32 default[2]) -> status
constexpr u32 XmlCursor_ReadVec3 = 0x004F9E0C;  // (c, hash, f32 out[3], const f32 default[3]) -> status
constexpr u32 XmlCursor_ReadHash = 0x004FA14C;  // (c, hash, u32* out, u32 default) -> status
constexpr u32 Memmove = 0x0071C3F0;
constexpr u32 Math_Cosf = 0x007344E0;
constexpr u32 Def_Construct = 0x0074596C;       // Def_Construct74596C
constexpr u32 Comp_GetF32 = 0x00922918;         // Comp_GetF32_922918(p) -> f1

constexpr u32 kK = 0x000C3FF8u;                 // .text f32 {100, 1, 10, 0.5, 0, -1, pi/180, pi/360, 60, 360}
constexpr u32 kEmpty = 0x00AC6790u;             // ""
constexpr u32 sColObjName = 0x00AC67C4u;        // "colObjName"
constexpr u32 kFlagNames = 0x00AC66CCu;         // u32[13] the hashes of the +336 flag bits

// The +340 flag bits read as booleans: {name hash, bit (IBM order)}; the
// default is the same bit of the defaults' flags.
struct BoolBit {
    u32 hash, bit;
};
constexpr BoolBit kBools[22] = {
    {0xBA6158CDu, 0},  {0xA499B8DAu, 1},  {0x0264927Cu, 2},  {0x4E552307u, 3},  {0x023E5375u, 4},  {0xB1445260u, 5},
    {0xAA5EBEECu, 6},  {0xE0C44379u, 7},  {0x579B202Bu, 8},  {0xE019565Du, 9},  {0x92A78501u, 12}, {0x6BEE8F45u, 14},
    {0x56FD4A4Du, 15}, {0x8CB95A92u, 16}, {0xDCEB5C01u, 17}, {0xAFD54C62u, 18}, {0x002700BAu, 19}, {0x0BC0A396u, 20},
    {0x0F5DE5C0u, 21}, {0x1667C898u, 22}, {0x884ECC2Au, 10}, {0x909A908Du, 11},
};
}  // namespace hc40
}  // namespace

// void DefLeaf_ReadBase_000C4020(a0, XmlCursor* c, x, Def* tmpl, LeafDef* o)
// (name?): Damage_Read0C3910 with the same arguments, then a default
// definition is constructed on the stack (Def_Construct74596C) and
// overlaid with o's current fields (o+0..+123, +128..+136, +144..+156,
// +160..+200, +208..+216, +224..+300, 18 bytes at +304, +324..+336 and
// bits 0..22 of +340), or, from a template whose +20 has bit 0x2000, with
// the template's fields (+384.., the +400 component's colour / f32 /
// scales, +192..+224, ...); a template without that bit only calls
// XmlCursor_DocPath(a0). Empty o->16 / o->20 strings take the defaults' (the
// same pointers). Then o's fields are read from c with those defaults:
// vec3 0x63662D61 (+128), "colObjName" (+144: the attribute or the
// default), the f32s +148..+200 (+176 at least 0), vec3 0xBCEEA49F +208
// and f32s +224 / +228 / +260 (degrees to radians when present), +232,
// +244, +248, +256 (default +196), the vec2 0x0D13FD3F +236 (+240 half
// degrees to radians when present), +264, an angle 0x7DEAD86B (default 60)
// whose cosine goes to +252 when present (-1 outside (0, 360)), +268, +272 / +276
// (squared when above 0), +280, hashes +284..+292, 22 flag bits of +340,
// hashes +296 / +300. Each child node then sets: 0x7C28C95E the 13 bits
// of +336 (named by the table at 0x00AC66CC, only when present);
// 0x1DD0B478 bit 13 of +340 and the f32s +324..+332; 0x03C599D8 the byte
// +321 = 1, the f32s +304..+316 and the bool +320.
void DefLeaf_ReadBase_000C4020(u32 a0, u32 c, u32 x, u32 tmpl, u32 o) {
    using namespace hc40;
    GuestFrame frame(832);
    const u32 sp = frame.addr();   // offsets as the original's frame; the defaults at sp + 416
    call(Damage_Read, a0, c, x, tmpl, o);
    auto kw = [](u32 k) { return gm<be_u32>(kK + 4 * k).get(); };
    gm<be_u32>(sp + 828) = kw(4);   // the f32 0 and -1 kept in the frame
    gm<be_u32>(sp + 824) = kw(5);
    const f64 k0 = F(kK), k1 = F(kK + 4), k4 = F(kK + 16), k5 = F(kK + 20);
    // The callee reads r3..r7 and f1..f13; the floats sit in slots it does not read.
    call(Def_Construct, sp + 416, gm<be_u32>(x + 12), kEmpty, kEmpty, kEmpty, 0x7641A763u,
         k5, k5,                          // f1, f2 (r9 / r10, not read)
         k5, k5, k5, k0, k1, k5, k1,      // f3..f9 (slots 112..160, left 0)
         1u,                              // s172
         k1, k1, k5, k4,                  // f10..f13 (slots 176..200, left 0)
         0x01164B40u, 0u, kw(5), 0x01164AFCu, kw(2), kw(2), kw(3), kw(4), kw(5), kw(5), kw(5),
         0u, 0x7641A763u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);

    // The defaults take o's fields.
    for (u32 k = 0; k < 124; k += 4) CopyW(sp + 416 + k, o + k);
    for (u32 k : {128u, 132u, 136u, 144u, 148u, 152u, 156u}) CopyW(sp + 416 + k, o + k);
    for (u32 k = 160; k <= 200; k += 4) CopyW(sp + 416 + k, o + k);
    for (u32 k = 208; k <= 216; k += 4) CopyW(sp + 416 + k, o + k);
    for (u32 k = 224; k <= 300; k += 4) CopyW(sp + 416 + k, o + k);
    call(Memmove, sp + 720, o + 304, 18);
    for (u32 k = 0; k < 16; k += 4) CopyW(sp + 740 + k, o + 324 + k);
    gm<be_u32>(sp + 756) = (gm<be_u32>(sp + 756) & 0x000001FFu) | (gm<be_u32>(o + 340) & 0xFFFFFE00u);

    if (tmpl == 0) {
    } else if ((gm<be_u32>(tmpl + 20) & 0x2000u) == 0) {
        call(XmlCursor_DocPath, a0);   // not a usable template (its return value unused)
    } else {
        // ... or the template's.
        for (u32 k = 0; k < 12; k += 4) CopyW(sp + 544 + k, tmpl + 384 + k);
        if (gm<be_u32>(tmpl + 400) != 0) {
            const u32 p = gm<be_u32>(tmpl + 400);
            gm<be_u32>(sp + 560) = gm<be_u32>(p + 120) & ~1u;
            gm<be_u32>(sp + 564) = StfsBits(call<f64>(Comp_GetF32, p + 224));
            const u32 q = gm<be_u32>(tmpl + 400);
            CopyW(sp + 568, q + 140);
            CopyW(sp + 572, q + 144);
            gm<be_u32>(sp + 576) = static_cast<u32>(gm<be_u16>(q + 404)) << 16;
            gm<be_u32>(sp + 580) = static_cast<u32>(gm<be_u16>(q + 406)) << 16;
        }
        for (u32 k = 0; k <= 32; k += 4) CopyW(sp + 584 + k, tmpl + 192 + k);
        for (u32 k = 0; k < 12; k += 4) CopyW(sp + 624 + k, tmpl + 240 + k);
        for (u32 k = 0; k < 12; k += 4) CopyW(sp + 640 + k, tmpl + 256 + k);
        for (u32 k = 0; k < 16; k += 4) CopyW(sp + 684 + k, tmpl + 300 + k);
        for (u32 k = 0; k < 12; k += 4) CopyW(sp + 700 + k, tmpl + 316 + k);
        CopyW(sp + 716, tmpl + 332);
        call(Memmove, sp + 720, tmpl + 336, 18);
        for (u32 k = 0; k < 12; k += 4) CopyW(sp + 740 + k, tmpl + 356 + k);
        CopyW(sp + 712, tmpl + 328);
        CopyW(sp + 672, tmpl + 288);
        CopyW(sp + 676, tmpl + 292);
        CopyW(sp + 652, tmpl + 268);
        CopyW(sp + 656, tmpl + 272);
        constexpr u32 kMask = 0xFFC9FE00u;   // bits 0..9, 12, 15..22 (5..9 merged first, the rest after the memmove)
        gm<be_u32>(sp + 756) = (gm<be_u32>(sp + 756) & ~kMask) | (gm<be_u32>(tmpl + 372) & kMask);
        CopyW(sp + 660, tmpl + 276);
        CopyW(sp + 668, tmpl + 284);
        CopyW(sp + 664, tmpl + 280);
    }

    gm<u8>(sp + 772) = 0;
    const f64 k4f = F(sp + 828);   // f31
    const u32 s16 = gm<be_u32>(o + 16);
    if (s16 == 0 || gm<u8>(s16) == 0) gm<be_u32>(o + 16) = gm<be_u32>(sp + 432);
    const u32 s20 = gm<be_u32>(o + 20);
    if (s20 == 0 || gm<u8>(s20) == 0) gm<be_u32>(o + 20) = gm<be_u32>(sp + 436);

    auto f32r = [&](u32 cur, u32 hash, u32 dst, f64 def) { return call<u32>(XmlCursor_ReadF32, cur, hash, dst, def); };
    auto d = [&](u32 off) { return F(sp + off); };
    // f32 *= k (fmuls) at o + off.
    auto scale = [&](u32 off, f64 k) { St(o + off, Muls(F(o + off), k)); };
    const f64 deg = F(kK + 24);

    call(XmlCursor_ReadVec3, c, 0x63662D61u, o + 128, sp + 544);
    u32 attr = call<u32>(XmlCursor_FindAttrByName, c, sColObjName);
    if (attr == 0) attr = gm<be_u32>(sp + 560);
    gm<be_u32>(o + 144) = attr;
    f32r(c, 0x0023FEF4u, o + 148, d(564));
    f32r(c, 0xD9BA76FAu, o + 152, d(568));
    f32r(c, 0x5AF2884Au, o + 156, d(572));
    f32r(c, 0xA1FDE21Du, o + 160, d(576));
    f32r(c, 0x8933D0E0u, o + 164, d(580));
    f32r(c, 0xCEDD379Au, o + 168, d(584));
    f32r(c, 0x43B894DBu, o + 172, d(588));
    f32r(c, 0xFA012435u, o + 176, d(592));
    {
        const u32 v = gm<be_u32>(o + 176);
        gm<be_u32>(o + 176) = F(o + 176) > k4f ? v : gm<be_u32>(sp + 828);   // max(v, 0); NaN gives 0
    }
    f32r(c, 0x824F3B6Fu, o + 180, d(596));
    f32r(c, 0xAD6099F3u, o + 184, d(600));
    f32r(c, 0x825857B0u, o + 188, d(604));
    f32r(c, 0x86D97BF8u, o + 192, d(608));
    f32r(c, 0x6D6DEDACu, o + 196, d(612));
    f32r(c, 0x48110963u, o + 200, d(616));
    if (call<u32>(XmlCursor_ReadVec3, c, 0xBCEEA49Fu, o + 208, sp + 624) == 0) {
        scale(208, deg);
        scale(212, deg);
        scale(216, deg);
    }
    if (f32r(c, 0x0026F042u, o + 224, d(640)) == 0) scale(224, deg);
    if (f32r(c, 0x287EDCD7u, o + 228, d(644)) == 0) scale(228, deg);
    f32r(c, 0xE6233EB6u, o + 232, d(648));
    f32r(c, 0x38D66ECBu, o + 244, d(660));
    f32r(c, 0x6C01B0DCu, o + 248, d(664));
    f32r(c, 0x717526C2u, o + 256, F(o + 196));
    if (f32r(c, 0x56C86E35u, o + 260, F(o + 260)) == 0) scale(260, deg);
    if (call<u32>(XmlCursor_ReadVec2, c, 0x0D13FD3Fu, o + 236, o + 236) == 0) scale(240, F(kK + 28));
    f32r(c, 0xDD16AC83u, o + 264, F(o + 264));
    gm<be_u32>(sp + 768) = 0x42700000u;
    if (f32r(c, 0x7DEAD86Bu, sp + 768, F(kK + 32)) == 0) {
        const f64 a = F(sp + 768);
        if (a <= k4f || a >= F(kK + 36)) {
            CopyW(o + 252, sp + 824);
        } else {
            St(o + 252, call<f64>(Math_Cosf, Muls(a, deg)));
        }
    }
    f32r(c, 0x263F3CA1u, o + 268, d(684));
    f32r(c, 0xF4521F7Fu, o + 272, d(688));
    if (F(o + 272) > k4f) scale(272, F(o + 272));
    f32r(c, 0xA11783B1u, o + 276, d(692));
    if (F(o + 276) > k4f) scale(276, F(o + 276));
    f32r(c, 0xF58DBE1Fu, o + 280, d(696));
    call(XmlCursor_ReadHash, c, 0x2D9FE345u, o + 284, gm<be_u32>(sp + 700));
    call(XmlCursor_ReadHash, c, 0xC8E6EA68u, o + 288, gm<be_u32>(sp + 704));
    call(XmlCursor_ReadHash, c, 0x51D7D9E6u, o + 292, gm<be_u32>(sp + 708));
    for (const BoolBit& b : kBools) {
        const u32 m = 0x80000000u >> b.bit;
        call(XmlCursor_ReadBool, c, b.hash, sp + 772, (gm<be_u32>(sp + 756) & m) != 0 ? 1u : 0u);
        gm<be_u32>(o + 340) = (gm<be_u32>(o + 340) & ~m) | ((gm<u8>(sp + 772) & 1u) != 0 ? m : 0);
    }
    call(XmlCursor_ReadHash, c, 0x64129428u, o + 296, gm<be_u32>(sp + 712));
    call(XmlCursor_ReadHash, c, 0x632DA119u, o + 300, gm<be_u32>(sp + 716));

    for (u32 n = call<u32>(XmlCursor_FirstChildAny, c); n != 0;
         n = call<u32>(XmlCursor_NextSiblingAny, n)) {
        const u32 h = (gm<u8>(n + 24) & 0x80u) != 0 ? gm<be_u32>(n + 20) : 0;
        if (h == 0x7C28C95Eu) {
            for (u32 k = 0; k < 13; ++k) {
                if (call<u32>(XmlCursor_ReadBool, n, gm<be_u32>(kFlagNames + 4 * k), sp + 773, 0) != 0) continue;
                const u32 m = 1u << k;
                gm<be_u32>(o + 336) = gm<u8>(sp + 773) != 0 ? (gm<be_u32>(o + 336) | m) : (gm<be_u32>(o + 336) & ~m);
            }
        } else if (h == 0x1DD0B478u) {
            gm<be_u32>(o + 340) = gm<be_u32>(o + 340) | 0x00040000u;
            f32r(n, 0xC40CA429u, o + 324, d(740));
            f32r(n, 0x17D91276u, o + 328, d(744));
            f32r(n, 0x52EB1B8Fu, o + 332, d(748));
        } else if (h == 0x03C599D8u) {
            gm<u8>(o + 321) = 1;
            f32r(n, 0x77C17F6Fu, o + 304, d(720));
            f32r(n, 0x9D9B30D5u, o + 308, d(724));
            f32r(n, 0x6CA62860u, o + 312, d(728));
            f32r(n, 0x6CA62772u, o + 316, d(732));
            call(XmlCursor_ReadBool, n, 0xB38E5179u, o + 320, gm<u8>(sp + 736));
        }
    }
}

}  // namespace sly4::obj

NATIVE(000C4020, sly4::obj::DefLeaf_ReadBase_000C4020)
