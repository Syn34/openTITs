// Game code, medium sweep part 7: anim commands that own child commands
// (the 0x0007DA1C family): their own attributes, then the PtrVector of the
// children built from the child elements (0x004B6DA0 accepts an element,
// 0x004B6C88 makes its command).
#include "mid7.h"

using namespace sly4;
using namespace sly4::mid7;

namespace {

namespace cfn {
constexpr u32 AnimCmd_ConstructBase = 0x004B9CB8;  // (o)
constexpr u32 Obj_InitVt_B21C98 = 0x004BB3B0;      // (o): +4 = 1, vtable 0x00B21C98
constexpr u32 PtrVector_Construct = 0x005516E0;    // (PtrVector*): all fields 0
constexpr u32 PtrVector_Reserve = 0x00551718;      // (PtrVector*, u32 capacity)
constexpr u32 AnimCmd_IsCmdElem = 0x004B6DA0;      // (node) -> bool (name?)
constexpr u32 AnimCmd_FromElem = 0x004B6C88;       // (node) -> AnimCmd* or 0 (name?)
}  // namespace cfn

void PtrPush(PPUContext& ctx, u32 v, u32 p);

// The children of cursor c into the PtrVector v ({+0 data, +4 u16
// capacity, +6 u16, +8 u16 count, +10 u16 high-water}): count the accepted
// elements, reserve that many, then append each command made. Returns the
// r3 the original ends with.
u32 ChildCmds(PPUContext& ctx, u32 c, u32 v) {
    u32 n = 0;
    for (u32 e = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c})); e != 0;
         e = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {e})))
        if (call(ctx, cfn::AnimCmd_IsCmdElem, {e}) != 0) ++n;
    u32 e = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c}));
    u32 r = e;
    if (n != 0) r = static_cast<u32>(call(ctx, cfn::PtrVector_Reserve, {v, n}));
    for (; e != 0; e = r = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {e}))) {
        if (call(ctx, cfn::AnimCmd_IsCmdElem, {e}) == 0) continue;
        const u32 cmd = static_cast<u32>(call(ctx, cfn::AnimCmd_FromElem, {e}));
        if (cmd != 0) PtrPush(ctx, v, cmd);
    }
    return r;
}

// The inlined 0x004B6DA0: the command-map lookup of element e's key (+20
// with bit 0x80 of +24, else 0) in the BST {+4 key, +8 value, +12 / +16
// children; unsigned compare} whose root is **(It + 12), keeping the
// iterator state at It (+96 s16 path depth, +16 path nodes, +100 the last
// node) and the value found in 0x011DE4A0. Returns the node found or 0.
// last_first: the store order of the second pass of 0x004C6778 (+100
// before the value).
constexpr u32 It = 0x010C2820, kFound = 0x011DE4A0;
u32 CmdMapLookup(u32 e, bool last_first) {
    const u32 key = (load<u8>(e + 24) & 0x80) != 0 ? load<u32>(e + 20) : 0;
    u32 node = load<u32>(load<u32>(It + 12));
    if (node == 0) {
        store<u32>(kFound, 0);
        store<u32>(It + 100, 0);
        return 0;
    }
    store<u16>(It + 96, 0);
    for (;;) {
        const u32 last = node;
        const u32 k = load<u32>(node + 4);
        if (k == key) {
            if (last_first) store<u32>(It + 100, node);
            store<u32>(kFound, load<u32>(node + 8));
            if (!last_first) store<u32>(It + 100, node);
            return node;
        }
        node = load<u32>(node + (k > key ? 12 : 16));
        if (node == 0) {
            if (last == 0) store<u16>(It + 96, 0);  // (unreachable in practice)
            store<u32>(It + 100, last);
            return 0;
        }
        const s32 d = static_cast<s16>(load<u16>(It + 96));
        store<u16>(It + 96, static_cast<u16>(d + 1));
        store<u32>(It + 16 + 4 * static_cast<u32>(d), last);
    }
}

// Append p to the PtrVector v ({+0 data, +4 u16 capacity, +8 u16 count,
// +10 u16 high-water}), growing it to twice its capacity when full.
void PtrPush(PPUContext& ctx, u32 v, u32 p) {
    const u32 cap = load<u16>(v + 4);
    if (load<u16>(v + 8) == cap) call(ctx, cfn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
    const u32 cnt = load<u16>(v + 8);
    store<u32>(load<u32>(v) + ((cnt << 2) & 0x3FFFCu), p);
    const u32 n1 = load<u16>(v + 8) + 1, hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n1));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFF) ? hw : n1));
}

}  // namespace

// u32 AnimCmd_Ctor_0007DA1C(AnimCmd* o, XmlCursor* c): vtable 0x00B0FBA8,
// children +24; hash +12, bool +20, +16 = (hash attribute 0x0027873A ==
// 0x0023FEEC).
DECOMP(0007DA1C, AnimCmd_Ctor_0007DA1C) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    call(ctx, cfn::AnimCmd_ConstructBase, {o});
    store<u32>(o, 0x00B0FBA8);
    call(ctx, cfn::PtrVector_Construct, {o + 24});
    store<u16>(o + 30, 4);
    store<u32>(o + 12, 0);
    store<u8>(o + 20, 0);
    store<u32>(o + 16, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x8A93973F, o + 12, 0);
    Stack frame(ctx, 16);
    const u32 s = frame.addr();
    store<u32>(s, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x0027873A, s, 0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x165BDB29, o + 20, 0);
    store<u32>(o + 16, load<u32>(s) == 0x0023FEECu ? 1 : 0);
    ret(ctx, ChildCmds(ctx, c, o + 24));
}

// u32 AnimCmd_Ctor_0007EFE4(AnimCmd* o, XmlCursor* c): vtable 0x00B0FC20,
// children +12; hash +32, s32 +36 (one less when given and positive),
// bool +40.
DECOMP(0007EFE4, AnimCmd_Ctor_0007EFE4) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    call(ctx, cfn::AnimCmd_ConstructBase, {o});
    store<u32>(o, 0x00B0FC20);
    call(ctx, cfn::PtrVector_Construct, {o + 12});
    store<u16>(o + 18, 4);
    store<u32>(o + 32, 0);
    store<u32>(o + 36, 0);
    store<u8>(o + 40, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xD782B635, o + 32, 0);
    const u32 r = XS32(ctx, c, 0x3683141C, o + 36);
    const u8 b40 = load<u8>(o + 40);
    if (r == 0 && static_cast<s32>(load<u32>(o + 36)) > 0) store<u32>(o + 36, load<u32>(o + 36) - 1);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x042BAED8, o + 40, b40);
    ret(ctx, ChildCmds(ctx, c, o + 12));
}

// u32 AnimCmd_Ctor_0007E110(AnimCmd* o, XmlCursor* c): vtable 0x00B0FBD0,
// children +28; hashes +12..+24 and bools +48 / +49 (default 0).
DECOMP(0007E110, AnimCmd_Ctor_0007E110) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    call(ctx, cfn::AnimCmd_ConstructBase, {o});
    store<u32>(o, 0x00B0FBD0);
    call(ctx, cfn::PtrVector_Construct, {o + 28});
    store<u16>(o + 34, 4);
    for (u32 k = 0; k < 4; ++k) store<u32>(o + 12 + 4 * k, 0);
    store<u8>(o + 48, 0);
    store<u8>(o + 49, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x8A93973F, o + 12, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x7A30D7E3, o + 16, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x79F40122, o + 20, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xC48E8CA9, o + 24, 0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x6C59DEC2, o + 48, 0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0xA9D1B0C2, o + 49, 0);
    ret(ctx, ChildCmds(ctx, c, o + 28));
}

// u32 XmlElem_Ctor_0007E944(XmlElem* o, XmlCursor* c): vtable 0x00B0FBF8,
// children +24; hashes +12 / +16, bool +44 (+45 = 0 unless that read
// returned 1), +20 = the mode of the hash attribute 0x04B8CC71
// (0x7A599AA9 -> 1, 0x58A96C30 -> 0, 0x0AEB2139 -> 2; default 1).
DECOMP(0007E944, XmlElem_Ctor_0007E944) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    call(ctx, cfn::Obj_InitVt_B21C98, {o});
    store<u32>(o + 12, 0);
    store<u32>(o, 0x00B0FBF8);
    store<u32>(o + 16, 0);
    store<u32>(o + 20, 1);
    call(ctx, cfn::PtrVector_Construct, {o + 24});
    store<u8>(o + 44, 0);
    store<u8>(o + 45, 1);
    store<u16>(o + 30, 4);
    XHash(ctx, c, 0x00011F9D, o + 12);
    XHash(ctx, c, 0x3419743C, o + 16);
    if (XBool(ctx, c, 0x0F6DEBB8, o + 44) != 1) store<u8>(o + 45, 0);
    Stack frame(ctx, 16);
    const u32 s = frame.addr();
    store<u32>(s, 0);
    if (XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x04B8CC71, s, 0) == 0) {
        const u32 h = load<u32>(s);
        if (h == 0x7A599AA9u) store<u32>(o + 20, 1);
        else if (h == 0x58A96C30u) store<u32>(o + 20, 0);
        else if (h == 0x0AEB2139u) store<u32>(o + 20, 2);
    }
    ret(ctx, ChildCmds(ctx, c, o + 24));
}

// u32 AnimCmdList_Read_004B6E74(List* o, XmlCursor* c): the child commands
// of c into the PtrVector at o + 8. The element lookup 0x004B6DA0 is
// inlined in the counting pass: the key (+20 of an element with bit 0x80
// of +24, else 0) is searched in the command map whose root is
// **(0x010C2820 + 12) (BST {+4 key, +8 value, +12 / +16 children},
// unsigned compare), with the iterator state at 0x010C2820 (+96 s16 path
// depth, +16 path nodes, +100 the last node) and the found value in
// 0x011DE4A0. The second pass appends every command 0x004B6C88 makes.
DECOMP(004B6E74, AnimCmdList_Read_004B6E74) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    s32 n = 0;
    for (u32 e = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c})); e != 0;) {
        if (CmdMapLookup(e, false) != 0) ++n;
        e = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {e}));
        if (e == 0 && n > 0) call(ctx, cfn::PtrVector_Reserve, {o + 8, static_cast<u32>(n)});
    }
    const u32 v = o + 8;
    u32 r = 0;
    for (u32 e = r = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c})); e != 0;
         e = r = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {e}))) {
        const u32 cmd = static_cast<u32>(call(ctx, cfn::AnimCmd_FromElem, {e}));
        if (cmd != 0) PtrPush(ctx, v, cmd);
    }
    ret(ctx, r);
}

// u32 AnimCmd_Ctor_0009829C(AnimCmd* o, XmlCursor* c): vtable 0x00B10D50;
// four 28-byte slots +12 (PtrVector, +20 / +24 hashes) and the PtrVector
// +136; hashes +128 / +132. Child elements (flag 0x80): 0x001F7330 fills
// the next slot (count +124, up to 4; hashes 0x79F40122 / 0xC48E8CA9) and
// keeps it when 0x0009810C(o, node, slot) succeeds; 0x86DF6221 is read
// into +136 by 0x0009810C while that is still empty. Returns 0.
DECOMP(0009829C, AnimCmd_Ctor_0009829C) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 kReadSlot = 0x0009810C;  // (o, node, slot) -> bool (name?)
    call(ctx, cfn::AnimCmd_ConstructBase, {o});
    store<u32>(o, 0x00B10D50);
    for (u32 i = 0; i < 4; ++i) {
        const u32 e = o + 12 + 28 * i;
        call(ctx, cfn::PtrVector_Construct, {e});
        store<u16>(e + 6, 4);
        store<u32>(e + 20, 0);
        store<u32>(e + 24, 0);
    }
    call(ctx, cfn::PtrVector_Construct, {o + 136});
    store<u16>(o + 142, 4);
    store<u32>(o + 128, 0);
    store<u32>(o + 132, 0);
    store<u32>(o + 124, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x8A93973F, o + 128, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x7A30D7E3, o + 132, 0);
    for (u32 n = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c})); n != 0;
         n = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {n}))) {
        if ((load<u8>(n + 24) & 0x80) == 0) continue;
        const u32 h = load<u32>(n + 20);
        if (h == 0x001F7330u) {
            const u32 k = load<u32>(o + 124);
            if (k >= 4) continue;
            const u32 e = o + 12 + 28 * k;
            XRead(ctx, xfn::XmlCursor_ReadHash, n, 0x79F40122, e + 20, 0);
            XRead(ctx, xfn::XmlCursor_ReadHash, n, 0xC48E8CA9, e + 24, 0);
            if (load<u32>(e + 20) == 0 && load<u32>(e + 24) == 0) continue;
            if (call(ctx, kReadSlot, {o, n, e}) != 0) store<u32>(o + 124, load<u32>(o + 124) + 1);
        } else if (h == 0x86DF6221u) {
            if (load<u16>(o + 144) == 0) call(ctx, kReadSlot, {o, n, o + 136});
        }
    }
    ret(ctx, 0);
}

// u32 AnimCmd_Ctor_000A3108(AnimCmd* o, XmlCursor* c): vtable 0x00B11698;
// the PtrVector-style array +12 of 16-byte records {0, hash 0x7A30D7E3,
// hash 0x3EE71C7A, hash 0x31F8E7E5} from the 0x286A440C children (initial
// capacity 16), +32 = the s32 attribute 0x368328E5 (0 when absent or -1),
// hash +36, f32 +40 (1) .. +52 (-1). Returns the last ReadF32 result.
DECOMP(000A3108, AnimCmd_Ctor_000A3108) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 kPtrVectorInit = 0x00551598;     // (PtrVector*, capacity, 0, 1)
    constexpr u32 kXmlNextSibling = 0x004F8FC8;    // (node, tag hash) -> node or 0
    constexpr u32 kXmlFirstChild = 0x004F92BC;     // (c, tag hash) -> node or 0
    constexpr u32 kTag = 0x286A440C;
    call(ctx, cfn::AnimCmd_ConstructBase, {o});
    const u32 v = o + 12;
    store<u32>(o, 0x00B11698);
    call(ctx, cfn::PtrVector_Construct, {v});
    store<u16>(o + 18, 16);
    call(ctx, kPtrVectorInit, {v, 16, 0, 1});
    store<u32>(o + 32, 0);
    store<u32>(o + 36, 0);
    store<u32>(o + 40, 0x3F800000);
    store<u32>(o + 44, 0);
    store<u32>(o + 48, 0);
    store<u32>(o + 52, 0xBF800000);
    Stack frame(ctx, 32);
    const u32 S = frame.addr() - 112;  // r1+112 the s32, +116 the record
    const u32 rec = S + 116;
    for (u32 n = static_cast<u32>(call(ctx, kXmlFirstChild, {c, kTag})); n != 0;
         n = static_cast<u32>(call(ctx, kXmlNextSibling, {n, kTag}))) {
        for (u32 k = 0; k < 4; ++k) store<u32>(rec + 4 * k, 0);
        XRead(ctx, xfn::XmlCursor_ReadHash, n, 0x7A30D7E3, rec + 4, 0);
        XRead(ctx, xfn::XmlCursor_ReadHash, n, 0x31F8E7E5, rec + 12, 0);
        XRead(ctx, xfn::XmlCursor_ReadHash, n, 0x3EE71C7A, rec + 8, 0);
        const u32 cap = load<u16>(o + 16);
        if (load<u16>(o + 20) == cap) call(ctx, cfn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
        const u32 cnt = load<u16>(o + 20);
        const u32 slot = load<u32>(o + 12) + ((cnt << 4) & 0xFFFF0u);
        const u32 n1 = load<u16>(o + 20) + 1, hw = load<u16>(o + 22);
        store<u32>(slot, load<u32>(rec));
        store<u32>(slot + 4, load<u32>(rec + 4));
        store<u32>(slot + 8, load<u32>(rec + 8));
        store<u16>(o + 20, static_cast<u16>(n1));
        store<u32>(slot + 12, load<u32>(rec + 12));
        store<u16>(o + 22, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFF) ? hw : n1));
    }
    const u32 s = S + 112;
    store<u32>(s, 0xFFFFFFFFu);
    call(ctx, xfn::XmlCursor_ReadS32, {c, 0x368328E5, s, ~0ull});  // r6 = -1 (64-bit, li)
    u32 val = load<u32>(s);
    const u32 d36 = load<u32>(o + 36);
    if (val == 0xFFFFFFFFu) {
        val = 0;
        store<u32>(s, 0);
    }
    store<u32>(o + 32, val);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x75751B32, o + 36, d36);
    XF32(ctx, c, 0xBD79437D, o + 40);
    XF32(ctx, c, 0x3EE9A5E8, o + 44);
    XF32(ctx, c, 0x3EEC218C, o + 48);
    ret(ctx, XF32(ctx, c, 0x32261AEB, o + 52));
}

// u32 AnimCmdList_Ctor_004C6778(List* o, XmlCursor* c): +4 = 1, vtable
// 0x00B22330, the child commands +28 (both passes with the command-map
// lookup inlined; only found elements are made), +16 = the hash of the
// name attribute 0x00AEF0A4, f32 +20 / +24 (-1e7 / 1e7, defaults the f32
// at 0x004C6774), bool +48. Returns 0 (or Reserve's r3 without children).
DECOMP(004C6778, AnimCmdList_Ctor_004C6778) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 K = 0x004C6774;
    const u32 v = o + 28;
    store<u16>(o + 4, 1);
    store<u32>(o, 0x00B22330);
    call(ctx, cfn::PtrVector_Construct, {v});
    store<u16>(o + 34, 4);
    store<u32>(o + 16, 0);
    store<u8>(o + 48, 0);
    store<u32>(o + 20, 0xCB189680);
    store<u32>(o + 24, 0x4B189680);
    const u32 a = static_cast<u32>(call(ctx, 0x004F9628, {c, 0x00AEF0A4}));  // XmlCursor_FindAttrByName
    if (a != 0) store<u32>(o + 16, static_cast<u32>(call(ctx, 0x0056A710, {a})));  // String_HashNoCase
    XF32Def(ctx, c, 0x00012A32, o + 20, K);
    XF32Def(ctx, c, 0x00012944, o + 24, K);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x6C59DEC2, o + 48, 0);
    u32 n = 0;
    for (u32 e = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c})); e != 0;
         e = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {e})))
        if (CmdMapLookup(e, false) != 0) ++n;
    u32 e = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c}));
    u32 r = n;
    if (n != 0) r = static_cast<u32>(call(ctx, cfn::PtrVector_Reserve, {v, n}));
    for (; e != 0; e = r = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {e}))) {
        if (CmdMapLookup(e, true) == 0) continue;
        const u32 cmd = static_cast<u32>(call(ctx, cfn::AnimCmd_FromElem, {e}));
        if (cmd != 0) PtrPush(ctx, v, cmd);
    }
    ret(ctx, r);
}

// u32 AnimCmdList_Ctor_004C5DF0(List* o, XmlCursor* c): +4 = 1, vtable
// 0x00B22308, the child commands +40 (as 004C6778); hash +36, the value
// attribute 0x04DA9951 read by its type (0x004F9768): 1 / 2 the f32 +32
// (default the f32 at 0x004C5DE8), 4 the vec3 +16 (default zero), else
// the hash +12; bool +60.
DECOMP(004C5DF0, AnimCmdList_Ctor_004C5DF0) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 K = 0x004C5DE8, kZeroVec = 0x01164B40, kValue = 0x04DA9951;
    constexpr u32 kXmlAttrType = 0x004F9768;  // (c, hash, u32* type) -> non-zero when present (name?)
    const u32 v = o + 40;
    store<u16>(o + 4, 1);
    store<u32>(o, 0x00B22308);
    call(ctx, cfn::PtrVector_Construct, {v});
    store<u32>(o + 36, 0);
    store<u16>(o + 46, 4);
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 16 + 4 * k, load<u32>(kZeroVec + 4 * k));
    ctx.fpr[1] = F32(load<u32>(kZeroVec + 8));
    store<u32>(o + 32, 0);
    store<u8>(o + 60, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xE42AC27C, o + 36, 0);
    Stack frame(ctx, 16);
    const u32 s = frame.addr();
    if (call(ctx, kXmlAttrType, {c, kValue, s}) != 0) {
        const s32 t = static_cast<s32>(load<u32>(s));
        if (t == 4) XRead(ctx, xfn::XmlCursor_ReadVec3, c, kValue, o + 16, kZeroVec);
        else if (t > 0 && t <= 2) XF32Def(ctx, c, kValue, o + 32, K);
        else XRead(ctx, xfn::XmlCursor_ReadHash, c, kValue, o + 12, 0);
    }
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x6C59DEC2, o + 60, 0);
    u32 n = 0;
    for (u32 e = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c})); e != 0;
         e = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {e})))
        if (CmdMapLookup(e, false) != 0) ++n;
    u32 e = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c}));
    u32 r = n;
    if (n != 0) r = static_cast<u32>(call(ctx, cfn::PtrVector_Reserve, {v, n}));
    for (; e != 0; e = r = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {e}))) {
        if (CmdMapLookup(e, true) == 0) continue;
        const u32 cmd = static_cast<u32>(call(ctx, cfn::AnimCmd_FromElem, {e}));
        if (cmd != 0) PtrPush(ctx, v, cmd);
    }
    ret(ctx, r);
}
