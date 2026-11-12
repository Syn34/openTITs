// The shader types' PickShader slots, the smaller shader Bind / End slots
// and the shader slots that are a bare blr.
#include "draw3_shader.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

namespace d3 = sly4::render::draw3;

// The material's ShaderType's shader `idx` (rlwinm 2,0,29 then a 32-bit add).
ShaderPrograms* shader_at(Material* mat, u32 idx) {
    const ShaderType* type = host<ShaderType>(or_zero_page(mat)->w16);
    return *at<gp<ShaderPrograms>>(or_zero_page(type)->shaders.get(), (idx << 2) & 0xFFFFFFFCu);
}
bool skinned(const DrawItem* item) { return or_zero_page(item->mesh.get())->palette; }
// Layered: the item's material flag (+100 bit 7; rlwinm 25,31,31) and a layer list.
be_u32* layers(const DrawItem* item) {
    if (((or_zero_page(item->mat.get())->b100 >> 7) & 1) == 0) return nullptr;
    return item->layers;
}
void set_variant(DrawItem* item, u32 v) { item->variant = static_cast<u8>(v); }
// The variant index of a layered pick: layer index * 2 (rlwimi 1,0,30 over the low bit) + 2.
u32 layered(u32 base, const be_u32* layer_list) { return (((*or_zero_page(layer_list) << 1) & 0xFFFFFFFEu) | (base & 1)) + 2; }
// Top nibble of material +120 (rlwinm 4,28,31).
u32 blend_mode(const Material* mat) { return or_zero_page(mat)->w120 >> 28; }
bool animated(const Material* mat) { return or_zero_page(mat)->w128 != 0; }
u32 texture_count(const Material* mat) { return or_zero_page(mat)->b44; }
const be_u32* params_of(const Material* mat) { return host<be_u32>(or_zero_page(mat)->w124); }
u32 word(u32 ea) { return global<be_u32>(ea); }

// The UV transform upload (2 vertex constants from 4) the simple Bind slots
// make, with both scales the f32 at `k` (1.0 in .text next to each).
void upload_uv(u32 anim, u32 k, u32 first = 4) {
    const f32 s = global<be_f32>(k);
    MaterialAnim_UploadUV(anim, first, s, s);
}

// Fragment constant `index` = {x, ?, ?, ?}. The original stores only the
// first word of its stack vector; the other three are whatever its frame
// held, and FProgram_SetConstant compares all four against the cached
// copy. Here they are whatever this frame holds.
void set_constant_x(u32 index, u32 x_word) {
    GuestFrame f(16);
    be_u32* v = f.as<be_u32>();
    v[0] = x_word;
    FProgram_SetConstant(index, v);
}

// Fragment constant 0 = the material's parameters (+124: three f32, the
// second constant of the pair for each missing one) and w (by default the
// second constant too).
void set_params_constant(Material* mat, u32 k, u32 w = 0) {
    const be_u32* params = params_of(mat);
    GuestFrame f(16);
    be_u32* v = f.as<be_u32>();
    for (u32 i = 0; i < 3; ++i) v[i] = params ? params[i].get() : word(k + 4);
    v[3] = word(w != 0 ? w : k + 4);
    FProgram_SetConstant(0, v);
}

// The texture-count picks: variant 0 and the base index; a layered item
// takes a variant and an index step by the material's texture count (+44:
// == lo / == lo + 1 / else, or <= lo / <= hi / else), added to the base
// with the layer index folded in (rlwimi 1,0,30) or to the base alone.
struct TexRule {
    bool eq;
    u32 lo, hi;
    u8 variant[3];
    u32 step[3];
};
ShaderPrograms* pick_by_textures(Material* mat, DrawItem* item, u32 base, bool with_layer, const TexRule& r) {
    set_variant(item, 0);
    u32 idx = base;
    if (const be_u32* l = layers(item)) {
        const u32 n = texture_count(mat);
        const u32 k = r.eq ? (n == r.lo ? 0 : n == r.hi ? 1 : 2) : (n <= r.lo ? 0 : n <= r.hi ? 1 : 2);
        const u32 from = with_layer ? (((*l << 1) & 0xFFFFFFFEu) | (base & 1)) : base;
        set_variant(item, r.variant[k]);
        idx = from + r.step[k];
    }
    return shader_at(mat, idx);
}
constexpr TexRule kEq1{true, 1, 2, {1, 3, 7}, {2, 16, 32}};
constexpr TexRule kEq2{true, 2, 3, {1, 3, 7}, {2, 16, 32}};
constexpr TexRule kLe23{false, 2, 3, {1, 3, 7}, {2, 16, 32}};

// Skinned meshes (+112 skin and a palette).
bool skin_and_palette(const DrawItem* item) {
    const DrawMesh* mesh = or_zero_page(item->mesh.get());
    return mesh->skin && mesh->palette;
}

// Variant 0 and the base index; a layered item takes variant 1 and the layered index.
ShaderPrograms* pick_layered(Material* mat, DrawItem* item, u32 idx, bool allow = true) {
    set_variant(item, 0);
    if (const be_u32* l = layers(item); l && allow) {
        set_variant(item, 1);
        idx = layered(idx, l);
    }
    return shader_at(mat, idx);
}

}  // namespace

// ---------------------------------------------------------------- PickShader (ShaderType slot 16)
// ShaderPrograms* <Type>Type_PickShader(ShaderType* t, Material* mat, DrawItem* item)

// PrimTxr, PrimNoTxr, Shadow, SlyShadowMap: the type's own first shader.
ShaderPrograms* ShaderType_PickFirst(ShaderType* t) { return *or_zero_page(t->shaders.get()); }

// No shader (the item isn't drawn by these types).
ShaderPrograms* ParticleMaterialType_PickShader() { return nullptr; }
ShaderPrograms* SAC_ParticleType_PickShader() { return nullptr; }

// SAC_Panorama: notes that a panorama was seen (g::panorama_used); no shader.
ShaderPrograms* SAC_PanoramaType_PickShader() {
    u8& used = global<u8>(d3::g::panorama_used);
    if (used == 0) used = 1;
    return nullptr;
}

// The material type's first shader.
ShaderPrograms* SimpleColorTxrType_PickShader(ShaderType*, Material* mat) { return shader_at(mat, 0); }
ShaderPrograms* SimpleColorTxrUVType_PickShader(ShaderType*, Material* mat) { return shader_at(mat, 0); }
ShaderPrograms* ScreenWarpType_PickShader(ShaderType*, Material* mat) { return shader_at(mat, 0); }
ShaderPrograms* LineOutlineType_PickShader(ShaderType*, Material* mat) { return shader_at(mat, 0); }
ShaderPrograms* GeoParticleType_PickShader(ShaderType*, Material* mat) { return shader_at(mat, 0); }
ShaderPrograms* ProjTxr_DeferredType_PickShader(ShaderType*, Material* mat) { return shader_at(mat, 0); }
ShaderPrograms* SelfLightType_PickShader(ShaderType*, Material* mat) { return shader_at(mat, 0); }
ShaderPrograms* WaterType_PickShader(ShaderType*, Material* mat) { return shader_at(mat, 0); }
ShaderPrograms* SlyScreenWarpType_PickShader(ShaderType*, Material* mat) { return shader_at(mat, 0); }

// The first shader, variant 0.
ShaderPrograms* SAC_EnvironmentType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    set_variant(item, 0);
    return shader_at(mat, 0);
}
ShaderPrograms* SAC_PropType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    set_variant(item, 0);
    return shader_at(mat, 0);
}
ShaderPrograms* SAC_LitPropType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    set_variant(item, 0);
    return shader_at(mat, 0);
}
ShaderPrograms* SAC_FastPropType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    set_variant(item, 0);
    return shader_at(mat, 0);
}
ShaderPrograms* SAC_EnvMapType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    set_variant(item, 0);
    return shader_at(mat, 0);
}
ShaderPrograms* SlyShrubType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    set_variant(item, 0);
    return shader_at(mat, 0);
}

// Debug: shader 1 for skinned meshes, else 0.
ShaderPrograms* DebugType_PickShader(ShaderType*, Material* mat, DrawItem* item) { return shader_at(mat, skinned(item) ? 1 : 0); }

// Shader 1 for skinned meshes, else 0; variant 0.
ShaderPrograms* ProjTxrType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    const u32 idx = skinned(item) ? 1 : 0;
    set_variant(item, 0);
    return shader_at(mat, idx);
}
ShaderPrograms* ZPassType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    const u32 idx = skinned(item) ? 1 : 0;
    set_variant(item, 0);
    return shader_at(mat, idx);
}
ShaderPrograms* AngleIndexType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    const u32 idx = skinned(item) ? 1 : 0;
    set_variant(item, 0);
    return shader_at(mat, idx);
}

// SprParticle: variant and shader index from the material's parameters (+124 -> +8), 0 without.
ShaderPrograms* SprParticleType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    const be_u32* params = params_of(mat);
    const u32 v = params ? params[2].get() : 0;
    set_variant(item, v);
    return shader_at(mat, v);
}

// SAC_Skinned: shader 1 with more than one texture, else 0; variant 1.
ShaderPrograms* SAC_SkinnedType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    const u32 idx = texture_count(mat) > 1 ? 1 : 0;  // cmplwi
    set_variant(item, 1);
    return shader_at(mat, idx);
}

// SlyProjTxr: shader 1 and variant 1 for projected materials (+100 bit 4), else shader 0.
ShaderPrograms* SlyProjTxrType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    u32 idx = 0;
    if (((or_zero_page(mat)->b100 >> 4) & 1) != 0) {  // rlwinm 28,31,31
        idx = 1;
        set_variant(item, 1);
    }
    return shader_at(mat, idx);
}

// Shader 1 for a layered item, else 0.
ShaderPrograms* TextureWarpType_PickShader(ShaderType*, Material* mat, DrawItem* item) { return shader_at(mat, layers(item) ? 1 : 0); }
ShaderPrograms* SlyTextureWarpType_PickShader(ShaderType*, Material* mat, DrawItem* item) { return shader_at(mat, layers(item) ? 1 : 0); }

// Outline: (item +64 unset ? 2 : 0) + skinned.
ShaderPrograms* OutlineType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    const u32 idx = (item->w64 == 0 ? 2 : 0) + (skinned(item) ? 1 : 0);
    return shader_at(mat, idx);
}

// SlyDecal4: variant 0 and shader 0; a layered item takes variant 1 and
// shader (layer index + 1).
ShaderPrograms* SlyDecal4Type_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    set_variant(item, 0);
    u32 idx = 0;
    if (const be_u32* l = layers(item)) {
        set_variant(item, 1);
        idx = *l + 1;
    }
    return shader_at(mat, idx);
}

// SlyHeightFog: bit 0 animated (+128), bit 1 more than one texture, bit 2
// more than two; variant = the texture bits (1 / 3).
ShaderPrograms* SlyHeightFogType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    const u32 n = texture_count(mat);
    u32 idx = animated(mat) ? 1 : 0, v = 0;
    if (n > 1) v = 1, idx += 2;
    if (n > 2) v |= 2, idx += 4;
    set_variant(item, v);
    return shader_at(mat, idx);
}

// Base bit (animated / skinned); variant 0; a layered item takes variant 1
// and the layered index.
ShaderPrograms* Sly2UVType_PickShader(ShaderType*, Material* mat, DrawItem* item) { return pick_layered(mat, item, animated(mat) ? 1 : 0); }
ShaderPrograms* SlyCausticsType_PickShader(ShaderType*, Material* mat, DrawItem* item) { return pick_layered(mat, item, animated(mat) ? 1 : 0); }
ShaderPrograms* DecalType_PickShader(ShaderType*, Material* mat, DrawItem* item) { return pick_layered(mat, item, skinned(item) ? 1 : 0); }

// Skinned meshes (+112 skin and a palette) pick odd shaders; variant 0;
// a layered item takes variant 1 and the layered index.
ShaderPrograms* SlyCharacterFurType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    return pick_layered(mat, item, skin_and_palette(item) ? 1 : 0);
}
ShaderPrograms* SlyCharacterFurFinType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    return pick_layered(mat, item, skin_and_palette(item) ? 1 : 0);
}
// SlyCharacterEye: layered only with at most three textures.
ShaderPrograms* SlyCharacterEyeType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    u32 idx = skin_and_palette(item) ? 1 : 0;
    set_variant(item, 0);
    if (const be_u32* l = layers(item); l && texture_count(mat) <= 3) {
        set_variant(item, 1);
        idx = layered(idx, l);
    }
    return shader_at(mat, idx);
}

// Texture-count picks (see pick_by_textures).
ShaderPrograms* DiffuseType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    return pick_by_textures(mat, item, skinned(item) ? 1 : 0, true, kEq1);
}
ShaderPrograms* EnvMapType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    return pick_by_textures(mat, item, skinned(item) ? 1 : 0, true, kEq2);
}
ShaderPrograms* SlyCharacterType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    return pick_by_textures(mat, item, skin_and_palette(item) ? 1 : 0, true, kLe23);
}
ShaderPrograms* SlyCharacter2Type_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    return pick_by_textures(mat, item, skin_and_palette(item) ? 1 : 0, true, (TexRule{false, 2, 3, {3, 7, 15}, {10, 18, 26}}));
}
ShaderPrograms* SlyCharacterFakeFurType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    return pick_by_textures(mat, item, skin_and_palette(item) ? 1 : 0, true, (TexRule{false, 3, 4, {1, 3, 7}, {2, 10, 18}}));
}
ShaderPrograms* SlyCharacterCollectibleType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    return pick_by_textures(mat, item, skin_and_palette(item) ? 1 : 0, false, kLe23);
}
ShaderPrograms* SlyDecalType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    return pick_by_textures(mat, item, animated(mat) ? 1 : 0, true, (TexRule{false, 2, 4, {1, 3, 7}, {2, 16, 32}}));
}
ShaderPrograms* SlyEnvMapType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    return pick_by_textures(mat, item, animated(mat) ? 1 : 0, true, kEq2);
}
ShaderPrograms* SlyMirrorType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    return pick_by_textures(mat, item, animated(mat) ? 1 : 0, true, kEq2);
}
ShaderPrograms* SlyPanoramaType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    return pick_by_textures(mat, item, animated(mat) ? 1 : 0, true, kEq1);
}
ShaderPrograms* SlyPropType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    return pick_by_textures(mat, item, animated(mat) ? 1 : 0, true, kEq1);
}

// SlyTerrainSnow: snowy when layered or the parameters (+124) have +72
// set. Variant (snowy ? 4 : 0) | 2, index anim + (snowy ? 2 : 0) + 2; a
// layered item takes variant | 3 and the layered index + 14 instead.
ShaderPrograms* SlyTerrainSnowType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    bool snowy = layers(item) != nullptr;
    if (!snowy) {
        const be_u32* params = params_of(mat);
        snowy = params && params[18] != 0;
    }
    u32 idx = animated(mat) ? 1 : 0;
    const u32 v = snowy ? 4 : 0;
    set_variant(item, v | 2);
    if (const be_u32* l = layers(item)) {
        set_variant(item, v | 3);
        idx = (((*l << 1) & 0xFFFFFFFEu) | (idx & 1)) + 14;
    } else {
        idx += (snowy ? 2 : 0) + 2;
    }
    return shader_at(mat, idx);
}

// SlyTerrainSnowDecal: animated bit; variant 2 and index + 2, or for a
// layered item variant 3 and the layer index * 2 + 12 (+ the bit).
ShaderPrograms* SlyTerrainSnowDecalType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    u32 idx = animated(mat) ? 1 : 0;
    set_variant(item, 2);
    if (const be_u32* l = layers(item)) {
        set_variant(item, 3);
        idx = layered(idx, l) + 10;  // rlwimi, + 12
    } else {
        idx += 2;
    }
    return shader_at(mat, idx);
}

// SlyWater: animated bit + 2 when layered + 4 with more than two
// textures; the variant is the index.
ShaderPrograms* SlyWaterType_PickShader(ShaderType*, Material* mat, DrawItem* item) {
    u32 idx = animated(mat) ? 1 : 0;
    if (layers(item)) idx += 2;
    if (texture_count(mat) > 2) idx += 4;
    set_variant(item, idx);
    return shader_at(mat, idx);
}

// ---------------------------------------------------------------- Bind (Shader slot 8)
// void <Shader>_Bind(ShaderPrograms* s, Material* mat)

// The material's UV transform into vertex constants 4..5.
void SimpleColorTxrUV_Bind(ShaderPrograms*, Material* mat) { upload_uv(or_zero_page(mat)->w128, 0x005AE0A8); }
void SelfLight_Bind(ShaderPrograms*, Material* mat) { upload_uv(or_zero_page(mat)->w128, 0x005B47B4); }
void SAC_Skinned_Bind(ShaderPrograms*, Material* mat) { upload_uv(or_zero_page(mat)->w128, 0x005B6778); }
void SAC_Environment_Bind(ShaderPrograms*, Material* mat) { upload_uv(or_zero_page(mat)->w128, 0x005B7218); }

// ProjTxr: fragment constant 0.x = 1.0 / 0.0 by the blend mode (2: the second f32).
void ProjTxr_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005B0C6C;  // f32 pair in .text
    set_constant_x(0, word(k + (blend_mode(mat) == 2 ? 4 : 0)));
}

// Sly2UV: the UV transform when animated, then fragment constant 0.x by
// the blend mode (2: the first f32, else the second).
void Sly2UV_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005B9C78;  // f32 pair in .text
    if (const u32 anim = or_zero_page(mat)->w128; anim != 0) upload_uv(anim, k);
    set_constant_x(0, word(k + (blend_mode(mat) == 2 ? 0 : 4)));
}

// SlyCharacterFurFin: vertex constant 7 = {params +0, params +4, k, k}
// (k for missing parameters).
void SlyCharacterFurFin_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005C1364;  // f32 in .text
    const be_u32* params = params_of(mat);
    GuestFrame f(16);
    be_u32* v = f.as<be_u32>();
    v[0] = params ? params[0].get() : word(k);
    v[1] = params ? params[1].get() : word(k);
    v[2] = word(k);
    v[3] = word(k);
    call(fn::cellGcmSetVertexProgramConstants, gcm(), 7u, 4u, v);
}

// SlyCharacterFakeFur: SlyCharacter2's Bind, then fragment constant 5 =
// {params +52, k, k, k}.
void SlyCharacterFakeFur_Bind(ShaderPrograms* s, Material* mat) {
    constexpr u32 k = 0x005C1E3C;  // f32 in .text
    SlyCharacter2_Bind(s, mat);
    const be_u32* params = params_of(mat);
    GuestFrame f(16);
    be_u32* v = f.as<be_u32>();
    v[0] = params ? params[13].get() : word(k);
    for (u32 i = 1; i < 4; ++i) v[i] = word(k);
    FProgram_SetConstant(5, v);
}

// Decal: the UV transform with the material's parameters (+124: su, sv;
// k without), then fragment constant 0.x = k, or k + 4 for blend mode 2.
void Decal_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005B3094;  // f32 pair in .text (0.0, 1.0)
    const be_u32* params = params_of(mat);
    const u32 anim = or_zero_page(mat)->w128;
    const f32 su = params ? as_f32(params[0]) : static_cast<f32>(global<be_f32>(k));
    const f32 sv = params ? as_f32(params[1]) : static_cast<f32>(global<be_f32>(k));
    MaterialAnim_UploadUV(anim, 4, su, sv);
    set_constant_x(0, word(k + (blend_mode(mat) == 2 ? 4 : 0)));
}

// AngleIndex: the UV transform (scales k), then the parameters constant.
void AngleIndex_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005B5094;  // f32 pair in .text (1.0, 0.0)
    upload_uv(or_zero_page(mat)->w128, k);
    set_params_constant(mat, k);
}

// The UV transform when animated, then the parameters constant.
// k: f32 pair in .text (1.0, 0.0)
void SlyPanorama_Bind(ShaderPrograms*, Material* mat) {
    if (const u32 anim = or_zero_page(mat)->w128; anim != 0) upload_uv(anim, 0x005C9890);
    set_params_constant(mat, 0x005C9890);
}
void SlyProp_Bind(ShaderPrograms*, Material* mat) {
    if (const u32 anim = or_zero_page(mat)->w128; anim != 0) upload_uv(anim, 0x005CAD3C);
    set_params_constant(mat, 0x005CAD3C);
}
void SlyTerrain_Bind(ShaderPrograms*, Material* mat) {
    if (const u32 anim = or_zero_page(mat)->w128; anim != 0) upload_uv(anim, 0x005CC164);
    set_params_constant(mat, 0x005CC164);
}

// The UV transform, then the parameters constant with w = k for blend
// mode 2 (else k + 4); SlyEnvMap / SlyMirror only when animated, SlyMirror's
// transform to constants 8..9. k: f32 pair in .text (1.0, 0.0).
void EnvMap_Bind(ShaderPrograms*, Material* mat) {
    const u32 anim = or_zero_page(mat)->w128;
    upload_uv(anim, 0x005B34B4, 4);
    set_params_constant(mat, 0x005B34B4, blend_mode(mat) == 2 ? 0x005B34B4 : 0x005B34B4 + 4);
}
void SAC_EnvMap_Bind(ShaderPrograms*, Material* mat) {
    const u32 anim = or_zero_page(mat)->w128;
    upload_uv(anim, 0x005B8DE8, 4);
    set_params_constant(mat, 0x005B8DE8, blend_mode(mat) == 2 ? 0x005B8DE8 : 0x005B8DE8 + 4);
}
void SlyCharacter_Bind(ShaderPrograms*, Material* mat) {
    const u32 anim = or_zero_page(mat)->w128;
    upload_uv(anim, 0x005BB400, 4);
    set_params_constant(mat, 0x005BB400, blend_mode(mat) == 2 ? 0x005BB400 : 0x005BB400 + 4);
}
void SlyCharacterCollectible_Bind(ShaderPrograms*, Material* mat) {
    const u32 anim = or_zero_page(mat)->w128;
    upload_uv(anim, 0x005C27A8, 4);
    set_params_constant(mat, 0x005C27A8, blend_mode(mat) == 2 ? 0x005C27A8 : 0x005C27A8 + 4);
}
void SlyEnvMap_Bind(ShaderPrograms*, Material* mat) {
    const u32 anim = or_zero_page(mat)->w128;
    if (anim != 0) upload_uv(anim, 0x005C56D4, 4);
    set_params_constant(mat, 0x005C56D4, blend_mode(mat) == 2 ? 0x005C56D4 : 0x005C56D4 + 4);
}
void SlyMirror_Bind(ShaderPrograms*, Material* mat) {
    const u32 anim = or_zero_page(mat)->w128;
    if (anim != 0) upload_uv(anim, 0x005C8260, 8);
    set_params_constant(mat, 0x005C8260, blend_mode(mat) == 2 ? 0x005C8260 : 0x005C8260 + 4);
}

// GeoParticle: Draw_SetParticleState(0, material +100 bit 6, 32), then
// fragment constant 0 = {0, 0, 0, k - (parameters +12 set ? k : k + 4)}.
void GeoParticle_Bind(ShaderPrograms*, Material* mat) {
    constexpr u32 k = 0x005B0380;  // f32 pair in .text (1.0, 0.0)
    Draw_SetParticleState(0, static_cast<u8>((or_zero_page(mat)->b100 >> 6) & 1u), 32);  // rlwinm 26,31,31
    const be_u32* params = params_of(mat);
    const bool on = params && params[3] != 0;
    const f32 w = global<be_f32>(k) - global<be_f32>(on ? k : k + 4);  // fsubs
    GuestFrame f(16);
    be_u32* v = f.as<be_u32>();
    for (u32 i = 0; i < 3; ++i) v[i] = 0u;
    reinterpret_cast<be_f32*>(v)[3] = w;
    FProgram_SetConstant(0, v);
}

// ---------------------------------------------------------------- End (Shader slot 16)
// void <Shader>_End(ShaderPrograms* s, u32)

// Diffuse: alpha test off.
void Diffuse_End() { emit({kAlphaTestEnable, 0}); }

// ---------------------------------------------------------------- empty slots (blr)
void Debug_Bind() {}
void Debug_End() {}
void PrimTxr_Bind() {}
void PrimTxr_End() {}
void SimpleColorTxr_Bind() {}
void SimpleColorTxr_End() {}
void SimpleColorTxrUV_End() {}
void TextureWarp_End() {}
void ScreenWarp_End() {}
void LineOutline_Bind() {}
void LineOutline_Draw() {}
void LineOutline_End() {}
void Outline_Bind() {}
void Outline_End() {}
void GeoParticle_End() {}
void ProjTxr_End() {}
void ProjTxr_Deferred_Bind() {}
void ProjTxr_Deferred_End() {}
void ZPass_Bind() {}
void ZPass_End() {}
void Decal_End() {}
void EnvMap_End() {}
void SelfLight_End() {}
void AngleIndex_End() {}
void Water_End() {}
void SAC_Skinned_End() {}
void SAC_Environment_End() {}
void SAC_Prop_Bind() {}
void SAC_Prop_End() {}
void SAC_LitProp_Bind() {}
void SAC_LitProp_End() {}
void SAC_FastProp_Bind() {}
void SAC_FastProp_End() {}
void SAC_EnvMap_End() {}
void Sly2UV_End() {}
void SlyCaustics_End() {}
void SlyCharacter_End() {}
void SlyCharacter2_End() {}
void SlyCharacterEye_Bind() {}
void SlyCharacterEye_End() {}
void SlyCharacterFur_Bind() {}
void SlyCharacterFur_End() {}
void SlyCharacterFurFin_End() {}
void SlyCharacterFakeFur_End() {}
void SlyCharacterCollectible_End() {}
void SlyDecal_End() {}
void SlyDecal4_End() {}
void SlyEnvMap_End() {}
void SlyHeightFog_End() {}
void SlyMirror_End() {}
void SlyPanorama_End() {}
void SlyProjTxr_Bind() {}
void SlyProjTxr_Draw() {}
void SlyProjTxr_End() {}
void SlyProp_End() {}
void SlyScreenWarp_End() {}
void SlyShrub_Bind() {}
void SlyShrub_End() {}
void SlyTerrain_End() {}
void SlyTerrainSnow_End() {}
void SlyTerrainSnowDecal_End() {}
void SlyTextureWarp_End() {}
void SlyWater_End() {}
void SlyWater2_End() {}
void SlyWater3_End() {}

}  // namespace sly4::gfx

NATIVE(007A3518, sly4::gfx::ShaderType_PickFirst)
NATIVE(007ACF98, sly4::gfx::ParticleMaterialType_PickShader)
NATIVE(007AE9C4, sly4::gfx::SAC_ParticleType_PickShader)
NATIVE(007AE914, sly4::gfx::SAC_PanoramaType_PickShader)
NATIVE(005AE098, sly4::gfx::SimpleColorTxrType_PickShader)
NATIVE(005AE630, sly4::gfx::SimpleColorTxrUVType_PickShader)
NATIVE(005AFABC, sly4::gfx::ScreenWarpType_PickShader)
NATIVE(005AFBD8, sly4::gfx::LineOutlineType_PickShader)
NATIVE(005B068C, sly4::gfx::GeoParticleType_PickShader)
NATIVE(005B19B4, sly4::gfx::ProjTxr_DeferredType_PickShader)
NATIVE(005B5084, sly4::gfx::SelfLightType_PickShader)
NATIVE(005B6760, sly4::gfx::WaterType_PickShader)
NATIVE(005CDC18, sly4::gfx::SlyScreenWarpType_PickShader)
NATIVE(005B7880, sly4::gfx::SAC_EnvironmentType_PickShader)
NATIVE(005B7EEC, sly4::gfx::SAC_PropType_PickShader)
NATIVE(005B8618, sly4::gfx::SAC_LitPropType_PickShader)
NATIVE(005B8DC8, sly4::gfx::SAC_FastPropType_PickShader)
NATIVE(005B9C60, sly4::gfx::SAC_EnvMapType_PickShader)
NATIVE(005CE4E8, sly4::gfx::SlyShrubType_PickShader)
NATIVE(005ADD18, sly4::gfx::DebugType_PickShader)
NATIVE(005B17EC, sly4::gfx::ProjTxrType_PickShader)
NATIVE(005B1E3C, sly4::gfx::ZPassType_PickShader)
NATIVE(005B5914, sly4::gfx::AngleIndexType_PickShader)
NATIVE(007AD3DC, sly4::gfx::SprParticleType_PickShader)
NATIVE(005B71D4, sly4::gfx::SAC_SkinnedType_PickShader)
NATIVE(005CAD00, sly4::gfx::SlyProjTxrType_PickShader)
NATIVE(005AF24C, sly4::gfx::TextureWarpType_PickShader)
NATIVE(005D2090, sly4::gfx::SlyTextureWarpType_PickShader)
NATIVE(005B0330, sly4::gfx::OutlineType_PickShader)
NATIVE(005C5678, sly4::gfx::SlyDecal4Type_PickShader)
NATIVE(005C8204, sly4::gfx::SlyHeightFogType_PickShader)
NATIVE(005BA750, sly4::gfx::Sly2UVType_PickShader)
NATIVE(005BB388, sly4::gfx::SlyCausticsType_PickShader)
NATIVE(007AE08C, sly4::gfx::DecalType_PickShader)
NATIVE(005C12E4, sly4::gfx::SlyCharacterFurType_PickShader)
NATIVE(005C1DBC, sly4::gfx::SlyCharacterFurFinType_PickShader)
NATIVE(005BFA7C, sly4::gfx::SlyCharacterEyeType_PickShader)
NATIVE(005B2FD8, sly4::gfx::DiffuseType_PickShader)
NATIVE(005B46F8, sly4::gfx::EnvMapType_PickShader)
NATIVE(005BC800, sly4::gfx::SlyCharacterType_PickShader)
NATIVE(005BDC50, sly4::gfx::SlyCharacter2Type_PickShader)
NATIVE(005C26E8, sly4::gfx::SlyCharacterFakeFurType_PickShader)
NATIVE(005C380C, sly4::gfx::SlyCharacterCollectibleType_PickShader)
NATIVE(005C48EC, sly4::gfx::SlyDecalType_PickShader)
NATIVE(005C6A6C, sly4::gfx::SlyEnvMapType_PickShader)
NATIVE(005C97D8, sly4::gfx::SlyMirrorType_PickShader)
NATIVE(005CA9D8, sly4::gfx::SlyPanoramaType_PickShader)
NATIVE(005CC0AC, sly4::gfx::SlyPropType_PickShader)
NATIVE(005CF968, sly4::gfx::SlyTerrainSnowType_PickShader)
NATIVE(005D124C, sly4::gfx::SlyTerrainSnowDecalType_PickShader)
NATIVE(005D30F8, sly4::gfx::SlyWaterType_PickShader)
NATIVE(005AE0B0, sly4::gfx::SimpleColorTxrUV_Bind)
NATIVE(005B47B8, sly4::gfx::SelfLight_Bind)
NATIVE(005B6780, sly4::gfx::SAC_Skinned_Bind)
NATIVE(005B7220, sly4::gfx::SAC_Environment_Bind)
NATIVE(005B0C78, sly4::gfx::ProjTxr_Bind)
NATIVE(005B9C80, sly4::gfx::Sly2UV_Bind)
NATIVE(005C1368, sly4::gfx::SlyCharacterFurFin_Bind)
NATIVE(005C1E40, sly4::gfx::SlyCharacterFakeFur_Bind)
NATIVE(005B30A0, sly4::gfx::Decal_Bind)
NATIVE(005B50A0, sly4::gfx::AngleIndex_Bind)
NATIVE(005C9898, sly4::gfx::SlyPanorama_Bind)
NATIVE(005CAD48, sly4::gfx::SlyProp_Bind)
NATIVE(005CC170, sly4::gfx::SlyTerrain_Bind)
NATIVE(005B34C0, sly4::gfx::EnvMap_Bind)
NATIVE(005B8DF0, sly4::gfx::SAC_EnvMap_Bind)
NATIVE(005BB408, sly4::gfx::SlyCharacter_Bind)
NATIVE(005C27B0, sly4::gfx::SlyCharacterCollectible_Bind)
NATIVE(005C56E0, sly4::gfx::SlyEnvMap_Bind)
NATIVE(005C8268, sly4::gfx::SlyMirror_Bind)
NATIVE(005B0388, sly4::gfx::GeoParticle_Bind)
NATIVE(005B2914, sly4::gfx::Diffuse_End)
NATIVE(005ADA08, sly4::gfx::Debug_Bind)
NATIVE(007ABEAC, sly4::gfx::Debug_End)
NATIVE(005ADD50, sly4::gfx::PrimTxr_Bind)
NATIVE(007AC130, sly4::gfx::PrimTxr_End)
NATIVE(005ADEE0, sly4::gfx::SimpleColorTxr_Bind)
NATIVE(007AC450, sly4::gfx::SimpleColorTxr_End)
NATIVE(007AC604, sly4::gfx::SimpleColorTxrUV_End)
NATIVE(005AF07C, sly4::gfx::TextureWarp_End)
NATIVE(005AFAB8, sly4::gfx::ScreenWarp_End)
NATIVE(005AFACC, sly4::gfx::LineOutline_Bind)
NATIVE(005AFAD0, sly4::gfx::LineOutline_Draw)
NATIVE(007ACCA8, sly4::gfx::LineOutline_End)
NATIVE(005AFBE8, sly4::gfx::Outline_Bind)
NATIVE(007ACD64, sly4::gfx::Outline_End)
NATIVE(007AD014, sly4::gfx::GeoParticle_End)
NATIVE(007AD510, sly4::gfx::ProjTxr_End)
NATIVE(005B182C, sly4::gfx::ProjTxr_Deferred_Bind)
NATIVE(007AD5CC, sly4::gfx::ProjTxr_Deferred_End)
NATIVE(005B1AE4, sly4::gfx::ZPass_Bind)
NATIVE(007AD914, sly4::gfx::ZPass_End)
NATIVE(007ADA88, sly4::gfx::Decal_End)
NATIVE(007AE17C, sly4::gfx::EnvMap_End)
NATIVE(007AE238, sly4::gfx::SelfLight_End)
NATIVE(007AE2F4, sly4::gfx::AngleIndex_End)
NATIVE(005B6420, sly4::gfx::Water_End)
NATIVE(007AE468, sly4::gfx::SAC_Skinned_End)
NATIVE(007AE524, sly4::gfx::SAC_Environment_End)
NATIVE(005B78A0, sly4::gfx::SAC_Prop_Bind)
NATIVE(007AE5E0, sly4::gfx::SAC_Prop_End)
NATIVE(005B7F0C, sly4::gfx::SAC_LitProp_Bind)
NATIVE(007AE69C, sly4::gfx::SAC_LitProp_End)
NATIVE(005B8638, sly4::gfx::SAC_FastProp_Bind)
NATIVE(007AE758, sly4::gfx::SAC_FastProp_End)
NATIVE(007AE814, sly4::gfx::SAC_EnvMap_End)
NATIVE(007AEA40, sly4::gfx::Sly2UV_End)
NATIVE(007AEAFC, sly4::gfx::SlyCaustics_End)
NATIVE(005BC02C, sly4::gfx::SlyCharacter_End)
NATIVE(005BD79C, sly4::gfx::SlyCharacter2_End)
NATIVE(005BE0D4, sly4::gfx::SlyCharacterEye_Bind)
NATIVE(005BF144, sly4::gfx::SlyCharacterEye_End)
NATIVE(005BFB08, sly4::gfx::SlyCharacterFur_Bind)
NATIVE(005C0C24, sly4::gfx::SlyCharacterFur_End)
NATIVE(005C1A0C, sly4::gfx::SlyCharacterFurFin_End)
NATIVE(005C22A4, sly4::gfx::SlyCharacterFakeFur_End)
NATIVE(005C3208, sly4::gfx::SlyCharacterCollectible_End)
NATIVE(007AF0C0, sly4::gfx::SlyDecal_End)
NATIVE(007AF17C, sly4::gfx::SlyDecal4_End)
NATIVE(007AF238, sly4::gfx::SlyEnvMap_End)
NATIVE(007AF2F4, sly4::gfx::SlyHeightFog_End)
NATIVE(007AF3B0, sly4::gfx::SlyMirror_End)
NATIVE(007AF46C, sly4::gfx::SlyPanorama_End)
NATIVE(005CAA90, sly4::gfx::SlyProjTxr_Bind)
NATIVE(007AF528, sly4::gfx::SlyProjTxr_Draw)
NATIVE(007AF52C, sly4::gfx::SlyProjTxr_End)
NATIVE(007AF5E8, sly4::gfx::SlyProp_End)
NATIVE(005CDC14, sly4::gfx::SlyScreenWarp_End)
NATIVE(005CDC28, sly4::gfx::SlyShrub_Bind)
NATIVE(007AFC4C, sly4::gfx::SlyShrub_End)
NATIVE(007AF6A4, sly4::gfx::SlyTerrain_End)
NATIVE(007AFD08, sly4::gfx::SlyTerrainSnow_End)
NATIVE(007AFDC4, sly4::gfx::SlyTerrainSnowDecal_End)
NATIVE(005D1EC0, sly4::gfx::SlyTextureWarp_End)
NATIVE(005D2C00, sly4::gfx::SlyWater_End)
NATIVE(005D3F84, sly4::gfx::SlyWater2_End)
NATIVE(005D5A68, sly4::gfx::SlyWater3_End)
