// Game code, large-function sweep part 30: resolving the 20-byte typed
// references of an object's entry list (o->+2272) to the things they name.
#include "big30.h"

namespace sly4::obj {

namespace {
constexpr u32 Obj_FindById = 0x000D6A28;         // (u32 id) -> Obj* or 0
constexpr u32 Target_GetDef0D951C = 0x000D951C;  // (Obj*) -> definition or 0
constexpr u32 Volume_EnsureEffect_002202BC = 0x002202BC;  // (Obj*)
constexpr u32 Item_Hash39FEBC = 0x0039FEBC;      // (Item*) -> u32 hash
constexpr u32 Hash_ToString = 0x0056A6E0;        // (u32 hash) -> const char* (only for a debug message)
constexpr u32 SortedMap_Find733F8C = 0x00733F8C; // (map, const u32* key) -> value slot
constexpr u32 SortedMap_Find734750 = 0x00734750; // (map, const u32* key) -> s16* index slot (-1: none)
constexpr u32 SortedMap_Find73B608 = 0x0073B608; // (map, const u32* key) -> value slot
constexpr u32 kLib = 0x00F657FC;   // RefDefLib*: the definition library
constexpr u32 kMaps = 0x00F65BE8;  // +4 / +28: id maps for reference kinds 2 / 3

// An array header: +2 the u16 count.
struct RefCount {
    pad<2> _0;
    be_u16 count;  // +2
};
// The definition library: +4 groups (112 bytes each at +12), +28 the item
// base (336 bytes each), +36 / +44 the 368-byte records, +68 the name map
// of the groups.
struct RefDefLib {
    pad<4> _0;
    be_u32 groups;      // +4: RefCount*
    pad<4> _8;
    be_u32 group_base;  // +12
    pad<12> _16;
    be_u32 item_base;   // +28
    pad<4> _32;
    be_u32 recs;        // +36: RefCount*
    pad<4> _40;
    be_u32 rec_base;    // +44
    pad<20> _48;
    pad<4> name_map;    // +68 (a map; its address is used)
};
static_assert(offsetof(RefDefLib, rec_base) == 44 && offsetof(RefDefLib, name_map) == 68);
// The object flags Ref_Resolve tests.
struct RefObj {
    pad<192> _0;
    be_u32 flags;   // +192: bit 22 resolvable, bit 31 has a volume effect
    pad<4> _196;
    be_u32 flags2;  // +200: bit 1
};
static_assert(offsetof(RefObj, flags2) == 200);

// The "not found" tail: a debug name for warn, then +11 = +12 = 1.
void Missing(Ref* r, u8 warn) {
    if (warn != 0) call(Hash_ToString, r->id);
    r->u.b[3] = 1;
    r->pending = 1;
}
}  // namespace

// void f(Ref* r, u8 warn) (called by Section_Ready_0032549C,
// Obj_ShowEntries_00325CE4, Obj_RefreshSlots_00327684). For kinds 0..7,
// +12 = 0 and nothing more once +16 is set; otherwise by kind:
//  0: an object (Obj_FindById) with bit 22 of +192; with bit 31 of its +192
//     also Volume_EnsureEffect (found or not);
//  1: Target_GetDef0D951C of the object (+16 left alone without one);
//  2 / 3: the slot value of the maps at *0x00F65BE8 + 4 / + 28;
//  4: the first group (lib->+4 count u16 +2, 112 bytes at lib->+12) whose
//     +76 map has the id: item lib->+28 + 336 * index;
//  5: the first 368-byte record (count u16 +2 of lib->+36, base lib->+44)
//     whose Item_Hash39FEBC is the id;
//  6: the group named by the id in lib->+68: lib->+12 + 112 * index;
//  7: the object, when bit 1 of its +200 is set (+16 left alone otherwise,
//     and the debug name is made even without warn).
// Not found: the debug name (Hash_ToString, its result unused) for warn,
// +11 = +12 = 1. Other kinds do nothing. No caller reads r3 or other
// leftovers (regflow liveout).
void Ref_Resolve_00321324(Ref* r, u8 warn) {
    const s32 kind = r->kind;
    if (kind < 0 || kind > 7) return;
    r->pending = 0;
    if (r->target != 0u) return;
    GuestFrame frame(16);  // the original's id copies at 112..124(r1)
    be_u32* key = frame.as<be_u32>();
    switch (kind) {
    case 0: {
        const u32 x = call<u32>(Obj_FindById, r->id);
        const u32 v = (x != 0 && (gm<RefObj>(x).flags & 0x00400000u)) ? x : 0u;
        r->target = v;
        if (v == 0) Missing(r, warn);
        if (x != 0 && (gm<RefObj>(x).flags & 0x80000000u)) call(Volume_EnsureEffect_002202BC, x);
        return;
    }
    case 1: {
        const u32 x = call<u32>(Obj_FindById, r->id);
        if (x != 0) {
            const u32 d = call<u32>(Target_GetDef0D951C, x);
            r->target = d;
            if (d != 0) return;
        }
        Missing(r, warn);
        return;
    }
    case 2:
    case 3: {
        const u32 m = global<be_u32>(kMaps) + (kind == 2 ? 4u : 28u);
        be_u32* k = key + (kind == 2 ? 3 : 2);
        *k = r->id;
        const u32 slot = call<u32>(kind == 2 ? SortedMap_Find733F8C : SortedMap_Find73B608, m, k);
        const u32 v = gm<be_u32>(slot);
        r->target = v;
        if (v == 0) Missing(r, warn);
        return;
    }
    case 4: {
        const u32 lib = global<be_u32>(kLib);
        const u32 id = r->id;
        u32 v = 0;
        if (id != 0 && gm<RefCount>(gm<RefDefLib>(lib).groups).count != 0) {
            for (u32 i = 0, off = 0;; ++i, off += 112) {
                const u32 g = gm<RefDefLib>(lib).group_base + off;
                if (g != 0) {
                    key[1] = id;
                    const u32 slot = call<u32>(SortedMap_Find734750, g + 76, key + 1);
                    const s32 idx = gm<be_s16>(slot);
                    if (idx != -1) {
                        v = gm<RefDefLib>(lib).item_base + static_cast<u32>(idx) * 336u;
                        break;
                    }
                }
                if (i + 1 >= gm<RefCount>(gm<RefDefLib>(lib).groups).count) break;
            }
        }
        r->target = v;
        if (v == 0) Missing(r, warn);
        return;
    }
    case 5: {
        const u32 lib = global<be_u32>(kLib);
        const u32 id = r->id;
        u32 v = 0;
        if (id != 0 && gm<RefCount>(gm<RefDefLib>(lib).recs).count != 0) {
            for (u32 i = 0, off = 0;; ++i, off += 368) {
                const u32 e = gm<RefDefLib>(lib).rec_base + off;
                if (e != 0 && call<u32>(Item_Hash39FEBC, e) == id) {
                    v = e;
                    break;
                }
                if (i + 1 >= gm<RefCount>(gm<RefDefLib>(lib).recs).count) break;
            }
        }
        r->target = v;
        if (v == 0) Missing(r, warn);
        return;
    }
    case 6: {
        const u32 lib = global<be_u32>(kLib);
        u32 id = r->id, v = 0;
        key[0] = id;
        if (id != 0) {
            const u32 slot = call<u32>(SortedMap_Find734750, lib + 68, key);
            const s32 idx = gm<be_s16>(slot);
            id = r->id;
            if (idx != -1) v = gm<RefDefLib>(lib).group_base + static_cast<u32>(idx) * 112u;
        }
        r->target = v;
        if (v == 0) {
            if (warn != 0) call(Hash_ToString, id);  // the id as re-read after the lookup
            r->u.b[3] = 1;
            r->pending = 1;
        }
        return;
    }
    default: {  // 7
        const u32 x = call<u32>(Obj_FindById, r->id);
        const u32 id = r->id;
        if (x != 0 && (gm<RefObj>(x).flags2 & 2u)) {
            r->target = x;
            return;
        }
        call(Hash_ToString, id);  // even without warn
        r->u.b[3] = 1;
        r->pending = 1;
        return;
    }
    }
}

}  // namespace sly4::obj

NATIVE(00321324, sly4::obj::Ref_Resolve_00321324)
