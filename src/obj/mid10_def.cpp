// Game code, medium sweep part 10: the two leaf def constructors the
// DamageColDef / Owner_New families build their records with.
#include "mid10.h"

using namespace sly4;
using namespace sly4::mid10;

// void Def_Construct74FD80(Def* o, a, b, c, r7, r8 (unused), u32 w140,
//     const f32 v[3], f32 f76, f32 f136, f32 f160..f200 (f3..f13),
//     stack: f32 s204, s212, s220, s228, flags s236, s244, s252, s260, s268,
//     const u8* bytes s276, flags s300, s308, s316)
// The shared head (DefHeadInit), then +124 0, f32 +136, +140 w140, +144 v,
// f32 +160..+200, the four stack floats at +204..+216, the eight flags into
// bits 0..7 of +220 (bits 8 / 9 cleared, the rest kept), +0 |= 4, and the
// six bytes +128.. from bytes (1 each when it is null). Leaves f1 = s228.
DECOMP(0074FD80, Def_Construct74FD80) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 7);
    DefHeadInit(o, arg<u32>(ctx, 1), arg<u32>(ctx, 2), arg<u32>(ctx, 3), farg(ctx, 0));
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    u32 fl = load<u32>(o + 220) & 0x7FFFFFFFu;
    store<u32>(o + 124, 0);
    StoreF(o + 136, farg(ctx, 1));
    store<u32>(o + 140, arg<u32>(ctx, 6));
    const u32 w0 = load<u32>(o);
    fl = PutBit(fl, 0, SArg(ctx, 236));
    CopyW(o + 144, v);
    fl = PutBit(fl, 1, SArg(ctx, 244));
    fl = PutBit(fl, 2, SArg(ctx, 252));
    CopyW(o + 148, v + 4);
    fl = PutBit(fl, 3, SArg(ctx, 260));
    CopyW(o + 152, v + 8);
    for (u32 k = 0; k < 11; ++k) StoreF(o + 160 + 4 * k, farg(ctx, 2 + k));
    fl = PutBit(fl, 4, SArg(ctx, 268));
    fl = PutBit(fl, 5, SArg(ctx, 300));
    fl = PutBit(fl, 6, SArg(ctx, 308));
    fl = PutBit(fl, 7, SArg(ctx, 316));
    fl = PutBit(fl, 8, 0);
    fl = PutBit(fl, 9, 0);
    for (u32 k = 0; k < 4; ++k) CopyW(o + 204 + 4 * k, sp + 204 + 8 * k);
    store<u32>(o + 220, fl);
    store<u32>(o, w0 | 4u);
    const u32 bytes = SArg(ctx, 276);
    for (u32 i = 0; i < 6; ++i) store<u8>(o + 128 + i, bytes == 0 ? 1 : load<u8>(bytes + i));
    ctx.fpr[1] = LoadF(sp + 228);
    ctx.fpr[2] = LoadF(v + 8);
}

// void Def_Construct74596C(Def* o, a, b, c, u32 w144, f32 f148..f164
//     (f1..f5), f32 f76 (f6), f32 f168..f176 (f7..f9), f32 f192..f200
//     (f10..f12), f32 f224 (f13), stack: flags s156, s164, s172, const f32
//     v208[3] s212, f32 s228, const f32 v236[2] s236, f32 s244..s292, words
//     s300 / s308 / s316 / s324, flags s332..s364, s396, s404)
// The shared head (DefHeadInit), +128 the vec3 at 0x01164B40, the float
// args, constants (+188 1.0, +300 0xA77EB5E9, +304 1.0, +312 0.1, +316 10,
// +324 0.5, +328 1.0, +332 30), twelve flags into bits 0..11 of +340 (bits
// 12..14 / 17..21 cleared, 15 / 16 / 22 set, 23..31 kept), +0 |= 0x2000,
// then +304..+320 zeroed (over the 1.0 / 0.1 / 10 just stored). Leaves f1 =
// v236[1], f6 = s292.
DECOMP(0074596C, Def_Construct74596C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    DefHeadInit(o, arg<u32>(ctx, 1), arg<u32>(ctx, 2), arg<u32>(ctx, 3), farg(ctx, 5));
    const u32 v208 = SArg(ctx, 212), v236 = SArg(ctx, 236);
    constexpr u32 kVec4B40 = 0x01164B40;
    CopyW(o + 128, kVec4B40);
    CopyW(o + 132, kVec4B40 + 4);
    StoreF(o + 148, farg(ctx, 0));
    CopyW(o + 136, kVec4B40 + 8);
    for (u32 k = 0; k < 4; ++k) StoreF(o + 152 + 4 * k, farg(ctx, 1 + k));
    const u32 w0 = load<u32>(o);
    u32 fl = load<u32>(o + 340) & 0x7FFFFFFFu;
    for (u32 k = 0; k < 3; ++k) StoreF(o + 168 + 4 * k, farg(ctx, 6 + k));
    store<u32>(o + 180, 0);
    store<u32>(o + 184, 0);
    store<u32>(o + 188, 0x3F800000u);
    for (u32 k = 0; k < 3; ++k) StoreF(o + 192 + 4 * k, farg(ctx, 9 + k));
    CopyW(o + 208, v208);
    store<u32>(o + 144, arg<u32>(ctx, 4));
    CopyW(o + 212, v208 + 4);
    CopyW(o + 216, v208 + 8);
    StoreF(o + 224, farg(ctx, 12));
    store<u32>(o + 228, 0);
    CopyW(o + 232, sp + 228);
    CopyW(o + 236, v236);
    CopyW(o + 244, sp + 244);
    CopyW(o + 240, v236 + 4);
    CopyW(o + 248, sp + 252);
    CopyW(o + 252, sp + 260);
    store<u32>(o + 256, 0);
    store<u32>(o + 260, 0);
    store<u32>(o + 264, 0);
    for (u32 k = 0; k < 4; ++k) CopyW(o + 268 + 4 * k, sp + 268 + 8 * k);
    for (u32 k = 0; k < 4; ++k) store<u32>(o + 284 + 4 * k, SArg(ctx, 300 + 8 * k));
    store<u32>(o + 300, 0xA77EB5E9u);
    store<u32>(o + 304, 0x3F800000u);
    store<u32>(o + 308, 0);
    store<u32>(o + 312, 0x3DCCCCCDu);
    store<u32>(o + 316, 0x41200000u);
    store<u8>(o + 321, 0);
    store<u32>(o + 324, 0x3F000000u);
    store<u32>(o + 328, 0x3F800000u);
    store<u32>(o + 332, 0x41F00000u);
    store<u32>(o + 336, 0);
    // Bits 0..11: s348, s356, s364, 0, 0, s332, s340, s156, s164, s172, s396, s404.
    static constexpr u32 kFlagArg[12] = {348, 356, 364, 0, 0, 332, 340, 156, 164, 172, 396, 404};
    for (u32 b = 0; b < 12; ++b) fl = PutBit(fl, b, kFlagArg[b] ? SArg(ctx, kFlagArg[b]) : 0);
    fl = (fl & ~0x000E0000u) | 0x00018000u;  // bits 12..14 clear, 15 / 16 set
    fl = (fl & ~0x00007C00u) | 0x00000200u;  // bits 17..21 clear, 22 set
    store<u32>(o, w0 | 0x2000u);
    store<u32>(o + 340, fl);
    for (u32 k = 0; k < 5; ++k) store<u32>(o + 304 + 4 * k, 0);
    ctx.fpr[1] = LoadF(v236 + 4);
    ctx.fpr[6] = LoadF(sp + 292);
}

