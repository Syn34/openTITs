// The XML reader: XmlDoc loads a binary "XMLB" file (the .xml path with its
// extension swapped for .xmb) and hands out XmlCursors that walk its
// elements and read typed attributes. Tag and attribute names are compared
// by String_HashNoCase. Used by Tuning_Load (src/game/load.cpp).
// Addresses are guest addresses in the BCES01284 v01.02 EBOOT. A "?" in a
// comment marks a guess from how the code is used.
#pragma once

#include "../sly4.h"

namespace sly4 {
namespace xml {

// ---------------------------------------------------------------- functions
namespace xfn {
// decompiled in src/xml/doc.cpp
constexpr u32 XmlDoc_Construct = 0x004FA888;   // (XmlDoc*, const char* path)
constexpr u32 XmlDoc_Destruct = 0x004FA9C0;    // (XmlDoc*)
constexpr u32 XmlDoc_Parse = 0x004FAF10;       // (XmlDoc*, u32 unused) -> bool
constexpr u32 XmlDoc_LoadXmlb = 0x004FACCC;    // (XmlDoc*, File*, u32 unused) -> bool
constexpr u32 XmlElement_Relocate = 0x004FABB4;  // (XmlbElement*, u32 base)
// decompiled in src/xml/node.cpp
constexpr u32 XmlCursor_FirstChildRaw = 0x004F8ACC;   // (cursor, tag hash) -> XmlCursor* or 0
constexpr u32 XmlCursor_NextSiblingRaw = 0x004F8C70;  // (cursor, tag hash) -> XmlCursor* or 0
constexpr u32 XmlCursor_FirstChild = 0x004F92BC;      // (cursor, tag hash) -> XmlCursor* or 0
constexpr u32 XmlCursor_NextSibling = 0x004F8FC8;     // (cursor, tag hash) -> XmlCursor* or 0
constexpr u32 XmlCursor_FindAttr = 0x004F9534;        // (cursor, attr hash) -> const char* or 0
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;  // (cursor, const char*) -> const char* or 0
constexpr u32 XmlCursor_ReadBool = 0x004F986C;   // (cursor, hash, u8* out, u8 default) -> XmlRead
constexpr u32 XmlCursor_ReadS32 = 0x004F99BC;    // (cursor, hash, s32* out, s32 default)
constexpr u32 XmlCursor_ReadF32 = 0x004F9B20;    // (cursor, hash, f32* out), default in f1
constexpr u32 XmlCursor_ReadVec2 = 0x004F9C9C;   // (cursor, hash, f32 out[2], const f32 default[2])
constexpr u32 XmlCursor_ReadVec3 = 0x004F9E0C;   // (cursor, hash, f32 out[3], const f32 default[3])
constexpr u32 XmlCursor_ReadColor = 0x004F9F9C;  // (cursor, hash, u8 out[4], const u8 default[4]) (name?)
constexpr u32 XmlCursor_ReadHash = 0x004FA14C;   // (cursor, hash, u32* out, u32 default)
// elsewhere
constexpr u32 Mem_GetDefaultStack = 0x005525FC;  // () -> ApeMemStack* (src/memory/memstack.cpp)
constexpr u32 MemStack_Alloc = 0x0055260C;   // (ApeMemStack*, u32 size) -> pointer; 666 instructions (name?)
constexpr u32 String_Length = 0x00555FA4;    // (const char*) -> s32 (src/string/string_misc.cpp)
constexpr u32 String_Printf = 0x00551540;    // (char* dst, const char* fmt, ...) -> int (sprintf)
constexpr u32 Mem_Compare = 0x00555DB8;      // (const void*, const void*, u32 n) -> int (memcmp)
}  // namespace xfn

// Results of the XmlCursor_Read* functions. Whenever the result isn't kXmlOk
// the default has been stored in *out.
enum XmlRead : u32 {
    kXmlOk = 0,
    kXmlMissing = 1,    // no such attribute (or the cursor isn't valid)
    kXmlWrongType = 2,  // the attribute has another type
};

// Attribute value types: 4 bits per attribute in XmlbElement::types.
enum XmlType : u32 {
    kXmlString = 0,  // value -> { u32 String_HashNoCase(text); char text[]; }
    kXmlInt = 1,     // value -> u32
    kXmlFloat = 2,   // value -> f32
    kXmlVec2 = 3,    // value -> f32[2]
    kXmlVec3 = 4,    // value -> f32[3]
    kXmlColor = 5,   // value -> u8[4]
    kXmlHash = 6,    // value -> u32
};

// ---------------------------------------------------------------- the XMLB file
// Read into one buffer; every pointer in it is an offset from the start of
// the buffer until XmlDoc_LoadXmlb adds the buffer's address.
//   +0  "XMLB"
//   +16 u16 number of top-level elements
//   +20 XmlbElement* top-level elements[]
constexpr u32 kXmlbMagicSize = 4;
constexpr u32 kXmlbTopCount = 16;  // u16
constexpr u32 kXmlbTop = 20;       // u32[]

// An element in the file.
constexpr u32 kElemHash = 0;        // u32 String_HashNoCase(tag)
constexpr u32 kElemName = 4;        // u32 pointer (0 = none)
constexpr u32 kElemText = 8;        // u32 pointer (0 = none) (name?)
constexpr u32 kElemChildCount = 12; // u16
constexpr u32 kElemAttrCount = 14;  // u16
constexpr u32 kElemChildren = 16;   // XmlbElement*[]
constexpr u32 kElemAttrs = 20;      // XmlbAttr[] sorted by hash
constexpr u32 kElemTypes = 24;      // u32[]: XmlType of attribute i in bits (i % 8) * 4 of word i / 8
struct XmlbAttr {
    be_u32 hash;   // String_HashNoCase(name)
    gptr value;    // see XmlType
};
static_assert(sizeof(XmlbAttr) == 8);

// ---------------------------------------------------------------- XmlCursor (32 bytes)
// A position in a document. XmlDoc starts with one (the document itself,
// type 0) and holds 64 more, 4 per depth, reused in turn.
constexpr u32 kCurVtable = 0;   // kCursorVtable; slot 3 (+12) = AsElement() -> this
constexpr u32 kCurType = 4;     // u32: 0 document, 1 element
constexpr u32 kCur8 = 8;        // s32, -1 (unused here)
constexpr u32 kCur12 = 12;      // s32, -1 (unused here)
constexpr u32 kCurDoc = 16;     // XmlDoc* (0 in the document's own cursor)
constexpr u32 kCurHash = 20;    // u32 the element's tag hash
constexpr u32 kCurFlags = 24;   // u8: kCurValid
constexpr u32 kCurDepth = 25;   // u8: 0 for top-level elements
constexpr u32 kCurIndex = 26;   // u16 index in the parent's children (or the top-level list)
constexpr u32 kCurParent = 28;  // XmlbElement* parent, 0 for top-level elements
constexpr u32 kCursorSize = 32;
constexpr u8 kCurValid = 0x80;
constexpr u32 kCurVfnAsElement = 12;

// ---------------------------------------------------------------- XmlDoc (2240 bytes)
constexpr u32 kDocStatus = 32;     // u32, see XmlStatus
constexpr u32 kDoc36 = 36, kDoc40 = 40;  // u32, cleared
constexpr u32 kDocData = 44;       // u8* the XMLB buffer (from Mem_GetDefaultStack), 0 until loaded
constexpr u32 kDocPath = 48;       // char[128]
constexpr u32 kDocPathSize = 127;  // as passed to String_Copy
constexpr u32 kDocRing = 176;      // u8[16]: next cursor slot per depth
constexpr u32 kDocCursors = 192;   // XmlCursor[16][4]
constexpr u32 kDocDepths = 16, kDocSlotsPerDepth = 4, kDocCursorCount = 64;
constexpr u32 kDocDepthStride = kDocSlotsPerDepth * kCursorSize;  // 128
constexpr u32 kDocSize = 2240;
constexpr u32 kDocVfnAsDocument = 8;  // slot 2 = AsDocument() -> this

// XmlDoc +32 bit-fields.
enum XmlStatus : u32 {
    kStatusFailed = 0x80000000,     // set with the first error
    kStatus40000000 = 0x40000000,   // cleared at construction
    kStatusErrorMask = 0x3FC00000,  // error code << 22
    kStatusFieldMask = 0x003FC000,  // set to 4 << 14 at construction (version?)
};
constexpr u32 kStatusErrorShift = 22, kStatusFieldShift = 14;
constexpr u32 kErrorRead = 2;    // file missing, unreadable or short
constexpr u32 kErrorEmpty = 13;  // file of size 0

constexpr u32 kDocVtable = 0x00B230D0;     // XmlDoc
constexpr u32 kCursorVtable = 0x00B23090;  // XmlCursor
constexpr u32 kCursorBaseVtable = 0x00B23080;  // set by the destructors
constexpr u32 kXmbSuffix = 0x00AF0EC0;     // ".xmb"
constexpr u32 kModeRead = 0x00AF0EC8;      // "rb"
constexpr u32 kXmlbMagic = 0x00AF0EB8;     // "XMLB"

// File virtual functions (byte offsets into the vtable).
constexpr u32 kFileVfnClose = 8;   // ()
constexpr u32 kFileVfnRead = 12;   // (void* buf, u32 size, 0) -> u32 bytes read
constexpr u32 kFileVfnSize = 24;   // () -> s32
constexpr u32 kFileName = 4;       // char[] the path it was opened with
// ApeMemStack virtual functions.
constexpr u32 kStackVfnFree = 8;   // (void*)

// ---------------------------------------------------------------- helpers
// Inlined into every function that reads the document: record an error
// unless one is already recorded.
inline void XmlDoc_SetError(u32 doc, u32 code) {
    auto& status = field<u32>(doc, kDocStatus);
    const u32 s = status;
    if (s & kStatusFailed) return;
    status = ((s | kStatusFailed) & ~kStatusErrorMask) | ((code << kStatusErrorShift) & kStatusErrorMask);
}

// The XMLB element a (valid, element) cursor stands for.
inline u32 XmlCursor_Element(u32 cursor) {
    const u32 parent = field<u32>(cursor, kCurParent);
    const u32 index = field<u16>(cursor, kCurIndex);
    if (parent != 0) return load<u32>(load<u32>(parent + kElemChildren) + index * 4);
    const u32 data = field<u32>(load<u32>(cursor + kCurDoc), kDocData);
    return load<u32>(data + index * 4 + kXmlbTop);
}

// Binary search of an element's attributes by hash: the attribute (and its
// index), or 0.
inline u32 XmlElement_FindAttr(u32 elem, u32 hash, s32& index) {
    s32 hi = static_cast<s32>(field<u16>(elem, kElemAttrCount).get()) - 1, lo = 0;
    if (hi < 0) return 0;
    do {
        const s32 mid = (hi + lo) >> 1;
        const u32 attr = field<u32>(elem, kElemAttrs) + static_cast<u32>(mid) * sizeof(XmlbAttr);
        const u32 h = load<u32>(attr);
        if (h > hash) {
            hi = mid - 1;
        } else if (h < hash) {
            lo = mid + 1;
        } else {
            index = mid;
            return attr;
        }
    } while (hi >= lo);
    return 0;
}

// XmlType of attribute `index`.
inline u32 XmlElement_AttrType(u32 elem, s32 index) {
    const u32 i = static_cast<u32>(index);
    return (load<u32>(elem + kElemTypes + ((i >> 1) & 0x7FFFFFFC)) >> ((i << 2) & 0x1C)) & 0xF;
}

// Where an attribute's value is: strings skip their 4-byte hash.
inline u32 XmlAttr_Value(u32 attr, u32 type) {
    const u32 v = load<u32>(attr + 4);
    return type == kXmlString ? v + 4 : v;
}

// The lookup every XmlCursor_Read* starts with: false when the cursor
// isn't valid, the attribute is missing or its value pointer is 0.
inline bool XmlCursor_Lookup(u32 cursor, u32 hash, u32& type, u32& value) {
    if ((ref<u8>(cursor + kCurFlags) & kCurValid) == 0) return false;
    const u32 elem = XmlCursor_Element(cursor);
    s32 index = 0;
    const u32 attr = XmlElement_FindAttr(elem, hash, index);
    if (attr == 0) return false;
    type = XmlElement_AttrType(elem, index);
    value = XmlAttr_Value(attr, type);
    return value != 0;
}

inline u32 vfunc(u32 obj, u32 byte_offset) { return load<u32>(load<u32>(obj) + byte_offset); }
inline u32 call32(PPUContext& ctx, u32 addr, std::initializer_list<u64> args = {}) {
    return static_cast<u32>(call(ctx, addr, args));
}

}  // namespace xml
}  // namespace sly4
