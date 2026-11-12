// Vec_TransformByMatrix (0x00553A2C).
#include "view.h"

namespace sly4::view {
using namespace opentit::native;

// Transforms the point (in, 1) by m; returns out.
be_f32* Vec_TransformByMatrix(be_f32* out, const be_f32* in, const void* m) {
    GuestFrame frame(16);
    be_u32* p = frame.as<be_u32>();
    const be_u32* src = reinterpret_cast<const be_u32*>(in);
    for (u32 k = 0; k < 3; ++k) p[k] = src[k].get();
    p[3] = 0x3F800000u;  // 1.0f
    call(vfn::Vec4_TransformByMatrix, out, p, m);
    return out;
}

}  // namespace sly4::view

NATIVE(00553A2C, sly4::view::Vec_TransformByMatrix)
