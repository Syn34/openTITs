// TlKeyEvent_FromXml / TlKeyAnim_FromXml: event and animation keys of a
// <timeline> definition.
#include "tlbuild.h"

#include "../memory/mem.h"
#include "../uiload/uiload.h"

namespace sly4::tlbuild {
using namespace opentit::native;
using xml::XmlCursor;

namespace {

// The index of `hash` in a table of `n` u32 hashes, or n.
u32 IndexOf(u32 table, u32 n, u32 hash) {
    const be_u32* t = host<be_u32>(table);
    u32 i = 0;
    while (i < n && t[i] != hash) ++i;
    return i;
}

// Reads the enum attribute `attr` into an index of `table`; reports a
// value that isn't in it (index n). A failed read gives 0, reported only
// when `report_fail`.
u32 ReadEnum(XmlCursor* doc, XmlCursor* node, u32 attr, be_u32* out, u32 table, u32 n, bool report_fail) {
    if (xml::XmlCursor_ReadHash(node, attr, out, 0) != 0) {
        if (report_fail) uiload::XmlCursor_DocPath(doc);
        return 0;
    }
    const u32 i = IndexOf(table, n, *out);
    if (i == n) uiload::XmlCursor_DocPath(doc);
    return i;
}

enum class Kind { None, F32, Vec2, Vec3, S32, Color, Flags };

// The value type of each animation property (the original's compare tree).
Kind PropKind(u32 p) {
    if (p == 0 || p >= 26) return Kind::None;
    if (p == 1 || p == 15) return Kind::Vec3;
    if (p == 18) return Kind::Vec2;
    if ((p >= 2 && p <= 5) || p == 16 || p == 17 || p == 19 || p == 20) return Kind::F32;
    if ((p >= 7 && p <= 10) || (p >= 21 && p <= 23)) return Kind::S32;
    if (p == 6 || (p >= 11 && p <= 14)) return Kind::Color;
    return Kind::Flags;  // 24, 25
}

// Flag letters (either case): T / M / B set bits 4-5 to 0 / 1 / 2, L / C /
// R set bits 0-1 to 0 / 1 / 2.
void ParseFlags(u32 s, be_u32& word) {
    for (u32 p = s;;) {
        const s32 c = static_cast<s8>(gref<u8>(p));
        if (c == 0) return;
        const u32 w = word;
        switch (c) {
            case 'T': case 't': word = w & ~0x30u; break;
            case 'M': case 'm': word = (w & ~0x30u) | 0x10; break;
            case 'B': case 'b': word = (w & ~0x30u) | 0x20; break;
            case 'L': case 'l': word = w & ~0x3u; break;
            case 'C': case 'c': word = (w & ~0x3u) | 1; break;
            case 'R': case 'r': word = (w & ~0x3u) | 2; break;
            default: break;
        }
        ++p;
        if (p == 0) return;  // the original stops when the pointer wraps
    }
}

}  // namespace

// An 80-byte event key: the target name and the event (index in a table
// of 17 hashes; 17 when unknown, which is reported, 0 when unreadable).
// As in the original, the fields are still written when the allocation
// fails (at guest address 0).
TlKeyEvent* TlKeyEvent_FromXml(XmlCursor* doc, XmlCursor* node, void*, f32 time) {
    GuestFrame frame(16);
    be_u32* hash = frame.as<be_u32>();
    const u32 key = guest(memory::Mem_Alloc(kEventKeySize, 0, 16));
    TlKeyEvent& k = gref<TlKeyEvent>(key);
    if (key != 0) {
        k.next = 0u;
        k.time = time;
        k.index = 0u;
        k.vtable = kTlKeyEventVtbl;
        k.target[0] = 0;
    }
    if (const char* target = xml::XmlCursor_FindAttrByName(node, host<const char>(kStrTarget)))
        call(bfn::String_CopyN, k.target, target, 63u);
    u32 index = 0;
    if (xml::XmlCursor_ReadHash(node, kHashEvent, hash, 0) != 0) {
        uiload::XmlCursor_DocPath(doc);
    } else {
        index = IndexOf(kEventNames, kEventCount, *hash);
        if (index == kEventCount) {
            uiload::XmlCursor_DocPath(doc);
            xml::XmlCursor_FindAttr(node, kHashEvent);  // result unused
        }
    }
    k.index = index;
    return host<TlKeyEvent>(key);
}

// A 128-byte animation key: an f32 at +24, the target name, the animated
// property (index in a table of 26), its "from" / "to" values read as the
// property's type (the "to" attribute's hash depends on the tag), and two
// enums (+16 of 4, +20 of 3). Unknown enum values are reported. As in the
// original, the fields are still written when the allocation fails.
TlKeyAnim* TlKeyAnim_FromXml(XmlCursor* doc, XmlCursor* node, void*, f32 time) {
    GuestFrame frame(208 - 112);
    be_u32* fallback3 = frame.as<be_u32>(0);    // sp+112
    be_u32* prop_hash = frame.as<be_u32>(16);   // sp+128
    be_u32* ease_hash = frame.as<be_u32>(20);   // sp+132
    be_u32* mode_hash = frame.as<be_u32>(24);   // sp+136
    be_u32* fallback2 = frame.as<be_u32>(28);   // sp+140
    const u32 key = guest(memory::Mem_Alloc(kAnimKeySize, 0, 16));
    TlKeyAnim& k = gref<TlKeyAnim>(key);
    if (key != 0) {
        k.next = 0u;
        k.time = time;
        k.prop = 0u;
        k.vtable = kTlKeyAnimVtbl;
        k.ease = 0u;
        k.mode = 0u;
        reinterpret_cast<be_u32&>(k.f24) = 0u;
        k.target[0] = 0;
        call(bfn::memset, k.from, 0u, 32u);
    }
    const u32 tag = (node->flags & 0x80) != 0 ? node->hash.get() : 0u;
    const f32 zero = gref<be_f32>(kF32Zero);
    xml::XmlCursor_ReadF32(node, kHashAnimF24, &k.f24, zero);
    if (const char* target = xml::XmlCursor_FindAttrByName(node, host<const char>(kStrTarget)))
        call(bfn::String_CopyN, k.target, target, 63u);

    const u32 prop = ReadEnum(doc, node, kHashProp, prop_hash, kAnimProps, kAnimPropCount, true);
    k.prop = prop;
    const u32 to = tag == kTagAnimA ? kHashToA : kHashToB;
    be_u32* from_out = k.from;
    be_u32* to_out = k.to;

    switch (PropKind(prop)) {
        case Kind::None: break;
        case Kind::F32:
            xml::XmlCursor_ReadF32(node, kHashFrom, reinterpret_cast<be_f32*>(from_out), gref<be_f32>(kF32Max));
            xml::XmlCursor_ReadF32(node, to, reinterpret_cast<be_f32*>(to_out), zero);
            break;
        case Kind::Vec3:
            for (u32 j = 0; j < 3; ++j) fallback3[j] = kFltMaxBits;
            xml::XmlCursor_ReadVec3(node, kHashFrom, from_out, fallback3);
            xml::XmlCursor_ReadVec3(node, to, to_out, host<be_u32>(kDefaultVec3));
            break;
        case Kind::Vec2:
            fallback2[0] = kFltMaxBits;
            fallback2[1] = kFltMaxBits;
            xml::XmlCursor_ReadVec2(node, kHashFrom, from_out, fallback2);
            xml::XmlCursor_ReadVec2(node, to, to_out, host<be_u32>(kDefaultVec2));
            break;
        case Kind::S32:
            if (xml::XmlCursor_ReadS32(node, kHashFrom, from_out, 0) != 0) *from_out = kFltMaxBits;
            xml::XmlCursor_ReadS32(node, to, to_out, 0);
            break;
        case Kind::Color:
            if (xml::XmlCursor_ReadColor(node, kHashFrom, reinterpret_cast<u8*>(from_out), host<u8>(kDefaultColor)) != 0)
                *from_out = kFltMaxBits;
            xml::XmlCursor_ReadColor(node, to, reinterpret_cast<u8*>(to_out), host<u8>(kDefaultColor));
            break;
        case Kind::Flags:
            for (u32 j = 0; j < 2; ++j) {
                const u32 s = guest(xml::XmlCursor_FindAttr(node, j == 0 ? kHashFrom : to));
                if (s != 0) ParseFlags(s, j == 0 ? *from_out : *to_out);
            }
            break;
    }
    k.ease = ReadEnum(doc, node, kHashEase, ease_hash, kAnimEases, kAnimEaseCount, false);
    k.mode = ReadEnum(doc, node, kHashMode, mode_hash, kAnimModes, kAnimModeCount, false);
    return host<TlKeyAnim>(key);
}

}  // namespace sly4::tlbuild

NATIVE(004DEF50, sly4::tlbuild::TlKeyEvent_FromXml)
NATIVE(004DF0D0, sly4::tlbuild::TlKeyAnim_FromXml)
