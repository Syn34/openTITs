// The renderer range's smallest functions: empty and constant vtable
// slots, base-class destructors that only reset the vtable, and one-line
// getters / setters. Names carry the address until the classes are
// known; the comment above each gives the original signature and the
// vtables (and slot indices) that point at it.
#include <atomic>
#include <bit>

#include "gfx7.h"
#include "rsx.h"

namespace sly4::gfx {


namespace {

constexpr u32 kGlobal00F66FC8 = 0x00F66FC8;  // u32 (name?)
constexpr u32 kGlobal00F66FCC = 0x00F66FCC;  // f32 (name?)
constexpr u32 kGlobal010F0E00 = 0x010F0E00;  // u32 counter (DECOMP.md: the VBlank counter) (name?)
constexpr u32 kGlobal010F0E04 = 0x010F0E04;  // u32 (gcm.cpp kUnk010F0E04)
constexpr u32 kGlobal010DDA78 = 0x010DDA78;  // u32 (draw::g::current_surface + 4) (name?)
constexpr u32 kGlobal010C5A08 = 0x010C5A08;  // u32 + u8 flag (graphics.cpp kGfxUnk010C5A08)
constexpr u32 kGlobal01195090 = 0x01195090;  // u32[2] (rjobs.cpp kFlipSlots)
constexpr u32 kToc00F488BC = 0x00F488BC;     // TOC -32580: u32** (name?)

be_u32& word(const void* p, u32 off) { return *at<be_u32>(p, off); }

}  // namespace

// void ()  [called directly]
void Nop_00517FA8() {}

// void ()  [called directly]
void Nop_00518E60() {}

// void ()  [called directly]
void Nop_00518E64() {}

// void ()  [called directly]
void Nop_0051FF18() {}

// void ()  [called directly]
void Nop_00520028() {}

// void ()  [called directly]
void Array_Destruct() {}

// void ()  [vtable 00B253D0[2]]
void Nop_005B19CC() {}

// void ()  [vtable 00B253D0[3]]
void Nop_005B19D0() {}

// void ()  [vtable 00B26178[4]]
void Nop_005D7BCC() {}

// void ()  [vtable 00B234E8[4]]
void Nop_007A172C() {}

// void ()  [vtable 00B235BC[4]]
void Nop_007A1854() {}

// void ()  [vtable 00B235DC[4]]
void Nop_007A18F0() {}

// void ()  [vtable 00B23630[4]]
void Nop_007A1AE4() {}

// void ()  [vtable 00B23670[4]]
void Nop_007A1B3C() {}

// void ()  [vtable 00B23690[0], 00B236A8[0]]
void Nop_007A1B40() {}

// void ()  [vtable 00B23690[1], 00B236A8[1]]
void Nop_007A1B44() {}

// void ()  [vtable 00B236C0[4]]
void Nop_007A2150() {}

// void ()  [vtable 00B23CD0[4]]
void Nop_007A22CC() {}

// void ()  [vtable 00B23CD0[7], 00B24130[7]]
void Nop_007A22D0() {}

// void ()  [vtable 00B23D80[2]]
void Nop_007A2498() {}

// void ()  [vtable 00B23DA0[2]]
void Nop_007A2560() {}

// void ()  [vtable 00B23DE0[2]]
void Nop_007A26F0() {}

// void ()  [vtable 00B23E00[2]]
void Nop_007A27B8() {}

// void ()  [vtable 00B23E20[2]]
void Nop_007A2880() {}

// void ()  [vtable 00B23E40[2]]
void Nop_007A2948() {}

// void ()  [vtable 00B23E60[2]]
void Nop_007A2A10() {}

// void ()  [vtable 00B23E80[2]]
void Nop_007A2AD8() {}

// void ()  [vtable 00B23EA0[2]]
void Nop_007A2BA0() {}

// void ()  [vtable 00B24130[5]]
void Nop_007A3AD4() {}

// void ()  [vtable 00B24190[4]]
void Nop_007A3CFC() {}

// void ()  [vtable 00B24310[5], 00B244C0[5]]
void Nop_007A4D4C() {}

// void ()  [vtable 00B24310[6], 00B244C0[6]]
void Nop_007A4D50() {}

// void ()  [vtable 00B24310[11], 00B244C0[11]]
void Nop_007A4D5C() {}

// void ()  [vtable 00B24418[2]]
void Nop_007A4FF4() {}

// void ()  [vtable 00B24438[2]]
void Nop_007A50B0() {}

// u32 () -> 1  [called directly]
u32 True_0050129C() { return 1; }

// u32 () -> 1  [called directly]
u32 True_00525A1C() { return 1; }

// void (u32* p, u32 v): *p = v  [called directly]
void SetWord_0052F778(be_u32* p, u32 v) { *p = v; }

// u32 () -> 0  [called directly]
u32 False_00537818() { return 0; }

// u32 () -> 1  [called directly]
u32 True_0053B650() { return 1; }

// u32 () -> 1  [vtable 00B25178[1]]
u32 True_005AFBD0() { return 1; }

// u32 () -> 1  [vtable 00B253F0[1]]
u32 True_005B1ACC() { return 1; }

// u32 () -> 1  [vtable 00B256A8[1]]
u32 True_005B7878() { return 1; }

// u32 () -> 1  [vtable 00B256F8[1]]
u32 True_005B7EE4() { return 1; }

// u32 () -> 1  [vtable 00B25748[1]]
u32 True_005B8610() { return 1; }

// u32 () -> 1  [vtable 00B26118[1]]
u32 True_005D705C() { return 1; }

// u32 (this): this->+20  [vtable 00B23148[3], 00B23EC0[3]]
u32 Get20_0079ED80(u8* self) { return word(self, 20); }

// u32 () -> 0  [vtable 00B23170[2]]
u32 False_0079EDDC() { return 0; }

// u32 () -> 0  [vtable 00B23190[2]]
u32 False_0079F2CC() { return 0; }

// u32 () -> 0  [vtable 00B231B0[2]]
u32 False_0079F7FC() { return 0; }

// u32 () -> 0  [vtable 00B231D0[2]]
u32 False_0079FE14() { return 0; }

// u32 () -> 0  [vtable 00B232D8[4], 00B23308[4]]
u32 False_007A08A4() { return 0; }

// void (this, u8 v): this->+4 = v  [vtable 00B234C8[2], 00B234E8[2], 00B23514[2], 00B2353C[2] and 15 more]
void SetByte4_007A16BC(u8* self, u32 v) { *at<u8>(self, 4) = static_cast<u8>(v); }

// f32 (this): this->+84  [vtable 00B23CF8[7], 00B23D50[7], 00B24158[7], 00B24228[7]]
f32 GetF84_007A2328(u8* self) { return *at<be_f32>(self, 84); }

// void (this, f32 v): this->+84 = v  [vtable 00B23CF8[6], 00B23D50[6], 00B24158[6], 00B24228[6]]
void SetF84_007A2330(u8* self, f32 v) { *at<be_f32>(self, 84) = v; }

// u32 () -> 1  [vtable 00B23F00[1], 00B241B0[1], 00B24F18[1], 00B24F78[1] and 7 more]
u32 True_007A349C() { return 1; }

// u32 () -> 0  [vtable 00B24310[10], 00B244C0[10]]
u32 False_007A4D54() { return 0; }

// u32 () -> 0  [vtable 00B24310[12]]
u32 False_007A4D60() { return 0; }

// u64 (this): this->+288 (ld)  [vtable 00B243A8[6]]
u64 Get288_007A4DF0(u8* self) { return *at<be_u64>(self, 288); }

// u32 () -> 1  [called directly]
u32 True_007A527C() { return 1; }

// u32 () -> 1  [vtable 00B244C0[12]]
u32 True_007A5DD4() { return 1; }

// u32 () -> 0  [vtable 00B245D8[2]]
u32 False_007A5F7C() { return 0; }

// void (u32 v): *(u32*)0x00F66FC8 = v  [called directly]
void SetGlobal_00527650(u32 v) { global<be_u32>(kGlobal00F66FC8) = v; }

// void (f32 v): *(f32*)0x00F66FCC = v  [called directly]
void SetGlobalF_0052765C(f32 v) { global<be_f32>(kGlobal00F66FCC) = v; }

// void (this): this->+512 = this->+416  [vtable 00B23CD0[3]]
void Copy416To512_0052AA48(u8* self) { word(self, 512) = word(self, 416).get(); }

// void (this, const f32* v): this->+36 = *v (raw word; leaves f1 = *v)  [vtable 00B23170[5]]
void CopyF36_0079EEAC(u8* self, const be_u32* v) { word(self, 36) = v->get(); }  // lfs / stfs: raw word

// void (this, const f32* v): this->+36 = *v (raw word; leaves f1 = *v)  [vtable 00B23190[5]]
void CopyF36_0079F39C(u8* self, const be_u32* v) { word(self, 36) = v->get(); }  // lfs / stfs: raw word

// void* (this): this + 16  [vtable 00B23380[3], 00B23EE0[3]]
u32 Plus16_007A0E8C(u8* self) { return guest(self) + 16; }  // addic, rldicl 0,32

// void* (this): this + 16  [vtable 00B236C0[3], 00B240D0[3]]
u32 Plus16_007A2154(u8* self) { return guest(self) + 16; }  // addic, rldicl 0,32

// s32 (): *(s32*)0x010F0E04  [called directly]
s32 GetGlobal_00533778() { return static_cast<s32>(global<be_u32>(kGlobal010F0E04).get()); }  // extsw

// void (this): this->+652 ^= 1  [vtable 00B23D28[4], 00B24130[4]]
void ToggleByte652_005493F4(u8* self) { *at<u8>(self, 652) = static_cast<u8>(*at<u8>(self, 652) ^ 1); }

// u32 (this, x): x->+16->+12->+0  [vtable 00B253F0[4]]
u32 Get_005B1AD4(void*, u8* x) { return word(nullptr, word(*at<gp<u8>>(x, 16), 12)); }

// u32 (): (**(u32**)0x00F488BC)->+8 (TOC -32580)  [called directly]
u32 GetToc_005DF950() { return word(nullptr, word(nullptr, global<be_u32>(kToc00F488BC)) + 8); }

// void (this): this->vtable = 0x00B23160  [vtable 00B23170[0]]
void Destruct_0079C884(u8* self) { word(self, 0) = 0x00B23160u; }

// void (this): this->vtable = 0x00B23160  [vtable 00B23190[0]]
void Destruct_0079C894(u8* self) { word(self, 0) = 0x00B23160u; }

// void (this): this->vtable = 0x00B23160  [vtable 00B231B0[0]]
void Destruct_0079C8A4(u8* self) { word(self, 0) = 0x00B23160u; }

// void (this): this->vtable = 0x00B23160  [vtable 00B231D0[0]]
void Destruct_0079C8B4(u8* self) { word(self, 0) = 0x00B23160u; }

// void (this): this->vtable = 0x00B234C8  [vtable 00B23F38[0]]
void Destruct_0079E428(u8* self) { word(self, 0) = 0x00B234C8u; }

// void (this): this->vtable = 0x00B234C8  [vtable 00B23514[0]]
void Destruct_0079E438(u8* self) { word(self, 0) = 0x00B234C8u; }

// void (this): this->vtable = 0x00B234C8  [vtable 00B23F60[0]]
void Destruct_0079E448(u8* self) { word(self, 0) = 0x00B234C8u; }

// void (this): this->vtable = 0x00B234C8  [vtable 00B2353C[0]]
void Destruct_0079E458(u8* self) { word(self, 0) = 0x00B234C8u; }

// void (this): this->vtable = 0x00B234C8  [vtable 00B240B0[0]]
void Destruct_0079E468(u8* self) { word(self, 0) = 0x00B234C8u; }

// void (this): this->vtable = 0x00B234C8  [vtable 00B2356C[0]]
void Destruct_0079E478(u8* self) { word(self, 0) = 0x00B234C8u; }

// void (this): this->vtable = 0x00B234C8  [vtable 00B23F90[0]]
void Destruct_0079E488(u8* self) { word(self, 0) = 0x00B234C8u; }

// void (this): this->vtable = 0x00B234C8  [vtable 00B23590[0]]
void Destruct_0079E498(u8* self) { word(self, 0) = 0x00B234C8u; }

// void (this): this->vtable = 0x00B234C8  [vtable 00B235BC[0]]
void Destruct_0079E4A8(u8* self) { word(self, 0) = 0x00B234C8u; }

// void (this): this->vtable = 0x00B234C8  [vtable 00B23FC0[0]]
void Destruct_0079E4B8(u8* self) { word(self, 0) = 0x00B234C8u; }

// void (this): this->vtable = 0x00B234C8  [vtable 00B23FF0[0]]
void Destruct_0079E6BC(u8* self) { word(self, 0) = 0x00B234C8u; }

// void (this): this->vtable = 0x00B234C8  [vtable 00B24038[0]]
void Destruct_0079E6CC(u8* self) { word(self, 0) = 0x00B234C8u; }

// void (this): this->vtable = 0x00B234C8  [vtable 00B24090[0]]
void Destruct_0079E7A0(u8* self) { word(self, 0) = 0x00B234C8u; }

// void (this): this->vtable = 0x00B23CF8  [vtable 00B24158[0]]
void Destruct_0079E9A4(u8* self) { word(self, 0) = 0x00B23CF8u; }

// void (this): this->vtable = 0x00B23CF8  [vtable 00B23D50[0]]
void Destruct_0079E9E4(u8* self) { word(self, 0) = 0x00B23CF8u; }

// void (this): this->vtable = 0x00B23160  [vtable 00B23160[0]]
void Destruct_0079ED88(u8* self) { word(self, 0) = 0x00B23160u; }

// void (this): this->vtable = 0x00B231F0  [vtable 00B231F0[0]]
void Destruct_007A0374(u8* self) { word(self, 0) = 0x00B231F0u; }

// void (this): this->vtable = 0x00B231F0  [vtable 00B23200[0]]
void Destruct_007A03C8(u8* self) { word(self, 0) = 0x00B231F0u; }

// void (this): this->vtable = 0x00B23210  [vtable 00B23210[0]]
void Destruct_007A041C(u8* self) { word(self, 0) = 0x00B23210u; }

// void (this): this->vtable = 0x00B23210  [vtable 00B23220[0]]
void Destruct_007A0470(u8* self) { word(self, 0) = 0x00B23210u; }

// void (this): this->vtable = 0x00B23230  [vtable 00B23230[0]]
void Destruct_007A04C4(u8* self) { word(self, 0) = 0x00B23230u; }

// void (this): this->vtable = 0x00B23230  [vtable 00B23240[0]]
void Destruct_007A0518(u8* self) { word(self, 0) = 0x00B23230u; }

// void (this): this->vtable = 0x00B23250  [vtable 00B23250[0]]
void Destruct_007A056C(u8* self) { word(self, 0) = 0x00B23250u; }

// void (this): this->vtable = 0x00B23250  [vtable 00B23260[0]]
void Destruct_007A05C0(u8* self) { word(self, 0) = 0x00B23250u; }

// void (this): this->vtable = 0x00B23270  [vtable 00B23270[0]]
void Destruct_007A0614(u8* self) { word(self, 0) = 0x00B23270u; }

// void (this): this->vtable = 0x00B23270  [vtable 00B23280[0]]
void Destruct_007A0668(u8* self) { word(self, 0) = 0x00B23270u; }

// void (this): this->vtable = 0x00B23290  [vtable 00B23290[0]]
void Destruct_007A06BC(u8* self) { word(self, 0) = 0x00B23290u; }

// void (this): this->vtable = 0x00B23290  [vtable 00B232A0[0]]
void Destruct_007A0710(u8* self) { word(self, 0) = 0x00B23290u; }

// void (this): this->vtable = 0x00B232B0  [vtable 00B232B0[0]]
void Destruct_007A0764(u8* self) { word(self, 0) = 0x00B232B0u; }

// s32 (this): this->+72->+20  [vtable 00B23308[7], 00B23338[7]]
s32 Get72_20_007A08AC(u8* self) { return static_cast<s32>(word(*at<gp<u8>>(self, 72), 20).get()); }  // extsw

// s32 (this): this->+72->+20  [vtable 00B23308[8], 00B23338[8]]
s32 Get72_20_007A08BC(u8* self) { return static_cast<s32>(word(*at<gp<u8>>(self, 72), 20).get()); }  // extsw

// void (this): this->vtable = 0x00B233A0  [vtable 00B233A0[0]]
void Destruct_007A0EB4(u8* self) { word(self, 0) = 0x00B233A0u; }

// void (this): this->vtable = 0x00B234C8  [vtable 00B234C8[0]]
void Destruct_007A1668(u8* self) { word(self, 0) = 0x00B234C8u; }

// void (this): this->vtable = 0x00B234C8  [vtable 00B234E8[0]]
void Destruct_007A16D8(u8* self) { word(self, 0) = 0x00B234C8u; }

// void (this): this->vtable = 0x00B234C8  [vtable 00B235DC[0]]
void Destruct_007A189C(u8* self) { word(self, 0) = 0x00B234C8u; }

// void (this): this->vtable = 0x00B234C8  [vtable 00B23610[0]]
void Destruct_007A1A3C(u8* self) { word(self, 0) = 0x00B234C8u; }

// void (this): this->vtable = 0x00B234C8  [vtable 00B23630[0]]
void Destruct_007A1A90(u8* self) { word(self, 0) = 0x00B234C8u; }

// void (this): this->vtable = 0x00B234C8  [vtable 00B23670[0]]
void Destruct_007A1AE8(u8* self) { word(self, 0) = 0x00B234C8u; }

// void (this): this->vtable = 0x00B23CD0  [vtable 00B23CD0[0]]
void Destruct_007A2278(u8* self) { word(self, 0) = 0x00B23CD0u; }

// void (this): this->vtable = 0x00B23CF8  [vtable 00B23CF8[0]]
void Destruct_007A22D4(u8* self) { word(self, 0) = 0x00B23CF8u; }

// void (this): this->vtable = 0x00B23CF8  [vtable 00B24228[0]]
void Destruct_007A342C(u8* self) { word(self, 0) = 0x00B23CF8u; }

// void (this): this->vtable = 0x00B233A0  [vtable 00B24190[0]]
void Destruct_007A343C(u8* self) { word(self, 0) = 0x00B233A0u; }

// void (this): this->vtable = 0x00B24278  [vtable 00B24368[3]]
void Destruct_007A3DB8(u8* self) { word(self, 0) = 0x00B24278u; }

// void (this): this->vtable = 0x00B24278  [vtable 00B24388[3]]
void Destruct_007A3DC8(u8* self) { word(self, 0) = 0x00B24278u; }

// void (this): this->vtable = 0x00B24278  [vtable 00B24278[3]]
void Destruct_007A4B50(u8* self) { word(self, 0) = 0x00B24278u; }

// void (this): this->vtable = 0x00B242E8  [vtable 00B243A8[0]]
void Destruct_007A4D98(u8* self) { word(self, 0) = 0x00B242E8u; }

// void (this): this->vtable = 0x00B24458  [vtable 00B24478[2]]
void Destruct_007A5340(u8* self) { word(self, 0) = 0x00B24458u; }

// void (this): this->vtable = 0x00B24458  [vtable 00B24458[2]]
void Destruct_007A5D3C(u8* self) { word(self, 0) = 0x00B24458u; }

// void (this): this->vtable = 0x00B24500  [vtable 00B24500[3]]
void Destruct_007A5EBC(u8* self) { word(self, 0) = 0x00B24500u; }

// void (this): this->vtable = 0x00B24500  [vtable 00B245D8[3]]
void Destruct_007A5ECC(u8* self) { word(self, 0) = 0x00B24500u; }

// void (this): this->vtable = 0x00B2451C  [vtable 00B2451C[0]]
void Destruct_007A5FC8(u8* self) { word(self, 0) = 0x00B2451Cu; }

// u32 (this): min(this->u16 +40, 256)  [called directly]
u32 Min256_00500534(u8* self) {
    const u32 v = *at<be_u16>(self, 40);
    return v > 0x100 ? 0x100u : v;  // cmplwi
}

// void (this): this->+28 = *(u32*)0x010DDA78  [called directly]
void SetDrawCount_00500FB4(u8* self) { word(self, 28) = global<be_u32>(kGlobal010DDA78).get(); }

// void (this, f32 v): this->+112 = v, this->+52 |= 0x80  [called directly]
void SetF112Flag_00520094(u8* self, f32 v) {
    const u8 flags = static_cast<u8>(*at<u8>(self, 52) | 0x80);  // lbz before the stfs
    *at<be_f32>(self, 112) = v;
    *at<u8>(self, 52) = flags;
}

// void (): ++*(u32*)0x010F0E00  [called directly]
void VBlank_Handler() { global<be_u32>(kGlobal010F0E00) = global<be_u32>(kGlobal010F0E00) + 1u; }

// void* (this): this + 0x12080  [called directly]
u32 Plus12080_005382A8(u8* self) { return guest(self) + 0x12080; }  // addc, rldicl 0,32

// bool (this): this->+56 != 0  [called directly]
u32 Has56_0053A9C4(u8* self) { return word(self, 56) != 0 ? 1u : 0u; }  // cntlzw / rlwinm 27,31,31 / xori

// bool (this): this->u8 +4 != 0  [vtable 00B234C8[3], 00B234E8[3], 00B23514[3], 00B2356C[3] and 15 more]
u32 HasByte4_007A16C4(u8* self) { return *at<u8>(self, 4) != 0 ? 1u : 0u; }

// bool (this): this->u8 +4 != 0  [vtable 00B2353C[3], 00B240B0[3]]
u32 HasByte4_007A1774(u8* self) { return *at<u8>(self, 4) != 0 ? 1u : 0u; }

// bool (this, x): x->+40 == 0, after an lwsync  [vtable 00B24558[4], 00B245A4[4]]
u32 IsZero40_007A6484(void*, u8* x) {
    std::atomic_thread_fence(std::memory_order_acquire);  // lwsync
    return word(x, 40) == 0 ? 1u : 0u;
}

// void (this, bool a, bool b, bool c): this->u8 +54 bits 0x80 / 0x40 / 0x20 = a / b / c (bit 0 of each)  [called directly]
void SetFlags54_0050A214(u8* self, u32 a, u32 b, u32 c) {
    u32 v = *at<u8>(self, 54) & ~0xE0u;
    v |= (a & 1) << 7 | (b & 1) << 6 | (c & 1) << 5;  // rlwimi 7,24,24 / 6,25,25 / 5,26,26
    *at<u8>(self, 54) = static_cast<u8>(v);
}

// void (u32 v): *(u32*)0x010C5A08 = v, byte +4 = 1  [called directly]
void SetGlobal_0051299C(u32 v) {
    global<be_u32>(kGlobal010C5A08) = v;
    global<u8>(kGlobal010C5A08 + 4) = 1;
}

// void (): the two u32s at 0x01195090 = 0  [called directly]
void ClearFlipSlots_005AC634() {
    global<be_u32>(kGlobal01195090) = 0u;
    global<be_u32>(kGlobal01195090 + 4) = 0u;
}

}  // namespace sly4::gfx

NATIVE(00517FA8, sly4::gfx::Nop_00517FA8)
NATIVE(00518E60, sly4::gfx::Nop_00518E60)
NATIVE(00518E64, sly4::gfx::Nop_00518E64)
NATIVE(0051FF18, sly4::gfx::Nop_0051FF18)
NATIVE(00520028, sly4::gfx::Nop_00520028)
NATIVE(0054EC94, sly4::gfx::Array_Destruct)
NATIVE(005B19CC, sly4::gfx::Nop_005B19CC)
NATIVE(005B19D0, sly4::gfx::Nop_005B19D0)
NATIVE(005D7BCC, sly4::gfx::Nop_005D7BCC)
NATIVE(007A172C, sly4::gfx::Nop_007A172C)
NATIVE(007A1854, sly4::gfx::Nop_007A1854)
NATIVE(007A18F0, sly4::gfx::Nop_007A18F0)
NATIVE(007A1AE4, sly4::gfx::Nop_007A1AE4)
NATIVE(007A1B3C, sly4::gfx::Nop_007A1B3C)
NATIVE(007A1B40, sly4::gfx::Nop_007A1B40)
NATIVE(007A1B44, sly4::gfx::Nop_007A1B44)
NATIVE(007A2150, sly4::gfx::Nop_007A2150)
NATIVE(007A22CC, sly4::gfx::Nop_007A22CC)
NATIVE(007A22D0, sly4::gfx::Nop_007A22D0)
NATIVE(007A2498, sly4::gfx::Nop_007A2498)
NATIVE(007A2560, sly4::gfx::Nop_007A2560)
NATIVE(007A26F0, sly4::gfx::Nop_007A26F0)
NATIVE(007A27B8, sly4::gfx::Nop_007A27B8)
NATIVE(007A2880, sly4::gfx::Nop_007A2880)
NATIVE(007A2948, sly4::gfx::Nop_007A2948)
NATIVE(007A2A10, sly4::gfx::Nop_007A2A10)
NATIVE(007A2AD8, sly4::gfx::Nop_007A2AD8)
NATIVE(007A2BA0, sly4::gfx::Nop_007A2BA0)
NATIVE(007A3AD4, sly4::gfx::Nop_007A3AD4)
NATIVE(007A3CFC, sly4::gfx::Nop_007A3CFC)
NATIVE(007A4D4C, sly4::gfx::Nop_007A4D4C)
NATIVE(007A4D50, sly4::gfx::Nop_007A4D50)
NATIVE(007A4D5C, sly4::gfx::Nop_007A4D5C)
NATIVE(007A4FF4, sly4::gfx::Nop_007A4FF4)
NATIVE(007A50B0, sly4::gfx::Nop_007A50B0)
NATIVE(0050129C, sly4::gfx::True_0050129C)
NATIVE(00525A1C, sly4::gfx::True_00525A1C)
NATIVE(0052F778, sly4::gfx::SetWord_0052F778)
NATIVE(00537818, sly4::gfx::False_00537818)
NATIVE(0053B650, sly4::gfx::True_0053B650)
NATIVE(005AFBD0, sly4::gfx::True_005AFBD0)
NATIVE(005B1ACC, sly4::gfx::True_005B1ACC)
NATIVE(005B7878, sly4::gfx::True_005B7878)
NATIVE(005B7EE4, sly4::gfx::True_005B7EE4)
NATIVE(005B8610, sly4::gfx::True_005B8610)
NATIVE(005D705C, sly4::gfx::True_005D705C)
NATIVE(0079ED80, sly4::gfx::Get20_0079ED80)
NATIVE(0079EDDC, sly4::gfx::False_0079EDDC)
NATIVE(0079F2CC, sly4::gfx::False_0079F2CC)
NATIVE(0079F7FC, sly4::gfx::False_0079F7FC)
NATIVE(0079FE14, sly4::gfx::False_0079FE14)
NATIVE(007A08A4, sly4::gfx::False_007A08A4)
NATIVE(007A16BC, sly4::gfx::SetByte4_007A16BC)
NATIVE(007A2328, sly4::gfx::GetF84_007A2328)
NATIVE(007A2330, sly4::gfx::SetF84_007A2330)
NATIVE(007A349C, sly4::gfx::True_007A349C)
NATIVE(007A4D54, sly4::gfx::False_007A4D54)
NATIVE(007A4D60, sly4::gfx::False_007A4D60)
NATIVE(007A4DF0, sly4::gfx::Get288_007A4DF0)
NATIVE(007A527C, sly4::gfx::True_007A527C)
NATIVE(007A5DD4, sly4::gfx::True_007A5DD4)
NATIVE(007A5F7C, sly4::gfx::False_007A5F7C)
NATIVE(00527650, sly4::gfx::SetGlobal_00527650)
NATIVE(0052765C, sly4::gfx::SetGlobalF_0052765C)
NATIVE(0052AA48, sly4::gfx::Copy416To512_0052AA48)
NATIVE(0079EEAC, sly4::gfx::CopyF36_0079EEAC)
NATIVE(0079F39C, sly4::gfx::CopyF36_0079F39C)
NATIVE(007A0E8C, sly4::gfx::Plus16_007A0E8C)
NATIVE(007A2154, sly4::gfx::Plus16_007A2154)
NATIVE(00533778, sly4::gfx::GetGlobal_00533778)
NATIVE(005493F4, sly4::gfx::ToggleByte652_005493F4)
NATIVE(005B1AD4, sly4::gfx::Get_005B1AD4)
NATIVE(005DF950, sly4::gfx::GetToc_005DF950)
NATIVE(0079C884, sly4::gfx::Destruct_0079C884)
NATIVE(0079C894, sly4::gfx::Destruct_0079C894)
NATIVE(0079C8A4, sly4::gfx::Destruct_0079C8A4)
NATIVE(0079C8B4, sly4::gfx::Destruct_0079C8B4)
NATIVE(0079E428, sly4::gfx::Destruct_0079E428)
NATIVE(0079E438, sly4::gfx::Destruct_0079E438)
NATIVE(0079E448, sly4::gfx::Destruct_0079E448)
NATIVE(0079E458, sly4::gfx::Destruct_0079E458)
NATIVE(0079E468, sly4::gfx::Destruct_0079E468)
NATIVE(0079E478, sly4::gfx::Destruct_0079E478)
NATIVE(0079E488, sly4::gfx::Destruct_0079E488)
NATIVE(0079E498, sly4::gfx::Destruct_0079E498)
NATIVE(0079E4A8, sly4::gfx::Destruct_0079E4A8)
NATIVE(0079E4B8, sly4::gfx::Destruct_0079E4B8)
NATIVE(0079E6BC, sly4::gfx::Destruct_0079E6BC)
NATIVE(0079E6CC, sly4::gfx::Destruct_0079E6CC)
NATIVE(0079E7A0, sly4::gfx::Destruct_0079E7A0)
NATIVE(0079E9A4, sly4::gfx::Destruct_0079E9A4)
NATIVE(0079E9E4, sly4::gfx::Destruct_0079E9E4)
NATIVE(0079ED88, sly4::gfx::Destruct_0079ED88)
NATIVE(007A0374, sly4::gfx::Destruct_007A0374)
NATIVE(007A03C8, sly4::gfx::Destruct_007A03C8)
NATIVE(007A041C, sly4::gfx::Destruct_007A041C)
NATIVE(007A0470, sly4::gfx::Destruct_007A0470)
NATIVE(007A04C4, sly4::gfx::Destruct_007A04C4)
NATIVE(007A0518, sly4::gfx::Destruct_007A0518)
NATIVE(007A056C, sly4::gfx::Destruct_007A056C)
NATIVE(007A05C0, sly4::gfx::Destruct_007A05C0)
NATIVE(007A0614, sly4::gfx::Destruct_007A0614)
NATIVE(007A0668, sly4::gfx::Destruct_007A0668)
NATIVE(007A06BC, sly4::gfx::Destruct_007A06BC)
NATIVE(007A0710, sly4::gfx::Destruct_007A0710)
NATIVE(007A0764, sly4::gfx::Destruct_007A0764)
NATIVE(007A08AC, sly4::gfx::Get72_20_007A08AC)
NATIVE(007A08BC, sly4::gfx::Get72_20_007A08BC)
NATIVE(007A0EB4, sly4::gfx::Destruct_007A0EB4)
NATIVE(007A1668, sly4::gfx::Destruct_007A1668)
NATIVE(007A16D8, sly4::gfx::Destruct_007A16D8)
NATIVE(007A189C, sly4::gfx::Destruct_007A189C)
NATIVE(007A1A3C, sly4::gfx::Destruct_007A1A3C)
NATIVE(007A1A90, sly4::gfx::Destruct_007A1A90)
NATIVE(007A1AE8, sly4::gfx::Destruct_007A1AE8)
NATIVE(007A2278, sly4::gfx::Destruct_007A2278)
NATIVE(007A22D4, sly4::gfx::Destruct_007A22D4)
NATIVE(007A342C, sly4::gfx::Destruct_007A342C)
NATIVE(007A343C, sly4::gfx::Destruct_007A343C)
NATIVE(007A3DB8, sly4::gfx::Destruct_007A3DB8)
NATIVE(007A3DC8, sly4::gfx::Destruct_007A3DC8)
NATIVE(007A4B50, sly4::gfx::Destruct_007A4B50)
NATIVE(007A4D98, sly4::gfx::Destruct_007A4D98)
NATIVE(007A5340, sly4::gfx::Destruct_007A5340)
NATIVE(007A5D3C, sly4::gfx::Destruct_007A5D3C)
NATIVE(007A5EBC, sly4::gfx::Destruct_007A5EBC)
NATIVE(007A5ECC, sly4::gfx::Destruct_007A5ECC)
NATIVE(007A5FC8, sly4::gfx::Destruct_007A5FC8)
NATIVE(00500534, sly4::gfx::Min256_00500534)
NATIVE(00500FB4, sly4::gfx::SetDrawCount_00500FB4)
NATIVE(00520094, sly4::gfx::SetF112Flag_00520094)
NATIVE(00532D3C, sly4::gfx::VBlank_Handler)
NATIVE(005382A8, sly4::gfx::Plus12080_005382A8)
NATIVE(0053A9C4, sly4::gfx::Has56_0053A9C4)
NATIVE(007A16C4, sly4::gfx::HasByte4_007A16C4)
NATIVE(007A1774, sly4::gfx::HasByte4_007A1774)
NATIVE(007A6484, sly4::gfx::IsZero40_007A6484)
NATIVE(0050A214, sly4::gfx::SetFlags54_0050A214)
NATIVE(0051299C, sly4::gfx::SetGlobal_0051299C)
NATIVE(005AC634, sly4::gfx::ClearFlipSlots_005AC634)
