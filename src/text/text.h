// Text elements: setting their text, UTF-8 decoding, measuring lines and
// laying out glyph quads, plus two string helpers the text code uses.
// Addresses are guest addresses in the BCES01284 v01.02 EBOOT. A "?" in a
// comment marks a guess from how the code is used.
#pragma once

#include "../sly4.h"

namespace sly4 {
namespace text {

namespace tfn {
// decompiled in src/text/*.cpp
constexpr u32 UiText_SetText = 0x004DC1B8;       // (TextElem*, const char*, bool relayout)
constexpr u32 UiText_Layout = 0x004DB378;        // (TextElem*, bool force)
constexpr u32 Utf8_Next = 0x004DA380;            // (this, const char** p, u32* len or 0, bool advance) -> code point
constexpr u32 UiText_SpaceWidth = 0x004DB1E8;    // (TextElem*, const f32* scale) -> f1
constexpr u32 UiText_MeasureLines = 0x004DAEE8;  // (TextElem*, f32* widths, u32, u32, const f32* scale; f1) -> lines
constexpr u32 Ui_BuildPath = 0x004D3700;         // (Element*, char** cursor)
constexpr u32 StringTable_Lookup = 0x004F87C4;   // (Strings*, const char* id, const char* fallback, bool quiet)
// still recompiled
constexpr u32 UiText_Format = 0x004DB828;        // 612 instructions: expands the text into a buffer (name?)
constexpr u32 UiText_BuildGlyph = 0x004DAAA8;    // 270 instructions, FP + bit-packed quads (name?)
// elsewhere
constexpr u32 SmallAlloc = 0x0055238C;           // (u32 size)
constexpr u32 SmallFree = 0x00552420;            // (void* p, u32 size)  (symbols.txt: Pool_FreeSized)
constexpr u32 String_Assign = 0x00555F80;        // strcpy
constexpr u32 String_Length = 0x00555FA4;        // -> s32
constexpr u32 String_Printf = 0x00551540;        // sprintf
constexpr u32 String_Format = 0x005534D0;        // rotating static buffer
}  // namespace tfn

// ---------------------------------------------------------------- text element (UiText, vtable 0x00B227D8)
constexpr u32 kElemFlags = 136;      // u32: 0x80000000 active; bits 14-15 h-align, 18-19 v-align (0 / 1 / 2)
constexpr u32 kElemParent = 148;     // Element*
constexpr u32 kElemName = 96;        // char[]
constexpr u32 kTextFontSet = 168;    // FontSet*
constexpr u32 kTextLineSpacing = 172;  // f32
constexpr u32 kTextCharSpacing = 176;  // f32
constexpr u32 kTextFlags = 180;      // u32: kTextAutoLayout, kTextNoGlyphs; bits 22-25 used by the glyph builder
constexpr u32 kTextBufCap = 184;     // u32 bytes at kTextBuf
constexpr u32 kTextQuadCap = 188;    // u32 glyphs kTextQuads can hold
constexpr u32 kTextBuf = 192;        // char*: the formatted text
constexpr u32 kTextSourceCap = 196;  // u32 bytes at kTextSource
constexpr u32 kTextSource = 200;     // char*: the text as given
constexpr u32 kTextQuadBytes = 204;  // u32
constexpr u32 kTextQuads = 208;      // 32-byte glyph quads
constexpr u32 kTextGlyphCount = 216; // f32
constexpr u32 kTextLineCount = 220;  // f32
constexpr u32 kText224 = 224;        // f32: when > 0 the counts above are zeroed (name?)
constexpr u32 kTextAutoLayout = 0x20000000;
constexpr u32 kTextNoGlyphs = 0x40000000;  // every character measures as unknown; cleared by Layout (name?)
constexpr u32 kElemActive = 0x80000000;
constexpr u32 kQuadSize = 32;

// Virtual functions (byte offsets into the vtable).
constexpr u32 kVfnTextChanged = 148;  // ()
constexpr u32 kVfnGetScale = 168;     // sret f32[2] (x, y) (name?)

// ---------------------------------------------------------------- fonts
constexpr u32 kSetScale = 92;        // f32
constexpr u32 kSetLineScale = 96;    // f32
constexpr u32 kSetFontCount = 100;   // u32
constexpr u32 kSetFonts = 104;       // Font*[]
constexpr u32 kSetFontScale = 168;   // f32[] per font
constexpr u32 kSetFontKerning = 232; // f32[] per font
constexpr u32 kFontFirst = 96;       // u32 first code point (no map)
constexpr u32 kFontRange = 100;      // u8 number of code points (no map)
constexpr u32 kFontLineHeight = 102; // u16
constexpr u32 kFontGlyphs = 104;     // Glyph[] of 8 bytes: +2 u8 advance
constexpr u32 kFontMap = 108;        // u8[65536] code point -> glyph, 255 = none; 0 = use first/range
constexpr u32 kGlyphSize = 8, kGlyphAdvance = 2;
constexpr u32 kNoGlyph = 255;

constexpr u32 kZeroVec2 = 0x01164AFC;  // f32[2] of zeros

inline u32 vfunc(u32 obj, u32 byte_offset) { return load<u32>(load<u32>(obj) + byte_offset); }
inline u32 call32(PPUContext& ctx, u32 addr, std::initializer_list<u64> args = {}) {
    return static_cast<u32>(call(ctx, addr, args));
}

// The lookup every text function inlines: the first font with a glyph for
// `c`. `index` is (u32)-1 when there is none.
struct GlyphRef {
    u32 index;  // font index
    u32 glyph;  // glyph index in that font
};
inline GlyphRef FontSet_FindGlyph(u32 set, u32 c) {
    const u32 count = field<u32>(set, kSetFontCount);
    for (u32 i = 0; i < count; ++i) {
        const u32 font = load<u32>(set + kSetFonts + i * 4);
        if (field<u32>(font, kFontGlyphs) == 0) continue;
        const u32 map = field<u32>(font, kFontMap);
        if (map != 0) {
            if (c > 0xFFFF) continue;
            const u32 g = load<u8>(map + c);
            if (g == kNoGlyph) continue;
            return {i, g};
        }
        const u32 first = field<u32>(font, kFontFirst);
        if (c < first) continue;
        if (c < first + ref<u8>(font + kFontRange)) return {i, c - first};
    }
    return {0xFFFFFFFFu, 0xFFFFFFFFu};
}

// FPU single-precision operations as the PPU does them (fmuls / fadds / fmadds).
inline f64 fmuls(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a * b)); }
inline f64 fadds(f64 a, f64 b) { return static_cast<f64>(static_cast<f32>(a + b)); }
inline f64 fmadds(f64 a, f64 c, f64 b) { return static_cast<f64>(static_cast<f32>(std::fma(a, c, b))); }
inline f64 lfs(u32 ea) { return static_cast<f64>(load<f32>(ea)); }
inline void stfs(u32 ea, f64 v) { store<f32>(ea, static_cast<f32>(v)); }

}  // namespace text
}  // namespace sly4
