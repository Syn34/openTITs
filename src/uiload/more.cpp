// Small callees of the UI loader (see more.h).
#include "more.h"

#include "../memory/mem.h"
#include "../string/str.h"
#include "tlbuild.h"
#include "uiload.h"

namespace sly4::uimore {
using namespace opentit::native;
using xml::XmlCursor;
using xml::XmlDoc;

namespace {

// The next of the 4 pool cursors for `depth` (inlined in the original;
// 32-bit address arithmetic as there).
XmlCursor* XmlDoc_NextCursor(u32 doc, u32 depth) {
    u8& ring = gref<u8>(doc + depth + xml::kDocRing);
    const u32 slot = ring;
    XmlCursor* cursor = host<XmlCursor>(doc + ((depth << 7) & 0xFFFFFF80) + slot * 32 + xml::kDocCursors);
    const u32 next = slot + 1;
    ring = static_cast<u8>((next & 0xFF) >= 4 ? 0 : next);
    return cursor;
}

// The next element after this one under the same parent, whatever its tag
// (inlined into both walkers below).
XmlCursor* XmlCursor_NextSiblingRawAny(XmlCursor* cursor) {
    if ((cursor->flags & xml::kCurValid) == 0) return nullptr;
    const u32 parent = cursor->parent.ea;
    const u32 doc = cursor->doc.ea;
    u32 list, count;
    if (parent != 0) {
        list = gref<be_u32>(parent + xml::kElemChildren);
        count = gref<be_u16>(parent + xml::kElemChildCount);
    } else {
        const u32 data = gref<be_u32>(doc + xml::kDocData);
        list = data + xml::kXmlbTop;
        count = gref<be_u16>(data + xml::kXmlbTopCount);
    }
    const u32 i = cursor->index + 1u;
    if (i >= count) return nullptr;
    const u32 child = gref<be_u32>(list + i * 4);
    XmlCursor* out = XmlDoc_NextCursor(doc, cursor->depth);
    // `out` can be `cursor` itself once the ring has gone round; this is
    // the original's order of reads and writes.
    out->flags |= xml::kCurValid;
    out->depth = cursor->depth;
    const u32 parent2 = cursor->parent.ea;
    const u32 hash = gref<be_u32>(child);
    out->parent.ea = parent2;
    out->index = static_cast<u16>(i);
    const u32 doc2 = cursor->doc.ea;
    out->hash = hash;
    out->doc.ea = doc2;
    return out;
}

// From c on, the first cursor whose AsElement() isn't 0 (in practice c).
XmlCursor* FirstElementFrom(XmlCursor* c) {
    for (; c; c = XmlCursor_NextSiblingRawAny(c))
        if (vcall<u32>(c, xml::kCurVfnAsElement) != 0) return vcall<XmlCursor*>(c, xml::kCurVfnAsElement);
    return nullptr;
}

}  // namespace

// The first child element whatever its tag (for the document: the first
// top-level element), in the next pool cursor one level deeper.
XmlCursor* XmlCursor_FirstChildRawAny(XmlCursor* cursor) {
    u32 doc = cursor->doc.ea;
    if ((cursor->flags & xml::kCurValid) == 0) return nullptr;
    const u32 parent = cursor->parent.ea;
    const u32 type = cursor->type;
    if (parent == 0 && type == 0) doc = guest(cursor);  // the document itself

    u32 list, count, depth, elem;
    if (parent != 0) {
        depth = cursor->depth + 1u;
        elem = gref<be_u32>(gref<be_u32>(parent + xml::kElemChildren) + cursor->index * 4u);
        list = gref<be_u32>(elem + xml::kElemChildren);
        count = gref<be_u16>(elem + xml::kElemChildCount);
    } else {
        const u32 data = gref<be_u32>(doc + xml::kDocData);
        if (type != 0) {  // a top-level element
            depth = 1;
            elem = gref<be_u32>(data + cursor->index * 4u + xml::kXmlbTop);
            list = gref<be_u32>(elem + xml::kElemChildren);
            count = gref<be_u16>(elem + xml::kElemChildCount);
        } else {
            depth = 0;
            elem = 0;
            list = data + xml::kXmlbTop;
            count = gref<be_u16>(data + xml::kXmlbTopCount);
        }
    }
    if (count == 0) return nullptr;

    const u32 child = gref<be_u32>(list);
    XmlCursor* out = XmlDoc_NextCursor(doc, depth);
    const u8 flags = out->flags;
    out->depth = static_cast<u8>(depth);
    out->parent.ea = elem;
    out->index = 0;
    out->flags = static_cast<u8>(flags | xml::kCurValid);
    out->doc.ea = doc;
    out->hash = gref<be_u32>(child).get();
    return out;
}

// The first child element, whatever its tag (XmlCursor_FirstChild without
// the tag filter).
XmlCursor* XmlCursor_FirstChildAny(XmlCursor* cursor) { return FirstElementFrom(XmlCursor_FirstChildRawAny(cursor)); }

// The next element under the same parent, whatever its tag.
XmlCursor* XmlCursor_NextSiblingAny(XmlCursor* cursor) { return FirstElementFrom(XmlCursor_NextSiblingRawAny(cursor)); }

// The element's +4 string (0 for an invalid cursor).
const char* XmlCursor_Text(XmlCursor* cursor) {
    if ((cursor->flags & xml::kCurValid) == 0) return nullptr;
    const u32 parent = cursor->parent.ea;
    const u32 index = cursor->index;
    u32 elem;
    if (parent != 0)
        elem = gref<be_u32>(gref<be_u32>(parent + xml::kElemChildren) + index * 4);
    else
        elem = gref<be_u32>(gref<be_u32>(cursor->doc.ea + xml::kDocData) + index * 4 + xml::kXmlbTop);
    return host<const char>(gref<be_u32>(elem + xml::kElemName));
}

// strncasecmp for ASCII: -1, 0 or 1. Characters are signed; only A-Z fold.
s32 String_CompareN(const char* a, const char* b, s32 n) {
    auto lower = [](s32 c) -> s32 { return (c >= 'A' && c <= 'Z') ? static_cast<s8>(c + 32) : c; };
    const u32 pa = guest(a), pb = guest(b);
    for (s32 i = 0;; ++i) {
        const u8 ca = gref<u8>(pa + static_cast<u32>(i));
        if (ca == 0 && gref<u8>(pb + static_cast<u32>(i)) == 0) return 0;
        if (i >= n) return 0;
        const s32 la = lower(static_cast<s8>(ca));
        const s32 lb = lower(static_cast<s8>(gref<u8>(pb + static_cast<u32>(i))));
        if (la < lb) return -1;
        if (la > lb) return 1;
    }
}

// A new material for drawing with `tex` (one more reference to it); for a
// null texture, an untextured one.
Material* Tex_Acquire(void* tex) {
    if (!tex) {
        const u32 shader = call<u32>(mfn::Shader_FindByHash, kShaderUntextured);
        const u32 m = call<u32>(mfn::Material_Alloc);
        gref<Material>(m).shader = shader;  // no null check, as in the original
        return host<Material>(m);
    }
    const u32 shader = call<u32>(mfn::Shader_FindByHash, kShaderTextured);
    const u32 ma = call<u32>(mfn::Material_Alloc);
    Material& m = gref<Material>(ma);
    const u8 n = static_cast<u8>(m.tex_count + 1);
    m.shader = shader;
    const u32 index = static_cast<u32>(n) - 1;  // -1 when the count wrapped to 0
    m.tex_count = n;
    be_u32& slot = gref<be_u32>(ma + kMatTextures + index * 4);
    const u32 old = slot;
    if (old != guest(tex)) {
        slot = guest(tex);
        AddRef(tex);
        if (old != 0) Release(host<void>(old));
    }
    if (index == 0) m.tex0_word = static_cast<u16>(gref<be_u32>(guest(tex) + 112).get());
    return host<Material>(ma);
}

// [type 2] Stores the blend index in the flags and sets the material's
// blend state; depth test off, depth func ALWAYS.
void UiElem2_SetBlend(UiElemTex* e, u32 blend) {
    const u32 flags = e->elem.flags;
    const u32 m = e->material2;
    e->elem.flags = (flags & ~0x00C00000u) | ((blend << 22) & 0x00C00000u);
    if (m == 0) return;
    Blend_SetMode(gref<Material>(m).blend, gref<be_s32>(kBlendModes + blend * 4));
    Blend_SetDepth(gref<Material>(e->material2).blend, kGlAlways);
    be_u32* b = gref<Material>(e->material2).blend;
    b[2] = b[2] & 0x7FFFFFFFu;
}

// Three materials on the loaded texture, one per blend index ("av",
// "ad", "su"), with depth func LEQUAL (flag 0x80000000) or ALWAYS and
// depth test from flag 0x40000000.
void Texture_MakePages(UiTexture* t) {
    static constexpr s32 kModes[3] = {2, 3, 4};  // also stored at sp+112 in the original
    u32 h = t->handle;
    if (h == 0) return;
    for (u32 k = 0; k < 3; ++k) {
        Material* m = Tex_Acquire(host<void>(h));
        t->pages[k] = guest(m);
        Blend_SetMode(gref<Material>(guest(m)).blend, kModes[k]);
        Blend_SetDepth(gref<Material>(t->pages[k]).blend, (t->flags & 0x80000000) != 0 ? kGlLequal : kGlAlways);
        const u32 test = (t->flags & 0x40000000) != 0 ? 1 : 0;
        be_u32* b2 = gref<Material>(t->pages[k]).blend;
        b2[2] = (b2[2] & 0x7FFFFFFFu) | (test << 31);
        h = t->handle;
    }
}

// Points the element at a texture resource. Its material is kept when it
// already draws the same texture; otherwise the old material and resource
// are released and the material becomes the resource's page for the
// element's blend index (or a new one on the texture, or on the default
// texture when the resource isn't loaded or is null). Sets 0x02000000.
void UiElem_SetTexture(UiElemTex* e, UiTexture* res, const be_u32* uv0, const be_u32* uv1) {
    const u32 cur = e->material;
    bool keep = false;
    if (cur != 0) {
        if (res) {
            const u32 bound = gref<Material>(cur).textures[0];
            const u32 want = res->res.loaded != 2 ? uiload::Tex_GetDefault(0) : res->handle.get();
            keep = bound == want;
        }
        if (!keep) {
            if (const u32 old = e->tex_res) {
                Release(host<void>(old));
                e->tex_res = 0u;
            }
            Release(host<void>(e->material));
            e->material = 0u;
        }
    }

    e->tex_res = guest(res);
    const be_u32* u0 = res ? uv0 : host<be_u32>(kZeroVec2);
    const be_u32* u1 = res ? uv1 : host<be_u32>(kOneVec2);
    for (u32 k = 0; k < 2; ++k) e->uv0[k] = u0[k].get();  // lfs / stfs pairs
    for (u32 k = 0; k < 2; ++k) e->uv1[k] = u1[k].get();

    if (!keep) {
        auto set_blend = [&] { vcall(e, kVfnSetBlend, (e->elem.flags >> 22) & 3); };
        if (!res) {
            const u32 def = uiload::Tex_GetDefault(1);
            e->material = guest(Tex_Acquire(host<void>(def)));
            set_blend();
        } else {
            const u16 refs = static_cast<u16>(res->res.refs + 1);
            const u16 users = static_cast<u16>(res->res.users + 1);
            res->res.refs = refs;
            res->res.users = users;
            if (static_cast<s16>(users) > 0 && res->res.loaded < 1) res->res.loaded = static_cast<u16>(vcall<u32>(res, kResVfnLoad));
            // Blend index 3 reads the word after the three pages, as in the original.
            const u32 page = gref<be_u32>(guest(res) + kTexPages + ((e->elem.flags >> 22) & 3) * 4);
            e->material = page;
            if (page != 0) {
                AddRef(host<void>(page));
            } else {
                const u32 tex = res->res.loaded == 2 ? res->handle.get() : uiload::Tex_GetDefault(0);
                e->material = guest(Tex_Acquire(host<void>(tex)));
                set_blend();
            }
        }
    }
    e->elem.flags |= 0x02000000u;
}

// The texture already loaded from a file of that name, or 0.
u32 Tex_FindLoaded(u32 hash) {
    GuestFrame frame(16);
    be_u32* key = frame.as<be_u32>();
    *key = hash;
    const u32 map = gref<be_u32>(kTexDefaults + 24);
    return gref<be_u32>(call<u32>(mfn::Map_Find, map, key));
}

// Loads a texture file (any format the texture code knows by extension).
// On failure, with `fallback` (low byte), the default texture 0 (one more
// reference) or 0.
void* Tex_LoadFile(const char* path, u32 fallback) {
    GuestFrame frame(16);
    be_u32* size_out = frame.as<be_u32>();
    u32 tex = 0;
    const u32 data = call<u32>(mfn::File_LoadWhole, path, size_out);
    if (data != 0) {
        const u32 ext = call<u32>(mfn::Path_GetExtension, path);
        const u32 size = *size_out;
        const u32 t = call<u32>(mfn::Texture_Alloc);
        char* name = &gref<char>(t + 16);
        call(mfn::String_CopyN, name, path, 32u);
        gref<u8>(t + 47) = 0;
        gref<be_u32>(t + 48) = str::String_HashNoCase(name);
        tex = t;
        if (call<u32>(mfn::Texture_Init, t, data, size, ext, path) == 0) {
            Release(host<void>(t));
            tex = 0;
        }
        memory::Mem_Free(host<void>(data));
        if (tex != 0) return host<void>(tex);
    }
    if (static_cast<u8>(fallback) != 0) {
        tex = global<be_u32>(kTexDefaults);
        if (tex != 0) AddRef(host<void>(tex));
    }
    return host<void>(tex);
}

namespace {
struct TimelineDef {
    be_u32 vtable;        // +0 kTimelineDefVtbl
    char name[32];        // +4
    be_u32 hash;          // +36
    be_u32 inc;           // +40 Include*
    u8 flag;              // +44 (name?)
    pad<3> _45;
    be_u32 keys;          // +48 first key (chained by key +8)
    be_u32 next;          // +52
};
static_assert(sizeof(TimelineDef) == kTimelineDefSize);
}  // namespace

// <timeline name=...>: a 56-byte definition appended to the list at
// 0x00F6689C, with its keys (each child element) chained from +48 by +8.
void* Timeline_FromXml(XmlCursor* doc, XmlCursor* node, void* inc) {
    const char* name = xml::XmlCursor_FindAttrByName(node, host<const char>(kStrName));
    if (!name) {
        uiload::XmlCursor_DocPath(doc);
        return nullptr;
    }
    const u32 t = guest(memory::Mem_Alloc(kTimelineDefSize, 0, 16));
    TimelineDef& d = gref<TimelineDef>(t);
    if (t != 0) {
        d.vtable = kTimelineDefVtbl;
        d.hash = str::String_HashNoCase(name);
        d.inc = guest(inc);
        d.flag = 0;
        d.keys = 0u;
        d.next = 0u;
        call(mfn::String_CopyN, d.name, name, 31u);
        u32 last = global<be_u32>(kTimelineDefs);
        if (last == 0) {
            global<be_u32>(kTimelineDefs) = t;
        } else {
            while (gref<TimelineDef>(last).next != 0u) last = gref<TimelineDef>(last).next;
            gref<TimelineDef>(last).next = t;
        }
    }
    // With a failed allocation these touch guest addresses 44 and 48, as in the original.
    xml::XmlCursor_ReadBool(node, kHashTimelineFlag, &d.flag, 0);
    u32 prev = d.keys;
    for (XmlCursor* c = XmlCursor_FirstChildAny(node); c; c = XmlCursor_NextSiblingAny(c)) {
        const u32 key = guest(tlbuild::TimelineDef_KeyFromXml(doc, c, host<void>(t)));
        if (key == 0) continue;
        if (prev != 0)
            gref<be_u32>(prev + 8) = key;
        else
            d.keys = key;
        prev = key;
    }
    return host<void>(t);
}

// [type 8, vtable 0x00B22DD0] An element showing a world object: d +132
// (an object to use), d +136 (a class to instance) or else the object
// named d +128 (looked up, then instanced from the class of that name).
// d +140 set: the object itself is used (one more reference); otherwise a
// copy named "<name>_GUIInst". The object is scaled by the element's
// scale (times the parent's if flag 0x10000000) with x corrected for the
// screen's aspect ratio.
void UiElem8_Construct(UiElemTex* e, const void* desc) {
    const uiload::Elem8Desc* d = static_cast<const uiload::Elem8Desc*>(desc);
    GuestFrame frame(208 - 112);
    be_f32* scale = frame.as<be_f32>(0);    // sp+112
    be_f32* pscale = frame.as<be_f32>(16);  // sp+128
    be_u32* key = frame.as<be_u32>(32);     // sp+144

    call(mfn::UiElem_Construct, e, d);
    e->material = 0u;
    e->elem.vtable = kElem8Vtbl;
    u32 obj = d->obj;
    u32 cls = d->cls;
    if (obj == 0 && cls == 0) {
        obj = call<u32>(mfn::WorldObj_FindByHash, str::String_HashNoCase(d->name), 0u);
        if (obj == 0) {
            *key = str::String_HashNoCase(d->name);
            const u32 map = gref<be_u32>(global<be_u32>(kWorldClassMapHolder) + 16);
            cls = gref<be_u32>(call<u32>(mfn::HashMap_Find, map, key));
        }
    }
    if (obj != 0) {
        if (d->use_obj != 0) {
            e->material = obj;
            AddRef(host<void>(obj));
        } else {
            e->material = vcall<u32>(host<void>(obj), 56, 5u);
        }
    } else if (cls != 0) {
        e->material = call<u32>(mfn::WorldObj_Instance, cls, 0u);
    }

    u32 o = e->material;
    if (o == 0) return;
    if (d->use_obj == 0) {
        const u32 src_name = vcall<u32>(host<void>(gref<be_u32>(o + 208)), 12);
        const u32 inst_name = call<u32>(mfn::String_Format, kStrGuiInst, src_name);
        call(mfn::WorldObj_SetName, o, inst_name, 1u);
        o = e->material;
    }
    call(mfn::WorldObj_Func583E54, o, 12u);

    for (u32 k = 0; k < 3; ++k) reinterpret_cast<be_u32*>(scale)[k] = reinterpret_cast<const be_u32*>(e->elem.scale)[k].get();
    f32 sx = scale[0];
    text::UiElem* parent = e->elem.parent;
    if ((e->elem.flags & 0x10000000) != 0 && parent) {
        call_ptr(text::vslot(parent, 168), pscale, parent);
        sx = text::f32_mul(scale[0], pscale[0]);
        const f32 sy = text::f32_mul(scale[1], pscale[1]);
        const f32 sz = text::f32_mul(scale[2], pscale[2]);
        scale[0] = sx;
        scale[1] = sy;
        scale[2] = sz;
    }
    const f32 w = static_cast<f32>(static_cast<f64>(gref<be_u16>(kConfig + 4).get()));
    const f32 h = static_cast<f32>(static_cast<f64>(gref<be_u16>(kConfig + 6).get()));
    const f32 aspect = text::f32_div(w, h);
    const f32 k = text::f32_div(gref<be_f32>(kConfig + 16), aspect);
    scale[0] = text::f32_mul(sx, k);
    call(mfn::WorldObj_SetScale, e->material.get(), scale, 0u);
    vcall(host<void>(e->material), 40, 0u, 0u);
    call(mfn::UiElem8_Attach, e, e->material.get(), 1u);
}

}  // namespace sly4::uimore

NATIVE(004F899C, sly4::uimore::XmlCursor_FirstChildRawAny)
NATIVE(004F9140, sly4::uimore::XmlCursor_FirstChildAny)
NATIVE(004F8D74, sly4::uimore::XmlCursor_NextSiblingAny)
NATIVE(004FA744, sly4::uimore::XmlCursor_Text)
NATIVE(00555EB0, sly4::uimore::String_CompareN)
NATIVE(00523B7C, sly4::uimore::Tex_Acquire)
NATIVE(004DD08C, sly4::uimore::UiElem2_SetBlend)
NATIVE(004D5C64, sly4::uimore::Texture_MakePages)
NATIVE(004D8A80, sly4::uimore::UiElem_SetTexture)
NATIVE(0050C398, sly4::uimore::Tex_FindLoaded)
NATIVE(0051E68C, sly4::uimore::Tex_LoadFile)
NATIVE(004DF88C, sly4::uimore::Timeline_FromXml)
NATIVE(004D7F24, sly4::uimore::UiElem8_Construct)
