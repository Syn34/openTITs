// The element builders Ui_LoadElement dispatches to by tag (types 0, 1,
// 2, 6, 7, 8, 9). Each copies the loader's description, reads its own
// attributes (defaults from a template of the same type, if any) and
// constructs the element. The original also fills a second, default
// description on the stack that nothing reads; those stack stores are
// left out.
#include "loader.h"

#include "../common/common.h"
#include "../memory/mem.h"
#include "../string/str.h"
#include "more.h"

namespace sly4::uiload {
using namespace opentit::native;
using text::UiElem;

namespace {

namespace cfn {
constexpr u32 UiElem_Construct = 0x004D76D0;
constexpr u32 UiElem6_Build = 0x004D8E88;       // (e; f1) (name?)
constexpr u32 memset = 0x00726E54;
constexpr u32 memmove = 0x0071C3F0;
}  // namespace cfn

constexpr u32 kHashSize = 0x0026D801;
constexpr u32 kVfnSizeOut = 44;  // template: (sret f32[2], e)
constexpr u32 kFltMax = 0x7F7FFFFF;
constexpr u32 kMinusOne = 0xBF800000;  // -1.0f
constexpr u32 kElem6BuildArg = 0x004D9D98;  // f32 0.0

// The template's type (vfunc +12).
s32 TypeOf(const UiElem* t) { return vcall<s32>(t, 12); }
// The template's size (vfunc +44 with a result buffer).
void SizeOf(const UiElem* t, be_u32* out) { call_ptr(text::vslot(t, kVfnSizeOut), out, t); }

be_u32* words(void* p) { return static_cast<be_u32*>(p); }
const be_u32* words(const void* p) { return static_cast<const be_u32*>(p); }

void CopyBase(ElemDesc* d, const ElemDesc* base, bool flag126) {
    be_u32* dw = words(d);
    const be_u32* bw = words(base);
    dw[0] = bw[0].get();
    for (u32 row = 16; row <= 80; row += 16)
        for (u32 k = 0; k < 12; k += 4) dw[(row + k) / 4] = bw[(row + k) / 4].get();
    for (u32 k = 0; k < 4; ++k) d->color[k] = base->color[k];
    for (u32 off = 100; off <= 116; off += 4) dw[off / 4] = bw[off / 4].get();
    for (u32 k = 0; k < 6; ++k) d->flags[k] = base->flags[k];
    d->flags[6] = flag126 ? base->flags[6] : 0;
}

inline void Copy2(be_u32* dst, const be_u32* src) {
    dst[0] = src[0].get();
    dst[1] = src[1].get();
}

UiRes* FindRes(u32 hash) {
    for (UiRes* r = host<UiRes>(global<be_u32>(kResList)); r; r = r->next)
        if (r->hash == hash) return r;
    return nullptr;
}

// Picks the index of a hash in a table, or reports an error.
void ReadEnum(XmlCursor* doc, XmlCursor* node, u32 attr, u32 table, u32 n, be_u32* out, be_u32* tmp) {
    if (xml::XmlCursor_ReadHash(node, attr, tmp, 0) != 0) return;
    const be_u32* t = host<be_u32>(table);
    u32 k = 0;
    while (k < n && t[k] != *tmp) ++k;
    if (k < n)
        *out = k;
    else
        XmlCursor_DocPath(doc);
}

const be_u32* zero2() { return host<be_u32>(uimore::kZeroVec2); }
const be_u32* one2() { return host<be_u32>(uimore::kOneVec2); }

struct Vec2Desc {  // types 0 and 9: size and one more vec2
    ElemDesc base;   // +0
    be_u32 a[2];     // +128 size
    be_u32 b[2];     // +136
};
struct Elem7Desc {
    ElemDesc base;   // +0
    be_u32 size[2];  // +128
    be_u32 v[2];     // +136
    be_u32 e144;     // +144 enum
    be_u32 e148;     // +148 enum
    be_u32 f152;     // +152 f32 bits
};
struct Elem6Desc {
    ElemDesc base;   // +0
    be_u32 res[9];   // +128
    be_u32 size[2];  // +164
};
struct Elem2Desc {
    ElemDesc base;    // +0
    be_u32 size[2];   // +128
    u8 colors[16];    // +136
};
struct Elem1Desc {
    ElemDesc base;    // +0
    be_u32 tex;       // +128 UiTexture*
    be_u32 owned;     // +132
    be_u32 uv0[2];    // +136
    be_u32 uv1[2];    // +144
    be_u32 size[2];   // +152
};
static_assert(offsetof(Elem7Desc, f152) == 152 && offsetof(Elem6Desc, size) == 164 &&
              offsetof(Elem2Desc, colors) == 136 && offsetof(Elem1Desc, size) == 152);

// The fields the builders write past the element base (+168).
struct ElemExtra {
    UiElem elem;          // +0
    be_u32 w[22];         // +168..
};
be_u32& extra(UiElem* e, u32 off) { return reinterpret_cast<ElemExtra*>(e)->w[(off - 168) / 4]; }
u8& extra8(UiElem* e, u32 off) { return reinterpret_cast<u8*>(e)[off]; }

}  // namespace

// [type 9, vtable 0x00B22E98, 192 bytes] Two vec2s: size= (template
// default) and 0x76A60DC1 (default 0).
UiElem* UiElem9_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const UiElem* tmpl) {
    constexpr u32 kFrameBase = 112;
    GuestFrame frame(544 - kFrameBase);
    Vec2Desc* d = frame.as<Vec2Desc>(112 - kFrameBase);
    be_u32* def0 = frame.as<be_u32>(384 - kFrameBase);
    be_u32* def1 = frame.as<be_u32>(392 - kFrameBase);
    be_u32* tsize = frame.as<be_u32>(400 - kFrameBase);

    CopyBase(&d->base, base, true);
    Copy2(d->a, zero2());
    Copy2(d->b, zero2());
    Copy2(def0, zero2());
    Copy2(def1, zero2());
    if (tmpl) {
        if (TypeOf(tmpl) == 9) {
            SizeOf(tmpl, tsize);
            Copy2(def0, tsize);
        } else {
            XmlCursor_DocPath(doc);
        }
    }
    xml::XmlCursor_ReadVec2(node, kHashSize, d->a, def0);
    xml::XmlCursor_ReadVec2(node, 0x76A60DC1, d->b, def1);
    UiElem* e = static_cast<UiElem*>(common::SmallAlloc(192));
    if (!e) return nullptr;
    call(cfn::UiElem_Construct, e, d);
    e->vtable = 0x00B22E98u;
    for (u32 k = 0; k < 4; ++k) extra(e, 168 + k * 4) = words(&d->a)[k].get();
    return e;
}

// [type 8, 176 bytes] objName= (the string), 0xA8C8CC1C (bool), and the
// template's +168. The description's +126 flag is cleared.
UiElem* UiElem8_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const UiElem* tmpl) {
    GuestFrame frame(352 - 112);
    Elem8Desc* d = frame.as<Elem8Desc>();

    CopyBase(&d->base, base, false);
    d->name = host<const char>(kEmptyString);
    d->cls = 0u;
    d->use_obj = 0;
    u32 obj = 0;
    if (tmpl) {
        d->obj = 0u;
        if (TypeOf(tmpl) == 8)
            obj = gref<be_u32>(guest(tmpl) + 168);
        else
            XmlCursor_DocPath(doc);
    }
    d->obj = obj;
    d->name = xml::XmlCursor_FindAttrByName(node, host<const char>(0x00AEF580));  // "objName"
    xml::XmlCursor_ReadBool(node, 0xA8C8CC1C, &d->use_obj, 0);
    uimore::UiElemTex* e = static_cast<uimore::UiElemTex*>(common::SmallAlloc(176));
    if (!e) return nullptr;
    uimore::UiElem8_Construct(e, d);
    return &e->elem;
}

// [type 7, vtable 0x00B22D08, 192 bytes] size=, 0x8ACA5893 (vec2), two
// enums (0xFBA47770, 0xFF04AF8C) and a float (0xB2DC7023, default FLT_MAX
// or the template's +188).
UiElem* UiElem7_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const UiElem* tmpl) {
    constexpr u32 kFrameBase = 112;
    GuestFrame frame(608 - kFrameBase);
    Elem7Desc* d = frame.as<Elem7Desc>(112 - kFrameBase);
    be_u32* defsize = frame.as<be_u32>(400 - kFrameBase);
    be_u32* def1 = frame.as<be_u32>(408 - kFrameBase);
    be_u32* deffloat = frame.as<be_u32>(424 - kFrameBase);
    be_u32* tmp = frame.as<be_u32>(432 - kFrameBase);
    be_u32* tsize = frame.as<be_u32>(436 - kFrameBase);

    CopyBase(&d->base, base, true);
    Copy2(d->size, zero2());
    Copy2(d->v, zero2());
    d->e144 = 0u;
    d->e148 = 0u;
    d->f152 = kFltMax;
    Copy2(defsize, zero2());
    Copy2(def1, zero2());
    *deffloat = kFltMax;
    if (tmpl) {
        if (TypeOf(tmpl) == 7) {
            SizeOf(tmpl, tsize);
            Copy2(defsize, tsize);
            *deffloat = gref<be_u32>(guest(tmpl) + 188).get();  // (+184 / +185 are read but not used)
        } else {
            // (r31 is reloaded with the document from the stack just
            // before this call: the report gets the document.)
            XmlCursor_DocPath(doc);
        }
    }
    xml::XmlCursor_ReadVec2(node, kHashSize, d->size, defsize);
    xml::XmlCursor_ReadVec2(node, 0x8ACA5893, d->v, def1);
    ReadEnum(doc, node, 0xFBA47770, 0x00AEF590, 2, &d->e144, tmp);
    ReadEnum(doc, node, 0xFF04AF8C, 0x00AEF5A0, 2, &d->e148, tmp);
    xml::XmlCursor_ReadF32(node, 0xB2DC7023, reinterpret_cast<be_f32*>(&d->f152), *reinterpret_cast<be_f32*>(deffloat));
    UiElem* e = static_cast<UiElem*>(common::SmallAlloc(192));
    if (!e) return nullptr;
    call(cfn::UiElem_Construct, e, d);
    e->vtable = 0x00B22D08u;
    for (u32 k = 0; k < 4; ++k) extra(e, 168 + k * 4) = words(&d->size)[k].get();
    extra8(e, 184) = static_cast<u8>(d->e144.get());
    extra8(e, 185) = static_cast<u8>(d->e148.get());
    extra(e, 188) = d->f152.get();
    return e;
}

// [type 6, vtable 0x00B22C40, 256 bytes] Nine resources named by the
// attributes listed at 0x00F668A8 (defaults: the template's +168..+200)
// and size=.
UiElem* UiElem6_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const UiElem* tmpl) {
    constexpr u32 kFrameBase = 112;
    GuestFrame frame(640 - kFrameBase);
    Elem6Desc* d = frame.as<Elem6Desc>(112 - kFrameBase);
    be_u32* defres = frame.as<be_u32>(416 - kFrameBase);
    be_u32* defsize = frame.as<be_u32>(452 - kFrameBase);
    be_u32* tsize = frame.as<be_u32>(464 - kFrameBase);
    const be_u32* attr_names = host<be_u32>(0x00F668A8);  // const char*[9]

    CopyBase(&d->base, base, true);
    for (u32 k = 0; k < 9; ++k) d->res[k] = 0u;
    Copy2(d->size, zero2());
    for (u32 k = 0; k < 9; ++k) defres[k] = 0u;
    Copy2(defsize, zero2());
    if (tmpl) {
        if (TypeOf(tmpl) == 6) {
            for (u32 k = 0; k < 9; ++k) defres[k] = gref<be_u32>(guest(tmpl) + 168 + k * 4).get();
            SizeOf(tmpl, tsize);
            Copy2(defsize, tsize);
        } else {
            XmlCursor_DocPath(doc);
        }
    }
    for (u32 k = 0; k < 9; ++k) {
        d->res[k] = defres[k].get();
        const char* attr = xml::XmlCursor_FindAttrByName(node, host<const char>(attr_names[k]));
        if (!attr) continue;
        UiRes* r = FindRes(str::String_HashNoCase(attr));
        d->res[k] = guest(r);
        if (!r) XmlCursor_DocPath(doc);
    }
    xml::XmlCursor_ReadVec2(node, kHashSize, d->size, defsize);
    UiElem* e = static_cast<UiElem*>(memory::Mem_Alloc(256, 0, 16));
    if (!e) return nullptr;
    call(cfn::UiElem_Construct, e, d);
    e->vtable = 0x00B22C40u;
    extra(e, 240) = d->size[0].get();
    extra8(e, 248) = 1;
    extra(e, 244) = d->size[1].get();
    call(cfn::memset, &extra(e, 204), 0u, 36u);
    call(cfn::memmove, &extra(e, 168), d->res, 36u);
    call(cfn::UiElem6_Build, e, gref<be_f32>(kElem6BuildArg).get());
    return e;
}

// [type 2, vtable 0x00B22AB0, 208 bytes] size= and four corner colours;
// the default white texture.
UiElem* UiElem2_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const UiElem* tmpl) {
    constexpr u32 kFrameBase = 112;
    GuestFrame frame(576 - kFrameBase);
    Elem2Desc* d = frame.as<Elem2Desc>(112 - kFrameBase);
    be_u32* defsize = frame.as<be_u32>(400 - kFrameBase);
    u8* defcol = frame.as<u8>(408 - kFrameBase);
    be_u32* tsize = frame.as<be_u32>(432 - kFrameBase);
    static constexpr u32 kHashColors[4] = {0x63647CFB, 0x63647D01, 0x63647ACD, 0x63647AD3};

    CopyBase(&d->base, base, true);
    Copy2(d->size, zero2());
    for (u32 k = 0; k < 16; ++k) d->colors[k] = base->color[k & 3];  // the element's colour
    Copy2(defsize, zero2());
    const u8* white = host<u8>(kDefaultColor);
    for (u32 k = 0; k < 16; ++k) defcol[k] = white[k & 3];
    if (tmpl) {
        if (TypeOf(tmpl) == 2) {
            SizeOf(tmpl, tsize);
            Copy2(defsize, tsize);
            for (u32 k = 0; k < 16; ++k) defcol[k] = gref<u8>(guest(tmpl) + 176 + k);
        } else {
            XmlCursor_DocPath(doc);
        }
    }
    xml::XmlCursor_ReadVec2(node, kHashSize, d->size, defsize);
    for (u32 k = 0; k < 4; ++k) xml::XmlCursor_ReadColor(node, kHashColors[k], d->colors + k * 4, defcol + k * 4);
    uimore::UiElemTex* e = static_cast<uimore::UiElemTex*>(common::SmallAlloc(208));
    if (!e) return nullptr;
    call(cfn::UiElem_Construct, e, d);
    e->elem.vtable = 0x00B22AB0u;
    Copy2(&e->material, d->size);
    for (u32 k = 0; k < 16; ++k) reinterpret_cast<u8*>(e)[176 + k] = d->colors[k];
    const u32 tex = Tex_GetDefault(1);
    e->material2 = guest(uimore::Tex_Acquire(host<void>(tex)));
    uimore::UiElem2_SetBlend(e, (e->elem.flags >> 22) & 3);
    return &e->elem;
}

// [type 1, vtable 0x00B229E8, 208 bytes] texture= (a resource), two uv
// vec2s (0xAA1, 0xA61) and size= (default (-1, -1), not taken from the
// template).
UiElem* UiElem1_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const UiElem* tmpl) {
    constexpr u32 kFrameBase = 112;
    GuestFrame frame(592 - kFrameBase);
    Elem1Desc* d = frame.as<Elem1Desc>(112 - kFrameBase);
    be_u32* defuv0 = frame.as<be_u32>(408 - kFrameBase);
    be_u32* defuv1 = frame.as<be_u32>(416 - kFrameBase);
    be_u32* defsize = frame.as<be_u32>(424 - kFrameBase);

    CopyBase(&d->base, base, true);
    d->tex = 0u;
    d->owned = 0u;
    Copy2(d->uv0, zero2());
    Copy2(d->uv1, one2());
    d->size[0] = kMinusOne;
    d->size[1] = kMinusOne;
    Copy2(defuv0, zero2());
    Copy2(defuv1, one2());
    defsize[0] = kMinusOne;
    defsize[1] = kMinusOne;
    u32 tex = 0;
    if (tmpl) {
        if (TypeOf(tmpl) == 1) {
            Copy2(defuv0, &gref<be_u32>(guest(tmpl) + 176));
            Copy2(defuv1, &gref<be_u32>(guest(tmpl) + 184));
            tex = gref<be_u32>(guest(tmpl) + 172);
        } else {
            XmlCursor_DocPath(doc);
        }
    }
    d->tex = tex;
    if (const char* attr = xml::XmlCursor_FindAttrByName(node, host<const char>(0x00AEF4E4))) {  // "texture"
        UiRes* r = FindRes(str::String_HashNoCase(attr));
        d->tex = guest(r);
        if (!r) XmlCursor_DocPath(doc);
    }
    xml::XmlCursor_ReadVec2(node, 0xAA1, d->uv0, defuv0);
    xml::XmlCursor_ReadVec2(node, 0xA61, d->uv1, defuv1);
    xml::XmlCursor_ReadVec2(node, kHashSize, d->size, defsize);
    uimore::UiElemTex* e = static_cast<uimore::UiElemTex*>(common::SmallAlloc(208));
    if (!e) return nullptr;
    call(cfn::UiElem_Construct, e, d);
    e->elem.vtable = 0x00B229E8u;
    e->tex_res = 0u;
    const u32 owned = d->owned;  // always 0 here
    e->material = owned;
    for (u32 k = 0; k < 6; ++k) words(e->uv0)[k] = words(d->uv0)[k].get();
    if (owned != 0) {
        uimore::AddRef(host<void>(owned));
    } else {
        uimore::UiElem_SetTexture(e, host<uimore::UiTexture>(d->tex), e->uv0, e->uv1);
    }
    return &e->elem;
}

// [type 0, vtable 0x00B22920, 176 bytes] Just size=.
UiElem* UiElem0_FromXml(XmlCursor* doc, XmlCursor* node, const ElemDesc* base, const UiElem* tmpl) {
    constexpr u32 kFrameBase = 112;
    GuestFrame frame(576 - kFrameBase);
    Vec2Desc* d = frame.as<Vec2Desc>(112 - kFrameBase);
    be_u32* defsize = frame.as<be_u32>(384 - kFrameBase);
    be_u32* tsize = frame.as<be_u32>(400 - kFrameBase);

    CopyBase(&d->base, base, true);
    Copy2(d->a, zero2());
    Copy2(defsize, zero2());
    if (tmpl) {
        if (TypeOf(tmpl) == 0) {
            SizeOf(tmpl, tsize);
            Copy2(defsize, tsize);
        } else {
            XmlCursor_DocPath(doc);
        }
    }
    xml::XmlCursor_ReadVec2(node, kHashSize, d->a, defsize);
    UiElem* e = static_cast<UiElem*>(common::SmallAlloc(176));
    if (!e) return nullptr;
    call(cfn::UiElem_Construct, e, d);
    e->vtable = 0x00B22920u;
    Copy2(&extra(e, 168), d->a);
    return e;
}

}  // namespace sly4::uiload

NATIVE(004D7910, sly4::uiload::UiElem9_FromXml)
NATIVE(004D8200, sly4::uiload::UiElem8_FromXml)
NATIVE(004D8460, sly4::uiload::UiElem7_FromXml)
NATIVE(004D9DA0, sly4::uiload::UiElem6_FromXml)
NATIVE(004DD1F0, sly4::uiload::UiElem2_FromXml)
NATIVE(004DD800, sly4::uiload::UiElem1_FromXml)
NATIVE(004DDD2C, sly4::uiload::UiElem0_FromXml)
