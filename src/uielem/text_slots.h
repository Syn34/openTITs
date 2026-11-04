// Text element (vtable 0x00B227D8) destructor and update, the element
// tree unlinking they do, and the Timeline destructors (vtable 0x00B228A0,
// which follows the text vtable in memory).
#pragma once

#include "../text/text.h"

namespace sly4 {
namespace uielem {

namespace tsfn {
constexpr u32 UiText_Destruct = 0x004E1604;
constexpr u32 UiText_DestructThunk = 0x004E181C;  // calls UiText_Destruct
constexpr u32 UiText_CountGlyphs = 0x004E2178;
constexpr u32 UiQuad_Update = 0x004E1454;         // (Quad*; f1 dt, f2 = +232, r5 mode): 108 instructions (name?)
constexpr u32 SmallFree = 0x00552420;
constexpr u32 Mem_FreeFrom = 0x0054EE6C;
}  // namespace tsfn

constexpr u32 kTextVtbl = 0x00B227D8;
constexpr u32 kElemVtbl = 0x00B22710;      // the base element class
constexpr u32 kTimelineVtbl = 0x00B228A0;
constexpr u32 kUiRootList = 0x00F66868;    // first top-level element
constexpr u32 kUiTimelines = 0x00F668A0;   // Timeline*, linked by +28
constexpr u32 kUiFocus = 0x010C2AD8;       // Element*[4], cleared when the element goes (per user? name?)
constexpr u32 kUiFocusCount = 4;

constexpr u32 kElemFirstChild = 152, kElemNext = 156;
constexpr u32 kElemDetached = 0x01000000;  // +136: not in a parent's child list (name?)
constexpr u32 kElemSizeDirty = 0x02000000;
constexpr u32 kVfnDelete = 4;              // deleting destructor
constexpr u32 kVfnUpdate = 152;            // (f1 dt)
constexpr u32 kTextGlyphCountCached = 212; // u32: glyphs with a quad (UiText_CountGlyphs)
constexpr u32 kTextReveal = 224, kTextRevealElapsed = 228, kText232 = 232;
constexpr u32 kTextLayoutPending = 0x40000000;

// Timeline fields (see ui.h).
constexpr u32 kTlOwned = 8;     // u32 object deleted with the timeline when kTlOwns is set
constexpr u32 kTlOwns = 13;     // u8
constexpr u32 kTlKey = 16, kTlDelay = 20, kTlTracks = 24, kTlNext = 28;
constexpr u32 kTrackNext = 16;
constexpr u32 kTimelineSize = 32;

}  // namespace uielem
}  // namespace sly4
