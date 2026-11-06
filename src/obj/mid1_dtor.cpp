// Game code, medium sweep part 1: base destructors shared by many classes.
#include "mid1.h"

using namespace sly4;
using namespace sly4::mid1;

// T::~T() (vtable 0x00B12278; the base under VtB13718 / VtB123D0 /
// VtB12550): the owned object +164 released (vfn[1]) and cleared; 0x0048F298(this,
// 1) when +176 has 0x2000; for the attached object +160: when +172 has
// 0x400, the three rows of this's transform (+32 / +48 / +64 / +80, x y z)
// copied to its +16 / +32 / +48 / +64, WorldObj_MarkDirty and its vfn[10](0,
// 1); then its +276 = 0x25431D90, +280 = this->+204 and its vfn[2](); the
// vectors +340 / +252 / +248 deleted; then Dtor_B219D0_00464AD0(this).
DECOMP(000D7024, Obj_BaseDtor_000D7024) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B12278);
    const u32 owned = load<u32>(o + 164);
    if (owned != 0) {
        VCall(ctx, owned, 4, {owned});
        store<u32>(o + 164, 0);
    }
    if (load<u16>(o + 176) & 0x2000u) call(ctx, m1fn::Obj_Flag13Off_0048F298, {o, 1});
    u32 q = load<u32>(o + 160);
    if (q != 0) {
        if (load<u32>(o + 172) & 0x400u) {
            for (u32 row = 0; row < 4; ++row)
                for (u32 k = 0; k < 3; ++k) store<u32>(q + 16 + 16 * row + 4 * k, load<u32>(o + 32 + 16 * row + 4 * k));
            ctx.set_fpr_bits(1, LfsBits(load<u32>(o + 88)));  // f1 = the last word copied (lfs / stfs)
            call(ctx, m1fn::WorldObj_MarkDirty, {q, q});
            VCall(ctx, load<u32>(o + 160), 40, {load<u32>(o + 160), 0, 1});
            q = load<u32>(o + 160);
        }
        const u32 vt = load<u32>(q);  // read before the stores below
        store<u32>(q + 276, 0x25431D90);
        store<u32>(q + 280, load<u32>(o + 204));
        obj25::VCallVt(ctx, vt, 8, {q});
    }
    PtrVecDeleteSmall(ctx, o + 340);
    PtrVecDeleteSmall(ctx, o + 252);
    PtrVecDeleteSmall(ctx, o + 248);
    call(ctx, m1fn::Dtor_B219D0_00464AD0, {o});
}

// T::~T() (vtable 0x00B1E1F8; the base of the VtB1E3E0 / VtB1E458 /
// VtB1E4D0 / VtB1E918 / VtB1E990 classes): when +2572 is the world's
// +0x1281F4 entry, vtable 0x00B1E368 and World_ReleaseChild1281F4; then
// vtable 0x00B1E1F8; the new[]'d array +2544 (8-byte slots, count at -4)
// cleared from the back and freed; the five lists +2232[i] (each {+0 a
// PtrVector of PtrVectors}) emptied and freed; +2524 / +2512 / +2500 /
// +2488 freed and cleared; the vector +2272 deleted; then the base
// destructor 0x0048F6B0.
DECOMP(003286D8, Obj_BaseDtor_003286D8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 child = load<u32>(o + 2572);
    if (child != 0) {
        const u32 w = load<u32>(o + 8);
        const u32 cur = load<u32>(w + 0x1281F4u);
        if (cur == child) {
            store<u32>(o, 0x00B1E368);
            call(ctx, m1fn::World_ReleaseChild1281F4, {w, w, 0x00B1E368, cur});
        }
    }
    store<u32>(o, 0x00B1E1F8);
    const u32 arr = load<u32>(o + 2544);
    if (arr != 0) {
        const u32 hdr = arr - 4;
        for (s32 k = static_cast<s32>(load<u32>(hdr)) - 1; k >= 0; --k)
            call(ctx, m1fn::Ptr_FreeAndClear_00321240, {arr + 8 * static_cast<u32>(k)});
        call(ctx, m1fn::Mem_FreeFrom, {hdr, 0});
        store<u32>(o + 2544, 0);
    }
    for (u32 i = 0; i < 5; ++i) {
        const u32 slot = o + 2232 + 8 * i;
        u32 l = load<u32>(slot);
        if (l == 0) continue;
        for (u32 j = 0;; ++j) {
            const u32 v = load<u32>(l);  // l == 0 (cleared meanwhile) reads address 0, as the original
            const u32 count = v != 0 ? load<u16>(v + 8) : 0u;
            if (!(j < count)) break;
            const u32 x = load<u32>(load<u32>(load<u32>(l)) + 4 * j);
            if (x != 0) {
                call(ctx, m1fn::PtrVector_Clear, {x});
                call(ctx, m1fn::Nop, {x});
                call(ctx, m1fn::Mem_FreeFrom, {x, 0});
                l = load<u32>(slot);
            }
        }
        if (l == 0) continue;
        PtrVecDeleteSmall(ctx, l);
        call(ctx, m1fn::Mem_FreeFrom, {l, 0});
        store<u32>(slot, 0);
    }
    FreeSlot(ctx, o + 2524);
    FreeSlot(ctx, o + 2512);
    FreeSlot(ctx, o + 2500);
    FreeSlot(ctx, o + 2488);
    PtrVecDeleteSmall(ctx, o + 2272);
    call(ctx, m1fn::Obj_BaseDtor_0048F6B0, {o});
}
