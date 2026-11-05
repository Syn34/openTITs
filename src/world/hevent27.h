// Shared names for src/world/hevent27_*.cpp: more of the World event
// handler's remaining callees (sound shutdown, geometry instances, Havok
// world setup, camera). Guest addresses in the BCES01284 v01.02 EBOOT;
// "(name?)" marks a guess.
#pragma once

#include <bit>

#include "hevent22.h"

namespace sly4 {
namespace hevent27 {

namespace h27fn {
constexpr u32 Mem_Alloc = 0x0054ED38;          // (size, pool, align)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;       // (p, pool)
constexpr u32 Pool_Free = 0x00565C50;          // (MemPool*, p)
constexpr u32 Array_Destruct = 0x0054EC94;     // (Array*) (empty)
constexpr u32 SmallFree = 0x00552420;          // (p, size)
constexpr u32 FreeList_Init = 0x00551A58;      // (list, node size, nodes per block, 0, 0)
constexpr u32 Emitter_Stop = 0x00443424;       // (u32 handle, bool fade)
constexpr u32 Sound_Free = 0x00445390;         // (SoundMgr*, Sound*) (Sound_Free445390)
constexpr u32 Music_SetTime = 0x004473C0;      // (; f32) (Music_SetTime4473C0)
constexpr u32 PushStack_Changed = 0x00447B70;  // (; f32 time)
constexpr u32 Obj_ClearField8 = 0x00565A88;    // (o)
constexpr u32 Obj_GetField4 = 0x00556BF0;      // (p) -> p +4
constexpr u32 ClearFlipSlots = 0x005AC634;     // () (ClearFlipSlots_005AC634)
constexpr u32 Call5A9DB0 = 0x005AC7AC;         // (s32 skip, a, b, c, d) (Call5A9DB0_005AC7AC)
constexpr u32 Buffer_Sync = 0x00566318;        // (Buffer*, u32 size) (Buffer_Sync566318)
constexpr u32 Tex_FindLoaded = 0x0050C398;     // (name) -> texture or 0
constexpr u32 Obj_SetSlot20 = 0x004FEF48;      // (o, tex, s32 slot)
constexpr u32 Def_SetName = 0x00584BDC;        // (def, name, bool hash)
constexpr u32 WorldClass_Copy = 0x005858D4;    // (class) -> copy
constexpr u32 HashMap_Find2 = 0x007399A4;      // (map, const u32* key) -> value slot
}  // namespace h27fn

// The game's inlined {data, allocator} array release: data (+8 from the
// u16 count, here the base) is handed back to allocator vt +8 (alloc,
// data, u16 count * ((u16 +6 >> 1) & 0x7FFF)) when set, then
// Array_Destruct.
inline void array_release(PPUContext& ctx, u32 a) {
    const u32 size = static_cast<u32>(load<u16>(a + 0)) * ((static_cast<u32>(load<u16>(a + 6)) >> 1) & 0x7FFFu);
    const u32 data = load<u32>(a + 8);
    if (data != 0) {
        const u32 alloc = load<u32>(a + 12);
        hevent::vcall(ctx, alloc, 8, {alloc, data, size});
    }
    call(ctx, h27fn::Array_Destruct, {a});
}

inline f32 lf(u32 ea) { return std::bit_cast<f32>(load<u32>(ea)); }

}  // namespace hevent27
}  // namespace sly4
