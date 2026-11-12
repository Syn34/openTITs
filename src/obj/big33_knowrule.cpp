// Game code, large-function sweep part 33: one knowledge rule parsed from
// XML: the "Name" attribute, a value, the cause element (first child) and the
// result element (its next sibling), each resolved to a registered type.
#include "../sly4.h"
#include "big33.h"

using namespace sly4;
using namespace sly4::big33;

namespace {
constexpr u32 kStrName = 0x00ADD4CC;          // "Name"
constexpr u32 kFmtNoCause = 0x00ADD4D4;       // "Cannot find the cause element in knowledge (%s) (%s %d:%d)"
constexpr u32 kFmtNoCauseType = 0x00ADD488;   // "Cannot find a cause type (hash 0x%08x) in the system in (%s %d:%d)"
constexpr u32 kFmtNoResult = 0x00ADD514;      // "Cannot find the result element in knowledge (%s) (%s %d:%d)"
constexpr u32 kFmtNoResultType = 0x00ADD390;  // "Cannot find a result type (%s) in the system in (%s %d:%d)"
constexpr u32 kEmptyStr = 0x00B1FAC7;         // ""
constexpr u32 kKnowledgeSys = 0x00F65788;     // u32: the knowledge system (cause map at +0, result map at +24)
constexpr u32 kCauseTypes = 0x00F6503C;       // 28-byte entries, +24 the type descriptor
constexpr u32 kResultTypes = 0x00F65100;      // 44-byte entries, +40 the type descriptor
constexpr u32 kHashRuleValue = 0xE92E69A4;    // attribute read with XmlCursor_ReadS32 (default 50)
constexpr u32 kHashResultSpecial = 0xCD1F984F;  // result element tag handled by Knowledge_SpecialResult

constexpr u32 XmlCursor_DocPath = 0x004F8944;        // (cursor) -> const char*
constexpr u32 XmlCursor_NextSiblingAny = 0x004F8D74;  // (cursor) -> cursor
constexpr u32 XmlCursor_FirstChildAny = 0x004F9140;   // (cursor) -> cursor
constexpr u32 Xml_FindResolved = 0x004F9434;          // (cursor) -> cursor
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;  // (cursor, name) -> const char*
constexpr u32 XmlCursor_ReadS32 = 0x004F99BC;         // (cursor, hash, s32* out, s32 default)
constexpr u32 String_Format = 0x005534D0;             // (fmt, ...)
constexpr u32 Hash_ToString = 0x0056A6E0;             // (u32) -> const char*
constexpr u32 SortedMap_Find734750 = 0x00734750;      // (map, const u32* key) -> const u16*
constexpr u32 Obj_EvalChannels = 0x0038B3A8;          // (rule, ctx)
constexpr u32 Node_NotifyVars = 0x0038B588;           // (rule, owner)
constexpr u32 Knowledge_ResultSetup = 0x0038BE18;     // (type, ctx, 7, rule, s16 type+16, type+12, elem, u16* out)
constexpr u32 Knowledge_SpecialResult = 0x0038C0C8;   // (ctx, elem)
constexpr u32 Knowledge_CauseSetup = 0x0038C4E0;      // (type, ctx, 7, rule, s16 type+16, type+12, elem, u16* out)

u64 Sx32(u32 v) { return static_cast<u64>(static_cast<s64>(static_cast<s32>(v))); }
u64 Sx16(u16 v) { return static_cast<u64>(static_cast<s64>(static_cast<s16>(v))); }

// String_Format(fmt, a1, the document path of el, line + 1, column + 1).
void Report(PPUContext& ctx, u32 fmt, u32 a1, u32 el) {
    const u32 res = static_cast<u32>(call(ctx, Xml_FindResolved, {el}));
    const u32 path = static_cast<u32>(call(ctx, XmlCursor_DocPath, {res}));
    call(ctx, String_Format, {fmt, a1, path, Sx32(load<u32>(el + 8) + 1), Sx32(load<u32>(el + 12) + 1)});
}

// The element's tag hash: +20 when the flag 0x80 of the u8 +24 is set, else 0.
u32 TagHash(u32 el) { return (load<u8>(el + 24) & 0x80) ? load<u32>(el + 20) : 0; }
}  // namespace

// bool f(Ctx* a, XmlCursor* x, u32 val) (called by f_0038CCF4 and
// f_0038D498). The owner o = *a - 152 keeps 132-byte rules from o + 152, the
// s16 count at +144 and flags at +6. Rule n (n = the count) gets +0 = 1; with
// r = rule + 4: +4 = val, u16 +8 = x line + 1; when x has no non-empty "Name"
// attribute +0 = 0 (a non-empty one is not stored); +12 = the kHashRuleValue
// attribute (default 50). The first child's tag hash is looked up in the
// cause map: u16 r+18 = type, r+22 = the out value of Knowledge_CauseSetup (or
// the type when unknown, after an error message), r+20 = the type's s16 +16,
// r+24 = -1. The next sibling either has the tag kHashResultSpecial
// (Knowledge_SpecialResult(a, elem)) or is looked up in the result map:
// r+16 = 0, r+28 = type, r+32 = Knowledge_ResultSetup's out value, r+30 =
// type +16, r+34 = -1, r+36 = 0 and the count + 1. Then Obj_EvalChannels(r,
// a) and Node_NotifyVars(r, o) and 1. A missing cause or result element sets
// the flag 0x10 of o+6; any miss returns 0.
DECOMP(0038C7F0, Knowledge_ParseRule_0038C7F0) {
    const u32 a = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), val = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);  // sp+112: s32 value, +116 / +120 keys, +124 / +126 u16 outs
    const u32 sp = frame.addr();
    const u32 o = load<u32>(a) - 152;
    auto rule = [&]() { return static_cast<u32>(static_cast<s32>(static_cast<s16>(load<u16>(o + 144)))) * 132; };
    store<u32>(o + 152 + rule(), 1);
    const u32 r = o + 156 + rule();
    const u32 line = load<u32>(x + 8);
    store<u32>(r + 4, val);
    store<u16>(r + 8, static_cast<u16>(line + 1));
    // r5 (val) and r6 (line + 1) are also live at this call; it reads neither.
    const u32 nm = static_cast<u32>(call(ctx, XmlCursor_FindAttrByName, {x, kStrName}));
    if (nm == 0 || load<u8>(nm) == 0) store<u32>(r, 0);
    call(ctx, XmlCursor_ReadS32, {x, kHashRuleValue, sp, 50u});
    store<u32>(r + 12, load<u32>(sp));

    // Cause.
    const u32 c = static_cast<u32>(call(ctx, XmlCursor_FirstChildAny, {x}));
    if (c == 0) {
        Report(ctx, kFmtNoCause, load<u32>(r), x);
        store<u16>(o + 6, load<u16>(o + 6) | 0x10);
        return ret(ctx, 0u);
    }
    const u32 cr = r + 18;
    const u32 ch = TagHash(c);
    store<u32>(sp + 8, ch);
    u16 t = load<u16>(static_cast<u32>(call(ctx, SortedMap_Find734750, {load<u32>(kKnowledgeSys), sp + 8})));
    store<u16>(cr, t);
    u16 out;
    if (static_cast<s16>(t) == -1) {
        Report(ctx, kFmtNoCauseType, ch, c);
        t = load<u16>(cr);
        out = t;
    } else {
        const u32 ty = load<u32>(kCauseTypes + 24 + static_cast<u32>(static_cast<s16>(t)) * 28);
        call(ctx, Knowledge_CauseSetup, {ty, a, 7u, r, Sx16(load<u16>(ty + 16)), load<u32>(ty + 12), c, sp + 14});
        out = load<u16>(sp + 14);
        t = load<u16>(cr);
    }
    store<u16>(cr + 4, out);
    if (static_cast<s16>(t) == -1) return ret(ctx, 0u);
    {
        const u32 ty = load<u32>(kCauseTypes + 24 + static_cast<u32>(static_cast<s16>(t)) * 28);
        const u16 k = load<u16>(ty + 16);
        store<u16>(cr + 6, 0xFFFF);
        store<u16>(cr + 2, k);
    }

    // Result.
    const u32 d = static_cast<u32>(call(ctx, XmlCursor_NextSiblingAny, {c}));
    if (d == 0) {
        Report(ctx, kFmtNoResult, kEmptyStr, x);
        store<u16>(o + 6, load<u16>(o + 6) | 0x10);
        return ret(ctx, 0u);
    }
    if (TagHash(d) == kHashResultSpecial) {
        call(ctx, Knowledge_SpecialResult, {a, d});
    } else {
        store<u8>(r + 16, 0);
        const u32 ks = load<u32>(kKnowledgeSys);
        const u32 rr = r + 28;
        const u32 rh = TagHash(d);
        store<u32>(sp + 4, rh);
        u16 t2 = load<u16>(static_cast<u32>(call(ctx, SortedMap_Find734750, {ks + 24, sp + 4})));
        store<u16>(rr, t2);
        u16 out2;
        if (static_cast<s16>(t2) == -1) {
            const u32 str = static_cast<u32>(call(ctx, Hash_ToString, {rh}));
            Report(ctx, kFmtNoResultType, str, d);
            t2 = load<u16>(rr);
            out2 = t2;
        } else {
            const u32 ty = load<u32>(kResultTypes + 40 + static_cast<u32>(static_cast<s16>(t2)) * 44);
            call(ctx, Knowledge_ResultSetup, {ty, a, 7u, r, Sx16(load<u16>(ty + 16)), load<u32>(ty + 12), d, sp + 12});
            out2 = load<u16>(sp + 12);
            t2 = load<u16>(rr);
        }
        store<u16>(rr + 4, out2);
        if (static_cast<s16>(t2) == -1) return ret(ctx, 0u);
        const u32 ty = load<u32>(kResultTypes + 40 + static_cast<u32>(static_cast<s16>(t2)) * 44);
        const u16 k = load<u16>(ty + 16);
        store<u16>(rr + 6, 0xFFFF);
        store<u16>(rr + 2, k);
        store<u32>(rr + 8, 0);
        store<u16>(o + 144, static_cast<u16>(load<u16>(o + 144) + 1));
    }
    call(ctx, Obj_EvalChannels, {r, a});
    call(ctx, Node_NotifyVars, {r, o});
    ret(ctx, 1u);
}
