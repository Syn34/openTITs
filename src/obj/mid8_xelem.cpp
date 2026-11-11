// Game code, medium sweep part 8: XML element constructors on the
// Obj_InitVt_B21C98 base (vtable, defaults, then the attributes).
#include "mid8.h"

using namespace sly4;
using namespace sly4::mid8;

namespace {

namespace efn {
constexpr u32 Obj_InitVt_B21C98 = 0x004BB3B0;  // (o): +4 = 1, vtable 0x00B21C98 (reads r3 only)
constexpr u32 String_Copy = 0x00553684;        // (dst, src, size)
constexpr u32 Ptr_DerefNonZero = 0x00446960;   // (p) -> bool
constexpr u32 RefNode_Release = 0x004432DC;    // (node)
constexpr u32 Node_PushF65B7C = 0x00446984;    // (a, b) -> node
}  // namespace efn

// The ref node at o + nd (default +24) rebuilt from (+k, +k + 4) when +k
// is set and the old node is gone (Ptr_DerefNonZero(node) == 0:
// RefNode_Release, then Node_PushF65B7C); returns the last call's result
// (0 when +k is 0).
u32 RefNodeRebuild(PPUContext& ctx, u32 o, u32 k, u32 nd = 24) {
    if (load<u32>(o + k) == 0) return 0;
    const u32 r = static_cast<u32>(call(ctx, efn::Ptr_DerefNonZero, {load<u32>(o + nd)}));
    if (r != 0) return r;
    call(ctx, efn::RefNode_Release, {load<u32>(o + nd)});
    const u32 n = static_cast<u32>(call(ctx, efn::Node_PushF65B7C, {load<u32>(o + k), load<u32>(o + k + 4)}));
    store<u32>(o + nd, n);
    return n;
}

// The non-empty string attribute `hash` copied to dst (String_Copy, size n).
void CopyStrAttr(PPUContext& ctx, u32 c, u32 hash, u32 dst, u32 n) {
    const u32 s = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {c, hash}));
    if (s != 0 && load<u8>(s) != 0) call(ctx, efn::String_Copy, {dst, s, n});
}

}  // namespace

// u32 XmlElem_Ctor_000A2568(XmlElem* o, XmlCursor* c): vtable 0x00B115F8;
// hashes +12 / +16 / +20 (default 0; +16 / +20 then make the ref node
// +24), f32 +28 (1), hashes +32 / +36, bools +40 (true) / +41, hashes +44
// / +48 (0) / +52 (default +48), +124 / +128, s32 +132, the string
// 0xB3D53C18 at +56 (64 bytes). Returns the RefNodeRebuild result.
DECOMP(000A2568, XmlElem_Ctor_000A2568) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    call(ctx, efn::Obj_InitVt_B21C98, {o});
    store<u32>(o + 12, 0);
    store<u32>(o + 16, 0);
    store<u32>(o, 0x00B115F8);
    store<u32>(o + 20, 0);
    store<u32>(o + 24, 0);
    store<u32>(o + 28, 0x3F800000);
    store<u32>(o + 32, 0);
    store<u32>(o + 36, 0);
    store<u8>(o + 40, 1);
    store<u8>(o + 41, 0);
    store<u32>(o + 44, 0);
    store<u32>(o + 124, 0);
    store<u32>(o + 128, 0);
    store<u32>(o + 132, 0);
    store<u8>(o + 56, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xE3A6DCCD, o + 12, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x3A1B33FA, o + 16, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x001EFE3C, o + 20, 0);
    XF32(ctx, c, 0x973DDB1A, o + 28);
    XHash(ctx, c, 0x3A191FB5, o + 32);
    XHash(ctx, c, 0x9A0F95CA, o + 36);
    XBool(ctx, c, 0x8488565B, o + 40);
    XBool(ctx, c, 0x6D18DE7D, o + 41);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x7A30D7E3, o + 44, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xA7E4BEFF, o + 48, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x4B04D2A0, o + 52, load<u32>(o + 48));
    XHash(ctx, c, 0x1050F752, o + 124);
    XHash(ctx, c, 0x7E6EFA38, o + 128);
    XS32(ctx, c, 0x5CFBABDC, o + 132);
    CopyStrAttr(ctx, c, 0xB3D53C18, o + 56, 64);
    ret(ctx, RefNodeRebuild(ctx, o, 16));
}

// Prompt* Prompt_New_000928D0(?, XmlCursor* c) (reads r4 only): a 272-byte
// prompt (Mem_Alloc(272, 0, 16); 0 when that fails), Obj_InitVt base,
// vtable 0x00B10A58: the "msg" attribute copied to +12 (128 chars,
// when non-empty), bools 0x36F05514 (+270) / 0x21BE4C63 (+271, both true)
// for the X and circle buttons; +141 = String_Format of "$BUTTON_X %s
// $BUTTON_CIRCLE %s" / "$BUTTON_X %s" / "$BUTTON_CIRCLE %s" with the
// strings 0x3CD0795A / 0xC2803EA7 (defaults "$STR_YES" / "$STR_NO").
DECOMP(000928D0, Prompt_New_000928D0) {
    const u32 c = arg<u32>(ctx, 1);
    constexpr u32 kStrMsg = 0x00AC5668, kStrYes = 0x00AC5670, kStrNo = 0x00AC5680;
    constexpr u32 kFmtBoth = 0x00AC5688, kFmtX = 0x00AC56A8, kFmtCircle = 0x00AC56B8;
    const u32 p = static_cast<u32>(call(ctx, m7fn::Mem_Alloc, {272, 0, 16}));
    if (p == 0) return ret(ctx, 0);
    call(ctx, efn::Obj_InitVt_B21C98, {p, c});
    store<u8>(p + 12, 0);
    store<u8>(p + 141, 0);
    store<u32>(p, 0x00B10A58);
    store<u8>(p + 270, 1);
    store<u8>(p + 271, 1);
    const u32 s = static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, kStrMsg}));
    if (s != 0 && load<u8>(s) != 0) call(ctx, m8fn::String_CopyN, {p + 12, s, 128});
    XBool(ctx, c, 0x36F05514, p + 270);
    XBool(ctx, c, 0x21BE4C63, p + 271);
    u32 yes = kStrYes, no = kStrNo;
    if (call(ctx, xfn::XmlCursor_FindAttr, {c, 0x3CD0795A}) != 0) yes = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {c, 0x3CD0795A}));
    if (call(ctx, xfn::XmlCursor_FindAttr, {c, 0xC2803EA7}) != 0) no = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {c, 0xC2803EA7}));
    const bool x = load<u8>(p + 270) != 0, circle = load<u8>(p + 271) != 0;
    if (x || circle) {
        const u32 text = static_cast<u32>(x ? (circle ? call(ctx, mid1::m1fn::String_Format, {kFmtBoth, yes, no})
                                                      : call(ctx, mid1::m1fn::String_Format, {kFmtX, yes}))
                                            : call(ctx, mid1::m1fn::String_Format, {kFmtCircle, no}));
        call(ctx, m8fn::String_CopyN, {p + 141, text, 128});
    }
    ret(ctx, p);
}

namespace {

// A non-empty "name" (0x00AC59C0) attribute of node appended to o's
// 128-byte string slots at +12 (count +524, no bound; String_Copy 127).
void AddName(PPUContext& ctx, u32 o, u32 node) {
    const u32 s = static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {node, 0x00AC59C0}));
    if (s == 0 || load<u8>(s) == 0) return;
    const u32 n = load<u32>(o + 524);
    store<u32>(o + 524, n + 1);
    call(ctx, efn::String_Copy, {o + 12 + ((n << 7) & 0xFFFFFF80u), s, 127});
}

}  // namespace

// u32 XmlElem_Ctor_000A1D34(XmlElem* o, XmlCursor* c): vtable 0x00B115A8,
// Obj_ResetBlock_000A1C28 defaults; the name attribute of c and of each
// 0x04624710 child into the string slots (AddName), vec2 +528 / +536,
// hash +544, colours +548 / +552, bools +556 / +557 (+558 = whether that
// one was found) / +559..+562. Returns the last ReadBool result.
DECOMP(000A1D34, XmlElem_Ctor_000A1D34) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 Obj_ResetBlock_000A1C28 = 0x000A1C28, kLine = 0x04624710;
    call(ctx, efn::Obj_InitVt_B21C98, {o, c});
    store<u32>(o, 0x00B115A8);
    call(ctx, Obj_ResetBlock_000A1C28, {o});
    AddName(ctx, o, c);
    for (u32 node = static_cast<u32>(call(ctx, m8fn::XmlCursor_FirstChild, {c, kLine})); node != 0;
         node = static_cast<u32>(call(ctx, m8fn::XmlCursor_NextSibling, {node, kLine})))
        AddName(ctx, o, node);
    XVec2(ctx, c, 0x04B1112A, o + 528);
    XVec2(ctx, c, 0x8ACA5893, o + 536);
    XHash(ctx, c, 0x42E874AA, o + 544);
    XRead(ctx, xfn::XmlCursor_ReadColor, c, 0x3060AABF, o + 548, o + 548);
    XRead(ctx, xfn::XmlCursor_ReadColor, c, 0xBF1E5566, o + 552, o + 552);
    XBool(ctx, c, 0x8488565B, o + 556);
    const u32 r = XBool(ctx, c, 0x9C6B7DAB, o + 557);
    store<u8>(o + 558, r == 0 ? 1 : 0);
    XBool(ctx, c, 0x8D2C4D64, o + 559);
    XBool(ctx, c, 0x0020CE7C, o + 560);
    XBool(ctx, c, 0x4DFD4BFE, o + 561);
    ret(ctx, XBool(ctx, c, 0x367CF240, o + 562));
}

// u32 XmlElem_Ctor_000838F4(XmlElem* o, XmlCursor* c): vtable 0x00B0FF40;
// the target hash 0xE3A6DCCD (+28) or else 0x3A1B33FA (+12) - neither:
// return that (non-zero) ReadHash result; then hashes +16 / +32 / +36
// (default +32) / +40, vec3 +48 (zero), f32 +64 (1), hashes +68 / +72,
// bools +76 / +77 / +78 (default 0) / +79 (true), hashes +80 / +84, s32
// +88, bool +93 (bit 31 of the word +92 cleared first). Returns +28 when
// set, else the RefNodeRebuild result of (+12, +16).
DECOMP(000838F4, XmlElem_Ctor_000838F4) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 kZero = 0x01164B40;
    call(ctx, efn::Obj_InitVt_B21C98, {o, c});
    for (u32 off : {12u, 16u, 28u, 32u, 36u, 40u}) store<u32>(o + off, 0);
    store<u32>(o, 0x00B0FF40);
    store<u32>(o + 48, load<u32>(kZero));  // lfs / stfs
    store<u8>(o + 20, 0);
    store<u32>(o + 24, 0);
    store<u32>(o + 52, load<u32>(kZero + 4));
    store<u32>(o + 56, load<u32>(kZero + 8));
    const u32 w92 = load<u32>(o + 92) & 0x7FFFFFFFu;
    store<u32>(o + 64, 0x3F800000);
    store<u32>(o + 68, 0);
    store<u32>(o + 72, 0);
    store<u8>(o + 76, 0);
    store<u8>(o + 77, 0);
    store<u8>(o + 78, 0);
    store<u8>(o + 79, 1);
    store<u32>(o + 80, 0);
    store<u32>(o + 84, 0);
    store<u32>(o + 88, 0);
    store<u32>(o + 92, w92);
    store<u8>(o + 93, 0);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(kZero + 8)));
    u32 r = XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xE3A6DCCD, o + 28, 0);
    if (r != 0) {
        r = XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x3A1B33FA, o + 12, 0);
        if (r != 0) return ret(ctx, r);
    }
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x001EFE3C, o + 16, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xA7E4BEFF, o + 32, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x4B04D2A0, o + 36, load<u32>(o + 32));
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x7F247172, o + 40, 0);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x313AB0C1, o + 48, kZero);
    XF32(ctx, c, 0x973DDB1A, o + 64);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x3404783C, o + 68, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x9A0F95CA, o + 72, 0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0xD547CE43, o + 76, 0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0xFFB20224, o + 77, 0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x6334AE6F, o + 78, 0);
    XBool(ctx, c, 0x92BCF1F0, o + 79);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x00255BF3, o + 80, 0);
    XHash(ctx, c, 0x7E6EFA38, o + 84);
    XS32(ctx, c, 0x5CFBABDC, o + 88);
    XBool(ctx, c, 0xC6C71707, o + 93);
    const u32 t = load<u32>(o + 28);
    ret(ctx, t != 0 ? t : RefNodeRebuild(ctx, o, 12));
}

// u32 XmlElem_Ctor_000A1224(XmlElem* o, XmlCursor* c): vtable 0x00B11558;
// hash +164 (else the non-empty "message" attribute copied to +12, 64
// bytes), hash +168 (else "message2" to +77), hashes +172 /
// +176, s32 +180, hash +184, the flag bits 31 / 30 / 29 of +188 (bools
// 0x8488565B (true) / 0x1D373FEB / 0x92DE619A); the vector +144
// (capacity 6, 12-byte records {hash 0x0024728B, bool 0xF5897C08 (true)},
// 9 bytes copied) of the 0xDB7A793A child elements with a non-zero hash,
// at most 6. Returns 0.
DECOMP(000A1224, XmlElem_Ctor_000A1224) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 rec = frame.addr(), t124 = rec + 12;
    call(ctx, efn::Obj_InitVt_B21C98, {o, c});
    store<u32>(o, 0x00B11558);
    const u32 v = o + 144;
    mid1::PtrVecNew(ctx, v, 6, 12, 1);
    const u32 w = (load<u32>(o + 188) | 0x80000000u) & ~0x60000000u;
    for (u32 off = 164; off < 188; off += 4) store<u32>(o + off, 0);
    store<u8>(o + 12, 0);
    store<u32>(o + 188, w);
    store<u8>(o + 77, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xB823E1C0, o + 164, 0);
    if (load<u32>(o + 164) == 0) {
        const u32 s = static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, 0x00AC599C}));
        if (s != 0 && load<u8>(s) != 0) call(ctx, efn::String_Copy, {o + 12, s, 64});
    }
    XHash(ctx, c, 0x4C4845FC, o + 168);
    if (load<u32>(o + 168) == 0) {
        const u32 s = static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, 0x00AC59A4}));
        if (s != 0 && load<u8>(s) != 0) call(ctx, efn::String_Copy, {o + 77, s, 64});
    }
    XHash(ctx, c, 0x7A73437D, o + 172);
    XHash(ctx, c, 0xAB5BFA6A, o + 176);
    XS32(ctx, c, 0xE7CEA5A4, o + 180);
    XHash(ctx, c, 0xA8BF12E3, o + 184);
    store<u8>(t124, (load<u32>(o + 188) & 0x80000000u) != 0 ? 1 : 0);  // the default also goes to the byte first
    mid5::XBit<u32>(ctx, c, 0x8488565B, o + 188, 0x80000000u, t124);
    mid5::XBit<u32>(ctx, c, 0x1D373FEB, o + 188, 0x40000000u, t124);
    mid5::XBit<u32>(ctx, c, 0x92DE619A, o + 188, 0x20000000u, t124);
    for (u32 node = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c})); node != 0;
         node = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {node}))) {
        if ((load<u8>(node + 24) & 0x80) == 0 || load<u32>(node + 20) != 0xDB7A793A || load<u16>(v + 8) >= 6) continue;
        store<u32>(rec, 0);
        store<u32>(rec + 4, 0);
        store<u8>(rec + 8, 1);
        XRead(ctx, xfn::XmlCursor_ReadHash, node, 0x0024728B, rec, 0);
        XBool(ctx, node, 0xF5897C08, rec + 8);
        if (load<u32>(rec) == 0) continue;
        u32 n = load<u16>(v + 8);
        const u32 cap = load<u16>(v + 4);
        if (n == cap) {
            call(ctx, mid1::m1fn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
            n = load<u16>(v + 8);
        }
        call(ctx, m8fn::Memmove, {load<u32>(v) + n * 12, rec, 9});
        const u32 n1 = load<u16>(v + 8) + 1, hw = load<u16>(v + 10);
        store<u16>(v + 8, static_cast<u16>(n1));
        store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFF) ? hw : n1));
    }
    ret(ctx, 0);
}

// u32 XmlElem_Ctor_00093150(XmlElem* o, XmlCursor* c): vtable 0x00B10AD0;
// hashes +12 / +16 (0) / +20, vec3 +32 / +48 (zero), the euler angles
// 0x313AB844 (degrees; constants at 0x00093144) as the quaternion +64
// (default identity), vec3 +80 (an angle, scaled by the same constant),
// hashes +96..+108 / +120 (0), s32 +112 = 0x187976E3 (1; kept when
// negative) and +116 = 0x187975F5 (1; kept when below +112), the
// "entityName" attribute copied to +125 (64 bytes). Returns the
// String_Copy result (0 without the attribute).
DECOMP(00093150, XmlElem_Ctor_00093150) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 kZero = 0x01164B40, kQuat = 0x01164C90, kAngles = 0x00093144, kStrEntityName = 0x00AC56F4;
    Stack frame(ctx, 32);
    const u32 ang = frame.addr(), t128 = ang + 16;
    call(ctx, efn::Obj_InitVt_B21C98, {o, c});
    store<u32>(o + 12, 0);
    store<u32>(o + 16, 0);
    store<u32>(o + 20, 0);
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 32 + 4 * k, load<u32>(kZero + 4 * k));  // lfs / stfs
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 48 + 4 * k, load<u32>(kZero + 4 * k));
    store<u32>(o, 0x00B10AD0);
    for (u32 k = 0; k < 4; ++k) store<u32>(o + 64 + 4 * k, load<u32>(kQuat + 4 * k));
    store<u32>(o + 80, load<u32>(kZero));
    store<u32>(o + 84, load<u32>(kZero + 4));
    store<u32>(o + 96, 0);
    store<u32>(o + 88, load<u32>(kZero + 8));
    store<u32>(o + 100, 0);
    store<u32>(o + 104, 0);
    store<u32>(o + 108, 0);
    store<u32>(o + 112, 1);
    store<u32>(o + 116, 1);
    store<u32>(o + 120, 0);
    store<u8>(o + 125, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x8A93973F, o + 12, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x7F247172, o + 16, 0);
    XHash(ctx, c, 0xB2E59A9F, o + 20);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x313AB0C1, o + 32, kZero);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0xBF28AC66, o + 48, kZero);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x313AB844, ang, kZero);
    const mid7::EulerQuat q = mid7::EulerToQuat(ctx, ang, kAngles);
    store<u32>(o + 76, W32(q.w));
    store<u32>(o + 64, W32(q.x));
    store<u32>(o + 72, W32(q.z));
    store<u32>(o + 68, W32(q.y));
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x29033A69, o + 80, kZero);
    const f32 k0 = F32(load<u32>(kAngles));
    f32 v[3];
    for (u32 k = 0; k < 3; ++k) v[k] = F32(load<u32>(o + 80 + 4 * k)) * k0;
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 80 + 4 * k, W32(v[k]));
    ctx.fpr[1] = v[0];
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x00255BF3, o + 96, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xFB888634, o + 100, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0xE6580F29, o + 104, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x700F12AD, o + 108, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x6B7F80BC, o + 120, 0);
    XRead(ctx, xfn::XmlCursor_ReadS32, c, 0x187976E3, t128, 1);
    if (static_cast<s32>(load<u32>(t128)) >= 0) store<u32>(o + 112, load<u32>(t128));
    XRead(ctx, xfn::XmlCursor_ReadS32, c, 0x187975F5, t128, 1);
    if (static_cast<s32>(load<u32>(t128)) >= static_cast<s32>(load<u32>(o + 112))) store<u32>(o + 116, load<u32>(t128));
    const u32 s = static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, kStrEntityName}));
    if (s == 0) return ret(ctx, 0);
    ret(ctx, call(ctx, efn::String_Copy, {o + 125, s, 64}));
}

// u32 XmlElem_Ctor_00093D50(XmlElem* o, XmlCursor* c): vtable 0x00B10AF8,
// constants at 0x00093D40 {default, deg -> rad, 0.5}: hash +12, f32 +16 /
// +20 (default k[0]), vec3 +32 / +48 (zero; +48 is an angle, scaled by
// k[1]); the vector +64 (capacity 8) of 48-byte records from the
// 0x03D22A4D children with the hash 0x000107E5 ({+0 that hash, +4 hash
// 0x63BBEE91, +8 hash 0x00255BF3 (0), +16 vec3 0x313AB0C1 (zero), +32 the
// euler angles 0x313AB844 (degrees) as a quaternion}). Returns 0.
DECOMP(00093D50, XmlElem_Ctor_00093D50) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 kZero = 0x01164B40, kQuat = 0x01164C90, K = 0x00093D40, kChild = 0x03D22A4D;
    Stack frame(ctx, 64);
    const u32 ang = frame.addr(), rec = ang + 16;  // the original's r1 + 112 / + 128
    call(ctx, efn::Obj_InitVt_B21C98, {o, c});
    store<u32>(o + 12, 0);
    store<u32>(o, 0x00B10AF8);
    store<u32>(o + 16, 0);
    store<u32>(o + 20, 0);
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 32 + 4 * k, load<u32>(kZero + 4 * k));  // lfs / stfs
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 48 + 4 * k, load<u32>(kZero + 4 * k));
    const u32 v = o + 64;
    mid1::PtrVecNew(ctx, v, 8, 48, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x7F247172, o + 12, 0);
    XF32Def(ctx, c, 0x9D9B30D5, o + 16, K);
    XF32Def(ctx, c, 0x0863027A, o + 20, K);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x76D0D5A8, o + 32, kZero);
    XRead(ctx, xfn::XmlCursor_ReadVec3, c, 0x425E8628, o + 48, kZero);
    const f32 k1 = F32(load<u32>(K + 4));
    f32 a[3];
    for (u32 k = 0; k < 3; ++k) a[k] = F32(load<u32>(o + 48 + 4 * k)) * k1;
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 48 + 4 * k, W32(a[k]));
    ctx.fpr[1] = a[0];
    for (u32 node = static_cast<u32>(call(ctx, m8fn::XmlCursor_FirstChild, {c, kChild})); node != 0;
         node = static_cast<u32>(call(ctx, m8fn::XmlCursor_NextSibling, {node, kChild}))) {
        store<u32>(rec, 0);
        store<u32>(rec + 4, 0);
        store<u32>(rec + 8, 0);
        for (u32 k = 0; k < 3; ++k) store<u32>(rec + 16 + 4 * k, load<u32>(kZero + 4 * k));
        for (u32 k = 0; k < 4; ++k) store<u32>(rec + 32 + 4 * k, load<u32>(kQuat + 4 * k));
        if (XRead(ctx, xfn::XmlCursor_ReadHash, node, 0x000107E5, rec, 0) != 0) continue;
        XRead(ctx, xfn::XmlCursor_ReadHash, node, 0x00255BF3, rec + 8, 0);
        XRead(ctx, xfn::XmlCursor_ReadVec3, node, 0x313AB0C1, rec + 16, kZero);
        XRead(ctx, xfn::XmlCursor_ReadVec3, node, 0x313AB844, ang, kZero);
        XRead(ctx, xfn::XmlCursor_ReadHash, node, 0x63BBEE91, rec + 4, 0);
        const mid7::EulerQuat q = mid7::EulerToQuat(ctx, ang, K + 4);
        store<u32>(rec + 44, W32(q.w));
        store<u32>(rec + 32, W32(q.x));
        store<u32>(rec + 40, W32(q.z));
        store<u32>(rec + 36, W32(q.y));
        u32 n = load<u16>(v + 8);
        const u32 cap = load<u16>(v + 4);
        if (n == cap) {
            call(ctx, mid1::m1fn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
            n = load<u16>(v + 8);
        }
        const u32 e = load<u32>(v) + n * 48;
        for (u32 off : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u, 44u}) store<u32>(e + off, load<u32>(rec + off));  // lwz / stw, lfs / stfs
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(rec + 44)));
        const u32 n1 = load<u16>(v + 8) + 1, hw = load<u16>(v + 10);
        store<u16>(v + 8, static_cast<u16>(n1));
        store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFF) ? hw : n1));
    }
    ret(ctx, 0);
}

// u32 XmlElem_Ctor_000A0558(XmlElem* o, XmlCursor* c): vtable 0x00B114D8
// (a button prompt); constants at 0x000A054C: hashes +84 / +88 (0, the
// ref node +92's key), +80, f32 +96, bool +100 (true), f32 +112 (default
// k[1]), the "subtitle" attribute to +12 (64 bytes, when non-empty),
// hashes +104 / +108, bools +120 / +121 (0; both set by the bool
// 0x23D89F8B); the button 0x051086F8 picks the mask +116 (0 without the
// attribute; unchanged for an unknown button) and its name, so +128 =
// "promptText" or else "%s $STR_CONTINUE" of that name (64 chars); hash
// +124 (default the word at 0x00F64D08), +191 = 0, bool +192, hash +196,
// s32 +200, hash +204. +96 = 0 when below k[0] with no button. Returns
// the RefNodeRebuild result of (+84, +88) at +92.
DECOMP(000A0558, XmlElem_Ctor_000A0558) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 K = 0x000A054C, kStrSubtitle = 0x00AC58D0, kStrPromptText = 0x00AC5968, kFmtContinue = 0x00AC5978;
    struct Button {
        u32 hash, mask, name;
    };
    static constexpr Button kButtons[] = {
        {0x00000A9B, 1, 0x00AC58E0},   {0x00201CA2, 2, 0x00AC58F0},  {0x00239807, 4, 0x00AC5900},  {0x04A5C9FC, 8, 0x00AC5910},
        {0x87EBFA08, 16, 0x00AC5920},  {0x923F4D1D, 32, 0x00AC5938}, {0x767FB0D0, 64, 0x00AC5948}, {0x00000058, 128, 0x00AC5958},
        {0x03D6A020, 128, 0x00AC5958},
    };
    Stack frame(ctx, 16);
    const u32 t112 = frame.addr(), t116 = t112 + 4;
    call(ctx, efn::Obj_InitVt_B21C98, {o, c});
    store<u32>(o + 80, 0);
    store<u32>(o, 0x00B114D8);
    store<u32>(o + 92, 0);
    store<u32>(o + 96, 0);
    store<u8>(o + 100, 1);
    store<u32>(o + 104, 0);
    store<u32>(o + 108, 0);
    store<u8>(o + 192, 0);
    store<u32>(o + 124, load<u32>(0x00F64D08));
    store<u32>(o + 196, 0);
    store<u32>(o + 200, 0);
    store<u32>(o + 204, 0);
    store<u8>(o + 12, 0);
    store<u8>(o + 128, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x3A1B33FA, o + 84, 0);
    XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x001EFE3C, o + 88, 0);
    XHash(ctx, c, 0x1050F752, o + 80);
    XF32(ctx, c, 0xC05CEEB3, o + 96);
    XBool(ctx, c, 0x8488565B, o + 100);
    XF32Def(ctx, c, 0xE68FC882, o + 112, K + 4);
    {
        const u32 s = static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, kStrSubtitle}));
        if (s != 0 && load<u8>(s) != 0) call(ctx, efn::String_Copy, {o + 12, s, 64});
    }
    XHash(ctx, c, 0x9518F698, o + 104);
    XHash(ctx, c, 0x4897260D, o + 108);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0xD06EE4FE, o + 120, 0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x91BAC895, o + 121, 0);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x23D89F8B, t116, 0);
    if (load<u8>(t116) != 0) {
        store<u8>(o + 120, 1);
        store<u8>(o + 121, 1);
    }
    u32 name = 0x00AC58C8;  // ""
    if (XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x051086F8, t112, 0) != 0) {
        store<u32>(o + 116, 0);
    } else {
        for (const Button& b : kButtons)
            if (load<u32>(t112) == b.hash) {
                store<u32>(o + 116, b.mask);
                name = b.name;
                break;
            }
    }
    XHash(ctx, c, 0xE2F075ED, o + 124);
    u32 text = static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, kStrPromptText}));
    if (text == 0 && name != 0) text = static_cast<u32>(call(ctx, mid1::m1fn::String_Format, {kFmtContinue, name}));
    call(ctx, m8fn::String_CopyN, {o + 128, text, 64});
    store<u8>(o + 191, 0);
    XBool(ctx, c, 0x6D18DE7D, o + 192);
    XHash(ctx, c, 0x7E6EFA38, o + 196);
    XS32(ctx, c, 0x5CFBABDC, o + 200);
    XHash(ctx, c, 0x7A30D7E3, o + 204);
    const u32 w96 = load<u32>(o + 96);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(w96));  // the lfs of the compare
    if (F32(w96) < F32(load<u32>(K)) && load<u32>(o + 116) == 0) store<u32>(o + 96, 0);
    ret(ctx, RefNodeRebuild(ctx, o, 84, 92));
}

// u32 XmlElem_Ctor_00079938(XmlElem* o, XmlCursor* c): vtable 0x00B0F860
// and defaults; the hash 0x8A93973F (+12; absent -> return that non-zero
// ReadHash result), then hash +16, f32 +44 / +48, hash +52, colour +60
// (default the bytes at 0x01164CA0), the enum 0xDB885954 (+64, five
// names at 0x00AC4B98, default 2), hashes +68 / +72, vec3 +80 / +96 /
// +112 / +128, f32 +176 / +184 / +180, bools +192..+194, f32 +56 / +188,
// s32 +196, vec3 +144 / +160 (each defaulting to its value). With bool
// +193 and +48 below the f32 at 0x00079930 a warning is formatted (the
// document path and line of c) and its String_Format result returned;
// otherwise 0 (bool +193 clear) or 0x00079930 (the original's leftover r3).
DECOMP(00079938, XmlElem_Ctor_00079938) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 kZero = 0x01164B40, kColour = 0x01164CA0, kMin = 0x00079930, kFmt = 0x00AC4BB8, kEnumNames = 0x00AC4B98;
    constexpr u32 Xml_FindResolved_004F9434 = 0x004F9434, Xml_ReadEnum_004FA458 = 0x004FA458;
    call(ctx, efn::Obj_InitVt_B21C98, {o, c});
    store<u32>(o + 12, 0);
    store<u32>(o, 0x00B0F860);
    store<u32>(o + 16, 0);
    store<u32>(o + 32, 0xEB2F2138);
    store<u32>(o + 36, 0x3F800000);
    store<u32>(o + 40, 0);
    store<u32>(o + 44, 0x3F000000);
    store<u32>(o + 48, 0x3F800000);
    store<u32>(o + 52, 0);
    store<u32>(o + 56, 0x3F800000);
    for (u32 k = 0; k < 4; ++k) store<u8>(o + 60 + k, load<u8>(kColour + k));
    store<u32>(o + 64, 2);
    store<u32>(o + 68, 0);
    store<u32>(o + 72, 0);
    for (u32 off : {80u, 96u, 112u, 128u, 144u})
        for (u32 k = 0; k < 3; ++k) store<u32>(o + off + 4 * k, load<u32>(kZero + 4 * k));  // lfs / stfs
    store<u32>(o + 160, load<u32>(kZero));
    store<u32>(o + 164, load<u32>(kZero + 4));
    store<u32>(o + 176, 0x42960000);
    store<u32>(o + 168, load<u32>(kZero + 8));
    store<u32>(o + 180, 0xBF800000);
    store<u32>(o + 184, 0x3D4CCCCD);
    store<u32>(o + 188, 0xBF800000);
    store<u8>(o + 192, 0);
    store<u8>(o + 193, 0);
    store<u8>(o + 194, 0);
    store<u32>(o + 196, 1);
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(kZero + 8)));
    const u32 r = XRead(ctx, xfn::XmlCursor_ReadHash, c, 0x8A93973F, o + 12, 0);
    if (r != 0) return ret(ctx, r);
    XHash(ctx, c, 0x00255BF3, o + 16);
    XF32(ctx, c, 0x85A12EE6, o + 44);
    XF32(ctx, c, 0x67E713C9, o + 48);
    XHash(ctx, c, 0xD64960FB, o + 52);
    XRead(ctx, xfn::XmlCursor_ReadColor, c, 0x03D53743, o + 60, o + 60);
    call(ctx, Xml_ReadEnum_004FA458, {c, 0xDB885954, o + 64, kEnumNames, 5, load<u32>(o + 64)});
    XHash(ctx, c, 0x03C725ED, o + 68);
    XHash(ctx, c, 0x03C725EE, o + 72);
    XVec3(ctx, c, 0xCE80B9FE, o + 80);
    XVec3(ctx, c, 0xCE80B9FF, o + 96);
    XVec3(ctx, c, 0x8CB0B04F, o + 112);
    XVec3(ctx, c, 0x8CB0B050, o + 128);
    XF32(ctx, c, 0xD5B765A9, o + 176);
    XF32(ctx, c, 0x4E047B49, o + 184);
    XF32(ctx, c, 0xC1C61801, o + 180);
    XBool(ctx, c, 0x34D5B66F, o + 192);
    XBool(ctx, c, 0xCD7B5B32, o + 193);
    XBool(ctx, c, 0x50D7CE24, o + 194);
    XF32(ctx, c, 0x0028C84A, o + 56);
    XF32(ctx, c, 0xC6A387AD, o + 188);
    XS32(ctx, c, 0x31781726, o + 196);
    XVec3(ctx, c, 0x0418FA8A, o + 144);
    XVec3(ctx, c, 0x0418FA8B, o + 160);
    if (load<u8>(o + 193) == 0) return ret(ctx, 0);
    const f32 v = F32(load<u32>(o + 48)), m = F32(load<u32>(kMin));
    ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(o + 48)));
    ctx.fpr[2] = std::bit_cast<f64>(LfsBits(load<u32>(kMin)));
    if (!(v < m)) return ret(ctx, kMin);
    const u32 path = static_cast<u32>(call(ctx, m8fn::XmlCursor_DocPath, {static_cast<u32>(call(ctx, Xml_FindResolved_004F9434, {c}))}));
    const u32 line = load<u32>(static_cast<u32>(call(ctx, Xml_FindResolved_004F9434, {c})) + 36) + 1;
    ret(ctx, call(ctx, mid1::m1fn::String_Format, {kFmt, path, static_cast<u64>(static_cast<s64>(static_cast<s32>(line)))}));
}
