// The current view and small matrix helpers (Mat4_Ortho, Matrix_ScaleRows).
//
// View (View_GetCurrent): +60 f32 near, +64 f32 far, +96 u16 x, y, w, h
// viewport, +752 f32[3][4] rotation rows, +1008 f32[16] matrix.
#pragma once

#include "matrix.h"

namespace sly4 {
namespace view {

namespace v2fn {
constexpr u32 View_GetCurrent = 0x004FDBA8;   // decompiled: src/view/view2.cpp
constexpr u32 Mat4_Ortho = 0x0056A358;        // decompiled: src/view/view2.cpp
constexpr u32 Matrix_ScaleRows = 0x00554394;  // decompiled: src/view/view2.cpp
}  // namespace v2fn

namespace v2g {
constexpr u32 current_surface = 0x010DDA74;  // u32: object whose +672 is the current View (draw::g::current_surface)
constexpr u32 ortho_consts = 0x0056A34C;     // f32 { 2.0, 1.0 }
}  // namespace v2g

void* View_GetCurrent();
void Mat4_Ortho(be_f32* out, f64 l, f64 r, f64 b, f64 t, f64 n, f64 f);
void Matrix_ScaleRows(be_f32* m, const be_f32* scale);

}  // namespace view
}  // namespace sly4
