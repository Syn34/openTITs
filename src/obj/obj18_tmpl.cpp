// World objects, part 18: the template instances of 0x00730000-0x007A0000.
// The survey (normalised disassembly differing only in constants) found
// the same few bodies repeated for hundreds of classes: the destructors
// and deleting destructors (the class vtable stored, then the base
// destructor and / or the free), the sorted-map key comparators, tail
// wrappers, constant returns and empty virtuals. Each instance is one
// DECOMP line calling the shared helper with its constants; the names
// carry the class vtable (Vt<vtable>_Dtor / _Delete) where one is stored.
// Every instance was checked against the original in the PPC simulator.
#include "obj18.h"

using namespace sly4;
using namespace sly4::obj18;

namespace {

// ~T(): *this = T's vtable, then the base destructor (r4 = the vtable, as
// the original's call sequence leaves it).
void TDtor(PPUContext& ctx, u32 vt, u32 base) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt);
    call(ctx, base, {o, vt});
}

// The deleting destructor: nothing for a null this (r3 = 0), else ~T()
// and the free (Mem_FreeFrom(this, 0) or SmallFree(this, size)).
void TDelete(PPUContext& ctx, u32 vt, u32 base, u32 free_fn, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, vt);
    call(ctx, base, {o, vt});
    call(ctx, free_fn, {o, size});
}

// A destructor with nothing but the vtable store (r4 = the vtable left).
void TSetVtbl(PPUContext& ctx, u32 vt) {
    store<u32>(arg<u32>(ctx, 0), vt);
    ctx.gpr[4] = vt;
}

// The deleting destructor of such a class: r5 / r6 = this and the vtable
// as the original leaves them for the free; a null this leaves r4 = 0.
void TDeleteNoBase(PPUContext& ctx, u32 vt, u32 free_fn, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[4] = 0;
        return;
    }
    store<u32>(o, vt);
    call(ctx, free_fn, {o, size, o, vt});
}

// ~T() of the objects registered in a sorted map under the key at this +
// 4: the vtable, then remove(map, this + 4) (r5 / r6 = the map and the
// key's address, as the original leaves them).
void TDtorUnmap(PPUContext& ctx, u32 vt, u32 remove, u32 map) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, vt);
    call(ctx, remove, {map, o + 4, map, o + 4});
}

// Its deleting version (r5 / r6 = the vtable and the key's address).
void TDeleteUnmap(PPUContext& ctx, u32 vt, u32 remove, u32 map, u32 free_fn, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, vt);
    call(ctx, remove, {map, o + 4, vt, o + 4});
    call(ctx, free_fn, {o, size});
}

// s32 Compare(const u32* a, const u32* b): 1 / -1 / 0 for *a > / < / ==
// *b (unsigned). r4 / r5 / r6 = *a, the result and b, as the original
// leaves them.
void TCmpU32(PPUContext& ctx) {
    const u32 pa = arg<u32>(ctx, 0), pb = arg<u32>(ctx, 1);
    const u32 a = load<u32>(pa), b = load<u32>(pb);
    const s32 r = a > b ? 1 : (a < b ? -1 : 0);
    ctx.gpr[4] = a;
    ctx.gpr[5] = static_cast<u64>(static_cast<s64>(r));
    ctx.gpr[6] = pb;
    ret(ctx, r);
}

// A tail wrapper: the target with the same register arguments.
void TThunk(PPUContext& ctx, u32 target) {
    call(ctx, target, {ctx.gpr[3], ctx.gpr[4], ctx.gpr[5], ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], ctx.gpr[9], ctx.gpr[10]});
}

}  // namespace

// ---------------------------------------------------------------- D1 destructors: the vtable, then the base destructor (343)
// *this = vtable 0x00B0D934, then 0x004FC1A0(this)
DECOMP(00732D98, VtB0D934_Dtor) { TDtor(ctx, 0x00B0D934, 0x004FC1A0); }
// *this = vtable 0x00B0D99C, then 0x004FC1A0(this)
DECOMP(00732E08, VtB0D99C_Dtor) { TDtor(ctx, 0x00B0D99C, 0x004FC1A0); }
// *this = vtable 0x00B0D9D0, then 0x004FC1A0(this)
DECOMP(00732E38, VtB0D9D0_Dtor) { TDtor(ctx, 0x00B0D9D0, 0x004FC1A0); }
// *this = vtable 0x00B0DA04, then 0x004FC1A0(this)
DECOMP(00732E68, VtB0DA04_Dtor) { TDtor(ctx, 0x00B0DA04, 0x004FC1A0); }
// *this = vtable 0x00B0DA6C, then 0x004FC1A0(this)
DECOMP(00732E98, VtB0DA6C_Dtor) { TDtor(ctx, 0x00B0DA6C, 0x004FC1A0); }
// *this = vtable 0x00B0D968, then 0x004FC1A0(this)
DECOMP(00732EC8, VtB0D968_Dtor) { TDtor(ctx, 0x00B0D968, 0x004FC1A0); }
// *this = vtable 0x00B0DA38, then 0x004FC1A0(this)
DECOMP(00732EF8, VtB0DA38_Dtor) { TDtor(ctx, 0x00B0DA38, 0x004FC1A0); }
// *this = vtable 0x00B0DE40, then 0x00121E6C(this)
DECOMP(00733EFC, VtB0DE40_Dtor) { TDtor(ctx, 0x00B0DE40, 0x00121E6C); }
// *this = vtable 0x00B0DFD0, then 0x004F7108(this)
DECOMP(00733F2C, VtB0DFD0_Dtor) { TDtor(ctx, 0x00B0DFD0, 0x004F7108); }
// *this = vtable 0x00B0E010, then 0x004F79C0(this)
DECOMP(00733F5C, VtB0E010_Dtor) { TDtor(ctx, 0x00B0E010, 0x004F79C0); }
// *this = vtable 0x00B0E160, then 0x004F7108(this)
DECOMP(00734340, VtB0E160_Dtor) { TDtor(ctx, 0x00B0E160, 0x004F7108); }
// *this = vtable 0x00B0E1A0, then 0x004F79C0(this)
DECOMP(00734370, VtB0E1A0_Dtor) { TDtor(ctx, 0x00B0E1A0, 0x004F79C0); }
// *this = vtable 0x00B0E560, then 0x004F7108(this)
DECOMP(007346F0, VtB0E560_Dtor) { TDtor(ctx, 0x00B0E560, 0x004F7108); }
// *this = vtable 0x00B0E5A0, then 0x004F79C0(this)
DECOMP(00734720, VtB0E5A0_Dtor) { TDtor(ctx, 0x00B0E5A0, 0x004F79C0); }
// *this = vtable 0x00B0E6F0, then 0x004F7108(this)
DECOMP(00735360, VtB0E6F0_Dtor) { TDtor(ctx, 0x00B0E6F0, 0x004F7108); }
// *this = vtable 0x00B0E734, then 0x004F79C0(this)
DECOMP(00735390, VtB0E734_Dtor) { TDtor(ctx, 0x00B0E734, 0x004F79C0); }
// *this = vtable 0x00B0F1D0, then 0x004BB41C(this)
DECOMP(0073B5D8, VtB0F1D0_Dtor) { TDtor(ctx, 0x00B0F1D0, 0x004BB41C); }
// *this = vtable 0x00B0F1D0, then 0x004BB41C(this)
DECOMP(0073B800, VtB0F1D0_Dtor_0073B800) { TDtor(ctx, 0x00B0F1D0, 0x004BB41C); }
// *this = vtable 0x00B0F1D0, then 0x004BB41C(this)
DECOMP(0073B830, VtB0F1D0_Dtor_0073B830) { TDtor(ctx, 0x00B0F1D0, 0x004BB41C); }
// *this = vtable 0x00B0F1D0, then 0x004BB41C(this)
DECOMP(0073B860, VtB0F1D0_Dtor_0073B860) { TDtor(ctx, 0x00B0F1D0, 0x004BB41C); }
// *this = vtable 0x00B0F090, then 0x004BB41C(this)
DECOMP(0073BD68, VtB0F090_Dtor) { TDtor(ctx, 0x00B0F090, 0x004BB41C); }
// *this = vtable 0x00B0F180, then 0x004BB41C(this)
DECOMP(0073BDF0, VtB0F180_Dtor) { TDtor(ctx, 0x00B0F180, 0x004BB41C); }
// *this = vtable 0x00B0F1D0, then 0x004BB41C(this)
DECOMP(0073BE78, VtB0F1D0_Dtor_0073BE78) { TDtor(ctx, 0x00B0F1D0, 0x004BB41C); }
// *this = vtable 0x00B0F1D0, then 0x004BB41C(this)
DECOMP(0073BF00, VtB0F1D0_Dtor_0073BF00) { TDtor(ctx, 0x00B0F1D0, 0x004BB41C); }
// *this = vtable 0x00B0F1D0, then 0x004BB41C(this)
DECOMP(0073BF88, VtB0F1D0_Dtor_0073BF88) { TDtor(ctx, 0x00B0F1D0, 0x004BB41C); }
// *this = vtable 0x00B0F248, then 0x004BB41C(this)
DECOMP(0073C010, VtB0F248_Dtor) { TDtor(ctx, 0x00B0F248, 0x004BB41C); }
// *this = vtable 0x00B0F270, then 0x004BB41C(this)
DECOMP(0073C098, VtB0F270_Dtor) { TDtor(ctx, 0x00B0F270, 0x004BB41C); }
// *this = vtable 0x00B0F2E8, then 0x004BB41C(this)
DECOMP(0073C120, VtB0F2E8_Dtor) { TDtor(ctx, 0x00B0F2E8, 0x004BB41C); }
// *this = vtable 0x00B0F310, then 0x004BB41C(this)
DECOMP(0073C1A8, VtB0F310_Dtor) { TDtor(ctx, 0x00B0F310, 0x004BB41C); }
// *this = vtable 0x00B0F338, then 0x004BB41C(this)
DECOMP(0073C230, VtB0F338_Dtor) { TDtor(ctx, 0x00B0F338, 0x004BB41C); }
// *this = vtable 0x00B0F360, then 0x004BB41C(this)
DECOMP(0073C2B8, VtB0F360_Dtor) { TDtor(ctx, 0x00B0F360, 0x004BB41C); }
// *this = vtable 0x00B0F388, then 0x004BB41C(this)
DECOMP(0073C340, VtB0F388_Dtor) { TDtor(ctx, 0x00B0F388, 0x004BB41C); }
// *this = vtable 0x00B0F3D8, then 0x004BB41C(this)
DECOMP(0073C3C8, VtB0F3D8_Dtor) { TDtor(ctx, 0x00B0F3D8, 0x004BB41C); }
// *this = vtable 0x00B0F400, then 0x004BB41C(this)
DECOMP(0073C450, VtB0F400_Dtor) { TDtor(ctx, 0x00B0F400, 0x004BB41C); }
// *this = vtable 0x00B0F428, then 0x004BB41C(this)
DECOMP(0073C4D8, VtB0F428_Dtor) { TDtor(ctx, 0x00B0F428, 0x004BB41C); }
// *this = vtable 0x00B0F450, then 0x004BB41C(this)
DECOMP(0073C560, VtB0F450_Dtor) { TDtor(ctx, 0x00B0F450, 0x004BB41C); }
// *this = vtable 0x00B0F478, then 0x004BB41C(this)
DECOMP(0073C5E8, VtB0F478_Dtor) { TDtor(ctx, 0x00B0F478, 0x004BB41C); }
// *this = vtable 0x00B0F4A0, then 0x004BB41C(this)
DECOMP(0073C670, VtB0F4A0_Dtor) { TDtor(ctx, 0x00B0F4A0, 0x004BB41C); }
// *this = vtable 0x00B0F4C8, then 0x004BB41C(this)
DECOMP(0073C6F8, VtB0F4C8_Dtor) { TDtor(ctx, 0x00B0F4C8, 0x004BB41C); }
// *this = vtable 0x00B0F4F0, then 0x004BB41C(this)
DECOMP(0073C780, VtB0F4F0_Dtor) { TDtor(ctx, 0x00B0F4F0, 0x004BB41C); }
// *this = vtable 0x00B0F518, then 0x004BB41C(this)
DECOMP(0073C808, VtB0F518_Dtor) { TDtor(ctx, 0x00B0F518, 0x004BB41C); }
// *this = vtable 0x00B0F568, then 0x004BB41C(this)
DECOMP(0073C8E8, VtB0F568_Dtor) { TDtor(ctx, 0x00B0F568, 0x004BB41C); }
// *this = vtable 0x00B0F590, then 0x004BB41C(this)
DECOMP(0073C970, VtB0F590_Dtor) { TDtor(ctx, 0x00B0F590, 0x004BB41C); }
// *this = vtable 0x00B0F5B8, then 0x004BB41C(this)
DECOMP(0073CA28, VtB0F5B8_Dtor) { TDtor(ctx, 0x00B0F5B8, 0x004BB41C); }
// *this = vtable 0x00B0F630, then 0x004BB41C(this)
DECOMP(0073CAB0, VtB0F630_Dtor) { TDtor(ctx, 0x00B0F630, 0x004BB41C); }
// *this = vtable 0x00B0F658, then 0x004BB41C(this)
DECOMP(0073CB38, VtB0F658_Dtor) { TDtor(ctx, 0x00B0F658, 0x004BB41C); }
// *this = vtable 0x00B0F680, then 0x004BB41C(this)
DECOMP(0073CBC0, VtB0F680_Dtor) { TDtor(ctx, 0x00B0F680, 0x004BB41C); }
// *this = vtable 0x00B0F6A8, then 0x004BB41C(this)
DECOMP(0073CC48, VtB0F6A8_Dtor) { TDtor(ctx, 0x00B0F6A8, 0x004BB41C); }
// *this = vtable 0x00B0F6D0, then 0x004BB41C(this)
DECOMP(0073CCD0, VtB0F6D0_Dtor) { TDtor(ctx, 0x00B0F6D0, 0x004BB41C); }
// *this = vtable 0x00B0F6F8, then 0x004BB41C(this)
DECOMP(0073CD58, VtB0F6F8_Dtor) { TDtor(ctx, 0x00B0F6F8, 0x004BB41C); }
// *this = vtable 0x00B0F720, then 0x004BB41C(this)
DECOMP(0073CDE0, VtB0F720_Dtor) { TDtor(ctx, 0x00B0F720, 0x004BB41C); }
// *this = vtable 0x00B0F810, then 0x004BB41C(this)
DECOMP(0073CE68, VtB0F810_Dtor) { TDtor(ctx, 0x00B0F810, 0x004BB41C); }
// *this = vtable 0x00B0F888, then 0x004BB41C(this)
DECOMP(0073CEF0, VtB0F888_Dtor) { TDtor(ctx, 0x00B0F888, 0x004BB41C); }
// *this = vtable 0x00B0F8B0, then 0x004BB41C(this)
DECOMP(0073CF78, VtB0F8B0_Dtor) { TDtor(ctx, 0x00B0F8B0, 0x004BB41C); }
// *this = vtable 0x00B0F928, then 0x004BB41C(this)
DECOMP(0073D000, VtB0F928_Dtor) { TDtor(ctx, 0x00B0F928, 0x004BB41C); }
// *this = vtable 0x00B0F950, then 0x004BB41C(this)
DECOMP(0073D088, VtB0F950_Dtor) { TDtor(ctx, 0x00B0F950, 0x004BB41C); }
// *this = vtable 0x00B0FAE0, then 0x004BB41C(this)
DECOMP(0073D110, VtB0FAE0_Dtor) { TDtor(ctx, 0x00B0FAE0, 0x004BB41C); }
// *this = vtable 0x00B0FB08, then 0x004BB41C(this)
DECOMP(0073D198, VtB0FB08_Dtor) { TDtor(ctx, 0x00B0FB08, 0x004BB41C); }
// *this = vtable 0x00B0FB30, then 0x004BB41C(this)
DECOMP(0073D220, VtB0FB30_Dtor) { TDtor(ctx, 0x00B0FB30, 0x004BB41C); }
// *this = vtable 0x00B0FB58, then 0x004BB41C(this)
DECOMP(0073D2A8, VtB0FB58_Dtor) { TDtor(ctx, 0x00B0FB58, 0x004BB41C); }
// *this = vtable 0x00B0FB80, then 0x004BB41C(this)
DECOMP(0073D330, VtB0FB80_Dtor) { TDtor(ctx, 0x00B0FB80, 0x004BB41C); }
// *this = vtable 0x00B0FC70, then 0x004BB41C(this)
DECOMP(0073D3B8, VtB0FC70_Dtor) { TDtor(ctx, 0x00B0FC70, 0x004BB41C); }
// *this = vtable 0x00B0FC98, then 0x004BB41C(this)
DECOMP(0073D440, VtB0FC98_Dtor) { TDtor(ctx, 0x00B0FC98, 0x004BB41C); }
// *this = vtable 0x00B0FCE8, then 0x004BB41C(this)
DECOMP(0073D4C8, VtB0FCE8_Dtor) { TDtor(ctx, 0x00B0FCE8, 0x004BB41C); }
// *this = vtable 0x00B0FD10, then 0x004BB41C(this)
DECOMP(0073D550, VtB0FD10_Dtor) { TDtor(ctx, 0x00B0FD10, 0x004BB41C); }
// *this = vtable 0x00B0FD38, then 0x004BB41C(this)
DECOMP(0073D5D8, VtB0FD38_Dtor) { TDtor(ctx, 0x00B0FD38, 0x004BB41C); }
// *this = vtable 0x00B0FD60, then 0x004BB41C(this)
DECOMP(0073D660, VtB0FD60_Dtor) { TDtor(ctx, 0x00B0FD60, 0x004BB41C); }
// *this = vtable 0x00B0FD88, then 0x004BB41C(this)
DECOMP(0073D6E8, VtB0FD88_Dtor) { TDtor(ctx, 0x00B0FD88, 0x004BB41C); }
// *this = vtable 0x00B0FDB0, then 0x004BB41C(this)
DECOMP(0073D770, VtB0FDB0_Dtor) { TDtor(ctx, 0x00B0FDB0, 0x004BB41C); }
// *this = vtable 0x00B0FDD8, then 0x004BB41C(this)
DECOMP(0073D7F8, VtB0FDD8_Dtor) { TDtor(ctx, 0x00B0FDD8, 0x004BB41C); }
// *this = vtable 0x00B0FE00, then 0x004BB41C(this)
DECOMP(0073D880, VtB0FE00_Dtor) { TDtor(ctx, 0x00B0FE00, 0x004BB41C); }
// *this = vtable 0x00B0FE78, then 0x004BB41C(this)
DECOMP(0073D908, VtB0FE78_Dtor) { TDtor(ctx, 0x00B0FE78, 0x004BB41C); }
// *this = vtable 0x00B0FEA0, then 0x004BB41C(this)
DECOMP(0073D990, VtB0FEA0_Dtor) { TDtor(ctx, 0x00B0FEA0, 0x004BB41C); }
// *this = vtable 0x00B0FEC8, then 0x004BB41C(this)
DECOMP(0073DA18, VtB0FEC8_Dtor) { TDtor(ctx, 0x00B0FEC8, 0x004BB41C); }
// *this = vtable 0x00B0FF18, then 0x004BB41C(this)
DECOMP(0073DAA0, VtB0FF18_Dtor) { TDtor(ctx, 0x00B0FF18, 0x004BB41C); }
// *this = vtable 0x00B0F1D0, then 0x004BB41C(this)
DECOMP(0073DC10, VtB0F1D0_Dtor_0073DC10) { TDtor(ctx, 0x00B0F1D0, 0x004BB41C); }
// *this = vtable 0x00B0F1D0, then 0x004BB41C(this)
DECOMP(0073DD80, VtB0F1D0_Dtor_0073DD80) { TDtor(ctx, 0x00B0F1D0, 0x004BB41C); }
// *this = vtable 0x00B10008, then 0x004BB41C(this)
DECOMP(0073DE08, VtB10008_Dtor) { TDtor(ctx, 0x00B10008, 0x004BB41C); }
// *this = vtable 0x00B10058, then 0x004BB41C(this)
DECOMP(0073DE90, VtB10058_Dtor) { TDtor(ctx, 0x00B10058, 0x004BB41C); }
// *this = vtable 0x00B10080, then 0x004BB41C(this)
DECOMP(0073DF18, VtB10080_Dtor) { TDtor(ctx, 0x00B10080, 0x004BB41C); }
// *this = vtable 0x00B100D0, then 0x004BB41C(this)
DECOMP(0073DFA0, VtB100D0_Dtor) { TDtor(ctx, 0x00B100D0, 0x004BB41C); }
// *this = vtable 0x00B0F1D0, then 0x004BB41C(this)
DECOMP(0073E028, VtB0F1D0_Dtor_0073E028) { TDtor(ctx, 0x00B0F1D0, 0x004BB41C); }
// *this = vtable 0x00B10170, then 0x004BB41C(this)
DECOMP(0073E0B0, VtB10170_Dtor) { TDtor(ctx, 0x00B10170, 0x004BB41C); }
// *this = vtable 0x00B10198, then 0x004BB41C(this)
DECOMP(0073E138, VtB10198_Dtor) { TDtor(ctx, 0x00B10198, 0x004BB41C); }
// *this = vtable 0x00B0F1D0, then 0x004BB41C(this)
DECOMP(0073E1C0, VtB0F1D0_Dtor_0073E1C0) { TDtor(ctx, 0x00B0F1D0, 0x004BB41C); }
// *this = vtable 0x00B0F1D0, then 0x004BB41C(this)
DECOMP(0073E248, VtB0F1D0_Dtor_0073E248) { TDtor(ctx, 0x00B0F1D0, 0x004BB41C); }
// *this = vtable 0x00B102B0, then 0x004BB41C(this)
DECOMP(0073E2D0, VtB102B0_Dtor) { TDtor(ctx, 0x00B102B0, 0x004BB41C); }
// *this = vtable 0x00B102D8, then 0x004BB41C(this)
DECOMP(0073E358, VtB102D8_Dtor) { TDtor(ctx, 0x00B102D8, 0x004BB41C); }
// *this = vtable 0x00B10300, then 0x004BB41C(this)
DECOMP(0073E3E0, VtB10300_Dtor) { TDtor(ctx, 0x00B10300, 0x004BB41C); }
// *this = vtable 0x00B10328, then 0x004BB41C(this)
DECOMP(0073E468, VtB10328_Dtor) { TDtor(ctx, 0x00B10328, 0x004BB41C); }
// *this = vtable 0x00B10350, then 0x004BB41C(this)
DECOMP(0073E4F0, VtB10350_Dtor) { TDtor(ctx, 0x00B10350, 0x004BB41C); }
// *this = vtable 0x00B10378, then 0x004BB41C(this)
DECOMP(0073E578, VtB10378_Dtor) { TDtor(ctx, 0x00B10378, 0x004BB41C); }
// *this = vtable 0x00B103A0, then 0x004BB41C(this)
DECOMP(0073E600, VtB103A0_Dtor) { TDtor(ctx, 0x00B103A0, 0x004BB41C); }
// *this = vtable 0x00B103C8, then 0x004BB41C(this)
DECOMP(0073E688, VtB103C8_Dtor) { TDtor(ctx, 0x00B103C8, 0x004BB41C); }
// *this = vtable 0x00B103F0, then 0x004BB41C(this)
DECOMP(0073E710, VtB103F0_Dtor) { TDtor(ctx, 0x00B103F0, 0x004BB41C); }
// *this = vtable 0x00B10418, then 0x004BB41C(this)
DECOMP(0073E798, VtB10418_Dtor) { TDtor(ctx, 0x00B10418, 0x004BB41C); }
// *this = vtable 0x00B10440, then 0x004BB41C(this)
DECOMP(0073E820, VtB10440_Dtor) { TDtor(ctx, 0x00B10440, 0x004BB41C); }
// *this = vtable 0x00B10468, then 0x004BB41C(this)
DECOMP(0073E8A8, VtB10468_Dtor) { TDtor(ctx, 0x00B10468, 0x004BB41C); }
// *this = vtable 0x00B10490, then 0x004BB41C(this)
DECOMP(0073E930, VtB10490_Dtor) { TDtor(ctx, 0x00B10490, 0x004BB41C); }
// *this = vtable 0x00B104B8, then 0x004BB41C(this)
DECOMP(0073E9B8, VtB104B8_Dtor) { TDtor(ctx, 0x00B104B8, 0x004BB41C); }
// *this = vtable 0x00B0F1D0, then 0x004BB41C(this)
DECOMP(0073EA40, VtB0F1D0_Dtor_0073EA40) { TDtor(ctx, 0x00B0F1D0, 0x004BB41C); }
// *this = vtable 0x00B10530, then 0x004BB41C(this)
DECOMP(0073EAC8, VtB10530_Dtor) { TDtor(ctx, 0x00B10530, 0x004BB41C); }
// *this = vtable 0x00B0F1D0, then 0x004BB41C(this)
DECOMP(0073EB50, VtB0F1D0_Dtor_0073EB50) { TDtor(ctx, 0x00B0F1D0, 0x004BB41C); }
// *this = vtable 0x00B0F1D0, then 0x004BB41C(this)
DECOMP(0073EBD8, VtB0F1D0_Dtor_0073EBD8) { TDtor(ctx, 0x00B0F1D0, 0x004BB41C); }
// *this = vtable 0x00B105A8, then 0x004BB41C(this)
DECOMP(0073EC60, VtB105A8_Dtor) { TDtor(ctx, 0x00B105A8, 0x004BB41C); }
// *this = vtable 0x00B105F8, then 0x004BB41C(this)
DECOMP(0073ECE8, VtB105F8_Dtor) { TDtor(ctx, 0x00B105F8, 0x004BB41C); }
// *this = vtable 0x00B10620, then 0x004BB41C(this)
DECOMP(0073ED70, VtB10620_Dtor) { TDtor(ctx, 0x00B10620, 0x004BB41C); }
// *this = vtable 0x00B0F1D0, then 0x004BB41C(this)
DECOMP(0073EDF8, VtB0F1D0_Dtor_0073EDF8) { TDtor(ctx, 0x00B0F1D0, 0x004BB41C); }
// *this = vtable 0x00B0F1D0, then 0x004BB41C(this)
DECOMP(0073F000, VtB0F1D0_Dtor_0073F000) { TDtor(ctx, 0x00B0F1D0, 0x004BB41C); }
// *this = vtable 0x00B10738, then 0x004BB41C(this)
DECOMP(0073F088, VtB10738_Dtor) { TDtor(ctx, 0x00B10738, 0x004BB41C); }
// *this = vtable 0x00B10828, then 0x004BB41C(this)
DECOMP(0073F110, VtB10828_Dtor) { TDtor(ctx, 0x00B10828, 0x004BB41C); }
// *this = vtable 0x00B10850, then 0x004BB41C(this)
DECOMP(0073F198, VtB10850_Dtor) { TDtor(ctx, 0x00B10850, 0x004BB41C); }
// *this = vtable 0x00B108A0, then 0x004BB41C(this)
DECOMP(0073F220, VtB108A0_Dtor) { TDtor(ctx, 0x00B108A0, 0x004BB41C); }
// *this = vtable 0x00B108C8, then 0x004BB41C(this)
DECOMP(0073F2A8, VtB108C8_Dtor) { TDtor(ctx, 0x00B108C8, 0x004BB41C); }
// *this = vtable 0x00B108F0, then 0x004BB41C(this)
DECOMP(0073F330, VtB108F0_Dtor) { TDtor(ctx, 0x00B108F0, 0x004BB41C); }
// *this = vtable 0x00B10940, then 0x004BB41C(this)
DECOMP(0073F3B8, VtB10940_Dtor) { TDtor(ctx, 0x00B10940, 0x004BB41C); }
// *this = vtable 0x00B10990, then 0x004BB41C(this)
DECOMP(0073F440, VtB10990_Dtor) { TDtor(ctx, 0x00B10990, 0x004BB41C); }
// *this = vtable 0x00B109E0, then 0x004BB41C(this)
DECOMP(0073F4C8, VtB109E0_Dtor) { TDtor(ctx, 0x00B109E0, 0x004BB41C); }
// *this = vtable 0x00B10A08, then 0x004BB41C(this)
DECOMP(0073F550, VtB10A08_Dtor) { TDtor(ctx, 0x00B10A08, 0x004BB41C); }
// *this = vtable 0x00B10A30, then 0x004BB41C(this)
DECOMP(0073F5D8, VtB10A30_Dtor) { TDtor(ctx, 0x00B10A30, 0x004BB41C); }
// *this = vtable 0x00B10A58, then 0x004BB41C(this)
DECOMP(0073F660, VtB10A58_Dtor) { TDtor(ctx, 0x00B10A58, 0x004BB41C); }
// *this = vtable 0x00B10A80, then 0x004BB41C(this)
DECOMP(0073F6E8, VtB10A80_Dtor) { TDtor(ctx, 0x00B10A80, 0x004BB41C); }
// *this = vtable 0x00B10AD0, then 0x004BB41C(this)
DECOMP(0073F770, VtB10AD0_Dtor) { TDtor(ctx, 0x00B10AD0, 0x004BB41C); }
// *this = vtable 0x00B10B20, then 0x004BB41C(this)
DECOMP(0073F7F8, VtB10B20_Dtor) { TDtor(ctx, 0x00B10B20, 0x004BB41C); }
// *this = vtable 0x00B10B48, then 0x004BB41C(this)
DECOMP(0073F880, VtB10B48_Dtor) { TDtor(ctx, 0x00B10B48, 0x004BB41C); }
// *this = vtable 0x00B10B70, then 0x004BB41C(this)
DECOMP(0073F908, VtB10B70_Dtor) { TDtor(ctx, 0x00B10B70, 0x004BB41C); }
// *this = vtable 0x00B10BC0, then 0x004BB41C(this)
DECOMP(0073F990, VtB10BC0_Dtor) { TDtor(ctx, 0x00B10BC0, 0x004BB41C); }
// *this = vtable 0x00B10C38, then 0x004BB41C(this)
DECOMP(0073FA70, VtB10C38_Dtor) { TDtor(ctx, 0x00B10C38, 0x004BB41C); }
// *this = vtable 0x00B10C60, then 0x004BB41C(this)
DECOMP(0073FAF8, VtB10C60_Dtor) { TDtor(ctx, 0x00B10C60, 0x004BB41C); }
// *this = vtable 0x00B10C88, then 0x004BB41C(this)
DECOMP(0073FB80, VtB10C88_Dtor) { TDtor(ctx, 0x00B10C88, 0x004BB41C); }
// *this = vtable 0x00B10CB0, then 0x004BB41C(this)
DECOMP(0073FC08, VtB10CB0_Dtor) { TDtor(ctx, 0x00B10CB0, 0x004BB41C); }
// *this = vtable 0x00B10CD8, then 0x004BB41C(this)
DECOMP(0073FC90, VtB10CD8_Dtor) { TDtor(ctx, 0x00B10CD8, 0x004BB41C); }
// *this = vtable 0x00B10D00, then 0x004BB41C(this)
DECOMP(0073FD18, VtB10D00_Dtor) { TDtor(ctx, 0x00B10D00, 0x004BB41C); }
// *this = vtable 0x00B10D78, then 0x004BB41C(this)
DECOMP(0073FDA0, VtB10D78_Dtor) { TDtor(ctx, 0x00B10D78, 0x004BB41C); }
// *this = vtable 0x00B10DA0, then 0x004BB41C(this)
DECOMP(0073FE28, VtB10DA0_Dtor) { TDtor(ctx, 0x00B10DA0, 0x004BB41C); }
// *this = vtable 0x00B10E40, then 0x004BB41C(this)
DECOMP(0073FEB0, VtB10E40_Dtor) { TDtor(ctx, 0x00B10E40, 0x004BB41C); }
// *this = vtable 0x00B111B8, then 0x004BB41C(this)
DECOMP(0073FF38, VtB111B8_Dtor) { TDtor(ctx, 0x00B111B8, 0x004BB41C); }
// *this = vtable 0x00B11620, then 0x004BB41C(this)
DECOMP(0073FF68, VtB11620_Dtor) { TDtor(ctx, 0x00B11620, 0x004BB41C); }
// *this = vtable 0x00B11648, then 0x004BB41C(this)
DECOMP(0073FF98, VtB11648_Dtor) { TDtor(ctx, 0x00B11648, 0x004BB41C); }
// *this = vtable 0x00B11670, then 0x004BB41C(this)
DECOMP(0073FFC8, VtB11670_Dtor) { TDtor(ctx, 0x00B11670, 0x004BB41C); }
// *this = vtable 0x00B116C0, then 0x004BB41C(this)
DECOMP(00740060, VtB116C0_Dtor) { TDtor(ctx, 0x00B116C0, 0x004BB41C); }
// *this = vtable 0x00B11A80, then 0x004BB41C(this)
DECOMP(00740090, VtB11A80_Dtor) { TDtor(ctx, 0x00B11A80, 0x004BB41C); }
// *this = vtable 0x00B10F10, then 0x004BB41C(this)
DECOMP(007400C0, VtB10F10_Dtor) { TDtor(ctx, 0x00B10F10, 0x004BB41C); }
// *this = vtable 0x00B10F38, then 0x004BB41C(this)
DECOMP(00740148, VtB10F38_Dtor) { TDtor(ctx, 0x00B10F38, 0x004BB41C); }
// *this = vtable 0x00B10F60, then 0x004BB41C(this)
DECOMP(007401D0, VtB10F60_Dtor) { TDtor(ctx, 0x00B10F60, 0x004BB41C); }
// *this = vtable 0x00B10F88, then 0x004BB41C(this)
DECOMP(00740258, VtB10F88_Dtor) { TDtor(ctx, 0x00B10F88, 0x004BB41C); }
// *this = vtable 0x00B10FB0, then 0x004BB41C(this)
DECOMP(007402E0, VtB10FB0_Dtor) { TDtor(ctx, 0x00B10FB0, 0x004BB41C); }
// *this = vtable 0x00B10FD8, then 0x004BB41C(this)
DECOMP(00740368, VtB10FD8_Dtor) { TDtor(ctx, 0x00B10FD8, 0x004BB41C); }
// *this = vtable 0x00B11000, then 0x004BB41C(this)
DECOMP(007403F0, VtB11000_Dtor) { TDtor(ctx, 0x00B11000, 0x004BB41C); }
// *this = vtable 0x00B11028, then 0x004BB41C(this)
DECOMP(00740478, VtB11028_Dtor) { TDtor(ctx, 0x00B11028, 0x004BB41C); }
// *this = vtable 0x00B11050, then 0x004BB41C(this)
DECOMP(00740500, VtB11050_Dtor) { TDtor(ctx, 0x00B11050, 0x004BB41C); }
// *this = vtable 0x00B11078, then 0x004BB41C(this)
DECOMP(00740588, VtB11078_Dtor) { TDtor(ctx, 0x00B11078, 0x004BB41C); }
// *this = vtable 0x00B110A0, then 0x004BB41C(this)
DECOMP(00740610, VtB110A0_Dtor) { TDtor(ctx, 0x00B110A0, 0x004BB41C); }
// *this = vtable 0x00B110C8, then 0x004BB41C(this)
DECOMP(00740698, VtB110C8_Dtor) { TDtor(ctx, 0x00B110C8, 0x004BB41C); }
// *this = vtable 0x00B110F0, then 0x004BB41C(this)
DECOMP(00740720, VtB110F0_Dtor) { TDtor(ctx, 0x00B110F0, 0x004BB41C); }
// *this = vtable 0x00B11118, then 0x004BB41C(this)
DECOMP(007407A8, VtB11118_Dtor) { TDtor(ctx, 0x00B11118, 0x004BB41C); }
// *this = vtable 0x00B11140, then 0x004BB41C(this)
DECOMP(00740830, VtB11140_Dtor) { TDtor(ctx, 0x00B11140, 0x004BB41C); }
// *this = vtable 0x00B11168, then 0x004BB41C(this)
DECOMP(007408B8, VtB11168_Dtor) { TDtor(ctx, 0x00B11168, 0x004BB41C); }
// *this = vtable 0x00B11190, then 0x004BB41C(this)
DECOMP(00740940, VtB11190_Dtor) { TDtor(ctx, 0x00B11190, 0x004BB41C); }
// *this = vtable 0x00B11208, then 0x004BB41C(this)
DECOMP(00740AD4, VtB11208_Dtor) { TDtor(ctx, 0x00B11208, 0x004BB41C); }
// *this = vtable 0x00B11230, then 0x004BB41C(this)
DECOMP(00740B5C, VtB11230_Dtor) { TDtor(ctx, 0x00B11230, 0x004BB41C); }
// *this = vtable 0x00B11258, then 0x004BB41C(this)
DECOMP(00740BE4, VtB11258_Dtor) { TDtor(ctx, 0x00B11258, 0x004BB41C); }
// *this = vtable 0x00B11280, then 0x004BB41C(this)
DECOMP(00740C6C, VtB11280_Dtor) { TDtor(ctx, 0x00B11280, 0x004BB41C); }
// *this = vtable 0x00B112A8, then 0x004BB41C(this)
DECOMP(00740CF4, VtB112A8_Dtor) { TDtor(ctx, 0x00B112A8, 0x004BB41C); }
// *this = vtable 0x00B112D0, then 0x004BB41C(this)
DECOMP(00740D7C, VtB112D0_Dtor) { TDtor(ctx, 0x00B112D0, 0x004BB41C); }
// *this = vtable 0x00B112F8, then 0x004BB41C(this)
DECOMP(00740E04, VtB112F8_Dtor) { TDtor(ctx, 0x00B112F8, 0x004BB41C); }
// *this = vtable 0x00B11320, then 0x004BB41C(this)
DECOMP(00740E8C, VtB11320_Dtor) { TDtor(ctx, 0x00B11320, 0x004BB41C); }
// *this = vtable 0x00B11348, then 0x004BB41C(this)
DECOMP(00740F14, VtB11348_Dtor) { TDtor(ctx, 0x00B11348, 0x004BB41C); }
// *this = vtable 0x00B11370, then 0x004BB41C(this)
DECOMP(00740F9C, VtB11370_Dtor) { TDtor(ctx, 0x00B11370, 0x004BB41C); }
// *this = vtable 0x00B11398, then 0x004BB41C(this)
DECOMP(00741024, VtB11398_Dtor) { TDtor(ctx, 0x00B11398, 0x004BB41C); }
// *this = vtable 0x00B113C0, then 0x004BB41C(this)
DECOMP(007410AC, VtB113C0_Dtor) { TDtor(ctx, 0x00B113C0, 0x004BB41C); }
// *this = vtable 0x00B113E8, then 0x004BB41C(this)
DECOMP(00741134, VtB113E8_Dtor) { TDtor(ctx, 0x00B113E8, 0x004BB41C); }
// *this = vtable 0x00B11410, then 0x004BB41C(this)
DECOMP(007411BC, VtB11410_Dtor) { TDtor(ctx, 0x00B11410, 0x004BB41C); }
// *this = vtable 0x00B11438, then 0x004BB41C(this)
DECOMP(00741244, VtB11438_Dtor) { TDtor(ctx, 0x00B11438, 0x004BB41C); }
// *this = vtable 0x00B11460, then 0x004BB41C(this)
DECOMP(007412CC, VtB11460_Dtor) { TDtor(ctx, 0x00B11460, 0x004BB41C); }
// *this = vtable 0x00B11488, then 0x004BB41C(this)
DECOMP(00741354, VtB11488_Dtor) { TDtor(ctx, 0x00B11488, 0x004BB41C); }
// *this = vtable 0x00B114B0, then 0x004BB41C(this)
DECOMP(007413DC, VtB114B0_Dtor) { TDtor(ctx, 0x00B114B0, 0x004BB41C); }
// *this = vtable 0x00B11530, then 0x004BB41C(this)
DECOMP(00741600, VtB11530_Dtor) { TDtor(ctx, 0x00B11530, 0x004BB41C); }
// *this = vtable 0x00B11580, then 0x004BB41C(this)
DECOMP(00741770, VtB11580_Dtor) { TDtor(ctx, 0x00B11580, 0x004BB41C); }
// *this = vtable 0x00B115A8, then 0x004BB41C(this)
DECOMP(007417F8, VtB115A8_Dtor) { TDtor(ctx, 0x00B115A8, 0x004BB41C); }
// *this = vtable 0x00B115D0, then 0x004BB41C(this)
DECOMP(00741880, VtB115D0_Dtor) { TDtor(ctx, 0x00B115D0, 0x004BB41C); }
// *this = vtable 0x00B116E8, then 0x004BB41C(this)
DECOMP(00741B9C, VtB116E8_Dtor) { TDtor(ctx, 0x00B116E8, 0x004BB41C); }
// *this = vtable 0x00B11710, then 0x004BB41C(this)
DECOMP(00741C24, VtB11710_Dtor) { TDtor(ctx, 0x00B11710, 0x004BB41C); }
// *this = vtable 0x00B11738, then 0x004BB41C(this)
DECOMP(00741CAC, VtB11738_Dtor) { TDtor(ctx, 0x00B11738, 0x004BB41C); }
// *this = vtable 0x00B11760, then 0x004BB41C(this)
DECOMP(00741D34, VtB11760_Dtor) { TDtor(ctx, 0x00B11760, 0x004BB41C); }
// *this = vtable 0x00B11788, then 0x004BB41C(this)
DECOMP(00741DBC, VtB11788_Dtor) { TDtor(ctx, 0x00B11788, 0x004BB41C); }
// *this = vtable 0x00B117B0, then 0x004BB41C(this)
DECOMP(00741E44, VtB117B0_Dtor) { TDtor(ctx, 0x00B117B0, 0x004BB41C); }
// *this = vtable 0x00B117D8, then 0x004BB41C(this)
DECOMP(00741ECC, VtB117D8_Dtor) { TDtor(ctx, 0x00B117D8, 0x004BB41C); }
// *this = vtable 0x00B11800, then 0x004BB41C(this)
DECOMP(00741F54, VtB11800_Dtor) { TDtor(ctx, 0x00B11800, 0x004BB41C); }
// *this = vtable 0x00B11828, then 0x004BB41C(this)
DECOMP(00741FDC, VtB11828_Dtor) { TDtor(ctx, 0x00B11828, 0x004BB41C); }
// *this = vtable 0x00B11850, then 0x004BB41C(this)
DECOMP(00742064, VtB11850_Dtor) { TDtor(ctx, 0x00B11850, 0x004BB41C); }
// *this = vtable 0x00B11878, then 0x004BB41C(this)
DECOMP(007420EC, VtB11878_Dtor) { TDtor(ctx, 0x00B11878, 0x004BB41C); }
// *this = vtable 0x00B118A0, then 0x004BB41C(this)
DECOMP(00742174, VtB118A0_Dtor) { TDtor(ctx, 0x00B118A0, 0x004BB41C); }
// *this = vtable 0x00B118C8, then 0x004BB41C(this)
DECOMP(007421FC, VtB118C8_Dtor) { TDtor(ctx, 0x00B118C8, 0x004BB41C); }
// *this = vtable 0x00B118F0, then 0x004BB41C(this)
DECOMP(00742284, VtB118F0_Dtor) { TDtor(ctx, 0x00B118F0, 0x004BB41C); }
// *this = vtable 0x00B11940, then 0x004BB41C(this)
DECOMP(007423C0, VtB11940_Dtor) { TDtor(ctx, 0x00B11940, 0x004BB41C); }
// *this = vtable 0x00B11968, then 0x004BB41C(this)
DECOMP(00742448, VtB11968_Dtor) { TDtor(ctx, 0x00B11968, 0x004BB41C); }
// *this = vtable 0x00B11990, then 0x004BB41C(this)
DECOMP(007424D0, VtB11990_Dtor) { TDtor(ctx, 0x00B11990, 0x004BB41C); }
// *this = vtable 0x00B119B8, then 0x004BB41C(this)
DECOMP(00742558, VtB119B8_Dtor) { TDtor(ctx, 0x00B119B8, 0x004BB41C); }
// *this = vtable 0x00B119E0, then 0x004BB41C(this)
DECOMP(007425E0, VtB119E0_Dtor) { TDtor(ctx, 0x00B119E0, 0x004BB41C); }
// *this = vtable 0x00B11A08, then 0x004BB41C(this)
DECOMP(00742668, VtB11A08_Dtor) { TDtor(ctx, 0x00B11A08, 0x004BB41C); }
// *this = vtable 0x00B11A30, then 0x004BB41C(this)
DECOMP(007426F0, VtB11A30_Dtor) { TDtor(ctx, 0x00B11A30, 0x004BB41C); }
// *this = vtable 0x00B11A58, then 0x004BB41C(this)
DECOMP(00742778, VtB11A58_Dtor) { TDtor(ctx, 0x00B11A58, 0x004BB41C); }
// *this = vtable 0x00B11AA8, then 0x004BB41C(this)
DECOMP(00742858, VtB11AA8_Dtor) { TDtor(ctx, 0x00B11AA8, 0x004BB41C); }
// *this = vtable 0x00B11AF8, then 0x004BB41C(this)
DECOMP(007429C8, VtB11AF8_Dtor) { TDtor(ctx, 0x00B11AF8, 0x004BB41C); }
// *this = vtable 0x00B11B88, then 0x004B6884(this)
DECOMP(00742B0C, VtB11B88_Dtor) { TDtor(ctx, 0x00B11B88, 0x004B6884); }
// *this = vtable 0x00B11D98, then 0x000C2DA0(this)
DECOMP(007437D0, VtB11D98_Dtor) { TDtor(ctx, 0x00B11D98, 0x000C2DA0); }
// *this = vtable 0x00B1388C, then 0x009000A0(this)
DECOMP(00746B2C, VtB1388C_Dtor) { TDtor(ctx, 0x00B1388C, 0x009000A0); }
// *this = vtable 0x00B138B8, then 0x009000A0(this)
DECOMP(00746B5C, VtB138B8_Dtor) { TDtor(ctx, 0x00B138B8, 0x009000A0); }
// *this = vtable 0x00B13940, then 0x004FC1A0(this)
DECOMP(007489F0, VtB13940_Dtor) { TDtor(ctx, 0x00B13940, 0x004FC1A0); }
// *this = vtable 0x00B152C8, then 0x000E2A34(this)
DECOMP(007511B8, VtB152C8_Dtor) { TDtor(ctx, 0x00B152C8, 0x000E2A34); }
// *this = vtable 0x00B15508, then 0x000E2A34(this)
DECOMP(00751948, VtB15508_Dtor) { TDtor(ctx, 0x00B15508, 0x000E2A34); }
// *this = vtable 0x00B15748, then 0x000E2A34(this)
DECOMP(00751F4C, VtB15748_Dtor) { TDtor(ctx, 0x00B15748, 0x000E2A34); }
// *this = vtable 0x00B15BD8, then 0x000E2A34(this)
DECOMP(00752550, VtB15BD8_Dtor) { TDtor(ctx, 0x00B15BD8, 0x000E2A34); }
// *this = vtable 0x00B15E18, then 0x000E2A34(this)
DECOMP(00752B54, VtB15E18_Dtor) { TDtor(ctx, 0x00B15E18, 0x000E2A34); }
// *this = vtable 0x00B16280, then 0x000C2DA0(this)
DECOMP(0075399C, VtB16280_Dtor) { TDtor(ctx, 0x00B16280, 0x000C2DA0); }
// *this = vtable 0x00B163F0, then 0x000E2A34(this)
DECOMP(00753AAC, VtB163F0_Dtor) { TDtor(ctx, 0x00B163F0, 0x000E2A34); }
// *this = vtable 0x00B15970, then 0x000E2A34(this)
DECOMP(007540B0, VtB15970_Dtor) { TDtor(ctx, 0x00B15970, 0x000E2A34); }
// *this = vtable 0x00B15970, then 0x000E2A34(this)
DECOMP(00754430, VtB15970_Dtor_00754430) { TDtor(ctx, 0x00B15970, 0x000E2A34); }
// *this = vtable 0x00B15970, then 0x000E2A34(this)
DECOMP(00754A18, VtB15970_Dtor_00754A18) { TDtor(ctx, 0x00B15970, 0x000E2A34); }
// *this = vtable 0x00B15970, then 0x000E2A34(this)
DECOMP(00754D90, VtB15970_Dtor_00754D90) { TDtor(ctx, 0x00B15970, 0x000E2A34); }
// *this = vtable 0x00B15970, then 0x000E2A34(this)
DECOMP(00755034, VtB15970_Dtor_00755034) { TDtor(ctx, 0x00B15970, 0x000E2A34); }
// *this = vtable 0x00B15970, then 0x000E2A34(this)
DECOMP(00755190, VtB15970_Dtor_00755190) { TDtor(ctx, 0x00B15970, 0x000E2A34); }
// *this = vtable 0x00B15970, then 0x000E2A34(this)
DECOMP(00755218, VtB15970_Dtor_00755218) { TDtor(ctx, 0x00B15970, 0x000E2A34); }
// *this = vtable 0x00B17948, then 0x000E2A34(this)
DECOMP(00755FF8, VtB17948_Dtor) { TDtor(ctx, 0x00B17948, 0x000E2A34); }
// *this = vtable 0x00B17B88, then 0x000E2A34(this)
DECOMP(007565FC, VtB17B88_Dtor) { TDtor(ctx, 0x00B17B88, 0x000E2A34); }
// *this = vtable 0x00B18230, then 0x000E2A34(this)
DECOMP(00757308, VtB18230_Dtor) { TDtor(ctx, 0x00B18230, 0x000E2A34); }
// *this = vtable 0x00B18458, then 0x000E2A34(this)
DECOMP(00757390, VtB18458_Dtor) { TDtor(ctx, 0x00B18458, 0x000E2A34); }
// *this = vtable 0x00B18680, then 0x000E2A34(this)
DECOMP(00757420, VtB18680_Dtor) { TDtor(ctx, 0x00B18680, 0x000E2A34); }
// *this = vtable 0x00B188C0, then 0x000E2A34(this)
DECOMP(0075757C, VtB188C0_Dtor) { TDtor(ctx, 0x00B188C0, 0x000E2A34); }
// *this = vtable 0x00B19378, then 0x000C2DA0(this)
DECOMP(00759328, VtB19378_Dtor) { TDtor(ctx, 0x00B19378, 0x000C2DA0); }
// *this = vtable 0x00B193C8, then 0x000D729C(this)
DECOMP(00759438, VtB193C8_Dtor) { TDtor(ctx, 0x00B193C8, 0x000D729C); }
// *this = vtable 0x00B19510, then 0x000C2DA0(this)
DECOMP(0075AE14, VtB19510_Dtor) { TDtor(ctx, 0x00B19510, 0x000C2DA0); }
// *this = vtable 0x00B19510, then 0x000C2DA0(this)
DECOMP(0075AE9C, VtB19510_Dtor_0075AE9C) { TDtor(ctx, 0x00B19510, 0x000C2DA0); }
// *this = vtable 0x00B19510, then 0x000C2DA0(this)
DECOMP(0075B1E0, VtB19510_Dtor_0075B1E0) { TDtor(ctx, 0x00B19510, 0x000C2DA0); }
// *this = vtable 0x00B19510, then 0x000C2DA0(this)
DECOMP(0075B2F0, VtB19510_Dtor_0075B2F0) { TDtor(ctx, 0x00B19510, 0x000C2DA0); }
// *this = vtable 0x00B19510, then 0x000C2DA0(this)
DECOMP(0075B378, VtB19510_Dtor_0075B378) { TDtor(ctx, 0x00B19510, 0x000C2DA0); }
// *this = vtable 0x00B19510, then 0x000C2DA0(this)
DECOMP(0075B488, VtB19510_Dtor_0075B488) { TDtor(ctx, 0x00B19510, 0x000C2DA0); }
// *this = vtable 0x00B19510, then 0x000C2DA0(this)
DECOMP(0075B598, VtB19510_Dtor_0075B598) { TDtor(ctx, 0x00B19510, 0x000C2DA0); }
// *this = vtable 0x00B19510, then 0x000C2DA0(this)
DECOMP(0075B6A8, VtB19510_Dtor_0075B6A8) { TDtor(ctx, 0x00B19510, 0x000C2DA0); }
// *this = vtable 0x00B19510, then 0x000C2DA0(this)
DECOMP(0075B86C, VtB19510_Dtor_0075B86C) { TDtor(ctx, 0x00B19510, 0x000C2DA0); }
// *this = vtable 0x00B19510, then 0x000C2DA0(this)
DECOMP(0075B97C, VtB19510_Dtor_0075B97C) { TDtor(ctx, 0x00B19510, 0x000C2DA0); }
// *this = vtable 0x00B19510, then 0x000C2DA0(this)
DECOMP(0075BA04, VtB19510_Dtor_0075BA04) { TDtor(ctx, 0x00B19510, 0x000C2DA0); }
// *this = vtable 0x00B19510, then 0x000C2DA0(this)
DECOMP(0075BA8C, VtB19510_Dtor_0075BA8C) { TDtor(ctx, 0x00B19510, 0x000C2DA0); }
// *this = vtable 0x00B19510, then 0x000C2DA0(this)
DECOMP(0075BDB0, VtB19510_Dtor_0075BDB0) { TDtor(ctx, 0x00B19510, 0x000C2DA0); }
// *this = vtable 0x00B19510, then 0x000C2DA0(this)
DECOMP(0075BE38, VtB19510_Dtor_0075BE38) { TDtor(ctx, 0x00B19510, 0x000C2DA0); }
// *this = vtable 0x00B1BC30, then 0x00464AD0(this)
DECOMP(0075D348, VtB1BC30_Dtor) { TDtor(ctx, 0x00B1BC30, 0x00464AD0); }
// *this = vtable 0x00B1D040, then 0x00464AD0(this)
DECOMP(00761790, VtB1D040_Dtor) { TDtor(ctx, 0x00B1D040, 0x00464AD0); }
// *this = vtable 0x00B1D2C8, then 0x000180C8(this)
DECOMP(00765B64, VtB1D2C8_Dtor) { TDtor(ctx, 0x00B1D2C8, 0x000180C8); }
// *this = vtable 0x00B1F5D0, then 0x007D1CE4(this)
DECOMP(0076BE0C, VtB1F5D0_Dtor) { TDtor(ctx, 0x00B1F5D0, 0x007D1CE4); }
// *this = vtable 0x00B207A8, then 0x00490B34(this)
DECOMP(0077D220, VtB207A8_Dtor) { TDtor(ctx, 0x00B207A8, 0x00490B34); }
// *this = vtable 0x00B20E78, then 0x0042CA50(this)
DECOMP(0077F568, VtB20E78_Dtor) { TDtor(ctx, 0x00B20E78, 0x0042CA50); }
// *this = vtable 0x00B21118, then 0x0042CA50(this)
DECOMP(0077F598, VtB21118_Dtor) { TDtor(ctx, 0x00B21118, 0x0042CA50); }
// *this = vtable 0x00B21130, then 0x0042CA50(this)
DECOMP(0077F5C8, VtB21130_Dtor) { TDtor(ctx, 0x00B21130, 0x0042CA50); }
// *this = vtable 0x00B21148, then 0x0042CA50(this)
DECOMP(0077F5F8, VtB21148_Dtor) { TDtor(ctx, 0x00B21148, 0x0042CA50); }
// *this = vtable 0x00B213D0, then 0x0042CA50(this)
DECOMP(0077F6A0, VtB213D0_Dtor) { TDtor(ctx, 0x00B213D0, 0x0042CA50); }
// *this = vtable 0x00B20CE0, then 0x0042CA50(this)
DECOMP(0077F6D0, VtB20CE0_Dtor) { TDtor(ctx, 0x00B20CE0, 0x0042CA50); }
// *this = vtable 0x00B20CF8, then 0x0042CA50(this)
DECOMP(0077F758, VtB20CF8_Dtor) { TDtor(ctx, 0x00B20CF8, 0x0042CA50); }
// *this = vtable 0x00B20D10, then 0x0042CA50(this)
DECOMP(0077F7E8, VtB20D10_Dtor) { TDtor(ctx, 0x00B20D10, 0x0042CA50); }
// *this = vtable 0x00B20D28, then 0x0042CA50(this)
DECOMP(0077F870, VtB20D28_Dtor) { TDtor(ctx, 0x00B20D28, 0x0042CA50); }
// *this = vtable 0x00B20D40, then 0x0042CA50(this)
DECOMP(0077F900, VtB20D40_Dtor) { TDtor(ctx, 0x00B20D40, 0x0042CA50); }
// *this = vtable 0x00B20D58, then 0x0042CA50(this)
DECOMP(0077F988, VtB20D58_Dtor) { TDtor(ctx, 0x00B20D58, 0x0042CA50); }
// *this = vtable 0x00B20D70, then 0x0042CA50(this)
DECOMP(0077FA10, VtB20D70_Dtor) { TDtor(ctx, 0x00B20D70, 0x0042CA50); }
// *this = vtable 0x00B20D88, then 0x0042CA50(this)
DECOMP(0077FA98, VtB20D88_Dtor) { TDtor(ctx, 0x00B20D88, 0x0042CA50); }
// *this = vtable 0x00B20DA0, then 0x0042CA50(this)
DECOMP(0077FB28, VtB20DA0_Dtor) { TDtor(ctx, 0x00B20DA0, 0x0042CA50); }
// *this = vtable 0x00B20DB8, then 0x0042CA50(this)
DECOMP(0077FBB8, VtB20DB8_Dtor) { TDtor(ctx, 0x00B20DB8, 0x0042CA50); }
// *this = vtable 0x00B20DD0, then 0x0042CA50(this)
DECOMP(0077FC48, VtB20DD0_Dtor) { TDtor(ctx, 0x00B20DD0, 0x0042CA50); }
// *this = vtable 0x00B20DE8, then 0x0042CA50(this)
DECOMP(0077FCD0, VtB20DE8_Dtor) { TDtor(ctx, 0x00B20DE8, 0x0042CA50); }
// *this = vtable 0x00B20E00, then 0x0042CA50(this)
DECOMP(0077FD60, VtB20E00_Dtor) { TDtor(ctx, 0x00B20E00, 0x0042CA50); }
// *this = vtable 0x00B20E18, then 0x0042CA50(this)
DECOMP(0077FDF0, VtB20E18_Dtor) { TDtor(ctx, 0x00B20E18, 0x0042CA50); }
// *this = vtable 0x00B20E30, then 0x0042CA50(this)
DECOMP(0077FE78, VtB20E30_Dtor) { TDtor(ctx, 0x00B20E30, 0x0042CA50); }
// *this = vtable 0x00B20E48, then 0x0042CA50(this)
DECOMP(0077FF00, VtB20E48_Dtor) { TDtor(ctx, 0x00B20E48, 0x0042CA50); }
// *this = vtable 0x00B20E60, then 0x0042CA50(this)
DECOMP(0077FF88, VtB20E60_Dtor) { TDtor(ctx, 0x00B20E60, 0x0042CA50); }
// *this = vtable 0x00B20EA8, then 0x0042CA50(this)
DECOMP(00780068, VtB20EA8_Dtor) { TDtor(ctx, 0x00B20EA8, 0x0042CA50); }
// *this = vtable 0x00B20EC0, then 0x0042CA50(this)
DECOMP(007800F0, VtB20EC0_Dtor) { TDtor(ctx, 0x00B20EC0, 0x0042CA50); }
// *this = vtable 0x00B20ED8, then 0x0042CA50(this)
DECOMP(00780178, VtB20ED8_Dtor) { TDtor(ctx, 0x00B20ED8, 0x0042CA50); }
// *this = vtable 0x00B20EF0, then 0x0042CA50(this)
DECOMP(00780200, VtB20EF0_Dtor) { TDtor(ctx, 0x00B20EF0, 0x0042CA50); }
// *this = vtable 0x00B20F08, then 0x0042CA50(this)
DECOMP(00780290, VtB20F08_Dtor) { TDtor(ctx, 0x00B20F08, 0x0042CA50); }
// *this = vtable 0x00B20F20, then 0x0042CA50(this)
DECOMP(00780318, VtB20F20_Dtor) { TDtor(ctx, 0x00B20F20, 0x0042CA50); }
// *this = vtable 0x00B20F38, then 0x0042CA50(this)
DECOMP(007803A0, VtB20F38_Dtor) { TDtor(ctx, 0x00B20F38, 0x0042CA50); }
// *this = vtable 0x00B20F50, then 0x0042CA50(this)
DECOMP(00780428, VtB20F50_Dtor) { TDtor(ctx, 0x00B20F50, 0x0042CA50); }
// *this = vtable 0x00B20F80, then 0x0042CA50(this)
DECOMP(007804B8, VtB20F80_Dtor) { TDtor(ctx, 0x00B20F80, 0x0042CA50); }
// *this = vtable 0x00B20F98, then 0x0042CA50(this)
DECOMP(00780540, VtB20F98_Dtor) { TDtor(ctx, 0x00B20F98, 0x0042CA50); }
// *this = vtable 0x00B20FB0, then 0x0042CA50(this)
DECOMP(007805C8, VtB20FB0_Dtor) { TDtor(ctx, 0x00B20FB0, 0x0042CA50); }
// *this = vtable 0x00B20FC8, then 0x0042CA50(this)
DECOMP(00780650, VtB20FC8_Dtor) { TDtor(ctx, 0x00B20FC8, 0x0042CA50); }
// *this = vtable 0x00B20FE0, then 0x0042CA50(this)
DECOMP(007806D8, VtB20FE0_Dtor) { TDtor(ctx, 0x00B20FE0, 0x0042CA50); }
// *this = vtable 0x00B20FF8, then 0x0042CA50(this)
DECOMP(00780760, VtB20FF8_Dtor) { TDtor(ctx, 0x00B20FF8, 0x0042CA50); }
// *this = vtable 0x00B21010, then 0x0042CA50(this)
DECOMP(007807E8, VtB21010_Dtor) { TDtor(ctx, 0x00B21010, 0x0042CA50); }
// *this = vtable 0x00B21028, then 0x0042CA50(this)
DECOMP(00780870, VtB21028_Dtor) { TDtor(ctx, 0x00B21028, 0x0042CA50); }
// *this = vtable 0x00B211A8, then 0x0042CA50(this)
DECOMP(00780C60, VtB211A8_Dtor) { TDtor(ctx, 0x00B211A8, 0x0042CA50); }
// *this = vtable 0x00B211C0, then 0x0042CA50(this)
DECOMP(00780CE8, VtB211C0_Dtor) { TDtor(ctx, 0x00B211C0, 0x0042CA50); }
// *this = vtable 0x00B211D8, then 0x0042CA50(this)
DECOMP(00780D70, VtB211D8_Dtor) { TDtor(ctx, 0x00B211D8, 0x0042CA50); }
// *this = vtable 0x00B211F0, then 0x0042CA50(this)
DECOMP(00780DF8, VtB211F0_Dtor) { TDtor(ctx, 0x00B211F0, 0x0042CA50); }
// *this = vtable 0x00B21208, then 0x0042CA50(this)
DECOMP(00780E80, VtB21208_Dtor) { TDtor(ctx, 0x00B21208, 0x0042CA50); }
// *this = vtable 0x00B21220, then 0x0042CA50(this)
DECOMP(00780F10, VtB21220_Dtor) { TDtor(ctx, 0x00B21220, 0x0042CA50); }
// *this = vtable 0x00B21238, then 0x0042CA50(this)
DECOMP(00780F98, VtB21238_Dtor) { TDtor(ctx, 0x00B21238, 0x0042CA50); }
// *this = vtable 0x00B21250, then 0x0042CA50(this)
DECOMP(00781020, VtB21250_Dtor) { TDtor(ctx, 0x00B21250, 0x0042CA50); }
// *this = vtable 0x00B21268, then 0x0042CA50(this)
DECOMP(007810A8, VtB21268_Dtor) { TDtor(ctx, 0x00B21268, 0x0042CA50); }
// *this = vtable 0x00B21280, then 0x0042CA50(this)
DECOMP(00781130, VtB21280_Dtor) { TDtor(ctx, 0x00B21280, 0x0042CA50); }
// *this = vtable 0x00B21298, then 0x0042CA50(this)
DECOMP(007811C0, VtB21298_Dtor) { TDtor(ctx, 0x00B21298, 0x0042CA50); }
// *this = vtable 0x00B212B0, then 0x0042CA50(this)
DECOMP(00781248, VtB212B0_Dtor) { TDtor(ctx, 0x00B212B0, 0x0042CA50); }
// *this = vtable 0x00B212E0, then 0x0042CA50(this)
DECOMP(007812D0, VtB212E0_Dtor) { TDtor(ctx, 0x00B212E0, 0x0042CA50); }
// *this = vtable 0x00B212F8, then 0x0042CA50(this)
DECOMP(00781360, VtB212F8_Dtor) { TDtor(ctx, 0x00B212F8, 0x0042CA50); }
// *this = vtable 0x00B21310, then 0x0042CA50(this)
DECOMP(007813F0, VtB21310_Dtor) { TDtor(ctx, 0x00B21310, 0x0042CA50); }
// *this = vtable 0x00B21328, then 0x0042CA50(this)
DECOMP(00781480, VtB21328_Dtor) { TDtor(ctx, 0x00B21328, 0x0042CA50); }
// *this = vtable 0x00B21340, then 0x0042CA50(this)
DECOMP(00781510, VtB21340_Dtor) { TDtor(ctx, 0x00B21340, 0x0042CA50); }
// *this = vtable 0x00B21358, then 0x0042CA50(this)
DECOMP(00781598, VtB21358_Dtor) { TDtor(ctx, 0x00B21358, 0x0042CA50); }
// *this = vtable 0x00B21370, then 0x0042CA50(this)
DECOMP(00781620, VtB21370_Dtor) { TDtor(ctx, 0x00B21370, 0x0042CA50); }
// *this = vtable 0x00B21388, then 0x0042CA50(this)
DECOMP(007816B0, VtB21388_Dtor) { TDtor(ctx, 0x00B21388, 0x0042CA50); }
// *this = vtable 0x00B213A0, then 0x0042CA50(this)
DECOMP(00781740, VtB213A0_Dtor) { TDtor(ctx, 0x00B213A0, 0x0042CA50); }
// *this = vtable 0x00B213B8, then 0x0042CA50(this)
DECOMP(007817D0, VtB213B8_Dtor) { TDtor(ctx, 0x00B213B8, 0x0042CA50); }
// *this = vtable 0x00B213E8, then 0x0042CA50(this)
DECOMP(007818B8, VtB213E8_Dtor) { TDtor(ctx, 0x00B213E8, 0x0042CA50); }
// *this = vtable 0x00B21418, then 0x0042CA50(this)
DECOMP(00781A28, VtB21418_Dtor) { TDtor(ctx, 0x00B21418, 0x0042CA50); }
// *this = vtable 0x00B21470, then 0x004F7108(this)
DECOMP(00781AE0, VtB21470_Dtor) { TDtor(ctx, 0x00B21470, 0x004F7108); }
// *this = vtable 0x00B214A0, then 0x004F79C0(this)
DECOMP(00781B10, VtB214A0_Dtor) { TDtor(ctx, 0x00B214A0, 0x004F79C0); }
// *this = vtable 0x00B214B8, then 0x004F7108(this)
DECOMP(00781B40, VtB214B8_Dtor) { TDtor(ctx, 0x00B214B8, 0x004F7108); }
// *this = vtable 0x00B214FC, then 0x004F79C0(this)
DECOMP(00781B70, VtB214FC_Dtor) { TDtor(ctx, 0x00B214FC, 0x004F79C0); }
// *this = vtable 0x00B21518, then 0x004F7108(this)
DECOMP(00781BA0, VtB21518_Dtor) { TDtor(ctx, 0x00B21518, 0x004F7108); }
// *this = vtable 0x00B21548, then 0x004F79C0(this)
DECOMP(00781BD0, VtB21548_Dtor) { TDtor(ctx, 0x00B21548, 0x004F79C0); }
// *this = vtable 0x00B21518, then 0x004F7108(this)
DECOMP(00781C00, VtB21518_Dtor_00781C00) { TDtor(ctx, 0x00B21518, 0x004F7108); }
// *this = vtable 0x00B21548, then 0x004F79C0(this)
DECOMP(00781C30, VtB21548_Dtor_00781C30) { TDtor(ctx, 0x00B21548, 0x004F79C0); }
// *this = vtable 0x00B21518, then 0x004F7108(this)
DECOMP(00781C60, VtB21518_Dtor_00781C60) { TDtor(ctx, 0x00B21518, 0x004F7108); }
// *this = vtable 0x00B21548, then 0x004F79C0(this)
DECOMP(00781C90, VtB21548_Dtor_00781C90) { TDtor(ctx, 0x00B21548, 0x004F79C0); }
// *this = vtable 0x00B215F0, then 0x004F7108(this)
DECOMP(00781CC0, VtB215F0_Dtor) { TDtor(ctx, 0x00B215F0, 0x004F7108); }
// *this = vtable 0x00B21620, then 0x004F79C0(this)
DECOMP(00781CF0, VtB21620_Dtor) { TDtor(ctx, 0x00B21620, 0x004F79C0); }
// *this = vtable 0x00B21638, then 0x004F7108(this)
DECOMP(00781D20, VtB21638_Dtor) { TDtor(ctx, 0x00B21638, 0x004F7108); }
// *this = vtable 0x00B21668, then 0x004F79C0(this)
DECOMP(00781D50, VtB21668_Dtor) { TDtor(ctx, 0x00B21668, 0x004F79C0); }
// *this = vtable 0x00B21680, then 0x004F7108(this)
DECOMP(00781D80, VtB21680_Dtor) { TDtor(ctx, 0x00B21680, 0x004F7108); }
// *this = vtable 0x00B216B0, then 0x004F79C0(this)
DECOMP(00781DB0, VtB216B0_Dtor) { TDtor(ctx, 0x00B216B0, 0x004F79C0); }
// *this = vtable 0x00B216C8, then 0x004F7108(this)
DECOMP(00781DE0, VtB216C8_Dtor) { TDtor(ctx, 0x00B216C8, 0x004F7108); }
// *this = vtable 0x00B216F8, then 0x004F79C0(this)
DECOMP(00781E10, VtB216F8_Dtor) { TDtor(ctx, 0x00B216F8, 0x004F79C0); }
// *this = vtable 0x00B21710, then 0x004F7108(this)
DECOMP(00781E40, VtB21710_Dtor) { TDtor(ctx, 0x00B21710, 0x004F7108); }
// *this = vtable 0x00B21740, then 0x004F79C0(this)
DECOMP(00781E70, VtB21740_Dtor) { TDtor(ctx, 0x00B21740, 0x004F79C0); }
// *this = vtable 0x00B21758, then 0x004F7108(this)
DECOMP(00781EA0, VtB21758_Dtor) { TDtor(ctx, 0x00B21758, 0x004F7108); }
// *this = vtable 0x00B21788, then 0x004F79C0(this)
DECOMP(00781ED0, VtB21788_Dtor) { TDtor(ctx, 0x00B21788, 0x004F79C0); }
// *this = vtable 0x00B217A0, then 0x004F7108(this)
DECOMP(00781F00, VtB217A0_Dtor) { TDtor(ctx, 0x00B217A0, 0x004F7108); }
// *this = vtable 0x00B217D0, then 0x004F79C0(this)
DECOMP(00781F30, VtB217D0_Dtor) { TDtor(ctx, 0x00B217D0, 0x004F79C0); }
// *this = vtable 0x00B217F8, then 0x004F7108(this)
DECOMP(00781F60, VtB217F8_Dtor) { TDtor(ctx, 0x00B217F8, 0x004F7108); }
// *this = vtable 0x00B21828, then 0x004F79C0(this)
DECOMP(00781F90, VtB21828_Dtor) { TDtor(ctx, 0x00B21828, 0x004F79C0); }
// *this = vtable 0x00B21840, then 0x004F7108(this)
DECOMP(00781FC0, VtB21840_Dtor) { TDtor(ctx, 0x00B21840, 0x004F7108); }
// *this = vtable 0x00B21870, then 0x004F79C0(this)
DECOMP(00781FF0, VtB21870_Dtor) { TDtor(ctx, 0x00B21870, 0x004F79C0); }
// *this = vtable 0x00B21888, then 0x004F7108(this)
DECOMP(00782020, VtB21888_Dtor) { TDtor(ctx, 0x00B21888, 0x004F7108); }
// *this = vtable 0x00B218B8, then 0x004F79C0(this)
DECOMP(00782050, VtB218B8_Dtor) { TDtor(ctx, 0x00B218B8, 0x004F79C0); }

// ---------------------------------------------------------------- deleting destructors (null-checked): the vtable, the base destructor, then the free (340)
// deleting: vtable 0x00B0D934, 0x004FC1A0(this), 0x0054EE6C(this, 0)
DECOMP(00736050, VtB0D934_Delete) { TDelete(ctx, 0x00B0D934, 0x004FC1A0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0D968, 0x004FC1A0(this), 0x0054EE6C(this, 0)
DECOMP(00736114, VtB0D968_Delete) { TDelete(ctx, 0x00B0D968, 0x004FC1A0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0D99C, 0x004FC1A0(this), 0x0054EE6C(this, 0)
DECOMP(007361EC, VtB0D99C_Delete) { TDelete(ctx, 0x00B0D99C, 0x004FC1A0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0D9D0, 0x004FC1A0(this), 0x0054EE6C(this, 0)
DECOMP(007362C8, VtB0D9D0_Delete) { TDelete(ctx, 0x00B0D9D0, 0x004FC1A0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DA04, 0x004FC1A0(this), 0x0054EE6C(this, 0)
DECOMP(007363C4, VtB0DA04_Delete) { TDelete(ctx, 0x00B0DA04, 0x004FC1A0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DA38, 0x004FC1A0(this), 0x0054EE6C(this, 0)
DECOMP(007364B4, VtB0DA38_Delete) { TDelete(ctx, 0x00B0DA38, 0x004FC1A0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DA6C, 0x004FC1A0(this), 0x0054EE6C(this, 0)
DECOMP(007365A0, VtB0DA6C_Delete) { TDelete(ctx, 0x00B0DA6C, 0x004FC1A0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DE40, 0x00121E6C(this), 0x0054EE6C(this, 0)
DECOMP(00738864, VtB0DE40_Delete) { TDelete(ctx, 0x00B0DE40, 0x00121E6C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DFD0, 0x004F7108(this), 0x0054EE6C(this, 0)
DECOMP(007388E8, VtB0DFD0_Delete) { TDelete(ctx, 0x00B0DFD0, 0x004F7108, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0E010, 0x004F79C0(this), 0x0054EE6C(this, 0)
DECOMP(00738940, VtB0E010_Delete) { TDelete(ctx, 0x00B0E010, 0x004F79C0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0E160, 0x004F7108(this), 0x0054EE6C(this, 0)
DECOMP(00738B5C, VtB0E160_Delete) { TDelete(ctx, 0x00B0E160, 0x004F7108, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0E1A0, 0x004F79C0(this), 0x0054EE6C(this, 0)
DECOMP(00738BB4, VtB0E1A0_Delete) { TDelete(ctx, 0x00B0E1A0, 0x004F79C0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0E560, 0x004F7108(this), 0x0054EE6C(this, 0)
DECOMP(00738E40, VtB0E560_Delete) { TDelete(ctx, 0x00B0E560, 0x004F7108, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0E5A0, 0x004F79C0(this), 0x0054EE6C(this, 0)
DECOMP(00738E98, VtB0E5A0_Delete) { TDelete(ctx, 0x00B0E5A0, 0x004F79C0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0E6F0, 0x004F7108(this), 0x0054EE6C(this, 0)
DECOMP(00739164, VtB0E6F0_Delete) { TDelete(ctx, 0x00B0E6F0, 0x004F7108, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0E734, 0x004F79C0(this), 0x0054EE6C(this, 0)
DECOMP(007391BC, VtB0E734_Delete) { TDelete(ctx, 0x00B0E734, 0x004F79C0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F090, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073BD98, VtB0F090_Delete) { TDelete(ctx, 0x00B0F090, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F180, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073BE20, VtB0F180_Delete) { TDelete(ctx, 0x00B0F180, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F1D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073BEA8, VtB0F1D0_Delete) { TDelete(ctx, 0x00B0F1D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F1D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073BF30, VtB0F1D0_Delete_0073BF30) { TDelete(ctx, 0x00B0F1D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F1D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073BFB8, VtB0F1D0_Delete_0073BFB8) { TDelete(ctx, 0x00B0F1D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F248, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073C040, VtB0F248_Delete) { TDelete(ctx, 0x00B0F248, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F270, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073C0C8, VtB0F270_Delete) { TDelete(ctx, 0x00B0F270, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F2E8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073C150, VtB0F2E8_Delete) { TDelete(ctx, 0x00B0F2E8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F310, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073C1D8, VtB0F310_Delete) { TDelete(ctx, 0x00B0F310, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F338, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073C260, VtB0F338_Delete) { TDelete(ctx, 0x00B0F338, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F360, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073C2E8, VtB0F360_Delete) { TDelete(ctx, 0x00B0F360, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F388, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073C370, VtB0F388_Delete) { TDelete(ctx, 0x00B0F388, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F3D8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073C3F8, VtB0F3D8_Delete) { TDelete(ctx, 0x00B0F3D8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F400, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073C480, VtB0F400_Delete) { TDelete(ctx, 0x00B0F400, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F428, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073C508, VtB0F428_Delete) { TDelete(ctx, 0x00B0F428, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F450, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073C590, VtB0F450_Delete) { TDelete(ctx, 0x00B0F450, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F478, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073C618, VtB0F478_Delete) { TDelete(ctx, 0x00B0F478, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F4A0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073C6A0, VtB0F4A0_Delete) { TDelete(ctx, 0x00B0F4A0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F4C8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073C728, VtB0F4C8_Delete) { TDelete(ctx, 0x00B0F4C8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F4F0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073C7B0, VtB0F4F0_Delete) { TDelete(ctx, 0x00B0F4F0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F518, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073C838, VtB0F518_Delete) { TDelete(ctx, 0x00B0F518, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F1D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073C890, VtB0F1D0_Delete_0073C890) { TDelete(ctx, 0x00B0F1D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F568, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073C918, VtB0F568_Delete) { TDelete(ctx, 0x00B0F568, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F590, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073C9A0, VtB0F590_Delete) { TDelete(ctx, 0x00B0F590, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F5B8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073CA58, VtB0F5B8_Delete) { TDelete(ctx, 0x00B0F5B8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F630, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073CAE0, VtB0F630_Delete) { TDelete(ctx, 0x00B0F630, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F658, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073CB68, VtB0F658_Delete) { TDelete(ctx, 0x00B0F658, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F680, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073CBF0, VtB0F680_Delete) { TDelete(ctx, 0x00B0F680, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F6A8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073CC78, VtB0F6A8_Delete) { TDelete(ctx, 0x00B0F6A8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F6D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073CD00, VtB0F6D0_Delete) { TDelete(ctx, 0x00B0F6D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F6F8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073CD88, VtB0F6F8_Delete) { TDelete(ctx, 0x00B0F6F8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F720, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073CE10, VtB0F720_Delete) { TDelete(ctx, 0x00B0F720, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F810, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073CE98, VtB0F810_Delete) { TDelete(ctx, 0x00B0F810, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F888, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073CF20, VtB0F888_Delete) { TDelete(ctx, 0x00B0F888, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F8B0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073CFA8, VtB0F8B0_Delete) { TDelete(ctx, 0x00B0F8B0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F928, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073D030, VtB0F928_Delete) { TDelete(ctx, 0x00B0F928, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F950, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073D0B8, VtB0F950_Delete) { TDelete(ctx, 0x00B0F950, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FAE0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073D140, VtB0FAE0_Delete) { TDelete(ctx, 0x00B0FAE0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FB08, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073D1C8, VtB0FB08_Delete) { TDelete(ctx, 0x00B0FB08, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FB30, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073D250, VtB0FB30_Delete) { TDelete(ctx, 0x00B0FB30, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FB58, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073D2D8, VtB0FB58_Delete) { TDelete(ctx, 0x00B0FB58, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FB80, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073D360, VtB0FB80_Delete) { TDelete(ctx, 0x00B0FB80, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FC70, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073D3E8, VtB0FC70_Delete) { TDelete(ctx, 0x00B0FC70, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FC98, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073D470, VtB0FC98_Delete) { TDelete(ctx, 0x00B0FC98, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FCE8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073D4F8, VtB0FCE8_Delete) { TDelete(ctx, 0x00B0FCE8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FD10, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073D580, VtB0FD10_Delete) { TDelete(ctx, 0x00B0FD10, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FD38, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073D608, VtB0FD38_Delete) { TDelete(ctx, 0x00B0FD38, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FD60, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073D690, VtB0FD60_Delete) { TDelete(ctx, 0x00B0FD60, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FD88, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073D718, VtB0FD88_Delete) { TDelete(ctx, 0x00B0FD88, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FDB0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073D7A0, VtB0FDB0_Delete) { TDelete(ctx, 0x00B0FDB0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FDD8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073D828, VtB0FDD8_Delete) { TDelete(ctx, 0x00B0FDD8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FE00, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073D8B0, VtB0FE00_Delete) { TDelete(ctx, 0x00B0FE00, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FE78, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073D938, VtB0FE78_Delete) { TDelete(ctx, 0x00B0FE78, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FEA0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073D9C0, VtB0FEA0_Delete) { TDelete(ctx, 0x00B0FEA0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FEC8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073DA48, VtB0FEC8_Delete) { TDelete(ctx, 0x00B0FEC8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0FF18, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073DAD0, VtB0FF18_Delete) { TDelete(ctx, 0x00B0FF18, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F1D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073DC40, VtB0F1D0_Delete_0073DC40) { TDelete(ctx, 0x00B0F1D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F1D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073DDB0, VtB0F1D0_Delete_0073DDB0) { TDelete(ctx, 0x00B0F1D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10008, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073DE38, VtB10008_Delete) { TDelete(ctx, 0x00B10008, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10058, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073DEC0, VtB10058_Delete) { TDelete(ctx, 0x00B10058, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10080, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073DF48, VtB10080_Delete) { TDelete(ctx, 0x00B10080, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B100D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073DFD0, VtB100D0_Delete) { TDelete(ctx, 0x00B100D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F1D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073E058, VtB0F1D0_Delete_0073E058) { TDelete(ctx, 0x00B0F1D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10170, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073E0E0, VtB10170_Delete) { TDelete(ctx, 0x00B10170, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10198, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073E168, VtB10198_Delete) { TDelete(ctx, 0x00B10198, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F1D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073E1F0, VtB0F1D0_Delete_0073E1F0) { TDelete(ctx, 0x00B0F1D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F1D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073E278, VtB0F1D0_Delete_0073E278) { TDelete(ctx, 0x00B0F1D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B102B0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073E300, VtB102B0_Delete) { TDelete(ctx, 0x00B102B0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B102D8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073E388, VtB102D8_Delete) { TDelete(ctx, 0x00B102D8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10300, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073E410, VtB10300_Delete) { TDelete(ctx, 0x00B10300, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10328, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073E498, VtB10328_Delete) { TDelete(ctx, 0x00B10328, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10350, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073E520, VtB10350_Delete) { TDelete(ctx, 0x00B10350, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10378, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073E5A8, VtB10378_Delete) { TDelete(ctx, 0x00B10378, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B103A0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073E630, VtB103A0_Delete) { TDelete(ctx, 0x00B103A0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B103C8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073E6B8, VtB103C8_Delete) { TDelete(ctx, 0x00B103C8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B103F0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073E740, VtB103F0_Delete) { TDelete(ctx, 0x00B103F0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10418, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073E7C8, VtB10418_Delete) { TDelete(ctx, 0x00B10418, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10440, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073E850, VtB10440_Delete) { TDelete(ctx, 0x00B10440, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10468, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073E8D8, VtB10468_Delete) { TDelete(ctx, 0x00B10468, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10490, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073E960, VtB10490_Delete) { TDelete(ctx, 0x00B10490, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B104B8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073E9E8, VtB104B8_Delete) { TDelete(ctx, 0x00B104B8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F1D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073EA70, VtB0F1D0_Delete_0073EA70) { TDelete(ctx, 0x00B0F1D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10530, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073EAF8, VtB10530_Delete) { TDelete(ctx, 0x00B10530, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F1D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073EB80, VtB0F1D0_Delete_0073EB80) { TDelete(ctx, 0x00B0F1D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F1D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073EC08, VtB0F1D0_Delete_0073EC08) { TDelete(ctx, 0x00B0F1D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B105A8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073EC90, VtB105A8_Delete) { TDelete(ctx, 0x00B105A8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B105F8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073ED18, VtB105F8_Delete) { TDelete(ctx, 0x00B105F8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10620, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073EDA0, VtB10620_Delete) { TDelete(ctx, 0x00B10620, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F1D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073EE28, VtB0F1D0_Delete_0073EE28) { TDelete(ctx, 0x00B0F1D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F1D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073EF4C, VtB0F1D0_Delete_0073EF4C) { TDelete(ctx, 0x00B0F1D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F1D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073EFA8, VtB0F1D0_Delete_0073EFA8) { TDelete(ctx, 0x00B0F1D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F1D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073F030, VtB0F1D0_Delete_0073F030) { TDelete(ctx, 0x00B0F1D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10738, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073F0B8, VtB10738_Delete) { TDelete(ctx, 0x00B10738, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10828, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073F140, VtB10828_Delete) { TDelete(ctx, 0x00B10828, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10850, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073F1C8, VtB10850_Delete) { TDelete(ctx, 0x00B10850, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B108A0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073F250, VtB108A0_Delete) { TDelete(ctx, 0x00B108A0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B108C8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073F2D8, VtB108C8_Delete) { TDelete(ctx, 0x00B108C8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B108F0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073F360, VtB108F0_Delete) { TDelete(ctx, 0x00B108F0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10940, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073F3E8, VtB10940_Delete) { TDelete(ctx, 0x00B10940, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10990, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073F470, VtB10990_Delete) { TDelete(ctx, 0x00B10990, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B109E0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073F4F8, VtB109E0_Delete) { TDelete(ctx, 0x00B109E0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10A08, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073F580, VtB10A08_Delete) { TDelete(ctx, 0x00B10A08, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10A30, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073F608, VtB10A30_Delete) { TDelete(ctx, 0x00B10A30, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10A58, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073F690, VtB10A58_Delete) { TDelete(ctx, 0x00B10A58, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10A80, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073F718, VtB10A80_Delete) { TDelete(ctx, 0x00B10A80, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10AD0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073F7A0, VtB10AD0_Delete) { TDelete(ctx, 0x00B10AD0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10B20, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073F828, VtB10B20_Delete) { TDelete(ctx, 0x00B10B20, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10B48, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073F8B0, VtB10B48_Delete) { TDelete(ctx, 0x00B10B48, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10B70, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073F938, VtB10B70_Delete) { TDelete(ctx, 0x00B10B70, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10BC0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073F9C0, VtB10BC0_Delete) { TDelete(ctx, 0x00B10BC0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0F1D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073FA18, VtB0F1D0_Delete_0073FA18) { TDelete(ctx, 0x00B0F1D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10C38, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073FAA0, VtB10C38_Delete) { TDelete(ctx, 0x00B10C38, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10C60, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073FB28, VtB10C60_Delete) { TDelete(ctx, 0x00B10C60, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10C88, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073FBB0, VtB10C88_Delete) { TDelete(ctx, 0x00B10C88, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10CB0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073FC38, VtB10CB0_Delete) { TDelete(ctx, 0x00B10CB0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10CD8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073FCC0, VtB10CD8_Delete) { TDelete(ctx, 0x00B10CD8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10D00, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073FD48, VtB10D00_Delete) { TDelete(ctx, 0x00B10D00, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10D78, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073FDD0, VtB10D78_Delete) { TDelete(ctx, 0x00B10D78, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10DA0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073FE58, VtB10DA0_Delete) { TDelete(ctx, 0x00B10DA0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10E40, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0073FEE0, VtB10E40_Delete) { TDelete(ctx, 0x00B10E40, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10F10, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(007400F0, VtB10F10_Delete) { TDelete(ctx, 0x00B10F10, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10F38, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740178, VtB10F38_Delete) { TDelete(ctx, 0x00B10F38, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10F60, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740200, VtB10F60_Delete) { TDelete(ctx, 0x00B10F60, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10F88, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740288, VtB10F88_Delete) { TDelete(ctx, 0x00B10F88, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10FB0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740310, VtB10FB0_Delete) { TDelete(ctx, 0x00B10FB0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B10FD8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740398, VtB10FD8_Delete) { TDelete(ctx, 0x00B10FD8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11000, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740420, VtB11000_Delete) { TDelete(ctx, 0x00B11000, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11028, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(007404A8, VtB11028_Delete) { TDelete(ctx, 0x00B11028, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11050, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740530, VtB11050_Delete) { TDelete(ctx, 0x00B11050, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11078, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(007405B8, VtB11078_Delete) { TDelete(ctx, 0x00B11078, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B110A0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740640, VtB110A0_Delete) { TDelete(ctx, 0x00B110A0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B110C8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(007406C8, VtB110C8_Delete) { TDelete(ctx, 0x00B110C8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B110F0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740750, VtB110F0_Delete) { TDelete(ctx, 0x00B110F0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11118, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(007407D8, VtB11118_Delete) { TDelete(ctx, 0x00B11118, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11140, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740860, VtB11140_Delete) { TDelete(ctx, 0x00B11140, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11168, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(007408E8, VtB11168_Delete) { TDelete(ctx, 0x00B11168, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11190, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740970, VtB11190_Delete) { TDelete(ctx, 0x00B11190, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B111B8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(007409C8, VtB111B8_Delete) { TDelete(ctx, 0x00B111B8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11208, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740B04, VtB11208_Delete) { TDelete(ctx, 0x00B11208, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11230, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740B8C, VtB11230_Delete) { TDelete(ctx, 0x00B11230, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11258, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740C14, VtB11258_Delete) { TDelete(ctx, 0x00B11258, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11280, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740C9C, VtB11280_Delete) { TDelete(ctx, 0x00B11280, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B112A8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740D24, VtB112A8_Delete) { TDelete(ctx, 0x00B112A8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B112D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740DAC, VtB112D0_Delete) { TDelete(ctx, 0x00B112D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B112F8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740E34, VtB112F8_Delete) { TDelete(ctx, 0x00B112F8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11320, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740EBC, VtB11320_Delete) { TDelete(ctx, 0x00B11320, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11348, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740F44, VtB11348_Delete) { TDelete(ctx, 0x00B11348, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11370, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00740FCC, VtB11370_Delete) { TDelete(ctx, 0x00B11370, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11398, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00741054, VtB11398_Delete) { TDelete(ctx, 0x00B11398, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B113C0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(007410DC, VtB113C0_Delete) { TDelete(ctx, 0x00B113C0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B113E8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00741164, VtB113E8_Delete) { TDelete(ctx, 0x00B113E8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11410, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(007411EC, VtB11410_Delete) { TDelete(ctx, 0x00B11410, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11438, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00741274, VtB11438_Delete) { TDelete(ctx, 0x00B11438, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11460, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(007412FC, VtB11460_Delete) { TDelete(ctx, 0x00B11460, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11488, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00741384, VtB11488_Delete) { TDelete(ctx, 0x00B11488, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B114B0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0074140C, VtB114B0_Delete) { TDelete(ctx, 0x00B114B0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11530, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00741630, VtB11530_Delete) { TDelete(ctx, 0x00B11530, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11580, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(007417A0, VtB11580_Delete) { TDelete(ctx, 0x00B11580, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B115A8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00741828, VtB115A8_Delete) { TDelete(ctx, 0x00B115A8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B115D0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(007418B0, VtB115D0_Delete) { TDelete(ctx, 0x00B115D0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11620, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(007419BC, VtB11620_Delete) { TDelete(ctx, 0x00B11620, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11648, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00741A14, VtB11648_Delete) { TDelete(ctx, 0x00B11648, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11670, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00741A6C, VtB11670_Delete) { TDelete(ctx, 0x00B11670, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B116C0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00741B44, VtB116C0_Delete) { TDelete(ctx, 0x00B116C0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B116E8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00741BCC, VtB116E8_Delete) { TDelete(ctx, 0x00B116E8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11710, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00741C54, VtB11710_Delete) { TDelete(ctx, 0x00B11710, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11738, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00741CDC, VtB11738_Delete) { TDelete(ctx, 0x00B11738, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11760, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00741D64, VtB11760_Delete) { TDelete(ctx, 0x00B11760, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11788, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00741DEC, VtB11788_Delete) { TDelete(ctx, 0x00B11788, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B117B0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00741E74, VtB117B0_Delete) { TDelete(ctx, 0x00B117B0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B117D8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00741EFC, VtB117D8_Delete) { TDelete(ctx, 0x00B117D8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11800, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00741F84, VtB11800_Delete) { TDelete(ctx, 0x00B11800, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11828, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0074200C, VtB11828_Delete) { TDelete(ctx, 0x00B11828, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11850, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00742094, VtB11850_Delete) { TDelete(ctx, 0x00B11850, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11878, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0074211C, VtB11878_Delete) { TDelete(ctx, 0x00B11878, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B118A0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(007421A4, VtB118A0_Delete) { TDelete(ctx, 0x00B118A0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B118C8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(0074222C, VtB118C8_Delete) { TDelete(ctx, 0x00B118C8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B118F0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(007422B4, VtB118F0_Delete) { TDelete(ctx, 0x00B118F0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11940, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(007423F0, VtB11940_Delete) { TDelete(ctx, 0x00B11940, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11968, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00742478, VtB11968_Delete) { TDelete(ctx, 0x00B11968, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11990, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00742500, VtB11990_Delete) { TDelete(ctx, 0x00B11990, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B119B8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00742588, VtB119B8_Delete) { TDelete(ctx, 0x00B119B8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B119E0, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00742610, VtB119E0_Delete) { TDelete(ctx, 0x00B119E0, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11A08, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00742698, VtB11A08_Delete) { TDelete(ctx, 0x00B11A08, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11A30, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00742720, VtB11A30_Delete) { TDelete(ctx, 0x00B11A30, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11A58, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(007427A8, VtB11A58_Delete) { TDelete(ctx, 0x00B11A58, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11A80, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00742800, VtB11A80_Delete) { TDelete(ctx, 0x00B11A80, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11AA8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(00742888, VtB11AA8_Delete) { TDelete(ctx, 0x00B11AA8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11AF8, 0x004BB41C(this), 0x0054EE6C(this, 0)
DECOMP(007429F8, VtB11AF8_Delete) { TDelete(ctx, 0x00B11AF8, 0x004BB41C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B88, 0x004B6884(this), 0x00552420(this, 88)
DECOMP(00742B3C, VtB11B88_Delete) { TDelete(ctx, 0x00B11B88, 0x004B6884, 0x00552420, 88); }
// deleting: vtable 0x00B11D98, 0x000C2DA0(this), 0x0054EE6C(this, 0)
DECOMP(00743800, VtB11D98_Delete) { TDelete(ctx, 0x00B11D98, 0x000C2DA0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B13940, 0x004FC1A0(this), 0x0054EE6C(this, 0)
DECOMP(0074900C, VtB13940_Delete) { TDelete(ctx, 0x00B13940, 0x004FC1A0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B152C8, 0x000E2A34(this), 0x00552420(this, 336)
DECOMP(007511E8, VtB152C8_Delete) { TDelete(ctx, 0x00B152C8, 0x000E2A34, 0x00552420, 336); }
// deleting: vtable 0x00B15508, 0x000E2A34(this), 0x00552420(this, 336)
DECOMP(00751978, VtB15508_Delete) { TDelete(ctx, 0x00B15508, 0x000E2A34, 0x00552420, 336); }
// deleting: vtable 0x00B15748, 0x000E2A34(this), 0x00552420(this, 320)
DECOMP(00751F7C, VtB15748_Delete) { TDelete(ctx, 0x00B15748, 0x000E2A34, 0x00552420, 320); }
// deleting: vtable 0x00B15BD8, 0x000E2A34(this), 0x00552420(this, 384)
DECOMP(00752580, VtB15BD8_Delete) { TDelete(ctx, 0x00B15BD8, 0x000E2A34, 0x00552420, 384); }
// deleting: vtable 0x00B15E18, 0x000E2A34(this), 0x00552420(this, 336)
DECOMP(00752B84, VtB15E18_Delete) { TDelete(ctx, 0x00B15E18, 0x000E2A34, 0x00552420, 336); }
// deleting: vtable 0x00B16280, 0x000C2DA0(this), 0x00552420(this, 272)
DECOMP(007539CC, VtB16280_Delete) { TDelete(ctx, 0x00B16280, 0x000C2DA0, 0x00552420, 272); }
// deleting: vtable 0x00B163F0, 0x000E2A34(this), 0x00552420(this, 336)
DECOMP(00753ADC, VtB163F0_Delete) { TDelete(ctx, 0x00B163F0, 0x000E2A34, 0x00552420, 336); }
// deleting: vtable 0x00B15970, 0x000E2A34(this), 0x00552420(this, 416)
DECOMP(007540E0, VtB15970_Delete) { TDelete(ctx, 0x00B15970, 0x000E2A34, 0x00552420, 416); }
// deleting: vtable 0x00B15970, 0x000E2A34(this), 0x00552420(this, 416)
DECOMP(00754460, VtB15970_Delete_00754460) { TDelete(ctx, 0x00B15970, 0x000E2A34, 0x00552420, 416); }
// deleting: vtable 0x00B15970, 0x000E2A34(this), 0x00552420(this, 928)
DECOMP(00754A48, VtB15970_Delete_00754A48) { TDelete(ctx, 0x00B15970, 0x000E2A34, 0x00552420, 928); }
// deleting: vtable 0x00B15970, 0x000E2A34(this), 0x00552420(this, 992)
DECOMP(00754DC0, VtB15970_Delete_00754DC0) { TDelete(ctx, 0x00B15970, 0x000E2A34, 0x00552420, 992); }
// deleting: vtable 0x00B15970, 0x000E2A34(this), 0x00552420(this, 496)
DECOMP(00755064, VtB15970_Delete_00755064) { TDelete(ctx, 0x00B15970, 0x000E2A34, 0x00552420, 496); }
// deleting: vtable 0x00B15970, 0x000E2A34(this), 0x00552420(this, 416)
DECOMP(007551C0, VtB15970_Delete_007551C0) { TDelete(ctx, 0x00B15970, 0x000E2A34, 0x00552420, 416); }
// deleting: vtable 0x00B15970, 0x000E2A34(this), 0x00552420(this, 416)
DECOMP(00755248, VtB15970_Delete_00755248) { TDelete(ctx, 0x00B15970, 0x000E2A34, 0x00552420, 416); }
// deleting: vtable 0x00B17948, 0x000E2A34(this), 0x00552420(this, 496)
DECOMP(00756028, VtB17948_Delete) { TDelete(ctx, 0x00B17948, 0x000E2A34, 0x00552420, 496); }
// deleting: vtable 0x00B17B88, 0x000E2A34(this), 0x00552420(this, 320)
DECOMP(0075662C, VtB17B88_Delete) { TDelete(ctx, 0x00B17B88, 0x000E2A34, 0x00552420, 320); }
// deleting: vtable 0x00B18230, 0x000E2A34(this), 0x00552420(this, 384)
DECOMP(00757338, VtB18230_Delete) { TDelete(ctx, 0x00B18230, 0x000E2A34, 0x00552420, 384); }
// deleting: vtable 0x00B18458, 0x000E2A34(this), 0x00552420(this, 304)
DECOMP(007573C0, VtB18458_Delete) { TDelete(ctx, 0x00B18458, 0x000E2A34, 0x00552420, 304); }
// deleting: vtable 0x00B18680, 0x000E2A34(this), 0x00552420(this, 432)
DECOMP(00757450, VtB18680_Delete) { TDelete(ctx, 0x00B18680, 0x000E2A34, 0x00552420, 432); }
// deleting: vtable 0x00B188C0, 0x000E2A34(this), 0x00552420(this, 368)
DECOMP(007575AC, VtB188C0_Delete) { TDelete(ctx, 0x00B188C0, 0x000E2A34, 0x00552420, 368); }
// deleting: vtable 0x00B19378, 0x000C2DA0(this), 0x00552420(this, 192)
DECOMP(00759358, VtB19378_Delete) { TDelete(ctx, 0x00B19378, 0x000C2DA0, 0x00552420, 192); }
// deleting: vtable 0x00B193C8, 0x000D729C(this), 0x00552420(this, 448)
DECOMP(00759468, VtB193C8_Delete) { TDelete(ctx, 0x00B193C8, 0x000D729C, 0x00552420, 448); }
// deleting: vtable 0x00B19510, 0x000C2DA0(this), 0x00552420(this, 192)
DECOMP(0075AE44, VtB19510_Delete) { TDelete(ctx, 0x00B19510, 0x000C2DA0, 0x00552420, 192); }
// deleting: vtable 0x00B19510, 0x000C2DA0(this), 0x00552420(this, 208)
DECOMP(0075AECC, VtB19510_Delete_0075AECC) { TDelete(ctx, 0x00B19510, 0x000C2DA0, 0x00552420, 208); }
// deleting: vtable 0x00B19510, 0x000C2DA0(this), 0x00552420(this, 192)
DECOMP(0075B210, VtB19510_Delete_0075B210) { TDelete(ctx, 0x00B19510, 0x000C2DA0, 0x00552420, 192); }
// deleting: vtable 0x00B19510, 0x000C2DA0(this), 0x00552420(this, 176)
DECOMP(0075B320, VtB19510_Delete_0075B320) { TDelete(ctx, 0x00B19510, 0x000C2DA0, 0x00552420, 176); }
// deleting: vtable 0x00B19510, 0x000C2DA0(this), 0x00552420(this, 224)
DECOMP(0075B3A8, VtB19510_Delete_0075B3A8) { TDelete(ctx, 0x00B19510, 0x000C2DA0, 0x00552420, 224); }
// deleting: vtable 0x00B19510, 0x000C2DA0(this), 0x00552420(this, 208)
DECOMP(0075B4B8, VtB19510_Delete_0075B4B8) { TDelete(ctx, 0x00B19510, 0x000C2DA0, 0x00552420, 208); }
// deleting: vtable 0x00B19510, 0x000C2DA0(this), 0x00552420(this, 192)
DECOMP(0075B5C8, VtB19510_Delete_0075B5C8) { TDelete(ctx, 0x00B19510, 0x000C2DA0, 0x00552420, 192); }
// deleting: vtable 0x00B19510, 0x000C2DA0(this), 0x0054EE6C(this, 0)
DECOMP(0075B6D8, VtB19510_Delete_0075B6D8) { TDelete(ctx, 0x00B19510, 0x000C2DA0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B19510, 0x000C2DA0(this), 0x00552420(this, 176)
DECOMP(0075B89C, VtB19510_Delete_0075B89C) { TDelete(ctx, 0x00B19510, 0x000C2DA0, 0x00552420, 176); }
// deleting: vtable 0x00B19510, 0x000C2DA0(this), 0x00552420(this, 176)
DECOMP(0075B9AC, VtB19510_Delete_0075B9AC) { TDelete(ctx, 0x00B19510, 0x000C2DA0, 0x00552420, 176); }
// deleting: vtable 0x00B19510, 0x000C2DA0(this), 0x00552420(this, 208)
DECOMP(0075BA34, VtB19510_Delete_0075BA34) { TDelete(ctx, 0x00B19510, 0x000C2DA0, 0x00552420, 208); }
// deleting: vtable 0x00B19510, 0x000C2DA0(this), 0x00552420(this, 192)
DECOMP(0075BABC, VtB19510_Delete_0075BABC) { TDelete(ctx, 0x00B19510, 0x000C2DA0, 0x00552420, 192); }
// deleting: vtable 0x00B19510, 0x000C2DA0(this), 0x00552420(this, 224)
DECOMP(0075BDE0, VtB19510_Delete_0075BDE0) { TDelete(ctx, 0x00B19510, 0x000C2DA0, 0x00552420, 224); }
// deleting: vtable 0x00B19510, 0x000C2DA0(this), 0x00552420(this, 176)
DECOMP(0075BE68, VtB19510_Delete_0075BE68) { TDelete(ctx, 0x00B19510, 0x000C2DA0, 0x00552420, 176); }
// deleting: vtable 0x00B1BC30, 0x00464AD0(this), 0x0054EE6C(this, 0)
DECOMP(0075DBBC, VtB1BC30_Delete) { TDelete(ctx, 0x00B1BC30, 0x00464AD0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1D040, 0x00464AD0(this), 0x0054EE6C(this, 0)
DECOMP(0076585C, VtB1D040_Delete) { TDelete(ctx, 0x00B1D040, 0x00464AD0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1D2C8, 0x000180C8(this), 0x00552420(this, 32)
DECOMP(00765B94, VtB1D2C8_Delete) { TDelete(ctx, 0x00B1D2C8, 0x000180C8, 0x00552420, 32); }
// deleting: vtable 0x00B207A8, 0x00490B34(this), 0x0054EE6C(this, 0)
DECOMP(0077EEE8, VtB207A8_Delete) { TDelete(ctx, 0x00B207A8, 0x00490B34, 0x0054EE6C, 0); }
// deleting: vtable 0x00B20CE0, 0x0042CA50(this), 0x00552420(this, 24)
DECOMP(0077F700, VtB20CE0_Delete) { TDelete(ctx, 0x00B20CE0, 0x0042CA50, 0x00552420, 24); }
// deleting: vtable 0x00B20CF8, 0x0042CA50(this), 0x00552420(this, 128)
DECOMP(0077F788, VtB20CF8_Delete) { TDelete(ctx, 0x00B20CF8, 0x0042CA50, 0x00552420, 128); }
// deleting: vtable 0x00B20D10, 0x0042CA50(this), 0x00552420(this, 24)
DECOMP(0077F818, VtB20D10_Delete) { TDelete(ctx, 0x00B20D10, 0x0042CA50, 0x00552420, 24); }
// deleting: vtable 0x00B20D28, 0x0042CA50(this), 0x00552420(this, 24)
DECOMP(0077F8A0, VtB20D28_Delete) { TDelete(ctx, 0x00B20D28, 0x0042CA50, 0x00552420, 24); }
// deleting: vtable 0x00B20D40, 0x0042CA50(this), 0x00552420(this, 20)
DECOMP(0077F930, VtB20D40_Delete) { TDelete(ctx, 0x00B20D40, 0x0042CA50, 0x00552420, 20); }
// deleting: vtable 0x00B20D58, 0x0042CA50(this), 0x00552420(this, 112)
DECOMP(0077F9B8, VtB20D58_Delete) { TDelete(ctx, 0x00B20D58, 0x0042CA50, 0x00552420, 112); }
// deleting: vtable 0x00B20D70, 0x0042CA50(this), 0x00552420(this, 24)
DECOMP(0077FA40, VtB20D70_Delete) { TDelete(ctx, 0x00B20D70, 0x0042CA50, 0x00552420, 24); }
// deleting: vtable 0x00B20D88, 0x0042CA50(this), 0x00552420(this, 36)
DECOMP(0077FAC8, VtB20D88_Delete) { TDelete(ctx, 0x00B20D88, 0x0042CA50, 0x00552420, 36); }
// deleting: vtable 0x00B20DA0, 0x0042CA50(this), 0x00552420(this, 28)
DECOMP(0077FB58, VtB20DA0_Delete) { TDelete(ctx, 0x00B20DA0, 0x0042CA50, 0x00552420, 28); }
// deleting: vtable 0x00B20DB8, 0x0042CA50(this), 0x00552420(this, 24)
DECOMP(0077FBE8, VtB20DB8_Delete) { TDelete(ctx, 0x00B20DB8, 0x0042CA50, 0x00552420, 24); }
// deleting: vtable 0x00B20DD0, 0x0042CA50(this), 0x00552420(this, 36)
DECOMP(0077FC78, VtB20DD0_Delete) { TDelete(ctx, 0x00B20DD0, 0x0042CA50, 0x00552420, 36); }
// deleting: vtable 0x00B20DE8, 0x0042CA50(this), 0x00552420(this, 20)
DECOMP(0077FD00, VtB20DE8_Delete) { TDelete(ctx, 0x00B20DE8, 0x0042CA50, 0x00552420, 20); }
// deleting: vtable 0x00B20E00, 0x0042CA50(this), 0x00552420(this, 12)
DECOMP(0077FD90, VtB20E00_Delete) { TDelete(ctx, 0x00B20E00, 0x0042CA50, 0x00552420, 12); }
// deleting: vtable 0x00B20E18, 0x0042CA50(this), 0x00552420(this, 24)
DECOMP(0077FE20, VtB20E18_Delete) { TDelete(ctx, 0x00B20E18, 0x0042CA50, 0x00552420, 24); }
// deleting: vtable 0x00B20E30, 0x0042CA50(this), 0x00552420(this, 20)
DECOMP(0077FEA8, VtB20E30_Delete) { TDelete(ctx, 0x00B20E30, 0x0042CA50, 0x00552420, 20); }
// deleting: vtable 0x00B20E48, 0x0042CA50(this), 0x00552420(this, 12)
DECOMP(0077FF30, VtB20E48_Delete) { TDelete(ctx, 0x00B20E48, 0x0042CA50, 0x00552420, 12); }
// deleting: vtable 0x00B20E60, 0x0042CA50(this), 0x00552420(this, 24)
DECOMP(0077FFB8, VtB20E60_Delete) { TDelete(ctx, 0x00B20E60, 0x0042CA50, 0x00552420, 24); }
// deleting: vtable 0x00B20E78, 0x0042CA50(this), 0x00552420(this, 20)
DECOMP(00780010, VtB20E78_Delete) { TDelete(ctx, 0x00B20E78, 0x0042CA50, 0x00552420, 20); }
// deleting: vtable 0x00B20EA8, 0x0042CA50(this), 0x00552420(this, 32)
DECOMP(00780098, VtB20EA8_Delete) { TDelete(ctx, 0x00B20EA8, 0x0042CA50, 0x00552420, 32); }
// deleting: vtable 0x00B20EC0, 0x0042CA50(this), 0x00552420(this, 32)
DECOMP(00780120, VtB20EC0_Delete) { TDelete(ctx, 0x00B20EC0, 0x0042CA50, 0x00552420, 32); }
// deleting: vtable 0x00B20ED8, 0x0042CA50(this), 0x00552420(this, 52)
DECOMP(007801A8, VtB20ED8_Delete) { TDelete(ctx, 0x00B20ED8, 0x0042CA50, 0x00552420, 52); }
// deleting: vtable 0x00B20EF0, 0x0042CA50(this), 0x00552420(this, 32)
DECOMP(00780230, VtB20EF0_Delete) { TDelete(ctx, 0x00B20EF0, 0x0042CA50, 0x00552420, 32); }
// deleting: vtable 0x00B20F08, 0x0042CA50(this), 0x00552420(this, 132)
DECOMP(007802C0, VtB20F08_Delete) { TDelete(ctx, 0x00B20F08, 0x0042CA50, 0x00552420, 132); }
// deleting: vtable 0x00B20F20, 0x0042CA50(this), 0x00552420(this, 24)
DECOMP(00780348, VtB20F20_Delete) { TDelete(ctx, 0x00B20F20, 0x0042CA50, 0x00552420, 24); }
// deleting: vtable 0x00B20F38, 0x0042CA50(this), 0x00552420(this, 32)
DECOMP(007803D0, VtB20F38_Delete) { TDelete(ctx, 0x00B20F38, 0x0042CA50, 0x00552420, 32); }
// deleting: vtable 0x00B20F50, 0x0042CA50(this), 0x00552420(this, 20)
DECOMP(00780458, VtB20F50_Delete) { TDelete(ctx, 0x00B20F50, 0x0042CA50, 0x00552420, 20); }
// deleting: vtable 0x00B20F80, 0x0042CA50(this), 0x00552420(this, 40)
DECOMP(007804E8, VtB20F80_Delete) { TDelete(ctx, 0x00B20F80, 0x0042CA50, 0x00552420, 40); }
// deleting: vtable 0x00B20F98, 0x0042CA50(this), 0x00552420(this, 128)
DECOMP(00780570, VtB20F98_Delete) { TDelete(ctx, 0x00B20F98, 0x0042CA50, 0x00552420, 128); }
// deleting: vtable 0x00B20FB0, 0x0042CA50(this), 0x00552420(this, 36)
DECOMP(007805F8, VtB20FB0_Delete) { TDelete(ctx, 0x00B20FB0, 0x0042CA50, 0x00552420, 36); }
// deleting: vtable 0x00B20FC8, 0x0042CA50(this), 0x00552420(this, 20)
DECOMP(00780680, VtB20FC8_Delete) { TDelete(ctx, 0x00B20FC8, 0x0042CA50, 0x00552420, 20); }
// deleting: vtable 0x00B20FE0, 0x0042CA50(this), 0x00552420(this, 12)
DECOMP(00780708, VtB20FE0_Delete) { TDelete(ctx, 0x00B20FE0, 0x0042CA50, 0x00552420, 12); }
// deleting: vtable 0x00B20FF8, 0x0042CA50(this), 0x00552420(this, 12)
DECOMP(00780790, VtB20FF8_Delete) { TDelete(ctx, 0x00B20FF8, 0x0042CA50, 0x00552420, 12); }
// deleting: vtable 0x00B21010, 0x0042CA50(this), 0x00552420(this, 12)
DECOMP(00780818, VtB21010_Delete) { TDelete(ctx, 0x00B21010, 0x0042CA50, 0x00552420, 12); }
// deleting: vtable 0x00B21028, 0x0042CA50(this), 0x00552420(this, 12)
DECOMP(007808A0, VtB21028_Delete) { TDelete(ctx, 0x00B21028, 0x0042CA50, 0x00552420, 12); }
// deleting: vtable 0x00B21118, 0x0042CA50(this), 0x00552420(this, 40)
DECOMP(00780AC8, VtB21118_Delete) { TDelete(ctx, 0x00B21118, 0x0042CA50, 0x00552420, 40); }
// deleting: vtable 0x00B21130, 0x0042CA50(this), 0x00552420(this, 20)
DECOMP(00780B20, VtB21130_Delete) { TDelete(ctx, 0x00B21130, 0x0042CA50, 0x00552420, 20); }
// deleting: vtable 0x00B21148, 0x0042CA50(this), 0x00552420(this, 112)
DECOMP(00780B78, VtB21148_Delete) { TDelete(ctx, 0x00B21148, 0x0042CA50, 0x00552420, 112); }
// deleting: vtable 0x00B211A8, 0x0042CA50(this), 0x00552420(this, 40)
DECOMP(00780C90, VtB211A8_Delete) { TDelete(ctx, 0x00B211A8, 0x0042CA50, 0x00552420, 40); }
// deleting: vtable 0x00B211C0, 0x0042CA50(this), 0x00552420(this, 12)
DECOMP(00780D18, VtB211C0_Delete) { TDelete(ctx, 0x00B211C0, 0x0042CA50, 0x00552420, 12); }
// deleting: vtable 0x00B211D8, 0x0042CA50(this), 0x00552420(this, 12)
DECOMP(00780DA0, VtB211D8_Delete) { TDelete(ctx, 0x00B211D8, 0x0042CA50, 0x00552420, 12); }
// deleting: vtable 0x00B211F0, 0x0042CA50(this), 0x00552420(this, 32)
DECOMP(00780E28, VtB211F0_Delete) { TDelete(ctx, 0x00B211F0, 0x0042CA50, 0x00552420, 32); }
// deleting: vtable 0x00B21208, 0x0042CA50(this), 0x00552420(this, 24)
DECOMP(00780EB0, VtB21208_Delete) { TDelete(ctx, 0x00B21208, 0x0042CA50, 0x00552420, 24); }
// deleting: vtable 0x00B21220, 0x0042CA50(this), 0x00552420(this, 16)
DECOMP(00780F40, VtB21220_Delete) { TDelete(ctx, 0x00B21220, 0x0042CA50, 0x00552420, 16); }
// deleting: vtable 0x00B21238, 0x0042CA50(this), 0x00552420(this, 12)
DECOMP(00780FC8, VtB21238_Delete) { TDelete(ctx, 0x00B21238, 0x0042CA50, 0x00552420, 12); }
// deleting: vtable 0x00B21250, 0x0042CA50(this), 0x00552420(this, 12)
DECOMP(00781050, VtB21250_Delete) { TDelete(ctx, 0x00B21250, 0x0042CA50, 0x00552420, 12); }
// deleting: vtable 0x00B21268, 0x0042CA50(this), 0x00552420(this, 12)
DECOMP(007810D8, VtB21268_Delete) { TDelete(ctx, 0x00B21268, 0x0042CA50, 0x00552420, 12); }
// deleting: vtable 0x00B21280, 0x0042CA50(this), 0x00552420(this, 28)
DECOMP(00781160, VtB21280_Delete) { TDelete(ctx, 0x00B21280, 0x0042CA50, 0x00552420, 28); }
// deleting: vtable 0x00B21298, 0x0042CA50(this), 0x00552420(this, 20)
DECOMP(007811F0, VtB21298_Delete) { TDelete(ctx, 0x00B21298, 0x0042CA50, 0x00552420, 20); }
// deleting: vtable 0x00B212B0, 0x0042CA50(this), 0x00552420(this, 20)
DECOMP(00781278, VtB212B0_Delete) { TDelete(ctx, 0x00B212B0, 0x0042CA50, 0x00552420, 20); }
// deleting: vtable 0x00B212E0, 0x0042CA50(this), 0x00552420(this, 24)
DECOMP(00781300, VtB212E0_Delete) { TDelete(ctx, 0x00B212E0, 0x0042CA50, 0x00552420, 24); }
// deleting: vtable 0x00B212F8, 0x0042CA50(this), 0x00552420(this, 20)
DECOMP(00781390, VtB212F8_Delete) { TDelete(ctx, 0x00B212F8, 0x0042CA50, 0x00552420, 20); }
// deleting: vtable 0x00B21310, 0x0042CA50(this), 0x00552420(this, 24)
DECOMP(00781420, VtB21310_Delete) { TDelete(ctx, 0x00B21310, 0x0042CA50, 0x00552420, 24); }
// deleting: vtable 0x00B21328, 0x0042CA50(this), 0x00552420(this, 80)
DECOMP(007814B0, VtB21328_Delete) { TDelete(ctx, 0x00B21328, 0x0042CA50, 0x00552420, 80); }
// deleting: vtable 0x00B21340, 0x0042CA50(this), 0x00552420(this, 28)
DECOMP(00781540, VtB21340_Delete) { TDelete(ctx, 0x00B21340, 0x0042CA50, 0x00552420, 28); }
// deleting: vtable 0x00B21358, 0x0042CA50(this), 0x00552420(this, 20)
DECOMP(007815C8, VtB21358_Delete) { TDelete(ctx, 0x00B21358, 0x0042CA50, 0x00552420, 20); }
// deleting: vtable 0x00B21370, 0x0042CA50(this), 0x00552420(this, 12)
DECOMP(00781650, VtB21370_Delete) { TDelete(ctx, 0x00B21370, 0x0042CA50, 0x00552420, 12); }
// deleting: vtable 0x00B21388, 0x0042CA50(this), 0x00552420(this, 160)
DECOMP(007816E0, VtB21388_Delete) { TDelete(ctx, 0x00B21388, 0x0042CA50, 0x00552420, 160); }
// deleting: vtable 0x00B213A0, 0x0042CA50(this), 0x00552420(this, 16)
DECOMP(00781770, VtB213A0_Delete) { TDelete(ctx, 0x00B213A0, 0x0042CA50, 0x00552420, 16); }
// deleting: vtable 0x00B213B8, 0x0042CA50(this), 0x00552420(this, 24)
DECOMP(00781800, VtB213B8_Delete) { TDelete(ctx, 0x00B213B8, 0x0042CA50, 0x00552420, 24); }
// deleting: vtable 0x00B213D0, 0x0042CA50(this), 0x00552420(this, 12)
DECOMP(00781860, VtB213D0_Delete) { TDelete(ctx, 0x00B213D0, 0x0042CA50, 0x00552420, 12); }
// deleting: vtable 0x00B213E8, 0x0042CA50(this), 0x00552420(this, 36)
DECOMP(007818E8, VtB213E8_Delete) { TDelete(ctx, 0x00B213E8, 0x0042CA50, 0x00552420, 36); }
// deleting: vtable 0x00B21418, 0x0042CA50(this), 0x00552420(this, 20)
DECOMP(00781A58, VtB21418_Delete) { TDelete(ctx, 0x00B21418, 0x0042CA50, 0x00552420, 20); }
// deleting: vtable 0x00B21470, 0x004F7108(this), 0x0054EE6C(this, 0)
DECOMP(0078223C, VtB21470_Delete) { TDelete(ctx, 0x00B21470, 0x004F7108, 0x0054EE6C, 0); }
// deleting: vtable 0x00B214A0, 0x004F79C0(this), 0x00552420(this, 36)
DECOMP(00782294, VtB214A0_Delete) { TDelete(ctx, 0x00B214A0, 0x004F79C0, 0x00552420, 36); }
// deleting: vtable 0x00B214B8, 0x004F7108(this), 0x0054EE6C(this, 0)
DECOMP(00782374, VtB214B8_Delete) { TDelete(ctx, 0x00B214B8, 0x004F7108, 0x0054EE6C, 0); }
// deleting: vtable 0x00B214FC, 0x004F79C0(this), 0x00552420(this, 36)
DECOMP(007823CC, VtB214FC_Delete) { TDelete(ctx, 0x00B214FC, 0x004F79C0, 0x00552420, 36); }
// deleting: vtable 0x00B21518, 0x004F7108(this), 0x0054EE6C(this, 0)
DECOMP(00782438, VtB21518_Delete) { TDelete(ctx, 0x00B21518, 0x004F7108, 0x0054EE6C, 0); }
// deleting: vtable 0x00B21548, 0x004F79C0(this), 0x0054EE6C(this, 0)
DECOMP(00782490, VtB21548_Delete) { TDelete(ctx, 0x00B21548, 0x004F79C0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B21518, 0x004F7108(this), 0x00552420(this, 92)
DECOMP(00782568, VtB21518_Delete_00782568) { TDelete(ctx, 0x00B21518, 0x004F7108, 0x00552420, 92); }
// deleting: vtable 0x00B21548, 0x004F79C0(this), 0x00552420(this, 36)
DECOMP(007825C0, VtB21548_Delete_007825C0) { TDelete(ctx, 0x00B21548, 0x004F79C0, 0x00552420, 36); }
// deleting: vtable 0x00B21518, 0x004F7108(this), 0x00552420(this, 92)
DECOMP(00782698, VtB21518_Delete_00782698) { TDelete(ctx, 0x00B21518, 0x004F7108, 0x00552420, 92); }
// deleting: vtable 0x00B21548, 0x004F79C0(this), 0x00552420(this, 36)
DECOMP(007826F0, VtB21548_Delete_007826F0) { TDelete(ctx, 0x00B21548, 0x004F79C0, 0x00552420, 36); }
// deleting: vtable 0x00B215F0, 0x004F7108(this), 0x0054EE6C(this, 0)
DECOMP(007827CC, VtB215F0_Delete) { TDelete(ctx, 0x00B215F0, 0x004F7108, 0x0054EE6C, 0); }
// deleting: vtable 0x00B21620, 0x004F79C0(this), 0x00552420(this, 36)
DECOMP(00782824, VtB21620_Delete) { TDelete(ctx, 0x00B21620, 0x004F79C0, 0x00552420, 36); }
// deleting: vtable 0x00B21638, 0x004F7108(this), 0x0054EE6C(this, 0)
DECOMP(00782914, VtB21638_Delete) { TDelete(ctx, 0x00B21638, 0x004F7108, 0x0054EE6C, 0); }
// deleting: vtable 0x00B21668, 0x004F79C0(this), 0x00552420(this, 40)
DECOMP(0078296C, VtB21668_Delete) { TDelete(ctx, 0x00B21668, 0x004F79C0, 0x00552420, 40); }
// deleting: vtable 0x00B21680, 0x004F7108(this), 0x0054EE6C(this, 0)
DECOMP(00782A4C, VtB21680_Delete) { TDelete(ctx, 0x00B21680, 0x004F7108, 0x0054EE6C, 0); }
// deleting: vtable 0x00B216B0, 0x004F79C0(this), 0x00552420(this, 36)
DECOMP(00782AA4, VtB216B0_Delete) { TDelete(ctx, 0x00B216B0, 0x004F79C0, 0x00552420, 36); }
// deleting: vtable 0x00B216C8, 0x004F7108(this), 0x0054EE6C(this, 0)
DECOMP(00782B84, VtB216C8_Delete) { TDelete(ctx, 0x00B216C8, 0x004F7108, 0x0054EE6C, 0); }
// deleting: vtable 0x00B216F8, 0x004F79C0(this), 0x00552420(this, 36)
DECOMP(00782BDC, VtB216F8_Delete) { TDelete(ctx, 0x00B216F8, 0x004F79C0, 0x00552420, 36); }
// deleting: vtable 0x00B21710, 0x004F7108(this), 0x0054EE6C(this, 0)
DECOMP(00782CBC, VtB21710_Delete) { TDelete(ctx, 0x00B21710, 0x004F7108, 0x0054EE6C, 0); }
// deleting: vtable 0x00B21740, 0x004F79C0(this), 0x00552420(this, 36)
DECOMP(00782D14, VtB21740_Delete) { TDelete(ctx, 0x00B21740, 0x004F79C0, 0x00552420, 36); }
// deleting: vtable 0x00B21758, 0x004F7108(this), 0x0054EE6C(this, 0)
DECOMP(00782DF4, VtB21758_Delete) { TDelete(ctx, 0x00B21758, 0x004F7108, 0x0054EE6C, 0); }
// deleting: vtable 0x00B21788, 0x004F79C0(this), 0x00552420(this, 36)
DECOMP(00782E4C, VtB21788_Delete) { TDelete(ctx, 0x00B21788, 0x004F79C0, 0x00552420, 36); }
// deleting: vtable 0x00B217A0, 0x004F7108(this), 0x0054EE6C(this, 0)
DECOMP(00782F2C, VtB217A0_Delete) { TDelete(ctx, 0x00B217A0, 0x004F7108, 0x0054EE6C, 0); }
// deleting: vtable 0x00B217D0, 0x004F79C0(this), 0x00552420(this, 36)
DECOMP(00782F84, VtB217D0_Delete) { TDelete(ctx, 0x00B217D0, 0x004F79C0, 0x00552420, 36); }
// deleting: vtable 0x00B217F8, 0x004F7108(this), 0x0054EE6C(this, 0)
DECOMP(00783064, VtB217F8_Delete) { TDelete(ctx, 0x00B217F8, 0x004F7108, 0x0054EE6C, 0); }
// deleting: vtable 0x00B21828, 0x004F79C0(this), 0x00552420(this, 36)
DECOMP(007830BC, VtB21828_Delete) { TDelete(ctx, 0x00B21828, 0x004F79C0, 0x00552420, 36); }
// deleting: vtable 0x00B21840, 0x004F7108(this), 0x0054EE6C(this, 0)
DECOMP(0078319C, VtB21840_Delete) { TDelete(ctx, 0x00B21840, 0x004F7108, 0x0054EE6C, 0); }
// deleting: vtable 0x00B21870, 0x004F79C0(this), 0x00552420(this, 36)
DECOMP(007831F4, VtB21870_Delete) { TDelete(ctx, 0x00B21870, 0x004F79C0, 0x00552420, 36); }
// deleting: vtable 0x00B21888, 0x004F7108(this), 0x0054EE6C(this, 0)
DECOMP(007832D4, VtB21888_Delete) { TDelete(ctx, 0x00B21888, 0x004F7108, 0x0054EE6C, 0); }
// deleting: vtable 0x00B218B8, 0x004F79C0(this), 0x00552420(this, 36)
DECOMP(0078332C, VtB218B8_Delete) { TDelete(ctx, 0x00B218B8, 0x004F79C0, 0x00552420, 36); }

// ---------------------------------------------------------------- trivial destructors: only the vtable (108)
// *this = vtable 0x00B0DBF8
DECOMP(00732DC8, VtB0DBF8_DtorTrivial) { TSetVtbl(ctx, 0x00B0DBF8); }
// *this = vtable 0x00B0DBF8
DECOMP(00732DD8, VtB0DBF8_DtorTrivial_00732DD8) { TSetVtbl(ctx, 0x00B0DBF8); }
// *this = vtable 0x00B0DBF8
DECOMP(00732DE8, VtB0DBF8_DtorTrivial_00732DE8) { TSetVtbl(ctx, 0x00B0DBF8); }
// *this = vtable 0x00B0DBF8
DECOMP(00732DF8, VtB0DBF8_DtorTrivial_00732DF8) { TSetVtbl(ctx, 0x00B0DBF8); }
// *this = vtable 0x00B0DBF8
DECOMP(00732F28, VtB0DBF8_DtorTrivial_00732F28) { TSetVtbl(ctx, 0x00B0DBF8); }
// *this = vtable 0x00B0DABC
DECOMP(00733D50, VtB0DABC_DtorTrivial) { TSetVtbl(ctx, 0x00B0DABC); }
// *this = vtable 0x00B0DABC
DECOMP(00733D60, VtB0DABC_DtorTrivial_00733D60) { TSetVtbl(ctx, 0x00B0DABC); }
// *this = vtable 0x00B0DB3C
DECOMP(00733D70, VtB0DB3C_DtorTrivial) { TSetVtbl(ctx, 0x00B0DB3C); }
// *this = vtable 0x00B0DB3C
DECOMP(00733D80, VtB0DB3C_DtorTrivial_00733D80) { TSetVtbl(ctx, 0x00B0DB3C); }
// *this = vtable 0x00B0DB3C
DECOMP(00733D90, VtB0DB3C_DtorTrivial_00733D90) { TSetVtbl(ctx, 0x00B0DB3C); }
// *this = vtable 0x00B0DB3C
DECOMP(00733DA0, VtB0DB3C_DtorTrivial_00733DA0) { TSetVtbl(ctx, 0x00B0DB3C); }
// *this = vtable 0x00B0DB3C
DECOMP(00733DB0, VtB0DB3C_DtorTrivial_00733DB0) { TSetVtbl(ctx, 0x00B0DB3C); }
// *this = vtable 0x00B0DB3C
DECOMP(00733DC0, VtB0DB3C_DtorTrivial_00733DC0) { TSetVtbl(ctx, 0x00B0DB3C); }
// *this = vtable 0x00B0DBF8
DECOMP(00735F18, VtB0DBF8_DtorTrivial_00735F18) { TSetVtbl(ctx, 0x00B0DBF8); }
// *this = vtable 0x00B0EF40
DECOMP(007369FC, VtB0EF40_DtorTrivial) { TSetVtbl(ctx, 0x00B0EF40); }
// *this = vtable 0x00B0DABC
DECOMP(007375FC, VtB0DABC_DtorTrivial_007375FC) { TSetVtbl(ctx, 0x00B0DABC); }
// *this = vtable 0x00B0DAEC
DECOMP(00737850, VtB0DAEC_DtorTrivial) { TSetVtbl(ctx, 0x00B0DAEC); }
// *this = vtable 0x00B0DB3C
DECOMP(0073799C, VtB0DB3C_DtorTrivial_0073799C) { TSetVtbl(ctx, 0x00B0DB3C); }
// *this = vtable 0x00B0DABC
DECOMP(00739C4C, VtB0DABC_DtorTrivial_00739C4C) { TSetVtbl(ctx, 0x00B0DABC); }
// *this = vtable 0x00B11B20
DECOMP(00742A50, VtB11B20_DtorTrivial) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B1200C
DECOMP(00743558, VtB1200C_DtorTrivial) { TSetVtbl(ctx, 0x00B1200C); }
// *this = vtable 0x00B11DC4
DECOMP(007438B0, VtB11DC4_DtorTrivial) { TSetVtbl(ctx, 0x00B11DC4); }
// *this = vtable 0x00B12034
DECOMP(00744304, VtB12034_DtorTrivial) { TSetVtbl(ctx, 0x00B12034); }
// *this = vtable 0x00B0DABC
DECOMP(00746994, VtB0DABC_DtorTrivial_00746994) { TSetVtbl(ctx, 0x00B0DABC); }
// *this = vtable 0x00B0DABC
DECOMP(007469A4, VtB0DABC_DtorTrivial_007469A4) { TSetVtbl(ctx, 0x00B0DABC); }
// *this = vtable 0x00B12528
DECOMP(00746BC0, VtB12528_DtorTrivial) { TSetVtbl(ctx, 0x00B12528); }
// *this = vtable 0x00B13840
DECOMP(007484D4, VtB13840_DtorTrivial) { TSetVtbl(ctx, 0x00B13840); }
// *this = vtable 0x00B13840
DECOMP(0074854C, VtB13840_DtorTrivial_0074854C) { TSetVtbl(ctx, 0x00B13840); }
// *this = vtable 0x00B13840
DECOMP(007485A0, VtB13840_DtorTrivial_007485A0) { TSetVtbl(ctx, 0x00B13840); }
// *this = vtable 0x00B13974
DECOMP(00748A20, VtB13974_DtorTrivial) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B13974
DECOMP(00748A30, VtB13974_DtorTrivial_00748A30) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B0DAEC
DECOMP(00748A88, VtB0DAEC_DtorTrivial_00748A88) { TSetVtbl(ctx, 0x00B0DAEC); }
// *this = vtable 0x00B0DAEC
DECOMP(00748C1C, VtB0DAEC_DtorTrivial_00748C1C) { TSetVtbl(ctx, 0x00B0DAEC); }
// *this = vtable 0x00B13974
DECOMP(00749064, VtB13974_DtorTrivial_00749064) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B13C84
DECOMP(007490D0, VtB13C84_DtorTrivial) { TSetVtbl(ctx, 0x00B13C84); }
// *this = vtable 0x00B13C9C
DECOMP(0074915C, VtB13C9C_DtorTrivial) { TSetVtbl(ctx, 0x00B13C9C); }
// *this = vtable 0x00B13988
DECOMP(0074937C, VtB13988_DtorTrivial) { TSetVtbl(ctx, 0x00B13988); }
// *this = vtable 0x00B13998
DECOMP(007493D0, VtB13998_DtorTrivial) { TSetVtbl(ctx, 0x00B13998); }
// *this = vtable 0x00B139A8
DECOMP(00749424, VtB139A8_DtorTrivial) { TSetVtbl(ctx, 0x00B139A8); }
// *this = vtable 0x00B13974
DECOMP(007499E4, VtB13974_DtorTrivial_007499E4) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B13974
DECOMP(007499F4, VtB13974_DtorTrivial_007499F4) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B13974
DECOMP(00749A04, VtB13974_DtorTrivial_00749A04) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B13974
DECOMP(007594C8, VtB13974_DtorTrivial_007594C8) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B13974
DECOMP(007594D8, VtB13974_DtorTrivial_007594D8) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B13974
DECOMP(007594E8, VtB13974_DtorTrivial_007594E8) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B13C9C
DECOMP(007594F8, VtB13C9C_DtorTrivial_007594F8) { TSetVtbl(ctx, 0x00B13C9C); }
// *this = vtable 0x00B194F8
DECOMP(00759C3C, VtB194F8_DtorTrivial) { TSetVtbl(ctx, 0x00B194F8); }
// *this = vtable 0x00B13974
DECOMP(0075BEC0, VtB13974_DtorTrivial_0075BEC0) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B13974
DECOMP(0075BED0, VtB13974_DtorTrivial_0075BED0) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B13974
DECOMP(0075C044, VtB13974_DtorTrivial_0075C044) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B13974
DECOMP(0075D338, VtB13974_DtorTrivial_0075D338) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B13974
DECOMP(0075D378, VtB13974_DtorTrivial_0075D378) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B13974
DECOMP(0075DC14, VtB13974_DtorTrivial_0075DC14) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B13974
DECOMP(0075FF10, VtB13974_DtorTrivial_0075FF10) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B1BE78
DECOMP(00762AB8, VtB1BE78_DtorTrivial) { TSetVtbl(ctx, 0x00B1BE78); }
// *this = vtable 0x00B1BE78
DECOMP(00762B64, VtB1BE78_DtorTrivial_00762B64) { TSetVtbl(ctx, 0x00B1BE78); }
// *this = vtable 0x00B1D4D8
DECOMP(00767A38, VtB1D4D8_DtorTrivial) { TSetVtbl(ctx, 0x00B1D4D8); }
// *this = vtable 0x00B1D600
DECOMP(007681C4, VtB1D600_DtorTrivial) { TSetVtbl(ctx, 0x00B1D600); }
// *this = vtable 0x00B1D6A0
DECOMP(007681D4, VtB1D6A0_DtorTrivial) { TSetVtbl(ctx, 0x00B1D6A0); }
// *this = vtable 0x00B1D6A0
DECOMP(007681E4, VtB1D6A0_DtorTrivial_007681E4) { TSetVtbl(ctx, 0x00B1D6A0); }
// *this = vtable 0x00B1D6A0
DECOMP(007681F4, VtB1D6A0_DtorTrivial_007681F4) { TSetVtbl(ctx, 0x00B1D6A0); }
// *this = vtable 0x00B1D6A0
DECOMP(00768204, VtB1D6A0_DtorTrivial_00768204) { TSetVtbl(ctx, 0x00B1D6A0); }
// *this = vtable 0x00B1D9B8
DECOMP(00768214, VtB1D9B8_DtorTrivial) { TSetVtbl(ctx, 0x00B1D9B8); }
// *this = vtable 0x00B1D9B8
DECOMP(00768224, VtB1D9B8_DtorTrivial_00768224) { TSetVtbl(ctx, 0x00B1D9B8); }
// *this = vtable 0x00B1D9B8
DECOMP(00768234, VtB1D9B8_DtorTrivial_00768234) { TSetVtbl(ctx, 0x00B1D9B8); }
// *this = vtable 0x00B1D9B8
DECOMP(00768244, VtB1D9B8_DtorTrivial_00768244) { TSetVtbl(ctx, 0x00B1D9B8); }
// *this = vtable 0x00B1D9B8
DECOMP(00768254, VtB1D9B8_DtorTrivial_00768254) { TSetVtbl(ctx, 0x00B1D9B8); }
// *this = vtable 0x00B1D9B8
DECOMP(00768264, VtB1D9B8_DtorTrivial_00768264) { TSetVtbl(ctx, 0x00B1D9B8); }
// *this = vtable 0x00B1D9B8
DECOMP(00768274, VtB1D9B8_DtorTrivial_00768274) { TSetVtbl(ctx, 0x00B1D9B8); }
// *this = vtable 0x00B1D6A0
DECOMP(00768518, VtB1D6A0_DtorTrivial_00768518) { TSetVtbl(ctx, 0x00B1D6A0); }
// *this = vtable 0x00B1D9B8
DECOMP(007686B0, VtB1D9B8_DtorTrivial_007686B0) { TSetVtbl(ctx, 0x00B1D9B8); }
// *this = vtable 0x00B13974
DECOMP(0076BE7C, VtB13974_DtorTrivial_0076BE7C) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B13974
DECOMP(0076D690, VtB13974_DtorTrivial_0076D690) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B0EF40
DECOMP(0077035C, VtB0EF40_DtorTrivial_0077035C) { TSetVtbl(ctx, 0x00B0EF40); }
// *this = vtable 0x00F2B810
DECOMP(0077036C, VtF2B810_DtorTrivial) { TSetVtbl(ctx, 0x00F2B810); }
// *this = vtable 0x00B1F228
DECOMP(007703D0, VtB1F228_DtorTrivial) { TSetVtbl(ctx, 0x00B1F228); }
// *this = vtable 0x00B1F23C
DECOMP(00770424, VtB1F23C_DtorTrivial) { TSetVtbl(ctx, 0x00B1F23C); }
// *this = vtable 0x00B1F250
DECOMP(00770478, VtB1F250_DtorTrivial) { TSetVtbl(ctx, 0x00B1F250); }
// *this = vtable 0x00B1F26C
DECOMP(007704CC, VtB1F26C_DtorTrivial) { TSetVtbl(ctx, 0x00B1F26C); }
// *this = vtable 0x00B1F4C0
DECOMP(007706D8, VtB1F4C0_DtorTrivial) { TSetVtbl(ctx, 0x00B1F4C0); }
// *this = vtable 0x00B1F4E0
DECOMP(00770730, VtB1F4E0_DtorTrivial) { TSetVtbl(ctx, 0x00B1F4E0); }
// *this = vtable 0x00B13974
DECOMP(00771D40, VtB13974_DtorTrivial_00771D40) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B1F668
DECOMP(007727D8, VtB1F668_DtorTrivial) { TSetVtbl(ctx, 0x00B1F668); }
// *this = vtable 0x00B1F668
DECOMP(007727E8, VtB1F668_DtorTrivial_007727E8) { TSetVtbl(ctx, 0x00B1F668); }
// *this = vtable 0x00B1F668
DECOMP(007727F8, VtB1F668_DtorTrivial_007727F8) { TSetVtbl(ctx, 0x00B1F668); }
// *this = vtable 0x00B1F668
DECOMP(007728EC, VtB1F668_DtorTrivial_007728EC) { TSetVtbl(ctx, 0x00B1F668); }
// *this = vtable 0x00B1F668
DECOMP(00772C24, VtB1F668_DtorTrivial_00772C24) { TSetVtbl(ctx, 0x00B1F668); }
// *this = vtable 0x00B1F668
DECOMP(00773510, VtB1F668_DtorTrivial_00773510) { TSetVtbl(ctx, 0x00B1F668); }
// *this = vtable 0x00B0DBF8
DECOMP(00781AB0, VtB0DBF8_DtorTrivial_00781AB0) { TSetVtbl(ctx, 0x00B0DBF8); }
// *this = vtable 0x00B0DBF8
DECOMP(00781AC0, VtB0DBF8_DtorTrivial_00781AC0) { TSetVtbl(ctx, 0x00B0DBF8); }
// *this = vtable 0x00B0DBF8
DECOMP(00781AD0, VtB0DBF8_DtorTrivial_00781AD0) { TSetVtbl(ctx, 0x00B0DBF8); }
// *this = vtable 0x00B13974
DECOMP(00786A08, VtB13974_DtorTrivial_00786A08) { TSetVtbl(ctx, 0x00B13974); }
// *this = vtable 0x00B21A88
DECOMP(007887A8, VtB21A88_DtorTrivial) { TSetVtbl(ctx, 0x00B21A88); }
// *this = vtable 0x00B21A88
DECOMP(007889AC, VtB21A88_DtorTrivial_007889AC) { TSetVtbl(ctx, 0x00B21A88); }
// *this = vtable 0x00B11B20
DECOMP(00793B94, VtB11B20_DtorTrivial_00793B94) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(00793BA4, VtB11B20_DtorTrivial_00793BA4) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(00793BB4, VtB11B20_DtorTrivial_00793BB4) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(007940D0, VtB11B20_DtorTrivial_007940D0) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(00794210, VtB11B20_DtorTrivial_00794210) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B11B20
DECOMP(00794400, VtB11B20_DtorTrivial_00794400) { TSetVtbl(ctx, 0x00B11B20); }
// *this = vtable 0x00B228C4
DECOMP(00794A6C, VtB228C4_DtorTrivial) { TSetVtbl(ctx, 0x00B228C4); }
// *this = vtable 0x00B22698
DECOMP(00794EC0, VtB22698_DtorTrivial) { TSetVtbl(ctx, 0x00B22698); }
// *this = vtable 0x00B22698
DECOMP(00794ED0, VtB22698_DtorTrivial_00794ED0) { TSetVtbl(ctx, 0x00B22698); }
// *this = vtable 0x00B22698
DECOMP(00794EE0, VtB22698_DtorTrivial_00794EE0) { TSetVtbl(ctx, 0x00B22698); }
// *this = vtable 0x00B22698
DECOMP(00794EF0, VtB22698_DtorTrivial_00794EF0) { TSetVtbl(ctx, 0x00B22698); }
// *this = vtable 0x00B22698
DECOMP(00794F00, VtB22698_DtorTrivial_00794F00) { TSetVtbl(ctx, 0x00B22698); }
// *this = vtable 0x00B22698
DECOMP(00794F10, VtB22698_DtorTrivial_00794F10) { TSetVtbl(ctx, 0x00B22698); }
// *this = vtable 0x00B23080
DECOMP(0079B94C, VtB23080_DtorTrivial) { TSetVtbl(ctx, 0x00B23080); }

// ---------------------------------------------------------------- deleting destructors without a base destructor (83)
// deleting: vtable 0x00B0DBF8, 0x0054EE6C(this, 0)
DECOMP(00736688, VtB0DBF8_Delete) { TDeleteNoBase(ctx, 0x00B0DBF8, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DBF8, 0x0054EE6C(this, 0)
DECOMP(0073672C, VtB0DBF8_Delete_0073672C) { TDeleteNoBase(ctx, 0x00B0DBF8, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DBF8, 0x0054EE6C(this, 0)
DECOMP(007367A4, VtB0DBF8_Delete_007367A4) { TDeleteNoBase(ctx, 0x00B0DBF8, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DBF8, 0x0054EE6C(this, 0)
DECOMP(0073681C, VtB0DBF8_Delete_0073681C) { TDeleteNoBase(ctx, 0x00B0DBF8, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DBF8, 0x0054EE6C(this, 0)
DECOMP(007368B4, VtB0DBF8_Delete_007368B4) { TDeleteNoBase(ctx, 0x00B0DBF8, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DBF8, 0x0054EE6C(this, 0)
DECOMP(007369A4, VtB0DBF8_Delete_007369A4) { TDeleteNoBase(ctx, 0x00B0DBF8, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0EF40, 0x0054EE6C(this, 0)
DECOMP(00736A0C, VtB0EF40_Delete) { TDeleteNoBase(ctx, 0x00B0EF40, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DABC, 0x0054EE6C(this, 0)
DECOMP(0073760C, VtB0DABC_Delete) { TDeleteNoBase(ctx, 0x00B0DABC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DABC, 0x0054EE6C(this, 0)
DECOMP(00737650, VtB0DABC_Delete_00737650) { TDeleteNoBase(ctx, 0x00B0DABC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DABC, 0x0054EE6C(this, 0)
DECOMP(00737694, VtB0DABC_Delete_00737694) { TDeleteNoBase(ctx, 0x00B0DABC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DAEC, 0x0054EE6C(this, 0)
DECOMP(00737860, VtB0DAEC_Delete) { TDeleteNoBase(ctx, 0x00B0DAEC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DB3C, 0x0054EE6C(this, 0)
DECOMP(007379AC, VtB0DB3C_Delete) { TDeleteNoBase(ctx, 0x00B0DB3C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DB3C, 0x0054EE6C(this, 0)
DECOMP(00737FFC, VtB0DB3C_Delete_00737FFC) { TDeleteNoBase(ctx, 0x00B0DB3C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DB3C, 0x0054EE6C(this, 0)
DECOMP(00738154, VtB0DB3C_Delete_00738154) { TDeleteNoBase(ctx, 0x00B0DB3C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DB3C, 0x0054EE6C(this, 0)
DECOMP(00738204, VtB0DB3C_Delete_00738204) { TDeleteNoBase(ctx, 0x00B0DB3C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DB3C, 0x0054EE6C(this, 0)
DECOMP(0073829C, VtB0DB3C_Delete_0073829C) { TDeleteNoBase(ctx, 0x00B0DB3C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DABC, 0x0054EE6C(this, 0)
DECOMP(0073BD24, VtB0DABC_Delete_0073BD24) { TDeleteNoBase(ctx, 0x00B0DABC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(00742A60, VtB11B20_Delete) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1200C, 0x0054EE6C(this, 0)
DECOMP(00743568, VtB1200C_Delete) { TDeleteNoBase(ctx, 0x00B1200C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11DC4, 0x0054EE6C(this, 0)
DECOMP(007438C0, VtB11DC4_Delete) { TDeleteNoBase(ctx, 0x00B11DC4, 0x0054EE6C, 0); }
// deleting: vtable 0x00B12034, 0x00552420(this, 208)
DECOMP(00744314, VtB12034_Delete) { TDeleteNoBase(ctx, 0x00B12034, 0x00552420, 208); }
// deleting: vtable 0x00B12528, 0x0054EE6C(this, 0)
DECOMP(00746BD0, VtB12528_Delete) { TDeleteNoBase(ctx, 0x00B12528, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DABC, 0x0054EE6C(this, 0)
DECOMP(007473A8, VtB0DABC_Delete_007473A8) { TDeleteNoBase(ctx, 0x00B0DABC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DABC, 0x0054EE6C(this, 0)
DECOMP(007473EC, VtB0DABC_Delete_007473EC) { TDeleteNoBase(ctx, 0x00B0DABC, 0x0054EE6C, 0); }
// deleting: vtable 0x00B13840, 0x0054EE6C(this, 0)
DECOMP(007484E4, VtB13840_Delete) { TDeleteNoBase(ctx, 0x00B13840, 0x0054EE6C, 0); }
// deleting: vtable 0x00B13840, 0x0054EE6C(this, 0)
DECOMP(0074855C, VtB13840_Delete_0074855C) { TDeleteNoBase(ctx, 0x00B13840, 0x0054EE6C, 0); }
// deleting: vtable 0x00B13840, 0x0054EE6C(this, 0)
DECOMP(007485B0, VtB13840_Delete_007485B0) { TDeleteNoBase(ctx, 0x00B13840, 0x0054EE6C, 0); }
// deleting: vtable 0x00B13988, 0x0054EE6C(this, 0)
DECOMP(0074938C, VtB13988_Delete) { TDeleteNoBase(ctx, 0x00B13988, 0x0054EE6C, 0); }
// deleting: vtable 0x00B13998, 0x0054EE6C(this, 0)
DECOMP(007493E0, VtB13998_Delete) { TDeleteNoBase(ctx, 0x00B13998, 0x0054EE6C, 0); }
// deleting: vtable 0x00B139A8, 0x0054EE6C(this, 0)
DECOMP(00749434, VtB139A8_Delete) { TDeleteNoBase(ctx, 0x00B139A8, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DAEC, 0x00552420(this, 12)
DECOMP(007494C8, VtB0DAEC_Delete_007494C8) { TDeleteNoBase(ctx, 0x00B0DAEC, 0x00552420, 12); }
// deleting: vtable 0x00B0DAEC, 0x00552420(this, 128)
DECOMP(00749780, VtB0DAEC_Delete_00749780) { TDeleteNoBase(ctx, 0x00B0DAEC, 0x00552420, 128); }
// deleting: vtable 0x00B0DB3C, 0x0054EE6C(this, 0)
DECOMP(0074EAF4, VtB0DB3C_Delete_0074EAF4) { TDeleteNoBase(ctx, 0x00B0DB3C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DB3C, 0x0054EE6C(this, 0)
DECOMP(0074EB78, VtB0DB3C_Delete_0074EB78) { TDeleteNoBase(ctx, 0x00B0DB3C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1BE78, 0x0054EE6C(this, 0)
DECOMP(00762AC8, VtB1BE78_Delete) { TDeleteNoBase(ctx, 0x00B1BE78, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1BE78, 0x00552420(this, 208)
DECOMP(00762B74, VtB1BE78_Delete_00762B74) { TDeleteNoBase(ctx, 0x00B1BE78, 0x00552420, 208); }
// deleting: vtable 0x00B1D4D8, 0x0054EE6C(this, 0)
DECOMP(00767A48, VtB1D4D8_Delete) { TDeleteNoBase(ctx, 0x00B1D4D8, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1D600, 0x00552420(this, 20)
DECOMP(007684D4, VtB1D600_Delete) { TDeleteNoBase(ctx, 0x00B1D600, 0x00552420, 20); }
// deleting: vtable 0x00B1D6A0, 0x0054EE6C(this, 0)
DECOMP(00768528, VtB1D6A0_Delete) { TDeleteNoBase(ctx, 0x00B1D6A0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1D6A0, 0x00552420(this, 52)
DECOMP(00768580, VtB1D6A0_Delete_00768580) { TDeleteNoBase(ctx, 0x00B1D6A0, 0x00552420, 52); }
// deleting: vtable 0x00B1D6A0, 0x00552420(this, 44)
DECOMP(007685CC, VtB1D6A0_Delete_007685CC) { TDeleteNoBase(ctx, 0x00B1D6A0, 0x00552420, 44); }
// deleting: vtable 0x00B1D6A0, 0x00552420(this, 52)
DECOMP(00768620, VtB1D6A0_Delete_00768620) { TDeleteNoBase(ctx, 0x00B1D6A0, 0x00552420, 52); }
// deleting: vtable 0x00B1D6A0, 0x00552420(this, 48)
DECOMP(0076866C, VtB1D6A0_Delete_0076866C) { TDeleteNoBase(ctx, 0x00B1D6A0, 0x00552420, 48); }
// deleting: vtable 0x00B1D9B8, 0x0054EE6C(this, 0)
DECOMP(007686C0, VtB1D9B8_Delete) { TDeleteNoBase(ctx, 0x00B1D9B8, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1D9B8, 0x00552420(this, 36)
DECOMP(00768718, VtB1D9B8_Delete_00768718) { TDeleteNoBase(ctx, 0x00B1D9B8, 0x00552420, 36); }
// deleting: vtable 0x00B1D9B8, 0x00552420(this, 8)
DECOMP(00768764, VtB1D9B8_Delete_00768764) { TDeleteNoBase(ctx, 0x00B1D9B8, 0x00552420, 8); }
// deleting: vtable 0x00B1D9B8, 0x00552420(this, 80)
DECOMP(007687B0, VtB1D9B8_Delete_007687B0) { TDeleteNoBase(ctx, 0x00B1D9B8, 0x00552420, 80); }
// deleting: vtable 0x00B1D9B8, 0x00552420(this, 112)
DECOMP(007687FC, VtB1D9B8_Delete_007687FC) { TDeleteNoBase(ctx, 0x00B1D9B8, 0x00552420, 112); }
// deleting: vtable 0x00B1D9B8, 0x00552420(this, 8)
DECOMP(00768848, VtB1D9B8_Delete_00768848) { TDeleteNoBase(ctx, 0x00B1D9B8, 0x00552420, 8); }
// deleting: vtable 0x00B1D9B8, 0x00552420(this, 8)
DECOMP(00768894, VtB1D9B8_Delete_00768894) { TDeleteNoBase(ctx, 0x00B1D9B8, 0x00552420, 8); }
// deleting: vtable 0x00B1D9B8, 0x00552420(this, 12)
DECOMP(007688E0, VtB1D9B8_Delete_007688E0) { TDeleteNoBase(ctx, 0x00B1D9B8, 0x00552420, 12); }
// deleting: vtable 0x00B1F228, 0x0054EE6C(this, 0)
DECOMP(007703E0, VtB1F228_Delete) { TDeleteNoBase(ctx, 0x00B1F228, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1F23C, 0x0054EE6C(this, 0)
DECOMP(00770434, VtB1F23C_Delete) { TDeleteNoBase(ctx, 0x00B1F23C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1F250, 0x0054EE6C(this, 0)
DECOMP(00770488, VtB1F250_Delete) { TDeleteNoBase(ctx, 0x00B1F250, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1F26C, 0x0054EE6C(this, 0)
DECOMP(007704DC, VtB1F26C_Delete) { TDeleteNoBase(ctx, 0x00B1F26C, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1F4C0, 0x0054EE6C(this, 0)
DECOMP(007706E8, VtB1F4C0_Delete) { TDeleteNoBase(ctx, 0x00B1F4C0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1F4E0, 0x0054EE6C(this, 0)
DECOMP(00770740, VtB1F4E0_Delete) { TDeleteNoBase(ctx, 0x00B1F4E0, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0EF40, 0x0054EE6C(this, 0)
DECOMP(00771CB8, VtB0EF40_Delete_00771CB8) { TDeleteNoBase(ctx, 0x00B0EF40, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1F668, 0x0054EE6C(this, 0)
DECOMP(007728FC, VtB1F668_Delete) { TDeleteNoBase(ctx, 0x00B1F668, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1F668, 0x0054EE6C(this, 0)
DECOMP(00772B9C, VtB1F668_Delete_00772B9C) { TDeleteNoBase(ctx, 0x00B1F668, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1F668, 0x0054EE6C(this, 0)
DECOMP(00772BE0, VtB1F668_Delete_00772BE0) { TDeleteNoBase(ctx, 0x00B1F668, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1F668, 0x0054EE6C(this, 0)
DECOMP(00772C34, VtB1F668_Delete_00772C34) { TDeleteNoBase(ctx, 0x00B1F668, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1F668, 0x0054EE6C(this, 0)
DECOMP(00772C78, VtB1F668_Delete_00772C78) { TDeleteNoBase(ctx, 0x00B1F668, 0x0054EE6C, 0); }
// deleting: vtable 0x00B1F668, 0x0054EE6C(this, 0)
DECOMP(00773520, VtB1F668_Delete_00773520) { TDeleteNoBase(ctx, 0x00B1F668, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DBF8, 0x0054EE6C(this, 0)
DECOMP(00782080, VtB0DBF8_Delete_00782080) { TDeleteNoBase(ctx, 0x00B0DBF8, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DBF8, 0x0054EE6C(this, 0)
DECOMP(007820C4, VtB0DBF8_Delete_007820C4) { TDeleteNoBase(ctx, 0x00B0DBF8, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DBF8, 0x0054EE6C(this, 0)
DECOMP(00782170, VtB0DBF8_Delete_00782170) { TDeleteNoBase(ctx, 0x00B0DBF8, 0x0054EE6C, 0); }
// deleting: vtable 0x00B21A88, 0x0054EE6C(this, 0)
DECOMP(007887B8, VtB21A88_Delete) { TDeleteNoBase(ctx, 0x00B21A88, 0x0054EE6C, 0); }
// deleting: vtable 0x00B21A88, 0x0054EE6C(this, 0)
DECOMP(007889BC, VtB21A88_Delete_007889BC) { TDeleteNoBase(ctx, 0x00B21A88, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x00552420(this, 68)
DECOMP(007940E0, VtB11B20_Delete_007940E0) { TDeleteNoBase(ctx, 0x00B11B20, 0x00552420, 68); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(00794220, VtB11B20_Delete_00794220) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(00794410, VtB11B20_Delete_00794410) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(00794454, VtB11B20_Delete_00794454) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(00794498, VtB11B20_Delete_00794498) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B11B20, 0x0054EE6C(this, 0)
DECOMP(007944DC, VtB11B20_Delete_007944DC) { TDeleteNoBase(ctx, 0x00B11B20, 0x0054EE6C, 0); }
// deleting: vtable 0x00B22698, 0x0054EE6C(this, 0)
DECOMP(00795830, VtB22698_Delete) { TDeleteNoBase(ctx, 0x00B22698, 0x0054EE6C, 0); }
// deleting: vtable 0x00B22698, 0x0054EE6C(this, 0)
DECOMP(007958A4, VtB22698_Delete_007958A4) { TDeleteNoBase(ctx, 0x00B22698, 0x0054EE6C, 0); }
// deleting: vtable 0x00B22698, 0x0054EE6C(this, 0)
DECOMP(007958F4, VtB22698_Delete_007958F4) { TDeleteNoBase(ctx, 0x00B22698, 0x0054EE6C, 0); }
// deleting: vtable 0x00B22698, 0x0054EE6C(this, 0)
DECOMP(00795944, VtB22698_Delete_00795944) { TDeleteNoBase(ctx, 0x00B22698, 0x0054EE6C, 0); }
// deleting: vtable 0x00B22698, 0x0054EE6C(this, 0)
DECOMP(00795994, VtB22698_Delete_00795994) { TDeleteNoBase(ctx, 0x00B22698, 0x0054EE6C, 0); }
// deleting: vtable 0x00B22698, 0x0054EE6C(this, 0)
DECOMP(007959E4, VtB22698_Delete_007959E4) { TDeleteNoBase(ctx, 0x00B22698, 0x0054EE6C, 0); }
// deleting: vtable 0x00B228C4, 0x00552420(this, 48)
DECOMP(00796014, VtB228C4_Delete) { TDeleteNoBase(ctx, 0x00B228C4, 0x00552420, 48); }
// deleting: vtable 0x00B23080, 0x0054EE6C(this, 0)
DECOMP(0079B95C, VtB23080_Delete) { TDeleteNoBase(ctx, 0x00B23080, 0x0054EE6C, 0); }

// ---------------------------------------------------------------- destructors that take the object out of a sorted map (this + 4 the key) (37)
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(007374D8, VtB0DAAC_Dtor) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(007387B0, VtB0DAAC_Dtor_007387B0) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(00738DB0, VtB0DAAC_Dtor_00738DB0) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(00749668, VtB0DAAC_Dtor_00749668) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(0074EBBC, VtB0DAAC_Dtor_0074EBBC) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(0074EC00, VtB0DAAC_Dtor_0074EC00) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(0074EC44, VtB0DAAC_Dtor_0074EC44) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(0074EC88, VtB0DAAC_Dtor_0074EC88) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(0075D6A4, VtB0DAAC_Dtor_0075D6A4) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(0075D6E8, VtB0DAAC_Dtor_0075D6E8) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(0075DA44, VtB0DAAC_Dtor_0075DA44) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(00762E64, VtB0DAAC_Dtor_00762E64) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(007630C8, VtB0DAAC_Dtor_007630C8) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(00764140, VtB0DAAC_Dtor_00764140) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(007641EC, VtB0DAAC_Dtor_007641EC) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(007642E8, VtB0DAAC_Dtor_007642E8) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(00764394, VtB0DAAC_Dtor_00764394) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(00764440, VtB0DAAC_Dtor_00764440) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(007644EC, VtB0DAAC_Dtor_007644EC) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(00764598, VtB0DAAC_Dtor_00764598) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(0076479C, VtB0DAAC_Dtor_0076479C) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(00764CC0, VtB0DAAC_Dtor_00764CC0) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(00764D6C, VtB0DAAC_Dtor_00764D6C) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(00764E18, VtB0DAAC_Dtor_00764E18) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(00764EC4, VtB0DAAC_Dtor_00764EC4) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(00764F70, VtB0DAAC_Dtor_00764F70) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(0076501C, VtB0DAAC_Dtor_0076501C) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(007651E0, VtB0DAAC_Dtor_007651E0) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(007654F4, VtB0DAAC_Dtor_007654F4) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(007655A0, VtB0DAAC_Dtor_007655A0) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(0076564C, VtB0DAAC_Dtor_0076564C) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(007656F8, VtB0DAAC_Dtor_007656F8) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(007657A4, VtB0DAAC_Dtor_007657A4) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(007658B4, VtB0DAAC_Dtor_007658B4) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(00765960, VtB0DAAC_Dtor_00765960) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(00765A0C, VtB0DAAC_Dtor_00765A0C) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }
// *this = vtable 0x00B0DAAC, then 0x007330C0(map 0x01056F04, this + 4)
DECOMP(00765AB8, VtB0DAAC_Dtor_00765AB8) { TDtorUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04); }

// ---------------------------------------------------------------- their deleting versions (37)
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x0054EE6C(this, 0)
DECOMP(0073751C, VtB0DAAC_Delete) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x0054EE6C, 0); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 28)
DECOMP(007387F4, VtB0DAAC_Delete_007387F4) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 28); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 20)
DECOMP(0073993C, VtB0DAAC_Delete_0073993C) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 20); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 224)
DECOMP(007496AC, VtB0DAAC_Delete_007496AC) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 224); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 24)
DECOMP(0074ECFC, VtB0DAAC_Delete_0074ECFC) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 24); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 24)
DECOMP(0074ED64, VtB0DAAC_Delete_0074ED64) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 24); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 20)
DECOMP(0074EDCC, VtB0DAAC_Delete_0074EDCC) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 20); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 28)
DECOMP(0074EE34, VtB0DAAC_Delete_0074EE34) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 28); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 24)
DECOMP(0075D974, VtB0DAAC_Delete_0075D974) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 24); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 20)
DECOMP(0075D9DC, VtB0DAAC_Delete_0075D9DC) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 20); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 24)
DECOMP(0075DB28, VtB0DAAC_Delete_0075DB28) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 24); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 160)
DECOMP(00762EA8, VtB0DAAC_Delete_00762EA8) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 160); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 112)
DECOMP(0076310C, VtB0DAAC_Delete_0076310C) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 112); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 36)
DECOMP(00764184, VtB0DAAC_Delete_00764184) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 36); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 288)
DECOMP(00764230, VtB0DAAC_Delete_00764230) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 288); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 2904)
DECOMP(0076432C, VtB0DAAC_Delete_0076432C) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 2904); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 32)
DECOMP(007643D8, VtB0DAAC_Delete_007643D8) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 32); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 224)
DECOMP(00764484, VtB0DAAC_Delete_00764484) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 224); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 96)
DECOMP(00764530, VtB0DAAC_Delete_00764530) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 96); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 1040)
DECOMP(007645DC, VtB0DAAC_Delete_007645DC) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 1040); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 24)
DECOMP(007647E0, VtB0DAAC_Delete_007647E0) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 24); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 100)
DECOMP(00764D04, VtB0DAAC_Delete_00764D04) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 100); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 28)
DECOMP(00764DB0, VtB0DAAC_Delete_00764DB0) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 28); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 28)
DECOMP(00764E5C, VtB0DAAC_Delete_00764E5C) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 28); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 28)
DECOMP(00764F08, VtB0DAAC_Delete_00764F08) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 28); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 64)
DECOMP(00764FB4, VtB0DAAC_Delete_00764FB4) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 64); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 44)
DECOMP(00765060, VtB0DAAC_Delete_00765060) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 44); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 44)
DECOMP(00765224, VtB0DAAC_Delete_00765224) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 44); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 304)
DECOMP(00765538, VtB0DAAC_Delete_00765538) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 304); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 44)
DECOMP(007655E4, VtB0DAAC_Delete_007655E4) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 44); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 36)
DECOMP(00765690, VtB0DAAC_Delete_00765690) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 36); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 304)
DECOMP(0076573C, VtB0DAAC_Delete_0076573C) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 304); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 312)
DECOMP(007657E8, VtB0DAAC_Delete_007657E8) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 312); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 44)
DECOMP(007658F8, VtB0DAAC_Delete_007658F8) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 44); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 112)
DECOMP(007659A4, VtB0DAAC_Delete_007659A4) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 112); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 48)
DECOMP(00765A50, VtB0DAAC_Delete_00765A50) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 48); }
// deleting: vtable 0x00B0DAAC, 0x007330C0(map 0x01056F04, this + 4), 0x00552420(this, 44)
DECOMP(00765AFC, VtB0DAAC_Delete_00765AFC) { TDeleteUnmap(ctx, 0x00B0DAAC, 0x007330C0, 0x01056F04, 0x00552420, 44); }

// ---------------------------------------------------------------- the unsigned u32 key comparators (sorted map / qsort) (81)
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(007365F8, CmpU32_007365F8) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00736628, CmpU32_00736628) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00736658, CmpU32_00736658) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00736DD0, CmpU32_00736DD0) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00736FC4, CmpU32_00736FC4) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00736FF4, CmpU32_00736FF4) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00739840, CmpU32_00739840) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0073990C, CmpU32_0073990C) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0073BBF8, CmpU32_0073BBF8) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0073C9F8, CmpU32_0073C9F8) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0074307C, CmpU32_0074307C) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00746B90, CmpU32_00746B90) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00746CF8, CmpU32_00746CF8) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00746D28, CmpU32_00746D28) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00746D58, CmpU32_00746D58) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0074EA54, CmpU32_0074EA54) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0074EA84, CmpU32_0074EA84) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0074ECCC, CmpU32_0074ECCC) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0075396C, CmpU32_0075396C) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00759E44, CmpU32_00759E44) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0075D5C4, CmpU32_0075D5C4) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0075D5F4, CmpU32_0075D5F4) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00761E98, CmpU32_00761E98) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00762158, CmpU32_00762158) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00762260, CmpU32_00762260) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00762290, CmpU32_00762290) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(007622C0, CmpU32_007622C0) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(007623C8, CmpU32_007623C8) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00762410, CmpU32_00762410) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00762518, CmpU32_00762518) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00762620, CmpU32_00762620) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00762728, CmpU32_00762728) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00762830, CmpU32_00762830) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00762938, CmpU32_00762938) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0076308C, CmpU32_0076308C) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00764C90, CmpU32_00764C90) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00766A4C, CmpU32_00766A4C) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00766A7C, CmpU32_00766A7C) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00766AAC, CmpU32_00766AAC) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00766ADC, CmpU32_00766ADC) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00766B0C, CmpU32_00766B0C) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00766B3C, CmpU32_00766B3C) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(007670DC, CmpU32_007670DC) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0076710C, CmpU32_0076710C) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0076713C, CmpU32_0076713C) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00768DD0, CmpU32_00768DD0) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00768E00, CmpU32_00768E00) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0076BA58, CmpU32_0076BA58) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0076BA88, CmpU32_0076BA88) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(007707F0, CmpU32_007707F0) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00770820, CmpU32_00770820) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00770850, CmpU32_00770850) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00770880, CmpU32_00770880) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(007708B0, CmpU32_007708B0) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(007708E0, CmpU32_007708E0) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0077288C, CmpU32_0077288C) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(007728BC, CmpU32_007728BC) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0077A97C, CmpU32_0077A97C) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0077EB6C, CmpU32_0077EB6C) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00784A74, CmpU32_00784A74) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00784AA4, CmpU32_00784AA4) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00784AD4, CmpU32_00784AD4) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00784B04, CmpU32_00784B04) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00784B34, CmpU32_00784B34) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00784B64, CmpU32_00784B64) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00784B94, CmpU32_00784B94) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00788384, CmpU32_00788384) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0078848C, CmpU32_0078848C) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00788EC4, CmpU32_00788EC4) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00789F68, CmpU32_00789F68) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00789F98, CmpU32_00789F98) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00789FC8, CmpU32_00789FC8) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00789FF8, CmpU32_00789FF8) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00794780, CmpU32_00794780) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(007947B0, CmpU32_007947B0) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(007947E0, CmpU32_007947E0) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(00795874, CmpU32_00795874) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0079A008, CmpU32_0079A008) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0079B8BC, CmpU32_0079B8BC) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0079B8EC, CmpU32_0079B8EC) { TCmpU32(ctx); }
// (const u32* a, const u32* b) -> 1 / 0 / -1 (unsigned)
DECOMP(0079B91C, CmpU32_0079B91C) { TCmpU32(ctx); }

// ---------------------------------------------------------------- tail wrappers (44)
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(007338D4, Thunk_007338D4) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(007338F8, Thunk_007338F8) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(0073391C, Thunk_0073391C) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(00733940, Thunk_00733940) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(00733B84, Thunk_00733B84) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(00733BA8, Thunk_00733BA8) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(00733BCC, Thunk_00733BCC) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(00733BF0, Thunk_00733BF0) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(00733C14, Thunk_00733C14) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(00733C38, Thunk_00733C38) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(00733C5C, Thunk_00733C5C) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(00733C80, Thunk_00733C80) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(00733CA4, Thunk_00733CA4) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(00733CC8, Thunk_00733CC8) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(00739B2C, Thunk_00739B2C) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(00739B50, Thunk_00739B50) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(00739B74, Thunk_00739B74) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(00739B98, Thunk_00739B98) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(00739BBC, Thunk_00739BBC) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(00739BE0, Thunk_00739BE0) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(00739C04, Thunk_00739C04) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(00739C28, Thunk_00739C28) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(007458DC, Thunk_007458DC) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(00745900, Thunk_00745900) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(00745924, Thunk_00745924) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(00745948, Thunk_00745948) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(00748A40, Thunk_00748A40) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(00748A64, Thunk_00748A64) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(0076E3B8, Thunk_0076E3B8) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(0076E3DC, Thunk_0076E3DC) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(0077AD40, Thunk_0077AD40) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(0077AD64, Thunk_0077AD64) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(00793BC4, Thunk_00793BC4) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(00793BE8, Thunk_00793BE8) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(00793C0C, Thunk_00793C0C) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(00793C30, Thunk_00793C30) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(00793C54, Thunk_00793C54) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(00793C78, Thunk_00793C78) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(00793C9C, Thunk_00793C9C) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(00793CC0, Thunk_00793CC0) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(00799F10, Thunk_00799F10) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(00799F34, Thunk_00799F34) { TThunk(ctx, 0x00556D18); }
// tail wrapper of 0x00556C04 (the same arguments)
DECOMP(00799F58, Thunk_00799F58) { TThunk(ctx, 0x00556C04); }
// tail wrapper of 0x00556D18 (the same arguments)
DECOMP(00799F7C, Thunk_00799F7C) { TThunk(ctx, 0x00556D18); }

// ---------------------------------------------------------------- constant returns (li) (324)
// returns 0x0
DECOMP(007318B8, ReturnK_007318B8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007318C0, ReturnK_007318C0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007318C8, ReturnK_007318C8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00735FFC, ReturnK_00735FFC) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(007360F4, ReturnK_007360F4) { ret(ctx, static_cast<s32>(1)); }
// returns 0x2
DECOMP(007361B8, ReturnK_007361B8) { ret(ctx, static_cast<s32>(2)); }
// returns 0x4
DECOMP(007362A4, ReturnK_007362A4) { ret(ctx, static_cast<s32>(4)); }
// returns 0x5
DECOMP(00736390, ReturnK_00736390) { ret(ctx, static_cast<s32>(5)); }
// returns 0x6
DECOMP(007364AC, ReturnK_007364AC) { ret(ctx, static_cast<s32>(6)); }
// returns 0x7
DECOMP(00736558, ReturnK_00736558) { ret(ctx, static_cast<s32>(7)); }
// returns 0x0
DECOMP(007369E8, ReturnK_007369E8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007369F0, ReturnK_007369F0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007372F4, ReturnK_007372F4) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00737394, ReturnK_00737394) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0073739C, ReturnK_0073739C) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007373CC, ReturnK_007373CC) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(007374A8, ReturnK_007374A8) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(007374B8, ReturnK_007374B8) { ret(ctx, static_cast<s32>(1)); }
// returns 0x0
DECOMP(007374C8, ReturnK_007374C8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007374D0, ReturnK_007374D0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00737584, ReturnK_00737584) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0073758C, ReturnK_0073758C) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0073759C, ReturnK_0073759C) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(00737840, ReturnK_00737840) { ret(ctx, static_cast<s32>(1)); }
// returns 0x0
DECOMP(00737984, ReturnK_00737984) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(007379F4, ReturnK_007379F4) { ret(ctx, static_cast<s32>(1)); }
// returns 0x0
DECOMP(00738438, ReturnK_00738438) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(00738480, ReturnK_00738480) { ret(ctx, static_cast<s32>(1)); }
// returns 0x0
DECOMP(007384AC, ReturnK_007384AC) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007384B4, ReturnK_007384B4) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007384BC, ReturnK_007384BC) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007384C4, ReturnK_007384C4) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007384CC, ReturnK_007384CC) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007384D8, ReturnK_007384D8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007384E0, ReturnK_007384E0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007384E8, ReturnK_007384E8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007384F0, ReturnK_007384F0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007384F8, ReturnK_007384F8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00738500, ReturnK_00738500) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00738508, ReturnK_00738508) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00738510, ReturnK_00738510) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00738518, ReturnK_00738518) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(00738568, ReturnK_00738568) { ret(ctx, static_cast<s32>(1)); }
// returns 0xB
DECOMP(007386C0, ReturnK_007386C0) { ret(ctx, static_cast<s32>(11)); }
// returns 0x0
DECOMP(007386C8, ReturnK_007386C8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00738740, ReturnK_00738740) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(00738758, ReturnK_00738758) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(0073885C, ReturnK_0073885C) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00738B00, ReturnK_00738B00) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00738B18, ReturnK_00738B18) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00738CA4, ReturnK_00738CA4) { ret(ctx, static_cast<s32>(1)); }
// returns 0x8
DECOMP(00738D08, ReturnK_00738D08) { ret(ctx, static_cast<s32>(8)); }
// returns 0x0
DECOMP(00738DA8, ReturnK_00738DA8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x2
DECOMP(00738DF4, ReturnK_00738DF4) { ret(ctx, static_cast<s32>(2)); }
// returns 0x2
DECOMP(00738E0C, ReturnK_00738E0C) { ret(ctx, static_cast<s32>(2)); }
// returns 0x2
DECOMP(00738F98, ReturnK_00738F98) { ret(ctx, static_cast<s32>(2)); }
// returns 0xA
DECOMP(00738FA0, ReturnK_00738FA0) { ret(ctx, static_cast<s32>(10)); }
// returns 0x1
DECOMP(00738FC0, ReturnK_00738FC0) { ret(ctx, static_cast<s32>(1)); }
// returns 0x8
DECOMP(007392C0, ReturnK_007392C0) { ret(ctx, static_cast<s32>(8)); }
// returns 0x0
DECOMP(007392C8, ReturnK_007392C8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(007392D0, ReturnK_007392D0) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(0073960C, ReturnK_0073960C) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00739614, ReturnK_00739614) { ret(ctx, static_cast<s32>(1)); }
// returns 0x4
DECOMP(0073961C, ReturnK_0073961C) { ret(ctx, static_cast<s32>(4)); }
// returns 0x1
DECOMP(007396C0, ReturnK_007396C0) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00739790, ReturnK_00739790) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00739830, ReturnK_00739830) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00739838, ReturnK_00739838) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00739870, ReturnK_00739870) { ret(ctx, static_cast<s32>(1)); }
// returns 0x0
DECOMP(0073B8F0, ReturnK_0073B8F0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0073B8FC, ReturnK_0073B8FC) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00742AC0, ReturnK_00742AC0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00742AC8, ReturnK_00742AC8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00742AE0, ReturnK_00742AE0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00742AEC, ReturnK_00742AEC) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00742AF4, ReturnK_00742AF4) { ret(ctx, static_cast<s32>(0)); }
// returns 0xD
DECOMP(00742AFC, ReturnK_00742AFC) { ret(ctx, static_cast<s32>(13)); }
// returns 0x18
DECOMP(00742B04, ReturnK_00742B04) { ret(ctx, static_cast<s32>(24)); }
// returns 0x0
DECOMP(00742B94, ReturnK_00742B94) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00742B9C, ReturnK_00742B9C) { ret(ctx, static_cast<s32>(0)); }
// returns 0x10
DECOMP(00742BA4, ReturnK_00742BA4) { ret(ctx, static_cast<s32>(16)); }
// returns 0x58
DECOMP(00742BAC, ReturnK_00742BAC) { ret(ctx, static_cast<s32>(88)); }
// returns 0x0
DECOMP(00742BB4, ReturnK_00742BB4) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00742BBC, ReturnK_00742BBC) { ret(ctx, static_cast<s32>(0)); }
// returns 0xF
DECOMP(00742BC4, ReturnK_00742BC4) { ret(ctx, static_cast<s32>(15)); }
// returns 0x18
DECOMP(00742BCC, ReturnK_00742BCC) { ret(ctx, static_cast<s32>(24)); }
// returns 0x0
DECOMP(00742BD4, ReturnK_00742BD4) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00742BDC, ReturnK_00742BDC) { ret(ctx, static_cast<s32>(0)); }
// returns 0x11
DECOMP(00742BE4, ReturnK_00742BE4) { ret(ctx, static_cast<s32>(17)); }
// returns 0x18
DECOMP(00742BEC, ReturnK_00742BEC) { ret(ctx, static_cast<s32>(24)); }
// returns 0x0
DECOMP(00742BF4, ReturnK_00742BF4) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00742BFC, ReturnK_00742BFC) { ret(ctx, static_cast<s32>(0)); }
// returns 0x12
DECOMP(00742C04, ReturnK_00742C04) { ret(ctx, static_cast<s32>(18)); }
// returns 0x18
DECOMP(00742C0C, ReturnK_00742C0C) { ret(ctx, static_cast<s32>(24)); }
// returns 0x0
DECOMP(00742C14, ReturnK_00742C14) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00742C1C, ReturnK_00742C1C) { ret(ctx, static_cast<s32>(0)); }
// returns 0x13
DECOMP(00742C24, ReturnK_00742C24) { ret(ctx, static_cast<s32>(19)); }
// returns 0x18
DECOMP(00742C2C, ReturnK_00742C2C) { ret(ctx, static_cast<s32>(24)); }
// returns 0x0
DECOMP(00742C34, ReturnK_00742C34) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00742C3C, ReturnK_00742C3C) { ret(ctx, static_cast<s32>(0)); }
// returns 0x14
DECOMP(00742C44, ReturnK_00742C44) { ret(ctx, static_cast<s32>(20)); }
// returns 0x18
DECOMP(00742C4C, ReturnK_00742C4C) { ret(ctx, static_cast<s32>(24)); }
// returns 0x0
DECOMP(00742C54, ReturnK_00742C54) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00742C5C, ReturnK_00742C5C) { ret(ctx, static_cast<s32>(0)); }
// returns 0x15
DECOMP(00742C64, ReturnK_00742C64) { ret(ctx, static_cast<s32>(21)); }
// returns 0x18
DECOMP(00742C6C, ReturnK_00742C6C) { ret(ctx, static_cast<s32>(24)); }
// returns 0x0
DECOMP(007437C0, ReturnK_007437C0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007437C8, ReturnK_007437C8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00743870, ReturnK_00743870) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00743878, ReturnK_00743878) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00746F4C, ReturnK_00746F4C) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00746F54, ReturnK_00746F54) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00747054, ReturnK_00747054) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0074705C, ReturnK_0074705C) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007487D0, ReturnK_007487D0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00748970, ReturnK_00748970) { ret(ctx, static_cast<s32>(0)); }
// returns 0x3
DECOMP(00748FF0, ReturnK_00748FF0) { ret(ctx, static_cast<s32>(3)); }
// returns 0x5
DECOMP(00749478, ReturnK_00749478) { ret(ctx, static_cast<s32>(5)); }
// returns 0x3
DECOMP(0074961C, ReturnK_0074961C) { ret(ctx, static_cast<s32>(3)); }
// returns 0xB
DECOMP(00749638, ReturnK_00749638) { ret(ctx, static_cast<s32>(11)); }
// returns 0xE
DECOMP(00749654, ReturnK_00749654) { ret(ctx, static_cast<s32>(14)); }
// returns 0x2
DECOMP(00749714, ReturnK_00749714) { ret(ctx, static_cast<s32>(2)); }
// returns 0xA
DECOMP(0074976C, ReturnK_0074976C) { ret(ctx, static_cast<s32>(10)); }
// returns 0x6
DECOMP(007497C4, ReturnK_007497C4) { ret(ctx, static_cast<s32>(6)); }
// returns 0x7
DECOMP(00749800, ReturnK_00749800) { ret(ctx, static_cast<s32>(7)); }
// returns 0x9
DECOMP(0074981C, ReturnK_0074981C) { ret(ctx, static_cast<s32>(9)); }
// returns 0x8
DECOMP(0074983C, ReturnK_0074983C) { ret(ctx, static_cast<s32>(8)); }
// returns 0xC
DECOMP(00749858, ReturnK_00749858) { ret(ctx, static_cast<s32>(12)); }
// returns 0xD
DECOMP(0074986C, ReturnK_0074986C) { ret(ctx, static_cast<s32>(13)); }
// returns 0xF
DECOMP(00749998, ReturnK_00749998) { ret(ctx, static_cast<s32>(15)); }
// returns 0x10
DECOMP(007499B4, ReturnK_007499B4) { ret(ctx, static_cast<s32>(16)); }
// returns 0x11
DECOMP(007499D0, ReturnK_007499D0) { ret(ctx, static_cast<s32>(17)); }
// returns 0x1
DECOMP(0074F4E4, ReturnK_0074F4E4) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(0074F670, ReturnK_0074F670) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00753904, ReturnK_00753904) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00754428, ReturnK_00754428) { ret(ctx, static_cast<s32>(1)); }
// returns 0x0
DECOMP(00757418, ReturnK_00757418) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00759C2C, ReturnK_00759C2C) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00759C34, ReturnK_00759C34) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00762B0C, ReturnK_00762B0C) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00762B5C, ReturnK_00762B5C) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(00762CB8, ReturnK_00762CB8) { ret(ctx, static_cast<s32>(1)); }
// returns 0x0
DECOMP(007633CC, ReturnK_007633CC) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00763EF0, ReturnK_00763EF0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00768570, ReturnK_00768570) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(00768578, ReturnK_00768578) { ret(ctx, static_cast<s32>(1)); }
// returns 0x2
DECOMP(007685C4, ReturnK_007685C4) { ret(ctx, static_cast<s32>(2)); }
// returns 0x3
DECOMP(00768610, ReturnK_00768610) { ret(ctx, static_cast<s32>(3)); }
// returns 0x4
DECOMP(00768618, ReturnK_00768618) { ret(ctx, static_cast<s32>(4)); }
// returns 0x5
DECOMP(00768664, ReturnK_00768664) { ret(ctx, static_cast<s32>(5)); }
// returns 0xFFFFFFFF
DECOMP(00768708, ReturnK_00768708) { ret(ctx, static_cast<s32>(-1)); }
// returns 0x0
DECOMP(00768710, ReturnK_00768710) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(0076875C, ReturnK_0076875C) { ret(ctx, static_cast<s32>(1)); }
// returns 0x2
DECOMP(007687A8, ReturnK_007687A8) { ret(ctx, static_cast<s32>(2)); }
// returns 0x3
DECOMP(007687F4, ReturnK_007687F4) { ret(ctx, static_cast<s32>(3)); }
// returns 0x5
DECOMP(00768840, ReturnK_00768840) { ret(ctx, static_cast<s32>(5)); }
// returns 0x6
DECOMP(0076888C, ReturnK_0076888C) { ret(ctx, static_cast<s32>(6)); }
// returns 0x7
DECOMP(007688D8, ReturnK_007688D8) { ret(ctx, static_cast<s32>(7)); }
// returns 0x0
DECOMP(00768924, ReturnK_00768924) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(0076892C, ReturnK_0076892C) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00768934, ReturnK_00768934) { ret(ctx, static_cast<s32>(1)); }
// returns 0xFFFFFFFF
DECOMP(0076893C, ReturnK_0076893C) { ret(ctx, static_cast<s32>(-1)); }
// returns 0x0
DECOMP(00768950, ReturnK_00768950) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(0076896C, ReturnK_0076896C) { ret(ctx, static_cast<s32>(1)); }
// returns 0x2
DECOMP(00768988, ReturnK_00768988) { ret(ctx, static_cast<s32>(2)); }
// returns 0x6
DECOMP(007689A4, ReturnK_007689A4) { ret(ctx, static_cast<s32>(6)); }
// returns 0x4
DECOMP(007689C0, ReturnK_007689C0) { ret(ctx, static_cast<s32>(4)); }
// returns 0x5
DECOMP(007689DC, ReturnK_007689DC) { ret(ctx, static_cast<s32>(5)); }
// returns 0x7
DECOMP(007689F8, ReturnK_007689F8) { ret(ctx, static_cast<s32>(7)); }
// returns 0x3
DECOMP(00768A14, ReturnK_00768A14) { ret(ctx, static_cast<s32>(3)); }
// returns 0x0
DECOMP(00768A30, ReturnK_00768A30) { ret(ctx, static_cast<s32>(0)); }
// returns 0x8
DECOMP(00768A38, ReturnK_00768A38) { ret(ctx, static_cast<s32>(8)); }
// returns 0x0
DECOMP(00768A54, ReturnK_00768A54) { ret(ctx, static_cast<s32>(0)); }
// returns 0x9
DECOMP(00768A5C, ReturnK_00768A5C) { ret(ctx, static_cast<s32>(9)); }
// returns 0x0
DECOMP(00768A78, ReturnK_00768A78) { ret(ctx, static_cast<s32>(0)); }
// returns 0xA
DECOMP(00768A80, ReturnK_00768A80) { ret(ctx, static_cast<s32>(10)); }
// returns 0x0
DECOMP(00768A9C, ReturnK_00768A9C) { ret(ctx, static_cast<s32>(0)); }
// returns 0xB
DECOMP(00768AA4, ReturnK_00768AA4) { ret(ctx, static_cast<s32>(11)); }
// returns 0x0
DECOMP(00769A20, ReturnK_00769A20) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00769A7C, ReturnK_00769A7C) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00769B34, ReturnK_00769B34) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(00769B3C, ReturnK_00769B3C) { ret(ctx, static_cast<s32>(1)); }
// returns 0x13
DECOMP(0076B35C, ReturnK_0076B35C) { ret(ctx, static_cast<s32>(19)); }
// returns 0x2
DECOMP(0076B364, ReturnK_0076B364) { ret(ctx, static_cast<s32>(2)); }
// returns 0x3
DECOMP(0076B36C, ReturnK_0076B36C) { ret(ctx, static_cast<s32>(3)); }
// returns 0x0
DECOMP(0076B3F4, ReturnK_0076B3F4) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1A
DECOMP(0076B400, ReturnK_0076B400) { ret(ctx, static_cast<s32>(26)); }
// returns 0x4
DECOMP(0076B408, ReturnK_0076B408) { ret(ctx, static_cast<s32>(4)); }
// returns 0x5
DECOMP(0076B410, ReturnK_0076B410) { ret(ctx, static_cast<s32>(5)); }
// returns 0x6
DECOMP(0076B418, ReturnK_0076B418) { ret(ctx, static_cast<s32>(6)); }
// returns 0x12
DECOMP(0076B420, ReturnK_0076B420) { ret(ctx, static_cast<s32>(18)); }
// returns 0x7
DECOMP(0076B428, ReturnK_0076B428) { ret(ctx, static_cast<s32>(7)); }
// returns 0x9
DECOMP(0076B430, ReturnK_0076B430) { ret(ctx, static_cast<s32>(9)); }
// returns 0xA
DECOMP(0076B438, ReturnK_0076B438) { ret(ctx, static_cast<s32>(10)); }
// returns 0xB
DECOMP(0076B440, ReturnK_0076B440) { ret(ctx, static_cast<s32>(11)); }
// returns 0xC
DECOMP(0076B448, ReturnK_0076B448) { ret(ctx, static_cast<s32>(12)); }
// returns 0xD
DECOMP(0076B450, ReturnK_0076B450) { ret(ctx, static_cast<s32>(13)); }
// returns 0xE
DECOMP(0076BA08, ReturnK_0076BA08) { ret(ctx, static_cast<s32>(14)); }
// returns 0x11
DECOMP(0076BA10, ReturnK_0076BA10) { ret(ctx, static_cast<s32>(17)); }
// returns 0x21
DECOMP(0076BA18, ReturnK_0076BA18) { ret(ctx, static_cast<s32>(33)); }
// returns 0x1B
DECOMP(0076BA20, ReturnK_0076BA20) { ret(ctx, static_cast<s32>(27)); }
// returns 0x1C
DECOMP(0076BA28, ReturnK_0076BA28) { ret(ctx, static_cast<s32>(28)); }
// returns 0x1D
DECOMP(0076BA30, ReturnK_0076BA30) { ret(ctx, static_cast<s32>(29)); }
// returns 0x1E
DECOMP(0076BA38, ReturnK_0076BA38) { ret(ctx, static_cast<s32>(30)); }
// returns 0x22
DECOMP(0076BA40, ReturnK_0076BA40) { ret(ctx, static_cast<s32>(34)); }
// returns 0x1F
DECOMP(0076BA48, ReturnK_0076BA48) { ret(ctx, static_cast<s32>(31)); }
// returns 0x20
DECOMP(0076BA50, ReturnK_0076BA50) { ret(ctx, static_cast<s32>(32)); }
// returns 0x14
DECOMP(0076BAB8, ReturnK_0076BAB8) { ret(ctx, static_cast<s32>(20)); }
// returns 0x23
DECOMP(0076BAC0, ReturnK_0076BAC0) { ret(ctx, static_cast<s32>(35)); }
// returns 0x15
DECOMP(0076BAC8, ReturnK_0076BAC8) { ret(ctx, static_cast<s32>(21)); }
// returns 0x18
DECOMP(0076BDF4, ReturnK_0076BDF4) { ret(ctx, static_cast<s32>(24)); }
// returns 0x10
DECOMP(0076BDFC, ReturnK_0076BDFC) { ret(ctx, static_cast<s32>(16)); }
// returns 0x19
DECOMP(0076BE04, ReturnK_0076BE04) { ret(ctx, static_cast<s32>(25)); }
// returns 0x1
DECOMP(007703A4, ReturnK_007703A4) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(007703AC, ReturnK_007703AC) { ret(ctx, static_cast<s32>(1)); }
// returns 0xFF
DECOMP(007703B8, ReturnK_007703B8) { ret(ctx, static_cast<s32>(255)); }
// returns 0x0
DECOMP(00770684, ReturnK_00770684) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(00775714, ReturnK_00775714) { ret(ctx, static_cast<s32>(1)); }
// returns 0x0
DECOMP(0077584C, ReturnK_0077584C) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(00775984, ReturnK_00775984) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00775ABC, ReturnK_00775ABC) { ret(ctx, static_cast<s32>(1)); }
// returns 0x0
DECOMP(00776444, ReturnK_00776444) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(0077D900, ReturnK_0077D900) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(0077D910, SubMachine_WantsPause) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(0077D918, ReturnK_0077D918) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(0077D920, ReturnK_0077D920) { ret(ctx, static_cast<s32>(1)); }
// returns 0x0
DECOMP(0077E89C, ReturnK_0077E89C) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0077E8A4, ReturnK_0077E8A4) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0077E9AC, ReturnK_0077E9AC) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0077E9B4, ReturnK_0077E9B4) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0077E9BC, ReturnK_0077E9BC) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0077EB9C, ReturnK_0077EB9C) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(0077EDCC, ReturnK_0077EDCC) { ret(ctx, static_cast<s32>(1)); }
// returns 0x0
DECOMP(0077EE98, ReturnK_0077EE98) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0077EEA8, ReturnK_0077EEA8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0077EEB0, ReturnK_0077EEB0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0077EEB8, ReturnK_0077EEB8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(0077EEE0, ReturnK_0077EEE0) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(0077F7E0, ReturnK_0077F7E0) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(0077F8F8, ReturnK_0077F8F8) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(0077FB20, ReturnK_0077FB20) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(0077FBB0, ReturnK_0077FBB0) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(0077FC40, ReturnK_0077FC40) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(0077FD58, ReturnK_0077FD58) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(0077FDE8, ReturnK_0077FDE8) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00780288, ReturnK_00780288) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(007804B0, ReturnK_007804B0) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00780F08, ReturnK_00780F08) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(007811B8, ReturnK_007811B8) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00781358, ReturnK_00781358) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(007813E8, ReturnK_007813E8) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00781478, ReturnK_00781478) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00781508, ReturnK_00781508) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(007816A8, ReturnK_007816A8) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00781738, ReturnK_00781738) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(007817C8, ReturnK_007817C8) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00781858, ReturnK_00781858) { ret(ctx, static_cast<s32>(1)); }
// returns 0x0
DECOMP(00782430, ReturnK_00782430) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00793D4C, ReturnK_00793D4C) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00793D54, ReturnK_00793D54) { ret(ctx, static_cast<s32>(0)); }
// returns 0xC
DECOMP(00793DCC, ReturnK_00793DCC) { ret(ctx, static_cast<s32>(12)); }
// returns 0x18
DECOMP(00793E18, ReturnK_00793E18) { ret(ctx, static_cast<s32>(24)); }
// returns 0x1
DECOMP(00793E20, ReturnK_00793E20) { ret(ctx, static_cast<s32>(1)); }
// returns 0x2
DECOMP(00793E54, ReturnK_00793E54) { ret(ctx, static_cast<s32>(2)); }
// returns 0x18
DECOMP(00793EA4, ReturnK_00793EA4) { ret(ctx, static_cast<s32>(24)); }
// returns 0x1
DECOMP(00793EAC, ReturnK_00793EAC) { ret(ctx, static_cast<s32>(1)); }
// returns 0x1
DECOMP(00793ED8, ReturnK_00793ED8) { ret(ctx, static_cast<s32>(1)); }
// returns 0x18
DECOMP(00793F2C, ReturnK_00793F2C) { ret(ctx, static_cast<s32>(24)); }
// returns 0x0
DECOMP(00793F34, ReturnK_00793F34) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(00793F3C, ReturnK_00793F3C) { ret(ctx, static_cast<s32>(1)); }
// returns 0x5
DECOMP(00793F44, ReturnK_00793F44) { ret(ctx, static_cast<s32>(5)); }
// returns 0x28
DECOMP(00794040, ReturnK_00794040) { ret(ctx, static_cast<s32>(40)); }
// returns 0x0
DECOMP(00794048, ReturnK_00794048) { ret(ctx, static_cast<s32>(0)); }
// returns 0x3
DECOMP(00794050, ReturnK_00794050) { ret(ctx, static_cast<s32>(3)); }
// returns 0x30
DECOMP(00794068, ReturnK_00794068) { ret(ctx, static_cast<s32>(48)); }
// returns 0x0
DECOMP(00794070, ReturnK_00794070) { ret(ctx, static_cast<s32>(0)); }
// returns 0x4
DECOMP(00794078, ReturnK_00794078) { ret(ctx, static_cast<s32>(4)); }
// returns 0x60
DECOMP(00794090, ReturnK_00794090) { ret(ctx, static_cast<s32>(96)); }
// returns 0x0
DECOMP(00794098, ReturnK_00794098) { ret(ctx, static_cast<s32>(0)); }
// returns 0x6
DECOMP(007940A0, ReturnK_007940A0) { ret(ctx, static_cast<s32>(6)); }
// returns 0x1C
DECOMP(007940A8, ReturnK_007940A8) { ret(ctx, static_cast<s32>(28)); }
// returns 0x0
DECOMP(007940B0, ReturnK_007940B0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007940B8, ReturnK_007940B8) { ret(ctx, static_cast<s32>(0)); }
// returns 0xA
DECOMP(007940C0, ReturnK_007940C0) { ret(ctx, static_cast<s32>(10)); }
// returns 0x2C
DECOMP(007940C8, ReturnK_007940C8) { ret(ctx, static_cast<s32>(44)); }
// returns 0x1
DECOMP(00794124, ReturnK_00794124) { ret(ctx, static_cast<s32>(1)); }
// returns 0xB
DECOMP(0079412C, ReturnK_0079412C) { ret(ctx, static_cast<s32>(11)); }
// returns 0x0
DECOMP(00794180, ReturnK_00794180) { ret(ctx, static_cast<s32>(0)); }
// returns 0x44
DECOMP(00794188, ReturnK_00794188) { ret(ctx, static_cast<s32>(68)); }
// returns 0x0
DECOMP(00794190, ReturnK_00794190) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00794198, ReturnK_00794198) { ret(ctx, static_cast<s32>(0)); }
// returns 0x7
DECOMP(007941A0, ReturnK_007941A0) { ret(ctx, static_cast<s32>(7)); }
// returns 0x1C
DECOMP(007941A8, ReturnK_007941A8) { ret(ctx, static_cast<s32>(28)); }
// returns 0x0
DECOMP(007941B8, ReturnK_007941B8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007941C0, ReturnK_007941C0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007941C8, ReturnK_007941C8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007941D0, ReturnK_007941D0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(007941D8, ReturnK_007941D8) { ret(ctx, static_cast<s32>(1)); }
// returns 0x0
DECOMP(007941E0, ReturnK_007941E0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007941E8, ReturnK_007941E8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(007941F0, ReturnK_007941F0) { ret(ctx, static_cast<s32>(1)); }
// returns 0x0
DECOMP(007941F8, ReturnK_007941F8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00794200, ReturnK_00794200) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00794208, ReturnK_00794208) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(00794520, ReturnK_00794520) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(007960AC, ReturnK_007960AC) { ret(ctx, static_cast<s32>(0)); }
// returns 0x1
DECOMP(007962A0, ReturnK_007962A0) { ret(ctx, static_cast<s32>(1)); }
// returns 0x2
DECOMP(007963EC, ReturnK_007963EC) { ret(ctx, static_cast<s32>(2)); }
// returns 0x6
DECOMP(00796530, ReturnK_00796530) { ret(ctx, static_cast<s32>(6)); }
// returns 0x7
DECOMP(00796690, ReturnK_00796690) { ret(ctx, static_cast<s32>(7)); }
// returns 0x8
DECOMP(00796884, ReturnK_00796884) { ret(ctx, static_cast<s32>(8)); }
// returns 0x9
DECOMP(0079688C, ReturnK_0079688C) { ret(ctx, static_cast<s32>(9)); }
// returns 0x0
DECOMP(0079B9A0, ReturnK_0079B9A0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0079B9A8, ReturnK_0079B9A8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0079B9B0, ReturnK_0079B9B0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0079B9B8, ReturnK_0079B9B8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0079B9C0, ReturnK_0079B9C0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0079B9C8, ReturnK_0079B9C8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0079B9D0, ReturnK_0079B9D0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0079B9D8, ReturnK_0079B9D8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0079B9E0, ReturnK_0079B9E0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0079B9E8, ReturnK_0079B9E8) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0079B9F0, ReturnK_0079B9F0) { ret(ctx, static_cast<s32>(0)); }
// returns 0x0
DECOMP(0079B9F8, ReturnK_0079B9F8) { ret(ctx, static_cast<s32>(0)); }

// ---------------------------------------------------------------- constant returns (an address) (43)
// returns 0xB0DB30
DECOMP(00737990, ReturnK_00737990) { ret(ctx, 0x00B0DB30u); }
// returns 0xB0DFF8
DECOMP(007388BC, ReturnK_007388BC) { ret(ctx, 0x00B0DFF8u); }
// returns 0xB0E188
DECOMP(00738B44, ReturnK_00738B44) { ret(ctx, 0x00B0E188u); }
// returns 0xB0E588
DECOMP(00738E14, ReturnK_00738E14) { ret(ctx, 0x00B0E588u); }
// returns 0xB0E718
DECOMP(00739138, ReturnK_00739138) { ret(ctx, 0x00B0E718u); }
// returns 0x11E99FC
DECOMP(00746CEC, ReturnK_00746CEC) { ret(ctx, 0x011E99FCu); }
// returns 0xB13B28
DECOMP(00749480, ReturnK_00749480) { ret(ctx, 0x00B13B28u); }
// returns 0xB13B80
DECOMP(00749624, ReturnK_00749624) { ret(ctx, 0x00B13B80u); }
// returns 0xB13C08
DECOMP(00749640, ReturnK_00749640) { ret(ctx, 0x00B13C08u); }
// returns 0xB13C60
DECOMP(0074965C, ReturnK_0074965C) { ret(ctx, 0x00B13C60u); }
// returns 0xB13D38
DECOMP(0074971C, ReturnK_0074971C) { ret(ctx, 0x00B13D38u); }
// returns 0xB13D90
DECOMP(00749774, ReturnK_00749774) { ret(ctx, 0x00B13D90u); }
// returns 0xB13DE8
DECOMP(007497CC, ReturnK_007497CC) { ret(ctx, 0x00B13DE8u); }
// returns 0xB13E48
DECOMP(00749808, ReturnK_00749808) { ret(ctx, 0x00B13E48u); }
// returns 0xB13EA0
DECOMP(00749824, ReturnK_00749824) { ret(ctx, 0x00B13EA0u); }
// returns 0xB13F00
DECOMP(00749844, ReturnK_00749844) { ret(ctx, 0x00B13F00u); }
// returns 0xB13F60
DECOMP(00749860, ReturnK_00749860) { ret(ctx, 0x00B13F60u); }
// returns 0xB13FB8
DECOMP(00749874, ReturnK_00749874) { ret(ctx, 0x00B13FB8u); }
// returns 0xB14010
DECOMP(007499A0, ReturnK_007499A0) { ret(ctx, 0x00B14010u); }
// returns 0xB14068
DECOMP(007499BC, ReturnK_007499BC) { ret(ctx, 0x00B14068u); }
// returns 0xB140C0
DECOMP(007499D8, ReturnK_007499D8) { ret(ctx, 0x00B140C0u); }
// returns 0xB14600
DECOMP(0074F0E0, ReturnK_0074F0E0) { ret(ctx, 0x00B14600u); }
// returns 0xB14601
DECOMP(0074F0EC, ReturnK_0074F0EC) { ret(ctx, 0x00B14601u); }
// returns 0xB14630
DECOMP(0074F0F8, ReturnK_0074F0F8) { ret(ctx, 0x00B14630u); }
// returns 0xB14631
DECOMP(0074F104, ReturnK_0074F104) { ret(ctx, 0x00B14631u); }
// returns 0xB14660
DECOMP(0074F110, ReturnK_0074F110) { ret(ctx, 0x00B14660u); }
// returns 0xB14661
DECOMP(0074F11C, ReturnK_0074F11C) { ret(ctx, 0x00B14661u); }
// returns 0xB14690
DECOMP(0074F128, ReturnK_0074F128) { ret(ctx, 0x00B14690u); }
// returns 0xB14691
DECOMP(0074F134, ReturnK_0074F134) { ret(ctx, 0x00B14691u); }
// returns 0xB146C0
DECOMP(0074F140, ReturnK_0074F140) { ret(ctx, 0x00B146C0u); }
// returns 0xB146C1
DECOMP(0074F14C, ReturnK_0074F14C) { ret(ctx, 0x00B146C1u); }
// returns 0xB146F0
DECOMP(0074F158, ReturnK_0074F158) { ret(ctx, 0x00B146F0u); }
// returns 0xB146F1
DECOMP(0074F164, ReturnK_0074F164) { ret(ctx, 0x00B146F1u); }
// returns 0xB14720
DECOMP(0074F170, ReturnK_0074F170) { ret(ctx, 0x00B14720u); }
// returns 0xB14721
DECOMP(0074F17C, ReturnK_0074F17C) { ret(ctx, 0x00B14721u); }
// returns 0xB1DD08
DECOMP(00768944, ReturnK_00768944) { ret(ctx, 0x00B1DD08u); }
// returns 0xB214E0
DECOMP(007822EC, ReturnK_007822EC) { ret(ctx, 0x00B214E0u); }
// returns 0xB22588
DECOMP(007948D8, ReturnK_007948D8) { ret(ctx, 0x00B22588u); }
// returns 0x1164E00
DECOMP(007958E8, ReturnK_007958E8) { ret(ctx, 0x01164E00u); }
// returns 0x1164E04
DECOMP(00795938, ReturnK_00795938) { ret(ctx, 0x01164E04u); }
// returns 0x1164E08
DECOMP(00795988, ReturnK_00795988) { ret(ctx, 0x01164E08u); }
// returns 0x1164E0C
DECOMP(007959D8, ReturnK_007959D8) { ret(ctx, 0x01164E0Cu); }
// returns 0x1164E10
DECOMP(00795A28, ReturnK_00795A28) { ret(ctx, 0x01164E10u); }

// ---------------------------------------------------------------- empty functions (219)
// empty
DECOMP(00731858, Nop_00731858) { static_cast<void>(ctx); }
// empty
DECOMP(00735F90, Nop_00735F90) { static_cast<void>(ctx); }
// empty
DECOMP(00735F94, Nop_00735F94) { static_cast<void>(ctx); }
// empty
DECOMP(00735F98, Nop_00735F98) { static_cast<void>(ctx); }
// empty
DECOMP(00735F9C, Nop_00735F9C) { static_cast<void>(ctx); }
// empty
DECOMP(00735FA0, Nop_00735FA0) { static_cast<void>(ctx); }
// empty
DECOMP(00735FA4, Nop_00735FA4) { static_cast<void>(ctx); }
// empty
DECOMP(00735FA8, Nop_00735FA8) { static_cast<void>(ctx); }
// empty
DECOMP(00735FAC, Nop_00735FAC) { static_cast<void>(ctx); }
// empty
DECOMP(007369F8, Nop_007369F8) { static_cast<void>(ctx); }
// empty
DECOMP(00736BA0, Nop_00736BA0) { static_cast<void>(ctx); }
// empty
DECOMP(00736C9C, Nop_00736C9C) { static_cast<void>(ctx); }
// empty
DECOMP(00736D98, Nop_00736D98) { static_cast<void>(ctx); }
// empty
DECOMP(00736EC4, Nop_00736EC4) { static_cast<void>(ctx); }
// empty
DECOMP(00736FC0, Nop_00736FC0) { static_cast<void>(ctx); }
// empty
DECOMP(007370E8, Nop_007370E8) { static_cast<void>(ctx); }
// empty
DECOMP(007371E4, Nop_007371E4) { static_cast<void>(ctx); }
// empty
DECOMP(00737390, Nop_00737390) { static_cast<void>(ctx); }
// empty
DECOMP(007374B0, Nop_007374B0) { static_cast<void>(ctx); }
// empty
DECOMP(007374B4, Nop_007374B4) { static_cast<void>(ctx); }
// empty
DECOMP(007374C0, Nop_007374C0) { static_cast<void>(ctx); }
// empty
DECOMP(007374C4, Nop_007374C4) { static_cast<void>(ctx); }
// empty
DECOMP(00737594, Nop_00737594) { static_cast<void>(ctx); }
// empty
DECOMP(00737598, Nop_00737598) { static_cast<void>(ctx); }
// empty
DECOMP(007375AC, Nop_007375AC) { static_cast<void>(ctx); }
// empty
DECOMP(007375F4, Nop_007375F4) { static_cast<void>(ctx); }
// empty
DECOMP(007375F8, Nop_007375F8) { static_cast<void>(ctx); }
// empty
DECOMP(007378A4, Nop_007378A4) { static_cast<void>(ctx); }
// empty
DECOMP(007378A8, Nop_007378A8) { static_cast<void>(ctx); }
// empty
DECOMP(007378AC, Nop_007378AC) { static_cast<void>(ctx); }
// empty
DECOMP(007378B0, Nop_007378B0) { static_cast<void>(ctx); }
// empty
DECOMP(007378B4, Nop_007378B4) { static_cast<void>(ctx); }
// empty
DECOMP(007378B8, Nop_007378B8) { static_cast<void>(ctx); }
// empty
DECOMP(007378BC, Nop_007378BC) { static_cast<void>(ctx); }
// empty
DECOMP(007378C0, Nop_007378C0) { static_cast<void>(ctx); }
// empty
DECOMP(00737980, Nop_00737980) { static_cast<void>(ctx); }
// empty
DECOMP(0073798C, Nop_0073798C) { static_cast<void>(ctx); }
// empty
DECOMP(007379F0, Nop_007379F0) { static_cast<void>(ctx); }
// empty
DECOMP(0073835C, Nop_0073835C) { static_cast<void>(ctx); }
// empty
DECOMP(00738360, Nop_00738360) { static_cast<void>(ctx); }
// empty
DECOMP(007383F8, Nop_007383F8) { static_cast<void>(ctx); }
// empty
DECOMP(007383FC, Nop_007383FC) { static_cast<void>(ctx); }
// empty
DECOMP(00738400, Nop_00738400) { static_cast<void>(ctx); }
// empty
DECOMP(007384D4, Nop_007384D4) { static_cast<void>(ctx); }
// empty
DECOMP(00738570, Nop_00738570) { static_cast<void>(ctx); }
// empty
DECOMP(007386D0, Nop_007386D0) { static_cast<void>(ctx); }
// empty
DECOMP(007386D4, Nop_007386D4) { static_cast<void>(ctx); }
// empty
DECOMP(00738718, Nop_00738718) { static_cast<void>(ctx); }
// empty
DECOMP(0073871C, Nop_0073871C) { static_cast<void>(ctx); }
// empty
DECOMP(00738720, Nop_00738720) { static_cast<void>(ctx); }
// empty
DECOMP(00738748, Nop_00738748) { static_cast<void>(ctx); }
// empty
DECOMP(0073874C, Nop_0073874C) { static_cast<void>(ctx); }
// empty
DECOMP(00738750, Nop_00738750) { static_cast<void>(ctx); }
// empty
DECOMP(00738754, Nop_00738754) { static_cast<void>(ctx); }
// empty
DECOMP(00738CA0, Nop_00738CA0) { static_cast<void>(ctx); }
// empty
DECOMP(00738DA4, Nop_00738DA4) { static_cast<void>(ctx); }
// empty
DECOMP(00738DFC, Nop_00738DFC) { static_cast<void>(ctx); }
// empty
DECOMP(00738E08, Nop_00738E08) { static_cast<void>(ctx); }
// empty
DECOMP(00738EF0, Nop_00738EF0) { static_cast<void>(ctx); }
// empty
DECOMP(00739608, Nop_00739608) { static_cast<void>(ctx); }
// empty
DECOMP(007396B8, Nop_007396B8) { static_cast<void>(ctx); }
// empty
DECOMP(007396BC, Nop_007396BC) { static_cast<void>(ctx); }
// empty
DECOMP(0073975C, Nop_0073975C) { static_cast<void>(ctx); }
// empty
DECOMP(0073982C, Nop_0073982C) { static_cast<void>(ctx); }
// empty
DECOMP(0073B8EC, Nop_0073B8EC) { static_cast<void>(ctx); }
// empty
DECOMP(0073B8F8, Nop_0073B8F8) { static_cast<void>(ctx); }
// empty
DECOMP(0073B9FC, Nop_0073B9FC) { static_cast<void>(ctx); }
// empty
DECOMP(0073BB2C, Nop_0073BB2C) { static_cast<void>(ctx); }
// empty
DECOMP(0073BBF4, Nop_0073BBF4) { static_cast<void>(ctx); }
// empty
DECOMP(0073BCEC, Nop_0073BCEC) { static_cast<void>(ctx); }
// empty
DECOMP(0073EFA4, Nop_0073EFA4) { static_cast<void>(ctx); }
// empty
DECOMP(00742AE8, Nop_00742AE8) { static_cast<void>(ctx); }
// empty
DECOMP(0074354C, Nop_0074354C) { static_cast<void>(ctx); }
// empty
DECOMP(00743550, Nop_00743550) { static_cast<void>(ctx); }
// empty
DECOMP(00743554, Nop_00743554) { static_cast<void>(ctx); }
// empty
DECOMP(007435AC, Nop_007435AC) { static_cast<void>(ctx); }
// empty
DECOMP(007435B0, Nop_007435B0) { static_cast<void>(ctx); }
// empty
DECOMP(007435B4, Nop_007435B4) { static_cast<void>(ctx); }
// empty
DECOMP(007435CC, Nop_007435CC) { static_cast<void>(ctx); }
// empty
DECOMP(00743904, Nop_00743904) { static_cast<void>(ctx); }
// empty
DECOMP(00743918, Nop_00743918) { static_cast<void>(ctx); }
// empty
DECOMP(007442F4, Nop_007442F4) { static_cast<void>(ctx); }
// empty
DECOMP(007442F8, Nop_007442F8) { static_cast<void>(ctx); }
// empty
DECOMP(007442FC, Nop_007442FC) { static_cast<void>(ctx); }
// empty
DECOMP(00744300, Nop_00744300) { static_cast<void>(ctx); }
// empty
DECOMP(007444B4, Nop_007444B4) { static_cast<void>(ctx); }
// empty
DECOMP(00746B8C, Nop_00746B8C) { static_cast<void>(ctx); }
// empty
DECOMP(00746C14, Nop_00746C14) { static_cast<void>(ctx); }
// empty
DECOMP(00746C18, Nop_00746C18) { static_cast<void>(ctx); }
// empty
DECOMP(00746CAC, Nop_00746CAC) { static_cast<void>(ctx); }
// empty
DECOMP(00746CB0, Nop_00746CB0) { static_cast<void>(ctx); }
// empty
DECOMP(00746CE8, Nop_00746CE8) { static_cast<void>(ctx); }
// empty
DECOMP(00746E4C, Nop_00746E4C) { static_cast<void>(ctx); }
// empty
DECOMP(00746F14, Nop_00746F14) { static_cast<void>(ctx); }
// empty
DECOMP(00747C18, Nop_00747C18) { static_cast<void>(ctx); }
// empty
DECOMP(00747C1C, Nop_00747C1C) { static_cast<void>(ctx); }
// empty
DECOMP(00747C20, Nop_00747C20) { static_cast<void>(ctx); }
// empty
DECOMP(00747C24, Nop_00747C24) { static_cast<void>(ctx); }
// empty
DECOMP(00747C28, Nop_00747C28) { static_cast<void>(ctx); }
// empty
DECOMP(00747C2C, Nop_00747C2C) { static_cast<void>(ctx); }
// empty
DECOMP(00747C30, Nop_00747C30) { static_cast<void>(ctx); }
// empty
DECOMP(00747CD0, Nop_00747CD0) { static_cast<void>(ctx); }
// empty
DECOMP(00748240, Nop_00748240) { static_cast<void>(ctx); }
// empty
DECOMP(00748244, Nop_00748244) { static_cast<void>(ctx); }
// empty
DECOMP(00748248, Nop_00748248) { static_cast<void>(ctx); }
// empty
DECOMP(0074824C, Nop_0074824C) { static_cast<void>(ctx); }
// empty
DECOMP(00748250, Nop_00748250) { static_cast<void>(ctx); }
// empty
DECOMP(00748254, Nop_00748254) { static_cast<void>(ctx); }
// empty
DECOMP(00749378, Nop_00749378) { static_cast<void>(ctx); }
// empty
DECOMP(00749830, Nop_00749830) { static_cast<void>(ctx); }
// empty
DECOMP(0074EF94, Nop_0074EF94) { static_cast<void>(ctx); }
// empty
DECOMP(0074EF98, Nop_0074EF98) { static_cast<void>(ctx); }
// empty
DECOMP(0074EF9C, Nop_0074EF9C) { static_cast<void>(ctx); }
// empty
DECOMP(0074EFA0, Nop_0074EFA0) { static_cast<void>(ctx); }
// empty
DECOMP(0074EFA4, Nop_0074EFA4) { static_cast<void>(ctx); }
// empty
DECOMP(0074F0DC, Nop_0074F0DC) { static_cast<void>(ctx); }
// empty
DECOMP(0074F190, Nop_0074F190) { static_cast<void>(ctx); }
// empty
DECOMP(0074F2E4, Nop_0074F2E4) { static_cast<void>(ctx); }
// empty
DECOMP(0074F2E8, Nop_0074F2E8) { static_cast<void>(ctx); }
// empty
DECOMP(0074F2EC, Nop_0074F2EC) { static_cast<void>(ctx); }
// empty
DECOMP(0074F2F0, Nop_0074F2F0) { static_cast<void>(ctx); }
// empty
DECOMP(0074F2F4, Nop_0074F2F4) { static_cast<void>(ctx); }
// empty
DECOMP(0074F468, Nop_0074F468) { static_cast<void>(ctx); }
// empty
DECOMP(0074F46C, Nop_0074F46C) { static_cast<void>(ctx); }
// empty
DECOMP(0074F470, Nop_0074F470) { static_cast<void>(ctx); }
// empty
DECOMP(0074F474, Nop_0074F474) { static_cast<void>(ctx); }
// empty
DECOMP(0074F478, Nop_0074F478) { static_cast<void>(ctx); }
// empty
DECOMP(0074F47C, Nop_0074F47C) { static_cast<void>(ctx); }
// empty
DECOMP(0074F480, Nop_0074F480) { static_cast<void>(ctx); }
// empty
DECOMP(0074F5FC, Nop_0074F5FC) { static_cast<void>(ctx); }
// empty
DECOMP(0074F600, Nop_0074F600) { static_cast<void>(ctx); }
// empty
DECOMP(0074F604, Nop_0074F604) { static_cast<void>(ctx); }
// empty
DECOMP(0074F608, Nop_0074F608) { static_cast<void>(ctx); }
// empty
DECOMP(0074F60C, Nop_0074F60C) { static_cast<void>(ctx); }
// empty
DECOMP(007538F4, Nop_007538F4) { static_cast<void>(ctx); }
// empty
DECOMP(007538F8, Nop_007538F8) { static_cast<void>(ctx); }
// empty
DECOMP(007538FC, Nop_007538FC) { static_cast<void>(ctx); }
// empty
DECOMP(00753900, Nop_00753900) { static_cast<void>(ctx); }
// empty
DECOMP(00758634, Nop_00758634) { static_cast<void>(ctx); }
// empty
DECOMP(00758638, Nop_00758638) { static_cast<void>(ctx); }
// empty
DECOMP(0075863C, Nop_0075863C) { static_cast<void>(ctx); }
// empty
DECOMP(00758640, Nop_00758640) { static_cast<void>(ctx); }
// empty
DECOMP(00758644, Nop_00758644) { static_cast<void>(ctx); }
// empty
DECOMP(00759244, Nop_00759244) { static_cast<void>(ctx); }
// empty
DECOMP(00759248, Nop_00759248) { static_cast<void>(ctx); }
// empty
DECOMP(0075924C, Nop_0075924C) { static_cast<void>(ctx); }
// empty
DECOMP(00759250, Nop_00759250) { static_cast<void>(ctx); }
// empty
DECOMP(00759254, Nop_00759254) { static_cast<void>(ctx); }
// empty
DECOMP(00759E40, Nop_00759E40) { static_cast<void>(ctx); }
// empty
DECOMP(0075D4CC, Nop_0075D4CC) { static_cast<void>(ctx); }
// empty
DECOMP(0075D670, Nop_0075D670) { static_cast<void>(ctx); }
// empty
DECOMP(0075D674, Nop_0075D674) { static_cast<void>(ctx); }
// empty
DECOMP(0075D748, Nop_0075D748) { static_cast<void>(ctx); }
// empty
DECOMP(0075D74C, Nop_0075D74C) { static_cast<void>(ctx); }
// empty
DECOMP(0075D750, Nop_0075D750) { static_cast<void>(ctx); }
// empty
DECOMP(0075DAAC, Nop_0075DAAC) { static_cast<void>(ctx); }
// empty
DECOMP(0075DAB0, Nop_0075DAB0) { static_cast<void>(ctx); }
// empty
DECOMP(0075DAB4, Nop_0075DAB4) { static_cast<void>(ctx); }
// empty
DECOMP(0075DAB8, Nop_0075DAB8) { static_cast<void>(ctx); }
// empty
DECOMP(0075DABC, Nop_0075DABC) { static_cast<void>(ctx); }
// empty
DECOMP(0075DAC0, Nop_0075DAC0) { static_cast<void>(ctx); }
// empty
DECOMP(0075DAC4, Nop_0075DAC4) { static_cast<void>(ctx); }
// empty
DECOMP(00762AAC, Nop_00762AAC) { static_cast<void>(ctx); }
// empty
DECOMP(00762AB0, Nop_00762AB0) { static_cast<void>(ctx); }
// empty
DECOMP(00762AB4, Nop_00762AB4) { static_cast<void>(ctx); }
// empty
DECOMP(00763EE8, Nop_00763EE8) { static_cast<void>(ctx); }
// empty
DECOMP(00763EEC, Nop_00763EEC) { static_cast<void>(ctx); }
// empty
DECOMP(00767A8C, Nop_00767A8C) { static_cast<void>(ctx); }
// empty
DECOMP(00767A90, Nop_00767A90) { static_cast<void>(ctx); }
// empty
DECOMP(00767AA8, Nop_00767AA8) { static_cast<void>(ctx); }
// empty
DECOMP(00767AAC, Nop_00767AAC) { static_cast<void>(ctx); }
// empty
DECOMP(007684D0, Nop_007684D0) { static_cast<void>(ctx); }
// empty
DECOMP(0076856C, Nop_0076856C) { static_cast<void>(ctx); }
// empty
DECOMP(00768704, Nop_00768704) { static_cast<void>(ctx); }
// empty
DECOMP(00769A28, Nop_00769A28) { static_cast<void>(ctx); }
// empty
DECOMP(00769A98, Nop_00769A98) { static_cast<void>(ctx); }
// empty
DECOMP(0076B3FC, Nop_0076B3FC) { static_cast<void>(ctx); }
// empty
DECOMP(0077037C, Nop_0077037C) { static_cast<void>(ctx); }
// empty
DECOMP(00770380, Nop_00770380) { static_cast<void>(ctx); }
// empty
DECOMP(007703A0, Nop_007703A0) { static_cast<void>(ctx); }
// empty
DECOMP(007703B4, Nop_007703B4) { static_cast<void>(ctx); }
// empty
DECOMP(007703CC, Nop_007703CC) { static_cast<void>(ctx); }
// empty
DECOMP(00770520, Nop_00770520) { static_cast<void>(ctx); }
// empty
DECOMP(0077068C, Nop_0077068C) { static_cast<void>(ctx); }
// empty
DECOMP(00770690, Nop_00770690) { static_cast<void>(ctx); }
// empty
DECOMP(007706C8, Nop_007706C8) { static_cast<void>(ctx); }
// empty
DECOMP(007706CC, Nop_007706CC) { static_cast<void>(ctx); }
// empty
DECOMP(007706D0, Nop_007706D0) { static_cast<void>(ctx); }
// empty
DECOMP(007706D4, Nop_007706D4) { static_cast<void>(ctx); }
// empty
DECOMP(0077072C, Nop_0077072C) { static_cast<void>(ctx); }
// empty
DECOMP(007709D4, Nop_007709D4) { static_cast<void>(ctx); }
// empty
DECOMP(007714A0, Nop_007714A0) { static_cast<void>(ctx); }
// empty
DECOMP(007714A4, Nop_007714A4) { static_cast<void>(ctx); }
// empty
DECOMP(00771CFC, Nop_00771CFC) { static_cast<void>(ctx); }
// empty
DECOMP(00772940, Nop_00772940) { static_cast<void>(ctx); }
// empty
DECOMP(0077D8F0, Nop_0077D8F0) { static_cast<void>(ctx); }
// empty
DECOMP(0077D8F4, Nop_0077D8F4) { static_cast<void>(ctx); }
// empty
DECOMP(0077D908, Nop_0077D908) { static_cast<void>(ctx); }
// empty
DECOMP(0077D90C, Nop_0077D90C) { static_cast<void>(ctx); }
// empty
DECOMP(0077D928, Nop_0077D928) { static_cast<void>(ctx); }
// empty
DECOMP(0077EDD4, Nop_0077EDD4) { static_cast<void>(ctx); }
// empty
DECOMP(0077EEA0, Nop_0077EEA0) { static_cast<void>(ctx); }
// empty
DECOMP(0077EEA4, Nop_0077EEA4) { static_cast<void>(ctx); }
// empty
DECOMP(0077EEC0, Nop_0077EEC0) { static_cast<void>(ctx); }
// empty
DECOMP(0077EED4, Nop_0077EED4) { static_cast<void>(ctx); }
// empty
DECOMP(0077EED8, Nop_0077EED8) { static_cast<void>(ctx); }
// empty
DECOMP(0077EEDC, Nop_0077EEDC) { static_cast<void>(ctx); }
// empty
DECOMP(00788310, Nop_00788310) { static_cast<void>(ctx); }
// empty
DECOMP(00788314, Nop_00788314) { static_cast<void>(ctx); }
// empty
DECOMP(007945EC, Nop_007945EC) { static_cast<void>(ctx); }
// empty
DECOMP(007946B4, Nop_007946B4) { static_cast<void>(ctx); }
// empty
DECOMP(0079477C, Nop_0079477C) { static_cast<void>(ctx); }
// empty
DECOMP(007948D4, Nop_007948D4) { static_cast<void>(ctx); }
// empty
DECOMP(0079A0FC, Nop_0079A0FC) { static_cast<void>(ctx); }
// empty
DECOMP(0079A1C4, Nop_0079A1C4) { static_cast<void>(ctx); }
// empty
DECOMP(0079BA00, XmlCursor_AsElement) { static_cast<void>(ctx); }
// empty
DECOMP(0079BA04, Nop_0079BA04) { static_cast<void>(ctx); }
// empty
DECOMP(0079BA08, XmlDoc_AsDocument) { static_cast<void>(ctx); }
// empty
DECOMP(0079BA0C, Nop_0079BA0C) { static_cast<void>(ctx); }
