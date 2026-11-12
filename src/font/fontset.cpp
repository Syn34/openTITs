// FontSet (vtable 0x00B22658): up to 16 fonts used as one, each with its
// own scale.
#include "res.h"

#include "../common/common.h"
#include "../text/text.h"

namespace sly4::font {
using namespace opentit::native;
using text::FontSet;

namespace {

void FontSet_Teardown(FontSet* s) {
    s->res.vtable = kFontSetVtbl;
    for (u32 n; (n = s->font_count) != 0;) {
        s->font_count = n - 1;
        vcall(s->fonts[n - 1].get(), kVfnUnuse);
    }
    s->res.vtable = kResVtbl;
    Res_Unlink(&s->res);
    s->res.vtable = kRefVtbl;
}

}  // namespace

// [+0]
void FontSet_Destruct(FontSet* s) { FontSet_Teardown(s); }

// [+4]
void FontSet_Delete(FontSet* s) {
    if (!s) return;
    FontSet_Teardown(s);
    common::Mem_FreeFrom(s, 0);
}

// [+12] Loads every font (one more user each). Returns the first font
// state that isn't 2 (loaded), else 2 after scaling each font's +168 by
// line height of font 0 / its own line height (font 0 by 1.0).
u32 FontSet_Load(FontSet* s) {
    for (u32 i = 0; i < s->font_count; ++i) {
        UiRes& f = gref(s->fonts[i]).res;
        const u16 users = static_cast<u16>(f.users + 1);
        f.users = users;
        if (static_cast<s16>(users) > 0 && f.loaded < 1) f.loaded = static_cast<u16>(vcall<u32>(&f, kVfnLoad));
    }
    if (s->font_count == 0u) return 2;
    for (u32 j = 0; j < s->font_count; ++j) {
        const text::Font& f = gref(s->fonts[j]);
        const u32 st = f.res.loaded;
        if (st != 2) return st;
        f32 k = gref<be_f32>(kFontSetConsts);
        if (j != 0) {
            const f32 h0 = static_cast<f32>(gref(s->fonts[0]).line_height.get());
            const f32 hj = static_cast<f32>(f.line_height.get());
            k = text::f32_div(h0, hj);
        }
        s->font_scale[j] = text::f32_mul(s->font_scale[j], k);
    }
    return 2;
}

// [+16] One user fewer for every font. Returns the count as last read.
u32 FontSet_Unload(FontSet* s) {
    u32 n = s->font_count;
    if (n != 0) {
        u32 i = 0;
        do {
            UiRes& f = gref(s->fonts[i]).res;
            const u16 users = static_cast<u16>(f.users - 1);
            f.users = users;
            if (static_cast<s16>(users) <= 0 && f.loaded > 1) {
                vcall(&f, kVfnUnload);
                f.loaded = 0;
            }
            ++i;
            n = s->font_count;
        } while (i < n);
    }
    return n;
}

}  // namespace sly4::font

NATIVE(007952B0, sly4::font::FontSet_Destruct)
NATIVE(007953AC, sly4::font::FontSet_Delete)
NATIVE(007954D0, sly4::font::FontSet_Load)
NATIVE(00795650, sly4::font::FontSet_Unload)
