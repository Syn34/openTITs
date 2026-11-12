// The ShaderType slots 0 (Create: makes the type's shaders), 8 (the
// destructor) and 12 (delete). (ShaderType / Shader layouts: rsx.h.)
//
// Each Create allocates 44-byte Shaders (Mem_Alloc(44, 0, 16)), runs the
// Shader constructor, clears +12, sets the shader's vtable and loads its
// programs (Shader_Load(shader, vertex program name, fragment program
// name or 0, vertex parameter table, fragment parameter table): files
// "System\\Shaders\\<name>_V.vpo" / "<name>.fpo"), and appends each to the
// type's shader list (+12), a failed allocation as a null entry. The
// larger types take their names from 12-byte tables in .data ({+0
// vertex program, +4 its skinned twin, +8 fragment program}; a null +0
// leaves null entries).
#include "shader3.h"
#include "rsx.h"

#include <utility>

namespace sly4::gfx {

namespace {

namespace s3 = sly4::render::shader3;
namespace ty = sly4::render::shader3::types;

constexpr u32 kNull = 0;

// A name table entry.
struct NameEntry {
    be_u32 vertex;    // +0: const char*
    be_u32 skinned;   // +4: the skinned twin
    be_u32 fragment;  // +8
};

// Appends s to the type's shader list (PtrVector at +12), doubling the
// capacity when full.
void push(ShaderType* type, ShaderPrograms* s) {
    PtrVector* list = reinterpret_cast<PtrVector*>(&type->shaders);
    if (list->count == list->capacity)
        container::PtrVector_Reserve(list, (static_cast<u32>(list->capacity) << 1) & 0x1FFFEu);  // rlwinm 1,15,30
    const u32 count = list->count;
    *at<gp<ShaderPrograms>>(list->data.get(), (count << 2) & 0x3FFFCu) = s;  // rlwinm 2,14,29
    const u32 n = (list->count + 1u) & 0xFFFF;
    const u32 high = list->high_water;
    list->count = static_cast<u16>(n);
    list->high_water = static_cast<u16>(high > n ? high : n);
}

// A new shader with vtable vt and the programs vname / fname, or 0 when
// the allocation fails. The names are read after the allocation, as the
// original does for the table entries.
template <typename Names>
ShaderPrograms* make(u32 vt, Names names, u32 vparams, u32 fparams) {
    ShaderPrograms* s = call<ShaderPrograms*>(sly4::fn::Mem_Alloc, ty::kShaderSize, 0u, 16u);
    if (!s) return nullptr;
    const auto [vname, fname] = names();
    call(ty::fn::Shader_Construct, s);
    *at<u8>(s, 12) = 0;
    s->vtable = vt;
    call(ty::fn::Shader_Load, s, vname, fname, vparams, fparams);
    return s;
}

// One shader with fixed names.
void add(ShaderType* type, u32 vt, u32 vname, u32 fname, u32 vparams, u32 fparams) {
    push(type, make(vt, [=] { return std::pair{vname, fname}; }, vparams, fparams));
}

// Entries first..end - 1 of a name table: per entry one shader (+0, +8),
// with `skinned` a second (+4, +8); with `null_check` an entry whose +0
// is 0 adds null entries instead.
void add_table(ShaderType* type, u32 vt, u32 table, u32 first, u32 end, bool skinned, bool null_check, u32 vparams,
               u32 fparams) {
    for (u32 i = first; i < end; ++i) {  // cmplwi
        const NameEntry* e = host<NameEntry>(table + 12 * i);
        if (null_check && e->vertex == 0) {
            push(type, nullptr);
            if (skinned) push(type, nullptr);
            continue;
        }
        push(type, make(vt, [=] { return std::pair{e->vertex.get(), e->fragment.get()}; }, vparams, fparams));
        if (skinned) push(type, make(vt, [=] { return std::pair{e->skinned.get(), e->fragment.get()}; }, vparams, fparams));
    }
}

}  // namespace

// ---------------------------------------------------------------- Create (ShaderType slot 0)
// void <Type>Type_Create(ShaderType* t) (those without shaders take no argument)

void DebugType_Create(ShaderType* t) {
    add(t, 0x00B24EF8, 0x00B24F30, 0x00B24F30, kNull, 0x00F88AFC);  // "FlatColor", "FlatColor"
    add(t, 0x00B24EF8, 0x00B24F3A, 0x00B24F30, kNull, 0x00F88AFC);  // "FlatColor_SK", "FlatColor"
}
void PrimTxrType_Create(ShaderType* t) {
    add(t, 0x00B24F58, 0x00B24F8C, 0x00B24F8C, kNull, 0x00F88B14);  // "PrimTxr", "PrimTxr"
}
void PrimNoTxrType_Create(ShaderType* t) {
    add(t, 0x00B24F58, 0x00B24FB8, 0x00B24FB8, kNull, 0x00F88B14);  // "PrimNoTxr", "PrimNoTxr"
}
void SimpleColorTxrType_Create(ShaderType* t) {
    add(t, 0x00B24FD8, 0x00B25010, 0x00B25010, kNull, 0x00F88B24);  // "SimpleColorTxr", "SimpleColorTxr"
}
void SimpleColorTxrUVType_Create(ShaderType* t) {
    add(t, 0x00B25038, 0x00B25070, 0x00B25070, 0x00F88B5C, 0x00F88B3C);  // "SimpleColorTxrUV", "SimpleColorTxrUV"
}
void TextureWarpType_Create(ShaderType* t) {
    add(t, 0x00B250D0, 0x00AF8218, kNull, 0x00F88B7C, 0x00F88B6C);  // "TextureWarp", 0
    add(t, 0x00B250D0, 0x00AF8228, kNull, 0x00F88B7C, 0x00F88B6C);  // "TextureWarp_Lit", 0
}
void ScreenWarpType_Create(ShaderType* t) {
    add(t, 0x00B250F0, 0x00B25128, kNull, 0x00F88BD4, 0x00F88BC4);  // "ScreenWarp", 0
    add(t, 0x00B250F0, 0x00B25133, kNull, 0x00F88BD4, 0x00F88BC4);  // "ScreenWarp_Lit", 0
}
void LineOutlineType_Create(ShaderType* t) {
    add(t, 0x00B25158, 0x00AF8238, 0x00AF8238, 0x00F88C14, 0x00F88BEC);  // "LineOutline", "LineOutline"
}
void OutlineType_Create(ShaderType* t) {
    add(t, 0x00B251A8, 0x00AF8244, 0x00AF8244, 0x00F88C34, 0x00F88C1C);  // "Outline", "Outline"
    add(t, 0x00B251A8, 0x00AF824C, 0x00AF8244, 0x00F88C34, 0x00F88C1C);  // "Outline_SK", "Outline"
    add(t, 0x00B251A8, 0x00AF825C, 0x00AF8244, 0x00F88C34, 0x00F88C1C);  // "OutlineNS", "Outline"
    add(t, 0x00B251A8, 0x00AF826C, 0x00AF8244, 0x00F88C34, 0x00F88C1C);  // "OutlineNS_SK", "Outline"
}
void ParticleMaterialType_Create() {}  // no shaders
void GeoParticleType_Create(ShaderType* t) {
    add(t, 0x00B25220, 0x00B25258, 0x00B25258, 0x00F88C5C, 0x00F88C44);  // "GeoParticle", "GeoParticle"
}
void SprParticleType_Create(ShaderType* t) {
    add(t, 0x00B25278, 0x00B252AC, kNull, 0x00F88C7C, 0x00F88C6C);  // "SprParticle", 0
    add(t, 0x00B25278, 0x00B252B8, kNull, 0x00F88C7C, 0x00F88C6C);  // "SprParticle_Soft", 0
}
void ShadowType_Create() {}  // no shaders
void ProjTxrType_Create(ShaderType* t) {
    add(t, 0x00B25310, 0x00AF533C, 0x00AF533C, 0x00F88CAC, 0x00F88C8C);  // "ProjTxr", "ProjTxr"
    add(t, 0x00B25310, 0x00AF533C, 0x00AF533C, 0x00F88CAC, 0x00F88C8C);  // "ProjTxr", "ProjTxr"
}
void ProjTxr_DeferredType_Create(ShaderType* t) {
    add(t, 0x00B25358, 0x00B25390, 0x00B25390, 0x00F88CD4, 0x00F88CBC);  // "ProjTxr_Deferred", "ProjTxr_Deferred"
    add(t, 0x00B25358, 0x00B25390, 0x00B253A1, 0x00F88CD4, 0x00F88CBC);  // "ProjTxr_Deferred", "ProjTxr_Deferred_NF"
}
void ZPassType_Create(ShaderType* t) {
    add(t, 0x00B25420, 0x00AF5434, 0x00AF5434, 0x00F88D3C, 0x00F88D2C);  // "ZPass", "ZPass"
    add(t, 0x00B25420, 0x00AF543C, 0x00AF5434, 0x00F88D3C, 0x00F88D2C);  // "ZPass_SK", "ZPass"
}
void DiffuseType_Create(ShaderType* t) {
    add(t, 0x00B25490, 0x00AF545C, 0x00AF545C, 0x00F88D5C, 0x00F88D44);  // "Diffuse", "Diffuse"
    add(t, 0x00B25490, 0x00AF5464, 0x00AF545C, 0x00F88D5C, 0x00F88D44);  // "Diffuse_SK", "Diffuse"
    add_table(t, 0x00B25490, 0x00F88D7C, 1, 24, true, true, 0x00F88D5C, 0x00F88D44);
}
void DecalType_Create(ShaderType* t) {
    add(t, 0x00B254B0, 0x00B254E4, 0x00B254E4, 0x00F88EB4, 0x00F88E9C);  // "Decal", "Decal"
    add(t, 0x00B254B0, 0x00B254EA, 0x00B254E4, 0x00F88EB4, 0x00F88E9C);  // "Decal_SK", "Decal"
    add_table(t, 0x00B254B0, 0x00F88EC4, 1, 8, true, true, 0x00F88EB4, 0x00F88E9C);
}
void EnvMapType_Create(ShaderType* t) {
    add(t, 0x00B25508, 0x00AF5764, 0x00AF5764, 0x00F88F4C, 0x00F88F24);  // "EnvMapS", "EnvMapS"
    add(t, 0x00B25508, 0x00AF576C, 0x00AF5764, 0x00F88F4C, 0x00F88F24);  // "EnvMapS_SK", "EnvMapS"
    add_table(t, 0x00B25508, 0x00F88F84, 1, 24, true, true, 0x00F88F4C, 0x00F88F24);
}
void SelfLightType_Create(ShaderType* t) {
    add(t, 0x00B25550, 0x00AF8288, 0x00AF8288, 0x00F890BC, 0x00F890A4);  // "SelfLight_Lit", "SelfLight_Lit"
}
void AngleIndexType_Create(ShaderType* t) {
    add(t, 0x00B255A0, 0x00AF5964, 0x00AF5964, 0x00F890F4, 0x00F890DC);  // "AngleIndex", "AngleIndex"
    add(t, 0x00B255A0, 0x00AF5974, 0x00AF5964, 0x00F890F4, 0x00F890DC);  // "AngleIndex_SK", "AngleIndex"
}
void WaterType_Create(ShaderType* t) {
    add(t, 0x00B25618, 0x00AF8298, kNull, 0x00F89114, 0x00F8915C);  // "Water", 0
}
void SAC_SkinnedType_Create(ShaderType* t) {
    add(t, 0x00B25638, 0x00AF82A0, 0x00AF82A0, 0x00F8919C, kNull);  // "SAC_Skinned", "SAC_Skinned"
    add(t, 0x00B25638, 0x00AF82B0, 0x00AF82B0, 0x00F8919C, kNull);  // "SAC_Skinned_EnvMap", "SAC_Skinned_EnvMap"
}
void SAC_EnvironmentType_Create(ShaderType* t) {
    add(t, 0x00B25688, 0x00AF82C4, 0x00AF82C4, 0x00F891AC, kNull);  // "SAC_Environment", "SAC_Environment"
}
void SAC_PropType_Create(ShaderType* t) {
    add(t, 0x00B256D8, 0x00AF82D4, 0x00AF82D4, 0x00F891BC, kNull);  // "SAC_Prop", "SAC_Prop"
}
void SAC_LitPropType_Create(ShaderType* t) {
    add(t, 0x00B25728, 0x00AF82E0, 0x00AF82E0, 0x00F891CC, kNull);  // "SAC_LitProp", "SAC_LitProp"
}
void SAC_FastPropType_Create(ShaderType* t) {
    add(t, 0x00B25778, 0x00AF82EC, 0x00AF82EC, 0x00F891DC, kNull);  // "SAC_FastProp", "SAC_FastProp"
}
void SAC_EnvMapType_Create(ShaderType* t) {
    add(t, 0x00B257C8, 0x00AF82FC, 0x00AF82FC, 0x00F891EC, kNull);  // "SAC_EnvMap", "SAC_EnvMap"
}
// SAC_Panorama: no shaders; notes that the type was created (g::panorama_created).
void SAC_PanoramaType_Create() {
    u8& created = global<u8>(s3::g::panorama_created);
    if (created == 0) created = 1;
}
void SAC_ParticleType_Create() {}  // no shaders
void Sly2UVType_Create(ShaderType* t) {
    add(t, 0x00B25870, 0x00AF5B3C, 0x00AF5B3C, 0x00F8923C, 0x00F89224);  // "Sly2UV", "Sly2UV"
    add(t, 0x00B25870, 0x00AF5B44, 0x00AF5B3C, 0x00F8923C, 0x00F89224);  // "Sly2UV_UVX", "Sly2UV"
    add_table(t, 0x00B25870, 0x00F8924C, 1, 5, true, true, 0x00F8923C, 0x00F89224);
}
void SlyCausticsType_Create(ShaderType* t) {
    add(t, 0x00B258B8, 0x00AF5C0C, 0x00AF5C0C, 0x00F892A0, 0x00F89288);  // "SlyCaustics", "SlyCaustics"
    add(t, 0x00B258B8, 0x00AF5C1C, 0x00AF5C0C, 0x00F892A0, 0x00F89288);  // "SlyCaustics_UVX", "SlyCaustics"
    add_table(t, 0x00B258B8, 0x00F892B0, 1, 8, true, true, 0x00F892A0, 0x00F89288);
}
void SlyCharacterType_Create(ShaderType* t) {
    add(t, 0x00B25938, 0x00AF5CE4, 0x00AF5CE4, 0x00F89338, 0x00F89310);  // "SlyCharacter", "SlyCharacter"
    add(t, 0x00B25938, 0x00AF5CF4, 0x00AF5CE4, 0x00F89338, 0x00F89310);  // "SlyCharacter_SK", "SlyCharacter"
    add_table(t, 0x00B25938, 0x00F89358, 1, 24, true, true, 0x00F89338, 0x00F89310);
}
void SlyCharacter2Type_Create(ShaderType* t) {
    add(t, 0x00B25958, 0x00AF5FBC, 0x00AF5FBC, 0x00F894D0, 0x00F89478);  // "SlyCharacter2", "SlyCharacter2"
    add(t, 0x00B25958, 0x00AF5FCC, 0x00AF5FBC, 0x00F894D0, 0x00F89478);  // "SlyCharacter2_SK", "SlyCharacter2"
    add_table(t, 0x00B25958, 0x00F89508, 1, 17, true, true, 0x00F894D0, 0x00F89478);
}
void SlyCharacterEyeType_Create(ShaderType* t) {
    add(t, 0x00B259D8, 0x00AF6234, 0x00AF6234, 0x00F8961C, 0x00F895D4);  // "SlyCharacterEye", "SlyCharacterEye"
    add(t, 0x00B259D8, 0x00AF6244, 0x00AF6234, 0x00F8961C, 0x00F895D4);  // "SlyCharacterEye_SK", "SlyCharacterEye"
    add_table(t, 0x00B259D8, 0x00F89644, 1, 6, true, true, 0x00F8961C, 0x00F895D4);
}
void SlyCharacterFurType_Create(ShaderType* t) {
    add_table(t, 0x00B25A28, 0x00F8971C, 0, 10, true, true, 0x00F89704, 0x00F8968C);
}
void SlyCharacterFurFinType_Create(ShaderType* t) {
    add_table(t, 0x00B25A80, 0x00F897CC, 0, 5, true, true, 0x00F897C4, 0x00F89794);
}
void SlyCharacterFakeFurType_Create(ShaderType* t) {
    add_table(t, 0x00B25AD8, 0x00F89898, 0, 13, true, true, 0x00F89858, 0x00F89808);
}
void SlyCharacterCollectibleType_Create(ShaderType* t) {
    add_table(t, 0x00B25B30, 0x00F8997C, 0, 24, true, true, 0x00F8995C, 0x00F89934);
}
void SlyDecalType_Create(ShaderType* t) {
    add(t, 0x00B25B50, 0x00AF68EC, 0x00AF68EC, 0x00F89AB4, 0x00F89A9C);  // "SlyDecal", "SlyDecal"
    add(t, 0x00B25B50, 0x00AF68FC, 0x00AF68EC, 0x00F89AB4, 0x00F89A9C);  // "SlyDecal_UVX", "SlyDecal"
    add_table(t, 0x00B25B50, 0x00F89AD4, 1, 24, true, true, 0x00F89AB4, 0x00F89A9C);
}
void SlyDecal4Type_Create(ShaderType* t) {
    add_table(t, 0x00B25BA0, 0x00F89C24, 0, 6, false, true, 0x00F89C1C, 0x00F89BF4);
}
void SlyEnvMapType_Create(ShaderType* t) {
    add(t, 0x00B25BF0, 0x00AF6B14, 0x00AF6B14, 0x00F89C94, 0x00F89C6C);  // "SlyEnvMap", "SlyEnvMap"
    add(t, 0x00B25BF0, 0x00AF6B24, 0x00AF6B14, 0x00F89C94, 0x00F89C6C);  // "SlyEnvMap_UVX", "SlyEnvMap"
    add_table(t, 0x00B25BF0, 0x00F89CCC, 1, 24, true, true, 0x00F89C94, 0x00F89C6C);
}
void SlyHeightFogType_Create(ShaderType* t) {
    add_table(t, 0x00B25C40, 0x00F89E7C, 0, 4, true, false, 0x00F89E2C, 0x00F89DEC);
}
void SlyMirrorType_Create(ShaderType* t) {
    add(t, 0x00B25C90, 0x00AF6EB4, 0x00AF6EB4, 0x00F89EDC, 0x00F89EAC);  // "SlyMirror", "SlyMirror"
    add(t, 0x00B25C90, 0x00AF6EC4, 0x00AF6EB4, 0x00F89EDC, 0x00F89EAC);  // "SlyMirror_UVX", "SlyMirror"
    add_table(t, 0x00B25C90, 0x00F89F1C, 1, 24, true, true, 0x00F89EDC, 0x00F89EAC);
}
void SlyPanoramaType_Create(ShaderType* t) {
    add(t, 0x00B25CE0, 0x00AF705C, 0x00AF705C, 0x00F8A054, 0x00F8A03C);  // "SlyPanorama", "SlyPanorama"
    add(t, 0x00B25CE0, 0x00AF706C, 0x00AF705C, 0x00F8A054, 0x00F8A03C);  // "SlyPanorama_UVX", "SlyPanorama"
    add_table(t, 0x00B25CE0, 0x00F8A074, 1, 24, true, true, 0x00F8A054, 0x00F8A03C);
}
// SlyProjTxr: the two table entries (+0, +8), each also given fragment
// control 0xFC0 (Shader_SetFpControl).
void SlyProjTxrType_Create(ShaderType* t) {
    constexpr u32 table = 0x00F8A1FC;
    for (u32 k = 0; k < 2; ++k) {  // cmplwi
        const NameEntry* e = host<NameEntry>(table + 12 * k);
        ShaderPrograms* s = make(0x00B25D30, [=] { return std::pair{e->vertex.get(), e->fragment.get()}; }, 0x00F8A1F4, 0x00F8A194);
        if (s) call(ty::fn::Shader_SetFpControl, s, 0xFC0u);
        push(t, s);
    }
}
void SlyPropType_Create(ShaderType* t) {
    add(t, 0x00B25D80, 0x00AF7314, 0x00AF7314, 0x00F8A234, 0x00F8A214);  // "SlyProp", "SlyProp"
    add(t, 0x00B25D80, 0x00AF731C, 0x00AF7314, 0x00F8A234, 0x00F8A214);  // "SlyProp_UVX", "SlyProp"
    add_table(t, 0x00B25D80, 0x00F8A254, 1, 24, true, true, 0x00F8A234, 0x00F8A214);
}
void SlyTerrainType_Create(ShaderType* t) {
    add_table(t, 0x00B25EC0, 0x00F8A3AC, 0, 16, true, true, 0x00F8A38C, 0x00F8A374);
}
void SlyScreenWarpType_Create(ShaderType* t) {
    add(t, 0x00B25DC8, 0x00B25E00, kNull, 0x00F8A484, 0x00F8A46C);  // "SlyScreenWarp", 0
    add(t, 0x00B25DC8, 0x00B25E0E, kNull, 0x00F8A484, 0x00F8A46C);  // "SlyScreenWarp_Lit", 0
}
void SlyShadowMapType_Create() {}  // no shaders
void SlyShrubType_Create(ShaderType* t) {
    add(t, 0x00B25E70, 0x00AF8318, 0x00AF8318, 0x00F8A4D4, 0x00F8A49C);  // "SlyShrub", "SlyShrub"
}
void SlyTerrainSnowType_Create(ShaderType* t) {
    add_table(t, 0x00B25F10, 0x00F8A53C, 0, 11, true, true, 0x00F8A514, 0x00F8A4E4);
}
void SlyTerrainSnowDecalType_Create(ShaderType* t) {
    add_table(t, 0x00B25F60, 0x00F8A628, 0, 10, true, true, 0x00F8A5F8, 0x00F8A5C0);
}
void SlyTextureWarpType_Create(ShaderType* t) {
    add(t, 0x00B25FE8, 0x00AF8324, kNull, 0x00F8A6E8, 0x00F8A6A0);  // "SlyTextureWarp", 0
    add(t, 0x00B25FE8, 0x00AF8334, kNull, 0x00F8A6E8, 0x00F8A6A0);  // "SlyTextureWarp_Lit", 0
}
void SlyWaterType_Create(ShaderType* t) {
    add_table(t, 0x00B26038, 0x00F8A790, 0, 4, true, true, 0x00F8A738, 0x00F8A700);
}
void SlyWater2Type_Create(ShaderType* t) {
    add_table(t, 0x00B26088, 0x00F8A890, 0, 64, false, true, 0x00F8A860, 0x00F8A7C0);
}
void SlyWater3Type_Create(ShaderType* t) {
    add_table(t, 0x00B260D8, 0x00F8AC68, 0, 64, false, true, 0x00F8AC30, 0x00F8AB90);
}

// ---------------------------------------------------------------- destructor (slot 8) and delete (slot 12)

namespace {
// The base ShaderType destructor: base vtable, then the shader list
// cleared (its shaders are not deleted).
void destruct(ShaderType* t) {
    t->vtable = ty::kBaseVtable;
    call(ty::fn::PtrVector_Clear, &t->shaders);
    call(ty::fn::Nop, &t->shaders);
}
void destroy(ShaderType* t) {
    if (!t) return;
    destruct(t);
    call(ty::fn::Mem_FreeFrom, t, 0u);
}
}  // namespace

// void <Type>Type_Destruct(ShaderType* t)
void DebugType_Destruct(ShaderType* t) { destruct(t); }
void PrimTxrType_Destruct(ShaderType* t) { destruct(t); }
void PrimNoTxrType_Destruct(ShaderType* t) { destruct(t); }
void SimpleColorTxrType_Destruct(ShaderType* t) { destruct(t); }
void SimpleColorTxrUVType_Destruct(ShaderType* t) { destruct(t); }
void TextureWarpType_Destruct(ShaderType* t) { destruct(t); }
void ScreenWarpType_Destruct(ShaderType* t) { destruct(t); }
void LineOutlineType_Destruct(ShaderType* t) { destruct(t); }
void OutlineType_Destruct(ShaderType* t) { destruct(t); }
void ParticleMaterialType_Destruct(ShaderType* t) { destruct(t); }
void GeoParticleType_Destruct(ShaderType* t) { destruct(t); }
void SprParticleType_Destruct(ShaderType* t) { destruct(t); }
void ShadowType_Destruct(ShaderType* t) { destruct(t); }
void ProjTxrType_Destruct(ShaderType* t) { destruct(t); }
void ProjTxr_DeferredType_Destruct(ShaderType* t) { destruct(t); }
void ZPassType_Destruct(ShaderType* t) { destruct(t); }
void DiffuseType_Destruct(ShaderType* t) { destruct(t); }
void DecalType_Destruct(ShaderType* t) { destruct(t); }
void EnvMapType_Destruct(ShaderType* t) { destruct(t); }
void SelfLightType_Destruct(ShaderType* t) { destruct(t); }
void AngleIndexType_Destruct(ShaderType* t) { destruct(t); }
void WaterType_Destruct(ShaderType* t) { destruct(t); }
void SAC_SkinnedType_Destruct(ShaderType* t) { destruct(t); }
void SAC_EnvironmentType_Destruct(ShaderType* t) { destruct(t); }
void SAC_PropType_Destruct(ShaderType* t) { destruct(t); }
void SAC_LitPropType_Destruct(ShaderType* t) { destruct(t); }
void SAC_FastPropType_Destruct(ShaderType* t) { destruct(t); }
void SAC_EnvMapType_Destruct(ShaderType* t) { destruct(t); }
void SAC_PanoramaType_Destruct(ShaderType* t) { destruct(t); }
void SAC_ParticleType_Destruct(ShaderType* t) { destruct(t); }
void Sly2UVType_Destruct(ShaderType* t) { destruct(t); }
void SlyCausticsType_Destruct(ShaderType* t) { destruct(t); }
void SlyCharacterType_Destruct(ShaderType* t) { destruct(t); }
void SlyCharacter2Type_Destruct(ShaderType* t) { destruct(t); }
void SlyCharacterEyeType_Destruct(ShaderType* t) { destruct(t); }
void SlyCharacterFurType_Destruct(ShaderType* t) { destruct(t); }
void SlyCharacterFurFinType_Destruct(ShaderType* t) { destruct(t); }
void SlyCharacterFakeFurType_Destruct(ShaderType* t) { destruct(t); }
void SlyCharacterCollectibleType_Destruct(ShaderType* t) { destruct(t); }
void SlyDecalType_Destruct(ShaderType* t) { destruct(t); }
void SlyDecal4Type_Destruct(ShaderType* t) { destruct(t); }
void SlyEnvMapType_Destruct(ShaderType* t) { destruct(t); }
void SlyHeightFogType_Destruct(ShaderType* t) { destruct(t); }
void SlyMirrorType_Destruct(ShaderType* t) { destruct(t); }
void SlyPanoramaType_Destruct(ShaderType* t) { destruct(t); }
void SlyProjTxrType_Destruct(ShaderType* t) { destruct(t); }
void SlyPropType_Destruct(ShaderType* t) { destruct(t); }
void SlyTerrainType_Destruct(ShaderType* t) { destruct(t); }
void SlyScreenWarpType_Destruct(ShaderType* t) { destruct(t); }
void SlyShadowMapType_Destruct(ShaderType* t) { destruct(t); }
void SlyShrubType_Destruct(ShaderType* t) { destruct(t); }
void SlyTerrainSnowType_Destruct(ShaderType* t) { destruct(t); }
void SlyTerrainSnowDecalType_Destruct(ShaderType* t) { destruct(t); }
void SlyTextureWarpType_Destruct(ShaderType* t) { destruct(t); }
void SlyWaterType_Destruct(ShaderType* t) { destruct(t); }
void SlyWater2Type_Destruct(ShaderType* t) { destruct(t); }
void SlyWater3Type_Destruct(ShaderType* t) { destruct(t); }

// void <Type>Type_Delete(ShaderType* t)
void DebugType_Delete(ShaderType* t) { destroy(t); }
void PrimTxrType_Delete(ShaderType* t) { destroy(t); }
void PrimNoTxrType_Delete(ShaderType* t) { destroy(t); }
void SimpleColorTxrType_Delete(ShaderType* t) { destroy(t); }
void SimpleColorTxrUVType_Delete(ShaderType* t) { destroy(t); }
void TextureWarpType_Delete(ShaderType* t) { destroy(t); }
void ScreenWarpType_Delete(ShaderType* t) { destroy(t); }
void LineOutlineType_Delete(ShaderType* t) { destroy(t); }
void OutlineType_Delete(ShaderType* t) { destroy(t); }
void ParticleMaterialType_Delete(ShaderType* t) { destroy(t); }
void GeoParticleType_Delete(ShaderType* t) { destroy(t); }
void SprParticleType_Delete(ShaderType* t) { destroy(t); }
void ShadowType_Delete(ShaderType* t) { destroy(t); }
void ProjTxrType_Delete(ShaderType* t) { destroy(t); }
void ProjTxr_DeferredType_Delete(ShaderType* t) { destroy(t); }
void ZPassType_Delete(ShaderType* t) { destroy(t); }
void DiffuseType_Delete(ShaderType* t) { destroy(t); }
void DecalType_Delete(ShaderType* t) { destroy(t); }
void EnvMapType_Delete(ShaderType* t) { destroy(t); }
void SelfLightType_Delete(ShaderType* t) { destroy(t); }
void AngleIndexType_Delete(ShaderType* t) { destroy(t); }
void WaterType_Delete(ShaderType* t) { destroy(t); }
void SAC_SkinnedType_Delete(ShaderType* t) { destroy(t); }
void SAC_EnvironmentType_Delete(ShaderType* t) { destroy(t); }
void SAC_PropType_Delete(ShaderType* t) { destroy(t); }
void SAC_LitPropType_Delete(ShaderType* t) { destroy(t); }
void SAC_FastPropType_Delete(ShaderType* t) { destroy(t); }
void SAC_EnvMapType_Delete(ShaderType* t) { destroy(t); }
void SAC_PanoramaType_Delete(ShaderType* t) { destroy(t); }
void SAC_ParticleType_Delete(ShaderType* t) { destroy(t); }
void Sly2UVType_Delete(ShaderType* t) { destroy(t); }
void SlyCausticsType_Delete(ShaderType* t) { destroy(t); }
void SlyCharacterType_Delete(ShaderType* t) { destroy(t); }
void SlyCharacter2Type_Delete(ShaderType* t) { destroy(t); }
void SlyCharacterEyeType_Delete(ShaderType* t) { destroy(t); }
void SlyCharacterFurType_Delete(ShaderType* t) { destroy(t); }
void SlyCharacterFurFinType_Delete(ShaderType* t) { destroy(t); }
void SlyCharacterFakeFurType_Delete(ShaderType* t) { destroy(t); }
void SlyCharacterCollectibleType_Delete(ShaderType* t) { destroy(t); }
void SlyDecalType_Delete(ShaderType* t) { destroy(t); }
void SlyDecal4Type_Delete(ShaderType* t) { destroy(t); }
void SlyEnvMapType_Delete(ShaderType* t) { destroy(t); }
void SlyHeightFogType_Delete(ShaderType* t) { destroy(t); }
void SlyMirrorType_Delete(ShaderType* t) { destroy(t); }
void SlyPanoramaType_Delete(ShaderType* t) { destroy(t); }
void SlyProjTxrType_Delete(ShaderType* t) { destroy(t); }
void SlyPropType_Delete(ShaderType* t) { destroy(t); }
void SlyTerrainType_Delete(ShaderType* t) { destroy(t); }
void SlyScreenWarpType_Delete(ShaderType* t) { destroy(t); }
void SlyShadowMapType_Delete(ShaderType* t) { destroy(t); }
void SlyShrubType_Delete(ShaderType* t) { destroy(t); }
void SlyTerrainSnowType_Delete(ShaderType* t) { destroy(t); }
void SlyTerrainSnowDecalType_Delete(ShaderType* t) { destroy(t); }
void SlyTextureWarpType_Delete(ShaderType* t) { destroy(t); }
void SlyWaterType_Delete(ShaderType* t) { destroy(t); }
void SlyWater2Type_Delete(ShaderType* t) { destroy(t); }
void SlyWater3Type_Delete(ShaderType* t) { destroy(t); }

}  // namespace sly4::gfx

NATIVE(007ABEF4, sly4::gfx::DebugType_Create)
NATIVE(007AC178, sly4::gfx::PrimTxrType_Create)
NATIVE(007AC2E4, sly4::gfx::PrimNoTxrType_Create)
NATIVE(007AC498, sly4::gfx::SimpleColorTxrType_Create)
NATIVE(007AC64C, sly4::gfx::SimpleColorTxrUVType_Create)
NATIVE(005AF080, sly4::gfx::TextureWarpType_Create)
NATIVE(007AC8B8, sly4::gfx::ScreenWarpType_Create)
NATIVE(005AFAD4, sly4::gfx::LineOutlineType_Create)
NATIVE(005AFF0C, sly4::gfx::OutlineType_Create)
NATIVE(007ACE20, sly4::gfx::ParticleMaterialType_Create)
NATIVE(007AD05C, sly4::gfx::GeoParticleType_Create)
NATIVE(007AD210, sly4::gfx::SprParticleType_Create)
NATIVE(007AD484, sly4::gfx::ShadowType_Create)
NATIVE(005B155C, sly4::gfx::ProjTxrType_Create)
NATIVE(007AD614, sly4::gfx::ProjTxr_DeferredType_Create)
NATIVE(005B1C6C, sly4::gfx::ZPassType_Create)
NATIVE(005B2940, sly4::gfx::DiffuseType_Create)
NATIVE(007ADAD0, sly4::gfx::DecalType_Create)
NATIVE(005B4004, sly4::gfx::EnvMapType_Create)
NATIVE(005B4EC8, sly4::gfx::SelfLightType_Create)
NATIVE(005B56B0, sly4::gfx::AngleIndexType_Create)
NATIVE(005B6424, sly4::gfx::WaterType_Create)
NATIVE(005B6F54, sly4::gfx::SAC_SkinnedType_Create)
NATIVE(005B7780, sly4::gfx::SAC_EnvironmentType_Create)
NATIVE(005B7DEC, sly4::gfx::SAC_PropType_Create)
NATIVE(005B8518, sly4::gfx::SAC_LitPropType_Create)
NATIVE(005B8C60, sly4::gfx::SAC_FastPropType_Create)
NATIVE(005B9924, sly4::gfx::SAC_EnvMapType_Create)
NATIVE(007AE8D0, sly4::gfx::SAC_PanoramaType_Create)
NATIVE(007AE9AC, sly4::gfx::SAC_ParticleType_Create)
NATIVE(005BA1DC, sly4::gfx::Sly2UVType_Create)
NATIVE(005BAD8C, sly4::gfx::SlyCausticsType_Create)
NATIVE(005BC030, sly4::gfx::SlyCharacterType_Create)
NATIVE(005BD7A0, sly4::gfx::SlyCharacter2Type_Create)
NATIVE(005BF148, sly4::gfx::SlyCharacterEyeType_Create)
NATIVE(005C0C28, sly4::gfx::SlyCharacterFurType_Create)
NATIVE(005C1A10, sly4::gfx::SlyCharacterFurFinType_Create)
NATIVE(005C22A8, sly4::gfx::SlyCharacterFakeFurType_Create)
NATIVE(005C320C, sly4::gfx::SlyCharacterCollectibleType_Create)
NATIVE(005C4120, sly4::gfx::SlyDecalType_Create)
NATIVE(005C5104, sly4::gfx::SlyDecal4Type_Create)
NATIVE(005C6328, sly4::gfx::SlyEnvMapType_Create)
NATIVE(005C7B84, sly4::gfx::SlyHeightFogType_Create)
NATIVE(005C914C, sly4::gfx::SlyMirrorType_Create)
NATIVE(005CA368, sly4::gfx::SlyPanoramaType_Create)
NATIVE(005CAA98, sly4::gfx::SlyProjTxrType_Create)
NATIVE(005CB9D8, sly4::gfx::SlyPropType_Create)
NATIVE(005CCDF0, sly4::gfx::SlyTerrainType_Create)
NATIVE(007AF7A4, sly4::gfx::SlyScreenWarpType_Create)
NATIVE(007AFB94, sly4::gfx::SlyShadowMapType_Create)
NATIVE(005CE264, sly4::gfx::SlyShrubType_Create)
NATIVE(005CF28C, sly4::gfx::SlyTerrainSnowType_Create)
NATIVE(005D0974, sly4::gfx::SlyTerrainSnowDecalType_Create)
NATIVE(005D1EC4, sly4::gfx::SlyTextureWarpType_Create)
NATIVE(005D2C04, sly4::gfx::SlyWaterType_Create)
NATIVE(005D3F88, sly4::gfx::SlyWater2Type_Create)
NATIVE(005D5A6C, sly4::gfx::SlyWater3Type_Create)
NATIVE(007AA82C, sly4::gfx::DebugType_Destruct)
NATIVE(007AA87C, sly4::gfx::PrimTxrType_Destruct)
NATIVE(007AA8CC, sly4::gfx::PrimNoTxrType_Destruct)
NATIVE(007AA91C, sly4::gfx::SimpleColorTxrType_Destruct)
NATIVE(007AA96C, sly4::gfx::SimpleColorTxrUVType_Destruct)
NATIVE(007AA9BC, sly4::gfx::TextureWarpType_Destruct)
NATIVE(007AAA0C, sly4::gfx::ScreenWarpType_Destruct)
NATIVE(007AAA5C, sly4::gfx::LineOutlineType_Destruct)
NATIVE(007AAAAC, sly4::gfx::OutlineType_Destruct)
NATIVE(007AAAFC, sly4::gfx::ParticleMaterialType_Destruct)
NATIVE(007AAB4C, sly4::gfx::GeoParticleType_Destruct)
NATIVE(007AAB9C, sly4::gfx::SprParticleType_Destruct)
NATIVE(007AABEC, sly4::gfx::ShadowType_Destruct)
NATIVE(007AAC3C, sly4::gfx::ProjTxrType_Destruct)
NATIVE(007AAC8C, sly4::gfx::ProjTxr_DeferredType_Destruct)
NATIVE(007AAD2C, sly4::gfx::ZPassType_Destruct)
NATIVE(007AAD7C, sly4::gfx::DiffuseType_Destruct)
NATIVE(007AADCC, sly4::gfx::DecalType_Destruct)
NATIVE(007AAE1C, sly4::gfx::EnvMapType_Destruct)
NATIVE(007AAE6C, sly4::gfx::SelfLightType_Destruct)
NATIVE(007AAEBC, sly4::gfx::AngleIndexType_Destruct)
NATIVE(007AAF0C, sly4::gfx::WaterType_Destruct)
NATIVE(007AAF5C, sly4::gfx::SAC_SkinnedType_Destruct)
NATIVE(007AAFAC, sly4::gfx::SAC_EnvironmentType_Destruct)
NATIVE(007AAFFC, sly4::gfx::SAC_PropType_Destruct)
NATIVE(007AB04C, sly4::gfx::SAC_LitPropType_Destruct)
NATIVE(007AB09C, sly4::gfx::SAC_FastPropType_Destruct)
NATIVE(007AB0EC, sly4::gfx::SAC_EnvMapType_Destruct)
NATIVE(007AB13C, sly4::gfx::SAC_PanoramaType_Destruct)
NATIVE(007AB18C, sly4::gfx::SAC_ParticleType_Destruct)
NATIVE(007AB1DC, sly4::gfx::Sly2UVType_Destruct)
NATIVE(007AB22C, sly4::gfx::SlyCausticsType_Destruct)
NATIVE(007AB27C, sly4::gfx::SlyCharacterType_Destruct)
NATIVE(007AB2CC, sly4::gfx::SlyCharacter2Type_Destruct)
NATIVE(007AB31C, sly4::gfx::SlyCharacterEyeType_Destruct)
NATIVE(007AB36C, sly4::gfx::SlyCharacterFurType_Destruct)
NATIVE(007AB3BC, sly4::gfx::SlyCharacterFurFinType_Destruct)
NATIVE(007AB40C, sly4::gfx::SlyCharacterFakeFurType_Destruct)
NATIVE(007AB45C, sly4::gfx::SlyCharacterCollectibleType_Destruct)
NATIVE(007AB4AC, sly4::gfx::SlyDecalType_Destruct)
NATIVE(007AB4FC, sly4::gfx::SlyDecal4Type_Destruct)
NATIVE(007AB54C, sly4::gfx::SlyEnvMapType_Destruct)
NATIVE(007AB59C, sly4::gfx::SlyHeightFogType_Destruct)
NATIVE(007AB5EC, sly4::gfx::SlyMirrorType_Destruct)
NATIVE(007AB63C, sly4::gfx::SlyPanoramaType_Destruct)
NATIVE(007AB68C, sly4::gfx::SlyProjTxrType_Destruct)
NATIVE(007AB6DC, sly4::gfx::SlyPropType_Destruct)
NATIVE(007AB81C, sly4::gfx::SlyTerrainType_Destruct)
NATIVE(007AB72C, sly4::gfx::SlyScreenWarpType_Destruct)
NATIVE(007AB77C, sly4::gfx::SlyShadowMapType_Destruct)
NATIVE(007AB7CC, sly4::gfx::SlyShrubType_Destruct)
NATIVE(007AB86C, sly4::gfx::SlyTerrainSnowType_Destruct)
NATIVE(007AB8BC, sly4::gfx::SlyTerrainSnowDecalType_Destruct)
NATIVE(007AB90C, sly4::gfx::SlyTextureWarpType_Destruct)
NATIVE(007AB95C, sly4::gfx::SlyWaterType_Destruct)
NATIVE(007AB9AC, sly4::gfx::SlyWater2Type_Destruct)
NATIVE(007AB9FC, sly4::gfx::SlyWater3Type_Destruct)
NATIVE(007AC0BC, sly4::gfx::DebugType_Delete)
NATIVE(007AC270, sly4::gfx::PrimTxrType_Delete)
NATIVE(007AC3DC, sly4::gfx::PrimNoTxrType_Delete)
NATIVE(007AC590, sly4::gfx::SimpleColorTxrType_Delete)
NATIVE(007AC748, sly4::gfx::SimpleColorTxrUVType_Delete)
NATIVE(007AC800, sly4::gfx::TextureWarpType_Delete)
NATIVE(007ACC34, sly4::gfx::ScreenWarpType_Delete)
NATIVE(007ACCF0, sly4::gfx::LineOutlineType_Delete)
NATIVE(007ACDAC, sly4::gfx::OutlineType_Delete)
NATIVE(007ACFA0, sly4::gfx::ParticleMaterialType_Delete)
NATIVE(007AD158, sly4::gfx::GeoParticleType_Delete)
NATIVE(007AD410, sly4::gfx::SprParticleType_Delete)
NATIVE(007AD49C, sly4::gfx::ShadowType_Delete)
NATIVE(007AD558, sly4::gfx::ProjTxrType_Delete)
NATIVE(007AD7E4, sly4::gfx::ProjTxr_DeferredType_Delete)
NATIVE(007AD95C, sly4::gfx::ZPassType_Delete)
NATIVE(007ADA14, sly4::gfx::DiffuseType_Delete)
NATIVE(007AE108, sly4::gfx::DecalType_Delete)
NATIVE(007AE1C4, sly4::gfx::EnvMapType_Delete)
NATIVE(007AE280, sly4::gfx::SelfLightType_Delete)
NATIVE(007AE33C, sly4::gfx::AngleIndexType_Delete)
NATIVE(007AE3F4, sly4::gfx::WaterType_Delete)
NATIVE(007AE4B0, sly4::gfx::SAC_SkinnedType_Delete)
NATIVE(007AE56C, sly4::gfx::SAC_EnvironmentType_Delete)
NATIVE(007AE628, sly4::gfx::SAC_PropType_Delete)
NATIVE(007AE6E4, sly4::gfx::SAC_LitPropType_Delete)
NATIVE(007AE7A0, sly4::gfx::SAC_FastPropType_Delete)
NATIVE(007AE85C, sly4::gfx::SAC_EnvMapType_Delete)
NATIVE(007AE938, sly4::gfx::SAC_PanoramaType_Delete)
NATIVE(007AE9CC, sly4::gfx::SAC_ParticleType_Delete)
NATIVE(007AEA88, sly4::gfx::Sly2UVType_Delete)
NATIVE(007AEB44, sly4::gfx::SlyCausticsType_Delete)
NATIVE(007AEBFC, sly4::gfx::SlyCharacterType_Delete)
NATIVE(007AECB4, sly4::gfx::SlyCharacter2Type_Delete)
NATIVE(007AED6C, sly4::gfx::SlyCharacterEyeType_Delete)
NATIVE(007AEE24, sly4::gfx::SlyCharacterFurType_Delete)
NATIVE(007AEEDC, sly4::gfx::SlyCharacterFurFinType_Delete)
NATIVE(007AEF94, sly4::gfx::SlyCharacterFakeFurType_Delete)
NATIVE(007AF04C, sly4::gfx::SlyCharacterCollectibleType_Delete)
NATIVE(007AF108, sly4::gfx::SlyDecalType_Delete)
NATIVE(007AF1C4, sly4::gfx::SlyDecal4Type_Delete)
NATIVE(007AF280, sly4::gfx::SlyEnvMapType_Delete)
NATIVE(007AF33C, sly4::gfx::SlyHeightFogType_Delete)
NATIVE(007AF3F8, sly4::gfx::SlyMirrorType_Delete)
NATIVE(007AF4B4, sly4::gfx::SlyPanoramaType_Delete)
NATIVE(007AF574, sly4::gfx::SlyProjTxrType_Delete)
NATIVE(007AF630, sly4::gfx::SlyPropType_Delete)
NATIVE(007AF6EC, sly4::gfx::SlyTerrainType_Delete)
NATIVE(007AFB20, sly4::gfx::SlyScreenWarpType_Delete)
NATIVE(007AFBD8, sly4::gfx::SlyShadowMapType_Delete)
NATIVE(007AFC94, sly4::gfx::SlyShrubType_Delete)
NATIVE(007AFD50, sly4::gfx::SlyTerrainSnowType_Delete)
NATIVE(007AFE0C, sly4::gfx::SlyTerrainSnowDecalType_Delete)
NATIVE(007AFEC4, sly4::gfx::SlyTextureWarpType_Delete)
NATIVE(007AFF7C, sly4::gfx::SlyWaterType_Delete)
NATIVE(007B0034, sly4::gfx::SlyWater2Type_Delete)
NATIVE(007B00EC, sly4::gfx::SlyWater3Type_Delete)
