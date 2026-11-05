// Shared names for src/world/hevent19_*.cpp: callees named by the
// hevent18_*.cpp functions and more of the World event handler's remaining
// callees. Guest addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include "hevent18.h"

namespace sly4 {
namespace hevent19 {

namespace h19fn {
constexpr u32 SortedMap_Find76D894 = 0x0076D894;  // (map, const u32* key) -> value slot (symbols.txt)
constexpr u32 SortedMap_Find742EF4 = 0x00742EF4;  // (map, const u32* key) -> value slot (symbols.txt)
constexpr u32 Hash_ToString = 0x0056A6E0;         // (u32) -> "0x%x" (symbols.txt)
constexpr u32 Trophy_Unlock4FB4A0 = 0x004FB4A0;   // (Trophy*, s32 number) (name?)
constexpr u32 WorldObj_GetPeer = 0x0057A428;      // (obj) -> peer or 0 (symbols.txt)
constexpr u32 Peer_RewindAnims = 0x0057CDB8;      // (peer) (symbols.txt)
constexpr u32 Anim_Handle254438 = 0x00254438;     // (u32* out, Animator*, u32 key) (hevent19_a.cpp)
constexpr u32 Anim_CurrentSet297E3C = 0x00297E3C; // (Animator*) -> u32 current set key (hevent19_a.cpp)
constexpr u32 SortedMap_Find75E3E8 = 0x0075E3E8;  // (map, const u32* key) -> value slot (symbols.txt)
constexpr u32 SortedMap_Find76E0A8 = 0x0076E0A8;  // (map, const u32* key) -> value slot (symbols.txt)
constexpr u32 Pool_Create4B25C8 = 0x004B25C8;     // (Pool*, const {def, Owner*}*, u32* out handle, Owner*) (name?)
constexpr u32 Node_Construct3516C4 = 0x003516C4;  // (Node*): 108-byte node constructor (name?)
constexpr u32 Node_Init350178 = 0x00350178;       // (Node*, u32 owner arg, 0) (name?)
constexpr u32 Track_Slot38B118 = 0x0038B118;      // (const u16* track, Base*) -> slot (+4 of a 132-byte record) (name?)
constexpr u32 Rig_FindBoneA59468 = 0x00A59468;    // (Rig*, u32 hash, 0) -> s16 index or -1 (name?)
}  // namespace h19fn

// The animation handle pool (u32 at 0x00F557F8): +256 s32 count, +260
// 16-byte entries {+8 Object*, +12 u32: bit 31 free, bits 15..30 serial}.
// A handle is (u16 index << 16) | u16 serial; serial 0 is the null handle.
// The track object pool (u32 at 0x00F65798) has the same entries with the
// count at +24 and the entries at +28.
constexpr u32 kAnimPool = 0x00F557F8;
constexpr u32 kPoolCount = 256, kPoolEntries = 260, kPoolEntrySize = 16;
constexpr u32 kTrackPool = 0x00F65798;
constexpr u32 kTrackPoolCount = 24, kTrackPoolEntries = 28;
constexpr u32 kEntryObject = 8, kEntryWord = 12;

// The object a handle names, or 0 (null handle, index out of range, serial
// mismatch, free entry, no object). The entry word is read once more for
// the free bit (lbz), as the original does.
inline u32 handle_object(u32 count_ea, u32 entries_ea, u32 h) {
    const u32 serial = h & 0xFFFF;
    if (serial == 0) return 0;
    const u32 idx = h >> 16;
    if (!(static_cast<s32>(idx) < static_cast<s32>(load<u32>(count_ea)))) return 0;
    const u32 ent = load<u32>(entries_ea) + idx * kPoolEntrySize;
    if (((load<u32>(ent + kEntryWord) >> 15) & 0xFFFF) != serial) return 0;
    if ((load<u8>(ent + kEntryWord) >> 7) != 0) return 0;
    return load<u32>(ent + kEntryObject);
}
inline u32 handle_object(u32 pool, u32 h) { return handle_object(pool + kPoolCount, pool + kPoolEntries, h); }

}  // namespace hevent19
}  // namespace sly4
