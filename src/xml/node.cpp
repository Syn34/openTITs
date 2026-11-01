// XmlCursor: walking elements and reading attributes.
#include "xml.h"

using namespace sly4;
using namespace sly4::xml;

namespace {

// The next of the 4 pool cursors for `depth`, in turn.
u32 XmlDoc_NextCursor(u32 doc, u32 depth) {
    auto& ring = ref<u8>(doc + depth + kDocRing);
    const u32 slot = ring;
    const u32 cursor = doc + ((depth << 7) & 0xFFFFFF80) + slot * kCursorSize + kDocCursors;
    const u32 next = slot + 1;
    ring = static_cast<u8>((next & 0xFF) >= kDocSlotsPerDepth ? 0 : next);
    return cursor;
}

}  // namespace

// XmlCursor* XmlCursor_FirstChildRaw(XmlCursor* cursor, u32 tag_hash)
// The first child element with that tag (for the document: the first
// top-level element), in the next pool cursor one level deeper.
DECOMP(004F8ACC, XmlCursor_FirstChildRaw) {
    const u32 cursor = arg<u32>(ctx, 0);
    const u32 hash = arg<u32>(ctx, 1);

    u32 doc = field<u32>(cursor, kCurDoc);
    if ((ref<u8>(cursor + kCurFlags) & kCurValid) == 0) return ret(ctx, 0u);
    const u32 parent = field<u32>(cursor, kCurParent);
    const u32 type = field<u32>(cursor, kCurType);
    if (parent == 0 && type == 0) doc = cursor;  // the document itself

    u32 list, count, depth, elem;
    if (parent != 0) {
        depth = ref<u8>(cursor + kCurDepth) + 1u;
        elem = load<u32>(field<u32>(parent, kElemChildren) + field<u16>(cursor, kCurIndex) * 4u);
        list = field<u32>(elem, kElemChildren);
        count = field<u16>(elem, kElemChildCount);
    } else {
        const u32 data = field<u32>(doc, kDocData);
        if (type != 0) {  // a top-level element
            depth = 1;
            elem = load<u32>(data + field<u16>(cursor, kCurIndex) * 4u + kXmlbTop);
            list = field<u32>(elem, kElemChildren);
            count = field<u16>(elem, kElemChildCount);
        } else {
            depth = 0;
            elem = 0;
            list = data + kXmlbTop;
            count = field<u16>(data, kXmlbTopCount);
        }
    }
    if (count == 0) return ret(ctx, 0u);

    u32 i = 0;
    while (load<u32>(load<u32>(list + i * 4)) != hash)
        if (++i >= count) return ret(ctx, 0u);

    const u32 child = load<u32>(list + i * 4);
    const u32 out = XmlDoc_NextCursor(doc, depth);
    const u8 flags = ref<u8>(out + kCurFlags);
    ref<u8>(out + kCurDepth) = static_cast<u8>(depth);
    field<u32>(out, kCurParent) = elem;
    field<u16>(out, kCurIndex) = static_cast<u16>(i);
    ref<u8>(out + kCurFlags) = static_cast<u8>(flags | kCurValid);
    field<u32>(out, kCurDoc) = doc;
    field<u32>(out, kCurHash) = load<u32>(child + kElemHash);
    ret(ctx, out);
}

// XmlCursor* XmlCursor_NextSiblingRaw(XmlCursor* cursor, u32 tag_hash)
// The next element after this one, under the same parent, with that tag.
DECOMP(004F8C70, XmlCursor_NextSiblingRaw) {
    const u32 cursor = arg<u32>(ctx, 0);
    const u32 hash = arg<u32>(ctx, 1);
    if ((ref<u8>(cursor + kCurFlags) & kCurValid) == 0) return ret(ctx, 0u);

    const u32 parent = field<u32>(cursor, kCurParent);
    const u32 doc = field<u32>(cursor, kCurDoc);
    u32 list, count;
    if (parent != 0) {
        list = field<u32>(parent, kElemChildren);
        count = field<u16>(parent, kElemChildCount);
    } else {
        const u32 data = field<u32>(doc, kDocData);
        list = data + kXmlbTop;
        count = field<u16>(data, kXmlbTopCount);
    }

    u32 i = field<u16>(cursor, kCurIndex) + 1u;
    for (;; ++i) {
        if (i >= count) return ret(ctx, 0u);
        if (load<u32>(load<u32>(list + i * 4)) == hash) break;
    }

    const u32 child = load<u32>(list + i * 4);
    const u32 depth = ref<u8>(cursor + kCurDepth);
    const u32 out = XmlDoc_NextCursor(doc, depth);
    // `out` can be `cursor` itself once the ring has gone round; the order
    // of reads and writes below is the original's, which copes with that.
    ref<u8>(out + kCurFlags) |= kCurValid;
    ref<u8>(out + kCurDepth) = ref<u8>(cursor + kCurDepth);
    const u32 parent2 = field<u32>(cursor, kCurParent);
    const u32 child_hash = load<u32>(child + kElemHash);
    field<u32>(out, kCurParent) = parent2;
    field<u16>(out, kCurIndex) = static_cast<u16>(i);
    const u32 doc2 = field<u32>(cursor, kCurDoc);
    field<u32>(out, kCurHash) = child_hash;
    field<u32>(out, kCurDoc) = doc2;
    ret(ctx, out);
}

// XmlCursor* XmlCursor_FirstChild(XmlCursor* cursor, u32 tag_hash)
// FirstChildRaw, skipping cursors whose AsElement() is 0 (in practice it
// always returns the cursor).
DECOMP(004F92BC, XmlCursor_FirstChild) {
    const u32 cursor = arg<u32>(ctx, 0);
    const u32 hash = arg<u32>(ctx, 1);
    for (u32 c = call32(ctx, xfn::XmlCursor_FirstChildRaw, {cursor, hash}); c != 0;
         c = call32(ctx, xfn::XmlCursor_NextSiblingRaw, {c, hash})) {
        if (static_cast<u32>(call_ptr(ctx, vfunc(c, kCurVfnAsElement), {c})) != 0)
            return ret(ctx, call_ptr(ctx, vfunc(c, kCurVfnAsElement), {c}));
    }
    ret(ctx, 0u);
}

// XmlCursor* XmlCursor_NextSibling(XmlCursor* cursor, u32 tag_hash)
DECOMP(004F8FC8, XmlCursor_NextSibling) {
    const u32 cursor = arg<u32>(ctx, 0);
    const u32 hash = arg<u32>(ctx, 1);
    for (u32 c = call32(ctx, xfn::XmlCursor_NextSiblingRaw, {cursor, hash}); c != 0;
         c = call32(ctx, xfn::XmlCursor_NextSiblingRaw, {c, hash})) {
        if (static_cast<u32>(call_ptr(ctx, vfunc(c, kCurVfnAsElement), {c})) != 0)
            return ret(ctx, call_ptr(ctx, vfunc(c, kCurVfnAsElement), {c}));
    }
    ret(ctx, 0u);
}

// const char* XmlCursor_FindAttr(XmlCursor* cursor, u32 attr_hash)
// The attribute's text, or 0 when it is missing or isn't a string.
DECOMP(004F9534, XmlCursor_FindAttr) {
    const u32 cursor = arg<u32>(ctx, 0);
    const u32 hash = arg<u32>(ctx, 1);
    u32 type = 0, value = 0;
    if (!XmlCursor_Lookup(cursor, hash, type, value) || type != kXmlString) return ret(ctx, 0u);
    ret(ctx, value);
}

// const char* XmlCursor_FindAttrByName(XmlCursor* cursor, const char* name)
// Same, by name (a null name looks for hash 0).
DECOMP(004F9628, XmlCursor_FindAttrByName) {
    const u32 cursor = arg<u32>(ctx, 0);
    const u32 name = arg<u32>(ctx, 1);
    const u32 hash = name != 0 ? call32(ctx, fn::String_HashNoCase, {name}) : 0;
    u32 type = 0, value = 0;
    if (!XmlCursor_Lookup(cursor, hash, type, value) || type != kXmlString) return ret(ctx, 0u);
    ret(ctx, value);
}

// XmlRead XmlCursor_ReadBool(XmlCursor* cursor, u32 attr_hash, u8* out, u8 default)
// An int attribute, as 0 or 1.
DECOMP(004F986C, XmlCursor_ReadBool) {
    const u32 cursor = arg<u32>(ctx, 0), hash = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    const u8 fallback = arg<u8>(ctx, 3);
    u32 type = 0, value = 0;
    if (!XmlCursor_Lookup(cursor, hash, type, value)) {
        store<u8>(out, fallback);
        return ret(ctx, static_cast<u32>(kXmlMissing));
    }
    if (type != kXmlInt) {
        store<u8>(out, fallback);
        return ret(ctx, static_cast<u32>(kXmlWrongType));
    }
    store<u8>(out, load<u32>(value) != 0 ? 1 : 0);
    ret(ctx, static_cast<u32>(kXmlOk));
}

// XmlRead XmlCursor_ReadS32(XmlCursor* cursor, u32 attr_hash, s32* out, s32 default)
// An int, or a float truncated toward zero (fctiwz: saturating, NaN -> INT_MIN).
DECOMP(004F99BC, XmlCursor_ReadS32) {
    const u32 cursor = arg<u32>(ctx, 0), hash = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    const u32 fallback = arg<u32>(ctx, 3);
    u32 type = 0, value = 0;
    if (!XmlCursor_Lookup(cursor, hash, type, value)) {
        store<u32>(out, fallback);
        return ret(ctx, static_cast<u32>(kXmlMissing));
    }
    if (type == kXmlInt) {
        store<u32>(out, load<u32>(value));
    } else if (type == kXmlFloat) {
        store<u32>(out, static_cast<u32>(rt::fctiw(static_cast<f64>(load<f32>(value)), true)));
    } else {
        store<u32>(out, fallback);
        return ret(ctx, static_cast<u32>(kXmlWrongType));
    }
    ret(ctx, static_cast<u32>(kXmlOk));
}

// XmlRead XmlCursor_ReadF32(XmlCursor* cursor, u32 attr_hash, f32* out), default in f1
// A float, or an int converted to float.
DECOMP(004F9B20, XmlCursor_ReadF32) {
    const u32 cursor = arg<u32>(ctx, 0), hash = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    const f32 fallback = static_cast<f32>(farg(ctx, 0));  // stfs
    u32 type = 0, value = 0;
    if (!XmlCursor_Lookup(cursor, hash, type, value)) {
        store<f32>(out, fallback);
        return ret(ctx, static_cast<u32>(kXmlMissing));
    }
    if (type == kXmlFloat) {
        store<u32>(out, load<u32>(value));  // lfs + stfs: the bits are copied
    } else if (type == kXmlInt) {
        const s32 n = static_cast<s32>(load<u32>(value));
        store<f32>(out, static_cast<f32>(static_cast<f64>(n)));  // fcfid + frsp
    } else {
        store<f32>(out, fallback);
        return ret(ctx, static_cast<u32>(kXmlWrongType));
    }
    ret(ctx, static_cast<u32>(kXmlOk));
}

namespace {
// Copy `words` 32-bit values, one at a time (the default paths: lfs/stfs pairs).
void Xml_CopyWords(u32 dst, u32 src, u32 words) {
    for (u32 k = 0; k < words; ++k) store<u32>(dst + k * 4, load<u32>(src + k * 4));
}
// Copy them reading all first (the found paths).
void Xml_CopyWordsBuffered(u32 dst, u32 src, u32 words) {
    u32 tmp[3];
    for (u32 k = 0; k < words; ++k) tmp[k] = load<u32>(src + k * 4);
    for (u32 k = 0; k < words; ++k) store<u32>(dst + k * 4, tmp[k]);
}
void Xml_ReadVector(PPUContext& ctx, u32 want, u32 words) {
    const u32 cursor = arg<u32>(ctx, 0), hash = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    const u32 fallback = arg<u32>(ctx, 3);
    u32 type = 0, value = 0;
    if (!XmlCursor_Lookup(cursor, hash, type, value)) {
        Xml_CopyWords(out, fallback, words);
        return ret(ctx, static_cast<u32>(kXmlMissing));
    }
    if (type != want) {
        Xml_CopyWords(out, fallback, words);
        return ret(ctx, static_cast<u32>(kXmlWrongType));
    }
    Xml_CopyWordsBuffered(out, value, words);
    ret(ctx, static_cast<u32>(kXmlOk));
}
}  // namespace

// XmlRead XmlCursor_ReadVec2(XmlCursor* cursor, u32 attr_hash, f32 out[2], const f32 default[2])
DECOMP(004F9C9C, XmlCursor_ReadVec2) { Xml_ReadVector(ctx, kXmlVec2, 2); }

// XmlRead XmlCursor_ReadVec3(XmlCursor* cursor, u32 attr_hash, f32 out[3], const f32 default[3])
DECOMP(004F9E0C, XmlCursor_ReadVec3) { Xml_ReadVector(ctx, kXmlVec3, 3); }

// XmlRead XmlCursor_ReadColor(XmlCursor* cursor, u32 attr_hash, u8 out[4], const u8 default[4])
// Four bytes (RGBA?).
DECOMP(004F9F9C, XmlCursor_ReadColor) {
    const u32 cursor = arg<u32>(ctx, 0), hash = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    const u32 fallback = arg<u32>(ctx, 3);
    u32 type = 0, value = 0;
    const bool found = XmlCursor_Lookup(cursor, hash, type, value);
    if (!found || type != kXmlColor) {
        for (u32 k = 0; k < 4; ++k) store<u8>(out + k, load<u8>(fallback + k));
        return ret(ctx, static_cast<u32>(found ? kXmlWrongType : kXmlMissing));
    }
    u8 bytes[4];
    for (u32 k = 0; k < 4; ++k) bytes[k] = load<u8>(value + k);
    for (u32 k = 0; k < 4; ++k) store<u8>(out + k, bytes[k]);
    ret(ctx, static_cast<u32>(kXmlOk));
}

// XmlRead XmlCursor_ReadHash(XmlCursor* cursor, u32 attr_hash, u32* out, u32 default)
// A hash attribute, an int, or the hash stored in front of a string.
DECOMP(004FA14C, XmlCursor_ReadHash) {
    const u32 cursor = arg<u32>(ctx, 0), hash = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    const u32 fallback = arg<u32>(ctx, 3);
    u32 type = 0, value = 0;
    if (!XmlCursor_Lookup(cursor, hash, type, value)) {
        store<u32>(out, fallback);
        return ret(ctx, static_cast<u32>(kXmlMissing));
    }
    if (type == kXmlString) {
        store<u32>(out, load<u32>(value - 4));
    } else if (type == kXmlInt || type == kXmlHash) {
        store<u32>(out, load<u32>(value));
    } else {
        store<u32>(out, fallback);
        return ret(ctx, static_cast<u32>(kXmlWrongType));
    }
    ret(ctx, static_cast<u32>(kXmlOk));
}
