// Element placement used by the text code: the inherited scale (vfunc
// +168), the element's rectangle and world matrix (vfunc +160) and the
// projection of a point to the screen.
#pragma once

#include "text.h"

namespace sly4 {
namespace text {

namespace gfn {
constexpr u32 UiElem_GetScale = 0x004D7E70;       // vfunc +168: sret f32[3]
constexpr u32 UiElem_GetRect = 0x004E0648;        // vfunc +160: (a out f32[2], b out f32[2], m out f32[16])
constexpr u32 View_ProjectPoint = 0x00522750;     // (View*, f32 out[4], const f32 in[3])
// still recompiled
constexpr u32 View_Update = 0x005115F0;           // (View*, 0): 615 instructions (name?)
constexpr u32 Vec_TransformByMatrix = 0x00553A2C; // (f32 out[4], const f32 in[3], const Matrix*) (name?)
constexpr u32 Matrix_Multiply = 0x005541F0;       // (Matrix* a_and_out, const Matrix* b): VMX (name?)
}  // namespace gfn

// Element virtual functions used here (byte offsets into the vtable).
constexpr u32 kVfnSize = 140;        // sret f32[2]: the element's size (name?)
constexpr u32 kVfnWorldMatrix = 164; // sret f32[16]
constexpr u32 kVfnScreenSpace = 132; // () -> bool: flag 0x20 of +136 (UiElem_GetFlag05)
constexpr u32 kElemScale = 80;       // f32[3]
constexpr u32 kElemInheritScale = 0x10000000;  // +136: multiply by the parent's scale

constexpr u32 kScreenView = 0x00F66850;    // View*: set when there is a 3D view to project through (name?)
constexpr u32 kProjectInit = 0x01164BC0;   // f32[4]: initial contents of the projection outputs
constexpr u32 kScreenMatrix = 0x01164BD0;  // f32[16]: rows' xyz copied (update.h kViewMatrix)
constexpr u32 kViewProjMatrix = 1328;      // View +1328: the matrix points are transformed by

// Copy the xyz of the four rows of a 4x4 float matrix (raw words; w is left alone).
inline void Matrix_CopyXyz(be_f32* dst, const be_f32* src) {
    be_u32* d = reinterpret_cast<be_u32*>(dst);
    const be_u32* s = reinterpret_cast<const be_u32*>(src);
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 3; ++k) d[row * 4 + k] = s[row * 4 + k].get();
}


}  // namespace text
}  // namespace sly4
