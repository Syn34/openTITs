// The shader types' PickShader slots, the smaller shader Bind / End slots
// and the shader slots that are a bare blr.
#include "draw3_shader.h"

using namespace sly4;

namespace {

namespace d3 = sly4::render::draw3;
namespace sp = sly4::render::pool;

// The material's ShaderType's shader `idx` (rlwinm 2,0,29 then a 32-bit add).
u32 shader_at(u32 mat, u32 idx) {
    const u32 shaders = load<u32>(load<u32>(mat + d3::kMatType) + d3::kTypeShaders);
    return load<u32>(shaders + ((idx << 2) & 0xFFFFFFFCu));
}
bool skinned(u32 item) { return load<u32>(load<u32>(item + d3::kItemMesh) + d3::kMeshPalette) != 0; }
// Layered: the item's material flag (+100 bit 7; rlwinm 25,31,31) and a layer list.
u32 layers(u32 item) {
    if (((ref<u8>(load<u32>(item + d3::kItemMaterial) + d3::kMatFlags) >> 7) & 1) == 0) return 0;
    return load<u32>(item + d3::kItemLayers);
}
void set_variant(u32 item, u32 v) { store<u8>(item + d3::kItemVariant, static_cast<u8>(v)); }
// The variant index of a layered pick: layer index * 2 (rlwimi 1,0,30 over the low bit) + 2.
u32 layered(u32 base, u32 layer_list) { return (((load<u32>(layer_list) << 1) & 0xFFFFFFFEu) | (base & 1)) + 2; }
// Top nibble of material +120 (rlwinm 4,28,31).
u32 blend_mode(u32 mat) { return load<u32>(mat + d3::kMatBlend) >> 28; }

// The UV transform upload (2 vertex constants from 4) the simple Bind slots
// make, with both scales the f32 at `k` (1.0 in .text next to each).
void upload_uv(PPUContext& ctx, u32 anim, u32 k, u32 first = 4) {
    const f64 s = static_cast<f64>(load<f32>(k));
    callf(ctx, d3::fn::MaterialAnim_UploadUV, {anim, first}, {s, s});
}

// Fragment constant `index` = {x, ?, ?, ?}. The original stores only the
// first word of its stack vector; the other three are whatever its frame
// held, and FProgram_SetConstant compares all four against the cached
// copy. Here they are whatever this frame holds.
void set_constant_x(PPUContext& ctx, u32 index, u32 x_word) {
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), x_word);
    call(ctx, d3::fn::FProgram_SetConstant, {index, frame.addr()});
}

}  // namespace

// ---------------------------------------------------------------- PickShader (ShaderType slot 16)
// Shader* <Type>Type_PickShader(ShaderType* t, Material* mat, DrawItem* item)

// PrimTxr, PrimNoTxr, Shadow, SlyShadowMap: the type's own first shader.
DECOMP(007A3518, ShaderType_PickFirst) {
    const u32 t = arg<u32>(ctx, 0);
    ret(ctx, load<u32>(load<u32>(t + d3::kTypeShaders)));
}

// No shader (the item isn't drawn by these types).
DECOMP(007ACF98, ParticleMaterialType_PickShader) { ret(ctx, 0); }
DECOMP(007AE9C4, SAC_ParticleType_PickShader) { ret(ctx, 0); }

// SAC_Panorama: notes that a panorama was seen (g::panorama_used); no shader.
DECOMP(007AE914, SAC_PanoramaType_PickShader) {
    if (ref<u8>(d3::g::panorama_used) == 0) store<u8>(d3::g::panorama_used, 1);
    ret(ctx, 0);
}

// The material type's first shader.
DECOMP(005AE098, SimpleColorTxrType_PickShader) { ret(ctx, shader_at(arg<u32>(ctx, 1), 0)); }
DECOMP(005AE630, SimpleColorTxrUVType_PickShader) { ret(ctx, shader_at(arg<u32>(ctx, 1), 0)); }
DECOMP(005AFABC, ScreenWarpType_PickShader) { ret(ctx, shader_at(arg<u32>(ctx, 1), 0)); }
DECOMP(005AFBD8, LineOutlineType_PickShader) { ret(ctx, shader_at(arg<u32>(ctx, 1), 0)); }
DECOMP(005B068C, GeoParticleType_PickShader) { ret(ctx, shader_at(arg<u32>(ctx, 1), 0)); }
DECOMP(005B19B4, ProjTxr_DeferredType_PickShader) { ret(ctx, shader_at(arg<u32>(ctx, 1), 0)); }
DECOMP(005B5084, SelfLightType_PickShader) { ret(ctx, shader_at(arg<u32>(ctx, 1), 0)); }
DECOMP(005B6760, WaterType_PickShader) { ret(ctx, shader_at(arg<u32>(ctx, 1), 0)); }
DECOMP(005CDC18, SlyScreenWarpType_PickShader) { ret(ctx, shader_at(arg<u32>(ctx, 1), 0)); }

// The first shader, variant 0.
DECOMP(005B7880, SAC_EnvironmentType_PickShader) {
    set_variant(arg<u32>(ctx, 2), 0);
    ret(ctx, shader_at(arg<u32>(ctx, 1), 0));
}
DECOMP(005B7EEC, SAC_PropType_PickShader) {
    set_variant(arg<u32>(ctx, 2), 0);
    ret(ctx, shader_at(arg<u32>(ctx, 1), 0));
}
DECOMP(005B8618, SAC_LitPropType_PickShader) {
    set_variant(arg<u32>(ctx, 2), 0);
    ret(ctx, shader_at(arg<u32>(ctx, 1), 0));
}
DECOMP(005B8DC8, SAC_FastPropType_PickShader) {
    set_variant(arg<u32>(ctx, 2), 0);
    ret(ctx, shader_at(arg<u32>(ctx, 1), 0));
}
DECOMP(005B9C60, SAC_EnvMapType_PickShader) {
    set_variant(arg<u32>(ctx, 2), 0);
    ret(ctx, shader_at(arg<u32>(ctx, 1), 0));
}
DECOMP(005CE4E8, SlyShrubType_PickShader) {
    set_variant(arg<u32>(ctx, 2), 0);
    ret(ctx, shader_at(arg<u32>(ctx, 1), 0));
}

// Debug: shader 1 for skinned meshes, else 0.
DECOMP(005ADD18, DebugType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    ret(ctx, shader_at(mat, skinned(item) ? 1 : 0));
}

// Shader 1 for skinned meshes, else 0; variant 0.
DECOMP(005B17EC, ProjTxrType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    const u32 idx = skinned(item) ? 1 : 0;
    set_variant(item, 0);
    ret(ctx, shader_at(mat, idx));
}
DECOMP(005B1E3C, ZPassType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    const u32 idx = skinned(item) ? 1 : 0;
    set_variant(item, 0);
    ret(ctx, shader_at(mat, idx));
}
DECOMP(005B5914, AngleIndexType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    const u32 idx = skinned(item) ? 1 : 0;
    set_variant(item, 0);
    ret(ctx, shader_at(mat, idx));
}

// SprParticle: variant and shader index from the material's parameters (+124 -> +8), 0 without.
DECOMP(007AD3DC, SprParticleType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    const u32 params = load<u32>(mat + d3::kMatParams);
    const u32 v = params != 0 ? load<u32>(params + 8) : 0;
    set_variant(item, v);
    ret(ctx, shader_at(mat, v));
}

// SAC_Skinned: shader 1 with more than one texture, else 0; variant 1.
DECOMP(005B71D4, SAC_SkinnedType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    const u32 idx = ref<u8>(mat + d3::kMatTextureCount) > 1 ? 1 : 0;  // cmplwi
    set_variant(item, 1);
    ret(ctx, shader_at(mat, idx));
}

// SlyProjTxr: shader 1 and variant 1 for projected materials (+100 bit 4), else shader 0.
DECOMP(005CAD00, SlyProjTxrType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    u32 idx = 0;
    if (((ref<u8>(mat + d3::kMatFlags) >> 4) & 1) != 0) {  // rlwinm 28,31,31
        idx = 1;
        set_variant(item, 1);
    }
    ret(ctx, shader_at(mat, idx));
}

// Shader 1 for a layered item, else 0.
DECOMP(005AF24C, TextureWarpType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    ret(ctx, shader_at(mat, layers(item) != 0 ? 1 : 0));
}
DECOMP(005D2090, SlyTextureWarpType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    ret(ctx, shader_at(mat, layers(item) != 0 ? 1 : 0));
}

// Outline: (item +64 unset ? 2 : 0) + skinned.
DECOMP(005B0330, OutlineType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    const u32 idx = (load<u32>(item + 64) == 0 ? 2 : 0) + (skinned(item) ? 1 : 0);
    ret(ctx, shader_at(mat, idx));
}

// SlyDecal4: variant 0 and shader 0; a layered item takes variant 1 and
// shader (layer index + 1).
DECOMP(005C5678, SlyDecal4Type_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    set_variant(item, 0);
    u32 idx = 0;
    if (const u32 l = layers(item); l != 0) {
        set_variant(item, 1);
        idx = load<u32>(l) + 1;
    }
    ret(ctx, shader_at(mat, idx));
}

// SlyHeightFog: bit 0 animated (+128), bit 1 more than one texture, bit 2
// more than two; variant = the texture bits (1 / 3).
DECOMP(005C8204, SlyHeightFogType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    const u32 n = ref<u8>(mat + d3::kMatTextureCount);
    u32 idx = load<u32>(mat + d3::kMatAnim) != 0 ? 1 : 0, v = 0;
    if (n > 1) v = 1, idx += 2;
    if (n > 2) v |= 2, idx += 4;
    set_variant(item, v);
    ret(ctx, shader_at(mat, idx));
}

// Base bit `base` (0 / 1); variant 0; a layered item takes variant 1 and
// the layered index.
DECOMP(005BA750, Sly2UVType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    u32 idx = (load<u32>(mat + d3::kMatAnim) != 0) ? 1 : 0;
    set_variant(item, 0);
    if (const u32 l = layers(item); l != 0) {
        set_variant(item, 1);
        idx = layered(idx, l);
    }
    ret(ctx, shader_at(mat, idx));
}
DECOMP(005BB388, SlyCausticsType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    u32 idx = (load<u32>(mat + d3::kMatAnim) != 0) ? 1 : 0;
    set_variant(item, 0);
    if (const u32 l = layers(item); l != 0) {
        set_variant(item, 1);
        idx = layered(idx, l);
    }
    ret(ctx, shader_at(mat, idx));
}
DECOMP(007AE08C, DecalType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    u32 idx = (skinned(item)) ? 1 : 0;
    set_variant(item, 0);
    if (const u32 l = layers(item); l != 0) {
        set_variant(item, 1);
        idx = layered(idx, l);
    }
    ret(ctx, shader_at(mat, idx));
}

// Skinned meshes (+112 skin and a palette) pick odd shaders; variant 0;
// a layered item takes variant 1 and the layered index.
static bool skin_and_palette(u32 item) {
    const u32 mesh = load<u32>(item + d3::kItemMesh);
    return load<u32>(mesh + d3::kMeshSkin) != 0 && load<u32>(mesh + d3::kMeshPalette) != 0;
}
DECOMP(005C12E4, SlyCharacterFurType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    u32 idx = skin_and_palette(item) ? 1 : 0;
    set_variant(item, 0);
    if (const u32 l = layers(item); l != 0 && (true)) {
        set_variant(item, 1);
        idx = layered(idx, l);
    }
    ret(ctx, shader_at(mat, idx));
}
DECOMP(005C1DBC, SlyCharacterFurFinType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    u32 idx = skin_and_palette(item) ? 1 : 0;
    set_variant(item, 0);
    if (const u32 l = layers(item); l != 0 && (true)) {
        set_variant(item, 1);
        idx = layered(idx, l);
    }
    ret(ctx, shader_at(mat, idx));
}
// SlyCharacterEye: layered only with at most three textures.
DECOMP(005BFA7C, SlyCharacterEyeType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    u32 idx = skin_and_palette(item) ? 1 : 0;
    set_variant(item, 0);
    if (const u32 l = layers(item); l != 0 && (ref<u8>(mat + d3::kMatTextureCount) <= 3)) {
        set_variant(item, 1);
        idx = layered(idx, l);
    }
    ret(ctx, shader_at(mat, idx));
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
static u32 pick_by_textures(u32 mat, u32 item, u32 base, bool with_layer, const TexRule& r) {
    set_variant(item, 0);
    u32 idx = base;
    if (const u32 l = layers(item); l != 0) {
        const u32 n = ref<u8>(mat + d3::kMatTextureCount);
        const u32 k = r.eq ? (n == r.lo ? 0 : n == r.hi ? 1 : 2) : (n <= r.lo ? 0 : n <= r.hi ? 1 : 2);
        const u32 from = with_layer ? (((load<u32>(l) << 1) & 0xFFFFFFFEu) | (base & 1)) : base;
        set_variant(item, r.variant[k]);
        idx = from + r.step[k];
    }
    return shader_at(mat, idx);
}
constexpr TexRule kEq1{true, 1, 2, {1, 3, 7}, {2, 16, 32}};
constexpr TexRule kEq2{true, 2, 3, {1, 3, 7}, {2, 16, 32}};
constexpr TexRule kLe23{false, 2, 3, {1, 3, 7}, {2, 16, 32}};
DECOMP(005B2FD8, DiffuseType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    ret(ctx, pick_by_textures(mat, item, (skinned(item)) ? 1 : 0, true, kEq1));
}
DECOMP(005B46F8, EnvMapType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    ret(ctx, pick_by_textures(mat, item, (skinned(item)) ? 1 : 0, true, kEq2));
}
DECOMP(005BC800, SlyCharacterType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    ret(ctx, pick_by_textures(mat, item, (skin_and_palette(item)) ? 1 : 0, true, kLe23));
}
DECOMP(005BDC50, SlyCharacter2Type_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    ret(ctx, pick_by_textures(mat, item, (skin_and_palette(item)) ? 1 : 0, true, (TexRule{false, 2, 3, {3, 7, 15}, {10, 18, 26}})));
}
DECOMP(005C26E8, SlyCharacterFakeFurType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    ret(ctx, pick_by_textures(mat, item, (skin_and_palette(item)) ? 1 : 0, true, (TexRule{false, 3, 4, {1, 3, 7}, {2, 10, 18}})));
}
DECOMP(005C380C, SlyCharacterCollectibleType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    ret(ctx, pick_by_textures(mat, item, (skin_and_palette(item)) ? 1 : 0, false, kLe23));
}
DECOMP(005C48EC, SlyDecalType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    ret(ctx, pick_by_textures(mat, item, (load<u32>(mat + d3::kMatAnim) != 0) ? 1 : 0, true, (TexRule{false, 2, 4, {1, 3, 7}, {2, 16, 32}})));
}
DECOMP(005C6A6C, SlyEnvMapType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    ret(ctx, pick_by_textures(mat, item, (load<u32>(mat + d3::kMatAnim) != 0) ? 1 : 0, true, kEq2));
}
DECOMP(005C97D8, SlyMirrorType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    ret(ctx, pick_by_textures(mat, item, (load<u32>(mat + d3::kMatAnim) != 0) ? 1 : 0, true, kEq2));
}
DECOMP(005CA9D8, SlyPanoramaType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    ret(ctx, pick_by_textures(mat, item, (load<u32>(mat + d3::kMatAnim) != 0) ? 1 : 0, true, kEq1));
}
DECOMP(005CC0AC, SlyPropType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    ret(ctx, pick_by_textures(mat, item, (load<u32>(mat + d3::kMatAnim) != 0) ? 1 : 0, true, kEq1));
}

// SlyTerrainSnow: snowy when layered or the parameters (+124) have +72
// set. Variant (snowy ? 4 : 0) | 2, index anim + (snowy ? 2 : 0) + 2; a
// layered item takes variant | 3 and the layered index + 14 instead.
DECOMP(005CF968, SlyTerrainSnowType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    bool snowy = layers(item) != 0;
    if (!snowy) {
        const u32 params = load<u32>(mat + d3::kMatParams);
        snowy = params != 0 && load<u32>(params + 72) != 0;
    }
    u32 idx = load<u32>(mat + d3::kMatAnim) != 0 ? 1 : 0;
    const u32 v = snowy ? 4 : 0;
    set_variant(item, v | 2);
    if (const u32 l = layers(item); l != 0) {
        set_variant(item, v | 3);
        idx = (((load<u32>(l) << 1) & 0xFFFFFFFEu) | (idx & 1)) + 14;
    } else {
        idx += (snowy ? 2 : 0) + 2;
    }
    ret(ctx, shader_at(mat, idx));
}

// SlyTerrainSnowDecal: animated bit; variant 2 and index + 2, or for a
// layered item variant 3 and the layer index * 2 + 12 (+ the bit).
DECOMP(005D124C, SlyTerrainSnowDecalType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    u32 idx = load<u32>(mat + d3::kMatAnim) != 0 ? 1 : 0;
    set_variant(item, 2);
    if (const u32 l = layers(item); l != 0) {
        set_variant(item, 3);
        idx = layered(idx, l) + 10;  // rlwimi, + 12
    } else {
        idx += 2;
    }
    ret(ctx, shader_at(mat, idx));
}

// SlyWater: animated bit + 2 when layered + 4 with more than two
// textures; the variant is the index.
DECOMP(005D30F8, SlyWaterType_PickShader) {
    const u32 mat = arg<u32>(ctx, 1), item = arg<u32>(ctx, 2);
    u32 idx = load<u32>(mat + d3::kMatAnim) != 0 ? 1 : 0;
    if (layers(item) != 0) idx += 2;
    if (ref<u8>(mat + d3::kMatTextureCount) > 2) idx += 4;
    set_variant(item, idx);
    ret(ctx, shader_at(mat, idx));
}

// ---------------------------------------------------------------- Bind (Shader slot 8)
// void <Shader>_Bind(Shader* s, Material* mat)

// The material's UV transform into vertex constants 4..5.
DECOMP(005AE0B0, SimpleColorTxrUV_Bind) { upload_uv(ctx, load<u32>(arg<u32>(ctx, 1) + d3::kMatAnim), 0x005AE0A8); }
DECOMP(005B47B8, SelfLight_Bind) { upload_uv(ctx, load<u32>(arg<u32>(ctx, 1) + d3::kMatAnim), 0x005B47B4); }
DECOMP(005B6780, SAC_Skinned_Bind) { upload_uv(ctx, load<u32>(arg<u32>(ctx, 1) + d3::kMatAnim), 0x005B6778); }
DECOMP(005B7220, SAC_Environment_Bind) { upload_uv(ctx, load<u32>(arg<u32>(ctx, 1) + d3::kMatAnim), 0x005B7218); }

// ProjTxr: fragment constant 0.x = 1.0 / 0.0 by the blend mode (2: the second f32).
DECOMP(005B0C78, ProjTxr_Bind) {
    constexpr u32 k = 0x005B0C6C;  // f32 pair in .text
    const u32 mat = arg<u32>(ctx, 1);
    set_constant_x(ctx, 0, load<u32>(k + (blend_mode(mat) == 2 ? 4 : 0)));
}

// Sly2UV: the UV transform when animated, then fragment constant 0.x by
// the blend mode (2: the first f32, else the second).
DECOMP(005B9C80, Sly2UV_Bind) {
    constexpr u32 k = 0x005B9C78;  // f32 pair in .text
    const u32 mat = arg<u32>(ctx, 1);
    if (const u32 anim = load<u32>(mat + d3::kMatAnim); anim != 0) upload_uv(ctx, anim, k);
    set_constant_x(ctx, 0, load<u32>(k + (blend_mode(mat) == 2 ? 0 : 4)));
}

// SlyCharacterFurFin: vertex constant 7 = {params +0, params +4, k, k}
// (k for missing parameters).
DECOMP(005C1368, SlyCharacterFurFin_Bind) {
    constexpr u32 k = 0x005C1364;  // f32 in .text
    const u32 mat = arg<u32>(ctx, 1);
    const u32 params = load<u32>(mat + d3::kMatParams);
    Stack frame(ctx, 16);
    const u32 v = frame.addr();
    store<u32>(v, load<u32>(params != 0 ? params : k));
    store<u32>(v + 4, load<u32>(params != 0 ? params + 4 : k));
    store<u32>(v + 8, load<u32>(k));
    store<u32>(v + 12, load<u32>(k));
    call(ctx, fx::fn::cellGcmSetVertexProgramConstants, {load<u32>(fx::g::state), 7, 4, v});
}

// SlyCharacterFakeFur: SlyCharacter2's Bind, then fragment constant 5 =
// {params +52, k, k, k}.
DECOMP(005C1E40, SlyCharacterFakeFur_Bind) {
    constexpr u32 k = 0x005C1E3C;  // f32 in .text
    const u32 s = arg<u32>(ctx, 0), mat = arg<u32>(ctx, 1);
    call(ctx, d3::fn::SlyCharacter2_Bind, {s, mat});
    const u32 params = load<u32>(mat + d3::kMatParams);
    Stack frame(ctx, 16);
    const u32 v = frame.addr();
    store<u32>(v, load<u32>(params != 0 ? params + 52 : k));
    for (u32 i = 4; i < 16; i += 4) store<u32>(v + i, load<u32>(k));
    call(ctx, d3::fn::FProgram_SetConstant, {5, v});
}

// Decal: the UV transform with the material's parameters (+124: su, sv;
// k without), then fragment constant 0.x = k, or k + 4 for blend mode 2.
DECOMP(005B30A0, Decal_Bind) {
    constexpr u32 k = 0x005B3094;  // f32 pair in .text (0.0, 1.0)
    const u32 mat = arg<u32>(ctx, 1);
    const u32 params = load<u32>(mat + d3::kMatParams), anim = load<u32>(mat + d3::kMatAnim);
    const f64 su = static_cast<f64>(load<f32>(params != 0 ? params : k));
    const f64 sv = static_cast<f64>(load<f32>(params != 0 ? params + 4 : k));
    callf(ctx, d3::fn::MaterialAnim_UploadUV, {anim, 4}, {su, sv});
    set_constant_x(ctx, 0, load<u32>(k + (blend_mode(mat) == 2 ? 4 : 0)));
}

// Fragment constant 0 = the material's parameters (+124: three f32, the
// second constant of the pair for each missing one) and w (by default the
// second constant too).
static void set_params_constant(PPUContext& ctx, u32 mat, u32 k, u32 w = 0) {
    const u32 params = load<u32>(mat + d3::kMatParams);
    Stack frame(ctx, 16);
    const u32 v = frame.addr();
    for (u32 i = 0; i < 3; ++i) store<u32>(v + 4 * i, load<u32>(params != 0 ? params + 4 * i : k + 4));
    store<u32>(v + 12, load<u32>(w != 0 ? w : k + 4));
    call(ctx, d3::fn::FProgram_SetConstant, {0, v});
}

// AngleIndex: the UV transform (scales k), then the parameters constant.
DECOMP(005B50A0, AngleIndex_Bind) {
    constexpr u32 k = 0x005B5094;  // f32 pair in .text (1.0, 0.0)
    const u32 mat = arg<u32>(ctx, 1);
    upload_uv(ctx, load<u32>(mat + d3::kMatAnim), k);
    set_params_constant(ctx, mat, k);
}

// The UV transform when animated, then the parameters constant.
// k: f32 pair in .text (1.0, 0.0)
DECOMP(005C9898, SlyPanorama_Bind) {
    const u32 mat = arg<u32>(ctx, 1);
    if (const u32 anim = load<u32>(mat + d3::kMatAnim); anim != 0) upload_uv(ctx, anim, 0x005C9890);
    set_params_constant(ctx, mat, 0x005C9890);
}
DECOMP(005CAD48, SlyProp_Bind) {
    const u32 mat = arg<u32>(ctx, 1);
    if (const u32 anim = load<u32>(mat + d3::kMatAnim); anim != 0) upload_uv(ctx, anim, 0x005CAD3C);
    set_params_constant(ctx, mat, 0x005CAD3C);
}
DECOMP(005CC170, SlyTerrain_Bind) {
    const u32 mat = arg<u32>(ctx, 1);
    if (const u32 anim = load<u32>(mat + d3::kMatAnim); anim != 0) upload_uv(ctx, anim, 0x005CC164);
    set_params_constant(ctx, mat, 0x005CC164);
}

// The UV transform, then the parameters constant with w = k for blend
// mode 2 (else k + 4).
// k: f32 pair in .text (1.0, 0.0).
DECOMP(005B34C0, EnvMap_Bind) {
    const u32 mat = arg<u32>(ctx, 1);
    const u32 anim = load<u32>(mat + d3::kMatAnim);
    upload_uv(ctx, anim, 0x005B34B4, 4);
    set_params_constant(ctx, mat, 0x005B34B4, blend_mode(mat) == 2 ? (0x005B34B4) : (0x005B34B4) + 4);
}
DECOMP(005B8DF0, SAC_EnvMap_Bind) {
    const u32 mat = arg<u32>(ctx, 1);
    const u32 anim = load<u32>(mat + d3::kMatAnim);
    upload_uv(ctx, anim, 0x005B8DE8, 4);
    set_params_constant(ctx, mat, 0x005B8DE8, blend_mode(mat) == 2 ? (0x005B8DE8) : (0x005B8DE8) + 4);
}
DECOMP(005BB408, SlyCharacter_Bind) {
    const u32 mat = arg<u32>(ctx, 1);
    const u32 anim = load<u32>(mat + d3::kMatAnim);
    upload_uv(ctx, anim, 0x005BB400, 4);
    set_params_constant(ctx, mat, 0x005BB400, blend_mode(mat) == 2 ? (0x005BB400) : (0x005BB400) + 4);
}
DECOMP(005C27B0, SlyCharacterCollectible_Bind) {
    const u32 mat = arg<u32>(ctx, 1);
    const u32 anim = load<u32>(mat + d3::kMatAnim);
    upload_uv(ctx, anim, 0x005C27A8, 4);
    set_params_constant(ctx, mat, 0x005C27A8, blend_mode(mat) == 2 ? (0x005C27A8) : (0x005C27A8) + 4);
}
// Only when animated; SlyMirror's transform goes to constants 8..9.
DECOMP(005C56E0, SlyEnvMap_Bind) {
    const u32 mat = arg<u32>(ctx, 1);
    const u32 anim = load<u32>(mat + d3::kMatAnim);
    if (anim != 0) upload_uv(ctx, anim, 0x005C56D4, 4);
    set_params_constant(ctx, mat, 0x005C56D4, blend_mode(mat) == 2 ? (0x005C56D4) : (0x005C56D4) + 4);
}
DECOMP(005C8268, SlyMirror_Bind) {
    const u32 mat = arg<u32>(ctx, 1);
    const u32 anim = load<u32>(mat + d3::kMatAnim);
    if (anim != 0) upload_uv(ctx, anim, 0x005C8260, 8);
    set_params_constant(ctx, mat, 0x005C8260, blend_mode(mat) == 2 ? (0x005C8260) : (0x005C8260) + 4);
}

// GeoParticle: Draw_SetParticleState(0, material +100 bit 6, 32), then
// fragment constant 0 = {0, 0, 0, k - (parameters +12 set ? k : k + 4)}.
DECOMP(005B0388, GeoParticle_Bind) {
    constexpr u32 k = 0x005B0380;  // f32 pair in .text (1.0, 0.0)
    const u32 mat = arg<u32>(ctx, 1);
    call(ctx, d3::fn::Draw_SetParticleState, {0, (ref<u8>(mat + d3::kMatFlags) >> 6) & 1u, 32});  // rlwinm 26,31,31
    const u32 params = load<u32>(mat + d3::kMatParams);
    const bool on = params != 0 && load<u32>(params + 12) != 0;
    const f32 w = static_cast<f32>(static_cast<f64>(load<f32>(k)) - static_cast<f64>(load<f32>(on ? k : k + 4)));  // fsubs
    Stack frame(ctx, 16);
    const u32 v = frame.addr();
    for (u32 i = 0; i < 12; i += 4) store<u32>(v + i, 0);
    store<f32>(v + 12, w);
    call(ctx, d3::fn::FProgram_SetConstant, {0, v});
}

// ---------------------------------------------------------------- End (Shader slot 16)
// void <Shader>_End(Shader* s, u32)

// Diffuse: alpha test off.
DECOMP(005B2914, Diffuse_End) {
    fx::emit({render::layers4::kAlphaTest, 0});
    ctx.gpr[3] = load<u32>(fx::g::state);  // left in r3
}

// ---------------------------------------------------------------- empty slots (blr)
DECOMP(005ADA08, Debug_Bind) {}
DECOMP(007ABEAC, Debug_End) {}
DECOMP(005ADD50, PrimTxr_Bind) {}
DECOMP(007AC130, PrimTxr_End) {}
DECOMP(005ADEE0, SimpleColorTxr_Bind) {}
DECOMP(007AC450, SimpleColorTxr_End) {}
DECOMP(007AC604, SimpleColorTxrUV_End) {}
DECOMP(005AF07C, TextureWarp_End) {}
DECOMP(005AFAB8, ScreenWarp_End) {}
DECOMP(005AFACC, LineOutline_Bind) {}
DECOMP(005AFAD0, LineOutline_Draw) {}
DECOMP(007ACCA8, LineOutline_End) {}
DECOMP(005AFBE8, Outline_Bind) {}
DECOMP(007ACD64, Outline_End) {}
DECOMP(007AD014, GeoParticle_End) {}
DECOMP(007AD510, ProjTxr_End) {}
DECOMP(005B182C, ProjTxr_Deferred_Bind) {}
DECOMP(007AD5CC, ProjTxr_Deferred_End) {}
DECOMP(005B1AE4, ZPass_Bind) {}
DECOMP(007AD914, ZPass_End) {}
DECOMP(007ADA88, Decal_End) {}
DECOMP(007AE17C, EnvMap_End) {}
DECOMP(007AE238, SelfLight_End) {}
DECOMP(007AE2F4, AngleIndex_End) {}
DECOMP(005B6420, Water_End) {}
DECOMP(007AE468, SAC_Skinned_End) {}
DECOMP(007AE524, SAC_Environment_End) {}
DECOMP(005B78A0, SAC_Prop_Bind) {}
DECOMP(007AE5E0, SAC_Prop_End) {}
DECOMP(005B7F0C, SAC_LitProp_Bind) {}
DECOMP(007AE69C, SAC_LitProp_End) {}
DECOMP(005B8638, SAC_FastProp_Bind) {}
DECOMP(007AE758, SAC_FastProp_End) {}
DECOMP(007AE814, SAC_EnvMap_End) {}
DECOMP(007AEA40, Sly2UV_End) {}
DECOMP(007AEAFC, SlyCaustics_End) {}
DECOMP(005BC02C, SlyCharacter_End) {}
DECOMP(005BD79C, SlyCharacter2_End) {}
DECOMP(005BE0D4, SlyCharacterEye_Bind) {}
DECOMP(005BF144, SlyCharacterEye_End) {}
DECOMP(005BFB08, SlyCharacterFur_Bind) {}
DECOMP(005C0C24, SlyCharacterFur_End) {}
DECOMP(005C1A0C, SlyCharacterFurFin_End) {}
DECOMP(005C22A4, SlyCharacterFakeFur_End) {}
DECOMP(005C3208, SlyCharacterCollectible_End) {}
DECOMP(007AF0C0, SlyDecal_End) {}
DECOMP(007AF17C, SlyDecal4_End) {}
DECOMP(007AF238, SlyEnvMap_End) {}
DECOMP(007AF2F4, SlyHeightFog_End) {}
DECOMP(007AF3B0, SlyMirror_End) {}
DECOMP(007AF46C, SlyPanorama_End) {}
DECOMP(005CAA90, SlyProjTxr_Bind) {}
DECOMP(007AF528, SlyProjTxr_Draw) {}
DECOMP(007AF52C, SlyProjTxr_End) {}
DECOMP(007AF5E8, SlyProp_End) {}
DECOMP(005CDC14, SlyScreenWarp_End) {}
DECOMP(005CDC28, SlyShrub_Bind) {}
DECOMP(007AFC4C, SlyShrub_End) {}
DECOMP(007AF6A4, SlyTerrain_End) {}
DECOMP(007AFD08, SlyTerrainSnow_End) {}
DECOMP(007AFDC4, SlyTerrainSnowDecal_End) {}
DECOMP(005D1EC0, SlyTextureWarp_End) {}
DECOMP(005D2C00, SlyWater_End) {}
DECOMP(005D3F84, SlyWater2_End) {}
DECOMP(005D5A68, SlyWater3_End) {}
