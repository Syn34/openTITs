// World_HandleEvent callees, part 27b: geometry instances (texture
// binding, effect hookup and pool reset).
#include "hevent27.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent27;

namespace {
// ApeTListAllocator<ApeGeomInstance>: the allocator slot {+0 allocator*,
// +4 nodes per block}; the allocator is a 60-byte free list (vtable
// 0x00B0F008, +56 its name string, 352-byte nodes).
constexpr u32 kGeomAllocSlot = 0x00F8F144;
constexpr u32 kVtGeomFreeList = 0x00B0F008;
constexpr u32 kStrGeomFreeList = 0x00AEFBCB;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = ApeGeomInstance]"
constexpr u32 kGeomNodeSize = 352, kGeomNodeBias = 16;  // the instance sits 16 bytes into its node (+0 next node)
// Instance: vt +12 update; +12 component list (+4 + 4k, 4 slots, +8 kind);
// +208 class (u8 +112 part count, +132 + 8j part*: u16 +52 count, +48
// {key, Item*} entries, Item +16 -> +8 kind).
constexpr u32 kKindEffectHost = 0x87905FBFu, kKindEffectItem = 0xD4A448DFu;
// World class registry (0x00F8F12C -> +16 map by name hash).
constexpr u32 kClassRegistry = 0x00F8F12C;
}  // namespace

// ---------------------------------------------------------------- 0x004F23C4
// u32 GeomInstances_HookEffects4F23C4(World* w) (name?): clears the flip
// slots, makes sure the ApeGeomInstance free list exists (60-byte
// allocator, FreeList_Init(352, slot +4)), then for every live instance:
// vt +12 (instance), and when its component list has a 0x87905FBF entry
// in its first four slots before a null one, the first part item of kind
// 0xD4A448DF in the class's parts is handed to Call5A9DB0_005AC7AC(0,
// w +0, instance, item, entry). Returns 0.
DECOMP(004F23C4, GeomInstances_HookEffects4F23C4) {
    const u32 w = arg<u32>(ctx, 0);
    call(ctx, h27fn::ClearFlipSlots, {w});
    const u32 slot = load<u32>(kGeomAllocSlot);
    u32 alloc = load<u32>(slot);
    if (alloc == 0) {
        const u32 a = static_cast<u32>(call(ctx, h27fn::Mem_Alloc, {60, 0, 16}));
        if (a != 0) {
            const u32 per_block = load<u32>(slot + 4);
            for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u, 32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(a + off, 0);
            const u16 fl = load<u16>(a + 54);
            store<u16>(a + 52, 0);
            store<u16>(a + 54, static_cast<u16>(fl & 0x7FFF));
            store<u32>(a + 0, kVtGeomFreeList);
            store<u32>(a + 56, kStrGeomFreeList);
            call(ctx, h27fn::FreeList_Init, {a, kGeomNodeSize, per_block, 0, 0});
        }
        alloc = a;
        store<u32>(slot, alloc);
    }
    const u32 first = static_cast<u32>(call(ctx, h27fn::Obj_GetField4, {alloc}));
    for (u32 g = first != 0 ? first + kGeomNodeBias : 0; g != 0;) {
        vcall(ctx, g, 12, {g});
        const u32 comps = load<u32>(g + 12);
        if (comps != 0) {
            for (u32 k = 0; k < 4; ++k) {
                const u32 c = load<u32>(comps + 4 + 4 * k);
                if (c == 0) break;
                if (load<u32>(c + 8) != kKindEffectHost) continue;
                const u32 entry = load<u32>(comps + 4 * k + 4);
                if (entry == 0) break;
                const u32 cls = load<u32>(g + 208);
                const u32 nparts = load<u8>(cls + 112);
                u32 item = 0;
                for (u32 j = 0; j < nparts && item == 0; ++j) {
                    const u32 part = load<u32>(cls + 8 * j + 132);
                    const u32 n = load<u16>(part + 52);
                    for (u32 m = 0; m < n; ++m) {
                        const u32 it = load<u32>(load<u32>(part + 48) + 8 * m + 4);
                        if (load<u32>(load<u32>(it + 16) + 8) == kKindEffectItem) {
                            item = it;
                            break;
                        }
                    }
                }
                if (item != 0) call(ctx, h27fn::Call5A9DB0, {0, load<u32>(w), g, item, entry});
                break;
            }
        }
        const u32 next = load<u32>(g - kGeomNodeBias);
        g = next != 0 ? next + kGeomNodeBias : 0;
    }
    ret(ctx, 0u);
}

// ---------------------------------------------------------------- 0x004F41D8
// void GeomScene_Reset4F41D8(GeomScene* s) (name?): frees the +88 table's
// 24-byte records (and their +8 buffers of +12 words), calls vt +4 on the
// +40 table's objects, clears +84 of the +16 table's objects, then walks
// the +104 handle table from its first handle (idx << 16 | serial),
// calling vt +4 on each live entry's +8 object and releasing the handle
// (0x0079A328) until it is empty or a handle is stale; finally
// Buffer_Sync566318(+4, +4 -> +8) and +140 = 0. Counts +82 / +34 cleared.
DECOMP(004F41D8, GeomScene_Reset4F41D8) {
    const u32 s = arg<u32>(ctx, 0);
    for (s32 k = 0; k < static_cast<s32>(load<u16>(s + 82)); ++k) {
        const u32 r = load<u32>(load<u32>(s + 88) + 8 * static_cast<u32>(k) + 4);
        if (r == 0) continue;
        const u32 words = load<u32>(r + 12) << 2;
        const u32 buf = load<u32>(r + 8);
        if (buf != 0) {
            call(ctx, h27fn::SmallFree, {buf, words});
            store<u32>(r + 8, 0);
        }
        call(ctx, h27fn::SmallFree, {r, 24});
    }
    store<u16>(s + 82, 0);
    for (s32 k = 0; k < static_cast<s32>(load<u16>(s + 34)); ++k) {
        const u32 o = load<u32>(load<u32>(s + 40) + 8 * static_cast<u32>(k) + 4);
        if (o != 0) vcall(ctx, o, 4, {o});
    }
    store<u16>(s + 34, 0);
    for (s32 k = 0; k < static_cast<s32>(load<u16>(s + 10)); ++k) store<u32>(load<u32>(load<u32>(s + 16) + 8 * static_cast<u32>(k) + 4) + 84, 0);
    // Handle table at +104: +8 first index, +16 count, +24 capacity, +28
    // 16-byte entries (+8 object, +12 bit 31 free, bits 15..30 serial).
    const u32 T = s + 104;
    auto first_handle = [&]() -> u32 {
        if (load<u32>(s + 120) == 0) return 0;
        const u32 idx = load<u32>(s + 112);
        const u32 w = load<u32>(load<u32>(s + 132) + (idx << 4) + 12);
        return ((idx & 0xFFFF) << 16) | ((w >> 15) & 0xFFFF);
    };
    for (u32 h = first_handle();;) {
        const u32 serial = h & 0xFFFF;
        if (serial == 0) break;
        const u32 idx = h >> 16;
        if (!(static_cast<s32>(idx) < static_cast<s32>(load<u32>(s + 128)))) break;
        const u32 e = load<u32>(s + 132) + (idx << 4);
        if (((load<u32>(e + 12) >> 15) & 0xFFFF) != serial) break;
        if ((load<u8>(e + 12) >> 7) & 1) break;
        const u32 o = load<u32>(e + 8);
        if (o != 0) vcall(ctx, o, 4, {o});
        {
            Stack out(ctx, 16);
            call(ctx, 0x0079A328, {out.addr(), T, h});
        }
        h = first_handle();
    }
    const u32 buf = load<u32>(s + 4);
    call(ctx, h27fn::Buffer_Sync, {buf, load<u32>(buf + 8)});
    store<u32>(s + 140, 0);
}

// ---------------------------------------------------------------- 0x004F0000
// u32 Geom_BindClassTextures4F0000(-, Scene* sc) (name?): for each of the
// scene's +516 defs (u16 +524 count): the registered class of the def's
// +104 hash (stops at the first unknown one, returning the slot pointer)
// is copied (WorldClass_Copy), every {id -> texture name} entry of the
// def's +120 map whose texture is loaded is bound to each part item slot
// m whose (+20)[m] +48 is the id (Obj_SetSlot20(item, tex, m)), and the
// copy is named after the def (Def_SetName(copy, def, 1)). Returns the
// def count otherwise.
DECOMP(004F0000, Geom_BindClassTextures4F0000) {
    using namespace hevent6;
    const u32 sc = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    for (u32 i = 0; i < load<u16>(sc + 524); ++i) {
        const u32 def = load<u32>(load<u32>(sc + 516) + 4 * i);
        store<u32>(frame.addr(), load<u32>(def + 104));
        const u32 slotp = static_cast<u32>(call(ctx, h27fn::HashMap_Find2, {load<u32>(load<u32>(kClassRegistry) + 16), frame.addr()}));
        const u32 cls = load<u32>(slotp);
        if (cls == 0) return ret(ctx, slotp);
        const u32 copy = static_cast<u32>(call(ctx, h27fn::WorldClass_Copy, {cls}));
        const u32 m = def + 120;
        for (u32 node = map_first(m); node != 0;) {
            const u32 name = load<u32>(node + kNodeValue);
            const u32 id = load<u32>(node + kNodeKey);
            const u32 tex = static_cast<u32>(call(ctx, h27fn::Tex_FindLoaded, {name}));
            if (tex != 0) {
                for (u32 j = 0; j < load<u8>(copy + 112); ++j) {
                    const u32 pp = copy + 8 * j + 132;
                    for (u32 k = 0; k < load<u16>(load<u32>(pp) + 52); ++k) {
                        const u32 item = load<u32>(load<u32>(load<u32>(pp) + 48) + 8 * k + 4);
                        for (u32 s = 0; s < load<u8>(item + 44); ++s) {
                            if (load<u32>(load<u32>(item + 20 + 4 * s) + 48) == id) call(ctx, h27fn::Obj_SetSlot20, {item, tex, static_cast<u64>(static_cast<s64>(static_cast<s32>(s)))});
                        }
                    }
                }
            }
            const u32 cur = load<u32>(m + kMapNode);
            if (cur == 0) break;
            node = map_next(m, cur);
        }
        call(ctx, h27fn::Def_SetName, {copy, def, 1});
    }
    ret(ctx, static_cast<u32>(load<u16>(sc + 524)));
}
