// FontSet_FromXml.
#include "load.h"

#include "../memory/mem.h"
#include "../string/str.h"
#include "../text/text.h"
#include "../uiload/more.h"
#include "../uiload/uiload.h"
#include "../xml/xml.h"

namespace sly4::font {
using namespace opentit::native;

namespace {

// One more user; loads it (vfunc +12) when it isn't loaded yet.
void Res_Use(UiRes& r) {
    const u16 users = static_cast<u16>(r.users + 1);
    r.users = users;
    if (static_cast<s16>(users) > 0 && r.loaded < 1) r.loaded = static_cast<u16>(vcall<u32>(&r, kVfnLoad));
}

}  // namespace

// <... name= scale=(x, y) preload=> with <font name= scale= kerning=/>
// children naming fonts already on the resource list. Fonts added to a
// loaded set are loaded and scaled to font 0's line height.
text::FontSet* FontSet_FromXml(xml::XmlCursor* doc, xml::XmlCursor* node, u32 a) {
    GuestFrame frame(240 - 112);
    be_f32* font_scale = frame.as<be_f32>(0);  // sp+112
    be_f32* font_kern = frame.as<be_f32>(4);   // sp+116
    be_u32* def = frame.as<be_u32>(8);         // sp+120
    be_u32* scale = frame.as<be_u32>(16);      // sp+128
    u8* preload = frame.as<u8>(24);            // sp+136

    const char* name = xml::XmlCursor_FindAttrByName(node, host<const char>(kStrName));
    const be_f32* consts = host<be_f32>(kFontSetLoadConsts);
    const f32 one = consts[0];
    def[0] = 0x3F800000u;
    def[1] = 0x3F800000u;
    xml::XmlCursor_ReadVec2(node, kHashScale, scale, def);
    xml::XmlCursor_ReadBool(node, kHashPreload, preload, 0);
    if (!name) {
        uiload::XmlCursor_DocPath(doc);
        return nullptr;
    }

    const u32 set_ea = guest(memory::Mem_Alloc(kFontSetSize, 0, 16));
    text::FontSet& set = gref<text::FontSet>(set_ea);
    if (set_ea != 0) {
        const u8 pre = *preload;
        UiRes_Construct(&set.res, name, host<const char>(kEmptyPath), pre, a);
        reinterpret_cast<be_u32&>(set.scale) = scale[0].get();
        set.font_count = 0u;
        set.res.vtable = kFontSetVtbl;
        reinterpret_cast<be_u32&>(set.line_scale) = scale[1].get();
        call(lfn::memset, set.fonts, 0u, 64u);
        for (u32 k = 0; k < 16; ++k) {
            set.font_scale[k] = 1.0f;
            reinterpret_cast<be_u32&>(set.font_kerning[k]) = 0u;
        }
        if (pre != 0) Res_Use(set.res);
    }

    for (xml::XmlCursor* child = xml::XmlCursor_FirstChild(node, kHashFontTag); child;
         child = xml::XmlCursor_NextSibling(child, kHashFontTag)) {
        const char* fname = xml::XmlCursor_FindAttrByName(child, host<const char>(kStrName));
        if (!fname) {
            uiload::XmlCursor_DocPath(doc);
            continue;
        }
        UiRes* font = Res_Find(str::String_HashNoCase(fname));
        if (!font) {
            uiload::XmlCursor_DocPath(doc);
            continue;
        }
        xml::XmlCursor_ReadF32(child, kHashScale, font_scale, one);
        xml::XmlCursor_ReadF32(child, kHashKerning, font_kern, consts[1]);
        // With no set (allocation failed) these go to address 0 + offset, as in the original.
        const u32 n = set.font_count;
        gref<be_u32>(set_ea + kSetFonts + n * 4) = guest(font);
        uimore::AddRef(font);
        gref<be_u32>(set_ea + kSetFontScale + n * 4) = reinterpret_cast<be_u32*>(font_scale)->get();
        gref<be_u32>(set_ea + kSetKerning + n * 4) = reinterpret_cast<be_u32*>(font_kern)->get();
        if (static_cast<s32>(set.res.loaded) > 0) {
            Res_Use(*font);
            const u32 m = set.font_count;
            if (m != 0) {
                const f32 h0 = static_cast<f32>(gref(set.fonts[0]).line_height.get());
                const f32 h = static_cast<f32>(reinterpret_cast<text::Font*>(font)->line_height.get());
                be_f32& slot = gref<be_f32>(set_ea + kSetFontScale + m * 4);
                const f32 k = text::f32_div(h0, h);
                slot = text::f32_mul(slot, k);
            }
        }
        set.font_count = set.font_count + 1;
    }
    return host<text::FontSet>(set_ea);
}

}  // namespace sly4::font

NATIVE(004D71D0, sly4::font::FontSet_FromXml)
