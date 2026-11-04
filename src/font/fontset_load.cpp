// FontSet_FromXml.
#include "load.h"

using namespace sly4;
using namespace sly4::font;

namespace {

// One more user; loads it (vfunc +12) when it isn't loaded yet.
void Res_Use(PPUContext& ctx, u32 r) {
    const u16 users = static_cast<u16>(load<u16>(r + kResUsers) + 1);
    store<u16>(r + kResUsers, users);
    if (static_cast<s16>(users) > 0 && load<u16>(r + kResLoaded) < 1) {
        const u32 opd = load<u32>(load<u32>(r) + kVfnLoad);
        const u32 st = static_cast<u32>(call_ptr(ctx, opd, {r, opd, load<u32>(opd)}));  // r4 / r5 = OPD and entry
        store<u16>(r + kResLoaded, static_cast<u16>(st));
    }
}

}  // namespace

// FontSet* FontSet_FromXml(XmlCursor* doc, XmlCursor* node, u32 arg)
// <... name= scale=(x, y) preload=> with <font name= scale= kerning=/>
// children naming fonts already on the resource list. Fonts added to a
// loaded set are loaded and scaled to font 0's line height.
DECOMP(004D71D0, FontSet_FromXml) {
    const u32 doc = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2);

    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 240 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 font_scale = sp + 112, font_kern = sp + 116, def = sp + 120, scale = sp + 128, preload = sp + 136;

    const u32 name = static_cast<u32>(call(ctx, lfn::XmlCursor_FindAttrByName, {node, kStrName}));
    const f64 one = static_cast<f64>(load<f32>(kFontSetLoadConsts));
    store<u32>(def, 0x3F800000);
    store<u32>(def + 4, 0x3F800000);
    call(ctx, lfn::XmlCursor_ReadVec2, {node, kHashScale, scale, def});
    call(ctx, lfn::XmlCursor_ReadBool, {node, kHashPreload, preload, 0});
    if (name == 0) {
        call(ctx, lfn::XmlCursor_DocPath, {doc});
        return ret(ctx, 0u);
    }

    u32 set = static_cast<u32>(call(ctx, lfn::Mem_Alloc, {kFontSetSize, 0, 16}));
    if (set != 0) {
        const u8 pre = load<u8>(preload);
        call(ctx, rfn::UiRes_Construct, {set, name, kEmptyPath, pre, a});
        store<u32>(set + kSetScale, load<u32>(scale));
        store<u32>(set + kSetCount, 0);
        store<u32>(set, kFontSetVtbl);
        store<u32>(set + kSetLineScale, load<u32>(scale + 4));
        call(ctx, lfn::memset, {set + kSetFonts, 0, 64});
        for (u32 k = 0; k < 16; ++k) {
            store<u32>(set + kSetFontScale + k * 4, 0x3F800000);
            store<u32>(set + kSetKerning + k * 4, 0);
        }
        if (pre != 0) Res_Use(ctx, set);
    }

    for (u32 child = static_cast<u32>(call(ctx, lfn::XmlCursor_FirstChild, {node, kHashFontTag})); child != 0;
         child = static_cast<u32>(call(ctx, lfn::XmlCursor_NextSibling, {child, kHashFontTag}))) {
        const u32 fname = static_cast<u32>(call(ctx, lfn::XmlCursor_FindAttrByName, {child, kStrName}));
        if (fname == 0) {
            call(ctx, lfn::XmlCursor_DocPath, {doc});
            continue;
        }
        const u32 font = Res_FindByHash(static_cast<u32>(call(ctx, rfn::String_HashNoCase, {fname})));
        if (font == 0) {
            call(ctx, lfn::XmlCursor_DocPath, {doc});
            continue;
        }
        ctx.fpr[1] = one;
        call(ctx, lfn::XmlCursor_ReadF32, {child, kHashScale, font_scale});
        ctx.fpr[1] = static_cast<f64>(load<f32>(kFontSetLoadConsts + 4));
        call(ctx, lfn::XmlCursor_ReadF32, {child, kHashKerning, font_kern, kFontSetLoadConsts});  // r6 as in the original
        // With no set (allocation failed) these go to address 0 + offset, as in the original.
        const u32 n = load<u32>(set + kSetCount);
        store<u32>(set + kSetFonts + n * 4, font);
        store<u16>(font + kRefCount, static_cast<u16>(load<u16>(font + kRefCount) + 1));
        store<u32>(set + kSetFontScale + n * 4, load<u32>(font_scale));
        store<u32>(set + kSetKerning + n * 4, load<u32>(font_kern));
        if (static_cast<s32>(load<u16>(set + kResLoaded)) > 0) {
            Res_Use(ctx, font);
            const u32 m = load<u32>(set + kSetCount);
            if (m != 0) {
                const f64 h0 = static_cast<f64>(static_cast<f32>(static_cast<f64>(load<u16>(load<u32>(set + kSetFonts) + kFontLineHeight))));
                const f64 h = static_cast<f64>(static_cast<f32>(static_cast<f64>(load<u16>(font + kFontLineHeight))));
                const u32 slot = set + kSetFontScale + m * 4;
                const f64 k = static_cast<f64>(static_cast<f32>(h0 / h));
                store<f32>(slot, static_cast<f32>(static_cast<f64>(load<f32>(slot)) * k));
            }
        }
        store<u32>(set + kSetCount, load<u32>(set + kSetCount) + 1);
    }
    ret(ctx, set);
}
