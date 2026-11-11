// Game code, large-function sweep part 18: helpers shared by big18_*.cpp.
#pragma once

#include <bit>
#include <cmath>

#include "big17.h"

namespace sly4 {
namespace big18 {

using namespace opentit;
using namespace opentit::decomp;
using big13::LfsX;
using big17::SetF1;
using mid1::LfsBits;
using obj19::VCall;

namespace b18fn {
constexpr u32 Pause_Set2AB340 = 0x002AB340;        // (int)
constexpr u32 Handle_ResolveBit10000 = 0x0035B908; // (handle) -> speaker object or 0
constexpr u32 Obj_NotifyA362F1C = 0x00362F1C;      // (o, 0, 0)
constexpr u32 Obj_Play363890 = 0x00363890;         // (o, 0, o, 0, 0, const u8 colour[4]; f1)
constexpr u32 Handle_ResolveBit20000 = 0x00365B28; // (handle) -> shop object or 0
constexpr u32 Player_Get = 0x00365C24;             // () -> player or 0
constexpr u32 Pending_Build366E20 = 0x00366E20;    // ()
constexpr u32 Obj_StartMode368360 = 0x00368360;    // (o)
constexpr u32 Obj_ShopModeOff3684B8 = 0x003684B8;  // (o)
constexpr u32 Obj_SetTarget168 = 0x0037202C;       // (o, 1, 0)
constexpr u32 Obj_SelectEntry = 0x003734B0;        // (player, u32 hash)
constexpr u32 Obj_RequestFlag3736C4 = 0x003736C4;  // (player, 1) -> bool
constexpr u32 Obj_ShowPanel = 0x0037370C;          // (player, panel) -> bool
constexpr u32 Obj_NotifyB386140 = 0x00386140;      // (o, 0, 0)
constexpr u32 Obj_SpeakerTalk386560 = 0x00386560;  // (o, 0, ?, 0, 0, const u8 colour[4]; f1)
constexpr u32 Pause_Request44BB48 = 0x0044BB48;    // (u32 mask)
constexpr u32 Sound_Play44C474 = 0x0044C474;       // (out, sound, 0 x 9) (11 arguments)
constexpr u32 Ui_Find = 0x004D5544;                // (const char* name, 0) -> widget or 0
constexpr u32 World_SetFlags = 0x00599024;         // (u32)
constexpr u32 SortedMap_Find786C0C = 0x00786C0C;   // (map, const u32* key) -> slot
}  // namespace b18fn

}  // namespace big18
}  // namespace sly4
