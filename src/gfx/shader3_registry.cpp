// The shader registry's set-up: every ShaderType makes its shaders and is
// added to the registry.
#include "shader3.h"
#include "shadergroup.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

namespace s3 = sly4::render::shader3;
namespace hg = sly4::shgroup::g;

// The registry, made on first use: Mem_Alloc(20, 0, 16), constructed,
// +6 = 4, PtrVector_Init(512, 0, 0). A failed allocation leaves a null
// registry (and the push below then writes through it, as in the
// original).
PtrVector* registry() {
    if (global<u8>(hg::registry_made) != 0) return global<gp<PtrVector>>(hg::registry);
    PtrVector* r = call<PtrVector*>(sly4::fn::Mem_Alloc, 20u, 0u, 16u);
    if (r) {
        call(sly4::shgroup::fn::PtrVector_Construct, r);
        r->elem_size = 4;
        container::PtrVector_Init(r, 512, nullptr, 0);
    }
    global<gp<PtrVector>>(hg::registry) = r;
    global<u8>(hg::registry_made) = 1;
    return r;
}

// Type t makes its shaders (slot 0, Create) and joins the registry.
void register_type(ShaderType* t) {
    vcall(t, 0);
    PtrVector* r = or_zero_page(registry());
    if (r->count == r->capacity)
        container::PtrVector_Reserve(r, (static_cast<u32>(r->capacity) << 1) & 0xFFFFFFFEu);  // rlwinm 1,0,30
    const u32 count = r->count;
    const u32 n = r->count + 1u;
    const u32 high = r->high_water;
    *at<gp<ShaderType>>(r->data.get(), count << 2) = t;
    r->count = static_cast<u16>(n);
    r->high_water = static_cast<u16>(static_cast<s32>(high) > static_cast<s32>(n & 0xFFFF) ? high : n);  // cmpw / bgt
    be_u32& flags = *at<be_u32>(t, s3::kTypeFlags);
    flags = flags | s3::kTypeRegistered;
}

void register_table(u32 table) {
    for (const gp<ShaderType>* p = host<gp<ShaderType>>(table); *p; ++p) register_type(*p);
}

}  // namespace

// void Shader_RegisterAll()
// Notes the group texture cursor, then registers the ShaderTypes of the
// first table and, unless g::config +156 / +157 / +158 says otherwise,
// of the other three.
void Shader_RegisterAll() {
    global<be_u32>(s3::g::group_texture_cursor) = global<be_u32>(hg::group_textures + 8) + 56;
    register_table(s3::g::type_tables[0]);
    for (u32 k = 1; k < 4; ++k)
        if (config_u8(155 + k) == 0) register_table(s3::g::type_tables[k]);
}

}  // namespace sly4::gfx

NATIVE(0050B138, sly4::gfx::Shader_RegisterAll)
