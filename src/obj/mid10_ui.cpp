// Game code, medium sweep part 10: UI element code.
#include "mid10.h"

using namespace sly4;
using namespace sly4::mid10;

// void UiElem_Destruct4E7B28(UiElem* o)  (called by Delete_004E7D10)
// The destructor of the class with vtable 0x00B22C40 (nine owned children
// o->204[] released through vtable slot 4) with the UiElem base destructor
// (vtable 0x00B22710) inlined: releases the children o->152 until the list
// is empty, unlinks o from its parent's child list (o->148->152, or the
// root list at 0x00F66868; sibling link +156) unless flag 0x01000000 of
// +136 is set, and clears o out of the four slots at 0x010C2AD8. The list
// walk does not stop at a null link, as the original.
DECOMP(004E7B28, UiElem_Destruct4E7B28) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kRootList = 0x00F66868, kSlots = 0x010C2AD8;
    store<u32>(o, 0x00B22C40u);
    for (u32 k = 0; k < 9; ++k) {
        const u32 c = load<u32>(o + 204 + 4 * k);
        if (c != 0) VCall(ctx, c, 4, {c});
    }
    store<u32>(o, 0x00B22710u);
    for (u32 c = load<u32>(o + 152); c != 0; c = load<u32>(o + 152)) VCall(ctx, c, 4, {c});
    if ((load<u32>(o + 136) & 0x01000000u) == 0) {
        const u32 parent = load<u32>(o + 148);
        const u32 head = parent != 0 ? load<u32>(parent + 152) : load<u32>(kRootList);
        u32 prev = 0;
        if (head != o) {
            prev = head;
            for (u32 n = load<u32>(prev + 156); n != o; n = load<u32>(prev + 156)) prev = n;
        }
        if (prev != 0) store<u32>(prev + 156, load<u32>(o + 156));
        else if (parent != 0) store<u32>(parent + 152, load<u32>(o + 156));
        else store<u32>(kRootList, load<u32>(o + 156));
        store<u32>(o + 156, 0);
    }
    for (u32 k = 0; k < 4; ++k)
        if (o == 0 || load<u32>(kSlots + 4 * k) == o) store<u32>(kSlots + 4 * k, 0);
    ctx.gpr[3] = kSlots;  // as the original leaves r3
}
