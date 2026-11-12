// Ui_LoadElement and Ui_LoadFile.
#include "loader.h"

#include "../game/load.h"
#include "../view/matrix.h"
#include "more.h"

namespace sly4::uiload {
using namespace opentit::native;
using text::UiElem;

namespace {

// The element vfuncs the instance path reads its template through.
constexpr u32 kVfnGetMatrix = 16, kVfnScalePtr = 36, kVfnGetColor = 52;
constexpr u32 kVfnFieldA = 60, kVfnFieldB = 68, kVfnFieldC = 76;
constexpr u32 kVfnFlags[7] = {84, 92, 100, 108, 116, 124, 132};  // IsActive .. GetFlag05
constexpr u32 kVfnClone = 8;
constexpr u32 kElemFirstChild = 152, kElemNext = 156;

be_u32* words(void* p) { return static_cast<be_u32*>(p); }

void CopyRows(be_u32* dst, const be_u32* src) {  // four 3-float rows, 16 bytes apart
    for (u32 row = 0; row < 16; row += 4)
        for (u32 k = 0; k < 3; ++k) dst[row + k] = src[row + k].get();
}

// Fills a default description.
void DescDefaults(ElemDesc* d, const char* name) {
    d->name = name;
    CopyRows(words(d->matrix), host<be_u32>(kIdentity));
    for (u32 k = 0; k < 3; ++k) words(d->scale)[k] = host<be_u32>(kOneVec3)[k].get();
    for (u32 k = 0; k < 4; ++k) d->color[k] = host<u8>(kDefaultColor)[k];
    d->blend = 0u;
    d->parent = nullptr;
    d->arg = 0u;
    d->align = 0u;
    d->anchor = 0u;
    static constexpr u8 kFlags[7] = {1, 0, 0, 1, 1, 1, 0};
    for (u32 k = 0; k < 7; ++k) d->flags[k] = kFlags[k];
}

// The h (bits 0-1) / v (bits 4-5) alignment letters of "anchor" / "align".
void ApplyAlign(be_u32& field, const char* str) {
    for (u32 s = guest(str);; ++s) {
        const u8 c = gref<u8>(s);
        if (c == 0) break;
        u32 v = field;
        switch (static_cast<s8>(c)) {
            case 'T': case 't': v &= ~0x30u; break;
            case 'M': case 'm': v = (v & ~0x30u) | 0x10; break;
            case 'B': case 'b': v = (v & ~0x30u) | 0x20; break;
            case 'L': case 'l': v &= ~0x3u; break;
            case 'C': case 'c': v = (v & ~0x3u) | 0x1; break;
            case 'R': case 'r': v = (v & ~0x3u) | 0x2; break;
            default: continue;
        }
        field = v;
    }
}

// Calls a builder hook: (doc, node, desc, tmpl).
UiElem* CallHook(u32 hook, XmlCursor* doc, XmlCursor* node, ElemDesc* desc, UiElem* tmpl) {
    const u32 opd = global<be_u32>(hook);
    if (opd == 0) return nullptr;
    return call_ptr<UiElem*>(opd, doc, node, desc, tmpl);
}

using Builder = UiElem* (*)(XmlCursor*, XmlCursor*, const ElemDesc*, const UiElem*);
UiElem* BuildText(XmlCursor* doc, XmlCursor* node, const ElemDesc* d, const UiElem* t) {
    return &UiText_FromXml(doc, node, d, reinterpret_cast<const text::TextElem*>(t))->elem;
}

}  // namespace

// Builds the element a node describes (by tag), then its child nodes
// under it. An instance node (kTagInstance) copies the "from=" element's
// properties, builds an element of its type and, unless told otherwise,
// clones its children too. Common attributes: scale, color, the seven
// flags, pos, rot (degrees), blend, anchor, align.
UiElem* Ui_LoadElement(XmlCursor* doc, XmlCursor* node, UiElem* parent, u32 a) {
    constexpr u32 kFrameBase = 112;
    GuestFrame frame(544 - kFrameBase);
    be_f32* rot = frame.as<be_f32>(112 - kFrameBase);      // f32[16]
    ElemDesc* da = frame.as<ElemDesc>(176 - kFrameBase);   // the description that is built
    ElemDesc* db = frame.as<ElemDesc>(304 - kFrameBase);   // the defaults (the template's, for an instance)
    be_f32* angle = frame.as<be_f32>(432 - kFrameBase);
    u8* clone_children = frame.as<u8>(448 - kFrameBase);
    u8* color = frame.as<u8>(449 - kFrameBase);

    const u32 tag = (node->flags & 0x80) != 0 ? node->hash.get() : 0u;
    const char* name = xml::XmlCursor_FindAttrByName(node, host<const char>(0x00AEF4C0));
    if (!name) {
        XmlCursor_DocPath(doc);
        return nullptr;
    }
    DescDefaults(da, name);
    DescDefaults(db, name);
    *clone_children = 1;

    UiElem* tmpl = nullptr;
    if (tag == kTagInstance) {
        const char* from = xml::XmlCursor_FindAttrByName(node, host<const char>(kStrFrom));
        if (!from) {
            XmlCursor_DocPath(doc);
            return nullptr;
        }
        tmpl = reinterpret_cast<UiElem*>(gameload::Ui_Find(from, reinterpret_cast<gameload::FindElem*>(parent)));
        if (!tmpl) {
            XmlCursor_DocPath(doc);
            return nullptr;
        }
        CopyRows(words(db->matrix), &gref<be_u32>(vcall<u32>(tmpl, kVfnGetMatrix)));
        const u32 sc = vcall<u32>(tmpl, kVfnScalePtr);
        for (u32 k = 0; k < 3; ++k) words(db->scale)[k] = gref<be_u32>(sc + 4 * k).get();
        call_ptr(text::vslot(tmpl, kVfnGetColor), color, tmpl);
        for (u32 k = 0; k < 4; ++k) db->color[k] = color[k];
        db->blend = vcall<u32>(tmpl, kVfnFieldA);
        db->align = vcall<u32>(tmpl, kVfnFieldB);
        db->anchor = vcall<u32>(tmpl, kVfnFieldC);
        for (u32 k = 0; k < 7; ++k) db->flags[k] = static_cast<u8>(vcall<u32>(tmpl, kVfnFlags[k]));
        xml::XmlCursor_ReadBool(node, kHashCloneChildren, clone_children, 1);
    }

    da->parent = parent;
    da->arg = a;
    if (parent) db->flags[6] = (parent->flags & 0x20) != 0 ? 1 : 0;

    xml::XmlCursor_ReadVec3(node, kHashScale, words(da->scale), words(db->scale));
    xml::XmlCursor_ReadColor(node, kHashColor, da->color, db->color);
    for (u32 k = 0; k < 7; ++k) xml::XmlCursor_ReadBool(node, kHashFlags[k], &da->flags[k], db->flags[k]);
    CopyRows(words(da->matrix), words(db->matrix));
    xml::XmlCursor_ReadVec3(node, kHashPos, words(da->matrix[3]), words(da->matrix[3]));
    const be_f32* consts = host<be_f32>(kLoaderConsts);
    if (xml::XmlCursor_ReadF32(node, kHashRot, angle, consts[0]) == 0) {
        const f32 radians = text::f32_mul(*angle, consts[1]);
        view::Matrix_FromAxisAngle(rot, host<be_f32>(kRotAxis), radians);
        // The position is kept (it also lands in the matrix's last row).
        for (u32 k = 0; k < 3; ++k) words(rot)[12 + k] = words(da->matrix[3])[k].get();
        CopyRows(words(da->matrix), words(rot));
    }

    const u32 blends[3] = {kBlendNames, kBlendNames + 8, kBlendNames + 16};
    da->blend = db->blend.get();
    if (const char* blend = xml::XmlCursor_FindAttrByName(node, host<const char>(kStrBlend))) {
        u32 k = 0;
        while (k < 3 && uimore::String_CompareN(blend, host<const char>(blends[k]), 2) != 0) ++k;
        if (k < 3)
            da->blend = k;
        else
            XmlCursor_DocPath(doc);
    }
    da->anchor = db->anchor.get();
    da->align = db->align.get();
    for (u32 which = 0; which < 2; ++which) {
        const char* attr = xml::XmlCursor_FindAttrByName(node, host<const char>(which ? kStrAlign : kStrAnchor));
        if (attr) ApplyAlign(which ? da->align : da->anchor, attr);
    }

    UiElem* e = nullptr;
    const s32 st = static_cast<s32>(tag);
    if (st > 0x0400B14D) {
        switch (tag) {
            case 0x33FDE730: e = UiElem2_FromXml(doc, node, da, tmpl); break;
            case 0x30CF0025: e = UiElem8_FromXml(doc, node, da, tmpl); break;
            case 0x044D86D4: e = BuildText(doc, node, da, tmpl); break;
            case 0x0428B13B: e = UiElem1_FromXml(doc, node, da, tmpl); break;
            case 0x040EFE5F: e = UiElem7_FromXml(doc, node, da, tmpl); break;
            default:
                e = CallHook(kUiElementHook, doc, node, da, tmpl);
                if (!e) XmlCursor_DocPath(doc);
        }
    } else if (tag == 0x0400B14D) {
        e = UiElem0_FromXml(doc, node, da, tmpl);
    } else if (tag == kTagInstance) {
        const s32 type = vcall<s32>(tmpl, kVfnType);
        Builder fn = nullptr;
        switch (type) {
            case 0: fn = UiElem0_FromXml; break;
            case 1: fn = UiElem1_FromXml; break;
            case 2: fn = UiElem2_FromXml; break;
            case 3: fn = BuildText; break;
            case 5: fn = UiText5_FromXml; break;
            case 6: fn = UiElem6_FromXml; break;
            case 7: fn = UiElem7_FromXml; break;
            case 8: fn = UiElem8_FromXml; break;
            case 9: fn = UiElem9_FromXml; break;
            default: break;  // 4, 10 and up, negative: the hook
        }
        if (fn) {
            e = fn(doc, node, da, tmpl);
        } else {
            e = CallHook(kUiInstanceHook, doc, node, da, tmpl);
            if (!e) XmlCursor_DocPath(doc);
        }
    } else {
        switch (tag) {
            case 0x00274AAE: e = UiElem6_FromXml(doc, node, da, tmpl); break;
            case 0xAA8CFA44: e = UiElem9_FromXml(doc, node, da, tmpl); break;
            case 0x937B5F0A: e = UiText5_FromXml(doc, node, da, tmpl); break;
            default:
                e = CallHook(kUiElementHook, doc, node, da, tmpl);
                if (!e) XmlCursor_DocPath(doc);
        }
    }
    if (!e) return nullptr;

    if (tmpl && *clone_children != 0) {
        for (u32 c = gref<be_u32>(guest(tmpl) + kElemFirstChild); c != 0; c = gref<be_u32>(c + kElemNext)) {
            UiElem* child = host<UiElem>(c);
            vcall(child, kVfnClone, child->name, e, a, 1u);
        }
    }
    for (XmlCursor* child = uimore::XmlCursor_FirstChildAny(node); child; child = uimore::XmlCursor_NextSiblingAny(child))
        Ui_LoadElement(doc, child, e, a);
    return e;
}

// The <resources> section (an optional named bank, then textures, fonts
// and font sets), the timelines, then the top-level elements. Returns 1.
u32 Ui_LoadFile(void* inc, XmlCursor* doc, XmlCursor* root) {
    if (XmlCursor* res = xml::XmlCursor_FirstChild(root, kTagResources)) {
        if (const char* name = xml::XmlCursor_FindAttrByName(res, host<const char>(0x00AEF4C0))) {
            be_u32& bank = gref<be_u32>(guest(inc) + kIncludeBank);
            bank = call<u32>(lofn::Bank_Create, 0u);
            if (bank != 0u) {
                call(lofn::Bank_Open, bank.get(), name, 0u);
                call(lofn::Bank_Start, bank.get());
            }
        }
        struct Section { u32 tag, fn; };
        static constexpr Section kSections[3] = {
            {kTagTexture, lofn::Texture_FromXml}, {kTagFont, lofn::Font_FromXml}, {kTagFontSet, lofn::FontSet_FromXml}};
        for (const Section& s : kSections)
            for (XmlCursor* c = xml::XmlCursor_FirstChild(res, s.tag); c; c = xml::XmlCursor_NextSibling(c, s.tag))
                call(s.fn, doc, c, inc);
    }
    if (XmlCursor* tls = xml::XmlCursor_FirstChild(root, kTagTimelines))
        for (XmlCursor* c = xml::XmlCursor_FirstChild(tls, kTagTimeline); c; c = xml::XmlCursor_NextSibling(c, kTagTimeline))
            uimore::Timeline_FromXml(doc, c, inc);
    if (XmlCursor* els = xml::XmlCursor_FirstChild(root, kTagElements))
        for (XmlCursor* c = uimore::XmlCursor_FirstChildAny(els); c; c = uimore::XmlCursor_NextSiblingAny(c))
            Ui_LoadElement(doc, c, nullptr, guest(inc));
    return 1;
}

}  // namespace sly4::uiload

NATIVE(004DE0F8, sly4::uiload::Ui_LoadElement)
NATIVE(004DFA60, sly4::uiload::Ui_LoadFile)
