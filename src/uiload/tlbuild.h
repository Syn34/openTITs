// Timeline keys built from XML: event keys and animation keys (the two
// non-trivial kinds TimelineDef_KeyFromXml makes).
#pragma once

#include <cstddef>

#include "opentit/native.h"
#include "../sly4.h"
#include "../xml/xml.h"

namespace sly4 {
namespace tlbuild {

using namespace opentit;

namespace bfn {
constexpr u32 Mem_Alloc = 0x0054ED38;
constexpr u32 memset = 0x00726E54;
constexpr u32 String_CopyN = 0x00553638;            // (dst, src, n)
constexpr u32 XmlCursor_DocPath = 0x004F8944;
constexpr u32 XmlCursor_FindAttr = 0x004F9534;       // (cursor, hash) -> string or 0
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628; // (cursor, name) -> string or 0
constexpr u32 XmlCursor_ReadS32 = 0x004F99BC;        // (cursor, hash, s32* out, default) -> 0 when read
constexpr u32 XmlCursor_ReadF32 = 0x004F9B20;        // (cursor, hash, f32* out), default in f1
constexpr u32 XmlCursor_ReadVec2 = 0x004F9C9C;       // (cursor, hash, f32[2] out, const f32[2]* default)
constexpr u32 XmlCursor_ReadVec3 = 0x004F9E0C;       // (cursor, hash, f32[3] out, const f32[3]* default)
constexpr u32 XmlCursor_ReadColor = 0x004F9F9C;      // (cursor, hash, out, const default*) -> 0 when read
constexpr u32 XmlCursor_ReadHash = 0x004FA14C;       // (cursor, hash, u32* out, 0) -> 0 when read
}  // namespace bfn

constexpr u32 kStrTarget = 0x00AEF8B8;  // "target"

// Common key header (TlKey): +0 vtable, +4 f32 time, +8 next.
constexpr u32 kKeyTime = 4, kKeyNext = 8;

// Event key (80 bytes): +12 event index (17 = unknown), +16 char target[64].
constexpr u32 kTlKeyEventVtbl = 0x00B22908;
constexpr u32 kEventKeySize = 80;
constexpr u32 kEventIndex = 12, kEventTarget = 16;
constexpr u32 kHashEvent = 0x00012B61;
constexpr u32 kEventNames = 0x00AEF874, kEventCount = 17;  // u32 hashes

// Animation key (128 bytes).
constexpr u32 kTlKeyAnimVtbl = 0x00B228F0;
constexpr u32 kAnimKeySize = 128;
constexpr u32 kAnimProp = 12;     // u32: property index (26 = unknown)
constexpr u32 kAnimEase = 16;     // u32: index in kAnimEases (name?)
constexpr u32 kAnimMode = 20;     // u32: index in kAnimModes (name?)
constexpr u32 kAnimF24 = 24;      // f32 read from kHashAnimF24 (name?)
constexpr u32 kAnimTarget = 28;   // char[64]
constexpr u32 kAnimFrom = 96;     // 16 bytes; FLT_MAX in the first word when absent
constexpr u32 kAnimTo = 112;      // 16 bytes
constexpr u32 kHashFrom = 0x00210FAA;
constexpr u32 kHashToA = 0x04DA9951;  // the "to" attribute of a kTagAnimA key
constexpr u32 kHashToB = 0x00000A7B;  // ... of a kTagAnimB key
constexpr u32 kTagAnimA = 0x00014042;
constexpr u32 kHashAnimF24 = 0xB7EA3D94;
constexpr u32 kHashProp = 0xF373CDD5;
constexpr u32 kHashEase = 0x00264412, kHashMode = 0x0023BEA4;
constexpr u32 kAnimProps = 0x00AEF8CC, kAnimPropCount = 26;
constexpr u32 kAnimEases = 0x00AEF938, kAnimEaseCount = 4;
constexpr u32 kAnimModes = 0x00AEF948, kAnimModeCount = 3;
constexpr u32 kF32Zero = 0x004DF0C8, kF32Max = 0x004DF0CC;
constexpr u32 kFltMaxBits = 0x7F7FFFFF;
constexpr u32 kDefaultVec3 = 0x01164B40, kDefaultVec2 = 0x01164AFC, kDefaultColor = 0x01164CA0;

// ---------------------------------------------------------------- native structures
struct TlKeyEvent {
    be_u32 vtable;         // +0 kTlKeyEventVtbl
    be_f32 time;           // +4
    be_u32 next;           // +8
    be_u32 index;          // +12 event index (17 = unknown)
    char target[64];       // +16
};
static_assert(sizeof(TlKeyEvent) == kEventKeySize);

struct TlKeyAnim {
    be_u32 vtable;         // +0 kTlKeyAnimVtbl
    be_f32 time;           // +4
    be_u32 next;           // +8
    be_u32 prop;           // +12 property index (26 = unknown)
    be_u32 ease;           // +16 index in kAnimEases (name?)
    be_u32 mode;           // +20 index in kAnimModes (name?)
    be_f32 f24;            // +24 (name?)
    char target[64];       // +28
    native::pad<4> _92;
    be_u32 from[4];        // +96: FLT_MAX in the first word when absent
    be_u32 to[4];          // +112
};
static_assert(sizeof(TlKeyAnim) == kAnimKeySize && offsetof(TlKeyAnim, from) == kAnimFrom &&
              offsetof(TlKeyAnim, to) == kAnimTo && offsetof(TlKeyAnim, target) == kAnimTarget);

// Native functions (src/uiload/tlbuild.cpp).
TlKeyEvent* TlKeyEvent_FromXml(xml::XmlCursor* doc, xml::XmlCursor* node, void* def, f32 time);
TlKeyAnim* TlKeyAnim_FromXml(xml::XmlCursor* doc, xml::XmlCursor* node, void* def, f32 time);
void* TimelineDef_KeyFromXml(xml::XmlCursor* doc, xml::XmlCursor* node, void* def);

}  // namespace tlbuild
}  // namespace sly4
