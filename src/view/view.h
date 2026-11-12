// Vector and view helpers.
#pragma once

#include "opentit/native.h"
#include "../sly4.h"

namespace sly4 {
namespace view {

namespace vfn {
constexpr u32 Vec4_TransformByMatrix = 0x00553910;  // (f32 out[4], const f32 in[4], const Matrix*): VMX (name?)
}  // namespace vfn

be_f32* Vec_TransformByMatrix(be_f32* out, const be_f32* in, const void* m);

}  // namespace view
}  // namespace sly4
