// Game code, large-function sweep part 29: the XML def defaults record
// constructor used by the XmlDef factories / readers.
#include "../sly4.h"
#include "big29.h"

using namespace sly4;
using namespace sly4::big29;

namespace {
constexpr u32 PtrVector_Construct = 0x005516E0;  // (vec)
constexpr u32 kDefName = 0x00AD3C14;             // the default name string both factories pass
constexpr u32 kVec0 = 0x01164B40, kVec1 = 0x01164B50, kVec2 = 0x01164B60, kVec3 = 0x01164B70;  // global vectors (ZeroVec, ...)
}  // namespace

// u32 f(Rec* o, a, b, c, const u32 params[8], const Src* src, -, u32 r10;
// f64 f1, f64 f2; stack arguments 9, 10, 18..21) (called by f_00226FA8,
// XmlDef_Read23E590, XmlDef_New23FA28, XmlDef_Read244CF8, XmlDef_New245A30,
// Obj_Construct75CBEC). Fills the record: +12/+16/+20 = a/b/c, the flags
// half +120 = (old & 0xF) | 0x8200, +0 = 2, -1.0f at +36/+40/+72, f1 at
// +76, +88.. five floats (nibble 4..7 of +120) - i, params copied to +132,
// src's 0..476 copied (with holes) to +176.., r10 at +656, f2 at +660,
// +664 / +680 top bits from stack arguments 9 / 21, +668 = arg 18 (name
// length), +676 = arg 20, +8 |= 0x400, six bytes +124.. from arg 10 (1
// each when it is 0). With a length: +672 = String_Alloc(len + 1) and
// String_CopyN(+672, arg 19, len) (its r3). Else r3 = the last byte.
// f1 is left as src +472 (as loaded by its copy).
DECOMP(0075C364, DefRec_Construct_0075C364) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    const u32 prm = arg<u32>(ctx, 4), src = arg<u32>(ctx, 5), r10 = arg<u32>(ctx, 7);
    const f64 x1 = ctx.fpr[1], x2 = ctx.fpr[2];
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 top9 = StackArg(sp, 9), bytes = StackArg(sp, 10), len = StackArg(sp, 18);
    const u32 name = StackArg(sp, 19), a20 = StackArg(sp, 20), top21 = StackArg(sp, 21);
    constexpr u32 kMinus1 = 0xBF800000u;  // -1.0f
    const u32 h = (load<u16>(o + 120) & 0x000Fu) | 0x8200u;
    store<u32>(o + 12, a);
    store<u32>(o + 16, b);
    store<u32>(o + 20, c);
    for (u32 k = 24; k < 32; ++k) store<u8>(o + k, 0);
    store<u32>(o + 32, 0);
    store<u32>(o + 36, kMinus1);
    store<u32>(o + 40, kMinus1);
    store<u8>(o + 44, 0);
    for (u32 k = 48; k <= 68; k += 4) store<u32>(o + k, 0);
    store<u32>(o + 72, kMinus1);
    store<u32>(o + 4, 0);
    store<u32>(o + 8, 0);
    Sf(o + 76, x1);
    for (const u32 k : {80u, 84u, 108u, 112u, 116u}) store<u32>(o + k, 0);
    store<u16>(o + 120, static_cast<u16>(h));
    store<u16>(o + 122, 0);
    store<u32>(o + 0, 2);
    // Five floats: nibble 4..7 of the flags half (0 as just stored; the
    // first from the register, then re-read) minus the index.
    for (u32 i = 0; i < 5; ++i) {
        const u32 nib = ((i == 0 ? h : load<u16>(o + 120)) >> 4) & 0xF;
        store<u32>(o + 88 + 4 * i, Fb(static_cast<f64>(static_cast<s32>(nib - i))));
    }
    for (u32 k = 0; k < 32; k += 16) {
        u32 w[4];
        for (u32 j = 0; j < 4; ++j) w[j] = load<u32>(prm + k + 4 * j);
        for (u32 j = 0; j < 4; ++j) store<u32>(o + 132 + k + 4 * j, w[j]);
    }
    // src +off -> o +176 +off (raw words), in the original's store order.
    const u32 d = o + 176;
    auto cp = [&](u32 off) { store<u32>(d + off, load<u32>(src + off)); };
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
    store<u8>(d + 312, load<u8>(src + 312));
    cp(316);
    for (u32 j = 0; j < 2; ++j)  // the loop count is the 2 still in r12
        for (const u32 k : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u, 44u}) cp(320 + 48 * j + k);
    for (const u32 k : {416u, 420u, 424u, 432u, 436u, 440u, 448u, 464u, 468u}) cp(k);
    const u32 w664 = (load<u32>(o + 664) & 0x7FFFFFFFu) | (top9 << 31);
    const u32 w680 = (load<u32>(o + 680) & 0x7FFFFFFFu) | (top21 << 31);
    store<u32>(o + 656, r10);
    cp(472);
    SetF1(ctx, src + 472);  // f1 as the last lfs left it
    Sf(o + 660, x2);
    store<u32>(o + 664, w664);
    store<u32>(o + 672, 0);
    store<u32>(o + 668, len);
    store<u32>(o + 676, a20);
    store<u32>(o + 680, w680);
    store<u32>(o + 8, load<u32>(o + 8) | 0x400u);
    u32 v = 0;
    for (u32 k = 0; k < 6; ++k) {
        v = bytes == 0 ? 1 : load<u8>(bytes + k);
        store<u8>(o + 124 + k, static_cast<u8>(v));
    }
    if (len == 0) {
        ret(ctx, v);
        return;
    }
    const u32 p = static_cast<u32>(call(ctx, b29fn::String_Alloc, {len + 1}));
    store<u32>(o + 672, p);
    const s32 n = static_cast<s32>(load<u32>(o + 668));
    call(ctx, b29fn::String_CopyN, {p, name, static_cast<u64>(static_cast<s64>(n))});  // its r3
}

// u32 f(Rec* o, a, b, c, params, src, -, r10; f1..f4; stack arguments
// 9..14, 17) (called by XmlDef_Read22DFD8, XmlDef_New22F630). The base
// record: DefRec_Construct_0075C364(o, a, b, c, params, src, arg 14, r10;
// f1..f4; stack arguments 9 (u8), 10, 11..13 (u8), 14, 17 (u8), 18 = 0,
// 19 = the name 0x00AD3C14, 20 = 2, 21 = 0; 8 / 15 / 16 not written). Then
// the defaults +684..+1540 (three PtrVectors +760 / +1140 / +1328, the
// global vectors 0x01164B40.. copied raw, u16 +766 = 64, +1146 = 32, +1334
// = 4) and +8 |= 0x800 (r3). f1 is left as 0x01164B48 (the last lfs).
DECOMP(0075CBEC, DefRec_ConstructFull_0075CBEC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 sp0 = static_cast<u32>(ctx.gpr[1]);
    const u32 a9 = StackArg(sp0, 9) & 0xFF, a10 = StackArg(sp0, 10), a11 = StackArg(sp0, 11) & 0xFF, a12 = StackArg(sp0, 12) & 0xFF;
    const u32 a13 = StackArg(sp0, 13) & 0xFF, a14 = StackArg(sp0, 14), a17 = StackArg(sp0, 17) & 0xFF;
    {
        Stack frame(ctx, 320);
        const u32 sp = frame.addr();
        // Stack arguments 8 / 15 / 16 are not written: the callee sees whatever
        // the frame holds there (it does not read them).
        const u64 r4 = ctx.gpr[4], r5 = ctx.gpr[5], r6 = ctx.gpr[6], r7 = ctx.gpr[7], r8 = ctx.gpr[8], r10 = ctx.gpr[10];
        const u32 s8 = StackArg(sp, 8), s15 = StackArg(sp, 15), s16 = StackArg(sp, 16);
        const f64 f1 = ctx.fpr[1], f2 = ctx.fpr[2], f3 = ctx.fpr[3], f4 = ctx.fpr[4];
        callf(ctx, 0x0075C364, {o, r4, r5, r6, r7, r8, a14, r10, s8, a9, a10, a11, a12, a13, a14, s15, s16, a17, 0, kDefName, 2, 0}, {f1, f2, f3, f4});
    }
    auto W = [&](u32 off, u32 v) { store<u32>(o + off, v); };
    auto B = [&](u32 off, u32 v) { store<u8>(o + off, static_cast<u8>(v)); };
    auto C = [&](u32 off, u32 src) { store<u32>(o + off, load<u32>(src)); };
    for (const u32 k : {692u, 688u, 700u, 696u, 708u, 704u, 716u, 712u}) W(k, 0);
    for (u32 k = 720; k <= 736; k += 4) W(k, 0);
    W(740, 0x3ECCCCCDu), W(744, 0x41C80000u), W(748, 0x3F000000u), W(752, 0x41C80000u), W(756, 0x3F000000u), W(684, 0);
    call(ctx, PtrVector_Construct, {o + 760});
    for (u32 k = 780; k <= 804; k += 4) W(k, 0);
    W(808, 0x41200000u), W(812, 0x41200000u), W(816, 0);
    W(820, 0x3E4CCCCDu), W(824, 0x3E4CCCCDu), W(828, 0x3E4CCCCDu), W(832, 0);
    W(836, 0x40800000u), W(840, 0x40800000u), W(844, 0x40800000u);
    W(848, 0x3F000000u), W(852, 0x3F000000u), W(856, 0x3F000000u), W(864, 0), W(868, 0x3F800000u), W(872, 0x3F800000u);
    store<u16>(o + 766, 64);
    C(928, kVec0), C(932, kVec0 + 4), W(944, 0), C(936, kVec0 + 8), W(948, 0);
    B(860, 0), C(960, kVec0), B(861, 1), B(862, 1), B(863, 1), C(964, kVec0 + 4);
    B(876, 1), W(916, 0), C(968, kVec0 + 8), C(976, kVec0), B(992, 0), C(980, kVec0 + 4), B(880, 0), C(984, kVec0 + 8);
    call(ctx, PtrVector_Construct, {o + 1140});
    C(1008, kVec1), C(1012, kVec1 + 4), C(1016, kVec1 + 8), C(1024, kVec2);
    store<u16>(o + 1146, 32);
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
    call(ctx, PtrVector_Construct, {o + 1328});
    W(1348, 0x447A0000u), W(1364, 0x3F800000u), W(1368, 0x3F333333u), W(1360, 0x3FA66666u), W(1376, 0), W(1380, 0xC0A00000u);
    W(1372, 0xBE99999Au), W(1384, 0);
    store<u16>(o + 1334, 4);
    static constexpr u32 kC[][2] = {
        {1392, 0},           {1396, 0xC0A00000u}, {1400, 0},           {1408, 0xBF800000u}, {1412, 0x3F800000u}, {1424, 0},
        {1428, 0x40800000u}, {1432, 0x3F800000u}, {1436, 0x42C80000u}, {1440, 0x447A0000u}, {1452, 0x41A00000u}, {1456, 0x42480000u},
        {1468, 0xBF800000u}, {1472, 0},           {1476, 0},           {1492, 0},           {1444, 128},         {1496, 0},
        {1448, 0},           {1500, 0xBF800000u}, {1460, 0},           {1464, 0},
    };
    for (const auto& e : kC) W(e[0], e[1]);
    C(1504, kVec0), B(1480, 1), W(1488, 0), C(1508, kVec0 + 4), C(1512, kVec0 + 8);
    SetF1(ctx, kVec0 + 8);
    W(1520, 0), W(1524, 0), B(1528, 1), W(1532, 0), W(1536, 0xBF800000u), B(1540, 0);
    const u32 f = load<u32>(o + 8) | 0x800u;
    W(8, f);
    ctx.gpr[3] = f;
}
