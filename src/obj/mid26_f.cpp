// Game code, medium sweep part 26: the exit-time destructor of the tuning
// globals block 0x0106B6E0..0x0106C000 (no direct caller: registered with
// the C runtime).
#include "mid26.h"

using namespace sly4;
using namespace sly4::mid26;

namespace {
using namespace m26fn;
constexpr u32 kTuningVarDtor = 0x004FC1A0u;  // TuningVar_DestructBase_004FC1A0(var)
constexpr u32 kVtVarA = 0x00B0D968u;         // tuning variable vtables set before the base destructor
constexpr u32 kVtVarB = 0x00B0D99Cu;
constexpr u32 kVtVarC = 0x00B0D934u;
constexpr u32 kVtVarD = 0x00B0D9D0u;

// The pool array a (u16 cap +0, u16 element size * 2 +6, data +8,
// allocator +12) returns its data (allocator vt[8](al, data, size)) and is
// destroyed (Array_Destruct).
void PoolArrayFree26(PPUContext& ctx, u32 a) {
    const u32 sz = load<u16>(a) * ((static_cast<u32>(load<u16>(a + 6)) >> 1) & 0x7FFFu);
    if (const u32 d = load<u32>(a + 8); d != 0) {
        const u32 al = load<u32>(a + 12);
        VCall(ctx, al, 8, {al, d, sz});
    }
    call(ctx, 0x0054EC94u, {a});
}

void ClearVecF(PPUContext& ctx, u32 v) {
    call(ctx, PtrVector_Clear, {v});
    call(ctx, Nop, {v});
}

// A tuning variable: its vtable, then the base destructor.
void VarDtor(PPUContext& ctx, u32 var, u32 vt) {
    store<u32>(var, vt);
    call(ctx, kTuningVarDtor, {var});
}

struct VarVt {
    u32 var, vt;
};
constexpr VarVt kVarsA[] = {
    {0x0106BF00u, kVtVarB}, {0x0106BEB0u, kVtVarB}, {0x0106BE60u, kVtVarB}, {0x0106BE10u, kVtVarB},
    {0x0106BDC0u, kVtVarB}, {0x0106BD70u, kVtVarA}, {0x0106BD1Cu, kVtVarC}, {0x0106BCD0u, kVtVarB},
    {0x0106BC84u, kVtVarA},
};
constexpr VarVt kVarsB[] = {
    {0x0106BB50u, kVtVarB}, {0x0106BAF0u, kVtVarD}, {0x0106BA90u, kVtVarD}, {0x0106BA3Cu, kVtVarB},
    {0x0106B9F0u, kVtVarB}, {0x0106B9A4u, kVtVarC}, {0x0106B958u, kVtVarC}, {0x0106B908u, kVtVarC},
    {0x0106B8B8u, kVtVarB}, {0x0106B868u, kVtVarB}, {0x0106B818u, kVtVarB}, {0x0106B7C8u, kVtVarC},
    {0x0106B778u, kVtVarC}, {0x0106B72Cu, kVtVarC}, {0x0106B6E0u, kVtVarC},
};
}  // namespace

// void f(void): when the 64-bit handle 0x0106BFF8 is set, the object
// 0x0106C000 gets vtable 0x00B1F5D0 and 0x007D1CE4(it), and the handle is
// zeroed; 0x00ABF3E4(0x0106BFC8); the pool arrays 0x0106BBB4 / BBCC / BB9C
// freed; the variable 0x0106BF50 (its +76 = 0x00F2B810 first) and the
// first nine variables destroyed; vectors / pool arrays 0x0106BC70,
// BC58, BC40, BC00, BBE8, BC2C, BC14 released; the other fifteen variables
// destroyed. r3 = the last destructor's.
DECOMP(00355A90, Tuning_DestroyGlobals_00355A90) {
    if (load<u64>(0x0106BFF8u) != 0) {
        store<u32>(0x0106C000u, 0x00B1F5D0u);
        call(ctx, 0x007D1CE4u, {0x0106C000u});
        store<u64>(0x0106BFF8u, 0);
    }
    call(ctx, 0x00ABF3E4u, {0x0106BFC8u});
    PoolArrayFree26(ctx, 0x0106BBB4u);
    PoolArrayFree26(ctx, 0x0106BBCCu);
    PoolArrayFree26(ctx, 0x0106BB9Cu);
    store<u32>(0x0106BF9Cu, 0x00F2B810u);
    VarDtor(ctx, 0x0106BF50u, kVtVarA);
    for (const VarVt& v : kVarsA) VarDtor(ctx, v.var, v.vt);
    ClearVecF(ctx, 0x0106BC70u);
    ClearVecF(ctx, 0x0106BC58u);
    PoolArrayFree26(ctx, 0x0106BC40u);
    ClearVecF(ctx, 0x0106BC00u);
    PoolArrayFree26(ctx, 0x0106BBE8u);
    ClearVecF(ctx, 0x0106BC2Cu);
    PoolArrayFree26(ctx, 0x0106BC14u);
    for (const VarVt& v : kVarsB) VarDtor(ctx, v.var, v.vt);
}
