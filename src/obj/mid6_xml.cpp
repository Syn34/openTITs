// Game code, medium sweep part 6: def readers of the 0x000CBE10 family
// (XmlDef_ReadBase(a0, c, def) and then the def's own fields from 240 on,
// each defaulting to its current value).
#include "mid6.h"

using namespace sly4;
using namespace sly4::mid6;

namespace {

constexpr u32 kDegToRad17 = 0x0017EF40;  // f32 pi/180 just before 0017EF48
constexpr u32 kDegToRad19 = 0x00190C20;  // f32 pi/180 just before 00190C28
constexpr u32 kZero1CB = 0x001CB738;     // f32 0 just before 001CB740
constexpr u32 kKindHashes = 0x00F64A10;  // u32[2]: the hashes 001BF2A0 maps to an index
constexpr u32 kDefault1053BA8 = 0x01053BA8;  // f32: the default of 001ED004's +296

// The nine f32 fields shared by 001C0C64 / 001BF2A0 / 001CB740.
void ReadNineF32(PPUContext& ctx, u32 c, u32 o) {
    XF32(ctx, c, 0x7B2F95A0, o + 268);
    XF32(ctx, c, 0xD5F932F0, o + 240);
    XF32(ctx, c, 0xEABE8E36, o + 244);
    XF32(ctx, c, 0x800ACF3A, o + 248);
    XF32(ctx, c, 0xF9217A56, o + 252);
    XF32(ctx, c, 0x7BA17740, o + 256);
    XF32(ctx, c, 0xA9262FCF, o + 260);
    XF32(ctx, c, 0xA88C3581, o + 264);
}

// When the f32 at p was read (r == 0): p = max(p, *zero) (the word kept
// as is unless below; stored either way).
void ClampMin(u32 r, u32 p, u32 zero) {
    if (r != 0) return;
    u32 w = load<u32>(p);
    if (F32(w) < F32(load<u32>(zero))) w = load<u32>(zero);
    store<u32>(p, w);
}

}  // namespace

// u32 XmlDef_Read_001C0C64(a0, XmlCursor* c, Def* o): base, nine f32,
// two bool bits (31 / 30) of +276. Returns the flags word.
DECOMP(001C0C64, XmlDef_Read_001C0C64) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);
    call(ctx, XmlDef_ReadBase, {a0, c, o});
    ReadNineF32(ctx, c, o);
    XF32(ctx, c, 0x08C08CA8, o + 272);
    XBit<u32>(ctx, c, 0xD28465E5, o + 276, 0x80000000u, frame.addr());
    ret(ctx, XBit<u32>(ctx, c, 0xD02B978A, o + 276, 0x40000000u, frame.addr()));
}

// XmlDef_Read_001BF2A0(a0, c, o): base, nine f32, +272 = the index of the
// hash attribute 0xC03F596F in a two-entry table (0 when absent), +276
// f32, bools +280 / +281.
DECOMP(001BF2A0, XmlDef_Read_001BF2A0) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);
    call(ctx, XmlDef_ReadBase, {a0, c, o});
    ReadNineF32(ctx, c, o);
    const u32 tmp = frame.addr();
    store<u32>(tmp, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xC03F596F, tmp, 0);
    const u32 h = load<u32>(tmp);
    u32 idx = 0;
    for (u32 i = 0; i < 2; ++i)
        if (load<u32>(kKindHashes + 4 * i) == h) {
            idx = i;
            break;
        }
    store<u32>(o + 272, idx);
    XF32(ctx, c, 0xA6710C18, o + 276);
    XBool(ctx, c, 0xD28465E5, o + 280);
    XBool(ctx, c, 0xF0169515, o + 281);
}

// XmlDef_Read_0017EF48(a0, c, o): base, two vec3 (+256 given in degrees,
// converted), f32 / hash / bool fields to +316.
DECOMP(0017EF48, XmlDef_Read_0017EF48) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    call(ctx, XmlDef_ReadBase, {a0, c, o});
    XVec3(ctx, c, 0xADDC2CDD, o + 240);
    XVec3(ctx, c, 0x0948D097, o + 256);
    XF32(ctx, c, 0xCEDD379A, o + 272);
    XF32(ctx, c, 0x04B70827, o + 276);
    XF32(ctx, c, 0x001FE6E0, o + 280);
    XF32(ctx, c, 0x94E315E2, o + 284);
    XF32(ctx, c, 0x53CAD6F8, o + 292);
    XF32(ctx, c, 0x7D4D7117, o + 288);
    XHash(ctx, c, 0x6C3F4814, o + 296);
    XF32(ctx, c, 0xD1A77009, o + 300);
    XBool(ctx, c, 0x07248538, o + 304);
    XHash(ctx, c, 0x7DFA87B7, o + 308);
    XHash(ctx, c, 0xD49CCAAE, o + 312);
    XBool(ctx, c, 0xB8A0F299, o + 316);
    ScaleVec3(ctx, o + 256, kDegToRad17);
}

// XmlDef_Read_00190C28(a0, c, o): base, two vec3 (+256 in degrees), f32 /
// bool / hash fields to +320. Quirk: +308 defaults to +304's value.
DECOMP(00190C28, XmlDef_Read_00190C28) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    call(ctx, XmlDef_ReadBase, {a0, c, o});
    XVec3(ctx, c, 0xADDC2CDD, o + 240);
    XVec3(ctx, c, 0x0948D097, o + 256);
    XF32(ctx, c, 0xCEDD379A, o + 272);
    XF32(ctx, c, 0x04B70827, o + 276);
    XF32(ctx, c, 0x9FC855CA, o + 280);
    XF32(ctx, c, 0xB595B151, o + 284);
    XBool(ctx, c, 0x7883C439, o + 288);
    XHash(ctx, c, 0x4578545F, o + 292);
    XHash(ctx, c, 0x00241C53, o + 296);
    XF32(ctx, c, 0x5161B474, o + 300);
    XF32(ctx, c, 0x8FA16092, o + 304);
    XF32Def(ctx, c, 0x7ECB8527, o + 308, o + 304);
    XF32(ctx, c, 0xDBAF7E49, o + 312);
    XF32(ctx, c, 0x00DE534E, o + 316);
    XBool(ctx, c, 0x641217B3, o + 320);
    ScaleVec3(ctx, o + 256, kDegToRad19);
}

// XmlDef_Read_001CB740(a0, c, o): base, nine f32, hash +272, f32 +276 ..
// +300 (+284 / +288 / +292 kept >= 0 when read), s32 +304, bools +308 /
// +309.
DECOMP(001CB740, XmlDef_Read_001CB740) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    call(ctx, XmlDef_ReadBase, {a0, c, o});
    ReadNineF32(ctx, c, o);
    XHash(ctx, c, 0x53FFAFC2, o + 272);
    XF32(ctx, c, 0x09198A77, o + 276);
    XF32(ctx, c, 0x58E70C1B, o + 280);
    ClampMin(XF32(ctx, c, 0x0B109E97, o + 284), o + 284, kZero1CB);
    ClampMin(XF32(ctx, c, 0x0B10795B, o + 288), o + 288, kZero1CB);
    ClampMin(XF32(ctx, c, 0x0B13D227, o + 292), o + 292, kZero1CB);
    XF32(ctx, c, 0x667AB4F5, o + 296);
    XF32(ctx, c, 0xDBA26CDA, o + 300);
    XS32(ctx, c, 0x9F5A4080, o + 304);
    XBool(ctx, c, 0x3EFBF433, o + 308);
    XBool(ctx, c, 0x4128CDCD, o + 309);
}

// XmlDef_Read_001ED004(a0, c, o): base, two vec2, hashes +256..+288, f32
// +292..+304 (+296 defaulting to a global), bools +308..+315.
DECOMP(001ED004, XmlDef_Read_001ED004) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    call(ctx, XmlDef_ReadBase, {a0, c, o});
    XVec2(ctx, c, 0x64231FD8, o + 240);
    XVec2(ctx, c, 0x64231EEA, o + 248);
    XHash(ctx, c, 0x161851F3, o + 256);
    XHash(ctx, c, 0xEFE3F2BE, o + 260);
    XHash(ctx, c, 0x80F96352, o + 264);
    XHash(ctx, c, 0x93116591, o + 268);
    XHash(ctx, c, 0x3F7939FE, o + 272);
    XHash(ctx, c, 0x04623C7C, o + 276);
    XHash(ctx, c, 0xDEC8B85D, o + 280);
    XHash(ctx, c, 0x6350BDB4, o + 288);
    XF32(ctx, c, 0xC6BA9BDE, o + 292);
    XBool(ctx, c, 0x6C2C79EC, o + 308);
    XBool(ctx, c, 0x33DA09EF, o + 309);
    XBool(ctx, c, 0xF9C0C1B5, o + 310);
    XBool(ctx, c, 0x1E85A989, o + 311);
    XBool(ctx, c, 0xD6F80AE3, o + 312);
    XF32Def(ctx, c, 0xBE5301F3, o + 296, kDefault1053BA8);
    XF32(ctx, c, 0x48CDF4C0, o + 300);
    XBool(ctx, c, 0x850F9FE3, o + 313);
    XF32(ctx, c, 0x81B5B597, o + 304);
    XHash(ctx, c, 0xF24070D0, o + 284);
    XBool(ctx, c, 0x4DE83B5A, o + 314);
    XBool(ctx, c, 0x6BDE6A90, o + 315);
}
