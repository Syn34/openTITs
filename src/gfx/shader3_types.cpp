// The ShaderType slots 0 (Create: makes the type's shaders), 8 (the
// destructor) and 12 (delete). (ShaderType / Shader layouts: draw3_shader.h.)
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

using namespace sly4;

namespace {

namespace s3 = sly4::render::shader3;
namespace ty = sly4::render::shader3::types;

constexpr u32 kNull = 0;

// Appends s to the type's shader list (PtrVector at +12: +12 data, +16
// u16 capacity, +20 u16 count, +22 u16 high-water mark), doubling the
// capacity when full.
void push(PPUContext& ctx, u32 type, u32 s) {
    if (load<u16>(type + ty::kCount) == load<u16>(type + ty::kCapacity)) {
        call(ctx, ty::fn::PtrVector_Reserve, {type + ty::kShaders, (static_cast<u32>(load<u16>(type + ty::kCapacity)) << 1) & 0x1FFFEu});  // rlwinm 1,15,30
    }
    const u32 count = load<u16>(type + ty::kCount);
    store<u32>(load<u32>(type + ty::kShaders) + ((count << 2) & 0x3FFFCu), s);  // rlwinm 2,14,29
    const u32 n = (load<u16>(type + ty::kCount) + 1u) & 0xFFFF;
    const u32 high = load<u16>(type + ty::kHighWater);
    store<u16>(type + ty::kCount, static_cast<u16>(n));
    store<u16>(type + ty::kHighWater, static_cast<u16>(high > n ? high : n));
}

// A new shader with vtable vt and the programs vname / fname, or 0 when
// the allocation fails. The names are read after the allocation, as the
// original does for the table entries.
template <typename Names>
u32 make(PPUContext& ctx, u32 vt, Names names, u32 vparams, u32 fparams) {
    const u32 s = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {ty::kShaderSize, 0, 16}));
    if (s == 0) return 0;
    const auto [vname, fname] = names();
    call(ctx, ty::fn::Shader_Construct, {s});
    store<u8>(s + 12, 0);
    store<u32>(s, vt);
    call(ctx, ty::fn::Shader_Load, {s, vname, fname, vparams, fparams});
    return s;
}

// One shader with fixed names.
void add(PPUContext& ctx, u32 type, u32 vt, u32 vname, u32 fname, u32 vparams, u32 fparams) {
    push(ctx, type, make(ctx, vt, [=] { return std::pair{vname, fname}; }, vparams, fparams));
}

// Entries first..end - 1 of a name table: per entry one shader (+0, +8),
// with `skinned` a second (+4, +8); with `null_check` an entry whose +0
// is 0 adds null entries instead.
void add_table(PPUContext& ctx, u32 type, u32 vt, u32 table, u32 first, u32 end, bool skinned, bool null_check,
               u32 vparams, u32 fparams) {
    for (u32 i = first; i < end; ++i) {  // cmplwi
        const u32 e = table + 12 * i;
        if (null_check && load<u32>(e) == 0) {
            push(ctx, type, 0);
            if (skinned) push(ctx, type, 0);
            continue;
        }
        push(ctx, type, make(ctx, vt, [=] { return std::pair{load<u32>(e), load<u32>(e + 8)}; }, vparams, fparams));
        if (skinned) push(ctx, type, make(ctx, vt, [=] { return std::pair{load<u32>(e + 4), load<u32>(e + 8)}; }, vparams, fparams));
    }
}

}  // namespace

// ---------------------------------------------------------------- Create (ShaderType slot 0)
// void <Type>Type_Create(ShaderType* t)

DECOMP(007ABEF4, DebugType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B24EF8, 0x00B24F30, 0x00B24F30, kNull, 0x00F88AFC);  // "FlatColor", "FlatColor"
    add(ctx, t, 0x00B24EF8, 0x00B24F3A, 0x00B24F30, kNull, 0x00F88AFC);  // "FlatColor_SK", "FlatColor"
}
DECOMP(007AC178, PrimTxrType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B24F58, 0x00B24F8C, 0x00B24F8C, kNull, 0x00F88B14);  // "PrimTxr", "PrimTxr"
}
DECOMP(007AC2E4, PrimNoTxrType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B24F58, 0x00B24FB8, 0x00B24FB8, kNull, 0x00F88B14);  // "PrimNoTxr", "PrimNoTxr"
}
DECOMP(007AC498, SimpleColorTxrType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B24FD8, 0x00B25010, 0x00B25010, kNull, 0x00F88B24);  // "SimpleColorTxr", "SimpleColorTxr"
}
DECOMP(007AC64C, SimpleColorTxrUVType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25038, 0x00B25070, 0x00B25070, 0x00F88B5C, 0x00F88B3C);  // "SimpleColorTxrUV", "SimpleColorTxrUV"
}
DECOMP(005AF080, TextureWarpType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B250D0, 0x00AF8218, kNull, 0x00F88B7C, 0x00F88B6C);  // "TextureWarp", 0
    add(ctx, t, 0x00B250D0, 0x00AF8228, kNull, 0x00F88B7C, 0x00F88B6C);  // "TextureWarp_Lit", 0
}
DECOMP(007AC8B8, ScreenWarpType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B250F0, 0x00B25128, kNull, 0x00F88BD4, 0x00F88BC4);  // "ScreenWarp", 0
    add(ctx, t, 0x00B250F0, 0x00B25133, kNull, 0x00F88BD4, 0x00F88BC4);  // "ScreenWarp_Lit", 0
}
DECOMP(005AFAD4, LineOutlineType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25158, 0x00AF8238, 0x00AF8238, 0x00F88C14, 0x00F88BEC);  // "LineOutline", "LineOutline"
}
DECOMP(005AFF0C, OutlineType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B251A8, 0x00AF8244, 0x00AF8244, 0x00F88C34, 0x00F88C1C);  // "Outline", "Outline"
    add(ctx, t, 0x00B251A8, 0x00AF824C, 0x00AF8244, 0x00F88C34, 0x00F88C1C);  // "Outline_SK", "Outline"
    add(ctx, t, 0x00B251A8, 0x00AF825C, 0x00AF8244, 0x00F88C34, 0x00F88C1C);  // "OutlineNS", "Outline"
    add(ctx, t, 0x00B251A8, 0x00AF826C, 0x00AF8244, 0x00F88C34, 0x00F88C1C);  // "OutlineNS_SK", "Outline"
}
DECOMP(007ACE20, ParticleMaterialType_Create) {}  // no shaders
DECOMP(007AD05C, GeoParticleType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25220, 0x00B25258, 0x00B25258, 0x00F88C5C, 0x00F88C44);  // "GeoParticle", "GeoParticle"
}
DECOMP(007AD210, SprParticleType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25278, 0x00B252AC, kNull, 0x00F88C7C, 0x00F88C6C);  // "SprParticle", 0
    add(ctx, t, 0x00B25278, 0x00B252B8, kNull, 0x00F88C7C, 0x00F88C6C);  // "SprParticle_Soft", 0
}
DECOMP(007AD484, ShadowType_Create) {}  // no shaders
DECOMP(005B155C, ProjTxrType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25310, 0x00AF533C, 0x00AF533C, 0x00F88CAC, 0x00F88C8C);  // "ProjTxr", "ProjTxr"
    add(ctx, t, 0x00B25310, 0x00AF533C, 0x00AF533C, 0x00F88CAC, 0x00F88C8C);  // "ProjTxr", "ProjTxr"
}
DECOMP(007AD614, ProjTxr_DeferredType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25358, 0x00B25390, 0x00B25390, 0x00F88CD4, 0x00F88CBC);  // "ProjTxr_Deferred", "ProjTxr_Deferred"
    add(ctx, t, 0x00B25358, 0x00B25390, 0x00B253A1, 0x00F88CD4, 0x00F88CBC);  // "ProjTxr_Deferred", "ProjTxr_Deferred_NF"
}
DECOMP(005B1C6C, ZPassType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25420, 0x00AF5434, 0x00AF5434, 0x00F88D3C, 0x00F88D2C);  // "ZPass", "ZPass"
    add(ctx, t, 0x00B25420, 0x00AF543C, 0x00AF5434, 0x00F88D3C, 0x00F88D2C);  // "ZPass_SK", "ZPass"
}
DECOMP(005B2940, DiffuseType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25490, 0x00AF545C, 0x00AF545C, 0x00F88D5C, 0x00F88D44);  // "Diffuse", "Diffuse"
    add(ctx, t, 0x00B25490, 0x00AF5464, 0x00AF545C, 0x00F88D5C, 0x00F88D44);  // "Diffuse_SK", "Diffuse"
    add_table(ctx, t, 0x00B25490, 0x00F88D7C, 1, 24, true, true, 0x00F88D5C, 0x00F88D44);
}
DECOMP(007ADAD0, DecalType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B254B0, 0x00B254E4, 0x00B254E4, 0x00F88EB4, 0x00F88E9C);  // "Decal", "Decal"
    add(ctx, t, 0x00B254B0, 0x00B254EA, 0x00B254E4, 0x00F88EB4, 0x00F88E9C);  // "Decal_SK", "Decal"
    add_table(ctx, t, 0x00B254B0, 0x00F88EC4, 1, 8, true, true, 0x00F88EB4, 0x00F88E9C);
}
DECOMP(005B4004, EnvMapType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25508, 0x00AF5764, 0x00AF5764, 0x00F88F4C, 0x00F88F24);  // "EnvMapS", "EnvMapS"
    add(ctx, t, 0x00B25508, 0x00AF576C, 0x00AF5764, 0x00F88F4C, 0x00F88F24);  // "EnvMapS_SK", "EnvMapS"
    add_table(ctx, t, 0x00B25508, 0x00F88F84, 1, 24, true, true, 0x00F88F4C, 0x00F88F24);
}
DECOMP(005B4EC8, SelfLightType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25550, 0x00AF8288, 0x00AF8288, 0x00F890BC, 0x00F890A4);  // "SelfLight_Lit", "SelfLight_Lit"
}
DECOMP(005B56B0, AngleIndexType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B255A0, 0x00AF5964, 0x00AF5964, 0x00F890F4, 0x00F890DC);  // "AngleIndex", "AngleIndex"
    add(ctx, t, 0x00B255A0, 0x00AF5974, 0x00AF5964, 0x00F890F4, 0x00F890DC);  // "AngleIndex_SK", "AngleIndex"
}
DECOMP(005B6424, WaterType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25618, 0x00AF8298, kNull, 0x00F89114, 0x00F8915C);  // "Water", 0
}
DECOMP(005B6F54, SAC_SkinnedType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25638, 0x00AF82A0, 0x00AF82A0, 0x00F8919C, kNull);  // "SAC_Skinned", "SAC_Skinned"
    add(ctx, t, 0x00B25638, 0x00AF82B0, 0x00AF82B0, 0x00F8919C, kNull);  // "SAC_Skinned_EnvMap", "SAC_Skinned_EnvMap"
}
DECOMP(005B7780, SAC_EnvironmentType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25688, 0x00AF82C4, 0x00AF82C4, 0x00F891AC, kNull);  // "SAC_Environment", "SAC_Environment"
}
DECOMP(005B7DEC, SAC_PropType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B256D8, 0x00AF82D4, 0x00AF82D4, 0x00F891BC, kNull);  // "SAC_Prop", "SAC_Prop"
}
DECOMP(005B8518, SAC_LitPropType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25728, 0x00AF82E0, 0x00AF82E0, 0x00F891CC, kNull);  // "SAC_LitProp", "SAC_LitProp"
}
DECOMP(005B8C60, SAC_FastPropType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25778, 0x00AF82EC, 0x00AF82EC, 0x00F891DC, kNull);  // "SAC_FastProp", "SAC_FastProp"
}
DECOMP(005B9924, SAC_EnvMapType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B257C8, 0x00AF82FC, 0x00AF82FC, 0x00F891EC, kNull);  // "SAC_EnvMap", "SAC_EnvMap"
}
// SAC_Panorama: no shaders; notes that the type was created (g::panorama_created).
DECOMP(007AE8D0, SAC_PanoramaType_Create) {
    if (ref<u8>(s3::g::panorama_created) == 0) store<u8>(s3::g::panorama_created, 1);
}
DECOMP(007AE9AC, SAC_ParticleType_Create) {}  // no shaders
DECOMP(005BA1DC, Sly2UVType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25870, 0x00AF5B3C, 0x00AF5B3C, 0x00F8923C, 0x00F89224);  // "Sly2UV", "Sly2UV"
    add(ctx, t, 0x00B25870, 0x00AF5B44, 0x00AF5B3C, 0x00F8923C, 0x00F89224);  // "Sly2UV_UVX", "Sly2UV"
    add_table(ctx, t, 0x00B25870, 0x00F8924C, 1, 5, true, true, 0x00F8923C, 0x00F89224);
}
DECOMP(005BAD8C, SlyCausticsType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B258B8, 0x00AF5C0C, 0x00AF5C0C, 0x00F892A0, 0x00F89288);  // "SlyCaustics", "SlyCaustics"
    add(ctx, t, 0x00B258B8, 0x00AF5C1C, 0x00AF5C0C, 0x00F892A0, 0x00F89288);  // "SlyCaustics_UVX", "SlyCaustics"
    add_table(ctx, t, 0x00B258B8, 0x00F892B0, 1, 8, true, true, 0x00F892A0, 0x00F89288);
}
DECOMP(005BC030, SlyCharacterType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25938, 0x00AF5CE4, 0x00AF5CE4, 0x00F89338, 0x00F89310);  // "SlyCharacter", "SlyCharacter"
    add(ctx, t, 0x00B25938, 0x00AF5CF4, 0x00AF5CE4, 0x00F89338, 0x00F89310);  // "SlyCharacter_SK", "SlyCharacter"
    add_table(ctx, t, 0x00B25938, 0x00F89358, 1, 24, true, true, 0x00F89338, 0x00F89310);
}
DECOMP(005BD7A0, SlyCharacter2Type_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25958, 0x00AF5FBC, 0x00AF5FBC, 0x00F894D0, 0x00F89478);  // "SlyCharacter2", "SlyCharacter2"
    add(ctx, t, 0x00B25958, 0x00AF5FCC, 0x00AF5FBC, 0x00F894D0, 0x00F89478);  // "SlyCharacter2_SK", "SlyCharacter2"
    add_table(ctx, t, 0x00B25958, 0x00F89508, 1, 17, true, true, 0x00F894D0, 0x00F89478);
}
DECOMP(005BF148, SlyCharacterEyeType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B259D8, 0x00AF6234, 0x00AF6234, 0x00F8961C, 0x00F895D4);  // "SlyCharacterEye", "SlyCharacterEye"
    add(ctx, t, 0x00B259D8, 0x00AF6244, 0x00AF6234, 0x00F8961C, 0x00F895D4);  // "SlyCharacterEye_SK", "SlyCharacterEye"
    add_table(ctx, t, 0x00B259D8, 0x00F89644, 1, 6, true, true, 0x00F8961C, 0x00F895D4);
}
DECOMP(005C0C28, SlyCharacterFurType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add_table(ctx, t, 0x00B25A28, 0x00F8971C, 0, 10, true, true, 0x00F89704, 0x00F8968C);
}
DECOMP(005C1A10, SlyCharacterFurFinType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add_table(ctx, t, 0x00B25A80, 0x00F897CC, 0, 5, true, true, 0x00F897C4, 0x00F89794);
}
DECOMP(005C22A8, SlyCharacterFakeFurType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add_table(ctx, t, 0x00B25AD8, 0x00F89898, 0, 13, true, true, 0x00F89858, 0x00F89808);
}
DECOMP(005C320C, SlyCharacterCollectibleType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add_table(ctx, t, 0x00B25B30, 0x00F8997C, 0, 24, true, true, 0x00F8995C, 0x00F89934);
}
DECOMP(005C4120, SlyDecalType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25B50, 0x00AF68EC, 0x00AF68EC, 0x00F89AB4, 0x00F89A9C);  // "SlyDecal", "SlyDecal"
    add(ctx, t, 0x00B25B50, 0x00AF68FC, 0x00AF68EC, 0x00F89AB4, 0x00F89A9C);  // "SlyDecal_UVX", "SlyDecal"
    add_table(ctx, t, 0x00B25B50, 0x00F89AD4, 1, 24, true, true, 0x00F89AB4, 0x00F89A9C);
}
DECOMP(005C5104, SlyDecal4Type_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add_table(ctx, t, 0x00B25BA0, 0x00F89C24, 0, 6, false, true, 0x00F89C1C, 0x00F89BF4);
}
DECOMP(005C6328, SlyEnvMapType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25BF0, 0x00AF6B14, 0x00AF6B14, 0x00F89C94, 0x00F89C6C);  // "SlyEnvMap", "SlyEnvMap"
    add(ctx, t, 0x00B25BF0, 0x00AF6B24, 0x00AF6B14, 0x00F89C94, 0x00F89C6C);  // "SlyEnvMap_UVX", "SlyEnvMap"
    add_table(ctx, t, 0x00B25BF0, 0x00F89CCC, 1, 24, true, true, 0x00F89C94, 0x00F89C6C);
}
DECOMP(005C7B84, SlyHeightFogType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add_table(ctx, t, 0x00B25C40, 0x00F89E7C, 0, 4, true, false, 0x00F89E2C, 0x00F89DEC);
}
DECOMP(005C914C, SlyMirrorType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25C90, 0x00AF6EB4, 0x00AF6EB4, 0x00F89EDC, 0x00F89EAC);  // "SlyMirror", "SlyMirror"
    add(ctx, t, 0x00B25C90, 0x00AF6EC4, 0x00AF6EB4, 0x00F89EDC, 0x00F89EAC);  // "SlyMirror_UVX", "SlyMirror"
    add_table(ctx, t, 0x00B25C90, 0x00F89F1C, 1, 24, true, true, 0x00F89EDC, 0x00F89EAC);
}
DECOMP(005CA368, SlyPanoramaType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25CE0, 0x00AF705C, 0x00AF705C, 0x00F8A054, 0x00F8A03C);  // "SlyPanorama", "SlyPanorama"
    add(ctx, t, 0x00B25CE0, 0x00AF706C, 0x00AF705C, 0x00F8A054, 0x00F8A03C);  // "SlyPanorama_UVX", "SlyPanorama"
    add_table(ctx, t, 0x00B25CE0, 0x00F8A074, 1, 24, true, true, 0x00F8A054, 0x00F8A03C);
}
// SlyProjTxr: the two table entries (+0, +8), each also given fragment
// control 0xFC0 (Shader_SetFpControl).
DECOMP(005CAA98, SlyProjTxrType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    constexpr u32 table = 0x00F8A1FC;
    for (u32 k = 0; k < 2; ++k) {  // cmplwi
        const u32 e = table + 12 * k;
        const u32 s = make(ctx, 0x00B25D30, [=] { return std::pair{load<u32>(e), load<u32>(e + 8)}; }, 0x00F8A1F4, 0x00F8A194);
        if (s != 0) call(ctx, ty::fn::Shader_SetFpControl, {s, 0xFC0});
        push(ctx, t, s);
    }
}
DECOMP(005CB9D8, SlyPropType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25D80, 0x00AF7314, 0x00AF7314, 0x00F8A234, 0x00F8A214);  // "SlyProp", "SlyProp"
    add(ctx, t, 0x00B25D80, 0x00AF731C, 0x00AF7314, 0x00F8A234, 0x00F8A214);  // "SlyProp_UVX", "SlyProp"
    add_table(ctx, t, 0x00B25D80, 0x00F8A254, 1, 24, true, true, 0x00F8A234, 0x00F8A214);
}
DECOMP(005CCDF0, SlyTerrainType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add_table(ctx, t, 0x00B25EC0, 0x00F8A3AC, 0, 16, true, true, 0x00F8A38C, 0x00F8A374);
}
DECOMP(007AF7A4, SlyScreenWarpType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25DC8, 0x00B25E00, kNull, 0x00F8A484, 0x00F8A46C);  // "SlyScreenWarp", 0
    add(ctx, t, 0x00B25DC8, 0x00B25E0E, kNull, 0x00F8A484, 0x00F8A46C);  // "SlyScreenWarp_Lit", 0
}
DECOMP(007AFB94, SlyShadowMapType_Create) {}  // no shaders
DECOMP(005CE264, SlyShrubType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25E70, 0x00AF8318, 0x00AF8318, 0x00F8A4D4, 0x00F8A49C);  // "SlyShrub", "SlyShrub"
}
DECOMP(005CF28C, SlyTerrainSnowType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add_table(ctx, t, 0x00B25F10, 0x00F8A53C, 0, 11, true, true, 0x00F8A514, 0x00F8A4E4);
}
DECOMP(005D0974, SlyTerrainSnowDecalType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add_table(ctx, t, 0x00B25F60, 0x00F8A628, 0, 10, true, true, 0x00F8A5F8, 0x00F8A5C0);
}
DECOMP(005D1EC4, SlyTextureWarpType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add(ctx, t, 0x00B25FE8, 0x00AF8324, kNull, 0x00F8A6E8, 0x00F8A6A0);  // "SlyTextureWarp", 0
    add(ctx, t, 0x00B25FE8, 0x00AF8334, kNull, 0x00F8A6E8, 0x00F8A6A0);  // "SlyTextureWarp_Lit", 0
}
DECOMP(005D2C04, SlyWaterType_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add_table(ctx, t, 0x00B26038, 0x00F8A790, 0, 4, true, true, 0x00F8A738, 0x00F8A700);
}
DECOMP(005D3F88, SlyWater2Type_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add_table(ctx, t, 0x00B26088, 0x00F8A890, 0, 64, false, true, 0x00F8A860, 0x00F8A7C0);
}
DECOMP(005D5A6C, SlyWater3Type_Create) {
    const u32 t = arg<u32>(ctx, 0);
    add_table(ctx, t, 0x00B260D8, 0x00F8AC68, 0, 64, false, true, 0x00F8AC30, 0x00F8AB90);
}

// ---------------------------------------------------------------- destructor (slot 8) and delete (slot 12)

namespace {
// The base ShaderType destructor: base vtable, then the shader list
// cleared (its shaders are not deleted).
void destruct(PPUContext& ctx, u32 t) {
    store<u32>(t, ty::kBaseVtable);
    call(ctx, ty::fn::PtrVector_Clear, {t + ty::kShaders});
    call(ctx, ty::fn::Nop, {t + ty::kShaders});
}
void destroy(PPUContext& ctx, u32 t) {
    if (t == 0) return;
    destruct(ctx, t);
    call(ctx, ty::fn::Mem_FreeFrom, {t, 0});
}
}  // namespace

// void <Type>Type_Destruct(ShaderType* t)
DECOMP(007AA82C, DebugType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AA87C, PrimTxrType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AA8CC, PrimNoTxrType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AA91C, SimpleColorTxrType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AA96C, SimpleColorTxrUVType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AA9BC, TextureWarpType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AAA0C, ScreenWarpType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AAA5C, LineOutlineType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AAAAC, OutlineType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AAAFC, ParticleMaterialType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AAB4C, GeoParticleType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AAB9C, SprParticleType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AABEC, ShadowType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AAC3C, ProjTxrType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AAC8C, ProjTxr_DeferredType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AAD2C, ZPassType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AAD7C, DiffuseType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AADCC, DecalType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AAE1C, EnvMapType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AAE6C, SelfLightType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AAEBC, AngleIndexType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AAF0C, WaterType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AAF5C, SAC_SkinnedType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AAFAC, SAC_EnvironmentType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AAFFC, SAC_PropType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB04C, SAC_LitPropType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB09C, SAC_FastPropType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB0EC, SAC_EnvMapType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB13C, SAC_PanoramaType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB18C, SAC_ParticleType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB1DC, Sly2UVType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB22C, SlyCausticsType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB27C, SlyCharacterType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB2CC, SlyCharacter2Type_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB31C, SlyCharacterEyeType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB36C, SlyCharacterFurType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB3BC, SlyCharacterFurFinType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB40C, SlyCharacterFakeFurType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB45C, SlyCharacterCollectibleType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB4AC, SlyDecalType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB4FC, SlyDecal4Type_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB54C, SlyEnvMapType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB59C, SlyHeightFogType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB5EC, SlyMirrorType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB63C, SlyPanoramaType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB68C, SlyProjTxrType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB6DC, SlyPropType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB81C, SlyTerrainType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB72C, SlyScreenWarpType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB77C, SlyShadowMapType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB7CC, SlyShrubType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB86C, SlyTerrainSnowType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB8BC, SlyTerrainSnowDecalType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB90C, SlyTextureWarpType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB95C, SlyWaterType_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB9AC, SlyWater2Type_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AB9FC, SlyWater3Type_Destruct) { destruct(ctx, arg<u32>(ctx, 0)); }

// void <Type>Type_Delete(ShaderType* t)
DECOMP(007AC0BC, DebugType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AC270, PrimTxrType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AC3DC, PrimNoTxrType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AC590, SimpleColorTxrType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AC748, SimpleColorTxrUVType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AC800, TextureWarpType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007ACC34, ScreenWarpType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007ACCF0, LineOutlineType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007ACDAC, OutlineType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007ACFA0, ParticleMaterialType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AD158, GeoParticleType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AD410, SprParticleType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AD49C, ShadowType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AD558, ProjTxrType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AD7E4, ProjTxr_DeferredType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AD95C, ZPassType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007ADA14, DiffuseType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AE108, DecalType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AE1C4, EnvMapType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AE280, SelfLightType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AE33C, AngleIndexType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AE3F4, WaterType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AE4B0, SAC_SkinnedType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AE56C, SAC_EnvironmentType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AE628, SAC_PropType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AE6E4, SAC_LitPropType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AE7A0, SAC_FastPropType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AE85C, SAC_EnvMapType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AE938, SAC_PanoramaType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AE9CC, SAC_ParticleType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AEA88, Sly2UVType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AEB44, SlyCausticsType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AEBFC, SlyCharacterType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AECB4, SlyCharacter2Type_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AED6C, SlyCharacterEyeType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AEE24, SlyCharacterFurType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AEEDC, SlyCharacterFurFinType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AEF94, SlyCharacterFakeFurType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AF04C, SlyCharacterCollectibleType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AF108, SlyDecalType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AF1C4, SlyDecal4Type_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AF280, SlyEnvMapType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AF33C, SlyHeightFogType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AF3F8, SlyMirrorType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AF4B4, SlyPanoramaType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AF574, SlyProjTxrType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AF630, SlyPropType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AF6EC, SlyTerrainType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AFB20, SlyScreenWarpType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AFBD8, SlyShadowMapType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AFC94, SlyShrubType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AFD50, SlyTerrainSnowType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AFE0C, SlyTerrainSnowDecalType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AFEC4, SlyTextureWarpType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007AFF7C, SlyWaterType_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007B0034, SlyWater2Type_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
DECOMP(007B00EC, SlyWater3Type_Delete) { destroy(ctx, arg<u32>(ctx, 0)); }
