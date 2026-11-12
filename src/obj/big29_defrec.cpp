// Game code, large-function sweep part 29: the XML def defaults record
// constructor used by the XmlDef factories / readers.
#include "big29.h"

namespace sly4::obj {

namespace {
constexpr u32 PtrVector_Construct = 0x005516E0;  // (vec)
constexpr u32 kDefName = 0x00AD3C14;             // the default name string both factories pass
constexpr u32 kVec0 = 0x01164B40, kVec1 = 0x01164B50, kVec2 = 0x01164B60, kVec3 = 0x01164B70;  // global vectors (ZeroVec, ...)
constexpr u32 kMinus1 = 0xBF800000u;  // -1.0f
}  // namespace

// u32 f(Rec* o, a, b, c, const u32 params[8], const Src* src, -, u32 r10;
// f1, f2; stack arguments 9, 10, 18..21) (called by f_00226FA8,
// XmlDef_Read23E590, XmlDef_New23FA28, XmlDef_Read244CF8, XmlDef_New245A30,
// DefRec_ConstructFull_0075CBEC). The four float arguments sit in argument
// slots 6, 8, 15 and 16 (their GPR / stack slots are not read). Fills the
// record: +12/+16/+20 = a/b/c, the flags half +120 = (old & 0xF) | 0x8200,
// +0 = 2, -1.0f at +36/+40/+72, f1 at +76, +88.. five floats (nibble 4..7
// of +120) - i, params copied to +132, src's 0..476 copied (with holes) to
// +176.., r10 at +656, f2 at +660, +664 / +680 top bits from stack
// arguments 9 / 21, +668 = arg 18 (name length), +676 = arg 20, +8 |=
// 0x400, six bytes +124.. from arg 10 (1 each when it is 0). With a length:
// +672 = String_Alloc(len + 1) and String_CopyN(+672, arg 19, len). No
// caller reads r3 or f1 (r3.sh).
void DefRec_Construct_0075C364(DefRec* o, u32 a, u32 b, u32 c, const Quad* prm, const DefRecSrc* src, f32 x1, u32 r10, f32 x2,
                               u32 top9, u32 bytes, u32, u32, u32, u32, f32, f32, u32, u32 len, u32 name, u32 a20, u32 top21) {
    const u32 h = (o->flags_half & 0x000Fu) | 0x8200u;
    o->a = a;
    o->b = b;
    o->c = c;
    for (u8& z : o->z24) z = 0;
    o->w32 = 0u;
    set_bits(o->f36, kMinus1);
    set_bits(o->f40, kMinus1);
    o->b44 = 0;
    for (be_u32& z : o->w48) z = 0u;
    set_bits(o->f72, kMinus1);
    o->w4 = 0u;
    o->flags = 0u;
    o->f76 = x1;
    o->w80 = 0u;
    o->w84 = 0u;
    o->w108 = 0u;
    o->w112 = 0u;
    o->w116 = 0u;
    o->flags_half = static_cast<u16>(h);
    o->h122 = 0;
    o->kind = 2u;
    // Five floats: nibble 4..7 of the flags half (0 as just stored; the
    // first from the register, then re-read) minus the index.
    for (u32 i = 0; i < 5; ++i) {
        const u32 nib = ((i == 0 ? h : static_cast<u32>(o->flags_half)) >> 4) & 0xF;
        o->f88[i] = static_cast<f32>(static_cast<s32>(nib - i));
    }
    for (u32 k = 0; k < 2; ++k) {
        u32 w[4];
        for (u32 j = 0; j < 4; ++j) w[j] = prm[k].w[j];
        for (u32 j = 0; j < 4; ++j) o->params[k].w[j] = w[j];
    }
    // src +off -> o +176 +off (raw words), in the original's store order.
    auto cp = [&](u32 off) { o->copy.w[off / 4] = src->w[off / 4].get(); };
    cp(16);
    for (u32 k = 32; k <= 40; k += 4) cp(k);
    for (u32 k = 48; k <= 80; k += 4) cp(k);
    for (u32 k = 96; k <= 104; k += 4) cp(k);
    for (u32 k = 112; k <= 180; k += 4) cp(k);
    for (u32 k = 192; k <= 200; k += 4) cp(k);
    cp(208), cp(212), cp(0), cp(216);
    for (u32 k = 224; k <= 284; k += 4) cp(k);
    cp(292), cp(288);
    for (u32 k = 296; k <= 308; k += 4) cp(k);
    o->copy.b[312] = src->b[312];
    cp(316);
    for (u32 j = 0; j < 2; ++j)  // the loop count is the 2 still in r12
        for (const u32 k : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u, 44u}) cp(320 + 48 * j + k);
    for (const u32 k : {416u, 420u, 424u, 432u, 436u, 440u, 448u, 464u, 468u}) cp(k);
    const u32 w664 = (o->w664 & 0x7FFFFFFFu) | (top9 << 31);
    const u32 w680 = (o->w680 & 0x7FFFFFFFu) | (top21 << 31);
    o->r10 = r10;
    cp(472);
    o->f660 = x2;
    o->w664 = w664;
    o->name = 0u;
    o->len = len;
    o->a20 = a20;
    o->w680 = w680;
    o->flags = o->flags | 0x400u;
    for (u32 k = 0; k < 6; ++k) o->b124[k] = static_cast<u8>(bytes == 0 ? 1u : gm<u8>(bytes + k));
    if (len == 0) return;
    const u32 p = call<u32>(b29fn::String_Alloc, len + 1);
    o->name = p;
    const s32 n = static_cast<s32>(o->len.get());
    call(b29fn::String_CopyN, p, name, n);
}

// u32 f(Rec* o, a, b, c, params, src, -, r10; f1..f4; stack arguments
// 9..14, 17) (called by XmlDef_Read22DFD8, XmlDef_New22F630). The base
// record: DefRec_Construct_0075C364(o, a, b, c, params, src, r10; f1..f4;
// stack arguments 9 (u8), 10, 11..13 (u8), 14, 17 (u8), 18 = 0, 19 = the
// name 0x00AD3C14, 20 = 2, 21 = 0). Then the defaults +684..+1540 (three
// PtrVectors +760 / +1140 / +1328, the global vectors 0x01164B40.. copied
// raw, u16 +766 = 64, +1146 = 32, +1334 = 4) and +8 |= 0x800. No caller
// reads r3 or f1 (r3.sh).
void DefRec_ConstructFull_0075CBEC(DefRecFull* o, u32 a, u32 b, u32 c, const Quad* prm, const DefRecSrc* src, f32 x1, u32 r10, f32 x2,
                                   u32 a9, u32 a10, u32 a11, u32 a12, u32 a13, u32 a14, f32 x3, f32 x4, u32 a17) {
    DefRec_Construct_0075C364(&o->base, a, b, c, prm, src, x1, r10, x2, a9 & 0xFF, a10, a11 & 0xFF, a12 & 0xFF, a13 & 0xFF, a14, x3, x4,
                              a17 & 0xFF, 0u, kDefName, 2u, 0u);
    auto W = [&](u32 off, u32 v) { o->d.w[(off - 684) / 4] = v; };
    auto H = [&](u32 off, u32 v) { o->d.h[(off - 684) / 2] = static_cast<u16>(v); };
    auto B = [&](u32 off, u32 v) { o->d.b[off - 684] = static_cast<u8>(v); };
    auto C = [&](u32 off, u32 src) { W(off, global<be_u32>(src)); };
    auto V = [&](u32 off) { return &o->d.b[off - 684]; };
    for (const u32 k : {692u, 688u, 700u, 696u, 708u, 704u, 716u, 712u}) W(k, 0);
    for (u32 k = 720; k <= 736; k += 4) W(k, 0);
    W(740, 0x3ECCCCCDu), W(744, 0x41C80000u), W(748, 0x3F000000u), W(752, 0x41C80000u), W(756, 0x3F000000u), W(684, 0);
    call(PtrVector_Construct, V(760));
    for (u32 k = 780; k <= 804; k += 4) W(k, 0);
    W(808, 0x41200000u), W(812, 0x41200000u), W(816, 0);
    W(820, 0x3E4CCCCDu), W(824, 0x3E4CCCCDu), W(828, 0x3E4CCCCDu), W(832, 0);
    W(836, 0x40800000u), W(840, 0x40800000u), W(844, 0x40800000u);
    W(848, 0x3F000000u), W(852, 0x3F000000u), W(856, 0x3F000000u), W(864, 0), W(868, 0x3F800000u), W(872, 0x3F800000u);
    H(766, 64);
    C(928, kVec0), C(932, kVec0 + 4), W(944, 0), C(936, kVec0 + 8), W(948, 0);
    B(860, 0), C(960, kVec0), B(861, 1), B(862, 1), B(863, 1), C(964, kVec0 + 4);
    B(876, 1), W(916, 0), C(968, kVec0 + 8), C(976, kVec0), B(992, 0), C(980, kVec0 + 4), B(880, 0), C(984, kVec0 + 8);
    call(PtrVector_Construct, V(1140));
    C(1008, kVec1), C(1012, kVec1 + 4), C(1016, kVec1 + 8), C(1024, kVec2);
    H(1146, 32);
    C(1028, kVec2 + 4), C(1032, kVec2 + 8), C(1040, kVec3), C(1044, kVec3 + 4), W(1056, 0x3F200000u), C(1048, kVec3 + 8);
    static constexpr u32 kA[][2] = {
        {1060, 0x3F000000u}, {1064, 0x3EB33333u}, {1068, 0x3F200000u}, {1072, 0x3F000000u}, {1076, 0xBF000000u}, {1080, 0},
        {1084, 0x3F800000u}, {1088, 0x3F800000u}, {1092, 0x3F800000u}, {1096, 0x3F000000u}, {1100, 0x3E4CCCCDu}, {1104, 0x41F00000u},
        {1108, 0x443B8000u}, {1112, 0x3ECCCCCDu}, {1120, 0},           {1124, 0x3E126E98u}, {1128, 0xBD178D50u}, {1136, 0x3ECCCCCDu},
        {1160, 0},           {1164, 0x3ECCCCCDu}, {1168, 0},           {1184, 0x3F4CCCCDu}, {1188, 0x3F333333u}, {1192, 0},
    };
    for (const auto& e : kA) W(e[0], e[1]);
    B(1196, 1);
    static constexpr u32 kB[][2] = {
        {1200, 0x3F1C61AAu}, {1204, 0x41F9AAABu}, {1208, 0x447A0000u}, {1212, 0x45ABE000u}, {1216, 0x45EA6000u}, {1220, 0x43FA0000u},
        {1224, 0},           {1228, 0},           {1232, 0},           {1236, 0x3F4CCCCDu}, {1240, 0x3F4CCCCDu}, {1244, 0x3D4CCCCDu},
        {1248, 0x3DCCCCCDu}, {1252, 0x3E99999Au}, {1256, 0x43FA0000u}, {1260, 0x43020000u}, {1264, 0},           {1268, 0},
        {1272, 0},           {1276, 0x41700000u}, {1280, 0x3F800000u}, {1284, 0},           {1288, 0xBF800000u}, {1292, 0},
    };
    for (const auto& e : kB) W(e[0], e[1]);
    B(1296, 0);
    W(1300, 0), W(1304, 0), W(1308, 0), W(1312, 0x455AC000u), W(1316, 0x45CB2000u), W(1320, 0), W(1324, 0x3F800000u);
    call(PtrVector_Construct, V(1328));
    W(1348, 0x447A0000u), W(1364, 0x3F800000u), W(1368, 0x3F333333u), W(1360, 0x3FA66666u), W(1376, 0), W(1380, 0xC0A00000u);
    W(1372, 0xBE99999Au), W(1384, 0);
    H(1334, 4);
    static constexpr u32 kC[][2] = {
        {1392, 0},           {1396, 0xC0A00000u}, {1400, 0},           {1408, 0xBF800000u}, {1412, 0x3F800000u}, {1424, 0},
        {1428, 0x40800000u}, {1432, 0x3F800000u}, {1436, 0x42C80000u}, {1440, 0x447A0000u}, {1452, 0x41A00000u}, {1456, 0x42480000u},
        {1468, 0xBF800000u}, {1472, 0},           {1476, 0},           {1492, 0},           {1444, 128},         {1496, 0},
        {1448, 0},           {1500, 0xBF800000u}, {1460, 0},           {1464, 0},
    };
    for (const auto& e : kC) W(e[0], e[1]);
    C(1504, kVec0), B(1480, 1), W(1488, 0), C(1508, kVec0 + 4), C(1512, kVec0 + 8);
    W(1520, 0), W(1524, 0), B(1528, 1), W(1532, 0), W(1536, 0xBF800000u), B(1540, 0);
    o->base.flags = o->base.flags | 0x800u;
}

}  // namespace sly4::obj

NATIVE(0075C364, sly4::obj::DefRec_Construct_0075C364)
NATIVE(0075CBEC, sly4::obj::DefRec_ConstructFull_0075CBEC)
