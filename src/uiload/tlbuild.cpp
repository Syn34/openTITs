// TlKeyEvent_FromXml / TlKeyAnim_FromXml: event and animation keys of a
// <timeline> definition.
#include "tlbuild.h"

using namespace sly4;
using namespace sly4::tlbuild;

namespace {

// The index of `hash` in a table of `n` u32 hashes, or n.
u32 IndexOf(u32 table, u32 n, u32 hash) {
    u32 i = 0;
    while (i < n && load<u32>(table + i * 4) != hash) ++i;
    return i;
}

// Reads the enum attribute `attr` into an index of `table`; reports a
// value that isn't in it (index n). A failed read gives 0, reported only
// when `report_fail`.
u32 ReadEnum(PPUContext& ctx, u32 doc, u32 node, u32 attr, u32 out, u32 table, u32 n, bool report_fail) {
    if (static_cast<u32>(call(ctx, bfn::XmlCursor_ReadHash, {node, attr, out, 0})) != 0) {
        if (report_fail) call(ctx, bfn::XmlCursor_DocPath, {doc});
        return 0;
    }
    const u32 i = IndexOf(table, n, load<u32>(out));
    if (i == n) call(ctx, bfn::XmlCursor_DocPath, {doc});
    return i;
}

inline f64 lfs(u32 ea) { return static_cast<f64>(load<f32>(ea)); }

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
void ParseFlags(u32 s, u32 word) {
    for (u32 p = s;;) {
        const s32 c = static_cast<s8>(load<u8>(p));
        if (c == 0) return;
        const u32 w = load<u32>(word);
        switch (c) {
            case 'T': case 't': store<u32>(word, w & ~0x30u); break;
            case 'M': case 'm': store<u32>(word, (w & ~0x30u) | 0x10); break;
            case 'B': case 'b': store<u32>(word, (w & ~0x30u) | 0x20); break;
            case 'L': case 'l': store<u32>(word, w & ~0x3u); break;
            case 'C': case 'c': store<u32>(word, (w & ~0x3u) | 1); break;
            case 'R': case 'r': store<u32>(word, (w & ~0x3u) | 2); break;
            default: break;
        }
        ++p;
        if (p == 0) return;  // the original stops when the pointer wraps
    }
}

}  // namespace

// TlKey* TlKeyEvent_FromXml(XmlCursor* doc, XmlCursor* node, TimelineDef* def; f32 time in f1)
// An 80-byte event key: the target name and the event (index in a table
// of 17 hashes; 17 when unknown, which is reported, 0 when unreadable).
// As in the original, the fields are still written when the allocation
// fails (at guest address 0).
DECOMP(004DEF50, TlKeyEvent_FromXml) {
    const u32 doc = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1);
    const f64 time = farg(ctx, 0);
    Stack frame(ctx, 176 - 112);
    const u32 hash = frame.addr();  // sp+112
    u32 key = static_cast<u32>(call(ctx, bfn::Mem_Alloc, {kEventKeySize, 0, 16}));
    if (key != 0) {
        store<u32>(key + kKeyNext, 0);
        store<f32>(key + kKeyTime, static_cast<f32>(time));
        store<u32>(key + kEventIndex, 0);
        store<u32>(key, kTlKeyEventVtbl);
        store<u8>(key + kEventTarget, 0);
    }
    const u32 target = static_cast<u32>(call(ctx, bfn::XmlCursor_FindAttrByName, {node, kStrTarget}));
    if (target != 0) call(ctx, bfn::String_CopyN, {key + kEventTarget, target, 63});
    u32 index = 0;
    if (static_cast<u32>(call(ctx, bfn::XmlCursor_ReadHash, {node, kHashEvent, hash, 0})) != 0) {
        call(ctx, bfn::XmlCursor_DocPath, {doc});
    } else {
        index = IndexOf(kEventNames, kEventCount, load<u32>(hash));
        if (index == kEventCount) {
            call(ctx, bfn::XmlCursor_DocPath, {doc});
            call(ctx, bfn::XmlCursor_FindAttr, {node, kHashEvent});  // result unused
        }
    }
    store<u32>(key + kEventIndex, index);
    ret(ctx, key);
}

// TlKey* TlKeyAnim_FromXml(XmlCursor* doc, XmlCursor* node, TimelineDef* def; f32 time in f1)
// A 128-byte animation key: an f32 at +24, the target name, the animated
// property (index in a table of 26), its "from" / "to" values read as the
// property's type (the "to" attribute's hash depends on the tag), and two
// enums (+16 of 4, +20 of 3). Unknown enum values are reported. As in the
// original, the fields are still written when the allocation fails.
DECOMP(004DF0D0, TlKeyAnim_FromXml) {
    const u32 doc = arg<u32>(ctx, 0), node = arg<u32>(ctx, 1);
    const f64 time = farg(ctx, 0);
    Stack frame(ctx, 208 - 112);
    const u32 sp = frame.addr() - 112;
    u32 key = static_cast<u32>(call(ctx, bfn::Mem_Alloc, {kAnimKeySize, 0, 16}));
    if (key != 0) {
        store<u32>(key + kKeyNext, 0);
        store<f32>(key + kKeyTime, static_cast<f32>(time));
        store<u32>(key + kAnimProp, 0);
        store<u32>(key, kTlKeyAnimVtbl);
        store<u32>(key + kAnimEase, 0);
        store<u32>(key + kAnimMode, 0);
        store<u32>(key + kAnimF24, 0);
        store<u8>(key + kAnimTarget, 0);
        call(ctx, bfn::memset, {key + kAnimFrom, 0, 32});
    }
    const u32 tag = (load<u8>(node + 24) & 0x80) != 0 ? load<u32>(node + 20) : 0;
    const f64 zero = lfs(kF32Zero);  // f31 from here on
    callf(ctx, bfn::XmlCursor_ReadF32, {node, kHashAnimF24, key + kAnimF24}, {zero});
    const u32 target = static_cast<u32>(call(ctx, bfn::XmlCursor_FindAttrByName, {node, kStrTarget}));
    if (target != 0) call(ctx, bfn::String_CopyN, {key + kAnimTarget, target, 63});

    const u32 prop = ReadEnum(ctx, doc, node, kHashProp, sp + 128, kAnimProps, kAnimPropCount, true);
    store<u32>(key + kAnimProp, prop);
    const u32 to = tag == kTagAnimA ? kHashToA : kHashToB;
    const u32 from_out = key + kAnimFrom, to_out = key + kAnimTo;

    switch (PropKind(prop)) {
        case Kind::None: break;
        case Kind::F32:
            callf(ctx, bfn::XmlCursor_ReadF32, {node, kHashFrom, from_out}, {lfs(kF32Max)});
            callf(ctx, bfn::XmlCursor_ReadF32, {node, to, to_out}, {zero});
            break;
        case Kind::Vec3:
            for (u32 k = 0; k < 12; k += 4) store<u32>(sp + 112 + k, kFltMaxBits);
            call(ctx, bfn::XmlCursor_ReadVec3, {node, kHashFrom, from_out, sp + 112, kFltMaxBits});  // r7 as in the original (not read)
            call(ctx, bfn::XmlCursor_ReadVec3, {node, to, to_out, kDefaultVec3});
            break;
        case Kind::Vec2:
            store<u32>(sp + 140, kFltMaxBits);
            store<u32>(sp + 144, kFltMaxBits);
            call(ctx, bfn::XmlCursor_ReadVec2, {node, kHashFrom, from_out, sp + 140, kFltMaxBits});  // r7 as in the original (not read)
            call(ctx, bfn::XmlCursor_ReadVec2, {node, to, to_out, kDefaultVec2});
            break;
        case Kind::S32:
            if (static_cast<u32>(call(ctx, bfn::XmlCursor_ReadS32, {node, kHashFrom, from_out, 0})) != 0)
                store<u32>(from_out, kFltMaxBits);
            call(ctx, bfn::XmlCursor_ReadS32, {node, to, to_out, 0});
            break;
        case Kind::Color:
            if (static_cast<u32>(call(ctx, bfn::XmlCursor_ReadColor, {node, kHashFrom, from_out, kDefaultColor})) != 0)
                store<u32>(from_out, kFltMaxBits);
            call(ctx, bfn::XmlCursor_ReadColor, {node, to, to_out, kDefaultColor});
            break;
        case Kind::Flags:
            for (u32 j = 0; j < 2; ++j) {
                const u32 s = static_cast<u32>(call(ctx, bfn::XmlCursor_FindAttr, {node, j == 0 ? kHashFrom : to}));
                if (s != 0) ParseFlags(s, j == 0 ? from_out : to_out);
            }
            break;
    }
    store<u32>(key + kAnimEase, ReadEnum(ctx, doc, node, kHashEase, sp + 132, kAnimEases, kAnimEaseCount, false));
    store<u32>(key + kAnimMode, ReadEnum(ctx, doc, node, kHashMode, sp + 136, kAnimModes, kAnimModeCount, false));
    ret(ctx, key);
}
