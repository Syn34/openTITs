// Game code, large-function sweep part 33: one knowledge rule parsed from
// XML: the "Name" attribute, a value, the cause element (first child) and the
// result element (its next sibling), each resolved to a registered type.
#include "big33.h"

namespace sly4::obj {

// An XML element cursor.
struct XmlElem {
    pad<8> _0;
    be_u32 line;    // +8
    be_u32 column;  // +12
    pad<4> _16;
    be_u32 hash;    // +20: the tag hash, valid when bit 0x80 of +24 is set
    u8 flags;       // +24
};
static_assert(offsetof(XmlElem, hash) == 20 && offsetof(XmlElem, flags) == 24);

// A rule's cause or result: the type index, the type's +16 value, the
// setup's out value, -1.
struct KnowSlot {
    be_s16 type;  // +0
    be_u16 k;     // +2
    be_u16 out;   // +4
    be_u16 end;   // +6
};
struct KnowRule {
    be_u32 name;      // +0
    be_u32 val;       // +4
    be_u16 line;      // +8
    pad<2> _10;
    be_u32 value;     // +12
    u8 b16;           // +16
    pad<1> _17;
    KnowSlot cause;   // +18
    pad<2> _26;
    KnowSlot result;  // +28
    be_u32 r36;       // +36
    pad<88> _40;
};
static_assert(offsetof(KnowRule, cause) == 18 && offsetof(KnowRule, result) == 28 && sizeof(KnowRule) == 128);
// The owner's 132-byte rule entries (from +152).
struct KnowRuleEntry {
    be_u32 active;  // +0
    KnowRule rule;  // +4
};
struct KnowOwner {
    pad<6> _0;
    be_u16 flags;   // +6: 0x10 a cause / result element is missing
    pad<136> _8;
    be_s16 count;   // +144
    pad<6> _146;
    KnowRuleEntry rules[1];  // +152 (indexed by guest address)
};
static_assert(offsetof(KnowOwner, count) == 144 && offsetof(KnowOwner, rules) == 152);
struct KnowCtx {
    be_u32 rules;  // +0: its owner's rules (owner + 152)
};
// A registered cause / result type.
struct KnowType {
    pad<12> _0;
    be_u32 w12;  // +12
    be_s16 k;    // +16
};
struct KnowledgeSys {
    pad<24> causes;   // +0: sorted map hash -> u16 type
    pad<4> results;   // +24
};

namespace {
constexpr u32 kStrName = 0x00ADD4CC;          // "Name"
constexpr u32 kFmtNoCause = 0x00ADD4D4;       // "Cannot find the cause element in knowledge (%s) (%s %d:%d)"
constexpr u32 kFmtNoCauseType = 0x00ADD488;   // "Cannot find a cause type (hash 0x%08x) in the system in (%s %d:%d)"
constexpr u32 kFmtNoResult = 0x00ADD514;      // "Cannot find the result element in knowledge (%s) (%s %d:%d)"
constexpr u32 kFmtNoResultType = 0x00ADD390;  // "Cannot find a result type (%s) in the system in (%s %d:%d)"
constexpr u32 kEmptyStr = 0x00B1FAC7;         // ""
constexpr u32 kKnowledgeSys = 0x00F65788;     // KnowledgeSys*
constexpr u32 kCauseTypes = 0x00F6503C;       // 28-byte entries, +24 the KnowType
constexpr u32 kResultTypes = 0x00F65100;      // 44-byte entries, +40 the KnowType
constexpr u32 kHashRuleValue = 0xE92E69A4;    // attribute read with XmlCursor_ReadS32 (default 50)
constexpr u32 kHashResultSpecial = 0xCD1F984F;  // result element tag handled by Knowledge_SpecialResult

constexpr u32 XmlCursor_DocPath = 0x004F8944;        // (cursor) -> const char*
constexpr u32 XmlCursor_NextSiblingAny = 0x004F8D74;  // (cursor) -> cursor
constexpr u32 XmlCursor_FirstChildAny = 0x004F9140;   // (cursor) -> cursor
constexpr u32 Xml_FindResolved = 0x004F9434;          // (cursor) -> cursor
constexpr u32 XmlCursor_FindAttrByName = 0x004F9628;  // (cursor, name) -> const char*
constexpr u32 XmlCursor_ReadS32 = 0x004F99BC;         // (cursor, hash, s32* out, s32 default)
constexpr u32 Hash_ToString = 0x0056A6E0;             // (u32) -> const char*
constexpr u32 SortedMap_Find734750 = 0x00734750;      // (map, const u32* key) -> const u16*
constexpr u32 Obj_EvalChannels = 0x0038B3A8;          // (rule, ctx)
constexpr u32 Node_NotifyVars = 0x0038B588;           // (rule, owner)
constexpr u32 Knowledge_ResultSetup = 0x0038BE18;     // (type, ctx, 7, rule, s16 type->k, type->w12, elem, u16* out)
constexpr u32 Knowledge_SpecialResult = 0x0038C0C8;   // (ctx, elem)
constexpr u32 Knowledge_CauseSetup = 0x0038C4E0;      // (type, ctx, 7, rule, s16 type->k, type->w12, elem, u16* out)

// (unchecked: a null type is read at address 0)
KnowType& CauseType(s32 t) { return gm<KnowType>(gm<be_u32>(kCauseTypes + 24 + static_cast<u32>(t) * 28)); }
KnowType& ResultType(s32 t) { return gm<KnowType>(gm<be_u32>(kResultTypes + 40 + static_cast<u32>(t) * 44)); }

// String_Format(fmt, a1, the document path of el, line + 1, column + 1).
void Report(u32 fmt, u32 a1, const XmlElem* el) {
    const u32 res = call<u32>(Xml_FindResolved, el);
    const u32 path = call<u32>(XmlCursor_DocPath, res);
    call(fn::String_Format, fmt, a1, path, static_cast<s32>(el->line + 1), static_cast<s32>(el->column + 1));
}

// The element's tag hash, or 0 without one.
u32 TagHash(const XmlElem* el) { return (el->flags & 0x80) ? el->hash.get() : 0u; }
}  // namespace

// bool f(Ctx* a, XmlCursor* x, u32 val) (called by f_0038CCF4 and
// f_0038D498). The owner o = a->rules - 152 keeps 132-byte rules. Rule n
// (n = o->count) gets active = 1; its body r: val, line = x's line + 1;
// when x has no non-empty "Name" attribute name = 0 (a non-empty one is
// not stored); value = the kHashRuleValue attribute (default 50). The first
// child's tag hash is looked up in the cause map: cause.type, cause.out =
// the out value of Knowledge_CauseSetup (or the type when unknown, after an
// error message), cause.k = the type's k, cause.end = -1. The next sibling
// either has the tag kHashResultSpecial (Knowledge_SpecialResult(a, elem))
// or is looked up in the result map: b16 = 0, the result slot likewise
// (Knowledge_ResultSetup), r36 = 0 and the count + 1. Then
// Obj_EvalChannels(r, a) and Node_NotifyVars(r, o) and 1. A missing cause
// or result element sets the flag 0x10 of o->flags; any miss returns 0.
bool Knowledge_ParseRule_0038C7F0(KnowCtx* a, XmlElem* x, u32 val) {
    GuestFrame frame(16);  // sp+112: s32 value, +116 / +120 keys, +124 / +126 u16 outs
    be_s32* value = frame.as<be_s32>(0);
    be_u32* rkey = frame.as<be_u32>(4);
    be_u32* ckey = frame.as<be_u32>(8);
    be_u16* rout = frame.as<be_u16>(12);
    be_u16* cout = frame.as<be_u16>(14);
    const u32 o_ea = a->rules - 152;
    KnowOwner& o = gm<KnowOwner>(o_ea);
    KnowRuleEntry& ent = gm<KnowRuleEntry>(o_ea + 152 + static_cast<u32>(static_cast<s32>(o.count)) * 132);
    ent.active = 1u;
    KnowRule& r = ent.rule;
    const u32 line = x->line;
    r.val = val;
    r.line = static_cast<u16>(line + 1);
    const u32 nm = call<u32>(XmlCursor_FindAttrByName, x, kStrName);
    if (nm == 0 || gm<u8>(nm) == 0) r.name = 0u;
    call(XmlCursor_ReadS32, x, kHashRuleValue, value, 50u);
    r.value = static_cast<u32>(value->get());

    // Cause.
    XmlElem* c = call<XmlElem*>(XmlCursor_FirstChildAny, x);
    if (c == nullptr) {
        Report(kFmtNoCause, r.name, x);
        o.flags = static_cast<u16>(o.flags | 0x10);
        return false;
    }
    KnowSlot& cr = r.cause;
    const u32 ch = TagHash(c);
    *ckey = ch;
    KnowledgeSys& ks0 = gm<KnowledgeSys>(global<be_u32>(kKnowledgeSys));
    s16 t = gm<be_s16>(call<u32>(SortedMap_Find734750, &ks0.causes, ckey));
    cr.type = t;
    u16 out;
    if (t == -1) {
        Report(kFmtNoCauseType, ch, c);
        t = cr.type;
        out = static_cast<u16>(t);
    } else {
        KnowType& ty = CauseType(t);
        call(Knowledge_CauseSetup, &ty, a, 7u, &r, ty.k.get(), ty.w12.get(), c, cout);
        out = *cout;
        t = cr.type;
    }
    cr.out = out;
    if (t == -1) return false;
    {
        const u16 k = static_cast<u16>(CauseType(t).k);
        cr.end = 0xFFFF;
        cr.k = k;
    }

    // Result.
    XmlElem* d = call<XmlElem*>(XmlCursor_NextSiblingAny, c);
    if (d == nullptr) {
        Report(kFmtNoResult, kEmptyStr, x);
        o.flags = static_cast<u16>(o.flags | 0x10);
        return false;
    }
    if (TagHash(d) == kHashResultSpecial) {
        call(Knowledge_SpecialResult, a, d);
    } else {
        r.b16 = 0;
        KnowledgeSys& ks = gm<KnowledgeSys>(global<be_u32>(kKnowledgeSys));
        KnowSlot& rr = r.result;
        const u32 rh = TagHash(d);
        *rkey = rh;
        s16 t2 = gm<be_s16>(call<u32>(SortedMap_Find734750, &ks.results, rkey));
        rr.type = t2;
        u16 out2;
        if (t2 == -1) {
            const u32 str = call<u32>(Hash_ToString, rh);
            Report(kFmtNoResultType, str, d);
            t2 = rr.type;
            out2 = static_cast<u16>(t2);
        } else {
            KnowType& ty = ResultType(t2);
            call(Knowledge_ResultSetup, &ty, a, 7u, &r, ty.k.get(), ty.w12.get(), d, rout);
            out2 = *rout;
            t2 = rr.type;
        }
        rr.out = out2;
        if (t2 == -1) return false;
        const u16 k = static_cast<u16>(ResultType(t2).k);
        rr.end = 0xFFFF;
        rr.k = k;
        r.r36 = 0u;
        o.count = static_cast<s16>(o.count + 1);
    }
    call(Obj_EvalChannels, &r, a);
    call(Node_NotifyVars, &r, &o);
    return true;
}

}  // namespace sly4::obj

NATIVE(0038C7F0, sly4::obj::Knowledge_ParseRule_0038C7F0)
