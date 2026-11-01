// Names used by Game_Update (0x0045B858) and the small functions it calls.
// Addresses are guest addresses in the BCES01284 v01.02 EBOOT. A "?" in a
// comment marks a guess from how the code is used.
#pragma once

#include "../sly4.h"

namespace sly4 {
namespace update {

// ---------------------------------------------------------------- functions
// Called by Game_Update and still recompiled (or owned by another file).
namespace ufn {
constexpr u32 Ui_Find = 0x004D5544;            // (const char* name, Element* parent or 0) -> Element* (name?)
constexpr u32 String_Format = 0x005534D0;      // (const char* fmt, ...) -> static buffer (name?)
constexpr u32 FrameArena_Align = 0x0055329C;   // (Arena*)  decompiled in update_misc.cpp
constexpr u32 Input_SetMenuMode = 0x0044BA6C;  // (bool)    decompiled in update_misc.cpp
constexpr u32 Input_GetMenuMode = 0x0044BB3C;  // () -> u8  decompiled in update_misc.cpp
constexpr u32 Ui_SetInputHandler = 0x004D5048;  // (opd, arg) decompiled in update_misc.cpp
constexpr u32 Ui_SetEventHandler = 0x004D505C;  // (opd, arg) decompiled in update_misc.cpp
constexpr u32 Ui_SendEvent = 0x004D3DA4;        // (type, a, b, c) -> handler result or 1  (update_misc.cpp)
constexpr u32 Movie_SetPaused = 0x004D2284;     // (Movie*, bool)  (update_misc.cpp)
constexpr u32 Triggers_Clear = 0x000D0628;      // ()  (update_misc.cpp)
constexpr u32 FrameBuffer_End = 0x00510544;     // (FrameBuffer*)  decompiled in update_misc.cpp
constexpr u32 Systems_EndFrame = 0x005DBB30;    // () -> 1 (name?)  (update_misc.cpp)
constexpr u32 DebugHookA = 0x004A8AC8;          // () empty in retail  (update_misc.cpp)
constexpr u32 DebugHookB = 0x004A8ACC;          // (u32) empty in retail  (update_misc.cpp)
constexpr u32 Bink_Pause = 0x007B4990;          // (HBINK, s32 pause): RAD BinkPause (name?)
constexpr u32 Time_BeginFrame = 0x005DB6B8;     // () -> 1  decompiled in update_misc.cpp
constexpr u32 FrameBuffer_Begin = 0x0050FF64;   // (FrameBuffer*)  decompiled in update_misc.cpp
constexpr u32 Input_UpdateDevices = 0x005630D4;  // ()  decompiled in update_misc.cpp
constexpr u32 sub_0045B1E8 = 0x0045B1E8;        // (Player*, f32 dt) per-player update (name?)
constexpr u32 sub_004D4538 = 0x004D4538;        // (f32 dt) UI update? (name?)
constexpr u32 sub_0044B310 = 0x0044B310;        // (World* + 0x128230, f32 dt)
constexpr u32 sub_0045A9B8 = 0x0045A9B8;        // (World*)
constexpr u32 FlowState_GetMovieOwner = 0x00458D6C;  // (Session*) -> object with a movie, or 0 (update_misc.cpp)
constexpr u32 sub_005200AC = 0x005200AC;        // (View*, const ViewRect*)  (name?)
constexpr u32 sub_005201B0 = 0x005201B0;        // (View*, const float[16])  (name?)
constexpr u32 sub_000D0344 = 0x000D0344;        // () used by Triggers_Clear
constexpr u32 sub_006D530C = 0x006D530C;        // (u32 bits) turned on  ("src/common/api.cpp")
constexpr u32 sub_006D53F0 = 0x006D53F0;        // (u32 bits) turned off ("src/common/api.cpp")
constexpr u32 sub_0055E284 = 0x0055E284;
constexpr u32 sub_0050FDC8 = 0x0050FDC8;
}  // namespace ufn

// ---------------------------------------------------------------- globals
constexpr u32 kPrevMainState = 0x00F65BEC;  // u32: g::main_state at the start of this frame
constexpr u32 kAppliedViewW = 0x00F65DE8;   // u16: config +4 when the viewport was last applied
constexpr u32 kSession = 0x00F65CA0;        // u32: Session* (local users) (name?)
constexpr u32 kMainView = 0x00F6684C;       // u32: View* the viewport and matrix go to (name?)
constexpr u32 kViewMatrix = 0x01164BD0;     // float[16], rows copied without their w
constexpr u32 kDebugStall = 0x010BB8FC;     // u8: stall random frames for 16 ms (debug option?)
constexpr u32 kRandomLast = 0x01164888;     // u64: low 16 bits of the last random number
constexpr u32 kRandomState = 0x01164890;    // u32: seeded by main with the time base
constexpr u32 kDebugHookArg = 0x010C2C28;   // u32 passed to DebugHookB

// One step of the game's small random generator (inlined in the original
// wherever it's used): state = rotl16((rotl4(s) + rotl16(s)) ^ 0x87654321),
// with the low 16 bits of the mix kept at kRandomLast. Returns the state as
// it was before the step.
inline u32 Random_Step() {
    const auto rotl = [](u32 v, int n) { return (v << n) | (v >> (32 - n)); };
    const u32 seed = load<u32>(kRandomState);
    const u32 mixed = (rotl(seed, 4) + rotl(seed, 16)) ^ 0x87654321u;
    store<u32>(kRandomState, rotl(mixed, 16));
    store<u64>(kRandomLast, mixed & 0xFFFFu);
    return seed;
}

// Time_BeginFrame (0x005DB6B8) and its SystemState fields.
constexpr u32 kSystemStateFrameCount = 0;  // u32, +1 every frame
constexpr u32 kSystemStateTotalTime = 16;  // f32, sum of the frame times
constexpr u32 kSystemStateHeap = 28;       // u32 Heap* (see Main_InitSystems)
constexpr u32 kSystemStateFlags = 32;      // u8: 0x40 = leave the heap alone (name?)
constexpr u32 kConfigFrameTime = 0x44;     // g::config f32: raw frame time (name?)
constexpr f32 kMaxFrameTime = 4.0f;        // divided by SystemState +12 (config +0x14)

// The system message (error / notice box). Whoever raises one sets code and active.
constexpr u32 kSysMsgActive = 0x010BA024;    // u32
constexpr u32 kSysMsgCode = 0x010BA028;      // s32
constexpr u32 kSysMsgText = 0x010BA02C;      // char[256], given to the text elements
constexpr u32 kSysMsgShows = 0x010BA12C;     // u8[code]: nonzero when that code shows the box
constexpr u32 kSysMsgCallback = 0x00F65C8C;  // u32 OPD or 0: called with the code while active

// UI handlers (set by Ui_SetInputHandler / Ui_SetEventHandler), and the
// copies Game_Update saves when the system message takes over.
constexpr u32 kUiInputHandler = 0x00F6686C, kUiInputArg = 0x00F66870;
constexpr u32 kUiEventHandler = 0x00F66874, kUiEventArg = 0x00F66878;
constexpr u32 kSavedUiInputHandler = 0x010BB9E4, kSavedUiInputArg = 0x010BB9E8;
constexpr u32 kSavedUiEventHandler = 0x010BB9EC, kSavedUiEventArg = 0x010BB9F0;
constexpr u32 kSavedFadeState = 0x010BB9F4;     // u8: Common.Fade vfunc +84 before it was hidden
constexpr u32 kSavedLoadingState = 0x010BB9F5;  // u8: Common.Loading vfunc +84
constexpr u32 kSysMsgInputHandler = 0x00F39BE0;  // OPD of 0x00464794
constexpr u32 kSysMsgEventHandler = 0x00F39BE8;  // OPD of 0x00464804
constexpr u32 kUiEventSystemMessage = 12;

// Controller watch (sets kSysutilPadLost when no local user's pad is connected?)
constexpr u32 kPadWatchArmed = 0x00F65DEA;      // u8: 1 while watching for a disconnect
constexpr u32 kPadWatchState = 0x00F65C98;      // s32: 3 = waiting for a pad to come back?
constexpr u32 kPadLostCountdown = 0x00F65DEC;   // s32: frames left before pausing, -1 = idle

// Input_SetMenuMode / Input_GetMenuMode
constexpr u32 kMenuMode = 0x010AF1BD;         // u8
constexpr u32 kMenuModeReady = 0x010AEC60;    // u8: nothing is applied until this is set
constexpr u32 kMenuModeDirty = 0x00F65B88;    // u8: set to 1 by every Input_SetMenuMode
constexpr u32 kMenuModeExtraBits = 0x010AF1C0;  // u32, or-ed into the mask
constexpr u32 kMenuModeBit2 = 0x010AF1C4;     // u8: adds bit 2 to the mask
constexpr u32 kMenuModeMask = 0x00F65BE0;     // u32: mask last applied

// Triggers_Clear: three records of 20 bytes, u16 at +8.
constexpr u32 kTriggers = 0x01013B48;
constexpr u32 kTriggerStride = 20;

// ---------------------------------------------------------------- objects
// Game (g::game) fields used here.
constexpr u32 kGameWorld = 0x0000;        // u32 World* (vtable: +16 Update(f32 dt), +32 (0) -> 4 = quit)
constexpr u32 kGameSysMsgUi = 0x0004;     // u32 Element* "Common.System_Message", 0 while not shown
constexpr u32 kGameFrameArena = 0x00A0;   // u32 Arena* (FrameArena_Align)
constexpr u32 kGamePlayers = 0x00B0;      // Player[2]
constexpr u32 kPlayerSize = 0x2EC0;
constexpr u32 kPlayerPad = 28;            // u32 Pad*: u16 at +4, bit 0x8000 = connected?
constexpr u32 kGameCursors = 0x5E30;      // u32 Element*[2] "Common.Cursors.%02d", found lazily

// World fields.
constexpr u32 kWorldViews = 0x128168;     // u32 View*[4], may be 0
constexpr u32 kWorldUpdate2 = 0x128230;   // object given to sub_0044B310
constexpr u32 kWorldFrameRing = 0x1285E4; // u32 FrameRing* (see Main_RotateFrameRing)
constexpr u32 kFrameRingCurrent = 12;     // u32 FrameBuffer*
constexpr u32 kFrameRingInFrame = 20;     // u8

// Session fields.
constexpr u32 kSessionUserCount = 29;     // u8
constexpr u32 kSessionFlags = 45;         // u8: bit 0x40 = menu mode before the pause
constexpr u32 kSessionUsers = 0;          // users of 0x20C80 bytes from the start
constexpr u32 kSessionUserSize = 0x20C80;
constexpr u32 kUserPlayerIndex = 0x1071;  // u8 index into Game players

// UI element virtual functions (byte offsets into the vtable).
constexpr u32 kUiVfnType = 12;        // () -> 3 for text elements?
constexpr u32 kUiVfnState = 84;       // () -> u8 (visible?)
constexpr u32 kUiVfnSetVisible = 88;  // (bool)
constexpr u32 kUiVfnSetText = 188;    // (const char*, 0)
constexpr u32 kUiTypeText = 3;

constexpr u32 kWorldVfnUpdate = 16;  // (f32 dt)
constexpr u32 kWorldVfnPoll = 32;    // (0) -> u32
constexpr u32 kWorldPollQuit = 4;

// The SystemState (sly4.h) float at +4: frame time handed to every update.
constexpr u32 kSystemStateFrameTime = 4;

// Rectangle handed to sub_005200AC, built from config u16s +8, +10, +4, +6.
struct ViewRect {
    be_u16 x, y, width, height;  // (names?)
};
static_assert(sizeof(ViewRect) == 8);

// Argument block Ui_SendEvent hands to the event handler.
struct UiEvent {
    be_u32 type;
    be_u32 a;
    be_u32 handler_arg;
    be_u32 b;
    be_u32 c;
};
static_assert(sizeof(UiEvent) == 20);

}  // namespace update
}  // namespace sly4
