// Small renderer math leaves: the matrix helpers the shader Draw slots call
// (Matrix_SplitScale, Matrix_RotationZ) and SlyCharacterFur's shell count.
#include <bit>

#include "gfx7.h"

using namespace sly4;

namespace {

namespace sp = sly4::render::pool;
namespace g7 = sly4::render::gfx7;
namespace l3 = sly4::render::layers3;

}  // namespace

// void Matrix_SplitScale(const f32 m[16], f32 rot[16], f32 scale[3])
// scale[i] = |row i| (xyz) for rows 0..2, each stored before the next row
// is read; rot rows 0..2 = m's rows * (1 / scale[i]) (scale re-read);
// rot row 3 xyz = m's row 3 (raw words). No w lane is written. Leaves f1
// = m[14], as the last lfs does.
DECOMP(00554858, Matrix_SplitScale) {
    const u32 m = arg<u32>(ctx, 0), rot = arg<u32>(ctx, 1), scale = arg<u32>(ctx, 2);
    for (u32 i = 0; i < 3; ++i) {
        const u32 row = m + 16 * i;
        f64 len = sp::sgl(sp::lfs(row + 4) * sp::lfs(row + 4));  // fmuls
        len = sp::fmadds(sp::lfs(row), sp::lfs(row), len);
        len = sp::fmadds(sp::lfs(row + 8), sp::lfs(row + 8), len);
        sp::stfs(scale + 4 * i, sp::sgl(std::sqrt(len)));  // fsqrts
    }
    const f64 one = sp::lfs(g7::kSplitScaleOne);
    for (u32 i = 0; i < 3; ++i) {
        const f64 inv = sp::sgl(one / sp::lfs(scale + 4 * i));  // fdivs
        for (u32 k = 0; k < 3; ++k) sp::stfs(rot + 16 * i + 4 * k, sp::sgl(sp::lfs(m + 16 * i + 4 * k) * inv));
    }
    sp::copy_word(rot + 48, m + 48);
    sp::copy_word(rot + 52, m + 52);
    const f64 last = sp::lfs(m + 56);
    sp::copy_word(rot + 56, m + 56);
    ctx.fpr[1] = last;
}

// f32 Matrix_RotationZ(f32 out[16], f32 angle)
// out = a rotation about z: {c, s, 0, -}, {-s, c, 0, -}, {0, 0, 1, -},
// {0, 0, 0, -} (w lanes not written). The angle is first wrapped to
// [-pi, pi] (x = (a + sign(a) pi) - trunc((a + sign(a) pi) / 2pi) * 2pi -
// sign(a) pi, with sign as fsel: -pi for a < 0 or NaN); then the parabola
// sine y = 4/pi x - 4/pi^2 x |x|, refined to 0.225 (y |y| - y) + y; the
// cosine is the same of x + pi/2 (less 2pi above pi). Returns the cosine
// in f1, and leaves r4 = 0, r5 = 1.0f as the stores do.
DECOMP(00553F18, Matrix_RotationZ) {
    const u32 out = arg<u32>(ctx, 0);
    const f64 a = farg(ctx, 0);
    constexpr u32 k = g7::kRotZConsts;
    const f64 two_pi = sp::lfs(k + 16), four_over_pi = sp::lfs(k + 20), parab = sp::lfs(k + 24);
    const f64 refine = sp::lfs(k + 28), half_pi = sp::lfs(k + 32), pi = sp::lfs(k + 36);

    const f64 bias = sp::sgl(sp::sel_ge(a, load<f64>(k), load<f64>(k + 8)));  // fsel / frsp
    f64 t = sp::sgl(bias + a);
    const f64 turns = sp::sgl(rt::fcfid(rt::fctid(sp::sgl(t / two_pi), true)));  // fdivs, fctidz / fcfid / frsp
    t = sp::fnmsubs(turns, two_pi, t);
    const f64 x = sp::sgl(t - bias);

    // y = 4/pi x + (-4/pi^2 x) |x|; s = 0.225 (y |y| - y) + y
    auto parabola_sine = [&](f64 v) {
        f64 y = sp::sgl(v * parab);
        y = sp::sgl(y * std::fabs(v));
        y = sp::fmadds(v, four_over_pi, y);
        const f64 e = sp::fmsubs(y, std::fabs(y), y);
        return std::pair<f64, f64>{e, y};
    };
    const auto [es, ys] = parabola_sine(x);
    const f64 sine = sp::fmadds(es, refine, ys);
    const f64 neg_sine = sp::sgl(-std::fma(es, refine, ys));  // fnmadds
    f64 c = sp::sgl(x + half_pi);
    if (c > pi) c = sp::sgl(c - two_pi);  // fcmpu, bgt (NaN falls through)
    const auto [ec, yc] = parabola_sine(c);
    const f64 cosine = sp::fmadds(ec, refine, yc);

    sp::stfs(out + 4, sine);
    store<u32>(out + 8, 0);
    store<u32>(out + 24, 0);
    sp::stfs(out + 16, neg_sine);
    store<u32>(out + 32, 0);
    store<u32>(out + 36, 0);
    store<u32>(out + 40, 0x3F800000u);  // 1.0f
    store<u32>(out + 48, 0);
    store<u32>(out + 52, 0);
    store<u32>(out + 56, 0);
    sp::stfs(out + 0, cosine);
    sp::stfs(out + 20, cosine);
    ctx.gpr[4] = 0;            // the zero the stw's use
    ctx.gpr[5] = 0x3F800000u;  // the 1.0f
    ctx.fpr[1] = cosine;
}

// u16 Fur_ShellCount(Shader* s, DrawItem* item, f32* fade)
// The item's view depth d = -(view_matrix applied to the item position
// +48).z (vmaddfp order, row 3 added last); n = the material parameter
// word +76 (low 16 bits; 0 without parameters, material +28 +124). Far
// (d >= 12): *fade = 1, shells = trunc(m - m * clamp((d - 12) / 18, 0, 1))
// with m = max(n - 4, 0). Near (else, NaN too): t = clamp((d - 2) / 2,
// 0, 1), *fade = t / 2 + 1/2, shells = trunc((m - n) t + n). Returns the
// low 16 bits.
DECOMP(005BFB50, Fur_ShellCount) {
    const u32 item = arg<u32>(ctx, 1), fade_out = arg<u32>(ctx, 2);
    const u32 pos = item + 48;
    const f32 px = load<f32>(pos), py = load<f32>(pos + 4), pz = load<f32>(pos + 8);
    const u32 vm = l3::g::view_matrix;
    f32 vz = std::fma(load<f32>(vm + 8), px, 0.0f);
    vz = std::fma(load<f32>(vm + 24), py, vz);
    vz = std::fma(load<f32>(vm + 40), pz, vz);
    vz = load<f32>(vm + 56) + vz;  // vaddfp
    const f64 d = -static_cast<f64>(vz);

    const u32 params = load<u32>(load<u32>(item + 28) + 124);
    const u32 n = params != 0 ? (load<u32>(params + 76) & 0xFFFF) : 0;
    const u32 m = n < 4 ? 0 : ((n - 4) & 0xFFFF);  // cmpwi on the u16
    const f64 mf = sp::sgl(static_cast<f64>(m));

    constexpr u32 k = g7::kFurConsts;
    const f64 near = sp::lfs(k), one_f = sp::lfs(k + 16);
    const f64 one_d = load<f64>(k + 8), zero_d = load<f64>(k + 24);
    f64 shells, fade;
    if (d >= near) {  // cror gt|eq
        const f64 t = sp::sgl(sp::sgl(d - near) / sp::lfs(k + 32));
        fade = one_f;
        const f64 lo = sp::sgl(sp::sel_ge(sp::sgl(one_f - t), t, one_d));  // min(t, 1)
        const f64 c = sp::sgl(sp::sel_ge(lo, lo, zero_d));                // max(., 0)
        shells = sp::sgl(mf + -sp::sgl(mf * c));
    } else {
        const f64 base = sp::lfs(k + 56), half = sp::lfs(k + 60);
        const f64 nf = sp::sgl(static_cast<f64>(n));
        const f64 t = sp::sgl(sp::sgl(d - base) * half);
        const f64 lo = sp::sgl(sp::sel_ge(sp::sgl(one_f - t), t, one_d));
        const f64 c = sp::sgl(sp::sel_ge(lo, lo, zero_d));
        fade = sp::fmadds(c, half, half);
        shells = sp::fmadds(sp::sgl(mf - nf), c, nf);
    }
    const u64 bits = rt::fctid(shells, true);  // fctidz, then the low word
    sp::stfs(fade_out, fade);
    ctx.fpr[1] = std::bit_cast<f64>(bits);  // left in f1, as the original
    ret(ctx, static_cast<u32>(bits) & 0xFFFF);
}
