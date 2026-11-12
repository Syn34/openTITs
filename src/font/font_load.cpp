// Font_FromXml.
#include "load.h"

#include "../heap/heap.h"
#include "../memory/mem.h"
#include "../string/str.h"
#include "../text/text.h"
#include "../uiload/more.h"
#include "../uiload/uiload.h"
#include "../xml/xml.h"

namespace sly4::font {
using namespace opentit::native;

namespace {
inline u32 Scan(const char* str, be_u32* out) { return call<u32>(lfn2::sscanf, str, kStrHexFormat, out); }
}  // namespace

// <... name= texture= mapfilename= firstchar= + four bools>. firstchar is
// a character or "\u<hex>" (default ' '). A child element's text, if any,
// lists the characters glyph by glyph (separated by spaces, tabs, newlines
// or commas, each a character or "\u<hex>"), giving the code point ->
// glyph map (up to 255 glyphs; 0xFF = none).
text::Font* Font_FromXml(xml::XmlCursor* doc, xml::XmlCursor* node, u32 a) {
    GuestFrame frame(256 - 112);
    be_u32* first = frame.as<be_u32>(0);   // sp+112
    be_u32* value = frame.as<be_u32>(4);   // sp+116
    u8* bools = frame.as<u8>(8);           // sp+120
    char* token = frame.as<char>(16);      // sp+128 (no length limit, as in the original)

    const char* name = xml::XmlCursor_FindAttrByName(node, host<const char>(kStrName));
    const char* tex_name = xml::XmlCursor_FindAttrByName(node, host<const char>(kStrTexture));
    const char* map_file = xml::XmlCursor_FindAttrByName(node, host<const char>(kStrMapFile));
    const char* first_attr = xml::XmlCursor_FindAttrByName(node, host<const char>(kStrFirstChar));
    xml::XmlCursor_ReadBool(node, kHashPreload, bools + 0, 0);
    xml::XmlCursor_ReadBool(node, kHashFontFlag80, bools + 1, 0);
    xml::XmlCursor_ReadBool(node, kHashFontFlag40, bools + 2, 0);
    xml::XmlCursor_ReadBool(node, kHashFontFlag20, bools + 3, 0);
    if (!name || !tex_name) {
        uiload::XmlCursor_DocPath(doc);
        return nullptr;
    }
    UiRes* tex = Res_Find(str::String_HashNoCase(tex_name));
    if (!tex || !map_file) {
        uiload::XmlCursor_DocPath(doc);
        return nullptr;
    }

    *first = static_cast<u32>(' ');
    if (first_attr) {
        const u32 fa = guest(first_attr);
        const u8 c0 = gref<u8>(fa);
        if (c0 == '\\' && gref<u8>(fa + 1) == 'u') {
            if (Scan(host<const char>(fa + 2), first) == 0) {
                uiload::XmlCursor_DocPath(doc);
                *first = static_cast<u32>(' ');
            }
        } else if ((c0 & 0x80) == 0) {
            *first = static_cast<u32>(static_cast<s32>(static_cast<s8>(c0)));
        } else if (gref<u8>(fa + 1) != 0) {
            uiload::XmlCursor_DocPath(doc);
        } else {
            *first = c0;
        }
    }

    u8* map = nullptr;
    xml::XmlCursor* child = xml::XmlCursor_FirstChild(node, kHashFontMapTag);
    const char* text = child ? uimore::XmlCursor_Text(child) : nullptr;
    if (text) {
        map = static_cast<u8*>(heap::MemStack_Alloc(memory::Mem_GetDefaultStack(), kFontMapSize));
        call(lfn::memset, map, 0xFFu, kFontMapSize);
        u32 p = guest(text);
        char* out = token;
        u32 glyph = 0;
        token[0] = 0;
        while (glyph < kFontMapMaxGlyphs) {
            const s8 c = static_cast<s8>(gref<u8>(p));
            const bool sep = c == 0 || c == 9 || c == 10 || c == ' ' || c == ',';
            if (!sep) {  // part of a token
                *out++ = static_cast<char>(gref<u8>(p++));
                continue;
            }
            if (token[0] != 0) {
                *out = 0;
                *value = 0u;
                if (token[0] == '\\' && token[1] == 'u')
                    Scan(token + 2, value);
                else
                    Scan(token, value);
                const u32 v = *value;
                if (v == 0 || v > 0xFFFF)
                    uiload::XmlCursor_DocPath(doc);
                else
                    gref<u8>(guest(map) + v) = static_cast<u8>(glyph++);
                out = token;
                token[0] = 0;
            }
            const u8 consumed = gref<u8>(p++);
            if (consumed == 0) break;
        }
    }

    text::Font* result = nullptr;
    text::Font* font = static_cast<text::Font*>(memory::Mem_Alloc(kFontSize, 0, 16));
    if (font) {
        const u8 preload = bools[0];
        UiRes_Construct(&font->res, name, map_file, preload, a);
        font->texture = reinterpret_cast<UiTexture*>(tex);
        u8 flags = font->flags;
        font->first = first->get();
        font->res.vtable = kFontVtbl;
        flags = static_cast<u8>((flags & ~0x80) | ((bools[1] << 7) & 0x80));
        flags = static_cast<u8>((flags & ~0x40) | ((bools[2] << 6) & 0x40));
        flags = static_cast<u8>((flags & ~0x20) | ((bools[3] << 5) & 0x20));
        font->range = 0;
        font->line_height = 0;
        font->glyphs = nullptr;
        font->flags = flags;
        font->map = nullptr;
        for (u32 k = 0; k < 3; ++k) font->pages[k] = 0u;
        uimore::AddRef(font->texture.get());
        if (preload != 0) {
            const u16 users = static_cast<u16>(font->res.users + 1);
            font->res.users = users;
            if (static_cast<s16>(users) > 0 && font->res.loaded < 1) font->res.loaded = static_cast<u16>(vcall<u32>(font, kVfnLoad));
        }
        if (map) {
            u8* copy = static_cast<u8*>(memory::Mem_Alloc(kFontMapSize, 0, 16));
            font->map = copy;
            call(lfn2::memmove, copy, map, kFontMapSize);
        }
        result = font;
    }
    if (map) vcall(memory::Mem_GetDefaultStack(), 8, map);  // free
    return result;
}

}  // namespace sly4::font

NATIVE(004D601C, sly4::font::Font_FromXml)
