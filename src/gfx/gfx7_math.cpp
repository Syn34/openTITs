// Small renderer math leaves: the matrix helpers the shader Draw slots call
// (Matrix_SplitScale, Matrix_RotationZ) and SlyCharacterFur's shell count.
#include <bit>

#include "gfx7.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

namespace g7 = sly4::render::gfx7;

constexpr u32 kViewMatrix = 0x010DD840;  // f32[4][4] rows: world -> view (layers3::g::view_matrix)

f64 dbl(u32 ea) { return std::bit_cast<f64>(global<be_u64>(ea).get()); }

}  // namespace

// void Matrix_SplitScale(const f32 m[16], f32 rot[16], f32 scale[3])
// scale[i] = |row i| (xyz) for rows 0..2, each stored before the next row
// is read; rot rows 0..2 = m's rows * (1 / scale[i]) (scale re-read);
// rot row 3 xyz = m's row 3 (raw words). No w lane is written.
void Matrix_SplitScale(const be_f32* m, be_f32* rot, be_f32* scale) {
    for (u32 i = 0; i < 3; ++i) {
        const be_f32* row = at<be_f32>(m, 16 * i);
        f32 len = row[1] * row[1];  // fmuls
        len = fmadds(row[0], row[0], len);
        len = fmadds(row[2], row[2], len);
        *at<be_f32>(scale, 4 * i) = static_cast<f32>(std::sqrt(f64{len}));  // fsqrts
    }
    const f32 one = global<be_f32>(g7::kSplitScaleOne);
    for (u32 i = 0; i < 3; ++i) {
        const f32 inv = one / *at<be_f32>(scale, 4 * i);  // fdivs
        for (u32 k = 0; k < 3; ++k) *at<be_f32>(rot, 16 * i + 4 * k) = *at<be_f32>(m, 16 * i + 4 * k) * inv;
    }
    copy3(at<u8>(rot, 48), at<u8>(m, 48));
}

// void Matrix_RotationZ(f32 out[16], f32 angle)
// out = a rotation about z: {c, s, 0, -}, {-s, c, 0, -}, {0, 0, 1, -},
// {0, 0, 0, -} (w lanes not written). The angle is first wrapped to
// [-pi, pi] (x = (a + sign(a) pi) - trunc((a + sign(a) pi) / 2pi) * 2pi -
// sign(a) pi, with sign as fsel: -pi for a < 0 or NaN); then the parabola
// sine y = 4/pi x - 4/pi^2 x |x|, refined to 0.225 (y |y| - y) + y; the
// cosine is the same of x + pi/2 (less 2pi above pi).
void Matrix_RotationZ(be_f32* out, f32 a) {
    constexpr u32 k = g7::kRotZConsts;
    const f32 two_pi = global<be_f32>(k + 16), four_over_pi = global<be_f32>(k + 20), parab = global<be_f32>(k + 24);
    const f32 refine = global<be_f32>(k + 28), half_pi = global<be_f32>(k + 32), pi = global<be_f32>(k + 36);

    const f32 bias = static_cast<f32>(fsel(a, dbl(k), dbl(k + 8)));  // fsel / frsp
    f32 t = bias + a;
    const f32 turns = static_cast<f32>(static_cast<f64>(fctidz(t / two_pi)));  // fdivs, fctidz / fcfid / frsp
    t = fnmsubs(turns, two_pi, t);
    const f32 x = t - bias;

    // y = 4/pi x + (-4/pi^2 x) |x|; s = 0.225 (y |y| - y) + y
    struct Parabola {
        f32 e, y;
    };
    const auto parabola_sine = [&](f32 v) {
        f32 y = v * parab;
        y = y * std::fabs(v);
        y = fmadds(v, four_over_pi, y);
        return Parabola{fmsubs(y, std::fabs(y), y), y};
    };
    const Parabola s = parabola_sine(x);
    const f32 sine = fmadds(s.e, refine, s.y);
    const f32 neg_sine = fnmadds(s.e, refine, s.y);
    f32 c = x + half_pi;
    if (c > pi) c = c - two_pi;  // fcmpu, bgt (NaN falls through)
    const Parabola cs = parabola_sine(c);
    const f32 cosine = fmadds(cs.e, refine, cs.y);

    be_u32* w = reinterpret_cast<be_u32*>(out);
    out[1] = sine;
    w[2] = 0u;
    w[6] = 0u;
    out[4] = neg_sine;
    w[8] = 0u;
    w[9] = 0u;
    w[10] = 0x3F800000u;  // 1.0f
    w[12] = 0u;
    w[13] = 0u;
    w[14] = 0u;
    out[0] = cosine;
    out[5] = cosine;
}

// u16 Fur_ShellCount(Shader* s, DrawItem* item, f32* fade)
// The item's view depth d = -(view_matrix applied to the item position
// +48).z (vmaddfp order, row 3 added last); n = the material parameter
// word +76 (low 16 bits; 0 without parameters, material +28 +124). Far
// (d >= 12): *fade = 1, shells = trunc(m - m * clamp((d - 12) / 18, 0, 1))
// with m = max(n - 4, 0). Near (else, NaN too): t = clamp((d - 2) / 2,
// 0, 1), *fade = t / 2 + 1/2, shells = trunc((m - n) t + n). Returns the
// low 16 bits.
u32 Fur_ShellCount(ShaderPrograms*, DrawItem* item, be_f32* fade_out) {
    const f32 px = item->pos[0], py = item->pos[1], pz = item->pos[2];
    const be_f32* vm = host<be_f32>(kViewMatrix);
    f32 vz = std::fmaf(vm[2], px, 0.0f);
    vz = std::fmaf(vm[6], py, vz);
    vz = std::fmaf(vm[10], pz, vz);
    vz = vm[14] + vz;  // vaddfp
    const f32 d = -vz;

    const u32 params = or_zero_page(item->mat.get())->w124;
    const u32 n = params != 0 ? (host<be_u32>(params)[19] & 0xFFFF) : 0;
    const u32 m = n < 4 ? 0 : ((n - 4) & 0xFFFF);  // cmpwi on the u16
    const f32 mf = static_cast<f32>(static_cast<f64>(m));

    constexpr u32 k = g7::kFurConsts;
    const f32 near = global<be_f32>(k), one_f = global<be_f32>(k + 16);
    const f64 one_d = dbl(k + 8), zero_d = dbl(k + 24);
    f32 shells, fade;
    if (d >= near) {  // cror gt|eq
        const f32 t = (d - near) / global<be_f32>(k + 32);
        fade = one_f;
        const f32 lo = static_cast<f32>(fsel(one_f - t, t, one_d));  // min(t, 1)
        const f32 c = static_cast<f32>(fsel(lo, lo, zero_d));         // max(., 0)
        shells = mf + -(mf * c);
    } else {
        const f32 base = global<be_f32>(k + 56), half = global<be_f32>(k + 60);
        const f32 nf = static_cast<f32>(static_cast<f64>(n));
        const f32 t = (d - base) * half;
        const f32 lo = static_cast<f32>(fsel(one_f - t, t, one_d));
        const f32 c = static_cast<f32>(fsel(lo, lo, zero_d));
        fade = fmadds(c, half, half);
        shells = fmadds(mf - nf, c, nf);
    }
    const u64 word = static_cast<u64>(fctidz(shells));  // fctidz, then the low word
    *fade_out = fade;
    return static_cast<u32>(word) & 0xFFFF;
}

}  // namespace sly4::gfx

NATIVE(00554858, sly4::gfx::Matrix_SplitScale)
NATIVE(00553F18, sly4::gfx::Matrix_RotationZ)
NATIVE(005BFB50, sly4::gfx::Fur_ShellCount)
