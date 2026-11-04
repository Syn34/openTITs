// The last shader Draw slots (Shader vtable slot 12):
// void <Shader>_Draw(Shader* s, DrawItem* item)
#include "shader6.h"

using namespace sly4;
using namespace sly4::render::shader4::util;

namespace {

namespace s4 = sly4::render::shader4;
namespace s5 = sly4::render::shader5;
namespace s6 = sly4::render::shader6;
namespace l4 = sly4::render::layers4;

constexpr u32 kLightDir = 0x010DD8A0;     // view block +288: the camera rotation's column 2 (shader5_draw.cpp)
constexpr u32 kEye = 0x010DD8B0;          // view block +304: the camera position (render::g::camera_pos)
constexpr u32 kViewColumns = 0x010DD880;  // view block +256: the camera rotation transposed (3 rows)
constexpr u32 kViewRows320 = 0x010DD8C0;  // view block +320: view +1200.. (f32 rows)
constexpr u32 kFrameDelta = 0x010DD5AC;   // f32 (layers3::g::block_dd5a8 +4): the frame's time step (name?)
constexpr u32 kLayerScale = 0x010DD5B4;   // f32 (block_dd5a8 +12): the view's layer scale (shader4_frame.cpp kFogScale)
constexpr u32 Matrix_NearlyEqual = 0x005545B4;  // (const f32[16], const f32[16]; f1 eps) -> bool (audio/lib_math.cpp)
constexpr u32 kMeshBounds = 92;           // Mesh +92: f32 centre[3], radius

// {xyz of src, 1.0} into dst.
void vec_w1(u32 dst, u32 src) {
    for (u32 i = 0; i < 12; i += 4) sp::copy_word(dst + i, src + i);
    store<u32>(dst + 12, s2::kOne);
}

// The lit draws' pieces (shader4_util.h lit_draw): fragment constant
// `fconst` = `first`; for a translucent item (alpha +71 below 255) no
// depth writes and fragment constant `fconst` = {own blending ? 0 : 1,
// alpha * k, z_word, 0} (alpha blending on unless the material blends
// itself, material +104 bit 31).
void translucent_begin(PPUContext& ctx, u32 item, u32 k, u32 fconst, u32 first = s3::g::character_fconst2, u32 z_word = 0) {
    call(ctx, d3::fn::FProgram_SetConstant, {fconst, first});
    if (ref<u8>(item + kItemAlpha) >= 255) return;  // cmpwi
    fx::emit({l4::kDepthMask, 0});
    Stack frame(ctx, 16);
    const u32 v = frame.addr();
    store<u32>(v, 0);
    store<u32>(v + 8, z_word);
    store<u32>(v + 12, 0);
    sp::stfs(v + 4, byte_scaled(ref<u8>(item + kItemAlpha), sp::lfs(k)));  // fcfid, frsp, fmuls
    if (!mat_bit31(load<u32>(item + d3::kItemMaterial), kMatOwnBlend)) {
        fx::blend_on(fx::kSrcAlpha2, fx::kOneMinusSrcAlpha2);
        store<u32>(v, s2::kOne);
    }
    call(ctx, d3::fn::FProgram_SetConstant, {fconst, v});
}

// A mirrored item (+68 bit 7) draws with the front face flipped.
void mirror_begin(u32 item) {
    if ((ref<u8>(item + kItemFlags) & kMirrored) == 0) return;  // rlwinm 25,31,31
    fx::emit({l4::kCullEnable, 1});
    fx::emit({l4::kFrontFace, l4::kCw});
}
void mirror_end(u32 item) {
    if ((ref<u8>(item + kItemFlags) & kMirrored) == 0) return;
    fx::emit({l4::kCullEnable, 1});
    fx::emit({l4::kFrontFace, l4::kCcw});
}

// After a translucent draw the depth mask (material +112 bit 31) and
// blending are restored; r3 as lit_draw leaves it.
void translucent_end(PPUContext& ctx, u32 item) {
    if (ref<u8>(item + kItemAlpha) >= 255) {
        ctx.gpr[3] = ref<u8>(item + kItemAlpha);
        return;
    }
    if (mat_bit31(load<u32>(item + d3::kItemMaterial), kMatDepthRestore)) {
        fx::emit({l4::kDepthMask, ref<u8>(fx::g::state + 93) & 1u});
    }
    if (!mat_bit31(load<u32>(item + d3::kItemMaterial), kMatOwnBlend)) {
        fx::emit({l4::kBlendEnable, 0});
        ctx.gpr[3] = gcm();
    } else {
        ctx.gpr[3] = 1;
    }
}

// For a lit item (variant +69 bit 0) the layer list (unchecked) as
// fragment constants 16.. when variant bit 1 is set, else as vertex
// constants 16..; with variant bit 2 fragment constant 1 = {the camera in
// the item's frame, 1}. (shader5_draw.cpp's lit_layers.)
void lit_layers(PPUContext& ctx, u32 item) {
    if ((ref<u8>(item + d3::kItemVariant) & 1) == 0) return;
    if ((ref<u8>(item + d3::kItemVariant) & 2) != 0) {
        upload_layers_fragment(ctx, item);
    } else {
        upload_layer_list(ctx, load<u32>(item + d3::kItemMaterial), load<u32>(item + d3::kItemLayers));
    }
    if ((ref<u8>(item + d3::kItemVariant) & 4) != 0) {
        Stack frame(ctx, 32);  // the original's sp+112 / +128
        const u32 dir = frame.addr(), c1 = dir + 16;
        call(ctx, s4::fn::Listener_LocalDirection, {dir, kEye, item});
        vec_w1(c1, dir);
        call(ctx, d3::fn::FProgram_SetConstant, {1, c1});
    }
}

// {r, g, b, 1} from the packed colour word's bytes 2 / 1 / 0, each
// through fcfid / frsp times k (1/255), as fragment constant `index`.
void packed_colour(PPUContext& ctx, u32 index, u32 word, u32 k) {
    Stack frame(ctx, 16);  // the original's sp+224
    const u32 c = frame.addr();
    const f64 scale = sp::lfs(k);
    store<u32>(c + 12, s2::kOne);
    sp::stfs(c, sp::sgl(sp::sgl(static_cast<f64>((word >> 16) & 0xFF)) * scale));  // rlwinm 16,24,31
    sp::stfs(c + 4, sp::sgl(sp::sgl(static_cast<f64>((word >> 8) & 0xFF)) * scale));
    sp::stfs(c + 8, sp::sgl(sp::sgl(static_cast<f64>(word & 0xFF)) * scale));
    call(ctx, d3::fn::FProgram_SetConstant, {index, c});
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
void snow_scroll(PPUContext& ctx, u32 mat, const Snow& s, bool vertex) {
    const u32 k = s.k;
    const u32 zero = load<u32>(k + 4);
    u32 params = load<u32>(mat + d3::kMatParams);
    const auto pw = [&](u32 off) { return params != 0 ? load<u32>(params + off) : zero; };
    const auto pf = [&](u32 off) { return static_cast<f64>(std::bit_cast<f32>(pw(off))); };
    const u32 cur_u = pw(s.cur), cur_v = pw(s.cur + 4);  // f31 / f30
    u32 out_u = cur_u, out_v = cur_v;
    const u32 time_w = load<u32>(s3::g::material_time);
    const f64 time = std::bit_cast<f32>(time_w);
    if (!(sp::lfs(s.last_time) == time)) {  // fcmpu, bc 4,2
        store<u32>(s.last_time, time_w);
        callf(ctx, Matrix_NearlyEqual, {kViewRows320, kViewColumns}, {sp::lfs(k + 20)});
        store<u8>(s.moved, static_cast<u32>(ctx.gpr[3]) == 0 ? 1 : 0);  // cntlzw, rlwinm 27,31,31
        params = load<u32>(mat + d3::kMatParams);
    }
    if (!(pf(s.clock) == time)) {
        store<u32>(static_cast<u32>(call(ctx, s5::fn::Material_ParamSlot, {mat, s.slot_clock})), time_w);
        params = load<u32>(mat + d3::kMatParams);
        const f64 dt = sp::lfs(kFrameDelta), scale = sp::lfs(kLayerScale);
        f64 su = pf(s.rate), sv = pf(s.rate + 4);
        if (ref<u8>(s.moved) != 0) {
            su = pf(s.rate_moved);
            sv = pf(s.rate_moved + 4);
        }
        const f64 step = sp::sgl(sp::sgl(dt * scale) * sp::lfs(k + 8));  // fmuls x2
        const f64 du = sp::sgl(step * su), dv = sp::sgl(step * sv);
        const f64 one = sp::lfs(k + 12), minus_one = sp::lfs(k + 16);
        const f64 u0 = std::bit_cast<f32>(cur_u), v0 = std::bit_cast<f32>(cur_v);
        f64 u = sp::sgl(u0 + du), v = sp::sgl(v0 + dv);  // fadds
        if (u > one) {  // fcmpu, bc 12,1
            v = sp::sgl(sp::lfs(k + 4) + v);
            u = sp::sgl(minus_one + u);
        }
        if (v > one) {
            u = sp::sgl(sp::lfs(k + 4) + u);
            v = sp::sgl(minus_one + v);
        }
        sp::stfs(static_cast<u32>(call(ctx, s5::fn::Material_ParamSlot, {mat, s.slot_cur})), u);
        sp::stfs(static_cast<u32>(call(ctx, s5::fn::Material_ParamSlot, {mat, s.slot_cur + 1})), v);
        out_u = std::bit_cast<u32>(static_cast<f32>(u));
        out_v = std::bit_cast<u32>(static_cast<f32>(v));
    }
    Stack frame(ctx, 16);  // the original's sp+144
    const u32 c = frame.addr();
    store<u32>(c, out_u);
    store<u32>(c + 4, out_v);
    store<u32>(c + 8, 0);
    store<u32>(c + 12, 0);
    if (vertex) {
        set_vconsts(ctx, 8, 4, c);
    } else {
        call(ctx, d3::fn::FProgram_SetConstant, {1, c});
    }
}

// Whether the mesh's bounding sphere (Mesh +92) lies beyond the camera
// along the light: -((centre - eye) . dir) > radius + p +margin + p
// +margin+4, eye the camera and dir kLightDir both in the item's frame.
bool snow_far(PPUContext& ctx, u32 item, u32 mat, const Snow& s) {
    const u32 params = load<u32>(mat + d3::kMatParams);
    const auto pf = [&](u32 off) { return params != 0 ? sp::lfs(params + off) : sp::lfs(s.k + 4); };
    const f64 m0 = pf(s.margin), m1 = pf(s.margin + 4);
    const u32 b = load<u32>(item + d3::kItemMesh) + kMeshBounds;
    const f64 cx = sp::lfs(b), cy = sp::lfs(b + 4), cz = sp::lfs(b + 8), radius = sp::lfs(b + 12);
    Stack frame(ctx, 32);  // the original's sp+192 (eye) / +208 (dir)
    const u32 eye = frame.addr(), dir = eye + 16;
    call(ctx, s4::fn::Listener_LocalDirection, {eye, kEye, item});
    call(ctx, s4::fn::Vec3_RotateTo, {dir, kLightDir, item});
    const f64 dy = sp::sgl(cy - sp::lfs(eye + 4));
    const f64 dx = sp::sgl(cx - sp::lfs(eye));
    const f64 dz = sp::sgl(cz - sp::lfs(eye + 8));
    const f64 reach = sp::sgl(sp::sgl(radius + m0) + m1);  // fadds x2
    f64 d = sp::sgl(dy * sp::lfs(dir + 4));
    d = sp::fmadds(dx, sp::lfs(dir), d);
    d = sp::sgl(-std::fma(dz, sp::lfs(dir + 8), d));  // fnmadds
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
void snow_draw(PPUContext& ctx, u32 item, const Snow& s, bool decal) {
    const u32 k = s.k;
    const u32 mat = load<u32>(item + d3::kItemMaterial);
    upload_item_transform(item);
    apply_fog(ctx, item);
    if ((ref<u8>(item + d3::kItemVariant) & (decal ? 1 : 4)) != 0) {
        {
            Stack frame(ctx, 32);  // the original's sp+112 / +128
            const u32 eye = frame.addr(), c = eye + 16;
            call(ctx, s4::fn::Listener_LocalDirection, {eye, kEye, item});
            vec_w1(c, eye);
            call(ctx, d3::fn::FProgram_SetConstant, {decal ? 3u : 2u, c});
        }
        const u32 params = load<u32>(mat + d3::kMatParams);
        packed_colour(ctx, decal ? 4 : 3, params != 0 ? load<u32>(params + (decal ? 28 : 40)) : 0, k);
        if (decal) upload_layer_list(ctx, load<u32>(item + d3::kItemMaterial), load<u32>(item + d3::kItemLayers));
    }
    if (!decal && (ref<u8>(item + d3::kItemVariant) & 1) != 0) {
        upload_layer_list(ctx, load<u32>(item + d3::kItemMaterial), load<u32>(item + d3::kItemLayers));
    }
    if ((ref<u8>(item + d3::kItemVariant) & 2) != 0) {  // rlwinm 0,30,30
        snow_scroll(ctx, mat, s, !decal);
        const bool far = snow_far(ctx, item, mat, s);
        const u32 params = load<u32>(mat + d3::kMatParams);
        Stack frame(ctx, 16);  // the original's sp+160
        const u32 c = frame.addr();
        store<u32>(c, far || params == 0 ? load<u32>(k + 4) : load<u32>(params + s.shade));
        store<u32>(c + 12, 0);
        if (decal) {
            store<u32>(c + 4, params != 0 ? load<u32>(params + s.shade + 4) : load<u32>(k + 4));
            store<u32>(c + 8, params != 0 ? load<u32>(params + s.shade + 8) : load<u32>(k + 4));
            call(ctx, d3::fn::FProgram_SetConstant, {2, c});
        } else {
            const f64 a = params != 0 ? sp::lfs(params + s.shade + 4) : sp::lfs(k + 4);
            const f64 b = params != 0 ? sp::lfs(params + s.shade + 8) : sp::lfs(k + 4);
            const f64 q = sp::sgl(a / b);                      // fdivs
            sp::stfs(c + 8, -q);                               // fneg
            sp::stfs(c + 4, sp::sgl(sp::lfs(k + 12) / b));     // fdivs
            call(ctx, d3::fn::FProgram_SetConstant, {1, c});
        }
    }
    lit_draw(ctx, item, k, Stencil::Plain, 0);
}

}  // namespace

// SlyTerrainSnowDecal: snow_draw (the decal variant).
DECOMP(005CFDC0, SlyTerrainSnowDecal_Draw) {
    static constexpr Snow s{0x005CFDA4, 0x00F8B0F0, 0x00F8B0F4, 72, 64, 8, 80, 20, 18, 56, 16};
    snow_draw(ctx, arg<u32>(ctx, 1), s, true);
}

// SlyTerrainSnow: snow_draw (the plain variant).
DECOMP(005CE6A0, SlyTerrainSnow_Draw) {
    static constexpr Snow s{0x005CE684, 0x00F8B0E8, 0x00F8B0EC, 60, 52, 20, 68, 17, 15, 44, 28};
    snow_draw(ctx, arg<u32>(ctx, 1), s, false);
}

// SlyProp: the item transform and the view fog; vertex constant 7 = the
// item's tint bytes (+72..74) / 256 with w 255 / 256 (lvsl / vupkhsb /
// vupkhsh, masked to 0xFF, vcfsx 8); the lit layers (lit_layers); the lit
// draw's translucent constant (fragment constant 2) and mirroring; then
// its own stencil: bits = (stencil bit 7 | material +101) & the write
// mask; when bits | 0x40 differs from the cached reference it becomes the
// reference (the cache, render::g::stencil_bytes[0], gets bits: as in the
// original, without the 0x40) with the write mask bits (or the mask when
// bits are 0); with material +100 bit 3 the function NOTEQUAL against
// mask 0x40; the mesh's draw; after it the reference is cleared (render
// state +164) when bits are 0, the function goes back to ALWAYS / mask 0
// with material +100 bit 3, and either case re-sends the function.
DECOMP(005CAE18, SlyProp_Draw) {
    constexpr u32 k = 0x005CADF0;  // .text: u32 24, vperm mask (+16), f32 1/255 (+32)
    constexpr u32 kItemTint = 72;  // DrawItem +72: u8[4] (name?)
    constexpr u32 kNotEqual = 0x0205, kAlways = 0x0207;
    const u32 item = arg<u32>(ctx, 1);
    const u32 st = fx::g::state;
    upload_item_transform(item);
    apply_fog(ctx, item);
    {
        const u32 c = gcm();
        const u32 cur = load<u32>(c + 8);
        store<u32>(c + 8, cur + 24);
        store<u32>(cur, 0x00141EFCu);  // SET_TRANSFORM_CONSTANT_LOAD, 5 words
        store<u32>(cur + 4, 7);
        for (u32 i = 0; i < 3; ++i) store<f32>(cur + 8 + 4 * i, static_cast<f32>(ref<u8>(item + kItemTint + i)) / 256.0f);
        store<f32>(cur + 20, 255.0f / 256.0f);
    }
    lit_layers(ctx, item);
    translucent_begin(ctx, item, k + 32, 2);
    mirror_begin(item);
    const u32 flags = ref<u8>(item + kItemFlags);
    u32 mat = load<u32>(item + d3::kItemMaterial);
    const u32 mask = ref<u8>(render::g::stencil_bytes + 1);
    const u32 bits = (((flags & kStencilHigh) != 0 ? 0x80u : 0u) | ref<u8>(mat + kMatStencil)) & mask;  // rlwinm 27,31,31
    const u32 test = (ref<u8>(mat + d3::kMatFlags) & 8) != 0 ? 0x40u : 0u;  // rlwinm 29,31,31
    const u32 ref_value = bits | 0x40;
    if (ref_value != ref<u8>(render::g::stencil_bytes)) {
        store<u8>(render::g::stencil_bytes, static_cast<u8>(bits));
        store<u32>(st + 164, ref_value);
        fx::emit({sp::kStencilFunc, load<u32>(st + 160), ref_value, load<u32>(st + 172)});
        const u32 write = bits != 0 ? bits : mask;
        store<u32>(st + 168, write);
        fx::emit({sp::kStencilMask, write});
    }
    if (test != 0) {
        store<u32>(st + 160, kNotEqual);
        store<u32>(st + 172, test);
        fx::emit({sp::kStencilFunc, kNotEqual, load<u32>(st + 164), test});
    }
    draw_mesh(ctx, item);
    mirror_end(item);
    translucent_end(ctx, item);
    if (bits == 0) store<u32>(st + 164, 0);
    mat = load<u32>(item + d3::kItemMaterial);
    const bool tested = (ref<u8>(mat + d3::kMatFlags) & 8) != 0;
    if (tested) {
        store<u32>(st + 160, kAlways);
        store<u32>(st + 172, 0);
    }
    if (tested || bits == 0) {
        fx::emit({sp::kStencilFunc, load<u32>(st + 160), load<u32>(st + 164), load<u32>(st + 172)});
        ctx.gpr[3] = gcm();  // as the original leaves it
    } else {
        ctx.gpr[3] = 0;
    }
}

namespace {

constexpr u32 kViewRotation = 0x010DD840;  // view block +192 (shader4_draw.cpp)
constexpr u32 kMirrorBias = 0x01197260;    // f32[4][4]: SlyMirror's clip-to-texture bias, built on first use (name?)
constexpr u32 kMirrorBiasSet = 0x01197250;  // u8: kMirrorBias built (name?)

// out row i = m row 0 * rows[i].x + m row 1 * .y + m row 2 * .z (+ m row 3
// for i = 3) per lane in vmaddfp order (rows and m read with lvx), as
// upload_item_transform computes it.
void rows_through(u32 out, u32 rows, u32 m) {
    rows &= ~15u;
    m &= ~15u;
    for (u32 i = 0; i < 4; ++i) {
        const u32 row = rows + 16 * i;
        const f32 x = load<f32>(row), y = load<f32>(row + 4), z = load<f32>(row + 8);
        for (u32 k = 0; k < 4; ++k) {
            f32 v = std::fmaf(load<f32>(m + 4 * k), x, i == 3 ? load<f32>(m + 48 + 4 * k) : 0.0f);
            v = std::fmaf(load<f32>(m + 16 + 4 * k), y, v);
            store<f32>(out + 16 * i + 4 * k, std::fmaf(load<f32>(m + 32 + 4 * k), z, v));
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
DECOMP(005C8338, SlyMirror_Draw) {
    constexpr u32 k = 0x005C832C;  // .text: f32 0.0, 1/255
    const u32 item = arg<u32>(ctx, 1);
    const u32 mat = load<u32>(item + d3::kItemMaterial);
    const u32 params = load<u32>(mat + d3::kMatParams);
    // The original's sp+208 (M), +272 (item * M), +336 (* bias), +400 (transposed).
    Stack frame(ctx, 256);
    const u32 m = frame.addr(), t = m + 64, r = m + 128, tr = m + 192;
    for (u32 i = 0; i < 64; i += 4) store<u32>(m + i, params != 0 ? load<u32>(params + 12 + i) : load<u32>(k));
    upload_item_transform(item);
    if (ref<u8>(kMirrorBiasSet) == 0) {
        const u32 half = 0x3F000000u, zero = load<u32>(k);
        const u32 words[16] = {half, zero, zero, zero, zero, 0xBF000000u, zero, zero, zero, zero, half, zero, half, half, half, s2::kOne};
        for (u32 i = 0; i < 16; ++i) store<u32>(kMirrorBias + 4 * i, words[i]);
        store<u8>(kMirrorBiasSet, 1);
    }
    rows_through(t, item, m);
    call(ctx, s5::fn::Matrix_MultiplyTo, {r, t, kMirrorBias});
    for (u32 i = 0; i < 4; ++i) {
        for (u32 j = 0; j < 4; ++j) store<u32>(tr + 16 * j + 4 * i, load<u32>(r + 16 * i + 4 * j));  // vmrghw / vmrglw
    }
    set_vconsts(ctx, 4, 16, tr);
    call(ctx, d3::fn::FProgram_SetConstant, {2, mat + kMatColour});
    apply_fog(ctx, item);
    // The original's sp+160 (rows), +112 (camera), +128 (c).
    Stack frame2(ctx, 80);
    const u32 rows = frame2.addr(), eye = rows + 48, c = rows + 64;
    call(ctx, s4::fn::Matrix_RotateRows, {rows, item, kViewRotation});
    call(ctx, s4::fn::Listener_LocalDirection, {eye, kEye, item});
    vec_w1(c, eye);
    const u32 variant = ref<u8>(item + d3::kItemVariant);
    if ((variant & 1) != 0 && (variant & 2) != 0) {
        upload_layers_fragment(ctx, item);
        for (u32 i = 0; i < 3; ++i) call(ctx, d3::fn::FProgram_SetConstant, {3 + i, rows + 16 * i});
        call(ctx, d3::fn::FProgram_SetConstant, {6, c});
    } else {
        if ((variant & 1) != 0) upload_layer_list(ctx, load<u32>(item + d3::kItemMaterial), load<u32>(item + d3::kItemLayers));
        for (u32 i = 0; i < 3; ++i) call(ctx, s2::fn::cellGcmSetVertexProgramParameterBlock, {gcm(), 10 + i, 1, rows + 16 * i});
        set_vconsts(ctx, 13, 4, c);
    }
    lit_draw(ctx, item, k + 4, Stencil::Plain, 1);
}

namespace {

constexpr u32 Matrix_SplitScale = 0x00554858;  // (const f32 m[16], f32 rot[16], f32 scale[3]): rows 0..2 normalised (their lengths to scale), row 3 xyz copied; w lanes not written (name?)
constexpr u32 Matrix_RotationZ = 0x00553F18;   // (f32 out[16]; f1 angle) a rotation about z (polynomial sine); w lanes not written (name?)
constexpr u32 kEyeBlock = 0x01196A40;          // f32[16]: ProjTxr_BuildProjection's block for the eye projectors (name?)
constexpr u32 kFovScale = 0x010DD9E4;          // view block +612: min(1, tan(fov / 2) / tan(k)) (shader5.h)

// row (f32[4] at `v`, read as 3 scalars) = rot row 0 * x + row 1 * y + row
// 2 * z (+ row 3 for a point; vaddfp) per lane in vmaddfp order, rot read
// with lvx, all 4 lanes stored (stvx; lane 3 from rot's w lanes).
void eye_transform(u32 v, u32 rot, bool point) {
    const f32 x = std::bit_cast<f32>(lvlx_word0(v)), y = std::bit_cast<f32>(lvlx_word0(v + 4)), z = std::bit_cast<f32>(lvlx_word0(v + 8));
    rot &= ~15u;
    f32 out[4];
    for (u32 k = 0; k < 4; ++k) {
        f32 a = std::fmaf(load<f32>(rot + 4 * k), x, 0.0f);
        a = std::fmaf(load<f32>(rot + 16 + 4 * k), y, a);
        a = std::fmaf(load<f32>(rot + 32 + 4 * k), z, a);
        out[k] = point ? load<f32>(rot + 48 + 4 * k) + a : a;
    }
    for (u32 k = 0; k < 4; ++k) store<f32>((v & ~15u) + 4 * k, out[k]);
}

// The xyz of `rows` 3 rows (16 bytes apart) to dst's 3 rows and each as a
// vertex constant parameter block from `first`. The w lanes of dst are
// never written: what the guest stack holds there goes out with them.
void eye_rows(PPUContext& ctx, u32 dst, u32 rows, u32 first) {
    for (u32 r = 0; r < 3; ++r) {
        for (u32 i = 0; i < 12; i += 4) sp::copy_word(dst + 16 * r + i, rows + 16 * r + i);
    }
    for (u32 r = 0; r < 3; ++r) call(ctx, s2::fn::cellGcmSetVertexProgramParameterBlock, {gcm(), first + r, 1, dst + 16 * r});
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
DECOMP(005BE100, SlyCharacterEye_Draw) {
    constexpr u32 k = 0x005BE0D8;  // .text: f32 1.0, 0.0, 3.0, 1/255, 6.0; f64 1.0 (+24), 0.0 (+32)
    const u32 item = arg<u32>(ctx, 1);
    Stack frame(ctx, 1264);
    const u32 sp = frame.addr();
    upload_item_transform(item);
    apply_fog(ctx, item);
    if ((ref<u8>(item + d3::kItemVariant) & 1) != 0) {
        upload_layer_list(ctx, load<u32>(item + d3::kItemMaterial), load<u32>(item + d3::kItemLayers));
    }
    const u32 rot = sp + 464;
    call(ctx, Matrix_SplitScale, {item, rot, sp + 112});
    call(ctx, s4::fn::Listener_LocalDirection, {sp + 128, kEye, item});
    vec_w1(sp + 144, sp + 128);
    call(ctx, d3::fn::FProgram_SetConstant, {1, sp + 144});
    const u32 mat = load<u32>(item + d3::kItemMaterial);
    const auto pw = [&](u32 off) {
        const u32 params = load<u32>(mat + d3::kMatParams);
        return params != 0 ? load<u32>(params + off) : load<u32>(k + 4);
    };
    const auto pf = [&](u32 off) { return static_cast<f64>(std::bit_cast<f32>(pw(off))); };
    for (u32 v = 0; v < 6; ++v) {
        for (u32 i = 0; i < 3; ++i) store<u32>(sp + 160 + 16 * v + 4 * i, pw(12 + 12 * v + 4 * i));
    }
    const f64 fov = pf(4), aspect = pf(92), znear = pf(84), zfar = pf(88);
    for (u32 i = 0; i < 3; ++i) store<u32>(sp + 256 + 4 * i, pw(60 + 4 * i));
    store<u32>(sp + 268, s2::kOne);
    call(ctx, d3::fn::FProgram_SetConstant, {0, sp + 256});
    for (u32 v = 0; v < 6; ++v) eye_transform(sp + 160 + 16 * v, rot, v % 3 != 2);
    callf(ctx, s4::fn::Matrix_Perspective, {sp + 848}, {fov, aspect, znear, zfar});
    call(ctx, s4::fn::ProjTxr_BuildProjection, {sp + 272, sp + 528, sp + 160, sp + 176, sp + 192, sp + 848, rot, kEyeBlock});
    set_vconsts(ctx, 4, 16, sp + 528);
    for (u32 i = 0; i < 12; i += 4) sp::copy_word(sp + 352 + i, sp + 272 + i);
    store<u32>(sp + 364, s2::kOne);
    set_vconsts(ctx, 8, 4, sp + 352);
    callf(ctx, s4::fn::Matrix_Perspective, {sp + 784}, {fov, aspect, znear, zfar});
    call(ctx, s4::fn::ProjTxr_BuildProjection, {sp + 272, sp + 592, sp + 208, sp + 224, sp + 240, sp + 784, rot, kEyeBlock});
    set_vconsts(ctx, 9, 16, sp + 592);
    store<u32>(sp + 288, pw(96));
    store<u32>(sp + 292, pw(100));
    store<u32>(sp + 296, load<u32>(k + 4));
    store<u32>(sp + 300, load<u32>(k + 4));
    set_vconsts(ctx, 13, 4, sp + 288);
    for (u32 i = 0; i < 16; i += 4) store<u32>(sp + 304 + i, pw(112 + i));
    set_vconsts(ctx, 14, 4, sp + 304);
    callf(ctx, Matrix_RotationZ, {sp + 656}, {pf(104)});
    eye_rows(ctx, sp + 368, sp + 656, 33);
    callf(ctx, Matrix_RotationZ, {sp + 656}, {pf(108)});
    eye_rows(ctx, sp + 416, sp + 656, 36);
    store<u32>(sp + 320, pw(0));
    store<u32>(sp + 324, pw(8));
    store<u32>(sp + 328, s2::kOne);
    store<u32>(sp + 332, load<u32>(k + 4));
    {
        // The item position (+48, lvlx scalars) into view space.
        const f32 x = std::bit_cast<f32>(lvlx_word0(item + 48)), y = std::bit_cast<f32>(lvlx_word0(item + 52)), z = std::bit_cast<f32>(lvlx_word0(item + 56));
        f32 p[4];
        for (u32 i = 0; i < 4; ++i) {
            f32 a = std::fmaf(load<f32>(kViewRotation + 4 * i), x, 0.0f);
            a = std::fmaf(load<f32>(kViewRotation + 16 + 4 * i), y, a);
            a = std::fmaf(load<f32>(kViewRotation + 32 + 4 * i), z, a);
            p[i] = load<f32>(kViewRotation + 48 + 4 * i) + a;
        }
        for (u32 i = 0; i < 4; ++i) store<f32>(sp + 768 + 4 * i, p[i]);
        const f64 d = -sp::sgl(sp::lfs(sp + 776) * sp::lfs(kFovScale));  // fmuls, fneg
        const f64 three = sp::lfs(k + 8);
        if (d > three) {  // fcmpu, bc 12,1
            const f64 q = sp::sgl(sp::sgl(sp::lfs(k + 16) - d) / three);  // fsubs, fdivs
            f64 f = sp::sgl(sp::sel_ge(sp::sgl(sp::lfs(k) - q), q, load<f64>(k + 24)));  // min(.., 1)
            f = sp::sgl(sp::sel_ge(f, f, load<f64>(k + 32)));                            // max(.., 0)
            sp::stfs(sp + 328, f);
        }
    }
    call(ctx, d3::fn::FProgram_SetConstant, {3, sp + 320});
    lit_draw(ctx, item, k + 12, Stencil::With40, 2);
}

namespace {

constexpr u32 Fur_ShellCount = 0x005BFB50;          // (Shader*, DrawItem*, f32* scale) -> u16 shells: from the item's view depth and material parameters +76 (name?)
constexpr u32 Gcm_SetVertexProgramAt = 0x005E3C00;  // (context, ucode offset) (effect_draw.h)
constexpr u32 kFurLight = 0x00F66FD0;   // u32: the fur shaders' light record (+32 f32[3] direction, +48 f32[4]) (shader4_draw.cpp)
constexpr u32 kLights = 0x010DD9F0;     // view block +624: 6 f32[4] light vectors (shader4_setup.cpp)
constexpr u32 kZeroVec = 0x01164BC0;    // f32[4] (fx::g::zero_vconsts)

// The render state's vertex / fragment program (+20 / +24 / +28) made
// the shader's (+4 VProgram*: +4 the loaded ucode, +20 its offset; +8
// FProgram*): a new vertex program clears +8 and loads its ucode
// (Gcm_SetVertexProgramAt) when that changed too; a new fragment program
// sets +92 bit 31.
void bind_programs(PPUContext& ctx, u32 shader) {
    const u32 st = fx::g::state;
    const u32 vp = load<u32>(shader + 4);
    if (load<u32>(st + 20) != vp) {  // cmpw
        store<u32>(st + 20, vp);
        store<u32>(st + 8, 0xFFFFFFFFu);
        const u32 code = load<u32>(vp + 4);
        if (load<u32>(st + 24) != code) {
            store<u32>(st + 24, code);
            call(ctx, Gcm_SetVertexProgramAt, {gcm(), load<u32>(vp + 20)});
        }
    }
    const u32 fp = load<u32>(shader + 8);
    if (load<u32>(st + 28) != fp) {
        store<u32>(st + 28, fp);
        store<u32>(st + 92, load<u32>(st + 92) | 0x80000000u);
    }
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
DECOMP(005BFD20, SlyCharacterFur_Draw) {
    constexpr u32 k = 0x005BFCF0;  // .text: f32 0.0, 1.0, 1/255; vperm mask (+16); {0, 0, 0, 1} (+32)
    const u32 shader = arg<u32>(ctx, 0), item = arg<u32>(ctx, 1);
    upload_item_transform(item);
    apply_fog(ctx, item);
    // The original's sp+112 (c4), +128 (c5), +144 (c0), +160 (c8), +176 /
    // +192 (the light direction in / out), +208 (c9), +272 (c7).
    Stack frame(ctx, 192);
    const u32 c4 = frame.addr(), c5 = c4 + 16, c0 = c4 + 32, c8 = c4 + 48, din = c4 + 64, dout = c4 + 80, c9 = c4 + 96, c7 = c4 + 112;
    for (u32 v = 0; v < 64; v += 16) raw4(c4 + v, kZeroVec);
    const u32 mat0 = load<u32>(item + d3::kItemMaterial);
    const auto pw = [&](u32 off) {
        const u32 params = load<u32>(load<u32>(item + d3::kItemMaterial) + d3::kMatParams);
        return params != 0 ? load<u32>(params + off) : load<u32>(k);
    };
    const auto pf = [&](u32 off) { return static_cast<f64>(std::bit_cast<f32>(pw(off))); };
    f64 length = pf(36);
    store<u32>(c4, pw(0));
    store<u32>(c4 + 4, pw(4));
    store<u32>(c4 + 8, pw(8));
    store<u32>(c5, pw(28));
    store<u32>(c5 + 4, pw(32));
    const f64 base = pf(40);
    const u32 tip_w = pw(48);
    const f64 tip = std::bit_cast<f32>(tip_w);
    store<u32>(c0 + 8, tip_w);
    store<u32>(c0 + 12, pw(44));
    store<u32>(c8, pw(52));
    store<u32>(c8 + 4, pw(56));
    set_vconsts(ctx, 5, 4, c5);
    set_vconsts(ctx, 8, 4, c8);
    const u32 light = load<u32>(kFurLight);
    for (u32 i = 0; i < 12; i += 4) sp::copy_word(din + i, light + 32 + i);
    call(ctx, s4::fn::Vec3_RotateTo, {dout, din, item});
    set_vconsts(ctx, 6, 4, light + 48);
    vec_w1(c7, dout);
    set_vconsts(ctx, 7, 4, c7);
    for (u32 i = 4; i < 16; i += 4) sp::copy_word(c9 + i, kZeroVec + i);
    sp::copy_word(c9, kZeroVec);
    const u32 mesh = load<u32>(item + d3::kItemMesh);
    u32 index = load<u32>(mesh + d3::kMeshSkin) != 0 && load<u32>(mesh + d3::kMeshPalette) != 0 ? 1 : 0;
    if ((ref<u8>(item + d3::kItemVariant) & 1) != 0) {
        const u32 list = load<u32>(item + d3::kItemLayers);
        index = ((load<u32>(list) << 1) | (index & 1)) + 2;  // rlwimi 1,0,30
        upload_layer_list(ctx, load<u32>(item + d3::kItemMaterial), list);
        {
            Stack f(ctx, 16);  // the original's sp+224
            call(ctx, s4::fn::Listener_LocalDirection, {f.addr(), kEye, item});
            vec_w1(c9, f.addr());
        }
        set_vconsts(ctx, 9, 4, c9);
        const f64 scale = sp::lfs(k + 8);
        const auto bytes = [&](u32 w, u32 sh) { return sp::sgl(sp::sgl(static_cast<f64>((w >> sh) & 0xFF)) * scale); };  // fcfid, frsp, fmuls
        const u32 tint = pw(72);
        const f64 r = bytes(tint, 16), g = bytes(tint, 8), b = bytes(tint, 0), a = bytes(tint, 24);
        Stack f(ctx, 80);  // the original's sp+288 (columns) / +256 (a light)
        const u32 cols = f.addr(), lc = cols + 64;
        for (u32 j = 0; j < 3; ++j) {
            for (u32 i = 0; i < 4; ++i) store<u32>(cols + 16 * j + 4 * i, lvlx_word0(item + 16 * i + 4 * j));  // vmrghw, vperm
        }
        raw4(cols + 48, k + 32);
        set_vconsts(ctx, 33, 16, cols);
        for (u32 n = 0; n < 6; ++n) {  // cmplwi
            const u32 src = kLights + 16 * n;
            sp::stfs(lc, sp::sgl(sp::lfs(src) * r));
            sp::stfs(lc + 4, sp::sgl(sp::lfs(src + 4) * g));
            sp::stfs(lc + 8, sp::sgl(sp::lfs(src + 8) * b));
            sp::stfs(lc + 12, sp::sgl(sp::lfs(src + 12) * a));
            set_vconsts(ctx, 37 + n, 4, lc);
        }
        const f64 strength = pf(68);
        const u32 colour = pw(64);
        Stack f1(ctx, 16);  // the original's sp+240
        const u32 c1 = f1.addr();
        store<u32>(c1 + 12, pw(60));
        sp::stfs(c1, sp::sgl(bytes(colour, 16) * strength));
        sp::stfs(c1 + 4, sp::sgl(bytes(colour, 8) * strength));
        sp::stfs(c1 + 8, sp::sgl(bytes(colour, 0) * strength));
        call(ctx, d3::fn::FProgram_SetConstant, {1, c1});
    }
    const u32 shaders = load<u32>(load<u32>(mat0 + d3::kMatType) + d3::kTypeShaders);
    const u32 base_shader = load<u32>(shaders + 4 * index), shell_shader = load<u32>(shaders + 4 * (index + 10));
    bind_programs(ctx, base_shader);
    {
        const u32 flags = ref<u8>(item + kItemFlags);
        const u32 m = load<u32>(item + d3::kItemMaterial);
        const u32 value = (((flags & kStencilHigh) != 0 ? 0x80u : 0u) | ref<u8>(m + kMatStencil)) & ref<u8>(render::g::stencil_bytes + 1);  // rlwinm 27,31,31
        if (value != ref<u8>(render::g::stencil_bytes)) set_stencil_ref(value);
    }
    draw_mesh(ctx, item);
    bind_programs(ctx, shell_shader);
    fx::emit({fx::kDepthTestEnable, ref<u8>(fx::g::state + 93) & 1u});
    fx::emit({fx::kDepthMask, 0});
    fx::emit({fx::kBlendEnable, 1});
    fx::blend_on(fx::kSrcAlpha2, fx::kOneMinusSrcAlpha2);
    fx::emit({l4::kCullEnable, 1});
    fx::emit({l4::kFrontFace, l4::kCcw});
    Stack fs(ctx, 16);  // the original's sp+544
    const u32 n = static_cast<u32>(call(ctx, Fur_ShellCount, {shader, item, fs.addr()}));
    if (static_cast<s32>(n) > 0) {  // cmpwi
        length = sp::sgl(length * sp::lfs(fs.addr()));
        apply_fog(ctx, item);
        const f64 span = sp::sgl(sp::lfs(k + 4) - base);  // fsubs
        const f64 nf = sp::sgl(static_cast<f64>(n));      // fcfid, frsp
        for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(n); i = (i + 1) & 0xFFFF) {  // cmpw
            const f64 t = sp::sgl(sp::sgl(static_cast<f64>(i)) / nf);
            const f64 len = sp::sgl(length * t);
            const f64 fade = sp::fmadds(span, t, base);
            sp::stfs(c4 + 12, len);
            sp::stfs(c0 + 4, sp::sgl(fade * tip));
            sp::stfs(c0, sp::sgl(len / length));
            set_vconsts(ctx, 4, 4, c4);
            call(ctx, d3::fn::FProgram_SetConstant, {0, c0});
            draw_mesh(ctx, item);
        }
    }
    store<u64>(render::g::state_cache_a, ~0ull);
    store<u64>(render::g::state_cache_b, ~0ull);
    call(ctx, render::layer::RenderLayer_End);
}
