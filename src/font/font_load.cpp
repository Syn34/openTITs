// Font_FromXml.
#include "load.h"

using namespace sly4;
using namespace sly4::font;

namespace {

constexpr u32 kFontTexture = 92, kFontFirst = 96, kFontGlyphCount = 100, kFontFlags = 101;
constexpr u32 kFontGlyphs = 104, kFontMap = 108, kFontPages = 112;

inline u32 Scan(PPUContext& ctx, u32 str, u32 out) {
    return static_cast<u32>(call(ctx, lfn2::sscanf, {str, kStrHexFormat, out}));
}

}  // namespace

// Font* Font_FromXml(XmlCursor* doc, XmlCursor* node, u32 arg)
// <... name= texture= mapfilename= firstchar= + four bools>. firstchar is
// a character or "\u<hex>" (default ' '). A child element's text, if any,
// lists the characters glyph by glyph (separated by spaces, tabs, newlines
// or commas, each a character or "\u<hex>"), giving the code point ->
// glyph map (up to 255 glyphs; 0xFF = none).
DECOMP(004D601C, Font_FromXml) {
    const u32 doc = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2);

    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 256 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 first = sp + 112, value = sp + 116, bools = sp + 120, token = sp + 128;

    const u32 name = static_cast<u32>(call(ctx, lfn::XmlCursor_FindAttrByName, {node, kStrName}));
    u32 tex_name = static_cast<u32>(call(ctx, lfn::XmlCursor_FindAttrByName, {node, kStrTexture}));
    const u32 map_file = static_cast<u32>(call(ctx, lfn::XmlCursor_FindAttrByName, {node, kStrMapFile}));
    const u32 first_attr = static_cast<u32>(call(ctx, lfn::XmlCursor_FindAttrByName, {node, kStrFirstChar}));
    call(ctx, lfn::XmlCursor_ReadBool, {node, kHashPreload, bools + 0, 0});
    call(ctx, lfn::XmlCursor_ReadBool, {node, kHashFontFlag80, bools + 1, 0});
    call(ctx, lfn::XmlCursor_ReadBool, {node, kHashFontFlag40, bools + 2, 0});
    call(ctx, lfn::XmlCursor_ReadBool, {node, kHashFontFlag20, bools + 3, 0});
    if (name == 0 || tex_name == 0) {
        call(ctx, lfn::XmlCursor_DocPath, {doc});
        return ret(ctx, 0u);
    }
    const u32 tex = Res_FindByHash(static_cast<u32>(call(ctx, rfn::String_HashNoCase, {tex_name})));
    if (tex == 0 || map_file == 0) {
        call(ctx, lfn::XmlCursor_DocPath, {doc});
        return ret(ctx, 0u);
    }

    store<u32>(first, ' ');
    if (first_attr != 0) {
        const u8 c0 = load<u8>(first_attr);
        if (c0 == '\\' && load<u8>(first_attr + 1) == 'u') {
            if (Scan(ctx, first_attr + 2, first) == 0) {
                call(ctx, lfn::XmlCursor_DocPath, {doc});
                store<u32>(first, ' ');
            }
        } else if ((c0 & 0x80) == 0) {
            store<u32>(first, static_cast<u32>(static_cast<s32>(static_cast<s8>(c0))));
        } else if (load<u8>(first_attr + 1) != 0) {
            call(ctx, lfn::XmlCursor_DocPath, {doc});
        } else {
            store<u32>(first, c0);
        }
    }

    u32 map = 0;
    const u32 child = static_cast<u32>(call(ctx, lfn::XmlCursor_FirstChild, {node, kHashFontMapTag}));
    u32 text = child != 0 ? static_cast<u32>(call(ctx, lfn2::XmlCursor_Text, {child})) : 0;
    if (text != 0) {
        const u32 stack = static_cast<u32>(call(ctx, lfn2::Mem_GetDefaultStack, {}));
        map = static_cast<u32>(call(ctx, lfn2::MemStack_Alloc, {stack, kFontMapSize}));
        call(ctx, lfn::memset, {map, 0xFF, kFontMapSize});
        u32 out = token, glyph = 0;
        store<u8>(token, 0);
        while (glyph < kFontMapMaxGlyphs) {
            const s8 c = static_cast<s8>(load<u8>(text));
            const bool sep = c == 0 || c == 9 || c == 10 || c == ' ' || c == ',';
            if (!sep) {  // part of a token (no length limit, as in the original)
                store<u8>(out++, load<u8>(text++));
                continue;
            }
            if (load<u8>(token) != 0) {
                store<u8>(out, 0);
                store<u32>(value, 0);
                if (load<u8>(token) == '\\' && load<u8>(token + 1) == 'u')
                    Scan(ctx, token + 2, value);
                else
                    Scan(ctx, token, value);
                const u32 v = load<u32>(value);
                if (v == 0 || v > 0xFFFF)
                    call(ctx, lfn::XmlCursor_DocPath, {doc});
                else
                    store<u8>(map + v, static_cast<u8>(glyph++));
                out = token;
                store<u8>(token, 0);
            }
            const u8 consumed = load<u8>(text++);
            if (consumed == 0) break;
        }
    }

    u32 result = 0;
    const u32 font = static_cast<u32>(call(ctx, lfn::Mem_Alloc, {kFontSize, 0, 16}));
    if (font != 0) {
        const u8 preload = load<u8>(bools + 0);
        call(ctx, rfn::UiRes_Construct, {font, name, map_file, preload, a});
        store<u32>(font + kFontTexture, tex);
        u8 flags = load<u8>(font + kFontFlags);
        store<u32>(font + kFontFirst, load<u32>(first));
        store<u32>(font, kFontVtbl);
        flags = static_cast<u8>((flags & ~0x80) | ((load<u8>(bools + 1) << 7) & 0x80));
        flags = static_cast<u8>((flags & ~0x40) | ((load<u8>(bools + 2) << 6) & 0x40));
        flags = static_cast<u8>((flags & ~0x20) | ((load<u8>(bools + 3) << 5) & 0x20));
        store<u8>(font + kFontGlyphCount, 0);
        store<u16>(font + 102, 0);
        store<u32>(font + kFontGlyphs, 0);
        store<u8>(font + kFontFlags, flags);
        store<u32>(font + kFontMap, 0);
        for (u32 k = 0; k < 12; ++k) store<u8>(font + kFontPages + k, 0);
        const u32 t = load<u32>(font + kFontTexture);
        store<u16>(t + kRefCount, static_cast<u16>(load<u16>(t + kRefCount) + 1));
        if (preload != 0) {
            const u16 users = static_cast<u16>(load<u16>(font + kResUsers) + 1);
            store<u16>(font + kResUsers, users);
            if (static_cast<s16>(users) > 0 && load<u16>(font + kResLoaded) < 1) {
                const u32 opd = load<u32>(load<u32>(font) + kVfnLoad);
                const u32 st = static_cast<u32>(call_ptr(ctx, opd, {font, opd, load<u32>(opd)}));
                store<u16>(font + kResLoaded, static_cast<u16>(st));
            }
        }
        if (map != 0) {
            const u32 copy = static_cast<u32>(call(ctx, lfn::Mem_Alloc, {kFontMapSize, 0, 16}));
            store<u32>(font + kFontMap, copy);
            call(ctx, lfn2::memmove, {copy, map, kFontMapSize});
        }
        result = font;
    }
    if (map != 0) {
        const u32 stack = static_cast<u32>(call(ctx, lfn2::Mem_GetDefaultStack, {}));
        const u32 opd = load<u32>(load<u32>(stack) + 8);  // free
        call_ptr(ctx, opd, {stack, map, opd, load<u32>(opd)});  // r5 / r6 = OPD and entry
    }
    ret(ctx, result);
}
