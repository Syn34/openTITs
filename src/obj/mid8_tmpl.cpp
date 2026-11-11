// Game code, medium sweep part 8: component template readers (the
// 0x002879AC family, see mid7_tmpl.cpp for the shared prologue). Each
// returns the template, or 0 without a "name" attribute.
// Quirk (all): when SmallAlloc fails, the flag is written to address 16
// and the fields are read and written on p = 0.
#include "mid8.h"

using namespace sly4;
using namespace sly4::mid8;

// Tmpl* TmplRead_0028E624(a0, XmlCursor* c, u8 flag): kind 21, 2904 bytes,
// vtable 0x00B1C6C8: +20 the count of up to 8 groups of 360 bytes at +24
// ({char name[32], +32 name hash, +36 entry count, +40 up to 8 entries of
// 40 bytes {char name[32], +32 name hash, +36 hash 0xC7E35FE2}}), from the
// 0x040EFE5F children and their 0x8B5D7C75 children. A ninth group or
// entry, or one without a name, gets XmlCursor_DocPath(a0) and is skipped.
DECOMP(0028E624, TmplRead_0028E624) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    constexpr u32 kGroup = 0x040EFE5F, kEntry = 0x8B5D7C75, kEntryHash = 0xC7E35FE2;
    u32 p;
    if (!TmplBegin(ctx, a0, c, 2904, 21, true, p)) return ret(ctx, 0);
    if (p != 0) {
        store<u32>(p, 0x00B1C6C8);
        store<u32>(p + 20, 0);
        for (u32 k = 0; k < 8; ++k) {
            const u32 g = p + 24 + 360 * k;
            store<u32>(g + 32, 0);
            store<u32>(g + 36, 0);
            store<u8>(g, 0);
            call(ctx, m8fn::Memset, {g + 40, 0, 320});
        }
    }
    store<u8>(p + 16, flag);
    u32 n = 0;
    u32 g = p + 24;
    for (u32 node = static_cast<u32>(call(ctx, m8fn::XmlCursor_FirstChild, {c, kGroup})); node != 0;
         node = static_cast<u32>(call(ctx, m8fn::XmlCursor_NextSibling, {node, kGroup}))) {
        if (n >= 8) {
            call(ctx, m8fn::XmlCursor_DocPath, {a0});
            continue;
        }
        if (!NameAndHash(ctx, a0, node, g, g + 32)) continue;
        u32 m = 0;
        u32 e = g + 40;
        for (u32 sub = static_cast<u32>(call(ctx, m8fn::XmlCursor_FirstChild, {node, kEntry})); sub != 0;
             sub = static_cast<u32>(call(ctx, m8fn::XmlCursor_NextSibling, {sub, kEntry}))) {
            if (m >= 8) {
                call(ctx, m8fn::XmlCursor_DocPath, {a0});
                continue;
            }
            if (!NameAndHash(ctx, a0, sub, e, e + 32)) continue;
            call(ctx, xfn::XmlCursor_ReadHash, {sub, kEntryHash, e + 36, 0});
            e += 40;
            ++m;
        }
        g += 360;
        ++n;
        store<u32>(g - 360 + 36, m);
    }
    store<u32>(p + 20, n);
    ret(ctx, p);
}

namespace {

// The hash attribute 0x0024728B of the node (default 0, through the stack
// word tmp; the word is taken whether or not the attribute was there).
u32 ChildHash(PPUContext& ctx, u32 node, u32 tmp) {
    call(ctx, xfn::XmlCursor_ReadHash, {node, 0x0024728B, tmp, 0});
    return load<u32>(tmp);
}

// Up to max ChildHash values of the `hash` children of c into dst[].
void ChildHashes(PPUContext& ctx, u32 c, u32 hash, u32 dst, u32 max, u32 tmp) {
    u32 n = 0;
    for (u32 node = static_cast<u32>(call(ctx, m8fn::XmlCursor_FirstChild, {c, hash})); node != 0 && n < max;
         node = static_cast<u32>(call(ctx, m8fn::XmlCursor_NextSibling, {node, hash}))) {
        const u32 v = ChildHash(ctx, node, tmp);
        store<u32>(dst + 4 * n, v);
        ++n;
    }
}

}  // namespace

// Tmpl* TmplRead_00291AD0(a0, c, flag): kind 17, 96 bytes, vtable
// 0x00B1C7D0, all zero: the ChildHash values of up to 8 0x75215015
// children (+20), 8 0x6AD11467 children (+52), 2 0x895ACE01 children
// (+84) and of the first 0x5BDDD9A5 child (+92).
DECOMP(00291AD0, TmplRead_00291AD0) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    Stack frame(ctx, 16);
    const u32 tmp = frame.addr();
    u32 p;
    if (!TmplBegin(ctx, a0, c, 96, 17, true, p)) return ret(ctx, 0);
    if (p != 0) {
        store<u32>(p, 0x00B1C7D0);
        call(ctx, m8fn::Memset, {p + 20, 0, 32});
        for (u32 k = 0; k < 8; ++k) store<u8>(p + 84 + k, 0);
    }
    store<u8>(p + 16, flag);
    ChildHashes(ctx, c, 0x75215015, p + 20, 8, tmp);
    ChildHashes(ctx, c, 0x6AD11467, p + 52, 8, tmp);
    ChildHashes(ctx, c, 0x895ACE01, p + 84, 2, tmp);
    const u32 node = static_cast<u32>(call(ctx, m8fn::XmlCursor_FirstChild, {c, 0x5BDDD9A5}));
    if (node != 0) store<u32>(p + 92, ChildHash(ctx, node, tmp));
    ret(ctx, p);
}

// Tmpl* TmplRead_002B44AC(a0, c, flag): kind 46, 44 bytes, vtable
// 0x00B1CD48: the vector +20 (20-byte records, capacity 32) of the
// 0x7480D3BA children with a non-zero hash 0x000107E5 ({+0 the index (0 /
// 1) of the hash 0x0027873A in the table at 0x00AD538C (the last match, 0
// when none), +4 that hash, +8 hash 0x6D4342F5 (0), +12 f32 0x7307C873
// (0), +16 bool 0x83CA0519 (true)}, 17 bytes copied); then the f32
// 0x7307C873 of c at +40 (default 0).
DECOMP(002B44AC, TmplRead_002B44AC) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    constexpr u32 kChild = 0x7480D3BA, kTable = 0x00AD538C;
    Stack frame(ctx, 32);
    const u32 tmp = frame.addr(), rec = tmp + 4;
    u32 p;
    if (!TmplBegin(ctx, a0, c, 44, 46, true, p)) return ret(ctx, 0);
    if (p != 0) {
        store<u32>(p, 0x00B1CD48);
        mid1::PtrVecNew(ctx, p + 20, 32, 20, 1);
        store<u32>(p + 40, 0);
    }
    store<u8>(p + 16, flag);
    const u32 v = p + 20;
    for (u32 node = static_cast<u32>(call(ctx, m8fn::XmlCursor_FirstChild, {c, kChild})); node != 0;
         node = static_cast<u32>(call(ctx, m8fn::XmlCursor_NextSibling, {node, kChild}))) {
        for (u32 k = 0; k < 4; ++k) store<u32>(rec + 4 * k, 0);
        store<u8>(rec + 16, 1);
        call(ctx, xfn::XmlCursor_ReadHash, {node, 0x000107E5, rec + 4, 0});
        XHash(ctx, node, 0x6D4342F5, rec + 8);
        XF32(ctx, node, 0x7307C873, rec + 12);
        XBool(ctx, node, 0x83CA0519, rec + 16);
        store<u32>(tmp, 0);
        call(ctx, xfn::XmlCursor_ReadHash, {node, 0x0027873A, tmp, 0});
        const u32 kind = load<u32>(tmp), h = load<u32>(rec + 4);
        u32 idx = 0;
        for (u32 i = 0; i < 2; ++i)
            if (kind == load<u32>(kTable + 4 * i)) idx = i;
        store<u32>(rec, idx);
        if (h == 0) continue;
        u32 n = load<u16>(v + 8);
        const u32 cap = load<u16>(v + 4);
        if (n == cap) {
            call(ctx, mid1::m1fn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
            n = load<u16>(v + 8);
        }
        call(ctx, m8fn::Memmove, {load<u32>(v) + n * 20, rec, 17});
        const u32 n1 = load<u16>(v + 8) + 1, hw = load<u16>(v + 10);
        store<u16>(v + 8, static_cast<u16>(n1));
        store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFF) ? hw : n1));
    }
    XF32(ctx, c, 0x7307C873, p + 40);
    ret(ctx, p);
}

// Tmpl* TmplRead_002B9C74(a0, c, flag): kind 29, 304 bytes, vtable
// 0x00B1CEC0: hashes +20 (0xE2E902C9) / +24 (0x85D98325), f32 +28 (1),
// +32 / +36 (0), bools +40 / +41 (true); +44 the count of up to 8 32-byte
// records at +48 from the child elements ({+0 the element's hash (+20, 0
// unless bit 0x80 of its +24 byte), +4 hash 0x0024728B (0), +16 vec3
// 0x8ACA5893 (default the zero vector at 0x01164B40)}).
DECOMP(002B9C74, TmplRead_002B9C74) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    u32 p;
    if (!TmplBegin(ctx, a0, c, 304, 29, true, p)) return ret(ctx, 0);
    if (p != 0) {
        store<u32>(p + 24, 0x85D98325);
        store<u32>(p + 28, 0x3F800000);
        store<u32>(p, 0x00B1CEC0);
        store<u32>(p + 20, 0xE2E902C9);
        store<u32>(p + 32, 0);
        store<u32>(p + 36, 0);
        store<u8>(p + 40, 1);
        store<u8>(p + 41, 1);
    }
    store<u8>(p + 16, flag);
    store<u32>(p + 44, 0);
    XHash(ctx, c, 0x784848DF, p + 20);
    XHash(ctx, c, 0xF2006FF0, p + 24);
    XF32(ctx, c, 0x04B70827, p + 28);
    XF32(ctx, c, 0x7689C1C5, p + 32);
    XF32(ctx, c, 0x7689C0D7, p + 36);
    XBool(ctx, c, 0x86BE6BD3, p + 40);
    XBool(ctx, c, 0xE9B00400, p + 41);
    for (u32 node = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c})); node != 0;
         node = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {node}))) {
        const u32 n = load<u32>(p + 44);
        if (static_cast<s32>(n) >= 8) continue;
        store<u32>(p + 48 + 32 * n, (load<u8>(node + 24) & 0x80) != 0 ? load<u32>(node + 20) : 0);
        call(ctx, xfn::XmlCursor_ReadHash, {node, 0x0024728B, p + 52 + 32 * load<u32>(p + 44), 0});
        XRead(ctx, xfn::XmlCursor_ReadVec3, node, 0x8ACA5893, p + 64 + 32 * load<u32>(p + 44), 0x01164B40);
        store<u32>(p + 44, load<u32>(p + 44) + 1);
    }
    ret(ctx, p);
}

// Tmpl* TmplRead_00283D58(a0, c, flag): kind 15, 128 bytes, vtable
// 0x00B1C568: the flags word +32 (bit 31 bool 0x002480E8, bit 30 bool
// 0x724F4DDA, bits 22..29 s32 0xA8F0945A; defaults 0, 0, 2), f32 +36
// (0.05), +40 / +44 (0.1), +48 (30), vec3 +64 ({0, -9.81, 0}) and +80
// (the zero vector at 0x01164B40); the vector +96 (capacity 4) of the hash
// 0xA424A34F of each 0xA0702B13 child (a child without it gets
// XmlCursor_DocPath(a0)).
DECOMP(00283D58, TmplRead_00283D58) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    constexpr u32 kChild = 0xA0702B13;
    Stack frame(ctx, 16);
    const u32 t112 = frame.addr(), t116 = t112 + 4, t120 = t112 + 8;
    u32 p;
    if (!TmplBegin(ctx, a0, c, 128, 15, true, p)) return ret(ctx, 0);
    if (p != 0) {
        const u32 w = (load<u32>(p + 32) & 0x003FFFFFu) | (2u << 22);
        store<u32>(p, 0x00B1C568);
        store<u32>(p + 36, 0x3D4CCCCD);
        store<u32>(p + 64, 0);
        store<u32>(p + 40, 0x3DCCCCCD);
        store<u32>(p + 44, 0x3DCCCCCD);
        store<u32>(p + 32, w);
        store<u32>(p + 48, 0x41F00000);
        store<u32>(p + 72, 0);
        store<u32>(p + 68, 0xC11CF5C3);
        for (u32 k = 0; k < 3; ++k) store<u32>(p + 80 + 4 * k, load<u32>(0x01164B40 + 4 * k));  // lfs / stfs
        call(ctx, mid1::m1fn::PtrVector_Construct, {p + 96});
        store<u16>(p + 102, 4);
    }
    store<u8>(p + 16, flag);
    mid5::XBit<u32>(ctx, c, 0x002480E8, p + 32, 0x80000000u, t120);
    mid5::XBit<u32>(ctx, c, 0x724F4DDA, p + 32, 0x40000000u, t120);
    call(ctx, xfn::XmlCursor_ReadS32, {c, 0xA8F0945A, t112, (load<u32>(p + 32) >> 22) & 0xFFu});
    store<u32>(p + 32, (load<u32>(p + 32) & ~0x3FC00000u) | ((load<u32>(t112) & 0xFFu) << 22));
    XF32(ctx, c, 0x922FF60D, p + 36);
    XF32(ctx, c, 0xE42E17D0, p + 40);
    XF32(ctx, c, 0xE392DBA9, p + 44);
    XF32(ctx, c, 0xFBCA07B1, p + 48);
    XVec3(ctx, c, 0x3B83FE2E, p + 64);
    XVec3(ctx, c, 0x34BF0B12, p + 80);
    for (u32 node = static_cast<u32>(call(ctx, m8fn::XmlCursor_FirstChild, {c, kChild})); node != 0;
         node = static_cast<u32>(call(ctx, m8fn::XmlCursor_NextSibling, {node, kChild}))) {
        if (call(ctx, xfn::XmlCursor_ReadHash, {node, 0xA424A34F, t116, 0}) != 0) {
            call(ctx, m8fn::XmlCursor_DocPath, {a0});
            continue;
        }
        mid1::PtrVecPush(ctx, p + 96, load<u32>(t116));
    }
    ret(ctx, p);
}

// Tmpl* TmplRead_0026F2D0(a0, c, flag): kind 1, 224 bytes, vtable
// 0x00B13C74: six 24-byte records at +20 ({s32 +0 0, f32 +4 0, +8 90,
// +12 -FLT_MAX, vec2 +12 / +16 FLT_MAX, f32 +20 0.5}), each read from the
// child element whose hash (+20, 0 unless bit 0x80 of its +24 byte) is
// the matching one of the six at 0x00AD4D34 (other children get
// XmlCursor_DocPath(a0)); bools +164 / +165 / +166 / +208, hashes +168 /
// +172, vec3 +176 / +192 (the zero vector at 0x01164B40).
DECOMP(0026F2D0, TmplRead_0026F2D0) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    constexpr u32 kTable = 0x00AD4D34, kZero = 0x01164B40;
    u32 p;
    if (!TmplBegin(ctx, a0, c, 224, 1, true, p)) return ret(ctx, 0);
    if (p != 0) {
        store<u32>(p, 0x00B13C74);
        for (u32 k = 0; k < 6; ++k) {
            const u32 q = p + 20 + 24 * k;
            store<u32>(q, 0);
            store<u32>(q + 4, 0);
            store<u32>(q + 8, 0x42B40000);
            store<u32>(q + 12, 0xFF7FFFFF);
            store<u32>(q + 16, 0x7F7FFFFF);
            store<u32>(q + 20, 0x3F000000);
        }
        store<u8>(p + 164, 0);
        store<u8>(p + 165, 0);
        store<u8>(p + 166, 0);
        store<u32>(p + 168, 0);
        for (u32 k = 0; k < 3; ++k) store<u32>(p + 176 + 4 * k, load<u32>(kZero + 4 * k));  // lfs / stfs
        for (u32 k = 0; k < 3; ++k) store<u32>(p + 192 + 4 * k, load<u32>(kZero + 4 * k));
        ctx.fpr[1] = std::bit_cast<f64>(LfsBits(load<u32>(kZero + 8)));  // the copy went through f1
    }
    store<u8>(p + 16, flag);
    XBool(ctx, c, 0x91CB25B2, p + 164);
    XBool(ctx, c, 0x65AF9A49, p + 165);
    XHash(ctx, c, 0xA424A34F, p + 168);
    XHash(ctx, c, 0x75849CFE, p + 172);
    XVec3(ctx, c, 0x313AB0C1, p + 176);
    XVec3(ctx, c, 0xC333F453, p + 192);
    XBool(ctx, c, 0x69EC8F92, p + 208);
    XBool(ctx, c, 0xA0D5174C, p + 166);
    for (u32 node = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c})); node != 0;
         node = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {node}))) {
        const u32 h = (load<u8>(node + 24) & 0x80) != 0 ? load<u32>(node + 20) : 0;
        u32 i = 0;
        while (i < 6 && h != load<u32>(kTable + 4 * i)) ++i;
        if (i >= 6) {
            call(ctx, m8fn::XmlCursor_DocPath, {a0});
            continue;
        }
        const u32 q = p + 24 * i;
        XS32(ctx, node, 0xA3F9EF1E, q + 20);
        XF32(ctx, node, 0x8FA16092, q + 24);
        XF32(ctx, node, 0x03B880B3, q + 28);
        XVec2(ctx, node, 0x9C819064, q + 32);
        XF32(ctx, node, 0xF5DFB24A, q + 40);
    }
    ret(ctx, p);
}

// Tmpl* TmplRead_002B8328(a0, c, flag): kind 28, 64 bytes, vtable
// 0x00B1CE68: the vector +20 of 144-byte records (pushed by
// Vec144_Push75FF20) from the 0xA7E419C3 children ({+0 hash 0x0024728B
// (0), +4 hash 0x00255BF3 (0), +8 -1, +12 0, +16 vec3 0x00013DB7 (zero;
// an angle, scaled by the f32 at 0x002B8320), +32 vec3 0x00013634
// (zero), +48 0, +64 / +112 the zero vector, +80 the quaternion at
// 0x01164C90, +96 / +100 / +128 0; the other bytes are left as the stack
// had them), the vector +40 (capacity 12, at most 12) of the non-zero
// hash 0x79F40122 of the 0xD5B5E11B children; bool 0x4BEF99EC +60.
DECOMP(002B8328, TmplRead_002B8328) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    constexpr u32 kZero = 0x01164B40, kQuat = 0x01164C90, kScale = 0x002B8320;
    constexpr u32 Vec144_Push75FF20 = 0x0075FF20;
    Stack frame(ctx, 160);
    const u32 rec = frame.addr(), t256 = rec + 144;
    u32 p;
    if (!TmplBegin(ctx, a0, c, 64, 28, true, p)) {
        p = 0;
    } else {
        if (p != 0) {
            store<u32>(p, 0x00B1CE68);
            call(ctx, mid1::m1fn::PtrVector_Construct, {p + 20});
            store<u16>(p + 26, 144);
            mid1::PtrVecNew(ctx, p + 40, 12, 4, 1);
            store<u8>(p + 60, 0);
        }
        store<u8>(p + 16, flag);
        const f32 k = F32(load<u32>(kScale));
        for (u32 node = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c})); node != 0;
             node = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {node}))) {
            if ((load<u8>(node + 24) & 0x80) == 0) continue;
            const u32 h = load<u32>(node + 20);
            if (h == 0xA7E419C3) {
                store<u32>(rec, 0);
                store<u32>(rec + 4, 0);
                store<u32>(rec + 8, 0xFFFFFFFF);
                store<u32>(rec + 12, 0);
                for (u32 off : {16u, 32u, 64u, 112u})
                    for (u32 j = 0; j < 3; ++j) store<u32>(rec + off + 4 * j, load<u32>(kZero + 4 * j));
                store<u32>(rec + 48, 0);
                for (u32 j = 0; j < 4; ++j) store<u32>(rec + 80 + 4 * j, load<u32>(kQuat + 4 * j));
                store<u32>(rec + 96, 0);
                store<u32>(rec + 100, 0);
                store<u32>(rec + 128, 0);
                call(ctx, xfn::XmlCursor_ReadHash, {node, 0x0024728B, rec, 0});
                XHash(ctx, node, 0x00255BF3, rec + 4);
                XVec3(ctx, node, 0x00013634, rec + 32);
                XVec3(ctx, node, 0x00013DB7, rec + 16);
                f32 v[3];
                for (u32 j = 0; j < 3; ++j) v[j] = F32(load<u32>(rec + 16 + 4 * j)) * k;
                for (u32 j = 0; j < 3; ++j) store<u32>(rec + 16 + 4 * j, W32(v[j]));
                ctx.fpr[1] = v[0];
                call(ctx, Vec144_Push75FF20, {p + 20, rec});
            } else if (h == 0xD5B5E11B) {
                store<u32>(t256, 0);
                call(ctx, xfn::XmlCursor_ReadHash, {node, 0x79F40122, t256, 0});
                if (load<u32>(t256) == 0 || load<u16>(p + 48) >= 12) continue;
                mid1::PtrVecPush(ctx, p + 40, load<u32>(t256));
            }
        }
    }
    XBool(ctx, c, 0x4BEF99EC, p + 60);
    ret(ctx, p);
}

// Entry* TmplEntry_Read265D60(?, XmlCursor* node) (reads r4 only): a
// 64-byte entry (SmallAlloc; when that succeeded: +0 the quaternion at
// 0x01164C90, +16 the zero vector at 0x01164B40, +32..+56 0). With a
// "cloneFrom" attribute naming another entry (found in the entry map
// 0x0105BE30 by its String_HashNoCase, not this one), that entry's fields
// are copied. "inventoryGroupDefName": +36 = the value found in the map
// 0x0105BE18 for the hash 0x429417ED. Then the euler angles 0x53D63D93
// (degrees; constants at 0x00265D54) as the quaternion +0, vec3
// 0x2AC4FB37 +16, hashes +32 / +48 / +40 / +44, s32 +52, bool +56; with a
// "name" attribute, {name hash, entry} is appended to the entry map.
// Quirk: when SmallAlloc fails, everything is read and written on p = 0.
DECOMP(00265D60, TmplEntry_Read265D60) {
    const u32 node = arg<u32>(ctx, 1);
    constexpr u32 kEntryMap = 0x0105BE30, kGroupMap = 0x0105BE18;
    constexpr u32 kStrCloneFrom = 0x00AD4C14, kStrGroupDef = 0x00AD4C94, kAngles = 0x00265D54;
    constexpr u32 kZero = 0x01164B40, kQuat = 0x01164C90;
    constexpr u32 SortedMap_Find75FC00 = 0x0075FC00, SortedMap_Find75FD88 = 0x0075FD88;
    Stack frame(ctx, 32);
    const u32 ang = frame.addr(), t128 = ang + 16, t132 = ang + 20;
    u32 p = static_cast<u32>(call(ctx, m7fn::SmallAlloc, {64}));
    if (p != 0) {
        for (u32 k = 0; k < 4; ++k) store<u32>(p + 4 * k, load<u32>(kQuat + 4 * k));  // lfs / stfs
        store<u32>(p + 16, load<u32>(kZero));
        store<u32>(p + 32, 0);
        store<u32>(p + 20, load<u32>(kZero + 4));
        store<u32>(p + 36, 0);
        store<u32>(p + 24, load<u32>(kZero + 8));
        for (u32 off : {40u, 44u, 48u, 52u}) store<u32>(p + off, 0);
        store<u8>(p + 56, 0);
    }
    u32 s = static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {node, kStrCloneFrom}));
    if (s != 0) {
        store<u32>(t128, static_cast<u32>(call(ctx, m8fn::String_HashNoCase, {s})));
        const u32 src = load<u32>(static_cast<u32>(call(ctx, SortedMap_Find75FD88, {kEntryMap, t128})));
        if (src != 0 && src != p) {
            // the copy order of the original (lfs / stfs and lwz / stw pairs)
            for (u32 off : {0u, 4u, 8u, 32u, 12u, 16u, 40u, 20u, 44u, 24u, 36u, 48u, 52u}) store<u32>(p + off, load<u32>(src + off));
            store<u8>(p + 56, load<u8>(src + 56));
        }
    }
    s = static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {node, kStrGroupDef}));
    if (s != 0) {
        store<u32>(t132, 0);
        call(ctx, xfn::XmlCursor_ReadHash, {node, 0x429417ED, t132, 0});
        store<u32>(p + 36, load<u32>(static_cast<u32>(call(ctx, SortedMap_Find75FC00, {kGroupMap, t132}))));
    }
    XRead(ctx, xfn::XmlCursor_ReadVec3, node, 0x53D63D93, ang, kZero);
    const mid7::EulerQuat q = mid7::EulerToQuat(ctx, ang, kAngles);
    store<u32>(p + 12, W32(q.w));
    store<u32>(p, W32(q.x));
    store<u32>(p + 8, W32(q.z));
    store<u32>(p + 4, W32(q.y));
    ctx.fpr[1] = q.y;
    XVec3(ctx, node, 0x2AC4FB37, p + 16);
    XHash(ctx, node, 0x016126A2, p + 32);
    XHash(ctx, node, 0x13163268, p + 48);
    XHash(ctx, node, 0x5F5A6881, p + 40);
    XHash(ctx, node, 0xCDA698D9, p + 44);
    XS32(ctx, node, 0xE92E69A4, p + 52);
    XBool(ctx, node, 0xFA32EEBF, p + 56);
    s = static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {node, kStrName}));
    if (s != 0) {
        const u32 h = static_cast<u32>(call(ctx, m8fn::String_HashNoCase, {s}));
        RegPushKV(ctx, kEntryMap, h, p);
    }
    ret(ctx, p);
}

// Tmpl* TmplRead_00299A60(a0, c, flag): kind 24, 96 bytes, vtable
// 0x00B1C8C8; no XmlCursor_DocPath without a name. The u32 vector +20
// (capacity 4) gets, from the first 0x5F746274 child (only while it is
// empty), nine hashes read by name (the names at 0x00F64BF0, default
// 0x7DDC7E2D); the 8-byte vector +40 (capacity 8) {hash 0x000107E5, hash
// 0x7BA7CDEF} of each 0xDE88911C child. Hashes +60 / +64 / +68 / +76 /
// +80 (0), f32 +72 (1), +84 / +88 (the f32 at 0x01057968 / 0x010579B4);
// +92 = 1 when the bool 0xC09B3793 is set.
DECOMP(00299A60, TmplRead_00299A60) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    constexpr u32 XmlCursor_ReadHashByName = 0x004FA6E4, kNames = 0x00F64BF0;
    Stack frame(ctx, 16);
    const u32 t112 = frame.addr(), t116 = t112 + 4, t120 = t112 + 8, t124 = t112 + 12;
    u32 p;
    if (!TmplBegin(ctx, a0, c, 96, 24, false, p, 1, 1)) return ret(ctx, 0);
    if (p != 0) {
        store<u32>(p, 0x00B1C8C8);
        call(ctx, mid1::m1fn::PtrVector_Construct, {p + 20});
        store<u16>(p + 26, 4);
        call(ctx, mid1::m1fn::PtrVector_Construct, {p + 40});
        store<u32>(p + 72, 0x3F800000);
        store<u32>(p + 60, 0);
        store<u16>(p + 46, 8);
        store<u32>(p + 64, 0);
        store<u32>(p + 68, 0);
        store<u32>(p + 76, 0);
        store<u32>(p + 84, load<u32>(0x01057968));  // lfs / stfs
        store<u32>(p + 80, 0);
        store<u32>(p + 92, 0);
        store<u32>(p + 88, load<u32>(0x010579B4));
    }
    store<u8>(p + 16, flag);
    XHash(ctx, c, 0x620515DB, p + 60);
    XHash(ctx, c, 0x5D6F9F1D, p + 64);
    XHash(ctx, c, 0x7C4EBDF7, p + 68);
    XF32(ctx, c, 0xDEB00F78, p + 72);
    XHash(ctx, c, 0xC9796122, p + 76);
    XHash(ctx, c, 0xA7FC59B2, p + 80);
    XF32(ctx, c, 0x80244A9A, p + 84);
    XF32(ctx, c, 0x1867714F, p + 88);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0xC09B3793, t124, 0);
    if (load<u8>(t124) != 0) store<u32>(p + 92, 1);
    for (u32 node = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c})); node != 0;
         node = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {node}))) {
        const u32 h = (load<u8>(node + 24) & 0x80) != 0 ? load<u32>(node + 20) : 0;
        if (h == 0x5F746274) {
            if (load<u16>(p + 28) != 0) continue;
            for (u32 i = 0; i < 9; ++i) {
                store<u32>(t112, 0x7DDC7E2D);
                call(ctx, XmlCursor_ReadHashByName, {node, load<u32>(kNames + 4 * i), t112, 0x7DDC7E2D});
                mid1::PtrVecPush(ctx, p + 20, load<u32>(t112));
            }
        } else if (h == 0xDE88911C) {
            store<u32>(t116, 0);
            store<u32>(t120, 0);
            call(ctx, xfn::XmlCursor_ReadHash, {node, 0x000107E5, t116, 0});
            XHash(ctx, node, 0x7BA7CDEF, t120);
            const u32 v = p + 40;
            u32 n = load<u16>(v + 8);
            const u32 cap = load<u16>(v + 4);
            if (n == cap) {
                call(ctx, mid1::m1fn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
                n = load<u16>(v + 8);
            }
            const u32 e = load<u32>(v) + ((n << 3) & 0x7FFF8u);
            const u32 n1 = load<u16>(v + 8) + 1;
            store<u32>(e, load<u32>(t116));
            store<u16>(v + 8, static_cast<u16>(n1));
            store<u32>(e + 4, load<u32>(t120));
            const u32 hw = load<u16>(v + 10);
            store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFF) ? hw : n1));
        }
    }
    ret(ctx, p);
}

// Tmpl* TmplRead_002BC208(a0, c, flag): kind 36, 304 bytes, vtable
// 0x00B1CFC8 (EntityCompWheelchair): s32 +20 0xF4EF8B5F, hashes +24 / +28
// / +32 / +36 (0); up to two 0x0000FE3C arm children k, from whose child
// elements: 0x002678E2 the arm root (hash 0x0024728B at +40 + 4k; a
// second one prints "multiple Arm Root"), up to 16 0x001F32C4 IK bones
// (hash 0x0024728B, +48 + 64k), up to 8 0x04B529B7 slaves ({hash
// 0x0024728B, hash 0x8C3DFCAA}, +176 + 64k); the overflow messages go
// through String_Format with the template's name.
DECOMP(002BC208, TmplRead_002BC208) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    constexpr u32 kArm = 0x0000FE3C, kId = 0x0024728B;
    constexpr u32 kFmtArmRoot = 0x00AD53DC, kFmtBones = 0x00AD541C, kFmtSlaves = 0x00AD546C;
    // TmplBegin's prologue, written out to keep the name for the messages.
    u32 p;
    const u32 name = static_cast<u32>(call(ctx, m8fn::XmlCursor_FindAttrByName, {c, kStrName}));
    if (name == 0) {
        call(ctx, m8fn::XmlCursor_DocPath, {a0});
        return ret(ctx, 0);
    }
    p = static_cast<u32>(call(ctx, m7fn::SmallAlloc, {304}));
    if (p != 0) {
        store<u32>(p, kTmplVtBase);
        const u32 h = static_cast<u32>(call(ctx, m8fn::String_HashNoCase, {name}));
        store<u32>(p + 8, 0);
        store<u8>(p + 16, 0);
        store<u32>(p + 4, h);
        store<u32>(p + 12, 36);
        TmplRegister(ctx, p, 0);
        store<u32>(p, 0x00B1CFC8);
        store<u32>(p + 20, 0);
        store<u32>(p + 24, 0);
        for (u32 k = 0; k < 8; ++k) store<u8>(p + 28 + k, 0);
        for (u32 k = 0; k < 8; ++k) store<u8>(p + 40 + k, 0);
        call(ctx, m8fn::Memset, {p + 48, 0, 128});
    }
    store<u8>(p + 16, flag);
    XS32(ctx, c, 0xF4EF8B5F, p + 20);
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xF4E736E9, p + 24, 0});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x7F6B2FAD, p + 28, 0});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x7F6B2FB3, p + 32, 0});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xCF9ADB4C, p + 36, 0});
    u32 k = 0;
    for (u32 arm = static_cast<u32>(call(ctx, m8fn::XmlCursor_FirstChild, {c, kArm})); arm != 0;
         arm = static_cast<u32>(call(ctx, m8fn::XmlCursor_NextSibling, {arm, kArm})), ++k) {
        if (k >= 2) break;
        const u32 root = p + 40 + 4 * k;
        u32 nslaves = 0, nbones = 0;
        for (u32 node = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {arm})); node != 0;
             node = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {node}))) {
            if ((load<u8>(node + 24) & 0x80) == 0) continue;
            const u32 h = load<u32>(node + 20);
            if (h == 0x04B529B7) {
                if (nslaves >= 8) {
                    call(ctx, mid1::m1fn::String_Format, {kFmtSlaves, name, 8});
                    continue;
                }
                const u32 s = p + 176 + 64 * k + 8 * nslaves;
                call(ctx, xfn::XmlCursor_ReadHash, {node, kId, s, 0});
                call(ctx, xfn::XmlCursor_ReadHash, {node, 0x8C3DFCAA, s + 4, 0});
                ++nslaves;
            } else if (h == 0x002678E2) {
                if (load<u32>(root) != 0) call(ctx, mid1::m1fn::String_Format, {kFmtArmRoot, name});
                call(ctx, xfn::XmlCursor_ReadHash, {node, kId, root, 0});
            } else if (h == 0x001F32C4) {
                if (nbones >= 16) {
                    call(ctx, mid1::m1fn::String_Format, {kFmtBones, name, 16});
                    continue;
                }
                call(ctx, xfn::XmlCursor_ReadHash, {node, kId, p + 48 + 64 * k + 4 * nbones, 0});
                ++nbones;
            }
        }
    }
    ret(ctx, p);
}

// Tmpl* TmplRead_00286378(a0, c, flag): kind 16, 40 bytes, vtable
// 0x00B1C5C0: bool 0x2A9B2CB7 (+17, default 0); the vector +20 of 60-byte
// records from the 0x00010E11 children ({+0 hash 0x0024728B, +4
// 0x40795527, +8 0xE79D4302, +12 0x001F32C4, +20 0x1A1D3D81, +16
// 0x980FFA7A, +24 0x98247D0B (all default 0), bool +28 0x2B501799 and,
// when set, f32 +32 / +36 / +40 (0xD9DF2696 / 0xD9DF25A8 / 0xD212F8D0,
// default the f32 at 0x00286374), f32 +44..+56 (0.5)}). A missing
// attribute (all but 0xE79D4302 and the last four) gets
// XmlCursor_DocPath(a0); the record is pushed anyway.
// Quirk: bytes +30 / +31 of each record are the stack bytes the original
// never wrote.
DECOMP(00286378, TmplRead_00286378) {
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u8 flag = static_cast<u8>(ctx.gpr[5]);
    constexpr u32 kChild = 0x00010E11, kDef = 0x00286374;
    Stack frame(ctx, 64);
    const u32 rec = frame.addr();  // the original's r1 + 112
    u32 p;
    if (!TmplBegin(ctx, a0, c, 40, 16, true, p)) return ret(ctx, 0);
    if (p != 0) {
        store<u32>(p, 0x00B1C5C0);
        store<u8>(p + 17, 0);
        call(ctx, mid1::m1fn::PtrVector_Construct, {p + 20});
        store<u16>(p + 26, 60);
    }
    store<u8>(p + 16, flag);
    XRead(ctx, xfn::XmlCursor_ReadBool, c, 0x2A9B2CB7, p + 17, 0);
    auto need = [&](u32 r) {
        if (r != 0) call(ctx, m8fn::XmlCursor_DocPath, {a0});
    };
    const u32 v = p + 20;
    for (u32 node = static_cast<u32>(call(ctx, m8fn::XmlCursor_FirstChild, {c, kChild})); node != 0;
         node = static_cast<u32>(call(ctx, m8fn::XmlCursor_NextSibling, {node, kChild}))) {
        for (u32 off = 0; off < 28; off += 4) store<u32>(rec + off, 0);
        store<u8>(rec + 28, 0);
        store<u8>(rec + 29, 0);
        for (u32 off = 32; off < 44; off += 4) store<u32>(rec + off, 0);
        for (u32 off = 44; off < 60; off += 4) store<u32>(rec + off, 0x3F000000);
        need(XRead(ctx, xfn::XmlCursor_ReadHash, node, 0x0024728B, rec, 0));
        need(XRead(ctx, xfn::XmlCursor_ReadHash, node, 0x40795527, rec + 4, 0));
        XRead(ctx, xfn::XmlCursor_ReadHash, node, 0xE79D4302, rec + 8, 0);
        need(XRead(ctx, xfn::XmlCursor_ReadHash, node, 0x001F32C4, rec + 12, 0));
        need(XRead(ctx, xfn::XmlCursor_ReadHash, node, 0x1A1D3D81, rec + 20, 0));
        need(XRead(ctx, xfn::XmlCursor_ReadHash, node, 0x980FFA7A, rec + 16, 0));
        need(XRead(ctx, xfn::XmlCursor_ReadHash, node, 0x98247D0B, rec + 24, 0));
        need(XRead(ctx, xfn::XmlCursor_ReadBool, node, 0x2B501799, rec + 28, 0));
        if (load<u8>(rec + 28) != 0) {
            need(XF32Def(ctx, node, 0xD9DF2696, rec + 32, kDef));
            need(XF32Def(ctx, node, 0xD9DF25A8, rec + 36, kDef));
            need(XF32Def(ctx, node, 0xD212F8D0, rec + 40, kDef));
        }
        XF32(ctx, node, 0xC4E89AA8, rec + 44);
        XF32(ctx, node, 0xC4E89AA9, rec + 48);
        XF32(ctx, node, 0x1B750329, rec + 52);
        XF32(ctx, node, 0x1B75032A, rec + 56);
        u32 n = load<u16>(v + 8);
        const u32 cap = load<u16>(v + 4);
        if (n == cap) {
            call(ctx, mid1::m1fn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
            n = load<u16>(v + 8);
        }
        const u32 e = load<u32>(v) + n * 60;
        for (u32 off = 0; off < 60; off += 4) store<u32>(e + off, load<u32>(rec + off));
        const u32 n1 = load<u16>(v + 8) + 1, hw = load<u16>(v + 10);
        store<u16>(v + 8, static_cast<u16>(n1));
        store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n1 & 0xFFFF) ? hw : n1));
    }
    ret(ctx, p);
}
