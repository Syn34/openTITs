// The larger shader Draw slots (Shader vtable slot 12):
// void <Shader>_Draw(ShaderPrograms* s, DrawItem* item)
//
// DrawItem fields these read beyond rsx.h's: +68 u8 flags (bit 7:
// mirrored, drawn with clockwise front faces and culling; bit 5: stencil
// bit 7), +71 u8 alpha (255: opaque). Material: +101 u8 stencil bits,
// +104 bit 31: keeps its own blending when translucent, +112 bit 31:
// restores the depth mask (render state +93 bit 0) after a translucent
// draw.
#include "shader3.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

namespace s2 = sly4::render::shader2;
namespace s3 = sly4::render::shader3;

constexpr u32 kMeshDraw = 16;  // Mesh vtable slot 16: Draw(mesh, item)
constexpr u32 kMirrored = 0x80, kStencilHigh = 0x20;
constexpr u32 kStencilBytes = 0x010DDA60;  // u8[2]: {reference, write mask} (render::g::stencil_bytes)

u8 item_alpha(const DrawItem* item) { return *at<u8>(item, 71); }
Material* item_mat(const DrawItem* item) { return or_zero_page(item->mat.get()); }
u8 mat_stencil(const Material* mat) { return *at<u8>(mat, 101); }
void vconst(u32 first, const void* v) { call(fn::cellGcmSetVertexProgramConstants, gcm(), first, 4u, v); }
void fconst(u32 index, const void* v) { FProgram_SetConstant(index, static_cast<const be_u32*>(v)); }

// The view fog, as DrawItem_SetParticleState(item, 32) sets it but from
// the precomputed tables (s3::g::fog_vconsts / fog_colours): fog index
// item +68 bit 6, enabled by material +100 bit 6; vertex constant 32 is
// re-sent only when the index or its flag (render state +216 / +220..)
// changed; fragment constant 32 = the fog's colour.
void apply_fog(DrawItem* item) {
    RenderState& st = rs();
    const u32 fog = (item->flags68 >> 6) & 1;  // rlwinm 26,31,31
    const u32 enable = (item_mat(item)->b100 >> 6) & 1;
    const u32 colour = s3::g::fog_colours + 16 * fog;
    u8& enabled = global<u8>(s3::g::fog_enabled + fog);
    if (st.unk216 != fog || enabled != enable) {
        st.unk216 = fog;
        enabled = static_cast<u8>(enable);
        vconst(32, host<u8>(s3::g::fog_vconsts + 32 * fog + 16 * enable));
    }
    fconst(32, host<u8>(colour));
}

// The layer list as vertex constants 16..: {+16 * colour, +32 * colour,
// +48} and per layer entry (+0 u32 count, 48-byte entries from +64) {+0,
// +16 * colour, +32}, with colour the material's f32[4] at +64.
// (shader2_draw.cpp's Decal_Draw does the same.)
void upload_layer_list(Material* mat, const be_u32* list) {
    const be_f32* colour = at<be_f32>(mat, 64);
    const auto tinted = [&](be_f32* dst, const be_f32* src) {
        for (u32 i = 0; i < 4; ++i) dst[i] = src[i] * colour[i];  // fmuls
    };
    const auto raw = [](void* dst, const void* src) {
        for (u32 i = 0; i < 4; ++i) copy_word(static_cast<be_u32*>(dst) + i, static_cast<const be_u32*>(src) + i);
    };
    const auto field = [&](u32 off) { return at<be_f32>(list, off); };
    const u32 count = *at<be_u32>(list, 0);
    // The original builds this at sp+112 of a 336-byte frame without a
    // bound: more than 3 entries overrun its saved registers. Here the
    // frame is sized for the entry count.
    GuestFrame f(48 + 48 * count);
    be_f32* v = f.as<be_f32>();
    tinted(v, field(16));
    tinted(v + 4, field(32));
    raw(v + 8, field(48));
    be_f32* dst = v + 12;
    if (*at<be_u32>(list, 0) != 0) {
        u32 src = 64;
        for (u32 n = 0; n < *at<be_u32>(list, 0); ++n, src += 48, dst += 12) {  // cmplw
            raw(dst, field(src));
            tinted(dst + 4, field(src + 16));
            raw(dst + 8, field(src + 32));
        }
    }
    const u32 floats = static_cast<u32>(static_cast<s32>(guest(dst) - guest(v)) / 16) << 2;  // srawi / addze, rlwinm 2,0,29
    call(fn::cellGcmSetVertexProgramConstants, gcm(), 16u, floats, v);
}

// The layer list of a layered item (material +100 bit 7 and a list at item +44).
void upload_layers(DrawItem* item) {
    Material* mat = item_mat(item);
    if (((mat->b100 >> 7) & 1) == 0) return;  // rlwinm 25,31,31
    if (const be_u32* list = item->layers) upload_layer_list(mat, list);
}

// The mesh's draw (item +12, slot 16).
void draw_mesh(DrawItem* item) { vcall(or_zero_page(item->mesh.get()), kMeshDraw, item); }

// Stencil function reference = value (cached at render::g::stencil_bytes[0]
// and render state +164), then the stencil write mask = stencil_bytes[1]
// (render state +168).
void set_stencil_ref(u32 value) {
    RenderState& st = rs();
    u8* bytes = host<u8>(kStencilBytes);
    bytes[0] = static_cast<u8>(value);
    st.stencil_ref = value;
    emit({kStencilFunc, st.stencil_func, value, st.stencil_func_mask});
    const u32 mask = bytes[1];
    st.stencil_mask = mask;
    emit({kStencilMask, mask});
}

// The lit draw SlyCharacter2, the SAC_* shaders and SimpleColorTxrUV
// share (k: f32 1/255 in .text): fragment constant `index` (2; 0 for
// SimpleColorTxrUV) from g::character_fconst2; a translucent
// item (alpha below 255) draws
// without depth writes and with fragment constant 2 = {own blending ? 0 :
// 1, alpha / 255, 0, 0} (alpha blending on unless the material blends
// itself); a mirrored item flips the front face around the draw; the
// stencil reference becomes (stencil bit 7 | 0x40 | material +101) &
// the write mask (SlyCharacter2; the SAC_* shaders leave out 0x40,
// SimpleColorTxrUV leaves the stencil alone);
// after a translucent draw the depth mask and blending
// are restored as the material asks.
enum class Stencil { None, Plain, With40 };
void lit_draw(DrawItem* item, u32 k, Stencil stencil, u32 index = 2) {
    fconst(index, host<u8>(s3::g::character_fconst2));
    if (item_alpha(item) < 255) {  // cmpwi
        emit({kDepthMask, 0});
        GuestFrame f(16);
        be_u32* v = f.as<be_u32>();
        v[0] = 0u;
        v[2] = 0u;
        v[3] = 0u;
        reinterpret_cast<be_f32*>(v)[1] = static_cast<f32>(item_alpha(item)) * global<be_f32>(k);  // fcfid, frsp, fmuls
        if ((item_mat(item)->blend >> 31) == 0) {
            blend_on(kSrcAlpha2, kOneMinusSrcAlpha2);
            v[0] = s2::kOne;
        }
        fconst(index, v);
    }
    u32 flags = item->flags68;
    if ((flags & kMirrored) != 0) {  // rlwinm 25,31,31
        emit({kCullFaceEnable, 1});
        emit({kFrontFace, kGlCw});
        flags = item->flags68;
    }
    if (stencil != Stencil::None) {
        const u32 bits = ((flags & kStencilHigh) != 0 ? 0x80u : 0u) | (stencil == Stencil::With40 ? 0x40u : 0u) |
                         mat_stencil(item_mat(item));  // rlwinm 27,31,31
        const u8* bytes = host<u8>(kStencilBytes);
        const u32 value = bits & bytes[1];
        if (value != bytes[0]) set_stencil_ref(value);
    }
    draw_mesh(item);
    if ((item->flags68 & kMirrored) != 0) {
        emit({kCullFaceEnable, 1});
        emit({kFrontFace, kGlCcw});
    }
    if (item_alpha(item) >= 255) return;
    if ((item_mat(item)->blend2 >> 31) != 0) emit({kDepthMask, rs().dirty_b[1] & 1u});
    if ((item_mat(item)->blend >> 31) == 0) emit({kBlendEnable, 0});
}

// SlyCharacter2_Draw's body: the character set-up
// (SlyCharacter2_SetupDraw), then the lit draw.
void draw_character(ShaderPrograms* s, DrawItem* item, u32 k) {
    call(s3::fn::SlyCharacter2_SetupDraw, s, item);
    lit_draw(item, k, Stencil::With40);
}

// The SAC_* shaders: the item transform, view fog 0 off
// (Draw_SetParticleState(0, false, 32)), the shader's own vertex
// constants (SacExtra), then the lit draw.
enum class SacExtra {
    None,
    Colour,  // SAC_Prop: the default material colour (s3::g::default_colour) as vertex constant 7
    Light,   // SAC_LitProp: g::config +176.. / +192.. as vertex constants 16 / 20 (w 1.0), the light direction +208.. in item space as 23
    Tint,    // SAC_FastProp: the material's packed colour (parameters +0; 0 without) times the item's u8[4] +72, / 256, / 255, as vertex constant 7
};
void draw_sac(DrawItem* item, u32 k, SacExtra extra) {
    upload_item_matrix(item);
    Draw_SetParticleState(0, 0, 32);
    if (extra == SacExtra::Colour) {
        vconst(7, host<u8>(s3::g::default_colour));
    } else if (extra == SacExtra::Light) {
        const u8* cfg = reinterpret_cast<const u8*>(&config());
        // The original's sp+112 (constant 23), +128 (16), +144 (20), +160 (the direction).
        GuestFrame f(64);
        be_u32* v23 = f.as<be_u32>(0);
        be_u32* v16 = f.as<be_u32>(16);
        be_u32* v20 = f.as<be_u32>(32);
        be_u32* dir = f.as<be_u32>(48);
        for (u32 i = 0; i < 3; ++i) {
            copy_word(v16 + i, cfg + 176 + 4 * i);
            copy_word(v20 + i, cfg + 192 + 4 * i);
            copy_word(dir + i, cfg + 208 + 4 * i);
        }
        v16[3] = s2::kOne;
        v20[3] = s2::kOne;
        call(s3::fn::Vec3_ToItemSpace, dir, item);
        for (u32 i = 0; i < 3; ++i) copy_word(v23 + i, dir + i);
        v23[3] = s2::kOne;
        vconst(23, v23);
        vconst(16, v16);
        vconst(20, v20);
    } else if (extra == SacExtra::Tint) {
        const be_u32* params = host<be_u32>(item_mat(item)->w124);
        const u32 packed = params ? params[0].get() : 0;
        const f32 scale = global<be_f32>(k);
        GuestFrame f(16);
        be_f32* v = f.as<be_f32>();
        for (u32 i = 0; i < 4; ++i) {
            const u32 channel = (packed >> (8 * i)) & 0xFF;  // byte 3 - i of the word
            const u32 x = ((channel * *at<u8>(item, 72 + i)) >> 8) & 0xFF;  // mullw, rlwinm 24,24,31
            v[i] = static_cast<f32>(x) * scale;  // fcfid, frsp, fmuls
        }
        vconst(7, v);
    }
    lit_draw(item, k, Stencil::Plain);
}

// Sly2UV_Draw's body: the item transform, the fog, the layers (SlyDecal4:
// those of an item whose variant +69 bit 0 is set, the list unchecked),
// with `alpha_k` (SlyScreenWarp) vertex constant 4 = {1, 1, 1, item
// alpha / the f32 at alpha_k (255.0)}, the stencil reference ((stencil
// bit 7 | material +101) & the write mask), then the mesh's draw.
void draw_layered_stencil(DrawItem* item, bool by_variant = false, u32 alpha_k = 0) {
    upload_item_matrix(item);
    apply_fog(item);
    if (!by_variant) upload_layers(item);
    else if ((item->variant & 1) != 0) upload_layer_list(item_mat(item), item->layers);
    if (alpha_k != 0) {
        GuestFrame f(16);
        be_f32* v = f.as<be_f32>();
        for (u32 i = 0; i < 3; ++i) *reinterpret_cast<be_u32*>(v + i) = s2::kOne;
        v[3] = static_cast<f32>(item_alpha(item)) / global<be_f32>(alpha_k);  // fcfid, frsp, fdivs
        vconst(4, v);
    }
    const u32 flags = item->flags68;
    const u8* bytes = host<u8>(kStencilBytes);
    const u32 value = (((flags & kStencilHigh) != 0 ? 0x80u : 0u) | mat_stencil(item_mat(item))) & bytes[1];
    if (value != bytes[0]) set_stencil_ref(value);
    draw_mesh(item);
}

// SimpleColorTxrUV: the item transform, vertex constant 6 = the item's
// colour (item +64, u8[4] / 255; the default material colour without
// one), then the lit draw with fragment constant 0 and no stencil.
void draw_colour_uv(DrawItem* item, u32 k) {
    upload_item_matrix(item);
    GuestFrame f(16);
    be_f32* v = f.as<be_f32>();
    if (const u8* colour = host<u8>(item->w64)) {
        const f32 scale = global<be_f32>(k);
        for (u32 i = 0; i < 4; ++i) v[i] = static_cast<f32>(*at<u8>(colour, i)) * scale;  // fcfid, frsp, fmuls
    } else {
        for (u32 i = 0; i < 4; ++i) copy_word(v + i, host<u8>(s3::g::default_colour + 4 * i));
    }
    vconst(6, v);
    lit_draw(item, k, Stencil::None, 0);
}

}  // namespace

void SimpleColorTxrUV_Draw(ShaderPrograms*, DrawItem* item) { draw_colour_uv(item, 0x005AE0EC); }

void Sly2UV_Draw(ShaderPrograms*, DrawItem* item) { draw_layered_stencil(item); }
void SlyCaustics_Draw(ShaderPrograms*, DrawItem* item) { draw_layered_stencil(item); }
void SlyDecal4_Draw(ShaderPrograms*, DrawItem* item) { draw_layered_stencil(item, true); }
void SlyScreenWarp_Draw(ShaderPrograms*, DrawItem* item) { draw_layered_stencil(item, false, 0x005CD6E8); }

void SAC_Environment_Draw(ShaderPrograms*, DrawItem* item) { draw_sac(item, 0x005B725C, SacExtra::None); }
void SAC_Prop_Draw(ShaderPrograms*, DrawItem* item) { draw_sac(item, 0x005B78A4, SacExtra::Colour); }
void SAC_LitProp_Draw(ShaderPrograms*, DrawItem* item) { draw_sac(item, 0x005B7F10, SacExtra::Light); }
void SAC_FastProp_Draw(ShaderPrograms*, DrawItem* item) { draw_sac(item, 0x005B863C, SacExtra::Tint); }

void SlyCharacter2_Draw(ShaderPrograms* s, DrawItem* item) { draw_character(s, item, 0x005BD3AC); }
// SlyCharacterFakeFur: the same code (its own copy of k).
void SlyCharacterFakeFur_Draw(ShaderPrograms* s, DrawItem* item) { draw_character(s, item, 0x005C1EB4); }

}  // namespace sly4::gfx

NATIVE(005AE0F0, sly4::gfx::SimpleColorTxrUV_Draw)
NATIVE(005B9D0C, sly4::gfx::Sly2UV_Draw)
NATIVE(005BA8BC, sly4::gfx::SlyCaustics_Draw)
NATIVE(005C4C3C, sly4::gfx::SlyDecal4_Draw)
NATIVE(005CD6F0, sly4::gfx::SlyScreenWarp_Draw)
NATIVE(005B7260, sly4::gfx::SAC_Environment_Draw)
NATIVE(005B78A8, sly4::gfx::SAC_Prop_Draw)
NATIVE(005B7F18, sly4::gfx::SAC_LitProp_Draw)
NATIVE(005B8640, sly4::gfx::SAC_FastProp_Draw)
NATIVE(005BD3B0, sly4::gfx::SlyCharacter2_Draw)
NATIVE(005C1EB8, sly4::gfx::SlyCharacterFakeFur_Draw)
