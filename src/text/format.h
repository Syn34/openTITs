// UiText_Format: expands a text element's string (escapes, $string-table
// variables) into a buffer and word-wraps it to the element's width.
#pragma once

#include "text.h"

namespace sly4 {
namespace text {

// The state UiText_Format keeps through pointers, so that the recursion for
// $variables carries on where the outer call is (SetText passes 8 u32s).
struct FormatState {
    gptr out;        // o0: write cursor in the buffer (starts as the buffer)
    be_u32 bytes;    // o1: bytes written
    be_u32 glyphs;   // o2: characters written (glyph quads needed)
    be_u32 on_line;  // o3: characters on the current line
    be_f32 width;    // o4: width of the current line
    gptr brk;        // o5: where the last break opportunity is in the buffer, or 0
    be_f32 brk_width;  // o6: line width at that break
    be_u32 last;     // o7: last character handled
};

constexpr u32 kStrings = 0x00F6687C;      // Strings*: the string table $variables come from (0 = none)
constexpr u32 kFormatDepth = 0x00F668CC;  // s32: recursion depth, at most 10
constexpr u32 kMaxDepth = 10;
constexpr u32 kConfigLanguage = 0x28;     // g::config u32: 18 selects CJK line breaking (name?)
constexpr u32 kLanguageCjk = 18;
constexpr u32 kTextWrapShift = 6;         // kTextFlags bits 6..21: wrap width in pixels, 0 = none
constexpr u32 kNoBreakAfter = 0x00AEF78C;   // u32[11] sorted: opening brackets
constexpr u32 kNoBreakAfterCount = 11;
constexpr u32 kNoBreakBefore = 0x00AEF6C8;  // u32[49] sorted: closing punctuation, small kana
constexpr u32 kNoBreakBeforeCount = 49;
constexpr u32 kDepthMessage = 0x00AEF4A0;   // "" (a stripped message: the element path is appended)
constexpr u32 kVarNameSize = 64;            // the original's buffer (not bounds-checked)

}  // namespace text
}  // namespace sly4
