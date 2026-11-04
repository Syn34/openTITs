// The World object (Game+0) and the hierarchical state machine it derives
// from. Guest addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include "../sly4.h"

namespace sly4 {
namespace world {

// StateMachine (vtable 0x00B219D0): a stack of `levels` state ids, each
// level with two words and a block of per-state data.
//   +0 vtable  +4 u16 levels  +6 u16 data size per level (16-aligned)
//   +8 u8 ids[levels] (255 = none)  +12 f32 time[levels]  +16 u32 frames[levels]
//   +20 data[levels][size] (0 without data)  +24 u32 bytes allocated
// All of +8.. is one SmallAlloc block.
constexpr u32 kSmVtable = 0x00B219D0;
constexpr u32 kSmLevels = 4, kSmDataSize = 6, kSmIds = 8, kSmA = 12, kSmB = 16, kSmData = 20, kSmBytes = 24;
constexpr u32 kSmNone = 255;

// Virtual slots (both vtables).
constexpr u32 kVfnDelete = 4;     // (this, flags)
constexpr u32 kVfnInit = 8;       // World: called once by Game_Load
constexpr u32 kVfnShutdown = 12;  // SetState(0, 255)
constexpr u32 kVfnUpdate = 16;    // (f32 dt) -> bool not handled; the World overrides it
constexpr u32 kVfnAdvance = 24;   // (u32 level): next id
constexpr u32 kVfnTick = 20;      // (f32 dt) -> bool not handled
constexpr u32 kVfnSetState = 28;  // (u32 level, u8 id) -> 1
constexpr u32 kVfnGetState = 32;  // (u32 level) -> u8 id
constexpr u32 kVfnEvent = 36;     // (Event*)

// Event passed to vfunc +36: +0 type, then per type:
//   0 enter / 1 exit: +4 level, +8 previous id, +12 new id
//   2 update / 3 tick: +4 f32 dt, +8 u8 handled
constexpr u32 kEventEnter = 0, kEventExit = 1, kEventUpdate = 2, kEventTick = 3;

// World (vtable 0x00B21A00, 0x1286B0 bytes): a StateMachine of 4 levels with
// 128 bytes of data each. Level 0's id is kept at +48 (GetState(0)).
constexpr u32 kWorldVtable = 0x00B21A00;
constexpr u32 kWorldState0 = 48;      // u8
constexpr u32 kWorldSubState = 31;    // u8 (FlowState_GetMovieOwner: 14/15)
constexpr u32 kWorldSubFlag = 32;     // u8

namespace vfn {
constexpr u32 StateMachine_Construct = 0x0045CAA0;  // (this, u16 levels, u32 data size, u8 initial id)
constexpr u32 StateMachine_Shutdown = 0x00464B90;   // vfunc +12
constexpr u32 StateMachine_Tick = 0x00464CBC;       // vfunc +20
constexpr u32 StateMachine_SetState = 0x00464D1C;   // vfunc +28
constexpr u32 World_GetState = 0x0046B5DC;          // vfunc +32
constexpr u32 World_Delete = 0x00467004;            // vfunc +4
constexpr u32 World_Init = 0x004682C8;              // vfunc +8
constexpr u32 World_Destruct = 0x00466860;          // vfunc +0 (488 instructions)
constexpr u32 World_Update = 0x00469D98;            // vfunc +16 (1552 instructions)
constexpr u32 World_HandleEvent = 0x00491710;       // vfunc +36 (8211 instructions)
constexpr u32 World_Construct = 0x0045CC18;         // (this, u8 initial) 6393 instructions
}  // namespace vfn

}  // namespace world
}  // namespace sly4
