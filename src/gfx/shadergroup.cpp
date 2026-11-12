// Shader groups: Shader_LoadGroup builds (once) the material for one of six
// built-in groups from a shader found by name hash; Shader_FindByHash
// searches the shader registry, falling back for four known hashes;
// Material_Alloc takes an AglMaterial from its free list.
#include "shadergroup.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

namespace hfn = sly4::shgroup::fn;
namespace hg = sly4::shgroup::g;

// The AglMaterial free-list header: the list object, made on first use.
struct MaterialListHeader {
    gp<container::FreeList> list;  // +0
    be_u32 capacity;               // +4
};

// The registry vector, made on first use.
PtrVector* registry() {
    if (global<u8>(hg::registry_made) != 0) return global<gp<PtrVector>>(hg::registry);
    PtrVector* v = call<PtrVector*>(sly4::fn::Mem_Alloc, 20u, 0u, 16u);
    if (v) {
        call(hfn::PtrVector_Construct, v);
        v->elem_size = 4;
        container::PtrVector_Init(v, 512, nullptr, 0);
    }
    global<gp<PtrVector>>(hg::registry) = v;
    global<u8>(hg::registry_made) = 1;
    return v;
}

}  // namespace

// Material* Shader_LoadGroup(s32 group)
// Groups 0..5: 0/1 shader 0x03DE9E33, 2 0x05199F6B, 3 0x38FF7607, 4/5
// 0x1808E1E8 with texture 0 / 1 of 0x010DDADC in slot 0. The material's
// render state at +104 gets fixed blend words and +116 = 0x900 (0 for
// group 1).
Material* Shader_LoadGroup(s32 group) {
    gp<Material>* slot = at<gp<Material>>(host<u8>(hg::groups), static_cast<u32>(group * 4));
    if (Material* cached = *slot) return cached;
    u8* texture = nullptr;
    u32 state = 0x900;
    Shader* shader;
    const gp<u8>* textures = host<gp<u8>>(hg::group_textures);
    switch (group) {
    case 5:
        shader = Shader_FindByHash(0x1808E1E8);
        texture = textures[1];
        break;
    case 4:
        shader = Shader_FindByHash(0x1808E1E8);
        texture = textures[0];
        break;
    case 3: shader = Shader_FindByHash(0x38FF7607); break;
    case 2: shader = Shader_FindByHash(0x05199F6B); break;
    case 1:
        shader = Shader_FindByHash(0x03DE9E33);
        state = 0;
        break;
    default: shader = Shader_FindByHash(0x03DE9E33); break;
    }
    Material* m = or_zero_page(Material_Alloc());  // not checked by the original
    m->w16 = guest(shader);
    *slot = m;
    if (texture) {
        const u32 count = m->b44 + 1u;
        const s32 last = static_cast<s32>(count & 0xFF) - 1;
        m->b44 = static_cast<u8>(count);
        gp<u8>* entry = at<gp<u8>>(m, 20 + static_cast<u32>(last * 4));
        u8* old = *entry;
        if (old != texture) {
            *entry = texture;
            be_u16& refs = *at<be_u16>(texture, 4);
            refs = static_cast<u16>(refs + 1);
            if (old) vcall(old, 8);  // release
        }
        if (last == 0) *at<be_u16>(m, 48) = static_cast<u16>(*at<be_u32>(texture, 112));
    }
    Material* mm = or_zero_page(slot->get());
    const u32 w0 = mm->blend;
    mm->blend_eq = 0x8006;
    mm->blend_dst = 771;
    mm->blend = (w0 & 0x00007FFF) | 0x80000000 | 0x01810000;
    or_zero_page(slot->get())->state = static_cast<u16>(state);
    return *slot;
}

// Shader* Shader_FindByHash(u32 hash)
// Linear search of the shader registry by +8; unknown hashes 0xCD407FA6
// and 0x3C7C5C8B fall back to 0xD08FCDAD, 0xDECB5640 to 0x91B76D20 and
// 0x7CE30EBD to 0x8DB50B42. 0 if not found.
Shader* Shader_FindByHash(u32 hash) {
    PtrVector* v = or_zero_page(registry());
    const be_u32* data = reinterpret_cast<const be_u32*>(or_zero_page(v->data.get()));
    for (u32 i = 0; i < v->count; ++i) {
        Shader* s = host<Shader>(data[i]);
        if (or_zero_page(s)->hash == hash) return s;
    }
    u32 fallback;
    switch (hash) {
    case 0xCD407FA6: fallback = 0xD08FCDAD; break;
    case 0x3C7C5C8B: fallback = 0xD08FCDAD; break;
    case 0xDECB5640: fallback = 0x91B76D20; break;
    case 0x7CE30EBD: fallback = 0x8DB50B42; break;
    default: return nullptr;
    }
    return Shader_FindByHash(fallback);
}

// Material* Material_Alloc()
// Takes a 144-byte AglMaterial from the free list at 0x00F8F114 (making
// the list's allocator on first use) and constructs it; 0 when the list
// header is missing or the list is empty.
Material* Material_Alloc() {
    MaterialListHeader* header = global<gp<MaterialListHeader>>(hg::material_list);
    if (!header) return nullptr;
    container::FreeList* list = header->list;
    if (!list) {
        container::FreeList* obj = call<container::FreeList*>(sly4::fn::Mem_Alloc, 60u, 0u, 16u);
        if (obj) {
            const u32 capacity = header->capacity;
            obj->used_tail = nullptr;
            obj->used_head = nullptr;
            obj->used_count = 0u;
            obj->free_tail = nullptr;
            obj->free_head = nullptr;
            const u16 h54 = obj->flags;
            obj->free_count = 0u;
            obj->block_head = nullptr;
            obj->block_tail = nullptr;
            obj->block_count = 0u;
            obj->heap = nullptr;
            obj->elem_size = 0u;
            obj->used_peak = 0u;
            obj->items_per_block = 0;
            obj->flags = static_cast<u16>(h54 & 0x7FFF);
            obj->vtable = hg::vt_material_list;
            obj->name = hg::str_material_list;
            call(hfn::FreeList_Init, obj, 144u, capacity, 0u, 0u);
        }
        list = obj;
        header->list = list;
    }
    // A failed Mem_Alloc leaves 0 here: the original then reads guest address 0.
    Material* m = vcall<Material*>(or_zero_page(list), 12);
    if (!m) return nullptr;
    Material_Construct(m);
    m->vtable = hg::vt_material;
    return m;
}

}  // namespace sly4::gfx

NATIVE(0050F0F0, sly4::gfx::Shader_LoadGroup)
NATIVE(004FE990, sly4::gfx::Shader_FindByHash)
NATIVE(004FF2E8, sly4::gfx::Material_Alloc)
