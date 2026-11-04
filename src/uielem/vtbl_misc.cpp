// Small base-element virtual functions (vtable 0x00B1C9D0 / text 0x00B227D8).
#include "vtbl.h"

using namespace sly4;
using namespace sly4::uielem;

namespace {
constexpr f32 kDegToRad = 0.0174532924f;  // 0x004E0220 (0x3C8EFA35)
constexpr u32 kInvalidateClear = 228;     // f32 +228 zeroed (the reveal time, see UiElem_GetSize)

u32 Map_Lookup(PPUContext& ctx, u32 map, u32 name, u32 key_slot) {
    store<u32>(key_slot, static_cast<u32>(call(ctx, fn::String_HashNoCase, {name})));
    return load<u32>(static_cast<u32>(call(ctx, vfn::HashMap_Find, {map, key_slot})));
}
}  // namespace

// void UiElem_SetRotation(Element* e; f1 = degrees)  [vfunc +32]
// Replaces the element's 3x3 with a rotation about the fixed axis at
// 0x01164B70 (the position is read and written back unchanged).
DECOMP(004E0228, UiElem_SetRotation) {
    const u32 e = arg<u32>(ctx, 0);
    Stack frame(ctx, 64);  // Matrix_FromAxisAngle writes out+48..56 too
    const u32 m = frame.addr();
    const f64 rad = static_cast<f64>(static_cast<f32>(farg(ctx, 0) * static_cast<f64>(kDegToRad)));
    callf(ctx, vfn::Matrix_FromAxisAngle, {m, kRotationAxis}, {rad});
    u32 pos[3];
    for (u32 k = 0; k < 3; ++k) pos[k] = load<u32>(e + kElemPosition + k * 4);
    for (u32 row = 0; row < 3; ++row)
        for (u32 k = 0; k < 3; ++k) store<u32>(e + kElemMatrix + row * 16 + k * 4, load<u32>(m + row * 16 + k * 4));
    for (u32 k = 0; k < 3; ++k) store<u32>(e + kElemPosition + k * 4, pos[k]);
}

// void UiElem_Invalidate(Element* e)  [vfunc +148]
// Marks the element for a new layout and a new measurement, and does the
// same for all its children.
DECOMP(004E22DC, UiElem_Invalidate) {
    const u32 e = arg<u32>(ctx, 0);
    field<u32>(e, kInvalidateClear) = 0;
    field<u32>(e, kTextFlags) |= kElemLayoutPending;
    call(ctx, vfn::UiElem_InvalidateLocal, {e});
    field<u32>(e, kElemFlags) |= kElemSizeDirty;
    for (u32 c = field<u32>(e, kElemFirstChild); c != 0; c = field<u32>(c, kElemNext))
        call_ptr(ctx, vfunc(c, kVfnTextChanged), {c});
}

// void* UiElem_FindResource(Element*, const char* name)  [vfunc +180]
// The resource of that name: the first map, then the second.
DECOMP(004E2BE4, UiElem_FindResource) {
    const u32 name = arg<u32>(ctx, 1);
    Stack frame(ctx, 8);
    const u32 r = Map_Lookup(ctx, kResourcesA, name, frame.addr() + 4);
    if (r != 0) return ret(ctx, r);
    ret(ctx, Map_Lookup(ctx, kResourcesB, name, frame.addr()));
}

// void* UiElem_FindResourceOfType(Element*, const char* name, s32 type)  [vfunc +184]
// The same, accepting only a resource whose vfunc +8 returns `type`.
DECOMP(004E2C78, UiElem_FindResourceOfType) {
    const u32 name = arg<u32>(ctx, 1);
    const s32 type = arg<s32>(ctx, 2);
    Stack frame(ctx, 8);
    const u32 a = Map_Lookup(ctx, kResourcesA, name, frame.addr() + 4);
    if (a != 0 && static_cast<s32>(call_ptr(ctx, vfunc(a, kVfnResourceType), {a})) == type) return ret(ctx, a);
    const u32 b = Map_Lookup(ctx, kResourcesB, name, frame.addr());
    if (b == 0) return ret(ctx, 0u);
    if (static_cast<s32>(call_ptr(ctx, vfunc(b, kVfnResourceType), {b})) != type) return ret(ctx, 0u);
    ret(ctx, b);
}

// Color UiElem_GetColor(const Element* e)  [vfunc +172, sret in r3, e in r4]
// The element's colour, its rgb and/or alpha multiplied by the parent's
// (as (a * b) >> 8) when it inherits them.
DECOMP(004E0FDC, UiElem_GetWorldColor) {
    const u32 out = arg<u32>(ctx, 0);
    const u32 e = arg<u32>(ctx, 1);
    u32 c[4];
    for (u32 k = 0; k < 4; ++k) c[k] = load<u8>(e + kElemColor + k);
    const u32 parent = field<u32>(e, kElemParent);
    if (parent != 0) {
        Stack frame(ctx, 16);
        const u32 pc = frame.addr();
        call_ptr(ctx, vfunc(parent, kVfnColor), {pc, parent});
        const u32 flags = field<u32>(e, kElemFlags);
        const bool rgb = (flags & kElemInheritColor) != 0, alpha = (flags & kElemInheritAlpha) != 0;
        if (rgb)
            for (u32 k = 0; k < 3; ++k) c[k] = (c[k] * load<u8>(pc + k)) >> 8;
        if (alpha) c[3] = (c[3] * load<u8>(pc + 3)) >> 8;
    }
    for (u32 k = 0; k < 4; ++k) store<u8>(out + k, static_cast<u8>(c[k]));
}

// void UiText_Delete(TextElem* e)  [text vfunc +4: deleting destructor]
DECOMP(004E17D4, UiText_Delete) {
    const u32 e = arg<u32>(ctx, 0);
    if (e == 0) return;
    call(ctx, vfn::UiText_Destruct, {e});
    call(ctx, vfn::Mem_FreeFrom, {e, 0});
}
