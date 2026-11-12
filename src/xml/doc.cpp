// XmlDoc: loading a binary XML ("XMLB") file.
#include "xml.h"

#include "../heap/heap.h"
#include "../memory/mem.h"
#include "../string/str.h"

namespace sly4::xml {
using namespace opentit::native;

namespace {

constexpr u32 kPathBufferSize = 128;
constexpr u32 kCursorDocType = 0, kCursorElementType = 1;

void XmlCursor_Init(XmlCursor* c, u32 vtable, u32 type) {
    c->u12 = -1;
    c->u8_ = -1;
    c->doc = nullptr;
    c->type = type;
    c->hash = 0;
    c->flags &= static_cast<u8>(~kCurValid);
    c->parent = nullptr;
    c->depth = 0;
    c->index = 0;
    c->vtable = vtable;
}

}  // namespace

// XmlDoc(const char* path): the document cursor, its 64 element cursors and
// the path (the file is read by XmlDoc_Parse).
void XmlDoc_Construct(XmlDoc* doc, const char* path) {
    XmlCursor_Init(&doc->self, kDocVtable, kCursorDocType);
    doc->u40 = -1;
    doc->u36 = -1;
    for (u32 k = 0; k < kDocCursorCount; ++k) XmlCursor_Init(&doc->cursors[k / 4][k % 4], kCursorVtable, kCursorElementType);
    // The status bit-fields are set in two goes around the String_Copy, and
    // +36/+40 go from -1 to 0, as in the original.
    u32 status = doc->status;
    doc->data = nullptr;
    status = (status & ~kStatusFieldMask) | ((4u << kStatusFieldShift) & kStatusFieldMask);
    status &= ~kStatus40000000;
    doc->status = status;
    str::String_Copy(doc->path, path, kDocPathSize);
    status = doc->status;
    doc->u40 = 0;
    doc->u36 = 0;
    doc->status = status & ~kStatusFailed & ~kStatusErrorMask;
    for (u32 k = 0; k < kDocDepths; ++k) doc->ring[k] = 0;
}

void XmlDoc_Destruct(XmlDoc* doc) {
    doc->self.vtable = kDocVtable;
    if (doc->data) {
        LinearHeap* stack = memory::Mem_GetDefaultStack();
        vcall(stack, kStackVfnFree, doc->data.get());
    }
    for (u32 k = kDocCursorCount; k-- > 0;) doc->cursors[k / 4][k % 4].vtable = kCursorBaseVtable;
    doc->self.vtable = kCursorBaseVtable;
}

// Opens the document's path with the extension swapped for ".xmb" and
// loads it. Returns whether it loaded.
u32 XmlDoc_Parse(XmlDoc* doc, u32 unused) {
    // The document's path, through its AsDocument(); a non-document gets 0.
    u32 name = 0;
    if (doc->self.type == kCursorDocType) name = vcall<u32>(doc, kDocVfnAsDocument) + kDocPath;
    const s32 len = str::String_Length(host<char>(name));
    u32 dot = name + static_cast<u32>(len) - 1;  // the last '.', or name - 1 when there is none
    while (dot >= name && gref<u8>(dot) != '.') --dot;
    GuestFrame frame(kPathBufferSize);
    char* buf = frame.as<char>();
    char* out = buf;
    for (u32 p = name; p < dot; ++p) *out++ = static_cast<char>(gref<u8>(p));
    call(xfn::String_Printf, out, kXmbSuffix);  // variadic: through the guest ABI
    str::String_Copy(doc->path, buf, kDocPathSize);
    void* file = call<void*>(fn::File_Open, buf, kModeRead);
    if (!file) {
        XmlDoc_SetError(doc, kErrorRead);
        return 0;
    }
    const u32 ok = XmlDoc_LoadXmlb(doc, file, unused);
    vcall(file, kFileVfnClose);
    return ok;
}

// Reads the whole file into a block from the default memory stack, checks
// the "XMLB" magic and turns every offset in it into a pointer.
u32 XmlDoc_LoadXmlb(XmlDoc* doc, void* file, u32 /*unused*/) {
    if (!file) {
        XmlDoc_SetError(doc, kErrorRead);
        return 0;
    }
    doc->self.u12 = -1;
    doc->self.u8_ = -1;
    const s32 size = vcall<s32>(file, kFileVfnSize);
    if (size == 0) {
        XmlDoc_SetError(doc, kErrorEmpty);
        return 0;
    }
    LinearHeap* stack = memory::Mem_GetDefaultStack();
    u8* buf = static_cast<u8*>(heap::MemStack_Alloc(stack, static_cast<u32>(size) + 1));
    gref<u8>(guest(buf)) = 0;
    const s32 got = vcall<s32>(file, kFileVfnRead, buf, static_cast<u32>(size), 0u);
    if (got != size) {
        LinearHeap* s = memory::Mem_GetDefaultStack();
        vcall(s, kStackVfnFree, buf);
        XmlDoc_SetError(doc, kErrorRead);
        return 0;
    }
    gref<u8>(guest(buf) + static_cast<u32>(size)) = 0;
    str::String_Copy(doc->path, host<char>(guest(file) + kFileName), kDocPathSize);
    if (str::Mem_Compare(buf, host<char>(kXmlbMagic), kXmlbMagicSize) != 0) return 0;
    doc->data = reinterpret_cast<XmlbFile*>(buf);
    XmlbFile* data = doc->data;
    for (u32 k = 0; k < data->top_count; ++k) {
        be_u32& slot = gref<be_u32>(guest(buf) + kXmlbTop + k * 4);
        const u32 elem = slot + doc->data.ea;
        slot = elem;
        XmlElement_Relocate(host<XmlbElement>(elem), doc->data.ea);
    }
    doc->self.flags |= kCurValid;
    return 1;
}

// Adds `base` to the element's pointers (name, text, attributes and their
// values, children) and relocates the children.
void XmlElement_Relocate(XmlbElement* elem, u32 base) {
    const u32 name = elem->name, text = elem->text;
    const u32 children = elem->children.ea;
    u32 attrs = elem->attrs.ea;
    const u32 attr_count = elem->attr_count;
    const u32 child_count = elem->child_count;
    if (name != 0) elem->name = name + base;
    if (text != 0) elem->text = text + base;
    if (attrs != 0) {
        attrs += base;
        elem->attrs.ea = attrs;
    }
    // Attribute values are relocated even when the array pointer was 0.
    for (u32 k = 0; k < attr_count; ++k) gref<be_u32>(attrs + k * sizeof(XmlbAttr) + 4) += base;
    if (children != 0) elem->children.ea = elem->children.ea + base;
    if (child_count == 0) return;
    for (u32 k = 0; k < elem->child_count; ++k) {
        be_u32& c = gref<be_u32>(elem->children.ea + k * 4);
        c += base;
        XmlElement_Relocate(host<XmlbElement>(gref<be_u32>(elem->children.ea + k * 4)), base);
    }
}

}  // namespace sly4::xml

NATIVE(004FA888, sly4::xml::XmlDoc_Construct)
NATIVE(004FA9C0, sly4::xml::XmlDoc_Destruct)
NATIVE(004FAF10, sly4::xml::XmlDoc_Parse)
NATIVE(004FACCC, sly4::xml::XmlDoc_LoadXmlb)
NATIVE(004FABB4, sly4::xml::XmlElement_Relocate)
