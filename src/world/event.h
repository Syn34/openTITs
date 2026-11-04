// World_HandleEvent (World vfunc +36, 0x00491710, 8211 instructions, still
// recompiled) and the small helpers it calls out of line
// (src/world/event_small.cpp). Guest addresses in the BCES01284 v01.02
// EBOOT; "?" marks a guess.
//
// Structure (no jump tables; compare chains on the event and on GetState):
//   type < 0, type 2 (update; the World overrides Update): return 1.
//   type 0 enter (+4 level, +12 new id):
//     level 0 -> World_EnterLevel0 0x0047C8A0 (1481 instructions): new id 0 ->
//       0x0047D10C, 2 or 3 -> 0x0047D17C, other ids (1) load the level UI
//       ("Scripts/UI/%s.xml", "UI_%s", "Camera_Default", spline lists); return 1.
//     level 1, GetState(0) == 0: +30 (user filter) = 0, then by GetState(1):
//       7 -> 0x00491814 (session / user setup, 0x003568E0 on g 0x00F64D14),
//       8, 12, 14, 15, 13, 16..20, 22..25 -> the chain at 0x00499150..0x004996CC.
//     level 1, GetState(0) == 1 and GetState(1) == 6 -> per player
//       sub_004644E0 / sub_0046452C(player, 1) (0x00499114).
//     other levels: return 1.
//   type 1 exit (0x0049235C): level <= 1 -> by GetState(0) / GetState(1)
//     (0x00492378..); level > 1 -> 0x00492450.
//   type 3 tick (0x00491A3C, f32 dt at +4): per-user camera update over the
//     9 user blocks (Camera_IsSplit; tuning 0x010F0E20 value = split mode); then by
//     GetState(0) at 0x00491C10:
//       0 -> 0x00491C38 (front end: +16 / +20 checks, view
//          +0x1281E0 vfuncs, "InGame" / "Common" UI packages at 0x00492AF8),
//       1 -> 0x0049366C (in game: GetState(1) chains 0, 2..6, 12; hideout
//          prompt "$HUD_PROMPT_HIDEOUT", "InGame.Menus.Summary", gadget grid,
//          "Camera_Default"),
//       2, 3 -> 0x0049311C (loading: GetState(1) > 0 -> 0x0049373C; "Scripts/Tuning.xml",
//          "Default_Init" / "Default_Shutdown", the UI packages "Common", "InGame",
//          "FrontEnd", "LevelEditor", "ArtViewer", "BinkSubtitles"),
//       > 3 (255 = none) -> 0x00491F20 (common tail).
//     Level 2 and 3 sub-states drive the boot flow: save / settings load
//     errors ("STR_LOAD_SAVE_FAILED", "STR_LOAD_SETTINGS_FAILED",
//     "STR_NOT_YOUR_SETTINGS"), the splash movie "Movies/SanzaruSplash.bik",
//     "GUI/Screens/Sony_%s.tga" and "INITIAL_SAVING_SETTINGS" (0x00497E20..0x00498FF0).
//   Most paths end at 0x00499064 (return r3; r30 holds the result after
//   World_RequestState 0x004665C0 tails at 0x00499040 / 0x00499054).
//
// World level-0 states as far as the code shows (name?):
//   0 = front end / title, 1 = in game (a level is loaded), 2 and 3 = loading
//   (3 clears the flags 0x00F66FC0 / 0x00F88958 in World_Update), 255 = none.
#pragma once

#include "vt_world.h"

namespace sly4 {
namespace wevent {

namespace kfn {
constexpr u32 World_EnterLevel0 = 0x0047C8A0;  // (World*, Event*) (name?)
constexpr u32 sub_003A1500 = 0x003A1500;
constexpr u32 sub_004545B0 = 0x004545B0;
constexpr u32 sub_00010238 = 0x00010238;       // (u32)
constexpr u32 sub_000D1288 = 0x000D1288;       // (Object*, u32 message, 0) (name?)
constexpr u32 sub_002A3AD0 = 0x002A3AD0;
constexpr u32 sub_0036CDA8 = 0x0036CDA8;       // (Object*, Object*)
}  // namespace kfn

// Globals touched by the helpers.
constexpr u32 kSingleton11DF320 = 0x011DF320;  // u32 object (name?)
constexpr u32 kGlobalF64A68 = 0x00F64A68;      // u32 (name?)
constexpr u32 kListF9CDA8 = 0x00F9CDA8;        // vector, u16 count at +8 (name?)
constexpr u32 kGlobalF558D4 = 0x00F558D4;      // u32 (name?)
constexpr u32 kFlagF558E1 = 0x00F558E1;        // u8 (name?)
constexpr u32 kGlobalF558E4 = 0x00F558E4;      // u32 object: +672 -> +516 (name?)
constexpr u32 kGlobalF55904 = 0x00F55904;      // u32 (name?)
constexpr u32 kGlobalF557EC = 0x00F557EC;      // u32 (name?)
constexpr u32 kGlobal10B0100 = 0x010B0100;     // u32 (name?)
constexpr u32 kGlobal10592E8 = 0x010592E8;     // u32: sub_002A3AD0 creates it? (name?)
constexpr u32 kDisplayConfig = 0x010C5A20;     // +4 u16 width, +6 u16 height, +40 mode
constexpr u32 kTrophy = 0x00F668E8;            // u32 trophy manager (game/meta.h)

// World fields.
constexpr u32 kWorldBusy = 0x1282A0;           // u32: 0x40000000 = busy
constexpr u32 kWorldTimeScaleOn = 0x1282BC;    // u32: 0x80000000 = time scale active
constexpr u32 kWorldAudioScaleOn = 0x1282D8;   // u32: 0x80000000 = audio scale active
constexpr u32 kWorldObj128164 = 0x128164;      // u32 object (+5444 state) (name?)

}  // namespace wevent
}  // namespace sly4
