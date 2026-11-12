// Texture resource slots and Texture_FromXml.
#include "tex.h"

#include "../common/common.h"
#include "../memory/mem.h"
#include "../uiload/more.h"
#include "../uiload/uiload.h"
#include "../xml/xml.h"

namespace sly4::font {
using namespace opentit::native;

// <... name= filename= preload= + two flags>.
UiTexture* Texture_FromXml(xml::XmlCursor* doc, xml::XmlCursor* node, u32 a) {
    GuestFrame frame(16);
    u8* bools = frame.as<u8>();
    const char* name = xml::XmlCursor_FindAttrByName(node, host<const char>(kStrName));
    const char* file = xml::XmlCursor_FindAttrByName(node, host<const char>(kStrFileName));
    xml::XmlCursor_ReadBool(node, kHashPreload, bools + 0, 0);
    xml::XmlCursor_ReadBool(node, kHashFontFlag40, bools + 1, 0);
    xml::XmlCursor_ReadBool(node, kHashFontFlag20, bools + 2, 0);
    if (!name || !file) {
        uiload::XmlCursor_DocPath(doc);
        return nullptr;
    }
    UiTexture* t = static_cast<UiTexture*>(memory::Mem_Alloc(kTexSize, 0, 16));
    if (!t) return nullptr;
    const u8 preload = bools[0], b1 = bools[1], b2 = bools[2];
    UiRes_Construct(&t->res, name, file, preload, a);
    u32 f = t->flags & 0x7FFFFFFFu;
    t->handle = 0u;
    f |= static_cast<u32>(b1) << 31;
    f = (f & ~0x40000000u) | ((static_cast<u32>(b2) << 30) & 0x40000000);
    t->res.vtable = kTextureVtbl;
    t->flags = f;
    for (u32 k = 0; k < 3; ++k) t->pages[k] = 0u;
    if (preload != 0) {
        const u16 users = static_cast<u16>(t->res.users + 1);
        t->res.users = users;
        if (static_cast<s16>(users) > 0 && t->res.loaded < 1) t->res.loaded = static_cast<u16>(vcall<u32>(t, kVfnLoad));
    }
    return t;
}

// [+12] An already loaded texture with the same file name (one more
// reference), else the file itself; 2 on success, 3 on failure.
u32 Texture_Load(UiTexture* t) {
    u32 h = uimore::Tex_FindLoaded(t->res.file_hash);
    t->handle = h;
    if (h != 0) {
        uimore::AddRef(host<void>(h));
    } else {
        h = guest(uimore::Tex_LoadFile(t->res.path, 0));
        t->handle = h;
    }
    if (h == 0) return 3;
    uimore::Texture_MakePages(t);
    return 2;
}

// [+16]
void Texture_Unload(UiTexture* t) {
    for (u32 k = 0; k < 3; ++k) {
        const u32 p = t->pages[k];
        if (p == 0) continue;
        uimore::Release(host<void>(p));
        t->pages[k] = 0u;
    }
    const u32 h = t->handle;
    if (h == 0) return;
    uimore::Release(host<void>(h));
    t->handle = 0u;
}

// The texture resource adds nothing to the UiRes destructors.
void Texture_Destruct(UiTexture* t) { UiRes_Destruct(&t->res); }

void Texture_Delete(UiTexture* t) {
    if (!t) return;
    UiRes_Destruct(&t->res);
    common::Mem_FreeFrom(t, 0);
}

}  // namespace sly4::font

NATIVE(004D5A4C, sly4::font::Texture_FromXml)
NATIVE(004D5EAC, sly4::font::Texture_Load)
NATIVE(004D5F38, sly4::font::Texture_Unload)
NATIVE(00794FD0, sly4::font::Texture_Destruct)
NATIVE(0079503C, sly4::font::Texture_Delete)
