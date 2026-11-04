// Font (vtable 0x00B22638): one bitmap font, its glyph table and pages.
#include "res.h"

using namespace sly4;
using namespace sly4::font;

namespace {

constexpr u32 kFontTexture = 92;    // UiRes*: the texture resource the pages come from
constexpr u32 kFontGlyphCount = 100;// u8: glyphs allocated at +104 when nonzero
constexpr u32 kFontGlyphs = 104;    // Glyph[] (8 bytes each)
constexpr u32 kFontMap = 108;       // u8[65536] code point -> glyph, or 0
constexpr u32 kFontPages = 112;     // Ref*[4]

void Font_Teardown(PPUContext& ctx, u32 f) {
    store<u32>(f, kFontVtbl);
    const u32 tex = load<u32>(f + kFontTexture);  // no null check, as in the original
    const u32 opd = load<u32>(load<u32>(tex) + kVfnUnuse);
    call_ptr(ctx, opd, {tex, opd, load<u32>(opd)});  // r4 / r5 = OPD and entry
    if (load<u32>(f + kFontMap) != 0) call(ctx, rfn::Mem_FreeFrom, {load<u32>(f + kFontMap), 0});
    store<u32>(f, kResVtbl);
    Res_Unlink(ctx, f);
    store<u32>(f, kRefVtbl);
}

}  // namespace

// void Font_Destruct(Font* f)  [+0]
DECOMP(007950DC, Font_Destruct) { Font_Teardown(ctx, arg<u32>(ctx, 0)); }

// void Font_Delete(Font* f)  [+4]
DECOMP(007951B8, Font_Delete) {
    const u32 f = arg<u32>(ctx, 0);
    if (f == 0) return;
    Font_Teardown(ctx, f);
    call(ctx, rfn::Mem_FreeFrom, {f, 0, kRefVtbl});  // r5 as in the original
}

// void Font_Unload(Font* f)  [+16]
// Frees the glyph table, releases pages 0..2 (page 3 is left alone, as in
// the original) and drops a user of the texture resource.
DECOMP(004D70B0, Font_Unload) {
    const u32 f = arg<u32>(ctx, 0);
    if (load<u8>(f + kFontGlyphCount) != 0) {
        call(ctx, rfn::Mem_FreeFrom, {load<u32>(f + kFontGlyphs), 0});
        store<u32>(f + kFontGlyphs, 0);
        store<u8>(f + kFontGlyphCount, 0);
    }
    for (u32 k = 0; k < 3; ++k) {
        const u32 page = load<u32>(f + kFontPages + k * 4);
        if (page == 0) continue;
        const u32 opd = load<u32>(load<u32>(page) + 8);  // Release
        call_ptr(ctx, opd, {page, opd, load<u32>(opd)});
    }
    const u32 tex = load<u32>(f + kFontTexture);
    const u16 users = static_cast<u16>(load<u16>(tex + kResUsers) - 1);
    store<u16>(tex + kResUsers, users);
    if (static_cast<s16>(users) <= 0 && load<u16>(tex + kResLoaded) > 1) {
        const u32 opd = load<u32>(load<u32>(tex) + kVfnUnload);
        call_ptr(ctx, opd, {tex, opd, load<u32>(opd)});
        store<u16>(tex + kResLoaded, 0);
    }
}
