// Two small vector helpers the audio code uses.
#include <cmath>

#include "audio.h"

namespace sly4::audio {
using namespace opentit::native;

// pos - l->position rotated into the listener's frame (rows 0-2 of its matrix).
void Listener_LocalDirection(be_f32* out, const be_f32* pos, const be_f32* l) {
    const be_f32* lp = l + 12;
    const f32 dy = fsubs(pos[1], lp[1]);
    const f32 dx = fsubs(pos[0], lp[0]);
    const f32 dz = fsubs(pos[2], lp[2]);
    out[0] = fmadds(dz, l[2], fmadds(dx, l[0], fmuls(dy, l[1])));
    out[1] = fmadds(dz, l[6], fmadds(dx, l[4], fmuls(dy, l[5])));
    out[2] = fmadds(dz, l[10], fmadds(dx, l[8], fmuls(dy, l[9])));
}

// Compares the x, y, z of the four rows (w ignored).
bool Matrix_NearlyEqual(const be_f32* a, const be_f32* b, f32 eps) {
    for (const u32 k : {0u, 1u, 2u, 4u, 5u, 6u, 8u, 9u, 10u, 12u, 13u, 14u})
        if (!(std::fabs(fsubs(a[k], b[k])) < eps)) return false;
    return true;
}

}  // namespace sly4::audio

NATIVE(005532E4, sly4::audio::Listener_LocalDirection)
NATIVE(005545B4, sly4::audio::Matrix_NearlyEqual)
