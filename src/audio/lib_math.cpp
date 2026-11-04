// Two small vector helpers the audio code uses.
#include <cmath>

#include "snd.h"

using namespace sly4;
using namespace sly4::snd;

namespace {
inline f32 fmadds(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }
f64 lf(u32 ea) { return static_cast<f64>(load<f32>(ea)); }
}  // namespace

// void Listener_LocalDirection(f32 out[3], const f32 pos[3], const Listener* l)
// pos - l->position rotated into the listener's frame (rows 0-2 of its matrix).
DECOMP(005532E4, Listener_LocalDirection) {
    const u32 out = arg<u32>(ctx, 0), pos = arg<u32>(ctx, 1), l = arg<u32>(ctx, 2);
    const u32 lp = l + 48;
    const f32 dy = fsubs(lf(pos + 4), lf(lp + 4));
    const f32 dx = fsubs(lf(pos), lf(lp));
    const f32 dz = fsubs(lf(pos + 8), lf(lp + 8));
    store<f32>(out, fmadds(dz, lf(l + 8), fmadds(dx, lf(l), fmuls(dy, lf(l + 4)))));
    store<f32>(out + 4, fmadds(dz, lf(l + 24), fmadds(dx, lf(l + 16), fmuls(dy, lf(l + 20)))));
    store<f32>(out + 8, fmadds(dz, lf(l + 40), fmadds(dx, lf(l + 32), fmuls(dy, lf(l + 36)))));
}

// bool Matrix_NearlyEqual(const f32 a[16], const f32 b[16], f32 eps)
// Compares the x, y, z of the four rows (w ignored).
DECOMP(005545B4, Matrix_NearlyEqual) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const f64 eps = farg(ctx, 0);
    for (const u32 off : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u, 48u, 52u, 56u})
        if (!(std::fabs(fsubs(lf(a + off), lf(b + off))) < eps)) return ret(ctx, 0u);
    ret(ctx, 1u);
}
