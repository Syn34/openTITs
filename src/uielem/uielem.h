// The base UI element class's small virtual functions (vtable 0x00B1C9D0;
// text elements 0x00B227D8 and type-5 elements 0x00B22B78 share them) and
// the system message box handlers Game_Update installs.
// Addresses are guest addresses in the BCES01284 v01.02 EBOOT. A "?" in a
// comment marks a guess from how the code is used.
#pragma once

#include "../sly4.h"

namespace sly4 {
namespace uielem {

// ---------------------------------------------------------------- element layout
constexpr u32 kElemMatrix = 16;     // f32[4][4], rows 16 bytes apart; row 3 xyz = kElemPosition
constexpr u32 kElemPosition = 64;   // f32[3]
constexpr u32 kElemScale = 80;      // f32[3] (name?)
constexpr u32 kElemColor = 132;     // u8[4] (RGBA?)
constexpr u32 kElemFlags = 136;     // u32 bit-fields, see ElemFlag

enum ElemFlag : u32 {
    kElemActive = 0x80000000,     // vfuncs +84 / +88 (visible?)
    kElemFocusable = 0x40000000,  // +92 / +96
    kElemFlag29 = 0x20000000,     // +100 / +104
    kElemFlag28 = 0x10000000,     // +108 / +112
    kElemFlag27 = 0x08000000,     // +116 / +120
    kElemFlag26 = 0x04000000,     // +124 / +128
    kElemFlag05 = 0x00000020,     // +132 / +136
    kElemFieldA = 0x00C00000,     // 2 bits, +60 / +64
    kElemFieldB = 0x003FC000,     // 8 bits, +68 / +72
    kElemFieldC = 0x00003FC0,     // 8 bits, +76 / +80
};

// Virtual functions by byte offset into the vtable.
constexpr u32 kVfnType = 12;        // () -> 100 base, 3 text, 5 (name?)
constexpr u32 kVfnSetVisible = 88;  // (bool)
constexpr u32 kTypeBase = 100, kTypeText = 3, kType5 = 5;

constexpr u32 kZeroVec2 = 0x01164AFC;  // f32[2] the base GetSize returns (also Tuning's vec2 default)

// ---------------------------------------------------------------- system message box
namespace sfn {
constexpr u32 Ui_Find = 0x004D5544;            // (path, parent) -> Element*
constexpr u32 Input_SetMenuMode = 0x0044BA6C;  // (bool)
constexpr u32 Ui_SendEvent = 0x004D3DA4;       // (type, a, b, c)
constexpr u32 Ui_SetInputHandler = 0x004D5048; // (opd, arg)
constexpr u32 Ui_SetEventHandler = 0x004D505C; // (opd, arg)
}  // namespace sfn

constexpr u32 kFrontEnd = 0x00AE8244;       // "FrontEnd"
constexpr u32 kInGame = 0x00AE8254;         // "InGame"
constexpr u32 kCommonFade = 0x00AE825C;     // "Common.Fade"
constexpr u32 kCommonLoading = 0x00AE826C;  // "Common.Loading"
constexpr u32 kSavedFadeState = 0x010BB9F4, kSavedLoadingState = 0x010BB9F5;  // u8 (see update.h)
constexpr u32 kSavedUiInputHandler = 0x010BB9E4, kSavedUiInputArg = 0x010BB9E8;
constexpr u32 kSavedUiEventHandler = 0x010BB9EC, kSavedUiEventArg = 0x010BB9F0;
constexpr u32 kSysMsgActive = 0x010BA024;  // u32
constexpr u32 kSession = 0x00F65CA0;       // u32: the World/Session object (update.h kSession)
constexpr u32 kSessionMenuMode = 31;       // u8: menu mode to restore
constexpr u32 kGameSysMsgUi = 4;           // Game +4: Element* of the open message box
constexpr u32 kGamePlayers = 0xB0, kPlayerSize = 0x2EC0, kPlayerCount = 2;
constexpr u32 kUiEventSysMsgClosed = 14;   // (update.h: 12 when it opens) (name?)
constexpr u32 kUiEventButton80 = 5;        // ui.h kUiEventButton80

// Player fields copied into a UiInput (see player.h / ui.h).
constexpr u32 kPlayerFlags = 4, kPlayerHeld = 8, kPlayerPressed = 12, kPlayerValid = 20;
constexpr u32 kPlayerCursorX = 32, kPlayerCursorY = 36;
constexpr u32 kPlayerCursorStarted = 0x00800000;
constexpr u32 kInputPointerValid = 0x30000;

inline u32 vfunc(u32 obj, u32 byte_offset) { return load<u32>(load<u32>(obj) + byte_offset); }

}  // namespace uielem
}  // namespace sly4
