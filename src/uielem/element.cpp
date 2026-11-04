// The base UI element's accessors. Each is one virtual function of the
// base class (byte offset into the vtable in brackets).
#include "uielem.h"

using namespace sly4;
using namespace sly4::uielem;

namespace {

void Elem_GetFlag(PPUContext& ctx, u32 bit) {
    ret(ctx, (field<u32>(arg<u32>(ctx, 0), kElemFlags) & bit) != 0 ? 1u : 0u);
}
// Only bit 0 of the argument is used (rlwimi).
void Elem_SetFlag(PPUContext& ctx, u32 bit, u32 shift) {
    auto& f = field<u32>(arg<u32>(ctx, 0), kElemFlags);
    f = (f & ~bit) | ((arg<u32>(ctx, 1) << shift) & bit);
}
void Elem_GetField(PPUContext& ctx, u32 mask, u32 shift) {
    ret(ctx, (field<u32>(arg<u32>(ctx, 0), kElemFlags) & mask) >> shift);
}
void Elem_SetField(PPUContext& ctx, u32 mask, u32 shift) {
    auto& f = field<u32>(arg<u32>(ctx, 0), kElemFlags);
    f = (f & ~mask) | ((arg<u32>(ctx, 1) << shift) & mask);
}
// lfs/stfs pairs: the bits are copied one value at a time.
void Copy32(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }

}  // namespace

// u32 UiElem_Type()  [+12]
DECOMP(00762150, UiElem_Type) { ret(ctx, kTypeBase); }
DECOMP(00795E6C, UiText_Type) { ret(ctx, kTypeText); }
DECOMP(00796528, UiElem5_Type) { ret(ctx, kType5); }

// f32* UiElem_GetMatrix()  [+16]
DECOMP(00761B14, UiElem_GetMatrix) { ret(ctx, arg<u32>(ctx, 0) + kElemMatrix); }

// void UiElem_SetMatrix(const f32 m[16])  [+20]: the xyz of each row; w is left alone.
DECOMP(00761B20, UiElem_SetMatrix) {
    const u32 e = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 3; ++k) Copy32(e + kElemMatrix + row * 16 + k * 4, m + row * 16 + k * 4);
}

// f32* UiElem_GetPosition()  [+24]
DECOMP(00761B84, UiElem_GetPosition) { ret(ctx, arg<u32>(ctx, 0) + kElemPosition); }

// void UiElem_SetPosition(const f32 v[3])  [+28]
DECOMP(00761B90, UiElem_SetPosition) {
    const u32 e = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    for (u32 k = 0; k < 3; ++k) Copy32(e + kElemPosition + k * 4, v + k * 4);
}

// f32* UiElem_GetScale()  [+36]
DECOMP(00761BB4, UiElem_ScalePtr) { ret(ctx, arg<u32>(ctx, 0) + kElemScale); }

// void UiElem_SetScale(const f32 v[3])  [+40]
DECOMP(00761BC0, UiElem_SetScale) {
    const u32 e = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    for (u32 k = 0; k < 3; ++k) Copy32(e + kElemScale + k * 4, v + k * 4);
}

// Vec2 UiElem_GetSize()  [+44]: returned through r3; the base class has no size.
DECOMP(00761BDC, UiElem_SizePtr) {
    const u32 out = arg<u32>(ctx, 0);
    Copy32(out, kZeroVec2);
    Copy32(out + 4, kZeroVec2 + 4);
}

// void UiElem_SetSize(const Vec2&)  [+48]: ignored by the base class
DECOMP(00761BF8, UiElem_SetSize) {}

// Color UiElem_GetColor()  [+52]: returned through r3 (this in r4).
DECOMP(00761BFC, UiElem_GetColor) {
    const u32 out = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    for (u32 k = 0; k < 4; ++k) store<u8>(out + k, load<u8>(e + kElemColor + k));
}

// void UiElem_SetColor(u32 rgba)  [+56]
DECOMP(00761C28, UiElem_SetColor) {
    const u32 e = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    for (u32 k = 0; k < 4; ++k) store<u8>(e + kElemColor + k, static_cast<u8>(c >> (24 - 8 * k)));
}

// Bit-fields of +136.  [+60 .. +136]
DECOMP(00761C50, UiElem_GetFieldA) { Elem_GetField(ctx, kElemFieldA, 22); }
DECOMP(00761C5C, UiElem_SetFieldA) { Elem_SetField(ctx, kElemFieldA, 22); }
DECOMP(00761C6C, UiElem_GetFieldB) { Elem_GetField(ctx, kElemFieldB, 14); }
DECOMP(00761C78, UiElem_SetFieldB) { Elem_SetField(ctx, kElemFieldB, 14); }
DECOMP(00761C88, UiElem_GetFieldC) { Elem_GetField(ctx, kElemFieldC, 6); }
DECOMP(00761C94, UiElem_SetFieldC) { Elem_SetField(ctx, kElemFieldC, 6); }
DECOMP(00761CA4, UiElem_IsActive) { Elem_GetFlag(ctx, kElemActive); }
DECOMP(00761CBC, UiElem_SetActive) { Elem_SetFlag(ctx, kElemActive, 31); }
DECOMP(00761CD0, UiElem_IsFocusable) { Elem_GetFlag(ctx, kElemFocusable); }
DECOMP(00761CE8, UiElem_SetFocusable) { Elem_SetFlag(ctx, kElemFocusable, 30); }
DECOMP(00761CF8, UiElem_GetFlag29) { Elem_GetFlag(ctx, kElemFlag29); }
DECOMP(00761D10, UiElem_SetFlag29) { Elem_SetFlag(ctx, kElemFlag29, 29); }
DECOMP(00761D20, UiElem_GetFlag28) { Elem_GetFlag(ctx, kElemFlag28); }
DECOMP(00761D38, UiElem_SetFlag28) { Elem_SetFlag(ctx, kElemFlag28, 28); }
DECOMP(00761D48, UiElem_GetFlag27) { Elem_GetFlag(ctx, kElemFlag27); }
DECOMP(00761D60, UiElem_SetFlag27) { Elem_SetFlag(ctx, kElemFlag27, 27); }
DECOMP(00761D70, UiElem_GetFlag26) { Elem_GetFlag(ctx, kElemFlag26); }
DECOMP(00761D88, UiElem_SetFlag26) { Elem_SetFlag(ctx, kElemFlag26, 26); }
DECOMP(00761D98, UiElem_GetFlag05) { Elem_GetFlag(ctx, kElemFlag05); }
DECOMP(00761DB0, UiElem_SetFlag05) { Elem_SetFlag(ctx, kElemFlag05, 5); }

// The two draw callbacks Ui_Init gives the GUI render target: empty.
DECOMP(004D5870, Ui_DrawCallback0) {}
DECOMP(004D5874, Ui_DrawCallback1) {}
