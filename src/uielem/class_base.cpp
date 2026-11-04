// The base element class's own vtable slots (vtable 0x00B22710).
#include "class.h"

using namespace sly4;
using namespace sly4::uielem;
using namespace sly4::text;

namespace sly4 {
namespace uielem {

void ElemDestructBody(PPUContext& ctx, u32 e) {
    // Each child's deleting destructor (vfunc +4) takes it out of the list.
    for (u32 c; (c = load<u32>(e + kElemFirstChild)) != 0;) {
        const u32 opd = load<u32>(load<u32>(c) + 4);
        call_ptr(ctx, opd, {c, opd, load<u32>(opd)});  // r4 / r5 = OPD and entry, as in the original
    }
    if ((load<u32>(e + 136) & 0x01000000) == 0) {
        const u32 parent = load<u32>(e + 148);
        const u32 head = parent != 0 ? load<u32>(parent + kElemFirstChild) : load<u32>(kUiRoots0);
        u32 prev = 0;
        if (head != e) {
            // No end check: the element must be in the list, as in the original.
            u32 it = head;
            for (;;) {
                prev = it;
                it = load<u32>(it + kElemNext);
                if (it == e) break;
            }
        }
        if (prev != 0)
            store<u32>(prev + kElemNext, load<u32>(e + kElemNext));
        else if (parent != 0)
            store<u32>(parent + kElemFirstChild, load<u32>(e + kElemNext));
        else
            store<u32>(kUiRoots0, load<u32>(e + kElemNext));
        store<u32>(e + kElemNext, 0);
    }
    u32 last = 0;
    for (u32 k = 0; k < kUiFocusSlotCount; ++k) {
        if (e == 0 || (last = load<u32>(kUiFocusSlots + k * 4)) == e) store<u32>(kUiFocusSlots + k * 4, 0);
    }
    // Registers as the loop leaves them.
    ctx.gpr[3] = kUiFocusSlots;
    ctx.gpr[4] = 0;
    ctx.gpr[5] = kUiFocusSlotCount * 4;
    ctx.gpr[6] = (kUiFocusSlotCount - 1) * 4;
    ctx.gpr[7] = kUiFocusSlotCount;
    ctx.gpr[8] = last;
}

}  // namespace uielem
}  // namespace sly4

// void UiElem_Destruct(Element* e)  [vtable 0x00B22710 +0]
DECOMP(004DFF1C, UiElem_Destruct) {
    const u32 e = arg<u32>(ctx, 0);
    store<u32>(e, kVtblBase);
    ElemDestructBody(ctx, e);
}

// void UiElem_Delete(Element* e)  [+4]
DECOMP(004E0094, UiElem_Delete) {
    const u32 e = arg<u32>(ctx, 0);
    if (e == 0) return;
    store<u32>(e, kVtblBase);
    ElemDestructBody(ctx, e);
    call(ctx, clfn::Mem_FreeFrom, {e, 0});
}

// Element* UiElem_CloneNone(...)  [+8]: the base class can't be cloned.
DECOMP(007882F4, UiElem_CloneNone) { ret(ctx, 0); }

// void PureVirtual()  [+12 of several vtables]
// Calls the abort routine, which doesn't return. (In the original the
// code would fall through into the next function if it did.)
DECOMP(007318A4, PureVirtual) { call(ctx, clfn::PureVirtual_Abort, {}); }

// Vec2 UiElem_GetBaseSize(const Element* e)  [+140, sret in r3, e in r4]
DECOMP(007882FC, UiElem_GetBaseSize) {
    const u32 out = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    store<u32>(out, load<u32>(e + 140));
    store<u32>(out + 4, load<u32>(e + 144));
    ctx.fpr[1] = static_cast<f64>(load<f32>(e + 144));  // f1 as the original leaves it
}

// void UiElem_InvalidateBase(Element* e)  [+148]
// Marks the size dirty here and in every descendant (through their +148).
DECOMP(004E0508, UiElem_InvalidateBase) {
    const u32 e = arg<u32>(ctx, 0);
    store<u32>(e + 136, load<u32>(e + 136) | 0x02000000);
    for (u32 c = load<u32>(e + kElemFirstChild); c != 0; c = load<u32>(c + kElemNext)) {
        const u32 opd = vfunc(c, kVfnInvalidate);
        call_ptr(ctx, opd, {c, opd, load<u32>(opd)});  // r4 / r5 = OPD and entry
    }
}

// void UiElem_DrawChildren(Element* e)  [+156]
DECOMP(004E05A4, UiElem_DrawChildren) {
    const u32 e = arg<u32>(ctx, 0);
    if ((load<u32>(e + 136) & 0x80000000) == 0) return;
    for (u32 c = load<u32>(e + kElemFirstChild); c != 0; c = load<u32>(c + kElemNext)) {
        const u32 opd = vfunc(c, kVfnDraw);
        call_ptr(ctx, opd, {c, opd, load<u32>(opd)});  // r4 / r5 = OPD and entry
    }
}

// void* UiElem_FindResourceBase(Element*, const char* name)  [+180]
// Looks the name up in the global resource map only.
DECOMP(004E1368, UiElem_FindResourceBase) {
    const u32 name = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 key = frame.addr();
    store<u32>(key, static_cast<u32>(call(ctx, fn::String_HashNoCase, {name})));
    const u32 slot = static_cast<u32>(call(ctx, clfn::HashMap_Find, {kUiResources, key}));
    ret(ctx, load<u32>(slot));  // the map returns a slot even when the key is missing
}

// void* UiElem_FindResourceOfTypeBase(Element*, const char* name, s32 type)  [+184]
DECOMP(004E13B8, UiElem_FindResourceOfTypeBase) {
    const u32 name = arg<u32>(ctx, 1);
    const s32 type = arg<s32>(ctx, 2);
    Stack frame(ctx, 16);
    const u32 key = frame.addr();
    store<u32>(key, static_cast<u32>(call(ctx, fn::String_HashNoCase, {name})));
    const u32 res = load<u32>(static_cast<u32>(call(ctx, clfn::HashMap_Find, {kUiResources, key})));
    if (res == 0) return ret(ctx, 0u);
    const u32 opd = vfunc(res, kVfnResType);
    const s32 t = static_cast<s32>(call_ptr(ctx, opd, {res, opd, load<u32>(opd)}));  // r4 / r5 = OPD and entry
    ret(ctx, t == type ? res : 0u);
}
