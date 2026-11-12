// XmlCursor: walking elements and reading typed attributes.
#include "xml.h"

#include "../string/str.h"

namespace sly4::xml {
using namespace opentit::native;

namespace {

// The next cursor of `depth`'s ring of four in the document (32-bit address
// arithmetic: depths past 15 run over, as in the original).
XmlCursor* XmlDoc_NextCursor(XmlDoc* doc, u32 depth) {
    u8& ring = gref<u8>(guest(doc) + depth + kDocRing);
    const u32 slot = ring;
    XmlCursor* cursor = host<XmlCursor>(guest(doc) + ((depth << 7) & 0xFFFFFF80) + slot * kCursorSize + kDocCursors);
    const u32 next = slot + 1;
    ring = static_cast<u8>((next & 0xFF) >= kDocSlotsPerDepth ? 0 : next);
    return cursor;
}

be_u32* child_list(u32 elem) { return &gref<be_u32>(gref<be_u32>(elem + kElemChildren)); }

}  // namespace

// The first child of the cursor's element (or, for the document, the first
// top-level element) whose tag hash is `hash`, as a cursor from the
// document's ring for the next depth; 0 if there is none.
XmlCursor* XmlCursor_FirstChildRaw(XmlCursor* cursor, u32 hash) {
    XmlDoc* doc = cursor->doc;
    if ((cursor->flags & kCurValid) == 0) return nullptr;
    const u32 parent = cursor->parent.ea;
    const u32 type = cursor->type;
    if (parent == 0 && type == 0) doc = reinterpret_cast<XmlDoc*>(cursor);  // the document itself
    u32 list, count, depth, elem;
    if (parent != 0) {
        depth = cursor->depth + 1u;
        elem = child_list(parent)[cursor->index];
        list = gref<be_u32>(elem + kElemChildren);
        count = gref<be_u16>(elem + kElemChildCount);
    } else {
        const u32 data = gref<be_u32>(guest(doc) + kDocData);
        if (type != 0) {  // a top-level element
            depth = 1;
            elem = gref<be_u32>(data + cursor->index * 4u + kXmlbTop);
            list = gref<be_u32>(elem + kElemChildren);
            count = gref<be_u16>(elem + kElemChildCount);
        } else {
            depth = 0;
            elem = 0;
            list = data + kXmlbTop;
            count = gref<be_u16>(data + kXmlbTopCount);
        }
    }
    if (count == 0) return nullptr;
    u32 i = 0;
    while (gref<be_u32>(gref<be_u32>(list + i * 4)) != hash)
        if (++i >= count) return nullptr;
    const u32 child = gref<be_u32>(list + i * 4);
    XmlCursor* out = XmlDoc_NextCursor(doc, depth);
    const u8 flags = out->flags;
    out->depth = static_cast<u8>(depth);
    out->parent = host<XmlbElement>(elem);
    out->index = static_cast<u16>(i);
    out->flags = static_cast<u8>(flags | kCurValid);
    out->doc = doc;
    out->hash = gref<be_u32>(child + kElemHash).get();
    return out;
}

// The next sibling after the cursor with tag hash `hash`, as a cursor from
// the ring of the same depth; 0 if there is none.
XmlCursor* XmlCursor_NextSiblingRaw(XmlCursor* cursor, u32 hash) {
    if ((cursor->flags & kCurValid) == 0) return nullptr;
    const u32 parent = cursor->parent.ea;
    XmlDoc* doc = cursor->doc;
    u32 list, count;
    if (parent != 0) {
        list = gref<be_u32>(parent + kElemChildren);
        count = gref<be_u16>(parent + kElemChildCount);
    } else {
        const u32 data = gref<be_u32>(guest(doc) + kDocData);
        list = data + kXmlbTop;
        count = gref<be_u16>(data + kXmlbTopCount);
    }
    u32 i = cursor->index + 1u;
    for (;; ++i) {
        if (i >= count) return nullptr;
        if (gref<be_u32>(gref<be_u32>(list + i * 4)) == hash) break;
    }
    const u32 child = gref<be_u32>(list + i * 4);
    const u32 depth = cursor->depth;
    XmlCursor* out = XmlDoc_NextCursor(doc, depth);
    // `out` can be `cursor` itself once the ring has gone round; the order
    // of reads and writes below is the original's, which copes with that.
    out->flags |= kCurValid;
    out->depth = cursor->depth;
    const u32 parent2 = cursor->parent.ea;
    const u32 child_hash = gref<be_u32>(child + kElemHash);
    out->parent.ea = parent2;
    out->index = static_cast<u16>(i);
    const u32 doc2 = cursor->doc.ea;
    out->hash = child_hash;
    out->doc.ea = doc2;
    return out;
}

// The first child with tag hash `hash` that is an element (vfunc +12).
XmlCursor* XmlCursor_FirstChild(XmlCursor* cursor, u32 hash) {
    for (XmlCursor* c = XmlCursor_FirstChildRaw(cursor, hash); c; c = XmlCursor_NextSiblingRaw(c, hash))
        if (vcall<u32>(c, kCurVfnAsElement) != 0) return vcall<XmlCursor*>(c, kCurVfnAsElement);
    return nullptr;
}

XmlCursor* XmlCursor_NextSibling(XmlCursor* cursor, u32 hash) {
    for (XmlCursor* c = XmlCursor_NextSiblingRaw(cursor, hash); c; c = XmlCursor_NextSiblingRaw(c, hash))
        if (vcall<u32>(c, kCurVfnAsElement) != 0) return vcall<XmlCursor*>(c, kCurVfnAsElement);
    return nullptr;
}

// A string attribute's text, or 0.
const char* XmlCursor_FindAttr(XmlCursor* cursor, u32 hash) {
    u32 type = 0, value = 0;
    if (!XmlCursor_Lookup(cursor, hash, type, value) || type != kXmlString) return nullptr;
    return host<const char>(value);
}

const char* XmlCursor_FindAttrByName(XmlCursor* cursor, const char* name) {
    const u32 hash = name ? str::String_HashNoCase(name) : 0;
    u32 type = 0, value = 0;
    if (!XmlCursor_Lookup(cursor, hash, type, value) || type != kXmlString) return nullptr;
    return host<const char>(value);
}

u32 XmlCursor_ReadBool(XmlCursor* cursor, u32 hash, u8* out, u8 fallback) {
    u32 type = 0, value = 0;
    if (!XmlCursor_Lookup(cursor, hash, type, value)) {
        *out = fallback;
        return kXmlMissing;
    }
    if (type != kXmlInt) {
        *out = fallback;
        return kXmlWrongType;
    }
    *out = gref<be_u32>(value) != 0 ? 1 : 0;
    return kXmlOk;
}

u32 XmlCursor_ReadS32(XmlCursor* cursor, u32 hash, be_u32* out, u32 fallback) {
    u32 type = 0, value = 0;
    if (!XmlCursor_Lookup(cursor, hash, type, value)) {
        *out = fallback;
        return kXmlMissing;
    }
    if (type == kXmlInt) {
        *out = gref<be_u32>(value).get();
    } else if (type == kXmlFloat) {
        *out = static_cast<u32>(rt::fctiw(static_cast<f64>(gref<be_f32>(value).get()), true));
    } else {
        *out = fallback;
        return kXmlWrongType;
    }
    return kXmlOk;
}

// The default arrives in f1 (stored with stfs).
u32 XmlCursor_ReadF32(XmlCursor* cursor, u32 hash, be_f32* out, f32 fallback) {
    u32 type = 0, value = 0;
    if (!XmlCursor_Lookup(cursor, hash, type, value)) {
        *out = fallback;
        return kXmlMissing;
    }
    if (type == kXmlFloat) {
        *reinterpret_cast<be_u32*>(out) = gref<be_u32>(value).get();  // lfs + stfs: the bits are copied
    } else if (type == kXmlInt) {
        const s32 n = static_cast<s32>(gref<be_u32>(value).get());
        *out = static_cast<f32>(static_cast<f64>(n));  // fcfid + frsp
    } else {
        *out = fallback;
        return kXmlWrongType;
    }
    return kXmlOk;
}

namespace {

// Read every source word first, then store (the original's order for the value).
template <u32 N>
void copy_words(be_u32* dst, const be_u32* src, bool buffered) {
    if (!buffered) {
        for (u32 k = 0; k < N; ++k) dst[k] = src[k].get();
        return;
    }
    u32 tmp[N];
    for (u32 k = 0; k < N; ++k) tmp[k] = src[k];
    for (u32 k = 0; k < N; ++k) dst[k] = tmp[k];
}

template <u32 Want, u32 N>
u32 Xml_ReadVector(XmlCursor* cursor, u32 hash, be_u32* out, const be_u32* fallback) {
    u32 type = 0, value = 0;
    if (!XmlCursor_Lookup(cursor, hash, type, value)) {
        copy_words<N>(out, fallback, false);
        return kXmlMissing;
    }
    if (type != Want) {
        copy_words<N>(out, fallback, false);
        return kXmlWrongType;
    }
    copy_words<N>(out, &gref<be_u32>(value), true);
    return kXmlOk;
}

}  // namespace

u32 XmlCursor_ReadVec2(XmlCursor* c, u32 hash, be_u32* out, const be_u32* fallback) {
    return Xml_ReadVector<kXmlVec2, 2>(c, hash, out, fallback);
}
u32 XmlCursor_ReadVec3(XmlCursor* c, u32 hash, be_u32* out, const be_u32* fallback) {
    return Xml_ReadVector<kXmlVec3, 3>(c, hash, out, fallback);
}

u32 XmlCursor_ReadColor(XmlCursor* cursor, u32 hash, u8* out, const u8* fallback) {
    u32 type = 0, value = 0;
    const bool found = XmlCursor_Lookup(cursor, hash, type, value);
    if (!found || type != kXmlColor) {
        for (u32 k = 0; k < 4; ++k) out[k] = fallback[k];
        return found ? kXmlWrongType : kXmlMissing;
    }
    u8 bytes[4];
    for (u32 k = 0; k < 4; ++k) bytes[k] = gref<u8>(value + k);
    for (u32 k = 0; k < 4; ++k) out[k] = bytes[k];
    return kXmlOk;
}

u32 XmlCursor_ReadHash(XmlCursor* cursor, u32 hash, be_u32* out, u32 fallback) {
    u32 type = 0, value = 0;
    if (!XmlCursor_Lookup(cursor, hash, type, value)) {
        *out = fallback;
        return kXmlMissing;
    }
    if (type == kXmlString) {
        *out = gref<be_u32>(value - 4).get();
    } else if (type == kXmlInt || type == kXmlHash) {
        *out = gref<be_u32>(value).get();
    } else {
        *out = fallback;
        return kXmlWrongType;
    }
    return kXmlOk;
}

}  // namespace sly4::xml

NATIVE(004F8ACC, sly4::xml::XmlCursor_FirstChildRaw)
NATIVE(004F8C70, sly4::xml::XmlCursor_NextSiblingRaw)
NATIVE(004F92BC, sly4::xml::XmlCursor_FirstChild)
NATIVE(004F8FC8, sly4::xml::XmlCursor_NextSibling)
NATIVE(004F9534, sly4::xml::XmlCursor_FindAttr)
NATIVE(004F9628, sly4::xml::XmlCursor_FindAttrByName)
NATIVE(004F986C, sly4::xml::XmlCursor_ReadBool)
NATIVE(004F99BC, sly4::xml::XmlCursor_ReadS32)
NATIVE(004F9B20, sly4::xml::XmlCursor_ReadF32)
NATIVE(004F9C9C, sly4::xml::XmlCursor_ReadVec2)
NATIVE(004F9E0C, sly4::xml::XmlCursor_ReadVec3)
NATIVE(004F9F9C, sly4::xml::XmlCursor_ReadColor)
NATIVE(004FA14C, sly4::xml::XmlCursor_ReadHash)
