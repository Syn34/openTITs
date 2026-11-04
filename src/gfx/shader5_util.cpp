// The shaders' last helpers: the animated UV matrix, the perspective
// projection and the projector of ProjTxr, SelfLight's light and the view
// constants block.
#include "shader5.h"

using namespace sly4;

namespace {

namespace s3 = sly4::render::shader3;
namespace s4 = sly4::render::shader4;
namespace s5 = sly4::render::shader5;
namespace sp = sly4::render::pool;
namespace d3 = sly4::render::draw3;
namespace s2 = sly4::render::shader2;

// fctidz / fcfid / frsp: x truncated toward zero through a 64-bit integer.
f64 trunc_s(f64 x) { return sp::sgl(rt::fcfid(rt::fctid(x, true))); }

// fmuls / fctidz / fcfid / frsp / fdivs: x snapped to 1 / steps.
f64 snap(f64 x, f64 steps) { return sp::sgl(trunc_s(sp::sgl(x * steps)) / steps); }

// A vector register's four lanes.
struct V4 {
    f32 v[4];
};
V4 lvx(u32 ea) {
    ea &= ~15u;
    return {{load<f32>(ea), load<f32>(ea + 4), load<f32>(ea + 8), load<f32>(ea + 12)}};
}
void stvx(u32 ea, const V4& x) {
    ea &= ~15u;
    for (u32 k = 0; k < 4; ++k) store<f32>(ea + 4 * k, x.v[k]);
}
// vmaddfp: a * c + b per lane.
V4 madd(const V4& a, f32 c, const V4& b) {
    V4 r;
    for (u32 k = 0; k < 4; ++k) r.v[k] = std::fmaf(a.v[k], c, b.v[k]);
    return r;
}
V4 add(const V4& a, const V4& b) {
    V4 r;
    for (u32 k = 0; k < 4; ++k) r.v[k] = a.v[k] + b.v[k];
    return r;
}
constexpr V4 kZero4{{0.0f, 0.0f, 0.0f, 0.0f}};  // vspltisw 0

// The words at the given offsets from src to dst (lfs / stfs pairs).
void copy_rows(u32 dst, u32 src, std::initializer_list<u32> words) {
    for (u32 off : words) sp::copy_word(dst + off, src + off);
}

}  // namespace

// const f32* MaterialAnim_GetMatrix(MaterialAnim* a)
// The animated UV matrix (g::anim_matrix, rows of 4; the UV offset in row
// 2): offset U = +44 (snapped to 1 / +12 steps when those are set) or,
// with the wrap flag (+4), the fraction of V offset * V steps snapped the
// same way; V = +92 (snapped to 1 / +60 steps). With a rotation (+140
// turns, snapped to 1 / +108 steps) the matrix is translate(-pivot) *
// rotation (Matrix_FromAxisAngle about s3::g::rotation_axis) *
// translate(pivot), pivot +128 / +132, its row 2 then adding (U, V, 1.0);
// without one, the identity with row 2 = (U, V, 1).
DECOMP(0050B980, MaterialAnim_GetMatrix) {
    constexpr u32 k = 0x0050B970;  // .text: f32 0.0, 2 pi, 1.0
    const u32 a = arg<u32>(ctx, 0);
    const f64 zero = sp::lfs(k);
    const f64 u_steps = sp::lfs(a + 12), v_steps = sp::lfs(a + 60);
    f64 u = u_steps != zero ? snap(sp::lfs(a + 44), u_steps) : sp::lfs(a + 44);  // fcmpu, bne
    const f64 v_off = sp::lfs(a + 92);
    const f64 v = v_steps != zero ? snap(v_off, v_steps) : v_off;
    if (ref<u8>(a + 4) != 0) {  // cmpwi
        const f64 frames = sp::sgl(v_off * v_steps);
        u = snap(sp::sgl(frames - trunc_s(frames)), u_steps);
    }
    const f64 r_steps = sp::lfs(a + 108);
    const f64 turns = r_steps != zero ? snap(sp::lfs(a + 140), r_steps) : sp::lfs(a + 140);
    const u32 m = s5::g::anim_matrix, id = s5::g::anim_identity;
    if (turns == zero) {
        copy_rows(m, id, {0, 4, 8, 16, 20, 24, 48, 52, 56});
        sp::stfs(m + 32, u);
        sp::stfs(m + 36, v);
        store<u32>(m + 40, s2::kOne);
        ret(ctx, m);
        return;
    }
    callf(ctx, s3::fn::Matrix_FromAxisAngle, {m, s3::g::rotation_axis}, {sp::sgl(turns * sp::lfs(k + 4))});
    const u32 pu = load<u32>(a + 128), pv = load<u32>(a + 132);
    const u32 out = s5::g::anim_pivot_out, in = s5::g::anim_pivot_in;
    if (ref<u8>(s5::g::anim_pivot_out_set) == 0) {
        copy_rows(out, id, {0, 4, 8, 16, 20, 24, 32, 36, 40, 48, 52, 56});
        store<u8>(s5::g::anim_pivot_out_set, 1);
    }
    store<u32>(out + 32, pu ^ 0x80000000u);  // fneg
    store<u32>(out + 36, pv ^ 0x80000000u);
    store<u32>(out + 40, s2::kOne);
    if (ref<u8>(s5::g::anim_pivot_in_set) == 0) {
        copy_rows(in, id, {0, 4, 8, 16, 20, 24, 48, 52, 56});
        store<u8>(s5::g::anim_pivot_in_set, 1);
    }
    store<u32>(in + 32, pu);
    store<u32>(in + 36, pv);
    store<u32>(in + 40, s2::kOne);
    // C = R * in (row 3 adding in's row 3), D = out * C (row 3 adding C's
    // row 3), in vmaddfp order.
    V4 r[4], b[4], c[4], o[4];
    for (u32 i = 0; i < 4; ++i) {
        r[i] = lvx(m + 16 * i);
        b[i] = lvx(in + 16 * i);
        o[i] = lvx(out + 16 * i);
    }
    for (u32 i = 0; i < 4; ++i) c[i] = madd(b[2], r[i].v[2], madd(b[1], r[i].v[1], madd(b[0], r[i].v[0], kZero4)));
    c[3] = add(b[3], c[3]);
    V4 d[4];
    for (u32 i = 0; i < 4; ++i) d[i] = madd(c[2], o[i].v[2], madd(c[1], o[i].v[1], madd(c[0], o[i].v[0], kZero4)));
    d[3] = add(c[3], d[3]);
    for (u32 i = 0; i < 4; ++i) stvx(m + 16 * i, d[i]);
    sp::stfs(m + 32, sp::sgl(sp::lfs(m + 32) + u));
    sp::stfs(m + 36, sp::sgl(sp::lfs(m + 36) + v));
    sp::stfs(m + 40, sp::sgl(sp::lfs(m + 40) + sp::lfs(k + 8)));  // + 1.0, as the original
    ret(ctx, m);
}

// void Matrix_Perspective(f32 out[16]; f1 fov, f2 aspect, f3 near, f4 far)
// c = 1 / tan(fov * 0.5): out = {c / aspect, 0, 0, 0 | 0, c, 0, 0 | 0, 0,
// far / (near - far), -1 | 0, 0, far * near / (near - far), 0}; returns c
// in f1 (r3 0).
DECOMP(0056A200, Matrix_Perspective) {
    constexpr u32 k = 0x0056A1F8;  // .text: f32 0.5, 1.0
    const u32 out = arg<u32>(ctx, 0);
    const f64 fov = farg(ctx, 0), aspect = farg(ctx, 1), near = farg(ctx, 2), far = farg(ctx, 3);
    const f64 t = callf(ctx, s5::fn::tanf, {}, {sp::sgl(fov * sp::lfs(k))});
    const f64 c = sp::sgl(sp::lfs(k + 4) / t);
    for (u32 off : {4u, 8u, 12u, 16u, 24u, 28u, 32u, 36u}) store<u32>(out + off, 0);
    store<u32>(out + 44, 0xBF800000u);  // -1.0
    for (u32 off : {48u, 52u, 60u}) store<u32>(out + off, 0);
    const f64 depth = sp::sgl(near - far), product = sp::sgl(far * near);
    sp::stfs(out + 20, c);
    sp::stfs(out + 40, sp::sgl(far / depth));
    sp::stfs(out + 56, sp::sgl(product / depth));
    sp::stfs(out + 0, sp::sgl(c / aspect));
    ctx.gpr[3] = 0;  // as the original leaves it
    ret(ctx, c);
}

// void ProjTxr_BuildProjection(f32 pos_out[4], f32 m_out[16], const f32
// eye[3], target[3], up[3], const f32 proj[16], DrawItem* item, const f32
// block[16])
// The projector: L = Matrix_LookAt(eye, target, up); m_out = transpose(
// item rows (w 0, 0, 0, 1) * (L rows (w 0, 0, 0, 1) * (proj * block)))
// (Matrix_MultiplyTo); pos_out (16 bytes, stvx) = the item's 3x3 columns
// through L's column 2 (vmaddfp order; lane 3 junk, as the original).
DECOMP(005B0CF0, ProjTxr_BuildProjection) {
    constexpr u32 kMasks = 0x005B0CD0;  // .text: two vperm masks (read as the lanes below)
    (void)kMasks;
    const u32 pos_out = arg<u32>(ctx, 0), m_out = arg<u32>(ctx, 1);
    const u32 eye = arg<u32>(ctx, 2), target = arg<u32>(ctx, 3), up = arg<u32>(ctx, 4);
    const u32 proj = arg<u32>(ctx, 5), item = arg<u32>(ctx, 6), block = arg<u32>(ctx, 7);
    // The original's sp+112 (L), +176 (L, w), +240 (item, w), +304 / +368 /
    // +432 (products), +528 (L's column 2).
    Stack frame(ctx, 432);
    const u32 look = frame.addr(), lw = look + 64, iw = look + 128, p0 = look + 192, p1 = look + 256, p2 = look + 320, col = look + 416;
    call(ctx, s5::fn::Matrix_LookAt, {look, eye, target, up});
    for (u32 i = 0; i < 4; ++i) {
        copy_rows(lw + 16 * i, look + 16 * i, {0, 4, 8});
        store<u32>(lw + 16 * i + 12, i == 3 ? s2::kOne : 0u);
    }
    for (u32 i = 0; i < 4; ++i) {
        copy_rows(iw + 16 * i, item + 16 * i, {0, 4, 8});
        store<u32>(iw + 16 * i + 12, i == 3 ? s2::kOne : 0u);
    }
    call(ctx, s5::fn::Matrix_MultiplyTo, {p0, proj, block});
    call(ctx, s5::fn::Matrix_MultiplyTo, {p1, lw, p0});
    call(ctx, s5::fn::Matrix_MultiplyTo, {p2, iw, p1});
    {
        V4 r[4];
        for (u32 i = 0; i < 4; ++i) r[i] = lvx(p2 + 16 * i);
        for (u32 j = 0; j < 4; ++j) stvx(m_out + 16 * j, V4{{r[0].v[j], r[1].v[j], r[2].v[j], r[3].v[j]}});
    }
    const V4 l0 = lvx(look), l1 = lvx(look + 16), l2 = lvx(look + 32);
    stvx(col, V4{{l0.v[2], l1.v[2], l2.v[2], l0.v[2]}});  // vmrglw, vperm
    const V4 i0 = lvx(item), i1 = lvx(item + 16), i2 = lvx(item + 32);
    const V4 cx{{i0.v[0], i1.v[0], i2.v[0], i1.v[1]}};  // vmrghw, vmrghw
    const V4 cy{{i0.v[1], i1.v[1], i2.v[1], i0.v[0]}};  // vmrghw, vperm
    const V4 cz{{i0.v[2], i1.v[2], i2.v[2], i0.v[2]}};  // vmrglw, vperm
    const f32 t0 = load<f32>(col), t1 = load<f32>(col + 4), t2 = load<f32>(col + 8);  // lvlx, vspltw 0
    stvx(pos_out, madd(cz, t2, madd(cy, t1, madd(cx, t0, kZero4))));
    ctx.gpr[3] = 532;  // as the original leaves it
}

// void SelfLight_GetLight(Shader*, DrawItem* item, f32 a[4], f32 dir[4], f32 b[4])
// The first time a material with parameters (+124, word 0 nonzero) comes
// by, word 0 is looked up in the light map (Map_Find in g::light_map
// +16), the result's first word goes to parameter word 4 and word 0 is
// cleared (Material_ParamSlot for both). Then a = {1, 1, 1, 1}, dir =
// {0, 0, 0} and b = {1, 0, 0, 1}.
DECOMP(005B47F4, SelfLight_GetLight) {
    const u32 item = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), dir = arg<u32>(ctx, 3), b = arg<u32>(ctx, 4);
    const u32 mat = load<u32>(item + d3::kItemMaterial);
    const u32 params = load<u32>(mat + d3::kMatParams);
    if (params != 0) {
        if (const u32 key = load<u32>(params); key != 0) {
            store<u32>(static_cast<u32>(call(ctx, s5::fn::Material_ParamSlot, {mat, 0})), 0);
            Stack frame(ctx, 16);  // the original's sp+112
            store<u32>(frame.addr(), key);
            const u32 found = static_cast<u32>(call(ctx, s5::fn::Map_Find, {load<u32>(load<u32>(s5::g::light_map) + 16), frame.addr()}));
            const u32 light = load<u32>(found);
            store<u32>(static_cast<u32>(call(ctx, s5::fn::Material_ParamSlot, {mat, 4})), light);
        }
    }
    for (u32 i = 0; i < 16; i += 4) store<u32>(a + i, s2::kOne);
    for (u32 i = 0; i < 12; i += 4) store<u32>(dir + i, 0);
    store<u32>(b, s2::kOne);
    store<u32>(b + 4, 0);
    store<u32>(b + 8, 0);
    store<u32>(b + 12, s2::kOne);
    ctx.gpr[3] = s2::kOne;  // as the original leaves it
}

// void View_SetConstants(View* view)
// The view constants block (shader5.h) from the view.
DECOMP(005207D0, View_SetConstants) {
    constexpr u32 k = 0x005207A0;  // .text: two vperm masks, f32 0.5, 1.0, k (tan reference angle)
    const u32 view = arg<u32>(ctx, 0);
    const u32 b = s5::g::view_block;
    for (u32 i = 0; i < 64; i += 4) sp::copy_word(b + i, view + 1008 + i);
    for (u32 i = 0; i < 128; i += 4) sp::copy_word(b + 64 + i, view + 880 + i);
    for (u32 r = 0; r < 4; ++r) copy_rows(b + 192 + 16 * r, view + 752 + 16 * r, {0, 4, 8});
    {
        const V4 p = lvx(view + 800), ra = lvx(view + 752), rb = lvx(view + 768), rc = lvx(view + 784);
        V4 neg;
        for (u32 i = 0; i < 4; ++i) neg.v[i] = 0.0f - p.v[i];  // vsubfp from 0
        const V4 c0{{ra.v[0], rb.v[0], rc.v[0], rb.v[1]}};  // vmrghw, vmrghw
        const V4 c1{{ra.v[1], rb.v[1], rc.v[1], ra.v[0]}};  // vmrghw, vperm
        const V4 c2{{ra.v[2], rb.v[2], rc.v[2], ra.v[2]}};  // vmrglw, vperm
        stvx(b + 256, c0);
        stvx(b + 272, c1);
        stvx(b + 288, c2);
        stvx(b + 304, madd(c2, neg.v[2], madd(c1, neg.v[1], madd(c0, neg.v[0], kZero4))));
    }
    for (u32 r = 0; r < 4; ++r) copy_rows(b + 320 + 16 * r, view + 1200 + 16 * r, {0, 4, 8});
    for (u32 r = 0; r < 8; ++r) copy_rows(b + 384 + 16 * r, view + 1072 + 16 * r, {0, 4, 8});
    for (u32 i = 0; i < 2; ++i) {
        copy_rows(b + 512 + 16 * i, view + 144 + 64 * i, {0, 4, 8, 12});
        copy_rows(b + 544 + 32 * i, view + 160 + 64 * i, {0, 4, 8, 12});
        copy_rows(b + 560 + 32 * i, view + 176 + 64 * i, {0, 4, 8, 12});
    }
    if (ref<u8>(s5::g::fov_ref_set) == 0) {
        sp::stfs(s5::g::fov_ref, callf(ctx, s5::fn::tanf, {}, {sp::lfs(k + 40)}));
        store<u8>(s5::g::fov_ref_set, 1);
    }
    const f64 t = callf(ctx, s5::fn::tanf, {}, {sp::sgl(sp::lfs(view + 56) * sp::lfs(k + 32))});
    const f64 ratio = sp::sgl(t / sp::lfs(s5::g::fov_ref));
    const f64 one = sp::lfs(k + 36);
    sp::stfs(b + 612, sp::sel_ge(sp::sgl(ratio - one), one, ratio));  // fsel: min(ratio, 1)
    const f64 scale = sp::lfs(sly4::g::config + 320);
    for (u32 i = 0; i < 6; ++i) {
        for (u32 j = 0; j < 16; j += 4) sp::stfs(b + 624 + 16 * i + j, sp::sgl(sp::lfs(s5::g::config_vec4s + 16 * i + j) * scale));
    }
    store<u32>(b + 720, 0);
    store<u32>(b + 724, s2::kOne);
    store<u32>(b + 728, 0);
    store<u32>(b + 732, 0);
    const u32 draws = load<u32>(draw::g::current_surface + 4);
    store<u32>(b + 616, draws);
    ret(ctx, draws);  // as the original leaves r3
}
