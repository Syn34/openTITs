// The last shader Draw slots (Shader vtable slot 12):
// void <Shader>_Draw(Shader* s, DrawItem* item)
#include "shader6.h"
#include "shader_draw.h"

namespace sly4::gfx {

namespace {

using namespace sdraw;
namespace s4 = sly4::render::shader4;

constexpr u32 kLightDir = 0x010DD8A0;     // view block +288: the camera rotation's column 2 (shader5_draw.cpp)
constexpr u32 kEye = 0x010DD8B0;          // view block +304: the camera position (render::g::camera_pos)
constexpr u32 kViewColumns = 0x010DD880;  // view block +256: the camera rotation transposed (3 rows)
constexpr u32 kViewRows320 = 0x010DD8C0;  // view block +320: view +1200.. (f32 rows)
constexpr u32 kFrameDelta = 0x010DD5AC;   // f32 (layers3::g::block_dd5a8 +4): the frame's time step (name?)
constexpr u32 kLayerScale = 0x010DD5B4;   // f32 (block_dd5a8 +12): the view's layer scale (shader4_frame.cpp kFogScale)
constexpr u32 Matrix_NearlyEqual = 0x005545B4;  // (const f32[16], const f32[16]; f1 eps) -> bool (audio/lib_math.cpp)
constexpr u32 kMeshBounds = 92;           // Mesh +92: f32 centre[3], radius
constexpr u32 kStateCacheA = 0x010DD598, kStateCacheB = 0x010DD5A0;  // u64, reset to -1 (render::g::state_cache_a / _b)

const be_f32* as_rows(const void* p) { return static_cast<const be_f32*>(p); }
f64 dbl(u32 ea) { return std::bit_cast<f64>(global<be_u64>(ea).get()); }

// The camera in `space`'s frame (Listener_LocalDirection).
void eye_in(void* out, const void* space) { call(s4::fn::Listener_LocalDirection, out, kEye, space); }

// The lit draws' pieces (lit_draw_core): fragment constant `index` =
// `first`; for a translucent item (alpha +71 below 255) no depth writes
// and fragment constant `index` = {own blending ? 0 : 1, alpha * k,
// 0, 0} (alpha blending on unless the material blends itself, material
// +104 bit 31).
void translucent_begin(DrawItem* item, u32 k, u32 index) {
    fconst(index, host<u8>(s3::g::character_fconst2));
    if (item_alpha(item) >= 255) return;  // cmpwi
    emit({kDepthMask, 0});
    GuestFrame f(16);
    be_u32* v = f.as<be_u32>();
    v[0] = 0u;
    v[2] = 0u;
    v[3] = 0u;
    reinterpret_cast<be_f32*>(v)[1] = byte_scaled(item_alpha(item), global<be_f32>(k));  // fcfid, frsp, fmuls
    if (!mat_bit31(item_mat(item), 104)) {
        blend_on(kSrcAlpha2, kOneMinusSrcAlpha2);
        v[0] = kOne;
    }
    fconst(index, v);
}

// A mirrored item (+68 bit 7) draws with the front face flipped.
void mirror_begin(DrawItem* item) {
    if ((item->flags68 & kMirrored) == 0) return;  // rlwinm 25,31,31
    emit({kCullFaceEnable, 1});
    emit({kFrontFace, kGlCw});
}
void mirror_end(DrawItem* item) {
    if ((item->flags68 & kMirrored) == 0) return;
    emit({kCullFaceEnable, 1});
    emit({kFrontFace, kGlCcw});
}

// After a translucent draw the depth mask (material +112 bit 31) and
// blending are restored.
void translucent_end(DrawItem* item) {
    if (item_alpha(item) >= 255) return;
    if (mat_bit31(item_mat(item), 112)) emit({kDepthMask, rs().dirty_b[1] & 1u});
    if (!mat_bit31(item_mat(item), 104)) emit({kBlendEnable, 0});
}

// For a lit item (variant +69 bit 0) the layer list (unchecked) as
// fragment constants 16.. when variant bit 1 is set, else as vertex
// constants 16..; with variant bit 2 fragment constant 1 = {the camera in
// the item's frame, 1}. (shader5_draw.cpp's lit_layers.)
void lit_layers(DrawItem* item) {
    if ((item->variant & 1) == 0) return;
    if ((item->variant & 2) != 0) {
        upload_layers_fragment(item);
    } else {
        upload_layer_list(item_mat(item), item->layers);
    }
    if ((item->variant & 4) != 0) {
        GuestFrame f(32);  // the original's sp+112 / +128
        be_f32* dir = f.as<be_f32>(0);
        be_f32* c1 = f.as<be_f32>(16);
        eye_in(dir, item);
        vec3_w1(c1, dir);
        fconst(1, c1);
    }
}

// {r, g, b, 1} from the packed colour word's bytes 2 / 1 / 0, each
// through fcfid / frsp times k (1/255), as fragment constant `index`.
void packed_colour(u32 index, u32 word, u32 k) {
    GuestFrame f(16);  // the original's sp+224
    be_f32* c = f.as<be_f32>();
    const f32 scale = global<be_f32>(k);
    reinterpret_cast<be_u32*>(c)[3] = kOne;
    c[0] = byte_scaled((word >> 16) & 0xFF, scale);  // rlwinm 16,24,31
    c[1] = byte_scaled((word >> 8) & 0xFF, scale);
    c[2] = byte_scaled(word & 0xFF, scale);
    fconst(index, c);
}

// SlyTerrainSnow / SlyTerrainSnowDecal: the material parameter offsets
// (p +..) and the statics each uses.
struct Snow {
    u32 k;           // .text: f32 1/255, 0.0, 60.0, 1.0, -1.0, 0.005
    u32 last_time;   // f32: the clock the view check last ran at (name?)
    u32 moved;       // u8: the view moved at that check (name?)
    u32 cur;         // p +cur / +4: the scroll offset (u, v), written back to slots slot_cur / +1
    u32 rate;        // p +rate / +4: the scroll speed
    u32 rate_moved;  // p +rate_moved / +4: the scroll speed while the view moves
    u32 clock;       // p +clock: the clock the offset was last advanced at, written back to slot_clock
    u32 slot_clock, slot_cur;
    u32 margin;      // p +margin, +4: added to the mesh's bounding radius
    u32 shade;       // p +shade.. : the far-side colour constant's source
};

// The snow scroll: at a new material clock (s3::g::material_time) compared
// with `last_time` the view check runs first (`moved` = the view rows at
// kViewRows320 differ from kViewColumns by more than 0.005:
// Matrix_NearlyEqual); when the material's own clock (p +clock) differs
// from it, it is stored (Material_ParamSlot) and the offset advances by
// delta * scale * 60 * the speed (`rate`, or `rate_moved` after a view
// move), each coordinate wrapped by -1 above 1, and stored back. The
// offset {u, v, 0, 0} goes out as fragment constant 1 or vertex constant
// 8.
void snow_scroll(Material* mat, const Snow& s, bool vertex) {
    const u32 k = s.k;
    const u32 zero = global<be_u32>(k + 4);
    u32 params = mat->w124;
    const auto pw = [&](u32 off) { return params != 0 ? host<be_u32>(params + off)->get() : zero; };
    const auto pf = [&](u32 off) { return as_f32(pw(off)); };
    const u32 cur_u = pw(s.cur), cur_v = pw(s.cur + 4);  // f31 / f30
    u32 out_u = cur_u, out_v = cur_v;
    const u32 time_w = global<be_u32>(s3::g::material_time);
    const f32 time = as_f32(time_w);
    if (!(global<be_f32>(s.last_time) == time)) {  // fcmpu, bc 4,2
        global<be_u32>(s.last_time) = time_w;
        const u32 same = call<u32>(Matrix_NearlyEqual, kViewRows320, kViewColumns, global<be_f32>(k + 20).get());
        global<u8>(s.moved) = same == 0 ? 1 : 0;  // cntlzw, rlwinm 27,31,31
        params = mat->w124;
    }
    if (!(pf(s.clock) == time)) {
        *Material_ParamSlot(mat, s.slot_clock) = time_w;
        params = mat->w124;
        const f32 dt = global<be_f32>(kFrameDelta), scale = global<be_f32>(kLayerScale);
        f32 su = pf(s.rate), sv = pf(s.rate + 4);
        if (global<u8>(s.moved) != 0) {
            su = pf(s.rate_moved);
            sv = pf(s.rate_moved + 4);
        }
        const f32 step = (dt * scale) * global<be_f32>(k + 8);  // fmuls x2
        const f32 du = step * su, dv = step * sv;
        const f32 one = global<be_f32>(k + 12), minus_one = global<be_f32>(k + 16);
        f32 u = as_f32(cur_u) + du, v = as_f32(cur_v) + dv;  // fadds
        if (u > one) {  // fcmpu, bc 12,1
            v = global<be_f32>(k + 4) + v;
            u = minus_one + u;
        }
        if (v > one) {
            u = global<be_f32>(k + 4) + u;
            v = minus_one + v;
        }
        *Material_ParamSlot(mat, s.slot_cur) = bits(u);
        *Material_ParamSlot(mat, s.slot_cur + 1) = bits(v);
        out_u = bits(u);
        out_v = bits(v);
    }
    GuestFrame f(16);  // the original's sp+144
    be_u32* c = f.as<be_u32>();
    c[0] = out_u;
    c[1] = out_v;
    c[2] = 0u;
    c[3] = 0u;
    if (vertex) {
        vconsts(8, 4, c);
    } else {
        fconst(1, c);
    }
}

// Whether the mesh's bounding sphere (Mesh +92) lies beyond the camera
// along the light: -((centre - eye) . dir) > radius + p +margin + p
// +margin+4, eye the camera and dir kLightDir both in the item's frame.
bool snow_far(DrawItem* item, Material* mat, const Snow& s) {
    const u32 params = mat->w124;
    const auto pf = [&](u32 off) { return static_cast<f32>(*host<be_f32>(params != 0 ? params + off : s.k + 4)); };
    const f32 m0 = pf(s.margin), m1 = pf(s.margin + 4);
    const be_f32* b = at<be_f32>(item->mesh.get(), kMeshBounds);
    const f32 cx = b[0], cy = b[1], cz = b[2], radius = b[3];
    GuestFrame f(32);  // the original's sp+192 (eye) / +208 (dir)
    be_f32* eye = f.as<be_f32>(0);
    be_f32* dir = f.as<be_f32>(16);
    eye_in(eye, item);
    Vec3_RotateTo(dir, host<be_f32>(kLightDir), as_rows(item));
    const f32 dy = cy - eye[1];
    const f32 dx = cx - eye[0];
    const f32 dz = cz - eye[2];
    const f32 reach = (radius + m0) + m1;  // fadds x2
    f32 d = dy * dir[1];
    d = fmadds(dx, dir[0], d);
    d = fnmadds(dz, dir[2], d);
    return d > reach;  // fcmpu, bc 12,1
}

// SlyTerrainSnow (decal false) / SlyTerrainSnowDecal (decal true): the
// item transform and the view fog; with variant +69 bit 2 (the decal: bit
// 0) fragment constant 2 (3) = {the camera in the item's frame, 1} and 3
// (4) = the packed colour at p +40 (+28) (packed_colour, 0 without
// parameters); with bit 0 the layer list (unchecked) as vertex constants
// 16..; with bit 1 the snow scroll (snow_scroll: the plain shader's as
// vertex constant 8) and the far-side constant (snow_far: when beyond,
// x = 0.0, else p +shade; the plain shader {x, 1 / p+shade+8, -(p
// +shade+4 / p +shade+8), 0} as fragment constant 1, the decal {x, p
// +shade+4, p +shade+8, 0} as fragment constant 2); then the lit draw
// with fragment constant 0 (stencil without 0x40).
void snow_draw(DrawItem* item, const Snow& s, bool decal) {
    const u32 k = s.k;
    Material* mat = item_mat(item);
    upload_item_matrix(item);
    apply_fog(item);
    if ((item->variant & (decal ? 1 : 4)) != 0) {
        {
            GuestFrame f(32);  // the original's sp+112 / +128
            be_f32* eye = f.as<be_f32>(0);
            be_f32* c = f.as<be_f32>(16);
            eye_in(eye, item);
            vec3_w1(c, eye);
            fconst(decal ? 3u : 2u, c);
        }
        const u32 params = mat->w124;
        packed_colour(decal ? 4 : 3, params != 0 ? host<be_u32>(params + (decal ? 28 : 40))->get() : 0, k);
        if (decal) upload_layer_list(item_mat(item), item->layers);
    }
    if (!decal && (item->variant & 1) != 0) upload_layer_list(item_mat(item), item->layers);
    if ((item->variant & 2) != 0) {  // rlwinm 0,30,30
        snow_scroll(mat, s, !decal);
        const bool far = snow_far(item, mat, s);
        const u32 params = mat->w124;
        const auto pword = [&](u32 off) { return params != 0 ? host<be_u32>(params + off)->get() : global<be_u32>(k + 4).get(); };
        GuestFrame f(16);  // the original's sp+160
        be_u32* c = f.as<be_u32>();
        c[0] = far || params == 0 ? global<be_u32>(k + 4).get() : host<be_u32>(params + s.shade)->get();
        c[3] = 0u;
        if (decal) {
            c[1] = pword(s.shade + 4);
            c[2] = pword(s.shade + 8);
            fconst(2, c);
        } else {
            const f32 a = as_f32(pword(s.shade + 4));
            const f32 b = as_f32(pword(s.shade + 8));
            const f32 q = a / b;                                        // fdivs
            c[2] = bits(q) ^ 0x80000000u;                               // fneg
            c[1] = bits(global<be_f32>(k + 12) / b);                    // fdivs
            fconst(1, c);
        }
    }
    lit_draw(item, k, Stencil::Plain, 0);
}

}  // namespace

// SlyTerrainSnowDecal: snow_draw (the decal variant).
void SlyTerrainSnowDecal_Draw(ShaderPrograms*, DrawItem* item) {
    static constexpr Snow s{0x005CFDA4, 0x00F8B0F0, 0x00F8B0F4, 72, 64, 8, 80, 20, 18, 56, 16};
    snow_draw(item, s, true);
}

// SlyTerrainSnow: snow_draw (the plain variant).
void SlyTerrainSnow_Draw(ShaderPrograms*, DrawItem* item) {
    static constexpr Snow s{0x005CE684, 0x00F8B0E8, 0x00F8B0EC, 60, 52, 20, 68, 17, 15, 44, 28};
    snow_draw(item, s, false);
}

// SlyProp: the item transform and the view fog; vertex constant 7 = the
// item's tint bytes (+72..74) / 256 with w 255 / 256 (lvsl / vupkhsb /
// vupkhsh, masked to 0xFF, vcfsx 8); the lit layers (lit_layers); the lit
// draw's translucent constant (fragment constant 2) and mirroring; then
// its own stencil: bits = (stencil bit 7 | material +101) & the write
// mask; when bits | 0x40 differs from the cached reference it becomes the
// reference (the cache, stencil_bytes[0], gets bits: as in the original,
// without the 0x40) with the write mask bits (or the mask when bits are
// 0); with material +100 bit 3 the function NOTEQUAL against mask 0x40;
// the mesh's draw; after it the reference is cleared (render state +164)
// when bits are 0, the function goes back to ALWAYS / mask 0 with
// material +100 bit 3, and either case re-sends the function.
void SlyProp_Draw(ShaderPrograms*, DrawItem* item) {
    constexpr u32 k = 0x005CADF0;  // .text: u32 24, vperm mask (+16), f32 1/255 (+32)
    constexpr u32 kItemTint = 72;  // DrawItem +72: u8[4] (name?)
    constexpr u32 kNotEqual = 0x0205, kAlways = 0x0207;
    RenderState& st = rs();
    upload_item_matrix(item);
    apply_fog(item);
    {
        GcmContext* c = or_zero_page(gcm());
        const u32 cur = c->current;
        c->current = cur + 24;
        be_u32* w = host<be_u32>(cur);
        w = w ? w : at<be_u32>(nullptr, 0);
        w[0] = 0x00141EFCu;  // SET_TRANSFORM_CONSTANT_LOAD, 5 words
        w[1] = 7u;
        for (u32 i = 0; i < 3; ++i) reinterpret_cast<be_f32*>(w)[2 + i] = static_cast<f32>(*at<u8>(item, kItemTint + i)) / 256.0f;
        reinterpret_cast<be_f32*>(w)[5] = 255.0f / 256.0f;
    }
    lit_layers(item);
    translucent_begin(item, k + 32, 2);
    mirror_begin(item);
    const u32 flags = item->flags68;
    Material* mat = item_mat(item);
    u8* bytes = host<u8>(kStencilBytes);
    const u32 mask = bytes[1];
    const u32 bits = (((flags & kStencilHigh) != 0 ? 0x80u : 0u) | mat_stencil(mat)) & mask;  // rlwinm 27,31,31
    const u32 test = (mat->b100 & 8) != 0 ? 0x40u : 0u;  // rlwinm 29,31,31
    const u32 ref_value = bits | 0x40;
    if (ref_value != bytes[0]) {
        bytes[0] = static_cast<u8>(bits);
        st.stencil_ref = ref_value;
        emit({kStencilFunc, st.stencil_func, ref_value, st.stencil_func_mask});
        const u32 write = bits != 0 ? bits : mask;
        st.stencil_mask = write;
        emit({kStencilMask, write});
    }
    if (test != 0) {
        st.stencil_func = kNotEqual;
        st.stencil_func_mask = test;
        emit({kStencilFunc, kNotEqual, st.stencil_ref, test});
    }
    draw_mesh(item);
    mirror_end(item);
    translucent_end(item);
    if (bits == 0) st.stencil_ref = 0u;
    const bool tested = (item_mat(item)->b100 & 8) != 0;
    if (tested) {
        st.stencil_func = kAlways;
        st.stencil_func_mask = 0u;
    }
    if (tested || bits == 0) front_func();
}

namespace {

constexpr u32 kViewRotation = 0x010DD840;   // view block +192 (shader4_draw.cpp)
constexpr u32 kMirrorBias = 0x01197260;     // f32[4][4]: SlyMirror's clip-to-texture bias, built on first use (name?)
constexpr u32 kMirrorBiasSet = 0x01197250;  // u8: kMirrorBias built (name?)

// out row i = m row 0 * rows[i].x + m row 1 * .y + m row 2 * .z (+ m row 3
// for i = 3) per lane in vmaddfp order (rows and m read with lvx), as
// upload_item_matrix computes it.
void rows_through(be_f32* out, const void* rows0, const void* m0) {
    const be_f32* rows = aligned16(rows0);
    const be_f32* m = aligned16(m0);
    for (u32 i = 0; i < 4; ++i) {
        const f32 x = rows[4 * i], y = rows[4 * i + 1], z = rows[4 * i + 2];
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fmaf(m[k], x, i == 3 ? static_cast<f32>(m[12 + k]) : 0.0f);
            v = std::fmaf(m[4 + k], y, v);
            out[4 * i + k] = std::fmaf(m[8 + k], z, v);
        }
    }
}

}  // namespace

// SlyMirror: the item transform; vertex constants 4..7 = transpose(item *
// M * kMirrorBias), M the 4x4 of material parameters +12..+72 (0.0 each
// without parameters) applied as rows_through, kMirrorBias {0.5, -0.5,
// 0.5 scale, 0.5 offsets} (built once) by Matrix_MultiplyTo; fragment
// constant 2 = the material colour (+64); the view fog; rows = the item
// rows through kViewRotation (Matrix_RotateRows), c = {the camera in the
// item's frame, 1}; for a lit item (variant +69 bit 0) with bit 1 the
// layer list (unchecked) as fragment constants 16.. and rows / c as
// fragment constants 3..6, otherwise (bit 0 alone: the layer list as
// vertex constants 16.. first) rows as vertex constant parameter blocks
// 10..12 and c as vertex constant 13; then the lit draw with fragment
// constant 1 (stencil without 0x40).
void SlyMirror_Draw(ShaderPrograms*, DrawItem* item) {
    constexpr u32 k = 0x005C832C;  // .text: f32 0.0, 1/255
    Material* mat = item_mat(item);
    const u32 params = mat->w124;
    // The original's sp+208 (M), +272 (item * M), +336 (* bias), +400 (transposed).
    GuestFrame frame(256);
    be_u32* m = frame.as<be_u32>(0);
    be_f32* t = frame.as<be_f32>(64);
    be_u32* r = frame.as<be_u32>(128);
    be_u32* tr = frame.as<be_u32>(192);
    for (u32 i = 0; i < 16; ++i) m[i] = params != 0 ? host<be_u32>(params + 12 + 4 * i)->get() : global<be_u32>(k).get();
    upload_item_matrix(item);
    if (global<u8>(kMirrorBiasSet) == 0) {
        const u32 half = 0x3F000000u, zero = global<be_u32>(k);
        const u32 words[16] = {half, zero, zero, zero, zero, 0xBF000000u, zero, zero, zero, zero, half, zero, half, half, half, kOne};
        for (u32 i = 0; i < 16; ++i) global<be_u32>(kMirrorBias + 4 * i) = words[i];
        global<u8>(kMirrorBiasSet) = 1;
    }
    rows_through(t, item, m);
    Matrix_MultiplyTo(reinterpret_cast<be_f32*>(r), t, host<be_f32>(kMirrorBias));
    for (u32 i = 0; i < 4; ++i) {
        for (u32 j = 0; j < 4; ++j) tr[4 * j + i] = r[4 * i + j].get();  // vmrghw / vmrglw
    }
    vconsts(4, 16, tr);
    fconst(2, at<u8>(mat, 64));
    apply_fog(item);
    // The original's sp+160 (rows), +112 (camera), +128 (c).
    GuestFrame frame2(80);
    be_f32* rows = frame2.as<be_f32>(0);
    be_f32* eye = frame2.as<be_f32>(48);
    be_f32* c = frame2.as<be_f32>(64);
    Matrix_RotateRows(rows, as_rows(item), host<be_f32>(kViewRotation));
    eye_in(eye, item);
    vec3_w1(c, eye);
    const u32 variant = item->variant;
    if ((variant & 1) != 0 && (variant & 2) != 0) {
        upload_layers_fragment(item);
        for (u32 i = 0; i < 3; ++i) fconst(3 + i, rows + 4 * i);
        fconst(6, c);
    } else {
        if ((variant & 1) != 0) upload_layer_list(item_mat(item), item->layers);
        for (u32 i = 0; i < 3; ++i) call(s2::fn::cellGcmSetVertexProgramParameterBlock, gcm(), 10 + i, 1u, rows + 4 * i);
        vconsts(13, 4, c);
    }
    lit_draw(item, k + 4, Stencil::Plain, 1);
}

namespace {

constexpr u32 kEyeBlock = 0x01196A40;          // f32[16]: ProjTxr_BuildProjection's block for the eye projectors (name?)
constexpr u32 kFovScale = 0x010DD9E4;          // view block +612: min(1, tan(fov / 2) / tan(k)) (shader5.h)

// row (f32[4] at `v`, read as 3 scalars) = rot row 0 * x + row 1 * y + row
// 2 * z (+ row 3 for a point; vaddfp) per lane in vmaddfp order, rot read
// with lvx, all 4 lanes stored (stvx; lane 3 from rot's w lanes).
void eye_transform(u8* v, const u8* rot0, bool point) {
    const f32 x = lvlx_f32(v, 0), y = lvlx_f32(v, 4), z = lvlx_f32(v, 8);
    const be_f32* rot = aligned16(rot0);
    f32 out[4];
    for (u32 k = 0; k < 4; ++k) {
        f32 a = std::fmaf(rot[k], x, 0.0f);
        a = std::fmaf(rot[4 + k], y, a);
        a = std::fmaf(rot[8 + k], z, a);
        out[k] = point ? rot[12 + k] + a : a;
    }
    be_f32* o = aligned16_out(v);
    for (u32 k = 0; k < 4; ++k) o[k] = out[k];
}

// The xyz of `rows` 3 rows (16 bytes apart) to dst's 3 rows and each as a
// vertex constant parameter block from `first`. The w lanes of dst are
// never written: what the guest stack holds there goes out with them.
void eye_rows(u8* dst, const u8* rows, u32 first) {
    for (u32 r = 0; r < 3; ++r) copy3(dst + 16 * r, rows + 16 * r);
    for (u32 r = 0; r < 3; ++r) call(s2::fn::cellGcmSetVertexProgramParameterBlock, gcm(), first + r, 1u, dst + 16 * r);
}

}  // namespace

// SlyCharacterEye: the item transform and the view fog; for a lit item
// (variant +69 bit 0) the layer list (unchecked) as vertex constants 16..
// first. rot / scale = Matrix_SplitScale(item); fragment constant 1 = {the
// camera in the item's frame, 1}. With the material parameters p (0.0
// each when missing): two projectors, each from three vectors of p (+12..
// for the first, +48.. for the second; 3 words each: an eye point, a
// target point and an up direction) taken through rot (eye_transform) and
// a Matrix_Perspective(p+4, p+92, p+84, p+88) by ProjTxr_BuildProjection
// (with rot as its item and kEyeBlock): the first as vertex constants
// 4..7 and its position as 8 ({xyz, 1}), the second as 9..12; fragment
// constant 0 = {p+60, p+64, p+68, 1}; vertex constants 13 = {p+96, p+100,
// 0, 0} and 14 = {p+112, ..+124}; two z rotations (Matrix_RotationZ by
// p+104, p+108) as parameter blocks 33..35 and 36..38 (eye_rows);
// fragment constant 3 = {p+0, p+8, fade, 0}, fade = clamp((6 - d) / 3, 0,
// 1) for d = -(the item position's view-space z (through kViewRotation)
// * kFovScale) above 3, else 1; then the lit draw with fragment constant
// 2 and the stencil bit 0x40. The locals sit at the original's frame
// offsets (one frame of its size) so the unwritten w lanes read the same
// stack bytes as the original.
void SlyCharacterEye_Draw(ShaderPrograms*, DrawItem* item) {
    constexpr u32 k = 0x005BE0D8;  // .text: f32 1.0, 0.0, 3.0, 1/255, 6.0; f64 1.0 (+24), 0.0 (+32)
    GuestFrame frame(1264);
    u8* sp = frame.as<u8>();
    const auto W = [&](u32 off) { return reinterpret_cast<be_u32*>(sp + off); };
    const auto F = [&](u32 off) { return reinterpret_cast<be_f32*>(sp + off); };
    upload_item_matrix(item);
    apply_fog(item);
    if ((item->variant & 1) != 0) upload_layer_list(item_mat(item), item->layers);
    u8* rot = sp + 464;
    Matrix_SplitScale(as_rows(item), F(464), F(112));
    eye_in(sp + 128, item);
    vec3_w1(sp + 144, sp + 128);
    fconst(1, sp + 144);
    Material* mat = item_mat(item);
    const auto pw = [&](u32 off) {
        const u32 params = mat->w124;
        return params != 0 ? host<be_u32>(params + off)->get() : global<be_u32>(k + 4).get();
    };
    const auto pf = [&](u32 off) { return as_f32(pw(off)); };
    for (u32 v = 0; v < 6; ++v) {
        for (u32 i = 0; i < 3; ++i) *W(160 + 16 * v + 4 * i) = pw(12 + 12 * v + 4 * i);
    }
    const f32 fov = pf(4), aspect = pf(92), znear = pf(84), zfar = pf(88);
    for (u32 i = 0; i < 3; ++i) *W(256 + 4 * i) = pw(60 + 4 * i);
    *W(268) = kOne;
    fconst(0, sp + 256);
    for (u32 v = 0; v < 6; ++v) eye_transform(sp + 160 + 16 * v, rot, v % 3 != 2);
    const be_f32* r = as_rows(rot);
    const be_f32* blk = host<be_f32>(kEyeBlock);
    Matrix_Perspective(F(848), fov, aspect, znear, zfar);
    ProjTxr_BuildProjection(F(272), F(528), F(160), F(176), F(192), F(848), r, blk);
    vconsts(4, 16, sp + 528);
    vec3_w1(sp + 352, sp + 272);
    vconsts(8, 4, sp + 352);
    Matrix_Perspective(F(784), fov, aspect, znear, zfar);
    ProjTxr_BuildProjection(F(272), F(592), F(208), F(224), F(240), F(784), r, blk);
    vconsts(9, 16, sp + 592);
    *W(288) = pw(96);
    *W(292) = pw(100);
    *W(296) = global<be_u32>(k + 4).get();
    *W(300) = global<be_u32>(k + 4).get();
    vconsts(13, 4, sp + 288);
    for (u32 i = 0; i < 16; i += 4) *W(304 + i) = pw(112 + i);
    vconsts(14, 4, sp + 304);
    Matrix_RotationZ(F(656), pf(104));
    eye_rows(sp + 368, sp + 656, 33);
    Matrix_RotationZ(F(656), pf(108));
    eye_rows(sp + 416, sp + 656, 36);
    *W(320) = pw(0);
    *W(324) = pw(8);
    *W(328) = kOne;
    *W(332) = global<be_u32>(k + 4).get();
    {
        // The item position (+48, lvlx scalars) into view space.
        const f32 x = lvlx_f32(item, 48), y = lvlx_f32(item, 52), z = lvlx_f32(item, 56);
        const be_f32* vr = host<be_f32>(kViewRotation);
        f32 p[4];
        for (u32 i = 0; i < 4; ++i) {
            f32 a = std::fmaf(vr[i], x, 0.0f);
            a = std::fmaf(vr[4 + i], y, a);
            a = std::fmaf(vr[8 + i], z, a);
            p[i] = vr[12 + i] + a;
        }
        for (u32 i = 0; i < 4; ++i) F(768)[i] = p[i];
        const f32 d = -(*F(776) * global<be_f32>(kFovScale));  // fmuls, fneg
        const f32 three = global<be_f32>(k + 8);
        if (d > three) {  // fcmpu, bc 12,1
            const f32 q = (global<be_f32>(k + 16) - d) / three;  // fsubs, fdivs
            f32 f = static_cast<f32>(fsel(global<be_f32>(k) - q, q, dbl(k + 24)));  // min(.., 1)
            f = static_cast<f32>(fsel(f, f, dbl(k + 32)));                           // max(.., 0)
            *F(328) = f;
        }
    }
    fconst(3, sp + 320);
    lit_draw(item, k + 12, Stencil::With40, 2);
}

namespace {

constexpr u32 kFurLight = 0x00F66FD0;   // u32: the fur shaders' light record (+32 f32[3] direction, +48 f32[4]) (shader4_draw.cpp)
constexpr u32 kLights = 0x010DD9F0;     // view block +624: 6 f32[4] light vectors (shader4_setup.cpp)
constexpr u32 kZeroVec = 0x01164BC0;    // f32[4] (fx::g::zero_vconsts)

// The render state's vertex / fragment program made the shader's (its
// +4 VProgram*, +8 FProgram*).
void bind_programs(ShaderPrograms* shader) {
    shader = or_zero_page(shader);
    bind_vertex(shader->vprogram);
    bind_fragment(shader->fprogram);
}

}  // namespace

// SlyCharacterFur: the item transform and the view fog; with the material
// parameters p (0.0 each when missing) and z = fx::g::zero_vconsts'
// words: c4 = {p+0, p+4, p+8, z.w}, vertex constants 5 = {p+28, p+32,
// z.z, z.w} and 8 = {p+52, p+56, z.z, z.w}; with the fur light (kFurLight,
// unchecked) vertex constants 6 = its +48 and 7 = {its direction in the
// item's frame (Vec3_RotateTo), 1}. A program index: 1 for a skinned mesh
// (Mesh +112 and +84 set), else 0; for a lit item (variant +69 bit 0) +
// 2 + 2 * the layer count, the layer list (unchecked) as vertex constants
// 16.., vertex constant 9 = {the camera in the item's frame, 1}, 33..36 =
// the item transposed (columns, then {0, 0, 0, 1}), 37..42 = the view
// block's 6 light vectors (+624) times the colour bytes of p+72 / 255
// (bytes 2, 1, 0, 3 per lane), fragment constant 1 = {the bytes of p+64
// / 255 * p+68, p+60}. The base pass draws with the type's shader [index]
// (bind_programs; material +16 +12) and the stencil (without 0x40); then
// the shell pass: shader [index + 10], depth test as the render state's
// +93 bit 0, no depth writes, alpha blending (BLEND_ENABLE is sent
// twice), cull on with a CCW front face; n = Fur_ShellCount(s, item,
// &scale); for n > 0 the length L = p+36 * scale, the view fog again and
// per shell i (t = i / n) c4.w = L * t as vertex constant 4 and
// fragment constant 0 = {L * t / L, (t * (1 - p+40) + p+40) * p+48, p+48,
// p+44}, each a mesh draw. Then render::g::state_cache_a / _b = -1 and
// RenderLayer_End.
void SlyCharacterFur_Draw(ShaderPrograms* shader, DrawItem* item) {
    constexpr u32 k = 0x005BFCF0;  // .text: f32 0.0, 1.0, 1/255; vperm mask (+16); {0, 0, 0, 1} (+32)
    upload_item_matrix(item);
    apply_fog(item);
    // The original's sp+112 (c4), +128 (c5), +144 (c0), +160 (c8), +176 /
    // +192 (the light direction in / out), +208 (c9), +272 (c7).
    GuestFrame frame(192);
    be_u32* c4 = frame.as<be_u32>(0);
    be_u32* c5 = frame.as<be_u32>(16);
    be_u32* c0 = frame.as<be_u32>(32);
    be_u32* c8 = frame.as<be_u32>(48);
    be_f32* din = frame.as<be_f32>(64);
    be_f32* dout = frame.as<be_f32>(80);
    be_u32* c9 = frame.as<be_u32>(96);
    be_u32* c7 = frame.as<be_u32>(112);
    for (u32 v = 0; v < 4; ++v) raw4(c4 + 4 * v, host<u8>(kZeroVec));
    Material* mat0 = item_mat(item);
    const auto pw = [&](u32 off) {
        const u32 params = item_mat(item)->w124;
        return params != 0 ? host<be_u32>(params + off)->get() : global<be_u32>(k).get();
    };
    const auto pf = [&](u32 off) { return as_f32(pw(off)); };
    f32 length = pf(36);
    c4[0] = pw(0);
    c4[1] = pw(4);
    c4[2] = pw(8);
    c5[0] = pw(28);
    c5[1] = pw(32);
    const f32 base = pf(40);
    const u32 tip_w = pw(48);
    const f32 tip = as_f32(tip_w);
    c0[2] = tip_w;
    c0[3] = pw(44);
    c8[0] = pw(52);
    c8[1] = pw(56);
    vconsts(5, 4, c5);
    vconsts(8, 4, c8);
    const u32 light = global<be_u32>(kFurLight);
    copy3(din, at<u8>(nullptr, light + 32));
    Vec3_RotateTo(dout, din, as_rows(item));
    vconsts(6, 4, at<u8>(nullptr, light + 48));
    vec3_w1(c7, dout);
    vconsts(7, 4, c7);
    for (u32 i = 1; i < 4; ++i) copy_word(c9 + i, host<be_u32>(kZeroVec) + i);
    copy_word(c9, host<u8>(kZeroVec));
    const DrawMesh* mesh = or_zero_page(item->mesh.get());
    u32 index = mesh->skin.ea != 0 && mesh->palette.ea != 0 ? 1 : 0;
    if ((item->variant & 1) != 0) {
        const be_u32* list = item->layers;
        index = ((*at<be_u32>(list, 0) << 1) | (index & 1)) + 2;  // rlwimi 1,0,30
        upload_layer_list(item_mat(item), list);
        {
            GuestFrame f(16);  // the original's sp+224
            eye_in(f.as<u8>(), item);
            vec3_w1(c9, f.as<u8>());
        }
        vconsts(9, 4, c9);
        const f32 scale = global<be_f32>(k + 8);
        const auto bytes = [&](u32 w, u32 sh) { return byte_scaled((w >> sh) & 0xFF, scale); };  // fcfid, frsp, fmuls
        const u32 tint = pw(72);
        const f32 r = bytes(tint, 16), g = bytes(tint, 8), b = bytes(tint, 0), a = bytes(tint, 24);
        GuestFrame f(80);  // the original's sp+288 (columns) / +256 (a light)
        be_u32* cols = f.as<be_u32>(0);
        be_f32* lc = f.as<be_f32>(64);
        for (u32 j = 0; j < 3; ++j) {
            for (u32 i = 0; i < 4; ++i) cols[4 * j + i] = lvlx_word0(at<u8>(item, 16 * i + 4 * j));  // vmrghw, vperm
        }
        raw4(cols + 12, host<u8>(k + 32));
        vconsts(33, 16, cols);
        for (u32 n = 0; n < 6; ++n) {  // cmplwi
            const be_f32* src = host<be_f32>(kLights + 16 * n);
            lc[0] = src[0] * r;
            lc[1] = src[1] * g;
            lc[2] = src[2] * b;
            lc[3] = src[3] * a;
            vconsts(37 + n, 4, lc);
        }
        const f32 strength = pf(68);
        const u32 colour = pw(64);
        GuestFrame f1(16);  // the original's sp+240
        be_f32* c1 = f1.as<be_f32>();
        reinterpret_cast<be_u32*>(c1)[3] = pw(60);
        c1[0] = bytes(colour, 16) * strength;
        c1[1] = bytes(colour, 8) * strength;
        c1[2] = bytes(colour, 0) * strength;
        fconst(1, c1);
    }
    const u32 shaders = *at<be_u32>(*at<gp<u8>>(mat0, 16), 12);
    ShaderPrograms* base_shader = host<ShaderPrograms>(*at<be_u32>(nullptr, shaders + 4 * index));
    ShaderPrograms* shell_shader = host<ShaderPrograms>(*at<be_u32>(nullptr, shaders + 4 * (index + 10)));
    bind_programs(base_shader);
    item_stencil(item, item->flags68, 0);
    draw_mesh(item);
    bind_programs(shell_shader);
    emit({kDepthTestEnable, rs().dirty_b[1] & 1u});
    emit({kDepthMask, 0});
    emit({kBlendEnable, 1});
    blend_on(kSrcAlpha2, kOneMinusSrcAlpha2);
    emit({kCullFaceEnable, 1});
    emit({kFrontFace, kGlCcw});
    GuestFrame fs(16);  // the original's sp+544
    const u32 n = Fur_ShellCount(shader, item, fs.as<be_f32>());
    if (static_cast<s32>(n) > 0) {  // cmpwi
        length = length * *fs.as<be_f32>();
        apply_fog(item);
        const f32 span = global<be_f32>(k + 4) - base;  // fsubs
        const f32 nf = static_cast<f32>(static_cast<f64>(n));  // fcfid, frsp
        be_f32* c4f = reinterpret_cast<be_f32*>(c4);
        be_f32* c0f = reinterpret_cast<be_f32*>(c0);
        for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(n); i = (i + 1) & 0xFFFF) {  // cmpw
            const f32 t = static_cast<f32>(i) / nf;
            const f32 len = length * t;
            const f32 fade = fmadds(span, t, base);
            c4f[3] = len;
            c0f[1] = fade * tip;
            c0f[0] = len / length;
            vconsts(4, 4, c4);
            fconst(0, c0);
            draw_mesh(item);
        }
    }
    global<be_u64>(kStateCacheA) = ~u64{0};
    global<be_u64>(kStateCacheB) = ~u64{0};
    RenderLayer_End();
}

}  // namespace sly4::gfx

NATIVE(005CFDC0, sly4::gfx::SlyTerrainSnowDecal_Draw)
NATIVE(005CE6A0, sly4::gfx::SlyTerrainSnow_Draw)
NATIVE(005CAE18, sly4::gfx::SlyProp_Draw)
NATIVE(005C8338, sly4::gfx::SlyMirror_Draw)
NATIVE(005BE100, sly4::gfx::SlyCharacterEye_Draw)
NATIVE(005BFD20, sly4::gfx::SlyCharacterFur_Draw)
