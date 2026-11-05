// Shared names for src/world/hevent22_*.cpp: callees named by the
// hevent21_*.cpp functions and more of the World event handler's remaining
// callees. Guest addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include "hevent19.h"
#include "hevent6.h"

namespace sly4 {
namespace hevent22 {

namespace h22fn {
constexpr u32 Speaker_SayId = 0x004A5C90;    // (Speaker*, u32 text id, u8; f32) -> bool (Speaker_SayId4A5C90)
constexpr u32 Effect_SetHash = 0x004CF988;   // (Effect*, u32 hash) -> bool (Effect_SetHash4CF988)
constexpr u32 SortedMap_Find3 = 0x0076611C;  // (map, const u32* key) -> value slot (as hevent11.h)
constexpr u32 Array_Reserve = 0x00558F3C;    // (array, new capacity) (symbols.txt)
}  // namespace h22fn

// Object mode fields (as hevent21_a.cpp Obj_SetMode368870): +160 bits 1..31
// mode, bit 0 kept; +532 flags.
constexpr u32 kObjMode = 160, kObjModeFlags = 532;
// The per-mode effect hashes given to Effect_SetHash4CF988 (u32[mode]).
constexpr u32 kModeHashes = 0x00F64FB0;

// The engine's inlined array push used on {key, value} maps (u16 +0
// capacity, +2 count, +4 high-water mark, +6 flags (bit 0: unsorted), +8
// 8-byte entries): full arrays grow to twice their capacity first.
inline void map_append(PPUContext& ctx, u32 m, u32 key, u32 value) {
    if (load<u16>(m + 2) == load<u16>(m + 0)) call(ctx, h22fn::Array_Reserve, {m, (static_cast<u32>(load<u16>(m + 0)) << 1) & 0xFFFEu});
    const u32 data = load<u32>(m + 8);
    store<u32>(data + (static_cast<u32>(load<u16>(m + 2)) << 3), key);
    store<u32>(data + (static_cast<u32>(load<u16>(m + 2)) << 3) + 4, value);
    const u32 n1 = static_cast<u32>(load<u16>(m + 2)) + 1;
    const u32 hi = load<u16>(m + 4);
    const u32 fl = load<u16>(m + 6);
    store<u16>(m + 2, static_cast<u16>(n1));
    store<u16>(m + 4, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n1 & 0xFFFF) ? hi : n1));
    store<u16>(m + 6, static_cast<u16>(fl | 1u));
}

// Track object bases (as hevent19_b.cpp): object +0 points 152 bytes into
// its base; base +6 u16 flags, +144 u16 record count, +146 u16, 132-byte
// records from +152 {+0 s32 kind, +4 u16 type, ...}. Type table at
// 0x00F65100, 44 bytes per type, OPDs at +8 / +24 / +28 / +36.
constexpr u32 kBaseBias = 152, kBaseFlags = 6, kBaseCount = 144, kBaseExtra = 146, kRecSize = 132;
constexpr u32 kTypeTable = 0x00F65100, kTypeSize = 44;
// Record k of a base (k signed, as the original's sign-extended index).
inline u32 rec_at(u32 base, s32 k) { return base + kBaseBias + static_cast<u32>(k) * kRecSize; }
// The record when its index is in range and its kind is below 10, else 0
// (the original then reads through the null record).
inline u32 rec_checked(u32 base, s32 k) {
    if (!(k < static_cast<s32>(load<u16>(base + kBaseCount)))) return 0;
    const u32 r = rec_at(base, k);
    return static_cast<s32>(load<u32>(r)) < 10 ? r : 0;
}
inline u32 type_entry(s32 type) { return kTypeTable + static_cast<u32>(type) * kTypeSize; }

// hevent6::map_next, compiled with the right subtree's descent storing the
// path length before the entry (hevent6::map_push order).
inline u32 map_next_lf(u32 m, u32 node) {
    using namespace hevent6;
    u32 r = load<u32>(node + kNodeRight);
    if (r != 0) {
        map_push(m, node);
        while (load<u32>(r + kNodeLeft) != 0) {
            map_push(m, r);
            r = load<u32>(r + kNodeLeft);
        }
        store<u32>(m + kMapNode, r);
        return r;
    }
    u32 depth = load<u16>(m + kMapDepth);
    u32 child = node;
    for (;;) {
        if ((depth & 0xFFFF) == 0) {
            store<u32>(m + kMapNode, 0);
            return 0;
        }
        depth -= 1;
        store<u16>(m + kMapDepth, static_cast<u16>(depth));
        const u32 parent = load<u32>(m + kMapPath + static_cast<u32>(static_cast<s32>(static_cast<s16>(depth))) * 4);
        const bool from_left = load<u32>(parent + kNodeLeft) == child;
        store<u32>(m + kMapNode, parent);
        if (from_left) return parent;
        child = parent;
    }
}

// The effect def set lookup (as hevent21_g.cpp): def +24 Set, its map at
// +12 (hevent6.h layout). Returns the def named hash or 0; stores the
// current node and, on a hit, the global 0x011DE2A8. The set is re-read
// from the def after a hit.
constexpr u32 kEffectLastDef = 0x011DE2A8;
constexpr u32 kDefSet = 24, kSetMap = 12;
inline u32 effect_def_lookup(u32 def, u32 hash) {
    using namespace hevent6;
    const u32 set = load<u32>(def + kDefSet);
    if (set == 0) return 0;
    const u32 m = set + kSetMap;
    const u32 root = load<u32>(load<u32>(m));
    if (root == 0) {
        store<u32>(m + kMapNode, 0);
        store<u32>(kEffectLastDef, 0);
        return 0;
    }
    const u32 n = map_walk(m, root, hash);
    if (n == 0) return 0;
    store<u32>(m + kMapNode, n);
    store<u32>(kEffectLastDef, load<u32>(n + kNodeValue));
    const u32 set2 = load<u32>(def + kDefSet);
    if (set2 == 0) return 0;
    const u32 cur = load<u32>(set2 + kSetMap + kMapNode);
    return cur != 0 ? load<u32>(cur + kNodeValue) : 0;
}

}  // namespace hevent22
}  // namespace sly4
