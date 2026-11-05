// World objects, part 19: template instances found by the project-wide
// survey (0x00010000-0x005E0000 and 0x00730000-0x007A0000) of functions
// whose normalised disassembly differs only in constants. Each instance is
// one DECOMP line calling the shared body in obj19.h with the constants
// scanned from its own disassembly; every one was checked against the
// original in the PPC simulator. Names carry the class vtable
// (Vt<vtable>_Dtor / _Delete) where one is stored.
#include "obj19.h"

using namespace sly4;
using namespace sly4::obj19;

// ---------------------------------------------------------------- wrappers narrowing a byte argument
// 0x000E02F8(this, (u8)a1)
DECOMP(00228C0C, ThunkU8_00228C0C) { TThunkU8(ctx, 0x000E02F8); }
// 0x00490F88(this, (u8)a1)
DECOMP(003B3118, ThunkU8_003B3118) { TThunkU8(ctx, 0x00490F88); }
// 0x00490F88(this, (u8)a1)
DECOMP(003FD880, ThunkU8_003FD880) { TThunkU8(ctx, 0x00490F88); }
// 0x00490F88(this, (u8)a1)
DECOMP(00404410, ThunkU8_00404410) { TThunkU8(ctx, 0x00490F88); }
// 0x00490F88(this, (u8)a1)
DECOMP(00409EAC, ThunkU8_00409EAC) { TThunkU8(ctx, 0x00490F88); }
// 0x00490F88(this, (u8)a1)
DECOMP(004125F0, ThunkU8_004125F0) { TThunkU8(ctx, 0x00490F88); }
// 0x00490F88(this, (u8)a1)
DECOMP(00418E68, ThunkU8_00418E68) { TThunkU8(ctx, 0x00490F88); }
// 0x00490F88(this, (u8)a1)
DECOMP(0041E668, ThunkU8_0041E668) { TThunkU8(ctx, 0x00490F88); }
// 0x00490F88(this, (u8)a1)
DECOMP(00421440, ThunkU8_00421440) { TThunkU8(ctx, 0x00490F88); }

// ---------------------------------------------------------------- wrappers passing a null-checked second argument
// a1 ? 0x001E9228(a1) : 0
DECOMP(001E9C64, ThunkArg1NZ_001E9C64) { TThunkArg1NZ(ctx, 0x001E9228); }
// a1 ? 0x0036C160(a1) : 0
DECOMP(0036C33C, ThunkArg1NZ_0036C33C) { TThunkArg1NZ(ctx, 0x0036C160); }
// a1 ? 0x0036CEA0(a1) : 0
DECOMP(0036D0EC, ThunkArg1NZ_0036D0EC) { TThunkArg1NZ(ctx, 0x0036CEA0); }
// a1 ? 0x0036FCD0(a1) : 0
DECOMP(00370104, ThunkArg1NZ_00370104) { TThunkArg1NZ(ctx, 0x0036FCD0); }
// a1 ? 0x00370B88(a1) : 0
DECOMP(00370E04, ThunkArg1NZ_00370E04) { TThunkArg1NZ(ctx, 0x00370B88); }
// a1 ? 0x00373C28(a1) : 0
DECOMP(003742AC, ThunkArg1NZ_003742AC) { TThunkArg1NZ(ctx, 0x00373C28); }
// a1 ? 0x00374590(a1) : 0
DECOMP(003746D8, ThunkArg1NZ_003746D8) { TThunkArg1NZ(ctx, 0x00374590); }
// a1 ? 0x00376A20(a1) : 0
DECOMP(00376C20, ThunkArg1NZ_00376C20) { TThunkArg1NZ(ctx, 0x00376A20); }
// a1 ? 0x00379960(a1) : 0
DECOMP(00379B60, ThunkArg1NZ_00379B60) { TThunkArg1NZ(ctx, 0x00379960); }
// a1 ? 0x00379B90(a1) : 0
DECOMP(0037A560, ThunkArg1NZ_0037A560) { TThunkArg1NZ(ctx, 0x00379B90); }
// a1 ? 0x0037DED8(a1) : 0
DECOMP(0037E0D8, ThunkArg1NZ_0037E0D8) { TThunkArg1NZ(ctx, 0x0037DED8); }
// a1 ? 0x0037E108(a1) : 0
DECOMP(0037EC94, ThunkArg1NZ_0037EC94) { TThunkArg1NZ(ctx, 0x0037E108); }
// a1 ? 0x00380CF8(a1) : 0
DECOMP(00380EF8, ThunkArg1NZ_00380EF8) { TThunkArg1NZ(ctx, 0x00380CF8); }
// a1 ? 0x00380F28(a1) : 0
DECOMP(003812F4, ThunkArg1NZ_003812F4) { TThunkArg1NZ(ctx, 0x00380F28); }
// a1 ? 0x00388070(a1) : 0
DECOMP(00389334, ThunkArg1NZ_00389334) { TThunkArg1NZ(ctx, 0x00388070); }
// a1 ? 0x00389368(a1) : 0
DECOMP(00389544, ThunkArg1NZ_00389544) { TThunkArg1NZ(ctx, 0x00389368); }
// a1 ? 0x00389570(a1) : 0
DECOMP(00389C04, ThunkArg1NZ_00389C04) { TThunkArg1NZ(ctx, 0x00389570); }

// ---------------------------------------------------------------- member / base addresses: this + k
// returns this +176
DECOMP(00734A80, Member_00734A80) { TRetAdj(ctx, 176); }
// returns this +144
DECOMP(00734E60, Member_00734E60) { TRetAdj(ctx, 144); }
// returns this +176
DECOMP(00735E14, Member_00735E14) { TRetAdj(ctx, 176); }
// returns this +56
DECOMP(007435C0, Member_007435C0) { TRetAdj(ctx, 56); }
// returns this +288
DECOMP(00748C2C, Member_00748C2C) { TRetAdj(ctx, 288); }
// returns this +240
DECOMP(0074D258, Member_0074D258) { TRetAdj(ctx, 240); }
// returns this +64
DECOMP(0074D8B4, Member_0074D8B4) { TRetAdj(ctx, 64); }
// returns this +432
DECOMP(00757E2C, Member_00757E2C) { TRetAdj(ctx, 432); }
// returns this +448
DECOMP(00758040, Member_00758040) { TRetAdj(ctx, 448); }
// returns this +132
DECOMP(00762BC4, Member_00762BC4) { TRetAdj(ctx, 132); }
// returns this +64
DECOMP(00762E4C, Member_00762E4C) { TRetAdj(ctx, 64); }
// returns this +272
DECOMP(00762E58, Member_00762E58) { TRetAdj(ctx, 272); }
// returns this +24
DECOMP(007630BC, Member_007630BC) { TRetAdj(ctx, 24); }
// returns this +24
DECOMP(00763390, Member_00763390) { TRetAdj(ctx, 24); }
// returns this +32
DECOMP(0076339C, Member_0076339C) { TRetAdj(ctx, 32); }
// returns this +64
DECOMP(00765850, Member_00765850) { TRetAdj(ctx, 64); }

// ---------------------------------------------------------------- destructors entered through a secondary base: three vtables
// p = this -384: vtables 0x00B130E8 / 0x00B13288 / 0x00B132B0 at +0 / +380 / +384, then 0x000EB0F8(p)
DECOMP(000F2CF0, AdjDtor_000F2CF0) { TAdjDtorVt3(ctx, -384, 0x00B130E8, 380, 0x00B13288, 384, 0x00B132B0, 0x000EB0F8); }
// p = this -380: vtables 0x00B130E8 / 0x00B13288 / 0x00B132B0 at +0 / +380 / +384, then 0x000EB0F8(p)
DECOMP(000F2D38, AdjDtor_000F2D38) { TAdjDtorVt3(ctx, -380, 0x00B130E8, 380, 0x00B13288, 384, 0x00B132B0, 0x000EB0F8); }
// p = this -384: vtables 0x00B13308 / 0x00B134A8 / 0x00B134D0 at +0 / +380 / +384, then 0x000EB0F8(p)
DECOMP(000F3FE0, AdjDtor_000F3FE0) { TAdjDtorVt3(ctx, -384, 0x00B13308, 380, 0x00B134A8, 384, 0x00B134D0, 0x000EB0F8); }
// p = this -380: vtables 0x00B13308 / 0x00B134A8 / 0x00B134D0 at +0 / +380 / +384, then 0x000EB0F8(p)
DECOMP(000F4028, AdjDtor_000F4028) { TAdjDtorVt3(ctx, -380, 0x00B13308, 380, 0x00B134A8, 384, 0x00B134D0, 0x000EB0F8); }
// p = this -384: vtables 0x00B12EC8 / 0x00B13068 / 0x00B13090 at +0 / +380 / +384, then 0x000EB0F8(p)
DECOMP(00748444, AdjDtor_00748444) { TAdjDtorVt3(ctx, -384, 0x00B12EC8, 380, 0x00B13068, 384, 0x00B13090, 0x000EB0F8); }
// p = this -380: vtables 0x00B12EC8 / 0x00B13068 / 0x00B13090 at +0 / +380 / +384, then 0x000EB0F8(p)
DECOMP(0074848C, AdjDtor_0074848C) { TAdjDtorVt3(ctx, -380, 0x00B12EC8, 380, 0x00B13068, 384, 0x00B13090, 0x000EB0F8); }
// p = this -384: vtables 0x00B16678 / 0x00B1684C / 0x00B16874 at +0 / +380 / +384, then 0x001AE468(p)
DECOMP(00754398, AdjDtor_00754398) { TAdjDtorVt3(ctx, -384, 0x00B16678, 380, 0x00B1684C, 384, 0x00B16874, 0x001AE468); }
// p = this -380: vtables 0x00B16678 / 0x00B1684C / 0x00B16874 at +0 / +380 / +384, then 0x001AE468(p)
DECOMP(007543E0, AdjDtor_007543E0) { TAdjDtorVt3(ctx, -380, 0x00B16678, 380, 0x00B1684C, 384, 0x00B16874, 0x001AE468); }
// p = this -384: vtables 0x00B168E0 / 0x00B16AB4 / 0x00B16ADC at +0 / +380 / +384, then 0x001AE468(p)
DECOMP(00754718, AdjDtor_00754718) { TAdjDtorVt3(ctx, -384, 0x00B168E0, 380, 0x00B16AB4, 384, 0x00B16ADC, 0x001AE468); }
// p = this -380: vtables 0x00B168E0 / 0x00B16AB4 / 0x00B16ADC at +0 / +380 / +384, then 0x001AE468(p)
DECOMP(00754760, AdjDtor_00754760) { TAdjDtorVt3(ctx, -380, 0x00B168E0, 380, 0x00B16AB4, 384, 0x00B16ADC, 0x001AE468); }
// p = this -384: vtables 0x00B16DB0 / 0x00B16F84 / 0x00B16FAC at +0 / +380 / +384, then 0x001AE468(p)
DECOMP(00754D00, AdjDtor_00754D00) { TAdjDtorVt3(ctx, -384, 0x00B16DB0, 380, 0x00B16F84, 384, 0x00B16FAC, 0x001AE468); }
// p = this -380: vtables 0x00B16DB0 / 0x00B16F84 / 0x00B16FAC at +0 / +380 / +384, then 0x001AE468(p)
DECOMP(00754D48, AdjDtor_00754D48) { TAdjDtorVt3(ctx, -380, 0x00B16DB0, 380, 0x00B16F84, 384, 0x00B16FAC, 0x001AE468); }
// p = this -384: vtables 0x00B17008 / 0x00B171DC / 0x00B17204 at +0 / +380 / +384, then 0x001AE468(p)
DECOMP(00754FA4, AdjDtor_00754FA4) { TAdjDtorVt3(ctx, -384, 0x00B17008, 380, 0x00B171DC, 384, 0x00B17204, 0x001AE468); }
// p = this -380: vtables 0x00B17008 / 0x00B171DC / 0x00B17204 at +0 / +380 / +384, then 0x001AE468(p)
DECOMP(00754FEC, AdjDtor_00754FEC) { TAdjDtorVt3(ctx, -380, 0x00B17008, 380, 0x00B171DC, 384, 0x00B17204, 0x001AE468); }
// p = this -384: vtables 0x00B17728 / 0x00B178FC / 0x00B17924 at +0 / +380 / +384, then 0x001AE468(p)
DECOMP(00755500, AdjDtor_00755500) { TAdjDtorVt3(ctx, -384, 0x00B17728, 380, 0x00B178FC, 384, 0x00B17924, 0x001AE468); }
// p = this -380: vtables 0x00B17728 / 0x00B178FC / 0x00B17924 at +0 / +380 / +384, then 0x001AE468(p)
DECOMP(00755548, AdjDtor_00755548) { TAdjDtorVt3(ctx, -380, 0x00B17728, 380, 0x00B178FC, 384, 0x00B17924, 0x001AE468); }

// ---------------------------------------------------------------- destructors: two PtrVector members, then the base's owned PtrVector*
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(00739C5C, VtB12868_Dtor) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(00739D18, VtB12868_Dtor_00739D18) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(007469B4, VtB12868_Dtor_007469B4) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(00746A70, VtB12868_Dtor_00746A70) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(0074D330, VtB12868_Dtor_0074D330) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(0074D3EC, VtB12868_Dtor_0074D3EC) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(0074DF24, VtB12868_Dtor_0074DF24) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(0074E0DC, VtB12868_Dtor_0074E0DC) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(0074E610, VtB12868_Dtor_0074E610) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(0075005C, VtB12868_Dtor_0075005C) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(00750118, VtB12868_Dtor_00750118) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(007501D4, VtB12868_Dtor_007501D4) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(00750290, VtB12868_Dtor_00750290) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(0075034C, VtB12868_Dtor_0075034C) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(00750988, VtB12868_Dtor_00750988) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(00750A44, VtB12868_Dtor_00750A44) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(00750B00, VtB12868_Dtor_00750B00) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(00750BBC, VtB12868_Dtor_00750BBC) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(00750C78, VtB12868_Dtor_00750C78) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(00750D34, VtB12868_Dtor_00750D34) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(00750DF0, VtB12868_Dtor_00750DF0) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(00755590, VtB12868_Dtor_00755590) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(0075564C, VtB12868_Dtor_0075564C) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(00755A00, VtB12868_Dtor_00755A00) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(00755ABC, VtB12868_Dtor_00755ABC) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(00755B78, VtB12868_Dtor_00755B78) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(00755F3C, VtB12868_Dtor_00755F3C) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }
// *this = vtable 0x00B12868; ~PtrVector(+200, +180); *this = 0x00B12260; owned PtrVector* +16
DECOMP(00757A9C, VtB12868_Dtor_00757A9C) { TDtorVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16); }

// ---------------------------------------------------------------- their deleting versions
// deleting: the vtable 0x00B12868 destructor body, then 0x0054EE6C(this, 0)
DECOMP(00747430, VtB12868_Delete) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x0054EE6C, 0); }
// deleting: the vtable 0x00B12868 destructor body, then 0x0054EE6C(this, 0)
DECOMP(007475DC, VtB12868_Delete_007475DC) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x0054EE6C, 0); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 288)
DECOMP(00748164, VtB12868_Delete_00748164) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 288); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 288)
DECOMP(0074EE9C, VtB12868_Delete_0074EE9C) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 288); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 240)
DECOMP(0074F008, VtB12868_Delete_0074F008) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 240); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 304)
DECOMP(0074F208, VtB12868_Delete_0074F208) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 304); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 336)
DECOMP(0074F358, VtB12868_Delete_0074F358) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 336); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 336)
DECOMP(0074F4EC, VtB12868_Delete_0074F4EC) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 336); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 272)
DECOMP(00751240, VtB12868_Delete_00751240) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 272); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 272)
DECOMP(007519D0, VtB12868_Delete_007519D0) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 272); }
// deleting: the vtable 0x00B12868 destructor body, then 0x0054EE6C(this, 0)
DECOMP(0075247C, VtB12868_Delete_0075247C) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x0054EE6C, 0); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 256)
DECOMP(007525D8, VtB12868_Delete_007525D8) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 256); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 272)
DECOMP(00752BDC, VtB12868_Delete_00752BDC) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 272); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 272)
DECOMP(00753B34, VtB12868_Delete_00753B34) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 272); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 288)
DECOMP(00754138, VtB12868_Delete_00754138) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 288); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 288)
DECOMP(007544B8, VtB12868_Delete_007544B8) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 288); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 288)
DECOMP(007548E8, VtB12868_Delete_007548E8) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 288); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 784)
DECOMP(00754AA0, VtB12868_Delete_00754AA0) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 784); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 320)
DECOMP(007550BC, VtB12868_Delete_007550BC) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 320); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 288)
DECOMP(007552A0, VtB12868_Delete_007552A0) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 288); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 368)
DECOMP(00756080, VtB12868_Delete_00756080) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 368); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 256)
DECOMP(00756684, VtB12868_Delete_00756684) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 256); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 288)
DECOMP(00756C00, VtB12868_Delete_00756C00) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 288); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 320)
DECOMP(007574A8, VtB12868_Delete_007574A8) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 320); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 320)
DECOMP(00757604, VtB12868_Delete_00757604) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 320); }
// deleting: the vtable 0x00B12868 destructor body, then 0x0054EE6C(this, 0)
DECOMP(00758174, VtB12868_Delete_00758174) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x0054EE6C, 0); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 288)
DECOMP(00758AB4, VtB12868_Delete_00758AB4) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 288); }
// deleting: the vtable 0x00B12868 destructor body, then 0x00552420(this, 288)
DECOMP(00758F1C, VtB12868_Delete_00758F1C) { TDeleteVec2Ptr(ctx, 0x00B12868, 200, 180, 0x00B12260, 16, 0x00552420, 288); }

// ---------------------------------------------------------------- entry pushes (132-byte entries, type 7)
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(00776390, PushEntry7_00776390) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(007764BC, PushEntry7_007764BC) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(00776668, PushEntry7_00776668) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(00777070, PushEntry7_00777070) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(0077724C, PushEntry7_0077724C) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(00777458, PushEntry7_00777458) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(00777604, PushEntry7_00777604) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(00777818, PushEntry7_00777818) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(00777CA4, PushEntry7_00777CA4) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(007780A0, PushEntry7_007780A0) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(0077824C, PushEntry7_0077824C) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(007783F8, PushEntry7_007783F8) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(007785A4, PushEntry7_007785A4) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(00778750, PushEntry7_00778750) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(0077898C, PushEntry7_0077898C) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(00778B38, PushEntry7_00778B38) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(00778F18, PushEntry7_00778F18) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(00779124, PushEntry7_00779124) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(00779330, PushEntry7_00779330) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(0077950C, PushEntry7_0077950C) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(007796E8, PushEntry7_007796E8) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(007798F8, PushEntry7_007798F8) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(00779A50, PushEntry7_00779A50) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(00779BFC, PushEntry7_00779BFC) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(00779DDC, PushEntry7_00779DDC) { TPushEntry7(ctx); }
// (u16 v, Owner** h) -> s16 index: a type-7 entry of (*h - 152)
DECOMP(00779F88, PushEntry7_00779F88) { TPushEntry7(ctx); }

// ---------------------------------------------------------------- destructors through a secondary base owning a PtrVector* at +388
// p = this -384: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(001DC22C, AdjDtor_001DC22C) { TAdjDtorVecPtr(ctx, -384, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -380: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(001DC2E8, AdjDtor_001DC2E8) { TAdjDtorVecPtr(ctx, -380, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -384: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(001EA830, AdjDtor_001EA830) { TAdjDtorVecPtr(ctx, -384, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -380: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(001EA8EC, AdjDtor_001EA8EC) { TAdjDtorVecPtr(ctx, -380, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -384: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(001F64BC, AdjDtor_001F64BC) { TAdjDtorVecPtr(ctx, -384, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -380: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(001F6578, AdjDtor_001F6578) { TAdjDtorVecPtr(ctx, -380, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -384: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(00751DD4, AdjDtor_00751DD4) { TAdjDtorVecPtr(ctx, -384, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -380: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(00751E90, AdjDtor_00751E90) { TAdjDtorVecPtr(ctx, -380, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -384: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(00752304, AdjDtor_00752304) { TAdjDtorVecPtr(ctx, -384, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -380: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(007523C0, AdjDtor_007523C0) { TAdjDtorVecPtr(ctx, -380, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -384: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(007529DC, AdjDtor_007529DC) { TAdjDtorVecPtr(ctx, -384, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -380: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(00752A98, AdjDtor_00752A98) { TAdjDtorVecPtr(ctx, -380, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -384: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(00752FE0, AdjDtor_00752FE0) { TAdjDtorVecPtr(ctx, -384, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -380: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(0075309C, AdjDtor_0075309C) { TAdjDtorVecPtr(ctx, -380, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -384: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(00753F38, AdjDtor_00753F38) { TAdjDtorVecPtr(ctx, -384, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -380: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(00753FF4, AdjDtor_00753FF4) { TAdjDtorVecPtr(ctx, -380, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -384: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(00756484, AdjDtor_00756484) { TAdjDtorVecPtr(ctx, -384, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -380: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(00756540, AdjDtor_00756540) { TAdjDtorVecPtr(ctx, -380, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -384: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(00756A88, AdjDtor_00756A88) { TAdjDtorVecPtr(ctx, -384, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -380: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(00756B44, AdjDtor_00756B44) { TAdjDtorVecPtr(ctx, -380, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }

// ---------------------------------------------------------------- destructors: one PtrVector member, then the base destructor
// *this = vtable 0x00B10AF8; ~PtrVector(+64); 0x004BB41C(this)
DECOMP(000941FC, VtB10AF8_Dtor) { TDtorVec(ctx, 0x00B10AF8, 64, 0x004BB41C); }
// *this = vtable 0x00B0FF68; ~PtrVector(+12); 0x004BB41C(this)
DECOMP(0073DB28, VtB0FF68_Dtor) { TDtorVec(ctx, 0x00B0FF68, 12, 0x004BB41C); }
// *this = vtable 0x00B0FFB8; ~PtrVector(+12); 0x004BB41C(this)
DECOMP(0073DC98, VtB0FFB8_Dtor) { TDtorVec(ctx, 0x00B0FFB8, 12, 0x004BB41C); }
// *this = vtable 0x00B11698; ~PtrVector(+12); 0x004BB41C(this)
DECOMP(0073FFF8, VtB11698_Dtor) { TDtorVec(ctx, 0x00B11698, 12, 0x004BB41C); }
// *this = vtable 0x00B11508; ~PtrVector(+12); 0x004BB41C(this)
DECOMP(00741518, VtB11508_Dtor) { TDtorVec(ctx, 0x00B11508, 12, 0x004BB41C); }
// *this = vtable 0x00B11558; ~PtrVector(+144); 0x004BB41C(this)
DECOMP(00741688, VtB11558_Dtor) { TDtorVec(ctx, 0x00B11558, 144, 0x004BB41C); }
// *this = vtable 0x00B11AD0; ~PtrVector(+12); 0x004BB41C(this)
DECOMP(007428E0, VtB11AD0_Dtor) { TDtorVec(ctx, 0x00B11AD0, 12, 0x004BB41C); }
// *this = vtable 0x00B14FD8; ~PtrVector(+140); 0x000C2DA0(this)
DECOMP(00750EAC, VtB14FD8_Dtor) { TDtorVec(ctx, 0x00B14FD8, 140, 0x000C2DA0); }
// *this = vtable 0x00B15028; ~PtrVector(+344); 0x000D729C(this)
DECOMP(00751048, VtB15028_Dtor) { TDtorVec(ctx, 0x00B15028, 344, 0x000D729C); }
// *this = vtable 0x00B20428; ~PtrVector(+56); 0x00490B34(this)
DECOMP(0077AF08, VtB20428_Dtor) { TDtorVec(ctx, 0x00B20428, 56, 0x00490B34); }
// *this = vtable 0x00B20488; ~PtrVector(+104); 0x00490B34(this)
DECOMP(0077B194, VtB20488_Dtor) { TDtorVec(ctx, 0x00B20488, 104, 0x00490B34); }
// *this = vtable 0x00B206A8; ~PtrVector(+2340); 0x00490B34(this)
DECOMP(0077CF7C, VtB206A8_Dtor) { TDtorVec(ctx, 0x00B206A8, 2340, 0x00490B34); }
// *this = vtable 0x00B20C30; ~PtrVector(+72); 0x00490B34(this)
DECOMP(0077D75C, VtB20C30_Dtor) { TDtorVec(ctx, 0x00B20C30, 72, 0x00490B34); }
// *this = vtable 0x00B21070; ~PtrVector(+12); 0x0042CA50(this)
DECOMP(007808F8, VtB21070_Dtor) { TDtorVec(ctx, 0x00B21070, 12, 0x0042CA50); }
// *this = vtable 0x00B210A0; ~PtrVector(+140); 0x0042CA50(this)
DECOMP(007809E0, VtB210A0_Dtor) { TDtorVec(ctx, 0x00B210A0, 140, 0x0042CA50); }
// *this = vtable 0x00B21400; ~PtrVector(+12); 0x0042CA50(this)
DECOMP(00781940, VtB21400_Dtor) { TDtorVec(ctx, 0x00B21400, 12, 0x0042CA50); }

// ---------------------------------------------------------------- their deleting versions
// deleting: vtable 0x00B10AF8; ~PtrVector(+64); 0x004BB41C(this); 0x0054EE6C(this, 0)
DECOMP(00094264, VtB10AF8_Delete) { TDeleteVec(ctx, 0x00B10AF8, 64, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FF68; ~PtrVector(+12); 0x004BB41C(this); 0x0054EE6C(this, 0)
DECOMP(0073DB90, VtB0FF68_Delete) { TDeleteVec(ctx, 0x00B0FF68, 12, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FFB8; ~PtrVector(+12); 0x004BB41C(this); 0x0054EE6C(this, 0)
DECOMP(0073DD00, VtB0FFB8_Delete) { TDeleteVec(ctx, 0x00B0FFB8, 12, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11508; ~PtrVector(+12); 0x004BB41C(this); 0x0054EE6C(this, 0)
DECOMP(00741580, VtB11508_Delete) { TDeleteVec(ctx, 0x00B11508, 12, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11558; ~PtrVector(+144); 0x004BB41C(this); 0x0054EE6C(this, 0)
DECOMP(007416F0, VtB11558_Delete) { TDeleteVec(ctx, 0x00B11558, 144, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11698; ~PtrVector(+12); 0x004BB41C(this); 0x0054EE6C(this, 0)
DECOMP(00741AC4, VtB11698_Delete) { TDeleteVec(ctx, 0x00B11698, 12, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11AD0; ~PtrVector(+12); 0x004BB41C(this); 0x0054EE6C(this, 0)
DECOMP(00742948, VtB11AD0_Delete) { TDeleteVec(ctx, 0x00B11AD0, 12, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B14FD8; ~PtrVector(+140); 0x000C2DA0(this); 0x00552420(this, 224)
DECOMP(00750F14, VtB14FD8_Delete) { TDeleteVec(ctx, 0x00B14FD8, 140, 0x000C2DA0, 0x00552420, 224); }
// deleting: vtable 0x00B15028; ~PtrVector(+344); 0x000D729C(this); 0x00552420(this, 384)
DECOMP(007510B0, VtB15028_Delete) { TDeleteVec(ctx, 0x00B15028, 344, 0x000D729C, 0x00552420, 384); }
// deleting: vtable 0x00B20428; ~PtrVector(+56); 0x00490B34(this); 0x00552420(this, 176)
DECOMP(0077E8AC, VtB20428_Delete) { TDeleteVec(ctx, 0x00B20428, 56, 0x00490B34, 0x00552420, 176); }
// deleting: vtable 0x00B20488; ~PtrVector(+104); 0x00490B34(this); 0x00552420(this, 1632)
DECOMP(0077E92C, VtB20488_Delete) { TDeleteVec(ctx, 0x00B20488, 104, 0x00490B34, 0x00552420, 1632); }
// deleting: vtable 0x00B206A8; ~PtrVector(+2340); 0x00490B34(this); 0x00552420(this, 2432)
DECOMP(0077ED4C, VtB206A8_Delete) { TDeleteVec(ctx, 0x00B206A8, 2340, 0x00490B34, 0x00552420, 2432); }
// deleting: vtable 0x00B20C30; ~PtrVector(+72); 0x00490B34(this); 0x00552420(this, 120)
DECOMP(0077F4E8, VtB20C30_Delete) { TDeleteVec(ctx, 0x00B20C30, 72, 0x00490B34, 0x00552420, 120); }
// deleting: vtable 0x00B21070; ~PtrVector(+12); 0x0042CA50(this); 0x00552420(this, 52)
DECOMP(00780960, VtB21070_Delete) { TDeleteVec(ctx, 0x00B21070, 12, 0x0042CA50, 0x00552420, 52); }
// deleting: vtable 0x00B210A0; ~PtrVector(+140); 0x0042CA50(this); 0x00552420(this, 172)
DECOMP(00780A48, VtB210A0_Delete) { TDeleteVec(ctx, 0x00B210A0, 140, 0x0042CA50, 0x00552420, 172); }
// deleting: vtable 0x00B21400; ~PtrVector(+12); 0x0042CA50(this); 0x00552420(this, 32)
DECOMP(007819A8, VtB21400_Delete) { TDeleteVec(ctx, 0x00B21400, 12, 0x0042CA50, 0x00552420, 32); }

// ---------------------------------------------------------------- factories: Mem_Alloc, the base constructor, then the vtable
// p = Mem_Alloc(92, 0, 16); if p: 0x004F6ED8(p), *p = vtable 0x00B21470
DECOMP(007821D0, VtB21470_New) { TNew0Vt(ctx, 92, 0x004F6ED8, 0x00B21470); }
// p = Mem_Alloc(92, 0, 16); if p: 0x004F6ED8(p), *p = vtable 0x00B214B8
DECOMP(00782308, VtB214B8_New) { TNew0Vt(ctx, 92, 0x004F6ED8, 0x00B214B8); }
// p = Mem_Alloc(92, 0, 16); if p: 0x004F6ED8(p), *p = vtable 0x00B215F0
DECOMP(00782760, VtB215F0_New) { TNew0Vt(ctx, 92, 0x004F6ED8, 0x00B215F0); }
// p = Mem_Alloc(92, 0, 16); if p: 0x004F6ED8(p), *p = vtable 0x00B21638
DECOMP(00782898, VtB21638_New) { TNew0Vt(ctx, 92, 0x004F6ED8, 0x00B21638); }
// p = Mem_Alloc(92, 0, 16); if p: 0x004F6ED8(p), *p = vtable 0x00B21680
DECOMP(007829E0, VtB21680_New) { TNew0Vt(ctx, 92, 0x004F6ED8, 0x00B21680); }
// p = Mem_Alloc(92, 0, 16); if p: 0x004F6ED8(p), *p = vtable 0x00B216C8
DECOMP(00782B18, VtB216C8_New) { TNew0Vt(ctx, 92, 0x004F6ED8, 0x00B216C8); }
// p = Mem_Alloc(92, 0, 16); if p: 0x004F6ED8(p), *p = vtable 0x00B21710
DECOMP(00782C50, VtB21710_New) { TNew0Vt(ctx, 92, 0x004F6ED8, 0x00B21710); }
// p = Mem_Alloc(92, 0, 16); if p: 0x004F6ED8(p), *p = vtable 0x00B21758
DECOMP(00782D88, VtB21758_New) { TNew0Vt(ctx, 92, 0x004F6ED8, 0x00B21758); }
// p = Mem_Alloc(92, 0, 16); if p: 0x004F6ED8(p), *p = vtable 0x00B217A0
DECOMP(00782EC0, VtB217A0_New) { TNew0Vt(ctx, 92, 0x004F6ED8, 0x00B217A0); }
// p = Mem_Alloc(92, 0, 16); if p: 0x004F6ED8(p), *p = vtable 0x00B217F8
DECOMP(00782FF8, VtB217F8_New) { TNew0Vt(ctx, 92, 0x004F6ED8, 0x00B217F8); }
// p = Mem_Alloc(92, 0, 16); if p: 0x004F6ED8(p), *p = vtable 0x00B21840
DECOMP(00783130, VtB21840_New) { TNew0Vt(ctx, 92, 0x004F6ED8, 0x00B21840); }
// p = Mem_Alloc(92, 0, 16); if p: 0x004F6ED8(p), *p = vtable 0x00B21888
DECOMP(00783268, VtB21888_New) { TNew0Vt(ctx, 92, 0x004F6ED8, 0x00B21888); }
// p = Mem_Alloc(92, 0, 16); if p: 0x004F6ED8(p), *p = vtable 0x00B0DFD0
DECOMP(00783384, VtB0DFD0_New) { TNew0Vt(ctx, 92, 0x004F6ED8, 0x00B0DFD0); }
// p = Mem_Alloc(92, 0, 16); if p: 0x004F6ED8(p), *p = vtable 0x00B0E560
DECOMP(007833F0, VtB0E560_New) { TNew0Vt(ctx, 92, 0x004F6ED8, 0x00B0E560); }
// p = Mem_Alloc(92, 0, 16); if p: 0x004F6ED8(p), *p = vtable 0x00B0E160
DECOMP(0078345C, VtB0E160_New) { TNew0Vt(ctx, 92, 0x004F6ED8, 0x00B0E160); }
// p = Mem_Alloc(92, 0, 16); if p: 0x004F6ED8(p), *p = vtable 0x00B0E6F0
DECOMP(007834C8, VtB0E6F0_New) { TNew0Vt(ctx, 92, 0x004F6ED8, 0x00B0E6F0); }

// ---------------------------------------------------------------- deleting destructors through a secondary base: three vtables
// deleting via this -384: vtables 0x00B130E8 / 0x00B13288 / 0x00B132B0, 0x000EB0F8(p), 0x00552420(p, 752)
DECOMP(000F2C10, AdjDelete_000F2C10) { TAdjDeleteVt3(ctx, -384, 0x00B130E8, 380, 0x00B13288, 384, 0x00B132B0, 0x000EB0F8, 0x00552420, 752); }
// deleting via this -380: vtables 0x00B130E8 / 0x00B13288 / 0x00B132B0, 0x000EB0F8(p), 0x00552420(p, 752)
DECOMP(000F2C80, AdjDelete_000F2C80) { TAdjDeleteVt3(ctx, -380, 0x00B130E8, 380, 0x00B13288, 384, 0x00B132B0, 0x000EB0F8, 0x00552420, 752); }
// deleting via this -384: vtables 0x00B13308 / 0x00B134A8 / 0x00B134D0, 0x000EB0F8(p), 0x00552420(p, 688)
DECOMP(000F3F00, AdjDelete_000F3F00) { TAdjDeleteVt3(ctx, -384, 0x00B13308, 380, 0x00B134A8, 384, 0x00B134D0, 0x000EB0F8, 0x00552420, 688); }
// deleting via this -380: vtables 0x00B13308 / 0x00B134A8 / 0x00B134D0, 0x000EB0F8(p), 0x00552420(p, 688)
DECOMP(000F3F70, AdjDelete_000F3F70) { TAdjDeleteVt3(ctx, -380, 0x00B13308, 380, 0x00B134A8, 384, 0x00B134D0, 0x000EB0F8, 0x00552420, 688); }
// deleting via this -384: vtables 0x00B12EC8 / 0x00B13068 / 0x00B13090, 0x000EB0F8(p), 0x00552420(p, 640)
DECOMP(00748364, AdjDelete_00748364) { TAdjDeleteVt3(ctx, -384, 0x00B12EC8, 380, 0x00B13068, 384, 0x00B13090, 0x000EB0F8, 0x00552420, 640); }
// deleting via this -380: vtables 0x00B12EC8 / 0x00B13068 / 0x00B13090, 0x000EB0F8(p), 0x00552420(p, 640)
DECOMP(007483D4, AdjDelete_007483D4) { TAdjDeleteVt3(ctx, -380, 0x00B12EC8, 380, 0x00B13068, 384, 0x00B13090, 0x000EB0F8, 0x00552420, 640); }
// deleting via this -384: vtables 0x00B16678 / 0x00B1684C / 0x00B16874, 0x001AE468(p), 0x00552420(p, 576)
DECOMP(007542B8, AdjDelete_007542B8) { TAdjDeleteVt3(ctx, -384, 0x00B16678, 380, 0x00B1684C, 384, 0x00B16874, 0x001AE468, 0x00552420, 576); }
// deleting via this -380: vtables 0x00B16678 / 0x00B1684C / 0x00B16874, 0x001AE468(p), 0x00552420(p, 576)
DECOMP(00754328, AdjDelete_00754328) { TAdjDeleteVt3(ctx, -380, 0x00B16678, 380, 0x00B1684C, 384, 0x00B16874, 0x001AE468, 0x00552420, 576); }
// deleting via this -384: vtables 0x00B168E0 / 0x00B16AB4 / 0x00B16ADC, 0x001AE468(p), 0x00552420(p, 592)
DECOMP(00754638, AdjDelete_00754638) { TAdjDeleteVt3(ctx, -384, 0x00B168E0, 380, 0x00B16AB4, 384, 0x00B16ADC, 0x001AE468, 0x00552420, 592); }
// deleting via this -380: vtables 0x00B168E0 / 0x00B16AB4 / 0x00B16ADC, 0x001AE468(p), 0x00552420(p, 592)
DECOMP(007546A8, AdjDelete_007546A8) { TAdjDeleteVt3(ctx, -380, 0x00B168E0, 380, 0x00B16AB4, 384, 0x00B16ADC, 0x001AE468, 0x00552420, 592); }
// deleting via this -384: vtables 0x00B16DB0 / 0x00B16F84 / 0x00B16FAC, 0x001AE468(p), 0x00552420(p, 704)
DECOMP(00754C20, AdjDelete_00754C20) { TAdjDeleteVt3(ctx, -384, 0x00B16DB0, 380, 0x00B16F84, 384, 0x00B16FAC, 0x001AE468, 0x00552420, 704); }
// deleting via this -380: vtables 0x00B16DB0 / 0x00B16F84 / 0x00B16FAC, 0x001AE468(p), 0x00552420(p, 704)
DECOMP(00754C90, AdjDelete_00754C90) { TAdjDeleteVt3(ctx, -380, 0x00B16DB0, 380, 0x00B16F84, 384, 0x00B16FAC, 0x001AE468, 0x00552420, 704); }
// deleting via this -384: vtables 0x00B17008 / 0x00B171DC / 0x00B17204, 0x001AE468(p), 0x00552420(p, 656)
DECOMP(00754EC4, AdjDelete_00754EC4) { TAdjDeleteVt3(ctx, -384, 0x00B17008, 380, 0x00B171DC, 384, 0x00B17204, 0x001AE468, 0x00552420, 656); }
// deleting via this -380: vtables 0x00B17008 / 0x00B171DC / 0x00B17204, 0x001AE468(p), 0x00552420(p, 656)
DECOMP(00754F34, AdjDelete_00754F34) { TAdjDeleteVt3(ctx, -380, 0x00B17008, 380, 0x00B171DC, 384, 0x00B17204, 0x001AE468, 0x00552420, 656); }
// deleting via this -384: vtables 0x00B17728 / 0x00B178FC / 0x00B17924, 0x001AE468(p), 0x00552420(p, 576)
DECOMP(00755420, AdjDelete_00755420) { TAdjDeleteVt3(ctx, -384, 0x00B17728, 380, 0x00B178FC, 384, 0x00B17924, 0x001AE468, 0x00552420, 576); }
// deleting via this -380: vtables 0x00B17728 / 0x00B178FC / 0x00B17924, 0x001AE468(p), 0x00552420(p, 576)
DECOMP(00755490, AdjDelete_00755490) { TAdjDeleteVt3(ctx, -380, 0x00B17728, 380, 0x00B178FC, 384, 0x00B17924, 0x001AE468, 0x00552420, 576); }

// ---------------------------------------------------------------- wrappers: target((s16)a0, a1, &local)
// 0x007737A4((s16)a0, a1, &local16)
DECOMP(00775BC4, CallS16Local_00775BC4) { TCallS16Local(ctx, 0x007737A4); }
// 0x00773898((s16)a0, a1, &local16)
DECOMP(00776068, CallS16Local_00776068) { TCallS16Local(ctx, 0x00773898); }
// 0x00773998((s16)a0, a1, &local16)
DECOMP(007762F4, CallS16Local_007762F4) { TCallS16Local(ctx, 0x00773998); }
// 0x007742DC((s16)a0, a1, &local16)
DECOMP(00776814, CallS16Local_00776814) { TCallS16Local(ctx, 0x007742DC); }
// 0x007743CC((s16)a0, a1, &local16)
DECOMP(00776968, CallS16Local_00776968) { TCallS16Local(ctx, 0x007743CC); }
// 0x007744BC((s16)a0, a1, &local16)
DECOMP(00776ABC, CallS16Local_00776ABC) { TCallS16Local(ctx, 0x007744BC); }
// 0x007745AC((s16)a0, a1, &local16)
DECOMP(00776C10, CallS16Local_00776C10) { TCallS16Local(ctx, 0x007745AC); }
// 0x0077469C((s16)a0, a1, &local16)
DECOMP(00776D64, CallS16Local_00776D64) { TCallS16Local(ctx, 0x0077469C); }
// 0x0077478C((s16)a0, a1, &local16)
DECOMP(00776E88, CallS16Local_00776E88) { TCallS16Local(ctx, 0x0077478C); }
// 0x0077487C((s16)a0, a1, &local16)
DECOMP(00777B80, CallS16Local_00777B80) { TCallS16Local(ctx, 0x0077487C); }
// 0x0077496C((s16)a0, a1, &local16)
DECOMP(00777F7C, CallS16Local_00777F7C) { TCallS16Local(ctx, 0x0077496C); }
// 0x00774A5C((s16)a0, a1, &local16)
DECOMP(0077A2B4, CallS16Local_0077A2B4) { TCallS16Local(ctx, 0x00774A5C); }
// 0x00774B4C((s16)a0, a1, &local16)
DECOMP(0077A3D8, CallS16Local_0077A3D8) { TCallS16Local(ctx, 0x00774B4C); }
// 0x00774C3C((s16)a0, a1, &local16)
DECOMP(0077A82C, CallS16Local_0077A82C) { TCallS16Local(ctx, 0x00774C3C); }
// 0x00774D2C((s16)a0, a1, &local16)
DECOMP(0077A950, CallS16Local_0077A950) { TCallS16Local(ctx, 0x00774D2C); }

// ---------------------------------------------------------------- destructors removing the object from a global table
// *this = vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this)
DECOMP(00250384, VtB1BE30_Dtor) { TDtorUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940); }
// *this = vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this)
DECOMP(0027CBB8, VtB1BE30_Dtor_0027CBB8) { TDtorUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940); }
// *this = vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this)
DECOMP(00283644, VtB1BE30_Dtor_00283644) { TDtorUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940); }
// *this = vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this)
DECOMP(00287BB8, VtB1BE30_Dtor_00287BB8) { TDtorUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940); }
// *this = vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this)
DECOMP(00292CD8, VtB1BE30_Dtor_00292CD8) { TDtorUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940); }
// *this = vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this)
DECOMP(0029FC30, VtB1BE30_Dtor_0029FC30) { TDtorUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940); }
// *this = vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this)
DECOMP(002B0890, VtB1BE30_Dtor_002B0890) { TDtorUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940); }
// *this = vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this)
DECOMP(002B1F5C, VtB1BE30_Dtor_002B1F5C) { TDtorUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940); }
// *this = vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this)
DECOMP(002B9FF8, VtB1BE30_Dtor_002B9FF8) { TDtorUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940); }
// *this = vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this)
DECOMP(00763174, VtB1BE30_Dtor_00763174) { TDtorUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940); }
// *this = vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this)
DECOMP(007633D4, VtB1BE30_Dtor_007633D4) { TDtorUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940); }
// *this = vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this)
DECOMP(007635F0, VtB1BE30_Dtor_007635F0) { TDtorUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940); }
// *this = vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this)
DECOMP(00763850, VtB1BE30_Dtor_00763850) { TDtorUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940); }
// *this = vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this)
DECOMP(00763AB0, VtB1BE30_Dtor_00763AB0) { TDtorUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940); }
// *this = vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this)
DECOMP(00763CCC, VtB1BE30_Dtor_00763CCC) { TDtorUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940); }

// ---------------------------------------------------------------- their deleting versions
// deleting: vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this); 0x0054EE6C(this, 0)
DECOMP(00250480, VtB1BE30_Delete) { TDeleteUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this); 0x00552420(this, 96)
DECOMP(0027CCB4, VtB1BE30_Delete_0027CCB4) { TDeleteUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940, 0x00552420, 96); }
// deleting: vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this); 0x0054EE6C(this, 0)
DECOMP(00283740, VtB1BE30_Delete_00283740) { TDeleteUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this); 0x00552420(this, 40)
DECOMP(00287CB4, VtB1BE30_Delete_00287CB4) { TDeleteUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940, 0x00552420, 40); }
// deleting: vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this); 0x00552420(this, 104)
DECOMP(00292DD4, VtB1BE30_Delete_00292DD4) { TDeleteUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940, 0x00552420, 104); }
// deleting: vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this); 0x00552420(this, 28)
DECOMP(0029FD2C, VtB1BE30_Delete_0029FD2C) { TDeleteUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940, 0x00552420, 28); }
// deleting: vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this); 0x00552420(this, 32)
DECOMP(002B098C, VtB1BE30_Delete_002B098C) { TDeleteUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940, 0x00552420, 32); }
// deleting: vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this); 0x00552420(this, 64)
DECOMP(002B2058, VtB1BE30_Delete_002B2058) { TDeleteUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940, 0x00552420, 64); }
// deleting: vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this); 0x00552420(this, 192)
DECOMP(002BA0F4, VtB1BE30_Delete_002BA0F4) { TDeleteUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940, 0x00552420, 192); }
// deleting: vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this); 0x0054EE6C(this, 0)
DECOMP(00763270, VtB1BE30_Delete_00763270) { TDeleteUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this); 0x00552420(this, 96)
DECOMP(007634D0, VtB1BE30_Delete_007634D0) { TDeleteUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940, 0x00552420, 96); }
// deleting: vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this); 0x00552420(this, 80)
DECOMP(007636EC, VtB1BE30_Delete_007636EC) { TDeleteUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940, 0x00552420, 80); }
// deleting: vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this); 0x00552420(this, 96)
DECOMP(0076394C, VtB1BE30_Delete_0076394C) { TDeleteUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940, 0x00552420, 96); }
// deleting: vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this); 0x00552420(this, 96)
DECOMP(00763BAC, VtB1BE30_Delete_00763BAC) { TDeleteUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940, 0x00552420, 96); }
// deleting: vtable 0x00B1BE30; removed from table 0x01056F1C; 0x000D6940(this); 0x0054EE6C(this, 0)
DECOMP(00763DC8, VtB1BE30_Delete_00763DC8) { TDeleteUnregister(ctx, 0x00B1BE30, 0x01056F1C, 0x000D6940, 0x0054EE6C, 0); }

// ---------------------------------------------------------------- byte setters: *(u8*)this = k
// *(u8*)this = 0
DECOMP(007703C0, SetByte_007703C0) { TStoreU8K(ctx, 0); }
// *(u8*)this = 1
DECOMP(00770E78, SetByte_00770E78) { TStoreU8K(ctx, 1); }
// *(u8*)this = 1
DECOMP(00770E84, SetByte_00770E84) { TStoreU8K(ctx, 1); }
// *(u8*)this = 0
DECOMP(00770E90, SetByte_00770E90) { TStoreU8K(ctx, 0); }
// *(u8*)this = 0
DECOMP(00770E9C, SetByte_00770E9C) { TStoreU8K(ctx, 0); }
// *(u8*)this = 0
DECOMP(00770EA8, SetByte_00770EA8) { TStoreU8K(ctx, 0); }
// *(u8*)this = 0
DECOMP(00770EB4, SetByte_00770EB4) { TStoreU8K(ctx, 0); }
// *(u8*)this = 0
DECOMP(00770EC0, SetByte_00770EC0) { TStoreU8K(ctx, 0); }
// *(u8*)this = 0
DECOMP(00770ECC, SetByte_00770ECC) { TStoreU8K(ctx, 0); }
// *(u8*)this = 0
DECOMP(00770ED8, SetByte_00770ED8) { TStoreU8K(ctx, 0); }
// *(u8*)this = 0
DECOMP(00770EE4, SetByte_00770EE4) { TStoreU8K(ctx, 0); }
// *(u8*)this = 0
DECOMP(00770EF0, SetByte_00770EF0) { TStoreU8K(ctx, 0); }
// *(u8*)this = 0
DECOMP(00770EFC, SetByte_00770EFC) { TStoreU8K(ctx, 0); }
// *(u8*)this = 0
DECOMP(00771BE4, SetByte_00771BE4) { TStoreU8K(ctx, 0); }

// ---------------------------------------------------------------- factories: SmallAlloc, the base constructor (p, a1, a0), the vtable
// p = SmallAlloc(36); if p: 0x004F77F8(p, a1, a0), *p = vtable 0x00B214A0
DECOMP(0043B778, VtB214A0_New) { TNewSmall2Vt(ctx, 36, 0x004F77F8, 0x00B214A0); }
// p = SmallAlloc(36); if p: 0x004F77F8(p, a1, a0), *p = vtable 0x00B214FC
DECOMP(0043BBD0, VtB214FC_New) { TNewSmall2Vt(ctx, 36, 0x004F77F8, 0x00B214FC); }
// p = SmallAlloc(36); if p: 0x004F77F8(p, a1, a0), *p = vtable 0x00B21590
DECOMP(0043C128, VtB21590_New) { TNewSmall2Vt(ctx, 36, 0x004F77F8, 0x00B21590); }
// p = SmallAlloc(36); if p: 0x004F77F8(p, a1, a0), *p = vtable 0x00B215D8
DECOMP(0043C35C, VtB215D8_New) { TNewSmall2Vt(ctx, 36, 0x004F77F8, 0x00B215D8); }
// p = SmallAlloc(36); if p: 0x004F77F8(p, a1, a0), *p = vtable 0x00B21620
DECOMP(0043C46C, VtB21620_New) { TNewSmall2Vt(ctx, 36, 0x004F77F8, 0x00B21620); }
// p = SmallAlloc(40); if p: 0x004F77F8(p, a1, a0), *p = vtable 0x00B21668
DECOMP(0043C554, VtB21668_New) { TNewSmall2Vt(ctx, 40, 0x004F77F8, 0x00B21668); }
// p = SmallAlloc(36); if p: 0x004F77F8(p, a1, a0), *p = vtable 0x00B216B0
DECOMP(0043C664, VtB216B0_New) { TNewSmall2Vt(ctx, 36, 0x004F77F8, 0x00B216B0); }
// p = SmallAlloc(36); if p: 0x004F77F8(p, a1, a0), *p = vtable 0x00B216F8
DECOMP(0043C9C0, VtB216F8_New) { TNewSmall2Vt(ctx, 36, 0x004F77F8, 0x00B216F8); }
// p = SmallAlloc(36); if p: 0x004F77F8(p, a1, a0), *p = vtable 0x00B21740
DECOMP(0043CEC0, VtB21740_New) { TNewSmall2Vt(ctx, 36, 0x004F77F8, 0x00B21740); }
// p = SmallAlloc(36); if p: 0x004F77F8(p, a1, a0), *p = vtable 0x00B21788
DECOMP(0043D480, VtB21788_New) { TNewSmall2Vt(ctx, 36, 0x004F77F8, 0x00B21788); }
// p = SmallAlloc(36); if p: 0x004F77F8(p, a1, a0), *p = vtable 0x00B217D0
DECOMP(0043D91C, VtB217D0_New) { TNewSmall2Vt(ctx, 36, 0x004F77F8, 0x00B217D0); }
// p = SmallAlloc(36); if p: 0x004F77F8(p, a1, a0), *p = vtable 0x00B21828
DECOMP(0043DE30, VtB21828_New) { TNewSmall2Vt(ctx, 36, 0x004F77F8, 0x00B21828); }
// p = SmallAlloc(36); if p: 0x004F77F8(p, a1, a0), *p = vtable 0x00B21870
DECOMP(0043DFB8, VtB21870_New) { TNewSmall2Vt(ctx, 36, 0x004F77F8, 0x00B21870); }
// p = SmallAlloc(36); if p: 0x004F77F8(p, a1, a0), *p = vtable 0x00B218B8
DECOMP(0043E48C, VtB218B8_New) { TNewSmall2Vt(ctx, 36, 0x004F77F8, 0x00B218B8); }

// ---------------------------------------------------------------- deleting destructors through a secondary base owning a PtrVector* at +388
// deleting via this -384: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p); 0x00552420(p, 480)
DECOMP(00751C2C, AdjDelete_00751C2C) { TAdjDeleteVecPtr(ctx, -384, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 480); }
// deleting via this -380: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p); 0x00552420(p, 480)
DECOMP(00751D00, AdjDelete_00751D00) { TAdjDeleteVecPtr(ctx, -380, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 480); }
// deleting via this -384: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p); 0x00552420(p, 448)
DECOMP(0075215C, AdjDelete_0075215C) { TAdjDeleteVecPtr(ctx, -384, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 448); }
// deleting via this -380: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p); 0x00552420(p, 448)
DECOMP(00752230, AdjDelete_00752230) { TAdjDeleteVecPtr(ctx, -380, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 448); }
// deleting via this -384: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p); 0x00552420(p, 480)
DECOMP(00752834, AdjDelete_00752834) { TAdjDeleteVecPtr(ctx, -384, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 480); }
// deleting via this -380: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p); 0x00552420(p, 480)
DECOMP(00752908, AdjDelete_00752908) { TAdjDeleteVecPtr(ctx, -380, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 480); }
// deleting via this -384: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p); 0x00552420(p, 576)
DECOMP(00752E38, AdjDelete_00752E38) { TAdjDeleteVecPtr(ctx, -384, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 576); }
// deleting via this -380: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p); 0x00552420(p, 576)
DECOMP(00752F0C, AdjDelete_00752F0C) { TAdjDeleteVecPtr(ctx, -380, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 576); }
// deleting via this -384: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p); 0x00552420(p, 464)
DECOMP(00753D90, AdjDelete_00753D90) { TAdjDeleteVecPtr(ctx, -384, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 464); }
// deleting via this -380: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p); 0x00552420(p, 464)
DECOMP(00753E64, AdjDelete_00753E64) { TAdjDeleteVecPtr(ctx, -380, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 464); }
// deleting via this -384: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p); 0x00552420(p, 624)
DECOMP(007562DC, AdjDelete_007562DC) { TAdjDeleteVecPtr(ctx, -384, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 624); }
// deleting via this -380: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p); 0x00552420(p, 624)
DECOMP(007563B0, AdjDelete_007563B0) { TAdjDeleteVecPtr(ctx, -380, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 624); }
// deleting via this -384: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p); 0x00552420(p, 752)
DECOMP(007568E0, AdjDelete_007568E0) { TAdjDeleteVecPtr(ctx, -384, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 752); }
// deleting via this -380: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(p); 0x00552420(p, 752)
DECOMP(007569B4, AdjDelete_007569B4) { TAdjDeleteVecPtr(ctx, -380, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 752); }

// ---------------------------------------------------------------- 16-byte element array init
// (o, u16 count, Allocator* a, ?, x): fields, the data from a->vfn[0] (default allocator 0x00550EE8), then 0x00558F38(o)
DECOMP(00761DC0, Array16_Init_00761DC0) { THeapArrayInit(ctx, 0x00550EE8, 0x00558F38); }
// (o, u16 count, Allocator* a, ?, x): fields, the data from a->vfn[0] (default allocator 0x00550EE8), then 0x00558F38(o)
DECOMP(00761EC8, Array16_Init_00761EC8) { THeapArrayInit(ctx, 0x00550EE8, 0x00558F38); }
// (o, u16 count, Allocator* a, ?, x): fields, the data from a->vfn[0] (default allocator 0x00550EE8), then 0x00558F38(o)
DECOMP(00761FA0, Array16_Init_00761FA0) { THeapArrayInit(ctx, 0x00550EE8, 0x00558F38); }
// (o, u16 count, Allocator* a, ?, x): fields, the data from a->vfn[0] (default allocator 0x00550EE8), then 0x00558F38(o)
DECOMP(00762078, Array16_Init_00762078) { THeapArrayInit(ctx, 0x00550EE8, 0x00558F38); }
// (o, u16 count, Allocator* a, ?, x): fields, the data from a->vfn[0] (default allocator 0x00550EE8), then 0x00558F38(o)
DECOMP(00762188, Array16_Init_00762188) { THeapArrayInit(ctx, 0x00550EE8, 0x00558F38); }
// (o, u16 count, Allocator* a, ?, x): fields, the data from a->vfn[0] (default allocator 0x00550EE8), then 0x00558F38(o)
DECOMP(007622F0, Array16_Init_007622F0) { THeapArrayInit(ctx, 0x00550EE8, 0x00558F38); }
// (o, u16 count, Allocator* a, ?, x): fields, the data from a->vfn[0] (default allocator 0x00550EE8), then 0x00558F38(o)
DECOMP(00762440, Array16_Init_00762440) { THeapArrayInit(ctx, 0x00550EE8, 0x00558F38); }
// (o, u16 count, Allocator* a, ?, x): fields, the data from a->vfn[0] (default allocator 0x00550EE8), then 0x00558F38(o)
DECOMP(00762548, Array16_Init_00762548) { THeapArrayInit(ctx, 0x00550EE8, 0x00558F38); }
// (o, u16 count, Allocator* a, ?, x): fields, the data from a->vfn[0] (default allocator 0x00550EE8), then 0x00558F38(o)
DECOMP(00762650, Array16_Init_00762650) { THeapArrayInit(ctx, 0x00550EE8, 0x00558F38); }
// (o, u16 count, Allocator* a, ?, x): fields, the data from a->vfn[0] (default allocator 0x00550EE8), then 0x00558F38(o)
DECOMP(00762758, Array16_Init_00762758) { THeapArrayInit(ctx, 0x00550EE8, 0x00558F38); }
// (o, u16 count, Allocator* a, ?, x): fields, the data from a->vfn[0] (default allocator 0x00550EE8), then 0x00558F38(o)
DECOMP(00762860, Array16_Init_00762860) { THeapArrayInit(ctx, 0x00550EE8, 0x00558F38); }
// (o, u16 count, Allocator* a, ?, x): fields, the data from a->vfn[0] (default allocator 0x00550EE8), then 0x00558F38(o)
DECOMP(00762968, Array16_Init_00762968) { THeapArrayInit(ctx, 0x00550EE8, 0x00558F38); }
// (o, u16 count, Allocator* a, ?, x): fields, the data from a->vfn[0] (default allocator 0x00550EE8), then 0x00558F38(o)
DECOMP(007883B4, Array16_Init_007883B4) { THeapArrayInit(ctx, 0x00550EE8, 0x00558F38); }
// (o, u16 count, Allocator* a, ?, x): fields, the data from a->vfn[0] (default allocator 0x00550EE8), then 0x00558F38(o)
DECOMP(007884BC, Array16_Init_007884BC) { THeapArrayInit(ctx, 0x00550EE8, 0x00558F38); }

// ---------------------------------------------------------------- wrappers: target(this, k1, k2)
// 0x000D1288(this, 2, 0)
DECOMP(000D1334, CallK2_000D1334) { TCallK2(ctx, 0x000D1288, 2, 0); }
// 0x000D1288(this, 18, 0)
DECOMP(000D935C, CallK2_000D935C) { TCallK2(ctx, 0x000D1288, 18, 0); }
// 0x000D1288(this, 0, 0)
DECOMP(000D9384, CallK2_000D9384) { TCallK2(ctx, 0x000D1288, 0, 0); }
// 0x000D1288(this, 31, 0)
DECOMP(000D93AC, CallK2_000D93AC) { TCallK2(ctx, 0x000D1288, 31, 0); }
// 0x000D1288(this, 1, 0)
DECOMP(000D93D4, CallK2_000D93D4) { TCallK2(ctx, 0x000D1288, 1, 0); }
// 0x000D1288(this, 14, 0)
DECOMP(000D9424, CallK2_000D9424) { TCallK2(ctx, 0x000D1288, 14, 0); }
// 0x000D1288(this, 13, 0)
DECOMP(000D944C, CallK2_000D944C) { TCallK2(ctx, 0x000D1288, 13, 0); }
// 0x000D1288(this, 19, 0)
DECOMP(000D94F4, CallK2_000D94F4) { TCallK2(ctx, 0x000D1288, 19, 0); }
// 0x000D1288(this, 28, 0)
DECOMP(000D9544, CallK2_000D9544) { TCallK2(ctx, 0x000D1288, 28, 0); }
// 0x000D1288(this, 20, 0)
DECOMP(000D956C, CallK2_000D956C) { TCallK2(ctx, 0x000D1288, 20, 0); }
// 0x000D1288(this, 32, 0)
DECOMP(000D9594, CallK2_000D9594) { TCallK2(ctx, 0x000D1288, 32, 0); }
// 0x000D1288(this, 37, 0)
DECOMP(000D95C4, CallK2_000D95C4) { TCallK2(ctx, 0x000D1288, 37, 0); }
// 0x000D1288(this, 4, 0)
DECOMP(000D9614, CallK2_000D9614) { TCallK2(ctx, 0x000D1288, 4, 0); }

// ---------------------------------------------------------------- either of two global flags
// *(u8*)(0x0105654C +72) || *(u8*)(0x01056500 +72)
DECOMP(002116F4, AnyFlag2_002116F4) { TAnyFlag2(ctx, 0x0105654C, 0x01056500, 72); }
// *(u8*)(0x01056598 +72) || *(u8*)(0x01056500 +72)
DECOMP(00212D78, AnyFlag2_00212D78) { TAnyFlag2(ctx, 0x01056598, 0x01056500, 72); }
// *(u8*)(0x010565E4 +72) || *(u8*)(0x01056500 +72)
DECOMP(0021452C, AnyFlag2_0021452C) { TAnyFlag2(ctx, 0x010565E4, 0x01056500, 72); }
// *(u8*)(0x01056A10 +72) || *(u8*)(0x01056500 +72)
DECOMP(00216A64, AnyFlag2_00216A64) { TAnyFlag2(ctx, 0x01056A10, 0x01056500, 72); }
// *(u8*)(0x01056630 +72) || *(u8*)(0x01056500 +72)
DECOMP(00219864, AnyFlag2_00219864) { TAnyFlag2(ctx, 0x01056630, 0x01056500, 72); }
// *(u8*)(0x0105667C +72) || *(u8*)(0x01056500 +72)
DECOMP(00219FB0, AnyFlag2_00219FB0) { TAnyFlag2(ctx, 0x0105667C, 0x01056500, 72); }
// *(u8*)(0x010566C8 +72) || *(u8*)(0x01056500 +72)
DECOMP(0021D2C8, AnyFlag2_0021D2C8) { TAnyFlag2(ctx, 0x010566C8, 0x01056500, 72); }
// *(u8*)(0x01056714 +72) || *(u8*)(0x01056500 +72)
DECOMP(0021DE9C, AnyFlag2_0021DE9C) { TAnyFlag2(ctx, 0x01056714, 0x01056500, 72); }
// *(u8*)(0x01056760 +72) || *(u8*)(0x01056500 +72)
DECOMP(00220158, AnyFlag2_00220158) { TAnyFlag2(ctx, 0x01056760, 0x01056500, 72); }
// *(u8*)(0x010567AC +72) || *(u8*)(0x01056500 +72)
DECOMP(00221090, AnyFlag2_00221090) { TAnyFlag2(ctx, 0x010567AC, 0x01056500, 72); }
// *(u8*)(0x010567F8 +72) || *(u8*)(0x01056500 +72)
DECOMP(00221720, AnyFlag2_00221720) { TAnyFlag2(ctx, 0x010567F8, 0x01056500, 72); }
// *(u8*)(0x010568DC +72) || *(u8*)(0x01056500 +72)
DECOMP(00223C60, AnyFlag2_00223C60) { TAnyFlag2(ctx, 0x010568DC, 0x01056500, 72); }
// *(u8*)(0x01056928 +72) || *(u8*)(0x01056500 +72)
DECOMP(00225E50, AnyFlag2_00225E50) { TAnyFlag2(ctx, 0x01056928, 0x01056500, 72); }

// ---------------------------------------------------------------- virtual result tests: vfn[32](this, 0) == k
// this->vfn[32](this, 0) == 2
DECOMP(00312994, VIsK_00312994) { TVIsK(ctx, 32, 2); }
// this->vfn[32](this, 0) == 5
DECOMP(0031321C, VIsK_0031321C) { TVIsK(ctx, 32, 5); }
// this->vfn[32](this, 0) == 5
DECOMP(003141A8, VIsK_003141A8) { TVIsK(ctx, 32, 5); }
// this->vfn[32](this, 0) == 2
DECOMP(00315000, VIsK_00315000) { TVIsK(ctx, 32, 2); }
// this->vfn[32](this, 0) == 4
DECOMP(0031573C, VIsK_0031573C) { TVIsK(ctx, 32, 4); }
// this->vfn[32](this, 0) == 4
DECOMP(003164D8, VIsK_003164D8) { TVIsK(ctx, 32, 4); }
// this->vfn[32](this, 0) == 5
DECOMP(0031786C, VIsK_0031786C) { TVIsK(ctx, 32, 5); }
// this->vfn[32](this, 0) == 4
DECOMP(00318BD8, VIsK_00318BD8) { TVIsK(ctx, 32, 4); }
// this->vfn[32](this, 0) == 3
DECOMP(003198B0, VIsK_003198B0) { TVIsK(ctx, 32, 3); }
// this->vfn[32](this, 0) == 2
DECOMP(0031A32C, VIsK_0031A32C) { TVIsK(ctx, 32, 2); }
// this->vfn[32](this, 0) == 3
DECOMP(0031A7E8, VIsK_0031A7E8) { TVIsK(ctx, 32, 3); }
// this->vfn[32](this, 0) == 4
DECOMP(0031AEA4, VIsK_0031AEA4) { TVIsK(ctx, 32, 4); }
// this->vfn[32](this, 0) == 1
DECOMP(0031AEF0, VIsK_0031AEF0) { TVIsK(ctx, 32, 1); }

// ---------------------------------------------------------------- deleting destructors of pooled objects
// deleting: vtable 0x00B13974, then pool(*(r13 - 28332) +44)->vfn[3](pool, this, this->+4)
DECOMP(007491DC, VtB13974_Delete) { TDeletePooled(ctx, 0x00B13974, -28332); }
// deleting: vtable 0x00B13974, then pool(*(r13 - 28332) +44)->vfn[3](pool, this, this->+4)
DECOMP(00749248, VtB13974_Delete_00749248) { TDeletePooled(ctx, 0x00B13974, -28332); }
// deleting: vtable 0x00B13974, then pool(*(r13 - 28332) +44)->vfn[3](pool, this, this->+4)
DECOMP(0074E9E8, VtB13974_Delete_0074E9E8) { TDeletePooled(ctx, 0x00B13974, -28332); }
// deleting: vtable 0x00B13974, then pool(*(r13 - 28332) +44)->vfn[3](pool, this, this->+4)
DECOMP(00759CBC, VtB13974_Delete_00759CBC) { TDeletePooled(ctx, 0x00B13974, -28332); }
// deleting: vtable 0x00B13974, then pool(*(r13 - 28332) +44)->vfn[3](pool, this, this->+4)
DECOMP(0075D388, VtB13974_Delete_0075D388) { TDeletePooled(ctx, 0x00B13974, -28332); }
// deleting: vtable 0x00B13974, then pool(*(r13 - 28332) +44)->vfn[3](pool, this, this->+4)
DECOMP(0075D3F4, VtB13974_Delete_0075D3F4) { TDeletePooled(ctx, 0x00B13974, -28332); }
// deleting: vtable 0x00B13974, then pool(*(r13 - 28332) +44)->vfn[3](pool, this, this->+4)
DECOMP(0075D460, VtB13974_Delete_0075D460) { TDeletePooled(ctx, 0x00B13974, -28332); }
// deleting: vtable 0x00B13974, then pool(*(r13 - 28332) +44)->vfn[3](pool, this, this->+4)
DECOMP(0075D908, VtB13974_Delete_0075D908) { TDeletePooled(ctx, 0x00B13974, -28332); }
// deleting: vtable 0x00B13974, then pool(*(r13 - 28332) +44)->vfn[3](pool, this, this->+4)
DECOMP(00762A40, VtB13974_Delete_00762A40) { TDeletePooled(ctx, 0x00B13974, -28332); }
// deleting: vtable 0x00B13974, then pool(*(r13 - 28332) +44)->vfn[3](pool, this, this->+4)
DECOMP(00765E94, VtB13974_Delete_00765E94) { TDeletePooled(ctx, 0x00B13974, -28332); }
// deleting: vtable 0x00B13974, then pool(*(r13 - 28332) +44)->vfn[3](pool, this, this->+4)
DECOMP(00770784, VtB13974_Delete_00770784) { TDeletePooled(ctx, 0x00B13974, -28332); }
// deleting: vtable 0x00B13974, then pool(*(r13 - 28332) +44)->vfn[3](pool, this, this->+4)
DECOMP(00771D50, VtB13974_Delete_00771D50) { TDeletePooled(ctx, 0x00B13974, -28332); }
// deleting: vtable 0x00B13974, then pool(*(r13 - 28332) +44)->vfn[3](pool, this, this->+4)
DECOMP(00788318, VtB13974_Delete_00788318) { TDeletePooled(ctx, 0x00B13974, -28332); }

// ---------------------------------------------------------------- u32 table lookups
// ((u32*)0x00AD9970)[a1]
DECOMP(00768958, TableU32_00768958) { TTableU32(ctx, 0x00AD9970); }
// ((u32*)0x00AD99A8)[a1]
DECOMP(00768974, TableU32_00768974) { TTableU32(ctx, 0x00AD99A8); }
// ((u32*)0x00AD99D0)[a1]
DECOMP(00768990, TableU32_00768990) { TTableU32(ctx, 0x00AD99D0); }
// ((u32*)0x00AD99F8)[a1]
DECOMP(007689AC, TableU32_007689AC) { TTableU32(ctx, 0x00AD99F8); }
// ((u32*)0x00AD9A34)[a1]
DECOMP(007689C8, TableU32_007689C8) { TTableU32(ctx, 0x00AD9A34); }
// ((u32*)0x00AD9A6C)[a1]
DECOMP(007689E4, TableU32_007689E4) { TTableU32(ctx, 0x00AD9A6C); }
// ((u32*)0x00AD9ACC)[a1]
DECOMP(00768A00, TableU32_00768A00) { TTableU32(ctx, 0x00AD9ACC); }
// ((u32*)0x00AD9AFC)[a1]
DECOMP(00768A1C, TableU32_00768A1C) { TTableU32(ctx, 0x00AD9AFC); }
// ((u32*)0x00AD9B24)[a1]
DECOMP(00768A40, TableU32_00768A40) { TTableU32(ctx, 0x00AD9B24); }
// ((u32*)0x00AD9B34)[a1]
DECOMP(00768A64, TableU32_00768A64) { TTableU32(ctx, 0x00AD9B34); }
// ((u32*)0x00AD9B50)[a1]
DECOMP(00768A88, TableU32_00768A88) { TTableU32(ctx, 0x00AD9B50); }
// ((u32*)0x00AD9B6C)[a1]
DECOMP(00768AAC, TableU32_00768AAC) { TTableU32(ctx, 0x00AD9B6C); }

// ---------------------------------------------------------------- byte getters: (this->+216 + k)[i]
// ((u8*)(this->+216 +196))[(s32)a1]
DECOMP(000E7FD4, ByteAt_000E7FD4) { TByteAt(ctx, 216, 196); }
// ((u8*)(this->+216 +260))[(s32)a1]
DECOMP(00169248, ByteAt_00169248) { TByteAt(ctx, 216, 260); }
// ((u8*)(this->+216 +260))[(s32)a1]
DECOMP(00169260, ByteAt_00169260) { TByteAt(ctx, 216, 260); }
// ((u8*)(this->+216 +244))[(s32)a1]
DECOMP(00179FE8, ByteAt_00179FE8) { TByteAt(ctx, 216, 244); }
// ((u8*)(this->+216 +281))[(s32)a1]
DECOMP(00185AEC, ByteAt_00185AEC) { TByteAt(ctx, 216, 281); }
// ((u8*)(this->+216 +281))[(s32)a1]
DECOMP(00185B04, ByteAt_00185B04) { TByteAt(ctx, 216, 281); }
// ((u8*)(this->+216 +285))[(s32)a1]
DECOMP(00193234, ByteAt_00193234) { TByteAt(ctx, 216, 285); }
// ((u8*)(this->+216 +285))[(s32)a1]
DECOMP(0019324C, ByteAt_0019324C) { TByteAt(ctx, 216, 285); }
// ((u8*)(this->+216 +368))[(s32)a1]
DECOMP(001E4370, ByteAt_001E4370) { TByteAt(ctx, 216, 368); }
// ((u8*)(this->+216 +302))[(s32)a1]
DECOMP(001EB09C, ByteAt_001EB09C) { TByteAt(ctx, 216, 302); }
// ((u8*)(this->+216 +288))[(s32)a1]
DECOMP(001FC6CC, ByteAt_001FC6CC) { TByteAt(ctx, 216, 288); }

// ---------------------------------------------------------------- element addresses: this->+0 + i * 4
// &((u32*)this->+0)[a1]
DECOMP(0074D1E4, ElemAddr4_0074D1E4) { TIndexAddr4(ctx); }
// &((u32*)this->+0)[a1]
DECOMP(0074E88C, ElemAddr4_0074E88C) { TIndexAddr4(ctx); }
// &((u32*)this->+0)[a1]
DECOMP(0074E968, ElemAddr4_0074E968) { TIndexAddr4(ctx); }
// &((u32*)this->+0)[a1]
DECOMP(00757E18, ElemAddr4_00757E18) { TIndexAddr4(ctx); }
// &((u32*)this->+0)[a1]
DECOMP(007612F4, ElemAddr4_007612F4) { TIndexAddr4(ctx); }
// &((u32*)this->+0)[a1]
DECOMP(00761414, ElemAddr4_00761414) { TIndexAddr4(ctx); }
// &((u32*)this->+0)[a1]
DECOMP(0077D164, ElemAddr4_0077D164) { TIndexAddr4(ctx); }
// &((u32*)this->+0)[a1]
DECOMP(0077D5CC, ElemAddr4_0077D5CC) { TIndexAddr4(ctx); }
// &((u32*)this->+0)[a1]
DECOMP(0077D5E0, ElemAddr4_0077D5E0) { TIndexAddr4(ctx); }
// &((u32*)this->+0)[a1]
DECOMP(00787488, ElemAddr4_00787488) { TIndexAddr4(ctx); }

// ---------------------------------------------------------------- factories: SmallAlloc, then T(p, k)
// p = SmallAlloc(32); if p: 0x004F88FC(p, 0x00AC4DFC)
DECOMP(0007E03C, NewSmall_0007E03C) { TNewSmallK(ctx, 32, 0x004F88FC, 0x00AC4DFC); }
// p = SmallAlloc(32); if p: 0x004F88FC(p, 0x00AC4E08)
DECOMP(0007E870, NewSmall_0007E870) { TNewSmallK(ctx, 32, 0x004F88FC, 0x00AC4E08); }
// p = SmallAlloc(32); if p: 0x004F88FC(p, 0x00AC4F88)
DECOMP(00084C5C, NewSmall_00084C5C) { TNewSmallK(ctx, 32, 0x004F88FC, 0x00AC4F88); }
// p = SmallAlloc(32); if p: 0x004F88FC(p, 0x00AC5850)
DECOMP(00098AE8, NewSmall_00098AE8) { TNewSmallK(ctx, 32, 0x004F88FC, 0x00AC5850); }
// p = SmallAlloc(32); if p: 0x004F88FC(p, 0x00AEF014)
DECOMP(004C147C, NewSmall_004C147C) { TNewSmallK(ctx, 32, 0x004F88FC, 0x00AEF014); }
// p = SmallAlloc(32); if p: 0x004F88FC(p, 0x00AEF020)
DECOMP(004C1784, NewSmall_004C1784) { TNewSmallK(ctx, 32, 0x004F88FC, 0x00AEF020); }
// p = SmallAlloc(32); if p: 0x004F88FC(p, 0x00AEF038)
DECOMP(004C1B18, NewSmall_004C1B18) { TNewSmallK(ctx, 32, 0x004F88FC, 0x00AEF038); }
// p = SmallAlloc(32); if p: 0x004F88FC(p, 0x00AEF098)
DECOMP(004C6708, NewSmall_004C6708) { TNewSmallK(ctx, 32, 0x004F88FC, 0x00AEF098); }
// p = SmallAlloc(32); if p: 0x004F88FC(p, 0x00AEF0B0)
DECOMP(004C6F4C, NewSmall_004C6F4C) { TNewSmallK(ctx, 32, 0x004F88FC, 0x00AEF0B0); }
// p = SmallAlloc(32); if p: 0x004F88FC(p, 0x00AEF0B8)
DECOMP(004C82F8, NewSmall_004C82F8) { TNewSmallK(ctx, 32, 0x004F88FC, 0x00AEF0B8); }
// p = SmallAlloc(32); if p: 0x004F88FC(p, 0x00AEF0C4)
DECOMP(004C9674, NewSmall_004C9674) { TNewSmallK(ctx, 32, 0x004F88FC, 0x00AEF0C4); }

// ---------------------------------------------------------------- destructors: an owned object at +40, a PtrVector at +44, the base
// *this = vtable 0x00B1DCA8; owned +40 (0x004C8E80, SmallFree 140); ~PtrVector(+44); 0x00464AD0(this)
DECOMP(00312118, VtB1DCA8_Dtor) { TDtorOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0); }
// *this = vtable 0x00B1DCA8; owned +40 (0x004C8E80, SmallFree 140); ~PtrVector(+44); 0x00464AD0(this)
DECOMP(0031252C, VtB1DCA8_Dtor_0031252C) { TDtorOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0); }
// *this = vtable 0x00B1DCA8; owned +40 (0x004C8E80, SmallFree 140); ~PtrVector(+44); 0x00464AD0(this)
DECOMP(00313E38, VtB1DCA8_Dtor_00313E38) { TDtorOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0); }
// *this = vtable 0x00B1DCA8; owned +40 (0x004C8E80, SmallFree 140); ~PtrVector(+44); 0x00464AD0(this)
DECOMP(00314BB4, VtB1DCA8_Dtor_00314BB4) { TDtorOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0); }
// *this = vtable 0x00B1DCA8; owned +40 (0x004C8E80, SmallFree 140); ~PtrVector(+44); 0x00464AD0(this)
DECOMP(00315154, VtB1DCA8_Dtor_00315154) { TDtorOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0); }
// *this = vtable 0x00B1DCA8; owned +40 (0x004C8E80, SmallFree 140); ~PtrVector(+44); 0x00464AD0(this)
DECOMP(003161A4, VtB1DCA8_Dtor_003161A4) { TDtorOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0); }
// *this = vtable 0x00B1DCA8; owned +40 (0x004C8E80, SmallFree 140); ~PtrVector(+44); 0x00464AD0(this)
DECOMP(003170C0, VtB1DCA8_Dtor_003170C0) { TDtorOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0); }
// *this = vtable 0x00B1DCA8; owned +40 (0x004C8E80, SmallFree 140); ~PtrVector(+44); 0x00464AD0(this)
DECOMP(00319300, VtB1DCA8_Dtor_00319300) { TDtorOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0); }
// *this = vtable 0x00B1DCA8; owned +40 (0x004C8E80, SmallFree 140); ~PtrVector(+44); 0x00464AD0(this)
DECOMP(00319F7C, VtB1DCA8_Dtor_00319F7C) { TDtorOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0); }
// *this = vtable 0x00B1DCA8; owned +40 (0x004C8E80, SmallFree 140); ~PtrVector(+44); 0x00464AD0(this)
DECOMP(0031A43C, VtB1DCA8_Dtor_0031A43C) { TDtorOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0); }
// *this = vtable 0x00B1DCA8; owned +40 (0x004C8E80, SmallFree 140); ~PtrVector(+44); 0x00464AD0(this)
DECOMP(0031A984, VtB1DCA8_Dtor_0031A984) { TDtorOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0); }

// ---------------------------------------------------------------- their deleting versions
// deleting: vtable 0x00B1DCA8; owned +40; ~PtrVector(+44); 0x00464AD0(this); 0x0054EE6C(this, 0)
DECOMP(003121C0, VtB1DCA8_Delete) { TDeleteOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1DCA8; owned +40; ~PtrVector(+44); 0x00464AD0(this); 0x00552420(this, 76)
DECOMP(003125C8, VtB1DCA8_Delete_003125C8) { TDeleteOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0, 0x00552420, 76); }
// deleting: vtable 0x00B1DCA8; owned +40; ~PtrVector(+44); 0x00464AD0(this); 0x00552420(this, 68)
DECOMP(00313ED4, VtB1DCA8_Delete_00313ED4) { TDeleteOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0, 0x00552420, 68); }
// deleting: vtable 0x00B1DCA8; owned +40; ~PtrVector(+44); 0x00464AD0(this); 0x00552420(this, 76)
DECOMP(00314C50, VtB1DCA8_Delete_00314C50) { TDeleteOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0, 0x00552420, 76); }
// deleting: vtable 0x00B1DCA8; owned +40; ~PtrVector(+44); 0x00464AD0(this); 0x00552420(this, 92)
DECOMP(003151F0, VtB1DCA8_Delete_003151F0) { TDeleteOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0, 0x00552420, 92); }
// deleting: vtable 0x00B1DCA8; owned +40; ~PtrVector(+44); 0x00464AD0(this); 0x00552420(this, 88)
DECOMP(00316240, VtB1DCA8_Delete_00316240) { TDeleteOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0, 0x00552420, 88); }
// deleting: vtable 0x00B1DCA8; owned +40; ~PtrVector(+44); 0x00464AD0(this); 0x00552420(this, 192)
DECOMP(0031715C, VtB1DCA8_Delete_0031715C) { TDeleteOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0, 0x00552420, 192); }
// deleting: vtable 0x00B1DCA8; owned +40; ~PtrVector(+44); 0x00464AD0(this); 0x00552420(this, 72)
DECOMP(0031939C, VtB1DCA8_Delete_0031939C) { TDeleteOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0, 0x00552420, 72); }
// deleting: vtable 0x00B1DCA8; owned +40; ~PtrVector(+44); 0x00464AD0(this); 0x00552420(this, 68)
DECOMP(0031A018, VtB1DCA8_Delete_0031A018) { TDeleteOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0, 0x00552420, 68); }
// deleting: vtable 0x00B1DCA8; owned +40; ~PtrVector(+44); 0x00464AD0(this); 0x00552420(this, 68)
DECOMP(0031A4D8, VtB1DCA8_Delete_0031A4D8) { TDeleteOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0, 0x00552420, 68); }
// deleting: vtable 0x00B1DCA8; owned +40; ~PtrVector(+44); 0x00464AD0(this); 0x00552420(this, 96)
DECOMP(0031AA20, VtB1DCA8_Delete_0031AA20) { TDeleteOwnVec(ctx, 0x00B1DCA8, 0x004C8E80, 140, 0x00464AD0, 0x00552420, 96); }

// ---------------------------------------------------------------- factories: Mem_Alloc, the base constructor (p, a1), the vtable
// p = Mem_Alloc(12, 0, 16); if p: 0x004BB3B0(p, a1), *p = vtable 0x00B0F130
DECOMP(0006CB00, VtB0F130_New) { TNew1Vt(ctx, 12, 0x004BB3B0, 0x00B0F130); }
// p = Mem_Alloc(12, 0, 16); if p: 0x004BB3B0(p, a1), *p = vtable 0x00B0F248
DECOMP(0006ECF8, VtB0F248_New) { TNew1Vt(ctx, 12, 0x004BB3B0, 0x00B0F248); }
// p = Mem_Alloc(12, 0, 16); if p: 0x004BB3B0(p, a1), *p = vtable 0x00B0F2C0
DECOMP(0006F580, VtB0F2C0_New) { TNew1Vt(ctx, 12, 0x004BB3B0, 0x00B0F2C0); }
// p = Mem_Alloc(12, 0, 16); if p: 0x004BB3B0(p, a1), *p = vtable 0x00B0F338
DECOMP(0006FD20, VtB0F338_New) { TNew1Vt(ctx, 12, 0x004BB3B0, 0x00B0F338); }
// p = Mem_Alloc(12, 0, 16); if p: 0x004BB3B0(p, a1), *p = vtable 0x00B0FCC0
DECOMP(00080654, VtB0FCC0_New) { TNew1Vt(ctx, 12, 0x004BB3B0, 0x00B0FCC0); }
// p = Mem_Alloc(12, 0, 16); if p: 0x004BB3B0(p, a1), *p = vtable 0x00B0FDB0
DECOMP(00081754, VtB0FDB0_New) { TNew1Vt(ctx, 12, 0x004BB3B0, 0x00B0FDB0); }
// p = Mem_Alloc(12, 0, 16); if p: 0x004BB3B0(p, a1), *p = vtable 0x00B0FDD8
DECOMP(00081840, VtB0FDD8_New) { TNew1Vt(ctx, 12, 0x004BB3B0, 0x00B0FDD8); }
// p = Mem_Alloc(12, 0, 16); if p: 0x004BB3B0(p, a1), *p = vtable 0x00B10080
DECOMP(0008586C, VtB10080_New) { TNew1Vt(ctx, 12, 0x004BB3B0, 0x00B10080); }
// p = Mem_Alloc(12, 0, 16); if p: 0x004BB3B0(p, a1), *p = vtable 0x00B10DC8
DECOMP(000995D8, VtB10DC8_New) { TNew1Vt(ctx, 12, 0x004BB3B0, 0x00B10DC8); }
// p = Mem_Alloc(12, 0, 16); if p: 0x004BB3B0(p, a1), *p = vtable 0x00B119E0
DECOMP(000A62DC, VtB119E0_New) { TNew1Vt(ctx, 12, 0x004BB3B0, 0x00B119E0); }

// ---------------------------------------------------------------- base destructors: the owned PtrVector* at +16
// *this = vtable 0x00B12260; owned PtrVector* +16 (SmallFree 20)
DECOMP(0073B790, VtB12260_Dtor) { TDtorVecPtr(ctx, 0x00B12260, 16); }
// *this = vtable 0x00B12260; owned PtrVector* +16 (SmallFree 20)
DECOMP(00746060, VtB12260_Dtor_00746060) { TDtorVecPtr(ctx, 0x00B12260, 16); }
// *this = vtable 0x00B12260; owned PtrVector* +16 (SmallFree 20)
DECOMP(007460D0, VtB12260_Dtor_007460D0) { TDtorVecPtr(ctx, 0x00B12260, 16); }
// *this = vtable 0x00B12260; owned PtrVector* +16 (SmallFree 20)
DECOMP(00746F5C, VtB12260_Dtor_00746F5C) { TDtorVecPtr(ctx, 0x00B12260, 16); }
// *this = vtable 0x00B12260; owned PtrVector* +16 (SmallFree 20)
DECOMP(0074FD10, VtB12260_Dtor_0074FD10) { TDtorVecPtr(ctx, 0x00B12260, 16); }
// *this = vtable 0x00B12260; owned PtrVector* +16 (SmallFree 20)
DECOMP(007592B8, VtB12260_Dtor_007592B8) { TDtorVecPtr(ctx, 0x00B12260, 16); }
// *this = vtable 0x00B12260; owned PtrVector* +16 (SmallFree 20)
DECOMP(007599D0, VtB12260_Dtor_007599D0) { TDtorVecPtr(ctx, 0x00B12260, 16); }
// *this = vtable 0x00B12260; owned PtrVector* +16 (SmallFree 20)
DECOMP(00759A40, VtB12260_Dtor_00759A40) { TDtorVecPtr(ctx, 0x00B12260, 16); }
// *this = vtable 0x00B12260; owned PtrVector* +16 (SmallFree 20)
DECOMP(00759AB0, VtB12260_Dtor_00759AB0) { TDtorVecPtr(ctx, 0x00B12260, 16); }
// *this = vtable 0x00B12260; owned PtrVector* +16 (SmallFree 20)
DECOMP(00759BBC, VtB12260_Dtor_00759BBC) { TDtorVecPtr(ctx, 0x00B12260, 16); }

// ---------------------------------------------------------------- their deleting versions
// deleting: vtable 0x00B12260; owned PtrVector* +16; 0x0054EE6C(this, 0)
DECOMP(00746FCC, VtB12260_Delete) { TDeleteVecPtr(ctx, 0x00B12260, 16, 0x0054EE6C, 0); }
// deleting: vtable 0x00B12260; owned PtrVector* +16; 0x00552420(this, 192)
DECOMP(00751130, VtB12260_Delete_00751130) { TDeleteVecPtr(ctx, 0x00B12260, 16, 0x00552420, 192); }
// deleting: vtable 0x00B12260; owned PtrVector* +16; 0x00552420(this, 240)
DECOMP(00753A24, VtB12260_Delete_00753A24) { TDeleteVecPtr(ctx, 0x00B12260, 16, 0x00552420, 240); }
// deleting: vtable 0x00B12260; owned PtrVector* +16; 0x00552420(this, 192)
DECOMP(007593B0, VtB12260_Delete_007593B0) { TDeleteVecPtr(ctx, 0x00B12260, 16, 0x00552420, 192); }
// deleting: vtable 0x00B12260; owned PtrVector* +16; 0x0054EE6C(this, 0)
DECOMP(00759DA8, VtB12260_Delete_00759DA8) { TDeleteVecPtr(ctx, 0x00B12260, 16, 0x0054EE6C, 0); }
// deleting: vtable 0x00B12260; owned PtrVector* +16; 0x00552420(this, 256)
DECOMP(0075B268, VtB12260_Delete_0075B268) { TDeleteVecPtr(ctx, 0x00B12260, 16, 0x00552420, 256); }
// deleting: vtable 0x00B12260; owned PtrVector* +16; 0x00552420(this, 240)
DECOMP(0075B400, VtB12260_Delete_0075B400) { TDeleteVecPtr(ctx, 0x00B12260, 16, 0x00552420, 240); }
// deleting: vtable 0x00B12260; owned PtrVector* +16; 0x00552420(this, 208)
DECOMP(0075B510, VtB12260_Delete_0075B510) { TDeleteVecPtr(ctx, 0x00B12260, 16, 0x00552420, 208); }
// deleting: vtable 0x00B12260; owned PtrVector* +16; 0x00552420(this, 192)
DECOMP(0075B620, VtB12260_Delete_0075B620) { TDeleteVecPtr(ctx, 0x00B12260, 16, 0x00552420, 192); }
// deleting: vtable 0x00B12260; owned PtrVector* +16; 0x00552420(this, 464)
DECOMP(0075B8F4, VtB12260_Delete_0075B8F4) { TDeleteVecPtr(ctx, 0x00B12260, 16, 0x00552420, 464); }

// ---------------------------------------------------------------- event-record inits
// (o, src, a, b, c): kind 15, the time, src-derived defaults
DECOMP(004653F4, EventInit_004653F4) { TEventInit(ctx, 15, 0x00F65CA0); }
// (o, src, a, b, c): kind 16, the time, src-derived defaults
DECOMP(00465494, EventInit_00465494) { TEventInit(ctx, 16, 0x00F65CA0); }
// (o, src, a, b, c): kind 27, the time, src-derived defaults
DECOMP(00465534, EventInit_00465534) { TEventInit(ctx, 27, 0x00F65CA0); }
// (o, src, a, b, c): kind 19, the time, src-derived defaults
DECOMP(0046564C, EventInit_0046564C) { TEventInit(ctx, 19, 0x00F65CA0); }
// (o, src, a, b, c): kind 20, the time, src-derived defaults
DECOMP(004656EC, EventInit_004656EC) { TEventInit(ctx, 20, 0x00F65CA0); }
// (o, src, a, b, c): kind 21, the time, src-derived defaults
DECOMP(0046578C, EventInit_0046578C) { TEventInit(ctx, 21, 0x00F65CA0); }
// (o, src, a, b, c): kind 22, the time, src-derived defaults
DECOMP(0046582C, EventInit_0046582C) { TEventInit(ctx, 22, 0x00F65CA0); }
// (o, src, a, b, c): kind 24, the time, src-derived defaults
DECOMP(00465970, EventInit_00465970) { TEventInit(ctx, 24, 0x00F65CA0); }
// (o, src, a, b, c): kind 25, the time, src-derived defaults
DECOMP(00465A10, EventInit_00465A10) { TEventInit(ctx, 25, 0x00F65CA0); }
// (o, src, a, b, c): kind 32, the time, src-derived defaults
DECOMP(00465BB0, EventInit_00465BB0) { TEventInit(ctx, 32, 0x00F65CA0); }

// ---------------------------------------------------------------- destructors: the vtable, then a member's destructor
// *this = vtable 0x00B0DC70; 0x0062D08C(this + 216)
DECOMP(00738364, VtB0DC70_Dtor) { TDtorMember(ctx, 0x00B0DC70, 216, 0x0062D08C); }
// *this = vtable 0x00B0DC70; 0x0062D08C(this + 216)
DECOMP(00738998, VtB0DC70_Dtor_00738998) { TDtorMember(ctx, 0x00B0DC70, 216, 0x0062D08C); }
// *this = vtable 0x00B0DC70; 0x0062D08C(this + 216)
DECOMP(00738C0C, VtB0DC70_Dtor_00738C0C) { TDtorMember(ctx, 0x00B0DC70, 216, 0x0062D08C); }
// *this = vtable 0x00B0DC70; 0x0062D08C(this + 216)
DECOMP(00738D10, VtB0DC70_Dtor_00738D10) { TDtorMember(ctx, 0x00B0DC70, 216, 0x0062D08C); }
// *this = vtable 0x00B0DC70; 0x0062D08C(this + 216)
DECOMP(00739214, VtB0DC70_Dtor_00739214) { TDtorMember(ctx, 0x00B0DC70, 216, 0x0062D08C); }
// *this = vtable 0x00B0DC70; 0x0062D08C(this + 216)
DECOMP(00739624, VtB0DC70_Dtor_00739624) { TDtorMember(ctx, 0x00B0DC70, 216, 0x0062D08C); }
// *this = vtable 0x00B0DC70; 0x0062D08C(this + 216)
DECOMP(007396C8, VtB0DC70_Dtor_007396C8) { TDtorMember(ctx, 0x00B0DC70, 216, 0x0062D08C); }
// *this = vtable 0x00B0DC70; 0x0062D08C(this + 216)
DECOMP(00739798, VtB0DC70_Dtor_00739798) { TDtorMember(ctx, 0x00B0DC70, 216, 0x0062D08C); }
// *this = vtable 0x00B0DC70; 0x0062D08C(this + 216)
DECOMP(00739878, VtB0DC70_Dtor_00739878) { TDtorMember(ctx, 0x00B0DC70, 216, 0x0062D08C); }

// ---------------------------------------------------------------- Vec3 copies
// (f32 dst[3], const f32 src[3])
DECOMP(0010ED00, Vec3_Copy_0010ED00) { TCopyVec3(ctx); }
// (f32 dst[3], const f32 src[3])
DECOMP(001379D8, Vec3_Copy_001379D8) { TCopyVec3(ctx); }
// (f32 dst[3], const f32 src[3])
DECOMP(00173310, Vec3_Copy_00173310) { TCopyVec3(ctx); }
// (f32 dst[3], const f32 src[3])
DECOMP(00204D78, Vec3_Copy_00204D78) { TCopyVec3(ctx); }
// (f32 dst[3], const f32 src[3])
DECOMP(0026B2D0, Vec3_Copy_0026B2D0) { TCopyVec3(ctx); }
// (f32 dst[3], const f32 src[3])
DECOMP(0026B61C, Vec3_Copy_0026B61C) { TCopyVec3(ctx); }
// (f32 dst[3], const f32 src[3])
DECOMP(0034E458, Vec3_Copy_0034E458) { TCopyVec3(ctx); }
// (f32 dst[3], const f32 src[3])
DECOMP(003E7B08, Vec3_Copy_003E7B08) { TCopyVec3(ctx); }
// (f32 dst[3], const f32 src[3])
DECOMP(003E7BD8, Vec3_Copy_003E7BD8) { TCopyVec3(ctx); }

// ---------------------------------------------------------------- destructors owning a PtrVector* at +388 (three vtables)
// vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this)
DECOMP(001DC04C, VtB12898_Dtor) { TAdjDtorVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this)
DECOMP(001EA650, VtB12898_Dtor_001EA650) { TAdjDtorVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this)
DECOMP(001F62DC, VtB12898_Dtor_001F62DC) { TAdjDtorVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this)
DECOMP(00751AA4, VtB12898_Dtor_00751AA4) { TAdjDtorVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this)
DECOMP(00751FD4, VtB12898_Dtor_00751FD4) { TAdjDtorVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this)
DECOMP(007526AC, VtB12898_Dtor_007526AC) { TAdjDtorVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this)
DECOMP(00752CB0, VtB12898_Dtor_00752CB0) { TAdjDtorVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this)
DECOMP(00753C08, VtB12898_Dtor_00753C08) { TAdjDtorVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this)
DECOMP(00756154, VtB12898_Dtor_00756154) { TAdjDtorVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this)
DECOMP(00756758, VtB12898_Dtor_00756758) { TAdjDtorVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }

// ---------------------------------------------------------------- their deleting versions
// deleting: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this); 0x00552420(this, 432)
DECOMP(001DC104, VtB12898_Delete) { TAdjDeleteVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 432); }
// deleting: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this); 0x00552420(this, 464)
DECOMP(001EA708, VtB12898_Delete_001EA708) { TAdjDeleteVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 464); }
// deleting: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this); 0x00552420(this, 576)
DECOMP(001F6394, VtB12898_Delete_001F6394) { TAdjDeleteVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 576); }
// deleting: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this); 0x00552420(this, 480)
DECOMP(00751B5C, VtB12898_Delete_00751B5C) { TAdjDeleteVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 480); }
// deleting: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this); 0x00552420(this, 448)
DECOMP(0075208C, VtB12898_Delete_0075208C) { TAdjDeleteVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 448); }
// deleting: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this); 0x00552420(this, 480)
DECOMP(00752764, VtB12898_Delete_00752764) { TAdjDeleteVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 480); }
// deleting: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this); 0x00552420(this, 576)
DECOMP(00752D68, VtB12898_Delete_00752D68) { TAdjDeleteVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 576); }
// deleting: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this); 0x00552420(this, 464)
DECOMP(00753CC0, VtB12898_Delete_00753CC0) { TAdjDeleteVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 464); }
// deleting: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this); 0x00552420(this, 624)
DECOMP(0075620C, VtB12898_Delete_0075620C) { TAdjDeleteVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 624); }
// deleting: vtable 0x00B12898, owned PtrVector* +388; base vtable 0x00B126B8, 0x000DF934(this); 0x00552420(this, 752)
DECOMP(00756810, VtB12898_Delete_00756810) { TAdjDeleteVecPtr(ctx, 0, {0x00B12898, 0x00B12A44, 0x00B12A6C}, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 752); }

// ---------------------------------------------------------------- constructors: base(this, k), the +9 flag, the vtable
// 0x0042C9D0(this, 1); this->+9 = (u8)a1; *this = vtable 0x00B20FE0
DECOMP(00431FA8, VtB20FE0_Ctor) { TCtorFlagVt(ctx, 0x0042C9D0, 1, 0x00B20FE0); }
// 0x0042C9D0(this, 0); this->+9 = (u8)a1; *this = vtable 0x00B20FF8
DECOMP(0043202C, VtB20FF8_Ctor) { TCtorFlagVt(ctx, 0x0042C9D0, 0, 0x00B20FF8); }
// 0x0042C9D0(this, 1); this->+9 = (u8)a1; *this = vtable 0x00B21010
DECOMP(004320B0, VtB21010_Ctor) { TCtorFlagVt(ctx, 0x0042C9D0, 1, 0x00B21010); }
// 0x0042C9D0(this, 1); this->+9 = (u8)a1; *this = vtable 0x00B21028
DECOMP(00432134, VtB21028_Ctor) { TCtorFlagVt(ctx, 0x0042C9D0, 1, 0x00B21028); }
// 0x0042C9D0(this, 0); this->+9 = (u8)a1; *this = vtable 0x00B211C0
DECOMP(00437C0C, VtB211C0_Ctor) { TCtorFlagVt(ctx, 0x0042C9D0, 0, 0x00B211C0); }
// 0x0042C9D0(this, 1); this->+9 = (u8)a1; *this = vtable 0x00B21238
DECOMP(00438508, VtB21238_Ctor) { TCtorFlagVt(ctx, 0x0042C9D0, 1, 0x00B21238); }
// 0x0042C9D0(this, 1); this->+9 = (u8)a1; *this = vtable 0x00B21250
DECOMP(0043859C, VtB21250_Ctor) { TCtorFlagVt(ctx, 0x0042C9D0, 1, 0x00B21250); }
// 0x0042C9D0(this, 1); this->+9 = (u8)a1; *this = vtable 0x00B21268
DECOMP(00438630, VtB21268_Ctor) { TCtorFlagVt(ctx, 0x0042C9D0, 1, 0x00B21268); }
// 0x0042C9D0(this, 0); this->+9 = (u8)a1; *this = vtable 0x00B213D0
DECOMP(0043A7BC, VtB213D0_Ctor) { TCtorFlagVt(ctx, 0x0042C9D0, 0, 0x00B213D0); }

// ---------------------------------------------------------------- deleting destructors: the vtable, a member's destructor, the free
// deleting: vtable 0x00B0DC70; 0x0062D08C(this + 216); 0x0054EE6C(this, 0)
DECOMP(0073839C, VtB0DC70_Delete) { TDeleteMember(ctx, 0x00B0DC70, 216, 0x0062D08C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DC70; 0x0062D08C(this + 216); 0x0054EE6C(this, 0)
DECOMP(007389D0, VtB0DC70_Delete_007389D0) { TDeleteMember(ctx, 0x00B0DC70, 216, 0x0062D08C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DC70; 0x0062D08C(this + 216); 0x0054EE6C(this, 0)
DECOMP(00738C44, VtB0DC70_Delete_00738C44) { TDeleteMember(ctx, 0x00B0DC70, 216, 0x0062D08C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DC70; 0x0062D08C(this + 216); 0x0054EE6C(this, 0)
DECOMP(00738D48, VtB0DC70_Delete_00738D48) { TDeleteMember(ctx, 0x00B0DC70, 216, 0x0062D08C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DC70; 0x0062D08C(this + 216); 0x0054EE6C(this, 0)
DECOMP(0073924C, VtB0DC70_Delete_0073924C) { TDeleteMember(ctx, 0x00B0DC70, 216, 0x0062D08C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DC70; 0x0062D08C(this + 216); 0x0054EE6C(this, 0)
DECOMP(0073965C, VtB0DC70_Delete_0073965C) { TDeleteMember(ctx, 0x00B0DC70, 216, 0x0062D08C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DC70; 0x0062D08C(this + 216); 0x0054EE6C(this, 0)
DECOMP(00739700, VtB0DC70_Delete_00739700) { TDeleteMember(ctx, 0x00B0DC70, 216, 0x0062D08C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DC70; 0x0062D08C(this + 216); 0x0054EE6C(this, 0)
DECOMP(007397D0, VtB0DC70_Delete_007397D0) { TDeleteMember(ctx, 0x00B0DC70, 216, 0x0062D08C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DC70; 0x0062D08C(this + 216); 0x0054EE6C(this, 0)
DECOMP(007398B0, VtB0DC70_Delete_007398B0) { TDeleteMember(ctx, 0x00B0DC70, 216, 0x0062D08C, 0x0054EE6C, 0); }

// ---------------------------------------------------------------- SmallAlloc, T(p, this->+12 byte), then a registration
// p = SmallAlloc(12); if p: 0x00431FA8(p, this->+12); returns 0x0042C138(p)
DECOMP(0009FA7C, NewReg_0009FA7C) { TNewByteReg(ctx, 12, 0x00431FA8, 12, 0x0042C138); }
// p = SmallAlloc(12); if p: 0x0043202C(p, this->+12); returns 0x0042C138(p)
DECOMP(0009FBE0, NewReg_0009FBE0) { TNewByteReg(ctx, 12, 0x0043202C, 12, 0x0042C138); }
// p = SmallAlloc(12); if p: 0x004320B0(p, this->+12); returns 0x0042C138(p)
DECOMP(0009FD3C, NewReg_0009FD3C) { TNewByteReg(ctx, 12, 0x004320B0, 12, 0x0042C138); }
// p = SmallAlloc(12); if p: 0x00432134(p, this->+12); returns 0x0042C138(p)
DECOMP(0009FE98, NewReg_0009FE98) { TNewByteReg(ctx, 12, 0x00432134, 12, 0x0042C138); }
// p = SmallAlloc(12); if p: 0x00437C0C(p, this->+12); returns 0x0042C138(p)
DECOMP(000A3ADC, NewReg_000A3ADC) { TNewByteReg(ctx, 12, 0x00437C0C, 12, 0x0042C138); }
// p = SmallAlloc(12); if p: 0x00438508(p, this->+12); returns 0x0042C138(p)
DECOMP(000A452C, NewReg_000A452C) { TNewByteReg(ctx, 12, 0x00438508, 12, 0x0042C138); }
// p = SmallAlloc(12); if p: 0x0043859C(p, this->+12); returns 0x0042C138(p)
DECOMP(000A4698, NewReg_000A4698) { TNewByteReg(ctx, 12, 0x0043859C, 12, 0x0042C138); }
// p = SmallAlloc(12); if p: 0x00438630(p, this->+12); returns 0x0042C138(p)
DECOMP(000A4804, NewReg_000A4804) { TNewByteReg(ctx, 12, 0x00438630, 12, 0x0042C138); }
// p = SmallAlloc(12); if p: 0x0043A7BC(p, this->+12); returns 0x0042C138(p)
DECOMP(000A6F4C, NewReg_000A6F4C) { TNewByteReg(ctx, 12, 0x0043A7BC, 12, 0x0042C138); }

// ---------------------------------------------------------------- key claims (String_HashNoCase of the name)
// (this, Slot* s) -> 0 and s->+4 = this when this->+204 (or 0x0056A710(vfn[3]())) is s->+0
DECOMP(00099FE0, ClaimKey_00099FE0) { TClaimKey(ctx, 0x0056A710); }
// (this, Slot* s) -> 0 and s->+4 = this when this->+204 (or 0x0056A710(vfn[3]())) is s->+0
DECOMP(0009A074, ClaimKey_0009A074) { TClaimKey(ctx, 0x0056A710); }
// (this, Slot* s) -> 0 and s->+4 = this when this->+204 (or 0x0056A710(vfn[3]())) is s->+0
DECOMP(000F8528, ClaimKey_000F8528) { TClaimKey(ctx, 0x0056A710); }
// (this, Slot* s) -> 0 and s->+4 = this when this->+204 (or 0x0056A710(vfn[3]())) is s->+0
DECOMP(000F85BC, ClaimKey_000F85BC) { TClaimKey(ctx, 0x0056A710); }
// (this, Slot* s) -> 0 and s->+4 = this when this->+204 (or 0x0056A710(vfn[3]())) is s->+0
DECOMP(002F5D6C, ClaimKey_002F5D6C) { TClaimKey(ctx, 0x0056A710); }
// (this, Slot* s) -> 0 and s->+4 = this when this->+204 (or 0x0056A710(vfn[3]())) is s->+0
DECOMP(0043B02C, ClaimKey_0043B02C) { TClaimKey(ctx, 0x0056A710); }
// (this, Slot* s) -> 0 and s->+4 = this when this->+204 (or 0x0056A710(vfn[3]())) is s->+0
DECOMP(004AFCB4, ClaimKey_004AFCB4) { TClaimKey(ctx, 0x0056A710); }
// (this, Slot* s) -> 0 and s->+4 = this when this->+204 (or 0x0056A710(vfn[3]())) is s->+0
DECOMP(004D0ABC, ClaimKey_004D0ABC) { TClaimKey(ctx, 0x0056A710); }
// (this, Slot* s) -> 0 and s->+4 = this when this->+204 (or 0x0056A710(vfn[3]())) is s->+0
DECOMP(004D0B50, ClaimKey_004D0B50) { TClaimKey(ctx, 0x0056A710); }

// ---------------------------------------------------------------- u32 getters: this->+12[current index]
// ((u32*)this->+12)[(*(0x00F94A54 +16))->+64]
DECOMP(00734114, GetU32ByIndex_00734114) { TU32ByIndex(ctx, 0x00F94A54); }
// ((u32*)this->+12)[(*(0x00F94D0C +16))->+64]
DECOMP(00734C38, GetU32ByIndex_00734C38) { TU32ByIndex(ctx, 0x00F94D0C); }
// ((u32*)this->+12)[(*(0x00F94D24 +16))->+64]
DECOMP(00734C58, GetU32ByIndex_00734C58) { TU32ByIndex(ctx, 0x00F94D24); }
// ((u32*)this->+12)[(*(0x00F94D3C +16))->+64]
DECOMP(00734FE0, GetU32ByIndex_00734FE0) { TU32ByIndex(ctx, 0x00F94D3C); }
// ((u32*)this->+12)[(*(0x00F94CDC +16))->+64]
DECOMP(00735030, GetU32ByIndex_00735030) { TU32ByIndex(ctx, 0x00F94CDC); }
// ((u32*)this->+12)[(*(0x00F94CF4 +16))->+64]
DECOMP(00735050, GetU32ByIndex_00735050) { TU32ByIndex(ctx, 0x00F94CF4); }
// ((u32*)this->+12)[(*(0x00F94D54 +16))->+64]
DECOMP(00735290, GetU32ByIndex_00735290) { TU32ByIndex(ctx, 0x00F94D54); }
// ((u32*)this->+12)[(*(0x00F94C94 +16))->+64]
DECOMP(00735340, GetU32ByIndex_00735340) { TU32ByIndex(ctx, 0x00F94C94); }

// ---------------------------------------------------------------- byte getters: this->+12[current index * 4]
// ((u8*)this->+12)[(*(0x00F94A84 +16))->+64 * 4]
DECOMP(00734198, GetU8ByIndex_00734198) { TU8ByIndex(ctx, 0x00F94A84); }
// ((u8*)this->+12)[(*(0x00F94814 +16))->+64 * 4]
DECOMP(00734270, GetU8ByIndex_00734270) { TU8ByIndex(ctx, 0x00F94814); }
// ((u8*)this->+12)[(*(0x00F94BEC +16))->+64 * 4]
DECOMP(007349E0, GetU8ByIndex_007349E0) { TU8ByIndex(ctx, 0x00F94BEC); }
// ((u8*)this->+12)[(*(0x00F94B74 +16))->+64 * 4]
DECOMP(00734A00, GetU8ByIndex_00734A00) { TU8ByIndex(ctx, 0x00F94B74); }
// ((u8*)this->+12)[(*(0x00F9503C +16))->+64 * 4]
DECOMP(00734BD8, GetU8ByIndex_00734BD8) { TU8ByIndex(ctx, 0x00F9503C); }
// ((u8*)this->+12)[(*(0x00F94C04 +16))->+64 * 4]
DECOMP(00734C78, GetU8ByIndex_00734C78) { TU8ByIndex(ctx, 0x00F94C04); }
// ((u8*)this->+12)[(*(0x00F951A4 +16))->+64 * 4]
DECOMP(00734C98, GetU8ByIndex_00734C98) { TU8ByIndex(ctx, 0x00F951A4); }
// ((u8*)this->+12)[(*(0x00F95024 +16))->+64 * 4]
DECOMP(00735270, GetU8ByIndex_00735270) { TU8ByIndex(ctx, 0x00F95024); }

// ---------------------------------------------------------------- wrappers (b)
// wrapper of 0x000DD1CC (the same arguments)
DECOMP(001946F0, Thunk_001946F0) { TThunk(ctx, 0x000DD1CC); }
// wrapper of 0x000E7DC4 (the same arguments)
DECOMP(001EB078, Thunk_001EB078) { TThunk(ctx, 0x000E7DC4); }
// wrapper of 0x000D951C (the same arguments)
DECOMP(001F67B4, Thunk_001F67B4) { TThunk(ctx, 0x000D951C); }
// wrapper of 0x000D951C (the same arguments)
DECOMP(001F67D8, Thunk_001F67D8) { TThunk(ctx, 0x000D951C); }
// wrapper of 0x000E5000 (the same arguments)
DECOMP(003011B8, Thunk_003011B8) { TThunk(ctx, 0x000E5000); }
// wrapper of 0x000E5000 (the same arguments)
DECOMP(0030FB94, Thunk_0030FB94) { TThunk(ctx, 0x000E5000); }
// wrapper of 0x00518E78 (the same arguments)
DECOMP(0048C874, Thunk_0048C874) { TThunk(ctx, 0x00518E78); }
// wrapper of 0x0051A918 (the same arguments)
DECOMP(0048C91C, Thunk_0048C91C) { TThunk(ctx, 0x0051A918); }

// ---------------------------------------------------------------- SmallAlloc, T(p, owner, this + off, hash, this), then attach(owner, p)
// p = SmallAlloc(384); if p: 0x003F834C(p, this->+8, this + 2580, 0x7719A548, this); returns 0x0047C530(this->+8, p)
DECOMP(0032A9C4, NewAttach_0032A9C4) { TNewAttach(ctx, 384, 0x003F834C, 2580, 0x7719A548, 0x0047C530); }
// p = SmallAlloc(176); if p: 0x003B3148(p, this->+8, this + 2592, 0x7719A548, this); returns 0x0047C530(this->+8, p)
DECOMP(00331590, NewAttach_00331590) { TNewAttach(ctx, 176, 0x003B3148, 2592, 0x7719A548, 0x0047C530); }
// p = SmallAlloc(256); if p: 0x004015D0(p, this->+8, this + 2580, 0x7719A548, this); returns 0x0047C530(this->+8, p)
DECOMP(003356A8, NewAttach_003356A8) { TNewAttach(ctx, 256, 0x004015D0, 2580, 0x7719A548, 0x0047C530); }
// p = SmallAlloc(256); if p: 0x0040773C(p, this->+8, this + 2580, 0x7719A548, this); returns 0x0047C530(this->+8, p)
DECOMP(00335B14, NewAttach_00335B14) { TNewAttach(ctx, 256, 0x0040773C, 2580, 0x7719A548, 0x0047C530); }
// p = SmallAlloc(496); if p: 0x0040F244(p, this->+8, this + 2580, 0x7719A548, this); returns 0x0047C530(this->+8, p)
DECOMP(00336060, NewAttach_00336060) { TNewAttach(ctx, 496, 0x0040F244, 2580, 0x7719A548, 0x0047C530); }
// p = SmallAlloc(448); if p: 0x00416FA4(p, this->+8, this + 2580, 0x7719A548, this); returns 0x0047C530(this->+8, p)
DECOMP(003365C4, NewAttach_003365C4) { TNewAttach(ctx, 448, 0x00416FA4, 2580, 0x7719A548, 0x0047C530); }
// p = SmallAlloc(512); if p: 0x0041BB54(p, this->+8, this + 2580, 0x7719A548, this); returns 0x0047C530(this->+8, p)
DECOMP(00339128, NewAttach_00339128) { TNewAttach(ctx, 512, 0x0041BB54, 2580, 0x7719A548, 0x0047C530); }
// p = SmallAlloc(120); if p: 0x0041F1FC(p, this->+8, this + 2580, 0x7719A548, this); returns 0x0047C530(this->+8, p)
DECOMP(00339B14, NewAttach_00339B14) { TNewAttach(ctx, 120, 0x0041F1FC, 2580, 0x7719A548, 0x0047C530); }
// p = SmallAlloc(512); if p: 0x003D3704(p, this->+8, this + 2592, 0x7719A548, this); returns 0x0047C530(this->+8, p)
DECOMP(0033B91C, NewAttach_0033B91C) { TNewAttach(ctx, 512, 0x003D3704, 2592, 0x7719A548, 0x0047C530); }

// ---------------------------------------------------------------- XML-bound properties (XmlCursor_ReadHash, u32 at +12 = 0)
// p = Mem_Alloc(16, 0, 16); 0x004BB3B0(p, a1); vtable 0x00B0F900; 0x004FA14C(a1, 0x7D385CF6, &p->+12, 0)
DECOMP(0007AB80, VtB0F900_New) { TNewPropU32(ctx, 0x004BB3B0, 0x00B0F900, 0x7D385CF6, 0x004FA14C); }
// p = Mem_Alloc(16, 0, 16); 0x004BB3B0(p, a1); vtable 0x00B0F978; 0x004FA14C(a1, 0x7D385CF6, &p->+12, 0)
DECOMP(0007B2E8, VtB0F978_New) { TNewPropU32(ctx, 0x004BB3B0, 0x00B0F978, 0x7D385CF6, 0x004FA14C); }
// p = Mem_Alloc(16, 0, 16); 0x004BB3B0(p, a1); vtable 0x00B0F9C8; 0x004FA14C(a1, 0x7D385CF6, &p->+12, 0)
DECOMP(0007B830, VtB0F9C8_New) { TNewPropU32(ctx, 0x004BB3B0, 0x00B0F9C8, 0x7D385CF6, 0x004FA14C); }
// p = Mem_Alloc(16, 0, 16); 0x004BB3B0(p, a1); vtable 0x00B0FA40; 0x004FA14C(a1, 0x7D385CF6, &p->+12, 0)
DECOMP(0007C368, VtB0FA40_New) { TNewPropU32(ctx, 0x004BB3B0, 0x00B0FA40, 0x7D385CF6, 0x004FA14C); }
// p = Mem_Alloc(16, 0, 16); 0x004BB3B0(p, a1); vtable 0x00B0FA68; 0x004FA14C(a1, 0x7D385CF6, &p->+12, 0)
DECOMP(0007C5CC, VtB0FA68_New) { TNewPropU32(ctx, 0x004BB3B0, 0x00B0FA68, 0x7D385CF6, 0x004FA14C); }
// p = Mem_Alloc(16, 0, 16); 0x004BB3B0(p, a1); vtable 0x00B0FD10; 0x004FA14C(a1, 0x0024728B, &p->+12, 0)
DECOMP(00080AD0, VtB0FD10_New) { TNewPropU32(ctx, 0x004BB3B0, 0x00B0FD10, 0x0024728B, 0x004FA14C); }
// p = Mem_Alloc(16, 0, 16); 0x004BB3B0(p, a1); vtable 0x00B0FD38; 0x004FA14C(a1, 0x044FA364, &p->+12, 0)
DECOMP(00080BE4, VtB0FD38_New) { TNewPropU32(ctx, 0x004BB3B0, 0x00B0FD38, 0x044FA364, 0x004FA14C); }
// p = Mem_Alloc(16, 0, 16); 0x004BB3B0(p, a1); vtable 0x00B10210; 0x004FA14C(a1, 0x0027873A, &p->+12, 0)
DECOMP(000874E4, VtB10210_New) { TNewPropU32(ctx, 0x004BB3B0, 0x00B10210, 0x0027873A, 0x004FA14C); }
// p = Mem_Alloc(16, 0, 16); 0x004BB3B0(p, a1); vtable 0x00B10DF0; 0x004FA14C(a1, 0x0024728B, &p->+12, 0)
DECOMP(00099764, VtB10DF0_New) { TNewPropU32(ctx, 0x004BB3B0, 0x00B10DF0, 0x0024728B, 0x004FA14C); }

// ---------------------------------------------------------------- XML-bound properties (XmlCursor_ReadBool, u8 at +12 = 1)
// p = Mem_Alloc(16, 0, 16); 0x004BB3B0(p, a1); vtable 0x00B0F540; 0x004F986C(a1, 0x7A281983, &p->+12, 1)
DECOMP(000734E0, VtB0F540_New) { TNewPropU8(ctx, 0x004BB3B0, 0x00B0F540, 0x7A281983, 0x004F986C); }
// p = Mem_Alloc(16, 0, 16); 0x004BB3B0(p, a1); vtable 0x00B0FE28; 0x004F986C(a1, 0x04862DD6, &p->+12, 1)
DECOMP(00081F60, VtB0FE28_New) { TNewPropU8(ctx, 0x004BB3B0, 0x00B0FE28, 0x04862DD6, 0x004F986C); }
// p = Mem_Alloc(16, 0, 16); 0x004BB3B0(p, a1); vtable 0x00B10120; 0x004F986C(a1, 0x03B7AB29, &p->+12, 1)
DECOMP(00086970, VtB10120_New) { TNewPropU8(ctx, 0x004BB3B0, 0x00B10120, 0x03B7AB29, 0x004F986C); }
// p = Mem_Alloc(16, 0, 16); 0x004BB3B0(p, a1); vtable 0x00B11168; 0x004F986C(a1, 0x2247101A, &p->+12, 1)
DECOMP(0009CA98, VtB11168_New) { TNewPropU8(ctx, 0x004BB3B0, 0x00B11168, 0x2247101A, 0x004F986C); }
// p = Mem_Alloc(16, 0, 16); 0x004BB3B0(p, a1); vtable 0x00B11410; 0x004F986C(a1, 0x04862DD6, &p->+12, 1)
DECOMP(0009F9D4, VtB11410_New) { TNewPropU8(ctx, 0x004BB3B0, 0x00B11410, 0x04862DD6, 0x004F986C); }
// p = Mem_Alloc(16, 0, 16); 0x004BB3B0(p, a1); vtable 0x00B11438; 0x004F986C(a1, 0x04862DD6, &p->+12, 1)
DECOMP(0009FB38, VtB11438_New) { TNewPropU8(ctx, 0x004BB3B0, 0x00B11438, 0x04862DD6, 0x004F986C); }
// p = Mem_Alloc(16, 0, 16); 0x004BB3B0(p, a1); vtable 0x00B11460; 0x004F986C(a1, 0x04862DD6, &p->+12, 1)
DECOMP(0009FC94, VtB11460_New) { TNewPropU8(ctx, 0x004BB3B0, 0x00B11460, 0x04862DD6, 0x004F986C); }
// p = Mem_Alloc(16, 0, 16); 0x004BB3B0(p, a1); vtable 0x00B11488; 0x004F986C(a1, 0x04862DD6, &p->+12, 1)
DECOMP(0009FDF0, VtB11488_New) { TNewPropU8(ctx, 0x004BB3B0, 0x00B11488, 0x04862DD6, 0x004F986C); }
// p = Mem_Alloc(16, 0, 16); 0x004BB3B0(p, a1); vtable 0x00B117D8; 0x004F986C(a1, 0x23D89F8B, &p->+12, 1)
DECOMP(000A4484, VtB117D8_New) { TNewPropU8(ctx, 0x004BB3B0, 0x00B117D8, 0x23D89F8B, 0x004F986C); }

// ---------------------------------------------------------------- destructors: three vtables (+0 / +380 / +384), then the base
// vtables 0x00B130E8 / 0x00B13288 / 0x00B132B0 at +0 / +380 / +384, then 0x000EB0F8(this)
DECOMP(000F2B64, VtB130E8_Dtor) { TAdjDtorVt3(ctx, 0, 0x00B130E8, 380, 0x00B13288, 384, 0x00B132B0, 0x000EB0F8); }
// vtables 0x00B13308 / 0x00B134A8 / 0x00B134D0 at +0 / +380 / +384, then 0x000EB0F8(this)
DECOMP(000F3E54, VtB13308_Dtor) { TAdjDtorVt3(ctx, 0, 0x00B13308, 380, 0x00B134A8, 384, 0x00B134D0, 0x000EB0F8); }
// vtables 0x00B12EC8 / 0x00B13068 / 0x00B13090 at +0 / +380 / +384, then 0x000EB0F8(this)
DECOMP(007482B8, VtB12EC8_Dtor) { TAdjDtorVt3(ctx, 0, 0x00B12EC8, 380, 0x00B13068, 384, 0x00B13090, 0x000EB0F8); }
// vtables 0x00B16678 / 0x00B1684C / 0x00B16874 at +0 / +380 / +384, then 0x001AE468(this)
DECOMP(0075420C, VtB16678_Dtor) { TAdjDtorVt3(ctx, 0, 0x00B16678, 380, 0x00B1684C, 384, 0x00B16874, 0x001AE468); }
// vtables 0x00B168E0 / 0x00B16AB4 / 0x00B16ADC at +0 / +380 / +384, then 0x001AE468(this)
DECOMP(0075458C, VtB168E0_Dtor) { TAdjDtorVt3(ctx, 0, 0x00B168E0, 380, 0x00B16AB4, 384, 0x00B16ADC, 0x001AE468); }
// vtables 0x00B16DB0 / 0x00B16F84 / 0x00B16FAC at +0 / +380 / +384, then 0x001AE468(this)
DECOMP(00754B74, VtB16DB0_Dtor) { TAdjDtorVt3(ctx, 0, 0x00B16DB0, 380, 0x00B16F84, 384, 0x00B16FAC, 0x001AE468); }
// vtables 0x00B17008 / 0x00B171DC / 0x00B17204 at +0 / +380 / +384, then 0x001AE468(this)
DECOMP(00754E18, VtB17008_Dtor) { TAdjDtorVt3(ctx, 0, 0x00B17008, 380, 0x00B171DC, 384, 0x00B17204, 0x001AE468); }
// vtables 0x00B17728 / 0x00B178FC / 0x00B17924 at +0 / +380 / +384, then 0x001AE468(this)
DECOMP(00755374, VtB17728_Dtor) { TAdjDtorVt3(ctx, 0, 0x00B17728, 380, 0x00B178FC, 384, 0x00B17924, 0x001AE468); }

// ---------------------------------------------------------------- binary tree lookups
// (?, key, u32* out, Node* n) -> bool; *out = n->+8
DECOMP(0073AF5C, TreeFind_0073AF5C) { TTreeFind(ctx); }
// (?, key, u32* out, Node* n) -> bool; *out = n->+8
DECOMP(0076D014, TreeFind_0076D014) { TTreeFind(ctx); }
// (?, key, u32* out, Node* n) -> bool; *out = n->+8
DECOMP(0078B1B0, TreeFind_0078B1B0) { TTreeFind(ctx); }
// (?, key, u32* out, Node* n) -> bool; *out = n->+8
DECOMP(0078C110, TreeFind_0078C110) { TTreeFind(ctx); }
// (?, key, u32* out, Node* n) -> bool; *out = n->+8
DECOMP(0078D300, TreeFind_0078D300) { TTreeFind(ctx); }
// (?, key, u32* out, Node* n) -> bool; *out = n->+8
DECOMP(0079187C, TreeFind_0079187C) { TTreeFind(ctx); }
// (?, key, u32* out, Node* n) -> bool; *out = n->+8
DECOMP(00797C08, TreeFind_00797C08) { TTreeFind(ctx); }
// (?, key, u32* out, Node* n) -> bool; *out = n->+8
DECOMP(00798DF8, TreeFind_00798DF8) { TTreeFind(ctx); }

// ---------------------------------------------------------------- deleting destructors: three vtables
// deleting: vtables 0x00B130E8 / 0x00B13288 / 0x00B132B0, 0x000EB0F8(this), 0x00552420(this, 752)
DECOMP(000F2BA4, VtB130E8_Delete) { TAdjDeleteVt3(ctx, 0, 0x00B130E8, 380, 0x00B13288, 384, 0x00B132B0, 0x000EB0F8, 0x00552420, 752); }
// deleting: vtables 0x00B13308 / 0x00B134A8 / 0x00B134D0, 0x000EB0F8(this), 0x00552420(this, 688)
DECOMP(000F3E94, VtB13308_Delete) { TAdjDeleteVt3(ctx, 0, 0x00B13308, 380, 0x00B134A8, 384, 0x00B134D0, 0x000EB0F8, 0x00552420, 688); }
// deleting: vtables 0x00B12EC8 / 0x00B13068 / 0x00B13090, 0x000EB0F8(this), 0x00552420(this, 640)
DECOMP(007482F8, VtB12EC8_Delete) { TAdjDeleteVt3(ctx, 0, 0x00B12EC8, 380, 0x00B13068, 384, 0x00B13090, 0x000EB0F8, 0x00552420, 640); }
// deleting: vtables 0x00B16678 / 0x00B1684C / 0x00B16874, 0x001AE468(this), 0x00552420(this, 576)
DECOMP(0075424C, VtB16678_Delete) { TAdjDeleteVt3(ctx, 0, 0x00B16678, 380, 0x00B1684C, 384, 0x00B16874, 0x001AE468, 0x00552420, 576); }
// deleting: vtables 0x00B168E0 / 0x00B16AB4 / 0x00B16ADC, 0x001AE468(this), 0x00552420(this, 592)
DECOMP(007545CC, VtB168E0_Delete) { TAdjDeleteVt3(ctx, 0, 0x00B168E0, 380, 0x00B16AB4, 384, 0x00B16ADC, 0x001AE468, 0x00552420, 592); }
// deleting: vtables 0x00B16DB0 / 0x00B16F84 / 0x00B16FAC, 0x001AE468(this), 0x00552420(this, 704)
DECOMP(00754BB4, VtB16DB0_Delete) { TAdjDeleteVt3(ctx, 0, 0x00B16DB0, 380, 0x00B16F84, 384, 0x00B16FAC, 0x001AE468, 0x00552420, 704); }
// deleting: vtables 0x00B17008 / 0x00B171DC / 0x00B17204, 0x001AE468(this), 0x00552420(this, 656)
DECOMP(00754E58, VtB17008_Delete) { TAdjDeleteVt3(ctx, 0, 0x00B17008, 380, 0x00B171DC, 384, 0x00B17204, 0x001AE468, 0x00552420, 656); }
// deleting: vtables 0x00B17728 / 0x00B178FC / 0x00B17924, 0x001AE468(this), 0x00552420(this, 576)
DECOMP(007553B4, VtB17728_Delete) { TAdjDeleteVt3(ctx, 0, 0x00B17728, 380, 0x00B178FC, 384, 0x00B17924, 0x001AE468, 0x00552420, 576); }

// ---------------------------------------------------------------- field getters
// returns this->+388
DECOMP(00748238, GetField388_00748238) { TLoadU32(ctx, 388); }
// returns this->+388
DECOMP(0074EF8C, GetField388_0074EF8C) { TLoadU32(ctx, 388); }
// returns this->+388
DECOMP(0074F188, GetField388_0074F188) { TLoadU32(ctx, 388); }
// returns this->+388
DECOMP(0074F2DC, GetField388_0074F2DC) { TLoadU32(ctx, 388); }
// returns this->+388
DECOMP(0074F460, GetField388_0074F460) { TLoadU32(ctx, 388); }
// returns this->+388
DECOMP(0074F5F4, GetField388_0074F5F4) { TLoadU32(ctx, 388); }
// returns this->+388
DECOMP(0075923C, GetField388_0075923C) { TLoadU32(ctx, 388); }

// ---------------------------------------------------------------- u32 array push_back
// (Array* a, const u32* v): grow (0x00551718) when full, store, count / high-water mark
DECOMP(0074E8DC, PushU32_0074E8DC) { TPushU32(ctx, 0x00551718); }
// (Array* a, const u32* v): grow (0x00551718) when full, store, count / high-water mark
DECOMP(00761428, PushU32_00761428) { TPushU32(ctx, 0x00551718); }
// (Array* a, const u32* v): grow (0x00551718) when full, store, count / high-water mark
DECOMP(00767F94, PushU32_00767F94) { TPushU32(ctx, 0x00551718); }
// (Array* a, const u32* v): grow (0x00551718) when full, store, count / high-water mark
DECOMP(00768020, PushU32_00768020) { TPushU32(ctx, 0x00551718); }
// (Array* a, const u32* v): grow (0x00551718) when full, store, count / high-water mark
DECOMP(007680AC, PushU32_007680AC) { TPushU32(ctx, 0x00551718); }
// (Array* a, const u32* v): grow (0x00551718) when full, store, count / high-water mark
DECOMP(00768138, PushU32_00768138) { TPushU32(ctx, 0x00551718); }
// (Array* a, const u32* v): grow (0x00551718) when full, store, count / high-water mark
DECOMP(0077214C, PushU32_0077214C) { TPushU32(ctx, 0x00551718); }

// ---------------------------------------------------------------- Vec3 copies from a global
// dst[0..2] = ((f32*)0x01164B40)[0..2]
DECOMP(007436E0, Vec3_CopyGlobal_007436E0) { TCopyGlobalVec3(ctx, 0x01164B40); }
// dst[0..2] = ((f32*)0x01164B40)[0..2]
DECOMP(00743778, Vec3_CopyGlobal_00743778) { TCopyGlobalVec3(ctx, 0x01164B40); }
// dst[0..2] = ((f32*)0x01164B40)[0..2]
DECOMP(0074379C, Vec3_CopyGlobal_0074379C) { TCopyGlobalVec3(ctx, 0x01164B40); }
// dst[0..2] = ((f32*)0x01164B40)[0..2]
DECOMP(00748528, Vec3_CopyGlobal_00748528) { TCopyGlobalVec3(ctx, 0x01164B40); }
// dst[0..2] = ((f32*)0x01164B40)[0..2]
DECOMP(0075D64C, Vec3_CopyGlobal_0075D64C) { TCopyGlobalVec3(ctx, 0x01164B40); }

// ---------------------------------------------------------------- float getters: (this->+216)->+232
// f1 = (this->+216)->+232
DECOMP(001691C8, GetF32_001691C8) { TF32Chain(ctx, 216, 232); }
// f1 = (this->+216)->+232
DECOMP(00170630, GetF32_00170630) { TF32Chain(ctx, 216, 232); }
// f1 = (this->+216)->+232
DECOMP(00185ABC, GetF32_00185ABC) { TF32Chain(ctx, 216, 232); }
// f1 = (this->+216)->+232
DECOMP(001931B4, GetF32_001931B4) { TF32Chain(ctx, 216, 232); }

// ---------------------------------------------------------------- constant address returns
// returns 0x00AE4674
DECOMP(004440E8, ReturnK_004440E8) { TRetU32(ctx, 0x00AE4674); }
// returns 0x00AE4674
DECOMP(004440F4, ReturnK_004440F4) { TRetU32(ctx, 0x00AE4674); }
// returns 0x00AE6AC0
DECOMP(00467C4C, ReturnK_00467C4C) { TRetU32(ctx, 0x00AE6AC0); }
// returns 0x00AE6ACC
DECOMP(0048A680, ReturnK_0048A680) { TRetU32(ctx, 0x00AE6ACC); }

// ---------------------------------------------------------------- field copies: dst[0..n) = src->+off[0..n)
// dst[0..1] = src->+168[0..1]
DECOMP(007960B4, CopyFields_007960B4) { TCopyFields(ctx, 168, 2); }
// dst[0..1] = src->+168[0..1]
DECOMP(007963F4, CopyFields_007963F4) { TCopyFields(ctx, 168, 2); }
// dst[0..1] = src->+168[0..1]
DECOMP(00796698, CopyFields_00796698) { TCopyFields(ctx, 168, 2); }
// dst[0..1] = src->+168[0..1]
DECOMP(00796894, CopyFields_00796894) { TCopyFields(ctx, 168, 2); }

// ---------------------------------------------------------------- field copies (Vec3)
// dst[0..2] = src[0..2]
DECOMP(00734D90, Vec3_Copy_00734D90) { TCopyFields(ctx, 0, 3); }
// dst[0..2] = src[0..2]
DECOMP(0074D080, Vec3_Copy_0074D080) { TCopyFields(ctx, 0, 3); }
// dst[0..2] = src[0..2]
DECOMP(0074DBEC, Vec3_Copy_0074DBEC) { TCopyFields(ctx, 0, 3); }

// ---------------------------------------------------------------- field copies (Vec3 at +416)
// dst[0..2] = src->+416[0..2]
DECOMP(0074EF70, CopyFields_0074EF70) { TCopyFields(ctx, 416, 3); }
// dst[0..2] = src->+416[0..2]
DECOMP(0074F444, CopyFields_0074F444) { TCopyFields(ctx, 416, 3); }
// dst[0..2] = src->+416[0..2]
DECOMP(0074F5C0, CopyFields_0074F5C0) { TCopyFields(ctx, 416, 3); }

// ---------------------------------------------------------------- vtable stores of a five-base object (through a secondary base)
// p = this -20: vtables at +12 / +20 / +8 / +16 / +0
DECOMP(00771224, AdjDtor_00771224) { TAdjSetVt5(ctx, -20, 0x00B1F250, 0x00B1F23C, 0x00B1F228, 0x00B1F26C, 0x00B13974); }
// p = this -16: vtables at +12 / +20 / +8 / +16 / +0
DECOMP(0077126C, AdjDtor_0077126C) { TAdjSetVt5(ctx, -16, 0x00B1F250, 0x00B1F23C, 0x00B1F228, 0x00B1F26C, 0x00B13974); }
// p = this -12: vtables at +12 / +20 / +8 / +16 / +0
DECOMP(007712B4, AdjDtor_007712B4) { TAdjSetVt5(ctx, -12, 0x00B1F250, 0x00B1F23C, 0x00B1F228, 0x00B1F26C, 0x00B13974); }
// p = this -8: vtables at +12 / +20 / +8 / +16 / +0
DECOMP(007712FC, AdjDtor_007712FC) { TAdjSetVt5(ctx, -8, 0x00B1F250, 0x00B1F23C, 0x00B1F228, 0x00B1F26C, 0x00B13974); }

// ---------------------------------------------------------------- field getters (+12)
// returns this->+12
DECOMP(00749764, GetField12_00749764) { TLoadU32(ctx, 12); }
// returns this->+12
DECOMP(00749834, GetField12_00749834) { TLoadU32(ctx, 12); }
// returns this->+12
DECOMP(007499C8, GetField12_007499C8) { TLoadU32(ctx, 12); }

// ---------------------------------------------------------------- byte setters: this->+4 = k
// *(u8*)(this + 4) = 0
DECOMP(00749150, SetByte4_00749150) { TStoreU8At(ctx, 4, 0); }
// *(u8*)(this + 4) = 1
DECOMP(007887FC, SetByte4_007887FC) { TStoreU8At(ctx, 4, 1); }
// *(u8*)(this + 4) = 0
DECOMP(00788808, SetByte4_00788808) { TStoreU8At(ctx, 4, 0); }

// ---------------------------------------------------------------- flag sets: this->+424 |= k
// this->+424 |= 0x0000000F
DECOMP(003A4000, SetFlags424_003A4000) { TOrU32(ctx, 424, 0x0000000F); }
// this->+424 |= 0x00000002
DECOMP(003AAAE0, SetFlags424_003AAAE0) { TOrU32(ctx, 424, 0x00000002); }
// this->+424 |= 0x00000004
DECOMP(003AAAF0, SetFlags424_003AAAF0) { TOrU32(ctx, 424, 0x00000004); }

// ---------------------------------------------------------------- *(u8*)a1 = 0, returns a1
// *(u8*)a1 = 0; returns a1
DECOMP(00742AD0, ClearByteRet_00742AD0) { TClearByteRet(ctx); }
// *(u8*)a1 = 0; returns a1
DECOMP(00794058, ClearByteRet_00794058) { TClearByteRet(ctx); }
// *(u8*)a1 = 0; returns a1
DECOMP(00794080, ClearByteRet_00794080) { TClearByteRet(ctx); }

// ---------------------------------------------------------------- element addresses: this->+0 + i * 8
// &((u64*)this->+0)[a1]
DECOMP(00735A0C, ElemAddr8_00735A0C) { TIndexAddr(ctx, 3); }
// &((u64*)this->+0)[a1]
DECOMP(00755CA0, ElemAddr8_00755CA0) { TIndexAddr(ctx, 3); }
// &((u64*)this->+0)[a1]
DECOMP(0077D5F4, ElemAddr8_0077D5F4) { TIndexAddr(ctx, 3); }

// ---------------------------------------------------------------- keyed float setters
// if (this->+12 == a1) this->+4 = f1
DECOMP(00767AF8, SetF32IfKey_00767AF8) { TSetF32IfKey(ctx); }
// if (this->+12 == a1) this->+4 = f1
DECOMP(00767BB0, SetF32IfKey_00767BB0) { TSetF32IfKey(ctx); }
// if (this->+12 == a1) this->+4 = f1
DECOMP(00767C0C, SetF32IfKey_00767C0C) { TSetF32IfKey(ctx); }

// ---------------------------------------------------------------- conditional byte clears
// if ((u8)a1) this->+41 = 0
DECOMP(002FD580, ClearByte41If_002FD580) { TClearByteIf(ctx, 41); }
// if ((u8)a1) this->+41 = 0
DECOMP(002FD748, ClearByte41If_002FD748) { TClearByteIf(ctx, 41); }
// if ((u8)a1) this->+41 = 0
DECOMP(002FE584, ClearByte41If_002FE584) { TClearByteIf(ctx, 41); }

// ---------------------------------------------------------------- float array getters: ((f32*)(this + k))[i]
// ((f32*)(this +56))[a1]
DECOMP(0074BFF8, GetF32At_0074BFF8) { TF32At(ctx, 56); }
// ((f32*)(this +256))[a1]
DECOMP(00760B18, GetF32At_00760B18) { TF32At(ctx, 256); }
// ((f32*)(this +264))[a1]
DECOMP(00760B30, GetF32At_00760B30) { TF32At(ctx, 264); }

// ---------------------------------------------------------------- named nodes with one binding
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B1FBC8); bind 0x00B1FBD4
DECOMP(00776570, NewNamed_00776570) { TNewNamed(ctx, 0x00B1FBC8, {{0x00B1FBD4, 5, 0}}); }
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B1FBD9); bind 0x00B1FBE6
DECOMP(0077671C, NewNamed_0077671C) { TNewNamed(ctx, 0x00B1FBD9, {{0x00B1FBE6, 5, 0}}); }
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B1FC78); bind 0x00B1FC8A
DECOMP(00776D90, NewNamed_00776D90) { TNewNamed(ctx, 0x00B1FC78, {{0x00B1FC8A, 5, 0}}); }
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B1FD0D); bind 0x00B1FD22
DECOMP(0077750C, NewNamed_0077750C) { TNewNamed(ctx, 0x00B1FD0D, {{0x00B1FD22, 2, 0}}); }
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B1FDDC); bind 0x00B1FDE8
DECOMP(00777BAC, NewNamed_00777BAC) { TNewNamed(ctx, 0x00B1FDDC, {{0x00B1FDE8, 2, 0}}); }
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B1FE4A); bind 0x00B1FE56
DECOMP(00778154, NewNamed_00778154) { TNewNamed(ctx, 0x00B1FE4A, {{0x00B1FE56, 5, 0}}); }
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B1FE60); bind 0x00B1FE78
DECOMP(00778300, NewNamed_00778300) { TNewNamed(ctx, 0x00B1FE60, {{0x00B1FE78, 4, 0}}); }
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B1FE81); bind 0x00B1FE93
DECOMP(007784AC, NewNamed_007784AC) { TNewNamed(ctx, 0x00B1FE81, {{0x00B1FE93, 3, 0}}); }
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B1FE99); bind 0x00B1FEAD
DECOMP(00778658, NewNamed_00778658) { TNewNamed(ctx, 0x00B1FE99, {{0x00B1FEAD, 3, 0}}); }
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B1FECD); bind 0x00B1FED9
DECOMP(00778A40, NewNamed_00778A40) { TNewNamed(ctx, 0x00B1FECD, {{0x00B1FED9, 4, 0}}); }
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B2004F); bind 0x00B2005C
DECOMP(00779B04, NewNamed_00779B04) { TNewNamed(ctx, 0x00B2004F, {{0x00B2005C, 4, 1}}); }
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B20084); bind 0x00B20096
DECOMP(00779E90, NewNamed_00779E90) { TNewNamed(ctx, 0x00B20084, {{0x00B20096, 2, 1}}); }
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B200FE); bind 0x00B2010E
DECOMP(0077A2E0, NewNamed_0077A2E0) { TNewNamed(ctx, 0x00B200FE, {{0x00B2010E, 5, 1}}); }

// ---------------------------------------------------------------- named nodes with two bindings
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B1FBF0); bind 0x00B1FBFF, 0x00B1FC0B
DECOMP(00776840, NewNamed_00776840) { TNewNamed(ctx, 0x00B1FBF0, {{0x00B1FBFF, 5, 0}, {0x00B1FC0B, 0, 0}}); }
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B1FC11); bind 0x00B1FC22, 0x00B1FC2E
DECOMP(00776994, NewNamed_00776994) { TNewNamed(ctx, 0x00B1FC11, {{0x00B1FC22, 5, 0}, {0x00B1FC2E, 3, 0}}); }
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B1FC34); bind 0x00B1FC44, 0x00B1FC50
DECOMP(00776AE8, NewNamed_00776AE8) { TNewNamed(ctx, 0x00B1FC34, {{0x00B1FC44, 5, 0}, {0x00B1FC50, 3, 0}}); }
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B1FC56); bind 0x00B1FC66, 0x00B1FC72
DECOMP(00776C3C, NewNamed_00776C3C) { TNewNamed(ctx, 0x00B1FC56, {{0x00B1FC66, 5, 0}, {0x00B1FC72, 3, 0}}); }
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B1FCCA); bind 0x00B1FCD5, 0x00B1FCDF
DECOMP(00777124, NewNamed_00777124) { TNewNamed(ctx, 0x00B1FCCA, {{0x00B1FCD5, 2, 0}, {0x00B1FCDF, 2, 1}}); }
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B1FFD2); bind 0x00B1FFDA, 0x00B1FFDF
DECOMP(007793E4, NewNamed_007793E4) { TNewNamed(ctx, 0x00B1FFD2, {{0x00B1FFDA, 2, 0}, {0x00B1FFDF, 4, 0}}); }
// p = Mem_Alloc(24, 0, 16); Node_Init(p, id, 0x00B1FFE7); bind 0x00B1FFF8, 0x00B1FFFD
DECOMP(007795C0, NewNamed_007795C0) { TNewNamed(ctx, 0x00B1FFE7, {{0x00B1FFF8, 2, 0}, {0x00B1FFFD, 4, 0}}); }

// ---------------------------------------------------------------- virtual calls returning 1
// this->vfn[2](this); returns 1
DECOMP(00737FBC, VCall8Ret1_00737FBC) { TVCallRet1(ctx, 8); }
// this->vfn[2](this); returns 1
DECOMP(00738114, VCall8Ret1_00738114) { TVCallRet1(ctx, 8); }
// this->vfn[2](this); returns 1
DECOMP(007381C4, VCall8Ret1_007381C4) { TVCallRet1(ctx, 8); }
// this->vfn[2](this); returns 1
DECOMP(0073825C, VCall8Ret1_0073825C) { TVCallRet1(ctx, 8); }
// this->vfn[2](this); returns 1
DECOMP(0074EAB4, VCall8Ret1_0074EAB4) { TVCallRet1(ctx, 8); }
// this->vfn[2](this); returns 1
DECOMP(0074EB38, VCall8Ret1_0074EB38) { TVCallRet1(ctx, 8); }

// ---------------------------------------------------------------- position copies with a default
// dst = a1->+388 ? (a1->+388)->+432 : *(Vec3*)0x01164B40
DECOMP(000ED5D8, CopyPosOrDefault_000ED5D8) { TCopyPosOrDefault(ctx, 0x01164B40); }
// dst = a1->+388 ? (a1->+388)->+432 : *(Vec3*)0x01164B40
DECOMP(00168714, CopyPosOrDefault_00168714) { TCopyPosOrDefault(ctx, 0x01164B40); }
// dst = a1->+388 ? (a1->+388)->+432 : *(Vec3*)0x01164B40
DECOMP(0016F2B4, CopyPosOrDefault_0016F2B4) { TCopyPosOrDefault(ctx, 0x01164B40); }
// dst = a1->+388 ? (a1->+388)->+432 : *(Vec3*)0x01164B40
DECOMP(00178744, CopyPosOrDefault_00178744) { TCopyPosOrDefault(ctx, 0x01164B40); }
// dst = a1->+388 ? (a1->+388)->+432 : *(Vec3*)0x01164B40
DECOMP(00184B74, CopyPosOrDefault_00184B74) { TCopyPosOrDefault(ctx, 0x01164B40); }
// dst = a1->+388 ? (a1->+388)->+432 : *(Vec3*)0x01164B40
DECOMP(00192464, CopyPosOrDefault_00192464) { TCopyPosOrDefault(ctx, 0x01164B40); }

// ---------------------------------------------------------------- 24-byte clones
// p = SmallAlloc(24), vtable 0x00B11BD0, +20 / +8 copied
DECOMP(000A7B58, VtB11BD0_Clone) { TClone24(ctx, 0x00B11BD0); }
// p = SmallAlloc(24), vtable 0x00B11C18, +20 / +8 copied
DECOMP(000A7D30, VtB11C18_Clone) { TClone24(ctx, 0x00B11C18); }
// p = SmallAlloc(24), vtable 0x00B11C60, +20 / +8 copied
DECOMP(000A7F08, VtB11C60_Clone) { TClone24(ctx, 0x00B11C60); }
// p = SmallAlloc(24), vtable 0x00B11CA8, +20 / +8 copied
DECOMP(000A80E0, VtB11CA8_Clone) { TClone24(ctx, 0x00B11CA8); }
// p = SmallAlloc(24), vtable 0x00B11CF0, +20 / +8 copied
DECOMP(000A82B8, VtB11CF0_Clone) { TClone24(ctx, 0x00B11CF0); }
// p = SmallAlloc(24), vtable 0x00B11D38, +20 / +8 copied
DECOMP(000A8490, VtB11D38_Clone) { TClone24(ctx, 0x00B11D38); }

// ---------------------------------------------------------------- SmallAlloc, then T(p, this->+8, this + off, hash, this)
// p = SmallAlloc(384); if p: 0x003F834C(p, this->+8, this + 2580, 0x7719A548, this)
DECOMP(0032AA60, NewOwned_0032AA60) { TNewOwned(ctx, 384, 0x003F834C, 2580, 0x7719A548); }
// p = SmallAlloc(256); if p: 0x004015D0(p, this->+8, this + 2580, 0x7719A548, this)
DECOMP(00335744, NewOwned_00335744) { TNewOwned(ctx, 256, 0x004015D0, 2580, 0x7719A548); }
// p = SmallAlloc(256); if p: 0x0040773C(p, this->+8, this + 2580, 0x7719A548, this)
DECOMP(00335BB0, NewOwned_00335BB0) { TNewOwned(ctx, 256, 0x0040773C, 2580, 0x7719A548); }
// p = SmallAlloc(496); if p: 0x0040F244(p, this->+8, this + 2580, 0x7719A548, this)
DECOMP(003360FC, NewOwned_003360FC) { TNewOwned(ctx, 496, 0x0040F244, 2580, 0x7719A548); }
// p = SmallAlloc(448); if p: 0x00416FA4(p, this->+8, this + 2580, 0x7719A548, this)
DECOMP(00336660, NewOwned_00336660) { TNewOwned(ctx, 448, 0x00416FA4, 2580, 0x7719A548); }
// p = SmallAlloc(512); if p: 0x0041BB54(p, this->+8, this + 2580, 0x7719A548, this)
DECOMP(003391C4, NewOwned_003391C4) { TNewOwned(ctx, 512, 0x0041BB54, 2580, 0x7719A548); }

// ---------------------------------------------------------------- float range gates
// (this, f1, f2): flag / range test, else vfn[105](this, 1, 0)
DECOMP(001AF018, FloatGate_001AF018) { TFloatGate(ctx, 0x001AF010, 1, 0); }
// (this, f1, f2): flag / range test, else vfn[105](this, 2, 0)
DECOMP(001AF0A8, FloatGate_001AF0A8) { TFloatGate(ctx, 0x001AF0A4, 2, 0); }
// (this, f1, f2): flag / range test, else vfn[105](this, 3, 0)
DECOMP(001AF138, FloatGate_001AF138) { TFloatGate(ctx, 0x001AF134, 3, 0); }
// (this, f1, f2): flag / range test, else vfn[105](this, 4, 1)
DECOMP(001AF1C8, FloatGate_001AF1C8) { TFloatGate(ctx, 0x001AF1C4, 4, 1); }
// (this, f1, f2): flag / range test, else vfn[105](this, 6, 0)
DECOMP(001AF2F0, FloatGate_001AF2F0) { TFloatGate(ctx, 0x001AF2EC, 6, 0); }
// (this, f1, f2): flag / range test, else vfn[105](this, 1, 0)
DECOMP(001AF380, FloatGate_001AF380) { TFloatGate(ctx, 0x001AF37C, 1, 0); }

// ---------------------------------------------------------------- destructors through a secondary base: the member at +436, then the base
// p = this -384: vtable 0x00B18B48; 0x00638498(p + 436); base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(007584C4, AdjDtor_007584C4) { TAdjDtorMember(ctx, -384, {0x00B18B48, 0x00B18CE0, 0x00B18D08}, 0x00638498, 436, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -380: vtable 0x00B18B48; 0x00638498(p + 436); base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(00758550, AdjDtor_00758550) { TAdjDtorMember(ctx, -380, {0x00B18B48, 0x00B18CE0, 0x00B18D08}, 0x00638498, 436, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -384: vtable 0x00B18B48; 0x00638498(p + 436); base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(00758924, AdjDtor_00758924) { TAdjDtorMember(ctx, -384, {0x00B18B48, 0x00B18CE0, 0x00B18D08}, 0x00638498, 436, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -380: vtable 0x00B18B48; 0x00638498(p + 436); base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(007589B0, AdjDtor_007589B0) { TAdjDtorMember(ctx, -380, {0x00B18B48, 0x00B18CE0, 0x00B18D08}, 0x00638498, 436, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -384: vtable 0x00B18B48; 0x00638498(p + 436); base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(00758E04, AdjDtor_00758E04) { TAdjDtorMember(ctx, -384, {0x00B18B48, 0x00B18CE0, 0x00B18D08}, 0x00638498, 436, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }
// p = this -380: vtable 0x00B18B48; 0x00638498(p + 436); base vtable 0x00B126B8, 0x000DF934(p)
DECOMP(00758E90, AdjDtor_00758E90) { TAdjDtorMember(ctx, -380, {0x00B18B48, 0x00B18CE0, 0x00B18D08}, 0x00638498, 436, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934); }

// ---------------------------------------------------------------- their deleting versions
// deleting via this -384: vtable 0x00B18B48; 0x00638498(p + 436); base 0x000DF934(p); 0x0054EE6C(p, 0)
DECOMP(00758374, AdjDelete_00758374) { TAdjDeleteMember(ctx, -384, {0x00B18B48, 0x00B18CE0, 0x00B18D08}, 0x00638498, 436, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x0054EE6C, 0); }
// deleting via this -380: vtable 0x00B18B48; 0x00638498(p + 436); base 0x000DF934(p); 0x0054EE6C(p, 0)
DECOMP(0075841C, AdjDelete_0075841C) { TAdjDeleteMember(ctx, -380, {0x00B18B48, 0x00B18CE0, 0x00B18D08}, 0x00638498, 436, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x0054EE6C, 0); }
// deleting via this -384: vtable 0x00B18B48; 0x00638498(p + 436); base 0x000DF934(p); 0x00552420(p, 448)
DECOMP(007587D4, AdjDelete_007587D4) { TAdjDeleteMember(ctx, -384, {0x00B18B48, 0x00B18CE0, 0x00B18D08}, 0x00638498, 436, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 448); }
// deleting via this -380: vtable 0x00B18B48; 0x00638498(p + 436); base 0x000DF934(p); 0x00552420(p, 448)
DECOMP(0075887C, AdjDelete_0075887C) { TAdjDeleteMember(ctx, -380, {0x00B18B48, 0x00B18CE0, 0x00B18D08}, 0x00638498, 436, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 448); }
// deleting via this -384: vtable 0x00B18B48; 0x00638498(p + 436); base 0x000DF934(p); 0x00552420(p, 608)
DECOMP(00758CB4, AdjDelete_00758CB4) { TAdjDeleteMember(ctx, -384, {0x00B18B48, 0x00B18CE0, 0x00B18D08}, 0x00638498, 436, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 608); }
// deleting via this -380: vtable 0x00B18B48; 0x00638498(p + 436); base 0x000DF934(p); 0x00552420(p, 608)
DECOMP(00758D5C, AdjDelete_00758D5C) { TAdjDeleteMember(ctx, -380, {0x00B18B48, 0x00B18CE0, 0x00B18D08}, 0x00638498, 436, {0x00B126B8, 0x00B1200C, 0x00B12528}, 0x000DF934, 0x00552420, 608); }

// ---------------------------------------------------------------- 24-byte objects initialised from a lookup
// p = SmallAlloc(24), vtable 0x00B11BD0; r = 0x004F9628(a1, 0x00AC5A5C); p->+8 = r ? 0x0056A710(r) : 0
DECOMP(000A7BD8, VtB11BD0_New) { TNewLookup24(ctx, 0x00B11BD0, 0x004F9628, 0x00AC5A5C, 0x0056A710); }
// p = SmallAlloc(24), vtable 0x00B11C18; r = 0x004F9628(a1, 0x00AC5A5C); p->+8 = r ? 0x0056A710(r) : 0
DECOMP(000A7DB0, VtB11C18_New) { TNewLookup24(ctx, 0x00B11C18, 0x004F9628, 0x00AC5A5C, 0x0056A710); }
// p = SmallAlloc(24), vtable 0x00B11C60; r = 0x004F9628(a1, 0x00AC5A5C); p->+8 = r ? 0x0056A710(r) : 0
DECOMP(000A7F88, VtB11C60_New) { TNewLookup24(ctx, 0x00B11C60, 0x004F9628, 0x00AC5A5C, 0x0056A710); }
// p = SmallAlloc(24), vtable 0x00B11CA8; r = 0x004F9628(a1, 0x00AC5A5C); p->+8 = r ? 0x0056A710(r) : 0
DECOMP(000A8160, VtB11CA8_New) { TNewLookup24(ctx, 0x00B11CA8, 0x004F9628, 0x00AC5A5C, 0x0056A710); }
// p = SmallAlloc(24), vtable 0x00B11CF0; r = 0x004F9628(a1, 0x00AC5A5C); p->+8 = r ? 0x0056A710(r) : 0
DECOMP(000A8338, VtB11CF0_New) { TNewLookup24(ctx, 0x00B11CF0, 0x004F9628, 0x00AC5A5C, 0x0056A710); }
// p = SmallAlloc(24), vtable 0x00B11D38; r = 0x004F9628(a1, 0x00AC5A5C); p->+8 = r ? 0x0056A710(r) : 0
DECOMP(000A8510, VtB11D38_New) { TNewLookup24(ctx, 0x00B11D38, 0x004F9628, 0x00AC5A5C, 0x0056A710); }

// ---------------------------------------------------------------- map lookups by value: *find(map, &key)
// *0x0076DF20(0x0106BB9C, &key)
DECOMP(003482B0, MapGet_003482B0) { TMapGetByValue(ctx, 0x0076DF20, 0x0106BB9C); }
// *0x0076E230(0x0106BBB4, &key)
DECOMP(00348650, MapGet_00348650) { TMapGetByValue(ctx, 0x0076E230, 0x0106BBB4); }
// *0x0076E0A8(0x0106BBCC, &key)
DECOMP(003489B0, MapGet_003489B0) { TMapGetByValue(ctx, 0x0076E0A8, 0x0106BBCC); }
// *0x00784308(0x010AFEC8, &key)
DECOMP(0044C404, MapGet_0044C404) { TMapGetByValue(ctx, 0x00784308, 0x010AFEC8); }

// ---------------------------------------------------------------- destructors clearing a link
// *this = vtable 0x00B20840; link +4 flagged (0x1000 at +176) and cleared
DECOMP(003F6848, VtB20840_Dtor) { TDtorUnlink(ctx, 0x00B20840); }
// *this = vtable 0x00B208E8; link +4 flagged (0x1000 at +176) and cleared
DECOMP(003FF858, VtB208E8_Dtor) { TDtorUnlink(ctx, 0x00B208E8); }
// *this = vtable 0x00B20990; link +4 flagged (0x1000 at +176) and cleared
DECOMP(0040549C, VtB20990_Dtor) { TDtorUnlink(ctx, 0x00B20990); }
// *this = vtable 0x00B20A38; link +4 flagged (0x1000 at +176) and cleared
DECOMP(0040ACD4, VtB20A38_Dtor) { TDtorUnlink(ctx, 0x00B20A38); }
// *this = vtable 0x00B20B88; link +4 flagged (0x1000 at +176) and cleared
DECOMP(0041A35C, VtB20B88_Dtor) { TDtorUnlink(ctx, 0x00B20B88); }

// ---------------------------------------------------------------- a1 ? set(this, conv(a1), k) : 0
// a1 ? 0x000D6ED0(this, 0x0056A710(a1), 0x00F33608) : 0
DECOMP(001D2CFC, SetConverted_001D2CFC) { TSetConverted(ctx, 0x0056A710, 0x000D6ED0, 0x00F33608); }
// a1 ? 0x000D6ED0(this, 0x0056A710(a1), 0x00F33CD0) : 0
DECOMP(001EB9CC, SetConverted_001EB9CC) { TSetConverted(ctx, 0x0056A710, 0x000D6ED0, 0x00F33CD0); }
// a1 ? 0x000D6ED0(this, 0x0056A710(a1), 0x00F38250) : 0
DECOMP(003A1828, SetConverted_003A1828) { TSetConverted(ctx, 0x0056A710, 0x000D6ED0, 0x00F38250); }
// a1 ? 0x000D6ED0(this, 0x0056A710(a1), 0x00F38260) : 0
DECOMP(003A1BA8, SetConverted_003A1BA8) { TSetConverted(ctx, 0x0056A710, 0x000D6ED0, 0x00F38260); }
// a1 ? 0x000D6ED0(this, 0x0056A710(a1), 0x00F38270) : 0
DECOMP(003A20A4, SetConverted_003A20A4) { TSetConverted(ctx, 0x0056A710, 0x000D6ED0, 0x00F38270); }

// ---------------------------------------------------------------- destructors freeing a buffer at +148
// *this = vtable 0x00B1F6A0; Mem_FreeFrom(+148, 0); *this = 0x00B1F668
DECOMP(0077295C, VtB1F6A0_Dtor) { TDtorFreeBuf(ctx, 0x00B1F6A0, 148, 0x00B1F668); }
// *this = vtable 0x00B1F70C; Mem_FreeFrom(+148, 0); *this = 0x00B1F668
DECOMP(00772ABC, VtB1F70C_Dtor) { TDtorFreeBuf(ctx, 0x00B1F70C, 148, 0x00B1F668); }
// *this = vtable 0x00B1F828; Mem_FreeFrom(+148, 0); *this = 0x00B1F668
DECOMP(00772CBC, VtB1F828_Dtor) { TDtorFreeBuf(ctx, 0x00B1F828, 148, 0x00B1F668); }
// *this = vtable 0x00B1F9F8; Mem_FreeFrom(+148, 0); *this = 0x00B1F668
DECOMP(00773564, VtB1F9F8_Dtor) { TDtorFreeBuf(ctx, 0x00B1F9F8, 148, 0x00B1F668); }
// *this = vtable 0x00B1F6A0; Mem_FreeFrom(+148, 0); *this = 0x00B1F668
DECOMP(00773644, VtB1F6A0_Dtor_00773644) { TDtorFreeBuf(ctx, 0x00B1F6A0, 148, 0x00B1F668); }

// ---------------------------------------------------------------- deleting destructors of pooled objects (fixed size)
// deleting: vtable 0x00B13C84, then pool->vfn[3](pool, this, 8)
DECOMP(007490E0, VtB13C84_Delete) { TDeletePooledK(ctx, 0x00B13C84, -28332, 8); }
// deleting: vtable 0x00B13C9C, then pool->vfn[3](pool, this, 8)
DECOMP(0074916C, VtB13C9C_Delete) { TDeletePooledK(ctx, 0x00B13C9C, -28332, 8); }
// deleting: vtable 0x00B194F8, then pool->vfn[3](pool, this, 4)
DECOMP(00759C4C, VtB194F8_Delete) { TDeletePooledK(ctx, 0x00B194F8, -28332, 4); }
// deleting: vtable 0x00B13C9C, then pool->vfn[3](pool, this, 8)
DECOMP(00759D38, VtB13C9C_Delete_00759D38) { TDeletePooledK(ctx, 0x00B13C9C, -28332, 8); }
// deleting: vtable 0x00F2B810, then pool->vfn[3](pool, this, 4)
DECOMP(00771B74, VtF2B810_Delete) { TDeletePooledK(ctx, 0x00F2B810, -28332, 4); }

// ---------------------------------------------------------------- deleting destructors clearing a link
// deleting: vtable 0x00B20840; link +4 flagged and cleared; SmallFree(this, 176)
DECOMP(003F6888, VtB20840_Delete) { TDeleteUnlink(ctx, 0x00B20840, 176); }
// deleting: vtable 0x00B208E8; link +4 flagged and cleared; SmallFree(this, 192)
DECOMP(003FF898, VtB208E8_Delete) { TDeleteUnlink(ctx, 0x00B208E8, 192); }
// deleting: vtable 0x00B20990; link +4 flagged and cleared; SmallFree(this, 224)
DECOMP(004054DC, VtB20990_Delete) { TDeleteUnlink(ctx, 0x00B20990, 224); }
// deleting: vtable 0x00B20A38; link +4 flagged and cleared; SmallFree(this, 384)
DECOMP(0040AD14, VtB20A38_Delete) { TDeleteUnlink(ctx, 0x00B20A38, 384); }
// deleting: vtable 0x00B20B88; link +4 flagged and cleared; SmallFree(this, 176)
DECOMP(0041A39C, VtB20B88_Delete) { TDeleteUnlink(ctx, 0x00B20B88, 176); }

// ---------------------------------------------------------------- short event-record inits
// (o, src, b, c): kind 4, the time, src-derived defaults
DECOMP(0046502C, EventInit_0046502C) { TEventInitShort(ctx, 4, 0x00F65CA0); }
// (o, src, b, c): kind 5, the time, src-derived defaults
DECOMP(004650A8, EventInit_004650A8) { TEventInitShort(ctx, 5, 0x00F65CA0); }
// (o, src, b, c): kind 31, the time, src-derived defaults
DECOMP(00465124, EventInit_00465124) { TEventInitShort(ctx, 31, 0x00F65CA0); }
// (o, src, b, c): kind 11, the time, src-derived defaults
DECOMP(0046528C, EventInit_0046528C) { TEventInitShort(ctx, 11, 0x00F65CA0); }
// (o, src, b, c): kind 12, the time, src-derived defaults
DECOMP(00465308, EventInit_00465308) { TEventInitShort(ctx, 12, 0x00F65CA0); }

// ---------------------------------------------------------------- deleting destructors freeing a buffer at +148
// deleting: vtable 0x00B1F6A0; Mem_FreeFrom(+148, 0); *this = 0x00B1F668; Mem_FreeFrom(this, 0)
DECOMP(007729C0, VtB1F6A0_Delete) { TDeleteFreeBuf(ctx, 0x00B1F6A0, 148, 0x00B1F668); }
// deleting: vtable 0x00B1F70C; Mem_FreeFrom(+148, 0); *this = 0x00B1F668; Mem_FreeFrom(this, 0)
DECOMP(00772B20, VtB1F70C_Delete) { TDeleteFreeBuf(ctx, 0x00B1F70C, 148, 0x00B1F668); }
// deleting: vtable 0x00B1F828; Mem_FreeFrom(+148, 0); *this = 0x00B1F668; Mem_FreeFrom(this, 0)
DECOMP(00772D20, VtB1F828_Delete) { TDeleteFreeBuf(ctx, 0x00B1F828, 148, 0x00B1F668); }
// deleting: vtable 0x00B1F9F8; Mem_FreeFrom(+148, 0); *this = 0x00B1F668; Mem_FreeFrom(this, 0)
DECOMP(007735C8, VtB1F9F8_Delete) { TDeleteFreeBuf(ctx, 0x00B1F9F8, 148, 0x00B1F668); }
// deleting: vtable 0x00B1F6A0; Mem_FreeFrom(+148, 0); *this = 0x00B1F668; Mem_FreeFrom(this, 0)
DECOMP(007736A8, VtB1F6A0_Delete_007736A8) { TDeleteFreeBuf(ctx, 0x00B1F6A0, 148, 0x00B1F668); }

// ---------------------------------------------------------------- destructors releasing the objects of a PtrVector
// *this = vtable 0x00B21FD8; each object of +8 vfn[2]; ~PtrVector(+8); *this = 0x00B11B20
DECOMP(004C0C14, VtB21FD8_Dtor) { TDtorOwnerVec(ctx, 0x00B21FD8, 0x00B11B20); }
// *this = vtable 0x00B21FD8; each object of +8 vfn[2]; ~PtrVector(+8); *this = 0x00B11B20
DECOMP(004C0FB4, VtB21FD8_Dtor_004C0FB4) { TDtorOwnerVec(ctx, 0x00B21FD8, 0x00B11B20); }
// *this = vtable 0x00B21FD8; each object of +8 vfn[2]; ~PtrVector(+8); *this = 0x00B11B20
DECOMP(004C12A0, VtB21FD8_Dtor_004C12A0) { TDtorOwnerVec(ctx, 0x00B21FD8, 0x00B11B20); }
// *this = vtable 0x00B21FD8; each object of +8 vfn[2]; ~PtrVector(+8); *this = 0x00B11B20
DECOMP(004C15A8, VtB21FD8_Dtor_004C15A8) { TDtorOwnerVec(ctx, 0x00B21FD8, 0x00B11B20); }
// *this = vtable 0x00B21FD8; each object of +8 vfn[2]; ~PtrVector(+8); *this = 0x00B11B20
DECOMP(004C18E0, VtB21FD8_Dtor_004C18E0) { TDtorOwnerVec(ctx, 0x00B21FD8, 0x00B11B20); }

// ---------------------------------------------------------------- u8 field getters
// returns (u8)this->+513
DECOMP(0005FADC, GetU8_201_0005FADC) { TLoadU8(ctx, 513); }
// returns (u8)this->+16
DECOMP(00434D8C, GetU8_10_00434D8C) { TLoadU8(ctx, 16); }
// returns (u8)this->+0
DECOMP(004A08B8, GetU8_0_004A08B8) { TLoadU8(ctx, 0); }
// returns (u8)this->+0
DECOMP(004A08C0, GetU8_0_004A08C0) { TLoadU8(ctx, 0); }
// returns (u8)this->+532
DECOMP(007382E8, GetU8_214_007382E8) { TLoadU8(ctx, 532); }
// returns (u8)this->+533
DECOMP(007382F0, GetU8_215_007382F0) { TLoadU8(ctx, 533); }
// returns (u8)this->+8
DECOMP(00767A94, GetU8_8_00767A94) { TLoadU8(ctx, 8); }
// returns (u8)this->+2412
DECOMP(00769B00, GetU8_96C_00769B00) { TLoadU8(ctx, 2412); }
// returns (u8)this->+54
DECOMP(0077EEC4, GetU8_36_0077EEC4) { TLoadU8(ctx, 54); }
// returns (u8)this->+360
DECOMP(0077EF40, GetU8_168_0077EF40) { TLoadU8(ctx, 360); }
// returns (u8)this->+361
DECOMP(0077EF48, GetU8_169_0077EF48) { TLoadU8(ctx, 361); }
// returns (u8)this->+362
DECOMP(0077EF50, GetU8_16A_0077EF50) { TLoadU8(ctx, 362); }
// returns (u8)this->+363
DECOMP(0077EF58, GetU8_16B_0077EF58) { TLoadU8(ctx, 363); }
// returns (u8)this->+54
DECOMP(0077EF60, GetU8_36_0077EF60) { TLoadU8(ctx, 54); }
// returns (u8)this->+236
DECOMP(0077F0D0, GetU8_EC_0077F0D0) { TLoadU8(ctx, 236); }
// returns (u8)this->+237
DECOMP(0077F0D8, GetU8_ED_0077F0D8) { TLoadU8(ctx, 237); }
// returns (u8)this->+238
DECOMP(0077F0E0, GetU8_EE_0077F0E0) { TLoadU8(ctx, 238); }
// returns (u8)this->+239
DECOMP(0077F0E8, GetU8_EF_0077F0E8) { TLoadU8(ctx, 239); }
// returns (u8)this->+228
DECOMP(0077F17C, GetU8_E4_0077F17C) { TLoadU8(ctx, 228); }
// returns (u8)this->+229
DECOMP(0077F184, GetU8_E5_0077F184) { TLoadU8(ctx, 229); }
// returns (u8)this->+230
DECOMP(0077F18C, GetU8_E6_0077F18C) { TLoadU8(ctx, 230); }
// returns (u8)this->+231
DECOMP(0077F194, GetU8_E7_0077F194) { TLoadU8(ctx, 231); }
// returns (u8)this->+476
DECOMP(0077F228, GetU8_1DC_0077F228) { TLoadU8(ctx, 476); }
// returns (u8)this->+477
DECOMP(0077F230, GetU8_1DD_0077F230) { TLoadU8(ctx, 477); }
// returns (u8)this->+478
DECOMP(0077F238, GetU8_1DE_0077F238) { TLoadU8(ctx, 478); }
// returns (u8)this->+479
DECOMP(0077F240, GetU8_1DF_0077F240) { TLoadU8(ctx, 479); }
// returns (u8)this->+424
DECOMP(0077F338, GetU8_1A8_0077F338) { TLoadU8(ctx, 424); }
// returns (u8)this->+425
DECOMP(0077F340, GetU8_1A9_0077F340) { TLoadU8(ctx, 425); }
// returns (u8)this->+426
DECOMP(0077F348, GetU8_1AA_0077F348) { TLoadU8(ctx, 426); }
// returns (u8)this->+480
DECOMP(0077F43C, GetU8_1E0_0077F43C) { TLoadU8(ctx, 480); }
// returns (u8)this->+481
DECOMP(0077F444, GetU8_1E1_0077F444) { TLoadU8(ctx, 481); }
// returns (u8)this->+482
DECOMP(0077F44C, GetU8_1E2_0077F44C) { TLoadU8(ctx, 482); }
// returns (u8)this->+483
DECOMP(0077F454, GetU8_1E3_0077F454) { TLoadU8(ctx, 483); }

// ---------------------------------------------------------------- u32 field getters
// returns this->+180
DECOMP(007376D8, GetU32_B4_007376D8) { TLoadU32(ctx, 180); }
// returns this->+184
DECOMP(007376E0, GetU32_B8_007376E0) { TLoadU32(ctx, 184); }
// returns this->+356
DECOMP(00737848, GetU32_164_00737848) { TLoadU32(ctx, 356); }
// returns this->+396
DECOMP(00738344, GetU32_18C_00738344) { TLoadU32(ctx, 396); }
// returns this->+400
DECOMP(0073834C, GetU32_190_0073834C) { TLoadU32(ctx, 400); }
// returns this->+404
DECOMP(00738354, GetU32_194_00738354) { TLoadU32(ctx, 404); }
// returns this->+644
DECOMP(007442EC, GetU32_284_007442EC) { TLoadU32(ctx, 644); }
// returns this->+432
DECOMP(007444AC, GetU32_1B0_007444AC) { TLoadU32(ctx, 432); }
// returns this->+224
DECOMP(007470BC, GetU32_E0_007470BC) { TLoadU32(ctx, 224); }
// returns this->+228
DECOMP(007470C4, GetU32_E4_007470C4) { TLoadU32(ctx, 228); }
// returns this->+352
DECOMP(00747398, GetU32_160_00747398) { TLoadU32(ctx, 352); }
// returns this->+348
DECOMP(007473A0, GetU32_15C_007473A0) { TLoadU32(ctx, 348); }
// returns this->+84
DECOMP(00749630, GetU32_54_00749630) { TLoadU32(ctx, 84); }
// returns this->+16
DECOMP(0074964C, GetU32_10_0074964C) { TLoadU32(ctx, 16); }
// returns this->+144
DECOMP(007497D8, GetU32_90_007497D8) { TLoadU32(ctx, 144); }
// returns this->+592
DECOMP(00749814, GetU32_250_00749814) { TLoadU32(ctx, 592); }
// returns this->+192
DECOMP(00749850, GetU32_C0_00749850) { TLoadU32(ctx, 192); }
// returns this->+84
DECOMP(00749880, GetU32_54_00749880) { TLoadU32(ctx, 84); }
// returns this->+136
DECOMP(007499AC, GetU32_88_007499AC) { TLoadU32(ctx, 136); }
// returns this->+208
DECOMP(0074CB1C, GetU32_D0_0074CB1C) { TLoadU32(ctx, 208); }
// returns this->+44
DECOMP(0074CE18, GetU32_2C_0074CE18) { TLoadU32(ctx, 44); }
// returns this->+576
DECOMP(007538EC, GetU32_240_007538EC) { TLoadU32(ctx, 576); }
// returns this->+416
DECOMP(0075862C, GetU32_1A0_0075862C) { TLoadU32(ctx, 416); }
// returns this->+428
DECOMP(007594C0, GetU32_1AC_007594C0) { TLoadU32(ctx, 428); }
// returns this->+428
DECOMP(0075B7E4, GetU32_1AC_0075B7E4) { TLoadU32(ctx, 428); }
// returns this->+396
DECOMP(0075DAA4, GetU32_18C_0075DAA4) { TLoadU32(ctx, 396); }
// returns this->+416
DECOMP(0075DBB4, GetU32_1A0_0075DBB4) { TLoadU32(ctx, 416); }
// returns this->+48
DECOMP(00762B14, GetU32_30_00762B14) { TLoadU32(ctx, 48); }
// returns this->+2208
DECOMP(00769A9C, GetU32_8A0_00769A9C) { TLoadU32(ctx, 2208); }
// returns this->+2400
DECOMP(00769AF8, GetU32_960_00769AF8) { TLoadU32(ctx, 2400); }
// returns this->+48
DECOMP(0077D92C, SubMachine_GetWorld) { TLoadU32(ctx, 48); }
// returns this->+20
DECOMP(00793DC4, GetU32_14_00793DC4) { TLoadU32(ctx, 20); }
// returns this->+20
DECOMP(00793E4C, GetU32_14_00793E4C) { TLoadU32(ctx, 20); }

// ---------------------------------------------------------------- flag-bit getters
// returns bit 31 of this->+416
DECOMP(002697F8, GetFlag_002697F8) { TFlagBit(ctx, 416, 1); }
// returns bit 12 of this->+40
DECOMP(0074B570, GetFlag_0074B570) { TFlagBit(ctx, 40, 20); }
// returns bit 11 of this->+244
DECOMP(0074B588, GetFlag_0074B588) { TFlagBit(ctx, 244, 21); }
// returns bit 1 of this->+172
DECOMP(0074B5A0, GetFlag_0074B5A0) { TFlagBit(ctx, 172, 31); }
// returns bit 30 of this->+40
DECOMP(0074B66C, GetFlag_0074B66C) { TFlagBit(ctx, 40, 2); }
// returns bit 31 of this->+40
DECOMP(0074BBEC, GetFlag_0074BBEC) { TFlagBit(ctx, 40, 1); }
// returns bit 31 of this->+96
DECOMP(0074C010, GetFlag_0074C010) { TFlagBit(ctx, 96, 1); }
// returns bit 30 of this->+96
DECOMP(0074C364, GetFlag_0074C364) { TFlagBit(ctx, 96, 2); }
// returns bit 29 of this->+96
DECOMP(0074C37C, GetFlag_0074C37C) { TFlagBit(ctx, 96, 3); }
// returns bit 15 of this->+164
DECOMP(0074C6AC, GetFlag_0074C6AC) { TFlagBit(ctx, 164, 17); }
// returns bit 7 of this->+172
DECOMP(0074D808, GetFlag_0074D808) { TFlagBit(ctx, 172, 25); }
// returns bit 16 of this->+40
DECOMP(0074E720, GetFlag_0074E720) { TFlagBit(ctx, 40, 16); }
// returns bit 21 of this->+68
DECOMP(00760B00, GetFlag_00760B00) { TFlagBit(ctx, 68, 11); }
// returns bit 14 of this->+68
DECOMP(00760B48, GetFlag_00760B48) { TFlagBit(ctx, 68, 18); }
// returns bit 10 of this->+68
DECOMP(00760B60, GetFlag_00760B60) { TFlagBit(ctx, 68, 22); }
// returns bit 24 of this->+148
DECOMP(00763EF8, GetFlag_00763EF8) { TFlagBit(ctx, 148, 8); }
// returns bit 24 of this->+656
DECOMP(0077D0FC, GetFlag_0077D0FC) { TFlagBit(ctx, 656, 8); }
// returns bit 29 of this->+136
DECOMP(007962F4, GetFlag_007962F4) { TFlagBit(ctx, 136, 3); }

// ---------------------------------------------------------------- Vec3 field copies: (this + d)[0..2] = (a1 + s)[0..2]
// this->+640[0..2] = a1->+0[0..2]
DECOMP(000F43FC, SetVec3_280_000F43FC) { TCopyFields2(ctx, 640, 0, 3); }
// this->+0[0..2] = a1->+512[0..2]
DECOMP(0023C7E8, SetVec3_0_0023C7E8) { TCopyFields2(ctx, 0, 512, 3); }
// this->+0[0..2] = a1->+528[0..2]
DECOMP(0023C804, SetVec3_0_0023C804) { TCopyFields2(ctx, 0, 528, 3); }
// this->+176[0..2] = a1->+0[0..2]
DECOMP(00735B68, SetVec3_B0_00735B68) { TCopyFields2(ctx, 176, 0, 3); }
// this->+80[0..2] = a1->+0[0..2]
DECOMP(007362AC, SetVec3_50_007362AC) { TCopyFields2(ctx, 80, 0, 3); }
// this->+0[0..2] = a1->+608[0..2]
DECOMP(0074BBD0, SetVec3_0_0074BBD0) { TCopyFields2(ctx, 0, 608, 3); }
// this->+0[0..2] = a1->+80[0..2]
DECOMP(0074C348, SetVec3_0_0074C348) { TCopyFields2(ctx, 0, 80, 3); }
// this->+0[0..2] = a1->+592[0..2]
DECOMP(0074C5E4, SetVec3_0_0074C5E4) { TCopyFields2(ctx, 0, 592, 3); }
// this->+0[0..2] = a1->+80[0..2]
DECOMP(0074CC8C, SetVec3_0_0074CC8C) { TCopyFields2(ctx, 0, 80, 3); }
// this->+0[0..2] = a1->+208[0..2]
DECOMP(0074D01C, SetVec3_0_0074D01C) { TCopyFields2(ctx, 0, 208, 3); }
// this->+48[0..2] = a1->+0[0..2]
DECOMP(0074D728, SetVec3_30_0074D728) { TCopyFields2(ctx, 48, 0, 3); }
// this->+32[0..2] = a1->+0[0..2]
DECOMP(0074DBD0, SetVec3_20_0074DBD0) { TCopyFields2(ctx, 32, 0, 3); }
// this->+16[0..2] = a1->+0[0..2]
DECOMP(0074DC08, SetVec3_10_0074DC08) { TCopyFields2(ctx, 16, 0, 3); }
// this->+0[0..2] = a1->+400[0..2]
DECOMP(0075D72C, SetVec3_0_0075D72C) { TCopyFields2(ctx, 0, 400, 3); }
// this->+0[0..2] = a1->+400[0..2]
DECOMP(0075DA88, SetVec3_0_0075DA88) { TCopyFields2(ctx, 0, 400, 3); }
// this->+0[0..2] = a1->+32[0..2]
DECOMP(00762B1C, SetVec3_0_00762B1C) { TCopyFields2(ctx, 0, 32, 3); }
// this->+48[0..2] = a1->+0[0..2]
DECOMP(0077CE0C, SetVec3_30_0077CE0C) { TCopyFields2(ctx, 48, 0, 3); }

// ---------------------------------------------------------------- global getters
// returns *(u32*)0x00F64988
DECOMP(0017574C, GetGlobal_0017574C) { TLoadGlobalU32(ctx, 0x00F64988); }
// returns *(u32*)0x00F65860
DECOMP(007821B4, GetGlobal_007821B4) { TLoadGlobalU32(ctx, 0x00F65860); }
// returns *(u32*)0x00F65864
DECOMP(00782424, GetGlobal_00782424) { TLoadGlobalU32(ctx, 0x00F65864); }
// returns *(u32*)0x00F65868
DECOMP(007824E8, GetGlobal_007824E8) { TLoadGlobalU32(ctx, 0x00F65868); }
// returns *(u32*)0x00F6586C
DECOMP(00782618, GetGlobal_00782618) { TLoadGlobalU32(ctx, 0x00F6586C); }
// returns *(u32*)0x00F65870
DECOMP(00782748, GetGlobal_00782748) { TLoadGlobalU32(ctx, 0x00F65870); }
// returns *(u32*)0x00F65874
DECOMP(0078287C, GetGlobal_0078287C) { TLoadGlobalU32(ctx, 0x00F65874); }
// returns *(u32*)0x00F65878
DECOMP(007829C4, GetGlobal_007829C4) { TLoadGlobalU32(ctx, 0x00F65878); }
// returns *(u32*)0x00F6587C
DECOMP(00782AFC, GetGlobal_00782AFC) { TLoadGlobalU32(ctx, 0x00F6587C); }
// returns *(u32*)0x00F65880
DECOMP(00782C34, GetGlobal_00782C34) { TLoadGlobalU32(ctx, 0x00F65880); }
// returns *(u32*)0x00F65884
DECOMP(00782D6C, GetGlobal_00782D6C) { TLoadGlobalU32(ctx, 0x00F65884); }
// returns *(u32*)0x00F65888
DECOMP(00782EA4, GetGlobal_00782EA4) { TLoadGlobalU32(ctx, 0x00F65888); }
// returns *(u32*)0x00F658D8
DECOMP(00782FDC, GetGlobal_00782FDC) { TLoadGlobalU32(ctx, 0x00F658D8); }
// returns *(u32*)0x00F658DC
DECOMP(00783114, GetGlobal_00783114) { TLoadGlobalU32(ctx, 0x00F658DC); }
// returns *(u32*)0x00F658E0
DECOMP(0078324C, GetGlobal_0078324C) { TLoadGlobalU32(ctx, 0x00F658E0); }

// ---------------------------------------------------------------- byte setters: this->+off = k
// *(u8*)(this + 6256) = 1
DECOMP(0005BB08, SetByte_1870_0005BB08) { TStoreU8At(ctx, 6256, 1); }
// *(u8*)(this + 120) = 1
DECOMP(00313268, SetByte_78_00313268) { TStoreU8At(ctx, 120, 1); }
// *(u8*)(this + 64) = 1
DECOMP(003141F4, SetByte_40_003141F4) { TStoreU8At(ctx, 64, 1); }
// *(u8*)(this + 88) = 1
DECOMP(003157B4, SetByte_58_003157B4) { TStoreU8At(ctx, 88, 1); }
// *(u8*)(this + 16) = 1
DECOMP(00434E00, SetByte_10_00434E00) { TStoreU8At(ctx, 16, 1); }
// *(u8*)(this + 28) = 1
DECOMP(00766C2C, SetByte_1C_00766C2C) { TStoreU8At(ctx, 28, 1); }
// *(u8*)(this + 28) = 0
DECOMP(00766C38, SetByte_1C_00766C38) { TStoreU8At(ctx, 28, 0); }
// *(u8*)(this + 8) = 1
DECOMP(00767A9C, SetByte_8_00767A9C) { TStoreU8At(ctx, 8, 1); }

// ---------------------------------------------------------------- u32 field setters
// this->+336 = a1
DECOMP(000FCBC8, SetU32_150_000FCBC8) { TStoreU32Arg(ctx, 336); }
// this->+840 = a1
DECOMP(001EBDE4, SetU32_348_001EBDE4) { TStoreU32Arg(ctx, 840); }
// this->+20 = a1
DECOMP(00327920, SetU32_14_00327920) { TStoreU32Arg(ctx, 20); }
// this->+72 = a1
DECOMP(007360FC, SetU32_48_007360FC) { TStoreU32Arg(ctx, 72); }
// this->+72 = a1
DECOMP(00736598, SetU32_48_00736598) { TStoreU32Arg(ctx, 72); }
// this->+336 = a1
DECOMP(007373A4, SetU32_150_007373A4) { TStoreU32Arg(ctx, 336); }
// this->+20 = a1
DECOMP(00769A2C, SetU32_14_00769A2C) { TStoreU32Arg(ctx, 20); }

// ---------------------------------------------------------------- float field getters
// f1 = this->+608
DECOMP(00211DF0, GetF32_260_00211DF0) { TLoadF32(ctx, 608); }
// f1 = this->+528
DECOMP(007382E0, GetF32_210_007382E0) { TLoadF32(ctx, 528); }
// f1 = this->+536
DECOMP(007382F8, GetF32_218_007382F8) { TLoadF32(ctx, 536); }
// f1 = this->+540
DECOMP(00738300, GetF32_21C_00738300) { TLoadF32(ctx, 540); }
// f1 = this->+20
DECOMP(00793EB4, GetF32_14_00793EB4) { TLoadF32(ctx, 20); }
// f1 = this->+28
DECOMP(007941B0, GetF32_1C_007941B0) { TLoadF32(ctx, 28); }

// ---------------------------------------------------------------- float getters: (this->+a)->+b
// f1 = (this->+216)->+276
DECOMP(000E886C, GetF32_000E886C) { TF32Chain(ctx, 216, 276); }
// f1 = (this->+216)->+216
DECOMP(000EDB98, GetF32_000EDB98) { TF32Chain(ctx, 216, 216); }
// f1 = (this->+216)->+236
DECOMP(00179F68, GetF32_00179F68) { TF32Chain(ctx, 216, 236); }
// f1 = (this->+216)->+268
DECOMP(001FC564, GetF32_001FC564) { TF32Chain(ctx, 216, 268); }
// f1 = (this->+216)->+224
DECOMP(00229A0C, GetF32_00229A0C) { TF32Chain(ctx, 216, 224); }
// f1 = (this->+208)->+120
DECOMP(0074CFB0, GetF32_0074CFB0) { TF32Chain(ctx, 208, 120); }

// ---------------------------------------------------------------- u8 field setters
// *(u8*)(this + 532) = a1
DECOMP(001071C4, SetU8_214_001071C4) { TStoreU8Arg(ctx, 532); }
// *(u8*)(this + 16) = a1
DECOMP(0025069C, SetU8_10_0025069C) { TStoreU8Arg(ctx, 16); }
// *(u8*)(this + 72) = a1
DECOMP(00736004, SetU8_48_00736004) { TStoreU8Arg(ctx, 72); }
// *(u8*)(this + 16) = a1
DECOMP(007375A4, SetU8_10_007375A4) { TStoreU8Arg(ctx, 16); }
// *(u8*)(this + 54) = a1
DECOMP(0077EECC, SetU8_36_0077EECC) { TStoreU8Arg(ctx, 54); }

// ---------------------------------------------------------------- AVL rotations
// (?, Node** pp): the +12 child up, heights fixed
DECOMP(00739DD4, Avl_RotateR_00739DD4) { TAvlRotate(ctx, 12, 16); }
// (?, Node** pp): the +16 child up, heights fixed
DECOMP(00739F1C, Avl_RotateL_00739F1C) { TAvlRotate(ctx, 16, 12); }
// (?, Node** pp): the +12 child up, heights fixed
DECOMP(00749D40, Avl_RotateR_00749D40) { TAvlRotate(ctx, 12, 16); }
// (?, Node** pp): the +16 child up, heights fixed
DECOMP(00749E88, Avl_RotateL_00749E88) { TAvlRotate(ctx, 16, 12); }
// (?, Node** pp): the +12 child up, heights fixed
DECOMP(0076BE8C, Avl_RotateR_0076BE8C) { TAvlRotate(ctx, 12, 16); }
// (?, Node** pp): the +16 child up, heights fixed
DECOMP(0076BFD4, Avl_RotateL_0076BFD4) { TAvlRotate(ctx, 16, 12); }
// (?, Node** pp): the +12 child up, heights fixed
DECOMP(0077B570, Avl_RotateR_0077B570) { TAvlRotate(ctx, 12, 16); }
// (?, Node** pp): the +16 child up, heights fixed
DECOMP(0077B6B8, Avl_RotateL_0077B6B8) { TAvlRotate(ctx, 16, 12); }
// (?, Node** pp): the +12 child up, heights fixed
DECOMP(007854B4, Avl_RotateR_007854B4) { TAvlRotate(ctx, 12, 16); }
// (?, Node** pp): the +16 child up, heights fixed
DECOMP(007855FC, Avl_RotateL_007855FC) { TAvlRotate(ctx, 16, 12); }
// (?, Node** pp): the +12 child up, heights fixed
DECOMP(0078A028, Avl_RotateR_0078A028) { TAvlRotate(ctx, 12, 16); }
// (?, Node** pp): the +16 child up, heights fixed
DECOMP(0078A170, Avl_RotateL_0078A170) { TAvlRotate(ctx, 16, 12); }
// (?, Node** pp): the +12 child up, heights fixed
DECOMP(0078C178, Avl_RotateR_0078C178) { TAvlRotate(ctx, 12, 16); }
// (?, Node** pp): the +16 child up, heights fixed
DECOMP(0078C2C0, Avl_RotateL_0078C2C0) { TAvlRotate(ctx, 16, 12); }
// (?, Node** pp): the +12 child up, heights fixed
DECOMP(0078E2C8, Avl_RotateR_0078E2C8) { TAvlRotate(ctx, 12, 16); }
// (?, Node** pp): the +16 child up, heights fixed
DECOMP(0078E410, Avl_RotateL_0078E410) { TAvlRotate(ctx, 16, 12); }
// (?, Node** pp): the +12 child up, heights fixed
DECOMP(0078F180, Avl_RotateR_0078F180) { TAvlRotate(ctx, 12, 16); }
// (?, Node** pp): the +16 child up, heights fixed
DECOMP(0078F2C8, Avl_RotateL_0078F2C8) { TAvlRotate(ctx, 16, 12); }
// (?, Node** pp): the +12 child up, heights fixed
DECOMP(00796A80, Avl_RotateR_00796A80) { TAvlRotate(ctx, 12, 16); }
// (?, Node** pp): the +16 child up, heights fixed
DECOMP(00796BC8, Avl_RotateL_00796BC8) { TAvlRotate(ctx, 16, 12); }
// (?, Node** pp): the +12 child up, heights fixed
DECOMP(00797C70, Avl_RotateR_00797C70) { TAvlRotate(ctx, 12, 16); }
// (?, Node** pp): the +16 child up, heights fixed
DECOMP(00797DB8, Avl_RotateL_00797DB8) { TAvlRotate(ctx, 16, 12); }

// ---------------------------------------------------------------- lookups of the object under a global key
// key = *0x00F64CF0; p = *0x0076611C(0x0106C228, &key); p && (p->+4 & 0x2) ? p : 0
DECOMP(007721D8, FindFlagged_007721D8) { TFindFlagged(ctx, 0x00F64CF0, 0x0076611C, 0x0106C228, 0x2); }
// key = *0x00F64CF4; p = *0x0076611C(0x0106C228, &key); p && (p->+4 & 0x2) ? p : 0
DECOMP(00772258, FindFlagged_00772258) { TFindFlagged(ctx, 0x00F64CF4, 0x0076611C, 0x0106C228, 0x2); }
// key = *0x00F64CFC; p = *0x0076611C(0x0106C228, &key); p && (p->+4 & 0x2) ? p : 0
DECOMP(007722D8, FindFlagged_007722D8) { TFindFlagged(ctx, 0x00F64CFC, 0x0076611C, 0x0106C228, 0x2); }
// key = *0x00F64D00; p = *0x0076611C(0x0106C228, &key); p && (p->+4 & 0x2) ? p : 0
DECOMP(00772358, FindFlagged_00772358) { TFindFlagged(ctx, 0x00F64D00, 0x0076611C, 0x0106C228, 0x2); }
// key = *0x00F64CF8; p = *0x0076611C(0x0106C228, &key); p && (p->+4 & 0x2) ? p : 0
DECOMP(007723D8, FindFlagged_007723D8) { TFindFlagged(ctx, 0x00F64CF8, 0x0076611C, 0x0106C228, 0x2); }
// key = *0x00F64D04; p = *0x0076611C(0x0106C228, &key); p && (p->+4 & 0x40) ? p : 0
DECOMP(00772458, FindFlagged_00772458) { TFindFlagged(ctx, 0x00F64D04, 0x0076611C, 0x0106C228, 0x40); }
// key = *0x00F64D2C; p = *0x0076611C(0x0106C228, &key); p && (p->+4 & 0x400) ? p : 0
DECOMP(007724D8, FindFlagged_007724D8) { TFindFlagged(ctx, 0x00F64D2C, 0x0076611C, 0x0106C228, 0x400); }
// key = *0x00F64D30; p = *0x0076611C(0x0106C228, &key); p && (p->+4 & 0x400) ? p : 0
DECOMP(00772558, FindFlagged_00772558) { TFindFlagged(ctx, 0x00F64D30, 0x0076611C, 0x0106C228, 0x400); }
// key = *0x00F64D34; p = *0x0076611C(0x0106C228, &key); p && (p->+4 & 0x400) ? p : 0
DECOMP(007725D8, FindFlagged_007725D8) { TFindFlagged(ctx, 0x00F64D34, 0x0076611C, 0x0106C228, 0x400); }
// key = *0x00F64D38; p = *0x0076611C(0x0106C228, &key); p && (p->+4 & 0x400) ? p : 0
DECOMP(00772658, FindFlagged_00772658) { TFindFlagged(ctx, 0x00F64D38, 0x0076611C, 0x0106C228, 0x400); }
// key = *0x00F64D48; p = *0x0076611C(0x0106C228, &key); p && (p->+4 & 0x10000) ? p : 0
DECOMP(007726D8, FindFlagged_007726D8) { TFindFlagged(ctx, 0x00F64D48, 0x0076611C, 0x0106C228, 0x10000); }
// key = *0x00F64D4C; p = *0x0076611C(0x0106C228, &key); p && (p->+4 & 0x20000) ? p : 0
DECOMP(00772758, FindFlagged_00772758) { TFindFlagged(ctx, 0x00F64D4C, 0x0076611C, 0x0106C228, 0x20000); }

// ---------------------------------------------------------------- 4x3 block copies
// this->+128[4x3] = a1->+0[4x3]
DECOMP(00012DE0, CopyMat43_00012DE0) { TCopyMat43(ctx, 128, 0); }
// this->+0[4x3] = a1->+32[4x3]
DECOMP(00734134, CopyMat43_00734134) { TCopyMat43(ctx, 0, 32); }
// this->+32[4x3] = a1->+0[4x3]
DECOMP(00734D2C, CopyMat43_00734D2C) { TCopyMat43(ctx, 32, 0); }
// this->+96[4x3] = a1->+0[4x3]
DECOMP(007435D0, CopyMat43_007435D0) { TCopyMat43(ctx, 96, 0); }
// this->+0[4x3] = a1->+0[4x3]
DECOMP(00748BB8, CopyMat43_00748BB8) { TCopyMat43(ctx, 0, 0); }
// this->+16[4x3] = a1->+0[4x3]
DECOMP(0074CB80, CopyMat43_0074CB80) { TCopyMat43(ctx, 16, 0); }
// this->+16[4x3] = a1->+0[4x3]
DECOMP(0074CD7C, CopyMat43_0074CD7C) { TCopyMat43(ctx, 16, 0); }
// this->+352[4x3] = a1->+0[4x3]
DECOMP(00760DBC, CopyMat43_00760DBC) { TCopyMat43(ctx, 352, 0); }
// this->+0[4x3] = a1->+0[4x3]
DECOMP(0076FFFC, CopyMat43_0076FFFC) { TCopyMat43(ctx, 0, 0); }
