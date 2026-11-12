// Text elements: setting their text, UTF-8 decoding, measuring lines and
// laying out glyph quads, plus two string helpers the text code uses.
// Addresses are guest addresses in the BCES01284 v01.02 EBOOT. A "?" in a
// comment marks a guess from how the code is used.
#pragma once

#include <cmath>
#include <cstddef>

#include "opentit/native.h"
#include "../sly4.h"
#include "../font/res.h"

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
constexpr u32 kTextFlags = 180;      // u32: kTextMonospace, kTextAutoLayout; bits 22-25 used by the glyph builder
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
constexpr u32 kTextAutoLayout = 0x40000000;  // layout pending: set by UiElem_Invalidate, cleared by Layout
constexpr u32 kTextMonospace = 0x80000000;  // every character advances by the space width
constexpr u32 kElemActive = 0x80000000;
constexpr u32 kQuadSize = 32;

// Virtual functions (byte offsets into the vtable).
constexpr u32 kVfnTextChanged = 148;  // ()
constexpr u32 kVfnGetScale = 168;     // sret scale, inherited from the parent (UiElem_GetScale 0x004D7E70)

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

// ---------------------------------------------------------------- native structures
using native::gp;
using native::pad;

// A glyph (8 bytes, Font::glyphs).
struct Glyph {
    u8 top;        // +0
    u8 rows;       // +1
    u8 advance;    // +2
    u8 channel;    // +3 (name?)
    be_u16 u;      // +4
    be_u16 v;      // +6
};
static_assert(sizeof(Glyph) == kGlyphSize);

struct Font {
    font::UiRes res;      // +0
    gp<font::UiTexture> texture;  // +92 the texture resource the pages come from
    be_u32 first;         // +96 first code point (no map)
    u8 range;             // +100 number of code points (no map); glyphs allocated when nonzero
    u8 flags;             // +101 0x80 -> quad bit 19
    be_u16 line_height;   // +102
    gp<Glyph> glyphs;     // +104
    gp<u8> map;           // +108 u8[65536] code point -> glyph, 255 = none; 0 = use first/range
    be_u32 pages[4];      // +112 by the element's +136 bits 22-23
};
static_assert(offsetof(Font, first) == kFontFirst && offsetof(Font, line_height) == kFontLineHeight &&
              offsetof(Font, glyphs) == kFontGlyphs && offsetof(Font, map) == kFontMap &&
              offsetof(Font, pages) == 112);

struct FontSet {
    font::UiRes res;           // +0
    be_f32 scale;              // +92
    be_f32 line_scale;         // +96
    be_u32 font_count;         // +100
    gp<Font> fonts[16];        // +104
    be_f32 font_scale[16];     // +168
    be_f32 font_kerning[16];   // +232
};
static_assert(offsetof(FontSet, scale) == kSetScale && offsetof(FontSet, font_count) == kSetFontCount &&
              offsetof(FontSet, fonts) == kSetFonts && offsetof(FontSet, font_scale) == kSetFontScale &&
              offsetof(FontSet, font_kerning) == kSetFontKerning);

// A glyph quad (32 bytes, TextElem::quads); see glyph_build.h for the bits.
struct GlyphQuad {
    be_f32 x, y, w, h;  // +0
    be_u64 bits;        // +16
    be_u32 page;        // +24
    be_u32 _28;         // +28 set to 0
};
static_assert(sizeof(GlyphQuad) == kQuadSize);

// The element base (UiElem) as far as the text code uses it.
struct UiElem {
    be_u32 vtable;        // +0
    pad<76> _4;
    be_f32 scale[3];      // +80
    pad<4> _92;
    char name[40];        // +96
    be_u32 flags;         // +136: kElemActive; bits 14-15 h-align, 18-19 v-align, 22-23 page
    pad<8> _140;
    gp<UiElem> parent;    // +148
    pad<16> _152;
};
static_assert(offsetof(UiElem, scale) == 80 && offsetof(UiElem, name) == kElemName &&
              offsetof(UiElem, flags) == kElemFlags && offsetof(UiElem, parent) == kElemParent &&
              sizeof(UiElem) == 168);

// A text element (UiText, vtable 0x00B227D8).
struct TextElem {
    UiElem elem;              // +0
    gp<FontSet> font_set;     // +168
    be_f32 line_spacing;      // +172
    be_f32 char_spacing;      // +176
    be_u32 text_flags;        // +180: kTextMonospace, kTextAutoLayout; bits 6-21 wrap width; 22-25 glyph mode
    be_u32 buf_cap;           // +184 bytes at buf
    be_u32 quad_cap;          // +188 glyphs quads can hold
    gp<char> buf;             // +192 the formatted text
    be_u32 source_cap;        // +196 bytes at source
    gp<char> source;          // +200 the text as given
    be_u32 quad_bytes;        // +204
    gp<GlyphQuad> quads;      // +208
    pad<4> _212;
    be_f32 glyph_count;       // +216
    be_f32 line_count;        // +220
    be_f32 f224;              // +224: when > 0 the counts above are zeroed (name?)
};
static_assert(offsetof(TextElem, font_set) == kTextFontSet && offsetof(TextElem, text_flags) == kTextFlags &&
              offsetof(TextElem, buf) == kTextBuf && offsetof(TextElem, source) == kTextSource &&
              offsetof(TextElem, quads) == kTextQuads && offsetof(TextElem, glyph_count) == kTextGlyphCount &&
              offsetof(TextElem, f224) == kText224);

struct Vec2f { be_f32 x, y; };
struct Vec3f { be_f32 x, y, z; };

// The first font of the set with a glyph for `c` (the lookup every text
// function inlines). The font pointers are read unchecked, as the original
// does. `index` is (u32)-1 when there is none.
inline GlyphRef FindGlyph(const FontSet& set, u32 c) {
    const u32 count = set.font_count;
    for (u32 i = 0; i < count; ++i) {
        const Font& font = gref(set.fonts[i]);
        if (font.glyphs.ea == 0u) continue;
        if (font.map.ea != 0u) {
            if (c > 0xFFFF) continue;
            const u32 g = gref<u8>(font.map.ea + c);
            if (g == kNoGlyph) continue;
            return {i, g};
        }
        const u32 first = font.first;
        if (c < first) continue;
        if (c < first + font.range) return {i, c - first};
    }
    return {0xFFFFFFFFu, 0xFFFFFFFFu};
}

inline f32 f32_mul(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) * b); }
inline f32 f32_add(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) + b); }
inline f32 f32_sub(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) - b); }
inline f32 f32_div(f32 a, f32 b) { return static_cast<f32>(static_cast<f64>(a) / b); }
inline f32 f32_madd(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), c, b)); }

// The function descriptor in obj's vtable at byte offset `slot` (for
// virtual functions that take an sret pointer before `this`).
inline u32 vslot(const void* obj, u32 slot) {
    return gref<be_u32>(gref<be_u32>(opentit::native::guest(obj)) + slot);
}

// Native functions (src/text).
void Ui_BuildPath(UiElem* e, be_u32* cursor);
const char* StringTable_Lookup(void* strings, const char* id, const char* fallback, bool quiet);
u32 Utf8_Next(void*, be_u32* p, be_u32* len, bool advance);
f32 UiText_SpaceWidth(TextElem* e, const be_f32* scale);
u32 UiText_MeasureLines(TextElem* e, be_f32* widths, u32, u32, const be_f32* scale, f32 space);
void UiText_SetText(TextElem* e, const char* text, bool relayout);
void UiText_Layout(TextElem* e, bool force);
void UiElem_GetScale(Vec3f* out, UiElem* e);
void View_ProjectPoint(void* view, be_f32* out, const be_f32* in);
void UiElem_GetRect(UiElem* e, be_f32* a, be_f32* b, be_f32* m);
u32 UiText_BuildGlyph(TextElem* e, GlyphQuad* q, u32 c, be_f32* x, be_f32* y, u32, const be_f32* scale, u32 line,
                      f32 space);

}  // namespace text
}  // namespace sly4
