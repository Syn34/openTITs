// Game code, medium sweep part 3: the slot-def list factories (the
// "new + read" entries of the UI slot-def registry; the readers are in
// mid1_slot.cpp / mid2_slot.cpp).
#include "mid3.h"

using namespace sly4;
using namespace sly4::mid3;

namespace {

// new SlotDefList-derived def (176 bytes, Mem_Alloc(176, 0, 16)): the
// inlined SlotDef base (mid2::SlotDefBaseInit), +148 / +152 = 0, +160 =
// 0, +156 = -1.0f, the class's vtable, +80 |= bits; then
// SlotDefList_Read_0035C3E4(p, x, cursor) - also for a failed allocation,
// with a null this, as the original. Returns p; r3 = Read's result.
u32 SlotDefListNew(PPUContext& ctx, u32 vt, u32 bits) {
    const u32 x = arg<u32>(ctx, 0), cursor = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, m3fn::Mem_Alloc, {176, 0, 16}));
    if (p != 0) {
        mid2::SlotDefBaseInit(ctx, p);
        store<u32>(p + 148, 0);
        store<u32>(p + 152, 0);
        store<u8>(p + 160, 0);
        store<u32>(p + 156, mid2::kMinusOneF);
        store<u32>(p + 0, vt);
        store<u32>(p + 80, load<u32>(p + 80) | bits);
    }
    call(ctx, m3fn::SlotDefList_Read_0035C3E4, {p, x, cursor});
    return p;
}

}  // namespace

// SlotDef* f(x, XmlCursor* c): new def (vtable 0x00B1FA38, +80 |=
// 0x00020001) read from c; returns it.
DECOMP(0035C8CC, SlotDefListB1FA38_New_0035C8CC) {
    ret(ctx, SlotDefListNew(ctx, 0x00B1FA38, 0x00020001u));
}

// f(x, XmlCursor* c): new def (vtable 0x00B1F6A0, +80 |= 1) read from c;
// returns SlotDefList_Read_0035C3E4's result (the original drops the
// pointer).
DECOMP(00361108, SlotDefListB1F6A0_New_00361108) {
    SlotDefListNew(ctx, 0x00B1F6A0, 1u);
}
