// Shader groups: Shader_LoadGroup builds (once) the material for one of six
// built-in groups from a shader found by name hash; Shader_FindByHash
// searches the shader registry, falling back for four known hashes;
// Material_Alloc takes an AglMaterial from its free list.
#include "shadergroup.h"

using namespace sly4;

namespace {

namespace hfn = sly4::shgroup::fn;
namespace hg = sly4::shgroup::g;

u32 find_shader(PPUContext& ctx, u32 hash) {
    return static_cast<u32>(call(ctx, hfn::Shader_FindByHash, {hash}));
}

// The registry vector, made on first use.
u32 registry(PPUContext& ctx) {
    if (ref<u8>(hg::registry_made) != 0) return load<u32>(hg::registry);
    const u32 v = static_cast<u32>(call(ctx, fn::Mem_Alloc, {20, 0, 16}));
    if (v != 0) {
        call(ctx, hfn::PtrVector_Construct, {v});
        store<u16>(v + 6, 4);
        call(ctx, hfn::PtrVector_Init, {v, 512, 0, 0});
    }
    store<u32>(hg::registry, v);
    ref<u8>(hg::registry_made) = 1;
    return v;
}

}  // namespace

// AglMaterial* Shader_LoadGroup(s32 group)
// Groups 0..5: 0/1 shader 0x03DE9E33, 2 0x05199F6B, 3 0x38FF7607, 4/5
// 0x1808E1E8 with texture 0 / 1 of 0x010DDADC in slot 0. The material's
// render state at +104 gets fixed blend words and +116 = 0x900 (0 for
// group 1).
DECOMP(0050F0F0, Shader_LoadGroup) {
    const s32 group = arg<s32>(ctx, 0);
    const u32 slot = hg::groups + static_cast<u32>(group * 4);
    if (const u32 cached = load<u32>(slot); cached != 0) {
        ctx.gpr[3] = cached;
        return;
    }
    u32 texture = 0;
    u32 state = 0x900;
    u32 shader;
    switch (group) {
    case 5:
        shader = find_shader(ctx, 0x1808E1E8);
        texture = load<u32>(hg::group_textures + 4);
        break;
    case 4:
        shader = find_shader(ctx, 0x1808E1E8);
        texture = load<u32>(hg::group_textures);
        break;
    case 3: shader = find_shader(ctx, 0x38FF7607); break;
    case 2: shader = find_shader(ctx, 0x05199F6B); break;
    case 1:
        shader = find_shader(ctx, 0x03DE9E33);
        state = 0;
        break;
    default: shader = find_shader(ctx, 0x03DE9E33); break;
    }
    // r3 going into Material_Alloc is 0x010DDADC for group 5; it reads no argument.
    const u32 m = static_cast<u32>(call(ctx, hfn::Material_Alloc));
    store<u32>(m + 16, shader);
    store<u32>(slot, m);
    if (texture != 0) {
        const u32 count = ref<u8>(m + 44) + 1u;
        const s32 last = static_cast<s32>(count & 0xFF) - 1;
        ref<u8>(m + 44) = static_cast<u8>(count);
        const u32 entry = m + 20 + static_cast<u32>(last * 4);
        const u32 old = load<u32>(entry);
        if (old != texture) {
            store<u32>(entry, texture);
            store<u16>(texture + 4, static_cast<u16>(load<u16>(texture + 4) + 1));
            if (old != 0) call_ptr(ctx, load<u32>(load<u32>(old) + 8), {old});  // release
        }
        if (last == 0) store<u16>(m + 48, static_cast<u16>(load<u32>(texture + 112)));
    }
    const u32 rs = load<u32>(slot) + 104;
    const u32 w0 = load<u32>(rs);
    store<u16>(rs + 4, 0x8006);
    store<u16>(rs + 6, 771);
    store<u32>(rs, (w0 & 0x00007FFF) | 0x80000000 | 0x01810000);
    store<u16>(load<u32>(slot) + 116, static_cast<u16>(state));
    ctx.gpr[3] = load<u32>(slot);
}

// Shader* Shader_FindByHash(u32 hash)
// Linear search of the shader registry by +8; unknown hashes 0xCD407FA6
// and 0x3C7C5C8B fall back to 0xD08FCDAD, 0xDECB5640 to 0x91B76D20 and
// 0x7CE30EBD to 0x8DB50B42. 0 if not found.
DECOMP(004FE990, Shader_FindByHash) {
    const u32 hash = arg<u32>(ctx, 0);
    const u32 v = registry(ctx);
    for (u32 i = 0; i < load<u16>(v + 8); ++i) {
        const u32 s = load<u32>(load<u32>(v) + 4 * i);
        if (load<u32>(s + 8) == hash) {
            ctx.gpr[3] = s;
            return;
        }
    }
    u32 fallback;
    switch (hash) {
    case 0xCD407FA6: fallback = 0xD08FCDAD; break;
    case 0x3C7C5C8B: fallback = 0xD08FCDAD; break;
    case 0xDECB5640: fallback = 0x91B76D20; break;
    case 0x7CE30EBD: fallback = 0x8DB50B42; break;
    default: ctx.gpr[3] = 0; return;
    }
    ctx.gpr[3] = call(ctx, hfn::Shader_FindByHash, {fallback});
}

// AglMaterial* Material_Alloc()
// Takes a 144-byte AglMaterial from the free list at 0x00F8F114 (making
// the list's allocator on first use) and constructs it; 0 when the list
// header is missing or the list is empty.
DECOMP(004FF2E8, Material_Alloc) {
    const u32 list = load<u32>(hg::material_list);
    if (list == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    u32 alloc = load<u32>(list);
    if (alloc == 0) {
        const u32 obj = static_cast<u32>(call(ctx, fn::Mem_Alloc, {60, 0, 16}));
        if (obj != 0) {
            const u32 capacity = load<u32>(list + 4);
            for (u32 off : {8u, 4u, 12u, 20u, 16u}) store<u32>(obj + off, 0);
            const u16 h54 = load<u16>(obj + 54);
            for (u32 off : {24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(obj + off, 0);
            store<u16>(obj + 52, 0);
            store<u16>(obj + 54, static_cast<u16>(h54 & 0x7FFF));
            store<u32>(obj, hg::vt_material_list);
            store<u32>(obj + 56, hg::str_material_list);
            call(ctx, hfn::FreeList_Init, {obj, 144, capacity, 0, 0});
        }
        alloc = obj;  // 0 if Mem_Alloc failed: the original then reads guest address 0
        store<u32>(list, alloc);
    }
    const u32 m = static_cast<u32>(call_ptr(ctx, load<u32>(load<u32>(alloc) + 12), {alloc}));
    if (m == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, hfn::Material_Construct, {m});
    store<u32>(m, hg::vt_material);
    ctx.gpr[3] = m;
}
