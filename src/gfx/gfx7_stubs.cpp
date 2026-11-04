// The renderer range's smallest functions: empty and constant vtable
// slots, base-class destructors that only reset the vtable, and one-line
// getters / setters. Names carry the address until the classes are
// known; the comment above each gives the original signature and the
// vtables (and slot indices) that point at it.
//
// Where the original leaves a scratch register set (the vtable address in
// r4, a global's high half in r3 / r4, ...), it is set the same way.
#include <atomic>
#include <bit>

#include "gfx7.h"

using namespace sly4;

namespace {

constexpr u32 kGlobal00F66FC8 = 0x00F66FC8;  // u32 (name?)
constexpr u32 kGlobal00F66FCC = 0x00F66FCC;  // f32 (name?)
constexpr u32 kGlobal010F0E00 = 0x010F0E00;  // u32 counter (DECOMP.md: the VBlank counter) (name?)
constexpr u32 kGlobal010F0E04 = 0x010F0E04;  // u32 (gcm.cpp kUnk010F0E04)
constexpr u32 kGlobal010DDA78 = 0x010DDA78;  // u32 (draw::g::current_surface + 4) (name?)
constexpr u32 kGlobal010C5A08 = 0x010C5A08;  // u32 + u8 flag (graphics.cpp kGfxUnk010C5A08)
constexpr u32 kGlobal01195090 = 0x01195090;  // u32[2] (rjobs.cpp kFlipSlots)
constexpr u32 kToc00F488BC = 0x00F488BC;     // TOC -32580: u32** (name?)

}  // namespace

// void ()  [called directly]
DECOMP(00517FA8, Nop_00517FA8) {}

// void ()  [called directly]
DECOMP(00518E60, Nop_00518E60) {}

// void ()  [called directly]
DECOMP(00518E64, Nop_00518E64) {}

// void ()  [called directly]
DECOMP(0051FF18, Nop_0051FF18) {}

// void ()  [called directly]
DECOMP(00520028, Nop_00520028) {}

// void ()  [called directly]
DECOMP(0054EC94, Array_Destruct) {}

// void ()  [vtable 00B253D0[2]]
DECOMP(005B19CC, Nop_005B19CC) {}

// void ()  [vtable 00B253D0[3]]
DECOMP(005B19D0, Nop_005B19D0) {}

// void ()  [vtable 00B26178[4]]
DECOMP(005D7BCC, Nop_005D7BCC) {}

// void ()  [vtable 00B234E8[4]]
DECOMP(007A172C, Nop_007A172C) {}

// void ()  [vtable 00B235BC[4]]
DECOMP(007A1854, Nop_007A1854) {}

// void ()  [vtable 00B235DC[4]]
DECOMP(007A18F0, Nop_007A18F0) {}

// void ()  [vtable 00B23630[4]]
DECOMP(007A1AE4, Nop_007A1AE4) {}

// void ()  [vtable 00B23670[4]]
DECOMP(007A1B3C, Nop_007A1B3C) {}

// void ()  [vtable 00B23690[0], 00B236A8[0]]
DECOMP(007A1B40, Nop_007A1B40) {}

// void ()  [vtable 00B23690[1], 00B236A8[1]]
DECOMP(007A1B44, Nop_007A1B44) {}

// void ()  [vtable 00B236C0[4]]
DECOMP(007A2150, Nop_007A2150) {}

// void ()  [vtable 00B23CD0[4]]
DECOMP(007A22CC, Nop_007A22CC) {}

// void ()  [vtable 00B23CD0[7], 00B24130[7]]
DECOMP(007A22D0, Nop_007A22D0) {}

// void ()  [vtable 00B23D80[2]]
DECOMP(007A2498, Nop_007A2498) {}

// void ()  [vtable 00B23DA0[2]]
DECOMP(007A2560, Nop_007A2560) {}

// void ()  [vtable 00B23DE0[2]]
DECOMP(007A26F0, Nop_007A26F0) {}

// void ()  [vtable 00B23E00[2]]
DECOMP(007A27B8, Nop_007A27B8) {}

// void ()  [vtable 00B23E20[2]]
DECOMP(007A2880, Nop_007A2880) {}

// void ()  [vtable 00B23E40[2]]
DECOMP(007A2948, Nop_007A2948) {}

// void ()  [vtable 00B23E60[2]]
DECOMP(007A2A10, Nop_007A2A10) {}

// void ()  [vtable 00B23E80[2]]
DECOMP(007A2AD8, Nop_007A2AD8) {}

// void ()  [vtable 00B23EA0[2]]
DECOMP(007A2BA0, Nop_007A2BA0) {}

// void ()  [vtable 00B24130[5]]
DECOMP(007A3AD4, Nop_007A3AD4) {}

// void ()  [vtable 00B24190[4]]
DECOMP(007A3CFC, Nop_007A3CFC) {}

// void ()  [vtable 00B24310[5], 00B244C0[5]]
DECOMP(007A4D4C, Nop_007A4D4C) {}

// void ()  [vtable 00B24310[6], 00B244C0[6]]
DECOMP(007A4D50, Nop_007A4D50) {}

// void ()  [vtable 00B24310[11], 00B244C0[11]]
DECOMP(007A4D5C, Nop_007A4D5C) {}

// void ()  [vtable 00B24418[2]]
DECOMP(007A4FF4, Nop_007A4FF4) {}

// void ()  [vtable 00B24438[2]]
DECOMP(007A50B0, Nop_007A50B0) {}

// u32 () -> 1  [called directly]
DECOMP(0050129C, True_0050129C) {
    ret(ctx, 1u);
}

// u32 () -> 1  [called directly]
DECOMP(00525A1C, True_00525A1C) {
    ret(ctx, 1u);
}

// void (u32* p, u32 v): *p = v  [called directly]
DECOMP(0052F778, SetWord_0052F778) {
    store<u32>(arg<u32>(ctx, 0), arg<u32>(ctx, 1));
}

// u32 () -> 0  [called directly]
DECOMP(00537818, False_00537818) {
    ret(ctx, 0u);
}

// u32 () -> 1  [called directly]
DECOMP(0053B650, True_0053B650) {
    ret(ctx, 1u);
}

// u32 () -> 1  [vtable 00B25178[1]]
DECOMP(005AFBD0, True_005AFBD0) {
    ret(ctx, 1u);
}

// u32 () -> 1  [vtable 00B253F0[1]]
DECOMP(005B1ACC, True_005B1ACC) {
    ret(ctx, 1u);
}

// u32 () -> 1  [vtable 00B256A8[1]]
DECOMP(005B7878, True_005B7878) {
    ret(ctx, 1u);
}

// u32 () -> 1  [vtable 00B256F8[1]]
DECOMP(005B7EE4, True_005B7EE4) {
    ret(ctx, 1u);
}

// u32 () -> 1  [vtable 00B25748[1]]
DECOMP(005B8610, True_005B8610) {
    ret(ctx, 1u);
}

// u32 () -> 1  [vtable 00B26118[1]]
DECOMP(005D705C, True_005D705C) {
    ret(ctx, 1u);
}

// u32 (this): this->+20  [vtable 00B23148[3], 00B23EC0[3]]
DECOMP(0079ED80, Get20_0079ED80) {
    ret(ctx, load<u32>(arg<u32>(ctx, 0) + 20));
}

// u32 () -> 0  [vtable 00B23170[2]]
DECOMP(0079EDDC, False_0079EDDC) {
    ret(ctx, 0u);
}

// u32 () -> 0  [vtable 00B23190[2]]
DECOMP(0079F2CC, False_0079F2CC) {
    ret(ctx, 0u);
}

// u32 () -> 0  [vtable 00B231B0[2]]
DECOMP(0079F7FC, False_0079F7FC) {
    ret(ctx, 0u);
}

// u32 () -> 0  [vtable 00B231D0[2]]
DECOMP(0079FE14, False_0079FE14) {
    ret(ctx, 0u);
}

// u32 () -> 0  [vtable 00B232D8[4], 00B23308[4]]
DECOMP(007A08A4, False_007A08A4) {
    ret(ctx, 0u);
}

// void (this, u8 v): this->+4 = v  [vtable 00B234C8[2], 00B234E8[2], 00B23514[2], 00B2353C[2] and 15 more]
DECOMP(007A16BC, SetByte4_007A16BC) {
    store<u8>(arg<u32>(ctx, 0) + 4, arg<u8>(ctx, 1));
}

// f32 (this): this->+84  [vtable 00B23CF8[7], 00B23D50[7], 00B24158[7], 00B24228[7]]
DECOMP(007A2328, GetF84_007A2328) {
    ctx.fpr[1] = static_cast<f64>(load<f32>(arg<u32>(ctx, 0) + 84));
}

// void (this, f32 v): this->+84 = v  [vtable 00B23CF8[6], 00B23D50[6], 00B24158[6], 00B24228[6]]
DECOMP(007A2330, SetF84_007A2330) {
    store<f32>(arg<u32>(ctx, 0) + 84, static_cast<f32>(farg(ctx, 0)));
}

// u32 () -> 1  [vtable 00B23F00[1], 00B241B0[1], 00B24F18[1], 00B24F78[1] and 7 more]
DECOMP(007A349C, True_007A349C) {
    ret(ctx, 1u);
}

// u32 () -> 0  [vtable 00B24310[10], 00B244C0[10]]
DECOMP(007A4D54, False_007A4D54) {
    ret(ctx, 0u);
}

// u32 () -> 0  [vtable 00B24310[12]]
DECOMP(007A4D60, False_007A4D60) {
    ret(ctx, 0u);
}

// u64 (this): this->+288 (ld)  [vtable 00B243A8[6]]
DECOMP(007A4DF0, Get288_007A4DF0) {
    ctx.gpr[3] = load<u64>(arg<u32>(ctx, 0) + 288);
}

// u32 () -> 1  [called directly]
DECOMP(007A527C, True_007A527C) {
    ret(ctx, 1u);
}

// u32 () -> 1  [vtable 00B244C0[12]]
DECOMP(007A5DD4, True_007A5DD4) {
    ret(ctx, 1u);
}

// u32 () -> 0  [vtable 00B245D8[2]]
DECOMP(007A5F7C, False_007A5F7C) {
    ret(ctx, 0u);
}

// void (u32 v): *(u32*)0x00F66FC8 = v  [called directly]
DECOMP(00527650, SetGlobal_00527650) {
    store<u32>(kGlobal00F66FC8, arg<u32>(ctx, 0));
    ctx.gpr[4] = 0x00F60000u;  // lis
}

// void (f32 v): *(f32*)0x00F66FCC = v  [called directly]
DECOMP(0052765C, SetGlobalF_0052765C) {
    store<f32>(kGlobal00F66FCC, static_cast<f32>(farg(ctx, 0)));
    ctx.gpr[3] = 0x00F60000u;  // lis r3 (void: r3 left as the high half)
}

// void (this): this->+512 = this->+416  [vtable 00B23CD0[3]]
DECOMP(0052AA48, Copy416To512_0052AA48) {
    const u32 self = arg<u32>(ctx, 0);
    const u32 v = load<u32>(self + 416);
    store<u32>(self + 512, v);
    ctx.gpr[4] = v;
}

// void (this, const f32* v): this->+36 = *v (raw word; leaves f1 = *v)  [vtable 00B23170[5]]
DECOMP(0079EEAC, CopyF36_0079EEAC) {
    const u32 v = load<u32>(arg<u32>(ctx, 1));  // lfs / stfs: raw word
    store<u32>(arg<u32>(ctx, 0) + 36, v);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(v));
}

// void (this, const f32* v): this->+36 = *v (raw word; leaves f1 = *v)  [vtable 00B23190[5]]
DECOMP(0079F39C, CopyF36_0079F39C) {
    const u32 v = load<u32>(arg<u32>(ctx, 1));  // lfs / stfs: raw word
    store<u32>(arg<u32>(ctx, 0) + 36, v);
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(v));
}

// void* (this): this + 16  [vtable 00B23380[3], 00B23EE0[3]]
DECOMP(007A0E8C, Plus16_007A0E8C) {
    ret(ctx, static_cast<u32>(ctx.gpr[3] + 16));  // addic, rldicl 0,32
}

// void* (this): this + 16  [vtable 00B236C0[3], 00B240D0[3]]
DECOMP(007A2154, Plus16_007A2154) {
    ret(ctx, static_cast<u32>(ctx.gpr[3] + 16));  // addic, rldicl 0,32
}

// s32 (): *(s32*)0x010F0E04  [called directly]
DECOMP(00533778, GetGlobal_00533778) {
    ret(ctx, static_cast<s32>(load<u32>(kGlobal010F0E04)));  // extsw
}

// void (this): this->+652 ^= 1  [vtable 00B23D28[4], 00B24130[4]]
DECOMP(005493F4, ToggleByte652_005493F4) {
    const u32 self = arg<u32>(ctx, 0);
    const u8 v = static_cast<u8>(load<u8>(self + 652) ^ 1);
    store<u8>(self + 652, v);
    ctx.gpr[4] = v;
}

// u32 (this, x): x->+16->+12->+0  [vtable 00B253F0[4]]
DECOMP(005B1AD4, Get_005B1AD4) {
    ret(ctx, load<u32>(load<u32>(load<u32>(arg<u32>(ctx, 1) + 16) + 12)));
}

// u32 (): (**(u32**)0x00F488BC)->+8 (TOC -32580)  [called directly]
DECOMP(005DF950, GetToc_005DF950) {
    const u32 r11 = load<u32>(kToc00F488BC);
    const u32 r9 = load<u32>(r11);
    ret(ctx, load<u32>(r9 + 8));
    ctx.gpr[11] = r11;
    ctx.gpr[9] = r9;
}

// void (this): this->vtable = 0x00B23160  [vtable 00B23170[0]]
DECOMP(0079C884, Destruct_0079C884) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23160u);  // the base vtable
    ctx.gpr[4] = 0x00B23160u;
}

// void (this): this->vtable = 0x00B23160  [vtable 00B23190[0]]
DECOMP(0079C894, Destruct_0079C894) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23160u);  // the base vtable
    ctx.gpr[4] = 0x00B23160u;
}

// void (this): this->vtable = 0x00B23160  [vtable 00B231B0[0]]
DECOMP(0079C8A4, Destruct_0079C8A4) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23160u);  // the base vtable
    ctx.gpr[4] = 0x00B23160u;
}

// void (this): this->vtable = 0x00B23160  [vtable 00B231D0[0]]
DECOMP(0079C8B4, Destruct_0079C8B4) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23160u);  // the base vtable
    ctx.gpr[4] = 0x00B23160u;
}

// void (this): this->vtable = 0x00B234C8  [vtable 00B23F38[0]]
DECOMP(0079E428, Destruct_0079E428) {
    store<u32>(arg<u32>(ctx, 0), 0x00B234C8u);  // the base vtable
    ctx.gpr[4] = 0x00B234C8u;
}

// void (this): this->vtable = 0x00B234C8  [vtable 00B23514[0]]
DECOMP(0079E438, Destruct_0079E438) {
    store<u32>(arg<u32>(ctx, 0), 0x00B234C8u);  // the base vtable
    ctx.gpr[4] = 0x00B234C8u;
}

// void (this): this->vtable = 0x00B234C8  [vtable 00B23F60[0]]
DECOMP(0079E448, Destruct_0079E448) {
    store<u32>(arg<u32>(ctx, 0), 0x00B234C8u);  // the base vtable
    ctx.gpr[4] = 0x00B234C8u;
}

// void (this): this->vtable = 0x00B234C8  [vtable 00B2353C[0]]
DECOMP(0079E458, Destruct_0079E458) {
    store<u32>(arg<u32>(ctx, 0), 0x00B234C8u);  // the base vtable
    ctx.gpr[4] = 0x00B234C8u;
}

// void (this): this->vtable = 0x00B234C8  [vtable 00B240B0[0]]
DECOMP(0079E468, Destruct_0079E468) {
    store<u32>(arg<u32>(ctx, 0), 0x00B234C8u);  // the base vtable
    ctx.gpr[4] = 0x00B234C8u;
}

// void (this): this->vtable = 0x00B234C8  [vtable 00B2356C[0]]
DECOMP(0079E478, Destruct_0079E478) {
    store<u32>(arg<u32>(ctx, 0), 0x00B234C8u);  // the base vtable
    ctx.gpr[4] = 0x00B234C8u;
}

// void (this): this->vtable = 0x00B234C8  [vtable 00B23F90[0]]
DECOMP(0079E488, Destruct_0079E488) {
    store<u32>(arg<u32>(ctx, 0), 0x00B234C8u);  // the base vtable
    ctx.gpr[4] = 0x00B234C8u;
}

// void (this): this->vtable = 0x00B234C8  [vtable 00B23590[0]]
DECOMP(0079E498, Destruct_0079E498) {
    store<u32>(arg<u32>(ctx, 0), 0x00B234C8u);  // the base vtable
    ctx.gpr[4] = 0x00B234C8u;
}

// void (this): this->vtable = 0x00B234C8  [vtable 00B235BC[0]]
DECOMP(0079E4A8, Destruct_0079E4A8) {
    store<u32>(arg<u32>(ctx, 0), 0x00B234C8u);  // the base vtable
    ctx.gpr[4] = 0x00B234C8u;
}

// void (this): this->vtable = 0x00B234C8  [vtable 00B23FC0[0]]
DECOMP(0079E4B8, Destruct_0079E4B8) {
    store<u32>(arg<u32>(ctx, 0), 0x00B234C8u);  // the base vtable
    ctx.gpr[4] = 0x00B234C8u;
}

// void (this): this->vtable = 0x00B234C8  [vtable 00B23FF0[0]]
DECOMP(0079E6BC, Destruct_0079E6BC) {
    store<u32>(arg<u32>(ctx, 0), 0x00B234C8u);  // the base vtable
    ctx.gpr[4] = 0x00B234C8u;
}

// void (this): this->vtable = 0x00B234C8  [vtable 00B24038[0]]
DECOMP(0079E6CC, Destruct_0079E6CC) {
    store<u32>(arg<u32>(ctx, 0), 0x00B234C8u);  // the base vtable
    ctx.gpr[4] = 0x00B234C8u;
}

// void (this): this->vtable = 0x00B234C8  [vtable 00B24090[0]]
DECOMP(0079E7A0, Destruct_0079E7A0) {
    store<u32>(arg<u32>(ctx, 0), 0x00B234C8u);  // the base vtable
    ctx.gpr[4] = 0x00B234C8u;
}

// void (this): this->vtable = 0x00B23CF8  [vtable 00B24158[0]]
DECOMP(0079E9A4, Destruct_0079E9A4) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23CF8u);  // the base vtable
    ctx.gpr[4] = 0x00B23CF8u;
}

// void (this): this->vtable = 0x00B23CF8  [vtable 00B23D50[0]]
DECOMP(0079E9E4, Destruct_0079E9E4) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23CF8u);  // the base vtable
    ctx.gpr[4] = 0x00B23CF8u;
}

// void (this): this->vtable = 0x00B23160  [vtable 00B23160[0]]
DECOMP(0079ED88, Destruct_0079ED88) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23160u);  // the base vtable
    ctx.gpr[4] = 0x00B23160u;
}

// void (this): this->vtable = 0x00B231F0  [vtable 00B231F0[0]]
DECOMP(007A0374, Destruct_007A0374) {
    store<u32>(arg<u32>(ctx, 0), 0x00B231F0u);  // the base vtable
    ctx.gpr[4] = 0x00B231F0u;
}

// void (this): this->vtable = 0x00B231F0  [vtable 00B23200[0]]
DECOMP(007A03C8, Destruct_007A03C8) {
    store<u32>(arg<u32>(ctx, 0), 0x00B231F0u);  // the base vtable
    ctx.gpr[4] = 0x00B231F0u;
}

// void (this): this->vtable = 0x00B23210  [vtable 00B23210[0]]
DECOMP(007A041C, Destruct_007A041C) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23210u);  // the base vtable
    ctx.gpr[4] = 0x00B23210u;
}

// void (this): this->vtable = 0x00B23210  [vtable 00B23220[0]]
DECOMP(007A0470, Destruct_007A0470) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23210u);  // the base vtable
    ctx.gpr[4] = 0x00B23210u;
}

// void (this): this->vtable = 0x00B23230  [vtable 00B23230[0]]
DECOMP(007A04C4, Destruct_007A04C4) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23230u);  // the base vtable
    ctx.gpr[4] = 0x00B23230u;
}

// void (this): this->vtable = 0x00B23230  [vtable 00B23240[0]]
DECOMP(007A0518, Destruct_007A0518) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23230u);  // the base vtable
    ctx.gpr[4] = 0x00B23230u;
}

// void (this): this->vtable = 0x00B23250  [vtable 00B23250[0]]
DECOMP(007A056C, Destruct_007A056C) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23250u);  // the base vtable
    ctx.gpr[4] = 0x00B23250u;
}

// void (this): this->vtable = 0x00B23250  [vtable 00B23260[0]]
DECOMP(007A05C0, Destruct_007A05C0) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23250u);  // the base vtable
    ctx.gpr[4] = 0x00B23250u;
}

// void (this): this->vtable = 0x00B23270  [vtable 00B23270[0]]
DECOMP(007A0614, Destruct_007A0614) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23270u);  // the base vtable
    ctx.gpr[4] = 0x00B23270u;
}

// void (this): this->vtable = 0x00B23270  [vtable 00B23280[0]]
DECOMP(007A0668, Destruct_007A0668) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23270u);  // the base vtable
    ctx.gpr[4] = 0x00B23270u;
}

// void (this): this->vtable = 0x00B23290  [vtable 00B23290[0]]
DECOMP(007A06BC, Destruct_007A06BC) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23290u);  // the base vtable
    ctx.gpr[4] = 0x00B23290u;
}

// void (this): this->vtable = 0x00B23290  [vtable 00B232A0[0]]
DECOMP(007A0710, Destruct_007A0710) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23290u);  // the base vtable
    ctx.gpr[4] = 0x00B23290u;
}

// void (this): this->vtable = 0x00B232B0  [vtable 00B232B0[0]]
DECOMP(007A0764, Destruct_007A0764) {
    store<u32>(arg<u32>(ctx, 0), 0x00B232B0u);  // the base vtable
    ctx.gpr[4] = 0x00B232B0u;
}

// s32 (this): this->+72->+20  [vtable 00B23308[7], 00B23338[7]]
DECOMP(007A08AC, Get72_20_007A08AC) {
    ret(ctx, static_cast<s32>(load<u32>(load<u32>(arg<u32>(ctx, 0) + 72) + 20)));  // extsw
}

// s32 (this): this->+72->+20  [vtable 00B23308[8], 00B23338[8]]
DECOMP(007A08BC, Get72_20_007A08BC) {
    ret(ctx, static_cast<s32>(load<u32>(load<u32>(arg<u32>(ctx, 0) + 72) + 20)));  // extsw
}

// void (this): this->vtable = 0x00B233A0  [vtable 00B233A0[0]]
DECOMP(007A0EB4, Destruct_007A0EB4) {
    store<u32>(arg<u32>(ctx, 0), 0x00B233A0u);  // the base vtable
    ctx.gpr[4] = 0x00B233A0u;
}

// void (this): this->vtable = 0x00B234C8  [vtable 00B234C8[0]]
DECOMP(007A1668, Destruct_007A1668) {
    store<u32>(arg<u32>(ctx, 0), 0x00B234C8u);  // the base vtable
    ctx.gpr[4] = 0x00B234C8u;
}

// void (this): this->vtable = 0x00B234C8  [vtable 00B234E8[0]]
DECOMP(007A16D8, Destruct_007A16D8) {
    store<u32>(arg<u32>(ctx, 0), 0x00B234C8u);  // the base vtable
    ctx.gpr[4] = 0x00B234C8u;
}

// void (this): this->vtable = 0x00B234C8  [vtable 00B235DC[0]]
DECOMP(007A189C, Destruct_007A189C) {
    store<u32>(arg<u32>(ctx, 0), 0x00B234C8u);  // the base vtable
    ctx.gpr[4] = 0x00B234C8u;
}

// void (this): this->vtable = 0x00B234C8  [vtable 00B23610[0]]
DECOMP(007A1A3C, Destruct_007A1A3C) {
    store<u32>(arg<u32>(ctx, 0), 0x00B234C8u);  // the base vtable
    ctx.gpr[4] = 0x00B234C8u;
}

// void (this): this->vtable = 0x00B234C8  [vtable 00B23630[0]]
DECOMP(007A1A90, Destruct_007A1A90) {
    store<u32>(arg<u32>(ctx, 0), 0x00B234C8u);  // the base vtable
    ctx.gpr[4] = 0x00B234C8u;
}

// void (this): this->vtable = 0x00B234C8  [vtable 00B23670[0]]
DECOMP(007A1AE8, Destruct_007A1AE8) {
    store<u32>(arg<u32>(ctx, 0), 0x00B234C8u);  // the base vtable
    ctx.gpr[4] = 0x00B234C8u;
}

// void (this): this->vtable = 0x00B23CD0  [vtable 00B23CD0[0]]
DECOMP(007A2278, Destruct_007A2278) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23CD0u);  // the base vtable
    ctx.gpr[4] = 0x00B23CD0u;
}

// void (this): this->vtable = 0x00B23CF8  [vtable 00B23CF8[0]]
DECOMP(007A22D4, Destruct_007A22D4) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23CF8u);  // the base vtable
    ctx.gpr[4] = 0x00B23CF8u;
}

// void (this): this->vtable = 0x00B23CF8  [vtable 00B24228[0]]
DECOMP(007A342C, Destruct_007A342C) {
    store<u32>(arg<u32>(ctx, 0), 0x00B23CF8u);  // the base vtable
    ctx.gpr[4] = 0x00B23CF8u;
}

// void (this): this->vtable = 0x00B233A0  [vtable 00B24190[0]]
DECOMP(007A343C, Destruct_007A343C) {
    store<u32>(arg<u32>(ctx, 0), 0x00B233A0u);  // the base vtable
    ctx.gpr[4] = 0x00B233A0u;
}

// void (this): this->vtable = 0x00B24278  [vtable 00B24368[3]]
DECOMP(007A3DB8, Destruct_007A3DB8) {
    store<u32>(arg<u32>(ctx, 0), 0x00B24278u);  // the base vtable
    ctx.gpr[4] = 0x00B24278u;
}

// void (this): this->vtable = 0x00B24278  [vtable 00B24388[3]]
DECOMP(007A3DC8, Destruct_007A3DC8) {
    store<u32>(arg<u32>(ctx, 0), 0x00B24278u);  // the base vtable
    ctx.gpr[4] = 0x00B24278u;
}

// void (this): this->vtable = 0x00B24278  [vtable 00B24278[3]]
DECOMP(007A4B50, Destruct_007A4B50) {
    store<u32>(arg<u32>(ctx, 0), 0x00B24278u);  // the base vtable
    ctx.gpr[4] = 0x00B24278u;
}

// void (this): this->vtable = 0x00B242E8  [vtable 00B243A8[0]]
DECOMP(007A4D98, Destruct_007A4D98) {
    store<u32>(arg<u32>(ctx, 0), 0x00B242E8u);  // the base vtable
    ctx.gpr[4] = 0x00B242E8u;
}

// void (this): this->vtable = 0x00B24458  [vtable 00B24478[2]]
DECOMP(007A5340, Destruct_007A5340) {
    store<u32>(arg<u32>(ctx, 0), 0x00B24458u);  // the base vtable
    ctx.gpr[4] = 0x00B24458u;
}

// void (this): this->vtable = 0x00B24458  [vtable 00B24458[2]]
DECOMP(007A5D3C, Destruct_007A5D3C) {
    store<u32>(arg<u32>(ctx, 0), 0x00B24458u);  // the base vtable
    ctx.gpr[4] = 0x00B24458u;
}

// void (this): this->vtable = 0x00B24500  [vtable 00B24500[3]]
DECOMP(007A5EBC, Destruct_007A5EBC) {
    store<u32>(arg<u32>(ctx, 0), 0x00B24500u);  // the base vtable
    ctx.gpr[4] = 0x00B24500u;
}

// void (this): this->vtable = 0x00B24500  [vtable 00B245D8[3]]
DECOMP(007A5ECC, Destruct_007A5ECC) {
    store<u32>(arg<u32>(ctx, 0), 0x00B24500u);  // the base vtable
    ctx.gpr[4] = 0x00B24500u;
}

// void (this): this->vtable = 0x00B2451C  [vtable 00B2451C[0]]
DECOMP(007A5FC8, Destruct_007A5FC8) {
    store<u32>(arg<u32>(ctx, 0), 0x00B2451Cu);  // the base vtable
    ctx.gpr[4] = 0x00B2451Cu;
}

// u32 (this): min(this->u16 +40, 256)  [called directly]
DECOMP(00500534, Min256_00500534) {
    const u32 v = load<u16>(arg<u32>(ctx, 0) + 40);
    ret(ctx, v > 0x100 ? 0x100u : v);  // cmplwi
}

// void (this): this->+28 = *(u32*)0x010DDA78  [called directly]
DECOMP(00500FB4, SetDrawCount_00500FB4) {
    const u32 v = load<u32>(kGlobal010DDA78);
    store<u32>(arg<u32>(ctx, 0) + 28, v);
    ctx.gpr[4] = v;
}

// void (this, f32 v): this->+112 = v, this->+52 |= 0x80  [called directly]
DECOMP(00520094, SetF112Flag_00520094) {
    const u32 self = arg<u32>(ctx, 0);
    const u8 flags = static_cast<u8>(load<u8>(self + 52) | 0x80);  // lbz before the stfs
    store<f32>(self + 112, static_cast<f32>(farg(ctx, 0)));
    store<u8>(self + 52, flags);
    ctx.gpr[4] = flags;
}

// void (): ++*(u32*)0x010F0E00  [called directly]
DECOMP(00532D3C, VBlank_Handler) {
    const u64 v = static_cast<u64>(load<u32>(kGlobal010F0E00)) + 1;  // addic on the zero-extended word
    store<u32>(kGlobal010F0E00, static_cast<u32>(v));
    ctx.gpr[3] = 0x010F0000u;  // lis r3
    ctx.gpr[4] = v;
}

// void* (this): this + 0x12080  [called directly]
DECOMP(005382A8, Plus12080_005382A8) {
    ret(ctx, static_cast<u32>(ctx.gpr[3] + 0x12080));  // addc, rldicl 0,32
    ctx.gpr[4] = 0x12080;
}

// bool (this): this->+56 != 0  [called directly]
DECOMP(0053A9C4, Has56_0053A9C4) {
    ret(ctx, load<u32>(arg<u32>(ctx, 0) + 56) != 0 ? 1u : 0u);  // cntlzw / rlwinm 27,31,31 / xori
}

// bool (this): this->u8 +4 != 0  [vtable 00B234C8[3], 00B234E8[3], 00B23514[3], 00B2356C[3] and 15 more]
DECOMP(007A16C4, HasByte4_007A16C4) {
    ret(ctx, load<u8>(arg<u32>(ctx, 0) + 4) != 0 ? 1u : 0u);
}

// bool (this): this->u8 +4 != 0  [vtable 00B2353C[3], 00B240B0[3]]
DECOMP(007A1774, HasByte4_007A1774) {
    ret(ctx, load<u8>(arg<u32>(ctx, 0) + 4) != 0 ? 1u : 0u);
}

// bool (this, x): x->+40 == 0, after an lwsync  [vtable 00B24558[4], 00B245A4[4]]
DECOMP(007A6484, IsZero40_007A6484) {
    std::atomic_thread_fence(std::memory_order_acquire);  // lwsync
    ret(ctx, load<u32>(arg<u32>(ctx, 1) + 40) == 0 ? 1u : 0u);
}

// void (this, bool a, bool b, bool c): this->u8 +54 bits 0x80 / 0x40 / 0x20 = a / b / c (bit 0 of each)  [called directly]
DECOMP(0050A214, SetFlags54_0050A214) {
    const u32 self = arg<u32>(ctx, 0);
    u32 v = load<u8>(self + 54) & ~0xE0u;
    v |= (arg<u32>(ctx, 1) & 1) << 7 | (arg<u32>(ctx, 2) & 1) << 6 | (arg<u32>(ctx, 3) & 1) << 5;  // rlwimi 7,24,24 / 6,25,25 / 5,26,26
    store<u8>(self + 54, static_cast<u8>(v));
    ctx.gpr[7] = v;
}

// void (u32 v): *(u32*)0x010C5A08 = v, byte +4 = 1  [called directly]
DECOMP(0051299C, SetGlobal_0051299C) {
    store<u32>(kGlobal010C5A08, arg<u32>(ctx, 0));
    store<u8>(kGlobal010C5A08 + 4, 1);
    ctx.gpr[4] = 0x010C0000u;
    ctx.gpr[5] = 1;
    ctx.gpr[6] = kGlobal010C5A08;
}

// void (): the two u32s at 0x01195090 = 0  [called directly]
DECOMP(005AC634, ClearFlipSlots_005AC634) {
    store<u32>(kGlobal01195090, 0);
    store<u32>(kGlobal01195090 + 4, 0);
    ctx.gpr[3] = 0x01190000u;  // lis r3
    ctx.gpr[4] = 0;
    ctx.gpr[5] = kGlobal01195090;
}
