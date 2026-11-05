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

// ---------------------------------------------------------------- global clears, then a call; returns 1
// *(u32*)0x01068720 = 0; 0x00569388(a0, 0); returns 1
DECOMP(002E9E84, ClearGlobal_002E9E84) { TClearGlobalCall(ctx, 0x01068720, 0x01070000, 0x00569388); }
// *(u32*)0x00F65790 = 0; 0x00569388(a0, 0); returns 1
DECOMP(00391460, ClearGlobal_00391460) { TClearGlobalCall(ctx, 0x00F65790, 0x00F60000, 0x00569388); }
// *(u32*)0x00F65D30 = 0; 0x00569388(a0, 0); returns 1
DECOMP(0048C15C, ClearGlobal_0048C15C) { TClearGlobalCall(ctx, 0x00F65D30, 0x00F60000, 0x00569388); }
// *(u32*)0x00F65D34 = 0; 0x00569388(a0, 0); returns 1
DECOMP(0048C388, ClearGlobal_0048C388) { TClearGlobalCall(ctx, 0x00F65D34, 0x00F60000, 0x00569388); }
// *(u32*)0x00F65FEC = 0; 0x00569388(a0, 0); returns 1
DECOMP(004B2B18, ClearGlobal_004B2B18) { TClearGlobalCall(ctx, 0x00F65FEC, 0x00F60000, 0x00569388); }
// *(u32*)0x00F668D4 = 0; 0x00569388(a0, 0); returns 1
DECOMP(004F4F58, ClearGlobal_004F4F58) { TClearGlobalCall(ctx, 0x00F668D4, 0x00F60000, 0x00569388); }
// *(u32*)0x00F668D4 = 0; 0x00569388(a0, 0); returns 1
DECOMP(004F51C0, ClearGlobal_004F51C0) { TClearGlobalCall(ctx, 0x00F668D4, 0x00F60000, 0x00569388); }

// ---------------------------------------------------------------- destructors releasing a member object
// *this = vtable 0x00B12048; 0x007D4EA8(+240); *this = 0x00B11D98; 0x000C2DA0(this)
DECOMP(000C0F08, VtB12048_Dtor) { TDtorRelease(ctx, 0x00B12048, 240, 0x007D4EA8, 0x00B11D98, 0x000C2DA0); }
// *this = vtable 0x00B14360; 0x007D4EA8(+268); *this = 0x00B11D98; 0x000C2DA0(this)
DECOMP(001667DC, VtB14360_Dtor) { TDtorRelease(ctx, 0x00B14360, 268, 0x007D4EA8, 0x00B11D98, 0x000C2DA0); }
// *this = vtable 0x00B14590; 0x007D4EA8(+244); *this = 0x00B11D98; 0x000C2DA0(this)
DECOMP(0016A740, VtB14590_Dtor) { TDtorRelease(ctx, 0x00B14590, 244, 0x007D4EA8, 0x00B11D98, 0x000C2DA0); }
// *this = vtable 0x00B14918; 0x007D4EA8(+252); *this = 0x00B11D98; 0x000C2DA0(this)
DECOMP(001762F8, VtB14918_Dtor) { TDtorRelease(ctx, 0x00B14918, 252, 0x007D4EA8, 0x00B11D98, 0x000C2DA0); }
// *this = vtable 0x00B14B50; 0x007D4EA8(+288); *this = 0x00B11D98; 0x000C2DA0(this)
DECOMP(0017CC7C, VtB14B50_Dtor) { TDtorRelease(ctx, 0x00B14B50, 288, 0x007D4EA8, 0x00B11D98, 0x000C2DA0); }
// *this = vtable 0x00B18B00; 0x007D4EA8(+296); *this = 0x00B11D98; 0x000C2DA0(this)
DECOMP(001F8F8C, VtB18B00_Dtor) { TDtorRelease(ctx, 0x00B18B00, 296, 0x007D4EA8, 0x00B11D98, 0x000C2DA0); }
// *this = vtable 0x00B18B00; 0x007D4EA8(+296); *this = 0x00B11D98; 0x000C2DA0(this)
DECOMP(00757A3C, VtB18B00_Dtor_00757A3C) { TDtorRelease(ctx, 0x00B18B00, 296, 0x007D4EA8, 0x00B11D98, 0x000C2DA0); }

// ---------------------------------------------------------------- their deleting versions
// deleting: vtable 0x00B12048; 0x007D4EA8(+240); 0x000C2DA0(this); 0x00552420(this, 272)
DECOMP(000C0F68, VtB12048_Delete) { TDeleteRelease(ctx, 0x00B12048, 240, 0x007D4EA8, 0x00B11D98, 0x000C2DA0, 0x00552420, 272); }
// deleting: vtable 0x00B14360; 0x007D4EA8(+268); 0x000C2DA0(this); 0x00552420(this, 288)
DECOMP(0016683C, VtB14360_Delete) { TDeleteRelease(ctx, 0x00B14360, 268, 0x007D4EA8, 0x00B11D98, 0x000C2DA0, 0x00552420, 288); }
// deleting: vtable 0x00B14590; 0x007D4EA8(+244); 0x000C2DA0(this); 0x00552420(this, 272)
DECOMP(0016A7A0, VtB14590_Delete) { TDeleteRelease(ctx, 0x00B14590, 244, 0x007D4EA8, 0x00B11D98, 0x000C2DA0, 0x00552420, 272); }
// deleting: vtable 0x00B14918; 0x007D4EA8(+252); 0x000C2DA0(this); 0x0054EE6C(this, 0)
DECOMP(00176358, VtB14918_Delete) { TDeleteRelease(ctx, 0x00B14918, 252, 0x007D4EA8, 0x00B11D98, 0x000C2DA0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B14B50; 0x007D4EA8(+288); 0x000C2DA0(this); 0x00552420(this, 304)
DECOMP(0017CCDC, VtB14B50_Delete) { TDeleteRelease(ctx, 0x00B14B50, 288, 0x007D4EA8, 0x00B11D98, 0x000C2DA0, 0x00552420, 304); }
// deleting: vtable 0x00B18B00; 0x007D4EA8(+296); 0x000C2DA0(this); 0x0054EE6C(this, 0)
DECOMP(001F8FEC, VtB18B00_Delete) { TDeleteRelease(ctx, 0x00B18B00, 296, 0x007D4EA8, 0x00B11D98, 0x000C2DA0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B18B00; 0x007D4EA8(+296); 0x000C2DA0(this); 0x00552420(this, 464)
DECOMP(00758A3C, VtB18B00_Delete_00758A3C) { TDeleteRelease(ctx, 0x00B18B00, 296, 0x007D4EA8, 0x00B11D98, 0x000C2DA0, 0x00552420, 464); }

// ---------------------------------------------------------------- destructors: a member handed to fn(q, vt), then the base
// *this = vtable 0x00B0F5E0; 0x004432DC(this->+20); 0x004BB41C(this)
DECOMP(00074118, VtB0F5E0_Dtor) { TDtorCallMember(ctx, 0x00B0F5E0, 20, 0x004432DC, 0x004BB41C); }
// *this = vtable 0x00B212C8; 0x004432DC(this->+12); 0x0042CA50(this)
DECOMP(00438C0C, VtB212C8_Dtor) { TDtorCallMember(ctx, 0x00B212C8, 12, 0x004432DC, 0x0042CA50); }
// *this = vtable 0x00B111E0; 0x004432DC(this->+20); 0x004BB41C(this)
DECOMP(00740A20, VtB111E0_Dtor) { TDtorCallMember(ctx, 0x00B111E0, 20, 0x004432DC, 0x004BB41C); }
// *this = vtable 0x00B114D8; 0x004432DC(this->+92); 0x004BB41C(this)
DECOMP(00741464, VtB114D8_Dtor) { TDtorCallMember(ctx, 0x00B114D8, 92, 0x004432DC, 0x004BB41C); }
// *this = vtable 0x00B115F8; 0x004432DC(this->+24); 0x004BB41C(this)
DECOMP(00741908, VtB115F8_Dtor) { TDtorCallMember(ctx, 0x00B115F8, 24, 0x004432DC, 0x004BB41C); }
// *this = vtable 0x00B11918; 0x004432DC(this->+20); 0x004BB41C(this)
DECOMP(0074230C, VtB11918_Dtor) { TDtorCallMember(ctx, 0x00B11918, 20, 0x004432DC, 0x004BB41C); }

// ---------------------------------------------------------------- their deleting versions
// deleting: vtable 0x00B0F5E0; 0x004432DC(this->+20); 0x004BB41C(this); 0x0054EE6C(this, 0)
DECOMP(00074164, VtB0F5E0_Delete) { TDeleteCallMember(ctx, 0x00B0F5E0, 20, 0x004432DC, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B212C8; 0x004432DC(this->+12); 0x0042CA50(this); 0x00552420(this, 20)
DECOMP(00438C58, VtB212C8_Delete) { TDeleteCallMember(ctx, 0x00B212C8, 12, 0x004432DC, 0x0042CA50, 0x00552420, 20); }
// deleting: vtable 0x00B111E0; 0x004432DC(this->+20); 0x004BB41C(this); 0x0054EE6C(this, 0)
DECOMP(00740A6C, VtB111E0_Delete) { TDeleteCallMember(ctx, 0x00B111E0, 20, 0x004432DC, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B114D8; 0x004432DC(this->+92); 0x004BB41C(this); 0x0054EE6C(this, 0)
DECOMP(007414B0, VtB114D8_Delete) { TDeleteCallMember(ctx, 0x00B114D8, 92, 0x004432DC, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B115F8; 0x004432DC(this->+24); 0x004BB41C(this); 0x0054EE6C(this, 0)
DECOMP(00741954, VtB115F8_Delete) { TDeleteCallMember(ctx, 0x00B115F8, 24, 0x004432DC, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11918; 0x004432DC(this->+20); 0x004BB41C(this); 0x0054EE6C(this, 0)
DECOMP(00742358, VtB11918_Delete) { TDeleteCallMember(ctx, 0x00B11918, 20, 0x004432DC, 0x004BB41C, 0x0054EE6C, 0); }

// ---------------------------------------------------------------- global flag-bit tests
// !(bit 30 of *(*0x00F65CA0 + 0x001283BC))
DECOMP(0042F240, GlobalBitClear_0042F240) { TGlobalBitClear(ctx, 0x00F65CA0, 0x001283BC, 2); }
// !(bit 31 of *(*0x00F65CA0 + 0x001282BC))
DECOMP(0042F398, GlobalBitClear_0042F398) { TGlobalBitClear(ctx, 0x00F65CA0, 0x001282BC, 1); }
// !(bit 29 of *(*0x00F65CA0 + 0x00128304))
DECOMP(0042F564, GlobalBitClear_0042F564) { TGlobalBitClear(ctx, 0x00F65CA0, 0x00128304, 3); }
// !(bit 30 of *(*0x00F65CA0 + 0x00128474))
DECOMP(00433914, GlobalBitClear_00433914) { TGlobalBitClear(ctx, 0x00F65CA0, 0x00128474, 2); }
// !(bit 30 of *(*0x00F65CA0 + 0x001282A0))
DECOMP(004342B8, GlobalBitClear_004342B8) { TGlobalBitClear(ctx, 0x00F65CA0, 0x001282A0, 2); }

// ---------------------------------------------------------------- virtual wrappers: this->vfn[k](this)
// this->vfn[95](this)
DECOMP(0018CDDC, VCall380_0018CDDC) { TVCallThis(ctx, 380); }
// this->vfn[95](this)
DECOMP(001962A8, VCall380_001962A8) { TVCallThis(ctx, 380); }
// this->vfn[63](this)
DECOMP(001E40E0, VCall252_001E40E0) { TVCallThis(ctx, 252); }
// this->vfn[7](this)
DECOMP(00749728, VCall28_00749728) { TVCallThis(ctx, 28); }
// this->vfn[8](this)
DECOMP(00795D70, VCall32_00795D70) { TVCallThis(ctx, 32); }

// ---------------------------------------------------------------- Vec2 setters followed by a virtual update
// this->+168[0..1] = a1[0..1]; this->vfn[37](this)
DECOMP(007960C8, SetVec2_A8_007960C8) { TSetVec2VCall(ctx, 168, 148); }
// this->+192[0..1] = a1[0..1]; this->vfn[37](this)
DECOMP(007962A8, SetVec2_C0_007962A8) { TSetVec2VCall(ctx, 192, 148); }
// this->+168[0..1] = a1[0..1]; this->vfn[37](this)
DECOMP(00796408, SetVec2_A8_00796408) { TSetVec2VCall(ctx, 168, 148); }
// this->+240[0..1] = a1[0..1]; this->vfn[37](this)
DECOMP(0079654C, SetVec2_F0_0079654C) { TSetVec2VCall(ctx, 240, 148); }
// this->+168[0..1] = a1[0..1]; this->vfn[37](this)
DECOMP(007966AC, SetVec2_A8_007966AC) { TSetVec2VCall(ctx, 168, 148); }
// this->+168[0..1] = a1[0..1]; this->vfn[37](this)
DECOMP(007968A8, SetVec2_A8_007968A8) { TSetVec2VCall(ctx, 168, 148); }

// ---------------------------------------------------------------- keyed lookups with a flag test
// key ? (p = *0x0076611C(0x0106C228, &key)) && (p->+4 & 0x8000) ? p : 0 : 0
DECOMP(00368A48, FindFlagged_00368A48) { TFindFlaggedArg(ctx, 0x0076611C, 0x0106C228, 0x8000); }
// key ? (p = *0x0076611C(0x0106C228, &key)) && (p->+4 & 0x4000) ? p : 0 : 0
DECOMP(00368DB0, FindFlagged_00368DB0) { TFindFlaggedArg(ctx, 0x0076611C, 0x0106C228, 0x4000); }
// key ? (p = *0x0076611C(0x0106C228, &key)) && (p->+4 & 0x800) ? p : 0 : 0
DECOMP(00368E2C, FindFlagged_00368E2C) { TFindFlaggedArg(ctx, 0x0076611C, 0x0106C228, 0x800); }
// key ? (p = *0x0076611C(0x0106C228, &key)) && (p->+4 & 0x1000) ? p : 0 : 0
DECOMP(00368EA8, FindFlagged_00368EA8) { TFindFlaggedArg(ctx, 0x0076611C, 0x0106C228, 0x1000); }
// key ? (p = *0x0076611C(0x0106C228, &key)) && (p->+4 & 0x2000) ? p : 0 : 0
DECOMP(00368F24, FindFlagged_00368F24) { TFindFlaggedArg(ctx, 0x0076611C, 0x0106C228, 0x2000); }
// key ? (p = *0x0076611C(0x0106C228, &key)) && (p->+4 & 0x4) ? p : 0 : 0
DECOMP(00368FA0, FindFlagged_00368FA0) { TFindFlaggedArg(ctx, 0x0076611C, 0x0106C228, 0x4); }

// ---------------------------------------------------------------- node record floats
// (Node* n) -> f1 (byte +2 of the sub-block indexes the floats)
DECOMP(00774EB0, NodeF32_00774EB0) { TNodeF32(ctx, 2); }
// (Node* n) -> f1 (byte +3 of the sub-block indexes the floats)
DECOMP(00774F44, NodeF32_00774F44) { TNodeF32(ctx, 3); }
// (Node* n) -> f1 (byte +4 of the sub-block indexes the floats)
DECOMP(00774FD8, NodeF32_00774FD8) { TNodeF32(ctx, 4); }
// (Node* n) -> f1 (byte +12 of the sub-block indexes the floats)
DECOMP(0077506C, NodeF32_0077506C) { TNodeF32(ctx, 12); }
// (Node* n) -> f1 (byte +13 of the sub-block indexes the floats)
DECOMP(00775100, NodeF32_00775100) { TNodeF32(ctx, 13); }
// (Node* n) -> f1 (byte +14 of the sub-block indexes the floats)
DECOMP(00775194, NodeF32_00775194) { TNodeF32(ctx, 14); }

// ---------------------------------------------------------------- destructors freeing a field buffer
// *this = vtable 0x00B23070; Mem_FreeFrom(this->+4, 0), cleared
DECOMP(004F85F0, VtB23070_Dtor) { TDtorFreeField(ctx, 0x00B23070, 4); }
// *this = vtable 0x00B1D434; Mem_FreeFrom(this->+12, 0), cleared
DECOMP(00766B6C, VtB1D434_Dtor) { TDtorFreeField(ctx, 0x00B1D434, 12); }
// *this = vtable 0x00B1D434; Mem_FreeFrom(this->+12, 0), cleared
DECOMP(0076716C, VtB1D434_Dtor_0076716C) { TDtorFreeField(ctx, 0x00B1D434, 12); }
// *this = vtable 0x00B1D434; Mem_FreeFrom(this->+12, 0), cleared
DECOMP(0076722C, VtB1D434_Dtor_0076722C) { TDtorFreeField(ctx, 0x00B1D434, 12); }
// *this = vtable 0x00B1D434; Mem_FreeFrom(this->+12, 0), cleared
DECOMP(00784BC4, VtB1D434_Dtor_00784BC4) { TDtorFreeField(ctx, 0x00B1D434, 12); }

// ---------------------------------------------------------------- their deleting versions
// deleting: vtable 0x00B23070; Mem_FreeFrom(this->+4, 0); 0x0054EE6C(this, 0)
DECOMP(004F8644, VtB23070_Delete) { TDeleteFreeField(ctx, 0x00B23070, 4, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1D434; Mem_FreeFrom(this->+12, 0); 0x0054EE6C(this, 0)
DECOMP(00766BC0, VtB1D434_Delete) { TDeleteFreeField(ctx, 0x00B1D434, 12, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1D434; Mem_FreeFrom(this->+12, 0); 0x0054EE6C(this, 0)
DECOMP(007671C0, VtB1D434_Delete_007671C0) { TDeleteFreeField(ctx, 0x00B1D434, 12, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1D434; Mem_FreeFrom(this->+12, 0); 0x0054EE6C(this, 0)
DECOMP(00767280, VtB1D434_Delete_00767280) { TDeleteFreeField(ctx, 0x00B1D434, 12, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1D434; Mem_FreeFrom(this->+12, 0); 0x00552420(this, 56)
DECOMP(00784C18, VtB1D434_Delete_00784C18) { TDeleteFreeField(ctx, 0x00B1D434, 12, 0x00552420, 56); }

// ---------------------------------------------------------------- derived-object refreshes
// this->+12 set and !0x00446960(+20): 0x004432DC(+20), +20 = 0x00446984(+12, +16)
DECOMP(000740B0, Remake_000740B0) { TRemake(ctx, 12, 0x00446960, 0x004432DC, 0x00446984); }
// this->+12 set and !0x00446960(+20): 0x004432DC(+20), +20 = 0x00446984(+12, +16)
DECOMP(0009CF8C, Remake_0009CF8C) { TRemake(ctx, 12, 0x00446960, 0x004432DC, 0x00446984); }
// this->+84 set and !0x00446960(+92): 0x004432DC(+92), +92 = 0x00446984(+84, +88)
DECOMP(000A04E4, Remake_000A04E4) { TRemake(ctx, 84, 0x00446960, 0x004432DC, 0x00446984); }
// this->+16 set and !0x00446960(+24): 0x004432DC(+24), +24 = 0x00446984(+16, +20)
DECOMP(000A2500, Remake_000A2500) { TRemake(ctx, 16, 0x00446960, 0x004432DC, 0x00446984); }
// this->+12 set and !0x00446960(+20): 0x004432DC(+20), +20 = 0x00446984(+12, +16)
DECOMP(000A555C, Remake_000A555C) { TRemake(ctx, 12, 0x00446960, 0x004432DC, 0x00446984); }

// ---------------------------------------------------------------- float field tests against a constant
// any (this->+216)->{212, 196, 204, 224, 232} != *0x001687A8
DECOMP(001687B0, AnyF32Ne_001687B0) { TAnyF32Ne(ctx, 0x001687A8, {212, 196, 204, 224, 232}); }
// any (this->+216)->{212, 196, 204, 224, 232} != *0x0016F4F0
DECOMP(0016F4F8, AnyF32Ne_0016F4F8) { TAnyF32Ne(ctx, 0x0016F4F0, {212, 196, 204, 224, 232}); }
// any (this->+216)->{212, 196, 204, 224, 232} != *0x00184CB8
DECOMP(00184CC0, AnyF32Ne_00184CC0) { TAnyF32Ne(ctx, 0x00184CB8, {212, 196, 204, 224, 232}); }
// any (this->+216)->{212, 196, 204, 224, 232} != *0x001925A8
DECOMP(001925B0, AnyF32Ne_001925B0) { TAnyF32Ne(ctx, 0x001925A8, {212, 196, 204, 224, 232}); }
// any (this->+216)->{248, 232, 240, 260, 268} != *0x001FB880
DECOMP(001FB888, AnyF32Ne_001FB888) { TAnyF32Ne(ctx, 0x001FB880, {248, 232, 240, 260, 268}); }

// ---------------------------------------------------------------- wrappers narrowing a byte argument (b)
// 0x000ED35C(this, (u8)a1)
DECOMP(000F4070, ThunkU8_000F4070) { TThunkU8(ctx, 0x000ED35C); }
// 0x000ED35C(this, (u8)a1)
DECOMP(000F51D4, ThunkU8_000F51D4) { TThunkU8(ctx, 0x000ED35C); }
// 0x001EE680(this, (u8)a1)
DECOMP(001EE964, ThunkU8_001EE964) { TThunkU8(ctx, 0x001EE680); }
// 0x0020D5D4(this, (u8)a1)
DECOMP(00220DF0, ThunkU8_00220DF0) { TThunkU8(ctx, 0x0020D5D4); }

// ---------------------------------------------------------------- wrappers passing a constant
// 0x005534D0(0x00B0DDA0)
DECOMP(0001F2F8, CallK_0001F2F8) { TCallK(ctx, 0x005534D0, 0x00B0DDA0); }
// 0x005534D0(0x00B0DDA0)
DECOMP(00024FCC, CallK_00024FCC) { TCallK(ctx, 0x005534D0, 0x00B0DDA0); }
// 0x005534D0(0x00B0DDA0)
DECOMP(0003352C, CallK_0003352C) { TCallK(ctx, 0x005534D0, 0x00B0DDA0); }
// 0x005534D0(0x00B0DDA0)
DECOMP(00738574, CallK_00738574) { TCallK(ctx, 0x005534D0, 0x00B0DDA0); }

// ---------------------------------------------------------------- base destructors through a secondary base (three vtables)
// p = this -384: vtables 0x00B126B8 / 0x00B12528 / 0x00B1200C at +0 / +384 / +380, then 0x000DF934(p)
DECOMP(00168350, AdjDtor_00168350) { TAdjDtorVt3b(ctx, -384, 0x00B126B8, 0x00B12528, 0x00B1200C, 0x000DF934); }
// p = this -380: vtables 0x00B126B8 / 0x00B12528 / 0x00B1200C at +0 / +384 / +380, then 0x000DF934(p)
DECOMP(001683A0, AdjDtor_001683A0) { TAdjDtorVt3b(ctx, -380, 0x00B126B8, 0x00B12528, 0x00B1200C, 0x000DF934); }
// p = this -384: vtables 0x00B126B8 / 0x00B12528 / 0x00B1200C at +0 / +384 / +380, then 0x000DF934(p)
DECOMP(0075919C, AdjDtor_0075919C) { TAdjDtorVt3b(ctx, -384, 0x00B126B8, 0x00B12528, 0x00B1200C, 0x000DF934); }
// p = this -380: vtables 0x00B126B8 / 0x00B12528 / 0x00B1200C at +0 / +384 / +380, then 0x000DF934(p)
DECOMP(007591EC, AdjDtor_007591EC) { TAdjDtorVt3b(ctx, -380, 0x00B126B8, 0x00B12528, 0x00B1200C, 0x000DF934); }
