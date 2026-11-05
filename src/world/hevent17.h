// Shared names for src/world/hevent17_*.cpp: callees named by the
// hevent16_*.cpp functions and more of the World event handler's remaining
// callees. Guest addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include "hevent16.h"

namespace sly4 {
namespace hevent17 {

namespace h17fn {
constexpr u32 SortedMap_Find76DC10 = 0x0076DC10;  // (map, const u32* key) -> value slot (symbols.txt)
constexpr u32 Set_Find = 0x004BCE90;              // (u32 name hash) -> Set* or 0 (symbols.txt)
constexpr u32 HashedName_Set = 0x0036960C;        // (u8 user, const char* name) (symbols.txt)
constexpr u32 User0_RefreshTarget = 0x0047BFD4;   // (World*, u32 user) (symbols.txt)
constexpr u32 SmallAlloc = 0x0055238C;            // (size) -> block or 0 (as hevent7.h)
constexpr u32 PtrVector_Construct = 0x005516E0;   // (vec) (as hevent9.h)
constexpr u32 PtrVector_Reserve = 0x00551718;     // (vec, new capacity) (as hevent8.h)
constexpr u32 Effect_Bind4A63FC = 0x004A63FC;     // (Effect** slot, u32 name, u32 set, owner, value, u8) (hevent17_a.cpp)
constexpr u32 Anim_Reset2980AC = 0x002980AC;      // (Animator*, Set*) (hevent17_a.cpp)
}  // namespace h17fn

// The current world, as hevent.h kCurrentWorld.
using hevent::kCurrentWorld;

// World +0x12847C: the "main" Object* (+164 u32 flags, bit 31: active).
constexpr u32 kWorldMain = 0x12847C;
constexpr u32 kObjFlags164 = 164, kObjActive = 0x80000000u;

// The scene handle (as hevent16_a.cpp): u32 at 0x00F557F0.
constexpr u32 kSceneHandle = 0x00F557F0;

}  // namespace hevent17
}  // namespace sly4
