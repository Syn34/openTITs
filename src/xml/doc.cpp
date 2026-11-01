// XmlDoc: construction, loading the binary .xmb file, and destruction.
#include "xml.h"

using namespace sly4;
using namespace sly4::xml;

namespace {

constexpr u32 kPathBufferSize = 128;
constexpr u32 kCursorDocType = 0, kCursorElementType = 1;

// The fields every XmlCursor constructor sets (the document's own cursor
// and the 64 in the pool differ only in type and vtable).
void XmlCursor_Init(u32 cursor, u32 vtable, u32 type) {
    field<s32>(cursor, kCur12) = -1;
    field<s32>(cursor, kCur8) = -1;
    field<u32>(cursor, kCurDoc) = 0;
    field<u32>(cursor, kCurType) = type;
    field<u32>(cursor, kCurHash) = 0;
    ref<u8>(cursor + kCurFlags) &= static_cast<u8>(~kCurValid);
    field<u32>(cursor, kCurParent) = 0;
    ref<u8>(cursor + kCurDepth) = 0;
    field<u16>(cursor, kCurIndex) = 0;
    field<u32>(cursor, kCurVtable) = vtable;
}

}  // namespace

// void XmlDoc_Construct(XmlDoc* doc, const char* path)
DECOMP(004FA888, XmlDoc_Construct) {
    const u32 doc = arg<u32>(ctx, 0);
    const u32 path = arg<u32>(ctx, 1);

    XmlCursor_Init(doc, kDocVtable, kCursorDocType);
    field<s32>(doc, kDoc40) = -1;
    field<s32>(doc, kDoc36) = -1;
    for (u32 k = 0; k < kDocCursorCount; ++k)
        XmlCursor_Init(doc + kDocCursors + k * kCursorSize, kCursorVtable, kCursorElementType);

    // The status bit-fields are set in two goes around the String_Copy, and
    // +36/+40 go from -1 to 0, as in the original.
    u32 status = field<u32>(doc, kDocStatus);
    field<u32>(doc, kDocData) = 0;
    status = (status & ~kStatusFieldMask) | ((4u << kStatusFieldShift) & kStatusFieldMask);
    status &= ~kStatus40000000;
    field<u32>(doc, kDocStatus) = status;
    call(ctx, fn::String_Copy, {doc + kDocPath, path, kDocPathSize});
    status = field<u32>(doc, kDocStatus);
    field<u32>(doc, kDoc40) = 0;
    field<u32>(doc, kDoc36) = 0;
    field<u32>(doc, kDocStatus) = status & ~kStatusFailed & ~kStatusErrorMask;
    for (u32 k = 0; k < kDocDepths; k += 4) field<u32>(doc, kDocRing + k) = 0;
}

// void XmlDoc_Destruct(XmlDoc* doc)
// Frees the XMLB buffer and runs the cursor destructors (last to first).
DECOMP(004FA9C0, XmlDoc_Destruct) {
    const u32 doc = arg<u32>(ctx, 0);
    field<u32>(doc, kCurVtable) = kDocVtable;
    if (field<u32>(doc, kDocData) != 0) {
        const u32 stack = call32(ctx, xfn::Mem_GetDefaultStack, {doc});
        call_ptr(ctx, vfunc(stack, kStackVfnFree), {stack, field<u32>(doc, kDocData)});
    }
    for (u32 k = kDocCursorCount; k-- > 0;)
        field<u32>(doc, kDocCursors + k * kCursorSize + kCurVtable) = kCursorBaseVtable;
    field<u32>(doc, kCurVtable) = kCursorBaseVtable;
}

// bool XmlDoc_Parse(XmlDoc* doc, u32 unused)
// Opens the document's path with its extension replaced by ".xmb" (or
// ".xmb" appended when it has none) and loads it.
DECOMP(004FAF10, XmlDoc_Parse) {
    const u32 doc = arg<u32>(ctx, 0);
    const u32 unused = arg<u32>(ctx, 1);

    // The document's path, through its AsDocument(); a non-document gets 0.
    u32 name = 0;
    if (field<u32>(doc, kCurType) == kCursorDocType)
        name = static_cast<u32>(call_ptr(ctx, vfunc(doc, kDocVfnAsDocument), {doc})) + kDocPath;

    const s32 len = static_cast<s32>(call32(ctx, xfn::String_Length, {name}));
    u32 dot = name + static_cast<u32>(len) - 1;  // the last '.', or name - 1 when there is none
    while (dot >= name && load<u8>(dot) != '.') --dot;

    Stack frame(ctx, kPathBufferSize);
    const u32 buf = frame.addr();
    u32 out = buf;
    for (u32 p = name; p < dot; ++p) store<u8>(out++, load<u8>(p));
    call(ctx, xfn::String_Printf, {out, kXmbSuffix});
    call(ctx, fn::String_Copy, {doc + kDocPath, buf, kDocPathSize});

    const u32 file = call32(ctx, fn::File_Open, {buf, kModeRead});
    if (file == 0) {
        XmlDoc_SetError(doc, kErrorRead);
        return ret(ctx, 0u);
    }
    const u64 ok = call(ctx, xfn::XmlDoc_LoadXmlb, {doc, file, unused});
    call_ptr(ctx, vfunc(file, kFileVfnClose), {file});
    ret(ctx, ok);
}

// bool XmlDoc_LoadXmlb(XmlDoc* doc, File* file, u32 unused)
// Reads the whole file into a buffer from the default memory stack, checks
// the "XMLB" magic and turns its offsets into pointers. On a bad magic the
// buffer is neither kept nor freed, and no error is recorded.
DECOMP(004FACCC, XmlDoc_LoadXmlb) {
    const u32 doc = arg<u32>(ctx, 0);
    const u32 file = arg<u32>(ctx, 1);
    if (file == 0) {
        XmlDoc_SetError(doc, kErrorRead);
        return ret(ctx, 0u);
    }

    field<s32>(doc, kCur12) = -1;
    field<s32>(doc, kCur8) = -1;
    const s32 size = static_cast<s32>(static_cast<u32>(call_ptr(ctx, vfunc(file, kFileVfnSize), {file})));
    if (size == 0) {
        XmlDoc_SetError(doc, kErrorEmpty);
        return ret(ctx, 0u);
    }

    const u32 stack = call32(ctx, xfn::Mem_GetDefaultStack);
    const u32 buf = call32(ctx, xfn::MemStack_Alloc, {stack, static_cast<u32>(size) + 1});
    store<u8>(buf, 0);
    const u32 got = static_cast<u32>(
        call_ptr(ctx, vfunc(file, kFileVfnRead), {file, buf, static_cast<u32>(size), 0}));
    if (static_cast<s32>(got) != size) {
        const u32 s = call32(ctx, xfn::Mem_GetDefaultStack);
        call_ptr(ctx, vfunc(s, kStackVfnFree), {s, buf});
        XmlDoc_SetError(doc, kErrorRead);
        return ret(ctx, 0u);
    }
    store<u8>(buf + static_cast<u32>(size), 0);
    call(ctx, fn::String_Copy, {doc + kDocPath, file + kFileName, kDocPathSize});
    if (call32(ctx, xfn::Mem_Compare, {buf, kXmlbMagic, kXmlbMagicSize}) != 0) return ret(ctx, 0u);

    field<u32>(doc, kDocData) = buf;
    for (u32 k = 0; k < field<u16>(buf, kXmlbTopCount); ++k) {
        const u32 slot = buf + kXmlbTop + k * 4;
        const u32 elem = load<u32>(slot) + field<u32>(doc, kDocData);
        store<u32>(slot, elem);
        call(ctx, xfn::XmlElement_Relocate, {elem, field<u32>(doc, kDocData)});
    }
    ref<u8>(doc + kCurFlags) |= kCurValid;
    ret(ctx, 1u);
}

// void XmlElement_Relocate(XmlbElement* elem, u32 base)
// Adds `base` to the element's pointers (those that aren't 0), its
// attribute values and its children, then does the same for each child.
DECOMP(004FABB4, XmlElement_Relocate) {
    const u32 elem = arg<u32>(ctx, 0);
    const u32 base = arg<u32>(ctx, 1);

    const u32 name = field<u32>(elem, kElemName), text = field<u32>(elem, kElemText);
    const u32 children = field<u32>(elem, kElemChildren);
    u32 attrs = field<u32>(elem, kElemAttrs);
    const u32 attr_count = field<u16>(elem, kElemAttrCount);
    const u32 child_count = field<u16>(elem, kElemChildCount);
    if (name != 0) field<u32>(elem, kElemName) = name + base;
    if (text != 0) field<u32>(elem, kElemText) = text + base;
    if (attrs != 0) {
        attrs += base;
        field<u32>(elem, kElemAttrs) = attrs;
    }
    // Attribute values are relocated even when the array pointer was 0.
    for (u32 k = 0; k < attr_count; ++k) field<u32>(attrs + k * sizeof(XmlbAttr), 4) += base;
    if (children != 0) field<u32>(elem, kElemChildren) = field<u32>(elem, kElemChildren) + base;

    if (child_count == 0) return;
    for (u32 k = 0; k < field<u16>(elem, kElemChildCount); ++k) {
        field<u32>(field<u32>(elem, kElemChildren), k * 4) += base;
        call(ctx, xfn::XmlElement_Relocate, {load<u32>(field<u32>(elem, kElemChildren) + k * 4), base});
    }
}
