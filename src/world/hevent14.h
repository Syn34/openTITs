// Shared names for src/world/hevent14_*.cpp: callees named by the
// hevent13_*.cpp functions. Guest addresses in the BCES01284 v01.02 EBOOT;
// "?" marks a guess.
#pragma once

#include "hevent13.h"

namespace sly4 {
namespace hevent14 {

namespace h14fn {
constexpr u32 Effect_AddArg49F080 = 0x0049F080;     // (Effect*, Arg*, e, u8, u8) (name?)
constexpr u32 Avl_RotateLeft = 0x00792040;          // (Tree*, Node** link) (hevent14_b.cpp)
constexpr u32 Avl_RotateRight = 0x00791EF8;         // (Tree*, Node** link) (hevent14_b.cpp)
constexpr u32 Avl_EraseTwo = 0x00792188;            // (Tree*, Node** link, Node** sub, u8* done) (hevent14_b.cpp)
constexpr u32 Entry_Index325EBC = 0x00325EBC;       // (Entry*, Object*) -> s32 (hevent14_a.cpp)
constexpr u32 World_StartLevel = 0x004674C4;        // (World*, level, 0) (symbols.txt)
constexpr u32 User_Travel484BE0 = 0x00484BE0;       // 9 arguments (name?)
constexpr u32 World_SpawnUsers487670 = 0x00487670;  // (World*, Entry*) (hevent13_e.cpp)
constexpr u32 User_Stay488178 = 0x00488178;         // (World*, UserBlock*, Mat*) (hevent13_b.cpp)
constexpr u32 Obj_Release12BF70 = 0x0012BF70;       // (Object*) (name?)
}  // namespace h14fn

// Object (player) bits read by the entry code: +188 bit 5, +192 bit 3, +216
// owner (+52 point hash, +192 alt hash).
constexpr u32 kObjFlags188 = 188, kObjFlags192 = 192, kObjOwner = 216;
constexpr u32 kOwnerHash = 52, kOwnerAltHash = 192;
constexpr u32 kObjFlag188Bit5 = 0x20, kObjFlag192Bit3 = 0x08;
// Entry +2416: spawn hashes for the 9 users; the "any point" hash.
constexpr u32 kEntrySpawns = 2416, kNumSpawns = 9;
constexpr u32 kAnyPoint = 0x14120;

}  // namespace hevent14
}  // namespace sly4
