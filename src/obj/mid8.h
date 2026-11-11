// Game code, medium sweep part 8: helpers shared by mid8_*.cpp (still
// recompiled game functions of 120-400 instructions: component template
// readers, XML element constructors and def readers).
#pragma once

#include <bit>
#include <cmath>

#include "mid7.h"

namespace sly4 {
namespace mid8 {

using namespace opentit;
using namespace opentit::decomp;
using mid1::LfsBits;
using mid5::XBool;
using mid5::XF32;
using mid5::XHash;
using mid5::XRead;
using mid5::XS32;
using mid6::F32;
using mid6::W32;
using mid6::XF32Def;
using mid6::XVec2;
using mid6::XVec3;
using mid7::XBoolVia;
using obj19::VCall;
namespace xfn = mid5::xfn;
namespace m7fn = mid7::m7fn;

namespace m8fn {
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;  // (c, const char*) -> const char* or 0
constexpr u32 XmlCursor_DocPath = 0x004F8944;         // (x) (reads r3 only)
constexpr u32 XmlCursor_FirstChild = 0x004F92BC;      // (c, hash) -> the first child element with that hash, or 0
constexpr u32 XmlCursor_NextSibling = 0x004F8FC8;     // (node, hash) -> the next sibling with that hash, or 0
constexpr u32 String_HashNoCase = 0x0056A710;         // (const char*) -> u32
constexpr u32 String_CopyN = 0x00553638;              // (dst, src, n)
constexpr u32 Array_Reserve = 0x00558F3C;             // (Array*, u16 capacity)
constexpr u32 Memset = 0x00726E54;                    // (dst, c, n)
constexpr u32 Memmove = 0x0071C3F0;                   // (dst, src, n)
constexpr u32 Obj_InitVt = 0x004BB3B0;                // (o, c): +4 (u16) = 1, +0 = 0x00B21C98
}  // namespace m8fn

constexpr u32 kStrName = 0x00AD4C0C;       // "name"
constexpr u32 kTmplRegistry = 0x01056F04;  // {+0 u16 cap, +2 u16 count, +4 u16 high-water, +6 u16 flags, +8 {u32 hash, Tmpl*}[]}
constexpr u32 kTmplVtBase = 0x00B0DAAC;    // the template base vtable

// {hash, tmpl} appended to the template registry (Array_Reserve doubling
// when full), count / high-water / flags |= 1 updated.
// regOrder: 0 = the entry's template stored before the count; 1 = after.
// flagsFirst: the flags stored before the high-water.
inline void TmplRegister(PPUContext& ctx, u32 p, int regOrder, bool flagsFirst = false) {
    const u32 R = kTmplRegistry;
    const u32 cap = load<u16>(R);
    if (load<u16>(R + 2) == cap) call(ctx, m8fn::Array_Reserve, {R, (cap << 1) & 0xFFFEu});
    const u32 n = load<u16>(R + 2), data = load<u32>(R + 8);
    store<u32>(data + ((n << 3) & 0x7FFF8u), load<u32>(p + 4));
    const u32 n1 = n + 1, hw = load<u16>(R + 4), fl = load<u16>(R + 6);
    if (regOrder == 0) store<u32>(data + (n << 3) + 4, p);
    store<u16>(R + 2, static_cast<u16>(n1));
    if (regOrder == 1) store<u32>(data + (n << 3) + 4, p);
    if (flagsFirst) store<u16>(R + 6, static_cast<u16>(fl | 1));
    store<u16>(R + 4, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFF) ? hw : n1));
    if (!flagsFirst) store<u16>(R + 6, static_cast<u16>(fl | 1));
}

// {key, value} appended to a registry of the same layout: key, count,
// value (valueFirst: key, value, count), high-water, flags |= 1.
inline void RegPushKV(PPUContext& ctx, u32 R, u32 key, u32 value, bool valueFirst = false) {
    const u32 cap = load<u16>(R);
    if (load<u16>(R + 2) == cap) call(ctx, m8fn::Array_Reserve, {R, (cap << 1) & 0xFFFEu});
    const u32 data = load<u32>(R + 8), n = load<u16>(R + 2);
    store<u32>(data + ((n << 3) & 0x7FFF8u), key);
    const u32 n1 = n + 1;
    if (valueFirst) store<u32>(data + (n << 3) + 4, value);
    store<u16>(R + 2, static_cast<u16>(n1));
    if (!valueFirst) store<u32>(data + (n << 3) + 4, value);
    const u32 hw = load<u16>(R + 4), fl = load<u16>(R + 6);
    store<u16>(R + 4, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFF) ? hw : n1));
    store<u16>(R + 6, static_cast<u16>(fl | 1));
}

// The shared template prologue (the TmplRead family, see mid7_tmpl.cpp):
// the "name" attribute (none -> optionally XmlCursor_DocPath(a0), false);
// p = SmallAlloc(size) and, when that succeeded, the registered template
// base (vtable 0x00B0DAAC, +4 the name's String_HashNoCase, +8 0, +12 kind,
// +16 0). baseOrder: the store order of the base fields: 0 = +8, +16, +4,
// +12; 1 = +16, +8, +4, +12; 2 = +8, +4, +12, +16.
inline bool TmplBegin(PPUContext& ctx, u32 a0, u32 c, u32 size, u32 kind, bool docpath, u32& p, int baseOrder = 0, int regOrder = 0,
                      bool flagsFirst = false) {
    p = 0;
    const u32 name = static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, kStrName}));
    if (name == 0) {
        if (docpath) call(ctx, m8fn::XmlCursor_DocPath, {a0});
        return false;
    }
    p = static_cast<u32>(call(ctx, m7fn::SmallAlloc, {size}));
    if (p == 0) return true;
    store<u32>(p, kTmplVtBase);
    const u32 h = static_cast<u32>(call(ctx, m8fn::String_HashNoCase, {name}));
    if (baseOrder == 1) store<u8>(p + 16, 0);
    store<u32>(p + 8, 0);
    if (baseOrder == 0) store<u8>(p + 16, 0);
    store<u32>(p + 4, h);
    store<u32>(p + 12, kind);
    if (baseOrder == 2) store<u8>(p + 16, 0);
    TmplRegister(ctx, p, regOrder, flagsFirst);
    return true;
}

// The node's "name" attribute copied (31 chars) to dst and its
// String_HashNoCase stored at hdst; false (and XmlCursor_DocPath(a0)) when
// it has none.
inline bool NameAndHash(PPUContext& ctx, u32 a0, u32 node, u32 dst, u32 hdst) {
    const u32 nm = static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {node, kStrName}));
    if (nm == 0) {
        call(ctx, m8fn::XmlCursor_DocPath, {a0});
        return false;
    }
    call(ctx, m8fn::String_CopyN, {dst, nm, 31});
    store<u32>(hdst, static_cast<u32>(call(ctx, m8fn::String_HashNoCase, {nm})));
    return true;
}

}  // namespace mid8
}  // namespace sly4
