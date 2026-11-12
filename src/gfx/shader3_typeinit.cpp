// The ShaderType statics' constructor and destructor (run from the
// static initialiser / exit lists), with the static matrices the
// constructor sets up between them.
#include "shader3.h"
#include "rsx.h"

#include <iterator>

namespace sly4::gfx {

namespace {

namespace ty = sly4::render::shader3::types;

struct TypeInit {
    u32 obj, name, vtable;
};

// The ShaderType statics in construction order: {object, name, vtable}.
constexpr TypeInit kTypes[] = {
    {0x01196A80, 0x00B24F47, 0x00B24F18},  // "Debug"
    {0x01196AA0, 0x00B24F94, 0x00B24F78},  // "PrimTxr"
    {0x01196AC0, 0x00B24FC2, 0x00B24FA4},  // "PrimNoTxr"
    {0x01196AE0, 0x00B2501F, 0x00B24FF8},  // "SimpleColorTxr"
    {0x01196B00, 0x00B25081, 0x00B25058},  // "SimpleColorTxrUV"
    {0x01196B20, 0x00B250B8, 0x00B250A0},  // "TextureWarp"
    {0x01196B40, 0x00B25142, 0x00B25110},  // "ScreenWarp"
    {0x01196B60, 0x00B25190, 0x00B25178},  // "LineOutline"
    {0x01196B80, 0x00B251E0, 0x00B251C8},  // "Outline"
    {0x01196BA0, 0x00B25204, 0x00B251F0},  // "ParticleMaterial"
    {0x01196BC0, 0x00B25264, 0x00B25240},  // "GeoParticle"
    {0x01196BE0, 0x00B252C9, 0x00B25298},  // "SprParticle"
    {0x01196C00, 0x00B252FC, 0x00B252E8},  // "Shadow"
    {0x01196C20, 0x00B25348, 0x00B25330},  // "ProjTxr"
    {0x01196C40, 0x00B253B5, 0x00B25378},  // "ProjTxr_Deferred"
    {0x01196C60, 0x00B25408, 0x00B253F0},  // "FluidSim"
    {0x01196C80, 0x00B25458, 0x00B25440},  // "ZPass"
    {0x01196CA0, 0x00B25480, 0x00B25468},  // "Diffuse"
    {0x01196CC0, 0x00B254F3, 0x00B254D0},  // "Decal"
    {0x01196CE0, 0x00B25540, 0x00B25528},  // "EnvMap"
    {0x01196D00, 0x00B25588, 0x00B25570},  // "SelfLight"
    {0x01196D20, 0x00B255D8, 0x00B255C0},  // "AngleIndex"
    {0x01196D40, 0x00B25608, 0x00B255F0},  // "Water"
    {0x01196D60, 0x00B25670, 0x00B25658},  // "SAC_Skinned"
    {0x01196D80, 0x00B256C0, 0x00B256A8},  // "SAC_Environment"
    {0x01196DA0, 0x00B25710, 0x00B256F8},  // "SAC_Prop"
    {0x01196DC0, 0x00B25760, 0x00B25748},  // "SAC_LitProp"
    {0x01196DE0, 0x00B257B0, 0x00B25798},  // "SAC_FastProp"
    {0x01196E00, 0x00B25800, 0x00B257E8},  // "SAC_EnvMap"
    {0x01196E20, 0x00B25828, 0x00B25814},  // "SAC_Panorama"
    {0x01196E40, 0x00B25854, 0x00B25840},  // "SAC_Particle"
    {0x01196E60, 0x00B258A8, 0x00B25890},  // "Sly2UV"
    {0x01196E80, 0x00B258F0, 0x00B258D8},  // "SlyCaustics"
    {0x01196EA0, 0x00B25920, 0x00B25908},  // "SlyCharacter"
    {0x01196EC0, 0x00B25990, 0x00B25978},  // "SlyCharacter2"
    {0x01196EE0, 0x00B259C0, 0x00B259A8},  // "SlyCharacterEye"
    {0x01196F00, 0x00B25A10, 0x00B259F8},  // "SlyCharacterFur"
    {0x01196F20, 0x00B25A60, 0x00B25A48},  // "SlyCharacterFurFin"
    {0x01196F40, 0x00B25AB8, 0x00B25AA0},  // "SlyCharacterFakeFur"
    {0x01196F60, 0x00B25B10, 0x00B25AF8},  // "SlyCharacterCollectible"
    {0x01196FB0, 0x00B25B88, 0x00B25B70},  // "SlyDecal"
    {0x01196FD0, 0x00B25BD8, 0x00B25BC0},  // "SlyDecal4"
    {0x01196FF0, 0x00B25C28, 0x00B25C10},  // "SlyEnvMap"
    {0x01197010, 0x00B25C78, 0x00B25C60},  // "SlyHeightFog"
    {0x01197030, 0x00B25CC8, 0x00B25CB0},  // "SlyMirror"
    {0x01197050, 0x00B25D18, 0x00B25D00},  // "SlyPanorama"
    {0x01197070, 0x00B25D68, 0x00B25D50},  // "SlyProjTxr"
    {0x01197090, 0x00B25DB8, 0x00B25DA0},  // "SlyProp"
    {0x011970D0, 0x00B25E20, 0x00B25DE8},  // "SlyScreenWarp"
    {0x011970F0, 0x00B25E5A, 0x00B25E40},  // "SlyShadowMap"
    {0x01197110, 0x00B25EA8, 0x00B25E90},  // "SlyShrub"
    {0x011970B0, 0x00B25EF8, 0x00B25EE0},  // "SlyTerrain"
    {0x01197130, 0x00B25F48, 0x00B25F30},  // "SlyTerrainSnow"
    {0x01197150, 0x00B25F98, 0x00B25F80},  // "SlyTerrainSnowDecal"
    {0x01197170, 0x00B25FD0, 0x00B25FB8},  // "SlyTextureWarp"
    {0x01197190, 0x00B26020, 0x00B26008},  // "SlyWater"
    {0x011971B0, 0x00B26070, 0x00B26058},  // "SlyWater2"
    {0x011971D0, 0x00B260C0, 0x00B260A8},  // "SlyWater3"
    {0x011971F0, 0x00B26130, 0x00B26118},  // "SHO_Default"
    {0x01197210, 0x00B26160, 0x00B26148},  // "SHO_TextureWarp"
    {0x01197230, 0x00B261D0, 0x00B261B8},  // "SHO_NightBlend"
};

// Static data the constructor fills in between the types (after the
// type it follows in kTypes): the projected-texture bias matrices
// (texture space = 0.5 * clip + 0.5, y flipped) and a colour block.
constexpr u32 kBias1[] = {0x3F000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xBF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3F000000, 0x00000000, 0x3F000000, 0x3F000000, 0x3F000000, 0x3F800000};
constexpr u32 kBias2[] = {0x3F000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xBF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3F000000, 0x00000000, 0x3F000000, 0x3F000000, 0x3F000000, 0x3F800000};
constexpr u32 kColours[] = {0x3F000000, 0x3F000000, 0x3F000000, 0x3F800000, 0x3F19999A, 0x3F19999A, 0x3F19999A, 0x3F800000, 0x00000000, 0x00000000, 0x3F800000};
struct DataInit {
    u32 after, addr;  // after kTypes[after - 1]
    const u32* words;
    u32 count;
};
constexpr DataInit kData[] = {
    {13, 0x01196A00, kBias1, 16},
    {35, 0x01196A40, kBias2, 16},
    {40, 0x01196F80, kColours, 11},
};

// The ShaderType statics in destruction order.
constexpr u32 kDestroyOrder[] = {
    0x01197230, 0x01197210, 0x011971F0, 0x011971D0, 0x011971B0, 0x01197190,
    0x01197170, 0x01197150, 0x01197130, 0x011970B0, 0x01197110, 0x011970F0,
    0x011970D0, 0x01197090, 0x01197070, 0x01197050, 0x01197030, 0x01197010,
    0x01196FF0, 0x01196FD0, 0x01196FB0, 0x01196F60, 0x01196F40, 0x01196F20,
    0x01196F00, 0x01196EE0, 0x01196EC0, 0x01196EA0, 0x01196E80, 0x01196E60,
    0x01196E40, 0x01196E20, 0x01196E00, 0x01196DE0, 0x01196DC0, 0x01196DA0,
    0x01196D80, 0x01196D60, 0x01196D40, 0x01196D20, 0x01196D00, 0x01196CE0,
    0x01196CC0, 0x01196CA0, 0x01196C80, 0x01196C60, 0x01196C40, 0x01196C20,
    0x01196C00, 0x01196BE0, 0x01196BC0, 0x01196BA0, 0x01196B80, 0x01196B60,
    0x01196B40, 0x01196B20, 0x01196B00, 0x01196AE0, 0x01196AC0, 0x01196AA0,
    0x01196A80,
};

// ShaderType::ShaderType(name), then the derived vtable: +0 base vtable,
// +4 name, +8 String_HashNoCase(name), +12 PtrVector (+18 u16 = 4,
// initial capacity 2).
void construct(const TypeInit& t) {
    ShaderType* obj = host<ShaderType>(t.obj);
    obj->vtable = ty::kBaseVtable;
    obj->name = host<const char>(t.name);
    obj->hash = call<u32>(sly4::fn::String_HashNoCase, t.name);
    call(ty::fn::PtrVector_Construct, &obj->shaders);
    *at<be_u16>(obj, 18) = 4;
    container::PtrVector_Init(reinterpret_cast<PtrVector*>(&obj->shaders), 2, nullptr, 0);
    obj->vtable = t.vtable;
}
}  // namespace

// void ShaderTypes_Construct()
void ShaderTypes_Construct() {
    u32 next = 0;
    for (u32 i = 0; i < std::size(kTypes); ++i) {
        construct(kTypes[i]);
        if (next < std::size(kData) && kData[next].after == i + 1) {
            const DataInit& d = kData[next++];
            be_u32* dst = host<be_u32>(d.addr);
            for (u32 k = 0; k < d.count; ++k) dst[k] = d.words[k];
        }
    }
}

// void ShaderTypes_Destruct(): each static's base destructor (base
// vtable, shader list cleared).
void ShaderTypes_Destruct() {
    for (const u32 ea : kDestroyOrder) {
        ShaderType* t = host<ShaderType>(ea);
        t->vtable = ty::kBaseVtable;
        call(ty::fn::PtrVector_Clear, &t->shaders);
        call(ty::fn::Nop, &t->shaders);
    }
}

}  // namespace sly4::gfx

NATIVE(005D9810, sly4::gfx::ShaderTypes_Construct)
NATIVE(005D8E50, sly4::gfx::ShaderTypes_Destruct)
