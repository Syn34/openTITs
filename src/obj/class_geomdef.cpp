// ApeGeomDef: the "world class" objects instance from (kind 20).
#include "classes.h"

using namespace sly4;
using namespace sly4::cls;

namespace {

// The inlined release of the shared block at +108 (a no-op in the
// constructor, which has just cleared it).
void ReleaseShared(PPUContext& ctx, u32 g) {
    const u32 b = load<u32>(g + kGeomShared);
    if (b == 0) return;
    store<u32>(g + kGeomShared, 0);
    const u16 refs = static_cast<u16>(load<u16>(b) - 1);
    store<u16>(b, refs);
    if (static_cast<s16>(refs) > 0) return;
    const u32 data = load<u32>(b + 4);
    if (data != 0) {
        call(ctx, cfn::Mem_FreeFrom, {data - 16, 0});
        store<u32>(b + 4, 0);
    }
    call(ctx, cfn::Mem_FreeFrom, {b, 0});
}

// The sphere / name part both constructor stages reset.
void ResetSphere(u32 g) {
    for (u32 k = 0; k < 16; k += 4) store<u32>(g + 48 + k, load<u32>(kDefaultSphere + k));
    store<u32>(g + 96, 0);
    store<u32>(g + 100, 0);
    store<u8>(g + 64, 0);
    store<u32>(g + 104, 0);
}

}  // namespace

// ApeGeomDef* GeomDef_New()  ("WorldClass_Alloc" in symbols.txt)
// A 176-byte ApeGeomDef from its free list (made on first use; 0 when the
// list is empty): the root header as kind 20 with a new object serial, an
// empty box collapsed to the origin, the default sphere, empty name and
// topology table, epsilon 1e-5 at +120, and no skeleton definition. Goes
// through both constructor stages (vtables 0x00B246B0, 0x00B246D8).
DECOMP(007A71A4, GeomDef_New) {
    const u32 header = load<u32>(kGeomDefList);
    u32 alloc = load<u32>(header);
    if (alloc == 0) {
        alloc = 0;
        const u32 obj = static_cast<u32>(call(ctx, obj4::o4fn::Mem_Alloc, {60, 0, 16}));
        if (obj != 0) {
            for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u, 32u, 28u}) store<u32>(obj + off, 0);
            const u16 h54 = load<u16>(obj + 54);
            for (u32 off : {36u, 40u, 44u, 48u}) store<u32>(obj + off, 0);
            store<u16>(obj + 52, 0);
            store<u16>(obj + 54, static_cast<u16>(h54 & 0x7FFF));
            store<u32>(obj, kGeomDefListVtbl);
            store<u32>(obj + 56, kStrGeomDefList);
            // r8 / r9 = the vtable and the name here; FreeList_Init reads r3..r7 only.
            call(ctx, obj4::o4fn::FreeList_Init, {obj, kGeomDefSize, load<u32>(header + 4), 0, 0});
            alloc = obj;
        }
        store<u32>(header, alloc);  // 0 if Mem_Alloc failed: the original then calls through 0
    }
    const u32 opd = load<u32>(load<u32>(alloc) + 12);
    const u32 g = static_cast<u32>(call_ptr(ctx, opd, {alloc, opd, load<u32>(opd)}));  // r4 / r5 = OPD and entry
    if (g == 0) return ret(ctx, 0u);

    const u32 w4 = (load<u32>(g + 4) & 0x3FFu) | 0x00010000u | (kKindGeomDef << 10);
    store<u16>(g + 4, 1);
    store<u32>(g + 4, w4);
    const u32 serial = load<u32>(obj2::kObjSerial) + 1;
    store<u32>(obj2::kObjSerial, serial);
    u32 w8 = load<u32>(g + 8);
    store<u32>(g + 32, obj4::kFltMinNeg);
    w8 = (w8 & 0x7FFu) | ((serial << 11) & 0xFFFFF800u);
    w8 &= ~0x400u;
    store<u32>(g, kVtblGeomBase);
    store<u32>(g + 36, obj4::kFltMinNeg);
    w8 &= ~0x3C0u;
    store<u32>(g + 16, obj4::kFltMax);
    store<u32>(g + 20, obj4::kFltMax);
    store<u32>(g + 24, obj4::kFltMax);
    store<u32>(g + 40, obj4::kFltMinNeg);
    store<u32>(g + kGeomShared, 0);
    store<u32>(g + 12, 0);
    store<u32>(g + 8, w8);
    call(ctx, cfn::Box_SetPoint, {g + 16, kOrigin});
    {
        const u32 shared = load<u32>(g + kGeomShared);
        ResetSphere(g);
        if (shared != 0) ReleaseShared(ctx, g);
    }

    store<u32>(g, kVtblGeomDef);
    for (u32 off : {132u, 140u, 148u, 156u}) {
        store<u32>(g + off + 4, 0);
        store<u32>(g + off, 0);
    }
    store<u32>(g + obj2::kClassRes, 0);
    call(ctx, cfn::Box_SetPoint, {g + 16, kOrigin});
    {
        const u32 shared = load<u32>(g + kGeomShared);
        ResetSphere(g);
        if (shared != 0) ReleaseShared(ctx, g);
    }
    for (u32 off = 112; off < 116; ++off) store<u8>(g + off, 0);
    store<u32>(g + 116, 0);
    const u32 res = load<u32>(g + obj2::kClassRes);
    store<u32>(g + 120, kGeomEpsilon);
    store<u32>(g + 124, 0);
    store<u32>(g + 128, 0);
    if (res != 0) {
        store<u32>(g + obj2::kClassRes, 0);
        call(ctx, cfn::SkeletonDef_Release, {res});
    }
    ret(ctx, g);
}
