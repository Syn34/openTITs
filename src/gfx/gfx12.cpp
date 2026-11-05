// The renderer range's next recompiled functions: the callees gfx11*.cpp
// left (the vertex-constant matrix upload, the frustum planes, the shadow
// projection's frustum corners and bounds, the trail's screen-space fade
// and the topology record holder) and the next smallest ones. Names
// carry the address where the role is unclear.
#include <atomic>
#include <bit>
#include <cmath>

#include "fx.h"
#include "gfx12.h"

using namespace sly4;

namespace {

namespace g11 = sly4::render::gfx11;
namespace g12 = sly4::render::gfx12;
namespace g8 = sly4::render::gfx8;
using g11::lfs;
using g11::sgl;

// The f32 store of an FPR value (stfs).
void stfs12(u32 ea, f64 v) { store<u32>(ea, g11::stfs_bits(v)); }

// The 4x4 at m (16-byte aligned, lvx / stvx) transposed in place.
void transpose12(u32 m) {
    m &= ~15u;
    u32 w[4][4];
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 4; ++k) w[i][k] = load<u32>(m + 16 * i + 4 * k);
    for (u32 i = 0; i < 4; ++i)
        for (u32 k = 0; k < 4; ++k) store<u32>(m + 16 * i + 4 * k, w[k][i]);
}

// Word 0 of lvlx(ea): the bytes from ea to the end of its 16-byte block,
// zero filled.
f32 lvlx_f32(u32 ea) {
    u32 w = 0;
    for (u32 i = 0; i < 4; ++i) w = (w << 8) | ((ea & 15) + i < 16 ? load<u8>(ea + i) : 0u);
    return std::bit_cast<f32>(w);
}

// r = m (rows 0..2 xyz, read with lvx) * b in vmaddfp order, lane by lane:
// b0 * x + (row 3 ? b3 : 0), + b1 * y, + b2 * z. Addresses rounded down
// to 16.
void mul_affine12(u32 m, u32 b, f32 r[4][4]) {
    m &= ~15u;
    b &= ~15u;
    for (u32 i = 0; i < 4; ++i) {
        const f32 x = load<f32>(m + 16 * i), y = load<f32>(m + 16 * i + 4), z = load<f32>(m + 16 * i + 8);
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fmaf(load<f32>(b + 4 * k), x, i == 3 ? load<f32>(b + 48 + 4 * k) : 0.0f);
            v = std::fmaf(load<f32>(b + 16 + 4 * k), y, v);
            r[i][k] = std::fmaf(load<f32>(b + 32 + 4 * k), z, v);
        }
    }
}

// The point (x, y, z) through the 4x4 at b (lvx rows) in vmaddfp order:
// b0 * x + 0, + b1 * y, + b2 * z, then b3 + that (vaddfp).
void point_through12(u32 b, f32 x, f32 y, f32 z, f32 out[4]) {
    b &= ~15u;
    for (u32 k = 0; k < 4; ++k) {
        f32 v = std::fmaf(load<f32>(b + 4 * k), x, 0.0f);
        v = std::fmaf(load<f32>(b + 16 + 4 * k), y, v);
        v = std::fmaf(load<f32>(b + 32 + 4 * k), z, v);
        out[k] = load<f32>(b + 48 + 4 * k) + v;
    }
}

}  // namespace

// void VpConst_Set_004FCC34(const f32 m[16])   (Sprite_Bind_00501820; name?)
// Vertex constants 0..3 = (m * the view constants block's first matrix
// (layers3::g::block_dd780 +0, the view's +1008)) transposed: rows of m
// (rows 0..2 xyz, row 3 adding the block's row 3), in vmaddfp order.
// Leaves r3 as cellGcmSetVertexProgramConstants does.
DECOMP(004FCC34, VpConst_Set_004FCC34) {
    const u32 m = arg<u32>(ctx, 0);
    f32 r[4][4];
    mul_affine12(m, sly4::render::shader5::g::view_block, r);
    Stack frame(ctx, 64);  // the original's sp+112..175
    const u32 t = frame.addr();
    for (u32 k = 0; k < 4; ++k)
        for (u32 i = 0; i < 4; ++i) store<f32>(t + 16 * k + 4 * i, r[i][k]);
    ret(ctx, call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 0, 16, t}));
}

// void Frustum_FromMatrix(Frustum* f, const f32 m[16])   (name?)
// The six planes of the clip matrix m (rows; c_j = column j: m[j],
// m[4 + j], m[8 + j], m[12 + j]): c3 + c0, c3 - c0, c3 - c1, c3 + c1, c2
// and c3 - c2, each scaled by 1 / |xyz| (f32[6][4] at +0). Then two
// transposed copies for 4-wide tests: +96 = planes 0..3 as columns, +160
// = planes 4, 5 and g12::g::frustum_pad twice, transposed. Leaves r3 =
// f + 208 and f1 = frustum_pad[3].
DECOMP(004FDEA0, Frustum_FromMatrix) {
    const u32 f = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    auto c = [&](u32 row, u32 col) { return lfs(m + 16 * row + 4 * col); };
    for (u32 r = 0; r < 4; ++r) stfs12(f + 4 * r, sgl(c(r, 3) + c(r, 0)));
    for (u32 r = 0; r < 4; ++r) stfs12(f + 16 + 4 * r, sgl(c(r, 3) - c(r, 0)));
    for (u32 r = 0; r < 4; ++r) stfs12(f + 32 + 4 * r, sgl(c(r, 3) - c(r, 1)));
    for (u32 r = 0; r < 4; ++r) stfs12(f + 48 + 4 * r, sgl(c(r, 3) + c(r, 1)));
    for (u32 r = 0; r < 4; ++r) store<u32>(f + 64 + 4 * r, load<u32>(m + 16 * r + 8));  // lfs / stfs: raw
    for (u32 r = 0; r < 4; ++r) stfs12(f + 80 + 4 * r, sgl(c(r, 3) - c(r, 2)));
    const f64 one = lfs(0x004FDE9C);  // 1.0
    for (u32 i = 0; i < 6; ++i) {
        const u32 p = f + 16 * i;
        const f64 y = lfs(p + 4), x = lfs(p), z = lfs(p + 8), w = lfs(p + 12);
        f64 len = sgl(y * y);
        len = g11::madds(x, x, len);
        len = g11::madds(z, z, len);
        const f64 inv = sgl(one / sgl(std::sqrt(len)));  // fsqrts, fdivs
        stfs12(p, sgl(x * inv));
        stfs12(p + 4, sgl(y * inv));
        stfs12(p + 8, sgl(z * inv));
        stfs12(p + 12, sgl(w * inv));
    }
    for (u32 k = 0; k < 96; k += 4) store<u32>(f + 96 + k, load<u32>(f + k));  // lfs / stfs: raw
    for (u32 k = 0; k < 32; k += 4) store<u32>(f + 192 + k, load<u32>(g12::g::frustum_pad + (k & 15)));
    transpose12(f + 96);
    transpose12(f + 160);
    ctx.fpr[1] = lfs(g12::g::frustum_pad + 12);
    ret(ctx, f + 208);
}

// void Material_AddTexture_004FF008(Material* m, Texture* t)   (the
// shaders' LoadParams; name?)
// The next texture slot: u8 count +44 += 1 (wrapping), slot +20 + 4 *
// (count - 1) (a wrapped count writes +16). A different texture there is
// replaced: the new one's u16 +4 references += 1, the old one released
// (vtable slot 8). Slot 0 also sets u16 +48 = t +112 (0 for no texture),
// even when unchanged. Leaves r3 as the original does.
DECOMP(004FF008, Material_AddTexture_004FF008) {
    const u32 m = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u8 count = static_cast<u8>(load<u8>(m + 44) + 1);
    const s32 idx = static_cast<s32>(count) - 1;
    store<u8>(m + 44, count);
    const u32 slot = m + 20 + 4 * static_cast<u32>(idx);
    const u32 old = load<u32>(slot);
    u64 r3 = old;
    if (old != t) {
        store<u32>(slot, t);
        if (t != 0) store<u16>(t + 4, static_cast<u16>(load<u16>(t + 4) + 1));
        if (old != 0) r3 = fx::vcall(ctx, old, 8, {old});
    }
    if (idx == 0) {
        r3 = t != 0 ? load<u32>(t + 112) : 0u;
        store<u16>(m + 48, static_cast<u16>(r3));
    }
    ret(ctx, r3);
}

namespace {

// A TopoRecord at zero references freed as TopoRecord_Set does: each
// entry of its +8 array (u32 count +4, re-read after each free) gives its
// 24-byte items (+0, u16 count +14) back (+4 pool block; +12 / +16 / +20
// freed and zeroed), then the items, the entry (its slot zeroed); then
// the array (zeroed) and, when non-null, the record. Returns the record
// free's r3 (0 for a null record).
u64 topo_free12(PPUContext& ctx, u32 r) {
    u32 count = load<u32>(r + 4);
    if (count != 0) {
        u32 i = 0;
        do {
            const u32 e = load<u32>(load<u32>(r + 8) + 4 * i);
            if (e != 0) {
                if (load<u32>(e) != 0) {
                    for (u32 j = 0, off = 0; j < load<u16>(e + 14); ++j, off += 24) {
                        const u32 it = load<u32>(e) + off;
                        if (load<u32>(it + 4) != 0) call(ctx, g8::fn::Mem_FreePool, {load<u32>(it + 4), 0, 0});
                        for (u32 k : {12u, 16u, 20u}) {
                            if (load<u32>(it + k) == 0) continue;
                            call(ctx, g8::fn::Mem_FreeFrom, {load<u32>(it + k), 0});
                            store<u32>(it + k, 0);
                        }
                    }
                    call(ctx, g8::fn::Mem_FreePool, {load<u32>(e), 0, 0});
                }
                call(ctx, g8::fn::Mem_FreeFrom, {e, 0});
                const u32 arr = load<u32>(r + 8);
                count = load<u32>(r + 4);
                store<u32>(arr + 4 * i, 0);
            }
            i = (i + 1) & 0xFFFF;
        } while (i < count);
    }
    if (load<u32>(r + 8) != 0) {
        call(ctx, g8::fn::Mem_FreeFrom, {load<u32>(r + 8), 0});
        store<u32>(r + 8, 0);
    }
    return r != 0 ? call(ctx, g8::fn::Mem_FreeFrom, {r, 0}) : 0;
}

}  // namespace

// TopoRecordRef* TopoRecordRef_New(Topology* o)   (Iff_HandleRecord_00523C98;
// name?)
// A new 16-byte TopoRecordRef {+0 record, +4 o, +8 u8 {25, 25, 25, 255},
// +12 7.25f} holding a new 12-byte TopoRecord {u16 references, +4 u32
// count 0, +8 entry array of config +500 (or 1) pointers}: Mem_Alloc
// (16, 0, 16) / (12, 0, 16) / (n * 4, 0, 16), TopoRecord_Set, then the
// creation's reference dropped (TopoRecord_Set's release inline; the
// record's own free skipped for 0). Quirk: failed allocations are
// written through anyway (addresses 4..15 and 4).
DECOMP(004FD8D4, TopoRecordRef_New) {
    const u32 o = arg<u32>(ctx, 0);
    u32 ref = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {16, 0, 16}));
    if (ref != 0) store<u32>(ref, 0);
    store<u32>(ref + 4, o);
    store<u8>(ref + 8, 25);
    store<u8>(ref + 9, 25);
    store<u8>(ref + 10, 25);
    store<u8>(ref + 11, 255);
    store<u32>(ref + 12, 0x40E80000u);  // 7.25f
    const u32 rec = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {12, 0, 16}));
    if (rec != 0) store<u16>(rec, 1);
    store<u32>(rec + 4, 0);
    const u32 slots = load<u16>(sly4::g::config + g12::kTopoRecordDefault);
    const u32 n = slots != 0 ? slots : 1u;
    store<u32>(rec + 8, static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {n << 2, 0, 16})));
    call(ctx, g12::fn::TopoRecord_Set, {ref, rec});
    const u32 r = load<u32>(ref);
    const u16 refs = static_cast<u16>(load<u16>(r) - 1);
    store<u16>(r, refs);
    if (static_cast<s16>(refs) <= 0) topo_free12(ctx, r);
    ret(ctx, ref);
}

// void Shadow_FrustumCorners_0052B4B8(const Matrix* inv_proj, -, -, f32
// out[32]; f1 near, f2 far)   (ShadowProj_Build_0052B688; name?)
// The four clip-space corners (-1, 1) (1, 1) (1, -1) (-1, -1) (z 0, w 1)
// through inv_proj (Vec4_Transform) and divided by w, then scaled to
// view depth -near (out[0..15], xyz of 16-byte rows) and -far (out[16..
// 31]): p * (-d / p.z). Leaves r3 = &corner[3].z on the stack and f1 =
// the last value.
DECOMP(0052B4B8, Shadow_FrustumCorners_0052B4B8) {
    const u32 m = arg<u32>(ctx, 0), out = arg<u32>(ctx, 3);
    const f64 near_neg = -farg(ctx, 0), far_neg = -farg(ctx, 1);
    const f64 one = lfs(0x0052B4B4);  // 1.0
    Stack frame(ctx, 144);            // the original's sp+112..255
    const u32 base = frame.addr();
    constexpr u32 kM1 = 0xBF800000u, kP1 = 0x3F800000u;
    constexpr u32 kCorners[16] = {kM1, kP1, 0, kP1, kP1, kP1, 0, kP1, kP1, kM1, 0, kP1, kM1, kM1, 0, kP1};
    for (u32 k = 0; k < 16; ++k) store<u32>(base + 4 * k, kCorners[k]);
    f64 last = 0;
    for (u32 i = 0; i < 4; ++i) {
        const u32 p = base + 16 * i, o = out + 16 * i;
        call(ctx, g12::fn::Vec4_Transform, {p, m});
        const f64 inv = sgl(one / lfs(p + 12));
        const f64 x = sgl(lfs(p) * inv), y = sgl(lfs(p + 4) * inv), z = sgl(lfs(p + 8) * inv), w = sgl(lfs(p + 12) * inv);
        stfs12(p, x);
        stfs12(p + 4, y);
        stfs12(p + 8, z);
        stfs12(p + 12, w);
        const f64 pz = lfs(p + 8);
        const f64 kn = sgl(near_neg / pz), kf = sgl(far_neg / pz);
        stfs12(o, sgl(x * kn));
        const f64 py = lfs(p + 4);
        stfs12(o + 4, sgl(py * kn));
        stfs12(o + 8, sgl(pz * kn));
        stfs12(o + 64, sgl(x * kf));
        stfs12(o + 68, sgl(py * kf));
        last = sgl(pz * kf);
        stfs12(o + 72, last);
    }
    ctx.fpr[1] = last;
    ret(ctx, base + 48 + 8);  // r3 as the original leaves it
}

// void Shadow_Bounds_0052B2EC(f32 lo[3], f32 hi[3], const f32 corners[8][4],
// const Matrix* a, const Matrix* b)   (ShadowProj_Build_0052B688; name?)
// The box around the 8 corners (xyz of 16-byte rows) taken through a then
// b (VMX, w = 1) with z negated: lo / hi start at +-FLT_MAX and keep the
// per-axis min / max (fsel: a NaN difference takes the corner). Leaves
// f1 = hi[1].
DECOMP(0052B2EC, Shadow_Bounds_0052B2EC) {
    const u32 lo = arg<u32>(ctx, 0), hi = arg<u32>(ctx, 1), corners = arg<u32>(ctx, 2);
    const u32 a = arg<u32>(ctx, 3), b = arg<u32>(ctx, 4);
    constexpr u32 kFltMax = 0x7F7FFFFFu, kFltMaxNeg = 0xFF7FFFFFu;
    for (u32 k = 0; k < 12; k += 4) store<u32>(lo + k, kFltMax);
    for (u32 k = 0; k < 12; k += 4) store<u32>(hi + k, kFltMaxNeg);
    // The original also copies each corner's xyz to its stack slot first;
    // the vector store overwrites them before they are read.
    Stack frame(ctx, 80);  // the original's sp+48..127
    const u32 t = frame.addr();
    f64 hy_new = 0;
    for (u32 i = 0; i < 8; ++i) {
        const u32 c = corners + 16 * i;
        f32 v[4], u[4];
        point_through12(a, lvlx_f32(c), lvlx_f32(c + 4), lvlx_f32(c + 8), v);
        for (u32 k = 0; k < 4; ++k) store<f32>(t + 4 * k, v[k]);
        point_through12(b, lvlx_f32(t), lvlx_f32(t + 4), lvlx_f32(t + 8), u);
        for (u32 k = 0; k < 4; ++k) store<f32>(t + 4 * k, u[k]);
        const f64 tx = lfs(t), ty = lfs(t + 4), tz = -lfs(t + 8);
        const f64 lx = lfs(lo), ly = lfs(lo + 4), lz = lfs(lo + 8);
        stfs12(lo, sgl(lx - tx) >= 0.0 ? tx : lx);
        stfs12(lo + 8, sgl(lz - tz) >= 0.0 ? tz : lz);
        stfs12(lo + 4, sgl(ly - ty) >= 0.0 ? ty : ly);
        const f64 hx = lfs(hi), hy = lfs(hi + 4), hz = lfs(hi + 8);
        hy_new = sgl(hy - ty) >= 0.0 ? hy : ty;
        stfs12(hi, sgl(hx - tx) >= 0.0 ? hx : tx);
        stfs12(hi + 4, hy_new);
        stfs12(hi + 8, sgl(hz - tz) >= 0.0 ? hz : tz);
    }
    ctx.fpr[1] = hy_new;
    ret(ctx, lo);
}

// f32 Trail_Fn_0051EB20(Trail* o)   (Trail_UpdateFade_0051ED00; name?)
// The trail's on-screen visibility: the circle at the projected centre
// (+64 / +68, radius +80, in screen units: x by config +4, y by config
// +6) clipped to the rectangle +24..+32 / +28..+36, the clipped area /
// (4 r^2). Returns that only when the fade +72 is in [0, 1] and the
// centre +48 is in front of the camera (view +848.. forward, view +864..
// position); else 0. Leaves r3 = o + 72 (fade below 0) or 0x0051EB14.
DECOMP(0051EB20, Trail_Fn_0051EB20) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kK = 0x0051EB14;  // f32 4.0, 0.0, 1.0
    const f64 w = static_cast<f64>(load<u16>(sly4::g::config + 4));  // fcfid / frsp: exact
    const f64 h = static_cast<f64>(load<u16>(sly4::g::config + 6));
    const f64 x_lo = sgl(lfs(o + 24) * w), x_hi = sgl(lfs(o + 32) * w);
    const f64 r = sgl(lfs(o + 80) * h), y_lo = sgl(lfs(o + 28) * h);
    const f64 cx0 = g11::msubs(lfs(o + 64), w, r), cx1 = g11::madds(lfs(o + 64), w, r);
    const f64 cy0 = g11::msubs(lfs(o + 68), h, r), cy1 = g11::madds(lfs(o + 68), h, r);
    const f64 area = sgl(sgl(r * lfs(kK)) * r);
    const f64 y_hi = sgl(lfs(o + 36) * h);
    const f64 x0 = sgl(cx0 - x_lo) >= 0.0 ? cx0 : x_lo;  // fsel: max / min
    const f64 x1 = sgl(cx1 - x_hi) >= 0.0 ? x_hi : cx1;
    const f64 y0 = sgl(cy0 - y_lo) >= 0.0 ? cy0 : y_lo;
    const f64 y1 = sgl(cy1 - y_hi) >= 0.0 ? y_hi : cy1;
    const f64 zero = lfs(kK + 4);
    const f64 dx = x0 < x1 ? sgl(x1 - x0) : zero;
    const f64 dy = y0 < y1 ? sgl(y1 - y0) : zero;
    const f64 seen = sgl(dx * dy);
    const u32 view = load<u32>(load<u32>(sly4::draw::g::current_surface) + 672);
    const u32 c = o + 48, fwd = view + 848, pos = view + 864;
    const f64 ey = sgl(lfs(c + 4) - lfs(pos + 4)), ex = sgl(lfs(c) - lfs(pos)), ez = sgl(lfs(c + 8) - lfs(pos + 8));
    f64 d = sgl(-lfs(fwd + 4) * ey);
    d = g11::madds(-lfs(fwd), ex, d);
    d = g11::madds(-lfs(fwd + 8), ez, d);
    const f64 fade = lfs(o + 72);
    f64 v = zero;
    u32 r3 = o + 72;
    if (fade >= zero) {
        r3 = kK;
        if (fade <= lfs(kK + 8) && d > zero) v = sgl(seen / area);
    }
    ctx.fpr[1] = v;
    ret(ctx, r3);
}

// void Trail_Fn_0051E8E0(Trail* o, const f32 sphere[4], f32 out[4], f32*
// radius)   (Trail_UpdateFade_0051ED00 passes o+48, o+64, o+80; name?)
// The sphere's screen position: its centre through the view's matrix
// (+880, Vec4_Transform) divided by w, plus g12::g::screen_bias, times
// 0.5 with y flipped (1 - y / 2): out = that {x, y, z, w}; *radius = the
// screen distance to the centre offset by radius along the view's +832
// axis, projected the same way. Leaves r3 as the second Vec4_Transform
// does and f1 = *radius.
DECOMP(0051E8E0, Trail_Fn_0051E8E0) {
    const u32 s = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2), rad = arg<u32>(ctx, 3);
    constexpr u32 kK = 0x0051E8D8;  // f32 1.0, 0.5
    const f64 one = lfs(kK);
    const u32 view = load<u32>(load<u32>(sly4::draw::g::current_surface) + 672);
    const u32 m = view + 880, axis = view + 832, bias = g12::g::screen_bias;
    const u32 sx = load<u32>(s), sy = load<u32>(s + 4), sz = load<u32>(s + 8);
    const f64 sr = lfs(s + 12);
    const f64 ax = lfs(axis), ay = lfs(axis + 4), az = lfs(axis + 8);
    Stack frame(ctx, 160);  // the original's sp+112..271
    const u32 p = frame.addr(), q = p + 16;
    store<u32>(p, sx);  // lfs / stfs: raw
    store<u32>(p + 4, sy);
    store<u32>(p + 8, sz);
    store<u32>(p + 12, 0x3F800000u);
    call(ctx, g12::fn::Vec4_Transform, {p, m});
    const f64 qx = g11::madds(ax, sr, sly4::render::gfx10::lfs_value(sx)), qy = g11::madds(ay, sr, sly4::render::gfx10::lfs_value(sy)), qz = g11::madds(az, sr, sly4::render::gfx10::lfs_value(sz));
    {
        const f64 pw = lfs(p + 12);
        const f64 inv = sgl(one / pw);
        const f64 x = sgl(lfs(p) * inv), y = sgl(lfs(p + 4) * inv), z = sgl(lfs(p + 8) * inv), w = sgl(pw * inv);
        const f64 bx = sgl(x + lfs(bias)), by = sgl(y + lfs(bias + 4));
        stfs12(p, x);
        stfs12(p + 4, y);
        stfs12(p + 8, z);
        stfs12(p + 12, w);
        const f64 bz = sgl(z + lfs(bias + 8)), bw = sgl(w + lfs(bias + 12));
        const f64 half = lfs(kK + 4);
        stfs12(q, qx);
        stfs12(q + 4, qy);
        stfs12(q + 8, qz);
        store<u32>(q + 12, 0x3F800000u);
        stfs12(p, sgl(bx * half));
        stfs12(p + 8, sgl(bz * half));
        stfs12(p + 12, sgl(bw * half));
        stfs12(p + 4, g11::nmsubs(by, half, one));
    }
    const u64 r3 = call(ctx, g12::fn::Vec4_Transform, {q, m});
    const f64 half = lfs(kK + 4);
    const f64 qw = lfs(q + 12);
    const f64 inv = sgl(one / qw);
    const f64 y = sgl(lfs(q + 4) * inv), x = sgl(lfs(q) * inv), z = sgl(lfs(q + 8) * inv), w = sgl(qw * inv);
    const f64 px = lfs(p);
    const f64 by = sgl(y + lfs(bias + 4));
    stfs12(q + 4, y);
    stfs12(q, x);
    stfs12(q + 8, z);
    stfs12(q + 12, w);
    stfs12(out, px);
    const f64 bx = sgl(x + lfs(bias));
    const f64 py = lfs(p + 4);
    stfs12(out + 4, py);
    const f64 ey = sgl(py - g11::nmsubs(by, half, one));
    const f64 ex = g11::nmsubs(bx, half, px);
    const f64 d2 = g11::madds(ex, ex, sgl(ey * ey));
    store<u32>(out + 8, load<u32>(p + 8));  // lfs / stfs: raw
    store<u32>(out + 12, load<u32>(p + 12));
    const f64 dist = sgl(std::sqrt(d2));  // fsqrts
    stfs12(rad, dist);
    ctx.fpr[1] = dist;
    ret(ctx, r3);
}

namespace {

// floorf(v / s) * s as Shadow_SnapBounds_0052AF78 inlines it: q = v / s;
// q itself when q - trunc(q) is +0 or |q| >= 2^24 (exponent check), else
// trunc(q) (fctidz / fcfid), or (fctiwz(q) - 1) for a negative q. *r3
// gets the fctiwz word (minus 1 on the negative path) as the original
// leaves it.
f64 snap12(f64 v, f64 s, u64& r3) {
    const f64 q = sgl(v / s);
    const f64 tq = sgl(g11::trunc_d(q));
    const u32 iw = static_cast<u32>(rt::fctiw(q, true));
    r3 = iw;
    f64 f = q;
    const u32 qb = g11::stfs_bits(q);
    if (g11::stfs_bits(sgl(tq - q)) != 0 && static_cast<s32>(qb & 0x7F800000u) < 0x4B800000) {
        if ((qb & 0x80000000u) != 0) {
            const s32 m1 = static_cast<s32>(iw - 1);
            r3 = static_cast<u64>(static_cast<s64>(m1));
            f = sgl(static_cast<f64>(m1));
        } else {
            f = tq;
        }
    }
    return sgl(f * s);
}

}  // namespace

// void Shadow_SnapBounds_0052AF78(f32 lo[3], f32 hi[3], u32 w, u32 h)
// (ShadowProj_Build_0052B688; name?)
// Snaps the shadow box's x / y to its texel grid: cell = (hi - lo) / w
// (x) and / h (y); lo.x, lo.y, hi.x, hi.y become floor(v / cell) * cell.
// Leaves f1 = the new hi.y and r3 the last floor's integer word.
DECOMP(0052AF78, Shadow_SnapBounds_0052AF78) {
    const u32 lo = arg<u32>(ctx, 0), hi = arg<u32>(ctx, 1);
    const f64 cx = sgl(sgl(lfs(hi) - lfs(lo)) / sgl(static_cast<f64>(arg<u32>(ctx, 2))));
    const f64 cy = sgl(sgl(lfs(hi + 4) - lfs(lo + 4)) / sgl(static_cast<f64>(arg<u32>(ctx, 3))));
    u64 r3 = 0;
    stfs12(lo, snap12(lfs(lo), cx, r3));
    stfs12(lo + 4, snap12(lfs(lo + 4), cy, r3));
    stfs12(hi, snap12(lfs(hi), cx, r3));
    const f64 last = snap12(lfs(hi + 4), cy, r3);
    stfs12(hi + 4, last);
    ctx.fpr[1] = last;
    ret(ctx, r3);
}

namespace {

// min(hi, max(lo, p)) with fsel (a NaN difference takes the bound).
f64 clamp12(f64 p, f64 lo, f64 hi) {
    const f64 a = sgl(hi - p) >= 0.0 ? p : hi;
    return sgl(a - lo) >= 0.0 ? a : lo;
}

}  // namespace

// void Shadow_DepthRange_0052AA98(f32* near, f32* far, const f32 b[8],
// const Matrix* view, const f32 dir[3], const f32 box[8])
// (ShadowProj_Build_0052B688; name?)
// The depth range the light's box covers: the 4 corners of b's near face
// (x b[0] / b[4], y b[1] / b[5], z b[2]) taken from view space to world
// space (the inverse of view's rotation and translation, VMX), then for
// each axis j with |dir[j]| >= 0.001 the rays corner + t dir to box's lo
// [j] (+0) and hi[j] (+16) planes, each hit clamped into the box and
// taken through view: -z widens [*near, *far] (from FLT_MAX / -FLT_MAX).
// Unchanged ends fall back to b[2] / b[6]. Leaves f1 / r3 as the
// original does.
DECOMP(0052AA98, Shadow_DepthRange_0052AA98) {
    const u32 pn = arg<u32>(ctx, 0), pf = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 m = arg<u32>(ctx, 3) & ~15u, dir = arg<u32>(ctx, 4), lo = arg<u32>(ctx, 5), hi = lo + 16;
    constexpr u32 kK = 0x0052AA60;  // f32 FLT_MAX, -FLT_MAX; +48 f32 0.001
    store<u32>(pn, 0x7F7FFFFFu);
    store<u32>(pf, 0xFF7FFFFFu);
    auto mv = [&](u32 row, u32 col) { return load<f32>(m + 16 * row + 4 * col); };
    f32 t[3];  // -R^T t: the inverse's translation
    for (u32 k = 0; k < 3; ++k) {
        f32 v = std::fmaf(mv(k, 0), 0.0f - mv(3, 0), 0.0f);
        v = std::fmaf(mv(k, 1), 0.0f - mv(3, 1), v);
        t[k] = std::fmaf(mv(k, 2), 0.0f - mv(3, 2), v);
    }
    auto view_z = [&](f32 x, f32 y, f32 z) {
        f32 v = std::fmaf(mv(0, 2), x, 0.0f);
        v = std::fmaf(mv(1, 2), y, v);
        v = std::fmaf(mv(2, 2), z, v);
        return mv(3, 2) + v;
    };
    const f64 eps = lfs(kK + 48);
    f64 far_reg = lfs(kK + 4);  // f5: the last far written
    auto widen = [&](f32 z) {
        const f64 nz = -sly4::render::gfx10::lfs_value(std::bit_cast<u32>(z));  // stvx, lfs, fneg
        const f64 n = lfs(pn);
        stfs12(pn, sgl(n - nz) >= 0.0 ? nz : n);
        const f64 f = lfs(pf);
        far_reg = sgl(f - nz) >= 0.0 ? f : nz;
        stfs12(pf, far_reg);
    };
    const u32 bz = load<u32>(b + 8);
    const u32 corner_xy[4][2] = {{load<u32>(b), load<u32>(b + 4)},
                                 {load<u32>(b), load<u32>(b + 20)},
                                 {load<u32>(b + 16), load<u32>(b + 4)},
                                 {load<u32>(b + 16), load<u32>(b + 20)}};
    for (u32 i = 0; i < 4; ++i) {
        const f32 x = std::bit_cast<f32>(corner_xy[i][0]), y = std::bit_cast<f32>(corner_xy[i][1]), z = std::bit_cast<f32>(bz);
        f64 c[3];
        for (u32 k = 0; k < 3; ++k) {
            f32 v = std::fmaf(mv(k, 0), x, 0.0f);  // row k of the rotation: R^T (x, y, z)
            v = std::fmaf(mv(k, 1), y, v);
            v = std::fmaf(mv(k, 2), z, v);
            c[k] = sly4::render::gfx10::lfs_value(std::bit_cast<u32>(v + t[k]));
        }
        for (u32 j = 0; j < 3; ++j) {
            const f64 d = lfs(dir + 4 * j);
            if (std::fabs(d) < eps) continue;
            {
                const f64 s = sgl(sgl(lfs(lo + 4 * j) - c[j]) / d);
                const f64 px = g11::madds(s, lfs(dir), c[0]);
                const f64 hx = lfs(hi), lx = lfs(lo);
                const f64 py = g11::madds(s, lfs(dir + 4), c[1]), pz = g11::madds(s, lfs(dir + 8), c[2]);
                const f64 qx = clamp12(px, lx, hx);
                const f64 qy = clamp12(py, lfs(lo + 4), lfs(hi + 4));
                const f64 qz = clamp12(pz, lfs(lo + 8), lfs(hi + 8));
                widen(view_z(static_cast<f32>(qx), static_cast<f32>(qy), static_cast<f32>(qz)));
            }
            {
                const f64 s = sgl(sgl(lfs(hi + 4 * j) - c[j]) / lfs(dir + 4 * j));
                const f64 px = g11::madds(s, lfs(dir), c[0]);
                const f64 hx = lfs(hi), lx = lfs(lo);
                const f64 py = g11::madds(s, lfs(dir + 4), c[1]), pz = g11::madds(s, lfs(dir + 8), c[2]);
                const f64 qx = clamp12(px, lx, hx);
                const f64 qy = clamp12(py, lfs(lo + 4), lfs(hi + 4));
                const f64 qz = clamp12(pz, lfs(lo + 8), lfs(hi + 8));
                widen(view_z(static_cast<f32>(qx), static_cast<f32>(qy), static_cast<f32>(qz)));
            }
        }
    }
    f64 f1 = lfs(pn);
    u32 r3 = pn;
    if (f1 == lfs(kK)) {
        f1 = lfs(b + 8);
        store<u32>(pn, load<u32>(b + 8));  // lfs / stfs: raw
        far_reg = lfs(pf);
    }
    if (far_reg == lfs(kK + 4)) {
        r3 = b + 24;
        f1 = lfs(b + 24);
        store<u32>(pf, load<u32>(b + 24));
    }
    ctx.fpr[1] = f1;
    ret(ctx, r3);
}

// void DrawQueue_Build_00516970(Batch* q)   (DrawQueue_Run_00516CD0; name?)
// Copies the draw records of q's list (+0: {Record** +0, u16 count +8})
// into per-frame memory and links them into job batches: q +12 first /
// +16 last copy, +20 / +24 two fresh u32 counters (MemStack_Alloc(frame
// stack, 4), zeroed), +8 = 0x8000 once a copy is made. A record counts
// when the u16 +8 of its first u16 +256 16-byte slots (0xFFFF: unused)
// add up to non-zero (u16 sums); when the batch already holds one
// record (*+20 == 1) it is submitted (DrawQueue_Submit_0051669C) and
// restarted. A copy (352 bytes) takes the 16 slots' +0 / +4 / u16 +8 /
// +12, +256, the f32s +272..+280 / +288.. / +304.. / +320.., the bytes
// +336..+339 and +340; each used slot's +12 = cellGcmAddressToOffset of
// its +4. A batch left non-empty is submitted at the end. Leaves r3 as
// the original does.
DECOMP(00516970, DrawQueue_Build_00516970) {
    const u32 q = arg<u32>(ctx, 0);
    auto fresh = [&] {
        store<u32>(q + 20, static_cast<u32>(call(ctx, g12::fn::MemStack_Alloc, {load<u32>(g12::g::frame_stack), 4})));
        store<u32>(q + 24, static_cast<u32>(call(ctx, g12::fn::MemStack_Alloc, {load<u32>(g12::g::frame_stack), 4})));
        store<u32>(load<u32>(q + 20), 0);
        store<u32>(load<u32>(q + 24), 0);
    };
    store<u32>(q + 12, 0);
    store<u32>(q + 16, 0);
    const u32 list = load<u32>(q);
    fresh();
    store<u32>(q + 8, 0);
    for (u32 i = 0; i < load<u16>(list + 8); i = (i + 1) & 0xFFFF) {
        const u32 rec = load<u32>(load<u32>(list) + 4 * i);
        const u32 n = load<u16>(rec + 256);
        u32 sum = 0;
        for (u32 k = 0; k < n; ++k) {
            const u32 v = load<u16>(rec + 16 * k + 8);
            if (v != 0xFFFF) sum = (sum + v) & 0xFFFF;
        }
        if (((sum << 1) & 0xFFFF) == 0) continue;
        if (load<u32>(load<u32>(q + 20)) == 1) {
            call(ctx, g12::fn::DrawQueue_Submit_0051669C, {q});
            store<u32>(q + 12, 0);
            store<u32>(q + 16, 0);
            fresh();
        }
        store<u32>(q + 8, 0x8000);
        const u32 c = static_cast<u32>(call(ctx, g12::fn::MemStack_Alloc, {load<u32>(g12::g::frame_stack), 352}));
        for (u32 k = 0; k < 256; k += 16) {
            store<u32>(c + k, load<u32>(rec + k));  // lfs / stfs: raw
            store<u32>(c + k + 4, load<u32>(rec + k + 4));
            store<u16>(c + k + 8, load<u16>(rec + k + 8));
            store<u32>(c + k + 12, load<u32>(rec + k + 12));
        }
        store<u16>(c + 256, load<u16>(rec + 256));
        for (u32 off : {272u, 276u, 280u, 288u, 292u, 296u, 304u, 308u, 312u, 320u, 324u, 328u})
            store<u32>(c + off, load<u32>(rec + off));  // lfs / stfs: raw
        for (u32 k = 336; k < 340; ++k) store<u8>(c + k, load<u8>(rec + k));
        ctx.fpr[1] = lfs(rec + 340);
        store<u32>(c + 340, load<u32>(rec + 340));
        for (u32 k = 0; k < load<u16>(c + 256); k = (k + 1) & 0xFFFF) {
            const u32 slot = c + 16 * k;
            if (load<u16>(slot + 8) != 0xFFFF) call(ctx, g11::fn::cellGcmAddressToOffset, {load<u32>(slot + 4), slot + 12});
        }
        if (load<u32>(q + 12) == 0) store<u32>(q + 12, c);
        store<u32>(q + 16, c);
        const u32 cnt = load<u32>(q + 20);
        store<u32>(cnt, load<u32>(cnt) + 1);
    }
    u64 r3 = load<u32>(load<u32>(q + 20));
    if (r3 != 0) r3 = call(ctx, g12::fn::DrawQueue_Submit_0051669C, {q});
    ret(ctx, r3);
}

namespace {

// The inline base constructor RenderTarget_Init_00521110 runs twice: u16
// +4 = 1, then the word +4 = 1 << 16 | old low bits & 0x3FF; the serial
// g12::g::obj_serial += 1; +12 = 0; vtable vt_obj_base (after vt_obj_base0
// for the first object); +8 = serial << 11 | old & 0x3F; u16 +96..+110 =
// 0; two 64-byte attribute records at +128 ({u8 0, u8 1, obj_init_bytes,
// f32 50.0 at +12}); then the u32 / u8 quads at +520.. / +536.. of the
// 2 x 2 blocks (68 / 32 apart) zeroed.
void obj_init12(u32 o, bool first) {
    const u32 w4 = load<u32>(o + 4);
    store<u16>(o + 4, 1);
    store<u32>(o + 4, (1u << 16) | (w4 & 0x3FFu));
    if (first) store<u32>(o, g12::vt_obj_base0);
    const u32 serial = load<u32>(g12::g::obj_serial) + 1;
    store<u32>(g12::g::obj_serial, serial);
    const u32 w8 = load<u32>(o + 8);
    if (!first) store<u32>(o, g12::vt_obj_base);
    store<u32>(o + 12, 0);
    store<u16>(o + 96, 0);
    store<u16>(o + 98, 0);
    if (first) store<u32>(o, g12::vt_obj_base);
    store<u16>(o + 100, 0);
    store<u16>(o + 102, 0);
    store<u32>(o + 8, (serial << 11) | (w8 & 0x3Fu));
    for (u32 k = 104; k <= 110; k += 2) store<u16>(o + k, 0);
    for (u32 r = o + 128; r < o + 256; r += 64) {
        store<u8>(r, 0);
        for (u32 k = 0; k < 4; ++k) store<u8>(r + 2 + k, load<u8>(g12::g::obj_init_bytes + k));
        store<u32>(r + 12, 0x42480000u);  // 50.0f
        store<u8>(r + 1, 1);
    }
    for (u32 a = 0; a < 2; ++a)
        for (u32 k = 0; k < 2; ++k) {
            const u32 p = o + 68 * a + 32 * k;
            for (u32 b : {520u, 536u}) {
                store<u32>(p + b, 0);
                store<u32>(p + b + 4, 0);
                store<u32>(p + b + 8, 0);
                store<u8>(p + b + 12, 0);
            }
        }
}

}  // namespace

// void RenderTarget_Init_00521110(RenderTarget* rt, RenderSurface* s)
// (AglRenderTarget_New_0052317C; name?)
// The render target's base: obj_init12, u8 +48 = 1, +516 = s (u16 +4
// references += 1), +512 = s +108, RenderTarget_Setup_005203A8, name +16
// "RenderTarget_Surface" (32 bytes). g12::g::current_target == rt is
// cleared, and +52 bit 2 set when it then equals rt (never, for a real
// rt). Then its view: +672 = Mem_Alloc(1840, 0, 16), memset to 0 (even
// when the allocation failed: address 0), constructed like an
// AglRenderTarget (obj_init12, vtable vt_render_target, u64 +1808 = 0,
// u8 +1816 = 0, +1824 = 4.0, +1828 = 0.01, u8 +1776 bits 0x20 / 0x10
// cleared). Leaves r3 = the view or 0.
DECOMP(00521110, RenderTarget_Init_00521110) {
    const u32 rt = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    obj_init12(rt, true);
    store<u8>(rt + 48, 1);
    const u32 s108 = load<u32>(s + 108);
    store<u32>(rt + 516, s);
    store<u32>(rt + 512, s108);
    store<u16>(s + 4, static_cast<u16>(load<u16>(s + 4) + 1));
    call(ctx, g12::fn::RenderTarget_Setup_005203A8, {rt});
    call(ctx, g12::fn::String_Copy, {rt + 16, g12::g::str_render_target_surface, 32});
    u32 cur = load<u32>(g12::g::current_target);
    if (cur == rt) {
        store<u32>(g12::g::current_target, 0);
        cur = 0;
    }
    const u32 b52 = load<u8>(rt + 52);
    store<u8>(rt + 52, static_cast<u8>((b52 & ~4u) | (cur - rt == 0 ? 4u : 0u)));  // cntlzw / rlwimi
    const u32 mem = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {1840, 0, 16}));
    const u32 v = static_cast<u32>(call(ctx, g12::fn::memset, {mem, 0, 1840}));
    if (v != 0) {
        obj_init12(v, false);
        const u32 b1776 = load<u8>(v + 1776);
        store<u64>(v + 1808, 0);
        store<u32>(v, g12::vt_render_target);
        store<u8>(v + 1816, 0);
        store<u32>(v + 1824, 0x40800000u);  // 4.0f
        store<u32>(v + 1828, 0x3C23D70Au);  // 0.01f
        store<u8>(v + 1776, static_cast<u8>(b1776 & ~0x30u));
    }
    store<u32>(rt + 672, v);
    ret(ctx, v);
}

// void Particle_CollidePlanes_00505E90(Point* a)   (f_00506050; name?)
// The particles of a's ring (+72: {+0 base, +4 end, +8 head, +12 first,
// +20 count}, 36-byte entries) against the emitter's planes (a+4 +132:
// u32 count +8, f32 +12 bounce, f32[4] planes from +16): each particle's
// position at dt = a+8 - p+12 (Particle_PositionAt_00505D50; f2 = a+4
// +8) behind a plane (n.p + d < the constant 0x00505E88) bounces
// (Particle_Integrate_00505C30). Skipped without planes or a+24. Leaves
// r3 / f1 as the original does.
DECOMP(00505E90, Particle_CollidePlanes_00505E90) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 o = load<u32>(load<u32>(a + 4) + 132);
    if (load<u32>(o + 8) == 0 || load<u32>(a + 24) == 0) return ret(ctx, 0u);
    const u32 ring = load<u32>(a + 72);
    u32 p = load<u32>(ring + 20) != 0 ? load<u32>(ring + 12) : 0u;
    const f64 limit = lfs(0x00505E88);
    Stack frame(ctx, 80);  // the original's sp+112..191
    const u32 pos = frame.addr();
    u32 cur = 0;
    do {
        const f64 now = lfs(a + 8);
        const f64 dt = sgl(now - lfs(p + 12));
        ctx.fpr[1] = now;
        u32 pl = o + 16;
        for (u32 n = load<u32>(o + 8); n != 0; --n, pl += 16) {
            const u32 e = load<u32>(a + 4);
            callf(ctx, g12::fn::Particle_PositionAt_00505D50, {a, p, pos, e}, {dt, lfs(e + 8)});
            f64 d = sgl(lfs(pl + 4) * lfs(pos + 4));
            d = g11::madds(lfs(pl), lfs(pos), d);
            d = g11::madds(lfs(pl + 8), lfs(pos + 8), d);
            d = sgl(d + lfs(pl + 12));
            ctx.fpr[1] = d;
            if (d < limit) callf(ctx, g12::fn::Particle_Integrate_00505C30, {a, p, pos, pl}, {lfs(o + 12)});
        }
        const u32 r = load<u32>(a + 72);
        const u32 next = p + 36;
        cur = static_cast<s32>(next - load<u32>(r + 4)) < 0 ? next : load<u32>(r);  // wrap at the end
        p = cur != load<u32>(r + 8) ? cur : 0u;                                      // stop at the head
    } while (p != 0);
    ret(ctx, cur);
}

// s32 InflateQueue_Wait_007A6498(InflateQueue* q, Job* j, u32* out)
// [InflateQueue vtable 0x00B24558 / 0x00B245A4 slot 5] (name?)
// Waits for job j (44-byte jobs from q+11268; event bit 1 << index) while
// its +40 is set (cellSpursEventFlagWait(q+10624, &bits, 1)); copies its
// result (+28 dst, +20 + +32 src, +36 bytes) when both are set; *out =
// +36 (with a dst) or +24; clears the job and atomically its bit in
// q+11264. Returns 0, or -1 without a job.
DECOMP(007A6498, InflateQueue_Wait_007A6498) {
    const u32 q = arg<u32>(ctx, 0), j = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    if (j == 0) return ret(ctx, -1);
    const s32 idx = static_cast<s32>(j - (q + 11268)) / 44;
    const u32 sh = static_cast<u32>(idx) & 63;
    const u32 bit = sh < 32 ? 1u << sh : 0u;  // slw
    Stack frame(ctx, 48);  // the original's sp+112..159
    const u32 bits = frame.addr();
    store<u16>(bits, static_cast<u16>(bit));
    while (load<u32>(j + 40) != 0) call(ctx, g12::fn::cellSpursEventFlagWait, {q + 10624, bits, 1});
    u32 dst = load<u32>(j + 28);
    if (dst != 0 && load<u32>(j + 20) != 0) {
        call(ctx, g12::fn::memmove, {dst, load<u32>(j + 20) + load<u32>(j + 32), load<u32>(j + 36)});
        dst = load<u32>(j + 28);
    }
    store<u32>(out, dst != 0 ? load<u32>(j + 36) : load<u32>(j + 24));
    call(ctx, g12::fn::memset, {j, 0, 44});
    std::atomic_ref<u32> word(ref<u32>(q + 11264));  // lwarx / stwcx.: *+11264 &= ~bit
    for (;;) {
        std::atomic_thread_fence(std::memory_order_release);  // lwsync
        const u32 seen = load<u32>(q + 11264);
        u32 expect = __builtin_bswap32(seen);
        if (word.compare_exchange_strong(expect, __builtin_bswap32(seen & ~bit))) break;
    }
    ret(ctx, 0);
}

// VertexFormat* MeshFormat_Get_0051B5C0(bool second)   (Mesh_CreateSphere,
// Mesh_CreateCube, f_0051B788; name?)
// The two built-in mesh vertex formats (g12::g::mesh_formats, 128 bytes
// each), built once from the templates: stride +120 = 24, +124 |= 0x8000,
// +126 = the OR of 1 << (register base + register) per attribute, the
// 12-byte attributes copied up to and with the one whose +4 is 0.
DECOMP(0051B5C0, MeshFormat_Get_0051B5C0) {
    const bool second = (arg<u32>(ctx, 0) & 0xFF) != 0;
    if (load<u8>(g12::g::mesh_formats_ready) == 0) {
        const u32 tpl[2] = {g12::g::mesh_format_tpl0, g12::g::mesh_format_tpl1};
        for (u32 f = 0; f < 2; ++f) {
            const u32 fmt = g12::g::mesh_formats + 128 * f;
            const u32 flags = load<u16>(fmt + 124);
            if (f == 0) store<u8>(g12::g::mesh_formats_ready, 1);
            store<u16>(fmt + 120, 24);
            store<u16>(fmt + 126, 0);
            store<u16>(fmt + 124, static_cast<u16>(flags | 0x8000));
            for (u32 src = tpl[f], dst = fmt;; src += 12, dst += 12) {
                const u32 type = load<u32>(src + 4), base = load<u32>(src);
                store<u32>(dst + 4, type);
                store<u32>(dst, base);
                store<u8>(dst + 8, load<u8>(src + 8));
                const u32 reg = load<u8>(src + 9);
                store<u8>(dst + 9, static_cast<u8>(reg));
                store<u8>(dst + 10, load<u8>(src + 10));
                if (type == 0) break;
                const u32 sh = (base + reg) & 0x3F;  // slw
                store<u16>(fmt + 126, static_cast<u16>(load<u16>(fmt + 126) | (sh < 32 ? 1u << sh : 0u)));
            }
        }
    }
    ret(ctx, second ? g12::g::mesh_formats + 128 : g12::g::mesh_formats);
}

namespace {

// ApeTListAllocator<T>::GetAllocator() inlined: the free list at slot +0,
// built on first use as a 60-byte ApeTFreeList {vtable vt, +4..+52
// cleared, bit 0x8000 of u16 +54 cleared, +56 the signature name} and
// FreeList_Init(list, elem_size, slot +4 heap, 0, 0); a failed allocation
// leaves it null.
u32 get_list12(PPUContext& ctx, u32 slot, u32 vt, u32 name, u32 elem_size) {
    u32 fl = load<u32>(slot);
    if (fl == 0) {
        const u32 p = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {60, 0, 16}));
        if (p != 0) {
            const u32 heap = load<u32>(slot + 4);
            for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(p + off, 0);
            const u32 h = load<u16>(p + 54) & ~0x8000u;
            store<u16>(p + 52, 0);
            store<u16>(p + 54, static_cast<u16>(h));
            store<u32>(p, vt);
            store<u32>(p + 56, name);
            call(ctx, g12::fn::FreeList_Init, {p, elem_size, heap, 0, 0});
        }
        fl = p;
        store<u32>(slot, fl);
    }
    return fl;
}

}  // namespace

// bool Iff_HandleSubMesh_00524500(IffReader* r, Topology* t)   [an IFF
// chunk handler] (name?)
// A new AglSubMesh from its ApeTListAllocator (the free list
// *g12::g::submesh_list, built on first use: a 60-byte ApeTFreeList,
// FreeList_Init(list, 116, heap, 0, 0)) through virtual slot 3 (+12),
// SubMesh_Construct5386DC; Topology_AddDraw(t, it, its +16), its virtual
// slot 2 (+8), then its chunks. Returns 1. Quirk: no allocator / list /
// entry carries on with 0.
DECOMP(00524500, Iff_HandleSubMesh_00524500) {
    const u32 r = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    const u32 slot = load<u32>(g12::g::submesh_list);
    u32 sm = 0;
    if (slot != 0) {
        const u32 fl = get_list12(ctx, slot, g12::g::vt_submesh_list, g12::g::str_submesh_list, 116);
        const u32 e = static_cast<u32>(fx::vcall(ctx, fl, 12, {fl}));
        if (e != 0) {
            call(ctx, g12::fn::SubMesh_Construct5386DC, {e});
            sm = e;
        }
    }
    call(ctx, g12::fn::Topology_AddDraw, {t, sm, load<u32>(sm + 16)});
    fx::vcall(ctx, sm, 8, {sm});
    call(ctx, g12::fn::Iff_ReadChunks, {r, sm});
    ret(ctx, 1);
}

// void LightList_Snapshot_005ACDE4()   (sub_0028E320; name?)
// Copies the frame's lights: the u16 counts g12::g::light_counts[3] to
// light_counts_copy, each of the first count[0] lights (light_src[i].+0)
// into the 144-byte light_copies[i] (u8 +0, f32 +16..+24 / +32.. / +48..
// / +64.. / +80.., u32 +4 / +96, f32 +92, u8 +100..+103, f32 +104..+116;
// +132 / +136 = 0; +128 = light_src[i].+4); then the counts cleared.
// Leaves r3 = 0x01190000.
DECOMP(005ACDE4, LightList_Snapshot_005ACDE4) {
    const u32 n = load<u16>(g12::g::light_counts);
    const u16 n1 = load<u16>(g12::g::light_counts + 2), n2 = load<u16>(g12::g::light_counts + 4);
    store<u16>(g12::g::light_counts_copy, static_cast<u16>(n));
    store<u16>(g12::g::light_counts_copy + 2, n1);
    store<u16>(g12::g::light_counts_copy + 4, n2);
    for (u32 i = 0; i < n; i = (i + 1) & 0xFFFF) {
        const u32 e = g12::g::light_src + 8 * i, d = g12::g::light_copies + 144 * i;
        const u32 s = load<u32>(e);
        store<u8>(d, load<u8>(s));
        for (u32 off : {16u, 20u, 24u, 32u, 36u, 40u, 48u, 52u, 56u, 4u, 64u, 68u, 72u, 80u, 84u, 88u, 96u, 92u})
            store<u32>(d + off, load<u32>(s + off));  // lfs / stfs / lwz / stw: raw
        for (u32 k = 100; k < 104; ++k) store<u8>(d + k, load<u8>(s + k));
        for (u32 off : {104u, 108u, 112u}) store<u32>(d + off, load<u32>(s + off));
        store<u32>(d + 132, 0);
        store<u32>(d + 116, load<u32>(s + 116));
        ctx.fpr[1] = lfs(e + 4);
        store<u32>(d + 136, 0);
        store<u32>(d + 128, load<u32>(e + 4));
    }
    store<u16>(g12::g::light_counts + 4, 0);
    store<u16>(g12::g::light_counts + 2, 0);
    store<u16>(g12::g::light_counts, 0);
    ret(ctx, 0x01190000u);  // r3 as the original leaves it
}

// void DrawRecordList_Reset_0050F9A4()   (f_0050FB5C; name?)
// The draw-record list (g12::g::draw_records, set up on first use as
// Topology_Apply does: u8 +0 = 1, PtrVector +4 (u16 +10 = 160), +32 = 0,
// +128 / +256 atomically 0): u16 +12 = 0; the PtrVector* +24 cleared
// and freed, the 160-byte block +28 (its pool parts +128 / +144, twice,
// the second time in reverse; the guard byte draw_records_guard2 set)
// freed; then +32's virtual slot 2 (+8). Leaves r3 as that call does.
DECOMP(0050F9A4, DrawRecordList_Reset_0050F9A4) {
    constexpr u32 L = g12::g::draw_records;
    if (load<u8>(g12::g::draw_records_ready) == 0) {
        store<u8>(L, 1);
        call(ctx, g12::fn::PtrVector_Construct, {L + 4});
        store<u16>(L + 10, 160);
        store<u32>(L + 32, 0);
        std::atomic_ref<u32>(ref<u32>(L + 128)).exchange(0);  // lwarx / stwcx.
        std::atomic_ref<u32>(ref<u32>(L + 256)).exchange(0);
        store<u8>(g12::g::draw_records_ready, 1);
    }
    store<u16>(L + 12, 0);
    if (const u32 v = load<u32>(L + 24); v != 0) {
        call(ctx, g8::fn::PtrVector_Clear, {v});
        call(ctx, g8::fn::Nop, {v});
        call(ctx, g8::fn::Mem_FreeFrom, {v, 0});
        store<u32>(L + 24, 0);
    }
    if (const u32 b = load<u32>(L + 28); b != 0) {
        call(ctx, sly4::render::gfx9::fn::Pool_DestructPart, {b + 128});
        call(ctx, sly4::render::gfx9::fn::Pool_DestructPart, {b + 144});
        if (load<u8>(g12::g::draw_records_guard2) == 0) store<u8>(g12::g::draw_records_guard2, 1);
        call(ctx, sly4::render::gfx9::fn::Pool_DestructPart, {b + 144});
        call(ctx, sly4::render::gfx9::fn::Pool_DestructPart, {b + 128});
        call(ctx, g8::fn::Mem_FreeFrom, {b, 0});
        store<u32>(L + 28, 0);
    }
    const u32 o = load<u32>(L + 32);
    ret(ctx, fx::vcall(ctx, o, 8, {o}));
}

// RenderState* RenderState_Invalidate_0051038C()   (f_000AD910, f_0049AE40;
// name?)
// Forgets the cached GCM state: when the display (draw::g::display) has
// +52 bits 22..24 set, u8 +136 clear and u8 +137 set, it is rebound
// first (Display_Prepare_005358A8(3), Display_BindScreenTargets, the two
// u64s at 0x010DD598 = -1). Then display +136 = 0, the screen's +184 = 2,
// Surface_Bind_0053B668(display, 1); render state (fx::g::state) +4 /
// +16..+28 / +52..+80 / +88 = 0, +8 / +216 = -1, u16 +32 = 0xFFFF, u8
// +220 / +221 = 0, +92 bit 31 cleared, and the stencil defaults (front
// +160.., back +188..: func ALWAYS, ref 0, masks -1, ops KEEP). Returns
// the render state.
DECOMP(0051038C, RenderState_Invalidate_0051038C) {
    const u32 d = load<u32>(sly4::draw::g::display);
    if (((load<u32>(d + 52) >> 22) & 7) != 0 && load<u8>(d + 136) == 0 && load<u8>(d + 137) != 0) {
        call(ctx, g12::fn::Display_Prepare_005358A8, {3});
        call(ctx, g12::fn::Display_BindScreenTargets);
        store<u64>(g12::g::frame_cache_dd598, ~u64{0});
        store<u64>(g12::g::frame_cache_dd598 + 8, ~u64{0});
    }
    store<u8>(d + 136, 0);
    store<u32>(g12::g::screen + 184, 2);
    call(ctx, g12::fn::Surface_Bind_0053B668, {d, 1});
    constexpr u32 S = fx::g::state;
    constexpr u32 kAlways = 0x0207, kKeep = 0x1E00, kAll = 0xFFFFFFFFu;
    store<u32>(S + 4, 0);
    store<u32>(S + 8, kAll);
    for (u32 off = 16; off <= 28; off += 4) store<u32>(S + off, 0);
    store<u16>(S + 32, 0xFFFF);
    store<u32>(S + 88, 0);
    for (u32 off = 52; off <= 80; off += 4) store<u32>(S + off, 0);
    store<u32>(S + 216, kAll);
    store<u8>(S + 220, 0);
    store<u8>(S + 221, 0);
    const u32 w92 = load<u32>(S + 92) & 0x7FFFFFFFu;
    store<u32>(S + 160, kAlways);
    store<u32>(S + 164, 0);
    store<u32>(S + 168, kAll);
    store<u32>(S + 172, kAll);
    store<u32>(S + 92, w92);
    for (u32 off = 176; off <= 184; off += 4) store<u32>(S + off, kKeep);
    store<u32>(S + 188, kAlways);
    store<u32>(S + 192, 0);
    store<u32>(S + 196, kAll);
    store<u32>(S + 200, kAll);
    for (u32 off = 204; off <= 212; off += 4) store<u32>(S + off, kKeep);
    ret(ctx, S);
}

// void RenderTargets_Update_00511F8C()   (Render_Phase_005124F4; name?)
// Every live render target (u16 +4 > 0, signed) of the AglRenderTarget
// allocator's list (get_list12; nodes linked through -16): View_Update
// (rt, 0) then RenderTarget_Draw_005109B0(rt). Quirk: the allocator
// pointer itself is not checked. Leaves r3 = 0.
DECOMP(00511F8C, RenderTargets_Update_00511F8C) {
    const u32 fl = get_list12(ctx, load<u32>(g12::g::render_targets), g12::g::vt_render_target_list,
                              g12::g::str_render_target_list, 1840);
    const u32 first = static_cast<u32>(call(ctx, g12::fn::Obj_GetField4, {fl}));
    auto next = [](u32 rt) {
        const u32 n = load<u32>(rt - 16);
        return n != 0 ? n + 16 : 0u;
    };
    u32 rt = first != 0 ? first + 16 : 0u;
    while (rt != 0 && static_cast<s16>(load<u16>(rt + 4)) <= 0) rt = next(rt);
    while (rt != 0) {
        call(ctx, g12::fn::View_Update, {rt, 0});
        call(ctx, g12::fn::RenderTarget_Draw_005109B0, {rt});
        do rt = next(rt);
        while (rt != 0 && static_cast<s16>(load<u16>(rt + 4)) <= 0);
    }
    ret(ctx, 0u);
}

namespace {

// The texture LoadParams binds after its params: added to m
// (Material_AddTexture_004FF008), then sized to m+20's u16 +122 / +124
// through its virtual slot 4 (+16): (t, w, h, 1).
void add_sized_texture12(PPUContext& ctx, u32 m, u32 t) {
    call(ctx, g11::fn::Material_AddTexture_004FF008, {m, t});
    const u32 sz = load<u32>(m + 20);
    fx::vcall(ctx, t, 16, {t, load<u16>(sz + 122), load<u16>(sz + 124), 1});
}

// The LoadParams body 005CA818, 005C95FC and 005B2DF0 share: unless
// config +152 bit 7, with u8 d+84 set, the texture named d+8
// (Tex_FindLoaded) added to m and sized as add_sized_texture12, but with
// the registers after the four arguments as each compiled sequence
// leaves them (`tail`: r7, r8 (, r9); kOpd / kW / kH stand for the OPD
// and the sizes); params 0..2 = d+88 / +92 / +96 (Material_ParamSlot);
// then unless config +152 bit 6 the texture named d+12
// (add_sized_texture12). `after` runs once past the first texture's
// lookup, whichever way the rest goes. Returns 1.
constexpr u64 kOpd = ~u64{0}, kW = ~u64{0} - 1, kH = ~u64{0} - 2;
template <typename After>
void load_params12(PPUContext& ctx, u32 m, u32 d, std::initializer_list<u64> tail, After after) {
    constexpr u32 kCfg = sly4::g::config;
    if ((load<u8>(kCfg + 152) & 0x80) != 0) return ret(ctx, 1);
    if (load<u8>(d + 84) == 0) return ret(ctx, 1);
    const u32 t = static_cast<u32>(call(ctx, g11::fn::Tex_FindLoaded, {load<u32>(d + 8)}));
    if (t != 0) {
        call(ctx, g11::fn::Material_AddTexture_004FF008, {m, t});
        {
            const u32 sz = load<u32>(m + 20);
            const u32 w = load<u16>(sz + 122), h = load<u16>(sz + 124);
            const u32 opd = load<u32>(load<u32>(t) + 16);
            u64 r[3];
            u32 n = 0;
            for (u64 v : tail) r[n++] = v == kOpd ? opd : v == kW ? w : v == kH ? h : v;
            if (n == 3) call_ptr(ctx, opd, {t, w, h, 1, r[0], r[1], r[2]});
            else call_ptr(ctx, opd, {t, w, h, 1, r[0], r[1]});
        }
        for (u32 k = 0; k < 3; ++k) {
            const u32 v = load<u32>(d + 88 + 4 * k);  // lfs / stfs: raw
            const u32 slot = static_cast<u32>(call(ctx, g12::fn::Material_ParamSlot, {m, k}));
            store<u32>(slot, v);
        }
        if ((load<u8>(kCfg + 152) & 0x40) == 0) {
            const u32 t2 = static_cast<u32>(call(ctx, g11::fn::Tex_FindLoaded, {load<u32>(d + 12)}));
            if (t2 != 0) add_sized_texture12(ctx, m, t2);
        }
    }
    after();
    ret(ctx, 1);
}

}  // namespace

// bool LoadParams_005CA818(ShaderType*, Material* m, const Desc* d)
// [a shader type's LoadParams] (name?)
// load_params12; the first texture's call leaves r7 = the OPD, r8 / r9 =
// the two sizes. (Without the first texture it returns at once.)
DECOMP(005CA818, LoadParams_005CA818) { load_params12(ctx, arg<u32>(ctx, 1), arg<u32>(ctx, 2), {kOpd, kW, kH}, [] {}); }

// bool LoadParams_005C95FC(ShaderType*, Material* m, const Desc* d)
// [a shader type's LoadParams] (name?)
// The default texture 5 (Tex_GetDefault) added to m first, then
// load_params12; the first texture's call leaves r7 = the height, r8 =
// the width.
DECOMP(005C95FC, LoadParams_005C95FC) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    call(ctx, g11::fn::Material_AddTexture_004FF008, {m, static_cast<u32>(call(ctx, g12::fn::Tex_GetDefault, {5}))});
    load_params12(ctx, m, d, {kH, kW}, [] {});
}

// bool Iff_HandleSkinWeights_00525A24(IffReader* r, Mesh* o)   [an IFF
// chunk handler] (name?)
// o+72 = Mem_Alloc(8 * o+40, 0, 16): per vertex u8 bones[4], u8
// weights[4]. A chunk of 12 bytes per vertex (u8 bones[4], u16
// weights[4], byte-swapped in place when the reader's u8 +12 bit 1 is
// set) is read into default-stack memory and narrowed (the weights'
// high bytes), the memory given back (its virtual slot 2); a chunk of 8
// per vertex is read directly. Then Mesh_SetFlag_004FCD78(o, 3).
// Returns 1.
DECOMP(00525A24, Iff_HandleSkinWeights_00525A24) {
    const u32 r = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    store<u32>(o + 72, static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {(load<u32>(o + 40) << 3) & ~7u, 0, 16})));
    const u32 size = static_cast<u32>(call(ctx, g12::fn::Iff_ChunkSize, {r}));
    const u32 n = load<u32>(o + 40);
    if (static_cast<s32>(size) == static_cast<s32>(n * 12)) {
        const u32 stk = static_cast<u32>(call(ctx, g12::fn::Mem_GetDefaultStack));
        const u32 buf = static_cast<u32>(call(ctx, g12::fn::MemStack_Alloc, {stk, size}));
        call(ctx, g12::fn::FileReader_Read, {r, buf, size});
        const u32 dst = load<u32>(o + 72);
        for (u32 i = 0; i < load<u32>(o + 40); ++i) {
            const u32 s = buf + 12 * i, d = dst + 8 * i;
            for (u32 k = 0; k < 4; ++k) {
                const u32 w = s + 4 + 2 * k;
                if ((load<u8>(r + 12) & 2) != 0) store<u16>(w, static_cast<u16>((load<u8>(w + 1) << 8) | load<u8>(w)));
                store<u8>(d + k, load<u8>(s + k));
                store<u8>(d + 4 + k, static_cast<u8>(load<u16>(w) >> 8));
            }
        }
        const u32 st = static_cast<u32>(call(ctx, g12::fn::Mem_GetDefaultStack));
        fx::vcall(ctx, st, 8, {st, buf});
    } else if (static_cast<s32>(size) == static_cast<s32>(n << 3)) {
        call(ctx, g12::fn::FileReader_Read, {r, load<u32>(o + 72), size});
    }
    call(ctx, g12::fn::Mesh_SetFlag_004FCD78, {o, 3});
    ret(ctx, 1);
}

// RenderState_Reset_00517FB8()   (ResetGcmState_00518180, f_00518E78;
// name?)
// Back to the default GCM state: Draw_BindScreenTargets, the texture
// cache invalidated, the copy programs (fx::g::vp_copy / fp_copy) made
// current in the render state cache (+20 / +24: Gcm_SetVertexProgramAt
// of its +20 when the ucode +4 differs, +8 = -1; +28 / +92 bit 31),
// Draw_ResetStateCall(0, 0), blending on (ONE, ONE / SRC_ALPHA, FUNC_ADD),
// colour mask RGB, texture unit 0 = g12::g::reset_texture, the quad /
// depth thunks, colour mask RGBA. Leaves r3 = the GCM context.
DECOMP(00517FB8, RenderState_Reset_00517FB8) {
    constexpr u32 S = fx::g::state;
    call(ctx, g12::fn::Draw_BindScreenTargets);
    call(ctx, fx::fn::cellGcmSetInvalidateTextureCache, {load<u32>(S), 1});
    const u32 vp = load<u32>(fx::g::vp_copy);
    if (load<u32>(S + 20) != vp) {
        store<u32>(S + 20, vp);
        store<u32>(S + 8, 0xFFFFFFFFu);
        const u32 ucode = load<u32>(vp + 4);
        if (load<u32>(S + 24) != ucode) {
            const u32 c = load<u32>(S);
            store<u32>(S + 24, ucode);
            call(ctx, g12::fn::Gcm_SetVertexProgramAt, {c, load<u32>(vp + 20)});
        }
    }
    const u32 fp = load<u32>(fx::g::fp_copy);
    if (load<u32>(S + 28) != fp) {
        const u32 w92 = load<u32>(S + 92);
        store<u32>(S + 28, fp);
        store<u32>(S + 92, w92 | 0x80000000u);
    }
    call(ctx, fx::fn::Draw_ResetStateCall, {0, 0});
    auto emit2 = [](u32 m, u32 v) {
        const u32 c = load<u32>(S), cur = load<u32>(c + 8);
        store<u32>(cur, m);
        store<u32>(cur + 4, v);
        store<u32>(c + 8, cur + 8);
    };
    emit2(fx::kBlendEnable, 1);
    {
        const u32 c = load<u32>(S), cur = load<u32>(c + 8);
        store<u32>(cur, fx::kBlendFunc);
        store<u32>(cur + 4, fx::kOne2);
        store<u32>(cur + 8, fx::kSrcAlpha2);
        store<u32>(c + 8, cur + 12);
    }
    emit2(fx::kBlendEquation, fx::kFuncAdd2);
    {
        const u32 c = load<u32>(S), cur = load<u32>(c + 8);
        store<u32>(cur, fx::kColorMask);
        store<u32>(c + 8, cur + 8);
        store<u32>(cur + 4, 0x00010101u);  // R, G, B
    }
    call(ctx, fx::fn::Draw_SetTexture0Call, {load<u32>(g12::g::reset_texture), 0, 1});
    call(ctx, g11::fn::Thunk_PrepareQuad_0053C850);
    call(ctx, g11::fn::Thunk_RestoreDepth_0053C830);
    const u32 c = load<u32>(S), cur = load<u32>(c + 8);
    store<u32>(cur, fx::kColorMask);
    store<u32>(cur + 4, 0x01010101u);  // R, G, B, A
    store<u32>(c + 8, cur + 8);
    ret(ctx, c);
}

// u32 DrawRing_Init_0050EB08(DrawRing* o, u32 size, u32 count, u8 buffers,
// u8 b)   (FrameRing_Construct, f_0050ECD4; name?)
// o+8 = size rounded up to 16, +0 / +12 / +16 = Mem_Alloc(that, 0, 16),
// +4 = its end; the two 16-byte buffer parts +20 / +36 reset ({0, 1, 2,
// tag bits 28..31 of +12 cleared}); +52 = count, u8 +56
// = buffers, +57 / +58 = 0, u8 +76 = b, u16 +164..+170 / +202..+208 = 0;
// each of the buffers parts (from +20, 16 apart) gets 24 * count bytes
// (Buffer_Alloc52F198(part, .., 128, 1, 2)); then +80..+88 / +160 / +192
// = -1, +96 = 100000.0, u64 +176 / +184 = -1, u16 +164 = 0xFFFF, +196 =
// -1.0, u8 +200 = 0. Leaves r3 = 0xFFFF.
DECOMP(0050EB08, DrawRing_Init_0050EB08) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 count = ctx.gpr[5];
    const u32 nb = arg<u32>(ctx, 3) & 0xFF, b = arg<u32>(ctx, 4) & 0xFF;
    store<u32>(o + 8, (arg<u32>(ctx, 1) + 15) & ~15u);
    const u32 p = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {load<u32>(o + 8), 0, 16}));
    const u32 sz = load<u32>(o + 8);
    store<u32>(o, p);
    store<u32>(o + 16, p);
    store<u32>(o + 12, p);
    store<u32>(o + 4, p + sz);
    for (u32 part = o + 20; part < o + 52; part += 16) {
        // rlwinm keeps only bit 0 of +4 / bits 1..3 of +8, then 1 / 1 << 1 go in:
        // the words become 1 and 2 whatever they held.
        const u32 w4 = (load<u32>(part + 4) & 1) | 1, w8 = ((load<u32>(part + 8) & 0xEu) & ~0xEu) | (1u << 1);
        const u32 w12 = load<u32>(part + 12) & 0x0FFFFFFFu;
        store<u32>(part, 0);
        store<u32>(part + 4, w4);
        store<u32>(part + 8, w8);
        store<u32>(part + 12, w12);
    }
    store<u32>(o + 52, static_cast<u32>(count));
    store<u8>(o + 56, static_cast<u8>(nb));
    store<u8>(o + 57, 0);
    store<u8>(o + 58, 0);
    store<u8>(o + 76, static_cast<u8>(b));
    for (u32 off : {164u, 166u, 168u, 170u, 202u, 204u, 206u, 208u}) store<u16>(o + off, 0);
    const u32 bytes = static_cast<u32>((count << 3) * 3);  // rldicr: the full register
    if (nb != 0)
        for (u32 k = 0; k < load<u8>(o + 56); ++k) call(ctx, g12::fn::Buffer_Alloc52F198, {o + 20 + 16 * k, bytes, 128, 1, 2});
    store<u32>(o + 80, 0xFFFFFFFFu);
    store<u32>(o + 84, 0xFFFFFFFFu);
    store<u32>(o + 88, 0xFFFFFFFFu);
    store<u32>(o + 96, 0x47C35000u);  // 100000.0f
    store<u32>(o + 160, 0xFFFFFFFFu);
    store<u64>(o + 176, ~u64{0});
    store<u16>(o + 164, 0xFFFF);
    store<u64>(o + 184, ~u64{0});
    store<u32>(o + 192, 0xFFFFFFFFu);
    store<u32>(o + 196, 0xBF800000u);  // -1.0f
    store<u8>(o + 200, 0);
    ret(ctx, 0xFFFFu);
}

// void Display_BindAsTexture_00534DB4(RenderSurface* d, u32 unit)
// (f_005358A8; name?)
// The display's colour surface as texture `unit` (u8): filter (0, 1, 1,
// 1), control (on, lod 0..15 << 8, no aniso), address (3, 3, 3, 0, 1,
// 0) and the CellGcmTexture {A8R8G8B8 | LN | NR (0xA5), 1 mipmap, 2D,
// remap 0xAA93, width config +4 * 2, height config +6 (* 2 unless the
// display's +52 bits 22..24 are 2), depth 1, location / pitch / offset
// from the colour target (+132 with config +34 (stereo), else +80): its
// +4 bit 0, +32, +4 >> 1}; then render state +52 + 4 * unit = 0 (the
// texture cache). Leaves r3 = &render state +52.
DECOMP(00534DB4, Display_BindAsTexture_00534DB4) {
    const u32 d = arg<u32>(ctx, 0);
    const u32 unit = arg<u32>(ctx, 1), idx = unit & 0xFF;
    constexpr u32 S = fx::g::state, kCfg = sly4::g::config;
    call(ctx, g12::fn::cellGcmSetTextureFilter, {load<u32>(S), idx, 0, 1, 1, 1});
    call(ctx, g12::fn::cellGcmSetTextureControl, {load<u32>(S), idx, 1, 0, 3840, 0});
    call(ctx, g12::fn::cellGcmSetTextureAddress, {load<u32>(S), idx, 3, 3, 3, 0, 1, 0});
    Stack frame(ctx, 64);  // the original's sp+112..175
    const u32 tex = frame.addr();
    store<u8>(tex, 0xA5);
    store<u8>(tex + 1, 1);
    store<u8>(tex + 2, 2);
    store<u8>(tex + 3, 0);
    store<u32>(tex + 4, 0xAA93);
    const u32 mode = (load<u32>(d + 52) >> 22) & 7;
    const u32 c = load<u32>(S);
    const u8 stereo = load<u8>(kCfg + 34);
    store<u16>(tex + 8, static_cast<u16>(load<u16>(kCfg + 4) << 1));
    store<u16>(tex + 10, static_cast<u16>(mode == 2 ? load<u16>(kCfg + 6) : load<u16>(kCfg + 6) << 1));
    store<u16>(tex + 12, 1);
    const u32 target = d + (stereo != 0 ? 132 : 80);
    store<u32>(tex + 16, load<u32>(load<u32>(target) + 32));
    store<u8>(tex + 14, static_cast<u8>(load<u32>(load<u32>(target) + 4) & 1));
    store<u8>(tex + 15, 0);
    store<u32>(tex + 20, load<u32>(load<u32>(target) + 4) >> 1);
    call(ctx, g12::fn::cellGcmSetTexture, {c, idx, tex});
    store<u32>(S + 52 + static_cast<u32>(static_cast<s32>(unit << 2)), 0);  // rldicr / extsw: the full unit
    ret(ctx, S + 52);
}

namespace {

// The display's colour target for the current eye / buffer set: display
// +148 bit 7 takes the screen's +128; with config +34 (stereo) the
// screen's +112 / +120 (screen +184 == 1 or not) + (display +52 bits
// 30..31) * 4; else the screen's +92 + that.
u32 display_colour12() {
    const u32 disp = load<u32>(sly4::draw::g::display), scr = g12::g::screen;
    if ((load<u8>(disp + 148) & 0x80) != 0) return load<u32>(scr + 128);
    const u32 sel = std::rotl(load<u32>(disp + 52), 4) & 0xC;  // rlwinm 4,28,29
    if (load<u8>(sly4::g::config + 34) == 0) return load<u32>(scr + sel + 92);
    return load<u32>(scr + sel + (load<u32>(scr + 184) == 1 ? 112 : 120));
}

}  // namespace

// void Bloom_Update_0053DF98(Bloom* o)   [called through a pointer] (name?)
// When o's virtual slot 3 (+12) says it is on and the strength o+48 is
// not below the constant 0x0053DF94: Bloom_Chain({o+32 xyz, o+48, the
// display's colour target, that target's +40 (looked up again)}), the
// "used" byte g12::g::bloom_used set first. Leaves r3 / f1 as the
// original does.
DECOMP(0053DF98, Bloom_Update_0053DF98) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 on = fx::vcall(ctx, o, 12, {o});
    if (static_cast<u32>(on) == 0) return ret(ctx, on);
    Stack frame(ctx, 48);  // the original's sp+112..159
    const u32 a = frame.addr();
    const u32 strength = load<u32>(o + 48);
    ctx.fpr[1] = lfs(o + 48);
    store<u32>(a + 16, strength);  // lfs / stfs: raw
    for (u32 k = 0; k < 12; k += 4) store<u32>(a + k, load<u32>(o + 32 + k));
    store<u32>(a + 20, display_colour12());
    const u32 t40 = load<u32>(display_colour12() + 40);
    constexpr u32 kMin = 0x0053DF94;
    if (lfs(o + 48) < lfs(kMin)) return ret(ctx, kMin);
    store<u32>(a + 24, t40);
    if (load<u8>(g12::g::bloom_used) == 0) store<u8>(g12::g::bloom_used, 1);
    ret(ctx, call(ctx, g12::fn::Bloom_Chain, {a}));
}

namespace {

// Appends s to the shader type's list (PtrVector +12: +12 data, u16 +16
// capacity, +20 count, +22 high-water mark), doubling the capacity when
// full (shader3_types.cpp's push). Returns the old high-water mark (the
// original's r3).
u32 push_shader12(PPUContext& ctx, u32 type, u32 s) {
    if (load<u16>(type + 20) == load<u16>(type + 16))
        call(ctx, g12::fn::PtrVector_Reserve, {type + 12, (static_cast<u32>(load<u16>(type + 16)) << 1) & 0x1FFFEu});
    const u32 count = load<u16>(type + 20);
    store<u32>(load<u32>(type + 12) + ((count << 2) & 0x3FFFCu), s);
    const u32 n = (load<u16>(type + 20) + 1u) & 0xFFFF;
    const u32 high = load<u16>(type + 22);
    store<u16>(type + 20, static_cast<u16>(n));
    store<u16>(type + 22, static_cast<u16>(static_cast<s32>(high) > static_cast<s32>(n) ? high : n));
    return high;
}

// A new 44-byte shader (vtable vt, programs vname / fname) appended to
// the type; a failed allocation appends 0.
u32 add_shader12(PPUContext& ctx, u32 type, u32 vt, u32 vname, u32 fname, u32 vparams, u32 fparams) {
    const u32 s = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {44, 0, 16}));
    if (s != 0) {
        call(ctx, g12::fn::Shader_Construct, {s});
        store<u8>(s + 12, 0);
        store<u32>(s, vt);
        call(ctx, g12::fn::Shader_Load, {s, vname, fname, vparams, fparams});
    }
    return push_shader12(ctx, type, s);
}

}  // namespace

// void SHOTextureWarpType_Create(ShaderType* t)   [ShaderType slot 0]
// The type's two shaders: "SHO_TextureWarp" and "SHO_TextureWarp_Lit"
// (no fragment program name; vtable 0x00B26178, parameters 0x00F8B010 /
// 0x00F8B000). Leaves r3 = the list's old high-water mark.
DECOMP(005D7BD0, SHOTextureWarpType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add_shader12(ctx, t, 0x00B26178, 0x00AF8348, 0, 0x00F8B010, 0x00F8B000);  // "SHO_TextureWarp"
    ret(ctx, add_shader12(ctx, t, 0x00B26178, 0x00AF8358, 0, 0x00F8B010, 0x00F8B000));  // "SHO_TextureWarp_Lit"
}

// void Job_SetView_005380D0(Job* j, RenderTarget* rt)   (Call_00548508;
// name?)
// The job's view record (j +0x12000 = rt +1392 + 128 * (j +0x12410), or
// rt +1648 when the surface rt +512 has u8 +148 bit 7): the viewport
// (u16 x / y / w / h from rt +104), depth 0..1, the 4x4 at rt +944
// (VMX copy), scale (w * 0.5, h * -0.5, 1, 0) and offset (x + w * 0.5,
// y + h * 0.5, 0, 0) from rt +96.., u8 +112 = the surface's colour
// target (+128 with config +34, else +76) +20 (0 for none). Leaves r3 /
// f1 as the original does.
DECOMP(005380D0, Job_SetView_005380D0) {
    const u32 j = arg<u32>(ctx, 0), rt = arg<u32>(ctx, 1);
    u32 v = rt + 1648;
    if ((load<u8>(load<u32>(rt + 512) + 148) & 0x80) == 0) v = rt + ((load<u32>(j + 0x12410) << 7) & 0xFFFFFF80u) + 1392;
    store<u32>(j + 0x12000, v);
    store<u16>(v, load<u16>(rt + 104));
    store<u16>(v + 2, load<u16>(rt + 106));
    store<u16>(v + 4, load<u16>(rt + 108));
    store<u32>(v + 8, 0);
    store<u16>(v + 6, load<u16>(rt + 110));
    store<u32>(v + 12, 0x3F800000u);  // 1.0f
    {
        const u32 src = (rt + 944) & ~15u, dst = (v + 16) & ~15u;
        u32 w[16];
        for (u32 k = 0; k < 16; ++k) w[k] = load<u32>(src + 4 * k);  // lvx x4, then stvx x4
        for (u32 k = 0; k < 16; ++k) store<u32>(dst + 4 * k, w[k]);
    }
    constexpr u32 kK = 0x005380C4;  // f32 0.5, -0.5
    const u32 r = rt + 96;
    const f64 sx = sgl(static_cast<f64>(load<u16>(r + 4)) * lfs(kK));
    stfs12(v + 80, sx);
    const f64 h = static_cast<f64>(load<u16>(r + 6));
    store<u32>(v + 88, 0x3F800000u);
    store<u32>(v + 92, 0);
    stfs12(v + 84, sgl(h * lfs(kK + 4)));
    stfs12(v + 96, sgl(static_cast<f64>(load<u16>(r)) + sx));
    const f64 y = static_cast<f64>(load<u16>(r + 2)), h2 = static_cast<f64>(load<u16>(r + 6));
    store<u32>(v + 104, 0);
    store<u32>(v + 108, 0);
    const f64 oy = g11::madds(h2, lfs(kK), y);
    stfs12(v + 100, oy);
    ctx.fpr[1] = oy;
    const u32 s = load<u32>(rt + 512);
    const bool stereo = load<u8>(sly4::g::config + 34) != 0;
    const u32 slot = s + (stereo ? 128 : 76);
    u32 r3 = s + 76, id = 0;
    if (load<u32>(slot) != 0) {
        r3 = load<u32>(slot);
        id = load<u32>(r3 + 20);
    }
    store<u8>(v + 112, static_cast<u8>(id));
    ret(ctx, r3);
}

// bool LoadParams_005D2F20(ShaderType*, Material* m, const Desc* d)
// [a shader type's LoadParams] (name?)
// Unless Shader_IsReady_005AF298(g12::g::sho_texture_warp_type, m, d)
// fails (then 0): params 10..14 = d+128.. when the desc version (u8
// d+84) is at least the index, else the defaults at 0x005D2F10 (10 / 11
// their own, 12..14 the third); param 15 = d+148 (version 15+) or 1;
// then the texture named d+8 (Tex_FindLoaded) gets virtual slot 4 (+16)
// (t, 1, 1, 1) and is added to m. Returns 1.
DECOMP(005D2F20, LoadParams_005D2F20) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    if (static_cast<u32>(call(ctx, g12::fn::Shader_IsReady_005AF298, {g12::g::sho_texture_warp_type, m, d})) == 0) return ret(ctx, 0u);
    constexpr u32 kK = 0x005D2F10;  // f32 defaults for params 10, 11, 12..14
    auto ver = [&] { return static_cast<s32>(load<u8>(d + 84)); };
    auto param = [&](u32 k, u32 v) { store<u32>(static_cast<u32>(call(ctx, g12::fn::Material_ParamSlot, {m, k})), v); };
    param(10, ver() < 10 ? load<u32>(kK) : load<u32>(d + 128));  // lfs / stfs: raw
    param(11, ver() < 11 ? load<u32>(kK + 4) : load<u32>(d + 132));
    const u32 k2 = load<u32>(kK + 8);
    param(12, ver() < 12 ? k2 : load<u32>(d + 136));
    param(13, ver() < 13 ? k2 : load<u32>(d + 140));
    param(14, ver() < 14 ? k2 : load<u32>(d + 144));
    param(15, ver() < 15 ? 1u : load<u32>(d + 148));
    const u32 t = static_cast<u32>(call(ctx, g11::fn::Tex_FindLoaded, {load<u32>(d + 8)}));
    if (t != 0) {
        fx::vcall(ctx, t, 16, {t, 1, 1, 1});
        call(ctx, g11::fn::Material_AddTexture_004FF008, {m, t});
    }
    ret(ctx, 1);
}

// void Display_ScaleToTarget_00543F40(Effect* e)   [called through a
// pointer] (name?)
// A scaled copy of the display's colour target (display +148 bit 7: the
// screen's +132; stereo (config +34): screen +104 / +108 by screen +184;
// else screen +88) into e+140's surface (u16 +36 / +38 size, +4 offset
// << 1, +32 pitch): cellGcmSetTransferScaleMode(ctx, 0, 0) then
// cellGcmSetTransferScaleSurface with the CellGcmTransferScale {1, 3, 3,
// clip / out 0, 0, w, h, ratios in / out * 2^20, in size, pitch, origin 2,
// interp 0, offset, in 8, 8} and CellGcmTransferSurface {10 (A8R8G8B8),
// pitch, offset >> 1}. Leaves r3 as that call and f1 = the y ratio
// word.
DECOMP(00543F40, Display_ScaleToTarget_00543F40) {
    const u32 e = arg<u32>(ctx, 0);
    const u32 disp = load<u32>(sly4::draw::g::display), scr = g12::g::screen;
    u32 t;
    if ((load<u8>(disp + 148) & 0x80) != 0) t = load<u32>(scr + 132);
    else if (load<u8>(sly4::g::config + 34) != 0) t = load<u32>(scr + (load<u32>(scr + 184) == 1 ? 104 : 108));
    else t = load<u32>(scr + 88);
    const u32 o = load<u32>(e + 140);
    const u32 in_w = load<u16>(t + 24), in_h = load<u16>(t + 26), out_w = load<u16>(o + 36), out_h = load<u16>(o + 38);
    Stack frame(ctx, 112);  // the original's sp+112..223
    const u32 surf = frame.addr(), sc = surf + 12;
    store<u32>(sc, 1);       // conversion
    store<u32>(sc + 4, 3);   // format
    store<u32>(sc + 8, 3);   // operation
    store<u16>(sc + 12, 0);  // clip x / y / w / h
    store<u16>(sc + 14, 0);
    store<u16>(sc + 16, static_cast<u16>(out_w));
    store<u16>(sc + 18, static_cast<u16>(out_h));
    store<u16>(sc + 20, 0);  // out x / y / w / h
    store<u16>(sc + 22, 0);
    store<u16>(sc + 24, static_cast<u16>(out_w));
    store<u16>(sc + 26, static_cast<u16>(out_h));
    store<u16>(sc + 48, 8);  // in x / y (12.4: 0.5)
    store<u16>(sc + 50, 8);
    store<u16>(sc + 36, static_cast<u16>(in_w));
    store<u16>(sc + 38, static_cast<u16>(in_h));
    store<u16>(sc + 40, static_cast<u16>(load<u32>(t + 32)));  // pitch
    store<u8>(sc + 43, 0);   // interp
    store<u32>(sc + 44, load<u32>(t + 36));  // offset
    store<u32>(surf, 10);    // CELL_GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8
    store<u8>(sc + 42, 2);   // origin
    store<u16>(surf + 4, static_cast<u16>(load<u32>(o + 32)));
    store<u32>(surf + 8, load<u32>(o + 4) >> 1);
    const f64 k = lfs(0x00543F3C);  // 2^20
    const u64 rx = rt::fctiw(sgl(sgl(sgl(static_cast<f64>(in_w)) / sgl(static_cast<f64>(out_w))) * k), true);
    store<u32>(sc + 28, static_cast<u32>(rx));  // stfiwx
    const u64 ry = rt::fctiw(sgl(sgl(sgl(static_cast<f64>(in_h)) / sgl(static_cast<f64>(out_h))) * k), true);
    store<u32>(sc + 32, static_cast<u32>(ry));
    ctx.fpr[1] = std::bit_cast<f64>(ry);  // the fctiwz result stays in f1
    call(ctx, g12::fn::cellGcmSetTransferScaleMode, {load<u32>(fx::g::state), 0, 0});
    ret(ctx, call(ctx, g12::fn::cellGcmSetTransferScaleSurface, {load<u32>(fx::g::state), sc, surf}));
}

// void Emitter_Update_007A0C10(Emitter* o; f1 dt)   [called through a
// pointer] (name?)
// A prewarming emitter (u8 +64 bit 6 and its desc +4's u8 +42 bit 6)
// first steps the time +8 back by one interval (desc +4 lifetime / u16
// +40 count) and emits (Emitter_Emit_0079D280, f1 0) one interval apart
// until its ring (+72) is full (+20 == +16). Then Emitter_Advance_00500908
// (dt); the ring's oldest particles whose age (time - +12 + 0.01) reached
// the lifetime are dropped (count +20 - 1, first +12 to the next 36-byte
// entry, wrapping); while the rate accumulator +16 is at least 1, one
// particle per unit (time offsets dt / +16 apart from 0), +16 -= 1 each.
// Leaves r3 / f1 as the original does.
DECOMP(007A0C10, Emitter_Update_007A0C10) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    constexpr u32 kK = 0x007A0C00;  // f32 0.01, 0.0, 1.0
    if ((load<u8>(o + 64) & 0x40) != 0 && (load<u8>(load<u32>(o + 4) + 42) & 0x40) != 0) {
        const u32 d = load<u32>(o + 4);
        const f64 step = sgl(lfs(d + 4) / sgl(static_cast<f64>(load<u16>(d + 40))));
        stfs12(o + 8, sgl(lfs(o + 8) - step));
        auto full = [&] {
            const u32 r = load<u32>(o + 72);
            return load<u32>(r + 20) == load<u32>(r + 16);
        };
        if (!full()) {
            const f64 zero = lfs(kK + 4);
            do {
                stfs12(o + 8, sgl(lfs(o + 8) + step));
                callf(ctx, g12::fn::Emitter_Emit_0079D280, {o}, {zero});
            } while (!full());
        }
    }
    callf(ctx, g12::fn::Emitter_Advance_00500908, {o}, {dt});
    const u32 ring = load<u32>(o + 72);
    const f64 now = lfs(o + 8);
    const u32 d = load<u32>(o + 4);
    f64 rate = lfs(o + 16);
    const f64 k0 = lfs(kK);
    for (u32 count = load<u32>(ring + 20);;) {
        const u32 p = count != 0 ? load<u32>(ring + 12) : 0u;
        if (p == 0) break;
        if (!(sgl(sgl(now - lfs(p + 12)) + k0) >= lfs(d + 4))) break;
        const u32 next = load<u32>(ring + 12) + 36, end = load<u32>(ring + 4), base = load<u32>(ring);
        store<u32>(ring + 20, count - 1);
        store<u32>(ring + 12, static_cast<s32>(next - end) < 0 ? next : base);  // wrap at the end
        count -= 1;
    }
    ctx.fpr[1] = now;
    const f64 one = lfs(kK + 8);
    if (rate < one) return ret(ctx, kK);
    rate = sgl(dt / rate);
    f64 t = lfs(kK + 4);
    u64 r3 = 0;
    f64 left = 0;
    do {
        callf(ctx, g12::fn::Emitter_Emit_0079D280, {o}, {t});
        r3 = ctx.gpr[3];
        t = sgl(t + rate);
        left = sgl(lfs(o + 16) - one);
        stfs12(o + 16, left);
    } while (left >= one);
    ctx.fpr[1] = left;
    ret(ctx, r3);
}

namespace {

// The 16-byte buffer part {+0 p, +4 bit 0: pool 5, +8 bit 0: not owned,
// +12 tag in bits 28..31} given back as Pool_DestructPart does, inlined.
// Returns r3 as the inlined code leaves it (0, or p when not owned).
u32 free_part12(PPUContext& ctx, u32 part) {
    const u32 p = load<u32>(part);
    if (p == 0) return 0;
    if ((load<u32>(part + 8) & 1) != 0) return p;
    const u32 pool = (load<u32>(part + 4) & 1) != 0 ? 5u : 4u;
    call(ctx, g8::fn::Mem_FreePool, {p, pool, load<u32>(part + 12) >> 28});
    store<u32>(part, 0);
    return 0;
}

}  // namespace

// void SubMesh_Destruct_00538724(SubMesh* o)   (Delete_00538908; name?)
// Vtable 0x00B23EC0; the skin +112 ({+0 128-byte bone records, +4 parts,
// u32 +8 count}): each part (free_part12, freed, its slot zeroed), each
// record's +16 (Mem_FreePool(.., 6, 3)), the part array, the records and
// the skin freed; then Obj_DestructBase_004FD198. Leaves r3 as that.
DECOMP(00538724, SubMesh_Destruct_00538724) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B23EC0u);
    u32 sk = load<u32>(o + 112);
    if (sk != 0) {
        for (u32 i = 0; i < load<u32>(sk + 8); ++i) {
            const u32 part = load<u32>(load<u32>(sk + 4) + 4 * i);
            if (part == 0) continue;
            free_part12(ctx, part);
            call(ctx, g8::fn::Mem_FreeFrom, {part, 0});
            store<u32>(load<u32>(load<u32>(o + 112) + 4) + 4 * i, 0);
            sk = load<u32>(o + 112);
        }
        for (u32 j = 0; j < load<u32>(sk + 8); ++j) {
            call(ctx, g8::fn::Mem_FreePool, {load<u32>(load<u32>(sk) + 128 * j + 16), 6, 3});
            sk = load<u32>(o + 112);
        }
        call(ctx, g8::fn::Mem_FreeFrom, {load<u32>(sk + 4), 0});
        call(ctx, g8::fn::Mem_FreeFrom, {load<u32>(load<u32>(o + 112)), 0});
        call(ctx, g8::fn::Mem_FreeFrom, {load<u32>(o + 112), 0});
    }
    ret(ctx, call(ctx, g12::fn::Obj_DestructBase_004FD198, {o}));
}

// Shader* SelectShader_005D8C6C(ShaderType*, Material* m, DrawItem* it)
// [a shader type's SelectShader] (name?)
// The shader at m+16 +12 [variant * 2 + (m+128 != 0)]. Variant: with
// it+28's u8 +100 bit 7 and a skin it+44, its first word + 4 (item u8
// +69 = 1, else 0); otherwise bit 1 = opaque (u8 it+71 == 255) and bit 0
// = within the fog start: unless m's u8 +100 bit 6, always; else the
// mesh's (it+12) bounding sphere (+92 centre, +104 radius) through it
// (VMX) and the view matrix, not when g12::g::fog_start < radius - z.
DECOMP(005D8C6C, SelectShader_005D8C6C) {
    const u32 m = arg<u32>(ctx, 1), it = arg<u32>(ctx, 2);
    store<u8>(it + 69, 0);
    const u32 mesh = load<u32>(it + 12);
    u32 variant;
    if ((load<u8>(load<u32>(it + 28) + 100) & 0x80) != 0 && load<u32>(it + 44) != 0) {
        const u32 sk = load<u32>(load<u32>(it + 44));
        store<u8>(it + 69, 1);
        variant = sk + 4;
    } else {
        bool beyond = false;
        if ((load<u8>(m + 100) & 0x40) != 0) {
            const u32 c = mesh + 92, rows = it & ~15u, vm = g12::g::view_matrix;
            const f32 x = lvlx_f32(c), y = lvlx_f32(c + 4), z = lvlx_f32(c + 8);
            f32 w[3];
            for (u32 k = 0; k < 3; ++k) {
                f32 v = std::fmaf(load<f32>(rows + 4 * k), x, 0.0f);
                v = std::fmaf(load<f32>(rows + 16 + 4 * k), y, v);
                v = std::fmaf(load<f32>(rows + 32 + 4 * k), z, v);
                w[k] = load<f32>(rows + 48 + 4 * k) + v;
            }
            f32 vz = std::fmaf(load<f32>(vm + 8), w[0], 0.0f);
            vz = std::fmaf(load<f32>(vm + 24), w[1], vz);
            vz = std::fmaf(load<f32>(vm + 40), w[2], vz);
            vz = load<f32>(vm + 56) + vz;
            const f64 d = sgl(lfs(c + 12) - sly4::render::gfx10::lfs_value(std::bit_cast<u32>(vz)));
            const f64 fog = lfs(g12::g::fog_start);
            ctx.fpr[1] = d;
            ctx.fpr[2] = fog;
            beyond = fog < d;
        }
        const bool translucent = static_cast<s32>(load<u8>(it + 71)) < 255;
        variant = (translucent ? 0u : 2u) | (beyond ? 0u : 1u);
    }
    const u32 i = ((variant << 1) & 0xFFFFFFFEu) | (load<u32>(m + 128) != 0 ? 1u : 0u);
    ret(ctx, load<u32>(load<u32>(load<u32>(m + 16) + 12) + 4 * i));
}

// void Pool_AllocEntry_0079C07C(EntryPool* L)   (f_004FB704; name?)
// Moves the free list's head entry (16-byte entries at +28 {+0 next, +4
// prev, +12 flags}; free head +8 / tail +12 / count +16) to the used
// list (tail +4, head +0 only when it was empty or mode +24 is 1): it is
// unlinked (neighbours or the free ends patched), linked after the used
// tail (or alone), its flags get bit 31 set and bits 15..30 cleared; free
// count - 1 (both free ends -1 once empty), used count +20 + 1. Nothing
// without free entries. Leaves r3 = L.
DECOMP(0079C07C, Pool_AllocEntry_0079C07C) {
    const u32 L = arg<u32>(ctx, 0);
    const u32 nfree = load<u32>(L + 16), ftail = load<u32>(L + 12);
    const u32 tail = load<u32>(L + 4);
    if (nfree == 0) return ret(ctx, L);
    // The original also reads the head entry's flags (+12 >> 15, a
    // generation) into an unused stack word.
    const u32 fh = load<u32>(L + 8);
    const u32 idx = fh & 0xFFFF, e = idx << 4;
    const u32 base = load<u32>(L + 28);
    if (idx != fh) store<u32>(base + ((load<u32>(base + e) << 4) & ~15u) + 4, load<u32>(base + e + 4));
    else store<u32>(L + 8, load<u32>(base + e + 4));
    if (idx != ftail) {
        const u32 nx = load<u32>(base + e), pv = load<u32>(base + e + 4);
        store<u32>(base + ((pv << 4) & ~15u), nx);
    } else {
        store<u32>(L + 12, load<u32>(base + e));
    }
    auto mark = [&](u32 f) {
        const u8 b = static_cast<u8>(load<u8>(f) | 0x80);
        const u32 w = ((load<u32>(f) & 0x00FFFFFFu) | (u32{b} << 24)) & ~0x7FFF8000u;
        store<u8>(f, b);
        store<u32>(f, w);
    };
    if (static_cast<s32>(tail) == -1) {
        store<u32>(base + e, 0xFFFFFFFFu);
        store<u32>(base + e + 4, 0xFFFFFFFFu);
        mark(load<u32>(L + 28) + e + 12);
        store<u32>(L + 4, idx);
        store<u32>(L, idx);
    } else {
        store<u32>(base + ((tail << 4) & ~15u) + 4, idx);
        store<u32>(base + e, tail);
        const u32 ne = load<u32>(L + 28) + e;
        store<u32>(ne + 4, 0xFFFFFFFFu);
        mark(ne + 12);
        const u32 mode = load<u32>(L + 24);
        store<u32>(L + 4, idx);
        if (mode == 1) store<u32>(L, idx);
    }
    const u32 used = load<u32>(L + 20);
    store<u32>(L + 16, nfree - 1);
    store<u32>(L + 20, used + 1);
    if (nfree - 1 == 0) {
        store<u32>(L + 12, 0xFFFFFFFFu);
        store<u32>(L + 8, 0xFFFFFFFFu);
    }
    ret(ctx, L);
}

// void Graphics_RegisterTypes_0050EF04()   (Graphics_Init; name?)
// The AglTopology allocator holder (g12::g::topology_list = Mem_Alloc(16)
// {0, capacity, 0}: u16 config +356, 0 -> 1024, negative -> 0)
// and the topology map (g12::g::topology_map = a 24-byte Array {u16
// capacity, count 0, 0, element 16, +8 data from the default allocator's
// slot 0 (capacity * 8 bytes, align 16), +12 the allocator, +16 0, +20
// the compare g12::g::topology_cmp}, Array_Init); then the 'RHSM' /
// 'HSEM' / 'HHSM' chunk handlers and RegisterKLBM. Leaves r3 as that.
DECOMP(0050EF04, Graphics_RegisterTypes_0050EF04) {
    const u32 n = load<u16>(sly4::g::config + 356);
    u32 cap = 1024;
    if (n != 0) cap = static_cast<s16>(n) > 0 ? static_cast<u32>(static_cast<s32>(static_cast<s16>(n))) : 0u;
    const u32 h = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {16, 0, 16}));
    if (h != 0) {
        store<u32>(h, 0);
        store<u32>(h + 8, 0);
        store<u32>(h + 4, cap);
    }
    store<u32>(g12::g::topology_list, h);
    const u32 a = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {24, 0, 16}));
    if (a != 0) {
        store<u16>(a, static_cast<u16>(cap));
        store<u16>(a + 2, 0);
        store<u16>(a + 4, 0);
        store<u16>(a + 6, 16);
        store<u32>(a + 8, 0);
        const u32 al = static_cast<u32>(call(ctx, g12::fn::Mem_DefaultAllocator));
        const u32 c = load<u16>(a);
        store<u32>(a + 12, al);
        if (c != 0) {
            const u32 bytes = c * ((load<u16>(a + 6) >> 1) & 0x7FFF);
            const u32 opd = load<u32>(load<u32>(al));
            // r7 = the entry, r8 = the OPD (this sequence's order)
            store<u32>(a + 8, static_cast<u32>(call_ptr(ctx, opd, {al, bytes, 0, 16, load<u32>(opd), opd})));
        }
        call(ctx, g12::fn::Array_Init, {a});
        store<u32>(a + 16, 0);
        store<u32>(a + 20, g12::g::topology_cmp);
    }
    store<u32>(g12::g::topology_map, a);
    call(ctx, g12::fn::Iff_RegisterChunk, {0x5248534Du, 0x00F3B600, 0, 0, 0});  // 'RHSM'
    call(ctx, g12::fn::Iff_RegisterChunk, {0x4853454Du, 0x00F3B608, 0, 0, 0});  // 'HSEM'
    call(ctx, g12::fn::Iff_RegisterChunk, {0x4848534Du, 0x00F3B610, 0, 0, 0});  // 'HHSM'
    ret(ctx, call(ctx, g12::fn::RegisterKLBM));
}

// bool LoadParams_005B2DF0(ShaderType*, Material* m, const Desc* d)
// [a shader type's LoadParams] (name?)
// load_params12 (the first texture's call leaves r7 = the OPD, r8 / r9 =
// the sizes), then param 3 = d+100 for desc version (u8 d+84) 3+, else
// 1, past the first texture's lookup.
DECOMP(005B2DF0, LoadParams_005B2DF0) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    load_params12(ctx, m, d, {kOpd, kW, kH}, [&] {
        const u32 v = static_cast<s32>(load<u8>(d + 84)) < 3 ? 1u : load<u32>(d + 100);
        store<u32>(static_cast<u32>(call(ctx, g12::fn::Material_ParamSlot, {m, 3})), v);
    });
}

// void Draw_BindVertexArrays_0079E0C0(Buffer* b, VertexFormat* f, u32 offset,
// u32 stream)   (f_005159E8; name?)
// Points the vertex attribute arrays at b (+4: offset << 1 | location)
// + offset: render state +16 = f (+8 = -1 for a new format); unless +8
// already equals that address, +8 = it and per attribute of f (12-byte
// records up to +4 == 0) used by the vertex program (+20's u16 +10 &
// f's u16 +126) and not disabled (u8 +10 == 0): SET_VERTEX_DATA_ARRAY_
// FORMAT (stride f u16 +120 + 2 * stream << 8 | size << 4 | type) and
// _OFFSET (address + u8 +8 | location << 31) for register base +0 +
// u8 +9; arrays enabled before (u16 +32) but unused now are disabled
// (format 2), highest first; +32 = the used mask. Leaves r3 as the
// original does.
DECOMP(0079E0C0, Draw_BindVertexArrays_0079E0C0) {
    const u32 b = arg<u32>(ctx, 0), f = arg<u32>(ctx, 1), off = arg<u32>(ctx, 2), stream = arg<u32>(ctx, 3);
    constexpr u32 S = fx::g::state;
    u32 cur_addr = load<u32>(S + 8);
    const u32 vp = load<u32>(S + 20);
    if (load<u32>(S + 16) != f) {
        cur_addr = 0xFFFFFFFFu;
        store<u32>(S + 8, cur_addr);
    }
    store<u32>(S + 16, f);
    const u32 stride_at = f + ((stream << 1) & ~1u) + 120;
    const u32 w4 = load<u32>(b + 4);
    const u32 addr = (w4 >> 1) + off;
    const u32 stride = load<u16>(stride_at);
    if (cur_addr == addr) return ret(ctx, stride_at);
    store<u32>(S + 8, addr);
    const u32 used = load<u16>(vp + 10) & load<u16>(f + 126);
    if (load<u32>(f + 4) != 0) {
        const u32 st = (stride << 8) & 0xFF00, loc = (w4 << 31) & 0x80000000u;
        for (u32 a = f; load<u32>(a + 4) != 0; a += 12) {
            const u32 reg = load<u32>(a) + load<u8>(a + 9);
            const u32 sh = reg & 63, bit = sh < 32 ? 1u << sh : 0u;  // slw
            if (load<u8>(a + 10) != 0 || (bit & used) == 0) continue;
            const u32 type = load<u32>(a + 4);
            const u32 r4 = (reg << 2) & 0x3FC;
            const u32 c = load<u32>(S);
            const u32 fmt = ((static_cast<u32>(static_cast<s32>(type) >> 8) << 4) & 0xFF0) | (type & 0xFF);
            const u32 cur = load<u32>(c + 8);
            store<u32>(cur, (r4 + 0x1740) | 0x00040000u);  // SET_VERTEX_DATA_ARRAY_FORMAT + 4 * reg
            store<u32>(cur + 4, fmt | st);
            store<u32>(cur + 8, (r4 + 0x1680) | 0x00040000u);  // SET_VERTEX_DATA_ARRAY_OFFSET + 4 * reg
            store<u32>(cur + 12, (load<u8>(a + 8) + addr) | loc);
            store<u32>(c + 8, cur + 16);
        }
    }
    u32 off_mask = (load<u16>(S + 32) | used) ^ used;
    if (off_mask != 0) {
        for (s32 i = 15; i >= 0; --i) {
            const u32 bit = (1u << i) & 0xFFFF;
            if ((off_mask & bit) != 0) {
                const u32 en = load<u16>(S + 32);
                off_mask = (off_mask & ~bit) & 0xFFFF;
                const u32 c = load<u32>(S);
                store<u16>(S + 32, static_cast<u16>(en & ~bit));
                const u32 cur = load<u32>(c + 8);
                store<u32>(cur, ((static_cast<u32>(i) << 2) & 0x3FC) + 0x1740 + 0x00040000u);
                store<u32>(cur + 4, 2);  // disabled
                store<u32>(c + 8, cur + 8);
            }
            if (off_mask == 0) break;
        }
    }
    store<u16>(S + 32, static_cast<u16>(used));
    ret(ctx, used);
}

// Emitter* Emitter_New_00507E68(const Emitter* o)   [called through a
// pointer] (name?)
// An 80-byte emitter (SmallAlloc) built on o's properties o+4
// (Emitter_Construct_00500798, vtable 0x00B23308) with a particle ring
// +72 from ApeTMemClassFL<ring, 24> (g12::g::ring_list, built on first
// use: FreeList_Init(list, 24, 1024, 0, 0)) through virtual slot 3: u16
// props +40 (at most 256) 36-byte entries (SmallAlloc), {+0 / +8 / +12
// base, +4 end, +16 capacity, +20 0}; vtable 0x00B23338; +68 = o+68.
// Quirk: a failed allocation still writes +68 (address 68).
DECOMP(00507E68, Emitter_New_00507E68) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 e = static_cast<u32>(call(ctx, g12::fn::SmallAlloc, {80}));
    if (e != 0) {
        const u32 props = load<u32>(o + 4);
        call(ctx, g12::fn::Emitter_Construct_00500798, {e, props});
        store<u32>(e, 0x00B23308u);
        u32 fl = load<u32>(g12::g::ring_list);
        if (fl == 0) {
            const u32 p = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {60, 0, 16}));
            if (p != 0) {
                for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(p + off, 0);
                const u32 h = load<u16>(p + 54) & ~0x8000u;
                store<u16>(p + 52, 0);
                store<u16>(p + 54, static_cast<u16>(h));
                store<u32>(p, g12::g::vt_ring_list);
                store<u32>(p + 56, g12::g::str_ring_list);
                call(ctx, g12::fn::FreeList_Init, {p, 24, 1024, 0, 0});
            }
            fl = p;
            store<u32>(g12::g::ring_list, fl);
        }
        u32 ring = static_cast<u32>(fx::vcall(ctx, fl, 12, {fl}));
        if (ring != 0) {
            u32 n = load<u16>(props + 40);
            if (n > 256) n = 256;
            const u32 buf = static_cast<u32>(call(ctx, g12::fn::SmallAlloc, {u64{n} * 36}));
            store<u32>(ring, buf);
            store<u32>(ring + 8, buf);
            store<u32>(ring + 12, buf);
            store<u32>(ring + 16, n);
            store<u32>(ring + 20, 0);
            store<u32>(ring + 4, buf + n * 36);
        }
        store<u32>(e + 72, ring);
        store<u32>(e, 0x00B23338u);
    }
    store<u32>(e + 68, load<u32>(o + 68));
    ret(ctx, e);
}

// u8 Cmd_SetStencil_00515350(CmdStream* s)   (f_005159E8; name?)
// Reads the next aligned word of the command stream (+0 base, +4 end,
// +12 cursor; wrapping to the base when less than 16 bytes are left,
// then skipping a 12-byte header, wrapping again when 20 are not left)
// as {u8 enable, u8 ref}: enabled, both stencil ops REPLACE / KEEP
// (render state +176..+184) are sent as two SET_STENCIL_OP_FAIL blocks,
// func ALWAYS (+160), mask 255 (+168) and the ref (+164); disabled, the
// ops KEEP. Leaves r3 / f1 as the original does.
DECOMP(00515350, Cmd_SetStencil_00515350) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 end = load<u32>(s + 4), base = load<u32>(s);
    u32 p = load<u32>(s + 12);
    if (p + 15 > end) p = base;
    p = (p + 3) & ~3u;
    u32 q = p + 12;
    if (p + 19 > end) q = base;
    q = (q + 3) & ~3u;
    store<u32>(s + 12, q + 4);
    ctx.fpr[1] = lfs(q);
    const u8 enable = load<u8>(q), ref = load<u8>(q + 1);  // stfs / lbz of the word
    constexpr u32 S = fx::g::state;
    constexpr u32 kOp = 0x000C033Cu, kFunc = 0x000C0330u, kWriteMask = 0x0004032Cu;
    constexpr u32 kKeep = 0x1E00, kReplace = 0x1E01, kAlways = 0x0207;
    auto emit4 = [](u32 m, u32 a, u32 b2, u32 c2) {
        const u32 g = load<u32>(S), cur = load<u32>(g + 8);
        store<u32>(cur, m);
        store<u32>(cur + 4, a);
        store<u32>(cur + 8, b2);
        store<u32>(cur + 12, c2);
        store<u32>(g + 8, cur + 16);
    };
    const u32 op = enable != 0 ? kReplace : kKeep;
    store<u32>(S + 176, op);
    store<u32>(S + 184, op);
    emit4(kOp, op, load<u32>(S + 180), op);
    store<u32>(S + 180, kKeep);
    emit4(kOp, load<u32>(S + 176), kKeep, load<u32>(S + 184));
    if (enable == 0) return ret(ctx, kKeep);
    store<u32>(S + 160, kAlways);
    emit4(kFunc, kAlways, load<u32>(S + 164), load<u32>(S + 172));
    store<u32>(S + 168, 255);
    {
        const u32 g = load<u32>(S), cur = load<u32>(g + 8);
        store<u32>(cur, kWriteMask);
        store<u32>(cur + 4, 255);
        store<u32>(g + 8, cur + 8);
    }
    store<u32>(S + 164, ref);
    emit4(kFunc, load<u32>(S + 160), ref, load<u32>(S + 172));
    ret(ctx, ref);
}

namespace {

// A static array of `n` 52-byte target entries {part +0, target +40, u8
// +48 not owned} destroyed last to first: an owned entry's target frees
// its tile slot (u8 +56 clear: s8 +45 != -1 cleared in
// g12::g::tile_slots, then its part), its part, then itself (+40 = 0);
// then each entry's own part (free_part12). Returns r3 as the last part
// check leaves it.
u32 target_array_destruct12(PPUContext& ctx, u32 base, u32 n) {
    u32 r3 = 0;
    for (s32 i = static_cast<s32>(n) - 1; i >= 0; --i) {
        const u32 e = base + 52 * static_cast<u32>(i);
        if (load<u8>(e + 48) == 0) {
            if (const u32 t = load<u32>(e + 40); t != 0) {
                if (load<u8>(t + 56) == 0) {
                    const u32 slot = load<u8>(t + 45);
                    if (static_cast<s8>(slot) != -1) {
                        const u32 sh = slot & 63, bit = sh < 32 ? 1u << sh : 0u;  // slw
                        store<u16>(g12::g::tile_slots, static_cast<u16>(load<u16>(g12::g::tile_slots) & ~bit));
                    }
                    free_part12(ctx, t);
                }
                free_part12(ctx, t);
                call(ctx, g8::fn::Mem_FreeFrom, {t, 0});
                store<u32>(e + 40, 0);
            }
        }
        r3 = free_part12(ctx, e);
    }
    return r3;
}

}  // namespace

// void TargetArray_Destruct_0054E120()   (f_0054E30C; name?)
// target_array_destruct12 of the 5 entries at g12::g::static_targets.
DECOMP(0054E120, TargetArray_Destruct_0054E120) { ret(ctx, target_array_destruct12(ctx, g12::g::static_targets, 5)); }

// void TargetArray_Destruct_0054DB44()   (f_0054E30C; name?)
// target_array_destruct12 of the 2 entries at 0x01164188.
DECOMP(0054DB44, TargetArray_Destruct_0054DB44) { ret(ctx, target_array_destruct12(ctx, 0x01164188, 2)); }

// void TargetArray_Destruct_0054DD38()   (f_0054E30C; name?)
// target_array_destruct12 of the 2 entries at 0x011641F0.
DECOMP(0054DD38, TargetArray_Destruct_0054DD38) { ret(ctx, target_array_destruct12(ctx, 0x011641F0, 2)); }

// void TargetArray_Destruct_0054DF2C()   (f_0054E30C; name?)
// target_array_destruct12 of the 2 entries at 0x01164258.
DECOMP(0054DF2C, TargetArray_Destruct_0054DF2C) { ret(ctx, target_array_destruct12(ctx, 0x01164258, 2)); }

// void TopoRecordRef_Release_0079C47C(TopoRecordRef* ref)   (f_005087EC;
// name?)
// Drops one reference of ref's record (u16 +0); at zero it is freed
// (topo_free12). Leaves r3 as the original does.
DECOMP(0079C47C, TopoRecordRef_Release_0079C47C) {
    const u32 ref = arg<u32>(ctx, 0);
    const u32 r = load<u32>(ref);
    if (r == 0) return ret(ctx, ref);
    const u64 refs = static_cast<u64>(load<u16>(r)) - 1;  // addic: the full register
    store<u16>(r, static_cast<u16>(refs));
    if (static_cast<s16>(refs) > 0) return ret(ctx, refs);
    ret(ctx, topo_free12(ctx, r));
}

// bool Game_ShutdownSystems_005DB4BC()   (Game_Shutdown; name?)
// The function-local static g12::g::shutdown_holder (built on first use:
// +20 / +24 = 0, u8 +32 bits 7 / 6 cleared, +92.. 400 bytes zeroed,
// shutdown_holder_ptr = it) releases its object +28 (virtual slot 1,
// then +28 = 0); then Debug_FreePrimPools, Bank_FreeAll,
// Render_Shutdown_0050FB5C, Platform_Shutdown and Spurs_Shutdown, each
// stopping the chain (result 0) when it fails. Returns 1 when all
// succeed.
DECOMP(005DB4BC, Game_ShutdownSystems_005DB4BC) {
    constexpr u32 G = g12::g::shutdown_holder;
    auto build = [&] {
        const u32 b = load<u8>(G + 32);
        store<u32>(G + 20, 0);
        store<u32>(G + 24, 0);
        store<u8>(G + 32, static_cast<u8>(b & ~0xC0u));
        call(ctx, g12::fn::memset, {G + 92, 0, 400});
        store<u32>(g12::g::shutdown_holder_ptr, G);
        store<u8>(g12::g::shutdown_holder_ready, 1);
    };
    u32 ready = load<u8>(g12::g::shutdown_holder_ready);
    if (ready == 0) {
        build();
        ready = 1;
    }
    if (u32 o = load<u32>(G + 28); o != 0) {
        if (ready == 0) {  // not reachable: the guard is set by now
            build();
            o = load<u32>(G + 28);
        }
        if (o != 0) {
            fx::vcall(ctx, o, 4, {o});
            ready = load<u8>(g12::g::shutdown_holder_ready);
        }
        if (ready == 0) build();
        store<u32>(G + 28, 0);
    }
    for (u32 f : {g12::fn::Debug_FreePrimPools, g12::fn::Bank_FreeAll, g12::fn::Render_Shutdown_0050FB5C,
                  g12::fn::Platform_Shutdown, g12::fn::Spurs_Shutdown})
        if (static_cast<u32>(call(ctx, f)) == 0) return ret(ctx, 0);
    ret(ctx, 1);
}
