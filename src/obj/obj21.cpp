// Game code, part 21: a sweep of the smallest still-recompiled functions of
// the game code (0x00010000-0x005E0000 and 0x00730000-0x007A0000): field
// getters / setters, flag bits and small wrappers. Most have no known
// caller context yet, so their names describe what they do and carry the
// address. Each was checked against the original in the PPC simulator.
//
// Register notes: where the original leaves a value in r3 or f1 of a
// function that returns nothing, the value is set the same way, as the
// comments say ("r3 left ..."), so a caller reading it sees the same thing.
#include "obj21.h"

using namespace sly4;
using namespace sly4::obj21;

// ---------------------------------------------------------------- returns of an argument / constant
// u8 f(?, ?, u32 x): (u8)x
DECOMP(002AD4F4, Arg2_LowByte_002AD4F4) { ret(ctx, arg<u32>(ctx, 2) & 0xFFu); }
// u8 f(?, ?, u32 x): (u8)x
DECOMP(002AF5F0, Arg2_LowByte_002AF5F0) { ret(ctx, arg<u32>(ctx, 2) & 0xFFu); }
// u32 f(?, ?, u32 x): x
DECOMP(00771D00, Arg2_Return_00771D00) { ctx.gpr[3] = ctx.gpr[5]; }
// u32 f(): a constant hash (name?)
DECOMP(00738B50, Const_6B05BC2F) { ret(ctx, 0x6B05BC2Fu); }
// u32 f(): a constant hash (name?)
DECOMP(00782754, Const_5D1AFC2F) { ret(ctx, 0x5D1AFC2Fu); }

// ---------------------------------------------------------------- float min / max (fsel)
// f32 Math_Fmax(f32 a, f32 b): a - b >= 0 ? a : b (b when unordered)
DECOMP(00734DCC, Math_Fmax_00734DCC) {
    const f64 a = farg(ctx, 0), b = farg(ctx, 1);
    const f64 d = rt::to_single(a - b);
    ctx.fpr[3] = d;  // f3 left as the difference
    ret(ctx, d >= 0.0 ? a : b);
}
// f32 Math_Fmin(f32 a, f32 b): b - a >= 0 ? a : b (b when unordered)
DECOMP(007558D4, Math_Fmin_007558D4) {
    const f64 a = farg(ctx, 0), b = farg(ctx, 1);
    const f64 d = rt::to_single(b - a);
    ctx.fpr[3] = d;  // f3 left as the difference
    ret(ctx, d >= 0.0 ? a : b);
}

// ---------------------------------------------------------------- globals
// void f(u32 v): *(u32*)0x00F558C4 = v
DECOMP(000AB948, Global_SetF558C4) { store<u32>(0x00F558C4, arg<u32>(ctx, 0)); }
// void f(?, u32 v): *(u32*)0x00F64988 = v
DECOMP(00175740, Global_SetF64988) {
    store<u32>(0x00F64988, arg<u32>(ctx, 1));
    ctx.gpr[3] = 0x00F60000;  // r3 left as the address's high half
}
// void f(const Obj* o): *(u8*)0x010AF1BC = o->+12 (u8)
DECOMP(00073588, Global_Set10AF1BC) {
    const u8 v = load<u8>(arg<u32>(ctx, 0) + 12);
    store<u8>(0x010AF1BC, v);
    ret(ctx, v);  // r3 left as the byte
}
// void f(const Obj* o): *(f32*)0x010592F4 = o->+12 (f32)
DECOMP(000873F8, Global_Set10592F4) {
    const u32 bits = load<u32>(arg<u32>(ctx, 0) + 12);
    store<u32>(0x010592F4, bits);
    ret(ctx, std::bit_cast<f32>(bits));  // f1 left as the value
}
// u8 f(): *(u8*)0x01056548
DECOMP(0020FA10, Global_Get1056548) { ret(ctx, load<u8>(0x01056548)); }
// s32 f(): *(s32*)0x00F65D90
DECOMP(00483A24, Global_Get_F65D90) { ret(ctx, load<s32>(0x00F65D90)); }
// void f(f32 v): *(f32*)0x010C5A5C = v (field +60 of the block at 0x010C5A20)
DECOMP(0048C82C, Global_Set10C5A5C) {
    store<f32>(0x010C5A5C, static_cast<f32>(farg(ctx, 0)));
    ctx.gpr[3] = 0x010C5A20;  // r3 left as the block
}
// f32 f(): *(f32*)0x010C2478 (field +72 of the block at 0x010C2430)
DECOMP(004AFCA4, Global_Get10C2478) {
    ctx.gpr[3] = 0x010C2430;  // r3 left as the block
    ret(ctx, load<f32>(0x010C2478));
}

// ---------------------------------------------------------------- field getters
// bool f(const Obj* o): o->+304 bit 0
DECOMP(00738488, Field_Get304Bit0) { ret(ctx, load<u8>(arg<u32>(ctx, 0) + 304) & 1u); }
// void f(const Obj* o, u32* out): *out = o->+128; r3 left as the value
DECOMP(00762BB8, Field_Get128To_00762BB8) {
    const u32 v = load<u32>(arg<u32>(ctx, 0) + 128);
    store<u32>(arg<u32>(ctx, 1), v);
    ret(ctx, v);
}
// u32 f(const u32* p): *p with bit 0 cleared
DECOMP(0076FFF0, Word_ClearBit0_0076FFF0) { ret(ctx, load<u32>(arg<u32>(ctx, 0)) & ~1u); }
// u32 f(const Obj* o): o->+60->+204
DECOMP(0077CF70, Field_Get60_204) { ret(ctx, load<u32>(load<u32>(arg<u32>(ctx, 0) + 60) + 204)); }

// ---------------------------------------------------------------- field setters / copies
// void f(Obj* d, const Obj* s): d->+84 = s->+244
DECOMP(0027CDE0, Field_Copy244To84) {
    const u32 v = load<u32>(arg<u32>(ctx, 1) + 244);
    store<u32>(arg<u32>(ctx, 0) + 84, v);
    ctx.gpr[4] = v;  // r4 left as the value
}
// void f(Obj* o): o->+172 (u16) = 0
DECOMP(00370928, Field_Clear172h) { store<u16>(arg<u32>(ctx, 0) + 172, 0); }
// void f(Obj* d, const Obj* s): d->+4 (u8) = s->+4
DECOMP(00788258, Field_Copy4b_00788258) { store<u8>(arg<u32>(ctx, 0) + 4, load<u8>(arg<u32>(ctx, 1) + 4)); }
// void f(Obj* o, f32 d): o->+36 += d; f1 left as the sum
DECOMP(00014CF8, Field_Add36f) {
    const u32 o = arg<u32>(ctx, 0);
    const f32 v = static_cast<f32>(static_cast<f64>(load<f32>(o + 36)) + farg(ctx, 0));
    store<f32>(o + 36, v);
    ret(ctx, v);
}
// void f(const Obj* o): o->+4->+356 (f32) = o->+184
DECOMP(0010D9A0, Field_Copy184To4_356) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 bits = load<u32>(o + 184);
    store<u32>(load<u32>(o + 4) + 356, bits);
    ret(ctx, std::bit_cast<f32>(bits));  // f1 left as the value
}
// void f(Obj* o, const Obj* x): o->+496 = x, o->+684 (f32) = x->+28
DECOMP(00172B78, Field_SetRef496_00172B78) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 bits = load<u32>(x + 28);
    store<u32>(o + 496, x);
    store<u32>(o + 684, bits);
    ret(ctx, std::bit_cast<f32>(bits));  // f1 left as the value
}
// void f(u32 p[2]): p[0] = p[1] = 0
DECOMP(00321230, Pair_Clear_00321230) {
    const u32 p = arg<u32>(ctx, 0);
    store<u32>(p, 0);
    store<u32>(p + 4, 0);
}
// void f(Obj* o, u8 b): o->+0 = b, o->+4 = 0
DECOMP(004A090C, Field_SetByteClear4_004A090C) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o, static_cast<u8>(arg<u32>(ctx, 1)));
    store<u32>(o + 4, 0);
}
// void Vec3_Set(f32 v[3], f32 x, f32 y, f32 z)
DECOMP(004A11CC, Vec3_Set_004A11CC) {
    const u32 v = arg<u32>(ctx, 0);
    store<f32>(v, static_cast<f32>(farg(ctx, 0)));
    store<f32>(v + 4, static_cast<f32>(farg(ctx, 1)));
    store<f32>(v + 8, static_cast<f32>(farg(ctx, 2)));
}
// void f(Obj* o, f32 x): o->+72 (s32) = (s32)x, truncated
DECOMP(00736104, Field_SetInt72_00736104) {
    const u64 i = rt::fctiw(farg(ctx, 0), true);
    store<u32>(arg<u32>(ctx, 0) + 72, static_cast<u32>(i));
    ctx.fpr[1] = std::bit_cast<f64>(i);  // f1 left as the fctiwz result, as the recompiled code has it
}
// void f(u32 p[2]): p[0] (u32) = 0, p[1] (u16) = 0
DECOMP(00742CCC, Field_Clear0_4h_00742CCC) {
    const u32 p = arg<u32>(ctx, 0);
    store<u32>(p, 0);
    store<u16>(p + 4, 0);
}
// void f(Obj* o): o->+4 = 0x7F7FFFEE (just below FLT_MAX)
DECOMP(007490C0, Field_Set4Max_007490C0) { store<u32>(arg<u32>(ctx, 0) + 4, 0x7F7FFFEEu); }

// ---------------------------------------------------------------- flag bits
// void f(Obj* o, bool on): o->+172 bit 0x80 = on (bit 0 of the argument)
DECOMP(000DA0CC, Flag_Set172_80_000DA0CC) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 172, (load<u32>(o + 172) & ~0x80u) | ((arg<u32>(ctx, 1) << 7) & 0x80u));
}
// void f(Obj* o, bool on): o->+172 bit 0x40 = on
DECOMP(007373AC, Flag_Set172_40_007373AC) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 172, (load<u32>(o + 172) & ~0x40u) | ((arg<u32>(ctx, 1) << 6) & 0x40u));
}
// void f(Obj* o, bool on): o->+172 bit 0x20 = on
DECOMP(007373BC, Flag_Set172_20_007373BC) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 172, (load<u32>(o + 172) & ~0x20u) | ((arg<u32>(ctx, 1) << 5) & 0x20u));
}
// void f(Obj* o, bool on): o->+244 bit 0x800 = on
DECOMP(0074B65C, Flag_Set244_800_0074B65C) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 244, (load<u32>(o + 244) & ~0x800u) | ((arg<u32>(ctx, 1) << 11) & 0x800u));
}
// void f(Obj* o): o->+968 (u8) |= 2
DECOMP(0012D344, Flag_Set968_2) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o + 968, load<u8>(o + 968) | 2u);
}
// void f(Obj* o): o->+466 (u8) |= 0x20
DECOMP(007390F4, Flag_Set466_20) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o + 466, load<u8>(o + 466) | 0x20u);
}
// void f(Obj* o): o->+8 |= 0x40000000
DECOMP(0042CA60, Flag_Set8_40000000) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 8, load<u32>(o + 8) | 0x40000000u);
}
// void f(Obj* o): o->+8 &= ~0x80000000
DECOMP(00743908, Flag_Clear8_80000000) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 8, load<u32>(o + 8) & 0x7FFFFFFFu);
}

// ---------------------------------------------------------------- more flag bits (bit 0 of the argument)
// void f(Obj* o, bool on): o->+100 (u8) bit 0x20 = on
DECOMP(0074CB70, Flag_Set100b_20_0074CB70) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o + 100, static_cast<u8>((load<u8>(o + 100) & ~0x20u) | ((arg<u32>(ctx, 1) << 5) & 0x20u)));
}
// void f(Obj* o, u32 v): o->+432 (u16) bits 0x300 = v & 3
DECOMP(0074CE20, Flag_Set432h_300_0074CE20) {
    const u32 o = arg<u32>(ctx, 0);
    store<u16>(o + 432, static_cast<u16>((load<u16>(o + 432) & ~0x300u) | ((arg<u32>(ctx, 1) << 8) & 0x300u)));
}
// void f(Obj* o, bool on): o->+1268 bit 0x40000000 = on
DECOMP(0074DC24, Flag_Set1268_40000000_0074DC24) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 1268, (load<u32>(o + 1268) & ~0x40000000u) | ((arg<u32>(ctx, 1) << 30) & 0x40000000u));
}
// void f(Obj* o, bool on): o->+394 (u16) bit 0x2000 = on
DECOMP(00755D58, Flag_Set394h_2000_00755D58) {
    const u32 o = arg<u32>(ctx, 0);
    store<u16>(o + 394, static_cast<u16>((load<u16>(o + 394) & ~0x2000u) | ((arg<u32>(ctx, 1) << 13) & 0x2000u)));
}
// void f(Obj* o, bool on): o->+394 (u16) bit 0x8000 = on
DECOMP(00755D68, Flag_Set394h_8000_00755D68) {
    const u32 o = arg<u32>(ctx, 0);
    store<u16>(o + 394, static_cast<u16>((load<u16>(o + 394) & ~0x8000u) | ((arg<u32>(ctx, 1) << 15) & 0x8000u)));
}
// void f(Obj* o, bool on): o->+394 (u16) bit 0x800 = on
DECOMP(00755D78, Flag_Set394h_800_00755D78) {
    const u32 o = arg<u32>(ctx, 0);
    store<u16>(o + 394, static_cast<u16>((load<u16>(o + 394) & ~0x800u) | ((arg<u32>(ctx, 1) << 11) & 0x800u)));
}
// void f(Obj* o, bool on): o->+24 bit 0x10000000 = on
DECOMP(007616FC, Flag_Set24_10000000_007616FC) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 24, (load<u32>(o + 24) & ~0x10000000u) | ((arg<u32>(ctx, 1) << 28) & 0x10000000u));
}
// void f(Obj* o, bool on): o->+384 bit 0x80000000 = on
DECOMP(0075D690, Flag_Set384_80000000_0075D690) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 384, (load<u32>(o + 384) & 0x7FFFFFFFu) | (arg<u32>(ctx, 1) << 31));
}
// void f(Obj* o): o->+176 (u16) &= ~0x800
DECOMP(00735E20, Flag_Clear176h_800_00735E20) {
    const u32 o = arg<u32>(ctx, 0);
    store<u16>(o + 176, static_cast<u16>(load<u16>(o + 176) & ~0x800u));
    ctx.gpr[4] = 0;  // r4 left as 0
}
// void f(Obj* o): o->+168 |= 0x80000000
DECOMP(001158E4, Flag_Set168_80000000) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 168, load<u32>(o + 168) | 0x80000000u);
}
// void f(Obj* o, f32 v): o->+108 = v, o->+0 |= 0x40
DECOMP(0074E804, Field_Set108fFlag40_0074E804) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 flags = load<u32>(o);
    store<f32>(o + 108, static_cast<f32>(farg(ctx, 0)));
    store<u32>(o, flags | 0x40u);
}

// ---------------------------------------------------------------- booleans
// void f(Obj* o, u8 b): o->+88 (u8) = (b == 0)
DECOMP(0027C98C, Field_SetNot88b_0027C98C) {
    store<u8>(arg<u32>(ctx, 0) + 88, (arg<u32>(ctx, 1) & 0xFFu) == 0 ? 1 : 0);
}
// void f(Obj* o, u32 v): o->+72 (u8) = (v != 0)
DECOMP(0073600C, Field_SetBool72b_0073600C) {
    store<u8>(arg<u32>(ctx, 0) + 72, arg<u32>(ctx, 1) != 0 ? 1 : 0);
}
// bool f(const Obj* o): o->+100 != 0
DECOMP(00293938, Field_Has100_00293938) { ret(ctx, load<u32>(arg<u32>(ctx, 0) + 100) != 0 ? 1u : 0u); }
// bool f(const Obj* o): o->+18 (u8) == 5
DECOMP(00769A84, Field_Is18b5_00769A84) { ret(ctx, load<u8>(arg<u32>(ctx, 0) + 18) == 5 ? 1u : 0u); }

// ---------------------------------------------------------------- field getters / setters
// void f(Obj* o): o->+5 = o->+4 = 1 (u8)
DECOMP(00759D28, Field_Set4_5b_00759D28) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o + 5, 1);
    store<u8>(o + 4, 1);
}
// u32 f(const Obj* o): o->+416->+120 with bit 0 cleared
DECOMP(00759E30, Field_Get416_120_00759E30) {
    ret(ctx, load<u32>(load<u32>(arg<u32>(ctx, 0) + 416) + 120) & ~1u);
}
// void f(Obj* o, u32 v): o->+44 = v; r3 left as o + 36
DECOMP(007616EC, Field_Set44_007616EC) {
    const u32 p = arg<u32>(ctx, 0) + 36;
    store<u32>(p + 8, arg<u32>(ctx, 1));
    ret(ctx, p);
}
// void f(?, Obj* o): o->+64 = o->+8 = 0; r3 left 0
DECOMP(00771938, Field_Clear64_8_00771938) {
    const u32 o = arg<u32>(ctx, 1);
    store<u32>(o + 64, 0);
    store<u32>(o + 8, 0);
    ret(ctx, 0u);
}
// void f(?, Obj* o): o->+64 = o->+8 = 0; r3 left 0
DECOMP(00771948, Field_Clear64_8_00771948) {
    const u32 o = arg<u32>(ctx, 1);
    store<u32>(o + 64, 0);
    store<u32>(o + 8, 0);
    ret(ctx, 0u);
}
// u8 f(const Obj* o): o->+232 (u8)
DECOMP(0077CF60, Field_Get232b_0077CF60) { ret(ctx, load<u8>(arg<u32>(ctx, 0) + 224 + 8)); }
// void f(Obj* o): o->+68 (f32) = *(f32*)0x00F9CC0C
DECOMP(0001BE8C, Field_Set68FromF9CC0C) {
    const u32 bits = load<u32>(0x00F9CC0C);
    store<u32>(arg<u32>(ctx, 0) + 68, bits);
    ret(ctx, std::bit_cast<f32>(bits));  // f1 left as the value
}
// void f(Obj* o): o->+200 (f32) = *(f32*)0x010388A4
DECOMP(00738248, Field_Set200From10388A4) {
    const u32 bits = load<u32>(0x010388A4);
    store<u32>(arg<u32>(ctx, 0) + 200, bits);
    ret(ctx, std::bit_cast<f32>(bits));  // f1 left as the value
}
// void f(): (*(Obj**)0x00F64C38)->+44 (u16) = 0; r3 left as the object
DECOMP(0006F600, Global_ClearF64C38_44) {
    const u32 o = load<u32>(0x00F64C38);
    store<u16>(o + 44, 0);
    ret(ctx, o);
}
// void f(Obj* o): o->+4032 (u64) = 0; r3 left as o + 4032
DECOMP(000AE2A8, Field_Clear4032d_000AE2A8) {
    const u32 p = arg<u32>(ctx, 0) + 4032;
    store<u64>(p, 0);
    ret(ctx, p);
}
// void f(Obj* o, u32 v): if (o->+508 == 1) o->+512 = v
DECOMP(001DBA44, Field_Set512If508_001DBA44) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<s32>(o + 508) == 1) store<u32>(o + 512, arg<u32>(ctx, 1));
}
// void f(Obj* o): o->+64 (u8) = 0, o->+72 = o->+68 = 0
DECOMP(00312980, Field_Clear64_00312980) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o + 64, 0);
    store<u32>(o + 72, 0);
    store<u32>(o + 68, 0);
}
// void f(Obj* o, u32 v): if (o->+32) o->+32->+68 = v; r3 left as o->+32
DECOMP(00354198, Field_Set32_68_00354198) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 32);
    if (p != 0) store<u32>(p + 68, arg<u32>(ctx, 1));
    ret(ctx, p);
}
// s32 f(const u32* a, const u32* b): *a - *b (a qsort comparator)
DECOMP(00355FE4, Cmp_Word_00355FE4) {
    const u32 b = load<u32>(arg<u32>(ctx, 1));
    ctx.gpr[4] = b;  // r4 left as *b
    ret(ctx, static_cast<s32>(load<u32>(arg<u32>(ctx, 0)) - b));
}
// void f(u32 a, u32 b): *(u32*)0x0106C270 = b, *(u32*)0x00F64CB4 = a
DECOMP(00369D00, Global_Set106C270_F64CB4) {
    store<u32>(0x0106C270, arg<u32>(ctx, 1));
    store<u32>(0x00F64CB4, arg<u32>(ctx, 0));
}
// s32 f(): *(s32*)0x010F0EDC (field +188 of the block at 0x010F0E20)
DECOMP(0046458C, Global_Get10F0EDC) { ret(ctx, load<s32>(0x010F0EDC)); }
// void f(Obj* o, u8 b, f32 x): o->+0 = b, o->+4 = 0, o->+8 = x
DECOMP(004A08A4, Field_SetByte0_4_8f_004A08A4) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o, static_cast<u8>(arg<u32>(ctx, 1)));
    store<u32>(o + 4, 0);
    store<f32>(o + 8, static_cast<f32>(farg(ctx, 0)));
}
// void f(u32 p[3]): p[0] = p[1] = p[2] = 0
DECOMP(004A11B4, Vec3_Zero_004A11B4) {
    const u32 p = arg<u32>(ctx, 0);
    store<u32>(p, 0);
    store<u32>(p + 4, 0);
    store<u32>(p + 8, 0);
}
// void f(u32 i, u32 v): ((u32*)0x010C2AD8)[i] = v
DECOMP(004D5714, Table10C2AD8_Set) {
    const u32 off = arg<u32>(ctx, 0) << 2;
    store<u32>(0x010C2AD8 + off, arg<u32>(ctx, 1));
    ret(ctx, off);  // r3 left as i * 4
}
// u32 f(u32 i): ((u32*)0x010C2AD8)[i]
DECOMP(004D5728, Table10C2AD8_Get) { ret(ctx, load<u32>(0x010C2AD8 + (arg<u32>(ctx, 0) << 2))); }
// void f(Obj* o): o->+32 (u8) = 0, o->+36 = o->+40 = 0
DECOMP(0074C6C4, Field_Clear32_0074C6C4) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o + 32, 0);
    store<u32>(o + 36, 0);
    store<u32>(o + 40, 0);
}
// void f(f32 v[3]): v = {0.5, 0.5, 0.5}
DECOMP(0074F1F4, Vec3_SetHalf_0074F1F4) {
    const u32 v = arg<u32>(ctx, 0);
    for (u32 k = 0; k < 3; ++k) store<u32>(v + 4 * k, 0x3F000000u);  // 0.5f
}
// void f(Obj* o, s32 id, f32 x): if (o->+212 == id) o->+4 = x
DECOMP(00767B54, Field_Set4fIf212_00767B54) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<s32>(o + 212) == arg<s32>(ctx, 1)) store<f32>(o + 4, static_cast<f32>(farg(ctx, 0)));
}
// u32* f(Obj* o, u32 i): &o->+80[i]
DECOMP(00748CD0, Field_At80_00748CD0) {
    const u32 off = arg<u32>(ctx, 1) << 2;
    ctx.gpr[4] = off;  // r4 left as i * 4
    ret(ctx, arg<u32>(ctx, 0) + off + 80);
}
// u32* f(Obj* o, u32 i): &o->+72[i]
DECOMP(0074C448, Field_At72_0074C448) {
    const u32 off = arg<u32>(ctx, 1) << 2;
    ctx.gpr[4] = off;  // r4 left as i * 4
    ret(ctx, arg<u32>(ctx, 0) + off + 72);
}
// void f(u32* p): *p = 0, atomically (lwarx / stwcx. loop: an exchange with 0)
DECOMP(0074E97C, Atomic_Clear_0074E97C) {
    const u32 p = arg<u32>(ctx, 0);
    u32 old;
    for (;;) {
        old = load<u32>(p);
        if (mem::cas32(p, old, 0)) break;
    }
    ctx.gpr[4] = 0;    // r4 left 0
    ctx.gpr[5] = old;  // r5 left as the old value
}

// ---------------------------------------------------------------- float min / max / clamp
// f32 Math_Fclamp(f32 x, f32 lo, f32 hi): max(min(x, hi), lo) with fsel
DECOMP(00734E4C, Math_Fclamp_00734E4C) {
    f64 x = farg(ctx, 0);
    const f64 lo = farg(ctx, 1), hi = farg(ctx, 2);
    const f64 d1 = rt::to_single(hi - x);
    x = d1 >= 0.0 ? x : hi;
    const f64 d2 = rt::to_single(x - lo);
    ctx.fpr[4] = d1;  // f4 / f3 left as the differences
    ctx.fpr[3] = d2;
    ret(ctx, d2 >= 0.0 ? x : lo);
}
// f32 f(const f32* a, const f32* b): max(*a, *b) (fsel; *b when unordered)
DECOMP(0074C45C, Math_FmaxPtr_0074C45C) {
    const f64 a = load<f32>(arg<u32>(ctx, 0)), b = load<f32>(arg<u32>(ctx, 1));
    const f64 d = rt::to_single(a - b);
    ctx.fpr[2] = b;  // f2 / f3 left
    ctx.fpr[3] = d;
    ret(ctx, d >= 0.0 ? a : b);
}
// f32 f(const f32* a, const f32* b): min(*a, *b) (fsel; *a when unordered)
DECOMP(0074CFD4, Math_FminPtr_0074CFD4) {
    const f64 a = load<f32>(arg<u32>(ctx, 0)), b = load<f32>(arg<u32>(ctx, 1));
    const f64 d = rt::to_single(a - b);
    ctx.fpr[2] = b;  // f2 / f3 left
    ctx.fpr[3] = d;
    ret(ctx, d >= 0.0 ? b : a);
}

// ---------------------------------------------------------------- batch 3: getters
// bool f(const Obj* o): o->+2576 != 0
DECOMP(00769B44, Field_Has2576_00769B44) { ret(ctx, load<u32>(arg<u32>(ctx, 0) + 2576) != 0 ? 1u : 0u); }
// bool f(const Obj* o): o->+304 bit 0x40
DECOMP(00738494, Field_Get304Bit6_00738494) { ret(ctx, (load<u8>(arg<u32>(ctx, 0) + 304) >> 6) & 1u); }
// bool f(const Obj* o): o->+2338 bit 0x04
DECOMP(00772874, Field_Get2338Bit2_00772874) { ret(ctx, (load<u8>(arg<u32>(ctx, 0) + 2338) >> 2) & 1u); }
// bool f(const Obj* o): (o->+160 >> 1) == 5
DECOMP(0078751C, Field_Is160State5_0078751C) { ret(ctx, (load<u32>(arg<u32>(ctx, 0) + 160) >> 1) == 5 ? 1u : 0u); }
// bool f(const Obj* o): o->+4 != 13
DECOMP(0001F3D8, Field_Not4Is13_0001F3D8) { ret(ctx, load<s32>(arg<u32>(ctx, 0) + 4) == 13 ? 0u : 1u); }
// u32 f(const Obj* o): o->+4 ? 1 : o->+316 (u8)
DECOMP(0003D044, Field_Get316bOr1_0003D044) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 v = load<u32>(o + 4) != 0 ? 1u : load<u8>(o + 316);
    ctx.gpr[4] = v;  // r4 left as the result
    ret(ctx, v);
}
// u32 f(const u32* p): p ? *p : 0
DECOMP(00443150, Ptr_DerefOr0_00443150) {
    const u32 p = arg<u32>(ctx, 0);
    ret(ctx, p != 0 ? load<u32>(p) : 0u);
}
// u32 f(const Obj* o, u32 sel): sel ? o->+384 : o->+160
DECOMP(0074CFBC, Field_Get384Or160_0074CFBC) {
    const u32 o = arg<u32>(ctx, 0);
    ret(ctx, arg<u32>(ctx, 1) != 0 ? load<u32>(o + 384) : load<u32>(o + 160));
}
// u32 f(const Obj* o, s32 i): o->+40[i]
DECOMP(00760840, Field_Get40At_00760840) {
    const u32 off = arg<u32>(ctx, 1) << 2;
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(static_cast<s32>(off)));  // r4 left as i * 4
    ret(ctx, load<u32>(arg<u32>(ctx, 0) + 40 + off));
}
// u32 f(const Obj* o, s32 i): o->+72[i]
DECOMP(00760BC8, Field_Get72At_00760BC8) {
    const u32 off = arg<u32>(ctx, 1) << 2;
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(static_cast<s32>(off)));  // r4 left as i * 4
    ret(ctx, load<u32>(arg<u32>(ctx, 0) + 72 + off));
}
// u32 f(const Obj* o, s32 i): o->+2292[i]
DECOMP(00769AE0, Field_Get2292At_00769AE0) {
    const u32 off = arg<u32>(ctx, 1) << 2;
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(static_cast<s32>(off)));  // r4 left as i * 4
    ret(ctx, load<u32>(arg<u32>(ctx, 0) + 2292 + off));
}
// u32* f(Obj** o, u32 i): &(**o)[i]
DECOMP(0076EE64, Ptr_At_0076EE64) {
    const u32 off = arg<u32>(ctx, 1) << 2;
    ctx.gpr[4] = off;  // r4 left as i * 4
    ret(ctx, load<u32>(load<u32>(arg<u32>(ctx, 0))) + off);
}
// T* f(const Obj* o, u32 i): o->+388 + i * 96
DECOMP(001BD51C, Field_At388x96_001BD51C) {
    const u64 i32 = ctx.gpr[4] << 5;  // rldicr: 64-bit
    ctx.gpr[5] = i32 + i32;          // r5 / r4 left as i * 64 / i * 96 (64-bit)
    ctx.gpr[4] = i32 + ctx.gpr[5];
    ret(ctx, static_cast<u32>(load<u32>(arg<u32>(ctx, 0) + 388) + ctx.gpr[4]));
}
// u32 f(const Obj* o): o->+428 (r4 left the same)
DECOMP(00221068, Field_Get428_00221068) {
    const u32 v = load<u32>(arg<u32>(ctx, 0) + 428);
    ctx.gpr[4] = v;
    ret(ctx, v);
}
// u32 f(const Obj* o): o->+612 ? o->+612->+24 : 0
DECOMP(0023CC10, Field_Get612_24_0023CC10) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 612);
    ret(ctx, p != 0 ? load<u32>(p + 24) : 0u);
}
// f32 Vec2_LengthSq(const f32 v[2]): x * x + y * y (fused)
DECOMP(0077AECC, Vec2_LengthSq_0077AECC) {
    const u32 v = arg<u32>(ctx, 0);
    const f32 x = load<f32>(v), y = load<f32>(v + 4);
    const f32 yy = static_cast<f32>(static_cast<f64>(y) * y);
    ctx.fpr[2] = x;  // f2 left as x
    ret(ctx, static_cast<f32>(std::fma(static_cast<f64>(x), static_cast<f64>(x), static_cast<f64>(yy))));
}

// ---------------------------------------------------------------- batch 3: setters
// void f(u8* d, const Obj* s): *d = s->+12 (u8)
DECOMP(00771DBC, Field_Copy12b_00771DBC) {
    const u8 b = load<u8>(arg<u32>(ctx, 1) + 12);
    store<u8>(arg<u32>(ctx, 0), b);
    ctx.gpr[4] = b;  // r4 left as the byte
}
// void f(Obj* o, u8 b): if (o->+44) o->+44->+404 (u8) = b; r3 left as o->+44
DECOMP(0077B180, Field_Set44_404b_0077B180) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 44);
    if (p != 0) store<u8>(p + 404, static_cast<u8>(arg<u32>(ctx, 1)));
    ret(ctx, p);
}
// void f(Obj* o, bool on): o->+184 bit 0x80000000 = on
DECOMP(0077D608, Flag_Set184_80000000_0077D608) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 184, (load<u32>(o + 184) & 0x7FFFFFFFu) | (arg<u32>(ctx, 1) << 31));
}
// void f(Obj* o): o->+12..+24 = 1.0f (four floats)
DECOMP(000ADC28, Field_SetOnes12_000ADC28) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 k = 0; k < 4; ++k) store<u32>(o + 12 + 4 * k, 0x3F800000u);
}
// void f(s64 id): if (id == 2050) *(u32*)0x00F9D1B4 = 1 (a 64-bit compare)
DECOMP(000B0D34, Global_SetF9D1B4If2050) {
    if (ctx.gpr[3] != 2050) return;
    store<u32>(0x00F9D1B4, 1);
    ctx.gpr[3] = 0x00FA0000;  // r3 / r4 left
    ctx.gpr[4] = 1;
}
// void f(Obj* o): o->+228 = 0, o->+8 &= ~0x80000000
DECOMP(000B566C, Field_Clear228Flag8_000B566C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 v = load<u32>(o + 8);
    store<u32>(o + 228, 0);
    store<u32>(o + 8, v & 0x7FFFFFFFu);
}
// void f(Obj* o): o->+68 (u8) = 0, o->+72 = 0, o->+80 (u8) = 1
DECOMP(00108468, Field_Reset68_00108468) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o + 68, 0);
    store<u32>(o + 72, 0);
    store<u8>(o + 80, 1);
}
// void f(Obj* o): o->+36 = 0, ++o->+28
DECOMP(003124D0, Field_Clear36Inc28_003124D0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = load<u32>(o + 28);
    store<u32>(o + 36, 0);
    store<u32>(o + 28, n + 1);
}
// void f(Obj* o): o->+98 (u8) = 1, o->+100 (f32) = *o->+12
DECOMP(003178B8, Field_Set98_100f_003178B8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 src = load<u32>(o + 12);
    store<u8>(o + 98, 1);
    const u32 bits = load<u32>(src);
    store<u32>(o + 100, bits);
    ret(ctx, std::bit_cast<f32>(bits));  // f1 left as the value
}
// void f(Obj* o, bool on): if (o) o->+396 bit 0x02000000 = on
DECOMP(00367518, Flag_Set396_2000000_00367518) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    InsertBits32(o + 396, 0x02000000u, arg<u32>(ctx, 1) << 25);
}
// void f(Obj* o): if (o) ++o->+20
DECOMP(00446170, Field_Inc20_00446170) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o + 20, load<u32>(o + 20) + 1);
}
// void f(Obj* o): o->+4 (u16) = 1, o->+0 = 0x00B21C98 (a vtable)
DECOMP(004BB3B0, Obj_InitVt_B21C98) {
    const u32 o = arg<u32>(ctx, 0);
    store<u16>(o + 4, 1);
    store<u32>(o, 0x00B21C98);
}
// void f(Obj* o): o->+6 (u16) = 1, o->+0 = 0x00B1D3E0 (a vtable)
DECOMP(00765E7C, Obj_InitVt_B1D3E0) {
    const u32 o = arg<u32>(ctx, 0);
    store<u16>(o + 6, 1);
    store<u32>(o, 0x00B1D3E0);
}
// void f(Obj* o): if (o) o->+0 = 0x00B13974 (a vtable)
DECOMP(00749074, Obj_SetVt_B13974) {
    const u32 o = arg<u32>(ctx, 0);
    if (o != 0) store<u32>(o, 0x00B13974);
}
// void f(Obj* o): o->+0 = o->+4 = 0, o->+9 = o->+8 = 0 (u8)
DECOMP(007372DC, Field_Clear0_9_007372DC) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0);
    store<u32>(o + 4, 0);
    store<u8>(o + 9, 0);
    store<u8>(o + 8, 0);
}
// void f(u8 p[2]): p as a little-endian u16 rewritten big-endian: *(u16*)p = p[1] << 8 | p[0]
DECOMP(00742D10, U16_SwapInPlace_00742D10) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 hi = load<u8>(p + 1), lo = load<u8>(p);
    const u32 v = (hi << 8) | lo;
    store<u16>(p, static_cast<u16>(v));
    ctx.gpr[4] = v;  // r4 / r5 left
    ctx.gpr[5] = lo;
}
// void f(Obj* o, u32 i, u32 v): (o + i)->+656 (u8) bits 0x38 = v << 3; r3 left as o + i
DECOMP(0074BFE0, Flag_Set656At_38_0074BFE0) {
    const u32 p = arg<u32>(ctx, 0) + arg<u32>(ctx, 1);
    InsertBits8(p + 656, 0x38u, arg<u32>(ctx, 2) << 3);
    ret(ctx, p);
}
// void f(Obj* o, u32 i, bool on): (o + i)->+656 (u8) bit 0x40 = on; r3 left as o + i
DECOMP(0074D004, Flag_Set656At_40_0074D004) {
    const u32 p = arg<u32>(ctx, 0) + arg<u32>(ctx, 1);
    InsertBits8(p + 656, 0x40u, arg<u32>(ctx, 2) << 6);
    ret(ctx, p);
}
// void f(Vec4* out, const Vec4* c, const Vec4* a, const Vec4* b): *out = a * b + c (vmaddfp)
DECOMP(0074C6D8, Vec4_MulAdd_0074C6D8) {
    VSt(arg<u32>(ctx, 0), VMadd(VLd(arg<u32>(ctx, 2)), VLd(arg<u32>(ctx, 3)), VLd(arg<u32>(ctx, 1))));
}
// void f(Obj* o): ++o->+178 (u16); r3 left as o + 178
DECOMP(0074CC74, Field_Inc178h_0074CC74) {
    const u32 p = arg<u32>(ctx, 0) + 178;
    store<u16>(p, static_cast<u16>(load<u16>(p) + 1));
    ret(ctx, p);
}
// void f(f32 v[3]): v = {0, -7, 0}
DECOMP(0074F42C, Vec3_SetDown7_0074F42C) {
    const u32 v = arg<u32>(ctx, 0);
    store<u32>(v, 0);
    store<u32>(v + 4, 0xC0E00000u);  // -7.0f
    store<u32>(v + 8, 0);
}
// void f(f32 v[3]): v = {0, -7, 0}
DECOMP(0074F5DC, Vec3_SetDown7_0074F5DC) {
    const u32 v = arg<u32>(ctx, 0);
    store<u32>(v, 0);
    store<u32>(v + 4, 0xC0E00000u);  // -7.0f
    store<u32>(v + 8, 0);
}
// void f(Obj* o): o->+20 = 0, o->+4 = 0x7F7FFFEE (just below FLT_MAX)
DECOMP(007623F8, Field_Reset20_4_007623F8) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 20, 0);
    store<u32>(o + 4, 0x7F7FFFEEu);
}
// void f(Obj* o, bool on): o->+160 bit 0 = on
DECOMP(00772944, Flag_Set160Bit0_00772944) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 160, (load<u32>(o + 160) & ~1u) | (arg<u32>(ctx, 1) & 1u));
}
// void Vec3_Copy(f32 d[3], ?, const f32 s[3]) (word by word)
DECOMP(0003BE08, Vec3_Copy_0003BE08) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 2);
    u32 bits = 0;
    for (u32 k = 0; k < 3; ++k) {
        bits = load<u32>(s + 4 * k);
        store<u32>(d + 4 * k, bits);
    }
    ret(ctx, std::bit_cast<f32>(bits));  // f1 left as the last value
}
// void f(const Obj* o): (*(u8**)0x00F65CA0)[0x12828D] = o->+12 (u8)
DECOMP(00072C50, Global_SetF65CA0_12828D) {
    const u8 b = load<u8>(arg<u32>(ctx, 0) + 12);
    store<u8>(load<u32>(0x00F65CA0) + 0x0012828Du, b);
    ret(ctx, b);  // r3 left as the byte
}
// void f(const Obj* o): (*(u8**)0x00F65CA0)->+45 bit 0x08 = o->+12 bit 0
DECOMP(00086A18, Global_SetF65CA0_45Bit3) {
    const u32 g = load<u32>(0x00F65CA0);
    const u8 b = load<u8>(arg<u32>(ctx, 0) + 12);
    InsertBits8(g + 45, 0x08u, static_cast<u32>(b) << 3);
    ret(ctx, b);  // r3 left as the byte
}
// void f(): the three words at 0x00F9CD98 = 0
DECOMP(0009A320, Global_ClearF9CD98) {
    for (u32 k = 0; k < 3; ++k) store<u32>(0x00F9CD98 + 4 * k, 0);
    ctx.gpr[3] = 0x00FA0000;  // r3 left
}
// void f(Obj* o): o->+0: clear 0x40000000 and 0x20000000, set 0x10000000
DECOMP(000ADC40, Flag_Set0_10000000_000ADC40) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 v = (load<u32>(o) & ~0x60000000u) | 0x10000000u;
    store<u32>(o, v);
    ctx.gpr[4] = v;  // r4 left as the new word
}

// ---------------------------------------------------------------- batch 4
// u32 f(const Obj* o, u32 i): o->+372[i]->+12
DECOMP(00271C90, Field_Get372At_12_00271C90) {
    const u32 off = arg<u32>(ctx, 1) << 2;
    ctx.gpr[4] = off;  // r4 left as i * 4
    ret(ctx, load<u32>(load<u32>(arg<u32>(ctx, 0) + 372 + off) + 12));
}
// bool f(?, ?, u32 a, u32 b, Rec* r): r->+8 = a, r->+12 = b, r->+0->+56 = 0; returns 1
DECOMP(002DA718, Rec_Set8_12_002DA718) {
    const u32 r = arg<u32>(ctx, 4);
    const u32 owner = load<u32>(r);
    store<u32>(r + 8, arg<u32>(ctx, 2));
    store<u32>(r + 12, arg<u32>(ctx, 3));
    store<u32>(owner + 56, 0);
    ctx.gpr[4] = 0;      // r4 / r8 left
    ctx.gpr[8] = owner;
    ret(ctx, 1u);
}
// void f(): the three words at 0x0106B250 = 0
DECOMP(0032120C, Global_Clear106B250) {
    for (u32 k = 0; k < 3; ++k) store<u32>(0x0106B250 + 4 * k, 0);
    ctx.gpr[3] = 0x01070000;  // r3 / r4 / r5 left
    ctx.gpr[4] = 0;
    ctx.gpr[5] = 0x0106B250;
}
// void f(): if (*(Obj**)0x00F64CBC) its +220 (f32) = its +216
DECOMP(00369658, Global_F64CBC_Copy216) {
    const u32 o = load<u32>(0x00F64CBC);
    ret(ctx, o);  // r3 left as the object
    if (o == 0) return;
    const u32 bits = load<u32>(o + 216);
    store<u32>(o + 220, bits);
    ctx.fpr[1] = std::bit_cast<f32>(bits);
}
// void f(): if (*(Obj**)0x00F64CB8) its +220 (f32) = its +216
DECOMP(00369D58, Global_F64CB8_Copy216) {
    const u32 o = load<u32>(0x00F64CB8);
    ret(ctx, o);  // r3 left as the object
    if (o == 0) return;
    const u32 bits = load<u32>(o + 216);
    store<u32>(o + 220, bits);
    ctx.fpr[1] = std::bit_cast<f32>(bits);
}
// void f(Obj* o): o->+8 = o->+28 = 0 (u16), o->+40 = o, o->+44 = o + 20
DECOMP(003DC924, Obj_InitLinks_003DC924) {
    const u32 o = arg<u32>(ctx, 0);
    store<u16>(o + 8, 0);
    store<u16>(o + 28, 0);
    store<u32>(o + 40, o);
    store<u32>(o + 44, o + 20);
    ctx.gpr[4] = 0;  // r4 / r5 left
    ctx.gpr[5] = o + 20;
}
// void f(const Msg* m): if (m->+12 == 1) *(f32*)0x00F65B4C = m->+16
DECOMP(00439DA8, Global_SetF65B4CIf1) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 kind = load<u32>(m + 12);
    ctx.gpr[4] = kind;  // r4 left as the kind
    if (static_cast<s32>(kind) != 1) return;
    const u32 bits = load<u32>(m + 16);
    store<u32>(0x00F65B4C, bits);
    ctx.gpr[4] = 0x00F60000;
    ctx.fpr[1] = std::bit_cast<f32>(bits);
}
// void f(Obj* o, bool on): o->+88 (u8): clear 0x80, set 0x04, bit 0x40 = on
DECOMP(00455F34, Flag_Set88b_00455F34) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 v = (((load<u8>(o + 88) & ~0x80u) | 0x04u) & ~0x40u) | ((arg<u32>(ctx, 1) << 6) & 0x40u);
    store<u8>(o + 88, static_cast<u8>(v));
    ctx.gpr[5] = v;  // r5 left as the new byte (in a word)
}
// void f(): *(u32*)0x010B1780 = 0, atomically (lwarx / stwcx. exchange)
DECOMP(004570D8, Atomic_Clear10B1780) {
    constexpr u32 p = 0x010B1780;
    u32 old;
    for (;;) {
        old = load<u32>(p);
        if (mem::cas32(p, old, 0)) break;
    }
    ctx.gpr[3] = 0;  // r3 / r4 / r5 left
    ctx.gpr[4] = p;
    ctx.gpr[5] = old;
}
// f32 Vec3_LengthSq(const f32 v[3]): (y * y + x * x) + z * z, fused
DECOMP(00734BBC, Vec3_LengthSq_00734BBC) {
    const u32 v = arg<u32>(ctx, 0);
    const f64 x = load<f32>(v), y = load<f32>(v + 4), z = load<f32>(v + 8);
    f64 s = rt::to_single(y * y);
    s = rt::to_single(std::fma(x, x, s));
    ctx.fpr[2] = x;  // f2 / f3 left
    ctx.fpr[3] = z;
    ret(ctx, static_cast<f32>(std::fma(z, z, s)));
}
// T* f(const Obj* o, u32 i): o->+0 + i * 112
DECOMP(00735B84, Field_At0x112_00735B84) {
    const u64 i = ctx.gpr[4];
    ctx.gpr[5] = i << 7;  // r5 / r4 left (64-bit)
    ctx.gpr[4] = (i << 7) - (i << 4);
    ret(ctx, static_cast<u32>(load<u32>(arg<u32>(ctx, 0)) + ctx.gpr[4]));
}
// void Vec3_Copy(f32 d[3], ?, const f32 s[3]) (word by word)
DECOMP(00738778, Vec3_Copy_00738778) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 2);
    u32 bits = 0;
    for (u32 k = 0; k < 3; ++k) {
        bits = load<u32>(s + 4 * k);
        store<u32>(d + 4 * k, bits);
    }
    ret(ctx, std::bit_cast<f32>(bits));  // f1 left as the last value
}
// void f(Obj* o): o->+16 = 0x00F1E070, o->+0 = 0x00B13974 (the vtable of 00749074)
DECOMP(00749D24, Obj_InitVt_B13974_F1E070) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 16, 0x00F1E070);
    store<u32>(o, 0x00B13974);
    ctx.gpr[4] = 0x00F1E070;  // r4 / r5 left
    ctx.gpr[5] = 0x00B13974;
}
// void f(Obj* o): o->+136 = 0, o->+40 &= ~0x81FE0000
DECOMP(0074C42C, Field_Reset136_40_0074C42C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 v = load<u32>(o + 40) & ~0x81FE0000u;
    store<u32>(o + 136, 0);
    store<u32>(o + 40, v);
    ctx.gpr[4] = 0;  // r4 / r5 left
    ctx.gpr[5] = v;
}
// void Vec2_Scale(f32 out[2], const f32 in[2], f32 s)
DECOMP(0074C7F8, Vec2_Scale_0074C7F8) {
    const u32 out = arg<u32>(ctx, 0), in = arg<u32>(ctx, 1);
    const f64 s = farg(ctx, 0);
    const f64 y = load<f32>(in + 4);
    const f32 rx = static_cast<f32>(load<f32>(in) * s), ry = static_cast<f32>(y * s);
    store<f32>(out, rx);
    store<f32>(out + 4, ry);
    ctx.fpr[2] = rx;  // f2 / f3 left
    ctx.fpr[3] = y;
    ret(ctx, ry);
}
// void Vec2_ScaleInPlace(f32 v[2], f32 s)
DECOMP(00755DEC, Vec2_ScaleInPlace_00755DEC) {
    const u32 v = arg<u32>(ctx, 0);
    const f64 s = farg(ctx, 0);
    const f64 y = load<f32>(v + 4);
    const f32 rx = static_cast<f32>(load<f32>(v) * s), ry = static_cast<f32>(y * s);
    store<f32>(v, rx);
    store<f32>(v + 4, ry);
    ctx.fpr[2] = rx;  // f2 / f3 left
    ctx.fpr[3] = y;
    ret(ctx, ry);
}
// u16 f(Obj** o): *o ? (*o)->+8 (u16) : 0
DECOMP(0074CAE0, Ptr_Get8h_0074CAE0) {
    const u32 p = load<u32>(arg<u32>(ctx, 0));
    const u32 v = p != 0 ? load<u16>(p + 8) : 0u;
    ctx.gpr[4] = v;  // r4 left as the result
    ret(ctx, v);
}
// u16 f(Obj** o): *o ? (*o)->+8 (u16) : 0
DECOMP(0076EE48, Ptr_Get8h_0076EE48) {
    const u32 p = load<u32>(arg<u32>(ctx, 0));
    const u32 v = p != 0 ? load<u16>(p + 8) : 0u;
    ctx.gpr[4] = v;  // r4 left as the result
    ret(ctx, v);
}
// u16 f(const Obj* o, u32 i): o->+132[i * 2]->+52 (u16)
DECOMP(0074CB24, Field_Get132At_52h_0074CB24) {
    const u32 off = arg<u32>(ctx, 1) << 3;
    ctx.gpr[4] = off;  // r4 left as i * 8
    ret(ctx, load<u16>(load<u32>(arg<u32>(ctx, 0) + 132 + off) + 52));
}
// u32 f(const Obj* o): o->+256 ? *o->+256 : 0
DECOMP(0074D898, Field_Get256_0_0074D898) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 256);
    ret(ctx, p != 0 ? load<u32>(p) : 0u);
}
// bool f(const Obj* o): o->+181 (u8) != 0
DECOMP(0074DAA8, Field_Has181b_0074DAA8) { ret(ctx, load<u8>(arg<u32>(ctx, 0) + 181) != 0 ? 1u : 0u); }
// bool f(const Obj* o): o->+2327 (u8) == 0
DECOMP(0077ED30, Field_Not2327b_0077ED30) { ret(ctx, load<u8>(arg<u32>(ctx, 0) + 2327) == 0 ? 1u : 0u); }
// void f(Obj* o): o->+8 = 0x00B194F8, o->+0 = 0x00B13974 (the vtable of 00749074)
DECOMP(0075A668, Obj_InitVt_B13974_B194F8) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 8, 0x00B194F8);
    store<u32>(o, 0x00B13974);
    ctx.gpr[4] = 0x00B194F8;  // r4 / r5 left
    ctx.gpr[5] = 0x00B13974;
}
// void Vec4_Lerp(Vec4* out, const Vec4* a, const Vec4* b, const Vec4* t): (b - a) * t + a (VMX)
DECOMP(0076F47C, Vec4_Lerp_0076F47C) {
    const V4 a = VLd(arg<u32>(ctx, 1)), b = VLd(arg<u32>(ctx, 2)), t = VLd(arg<u32>(ctx, 3));
    V4 d;
    for (u32 i = 0; i < 4; ++i) d.w[i] = std::bit_cast<u32>(b.f(i) - a.f(i));
    VSt(arg<u32>(ctx, 0), VMadd(d, t, a));
}
// void f(Obj* o, f32 x): o->+510 (u16) = the high half of x's bits
DECOMP(0077CDA4, Field_Set510FromFloatHi_0077CDA4) {
    const s32 hi = static_cast<s32>(std::bit_cast<u32>(static_cast<f32>(farg(ctx, 0)))) >> 16;
    store<u16>(arg<u32>(ctx, 0) + 510, static_cast<u16>(hi));
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(hi));  // r4 left (srawi)
}
// s32 f(const Obj* o): (s64)o->+20, truncated; the low word
DECOMP(00793EBC, Field_GetInt20_00793EBC) {
    const u64 i = rt::fctid(load<f32>(arg<u32>(ctx, 0) + 20), true);
    ctx.fpr[1] = std::bit_cast<f64>(i);  // f1 left as the fctidz result
    ret(ctx, static_cast<u32>(i));
}
// void f(const Msg* m): if (g = (*(Obj**)0x00F64C20)->+80) g->+2581 (u8) = m->+12
DECOMP(00082008, Global_F64C20_Set2581_12) {
    const u32 g = load<u32>(load<u32>(0x00F64C20) + 80);
    ctx.gpr[4] = g;  // r4 left
    if (g == 0) return;
    const u8 b = load<u8>(arg<u32>(ctx, 0) + 12);
    store<u8>(g + 2581, b);
    ret(ctx, b);  // r3 left as the byte
}
// void f(const Msg* m): if (g = (*(Obj**)0x00F64C20)->+80) g->+2581 (u8) = m->+9
DECOMP(00432184, Global_F64C20_Set2581_9) {
    const u32 g = load<u32>(load<u32>(0x00F64C20) + 80);
    ctx.gpr[4] = g;  // r4 left
    if (g == 0) return;
    const u8 b = load<u8>(arg<u32>(ctx, 0) + 9);
    store<u8>(g + 2581, b);
    ret(ctx, b);  // r3 left as the byte
}
// void f(const Msg* m): if (g = (*(Obj**)0x00F64C20)->+80) g->+2572 (f32) = m->+12
DECOMP(000891E4, Global_F64C20_Set2572_000891E4) {
    const u32 g = load<u32>(load<u32>(0x00F64C20) + 80);
    ctx.gpr[4] = g;  // r4 left
    if (g == 0) return;
    const u32 bits = load<u32>(arg<u32>(ctx, 0) + 12);
    store<u32>(g + 2572, bits);
    ctx.fpr[1] = std::bit_cast<f32>(bits);
}
// void f(const Msg* m): if (g = (*(Obj**)0x00F64C20)->+80) g->+2572 (f32) = m->+12
DECOMP(004384E8, Global_F64C20_Set2572_004384E8) {
    const u32 g = load<u32>(load<u32>(0x00F64C20) + 80);
    ctx.gpr[4] = g;  // r4 left
    if (g == 0) return;
    const u32 bits = load<u32>(arg<u32>(ctx, 0) + 12);
    store<u32>(g + 2572, bits);
    ctx.fpr[1] = std::bit_cast<f32>(bits);
}
// void f(const Msg* m): if (m->+12 & 1) *(f32*)0x0108562C = m->+16
DECOMP(00091CD0, Global_Set108562C) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 bit = load<u32>(m + 12) & 1u;
    ctx.gpr[4] = bit;  // r4 left
    if (bit == 0) return;
    const u32 bits = load<u32>(m + 16);
    store<u32>(0x0108562C, bits);
    ctx.gpr[4] = 0x01080000;
    ctx.fpr[1] = std::bit_cast<f32>(bits);
}
// s32 f(u32 x): x byte-swapped
DECOMP(000A96D8, Bswap32_000A96D8) {
    const u32 x = arg<u32>(ctx, 0);
    ctx.gpr[4] = (x >> 8) & 0xFFu;  // r4 / r6 left as bytes 1 and 2
    ctx.gpr[6] = (x >> 16) & 0xFFu;
    ret(ctx, static_cast<s32>(__builtin_bswap32(x)));
}
// void f(): if (!*(u8*)0x010321C6) it = 1 (a once flag)
DECOMP(000DCA00, Once_Set10321C6) {
    const u8 b = load<u8>(0x010321C6);
    ret(ctx, b);  // r3 left
    if (b != 0) return;
    store<u8>(0x010321C6, 1);
    ctx.gpr[3] = 0x01030000;
    ctx.gpr[4] = 1;
}
// void f(): if (!*(u8*)0x010BD6C0) it = 1 (a once flag)
DECOMP(004586E8, Once_Set10BD6C0) {
    const u8 b = load<u8>(0x010BD6C0);
    ret(ctx, b);  // r3 left
    if (b != 0) return;
    store<u8>(0x010BD6C0, 1);
    ctx.gpr[3] = 0x010C0000;
    ctx.gpr[4] = 1;
}
// u32 f(u32 i): the word at 0x00AC9A1C + i * 28 (a table of 28-byte records)
DECOMP(0012B630, Table_AC9A1C_Get) {
    const u64 i = ctx.gpr[3];
    ctx.gpr[4] = i << 5;  // r4 / r5 left
    ctx.gpr[5] = 0x00AC9A1C;
    ret(ctx, load<u32>(0x00AC9A1C + static_cast<u32>((i << 5) - (i << 2))));
}
// void f(Obj* o): o->+972 bits 0xC0000000 = both set when the top bit was clear, else both clear
DECOMP(001D3A00, Flag_Toggle972_001D3A00) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 v = load<u32>(o + 972);
    const u32 n = (v >> 31) != 0 ? 31u : 32u;  // cntlzw of the top bit
    const u32 w = (v & 0x3FFFFFFFu) | ((v >> 31) != 0 ? 0u : 0xC0000000u);
    store<u32>(o + 972, w);
    ctx.gpr[4] = w;  // r4 / r5 left
    ctx.gpr[5] = n;
}
// void f(f32 out[3], const Obj* o): out = o->+216->+320..+328
DECOMP(001EB0BC, Field_Get216_320v_001EB0BC) {
    const u32 out = arg<u32>(ctx, 0), p = load<u32>(arg<u32>(ctx, 1) + 216);
    u32 bits = 0;
    for (u32 k = 0; k < 3; ++k) {
        bits = load<u32>(p + 320 + 4 * k);
        store<u32>(out + 4 * k, bits);
    }
    ctx.gpr[4] = p;  // r4 left
    ret(ctx, std::bit_cast<f32>(bits));  // f1 left as the last value
}
// bool f(const Obj* o): bit 0 of the u64 at o->+184
DECOMP(00229A20, Field_Get184dBit0_00229A20) {
    const u32 bit = static_cast<u32>(load<u64>(arg<u32>(ctx, 0) + 184)) & 1u;
    ctx.gpr[4] = bit;  // r4 left
    ret(ctx, bit != 0 ? 1u : 0u);
}
// u32 f(const Obj* o): o->+364 (u16) ? *o->+356 : 0
DECOMP(002C6980, Field_Get356First_002C6980) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 v = load<u16>(o + 364) != 0 ? load<u32>(load<u32>(o + 356)) : 0u;
    ctx.gpr[4] = v;  // r4 left
    ret(ctx, v);
}
// void f(Obj* o, bool reset): if (reset) o->+41 (u8) = 0; o->+48 (f32) = o->+44
DECOMP(002FD640, Field_Reset41_48_002FD640) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = arg<u32>(ctx, 1) & 0xFFu;
    const u32 bits = load<u32>(o + 44);
    ctx.gpr[4] = b;
    if (b != 0) {
        store<u8>(o + 41, 0);
        ctx.gpr[4] = 0;
    }
    store<u32>(o + 48, bits);
    ctx.fpr[1] = std::bit_cast<f32>(bits);  // f1 left as the value
}
// void f(Obj* o, bool reset): if (reset) o->+41 (u8) = 0; o->+48 = 0
DECOMP(002FE210, Field_Reset41_48z_002FE210) {
    const u32 o = arg<u32>(ctx, 0);
    if ((arg<u32>(ctx, 1) & 0xFFu) != 0) store<u8>(o + 41, 0);
    store<u32>(o + 48, 0);
    ctx.gpr[4] = 0;  // r4 left 0
}
// s32 f(const Obj* o): o->+32 ? o->+32->+68 : 0
DECOMP(00353E88, Field_Get32_68_00353E88) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 32);
    ret(ctx, p != 0 ? load<s32>(p + 68) : 0);
}
// void f(Obj* o): o->+88 = o->+184 = 0
DECOMP(0039BEDC, Field_Clear88_184_0039BEDC) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 88, 0);
    store<u32>(o + 184, 0);
    ctx.gpr[4] = o + 88;  // r3 / r4 / r5 left
    ctx.gpr[5] = 0;
    ret(ctx, o + 184);
}
// void f(): if ((*(Node**)0x00F6585C)->+28) *(Node**)0x00F6585C = it (step to the next)
DECOMP(0042BC24, Global_StepF6585C) {
    const u32 next = load<u32>(load<u32>(0x00F6585C) + 28);
    ret(ctx, next);  // r3 left
    if (next == 0) return;
    store<u32>(0x00F6585C, next);
    ctx.gpr[4] = 0x00F60000;
}
// bool f(): (*(Obj**)0x00F64C38)->+44 (u16) == 0
DECOMP(0042D89C, Global_F64C38_Is44Zero) {
    const u32 v = load<u16>(load<u32>(0x00F64C38) + 44);
    ctx.gpr[4] = v;  // r4 left
    ret(ctx, v == 0 ? 1u : 0u);
}

// ---------------------------------------------------------------- batch 5
// const char* f(const Obj* o): o->+0x1281C0, or the string at 0x00AE65E4 when null
DECOMP(0047C46C, Field_Get1281C0OrDefault) {
    const u32 v = load<u32>(arg<u32>(ctx, 0) + 0x001281C0u);
    ctx.gpr[4] = 0x001281C0;  // r4 left as the offset
    ret(ctx, v != 0 ? v : 0x00AE65E4u);
}
// void f(): (*(u8**)0x00F65CA0)->+44: set 0x40, clear 0x20
DECOMP(0048A4C0, Global_F65CA0_Flag44) {
    const u32 g = load<u32>(0x00F65CA0);
    const u32 v = (load<u8>(g + 44) | 0x40u) & ~0x20u;
    store<u8>(g + 44, static_cast<u8>(v));
    ctx.gpr[4] = 0;  // r3 / r4 / r5 left
    ctx.gpr[5] = v;
    ret(ctx, g);
}
// u32 f(const Obj* o): o->+184 ? o->+192 : 0x00AEF4A0 (a default)
DECOMP(004DC6F4, Field_Get192OrDefault_004DC6F4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 k = load<u32>(o + 184);
    ctx.gpr[4] = k;  // r4 left
    ret(ctx, k != 0 ? load<u32>(o + 192) : 0x00AEF4A0u);
}
// f32 Vec3_Length(const f32 v[3]): sqrt((y * y + x * x) + z * z), fused
DECOMP(00734DAC, Vec3_Length_00734DAC) {
    const u32 v = arg<u32>(ctx, 0);
    const f64 x = load<f32>(v), y = load<f32>(v + 4), z = load<f32>(v + 8);
    f64 s = rt::to_single(y * y);
    s = rt::to_single(std::fma(x, x, s));
    s = rt::to_single(std::fma(z, z, s));
    ctx.fpr[2] = x;  // f2 / f3 left
    ctx.fpr[3] = z;
    ret(ctx, static_cast<f32>(std::sqrt(s)));
}
// void f(u32 p[6]): p[0..5] = 0
DECOMP(00734FC0, Words6_Zero_00734FC0) {
    const u32 p = arg<u32>(ctx, 0);
    for (u32 k = 0; k < 6; ++k) store<u32>(p + 4 * k, 0);
}
// void Vec4_SetXyzW(f32 d[4], const f32 s[3], f32 w)
DECOMP(00735960, Vec4_SetXyzW_00735960) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    store<u32>(d, load<u32>(s));
    store<u32>(d + 4, load<u32>(s + 4));
    const u32 z = load<u32>(s + 8);
    store<f32>(d + 12, static_cast<f32>(farg(ctx, 0)));
    store<u32>(d + 8, z);
    ctx.fpr[2] = std::bit_cast<f32>(z);  // f2 left as z
}
// void f(const Obj* o, u32* arr): arr[o->+16->+64] = o->+20
DECOMP(00736894, Field_Store20At_00736894) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 16);
    const u32 v = load<u32>(o + 20);
    const u32 off = load<u32>(a + 64) << 2;
    const u32 p = arg<u32>(ctx, 1) + off;
    store<u32>(p, v);
    ctx.gpr[4] = p;  // r3 / r4 / r5 left
    ctx.gpr[5] = off;
    ret(ctx, v);
}
// T* f(const u8* id): the record at 0x010B19E0 + id[1] * 11968, +176
DECOMP(00748FD0, Table_10B19E0_At_00748FD0) {
    ctx.gpr[4] = 0x010B19E0;  // r4 left as the table
    ret(ctx, 0x010B19E0u + load<u8>(arg<u32>(ctx, 0) + 1) * 11968u + 176u);
}
// u32 f(const Obj* o, s32 key): o->+276 == key ? o->+280 : 0
DECOMP(0074C68C, Field_Get280If276_0074C68C) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 key = ctx.gpr[4];
    const u32 k = load<u32>(o + 276);
    const u32 v = static_cast<s32>(k) == static_cast<s32>(key) ? load<u32>(o + 280) : 0u;
    ctx.gpr[4] = v;  // r4 / r5 / r6 left
    ctx.gpr[5] = key;
    ctx.gpr[6] = k;
    ret(ctx, v);
}
// T* f(Obj** o, u32 i): **o + i * 240
DECOMP(0074CAFC, Ptr_At240_0074CAFC) {
    const u64 i = ctx.gpr[4];
    ctx.gpr[5] = i << 8;  // r4 / r5 left (64-bit)
    ctx.gpr[4] = (i << 8) - (i << 4);
    ret(ctx, static_cast<u32>(load<u32>(load<u32>(arg<u32>(ctx, 0))) + ctx.gpr[4]));
}
// u32 f(u32 a, u32 b, u32 c, u32 d): a | b << 16 | c << 5 | d << 10 (a packed key)
DECOMP(0074E3E0, Pack_Key_0074E3E0) {
    const u32 b = arg<u32>(ctx, 1) << 16, c = arg<u32>(ctx, 2) << 5, d = arg<u32>(ctx, 3) << 10;
    ctx.gpr[4] = c | d;  // r4 / r5 / r6 left
    ctx.gpr[5] = c;
    ctx.gpr[6] = d;
    ret(ctx, arg<u32>(ctx, 0) | b | c | d);
}
// bool f(const Obj* o): o->+48 < o->+36 (u16), unsigned
DECOMP(0074E754, Field_Less48_36_0074E754) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = load<u32>(o + 48), cap = load<u16>(o + 36);
    ctx.gpr[4] = cap;  // r4 / r5 left
    ctx.gpr[5] = n;
    ret(ctx, n < cap ? 1u : 0u);
}
// f32 f(const Obj* o, u32 i, s32 j): ((f32*)(o + 80 + i * 8))[j]
DECOMP(00760AE0, Field_GetF80At_00760AE0) {
    const u64 i8 = ctx.gpr[4] << 3, j4 = ctx.gpr[5] << 2;
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(static_cast<s32>(j4)));  // r4 / r5 left
    ctx.gpr[5] = j4;
    const u32 row = static_cast<u32>(ctx.gpr[3] + i8 + 80);
    ret(ctx, load<f32>(static_cast<u32>(row + ctx.gpr[4])));
    ctx.gpr[3] = row;  // r3 left as the row
}
// bool Math_NearlyEqual(f32 a, f32 b, f32 eps): |a - b| < eps
DECOMP(00761770, Math_NearlyEqual_00761770) {
    const f64 d = std::fabs(rt::to_single(farg(ctx, 0) - farg(ctx, 1)));
    const bool lt = d < farg(ctx, 2);
    ret(ctx, d);  // f1 left as |a - b|
    ret(ctx, lt ? 1u : 0u);
}
// u32 f(const Obj* o, s32 i): o->+0x1281E4[i]
DECOMP(0077CE9C, Field_Get1281E4At_0077CE9C) {
    const u32 off = arg<u32>(ctx, 1) << 2;
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(static_cast<s32>(off)));  // r4 / r5 left
    ctx.gpr[5] = 0x001281E4;
    ret(ctx, load<u32>(arg<u32>(ctx, 0) + 0x001281E4u + off));
}
// void f(Obj* o, const f32 v[3]): o->+176..+184 = v; r3 left as o + 176
DECOMP(00012E44, Field_Set176v_00012E44) {
    const u32 d = arg<u32>(ctx, 0) + 176, s = arg<u32>(ctx, 1);
    u32 bits = 0;
    for (u32 k = 0; k < 3; ++k) {
        bits = load<u32>(s + 4 * k);
        store<u32>(d + 4 * k, bits);
    }
    ret(ctx, std::bit_cast<f32>(bits));  // f1 left as the last value
    ctx.gpr[3] = d;
}
// u8 f(const Obj* o): o->+308->+12 indexed (u8, stride 4) by (*(T**)0x00F94854)->+64
DECOMP(00023F0C, Field_Get308ByF94854_00023F0C) {
    const u32 tbl = load<u32>(load<u32>(arg<u32>(ctx, 0) + 308) + 12);
    const u32 off = load<u32>(load<u32>(0x00F94854) + 64) << 2;
    ctx.gpr[4] = off;  // r4 left
    ret(ctx, load<u8>(tbl + off));
}
// u8 f(const Obj* o): the same with the index from (*(T**)0x00F94B84)->+64
DECOMP(00738FC8, Field_Get308ByF94B84_00738FC8) {
    const u32 tbl = load<u32>(load<u32>(arg<u32>(ctx, 0) + 308) + 12);
    const u32 off = load<u32>(load<u32>(0x00F94B84) + 64) << 2;
    ctx.gpr[4] = off;  // r4 left
    ret(ctx, load<u8>(tbl + off));
}
// f32 f(const Obj* o): the same with f32 and the index from (*(T**)0x00F94A1C)->+64
DECOMP(00738B20, Field_Get308ByF94A1C_00738B20) {
    const u32 tbl = load<u32>(load<u32>(arg<u32>(ctx, 0) + 308) + 12);
    const u32 off = load<u32>(load<u32>(0x00F94A1C) + 64) << 2;
    ctx.gpr[4] = off;  // r3 / r4 left
    ctx.gpr[3] = tbl;
    ret(ctx, load<f32>(tbl + off));
}
// u32 f(const Obj* o): o->+48->+12 indexed by (*(T**)0x010AB008)->+64
DECOMP(00061D9C, Field_Get48By10AB008_00061D9C) {
    const u32 tbl = load<u32>(load<u32>(arg<u32>(ctx, 0) + 48) + 12);
    const u32 off = load<u32>(load<u32>(0x010AB008) + 64) << 2;
    ctx.gpr[4] = off;  // r4 left
    ret(ctx, load<u32>(tbl + off));
}
// bool f(const Obj* o): 48 < o->+4 <= 52
DECOMP(00032770, Field_Is4In49_52_00032770) {
    const s32 v = load<s32>(arg<u32>(ctx, 0) + 4);
    ret(ctx, (v > 52 || !(v > 48)) ? 0u : 1u);
}
// f(): sub_000CE110(1)
DECOMP(000CE5B0, Call_CE110_1) { call(ctx, 0x000CE110, {1}); }
// f(a, b): sub_00260D70(a, b, 1)
DECOMP(00261304, Call_260D70_1) { call(ctx, 0x00260D70, {arg<u64>(ctx, 0), arg<u64>(ctx, 1), 1}); }
// f(Obj* o): sub_002F466C(o, o->+12)
DECOMP(002F47C4, Call_2F466C_12) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x002F466C, {ctx.gpr[3], load<u32>(o + 12)});
}
// f(a, b): sub_00470750(a, b, 0)
DECOMP(00472208, Call_470750_0) { call(ctx, 0x00470750, {arg<u64>(ctx, 0), arg<u64>(ctx, 1), 0}); }
// u32* f(const Obj* o): o->+160 ? o->+160 + 16 : 0x01164BD0 (a default)
DECOMP(000DD880, Field_Get160Body_000DD880) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 160);
    ret(ctx, p != 0 ? p + 16 : 0x01164BD0u);
}
// void f(Obj* o): o->+2540 = o->+2552 = 0; if (o->+2544) o->+2548 (f32) = o->+2544->+4
DECOMP(00325E4C, Field_Reset2540_00325E4C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 2544);
    store<u32>(o + 2540, 0);
    store<u32>(o + 2552, 0);
    ctx.gpr[4] = p;  // r4 / r5 left
    ctx.gpr[5] = 0;
    if (p == 0) return;
    const u32 bits = load<u32>(p + 4);
    store<u32>(o + 2548, bits);
    ctx.fpr[1] = std::bit_cast<f32>(bits);
}
// f32 f(const Obj* o): o->+32 ? o->+32->+64 : 0.0f
DECOMP(003541B0, Field_Get32_64f_003541B0) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 32);
    if (p != 0) {
        ret(ctx, p);
        ret(ctx, load<f32>(p + 64));
    } else {
        ret(ctx, 0x003541ACu);  // r3 left as the constant's address
        ret(ctx, 0.0f);          // the 0.0f stored in .text at 0x003541AC
    }
}
// void f(Obj* o, f32 x): o->+72 (u8) = (x != 0.0f)
DECOMP(00736028, Field_SetNonzero72_00736028) {
    const bool nz = !(farg(ctx, 0) == 0.0);  // fcmpu against the 0.0f at 0x00736020: NaN counts as nonzero
    store<u8>(arg<u32>(ctx, 0) + 72, nz ? 1 : 0);
    ctx.gpr[4] = nz ? 1 : 0;  // r4 / f2 left
    ctx.fpr[2] = 0.0;
}
// void f(Obj* o, s32 i): o->+72 (f32) = i
DECOMP(007361C0, Field_SetFloat72_007361C0) {
    const s32 i = arg<s32>(ctx, 1);
    const f32 v = static_cast<f32>(static_cast<f64>(static_cast<s64>(i)));
    store<f32>(arg<u32>(ctx, 0) + 72, v);
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(i));  // r4 / f1 left
    ret(ctx, v);
}
// void f(Obj* o): o->+16 = 1.0f, o->+20 = o->+32 = -1, o->+64 = o->+68 = 0
DECOMP(00736A50, Field_Reset16_00736A50) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 16, 0x3F800000u);
    store<s32>(o + 20, -1);
    store<u32>(o + 64, 0);
    store<s32>(o + 32, -1);
    store<u32>(o + 68, 0);
    ctx.gpr[4] = 0x3F800000u;  // r4 / r5 / r6 left
    ctx.gpr[5] = ~0ull;
    ctx.gpr[6] = 0;
}
// void Vec2_AddInPlace(f32 a[2], const f32 b[2])
DECOMP(00742CA8, Vec2_AddInPlace_00742CA8) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const f32 x = static_cast<f32>(static_cast<f64>(load<f32>(a)) + load<f32>(b));
    const f32 ay = load<f32>(a + 4);
    store<f32>(a, x);
    const f32 y = static_cast<f32>(static_cast<f64>(ay) + load<f32>(b + 4));
    store<f32>(a + 4, y);
    ctx.fpr[2] = ay;  // f2 left
    ret(ctx, y);
}
// void f(Obj* o, bool on, ?, ?, u8 k, f32 x, f32 y, f32 z): o->+92..+100 = x, y, z;
// o->+88: bit 0x80000000 = on, bits 0x7F800000 = k
DECOMP(0074D1F8, Field_Set88_92v_0074D1F8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 88);
    store<f32>(o + 92, static_cast<f32>(farg(ctx, 0)));
    store<f32>(o + 96, static_cast<f32>(farg(ctx, 1)));
    store<f32>(o + 100, static_cast<f32>(farg(ctx, 2)));
    const u32 v = (w & 0x007FFFFFu) | (arg<u32>(ctx, 1) << 31) | ((arg<u32>(ctx, 4) << 23) & 0x7F800000u);
    store<u32>(o + 88, v);
    ctx.gpr[5] = v;  // r5 left
}
// void f(u8 d[4], const u8 s[4]): byte by byte
DECOMP(0074E218, Bytes4_Copy_0074E218) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    u8 b = 0;
    for (u32 k = 0; k < 4; ++k) {
        b = load<u8>(s + k);
        store<u8>(d + k, b);
    }
    ctx.gpr[4] = b;  // r4 / r5 left as the last two bytes
    ctx.gpr[5] = load<u8>(d + 2);
}
// void f(u8 d[4], const u8 s[4]): byte by byte
DECOMP(0074E294, Bytes4_Copy_0074E294) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    u8 b = 0;
    for (u32 k = 0; k < 4; ++k) {
        b = load<u8>(s + k);
        store<u8>(d + k, b);
    }
    ctx.gpr[4] = b;  // r4 / r5 left as the last two bytes
    ctx.gpr[5] = load<u8>(d + 2);
}
// void Vec2_Add(f32 out[2], const f32 a[2], const f32 b[2])
DECOMP(00755D88, Vec2_Add_00755D88) {
    const u32 a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const f64 ax = load<f32>(a), bx = load<f32>(b), ay = load<f32>(a + 4), by = load<f32>(b + 4);
    const f32 x = static_cast<f32>(ax + bx), y = static_cast<f32>(ay + by);
    store<f32>(arg<u32>(ctx, 0), x);
    store<f32>(arg<u32>(ctx, 0) + 4, y);
    ctx.fpr[2] = y;  // f2..f4 left
    ctx.fpr[3] = ay;
    ctx.fpr[4] = by;
    ret(ctx, x);
}

// ---------------------------------------------------------------- batch 6
// void f(Sub* s): the outer object s - 8: +8 = 0x00B194F8, +0 = 0x00B13974 (vtables)
DECOMP(0075A778, Obj_InitVt_B13974_B194F8_Outer) {
    const u32 o = arg<u32>(ctx, 0) - 8;
    store<u32>(o + 8, 0x00B194F8);
    store<u32>(o, 0x00B13974);
    ctx.gpr[4] = 0x00B194F8;  // r3 / r4 / r5 left
    ctx.gpr[5] = 0x00B13974;
    ret(ctx, o);
}
// void f(Obj* o): the 48 bytes at o + 32 = 0 (three stvx)
DECOMP(007633A8, Field_Zero32x48_007633A8) {
    const u32 p = arg<u32>(ctx, 0) + 32;
    for (u32 k = 0; k < 3; ++k) VSt(p + 16 * k, V4{{0, 0, 0, 0}});
    ctx.gpr[4] = 16;  // r3 / r4 / r5 left
    ctx.gpr[5] = 32;
    ret(ctx, p);
}
// T* f(Obj* o): o->+18 (u8) == 2 ? 0x00B1E268 : o + 28
DECOMP(00769A34, Field_Body28Or_00769A34) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 k = load<u8>(o + 18);
    ctx.gpr[4] = k;  // r4 left
    ret(ctx, k == 2 ? 0x00B1E268u : o + 28);
}
// T* f(Obj* o): o->+18 (u8) == 2 ? 0x00B1E26C : o + 156
DECOMP(00769A58, Field_Body156Or_00769A58) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 k = load<u8>(o + 18);
    ctx.gpr[4] = k;  // r4 left
    ret(ctx, k == 2 ? 0x00B1E26Cu : o + 156);
}
// void f(Vec4 d[3], const Vec4 s[3]) (three lvx / stvx, interleaved)
DECOMP(0076F510, Vec4x3_Copy_0076F510) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    for (u32 k = 0; k < 3; ++k) VSt(d + 16 * k, VLd(s + 16 * k));
    ctx.gpr[5] = 16;  // r5 / r6 left
    ctx.gpr[6] = 32;
}
// void f(Obj* o, const f32 v[3]): o->+64..+72 = v, o->+80 (u8) = 1
DECOMP(0077D0D8, Field_Set64v_80_0077D0D8) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    store<u32>(o + 64, load<u32>(v));
    const u32 y = load<u32>(v + 4);
    store<u8>(o + 80, 1);
    store<u32>(o + 68, y);
    const u32 z = load<u32>(v + 8);
    store<u32>(o + 72, z);
    ctx.gpr[5] = 1;  // r5 / f1 left
    ret(ctx, std::bit_cast<f32>(z));
}
// f32 f(const Obj* o): (f32)o->+20 (s32)
DECOMP(00793E28, Field_GetIntAsFloat20_00793E28) {
    const s32 i = load<s32>(arg<u32>(ctx, 0) + 20);
    ret(ctx, i);  // r3 left as the integer
    ret(ctx, static_cast<f32>(static_cast<f64>(static_cast<s64>(i))));
}
// f(const Msg* m): sub_0042C00C(m->+12)
DECOMP(00080B74, Call_42C00C_12) { call(ctx, 0x0042C00C, {load<u32>(arg<u32>(ctx, 0) + 12)}); }
// f(const Msg* m): sub_002AB428(f32 m->+12) (r3 still m)
DECOMP(000872C0, Call_2AB428_12f) { callf(ctx, 0x002AB428, {ctx.gpr[3]}, {load<f32>(arg<u32>(ctx, 0) + 12)}); }
// f(const Msg* m): sub_002AB5F8(f32 m->+12) (r3 still m)
DECOMP(0008C400, Call_2AB5F8_12f) { callf(ctx, 0x002AB5F8, {ctx.gpr[3]}, {load<f32>(arg<u32>(ctx, 0) + 12)}); }
// f(const Msg* m): sub_00449E10(u8 m->+12)
DECOMP(00096954, Call_449E10_12b) { call(ctx, 0x00449E10, {load<u8>(arg<u32>(ctx, 0) + 12)}); }
// void f(u32* out, const Obj* o): *out = 1; *(u32*)0x00F645B8 = o->+1624 + 75, *(u32*)0x00F645BC = o->+1624 + 78
DECOMP(000B1E44, Global_SetF645B8_000B1E44) {
    const u32 v = load<u32>(arg<u32>(ctx, 1) + 1624);
    store<u32>(arg<u32>(ctx, 0), 1);
    store<u32>(0x00F645B8, v + 75);
    store<u32>(0x00F645BC, v + 78);
    ctx.gpr[4] = v + 78;  // r3..r7 left
    ctx.gpr[5] = ctx.gpr[6] = 0x00F60000;
    ctx.gpr[7] = 1;
    ret(ctx, v + 75);
}
// f(?, x): sub_0034A398(x)
DECOMP(000DBCF8, Call_34A398_Arg1) { call(ctx, 0x0034A398, {ctx.gpr[4], ctx.gpr[4]}); }
// void f(f32 out[4], const Obj* o): out = o->+216->+336..+348
DECOMP(001EB0DC, Field_Get216_336q_001EB0DC) {
    const u32 out = arg<u32>(ctx, 0), p = load<u32>(arg<u32>(ctx, 1) + 216);
    u32 bits = 0;
    for (u32 k = 0; k < 4; ++k) {
        bits = load<u32>(p + 336 + 4 * k);
        store<u32>(out + 4 * k, bits);
    }
    ctx.gpr[4] = p;  // r4 left
    ret(ctx, std::bit_cast<f32>(bits));  // f1 left as the last value
}
// void f(Obj* o, u32 i): ++(u8)o->(i * 112 + 122)
DECOMP(002540E0, Field_Inc122At_002540E0) {
    const u64 i = ctx.gpr[4];
    const u32 p = static_cast<u32>(ctx.gpr[3] + (i << 7) - (i << 4) + 122);
    const u32 v = load<u8>(p) + 1;
    store<u8>(p, static_cast<u8>(v));
    ctx.gpr[4] = v;  // r3 / r4 / r5 left
    ctx.gpr[5] = i << 7;
    ret(ctx, p);
}
// void f(Obj* o, u32 i): --(u8)o->(i * 112 + 122)
DECOMP(00254140, Field_Dec122At_00254140) {
    const u64 i = ctx.gpr[4];
    const u32 p = static_cast<u32>(ctx.gpr[3] + (i << 7) - (i << 4) + 122);
    const u64 v = static_cast<u64>(load<u8>(p)) - 1;
    store<u8>(p, static_cast<u8>(v));
    ctx.gpr[4] = v;  // r3 / r4 / r5 left
    ctx.gpr[5] = i << 7;
    ret(ctx, p);
}
// void f(f32 out[3], const Obj* o): out = {0, o->+84 * 0.5, 0}
DECOMP(002BC1E0, Vec3_SetHalfY84_002BC1E0) {
    const u32 out = arg<u32>(ctx, 0);
    const f64 y = load<f32>(arg<u32>(ctx, 1) + 84);
    store<u32>(out, 0);
    const f32 h = static_cast<f32>(y * 0.5);  // the 0.5f at 0x002BC1D8
    store<u32>(out + 8, 0);
    store<f32>(out + 4, h);
    ctx.gpr[4] = 0;  // r4 / r5 / f2 left
    ctx.gpr[5] = 0x002BC1D8;
    ctx.fpr[2] = 0.5;
    ret(ctx, h);
}
// void f(const Obj* o): p = o->+32: p->+992..+1000 = p->+432..+440
DECOMP(003124FC, Field_Copy432To992_003124FC) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 32);
    u32 bits = 0;
    for (u32 k = 0; k < 3; ++k) {
        bits = load<u32>(p + 432 + 4 * k);
        store<u32>(p + 992 + 4 * k, bits);
    }
    ctx.gpr[4] = p + 432;  // r3 / r4 / f1 left
    ret(ctx, p);
    ret(ctx, std::bit_cast<f32>(bits));
}
// void f(const Obj* o): p = o->+32: p->+992..+1000 = p->+432..+440
DECOMP(00315788, Field_Copy432To992_00315788) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 32);
    u32 bits = 0;
    for (u32 k = 0; k < 3; ++k) {
        bits = load<u32>(p + 432 + 4 * k);
        store<u32>(p + 992 + 4 * k, bits);
    }
    ctx.gpr[4] = p + 432;  // r3 / r4 / f1 left
    ret(ctx, p);
    ret(ctx, std::bit_cast<f32>(bits));
}
// bool f(const Obj* o, Rec* out): if (o->+2204) { out->+0 = it, out->+4 (u8) = o->+2558; 1 } else 0
DECOMP(00325DD0, Field_Get2204_00325DD0) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const u32 v = load<u32>(o + 2204);
    ctx.gpr[5] = v;  // r5 / r6 left
    if (v == 0) {
        ret(ctx, 0u);
        return;
    }
    const u8 b = load<u8>(o + 2558);
    store<u32>(out, v);
    store<u8>(out + 4, b);
    ctx.gpr[6] = b;
    ret(ctx, 1u);
}
// f(a, b, u8 c): sub_003260F0(a, b, c)
DECOMP(0032EA40, Call_3260F0_0032EA40) { call(ctx, 0x003260F0, {ctx.gpr[3], ctx.gpr[4], ctx.gpr[5] & 0xFF}); }
// f(a, b, u8 c): sub_003260F0(a, b, c)
DECOMP(00340990, Call_3260F0_00340990) { call(ctx, 0x003260F0, {ctx.gpr[3], ctx.gpr[4], ctx.gpr[5] & 0xFF}); }
// void f(bool* out, const Obj* o): *out = o->+8 && o->+24 (u8)
DECOMP(00347B04, Field_Is8And24_00347B04) {
    const u32 o = arg<u32>(ctx, 1);
    const u32 p = load<u32>(o + 8);
    ctx.gpr[6] = p;  // r4 / r5 / r6 left
    u32 r = 0;
    if (p != 0) {
        const u32 b = load<u8>(o + 24);
        ctx.gpr[4] = b;
        if (b != 0) r = 1;
    }
    ctx.gpr[5] = r;
    store<u8>(arg<u32>(ctx, 0), static_cast<u8>(r));
}
// void f(): the list head at 0x01089D08: +44 = +40; +40 = +40 == head ? head + 20 : head
DECOMP(003DDA50, List_Reset1089D08) {
    constexpr u32 head = 0x01089D08;
    const u32 first = load<u32>(head + 40);
    store<u32>(head + 44, first);
    const u32 v = first == head ? head + 20 : head;
    store<u32>(head + 40, v);
    ctx.gpr[4] = v;  // r3 / r4 / r5 left
    ctx.gpr[5] = first;
    ret(ctx, head);
}
// void f(bool on, u16 a, u16 b, u16 c): the block at 0x010AA1B0: +92 = c, +94 = b,
// +88: bit 0x80000000 = on, bits 0x7FFF8000 = a
DECOMP(0042A218, Global_Set10AA1B0) {
    constexpr u32 g = 0x010AA1B0;
    const u32 w = load<u32>(g + 88);
    store<u16>(g + 92, static_cast<u16>(arg<u32>(ctx, 3)));
    store<u16>(g + 94, static_cast<u16>(arg<u32>(ctx, 2)));
    const u32 v = (w & 0x00007FFFu) | (arg<u32>(ctx, 0) << 31) | ((arg<u32>(ctx, 1) << 15) & 0x7FFF8000u);
    store<u32>(g + 88, v);
    ctx.gpr[7] = g;  // r7 / r8 left
    ctx.gpr[8] = v;
}
// f(a, b, c, d, e): sub_00446EE4(a, b, c, d, 0, e)
DECOMP(0044BD4C, Call_446EE4_0044BD4C) {
    call(ctx, 0x00446EE4, {ctx.gpr[3], ctx.gpr[4], ctx.gpr[5], ctx.gpr[6], 0, ctx.gpr[7]});
}
// f(?, u8 x): sub_00369658(x)
DECOMP(0047BC40, Call_369658_0047BC40) { call(ctx, 0x00369658, {ctx.gpr[4] & 0xFF}); }
// f(?, x): sub_00369BC8(x)
DECOMP(0047C1F0, Call_369BC8_0047C1F0) { call(ctx, 0x00369BC8, {ctx.gpr[4]}); }
// f(Obj* o): sub_00479400(o->+8, o)
DECOMP(0048B308, Call_479400_0048B308) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x00479400, {load<u32>(o + 8), o});
}
// void f(Obj* o): o->+20[o->+6 (u16)] (u8) = 1, o->+44 (u8) |= 1
DECOMP(0048C7DC, Field_Mark20At6Set_0048C7DC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 i = load<u16>(o + 6);
    const u32 p = load<u32>(o + 20) + i;
    store<u8>(p, 1);
    const u32 v = load<u8>(o + 44) | 1u;
    store<u8>(o + 44, static_cast<u8>(v));
    ctx.gpr[4] = v;  // r4 / r5 / r6 left
    ctx.gpr[5] = p;
    ctx.gpr[6] = i;
}
// void f(Obj* o): o->+20[o->+6 (u16)] (u8) = 1, o->+44 (u8) &= ~1
DECOMP(0048C804, Field_Mark20At6Clear_0048C804) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 i = load<u16>(o + 6);
    const u32 p = load<u32>(o + 20) + i;
    store<u8>(p, 1);
    const u32 v = load<u8>(o + 44) & ~1u;
    store<u8>(o + 44, static_cast<u8>(v));
    ctx.gpr[4] = v;  // r4 / r5 / r6 left
    ctx.gpr[5] = p;
    ctx.gpr[6] = i;
}
// f(a..f, u8 g, u8 h): sub_004F37B0(a..h)
DECOMP(004F3C88, Call_4F37B0_004F3C88) {
    call(ctx, 0x004F37B0, {ctx.gpr[3], ctx.gpr[4], ctx.gpr[5], ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], ctx.gpr[9] & 0xFF, ctx.gpr[10] & 0xFF});
}
// f32 Vec3_Dot(const f32 a[3], const f32 b[3]): (ay * by + ax * bx) + az * bz, fused
DECOMP(00734B10, Vec3_Dot_00734B10) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const f64 ax = load<f32>(a), ay = load<f32>(a + 4), az = load<f32>(a + 8);
    const f64 bx = load<f32>(b), by = load<f32>(b + 4), bz = load<f32>(b + 8);
    f64 s = rt::to_single(ay * by);
    s = rt::to_single(std::fma(ax, bx, s));
    ctx.fpr[2] = bx;  // f2..f5 left
    ctx.fpr[3] = ax;
    ctx.fpr[4] = az;
    ctx.fpr[5] = bz;
    ret(ctx, static_cast<f32>(std::fma(az, bz, s)));
}
// void Vec3_DivInPlace(f32 v[3], f32 s)
DECOMP(00734DF8, Vec3_DivInPlace_00734DF8) {
    const u32 v = arg<u32>(ctx, 0);
    const f64 s = farg(ctx, 0);
    const f64 z = load<f32>(v + 8);
    const f32 x = static_cast<f32>(load<f32>(v) / s), y = static_cast<f32>(load<f32>(v + 4) / s), rz = static_cast<f32>(z / s);
    store<f32>(v, x);
    store<f32>(v + 4, y);
    store<f32>(v + 8, rz);
    ctx.fpr[2] = x;  // f2..f4 left
    ctx.fpr[3] = y;
    ctx.fpr[4] = z;
    ret(ctx, rz);
}
// void Vec3_Scale(f32 out[3], const f32 in[3], f32 s)
DECOMP(00734E6C, Vec3_Scale_00734E6C) {
    const u32 in = arg<u32>(ctx, 1);
    const f64 s = farg(ctx, 0);
    const f64 z = load<f32>(in + 8);
    const f32 x = static_cast<f32>(load<f32>(in) * s), y = static_cast<f32>(load<f32>(in + 4) * s), rz = static_cast<f32>(z * s);
    const u32 out = arg<u32>(ctx, 0);
    store<f32>(out, x);
    store<f32>(out + 4, y);
    store<f32>(out + 8, rz);
    ctx.fpr[2] = x;  // f2..f4 left
    ctx.fpr[3] = y;
    ctx.fpr[4] = z;
    ret(ctx, rz);
}
// void Vec3_ScaleInPlace(f32 v[3], f32 s)
DECOMP(007359E4, Vec3_ScaleInPlace_007359E4) {
    const u32 v = arg<u32>(ctx, 0);
    const f64 s = farg(ctx, 0);
    const f64 z = load<f32>(v + 8);
    const f32 x = static_cast<f32>(load<f32>(v) * s), y = static_cast<f32>(load<f32>(v + 4) * s), rz = static_cast<f32>(z * s);
    store<f32>(v, x);
    store<f32>(v + 4, y);
    store<f32>(v + 8, rz);
    ctx.fpr[2] = x;  // f2..f4 left
    ctx.fpr[3] = y;
    ctx.fpr[4] = z;
    ret(ctx, rz);
}
// void f(Obj* o, f32 x): o->+192 = max(x, 0.1f) (fsel against the constants at 0x00735AD0)
DECOMP(00735AE0, Field_SetMin01_192_00735AE0) {
    const f64 x = farg(ctx, 0);
    const f64 lo = std::bit_cast<f64>(0x3FB99999A0000000ull);  // 0.1f as a double, at 0x00735AD8
    const f64 d = rt::to_single(x - static_cast<f64>(std::bit_cast<f32>(0x3DCCCCCDu)));
    const f32 v = static_cast<f32>(d >= 0.0 ? x : lo);
    store<f32>(arg<u32>(ctx, 0) + 192, v);
    ctx.gpr[4] = 0x00735AD0;  // r4 / f2..f4 left
    ctx.fpr[2] = x;
    ctx.fpr[3] = std::bit_cast<f32>(0x3DCCCCCDu);
    ctx.fpr[4] = lo;
    ret(ctx, v);
}
// void f(const Obj* o, u32* arr): arr[o->+16->+64] = o->+20
DECOMP(007366CC, Field_Store20At_007366CC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 v = load<u32>(o + 20);
    const u32 p = arg<u32>(ctx, 1) + (load<u32>(load<u32>(o + 16) + 64) << 2);
    store<u32>(p, v);
    ctx.gpr[5] = v;  // r3 / r5 left
    ret(ctx, p);
}
// bool f(const Obj* o): o->+4 is 10 or 11
DECOMP(00738A2C, Field_Is4In10_11_00738A2C) {
    const s32 v = load<s32>(arg<u32>(ctx, 0) + 4);
    ctx.gpr[4] = static_cast<u32>(v);  // r4 left
    ret(ctx, (v == 10 || v == 11) ? 1u : 0u);
}

// ---------------------------------------------------------------- batch 7
// bool f(const Obj* o): o->+348 is 1 or 3
DECOMP(00738CE0, Field_Is348In1_3_00738CE0) {
    const s32 v = load<s32>(arg<u32>(ctx, 0) + 348);
    ctx.gpr[4] = static_cast<u32>(v);  // r4 left
    ret(ctx, (v == 1 || v == 3) ? 1u : 0u);
}
// bool f(const Obj* o): o->+424 > 0.0f (the constant at 0x00739108)
DECOMP(00739110, Field_Pos424_00739110) {
    const f64 v = load<f32>(arg<u32>(ctx, 0) + 424);
    ctx.gpr[4] = 0x00739108;  // r4 / f1 / f2 left
    ctx.fpr[1] = 0.0;
    ctx.fpr[2] = v;
    ret(ctx, v > 0.0 ? 1u : 0u);
}
// void f(u32* p): *p byte-swapped in place
DECOMP(00742D28, U32_SwapInPlace_00742D28) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 b3 = load<u8>(p + 3), b2 = load<u8>(p + 2), b1 = load<u8>(p + 1), b0 = load<u8>(p);
    const u32 v = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
    store<u32>(p, v);
    ctx.gpr[4] = v;  // r4..r7 left
    ctx.gpr[5] = b2;
    ctx.gpr[6] = b1;
    ctx.gpr[7] = b0;
}
// bool f(const Obj* o): o->+216->+208 != 0x0024A738 (a hash)
DECOMP(00747B40, Field_216_208Not24A738) {
    const u32 v = load<u32>(load<u32>(arg<u32>(ctx, 0) + 216) + 208);
    ctx.gpr[4] = v;  // r4 / r5 left
    ctx.gpr[5] = 0x0024A738;
    ret(ctx, v == 0x0024A738u ? 0u : 1u);
}
// bool f(const Obj* o): o->+216->+208 == 0xA970EC1F (a hash)
DECOMP(00747B68, Field_216_208IsA970EC1F) {
    const u32 v = load<u32>(load<u32>(arg<u32>(ctx, 0) + 216) + 208);
    ctx.gpr[4] = v;  // r4 / r5 left
    ctx.gpr[5] = 0xFFFFFFFFA970EC1Full;
    ret(ctx, v == 0xA970EC1Fu ? 1u : 0u);
}
// bool f(const Obj* o): o->+216->+284 != 0x0024A738 (a hash)
DECOMP(007585DC, Field_216_284Not24A738) {
    const u32 v = load<u32>(load<u32>(arg<u32>(ctx, 0) + 216) + 284);
    ctx.gpr[4] = v;  // r4 / r5 left
    ctx.gpr[5] = 0x0024A738;
    ret(ctx, v == 0x0024A738u ? 0u : 1u);
}
// bool f(const Obj* o): o->+216->+284 == 0xA970EC1F (a hash)
DECOMP(00758604, Field_216_284IsA970EC1F) {
    const u32 v = load<u32>(load<u32>(arg<u32>(ctx, 0) + 216) + 284);
    ctx.gpr[4] = v;  // r4 / r5 left
    ctx.gpr[5] = 0xFFFFFFFFA970EC1Full;
    ret(ctx, v == 0xA970EC1Fu ? 1u : 0u);
}
// void Vec3_Neg(f32 out[3], const f32 in[3]) (sign bits flipped)
DECOMP(0074B6B0, Vec3_Neg_0074B6B0) {
    const u32 out = arg<u32>(ctx, 0), in = arg<u32>(ctx, 1);
    u32 n[3];
    for (u32 k = 0; k < 3; ++k) n[k] = load<u32>(in + 4 * k) ^ 0x80000000u;
    for (u32 k = 0; k < 3; ++k) {
        store<u32>(out + 4 * k, n[k]);
        ctx.fpr[1 + k] = std::bit_cast<f32>(n[k]);  // f1..f3 left
    }
}
// void Vec3_ScaleInPlace(f32 v[3], f32 s)
DECOMP(00755E08, Vec3_ScaleInPlace_00755E08) {
    const u32 v = arg<u32>(ctx, 0);
    const f64 s = farg(ctx, 0);
    const f64 z = load<f32>(v + 8);
    const f32 x = static_cast<f32>(load<f32>(v) * s), y = static_cast<f32>(load<f32>(v + 4) * s), rz = static_cast<f32>(z * s);
    store<f32>(v, x);
    store<f32>(v + 4, y);
    store<f32>(v + 8, rz);
    ctx.fpr[2] = x;  // f2..f4 left
    ctx.fpr[3] = y;
    ctx.fpr[4] = z;
    ret(ctx, rz);
}
// void f(f32 out[3], const Obj* o): out = o->+160->+64..+72
DECOMP(0075D624, Field_Get160_64v_0075D624) {
    const u32 out = arg<u32>(ctx, 0), p = load<u32>(arg<u32>(ctx, 1) + 160) + 64;
    u32 bits = 0;
    for (u32 k = 0; k < 3; ++k) {
        bits = load<u32>(p + 4 * k);
        store<u32>(out + 4 * k, bits);
    }
    ctx.gpr[4] = p;  // r4 left
    ret(ctx, std::bit_cast<f32>(bits));  // f1 left as the last value
}
// void f(f32 out[3], const Obj* o): out = o->+96->+144..+152
DECOMP(0077AEE0, Field_Get96_144v_0077AEE0) {
    const u32 out = arg<u32>(ctx, 0), p = load<u32>(arg<u32>(ctx, 1) + 96) + 144;
    u32 bits = 0;
    for (u32 k = 0; k < 3; ++k) {
        bits = load<u32>(p + 4 * k);
        store<u32>(out + 4 * k, bits);
    }
    ctx.gpr[4] = p;  // r4 left
    ret(ctx, std::bit_cast<f32>(bits));  // f1 left as the last value
}
// bool f(const u8* bits, u32 n): *bits & (1 << n) (slw: 0 for n >= 32)
DECOMP(007613EC, Bits8_Test_007613EC) {
    const u32 n = arg<u32>(ctx, 1) & 63;
    const u32 m = n < 32 ? (1u << n) : 0u;
    ctx.gpr[4] = m;  // r4 / r5 left
    ctx.gpr[5] = 1;
    ret(ctx, (load<u8>(arg<u32>(ctx, 0)) & m) != 0 ? 1u : 0u);
}
// void f(Obj* o): o->+12 = 0x00B1F4C0, o->+8 = 0x00B1F4E0, o->+0 = 0x00B13974 (vtables)
DECOMP(0076EB78, Obj_InitVt_B13974_0076EB78) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 12, 0x00B1F4C0);
    store<u32>(o + 8, 0x00B1F4E0);
    store<u32>(o, 0x00B13974);
    ctx.gpr[4] = 0x00B1F4C0;  // r4..r6 left
    ctx.gpr[5] = 0x00B1F4E0;
    ctx.gpr[6] = 0x00B13974;
}
// s32 f(const s32* a, const s32* b): *a > *b ? 1 : *a < *b ? -1 : 0 (a qsort comparator)
DECOMP(00771B4C, Cmp_S32_00771B4C) {
    const s32 a = load<s32>(arg<u32>(ctx, 0)), b = load<s32>(arg<u32>(ctx, 1));
    ctx.gpr[4] = static_cast<u32>(b);  // r4 / r5 left
    ctx.gpr[5] = static_cast<u32>(a);
    ret(ctx, a > b ? 1 : (a < b ? -1 : 0));
}
// bool f(const Obj* o): 8 <= ((o->+4 >> 10) & 63) <= 14
DECOMP(0077B0D8, Field_Kind4In8_14_0077B0D8) {
    const s32 k = static_cast<s32>((load<u32>(arg<u32>(ctx, 0) + 4) >> 10) & 63u);
    ctx.gpr[4] = static_cast<u32>(k);  // r4 left
    ret(ctx, (k < 8 || k > 14) ? 0u : 1u);
}
// void f(Obj* o, u32 v): o->+132 = v (big-endian, byte by byte)
DECOMP(00796480, Field_SetBytes132_00796480) {
    const u32 p = arg<u32>(ctx, 0) + 132, v = arg<u32>(ctx, 1);
    store<u8>(p, static_cast<u8>(v >> 24));
    store<u8>(p + 1, static_cast<u8>(v >> 16));
    store<u8>(p + 2, static_cast<u8>(v >> 8));
    store<u8>(p + 3, static_cast<u8>(v));
    ctx.gpr[5] = v >> 24;  // r3 / r5..r7 left
    ctx.gpr[6] = (v >> 16) & 0xFFu;
    ctx.gpr[7] = (v >> 8) & 0xFFu;
    ret(ctx, p);
}
// bool f(const Obj* o): o->+68 && ((o->+68->+416 >> 28) & 7) == 1
DECOMP(00014D74, Field_68Kind1_00014D74) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 68);
    ctx.gpr[4] = (p != 0 && ((load<u32>(p + 416) >> 28) & 7u) == 1) ? 1u : 0u;
    ret(ctx, ctx.gpr[4]);
}
// f(): sub_00489EAC(*(T**)0x00F65CA0)
DECOMP(000817D4, Call_489EAC_F65CA0) { call(ctx, 0x00489EAC, {load<u32>(0x00F65CA0)}); }
// u8 f(const Obj* o): p = o->+160: p && ((p->+4 >> 10) & 63) == 8 ? p->+168 (u8) : 0
DECOMP(000DA31C, Field_160Kind8_168_000DA31C) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 160);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 k = (load<u32>(p + 4) >> 10) & 63u;
    ctx.gpr[4] = k;  // r4 left
    ret(ctx, k == 8 ? static_cast<u32>(load<u8>(p + 168)) : 0u);
}
// const char* f(Obj* o): o->+1416 if a non-empty string, else o + 56
DECOMP(0011EFDC, Field_Name1416Or56_0011EFDC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = load<u32>(o + 1416);
    ctx.gpr[4] = s;  // r4 / r5 left
    if (s != 0) {
        const u8 c = load<u8>(s);
        ctx.gpr[5] = c;
        if (c != 0) {
            ret(ctx, s);
            return;
        }
    }
    ret(ctx, o + 56);
}
// const char* f(Obj* o): o->+736 if a non-empty string, else o + 56
DECOMP(00226F70, Field_Name736Or56_00226F70) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = load<u32>(o + 736);
    ctx.gpr[4] = s;  // r4 / r5 left
    if (s != 0) {
        const u8 c = load<u8>(s);
        ctx.gpr[5] = c;
        if (c != 0) {
            ret(ctx, s);
            return;
        }
    }
    ret(ctx, o + 56);
}
// f(Obj* o, ?, c): sub_001AF470(o, o->+548, c, 1, f32 o->+544)
DECOMP(001AF56C, Call_1AF470_001AF56C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 v = load<u32>(o + 548);
    callf(ctx, 0x001AF470, {o, v, ctx.gpr[5], 1}, {load<f32>(o + 544)});
}
// bool f(const Obj* o): o->+404 && (o->+404->+164 bit 0x8000)
DECOMP(001BD4A0, Field_404Bit15_001BD4A0) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 404);
    ret(ctx, p != 0 ? (load<u32>(p + 164) >> 15) & 1u : 0u);
}
// u32 f(const Obj* o): o->+388->+8 ? o->+388->+44 >> 16 : o->+400 & 0xFFFFFF
DECOMP(002070E8, Field_Id388Or400_002070E8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 388);
    u32 v = load<u32>(o + 400) & 0x00FFFFFFu;
    const u32 k = load<u32>(a + 8);
    if (k != 0) v = load<u32>(a + 44) >> 16;
    ctx.gpr[4] = v;  // r4 / r5 left
    ctx.gpr[5] = k;
    ret(ctx, v);
}
// f(a, ...): sub_0028EBB8(a, ..., 0.0f) (the constant at 0x0028EE64)
DECOMP(0028EE68, Call_28EBB8_0) {
    ctx.gpr[4] = 0x0028EE64;  // r4 holds the constant's address in the call
    callf(ctx, 0x0028EBB8, {ctx.gpr[3], ctx.gpr[4]}, {0.0});
}
// f(a, b, c): sub_00295268(a, c, b, b)
DECOMP(00296810, Call_295268_Swap) { call(ctx, 0x00295268, {ctx.gpr[3], ctx.gpr[5], ctx.gpr[4], ctx.gpr[4]}); }
// f(a, b): sub_002A8AE0(a, 1, b, 1)
DECOMP(002AB8C4, Call_2A8AE0_002AB8C4) { call(ctx, 0x002A8AE0, {ctx.gpr[3], 1, ctx.gpr[4], 1}); }
// bool f(const Obj* o, u32 i): (u8)o->(i * 112 + 123) == 2
DECOMP(002BCA64, Field_Is123At2_002BCA64) {
    const u64 i = ctx.gpr[4];
    ctx.gpr[5] = i << 7;  // r4 / r5 left (64-bit)
    ctx.gpr[4] = (i << 7) - (i << 4);
    ret(ctx, load<u8>(static_cast<u32>(ctx.gpr[3] + ctx.gpr[4] + 123)) == 2 ? 1u : 0u);
}
// bool f(const Obj* o): o->+72 && (o->+72->+20 & 0x40)
DECOMP(002E5034, Field_72Flag40_002E5034) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 72);
    const u32 r = (p != 0 && (load<u32>(p + 20) & 0x40u) != 0) ? 1u : 0u;
    ctx.gpr[4] = r;  // r4 left
    ret(ctx, r);
}
// The record of the current slot of *(T**)0x00F65CA0: g + 80 + g->+4208 (u8) * 4056.
static u32 CurSlotRec(PPUContext& ctx, u32 scratch) {
    const u32 g = load<u32>(0x00F65CA0);
    const u32 off = load<u8>(g + 4208) * 4056u;
    ctx.gpr[scratch] = off;  // left as the offset
    return g + off + 80;
}
// void f(?, u32 v): the current slot record's +332 = v
DECOMP(002E6D6C, SlotRec_Set332) {
    const u32 r = CurSlotRec(ctx, 5);
    store<u32>(r + 332, arg<u32>(ctx, 1));
    ret(ctx, r);  // r3 left as the record
}
// u32 f(): the current slot record's +332
DECOMP(002E6D98, SlotRec_Get332) { ret(ctx, load<u32>(CurSlotRec(ctx, 4) + 332)); }
// u32 f(): the current slot record's +340
DECOMP(002E6E28, SlotRec_Get340) { ret(ctx, load<u32>(CurSlotRec(ctx, 4) + 340)); }
// f(const Obj* o): sub_00308F5C(o->+32->+872, 0)
DECOMP(0031417C, Call_308F5C_0031417C) { call(ctx, 0x00308F5C, {load<u32>(load<u32>(arg<u32>(ctx, 0) + 32) + 872), 0}); }
// f(const Obj* o): sub_00308F5C(o->+32->+872, 0)
DECOMP(003164AC, Call_308F5C_003164AC) { call(ctx, 0x00308F5C, {load<u32>(load<u32>(arg<u32>(ctx, 0) + 32) + 872), 0}); }
// f(a, ?, c): sub_0033CB98(c, a, 1)
DECOMP(0033CDC4, Call_33CB98_1) { call(ctx, 0x0033CB98, {ctx.gpr[5], ctx.gpr[3], 1}); }
// f(a, ?, c): sub_0033CB98(c, a, 0)
DECOMP(0033CDF0, Call_33CB98_0) { call(ctx, 0x0033CB98, {ctx.gpr[5], ctx.gpr[3], 0}); }
// f(Obj* o): if (o->+256) sub_00364408(o, o->+256)
DECOMP(00364580, Call_364408_If256) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 256);
    ctx.gpr[4] = p;  // r4 left
    if (p != 0) call(ctx, 0x00364408, {o, p});
}
// void f(Obj* o, bool on): o->+0 = 0x00B20CC8 (vtable), o->+4 = 0,
// o->+8: bit 0x80000000 = on, bit 0x40000000 cleared
DECOMP(0042C9D0, Obj_InitVt_B20CC8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 8);
    store<u32>(o, 0x00B20CC8);
    store<u32>(o + 4, 0);
    const u32 v = (w & 0x3FFFFFFFu) | (arg<u32>(ctx, 1) << 31);
    store<u32>(o + 8, v);
    ctx.gpr[5] = v;  // r5..r7 left
    ctx.gpr[6] = 0;
    ctx.gpr[7] = w;
}
// f(Obj* o): o->+16 = 0, Flag_Set8_40000000(o)
DECOMP(00431F4C, Obj_Clear16Flag_00431F4C) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 16, 0);
    call(ctx, 0x0042CA60, {o, 0});
}

// ---------------------------------------------------------------- batch 8
// void f(Obj* o): o->+12 = +16 = +24 = +32 = 0; o->+88 (u8) &= ~0x8C
DECOMP(00455B7C, Field_Reset88_00455B7C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u8>(o + 88) & ~0x8Cu;
    for (u32 k : {12u, 16u, 24u, 32u}) store<u32>(o + k, 0);
    store<u8>(o + 88, static_cast<u8>(b));
    ctx.gpr[4] = 0;  // r4 / r5 left
    ctx.gpr[5] = b;
}
// bool f(const Obj* o): o->+28 && (o->+28->+4 (u16) & 0x8000)
DECOMP(0045B820, Field_28Bit15_0045B820) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 28);
    ret(ctx, p != 0 ? static_cast<u32>(load<u16>(p + 4) >> 15) : 0u);
}
// bool f(a): sub_00569388(a, 0); returns 1
DECOMP(0048C074, Call_569388_Ret1) {
    call(ctx, 0x00569388, {ctx.gpr[3], 0});
    ret(ctx, 1u);
}
// s32 f(const char** a, const char** b): sub_00555DE8(*a, *b) (a qsort comparator on strings)
DECOMP(0049B4E0, Cmp_StrPtr_0049B4E0) {
    call(ctx, 0x00555DE8, {load<u32>(arg<u32>(ctx, 0)), load<u32>(arg<u32>(ctx, 1))});
}
// void f(Obj* o, const f32 v[3], f32 w): o->+224..+232 = v, o->+240 = w, o->+176 (u16) |= 0x800
DECOMP(00734E20, Field_Set224v_240_00734E20) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 f = load<u16>(o + 176) | 0x800u;
    store<u32>(o + 224, load<u32>(v));
    store<u32>(o + 228, load<u32>(v + 4));
    const u32 z = load<u32>(v + 8);
    store<f32>(o + 240, static_cast<f32>(farg(ctx, 0)));
    store<u32>(o + 232, z);
    store<u16>(o + 176, static_cast<u16>(f));
    ctx.gpr[5] = f;  // r5 / f2 left
    ctx.fpr[2] = std::bit_cast<f32>(z);
}
// void f(Obj* o, bool on): the u64 at o->+184 bit 0x4000 = on
DECOMP(00735E34, Flag_Set184d_4000_00735E34) {
    const u32 o = arg<u32>(ctx, 0);
    u64 v = load<u64>(o + 184);
    if ((arg<u32>(ctx, 1) & 0xFFu) != 0) {
        v |= 0x4000u;
    } else {
        v &= ~0x4000ull;
        ctx.gpr[6] = 0;  // r6 left 0
    }
    store<u64>(o + 184, v);
    ctx.gpr[4] = v;  // r4 left
}
// void* Mem_NewArray(u32 size): Mem_Alloc(size, 0, 16)
DECOMP(00735F28, Mem_NewArray) { call(ctx, fn::Mem_Alloc, {ctx.gpr[3], 0, 16}); }
// void f(Obj* o, const u8 s[4]): o->+72..+75 = s, byte by byte; r3 left as o + 72
DECOMP(00736398, Field_SetBytes72_00736398) {
    const u32 d = arg<u32>(ctx, 0) + 72, s = arg<u32>(ctx, 1);
    for (u32 k = 0; k < 4; ++k) store<u8>(d + k, load<u8>(s + k));
    ctx.gpr[4] = load<u8>(d + 3);  // r4 / r5 left as the last two bytes
    ctx.gpr[5] = load<u8>(d + 2);
    ret(ctx, d);
}
// f(Obj* o): sub_001072BC(o + 16)
DECOMP(00738198, Call_1072BC_16) { call(ctx, 0x001072BC, {arg<u32>(ctx, 0) + 16}); }
// void f(Obj* o, u64 bits, bool on): the u64 at o->+184 |= bits (on) or &= ~bits
DECOMP(0074B684, Flag_Set184dBits_0074B684) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 bits = ctx.gpr[4];
    const u64 v = load<u64>(o + 184);
    const u64 w = (arg<u32>(ctx, 2) & 0xFFu) != 0 ? (v | bits) : (v & ~bits);
    store<u64>(o + 184, w);
    ctx.gpr[4] = w;  // r4 / r5 left
    ctx.gpr[5] = v;
}
// u32 f(const Obj* o): n = o->+345 (u8): n ? o->+364[n] : 0
DECOMP(0074C470, Field_Get364AtN_0074C470) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = load<u8>(o + 345);
    if (n == 0) {
        ctx.gpr[4] = 0;  // r4 / r5 left
        ctx.gpr[5] = 0;
        ret(ctx, 0u);
        return;
    }
    const u32 v = load<u32>(o + 364 + (n << 2));
    ctx.gpr[4] = n << 2;
    ctx.gpr[5] = v;
    ret(ctx, v);
}
// T* f(Obj* o): o->+24 (s8) == 1 ? o + o->+16 (s8) : 0
DECOMP(0074C7CC, Field_SelfOffset16_0074C7CC) {
    const u32 o = arg<u32>(ctx, 0);
    const s8 k = load<s8>(o + 24);
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(k));  // r4 left
    if (k != 1) {
        ret(ctx, 0u);
        return;
    }
    const s8 off = load<s8>(o + 16);
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(off));
    ret(ctx, o + static_cast<u32>(static_cast<s32>(off)));
}
// void f(Rec* r, const f32 v[3], ?, ?, u32 a, u32 b, u32 c, f32 w): r->+0..+8 = v,
// +16 = w, +20 = a, +24 = b, +28 = c
DECOMP(0074D1B8, Rec_Set_0074D1B8) {
    const u32 r = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    store<u32>(r, load<u32>(v));
    const u32 y = load<u32>(v + 4);
    store<u32>(r + 20, arg<u32>(ctx, 3));
    store<u32>(r + 4, y);
    const u32 z = load<u32>(v + 8);
    store<f32>(r + 16, static_cast<f32>(farg(ctx, 0)));
    store<u32>(r + 8, z);
    store<u32>(r + 24, arg<u32>(ctx, 4));
    store<u32>(r + 28, arg<u32>(ctx, 5));
    ctx.fpr[2] = std::bit_cast<f32>(z);  // f2 left
}
// f(Obj* o): sub_00922918(o + 224) (a Havok call)
DECOMP(00757BF8, Call_922918_224) { call(ctx, 0x00922918, {arg<u32>(ctx, 0) + 224}); }
// void f(f32 out[3], const Obj* o, u32 i): out = o->+224 + i * 16 (three floats)
DECOMP(00760898, Field_Get224At_00760898) {
    const u64 i16 = ctx.gpr[5] << 4;
    const u32 p = static_cast<u32>(ctx.gpr[4] + i16 + 224), out = arg<u32>(ctx, 0);
    u32 bits = 0;
    for (u32 k = 0; k < 3; ++k) {
        bits = load<u32>(p + 4 * k);
        store<u32>(out + 4 * k, bits);
    }
    ctx.gpr[4] = p;  // r4 / r5 / f1 left
    ctx.gpr[5] = i16;
    ret(ctx, std::bit_cast<f32>(bits));
}
// void f(f32 out[3], const Obj* o, u32 i): out = o->+192 + i * 16 (three floats)
DECOMP(00760950, Field_Get192At_00760950) {
    const u64 i16 = ctx.gpr[5] << 4;
    const u32 p = static_cast<u32>(ctx.gpr[4] + i16 + 192), out = arg<u32>(ctx, 0);
    u32 bits = 0;
    for (u32 k = 0; k < 3; ++k) {
        bits = load<u32>(p + 4 * k);
        store<u32>(out + 4 * k, bits);
    }
    ctx.gpr[4] = p;  // r4 / r5 / f1 left
    ctx.gpr[5] = i16;
    ret(ctx, std::bit_cast<f32>(bits));
}
// void f(Obj* o, bool on): o->+20 bit 0x80 = on
DECOMP(00769B08, Flag_Set20_80_00769B08) {
    const u32 o = arg<u32>(ctx, 0);
    u32 v = load<u32>(o + 20);
    if ((arg<u32>(ctx, 1) & 0xFFu) != 0) {
        v |= 0x80u;
    } else {
        v &= ~0x80u;
        ctx.gpr[6] = 0;  // r6 left 0
    }
    store<u32>(o + 20, v);
    ctx.gpr[4] = v;  // r4 left
}
// void f(u8 out[4], const Obj* o): out = o->+132..+135 (bytes)
DECOMP(00796454, Field_GetBytes132_00796454) {
    const u32 s = arg<u32>(ctx, 1) + 132, d = arg<u32>(ctx, 0);
    const u8 b0 = load<u8>(s), b1 = load<u8>(s + 1), b2 = load<u8>(s + 2), b3 = load<u8>(s + 3);
    store<u8>(d, b0);
    store<u8>(d + 1, b1);
    store<u8>(d + 2, b2);
    store<u8>(d + 3, b3);
    ctx.gpr[4] = b3;  // r4..r7 left
    ctx.gpr[5] = b0;
    ctx.gpr[6] = b1;
    ctx.gpr[7] = b2;
}
// bool f(const Obj* o): o->+4 == 44 || 54 < o->+4 <= 58
DECOMP(00032740, Field_Is4Kind_00032740) {
    const s32 v = load<s32>(arg<u32>(ctx, 0) + 4);
    ret(ctx, (v > 54 ? v <= 58 : v == 44) ? 1u : 0u);
}
// f32 f(const Obj* o): o->+465 bit 0x40 ? 500.0f : 50.0f (the pair at 0x0003BE24)
DECOMP(0003BE30, Field_Pick465_0003BE30) {
    const bool on = (load<u8>(arg<u32>(ctx, 0) + 465) >> 6) & 1u;
    ctx.gpr[3] = 0x0003BE24;  // r3 left as the pair
    ret(ctx, load<f32>(0x0003BE24 + (on ? 0u : 4u)));
}
// f(): sub_002E4A0C(*(T**)0x00F64C20, 1)
DECOMP(0006CB80, Call_2E4A0C_F64C20) { call(ctx, 0x002E4A0C, {load<u32>(0x00F64C20), 1}); }
// f(): sub_002E26A8(*(T**)0x00F64C20, 1)
DECOMP(00099658, Call_2E26A8_F64C20) { call(ctx, 0x002E26A8, {load<u32>(0x00F64C20), 1}); }
// f(a, b, c): sub_000B1A88(a, b, c, 4056, *(s32*)0x01012794)
DECOMP(000B1C94, Call_B1A88_4056) {
    call(ctx, 0x000B1A88, {ctx.gpr[3], ctx.gpr[4], ctx.gpr[5], 4056, static_cast<u64>(static_cast<s64>(load<s32>(0x01012794)))});
}
// f(a, b, c): sub_000B1A88(a, b, c, 36, *(s32*)0x01012798)
DECOMP(000B1CC4, Call_B1A88_36) {
    call(ctx, 0x000B1A88, {ctx.gpr[3], ctx.gpr[4], ctx.gpr[5], 36, static_cast<u64>(static_cast<s64>(load<s32>(0x01012798)))});
}
// bool f(const Obj* o): (o->+184 (u64) & 0x20) && ((o->+172 >> 22) & 3) == 1
DECOMP(00229C94, Field_184Flag20Kind_00229C94) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 bit = static_cast<u32>(load<u64>(o + 184)) & 0x20u;
    ctx.gpr[5] = bit;  // r4 / r5 left
    const u32 r = (bit != 0 && ((load<u32>(o + 172) >> 22) & 3u) == 1) ? 1u : 0u;
    ctx.gpr[4] = r;
    ret(ctx, r);
}
// void f(Obj* o, u8 b): o->+16 = b; if (b) sub_00290084(o, b)
DECOMP(00290190, Field_Set16bCall_00290190) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    store<u8>(o + 16, static_cast<u8>(b));
    ctx.gpr[4] = b;
    if (b != 0) call(ctx, 0x00290084, {o, b});
}
// f(Obj* o): sub_002A8AE0(o, 0, o->+244 >> 28, 1)
DECOMP(002AB8F0, Call_2A8AE0_244) { call(ctx, 0x002A8AE0, {ctx.gpr[3], 0, load<u32>(arg<u32>(ctx, 0) + 244) >> 28, 1}); }
// bool f(Obj* o): o->+40 && o->+41 (u8) ? 1 : (o->+41 = 1, 0)
DECOMP(002FD598, Field_Once41_002FD598) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u8>(o + 40) != 0) {
        const u8 b = load<u8>(o + 41);
        ctx.gpr[4] = b;
        if (b != 0) {
            ret(ctx, 1u);
            return;
        }
    }
    store<u8>(o + 41, 1);
    ctx.gpr[4] = 1;  // r4 left 1
    ret(ctx, 0u);
}
// void f(Obj* o, f32 dt): o->+48 -= dt, then 0 when below 0.0f (the constant at 0x002FD698)
DECOMP(002FD6A0, Field_Countdown48_002FD6A0) {
    const u32 o = arg<u32>(ctx, 0);
    const f32 v = static_cast<f32>(static_cast<f64>(load<f32>(o + 48)) - farg(ctx, 0));
    store<f32>(o + 48, v);
    ctx.gpr[4] = 0x002FD698;  // r4 / f1 / f2 left
    ctx.fpr[2] = 0.0;
    ret(ctx, v);
    if (v < 0.0f) {
        store<u32>(o + 48, 0);
        ctx.gpr[4] = 0;
    }
}
// bool f(const Obj* o): o->+48 <= 0.0f (the constant at 0x002FD6D0)
DECOMP(002FD6D8, Field_Expired48_002FD6D8) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 v = load<f32>(o + 48);
    ctx.gpr[4] = 0x002FD6D0;  // r4 / r5 / f1 / f2 left
    ctx.gpr[5] = o;
    ctx.fpr[1] = 0.0;
    ctx.fpr[2] = v;
    ret(ctx, v <= 0.0 ? 1u : 0u);
}
// void f(const Obj* o): the current slot record's +344 (u8) = o->+261
DECOMP(003F0418, SlotRec_Set344b) {
    const u8 b = load<u8>(arg<u32>(ctx, 0) + 261);
    const u32 r = CurSlotRec(ctx, 5);
    store<u8>(r + 344, b);
    ctx.gpr[4] = r;  // r3 / r4 left
    ret(ctx, b);
}
// bool f(Obj* o, f32 d): o->+16 += d; o->+16 >= o->+12
DECOMP(00431F78, Field_Advance16_00431F78) {
    const u32 o = arg<u32>(ctx, 0);
    const f32 v = static_cast<f32>(static_cast<f64>(load<f32>(o + 16)) + farg(ctx, 0));
    const f64 end = load<f32>(o + 12);
    store<f32>(o + 16, v);
    ctx.gpr[4] = o;  // r4 / f1 / f2 left
    ctx.fpr[2] = end;
    ctx.fpr[1] = v;
    ret(ctx, static_cast<f64>(v) >= end ? 1u : 0u);
}
// f(Obj** o, ?, c): sub_006D5EE0(*o, 0, c, -1) (sound middleware)
DECOMP(0044DC1C, Call_6D5EE0_0044DC1C) { call(ctx, 0x006D5EE0, {load<u32>(arg<u32>(ctx, 0)), 0, ctx.gpr[5], ~0ull}); }
// bool f(...): sub_006D5E10(...) != 0 (sound middleware)
DECOMP(0044DC4C, Call_6D5E10_Bool) { ret(ctx, static_cast<u32>(call(ctx, 0x006D5E10, {ctx.gpr[3]})) != 0 ? 1u : 0u); }

// ---------------------------------------------------------------- late additions
// void f(Obj* o, bool on): o->+136 bit 0x200 = on
DECOMP(0074C67C, Flag_Set136_200_0074C67C) {
    const u32 o = arg<u32>(ctx, 0);
    InsertBits32(o + 136, 0x200u, arg<u32>(ctx, 1) << 9);
}
