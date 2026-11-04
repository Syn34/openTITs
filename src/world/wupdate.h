// Names used by World_Update (World vfunc +16, 0x00469D98) and the helpers
// it calls out of line (src/world/wupdate_fx.cpp). Addresses are guest
// addresses in the BCES01284 v01.02 EBOOT; "?" marks a guess.
#pragma once

#include <bit>
#include <cmath>

#include "vt_world.h"

namespace sly4 {
namespace wupdate {

// Virtual call. The original's call sequence leaves the OPD and its entry
// in the two registers after the arguments; we do the same.
inline u64 wcall(PPUContext& ctx, u32 obj, u32 slot, std::initializer_list<u64> args) {
    const u32 opd = load<u32>(load<u32>(obj) + slot);
    ctx.gpr[3 + args.size()] = opd;
    ctx.gpr[4 + args.size()] = load<u32>(opd);
    return call_ptr(ctx, opd, args);
}

// Ui vfunc +56 SetColor(ui, u8 rgba[4] by value). The original loads the
// colour into r4 over the OPD, so it leaves the entry in r5 and the last
// colour byte it loaded in r6.
inline void Ui_SetColor(PPUContext& ctx, u32 ui, u32 rgba, u32 r6) {
    const u32 opd = load<u32>(load<u32>(ui) + 56);
    ctx.gpr[5] = load<u32>(opd);
    ctx.gpr[6] = r6;
    call_ptr(ctx, opd, {ui, rgba});
}

inline u32 rgba_at(u32 ea) { return load<u32>(ea); }  // 4 colour bytes as one big-endian word

// ---------------------------------------------------------------- functions
namespace wfn {
constexpr u32 World_RequestState = 0x004665C0;   // decompiled in wupdate_fx.cpp
constexpr u32 Cutscene_ApplyCamera = 0x00468760;  // "
constexpr u32 MsgDialog_Open = 0x00468944;        // "
constexpr u32 World_UpdateAudioFilter = 0x00468A38;  // "
constexpr u32 Fade_Start = 0x00468C40;            // "
constexpr u32 PostFx_UpdateFilter = 0x00468EF8;   // "
constexpr u32 PostFx_UpdateTint = 0x004691E8;     // "
constexpr u32 PostFx_UpdateSwap = 0x004695A8;     // "
constexpr u32 PostFx_UpdateDof = 0x004697D8;      // "
constexpr u32 Fade_Update = 0x00469B90;           // "

constexpr u32 StateMachine_Update = 0x00464BD4;
constexpr u32 StateMachine_SetState = 0x00464D1C;
constexpr u32 Ui_Find = 0x004D5544;               // (name, parent) -> Ui*
constexpr u32 Ui_BringToFront = 0x004E02DC;       // (ui, 1): moves it in its sibling list (name?)
constexpr u32 String_Format = 0x005534D0;         // (fmt, ...) -> static buffer
constexpr u32 StringTable_Lookup = 0x004F87C4;    // (Strings*, id, fallback, quiet)
constexpr u32 Sysutil_CheckCallback = 0x0056C184;
constexpr u32 Audio_SetPaused = 0x0044BA6C;       // (bool)
constexpr u32 Display_GetSize = 0x004D4104;       // (f32 out[2]): config u16 +4, +6 as floats
constexpr u32 Handle_Resolve = 0x00365E30;        // (handle) -> object, 0 unless object +4 has bit 2 (name?)
constexpr u32 SaveData_Write = 0x000AE784;        // (path, data, size) (name?)
constexpr u32 Audio_SetFilterScale = 0x0044D740;  // (f32) (name?)
constexpr u32 Audio_IsEmitterValid = 0x004437CC;  // (u32 handle) -> bool (name?)
constexpr u32 Audio_SetEmitterFilter = 0x0044CCF8;  // (u32 handle, f32 cutoff) (name?)
constexpr u32 View_SetMatrix = 0x005201B0;        // (View*, const f32[16]) ("RenderTarget_SetMatrix")
constexpr u32 View_SetFov = 0x00520368;           // (View*, f32) (name?)
constexpr u32 Ease = 0x00787408;                  // (f32 t, f32 shape) -> f32 (name?)

// Called once per World_Update, still recompiled (names unknown).
constexpr u32 sub_0042A8B0 = 0x0042A8B0;  // (World*, f32 dt)
constexpr u32 sub_00787070 = 0x00787070;  // (World*, f32 dt)
constexpr u32 sub_00787190 = 0x00787190;  // (World*, f32 dt)
constexpr u32 sub_007872A8 = 0x007872A8;  // (World*, f32 dt)
constexpr u32 sub_007D3B80 = 0x007D3B80;  // () -> object; its vfunc +64 runs when World +45 bit 1 is set
constexpr u32 sub_003655D0 = 0x003655D0;  // (f32 0.0): while the world clock is stopped
constexpr u32 sub_000AC698 = 0x000AC698;  // ()
constexpr u32 sub_000AC83C = 0x000AC83C;  // () -> u32 kept at Game +0x5E38
constexpr u32 sub_000AC39C = 0x000AC39C;  // ()
constexpr u32 sub_004E3040 = 0x004E3040;  // (0, 0)
constexpr u32 sub_0036CA78 = 0x0036CA78;  // (object)
constexpr u32 sub_0036CB58 = 0x0036CB58;  // (object)
constexpr u32 sub_0036CC28 = 0x0036CC28;  // (object)
constexpr u32 sub_004A7EC0 = 0x004A7EC0;  // (object)
constexpr u32 sub_0044C43C = 0x0044C43C;  // (u32 hash) -> handle or 0 (name?)
constexpr u32 sub_0044D5BC = 0x0044D5BC;  // (handle, f32)
}  // namespace wfn

// cellSysutil import stubs.
namespace dlg {
constexpr u32 cellMsgDialogOpen2 = 0x00ABDB04;
constexpr u32 cellMsgDialogAbort = 0x00ABDAE4;
constexpr u32 cellMsgDialogClose = 0x00ABDA24;
// CELL_MSGDIALOG_TYPE_* bits.
constexpr u32 kSeNormal = 0x1, kBgInvisible = 0x4, kButtonYesNo = 0x10, kButtonOk = 0x20;
constexpr u32 kDisableCancel = 0x80, kDefaultCursorNo = 0x100;
}  // namespace dlg

// ---------------------------------------------------------------- globals
constexpr u32 kCurrentWorld = 0x00F65CA0;   // u32 World*
constexpr u32 kFadeUi = 0x00F65C88;         // u32 Ui* of "Common.Fade"
constexpr u32 kStrings = 0x00F6687C;        // u32 Strings* (StringTable_Lookup)
constexpr u32 kDialogState = 0x010BB850;    // u32: 3 while MsgDialog_Open's dialog is up
constexpr u32 kDialogCallback = 0x00F39D98;  // OPD given to cellMsgDialogOpen2
constexpr u32 kAudioFocus = 0x010AFEA4;     // u32 emitter handle (game/frame.h kAudioFocusHandle)
constexpr u32 kNpPromptState = 0x00F65C94;  // s32: 0 raise "must be signed in", 1 shown, 2 done (name?)
constexpr u32 kPadPromptState = 0x00F65C98;  // s32: same for "no controllers"; 3 = wait for a pad (Game_WatchPads)
constexpr u32 kCursorMask = 0x010B7830;     // u32: bit per player whose cursor is shown (Game +0x5E50)
constexpr u32 kHandleA = 0x00F64CF0, kHandleB = 0x00F64CF4, kHandleC = 0x00F64CFC, kHandleD = 0x00F64D00;  // (name?)
constexpr u32 kFlag66FC0 = 0x00F66FC0;      // u8 cleared when the world reaches state 3 (name?)
constexpr u32 kFlag88958 = 0x00F88958;      // u8 "
constexpr u32 kFlag9CDE2 = 0x00F9CDE2;      // u8 set by phase 2 (name?)
constexpr u32 kPending9CDE8 = 0x00F9CDE8;   // u64: phase 15 waits while nonzero (name?)
constexpr u32 kGame = 0x010B19E0;
constexpr u32 kGameValue5E38 = 0x5E38;      // u32 from sub_000AC83C
constexpr u32 kGameFlag5E3C = 0x5E3C;       // u8
constexpr u32 kGameFlags5E40 = 0x5E40;      // u8, bit 0x80
constexpr u32 kGamePlayers = 0xB0, kPlayerSize = 0x2EC0, kPlayerCount = 2;
constexpr u32 kPlayerFlags = 4;             // u32: 0x00800000 = cursor active (name?)
constexpr u32 kPlayerCursor = 32;           // f32 x, y in -1..1

// Tuning objects (value at +72).
constexpr u32 kTuning = 72;
constexpr u32 kTuneFilterLow = 0x010B9DC0;  // f32 filter cutoff while slowed (name?)
constexpr u32 kTuneFilterTime = 0x010B9E0C;  // f32 seconds to reach it (name?)
constexpr u32 kTuneDofScale = 0x010B9868;   // f32
constexpr u32 kTuneMaxFps = 0x010B7A5C;     // s32: the world step is at least 1 / this (name?)
constexpr u32 kTuneMinFps = 0x010B7A10;     // s32: the world step is at most 1 / this (name?)
constexpr u32 kTuneIntroTime = 0x010B7DEC;  // f32: phase 5 waits this long (name?)
constexpr u32 kTuneCursorColorOn = 0x010B7D54;   // u8[4] colour of a player's cursor (mask bit set)
constexpr u32 kTuneCursorColorOff = 0x010B7DA0;  // u8[4]
constexpr u32 kTuneFadeTime = 0x010B7C24;   // f32
constexpr u32 kTuneFlagA = 0x010B89D0;      // u8 (name?)
constexpr u32 kTuneFlagB = 0x010B8A1C;      // u8 (name?)

// ---------------------------------------------------------------- World fields
constexpr u32 kWorldSaveFlags = 44;      // u8: 0x80 save enabled (World_Destruct)
constexpr u32 kWorldFlags45 = 45;        // u8: 0x02 = run sub_007D3B80()->vfunc +64 once
constexpr u32 kWorldFlags28 = 28;        // u8: 0x20 (name?)
constexpr u32 kWorldUserCount = 29;      // u8
constexpr u32 kWorldUserFilter = 30;     // u8: only this user's cursor (255 = all)
constexpr u32 kWorldSubState = 31;       // u8
constexpr u32 kWorldCurState = 47;       // u8: level-0 state last entered
constexpr u32 kWorldNextState = 48;      // u8: level-0 state (GetState(0) reads it)
constexpr u32 kWorldNextSub = 49;        // u8: level-1 state to enter with it
constexpr u32 kWorldPhase = 50;          // u8: transition phase, 0 = none
constexpr u32 kWorldPhaseTime = 56;      // f32: time in the phase (stored as 0 words)
constexpr u32 kWorldTime = 60;           // f32: accumulated update time
constexpr u32 kWorldSaveData = 80, kWorldSaveSize = 4064;
constexpr u32 kWorldUsers = 0x1071;      // u8 player index of user i, stride 0x20C80
constexpr u32 kUserStride = 0x20C80;
constexpr u32 kWorldPlayerObj = 0x1078;  // u32 object followed by the camera (name?)
constexpr u32 kWorldViews = 0x128168;    // View*[4]
constexpr u32 kWorldPostFxFlags = 0x1281E0;  // u32 object, +468 bit 0x40000000 = no post effect active (name?)
constexpr u32 kWorldHudLock = 0x128214;  // u16: hides InGame.HUD while nonzero (name?)
constexpr u32 kWorldCutscene = 0x128224;  // u32 object: +4 delete, +16 Update(dt) -> bool, +60 f32 time scale
constexpr u32 kWorldCamera = 0x128230;   // f32 copy of the main view's matrix (view +816), rows of 3
constexpr u32 kWorldCamTarget = 0x128260;  // f32[3]: row 3, blended toward the player (name?)
constexpr u32 kWorldCamBlend = 0x128270;   // f32
constexpr u32 kWorldCamBlendTo = 0x128274, kWorldCamBlendFrom = 0x128278;  // f32
constexpr u32 kWorldCamBlendT = 0x12827C, kWorldCamBlendTime = 0x128280, kWorldCamBlendShape = 0x128284;
constexpr u32 kWorldFilterCutoff = 0x128288;  // f32
constexpr u32 kWorldFade = 0x128290;     // Fade (Fade_Start / Fade_Update)
constexpr u32 kWorldBusy = 0x1282A0;     // u32: 0x40000000 holds phases 1, 10 and 15 (name?)
constexpr u32 kWorldTimeScale = 0x1282AC;  // f32, applied while kWorldTimeScaleOn has 0x80000000
constexpr u32 kWorldTimeScaleOn = 0x1282BC;
constexpr u32 kWorldAudioScale = 0x1282C8;  // f32, applied while kWorldAudioScaleOn has 0x80000000
constexpr u32 kWorldAudioScaleOn = 0x1282D8;
constexpr u32 kWorldDof = 0x1282DC;      // PostFx_UpdateDof
constexpr u32 kWorldSwap = 0x128308;     // PostFx_UpdateSwap
constexpr u32 kWorldTint = 0x128334;     // PostFx_UpdateTint
constexpr u32 kWorldFilter = 0x1283A8;   // PostFx_UpdateFilter
constexpr u32 kWorldVectors = 0x128480, kWorldVectorCount = 13, kWorldVectorSize = 20;  // u16 count at +8
constexpr u32 kWorldPausedCallbacks = 0x128570;  // {opd, arg, -}[], u16 count at +8: run while paused in a cutscene
constexpr u32 kWorldSlowMo = 0x128588;   // {f32 scale, f32 time left (<0 = off), u8 on}[2], 12 bytes
constexpr u32 kWorldSkipSave = 0x1285A4;  // u8 (name?)
constexpr u32 kWorldPointer = 0x1285AC;  // u32 object: vfunc +144 (ret*) -> f32 x, y on screen (name?)
constexpr u32 kWorldTicker = 0x1286A4;   // u32 object: vfunc +8 with the frame time (name?)

// View / camera fields.
constexpr u32 kViewCamera = 656;   // u32 camera; +12 dof, +16 filter, +24 tint, +8 / +40 swap effects
constexpr u32 kViewMatrix = 816;   // f32[16] as rows of 4
constexpr u32 kViewPosition = 864; // f32[3]

// Post effect objects: vfunc +8 Enable(bool), +12 IsEnabled() -> bool.
constexpr u32 kFxEnable = 8, kFxIsEnabled = 12;

// UI vfuncs.
constexpr u32 kUiSetPosition = 28;  // (const f32 xyz[3])
constexpr u32 kUiGetColor = 52;     // (u8 out[4]) as a struct return: (out, ui)
constexpr u32 kUiIsVisible = 84;
constexpr u32 kUiSetVisible = 88;

// Fade (World +0x128290): a colour fade on the "Common.Fade" element.
constexpr u32 kFadeFrom = 0, kFadeTo = 4, kFadeDuration = 8, kFadeTime = 12, kFadeFlags = 16;
constexpr u32 kFadeHideWhenDone = 0x80000000u, kFadeRunning = 0x40000000u;

// Float constants of the original (0x00469D80).
constexpr f32 kZero = 0.0f, kOne = 1.0f, kHalf = 0.5f;
constexpr f32 kPhaseStep = 0.01f;   // 0x3C23D70A: phase timers count frames this way
constexpr f32 kEpsilon = 1e-5f;     // 0x3727C5AC
constexpr f32 kTwo = 2.0f;

// Post effects: the main view's camera, then the effect at `offset`.
inline u32 MainView() { return load<u32>(load<u32>(kCurrentWorld) + kWorldViews); }
inline u32 CameraFx(u32 view, u32 offset) { return load<u32>(load<u32>(view + kViewCamera) + offset); }

}  // namespace wupdate
}  // namespace sly4
