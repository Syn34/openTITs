// Text element per-frame updates: glyph cell animation, the app-level
// class's typewriter reveal, and the shared child update.
#pragma once

#include "slots.h"
#include "../text/glyph_build.h"

namespace sly4 {
namespace uielem {

namespace ufn2 {
constexpr u32 UiQuad_Update = 0x004E1454;
constexpr u32 UiText_UpdateQuads = 0x004E23A0;
constexpr u32 UiElem_UpdateChildren = 0x004D89CC;
constexpr u32 UiText_BuildGlyph = 0x004DAAA8;
constexpr u32 UiText_SpaceWidth = 0x004DB1E8;
constexpr u32 UiText_MeasureLines = 0x004DAEE8;
constexpr u32 memset = 0x00726E54;
}  // namespace ufn2

// Quad +16 bit-fields used by the cell animation (see glyph_build.h).
constexpr u32 kQuadCell = 11;      // 4 bits: current cell
constexpr u32 kQuadCells = 15;     // 4 bits: number of cells
constexpr u32 kQuadPingPong = 10;  // 1 bit: direction for kCellPingPong
constexpr u32 kQuadTimer = 28;     // f32 seconds since the last step (text::kQuad28)

// Cell animation modes (text +180 bits 22-25).
enum CellMode : s32 {
    kCellLoop = 0,      // also any mode < 0 or > 4
    kCellOnce = 1,      // forward, stops on the last cell
    kCellPingPong = 2,  // bit 10 holds the direction
    kCellLoopBack = 3,
    kCellOnceBack = 4,  // backward, stops on cell 0
};

// The app-level class's typewriter state (vtable 0x00B1C9D0).
constexpr u32 kTwTimer = 236;     // f32: counts down; a character appears each time it goes below 0
constexpr u32 kTwQuad = 240;      // u32: next quad to fill
constexpr u32 kTwCursor = 244;    // char*: next character of the formatted text
constexpr u32 kTwX = 248, kTwY = 252;  // f32 pen position
constexpr u32 kTwLine = 256;      // u32
constexpr u32 kTwPreview = 260;   // char*: text whose glyphs are previewed after the cursor (kText2Source)
constexpr u32 kTwPreviewQuads = 264;  // s32: quads after kTwQuad whose cells keep animating (kText2Glyphs)
constexpr u32 kTwPaused = 268;    // u32: nonzero = no update at all

}  // namespace uielem
}  // namespace sly4
