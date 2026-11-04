// Vec_TransformByMatrix (0x00553A2C).
#include "view.h"

using namespace sly4;
using namespace sly4::view;

namespace {
constexpr u32 kOne = 0x3F800000;  // 1.0f
}

// f32* Vec_TransformByMatrix(f32 out[4], const f32 in[3], const Matrix* m)
// Transforms the point (in, 1) by m; returns out.
DECOMP(00553A2C, Vec_TransformByMatrix) {
    const u32 out = arg<u32>(ctx, 0);
    const u32 in = arg<u32>(ctx, 1);
    const u32 m = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);
    const u32 p = frame.addr();
    for (u32 k = 0; k < 3; ++k) store<u32>(p + k * 4, load<u32>(in + k * 4));
    store<u32>(p + 12, kOne);
    call(ctx, vfn::Vec4_TransformByMatrix, {out, p, m});
    ret(ctx, out);
}
