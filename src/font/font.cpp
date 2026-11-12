// Font (vtable 0x00B22638): one bitmap font, its glyph table and pages.
#include "res.h"

#include "../common/common.h"
#include "../text/text.h"

namespace sly4::font {
using namespace opentit::native;
using text::Font;

namespace {

void Font_Teardown(Font* f) {
    f->res.vtable = kFontVtbl;
    vcall(f->texture.get(), kVfnUnuse);  // no null check, as in the original
    if (f->map) common::Mem_FreeFrom(f->map, 0);
    f->res.vtable = kResVtbl;
    Res_Unlink(&f->res);
    f->res.vtable = kRefVtbl;
}

}  // namespace

// [+0]
void Font_Destruct(Font* f) { Font_Teardown(f); }

// [+4]
void Font_Delete(Font* f) {
    if (!f) return;
    Font_Teardown(f);
    common::Mem_FreeFrom(f, 0);
}

// [+16] Frees the glyph table, releases pages 0..2 (page 3 is left alone,
// as in the original) and drops a user of the texture resource.
void Font_Unload(Font* f) {
    if (f->range != 0) {
        common::Mem_FreeFrom(f->glyphs, 0);
        f->glyphs = nullptr;
        f->range = 0;
    }
    for (u32 k = 0; k < 3; ++k) {
        const u32 page = f->pages[k];
        if (page == 0) continue;
        vcall(host<void>(page), 8);  // Release
    }
    UiTexture& tex = gref(f->texture);
    const u16 users = static_cast<u16>(tex.res.users - 1);
    tex.res.users = users;
    if (static_cast<s16>(users) <= 0 && tex.res.loaded > 1) {
        vcall(f->texture.get(), kVfnUnload);
        tex.res.loaded = 0;
    }
}

}  // namespace sly4::font

NATIVE(007950DC, sly4::font::Font_Destruct)
NATIVE(007951B8, sly4::font::Font_Delete)
NATIVE(004D70B0, sly4::font::Font_Unload)
