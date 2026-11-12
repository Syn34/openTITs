// The shaders' last helpers: the animated UV matrix, the perspective
// projection and the projector of ProjTxr, SelfLight's light and the view
// constants block.
#include "shader5.h"
#include "shader_draw.h"

namespace sly4::gfx {

namespace {

namespace s5 = sly4::render::shader5;
using sdraw::kOne;

// fctidz / fcfid / frsp: x truncated toward zero through a 64-bit integer.
f32 trunc_s(f32 x) { return static_cast<f32>(static_cast<f64>(fctidz(x))); }

// fmuls / fctidz / fcfid / frsp / fdivs: x snapped to 1 / steps.
f32 snap(f32 x, f32 steps) { return trunc_s(x * steps) / steps; }

// A vector register's four lanes.
struct V4 {
    f32 v[4];
};
V4 lvx(const void* p) {
    const be_f32* a = aligned16(p);
    return {{a[0], a[1], a[2], a[3]}};
}
void stvx(void* p, const V4& x) {
    be_f32* a = sdraw::aligned16_out(p);
    for (u32 k = 0; k < 4; ++k) a[k] = x.v[k];
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

// The words at the given byte offsets from src to dst (lfs / stfs pairs).
void copy_rows(void* dst, const void* src, std::initializer_list<u32> words) {
    for (u32 off : words) copy_word(at<u8>(dst, off), at<u8>(src, off));
}

}  // namespace

// The animation block as MaterialAnim_GetMatrix reads it (beyond UVAnim).
struct MatrixAnim {
    UVAnim uv;              // +0
    pad<12> _96;
    be_f32 r_steps;         // +108: rotation steps (0: continuous)
    pad<16> _112;
    be_u32 pivot_u;         // +128: f32
    be_u32 pivot_v;         // +132: f32
    pad<4> _136;
    be_f32 turns;           // +140
};
static_assert(offsetof(MatrixAnim, r_steps) == 108 && offsetof(MatrixAnim, pivot_u) == 128 && offsetof(MatrixAnim, turns) == 140);

// const f32* MaterialAnim_GetMatrix(MaterialAnim* a)
// The animated UV matrix (g::anim_matrix, rows of 4; the UV offset in row
// 2): offset U = +44 (snapped to 1 / +12 steps when those are set) or,
// with the wrap flag (+4), the fraction of V offset * V steps snapped the
// same way; V = +92 (snapped to 1 / +60 steps). With a rotation (+140
// turns, snapped to 1 / +108 steps) the matrix is translate(-pivot) *
// rotation (Matrix_FromAxisAngle about s3::g::rotation_axis) *
// translate(pivot), pivot +128 / +132, its row 2 then adding (U, V, 1.0);
// without one, the identity with row 2 = (U, V, 1).
be_f32* MaterialAnim_GetMatrix(MatrixAnim* a) {
    constexpr u32 k = 0x0050B970;  // .text: f32 0.0, 2 pi, 1.0
    const f32 zero = global<be_f32>(k);
    const f32 u_steps = a->uv.u_steps, v_steps = a->uv.v_steps;
    f32 u = u_steps != zero ? snap(a->uv.u_offset, u_steps) : static_cast<f32>(a->uv.u_offset);  // fcmpu, bne
    const f32 v_off = a->uv.v_offset;
    const f32 v = v_steps != zero ? snap(v_off, v_steps) : v_off;
    if (a->uv.wrap != 0) {  // cmpwi
        const f32 frames = v_off * v_steps;
        u = snap(frames - trunc_s(frames), u_steps);
    }
    const f32 r_steps = a->r_steps;
    const f32 turns = r_steps != zero ? snap(a->turns, r_steps) : static_cast<f32>(a->turns);
    be_f32* m = host<be_f32>(s5::g::anim_matrix);
    const be_f32* id = host<be_f32>(s5::g::anim_identity);
    if (turns == zero) {
        copy_rows(m, id, {0, 4, 8, 16, 20, 24, 48, 52, 56});
        m[8] = u;
        m[9] = v;
        reinterpret_cast<be_u32*>(m)[10] = kOne;
        return m;
    }
    call(sly4::render::shader3::fn::Matrix_FromAxisAngle, m, sly4::render::shader3::g::rotation_axis, turns * global<be_f32>(k + 4));
    const u32 pu = a->pivot_u, pv = a->pivot_v;
    be_u32* out = host<be_u32>(s5::g::anim_pivot_out);
    be_u32* in = host<be_u32>(s5::g::anim_pivot_in);
    if (global<u8>(s5::g::anim_pivot_out_set) == 0) {
        copy_rows(out, id, {0, 4, 8, 16, 20, 24, 32, 36, 40, 48, 52, 56});
        global<u8>(s5::g::anim_pivot_out_set) = 1;
    }
    out[8] = pu ^ 0x80000000u;  // fneg
    out[9] = pv ^ 0x80000000u;
    out[10] = kOne;
    if (global<u8>(s5::g::anim_pivot_in_set) == 0) {
        copy_rows(in, id, {0, 4, 8, 16, 20, 24, 48, 52, 56});
        global<u8>(s5::g::anim_pivot_in_set) = 1;
    }
    in[8] = pu;
    in[9] = pv;
    in[10] = kOne;
    // C = R * in (row 3 adding in's row 3), D = out * C (row 3 adding C's
    // row 3), in vmaddfp order.
    V4 r[4], b[4], c[4], o[4];
    for (u32 i = 0; i < 4; ++i) {
        r[i] = lvx(m + 4 * i);
        b[i] = lvx(in + 4 * i);
        o[i] = lvx(out + 4 * i);
    }
    for (u32 i = 0; i < 4; ++i) c[i] = madd(b[2], r[i].v[2], madd(b[1], r[i].v[1], madd(b[0], r[i].v[0], kZero4)));
    c[3] = add(b[3], c[3]);
    V4 d[4];
    for (u32 i = 0; i < 4; ++i) d[i] = madd(c[2], o[i].v[2], madd(c[1], o[i].v[1], madd(c[0], o[i].v[0], kZero4)));
    d[3] = add(c[3], d[3]);
    for (u32 i = 0; i < 4; ++i) stvx(m + 4 * i, d[i]);
    m[8] = m[8] + u;
    m[9] = m[9] + v;
    m[10] = m[10] + global<be_f32>(k + 8);  // + 1.0, as the original
    return m;
}

// void Matrix_Perspective(f32 out[16]; f1 fov, f2 aspect, f3 near, f4 far)
// c = 1 / tan(fov * 0.5): out = {c / aspect, 0, 0, 0 | 0, c, 0, 0 | 0, 0,
// far / (near - far), -1 | 0, 0, far * near / (near - far), 0}.
void Matrix_Perspective(be_f32* out, f32 fov, f32 aspect, f32 near, f32 far) {
    constexpr u32 k = 0x0056A1F8;  // .text: f32 0.5, 1.0
    const f64 t = call<f64>(s5::fn::tanf, fov * global<be_f32>(k));
    const f32 c = static_cast<f32>(static_cast<f32>(global<be_f32>(k + 4)) / t);
    be_u32* w = reinterpret_cast<be_u32*>(out);
    for (u32 i : {1u, 2u, 3u, 4u, 6u, 7u, 8u, 9u}) w[i] = 0u;
    w[11] = 0xBF800000u;  // -1.0
    for (u32 i : {12u, 13u, 15u}) w[i] = 0u;
    const f32 depth = near - far, product = far * near;
    out[5] = c;
    out[10] = far / depth;
    out[14] = product / depth;
    out[0] = c / aspect;
}

// void ProjTxr_BuildProjection(f32 pos_out[4], f32 m_out[16], const f32
// eye[3], target[3], up[3], const f32 proj[16], DrawItem* item, const f32
// block[16])
// The projector: L = Matrix_LookAt(eye, target, up); m_out = transpose(
// item rows (w 0, 0, 0, 1) * (L rows (w 0, 0, 0, 1) * (proj * block)))
// (Matrix_MultiplyTo); pos_out (16 bytes, stvx) = the item's 3x3 columns
// through L's column 2 (vmaddfp order; lane 3 junk, as the original).
void ProjTxr_BuildProjection(be_f32* pos_out, be_f32* m_out, const be_f32* eye, const be_f32* target, const be_f32* up,
                             const be_f32* proj, const be_f32* item, const be_f32* block) {
    // The original's sp+112 (L), +176 (L, w), +240 (item, w), +304 / +368 /
    // +432 (products), +528 (L's column 2).
    GuestFrame frame(432);
    be_u32* look = frame.as<be_u32>(0);
    be_u32* lw = frame.as<be_u32>(64);
    be_u32* iw = frame.as<be_u32>(128);
    be_f32* p0 = frame.as<be_f32>(192);
    be_f32* p1 = frame.as<be_f32>(256);
    be_f32* p2 = frame.as<be_f32>(320);
    be_f32* col = frame.as<be_f32>(416);
    call(s5::fn::Matrix_LookAt, look, eye, target, up);
    for (u32 i = 0; i < 4; ++i) {
        copy3(lw + 4 * i, look + 4 * i);
        lw[4 * i + 3] = i == 3 ? kOne : 0u;
    }
    for (u32 i = 0; i < 4; ++i) {
        copy3(iw + 4 * i, at<u8>(item, 16 * i));
        iw[4 * i + 3] = i == 3 ? kOne : 0u;
    }
    Matrix_MultiplyTo(p0, proj, block);
    Matrix_MultiplyTo(p1, reinterpret_cast<be_f32*>(lw), p0);
    Matrix_MultiplyTo(p2, reinterpret_cast<be_f32*>(iw), p1);
    {
        V4 r[4];
        for (u32 i = 0; i < 4; ++i) r[i] = lvx(p2 + 4 * i);
        for (u32 j = 0; j < 4; ++j) stvx(at<u8>(m_out, 16 * j), V4{{r[0].v[j], r[1].v[j], r[2].v[j], r[3].v[j]}});
    }
    const V4 l0 = lvx(look), l1 = lvx(look + 4), l2 = lvx(look + 8);
    stvx(col, V4{{l0.v[2], l1.v[2], l2.v[2], l0.v[2]}});  // vmrglw, vperm
    const V4 i0 = lvx(item), i1 = lvx(at<u8>(item, 16)), i2 = lvx(at<u8>(item, 32));
    const V4 cx{{i0.v[0], i1.v[0], i2.v[0], i1.v[1]}};  // vmrghw, vmrghw
    const V4 cy{{i0.v[1], i1.v[1], i2.v[1], i0.v[0]}};  // vmrghw, vperm
    const V4 cz{{i0.v[2], i1.v[2], i2.v[2], i0.v[2]}};  // vmrglw, vperm
    const f32 t0 = col[0], t1 = col[1], t2 = col[2];  // lvlx, vspltw 0
    stvx(pos_out, madd(cz, t2, madd(cy, t1, madd(cx, t0, kZero4))));
}

// void SelfLight_GetLight(Shader*, DrawItem* item, f32 a[4], f32 dir[4], f32 b[4])
// The first time a material with parameters (+124, word 0 nonzero) comes
// by, word 0 is looked up in the light map (Map_Find in g::light_map
// +16), the result's first word goes to parameter word 4 and word 0 is
// cleared (Material_ParamSlot for both). Then a = {1, 1, 1, 1}, dir =
// {0, 0, 0} and b = {1, 0, 0, 1}.
void SelfLight_GetLight(ShaderPrograms*, DrawItem* item, be_u32* a, be_u32* dir, be_u32* b) {
    Material* mat = or_zero_page(item->mat.get());
    if (const be_u32* params = host<be_u32>(sdraw::mat_params(mat))) {
        if (const u32 key = params[0]; key != 0) {
            *Material_ParamSlot(mat, 0) = 0u;
            GuestFrame frame(16);  // the original's sp+112
            be_u32* k = frame.as<be_u32>();
            *k = key;
            SortedMap* map = *at<gp<SortedMap>>(global<gp<u8>>(s5::g::light_map).get(), 16);
            const u32 light = *Map_Find3(or_zero_page(map), k);
            *Material_ParamSlot(mat, 4) = light;
        }
    }
    for (u32 i = 0; i < 4; ++i) a[i] = kOne;
    for (u32 i = 0; i < 3; ++i) dir[i] = 0u;
    b[0] = kOne;
    b[1] = 0u;
    b[2] = 0u;
    b[3] = kOne;
}

// void View_SetConstants(View* view)
// The view constants block (shader5.h) from the view.
void View_SetConstants(u8* view) {
    constexpr u32 k = 0x005207A0;  // .text: two vperm masks, f32 0.5, 1.0, k (tan reference angle)
    constexpr u32 kCurrentSurface = 0x010DDA74;
    u8* b = host<u8>(s5::g::view_block);
    for (u32 i = 0; i < 64; i += 4) copy_word(b + i, at<u8>(view, 1008 + i));
    for (u32 i = 0; i < 128; i += 4) copy_word(b + 64 + i, at<u8>(view, 880 + i));
    for (u32 r = 0; r < 4; ++r) copy_rows(b + 192 + 16 * r, at<u8>(view, 752 + 16 * r), {0, 4, 8});
    {
        const V4 p = lvx(at<u8>(view, 800)), ra = lvx(at<u8>(view, 752)), rb = lvx(at<u8>(view, 768)), rc = lvx(at<u8>(view, 784));
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
    for (u32 r = 0; r < 4; ++r) copy_rows(b + 320 + 16 * r, at<u8>(view, 1200 + 16 * r), {0, 4, 8});
    for (u32 r = 0; r < 8; ++r) copy_rows(b + 384 + 16 * r, at<u8>(view, 1072 + 16 * r), {0, 4, 8});
    for (u32 i = 0; i < 2; ++i) {
        copy_rows(b + 512 + 16 * i, at<u8>(view, 144 + 64 * i), {0, 4, 8, 12});
        copy_rows(b + 544 + 32 * i, at<u8>(view, 160 + 64 * i), {0, 4, 8, 12});
        copy_rows(b + 560 + 32 * i, at<u8>(view, 176 + 64 * i), {0, 4, 8, 12});
    }
    if (global<u8>(s5::g::fov_ref_set) == 0) {
        global<be_f32>(s5::g::fov_ref) = static_cast<f32>(call<f64>(s5::fn::tanf, global<be_f32>(k + 40).get()));
        global<u8>(s5::g::fov_ref_set) = 1;
    }
    const f64 t = call<f64>(s5::fn::tanf, *at<be_f32>(view, 56) * global<be_f32>(k + 32));
    const f32 ratio = static_cast<f32>(t / static_cast<f32>(global<be_f32>(s5::g::fov_ref)));
    const f32 one = global<be_f32>(k + 36);
    *reinterpret_cast<be_f32*>(b + 612) = static_cast<f32>(fsel(ratio - one, one, ratio));  // fsel: min(ratio, 1)
    const f32 scale = *at<be_f32>(&config(), 320);
    for (u32 i = 0; i < 6; ++i) {
        for (u32 j = 0; j < 16; j += 4) *reinterpret_cast<be_f32*>(b + 624 + 16 * i + j) = global<be_f32>(s5::g::config_vec4s + 16 * i + j) * scale;
    }
    be_u32* w720 = reinterpret_cast<be_u32*>(b + 720);
    w720[0] = 0u;
    w720[1] = kOne;
    w720[2] = 0u;
    w720[3] = 0u;
    const u32 draws = global<be_u32>(kCurrentSurface + 4);
    *reinterpret_cast<be_u32*>(b + 616) = draws;
}

}  // namespace sly4::gfx

NATIVE(0050B980, sly4::gfx::MaterialAnim_GetMatrix)
NATIVE(0056A200, sly4::gfx::Matrix_Perspective)
NATIVE(005B0CF0, sly4::gfx::ProjTxr_BuildProjection)
NATIVE(005B47F4, sly4::gfx::SelfLight_GetLight)
NATIVE(005207D0, sly4::gfx::View_SetConstants)
