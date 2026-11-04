// FontSet (vtable 0x00B22658): up to 16 fonts used as one, each with its
// own scale.
#include "res.h"

using namespace sly4;
using namespace sly4::font;

namespace {

void FontSet_Teardown(PPUContext& ctx, u32 s) {
    store<u32>(s, kFontSetVtbl);
    for (u32 n; (n = load<u32>(s + kSetCount)) != 0;) {
        store<u32>(s + kSetCount, n - 1);
        const u32 f = load<u32>(s + kSetFonts + (n - 1) * 4);
        const u32 opd = load<u32>(load<u32>(f) + kVfnUnuse);
        call_ptr(ctx, opd, {f, opd, load<u32>(opd)});  // r4 / r5 = OPD and entry
    }
    store<u32>(s, kResVtbl);
    Res_Unlink(ctx, s);
    store<u32>(s, kRefVtbl);
}

}  // namespace

// void FontSet_Destruct(FontSet* s)  [+0]
DECOMP(007952B0, FontSet_Destruct) { FontSet_Teardown(ctx, arg<u32>(ctx, 0)); }

// void FontSet_Delete(FontSet* s)  [+4]
DECOMP(007953AC, FontSet_Delete) {
    const u32 s = arg<u32>(ctx, 0);
    if (s == 0) return;
    FontSet_Teardown(ctx, s);
    call(ctx, rfn::Mem_FreeFrom, {s, 0, kRefVtbl});  // r5 as in the original
}

// u16 FontSet_Load(FontSet* s)  [+12]
// Loads every font (one more user each). Returns the first font state
// that isn't 2 (loaded), else 2 after scaling each font's +168 by
// line height of font 0 / its own line height (font 0 by 1.0).
DECOMP(007954D0, FontSet_Load) {
    const u32 s = arg<u32>(ctx, 0);
    for (u32 i = 0; i < load<u32>(s + kSetCount); ++i) {
        const u32 f = load<u32>(s + kSetFonts + i * 4);
        const u16 users = static_cast<u16>(load<u16>(f + kResUsers) + 1);
        store<u16>(f + kResUsers, users);
        if (static_cast<s16>(users) > 0 && load<u16>(f + kResLoaded) < 1) {
            const u32 opd = load<u32>(load<u32>(f) + kVfnLoad);
            const u32 st = static_cast<u32>(call_ptr(ctx, opd, {f, opd, load<u32>(opd)}));
            store<u16>(f + kResLoaded, static_cast<u16>(st));
        }
    }
    if (load<u32>(s + kSetCount) == 0) return ret(ctx, 2u);
    for (u32 j = 0; j < load<u32>(s + kSetCount); ++j) {
        const u32 f = load<u32>(s + kSetFonts + j * 4);
        const u32 st = load<u16>(f + kResLoaded);
        if (st != 2) return ret(ctx, st);
        const u32 slot = s + kSetFontScale + j * 4;
        f64 k = static_cast<f64>(load<f32>(kFontSetConsts));
        if (j != 0) {
            const f64 h0 = static_cast<f64>(static_cast<f32>(static_cast<f64>(load<u16>(load<u32>(s + kSetFonts) + kFontLineHeight))));
            const f64 hj = static_cast<f64>(static_cast<f32>(static_cast<f64>(load<u16>(f + kFontLineHeight))));
            k = static_cast<f64>(static_cast<f32>(h0 / hj));
        }
        store<f32>(slot, static_cast<f32>(static_cast<f64>(load<f32>(slot)) * k));
    }
    ret(ctx, 2u);
}

// void FontSet_Unload(FontSet* s)  [+16]: one user fewer for every font.
DECOMP(00795650, FontSet_Unload) {
    const u32 s = arg<u32>(ctx, 0);
    u32 n = load<u32>(s + kSetCount);
    if (n != 0) {
        u32 i = 0;
        do {
            const u32 f = load<u32>(s + kSetFonts + i * 4);
            const u16 users = static_cast<u16>(load<u16>(f + kResUsers) - 1);
            store<u16>(f + kResUsers, users);
            if (static_cast<s16>(users) <= 0 && load<u16>(f + kResLoaded) > 1) {
                const u32 opd = load<u32>(load<u32>(f) + kVfnUnload);
                call_ptr(ctx, opd, {f, opd, load<u32>(opd)});
                store<u16>(f + kResLoaded, 0);
            }
            ++i;
            n = load<u32>(s + kSetCount);
        } while (i < n);
    }
    ret(ctx, n);  // r3 = the count as last read, as in the original
}
