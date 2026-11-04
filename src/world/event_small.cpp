// Small out-of-line helpers of World_HandleEvent (0x00491710): bit-field
// getters and setters, null-checked accessors and thunks. Most callers are
// still recompiled, so each function leaves the volatile registers the way
// the original does where that is cheap (noted per function).
#include <bit>

#include "event.h"

using namespace sly4;
using namespace sly4::wevent;

namespace {

// Bit test on an array of 32-bit words: (words[i / 32] >> (i % 32)) & 1.
// Leaves r4 = the byte offset, r5 = the bit mask, r6 = 1.
void BitArrayTest(PPUContext& ctx, u32 base_offset) {
    const u32 o = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1);
    const u32 bit = 1u << (i & 31);
    const u32 off = (i >> 3) & 0x1FFFFFFCu;
    const u32 w = load<u32>(o + base_offset + off);
    ctx.gpr[4] = off;
    ctx.gpr[5] = bit;
    ctx.gpr[6] = 1;
    ret(ctx, (w & bit) != 0 ? 1u : 0u);
}

// Bits 1.. of the word at +160 (a state number) (name?)
u32 State160(u32 o) { return load<u32>(o + 160) >> 1; }

}  // namespace

// Empty functions.
DECOMP(0034CC30, Nop_0034CC30) {}
DECOMP(004F4490, Nop_004F4490) {}

// Object* Singleton_011DF320_Get(): *0x011DF320 (read through the TOC; r9 = its address).
DECOMP(007D3B80, Singleton_011DF320_Get) {
    ctx.gpr[9] = kSingleton11DF320;
    ret(ctx, load<u32>(kSingleton11DF320));
}

// void Global_F64A68_Clear(): *0x00F64A68 = 0.
DECOMP(002AAEA8, Global_F64A68_Clear) {
    store<u32>(kGlobalF64A68, 0);
    ctx.gpr[3] = 0x00F60000u;
    ctx.gpr[4] = 0;
}

// void Obj_SetBit2339_80(Object* o, bool on): byte +2339 bit 0x80.
DECOMP(0074C5C4, Obj_SetBit2339_80) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = (ref<u8>(o + 2339) & 0x7Fu) | ((arg<u32>(ctx, 1) << 7) & 0x80u);
    ref<u8>(o + 2339) = static_cast<u8>(b);
    ctx.gpr[5] = b;
}

// void Obj_SetBit4_8000(Object* o, bool on): word +4 bit 0x8000.
DECOMP(0077B170, Obj_SetBit4_8000) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = (load<u32>(o + 4) & ~0x8000u) | ((arg<u32>(ctx, 1) << 15) & 0x8000u);
    store<u32>(o + 4, w);
    ctx.gpr[5] = w;
}

// void Obj_SetBit136_400(Object* o, bool on): word +136 bit 0x400.
DECOMP(0078779C, Obj_SetBit136_400) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = (load<u32>(o + 136) & ~0x400u) | ((arg<u32>(ctx, 1) << 10) & 0x400u);
    store<u32>(o + 136, w);
    ctx.gpr[5] = w;
}

// void Obj_SetBit396_40000000(Object* o, bool on): word +396 bit 0x40000000.
DECOMP(00787864, Obj_SetBit396_40000000) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = (load<u32>(o + 396) & ~0x40000000u) | ((arg<u32>(ctx, 1) << 30) & 0x40000000u);
    store<u32>(o + 396, w);
    ctx.gpr[5] = w;
}

// void Obj_SetBit24128_80(Object* o, bool on): byte +24128 bit 0x80.
DECOMP(00787DB0, Obj_SetBit24128_80) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = (ref<u8>(o + 24128) & 0x7Fu) | ((arg<u32>(ctx, 1) << 7) & 0x80u);
    ref<u8>(o + 24128) = static_cast<u8>(b);
    ctx.gpr[5] = b;
}

// bool Obj_Field244Clear(Object* o): bits 20..27 of word +244 are all 0.
DECOMP(0029A40C, Obj_Field244Clear) {
    ret(ctx, ((load<u32>(arg<u32>(ctx, 0) + 244) >> 20) & 0xFF) == 0 ? 1u : 0u);
}

// void World_SetFlags44(World* w, bool b): +44 |= 0x40 and bit 0x20 = b.
DECOMP(004896F8, World_SetFlags44) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 b = ((ref<u8>(w + 44) | 0x40u) & ~0x20u) | ((arg<u32>(ctx, 1) << 5) & 0x20u);
    ref<u8>(w + 44) = static_cast<u8>(b);
    ctx.gpr[5] = b;
}

// void Rgba_Set(u8 out[4], u8 r, u8 g, u8 b, u8 a)
DECOMP(00735F7C, Rgba_Set) {
    const u32 p = arg<u32>(ctx, 0);
    for (u32 k = 0; k < 4; ++k) ref<u8>(p + k) = static_cast<u8>(arg<u32>(ctx, 1 + static_cast<int>(k)));
}

// bool Obj_State160Is0or1(Object* o): three instantiations of the same code.
DECOMP(0078761C, Obj_State160IsIdleA) { ret(ctx, State160(arg<u32>(ctx, 0)) == 0 ? 1u : 0u); }
DECOMP(00787950, Obj_State160IsIdleB) { ret(ctx, State160(arg<u32>(ctx, 0)) == 0 ? 1u : 0u); }
DECOMP(00787B6C, Obj_State160IsIdleC) { ret(ctx, State160(arg<u32>(ctx, 0)) == 0 ? 1u : 0u); }
// bool Obj_State160Is3(Object* o)
DECOMP(00787D2C, Obj_State160Is3) { ret(ctx, State160(arg<u32>(ctx, 0)) == 3 ? 1u : 0u); }
// bool Obj_State160Is2or3(Object* o). Leaves r4 = the state.
DECOMP(00787648, Obj_State160Is2or3) {
    const u32 s = State160(arg<u32>(ctx, 0));
    ctx.gpr[4] = s;
    ret(ctx, (s == 2 || s == 3) ? 1u : 0u);
}

// Single-bit getters: (word at +offset >> bit) & 1.
DECOMP(00748C38, Obj_Bit172_18) { ret(ctx, (load<u32>(arg<u32>(ctx, 0) + 172) >> 18) & 1); }
DECOMP(00787504, Obj_Bit360_31) { ret(ctx, (load<u32>(arg<u32>(ctx, 0) + 360) >> 31) & 1); }
DECOMP(00787604, Obj_Bit216_24) { ret(ctx, (load<u32>(arg<u32>(ctx, 0) + 216) >> 24) & 1); }
DECOMP(007877AC, Obj_Bit52_29) { ret(ctx, (load<u32>(arg<u32>(ctx, 0) + 52) >> 29) & 1); }
DECOMP(0078784C, Obj_Bit396_30) { ret(ctx, (load<u32>(arg<u32>(ctx, 0) + 396) >> 30) & 1); }

// bool Obj_Bit188_20(Object* o): (u32 at +188) & 0x20 (read as the low half of the u64 at +184).
DECOMP(00787E30, Obj_Bit188_20) {
    const u32 m = static_cast<u32>(load<u64>(arg<u32>(ctx, 0) + 184)) & 0x20u;
    ctx.gpr[4] = m;
    ret(ctx, m != 0 ? 1u : 0u);
}

// bool Obj_Byte656Bit6(Object* o, s32 i): (byte o[656 + i] >> 6) & 1.
DECOMP(00787E50, Obj_Byte656Bit6) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 i = static_cast<s32>(arg<u32>(ctx, 1));
    ret(ctx, (ref<u8>(o + 656 + static_cast<u32>(i)) >> 6) & 1u);
}

// void World_SetValue72IfFlag(World* w, u32 v): +72 = v while +44 bit 0x80 is set.
DECOMP(00787938, World_SetValue72IfFlag) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 f = (ref<u8>(w + 44) >> 7) & 1;
    ctx.gpr[5] = f;
    if (f == 1) store<u32>(w + 72, arg<u32>(ctx, 1));
}

// s32 World_GetValue72IfFlag(World* w): +72 while +44 bit 0x80 is set, else 0.
DECOMP(00787914, World_GetValue72IfFlag) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 f = (ref<u8>(w + 44) >> 7) & 1;
    const s32 v = f == 1 ? load<s32>(w + 72) : 0;
    ctx.gpr[5] = f;
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(v));
    ret(ctx, v);
}

// void World_TimeScaleOff(World* w) / World_AudioScaleOff: clear the blend's "on" bit.
DECOMP(00787D44, World_TimeScaleOff) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 v = load<u32>(w + kWorldTimeScaleOn) & 0x7FFFFFFFu;
    store<u32>(w + kWorldTimeScaleOn, v);
    ctx.gpr[4] = kWorldTimeScaleOn;
    ctx.gpr[5] = v;
}
DECOMP(00787D5C, World_AudioScaleOff) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 v = load<u32>(w + kWorldAudioScaleOn) & 0x7FFFFFFFu;
    store<u32>(w + kWorldAudioScaleOn, v);
    ctx.gpr[4] = kWorldAudioScaleOn;
    ctx.gpr[5] = v;
}

// bool World_IsNotBusy(World* w): +0x1282A0 bit 0x40000000 clear.
DECOMP(00787830, World_IsNotBusy) {
    const u32 w = arg<u32>(ctx, 0);
    ctx.gpr[4] = kWorldBusy;
    ret(ctx, (load<u32>(w + kWorldBusy) & 0x40000000u) == 0 ? 1u : 0u);
}

// u32 Obj_Get2416(Object* o, s32 i): o->words2416[i].
DECOMP(007881C4, Obj_Get2416) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 i = static_cast<s32>(arg<u32>(ctx, 1) << 2);
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(i));
    ret(ctx, load<u32>(o + 2416 + static_cast<u32>(i)));
}

// void Global_F9CDA8_Reset(): u16 count at 0x00F9CDB0 = 0 and *0x00F558D4 = 0 (name?)
DECOMP(000A884C, Global_F9CDA8_Reset) {
    store<u16>(kListF9CDA8 + 8, 0);
    store<u32>(kGlobalF558D4, 0);
    ctx.gpr[3] = kListF9CDA8;
    ctx.gpr[4] = 0;
    ctx.gpr[5] = 0x00F50000u;
}

// void Global_10B0100_Set(u32 v): stores v unless it is already there.
DECOMP(0044D9C0, Global_10B0100_Set) {
    const u32 v = arg<u32>(ctx, 0);
    const u32 old = load<u32>(kGlobal10B0100);
    ctx.gpr[4] = old;
    if (old == v) return;
    ctx.gpr[4] = 0x010B0000u;
    store<u32>(kGlobal10B0100, v);
}

// u32 Display_GetMode(): the display config's +40 word, 0 when it is 16 (name?)
DECOMP(0048CEE0, Display_GetMode) {
    const u32 m = load<u32>(kDisplayConfig + 40);
    ret(ctx, m == 16 ? 0u : m);
}

// u32 Obj_Get272_12(Object* o): o->p272 ? o->p272->+12 : 0.
DECOMP(004D2268, Obj_Get272_12) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 272);
    ret(ctx, p != 0 ? load<u32>(p + 12) : 0u);
}

// bool Obj_Ptr100IsNull(Object* o)
DECOMP(00787A2C, Obj_Ptr100IsNull) { ret(ctx, load<u32>(arg<u32>(ctx, 0) + 100) == 0 ? 1u : 0u); }

// u32 Obj_Get100_8(Object* o): o->p100 ? o->p100->+8 : 0. Leaves r4 = the result.
DECOMP(00787A48, Obj_Get100_8) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 100);
    const u32 v = p != 0 ? load<u32>(p + 8) : 0u;
    ctx.gpr[4] = v;
    ret(ctx, v);
}

// u16 PtrVecRef_Count(PtrVector** v): (*v) ? (*v)->count : 0. Leaves r4 = the result.
DECOMP(00788188, PtrVecRef_Count) {
    const u32 p = load<u32>(arg<u32>(ctx, 0));
    const u32 v = p != 0 ? load<u16>(p + 8) : 0u;
    ctx.gpr[4] = v;
    ret(ctx, v);
}

// Thunks: the same call with the same registers.
DECOMP(003A1808, Thunk_003A1500) { call(ctx, kfn::sub_003A1500); }
DECOMP(00454C88, Thunk_004545B0) { call(ctx, kfn::sub_004545B0); }

// u32 Array60_At(Array** a, u32 i): (*a)->data + i * 60.
DECOMP(007881A4, Array60_At) {
    const u64 i = ctx.gpr[4];
    const u32 data = load<u32>(load<u32>(arg<u32>(ctx, 0)));
    ctx.gpr[5] = i << 6;
    ctx.gpr[4] = (i << 6) - (i << 2);
    ret(ctx, data + static_cast<u32>(ctx.gpr[4]));
}

// void Global_F558E1_SetIfPending(): byte 0x00F558E1 = 1 while the list at 0x00F9CDA8 is non-empty (name?)
DECOMP(000A8868, Global_F558E1_SetIfPending) {
    const u32 n = load<u16>(kListF9CDA8 + 8);
    ctx.gpr[3] = n;
    if (n == 0) return;
    ref<u8>(kFlagF558E1) = 1;
    ctx.gpr[3] = 0x00F50000u;
    ctx.gpr[4] = 1;
}

// u32 Global_F558E4_Chain(): *0x00F55904 = (*0x00F558E4)->+672->+516 + 56, returned.
DECOMP(000AC83C, Global_F558E4_Chain) {
    const u32 v = load<u32>(load<u32>(load<u32>(kGlobalF558E4) + 672) + 516) + 56;
    store<u32>(kGlobalF55904, v);
    ctx.gpr[4] = 0x00F50000u;
    ctx.gpr[5] = v;
    ret(ctx, v);
}

// u16 Trophy_GetValue200(): the trophy manager's +200 (low half) once it is ready (+8).
DECOMP(000B28F0, Trophy_GetValue200) {
    const u32 t = load<u32>(kTrophy);
    const u32 ready = ref<u8>(t + 8);
    ctx.gpr[4] = t;
    ctx.gpr[5] = ready;
    ret(ctx, ready != 0 ? (load<u32>(t + 200) & 0xFFFFu) : 0u);
}

// u32 Obj_Get4_280(Object* o): o->p4 ? o->p4->+280 : 0. Leaves r4 = the result.
DECOMP(00346B84, Obj_Get4_280) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 4);
    const u32 v = p != 0 ? load<u32>(p + 280) : 0u;
    ctx.gpr[4] = v;
    ret(ctx, v);
}

// bool Ptr_DerefNonZero(u32* p): p && *p.
DECOMP(00446960, Ptr_DerefNonZero) {
    const u32 p = arg<u32>(ctx, 0);
    ret(ctx, (p != 0 && load<u32>(p) != 0) ? 1u : 0u);
}

// void Global_F557EC_Call(): sub_00010238(*0x00F557EC).
DECOMP(00019210, Global_F557EC_Call) { call(ctx, kfn::sub_00010238, {load<u32>(kGlobalF557EC)}); }

// void Obj_Notify24(Object* o) / Obj_Notify26: sub_000D1288(o, 24 / 26, 0).
DECOMP(000D949C, Obj_Notify24) { call(ctx, kfn::sub_000D1288, {arg<u32>(ctx, 0), 24, 0}); }
DECOMP(000D94CC, Obj_Notify26) { call(ctx, kfn::sub_000D1288, {arg<u32>(ctx, 0), 26, 0}); }

// bool Mask_Test(u32* p, u32 n): *p & (1 << n). Leaves r4 = the mask, r5 = 1.
DECOMP(0077B114, Mask_Test) {
    const u32 n = arg<u32>(ctx, 1) & 63;
    const u32 m = n < 32 ? (1u << n) : 0u;  // slw
    ctx.gpr[4] = m;
    ctx.gpr[5] = 1;
    ret(ctx, (load<u32>(arg<u32>(ctx, 0)) & m) != 0 ? 1u : 0u);
}

// Item* Obj_CurrentItem(Object* o): the entry (16-byte stride, first word)
// of o->+8's table for the index at +356, or 0 when out of range.
DECOMP(0037359C, Obj_CurrentItem) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = load<u32>(o + 8);
    const u32 i = load<u16>(o + 356);
    const u32 n = load<u32>(t + 152);
    ctx.gpr[5] = t;
    ctx.gpr[6] = n;
    u32 v = 0;
    if (i < n) v = load<u32>(load<u32>(t + 148) + ((i << 4) & 0xFFFF0u));
    ctx.gpr[4] = v;
    ret(ctx, v);
}

// void Obj_SetFlags40(Object* o, u32 bits, bool on): +40 |= bits or &= ~bits.
DECOMP(00787B80, Obj_SetFlags40) {
    const u32 o = arg<u32>(ctx, 0), bits = arg<u32>(ctx, 1);
    const u32 old = load<u32>(o + 40);
    const u32 v = (arg<u32>(ctx, 2) & 0xFF) != 0 ? (old | bits) : (old & ~bits);
    store<u32>(o + 40, v);
    ctx.gpr[4] = v;
    ctx.gpr[5] = old;
}

// void Global_10592E8_Ensure(): sub_002A3AD0() while *0x010592E8 is 0.
DECOMP(002A3EBC, Global_10592E8_Ensure) {
    if (load<u32>(kGlobal10592E8) == 0) call(ctx, kfn::sub_002A3AD0);
}

// bool World_Obj128164Is1(World* w): w->+0x128164 && its +5444 == 1 (name?)
DECOMP(00476C30, World_Obj128164Is1) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + kWorldObj128164);
    ctx.gpr[4] = kWorldObj128164;
    ret(ctx, (p != 0 && load<u32>(p + 5444) == 1) ? 1u : 0u);
}

// void Obj_Release36CDA8(Object* o): sub_0036CDA8(o, o) unless o is 0.
DECOMP(004A82DC, Obj_Release36CDA8) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[4] = o;
    if (o != 0) call(ctx, kfn::sub_0036CDA8, {o, o});
}

// Bit arrays: bit i of the u32[] at +offset.
DECOMP(00735000, Obj_Bit192) { BitArrayTest(ctx, 192); }
DECOMP(0074DC34, Obj_Bit28) { BitArrayTest(ctx, 28); }
DECOMP(00787674, Obj_Bit20) { BitArrayTest(ctx, 20); }

// bool Obj_Pair36Bit(Object* o, u32 i): bit 2i of the u32[] at +36 (2-bit fields).
DECOMP(0074BFB0, Obj_Pair36Bit) {
    const u32 o = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1);
    const u32 bit = 1u << ((i << 1) & 0x1Eu);
    const u32 off = (i >> 2) & 0x3FFFFFFCu;
    ctx.gpr[4] = off;
    ctx.gpr[5] = bit;
    ctx.gpr[6] = 1;
    ret(ctx, (load<u32>(o + 36 + off) & bit) != 0 ? 1u : 0u);
}
