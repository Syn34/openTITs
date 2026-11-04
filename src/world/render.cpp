// Render helpers called by the World_SubmitFrame steps.
#include <cmath>

#include "world.h"

using namespace sly4;
using namespace sly4::world;

namespace {
inline f32 fsubs(f64 a, f64 b) { return static_cast<f32>(a - b); }
inline f32 fdivs(f64 a, f64 b) { return static_cast<f32>(a / b); }
inline f32 fsqrts(f64 a) { return static_cast<f32>(std::sqrt(a)); }
inline f32 fmadds(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, b)); }
inline f32 fmsubs(f64 a, f64 c, f64 b) { return static_cast<f32>(std::fma(a, c, -b)); }
inline f32 fnmadds(f64 a, f64 c, f64 b) { return static_cast<f32>(-std::fma(a, c, b)); }
constexpr u32 kOne = 0x0056A0CC;  // f32 1.0 just before Matrix_LookAt
}  // namespace

// void Arena_Restore(Arena* arena, Mark* mark)  (name?)
// mark = { arena+12, 0, 0 }; arena+32 |= 0x40.
DECOMP(00565154, Arena_Restore) {
    const u32 arena = arg<u32>(ctx, 0), mark = arg<u32>(ctx, 1);
    const u32 top = field<u32>(arena, 12);
    field<u32>(mark, 4) = 0;
    field<u32>(mark, 0) = top;
    field<u32>(mark, 8) = 0;
    field<u8>(arena, 32) = static_cast<u8>(field<u8>(arena, 32) | 0x40);
}

// void Matrix_LookAt(f32 out[16], const f32 eye[3], const f32 target[3], const f32 up[3])  (name?)
// A view matrix: rows of the camera axes in columns, translation in row 3.
// The w slots (+12, +28, +44, +60) are not written.
DECOMP(0056A0D0, Matrix_LookAt) {
    const u32 out = arg<u32>(ctx, 0), eye = arg<u32>(ctx, 1), tgt = arg<u32>(ctx, 2), up = arg<u32>(ctx, 3);
    auto f = [](u32 ea) { return static_cast<f64>(load<f32>(ea)); };
    const f32 one = load<f32>(kOne);

    const f32 dy = fsubs(f(eye + 4), f(tgt + 4));
    const f32 dx = fsubs(f(eye), f(tgt));
    const f32 dz = fsubs(f(eye + 8), f(tgt + 8));
    const f32 len = fsqrts(fmadds(dz, dz, fmadds(dx, dx, static_cast<f32>(static_cast<f64>(dy) * dy))));
    const f32 inv = fdivs(one, len);
    const f32 zz = static_cast<f32>(static_cast<f64>(dz) * inv);
    const f32 zx = static_cast<f32>(static_cast<f64>(dx) * inv);
    const f32 zy = static_cast<f32>(static_cast<f64>(dy) * inv);
    const f64 ux = f(up), uy = f(up + 4), uz = f(up + 8);

    const f32 t4 = static_cast<f32>(ux * zz), t10 = static_cast<f32>(uy * zx), t9 = static_cast<f32>(uz * zy);
    f32 ay = fmsubs(uz, zx, t4);
    f32 az = fmsubs(ux, zy, t10);
    f32 ax = fmsubs(uy, zz, t9);
    store<f32>(out + 8, zx);
    const f32 alen = fsqrts(fmadds(az, az, fmadds(ax, ax, static_cast<f32>(static_cast<f64>(ay) * ay))));
    const f32 ainv = fdivs(one, alen);
    ay = static_cast<f32>(static_cast<f64>(ay) * ainv);
    ax = static_cast<f32>(static_cast<f64>(ax) * ainv);
    az = static_cast<f32>(static_cast<f64>(az) * ainv);

    const f32 s6 = static_cast<f32>(static_cast<f64>(zz) * ay);
    const f32 s8 = static_cast<f32>(static_cast<f64>(zy) * ax);
    const f32 s9 = static_cast<f32>(static_cast<f64>(zx) * az);
    store<f32>(out + 0, ax);
    const f32 by = fmsubs(zy, az, s6);
    const f32 bz = fmsubs(zx, ay, s8);
    const f32 bw = fmsubs(zz, ax, s9);
    store<f32>(out + 4, by);
    store<f32>(out + 16, ay);
    store<f32>(out + 20, bw);
    store<f32>(out + 24, zy);
    store<f32>(out + 32, az);
    store<f32>(out + 36, bz);
    store<f32>(out + 40, zz);

    const f64 ey = f(eye + 4), ex = f(eye), ez = f(eye + 8);
    const f32 p4 = static_cast<f32>(static_cast<f64>(ay) * ey);
    const f32 p9 = static_cast<f32>(static_cast<f64>(bw) * ey);
    const f32 p1 = static_cast<f32>(static_cast<f64>(zy) * ey);
    const f32 q4 = fmadds(ax, ex, p4);
    const f32 q6 = fmadds(by, ex, p9);
    const f32 q1 = fmadds(zx, ex, p1);
    store<f32>(out + 48, fnmadds(az, ez, q4));
    store<f32>(out + 52, fnmadds(bz, ez, q6));
    store<f32>(out + 56, fnmadds(zz, ez, q1));
}
