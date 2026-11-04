// The shader registry's set-up: every ShaderType makes its shaders and is
// added to the registry.
#include "shader3.h"

#include "shadergroup.h"

using namespace sly4;

namespace {

namespace s3 = sly4::render::shader3;
namespace ty = sly4::render::shader3::types;

// The registry, made on first use: Mem_Alloc(20, 0, 16), constructed,
// +6 = 4, PtrVector_Init(512, 0, 0). A failed allocation leaves a null
// registry (and the push below then writes through it, as in the
// original).
u32 registry(PPUContext& ctx) {
    if (ref<u8>(shgroup::g::registry_made) != 0) return load<u32>(shgroup::g::registry);
    u32 r = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {20, 0, 16}));
    if (r != 0) {
        call(ctx, shgroup::fn::PtrVector_Construct, {r});
        store<u16>(r + 6, 4);
        call(ctx, shgroup::fn::PtrVector_Init, {r, 512, 0, 0});
    }
    store<u32>(shgroup::g::registry, r);
    store<u8>(shgroup::g::registry_made, 1);
    return r;
}

// Type t makes its shaders (slot 0, Create) and joins the registry.
void register_type(PPUContext& ctx, u32 t) {
    fx::vcall(ctx, t, 0, {t});
    const u32 r = registry(ctx);
    if (load<u16>(r + s3::kRegCount) == load<u16>(r + s3::kRegCapacity)) {
        call(ctx, ty::fn::PtrVector_Reserve, {r, (static_cast<u32>(load<u16>(r + s3::kRegCapacity)) << 1) & 0xFFFFFFFEu});  // rlwinm 1,0,30
    }
    const u32 count = load<u16>(r + s3::kRegCount);
    const u32 n = load<u16>(r + s3::kRegCount) + 1u;
    const u32 high = load<u16>(r + s3::kRegHighWater);
    store<u32>(load<u32>(r + s3::kRegData) + (count << 2), t);
    store<u16>(r + s3::kRegCount, static_cast<u16>(n));
    store<u16>(r + s3::kRegHighWater, static_cast<u16>(static_cast<s32>(high) > static_cast<s32>(n & 0xFFFF) ? high : n));  // cmpw / bgt
    store<u32>(t + s3::kTypeFlags, load<u32>(t + s3::kTypeFlags) | s3::kTypeRegistered);
}

void register_table(PPUContext& ctx, u32 table) {
    for (u32 p = table; load<u32>(p) != 0; p += 4) register_type(ctx, load<u32>(p));
}

}  // namespace

// bool Shader_RegisterAll()
// Notes the group texture cursor, then registers the ShaderTypes of the
// first table and, unless g::config +156 / +157 / +158 says otherwise,
// of the other three. Returns true.
DECOMP(0050B138, Shader_RegisterAll) {
    store<u32>(s3::g::group_texture_cursor, load<u32>(shgroup::g::group_textures + 8) + 56);
    register_table(ctx, s3::g::type_tables[0]);
    for (u32 k = 1; k < 4; ++k) {
        if (ref<u8>(sly4::g::config + 155 + k) == 0) register_table(ctx, s3::g::type_tables[k]);
    }
    ret(ctx, 1u);
}
