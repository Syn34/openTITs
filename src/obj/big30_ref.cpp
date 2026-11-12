// Game code, large-function sweep part 30: resolving the 20-byte typed
// references of an object's entry list (o->+2272) to the things they name.
#include "../sly4.h"
#include "big30.h"

using namespace sly4;
using namespace sly4::big30;

namespace {
constexpr u32 Obj_FindById = 0x000D6A28;         // (u32 id) -> Obj* or 0
constexpr u32 Target_GetDef0D951C = 0x000D951C;  // (Obj*) -> definition or 0
constexpr u32 Volume_EnsureEffect_002202BC = 0x002202BC;  // (Obj*)
constexpr u32 Item_Hash39FEBC = 0x0039FEBC;      // (Item*) -> u32 hash
constexpr u32 Hash_ToString = 0x0056A6E0;        // (u32 hash) -> const char* (only for a debug message)
constexpr u32 SortedMap_Find733F8C = 0x00733F8C; // (map, const u32* key) -> value slot
constexpr u32 SortedMap_Find734750 = 0x00734750; // (map, const u32* key) -> s16* index slot (-1: none)
constexpr u32 SortedMap_Find73B608 = 0x0073B608; // (map, const u32* key) -> value slot
constexpr u32 kLib = 0x00F657FC;   // the definition library: +4 groups, +12 group base (112 bytes each),
                                   // +28 item base (336 bytes each), +36 / +44 the 368-byte records,
                                   // +68 the name map of the groups
constexpr u32 kMaps = 0x00F65BE8;  // +4 / +28: id maps for reference kinds 2 / 3

// The "not found" tail: a debug name for warn, then +11 = +12 = 1.
inline void Missing(PPUContext& ctx, u32 r, bool warn) {
    if (warn) call(ctx, Hash_ToString, {load<u32>(r)});
    store<u8>(r + 11, 1);
    store<u8>(r + 12, 1);
}
}  // namespace

// void f(Ref* r, u8 warn) (called by f_0032549C, Obj_ShowEntries_00325CE4,
// Obj_RefreshSlots_00327684). Ref: +0 id, +4 s32 kind, +11 u8 missing,
// +12 u8, +16 the resolved pointer. For kinds 0..7, +12 = 0 and nothing
// more once +16 is set; otherwise by kind:
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
// leftovers (regflow liveout), so r3 is not reproduced.
DECOMP(00321324, Ref_Resolve_00321324) {
    const u32 r = arg<u32>(ctx, 0);
    const bool warn = static_cast<u8>(ctx.gpr[4]) != 0;
    const s32 kind = static_cast<s32>(load<u32>(r + 4));
    if (kind < 0 || kind > 7) return;
    store<u8>(r + 12, 0);
    if (load<u32>(r + 16) != 0) return;
    Stack frame(ctx, 16);  // the original's id copies at 112..124(r1)
    const u32 key = frame.addr();
    switch (kind) {
    case 0: {
        const u32 x = static_cast<u32>(call(ctx, Obj_FindById, {load<u32>(r)}));
        const u32 v = (x != 0 && (load<u32>(x + 192) & 0x00400000u)) ? x : 0;
        store<u32>(r + 16, v);
        if (v == 0) Missing(ctx, r, warn);
        if (x != 0 && (load<u32>(x + 192) & 0x80000000u)) call(ctx, Volume_EnsureEffect_002202BC, {x});
        return;
    }
    case 1: {
        const u32 x = static_cast<u32>(call(ctx, Obj_FindById, {load<u32>(r)}));
        if (x != 0) {
            const u32 d = static_cast<u32>(call(ctx, Target_GetDef0D951C, {x}));
            store<u32>(r + 16, d);
            if (d != 0) return;
        }
        Missing(ctx, r, warn);
        return;
    }
    case 2:
    case 3: {
        const u32 m = load<u32>(kMaps) + (kind == 2 ? 4 : 28);
        store<u32>(key + (kind == 2 ? 12 : 8), load<u32>(r));
        const u32 slot = static_cast<u32>(call(ctx, kind == 2 ? SortedMap_Find733F8C : SortedMap_Find73B608, {m, key + (kind == 2 ? 12 : 8)}));
        const u32 v = load<u32>(slot);
        store<u32>(r + 16, v);
        if (v == 0) Missing(ctx, r, warn);
        return;
    }
    case 4: {
        const u32 lib = load<u32>(kLib), id = load<u32>(r);
        u32 v = 0;
        if (id != 0 && load<u16>(load<u32>(lib + 4) + 2) != 0) {
            for (u32 i = 0, off = 0;; ++i, off += 112) {
                const u32 g = load<u32>(lib + 12) + off;
                if (g != 0) {
                    store<u32>(key + 4, id);
                    const u32 slot = static_cast<u32>(call(ctx, SortedMap_Find734750, {g + 76, key + 4}));
                    const s32 idx = static_cast<s16>(load<u16>(slot));
                    if (idx != -1) {
                        v = load<u32>(lib + 28) + static_cast<u32>(idx) * 336;
                        break;
                    }
                }
                if (i + 1 >= load<u16>(load<u32>(lib + 4) + 2)) break;
            }
        }
        store<u32>(r + 16, v);
        if (v == 0) Missing(ctx, r, warn);
        return;
    }
    case 5: {
        const u32 lib = load<u32>(kLib), id = load<u32>(r);
        u32 v = 0;
        if (id != 0 && load<u16>(load<u32>(lib + 36) + 2) != 0) {
            for (u32 i = 0, off = 0;; ++i, off += 368) {
                const u32 e = load<u32>(lib + 44) + off;
                if (e != 0 && static_cast<u32>(call(ctx, Item_Hash39FEBC, {e})) == id) {
                    v = e;
                    break;
                }
                if (i + 1 >= load<u16>(load<u32>(lib + 36) + 2)) break;
            }
        }
        store<u32>(r + 16, v);
        if (v == 0) Missing(ctx, r, warn);
        return;
    }
    case 6: {
        const u32 lib = load<u32>(kLib);
        u32 id = load<u32>(r), v = 0;
        store<u32>(key, id);
        if (id != 0) {
            const u32 slot = static_cast<u32>(call(ctx, SortedMap_Find734750, {lib + 68, key}));
            const s32 idx = static_cast<s16>(load<u16>(slot));
            id = load<u32>(r);
            if (idx != -1) v = load<u32>(lib + 12) + static_cast<u32>(idx) * 112;
        }
        store<u32>(r + 16, v);
        if (v == 0) {
            if (warn) call(ctx, Hash_ToString, {id});
            store<u8>(r + 11, 1);
            store<u8>(r + 12, 1);
        }
        return;
    }
    default: {  // 7
        const u32 x = static_cast<u32>(call(ctx, Obj_FindById, {load<u32>(r)}));
        const u32 id = load<u32>(r);
        if (x != 0 && (load<u32>(x + 200) & 2)) {
            store<u32>(r + 16, x);
            return;
        }
        call(ctx, Hash_ToString, {id});  // even without warn
        store<u8>(r + 11, 1);
        store<u8>(r + 12, 1);
        return;
    }
    }
}
