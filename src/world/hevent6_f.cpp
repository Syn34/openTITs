// World_HandleEvent callees, part 6: the overlay effect restart and the
// sub-mesh builder (159..165 instructions).
#include "hevent6.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent6;

namespace sly4 {
namespace hevent6f {
namespace ffn {
constexpr u32 Set_Find = 0x004BCE90;            // (u32 name) -> Set*
constexpr u32 Effect_Create4A5E58 = 0x004A5E58; // (def, 0, 0, 1) -> Effect* (name?)
constexpr u32 Buffer_Alloc52F198 = 0x0052F198;  // (Buffer*, u32 bytes, 128, 1, 2) (name?)
constexpr u32 SubMesh_Construct5386DC = 0x005386DC;  // (SubMesh*) (name?)
constexpr u32 Mem_Alloc = 0x0054ED38;           // (size, pool, align)
constexpr u32 FreeList_Init = 0x00551A58;       // (list, elem_size, count, heap, bool)
}  // namespace ffn

constexpr u32 kOverlayEffect = 0x00F64A6C;      // u32 Effect* (name?)
constexpr u32 kOverlaySet = 0x7719A548;         // set name hash (as hevent_b.cpp)
constexpr u32 kEffectFlags = 136, kEffectAutoStart = 0x200;  // as hevent5_c.cpp

// The AglSubMesh free list (as Texture_Alloc's): holder {+0 list, +4 count}.
constexpr u32 kSubMeshPool = 0x00F8F120;
constexpr u32 kSubMeshListVtbl = 0x00B23DE0;
constexpr u32 kStrSubMeshList = 0x00AF127D;     // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = AglSubMesh]"
constexpr u32 kSubMeshSize = 116;
constexpr u32 kPrimTypes = 0x00F66F1C;          // u32[] by primitive type (name?)
}  // namespace hevent6f
}  // namespace sly4

using namespace sly4::hevent6f;

// void Overlay_Clear2AA9C8(u32 name) (name?): stops the overlay effect
// (0x00F64A6C: flags +136 |= 0x200, pointer cleared), then looks name up in
// the set 0x7719A548, else in the default set (Set_Find(0)); a found def
// gets a new effect (0x004A5E58(def, 0, 0, 1)) stored there with the auto
// start flag cleared. r3 is 0 on every path, as in the original.
DECOMP(002AA9C8, Overlay_Clear2AA9C8) {
    const u32 name = arg<u32>(ctx, 0);
    const u32 cur = load<u32>(kOverlayEffect);
    if (cur != 0) {
        store<u32>(cur + kEffectFlags, load<u32>(cur + kEffectFlags) | kEffectAutoStart);
        store<u32>(kOverlayEffect, 0);
    }
    const u32 s = static_cast<u32>(call(ctx, ffn::Set_Find, {kOverlaySet}));
    if (s == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 def = set_lookup(s, name);
    if (def == 0) {
        const u32 s0 = static_cast<u32>(call(ctx, ffn::Set_Find, {0}));
        if (s0 != 0) def = set_lookup(s0, name);
    }
    if (def != 0) {
        const u32 e = static_cast<u32>(call(ctx, ffn::Effect_Create4A5E58, {def, 0, 0, 1}));
        store<u32>(kOverlayEffect, e);
        if (e != 0) store<u32>(e + kEffectFlags, load<u32>(e + kEffectFlags) & ~kEffectAutoStart);
    }
    ret(ctx, 0u);
}

// SubMesh* Prim_Build52374C(Material* m, s32 type, u32 vertices, u32 indices,
// VertexFormat* fmt) (name?): a sub-mesh from the AglSubMesh free list
// (made on first use; no pool -> 0, and the fields below are then written at
// address 0, as the original does): +68 fmt, a vertex buffer (+109 u8 count
// of them, 16 bytes each from +24) of vertices * fmt stride (+120), +40
// vertices, a 16-bit index buffer at +44 (+60 = 1) with indices, +64
// indices, +76 the type's entry of 0x00F66F1C, +16 material (refcount +4;
// the old one released through vfunc +8).
DECOMP(0052374C, Prim_Build52374C) {
    const u32 m = arg<u32>(ctx, 0);
    const u32 type_off = static_cast<u32>(static_cast<s32>(arg<u32>(ctx, 1) << 2));
    const u32 nv = arg<u32>(ctx, 2), ni = arg<u32>(ctx, 3), fmt = arg<u32>(ctx, 4);
    const u32 holder = load<u32>(kSubMeshPool);
    const u32 ptype = load<u32>(kPrimTypes + type_off);
    u32 obj = 0;
    if (holder != 0) {
        u32 list = load<u32>(holder);
        if (list == 0) {
            const u32 l = static_cast<u32>(call(ctx, ffn::Mem_Alloc, {60, 0, 16}));
            if (l != 0) {
                const u32 count = load<u32>(holder + 4);
                for (u32 off : {8u, 4u, 12u, 20u, 16u}) store<u32>(l + off, 0);
                const u16 h54 = load<u16>(l + 54);
                for (u32 off : {24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(l + off, 0);
                store<u16>(l + 52, 0);
                store<u16>(l + 54, static_cast<u16>(h54 & 0x7FFF));
                store<u32>(l, kSubMeshListVtbl);
                store<u32>(l + 56, kStrSubMeshList);
                // r8 / r9 = the vtable and the string, as the original leaves them.
                call(ctx, ffn::FreeList_Init, {l, kSubMeshSize, count, 0, 0, kSubMeshListVtbl, kStrSubMeshList});
            }
            list = l;  // 0 if Mem_Alloc failed: the original then calls through guest address 0
            store<u32>(holder, list);
        }
        const u32 sm = static_cast<u32>(vcall(ctx, list, 12, {list}));
        if (sm != 0) {
            call(ctx, ffn::SubMesh_Construct5386DC, {sm});
            obj = sm;
        }
    }
    store<u32>(obj + 68, fmt);
    if (nv != 0) {
        const u32 n = load<u8>(obj + 109);
        store<u8>(obj + 109, static_cast<u8>(n + 1));
        const u32 bytes = nv * load<u16>(fmt + 120);
        call(ctx, ffn::Buffer_Alloc52F198, {obj + (n << 4) + 24, bytes, 128, 1, 2});
    }
    store<u32>(obj + 40, nv);
    if (ni != 0) {
        store<u32>(obj + 60, 1);
        call(ctx, ffn::Buffer_Alloc52F198, {obj + 44, ni << 1, 128, 1, 2});
    }
    store<u32>(obj + 64, ni);
    const u32 old = load<u32>(obj + 16);
    store<u32>(obj + 76, ptype);
    if (old != m) {
        store<u32>(obj + 16, m);
        if (m != 0) store<u16>(m + 4, static_cast<u16>(load<u16>(m + 4) + 1));
        if (old != 0) vcall(ctx, old, 8, {old});
    }
    ret(ctx, obj);
}
