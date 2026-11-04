// Timeline keys and tracks: the small classes whose vtables follow the
// Timeline's (0x00B228A0) in rodata. A key's +8 runs it when the timeline
// reaches it: (Key*, Timeline*; f1 = delay) -> new Track* or 0.
#pragma once

#include "class.h"

namespace sly4 {
namespace uielem {

constexpr u32 kTlKeyVtbl = 0x00B228B0;       // abstract key: +8 pure virtual
constexpr u32 kTlTrackVtbl = 0x00B228C4;     // abstract track: +8 pure virtual
constexpr u32 kTlKeyNopVtbl = 0x00B228D8;    // key that does nothing
constexpr u32 kTlKeyAnimVtbl = 0x00B228F0;   // key that starts a 48-byte track on an element
constexpr u32 kTlKeyEventVtbl = 0x00B22908;  // key that sends a UI event

constexpr u32 kTlElement = 8;         // Timeline: u32 Element* its paths are relative to
constexpr u32 kKeyAnimPath = 28;      // char[]: element path (Ui_Find)
constexpr u32 kKeyEventType = 12;     // u32 event type, 0 = none
constexpr u32 kKeyEventPath = 16;     // char[]: element path
constexpr u32 kTlTrackSize = 48;

namespace tlfn {
constexpr u32 Ui_Find = 0x004D5544;           // (const char* path, Element* parent) -> Element*
constexpr u32 TlTrack_Construct = 0x004E3790; // (Track*, Key*, Timeline*, Element*; f1 = delay) (name?)
}  // namespace tlfn

constexpr u32 kUiEventHandlerOpd = 0x00F66874;  // u32 OPD*: (UiEventBlock*) -> u32
constexpr u32 kUiEventHandlerArg = 0x00F66878;

}  // namespace uielem
}  // namespace sly4
