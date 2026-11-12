// The pieces the shader Draw slots inline (shader4*.cpp, shader5*.cpp,
// shader6*.cpp), native. shader3_draw.cpp has file-local copies of the
// first few.
//
// DrawItem fields beyond rsx.h's: +68 u8 flags (bit 7: mirrored; bit 6:
// fog index; bit 5: stencil bit 7), +69 u8 variant, +71 u8 alpha.
// Material: +101 u8 stencil bits, +104 bit 31: keeps its own blending
// when translucent, +112 bit 31: restores the depth mask after a
// translucent draw, +124 parameter block.
#pragma once

#include <cmath>

#include "rsx.h"
#include "shader3.h"

namespace sly4::gfx::sdraw {

namespace s2 = sly4::render::shader2;
namespace s3 = sly4::render::shader3;

constexpr u32 kMeshDraw = 16;  // Mesh vtable slot 16: Draw(mesh, item)
constexpr u32 kMirrored = 0x80, kStencilHigh = 0x20;
constexpr u32 kStencilBytes = 0x010DDA60;  // u8[2]: {reference, write mask} (render::g::stencil_bytes)
constexpr u32 kViewProj = 0x010DD7C0;      // f32[4][4] rows (layers4::g::view_proj)
constexpr u32 kOne = 0x3F800000;           // f32 1.0

inline u8 item_alpha(const DrawItem* item) { return *at<u8>(item, 71); }
inline Material* item_mat(const DrawItem* item) { return or_zero_page(item->mat.get()); }
inline u8 mat_stencil(const Material* mat) { return *at<u8>(mat, 101); }
// The material's parameter block (+124; 0 for none).
inline u32 mat_params(const Material* mat) { return mat->w124; }

inline void vconsts(u32 first, u32 floats, const void* v) { call(fn::cellGcmSetVertexProgramConstants, gcm(), first, floats, v); }
inline void fconst(u32 index, const void* v) { FProgram_SetConstant(index, static_cast<const be_u32*>(v)); }

// {v[0], v[1], v[2], 1.0} (raw words).
inline void vec3_w1(void* dst, const void* src) {
    copy3(dst, src);
    static_cast<be_u32*>(dst)[3] = kOne;
}

// The 16-byte aligned block at or below p, writable (stvx); never null.
inline be_f32* aligned16_out(void* p) { return reinterpret_cast<be_f32*>(mem::ptr(guest(p) & ~15u)); }

// The view fog from the precomputed tables (s3::g::fog_vconsts /
// fog_colours): fog index item +68 bit 6, enabled by material +100 bit
// 6; vertex constant 32 re-sent only when the index or its flag (render
// state +216 / +220..) changed; fragment constant 32 = the fog's colour.
inline void apply_fog(DrawItem* item) {
    RenderState& st = rs();
    const u32 fog = (item->flags68 >> 6) & 1;  // rlwinm 26,31,31
    const u32 enable = (item_mat(item)->b100 >> 6) & 1;
    const u32 colour = s3::g::fog_colours + 16 * fog;
    u8& enabled = global<u8>(s3::g::fog_enabled + fog);
    if (st.unk216 != fog || enabled != enable) {
        st.unk216 = fog;
        enabled = static_cast<u8>(enable);
        vconsts(32, 4, host<u8>(s3::g::fog_vconsts + 32 * fog + 16 * enable));
    }
    fconst(32, host<u8>(colour));
}

inline void tinted4(be_f32* dst, const be_f32* src, const be_f32* colour) {
    for (u32 i = 0; i < 4; ++i) dst[i] = src[i] * colour[i];  // fmuls
}
inline void raw4(void* dst, const void* src) {
    for (u32 i = 0; i < 4; ++i) copy_word(static_cast<be_u32*>(dst) + i, static_cast<const be_u32*>(src) + i);
}

// The layer list as vertex constants 16..: {+16 * colour, +32 * colour,
// +48} and per layer entry (+0 u32 count, 48-byte entries from +64) {+0,
// +16 * colour, +32}, with colour the material's f32[4] at +64. The
// original builds it in a fixed stack area without a bound (more entries
// overrun its frame); here the frame is sized for the count.
inline void upload_layer_list(Material* mat, const be_u32* list) {
    const be_f32* colour = at<be_f32>(mat, 64);
    const auto field = [&](u32 off) { return at<be_f32>(list, off); };
    const auto count = [&] { return at<be_u32>(list, 0)->get(); };
    GuestFrame f(48 + 48 * count());
    be_f32* v = f.as<be_f32>();
    tinted4(v, field(16), colour);
    tinted4(v + 4, field(32), colour);
    raw4(v + 8, field(48));
    be_f32* dst = v + 12;
    if (count() != 0) {
        u32 src = 64;
        for (u32 n = 0; n < count(); ++n, src += 48, dst += 12) {  // cmplw
            raw4(dst, field(src));
            tinted4(dst + 4, field(src + 16), colour);
            raw4(dst + 8, field(src + 32));
        }
    }
    const u32 floats = static_cast<u32>(static_cast<s32>(guest(dst) - guest(v)) / 16) << 2;  // srawi / addze, rlwinm 2,0,29
    vconsts(16, floats, v);
}

// The layer list (item +44, unchecked) as fragment constants 16..: {+16 *
// colour}, {+32 * colour}, {+48}, and per entry 3 more {+0}, {+16 *
// colour}, {+32}, colour the material's f32[4] at +64. The original
// builds the entries in a fixed stack area without a bound (more
// entries overrun its frame: SlyCharacter2_SetupDraw from sp+480 of 752
// bytes, SlyDecal from sp+384 of 608); here the frame is sized for the
// count.
inline void upload_layers_fragment(DrawItem* item) {
    const be_u32* list = item->layers;
    const be_f32* colour = at<be_f32>(item_mat(item), 64);
    const auto field = [&](u32 off) { return at<be_f32>(list, off); };
    const auto count = [&] { return at<be_u32>(list, 0)->get(); };
    GuestFrame head(48);
    be_f32* v = head.as<be_f32>();
    tinted4(v, field(16), colour);
    fconst(16, v);
    tinted4(v + 4, field(32), colour);
    raw4(v + 8, field(48));
    fconst(17, v + 4);
    fconst(18, v + 8);
    if (count() == 0) return;
    GuestFrame entries(48 * count());
    be_f32* dst = entries.as<be_f32>();
    u32 src = 64;
    u32 index = 19;
    for (u32 n = 0; n < count(); ++n, dst += 12, src += 48, index += 3) {  // cmplw
        raw4(dst, field(src));
        tinted4(dst + 4, field(src + 16), colour);
        raw4(dst + 8, field(src + 32));
        fconst(index, dst);
        fconst(index + 1, dst + 4);
        fconst(index + 2, dst + 8);
    }
}

// The layer list of a layered item (material +100 bit 7 and a list at item +44).
inline void upload_layers(DrawItem* item) {
    Material* mat = item_mat(item);
    if (((mat->b100 >> 7) & 1) == 0) return;  // rlwinm 25,31,31
    if (const be_u32* list = item->layers) upload_layer_list(mat, list);
}

// The mesh's draw (item +12, slot 16).
inline void draw_mesh(DrawItem* item) { vcall(or_zero_page(item->mesh.get()), kMeshDraw, item); }

// Top bit of a material word.
inline bool mat_bit31(const Material* mat, u32 off) { return (*at<be_u32>(mat, off) >> 31) != 0; }  // rlwinm 1,31,31

// Stencil function reference = value (cached at stencil_bytes[0] and
// render state +164), then the stencil write mask = stencil_bytes[1]
// (render state +168).
inline void set_stencil_ref(u32 value) {
    RenderState& st = rs();
    u8* bytes = host<u8>(kStencilBytes);
    bytes[0] = static_cast<u8>(value);
    st.stencil_ref = value;
    emit({kStencilFunc, st.stencil_func, value, st.stencil_func_mask});
    const u32 mask = bytes[1];
    st.stencil_mask = mask;
    emit({kStencilMask, mask});
}
// The stencil reference (bits & the write mask), set when it changed.
inline void stencil_ref_masked(u32 bits) {
    const u8* bytes = host<u8>(kStencilBytes);
    const u32 value = bits & bytes[1];
    if (value != bytes[0]) set_stencil_ref(value);
}
// (stencil bit 7 | extra | material +101) & the write mask.
inline void item_stencil(DrawItem* item, u32 flags, u32 extra) {
    stencil_ref_masked(((flags & kStencilHigh) != 0 ? 0x80u : 0u) | extra | mat_stencil(item_mat(item)));  // rlwinm 27,31,31
}

// Word 0 of lvlx(p): the bytes from p to the end of its 16-byte block,
// zero filled.
inline u32 lvlx_word0(const void* p) {
    const u32 ea = guest(p);
    u32 w = 0;
    for (u32 i = 0; i < 4; ++i) w = (w << 8) | ((ea & 15) + i < 16 ? *host<u8>(ea + i) : 0u);
    return w;
}
inline f32 lvlx_f32(const void* p, u32 off) { return std::bit_cast<f32>(lvlx_word0(at<u8>(p, off))); }  // lvlx, vspltw 0

// m (4 rows of f32[4], read with lvx) times the 3-vector (x, y, z) per
// lane, in vmaddfp order: m0 * x + 0, + m1 * y, + m2 * z.
inline void rows_times(f32 out[4], const void* m0, f32 x, f32 y, f32 z) {
    const be_f32* m = aligned16(m0);
    for (u32 k = 0; k < 4; ++k) {
        f32 v = std::fmaf(m[k], x, 0.0f);
        v = std::fmaf(m[4 + k], y, v);
        out[k] = std::fmaf(m[8 + k], z, v);
    }
}

// u8 -> f32 through fcfid / frsp, times a single.
inline f32 byte_scaled(u32 b, f32 k) { return static_cast<f32>(b) * k; }

// The translucent / mirrored / stencilled mesh draw the lit shaders end
// with (shader3_draw.cpp's lit_draw): fragment constant `index` from
// g::character_fconst2; a translucent item (alpha below 255) draws
// without depth writes and with fragment constant `index` = {own
// blending ? 0 : 1, alpha * k, 0, 0} (alpha blending on unless the
// material blends itself, material +104 bit 31); a mirrored item flips
// the front face around the draw; the stencil reference becomes (stencil
// bit 7 | extra | material +101) & the write mask (Stencil::None leaves
// it; TextureWarp leaves out the mirroring and the stencil, `mirror`
// false); after a translucent draw the depth mask (material +112 bit 31)
// and blending are restored.
enum class Stencil { None, Plain, With40 };
// The general form: `first` the value fragment constant `index` gets
// first, `z_word` lane 2 of the translucent constant, `extra` the stencil
// bits added (when `stencil`).
inline void lit_draw_core(DrawItem* item, u32 k, bool stencil, u32 extra, u32 index, const void* first, u32 z_word, bool mirror) {
    fconst(index, first);
    if (item_alpha(item) < 255) {  // cmpwi
        emit({kDepthMask, 0});
        GuestFrame f(16);
        be_u32* v = f.as<be_u32>();
        v[0] = 0u;
        v[2] = z_word;
        v[3] = 0u;
        reinterpret_cast<be_f32*>(v)[1] = byte_scaled(item_alpha(item), global<be_f32>(k));  // fcfid, frsp, fmuls
        if (!mat_bit31(item_mat(item), 104)) {
            blend_on(kSrcAlpha2, kOneMinusSrcAlpha2);
            v[0] = kOne;
        }
        fconst(index, v);
    }
    u32 flags = item->flags68;
    if (mirror && (flags & kMirrored) != 0) {  // rlwinm 25,31,31
        emit({kCullFaceEnable, 1});
        emit({kFrontFace, kGlCw});
        flags = item->flags68;
    }
    if (stencil) item_stencil(item, flags, extra);
    draw_mesh(item);
    if (mirror && (item->flags68 & kMirrored) != 0) {
        emit({kCullFaceEnable, 1});
        emit({kFrontFace, kGlCcw});
    }
    if (item_alpha(item) >= 255) return;
    if (mat_bit31(item_mat(item), 112)) emit({kDepthMask, rs().dirty_b[1] & 1u});
    if (!mat_bit31(item_mat(item), 104)) emit({kBlendEnable, 0});
}
inline void lit_draw(DrawItem* item, u32 k, Stencil stencil, u32 index = 2, bool mirror = true) {
    lit_draw_core(item, k, stencil != Stencil::None, stencil == Stencil::With40 ? 0x40u : 0u, index,
                  host<u8>(s3::g::character_fconst2), 0, mirror);
}

}  // namespace sly4::gfx::sdraw
